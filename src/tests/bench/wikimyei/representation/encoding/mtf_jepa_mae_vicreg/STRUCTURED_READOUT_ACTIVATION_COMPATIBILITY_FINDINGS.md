# SRR-3 — Structured Readout Activation Compatibility Findings

## Decision

**`downstream_bottleneck_remains_unresolved`**

This is the controlling Amendment-A2 spelling of the user's third allowed
decision. The original plan/protocol token
`downstream_bottleneck_unresolved` is an alias for the same outcome, not a
separate classification.

Do not activate `structured_cdsb_v1`, do not migrate the MDN checkpoint, and
do not run the planned head-only adaptation yet. Keep `all_tokens` active as
the explicit rollback.

This is not a reversal of SRR-1 or SRR-2. The structured representation remains
the accepted repair on the complete module-only surface, and the opt-in
production selector remains exactly equivalent to that accepted repair. SRR-3
found a new incompatibility at the next boundary: the live graph-first input is
partially observed, while `structured_cdsb_v1` fails an entire channel closed
unless all 24 planned tokens are valid.

## What SRR-3 tested

The actual downstream boundary is:

```text
frozen MTF tokens
  -> serving readout [M,3,32] with mask [M,3]
  -> graph restore [B,N,3,32]
  -> ChannelContextMDN backbone/adapters/mixture/direct head
```

Stage A was designed as a paired no-training comparison on historical anchors
`[760,1088)`: one retained encoder object per source batch, both production
readout selectors, then the same frozen MDN weights. Checkpoints, rows, order,
seeds, metrics, thresholds, augmentations, and the final holdout were frozen.

The original base config selected a policy-training wave that the graph-first
builder correctly rejected before source construction. Amendment A1 froze the
same MDN-evaluation wave used by the historical baseline capture. No model or
endpoint ran in that first orchestration attempt.

The corrected Stage-A attempt then reached the first paired production batch.
It stopped before feature persistence and before any MDN forward because mask
parity failed. A one-batch diagnostic named the exact invariant:

| Invariant | Result |
|---|---:|
| Same retained encoded object before/between/after both selectors | pass |
| Values shape, mask shape, dtype, device, and contiguity | pass |
| Invalid values exactly zero | pass |
| Readout values finite | pass |
| Source data and feature mask unchanged | pass |
| Legacy and structured validity masks identical | **fail** |

No Stage-A task endpoint was therefore computed. The attempt contains only
pre-metric manifests and an empty `stage_a` directory; there are no feature
CSVs, predictions, evaluator reports, completion receipt, or Stage-B artifact.

## Extent of the incompatibility

Amendment A2 authorized a non-gating, tokenizer-only census over the same
frozen `[760,1088)` range. Validity masks depend on token masks and metadata,
not latent values or encoder weights: the production `encode()` method copies
those tokenizer masks into its output, and both serving selectors derive their
validity masks from them. The census therefore used a zero-valued, shape-valid
carrier with the exact production token masks and metadata. It invoked both
production selectors on the same carrier, used zero encoder calls, never
accessed the MDN checkpoint, and emitted no features, predictions, or endpoint
metrics.

The counting unit is one anchor × node × channel position in the selector's
`[M,3]` validity mask.

| Coverage over 328 anchors × 4 nodes × 3 channels | `all_tokens` | `structured_cdsb_v1` |
|---|---:|---:|
| Valid positions | 3,936 / 3,936 | 1,312 / 3,936 |
| Valid fraction | 100% | 33.33% |

The 2,624 mismatches are completely channel-structured:

| Channel | Positions | Legacy valid | Structured valid | Mismatches |
|---|---:|---:|---:|---:|
| 0 | 1,312 | 1,312 | **0** | 1,312 |
| 1 | 1,312 | 1,312 | **0** | 1,312 |
| 2 | 1,312 | 1,312 | 1,312 | 0 |

There are no structured-only positions and no positions invalid in both arms.
The current structured selector therefore masks and exactly zeroes channels 0
and 1 for every retained anchor/node row while leaving channel 2 valid.

All census mechanics passed: exact source range and order, six tokenizer calls,
zero encoder calls, exact value/mask shapes, finite outputs, unchanged
parameters/buffers/evaluation mode, unchanged CPU/CUDA RNG, no augmentation,
optimizer, backward call, MDN access/forward, endpoint metric, or checkpoint
write. The final holdout `[1088,1170)` was not accessed.

One audit qualification is important. Because the census carrier is
zero-valued, its dynamic `invalid_zero_exact=true` observation is by itself
non-discriminating. The mask counts do not depend on that observation. Exact
invalid zeroing is instead supported by the production selector's explicit
`where(valid_mask, values, zeros)` operation and by the real-encoded first-batch
diagnostic, which reported `invalid_zero_exact=true`. That diagnostic output is
preserved as a clearly labeled postmortem transcription, not represented as a
contemporaneous runtime receipt.

