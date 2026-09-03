# SRR-3 — Structured Readout Activation Compatibility Gate

## Purpose

Determine whether the repaired `structured_cdsb_v1` readout can cross the
existing representation-to-MDN boundary safely, and whether activation can use
the frozen downstream checkpoint or requires a versioned downstream-head
migration.

SRR-1 and SRR-2 are fixed premises: `all_tokens` loses useful sequence
information; the production structured path is the accepted structured
readout; and its representation-quality result has already been audited. SRR-3
does not repeat those studies.

## Actual boundary

The frozen MTF encoder emits token values and masks. The serving-pool selector
maps either policy to `[M,3,32]`; the graph adapter reshapes that surface to
`[B,N,3,32]`; the frozen `ChannelContextMdn` then maps each 32-coordinate node
and channel through its learned backbone, channel adapters, mixture head, and
direct edge-return head. Shape compatibility is therefore necessary but not
sufficient: the first MDN layer and everything after it were co-adapted to the
old coordinate semantics.

The historical MDN checkpoint is also identity-bound to `all_tokens`. A legacy
checkpoint without the new field is deliberately interpreted as `all_tokens`
and must reject a structured-policy load. That guard remains intact.

## Staged plan

1. **Seal inputs and gates.** Freeze the two checkpoint hashes, active
   `all_tokens` DSL, historical synthetic-v1 rows, anchor splits, source order,
   seeds, metrics, thresholds, and compute ceiling.
2. **Stage A — frozen-head paired A/B.** On historical anchors `[760,1088)`,
   encode every source batch once. Retain the encoded object and derive both
   readouts from it. Pass both through the same evaluation-mode MDN weights
   without training. Compare contracts, distributions, predictions, and the
   precommitted task endpoints.
3. **Stage-A stop gate.** Stop if mechanics are invalid. If the frozen head is
   compatible and the structured arm meets the acceptance gate, do not train;
   recommend a versioned checkpoint identity migration while keeping
   `all_tokens` as rollback.
4. **Stage B — conditional matched head-only adaptation.** Run only when Stage
   A mechanics pass but the frozen head is semantically incompatible. Capture
   development readout features once per source batch for both arms. Fit equal
   deterministic per-edge ridge heads with the same split, feature layout,
   standardization, alpha grid, selection rule, and refit rule. Confirm only on
   the already-consumed `[760,1088)` historical range.
5. **Decision and audit.** Independently validate the sealed inputs, execution
   receipts, paired-row invariants, gates, and final classification. Do not open
   `[1088,1170)` and do not begin augmentation attribution.

## Allowed final decisions

- `safe_direct_activation`: only if checkpoint identity and Stage A both pass.
- `activation_requires_versioned_head_checkpoint_migration`: the structured
  surface is useful, but the legacy head identity or semantics cannot be used
  directly; the report states whether metadata-only migration or a fresh head
  is required.
- `downstream_bottleneck_unresolved`: bounded head adaptation does not recover
  the value or the experiment is otherwise scientifically inconclusive.

`all_tokens` remains the active default and explicit rollback throughout SRR-3.

