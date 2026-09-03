# Frozen protocol: SRR-1 — Structured Readout Repair

**Human name:** The Readout Replacement Test  
**Frozen:** 2026-08-27, before any SRR accepted-row probe fit; immutable when
the matching SHA-256 sidecar is created  
**Scope:** one no-training, module-only `CDSB` shadow-readout parity and quality
test

## 1. Claim and stopping point

SRR-1 tests whether the structure proven sufficient by PSM-1 can be implemented
as a serving-shaped shadow readout without losing its representation value.
The readout is deterministic, metadata-driven, fixed-width, and test-only.

The experiment stops with exactly one classification:

- `structured_readout_reproduced`;
- `readout_gate_failure`;
- `device_translation_failure`;
- `offline_reference_failure`;
- `parent_evidence_failure`; or
- `invalid_mechanics`.

SRR-1 performs no training, optimizer construction, backward pass, weight or
EMA update, launcher augmentation, production serving edit, graph/policy use,
or end-to-end execution. It makes no runtime-speed claim.

## 2. Sealed parent evidence

The following parent artifacts are read-only prerequisites and must match
before preflight and again before authoritative interpretation:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `POOLING_STRUCTURE_MECHANISM_MAP_PLAN.md` | 15,690 | `c8fc41261bf3c901fb36213371a08f4e1b601aa288be15ad1d6af3d26e0ee648` |
| `POOLING_STRUCTURE_MECHANISM_MAP_PROTOCOL.md` | 13,541 | `2f574310d79d581dbc39d4040d16431f8e067ae5d3583d6d4a5597b5a8ad72d3` |
| `POOLING_STRUCTURE_MECHANISM_MAP_PROTOCOL.sha256` | 110 | `9d4c5830cbea9fa6a8a3ccf5c832201cd0e5eb890069f8e2c7024bb14ae3fbc6` |
| `POOLING_STRUCTURE_MECHANISM_MAP_FINDINGS.md` | 8,275 | `8a6a46da7625e3f25e3402d8037336bc0fe0a2de7a377827bc6e6e7f5894196f` |
| `.build/tests/representation_psm_v1_prerun.sha256` | 4,102 | `22ea52b1c31916e0da57c436917076805d5482e49e38ae1bdea62cbce31418f2` |
| `.build/tests/representation_psm_v1_authoritative.log` | 255,304 | `8243798d5af03d66257cbd1fd9da49a16ff7d6ba3f9e6bc54b5568dae41aa8b9` |
| `.build/tests/representation_psm_v1_audit.log` | 1,293 | `f2aab5858536455e1bbd6b5ec0f6aa20e49b22d46abf0b94c6e9a7346a95f85a` |
| `.build/tests/representation_psm_v1_receipt.sha256` | 2,747 | `31917fd5763f0f7831a212cd8eb0a685435e20ddb1071c5d6c189c75ee72cf53` |

The required parent classification is
`coarse_position_separation_sufficient`, with one authoritative attempt,
`audit_pass=true`, zero optimizer steps, zero backward calls, and every
authorization flag false.

The frozen projection hashes are:

```text
Q_0    = f8c9f35282de2ee0
Q_psm  = ac8a43fd65b2c8a8
```

Stable tensor hashes use the same byte-hash routine as PSM-1.

## 3. Exact parent endpoint targets

`C`, canonical offline `D`, and `E` must reproduce the parent PSM log before
the shadow `R` result is interpreted. Absolute scalar tolerance is `1e-12`.

| Arm | Seed 17 AULC | Seed 31 AULC | Seed 47 AULC | Mean AULC | Reversal mean |
|---|---:|---:|---:|---:|---:|
| `C` channel | `0.51029806802395417` | `0.51214336890601575` | `0.53534605970628402` | `0.51926249887875131` | `0.57454427083333337` |
| `D` PSM `CDSB` | `0.60310336284296084` | `0.58334872682440442` | `0.59273298270071495` | `0.59306169078936` | `0.92952473958333337` |
| `E` encoder under `Q_psm` | `0.59528657538535634` | `0.57992865599289245` | `0.57468040681240407` | `0.58329854606355092` | `0.95686848958333337` |

The parent `D-C` contrast is `material_gain` with point
`0.073799191910608755` and 95% interval
`[0.055623120359470611,0.092895749215299014]`. The parent `D-E` contrast is
`noninferior` with point `0.0097631447258091173` and interval
`[0.0010916829253943708,0.019507040708884132]`.

