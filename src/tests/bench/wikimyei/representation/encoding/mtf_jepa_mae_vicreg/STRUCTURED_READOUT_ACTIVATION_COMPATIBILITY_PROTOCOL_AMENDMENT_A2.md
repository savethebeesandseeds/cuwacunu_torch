# SRR-3 Protocol Amendment A2 — Pre-Metric Mask-Contract Census

Sealed after the Stage-A mechanics stop and before any downstream forward,
endpoint evaluation, or Stage-B work.

## Trigger

Authoritative attempt 000002 stopped on the first retained source batch because
`all_tokens` and `structured_cdsb_v1` emitted unequal validity masks. A
diagnostic replay of that first batch named the failed invariant:
`pool_masks_exact=false`; shape/dtype/device, shared encoded-object integrity,
invalid-value zeroing, finiteness, and source-input immutability all passed.
Both runs stopped before feature persistence and before the frozen MDN forward,
so no task endpoint has been exposed.

The mismatch is structurally plausible: `all_tokens` declares a channel valid
when any channel token is valid, whereas `structured_cdsb_v1` requires all 24
tokens in its frozen channel plan. The first-batch stop does not quantify the
extent of that mismatch over the full frozen Stage-A population.

## Frozen diagnostic

Run exactly one no-head mask census on anchors `[760,1088)` using the already
frozen config, source ordering, seed, and representation checkpoint. For each
retained source batch, call the production tokenizer once, construct a
zero-valued shape-valid encoded carrier from that tokenizer's exact token masks
and metadata, and feed the same carrier object through both production readout
selectors. This is valid for a mask census because neither readout's validity
mask depends on latent values or encoder weights.

The census must record, globally and per channel:

- total mask cells;
- legacy-valid and structured-valid cells;
- both-valid, legacy-only, structured-only, and both-invalid cells;
- mismatch count and coverage fractions.

It must also prove exact readout shape/dtype/device contracts, shared-carrier
immutability, finite outputs, exact invalid-value zeroing, contiguous source
order, unchanged parameters/buffers/evaluation mode/RNG state, no augmentation,
zero encoder calls, zero MDN checkpoint access or forwards, zero feature or
prediction artifacts, zero endpoint metrics, zero optimizer/backward steps,
and no checkpoint writes. The census may use at most six tokenizer calls, one
per retained source batch, and may not access `[1088,1170)`.

## Interpretation and stop rule

This diagnostic does not weaken or replace the original identical-coverage
gate. Attempt 000002 remains a pre-metric mechanics failure. Any nonzero mask
mismatch confirms that the currently versioned structured readout is not
activation-compatible on the frozen production surface. Do not run Stage B:
a head-only adaptation cannot recover features that the readout has already
masked and zeroed, and intersection-only fitting would change the frozen
population and estimand.

The permitted SRR-3 decision is then `downstream_bottleneck_remains_unresolved`,
with `all_tokens` retained as the explicit rollback. The prerequisite for a
new activation attempt is a separately versioned partial-mask structured
readout contract, or an independently justified upstream mask normalization,
followed by representation validation on the sparse production surface.
