# Frozen protocol: SRR-2 — Production Structured Readout Parity

**Human name:** The Production Readout Equivalence Test  
**Frozen:** 2026-08-27, before any SRR-2 production source edit  
**Scope:** one append-only, opt-in serving-readout implementation and one
no-training production-versus-shadow parity proof

## 1. Claim and exclusions

SRR-2 tests whether production policy `structured_cdsb_v1`, selected through
the existing public serving interface, exactly reproduces the successful SRR-1
shadow representation.

SRR-2 does not test runtime speed and does not authorize or perform encoder
training, augmentation changes, optimizer construction, backward calls, weight
updates, active-policy changes, downstream-head migration or retraining,
end-to-end execution, or deployment.

The terminal result is exactly one of:

- `production_structured_readout_parity_reproduced`;
- `production_readout_gate_failure`;
- `device_translation_failure`;
- `production_shadow_parity_failure`;
- `sealed_reference_failure`;
- `backward_compatibility_failure`;
- `parent_evidence_failure`; or
- `invalid_mechanics`.

## 2. Immutable parent evidence

The following SRR-1 artifacts must match before preflight, immediately before
the authoritative attempt, and during independent audit:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `STRUCTURED_READOUT_REPAIR_PLAN.md` | 14,893 | `1f976d5da5a79323a8fce011b0b33e53b277517bd785b2fdea68aa1888338127` |
| `STRUCTURED_READOUT_REPAIR_PROTOCOL.md` | 21,848 | `ad7c9381d58a23e8f3cec27b59b44e6532aa561227ad22d57578cc6ba0a04946` |
| `STRUCTURED_READOUT_REPAIR_PROTOCOL.sha256` | 104 | `3b97b7431e34c3b875365bad27d0b9de67a5b7fd7007760eb34ab97b125140c5` |
| `STRUCTURED_READOUT_REPAIR_FINDINGS.md` | 10,393 | `b5b3458953f2f967a0229ea910c80810303d2e709bd6d9f237966c0e6b456c6a` |
| `.build/tests/representation_srr_v1_prerun.sha256` | 7,166 | `515c9c8a851b3aceb03c160e5c9c19fff5265774d51eb396c6d56123cf0d3acb` |
| `.build/tests/representation_srr_v1_authoritative.log` | 7,324,951 | `f38c99ef1294dab5f40f57fff79a958cd214c593eedd10284531976cda20ae6a` |
| `.build/tests/representation_srr_v1_audit.log` | 2,964 | `fe943fb2aa8ad26f53953364181f7c2b452692fde17643c5a8d94ca45c9bb841` |
| `.build/tests/representation_srr_v1_receipt.sha256` | 3,517 | `994be46cab5c4bbabf3b72ed30e5fa1a8ece9247722e16ae504b428dcd0fc207` |

Required parent facts are:

```text
terminal_result=structured_readout_reproduced
audit_pass=true
audit_error_count=0
authoritative_attempt_count=1
optimizer_steps=0
backward_calls=0
```

Every SRR-1 authorization flag remains false.

## 3. Pre-change production custody

Before this protocol was sealed, the exact SRR-2 production/config/test surface
was archived deterministically as:

```text
.build/tests/representation_srr2_v1_production_baseline.tar
bytes=829440
sha256=22f53f452836583f5402d1a28d67c9b3a9ac865094444a06b9726fd0f1c7b6dd
format=ustar
member_order=lexicographic
mtime=0
owner=0
group=0
```

Its 14 exact members are:

```text
src/config/README.md
src/config/grammar/wikimyei.representation.mtf_jepa_mae_vicreg.dsl.bnf
src/config/man/wikimyei.config.man
src/config/wikimyei.representation.mtf_jepa_mae_vicreg.dsl
src/include/jkimyei/training/inference/channel_graph_first_inference_launcher.h
src/include/kikijyeba/protocol/config_bundle.h
src/include/wikimyei/README.md
src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/README.md
src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h
src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg_spec.h
src/tests/bench/jkimyei/training/channel_graph_first_launchers/test_jkimyei_channel_graph_first_launchers.cpp
src/tests/bench/wikimyei/config/graph_first_specs/test_wikimyei_graph_first_specs.cpp
src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/Makefile
src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/test_wikimyei_mtf_jepa_mae_vicreg.cpp
```