All parent summary keys for these three arms—including ladder points, four
families, per-seed reversal, intervals, selected alphas, and shuffle
controls—must be reproduced, not merely the displayed means. Continuous
scalars use `1e-12`; discrete values, classifications, and selected-alpha
indices are exact.

## 4. Parent canonical `D` feature identities

For each accepted dataset and model seed, the canonical CPU-float64 `D` feature
tensor `[S,3,32]` must be byte-identical to the corresponding PSM
`channel_domain_scale_bin` tensor:

| Seed | probe train | probe validation | test | reversed train | reversed validation | reversed test |
|---|---|---|---|---|---|---|
| 17 | `fd676633045bf4eb` | `f5b1f42cff608cf8` | `91a23982f8cad89e` | `bcd5d393929ff872` | `a29adca524fa637e` | `49302fd5dca87d0f` |
| 31 | `ee2ebca7b09e54df` | `7e63ad5dd9a2f37f` | `624cdc1ea7ef3613` | `613ca3e7e2c85d9a` | `2ea912456e086a43` | `4b11e03466aff5e7` |
| 47 | `5faebeb61db5bc58` | `7bd13bbd5cce225c` | `991a19ec290ac589` | `16a8bebec4dc971a` | `71ba6bd2a496c0e2` | `613fec93300fbfaa` |

The corresponding `C` and `E` feature hashes are read from the sealed parent
log and must also match their exact parent keys. A mismatch is not rounded into
an endpoint comparison.

## 5. Frozen model and data

Inherit PSM-1 without modification:

- model seeds `17,31,47`, reconstructed through the accepted step-zero path;
- `C=3`, `H=30`, `F=9`, `d_model=32`, `latent_dim=32`;
- accepted scientific execution dtype `torch::kFloat32` on CUDA:0;
- scales `{8,16,32,64}`, strides `{4,8,16,32}`, both domains, production
  serving policy `all_tokens`;
- normalizer groups `0..255`;
- fit groups start `1,000,000`, count `256`;
- validation groups start `2,000,000`, count `128`;
- final-test groups start `3,000,000`, count `256`;
- the exact normalized rows, masks, original/reversed pairings, twelve targets,
  and four target families;
- sample ladder `{32,64,128,256}`;
- alphas `{1e-5,1e-4,1e-3,1e-2,1e-1,1}` with validation-only per-target
  selection and smallest-alpha tie breaking;
- feature standardization, target centering, primal ridge, R2, family means,
  macro means, AULC, reversal probe, and sign threshold zero;
- batch size `96` and contiguous row order.

Only fit, validation, test, and their exact reversed views are captured. No
nuisance, semantic, robustness, raw, native-width, tokenizer-only, training, or
end-to-end suite is added.

## 6. Shadow API and ownership boundary

The implementation is a test-only free function under the isolated benchmark.
It consumes:

- `mtf_jepa_mae_vicreg_encode_output_t` from the public `encode()` path;
- the frozen accepted model configuration; and
- `Q_psm`, supplied as a validated tensor rather than created or learned by the
  readout.

It returns the existing serving-shaped pair:

```text
values      [B,3,32]  same floating dtype/device as encoded.embeddings
valid_mask  [B,3]     bool on the same device
```

Values are contiguous and finite for valid channels. Invalid-channel values
are exactly zero. The probe harness alone flattens values to `[B,96]`.

The shadow owns no `torch::nn::Module`, parameter, buffer, cache with mutable
model state, optimizer, generator, seed, or randomness. It is not reachable
from a production enum, config parser, serving selector, forward method, or
launcher.

## 7. Metadata-derived canonical ordering

Input tokens are not trusted to arrive in a particular order. Validate all
metadata widths against `N`, then group by `channel_id`. Within a channel, sort
by:

```text
(domain_id, scale_id, start_index, width, source_token_index)
```

For the accepted scientific layout, `(domain_id,scale_id,start_index,width)`
must already be unique within each channel; therefore the final source-index
tie break cannot make the result permutation-dependent. Duplicate four-field
keys are rejected.

The accepted layout is exact:

- 72 total tokens, 24 per channel;
- channels exactly `0,1,2`;
- domains exactly `0,1`, 12 tokens per domain per channel;
- scales exactly `0,1,2,3` in each domain;
- per-domain scale counts exactly `7,3,1,1`;
- all three channels share the same ordered domain/scale/start/width layout.

Any out-of-range ID, missing channel/domain/scale, count mismatch, or layout
disagreement rejects the input before output is interpreted.

## 8. Frozen `CDSB` cells

Within one `(channel,domain,scale)` group, rank the canonical `(start,width)`
order from zero. For rank `r` among `n` tokens:

