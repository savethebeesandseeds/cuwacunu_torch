# SRR-2 — Production Structured Readout Parity

**Human name:** The Production Readout Equivalence Test  
**Status:** approved module-only production-parity work; activation remains
unauthorized  
**Parent result:** SRR-1 classified `structured_readout_reproduced`

## Plain-language objective

Move the exact deterministic `CDSB` readout proven by SRR-1 from its isolated
test-only shadow into the production serving selector, without changing the
active serving policy. Then prove that the new opt-in production branch returns
the same representation as the shadow for every row used by SRR-1.

This stage is about representation identity, not throughput. It does not train
the encoder, change augmentation, migrate a downstream head, activate the new
policy, or run an end-to-end system.

## What SRR-1 established

SRR-1 showed that the old all-token channel mean discards useful sequence
structure and that the deterministic channel/domain/scale/coarse-position
(`CDSB`) readout preserves it. On the sealed three-seed benchmark:

| Surface | Continuous AULC | Reversal AULC | Result |
|---|---:|---:|---|
| old channel mean `C` | `0.5192624989` | `0.5745442708` | order remains unresolved |
| structured shadow `R` | `0.5930617663` | `0.9295247396` | material gain and order-decodable |
| full encoder `E` | `0.5832985461` | `0.9568684896` | accepted encoder reference |

The shadow beat `C` materially, was noninferior to `E`, decoded reversal, and
passed the shuffled controls. Its terminal result was independently audited.

## The bounded question

> Does the opt-in production policy `structured_cdsb_v1`, reached through the
> real three-argument serving selector, reproduce the accepted SRR-1 shadow
> representation exactly while preserving every existing policy and identity
> rule?

SRR-2 has four separate burdens:

1. **Algorithm identity:** production and shadow independently derive the same
   canonical cells, lift, projection, mask, and values.
2. **Scientific-surface identity:** production and shadow outputs are identical
   for every accepted row in all six SRR-1 dataset views and all three seeds.
3. **Backward compatibility:** the four existing policies, defaults, parser,
   active DSL, checkpoint behavior, and public output contract remain intact.
4. **Production integration:** the inference adapter reaches the new branch
   through the existing public selector without changing tokenization,
   `encode()`, training `forward()`, or registered model state.

Failure in any burden prevents a parity claim.

## Exact production change

Append one enum value and exact policy spelling:

```text
structured_cdsb_v1
```

The public interface remains:

```text
select_mtf_serving_pool(encoded, policy, config)
    -> values [B,C,D], valid_mask [B,C]
```

Only the new branch performs the SRR-1 structured readout. It internally owns
the versioned, deterministic `Q_psm` construction. It adds no parameter,
registered buffer, optimizer state, RNG use, fitted statistic, mutable device
cache, or checkpoint payload.

The existing enum ordinals and names are unchanged. `all_tokens` remains the
configuration default, omitted-DSL fallback, and checked-in active policy.

## Exact representation contract

`structured_cdsb_v1` accepts only the architecture proven by SRR-1:

- channels `3`, history `30`, features `9`;
- model and latent widths `32`;
- frequency-domain tokens enabled;
- scales `8,16,32,64` and strides `4,8,16,32`;
- exactly `72` tokens, `24` per channel, with the accepted two-domain layout;
- exactly 16 nonempty cells per channel;
- frozen cell vector
  `0,0,1,1,1,2,2,3,4,5,6,7,8,8,9,9,9,10,10,11,12,13,14,15`;
- frozen `Q_0` hash `f8c9f35282de2ee0` and `Q_psm` hash
  `ac8a43fd65b2c8a8`.

Metadata is validated and sorted independently of incoming token order. A
channel is valid only when its sample mask, channel mask, and all 24 canonical
token masks are true. Invalid channels are exactly zero. Unsupported layouts,
partial channels, malformed metadata, non-finite embeddings, and contract
mismatches fail closed.

## Why quality can be transferred without refitting probes

The SRR-1 probe stack is a deterministic function of frozen feature tensors,
targets, split identities, permutations, bootstrap rows, and probe settings.
SRR-2 does not estimate a nearby score. It requires exact feature identity:

```text
for every seed and accepted dataset row:
    production CUDA values == shadow CUDA values byte for byte
    production masks       == shadow masks byte for byte
```

It also re-authenticates the shadow feature hashes recorded in the sealed
SRR-1 log. Therefore the production probe inputs are the same bytes as the
already audited shadow probe inputs. A deterministic downstream computation on
identical inputs has identical predictions, metrics, intervals, controls, and
classification. Refitting would duplicate compute without adding information.