The archive is immutable and is not regenerated after source edits. A
canonical base-to-candidate patch is later made from this archive and sealed in
the pre-run manifest. In particular, SRR-1 authenticated the then-live
production header hash `3562e214f7f0158ac4b60e0ed06d1e4152e8b995628a1b774d2db4cb36a1c5b2`;
SRR-2 intentionally changes that file, so the archive—not a false requirement
that the live header retain its old hash—preserves custody.

## 4. Append-only policy and compatibility

Append `structured_cdsb_v1` after all existing enum values. Preserve the exact
ordinals, names, and behavior of:

```text
all_tokens
time_only
frequency_only
domain_balanced
```

The exact parser/DSL/checkpoint spelling is `structured_cdsb_v1`. There is no
alias, normalization, alternate spelling, new key, or hidden fallback.

`all_tokens` remains:

- the C++ configuration default;
- the fallback when the DSL field is omitted;
- the value in the checked-in active DSL; and
- the interpretation of legacy downstream checkpoint metadata where the
  policy field is absent.

The policy already participates in the protocol fingerprint and downstream
checkpoint identity. Selecting `structured_cdsb_v1` must change that identity.
A legacy or explicitly different head must reject a structured-policy expected
identity; no silent migration is allowed.

## 5. Production ownership boundary

The public signature and result type remain unchanged:

```text
select_mtf_serving_pool(encoded, policy, config)
    -> mtf_serving_pool_output_t {
         values: [B,3,32],
         valid_mask: [B,3]
       }
```

The new branch lives behind this selector. The real inference adapter already
passes the configured policy to this call and must require no shape, stream,
MDN-input, tokenizer, encoder, or training change.

The production implementation must not include or call
`structured_readout_shadow.h`. The production and shadow implementations are
independent subjects compared by tests.

The implementation owns no `torch::nn::Module`, parameter, registered buffer,
optimizer, mutable device cache, fitted value, generator, or RNG call. The
canonical projection is an unexposed immutable CPU-float64 function-local
value, constructed deterministically and copied exactly once to the embedding
options per structured readout call.

## 6. Accepted architecture and metadata

The versioned policy accepts only:

```text
channels=3
history=30
features=9
d_model=32
latent_dim=32
frequency_domain=true
scales=8,16,32,64
strides=4,8,16,32
tokens=72
tokens_per_channel=24
```

Every metadata tensor is defined int64 `[72]`: channel, domain, scale, start,
and width. Metadata may originate on CPU or the embedding device and is copied
to CPU only to construct the canonical plan. Token values and masks may be CPU
or CUDA, but all value/mask tensors in one call share the embedding device.

Group by channel and sort independently by:

```text
(domain_id, scale_id, start_index, width, source_index)
```

Within a channel, the four-field key before `source_index` must be unique.
Channels are exactly `0,1,2`; domains exactly `0,1`; scales exactly `0,1,2,3`;
each domain has scale counts `7,3,1,1`; and all channels have the same ordered
domain/scale/start/width layout. Reject every mismatch before interpreting an
output.

## 7. Frozen CDSB transform

Within each `(channel,domain,scale)` group, rank canonical `(start,width)`
order. For rank `r` among `n` tokens:

```text
bin(r,n) = min(2, floor(3 * (r + 0.5) / n))
```

Enumerate nonempty bins in domain/scale/bin order. The exact 24-position cell
vector is:

```text
0,0,1,1,1,2,2,3,4,5,6,7,8,8,9,9,9,10,10,11,12,13,14,15
```

For each valid channel:

1. average each of the 16 cells in embedding dtype;
2. lift each cell mean to its assigned canonical token positions;
3. flatten token-major, latent-last to 768 values; and
4. multiply by frozen `Q_psm[768,32]`.

Return contiguous values in embedding dtype/device and a bool mask on the same
device. No alternative weighting, partial-cell estimate, learned pooling,
attention, or layout generalization is allowed.

## 8. Frozen projection

Construct CPU-float64 `Q_0` deterministically from SplitMix64 tag
`0x7273736d5f74655f`, normalize QR signs, and reproduce:

```text
Q_0_hash=f8c9f35282de2ee0
```

Construct the exact SRR-1 mean-preserving projection and reproduce:

```text
Q_psm_hash=ac8a43fd65b2c8a8
shape=[768,32]
device=cpu
dtype=float64
contiguous=true
```

Require finiteness and all three invariant maxima `<= 1e-10`:

- `max_abs(Q_psm' Q_psm - I)`;
- mean-subspace contrast error; and
- sum of 24 token blocks minus identity.

Hash or invariant failure rejects the policy. Runtime randomness or a
backend-dependent unchecked projection is forbidden.

## 9. Mask and value semantics

Inputs require:

- floating finite embeddings `[B,72,32]`;
- bool token mask `[B,72]` on the embedding device;
- bool sample-valid mask `[B]` on the embedding device; and
- bool channel-valid mask `[B,3]` on the embedding device.

For sample `b`, channel `c`:

```text
valid[b,c] = sample_valid_mask[b]
             AND channel_valid_mask[b,c]
             AND all 24 canonical token_mask entries for channel c
```

Every invalid channel's 32 values are exactly zero. Invalid data in one channel
cannot alter another. `pooled_by_channel`, if present in the encode result, is
not an input to this policy and poisoning it must not affect the result.

## 10. Focused mechanics acceptance

Before preflight, isolated tests must prove:

1. exact old and new enum/name/parser/default/DSL contracts;
2. unchanged three-argument public selector and output type;
3. exact cells, canonical plan, lift order, and flatten order;
4. independent CPU64 production/shadow/offline-reference byte equality;
5. CPU32 production/shadow byte equality;
6. CUDA32 and CUDA64 production/shadow byte equality when CUDA is available;
7. componentwise production-to-CPU64 reference error `<=2e-5` on CUDA32;
8. joint token-permutation invariance, within-cell invariance, cross-cell
   sensitivity, constant preservation, and partition idempotence;
9. all mask cases and exact invalid zeroing;
10. comprehensive malformed-input/layout/projection rejection;
11. unchanged inputs, parameters, buffers, CPU/CUDA RNG, and model mode;
12. repeated-call byte identity;
13. byte-identical golden outputs for all four old policies;
14. active DSL still `all_tokens` and omitted field still `all_tokens`;
15. protocol fingerprint inequality and exact checkpoint identity behavior; and
16. the inference adapter reaches the new selector branch without API or shape
    changes.

An unavailable CUDA device may skip CUDA unit cases but cannot satisfy CUDA
preflight or the authoritative experiment.

## 11. Parent shadow identities

SRR-2 reuses the exact model seeds and six dataset views from SRR-1. For each
capture, the CUDA-float32 shadow tensor copied to CPU float64 must reproduce the
following SRR-1 stable hash:

| Seed | probe train | probe validation | test | reversed train | reversed validation | reversed test |
|---|---|---|---|---|---|---|
| 17 | `57e76e4493236cde` | `3e47ce5037e0d775` | `5b77e7532b17b2cf` | `f05e70e9fb885c2a` | `f1503ff41308025d` | `bec04f00c87194c6` |
| 31 | `ac98c4dbbc249f65` | `52082321dc1c8bb2` | `b3e21f156069a2e5` | `2a79f6600a442fc2` | `8e43335bbae9c2f2` | `793ca302741a42e9` |
| 47 | `0025b31c8799c0b9` | `a6ffdc73e0f84b2d` | `ed67fadc9fc6e9b8` | `e2edbb651c2f2bf2` | `031eb4a14b7b1543` | `cc9089b1de4278aa` |