```text
bin(r,n) = min(2, floor(3 * (r + 0.5) / n))
```

Enumerate nonempty bins in canonical domain/scale/bin order. The accepted
24-position cell IDs per channel must be exactly:

```text
0,0,1,1,1,2,2,3,4,5,6,7,8,8,9,9,9,10,10,11,12,13,14,15
```

This gives 16 cells: the 7-token scale contributes `2/3/2`, the 3-token scale
contributes `1/1/1`, and each 1-token scale contributes its middle bin, repeated
for the second domain.

For a fully valid channel, compute each cell's arithmetic mean in encoder
dtype. Lift that mean back to every canonical token position assigned to the
cell, flatten token-major with latent dimension last to `[B,3,768]`, then
multiply each channel by `Q_psm[768,32]`.

No cell weighting, fitted statistic, attention score, positional parameter, or
alternative binning rule is allowed.

## 9. Frozen mask behavior

Scientific rows are fully observed and require every `token_mask`,
`sample_valid_mask`, and `channel_valid_mask` value to be true, exactly as in
PSM-1.

Partial masks are mechanics-only and fail closed under this fixed rule:

1. a channel is valid only if its input sample and channel masks are true and
   all 24 of its token-mask entries are true;
2. any false token mask makes that channel invalid and its full 32-value
   output exactly zero;
3. an invalid channel never borrows or extrapolates tokens from another cell
   or channel; and
4. a partial-mask pooling rule is explicitly deferred to a separate
   experiment.

The mechanics suite tests independent channel invalidation and zeroing, but no
scientific conclusion is drawn from partially observed rows in SRR-1.

## 10. Frozen projection

Construct CPU-float64 `Q_psm[768,32]` by the exact PSM-1 Section 6 algorithm
from `Q_0`, including QR sign normalization and the mean-preserving component.
It must reproduce stable hash `ac8a43fd65b2c8a8`.

Before use, require:

- `Q_0` hash `f8c9f35282de2ee0`;
- `Q_psm` shape `[768,32]`, CPU float64, finite and contiguous;
- `max_abs(Q_psm'Q_psm-I) <= 1e-10`;
- mean-subspace contrast error `<= 1e-10`;
- the sum of its 24 token blocks equals identity within `1e-10`.

The canonical CPU path receives CPU-float64 `Q_psm`. The production-like path
receives exactly one `.to(encoded.embeddings.options())` copy. The readout must
not silently recast either its input or projection.

## 11. Exact CPU-float64 equivalence

For a public captured encoder tensor copied to CPU float64, run both:

1. sealed offline PSM partition/lift/project `D`; and
2. the metadata-driven shadow with CPU-float64 embeddings and `Q_psm`.

Require identical shape, stride/contiguity contract, valid mask, stable tensor
hash, and tensor bytes. `max_abs <= 1e-12` is emitted as an additional numeric
diagnostic but does not replace byte identity.

This equality is required in pure mechanics, preflight, and every accepted
dataset/seed. Any failure is `offline_reference_failure` if the PSM reference
itself is valid, otherwise `invalid_mechanics`.

## 12. Production-like CUDA/dtype translation

For the same public `encode()` output, run `R` without moving embeddings off
CUDA and without changing their floating dtype. Require:

- values `[B,3,32]`, CUDA:0, encoder dtype, contiguous, finite;
- the encoder and returned value dtype are exactly `torch::kFloat32`;
- valid mask `[B,3]`, CUDA:0, bool, exactly equal to the canonical mask;
- repeated invocation byte-identical;
- after copying values to CPU float64 only for audit,
  `max_abs(R-D) <= 2e-5` on every row, dataset, and seed.

The `2e-5` componentwise absolute threshold is frozen before preflight. There
is no result-dependent relaxation, percentile exception, relative-only escape,
or aggregate averaging of the error. Exceeding it is
`device_translation_failure`, even if a probe result looks favorable.

## 13. Required pure mechanics tests

The focused mechanics target must pass all of the following before CUDA:

- exact accepted cell vector and 16 nonempty cells per channel;
- exact domain/scale counts and canonical ordering;
- global token-permutation invariance when embeddings, masks, and all metadata
  fields are permuted together;
- within-cell permutation and mean invariance;
- cross-bin sensitivity using a token perturbation whose output must change;
- exact lift indexing and token-major/latent-last flattening;
- partition idempotence and constant-token mean preservation;
- CPU-float64 byte identity with the offline PSM `D` algorithm;
- `Q_psm` projection/hash/orthogonality/block-sum checks;
- all-valid and fail-closed partial-mask semantics, including independent
  channel invalidation and zero output;
