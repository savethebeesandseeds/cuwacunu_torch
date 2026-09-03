# Frozen protocol: PSM-1 — Pooling Structure Mechanism Map

**Human name:** The Missing Structure Test  
**Frozen:** 2026-08-27, before any PSM accepted-row probe fit  
**Scope:** one no-training, module-only structure map

## 1. Claim and stopping point

PSM-1 asks which structure discarded by the current all-token mean is needed to
retain the sequence information available at the encoder surface. It compares
three deterministic summaries with the current channel mean and the full
encoder reference. It ends with exactly one of:

1. the earliest summary that restores continuous sequence-factor accessibility
   and reversal decodability;
2. `fixed_summaries_not_sufficient`; or
3. a named invalid or non-reproduced result.

PSM-1 does not measure velocity. It constructs no optimizer, performs no
backward pass, changes no parameter, runs no launcher augmentation, edits no
production path, and invokes no graph, policy, observer, or end-to-end system.

## 2. Frozen parent evidence

The following files are prerequisites and must match exactly before preflight
or authoritative execution:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `POOLING_STRUCTURE_MECHANISM_MAP_PLAN.md` | 15,690 | `c8fc41261bf3c901fb36213371a08f4e1b601aa288be15ad1d6af3d26e0ee648` |
| `REPRESENTATION_SURFACE_SUFFICIENCY_MAP_PROTOCOL.md` | 20,106 | `b1554abf3ebfab5de2f263649e34688b07c8d84f5050722f60099ee65f4abc0e` |
| `.build/tests/representation_rssm_v1_authoritative.log` | 422,058 | `16be35d27836fb72bbd777e1795f10433850c5108c34670fcc27f4d287059932` |
| `.build/tests/representation_rssm_v1_receipt.sha256` | 3,048 | `78c34ca547be965dc213367ad037b07163beb9f801f486b12e02773d0838e44f` |

The parent result is `serving_pooling_loss`. Required fixed-96 continuity
values are:

| Surface | Seed 17 | Seed 31 | Seed 47 | Fixed-seed mean |
|---|---:|---:|---:|---:|
| served `C` AULC | `0.51029806802386968` | `0.5121433689059538` | `0.53534605970626181` | `0.51926249887869513` |
| encoder under original RSSM projection AULC | `0.58626145257333262` | `0.56999408500250559` | `0.58033945194633074` | `0.5788649965073897` |

The served reversal mean is `0.57454427083333337` and unresolved. The encoder
fixed-96 reversal mean is `0.9560546875` and order-decodable. PSM reference
reproduction uses absolute tolerance `1e-12` for these scalar endpoints and
requires the original RSSM projection hash `f8c9f35282de2ee0` exactly.

## 3. Model, data, targets, and probes

Inherit the RSSM configuration and algorithms without modification:

- model seeds `17,31,47`, reconstructed with the same accepted step-zero path;
- `C=3`, `H=30`, `F=9`, `d_model=32`, `latent_dim=32`;
- scales `{8,16,32,64}`, strides `{4,8,16,32}`, both domains, serving policy
  `all_tokens`;
- normalizer groups `0..255`;
- fit groups start `1,000,000`, count `256`;
- validation groups start `2,000,000`, count `128`;
- final-test groups start `3,000,000`, count `256`;
- the same normalized data, masks, twelve targets, and four target families;
- sample ladder `{32,64,128,256}`;
- alphas `{1e-5,1e-4,1e-3,1e-2,1e-1,1}` with validation-only per-target
  selection and smallest-alpha tie breaking;
- the same feature standardization, target centering, primal ridge, R2, family
  means, macro means, and AULC calculations;
- exact time reversal of data and masks, paired group bootstrap, balanced order
  labels, and sign-accuracy threshold zero.

Only fit, validation, test, and their reversed views are captured. No RSSM
nuisance, semantic, native-width, raw, or tokenizer diagnostic is repeated.

## 4. One captured encoder surface

For every dataset and model seed, use the existing RSSM public sandwich and
direct mechanical check in eval/no-grad mode. Batch size remains `96` and row
order remains contiguous. Exactly two capture passes are required; retain the
first and use the second only for byte identity.

Required capture shape after audited regrouping is `[S,3,24,32]`. Each channel
must contain exactly:

- domain 0 scale counts `7,3,1,1`;
- domain 1 scale counts `7,3,1,1`;
- within-channel order `(domain,scale,start,width,source_token_index)`.

Tokenizer/encoder masks and metadata must be exact across repeats, datasets,
and seeds. Parameters, CPU RNG, CUDA RNG, and original model mode must be
unchanged. The parent tokenizer receipt remains exactly `72/12/60/60`.

Every PSM arm is derived in CPU float64 from this one encoder capture. An arm
must never cause another encoder forward pass.

## 5. Frozen nested partitions