This transport proof is valid only after all 18 seed/dataset captures pass. A
sampled comparison, tolerance-only comparison, or aggregate metric comparison
cannot transfer the quality result.

## Ordered execution plan

Every milestone is a stop gate.

| Milestone | Work | Evidence required | Cost |
|---|---|---|---:|
| 1. Freeze custody | Archive the exact pre-SRR-2 production/config/test surface and pin all SRR-1 evidence | deterministic archive hash, entry list, plan, protocol, sidecar | very low |
| 2. Implement production branch | Add the enum/parser/name and deterministic structured branch at the serving selector only | narrow reviewed delta; active policy unchanged | low |
| 3. Prove mechanics | Exhaustively test cells, metadata, masks, projection, purity, rejection, dtype/device, old policies, parser, checkpoint identity, and adapter reachability | isolated tests pass on CPU; CUDA cases pass when available | low |
| 4. Build parity harness | Reuse frozen SRR-1 capture generation; compute shadow and production from each one encoder capture | no probes, targets, optimizer, backward, augmentation, or end-to-end path | low–medium |
| 5. CUDA preflight | Run non-scientific groups through the exact candidate binary | production/shadow byte identity; CPU64 reference and translation bounds pass | low |
| 6. Seal candidate | Produce canonical base-to-candidate patch and pre-run manifest | hashes bind parent, archive, delta, sources, binaries, commands, environment | very low |
| 7. One parity run | Capture six frozen views twice for identity for seeds `17,31,47`; retain the first | all 18 retained and 18 repeated production/shadow tensors and masks byte-identical; parent hashes reproduced | medium, no probe fitting |
| 8. Independent audit | Recompute hashes, coverage, invariants, transport proof, and terminal precedence from the log | auditor result matches experiment result | low |
| 9. Human findings | State what changed, what was proven, limitations, and one next decision | clear findings and immutable receipt | very low |

## Required mechanics

The isolated suite must cover:

- stable old enum ordinals/names and the exact new name;
- exact parser acceptance and rejection, omitted fallback, and active DSL;
- the same three-argument public selector and output type;
- exact accepted layout, cell vector, lift order, flattening, and hashes;
- independent production-versus-shadow-versus-CPU64 equality;
- joint token permutation invariance, within-cell invariance, and cross-cell
  sensitivity;
- constant preservation and partition idempotence;
- all-valid, partial-token, missing-cell, whole-channel, whole-sample, and
  all-invalid masks, with unrelated channels unaffected;
- malformed embeddings, masks, metadata, IDs, duplicates, counts, layouts,
  dtypes, devices, non-finite values, and unknown enum rejection;
- output shape, stride, dtype, device, contiguity, finiteness, and exact zeroing;
- CPU float64 and float32 behavior; CUDA float64 and float32 parity when CUDA is
  present;
- repeat identity, input immutability, parameter/buffer identity, RNG identity,
  and model-mode restoration;
- byte-identical golden behavior for `all_tokens`, `time_only`,
  `frequency_only`, and `domain_balanced`;
- protocol fingerprint inequality, checkpoint round trip, legacy missing-policy
  fallback to `all_tokens`, mismatch rejection, malformed-policy rejection, and
  real inference-adapter reachability.

## Cost and scope controls

- Build and run only the focused SRR-2 tests, parity harness, and auditor.
- Warm CUDA before timings used only as diagnostics; latency has no pass/fail
  role.
- Do not fit any representation probe in SRR-2.
- Capture each dataset twice per seed only to establish full accepted-surface
  repeat identity; retain and interpret the first capture.
- Permit exactly one consumed authoritative parity attempt.
- Do not change training, augmentation, encoder architecture, tokenization,
  active serving policy, downstream heads, or deployment.
- Do not broaden the supported structured layout during this version.
- Do not retry a valid negative result.

## Terminal decision order

The first applicable result wins:

1. `invalid_mechanics`
2. `parent_evidence_failure`
3. `backward_compatibility_failure`
4. `sealed_reference_failure`
5. `production_shadow_parity_failure`
6. `device_translation_failure`
7. `production_readout_gate_failure`
8. `production_structured_readout_parity_reproduced`

`production_readout_gate_failure` means byte identity succeeded but the sealed
parent quality classification cannot be transported—for example because a
required parent gate or control is absent or invalid. Scientific interpretation
never overrides an earlier integrity or parity failure.

## What success authorizes

Success means the opt-in production implementation is representation-equivalent
to the SRR-1 shadow. It may support a separately frozen proposal to activate
the policy and deal explicitly with downstream-head compatibility.

It does not authorize activation, checkpoint migration, downstream retraining,
augmentation changes, a long run, end-to-end evaluation, or deployment.