- CPU float32, CPU float64, and, in preflight, CUDA dtype/device/shape/mask
  contracts;
- rejection of undefined or wrong-shaped tensors, projection mismatch,
  non-floating embeddings, non-bool masks, invalid metadata dtype/width/IDs,
  duplicate order keys, layout/count mismatch, and non-finite values;
- no parameter/module/RNG creation or mutation;
- pure gate tests immediately below, immediately above, and exactly on every
  inclusive or strict decision threshold.

## 14. Four scientific arms from one capture

For each seed and dataset, retain the first of two identical public captures
and derive:

| Code | Exact construction |
|---|---|
| `C` | current `select_mtf_serving_pool(...all_tokens...)`, then the parent mean-preserving identity path; do not recompute the mean after float32-to-float64 conversion |
| `D` | sealed CPU-float64 PSM `CDSB` partition/lift/project |
| `R` | test-only shadow on the original CUDA encoded output and dtype, copied to CPU float64 after return |
| `E` | full canonical ordered encoder tensor projected by CPU-float64 `Q_psm` |

No arm reruns the encoder. The first capture supplies every arm; the second is
used only for byte-identity checks.

The public sandwich must prove unchanged tokenize/encode metadata and masks,
public/direct encoder identity, exact serving identity, fully valid scientific
rows, exact token plan `72/12/60/60`, unchanged parameters, unchanged CPU and
CUDA RNG, restored model mode, eval, and no-grad.

## 15. Continuous endpoint and controls

For all four arms and three seeds, run the inherited twelve-target curve at
`32,64,128,256` fit groups. Store final-test predictions. Report ladder R2,
four family AULCs, seed macro AULCs, fixed-seed mean, and paired 95% intervals.

Use the exact PSM/RSSM continuous Sattolo tags:

```text
fit         0x7273736d5f74726e
validation  0x7273736d5f76616c
test        0x7273736d5f746573
```

Use exactly 512 shared held-out-group bootstrap rows with seed
`8387496322364763509`. The permutations, bootstrap rows, targets, and fitted
predictions are shared across arms. Bootstrap stored predictions only.

Every arm's shuffled continuous AULC must have point `<= 0.02` and 95% upper
bound `<= 0.05`.

## 16. Reversal endpoint and controls

For all four arms and three seeds, run the inherited balanced
original-versus-exactly-reversed probe at the same four sample counts. Bootstrap
paired groups only.

Use the exact order Sattolo tags:

```text
fit         0x7273736d5f6f7472 at lengths 64,128,256,512
validation  0x7273736d5f6f7661 at length 256
test        0x7273736d5f6f7465 at length 512
```

Every arm's shuffled-order point must be `<= 0.55` and its 95% upper bound
`<= 0.60`.

An arm is `order_decodable` only when accuracy AULC is `>= 0.60`, its 95% lower
bound is `> 0.50`, and at least two of three seed AULCs are `> 0.50`.

## 17. Continuous contrast rules

Contrasts are candidate minus named reference. From stored predictions, report
point, paired 95% interval, three seed deltas, and four family-AULC deltas.

- `material_gain`: point `>= +0.02`, lower bound `> 0`, and at least two seed
  deltas `> 0`;
- `noninferior`: lower bound `> -0.02`, at least two seed deltas `> -0.02`, and
  every family delta `> -0.05`;
- `unresolved`: otherwise.

Material gain takes precedence over overlapping noninferiority. All strict and
inclusive comparisons are literal.

## 18. Shadow quality gate

After all identity and control gates pass, `R` reproduces the structured
readout only when:

1. `R-C` is `material_gain`;
2. `R-E` is `noninferior` or `material_gain`;
3. `R` is `order_decodable`;
4. `C` remains not order-decodable and `E` remains order-decodable; and
5. all four arms pass both shuffle controls.

`D` must independently reproduce its parent material/noninferiority/order
classifications. `D` is an identity reference, not a fallback candidate. No
diagnostic, average feature error, or similarity plot may override this gate.

## 19. Invalidity precedence and terminal classification

Evaluate in the following order and stop at the first failure:

1. **Local validity:** finite values, shapes, masks, metadata, cells, lift,
   projection, capture repetition, public/direct parity, parameters/RNG,
   deterministic tables, manifest, zero training, and zero end-to-end work.
   Failure: `invalid_mechanics`.
2. **Parent validity:** every Section 2 file hash, audit result, attempt count,
   parent classification, and authorization flag. Failure:
   `parent_evidence_failure`.