Across SRR-3, only two source-dependent encoder calls were made: the original
first-batch stop and its one-batch diagnostic replay. This remains below the
precommitted six-call Stage-A ceiling.

## Why the head tests stopped

The `[B,3,32]` value shape is mechanically acceptable, but shape compatibility
is insufficient. The candidate changes effective coverage from three channels
to one before the graph adapter and MDN head. Running endpoint metrics would
therefore compare different input populations, not two readouts over the same
frozen task surface.

A head-only adaptation cannot recover the two channel representations already
masked and zeroed by the readout. Training only on the common valid subset
would reduce the task to channel 2 and change the frozen population and
estimand. It would not answer whether the repaired three-channel
representation improves downstream prediction. Stage B was correctly not
authorized.

The legacy MDN checkpoint also authenticates as `all_tokens` and safely rejects
`structured_cdsb_v1` identity. That guard remains correct. Even after the mask
boundary is repaired, activation will require a versioned readout/head
checkpoint identity; SRR-3 has not yet established whether that migration can
reuse the old head weights or requires adaptation.

## What we learned and how this advances the project

- The activation obstacle appears before downstream optimization: it is the
  structured readout's complete-block validity rule on the sparse production
  surface.
- We did not mistake an old head's failure on damaged inputs for failure of the
  repaired representation.
- We avoided an invalid head-training experiment and a long end-to-end run.
- The next repair boundary is now precise: partial-token support must be
  represented without discarding whole channels.

## What remains unknown

- Whether a partial-mask structured readout preserves the SRR-1 representation
  advantage on the actual sparse graph-first surface.
- Once all three channels have a valid, versioned structured surface, whether
  the existing frozen MDN weights preserve or improve direction, rank, RMSE,
  correlation, and best-asset endpoints.
- If those frozen weights are semantically incompatible, whether the bounded
  equal-compute head-only adaptation recovers the structured value.
- Whether augmentation contributes additional harm after the repaired
  readout/downstream boundary is held fixed.

## Recommended next gate

Proceed separately with **SRR-4 — Sparse-Surface Structured Readout Contract
Repair**:

1. Specify a versioned partial-mask policy; do not silently change
   `structured_cdsb_v1` and do not mark zeroed channels valid.
2. Preserve the structured channel/domain/scale layout using mask-aware cell
   summaries and an explicit support/normalization contract.
3. Validate representation quality on the actual sparse production mask
   surface, not only complete synthetic blocks.
4. Transport the accepted policy through the same production selector parity
   checks.
5. Repeat SRR-3 from Stage A. Run head-only Stage B only if masks and coverage
   match and the frozen head then fails the precommitted endpoint gate.

Augmentation attribution remains deferred until this boundary is repaired,
retested, and held fixed.

## Evidence

- Protocol: `STRUCTURED_READOUT_ACTIVATION_COMPATIBILITY_PROTOCOL.md`, SHA-256
  `1c24e92a49bb59b0f0a7db63917428399619a0783216f6f3c9049c5a46cbace3`
- Amendment A1: SHA-256
  `68d4a96394faad9e8da736bf41a240f54474f92480056072c3a4d5456f4e5b4c`
- Amendment A2: SHA-256
  `88aaaaa352f09b0d59bda313abd70743908f3058ab56a76fed535e1df2a57036`
- Corrected Stage-A pre-metric root:
  `.runtime/benchmarks/synthetic_continuous_graph_v1/wikimyei.mtf_jepa_mae_vicreg.srr3_activation_compatibility.v1.attempt_000002`
- Corrected Stage-A postmortem failure transcription:
  `.runtime/benchmarks/synthetic_continuous_graph_v1/wikimyei.mtf_jepa_mae_vicreg.srr3_activation_compatibility.v1.attempt_000002/postmortem.failure.receipt`
- First-batch diagnostic postmortem transcription:
  `.runtime/benchmarks/synthetic_continuous_graph_v1/wikimyei.mtf_jepa_mae_vicreg.srr3_activation_compatibility.diagnostic.v1/attempt_000001/postmortem.diagnostic.receipt`
- Mask-census receipt:
  `.runtime/benchmarks/synthetic_continuous_graph_v1/wikimyei.mtf_jepa_mae_vicreg.srr3_activation_compatibility.mask_census.v1/attempt_000001/mask_contract_census.receipt`,
  SHA-256
  `c5192c090e05302e0d480c76ebfe6ef47f0f34cf8d9e3832b435cca3aa63f094`
- The census protocol amendment was hash-sealed before dispatch. Its
  `authority.sha256` and `outputs.sha256` manifests were recorded and replayed
  after completion, so they are a retrospective evidence seal rather than a
  pre-dispatch seal. Two independent read-only audits verified all recorded
  hashes and the count algebra.
- Active production policy remains `all_tokens`.