Stable hashes are over contiguous CPU-float64 tensor bytes using the SRR-1
hash routine. Shape, mask hash, encoder hash, metadata structure hash, old
served hash, and canonical offline `D` hash must also reproduce their exact
keys in the sealed SRR-1 log. This authenticates that SRR-2 regenerated the
same scientific surface, not merely a same-shaped replacement.

## 12. Authoritative parity rule

Use model seeds `17,31,47` and exact SRR-1 groups:

- normalizer `0..255`;
- probe train from `1,000,000`, count `256`;
- probe validation from `2,000,000`, count `128`;
- test from `3,000,000`, count `256`;
- exact reversed train, validation, and test pairings;
- batch size `96`, contiguous row order, CUDA:0 float32.

For each of 18 seed/dataset captures, run the public encoder capture twice and
require complete capture, input, parameter, RNG, and model-mode identity.
Retain the first. From that same retained encoded object derive:

- `R`: sealed SRR-1 shadow implementation; and
- `P`: production `select_mtf_serving_pool` with
  `structured_cdsb_v1` passed explicitly.

Require exact equality of shape, strides/contiguity contract, dtype, device,
valid-mask bytes, and value bytes while both are on CUDA. Emit stable hashes
after the same CPU64 audit copy. Require `max_abs(P-R)=0`.

Independently run the canonical CPU64 offline reference `D` on the audit copy:

- CPU64 `P`, `R`, and `D` must be byte-identical;
- CUDA32 `P` and `R`, copied to CPU64, each require componentwise
  `max_abs(...-D)<=2e-5`;
- the parent shadow and reference hashes must match Section 11.

One row, mask, dataset, or seed mismatch is
`production_shadow_parity_failure`; an otherwise exact CUDA path exceeding the
reference tolerance is `device_translation_failure`.

Coverage is exact: 18 retained feature tensors plus 18 repeats, 3,840 retained
rows, 368,640 retained feature values, and 11,520 retained validity values.
The repeat covers another 368,640 values and 11,520 masks. Aggregate counts
must be recomputed from per-capture records, not trusted as self-reported facts.

## 13. Exact quality-transport theorem

The sealed SRR-1 quality computation is deterministic given:

```text
features, masks, targets, group splits, sample ladder, alpha grid,
standardization, target centering, fit/validation selection, test rows,
permutations, bootstrap rows, and decision thresholds.
```

SRR-2 authenticates the parent log and all deterministic identities, then
proves `P_features == R_features` and `P_masks == R_masks` byte for byte over
the complete domain consumed by every SRR-1 probe. Substitution therefore
implies every production prediction, selected alpha, metric, interval, shuffle
control, contrast, and classification equals the sealed shadow result.

Transport is accepted only if the parent independently audited facts remain:

```text
R-C classification=material_gain
R-E classification=noninferior
R order classification=order_decodable
continuous_shuffle_pass=true
order_shuffle_pass=true
terminal_result=structured_readout_reproduced
```

No new probe fit occurs. Exact identity over the full consumed input domain is
the evidence; tolerance, sampling, rounded scores, or qualitative similarity
cannot invoke this theorem.

## 14. Preflight

The candidate binary has exactly two experiment modes:

```text
--experiment production-structured-readout-parity-preflight --device cuda
--experiment production-structured-readout-parity --device cuda
```

Preflight uses only non-scientific groups:

```text
normalizer_start=4700000
normalizer_count=32
capture_start=4800000
capture_count=101
```

It runs one seed, contains no target generation, probe construction, fit,
validation selection, prediction, permutation, or bootstrap code path, and
does not consume the authoritative attempt. It must prove production/shadow
CUDA byte identity, CPU64 reference identity, the `2e-5` translation bound,
the public selector sandwich, complete masks/layout, parameter/RNG purity,
zero forbidden counters, and CUDA availability.