3. **Offline reproduction:** exact `C,D,E` feature identities and Section 3
   endpoints. Failure: `offline_reference_failure`.
4. **Device translation:** CPU shadow identity and every Section 12 CUDA
   contract. Failure: `device_translation_failure`.
5. **Controls:** every continuous and order shuffle gate for `C,D,R,E`.
   Failure: `invalid_mechanics` with a named control reason.
6. **Quality:** evaluate Section 18. Failure: `readout_gate_failure`; success:
   `structured_readout_reproduced`.

Stages 1–5 make the scientific tree unreadable. Stage 6 is an interpretable
positive or negative result. Mixed outcomes do not authorize selecting the
shadow by preference.

## 20. Preflight and attempt boundary

Required harness modes:

```text
--experiment structured-readout-repair-preflight --device cuda
--experiment structured-readout-repair --device cuda
```

Preflight uses only non-scientific normalizer groups starting `4,500,000`
with count `32` and capture groups starting `4,600,000` with count `101`. It
performs no target construction or probe fit. It proves public/direct capture,
metadata cells, masks, exact CPU64 offline/shadow identity, CUDA translation,
projection, deterministic tables, parameter/RNG stability, and zero-training
counters.

Preflight emits:

```text
optimizer_constructed=false
optimizer_steps=0
backward_calls=0
scientific_probe_fits=0
training_authorized=false
augmentation_change_authorized=false
long_run_authorized=false
production_or_end_to_end_authorized=false
follow_on_production_repair_authorized=false
```

It emits the fully specified authoritative command but does not run it.

After preflight, seal one pre-run manifest containing the protocol and sidecar,
all parent artifacts, all SRR sources/headers/tests, focused binaries, mechanics
and preflight logs, projection and layout hashes, permutation and bootstrap
hashes, expected parent feature identities, environment contract, and exact
authoritative command.

The authoritative run uses CUDA:0, `CUBLAS_WORKSPACE_CONFIG=:4096:8`, one CPU
Torch thread, deterministic cuDNN, deterministic algorithms, cuBLAS and cuDNN
TF32 disabled, eval, and no-grad.
It completes both captures and every pre-fit identity before fitting.

If a pre-fit check fails, emit exactly:

```text
srr.attempt.consumed=false
```

Immediately before the first accepted-row SRR probe fit, emit exactly:

```text
srr.attempt.consumed=true
```

There is at most one consumed authoritative invocation. A complete,
interpretable run is never repeated to improve or change its result. No Make
target may automatically launch authoritative mode.

## 21. Required artifacts

```text
STRUCTURED_READOUT_REPAIR_PLAN.md
STRUCTURED_READOUT_REPAIR_PROTOCOL.md
STRUCTURED_READOUT_REPAIR_PROTOCOL.sha256
structured_readout_shadow.h
structured_readout_repair_gate.h
test_structured_readout_shadow.cpp
test_structured_readout_repair_gate.cpp
quality_wikimyei_mtf_jepa_mae_vicreg_structured_readout_repair.cpp
test_structured_readout_repair_log_auditor.cpp
.build/tests/representation_srr_v1_mechanics.log
.build/tests/representation_srr_v1_preflight.log
.build/tests/representation_srr_v1_prerun.sha256
.build/tests/representation_srr_v1_authoritative.log
.build/tests/representation_srr_v1_audit.log
.build/tests/representation_srr_v1_receipt.sha256
STRUCTURED_READOUT_REPAIR_FINDINGS.md
```

The independent auditor must verify file hashes, unique finite machine keys,
parent evidence, exact feature identities, projection/layout/tables, counters,
all arm endpoints and controls, recomputed contrasts, gate inputs, invalidity
precedence, and terminal classification. It must report any limitation in what
can be recomputed from the log.

## 22. Final report and authorization boundary

The findings begin with one plain-language answer. Then show a single four-arm
table containing continuous AULC and interval, contrasts, reversal AULC and
interval, shuffle controls, CPU identity, CUDA translation maximum, and frozen
gate reading. Follow with validity, bounded interpretation, limitations, hashes,
and exactly one next recommendation.

Every final report repeats:

```text
training_authorized=false
augmentation_change_authorized=false
long_run_authorized=false
production_or_end_to_end_authorized=false
follow_on_production_repair_authorized=false
```

`structured_readout_reproduced` permits only a separately frozen proposal for
a production implementation and parity test. SRR-1 itself changes no production
code and does not clear the augmentations. After the audited terminal result,
SRR-1 stops.
