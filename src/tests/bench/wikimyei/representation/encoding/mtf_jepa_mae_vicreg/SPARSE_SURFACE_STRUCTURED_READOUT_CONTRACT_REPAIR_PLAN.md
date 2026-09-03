# SRR-4 — Sparse-Surface Structured Readout Contract Repair Plan

## Outcome sought

Repair only the mask/readout boundary exposed by SRR-3. The accepted
`structured_cdsb_v1` readout works on complete 24-token channel blocks, but the
live graph-first surface supplies front-padded histories of 4, 10, and 30 rows.
The v1 all-token validity rule therefore discards the first two channels before
the downstream head can see them.

SRR-4 will add an opt-in, separately versioned `structured_cdsb_sparse_v1`
policy that retains observed structured cells and completes absent cells only with a
contrast-neutral mean from their own time/frequency domain and scale. It will
not change `structured_cdsb_v1`, encoder weights, augmentations, training, the
active `all_tokens` DSL, or any checkpoint.

## Fixed facts; do not spend compute re-estimating them

- `all_tokens` averaging destroys useful sequence information on the complete
  module-only surface.
- `structured_cdsb_v1` is exactly equivalent to the accepted structured shadow
  and has already passed its representation-quality and production-parity
  audits.
- SRR-3 found 3,936/3,936 legacy-valid channel cells but only 1,312/3,936
  v1-valid cells on anchors `[760,1088)`. Channels 0 and 1 were always rejected;
  channel 2 was always accepted.
- This mismatch occurs before the graph adapter, MDN, and downstream endpoint.
  It is not evidence that the repaired representation or old head failed.

## Stages and stop gates

1. **Seal the contract.** Freeze the partial-mask estimator, policy name,
   authority, source ranges, metrics, thresholds, and compute ceiling before
   implementation or endpoint inspection.
2. **Implement and prove mechanics.** Append `structured_cdsb_sparse_v1`; leave v1 and
   all legacy policies unchanged. Require independent sparse-mask oracle tests,
   exact v1 parity on complete blocks, mask/finiteness/zeroing/purity tests, DSL
   parsing and identity propagation tests, and CPU/CUDA translation checks.
   Stop as `invalid_mechanics` on any failure.
3. **Capture the actual sparse surface once.** With the frozen encoder in eval
   and no-grad mode, retain each encoded object and feed that same object through
   `all_tokens` and the sparse policy. Capture `[0,730)` for fit/selection and `[760,1088)` for
   confirmation. The freshly captured legacy probes must be byte-identical to
   the already sealed probes.
4. **Run one bounded representation-value comparison.** Fit equal-compute,
   deterministic CPU-float64 ridge probes on the two 96-wide graph edge feature
   surfaces. Use only the frozen development split and inspect only the
   historical confirmation range. No MDN forward, head checkpoint, optimizer,
   backward call, augmentation, or final holdout is permitted.
5. **Decide.**
   - `sparse_structured_repair_qualified`: mechanics pass, coverage matches
     `all_tokens`, quality is noninferior, and at least two primary material
     value flags pass. Authorize a fresh SRR-3 Stage A with the sparse policy.
   - `sparse_surface_value_gate_not_passed`: mechanics and coverage pass, but
     the frozen quality gate does not qualify the candidate. This is not proof
     of no value. Keep `all_tokens`; do not repeat SRR-3 yet.
   - `invalid_mechanics`: any contract, authority, custody, or compute gate
     fails. Inspect no quality result and keep `all_tokens`.

## Explicit exclusions

- No change to the active serving policy or checkpoint identity.
- No rerun of SRR-1/SRR-2 probes, parity studies, or bootstraps.
- No downstream MDN test or head-only adaptation; those belong to the fresh
  SRR-3 run after SRR-4 qualifies.
- No augmentation attribution until this boundary is resolved and held fixed.
- No access to final anchors `[1088,1170)`.

## Human handoff requirement

The final findings must state plainly: what the sparse contract did, whether it
restored all three channel masks, whether it preserved value relative to
`all_tokens`, the exact decision, what remains unknown about the frozen MDN
head, and the single next authorized experiment. `all_tokens` remains the
explicit rollback in every outcome.