Let positions `0..23` denote the audited within-channel order. A partition
replaces every token in a cell by that cell's arithmetic mean, retaining a
lifted tensor of shape `[S,3,24,32]`.

| Arm | Cell IDs by position | Cells/channel |
|---|---|---:|
| `C` | all positions `0` | 1 |
| `CD` | `0..11 -> 0`, `12..23 -> 1` | 2 |
| `CDS` | runs of lengths `7,3,1,1,7,3,1,1` | 8 |
| `CDSB` | runs `2,3,2,1,1,1,1,1`, repeated for the second domain | 16 |
| `E` | position `p -> p` | 24 |

For `CDSB`, the runs within each `(domain,scale)` cell come from

```text
bin(r,n) = min(2, floor(3 * (r + 0.5) / n))
```

where `r` is zero-based rank and `n` the cell size. Empty bins are omitted from
the consecutive cell IDs. Thus the 7-token scale has `2/3/2` tokens, the
3-token scale has `1/1/1`, and each 1-token scale occupies its middle bin.

Required pure mechanics:

- exact cell counts `1,2,8,16,24`;
- every position belongs to exactly one nonempty cell;
- finite outputs and unchanged shape/dtype;
- idempotence of every partition;
- nesting: applying a coarser partition after a finer one equals applying the
  coarser partition directly within tolerance `1e-12`;
- `C` before projection equals the production all-token mean within `1e-12`;
- `E` lift is byte-identical to the captured encoder tensor.

## 6. Frozen common 96-wide projection

All five arms use one deterministic CPU-float64 `768 x 32` matrix `Q_psm` per
channel. Construct it only from the existing frozen RSSM matrix `Q_0`:

1. Construct `U[24*32,32]` with
   `U[(token*32+feature),column] = 1/sqrt(24)` when
   `feature == column`, otherwise zero. Hence `U' U = I`.
2. Form `Z = Q_0 - U(U'Q_0)`.
3. Compute reduced QR `Z = V R`.
4. Multiply each column of `V` by the sign of its `R` diagonal; zero is
   positive.
5. Form

```text
Q_psm = U / sqrt(24) + sqrt(23/24) * V
```

The following are mandatory before fitting:

- shape `[768,32]`, CPU float64, finite and contiguous;
- `max_abs(Q_psm' Q_psm - I) <= 1e-10`;
- `max_abs(U'V) <= 1e-10`;
- the sum of the 24 `32 x 32` token blocks equals identity within `1e-10`;
- projecting lifted `C` equals served `C` within `1e-10` on every row;
- projecting `E` equals direct captured-encoder projection with `Q_psm`
  within `1e-12`;
- its stable tensor hash is emitted in preflight, sealed in the manifest, and
  reproduced exactly by the authoritative run.

Each `[S,3,24,32]` lifted arm is flattened per channel, multiplied by `Q_psm`,
then flattened across channels to `[S,96]`. No arm-specific projection, PCA,
supervised fit, or result-dependent rotation exists.

For continuity only, full `E` is also projected with unchanged `Q_0`; its probe
and reversal endpoints must reproduce Section 2. These audit fits cannot enter
the PSM decision.

## 7. Continuous endpoint and controls

For every arm/seed, fit the inherited twelve-target curve on the four sample
counts and store final-test predictions. Primary performance is macro AULC.
Report all step R2 values, family AULCs, per-seed macro AULCs, fixed-seed mean,
and paired 95% intervals.

Use the exact RSSM continuous Sattolo permutations:

- fit tag `0x7273736d5f74726e`;
- validation tag `0x7273736d5f76616c`;
- test tag `0x7273736d5f746573`.

The permutations and target-shuffle path are shared by all arms and seeds.
Every arm's fixed-seed shuffled AULC must be `<= 0.02` and its paired 95% upper
bound must be `<= 0.05`.

Use exactly 512 RSSM held-out-group bootstrap rows, seed
`8387496322364763509`, shared by every arm, seed, family, contrast, and control.
Bootstrap stored predictions only; never rerun the model.

## 8. Reversal endpoint and controls

For every arm/seed, fit the inherited original-versus-reversed probe at the
same four group counts. Bootstrap paired groups only.

Use the exact RSSM order Sattolo tags:

- fit `0x7273736d5f6f7472` at lengths `64,128,256,512`;
- validation `0x7273736d5f6f7661` at length `256`;
- test `0x7273736d5f6f7465` at length `512`.

For every arm, shuffled-order accuracy must have point `<= 0.55` and paired
95% upper bound `<= 0.60`.

An arm is `order_decodable` only when accuracy AULC is `>= 0.60`, its 95% lower
bound is `> 0.50`, and at least two of three seed AULCs are `> 0.50`.

## 9. Continuous contrast rules

