# SRR-3R — Sparse Structured Activation Compatibility Gate

## Human question

The sparse structured readout is already known to preserve more sequence
information than `all_tokens`. This experiment asks the remaining practical
question: can the existing frozen MDN head use those changed features, or does
activation require a new versioned downstream head?

## Smallest decisive path

1. Seal the exact config, rows, checkpoints, policies, metrics, thresholds,
   seeds, source files, and previously authenticated SRR-4 feature artifacts.
2. Run one no-training confirmation capture on `[760,1088)`. Encode each batch
   once (at most six encoder calls), retain that output, and pass the same
   object through `all_tokens` and `structured_cdsb_sparse_v1`.
3. Load the historical MDN checkpoint honestly under its recorded
   `all_tokens` identity. Verify that loading it as the sparse policy is safely
   rejected, then feed both feature arms through the same already-loaded,
   frozen weights.
4. Stop after Stage A whenever the frozen head is compatible. Compatibility
   plus precommitted material value authorizes a versioned checkpoint-identity
   migration with the frozen weights preserved. Compatibility without material
   value retains `all_tokens` and does not authorize adaptation.
5. Only if Stage A classifies the old head as incompatible, conditionally admit
   the already sealed SRR-4 equal-compute ridge result as bounded head-recovery
   evidence. It used these exact sparse features, splits, heads, gates, and
   confirmation rows, so repeating it would add no evidence. Hash-verify and
   report it honestly as pre-existing evidence; do not encode or fit again.
6. Record one explicit outcome: frozen weights warrant a versioned identity
   migration; a fresh/adapted versioned head is required; the downstream
   bottleneck remains unresolved; or mechanics are invalid. Keep `all_tokens`
   active and as rollback.

## Stop rules

- Any authority, custody, pairing, shape, mask, finiteness, identity, replay,
  or encoder-budget failure stops before endpoint interpretation.
- Stage B is forbidden unless Stage A says `frozen_head_incompatible`.
- The final holdout `[1088,1170)` and augmentation attribution remain closed.
- No encoder, augmentation, existing checkpoint weight, or head is trained or
  changed by SRR-3R. Conditional Stage-B evidence is imported, not refit.

## Advancement supplied by the experiment

SRR-4 established that the repaired representation itself is valuable.
SRR-3R resolves the next boundary: whether activation can reuse the historical
head weights or must ship a newly trained, representation-versioned head.
