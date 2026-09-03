# SRR-3R — Sparse Structured Activation Compatibility Findings

Date: 2026-08-28
Terminal decision: `downstream_bottleneck_remains_unresolved`
Stage-A classification: `compatible_no_downstream_gain`

## Human conclusion

The repaired sparse representation is safe for the historical frozen MDN to
consume, but the historical head does not use its additional information.

Switching only the readout preserved every downstream validity row and did not
degrade any frozen compatibility gate. It also did not improve a single
direction, asset-rank, or best-asset decision. RMSE changed by less than one
thousandth of one percent, and correlation changed by about five millionths.
All four precommitted material-value flags failed.

This is an important localization result. SRR-4 already showed that the sparse
representation materially improves a freshly fit, equal-compute readout on
the same confirmation population. SRR-3R now shows that those useful feature
directions do not propagate through the historical MDN weights. The sparse
readout is no longer the failing boundary; the existing downstream head is
functionally insensitive to the repair.

Do not directly activate the sparse policy with the old checkpoint. Such a
switch would provide no demonstrated downstream benefit, and the checkpoint
correctly rejects the sparse policy identity. Keep `all_tokens` active and as
rollback. A separately versioned, production-relevant head adaptation gate is
needed before activation.

## What was tested

Stage A was a no-training, same-object counterfactual on historical anchors
`[760,1088)`:

```text
one frozen encoder output per source batch
  ├─ all_tokens -> existing graph/MDN adapters -> frozen historical MDN
  └─ structured_cdsb_sparse_v1 -> same adapters -> same frozen MDN instance
```

The representation encoder, graph rows, targets, config, checkpoints, seed,
metrics, thresholds, and compute ceiling were frozen. The historical MDN
checkpoint was loaded exactly once under its truthful `all_tokens` identity.
The sparse-policy identity was separately attempted and safely rejected before
weight loading. Candidate inference was explicitly a scientific manual feed of
sparse semantics into the already authenticated legacy head; no checkpoint was
relabeled, copied, rewritten, or bypassed.

## Mechanical and custody result

All pre-metric mechanics passed:

| Invariant | Observed | Result |
|---|---:|---|
| Anchors | 328, exactly `[760,1088)` | pass |
| Source batches / encoder calls | `6 / 6` | pass |
| MDN constructions / successful weight loads | `1 / 1` | pass |
| Identity-authentication attempts | 2 | pass |
| MDN forwards | 12, exactly two per batch | pass |
| Public node rows | 1,312 | pass |
| Context-mask cells | `3,936 / 3,936` in both arms | pass |
| Feature rows | 2,952 in both arms | pass |
| Prediction rows and valid rows | `2,952 / 2,952` in both arms | pass |
| Same retained encoder object | raw-byte stable throughout | pass |
| Representation and MDN parameters/buffers | unchanged | pass |
| CPU/CUDA RNG and evaluation modes | unchanged | pass |
| Sparse expected checkpoint identity | safely rejected | pass |
| Optimizer/backward/checkpoint writes | `0 / 0 / 0` | pass |
| Augmentation and final holdout | not used / not opened | pass |

The fresh feature artifacts reproduced the qualified SRR-4 confirmation
authorities byte exactly:

- `all_tokens` SHA-256
  `8f6f72b78c0708b5f23512ada4ca8536ea8818f8e2d3d9bc501401d8ab0ce3c7`;
- `structured_cdsb_sparse_v1` SHA-256
  `dfac215b73b08525dcba90d8891c8dede328ed99ec0117e2e2efaea6a5afbd73`.

The feature arms were close but genuinely different: global cosine was
`0.9972018`, mean row cosine `0.9972307`, and minimum row cosine `0.9953940`.
The lack of downstream movement is therefore not because both policies
produced identical feature bytes.

## Frozen-head endpoint result

Confidence intervals use the sealed 4,096 paired anchor-cluster bootstrap with
NumPy `PCG64` and seed `8387496322364763509`.

| Endpoint | `all_tokens` | Sparse structured | Paired change |
|---|---:|---:|---:|
| Directional accuracy | 0.48103 | 0.48103 | `0.00000`, 95% CI `[0,0]` |
| Pairwise rank accuracy | 0.50813 | 0.50813 | `0.00000`, 95% CI `[0,0]` |
| RMSE | 0.02833506 | 0.02833485 | ratio `0.9999923`, 95% CI `[0.9999848,0.9999999]` |
| Correlation | -0.007598 | -0.007593 | `+0.0000049`, 95% CI `[-0.0000293,+0.0000383]` |
| Best-asset agreement | 0.33943 | 0.33943 | `0.00000`, 95% CI `[0,0]` |
| Valid coverage | 1.00000 | 1.00000 | `0.00000` |

The MDN's predictions themselves barely changed: their mean moved from
`-0.00082368` to `-0.00082734`, and standard deviation from `0.00546615` to
`0.00546456`. The three decision-like endpoints—direction, pairwise rank, and
best asset—were exactly unchanged over the full paired surface.

## Gate decision

All compatibility/noninferiority gates passed:

- direction lower bound `0 >= -0.02`;
- rank lower bound `0 >= -0.02`;
- RMSE-ratio upper bound `0.9999999 <= 1.10`;
- finite outputs and exact coverage parity.

No material-value gate passed:

- direction gain was `0`, not at least `+0.02`;
- rank gain was `0`, not at least `+0.02`;
- RMSE ratio was `0.9999923`, not at most `0.95`;
- correlation gain was approximately `0.000005`, not at least `+0.05` with a
  positive lower bound.

The sealed classification is therefore `compatible_no_downstream_gain`.
Under the precommitted stop rule, bounded Stage-B evidence was not authorized
or admitted: zero development rows were opened, zero new encoder calls were
made, and zero heads were fit after Stage A. This avoids retrospectively
changing the experiment because the observed result was disappointing.

The final decision is `downstream_bottleneck_remains_unresolved`, not
`safe_direct_activation` and not a checkpoint-migration authorization.

## What this advances

SRR-3R resolves all three questions about the historical frozen boundary:

1. **Can the old head consume the sparse representation?** Yes mechanically.
   Shapes, masks, finiteness, graph transport, and every compatibility gate
   pass with full paired coverage.
2. **Does a readout-only switch improve the old frozen endpoints?** No. The
   old head makes exactly the same direction, rank, and best-asset decisions.
3. **Can the legacy checkpoint be activated directly as sparse?** No. Its
   `all_tokens` identity correctly rejects the sparse policy, and the manual
   feed demonstrates no material reason to migrate those frozen weights.

Together with SRR-4, the evidence now says:

```text
sparse structured features contain more task-accessible information
                           ↓
historical frozen MDN does not convert it into better predictions
```

That is stronger and more useful than saying merely that the representation
or end-to-end system is weak. It identifies the next engineering/scientific
boundary precisely.

## What remains unknown

- Whether a freshly initialized production `ChannelContextMDN`, trained only
  on cached frozen representations, can exploit the sparse feature advantage.
- Whether warm-start adaptation of the old head is sufficient, or whether its
  learned mapping/conditioning makes a fresh head necessary.
- Where the useful sparse displacement is attenuated: channel adapter,
  residual backbone, mixture path, or direct edge-return head.
- Whether a new sparse-policy MDN checkpoint passes a versioned identity load,
  rejects `all_tokens`, and preserves its accepted predictions after migration.
- Whether augmentation causes additional harm after the sparse readout and a
  compatible trained head are both held fixed.

## Recommended next gate

Proceed separately with **FHSL-1 — Frozen Head Signal-Transfer
Localization** before spending compute on head training.

This should be a zero-training, zero-encoder-call audit that reuses the paired
authenticated SRR-3R contexts and the frozen MDN checkpoint. Measure how the
actual `all_tokens`→sparse displacement changes in norm, cosine, and directional
gain at the channel adapters, residual backbone, mixture path, and direct
edge-return head. Precommit same-norm controls and a small interpolation along
the real paired displacement so saturation is distinguishable from simple
attenuation. Do not reopen targets to select a layer and do not open the final
holdout.

That result chooses the smallest trainable boundary instead of reflexively
retraining the entire MDN:

- if the displacement vanishes in the channel adapters, adapt or replace those
  adapters;
- if it survives the backbone but disappears at the output head, adapt only
  the direct/mixture readout;
- if the head responds but decisions do not cross their margins, test a
  bounded calibration/readout layer;
- if no production subpath preserves the displacement, authorize a fresh-head
  comparison.

The subsequent production gate should be **SRR-5 — Versioned Structured Head
Adaptation Gate**: cache both frozen context arms once, train only the
FHSL-1-implicated submodule from paired initialization and equal compute, and
compare fresh/adapted `all_tokens` versus sparse on the historical confirmation
population. No encoder call is allowed inside head training. A new checkpoint
must authenticate as `structured_cdsb_sparse_v1`, reject `all_tokens`, and
replay accepted predictions exactly before activation. Until then,
`all_tokens` remains active and augmentation attribution remains deferred.

## Authority and replay

- Sealed protocol SHA-256:
  `6deee9c2420e205828322cee34b8d5d43a83c98918670ede682d8e36e17de6da`.
- Runtime root:
  `.runtime/benchmarks/synthetic_continuous_graph_v1/wikimyei.mtf_jepa_mae_vicreg.srr3r_sparse_activation_compatibility.v1/attempt_000001`.
- Stage-A report and byte-exact replay SHA-256:
  `4e5f7aeba0f69a377ed6f4ac194eb6edbc47cec39183f4886abd5cd0699d7c3f`.
- Completion receipt SHA-256:
  `6bf37c7c529d93976c6310478b286f0249aa41b976e1750416866d513c765d72`.
- Final output manifest SHA-256:
  `47fcca4da0a097f529181fee785d484466e8afd4d26d1e3d85a3194fb99a4a7a`.
- Six custody manifests contain 806 references; the independent post-run audit
  found zero malformed, missing, or SHA-mismatched entries and reproduced the
  endpoint bounds, classification, and final decision.

No production configuration, active policy, or checkpoint was changed.