Every contrast is oriented named-arm minus reference-arm. From stored
predictions, report macro point, paired 95% interval, three seed deltas, and
four family-AULC deltas. Apply these rules in order:

- `material_gain`: point `>= +0.02`, interval low `> 0`, and at least two seed
  deltas `> 0`;
- `noninferior`: interval low `> -0.02`, at least two seed deltas `> -0.02`,
  and every family delta `> -0.05`;
- `unresolved`: otherwise.

Material gain takes precedence over overlapping noninferiority. All strict and
inclusive comparisons above are literal.

## 10. Validity and terminal gate

Validity precedence is:

1. finite values, capture, partitions, projection, parameters/RNG, identities,
   permutation/bootstrap, zero-training, and zero-end-to-end mechanics;
2. exact parent file and Section 2 endpoint reproduction;
3. every continuous and order shuffle control.

Failure at any stage emits `invalid_mechanics` or
`reference_reproduction_failure` as applicable and makes the scientific tree
unreadable.

Next require the PSM boundary itself:

- `E-C` continuous contrast is `material_gain`;
- `E` is `order_decodable`;
- `C` is not `order_decodable`.

Otherwise emit `encoder_boundary_not_reproduced` and stop without naming a
pooling axis.

Evaluate `CD`, `CDS`, then `CDSB`. An arm is restored only if:

1. arm-minus-`E` is `noninferior` or `material_gain`;
2. arm-minus-`C` is `material_gain`;
3. the arm is `order_decodable`.

The earliest restored arm determines:

| Arm | Terminal classification |
|---|---|
| `CD` | `domain_separation_sufficient` |
| `CDS` | `domain_scale_separation_sufficient` |
| `CDSB` | `coarse_position_separation_sufficient` |

If no arm restores both endpoints:

- any arm restoring both continuous clauses but not order gives
  `factors_restored_order_not_restored`;
- otherwise, any order-decodable arm not restoring both continuous clauses
  gives `order_restored_factors_not_restored`;
- otherwise give `fixed_summaries_not_sufficient`.

The pure gate and every threshold boundary must pass unit tests before
preflight. No diagnostic may override this tree.

## 11. Preflight, manifest, and attempt boundary

Required modes:

```text
--experiment pooling-structure-mechanism-map-preflight --device cuda
--experiment pooling-structure-mechanism-map --device cuda
```

Preflight uses non-scientific rows and emits:

```text
optimizer_constructed=false
optimizer_steps=0
backward_calls=0
scientific_probe_fits=0
training_authorized=false
long_run_authorized=false
production_or_end_to_end_authorized=false
follow_on_repair_authorized=false
```

It must prove the capture, token plan, partitions, projection, C/served identity,
E identity, parameter/RNG stability, and all deterministic permutation/bootstrap
contracts. It emits the fully specified authoritative command but does not run
it.

After preflight, seal one manifest containing protocol/sidecar, parent evidence,
relevant sources/headers/tests, binary, preflight log, projection hash,
permutation hashes, bootstrap hash, and final command.

The authoritative run uses CUDA:0, `CUBLAS_WORKSPACE_CONFIG=:4096:8`, one CPU
Torch thread, deterministic cuDNN, deterministic algorithms, eval, and no-grad.
It validates all identities and completes both capture passes before fitting.
If a pre-fit check fails, emit exactly `psm.attempt.consumed=false`. Immediately
before the first accepted-row PSM probe fit, emit exactly
`psm.attempt.consumed=true`. A consumed interpretable run is never rerun to
improve its result.

No Make target may automatically launch the authoritative mode.

## 12. Required artifacts and final report

```text
POOLING_STRUCTURE_MECHANISM_MAP_PLAN.md
POOLING_STRUCTURE_MECHANISM_MAP_PROTOCOL.md
POOLING_STRUCTURE_MECHANISM_MAP_PROTOCOL.sha256
pooling_structure_mechanism_map_gate.h
test_pooling_structure_mechanism_map_gate.cpp
.build/tests/representation_psm_v1_mechanics.log
.build/tests/representation_psm_v1_preflight.log
.build/tests/representation_psm_v1_prerun.sha256
.build/tests/representation_psm_v1_authoritative.log
.build/tests/representation_psm_v1_audit.log
.build/tests/representation_psm_v1_receipt.sha256
POOLING_STRUCTURE_MECHANISM_MAP_FINDINGS.md
```

The independent audit must verify file hashes, unique finite machine keys,
reference endpoints, controls, recomputed contrasts, gate inputs, and terminal
classification before the findings are scientific.

The final findings begin with a plain-language answer, followed by one table of
the five arms, uncertainty, reversal, controls, validity, the bounded mechanism
interpretation, and exactly one next recommendation. It always repeats:

```text
training_authorized=false
long_run_authorized=false
production_or_end_to_end_authorized=false
follow_on_repair_authorized=false
```

Then PSM-1 stops.