## 15. Attempt boundary and exact command

The authoritative attempt is consumed immediately before the first accepted
scientific capture is requested, after all parent, manifest, executable,
environment, CUDA, and zero-counter checks pass. Any failure after that marker
is the one authoritative result; it is not rerun because it is unfavorable.

The exact container command frozen into the pre-run manifest is:

```text
docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && ./.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity --experiment production-structured-readout-parity --device cuda'
```

The protocol freezes the binary path, CLI arguments, and semantics. The
manifest additionally binds the exact command bytes, working directory,
image/container identity, and environment. No Make target may run the
authoritative mode.

## 16. Pre-run manifest

After mechanics and preflight, seal one manifest containing SHA-256 and byte
length for:

- this plan, protocol, and protocol sidecar;
- all eight SRR-1 parent artifacts in Section 2;
- the immutable production baseline archive;
- the canonical base-to-candidate patch;
- production header, spec parser, launcher adapter, config fingerprint code,
  grammar/docs/active DSL, and affected existing tests;
- shadow implementation and its mechanics test;
- all new SRR-2 sources, tests, gate, harness, auditor, and Makefile;
- focused test binaries and exact build receipts;
- mechanics and preflight logs;
- parity binary and auditor binary;
- exact commands, deterministic tables, expected keys, and environment facts.

The manifest validates the archived base and live candidate separately. It does
not require the post-edit live source to equal the archived base.

## 17. Independent auditor

The auditor receives the sealed manifest and authoritative log. It must not
call the model or trust emitted booleans. It independently:

- authenticates parent evidence, baseline archive, candidate delta, sources,
  binaries, command, environment, and attempt count;
- parses exactly one value for every mandatory key and rejects unknown,
  duplicate, missing, non-finite, or malformed values;
- verifies three seeds, six exact datasets, row counts, shapes, hashes, masks,
  and 18 complete parity records;
- recomputes maximum errors and conjunctions from primitive records;
- verifies the parent shadow/reference hashes and all quality-transport
  premises against the sealed SRR-1 log;
- verifies old-policy/DSL/fingerprint/checkpoint/adapter compatibility facts
  from required mechanics records;
- verifies all forbidden counters and authorization flags; and
- applies Section 18 precedence independently.

The authoritative program emits measurements and a provisional result. Only
agreement with an `audit_pass=true`, `audit_error_count=0` auditor result can
support the final classification.

## 18. Terminal precedence

Apply the first true condition:

1. local mechanics, command, environment, attempt, capture, purity, finite,
   deterministic, zero-counter, or audit-input failure -> `invalid_mechanics`;
2. parent file/hash/classification/audit failure -> `parent_evidence_failure`;
3. old enum/parser/policy/default/DSL/fingerprint/checkpoint/adapter behavior
   failure -> `backward_compatibility_failure`;
4. projection, layout, shadow, canonical reference, or archived-base custody
   failure -> `sealed_reference_failure`;
5. any production/shadow value, mask, shape, stride, dtype, device, or full
   coverage mismatch -> `production_shadow_parity_failure`;
6. exact P/R parity but CPU64 reference or CUDA translation bound failure ->
   `device_translation_failure`;
7. any parent quality/control/transport premise failure ->
   `production_readout_gate_failure`;
8. all prior checks pass ->
   `production_structured_readout_parity_reproduced`.

No favorable metric, partial seed agreement, aggregate hash, or visual
inspection overrides this order.

## 19. Frozen authorization boundary

The authoritative log and audit must end with exactly:

```text
training_authorized=false
augmentation_change_authorized=false
long_run_authorized=false
active_policy_change_authorized=false
checkpoint_migration_authorized=false
downstream_retraining_authorized=false
end_to_end_authorized=false
deployment_authorized=false
```

Success permits only a separately frozen activation proposal. Until that
proposal explicitly handles downstream checkpoint identity and the old/new
readout comparison, the checked-in active policy remains `all_tokens`.
