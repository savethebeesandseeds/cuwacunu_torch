# VVA-1B Exact Authorized Seam Audit

Status: PASS, frozen before any VVA-1B authoritative optimizer update.

## Byte custody

```text
audit_schema=vva1b.authorized_seam_audit.v1
pre_seam_header_sha256=93640972e497dc49f37e7690e59c2f2e55f12ece25687fe4e6f6c96b28c3c9ea
post_seam_header_sha256=d52356ba3f1eb8205402e801950f6ca7b3fb0060d5344201434b91f9b2c7cc4b
isolated_contract_source_sha256=c48900a667c25ffbc10cb1c8805603fb94af97916b0bfd03aeac4010ca4d0a14
isolated_contract_binary_sha256=21c3566da0f24d49d881be596e423e33cbece761de3a6a11cf1b9e896f6b3c18
isolated_contract_runtime=PASS
only_authorized_changes=true
parameter_change=false
production_default_semantic_change=false
pool_definition_change=false
projector_definition_change=false
loss_definition_change=false
validity_definition_change=false
optimizer_change=false
```

The immutable pre-seam bytes are already bound by the authenticated VVA-1
harness and authoritative log. The complete post-seam header is bound by its
independent hash above. The exact authorized delta between those identities is
the inventory below; no other semantic category is authorized.

## Exact authorized delta inventory

1. **Experiment policy inventory.** Add
   `mtf_vicreg_view_pairing_policy_t` with exactly `independent_weak`,
   `tied_weak`, and `clean_identical`, plus its total name function. Add one
   configuration field whose default is exactly `independent_weak`. The default
   therefore selects the historical two-independent-weak-view behavior.

2. **Post-draw substitution.** Preserve the existing two calls to
   `weak_augment(x, feature_mask)` in their original order as `drawn_a` and
   `drawn_b`; then canonicalize the input and select only the used views:

   ```cpp
   independent_weak: view_a = drawn_a; view_b = drawn_b;
   tied_weak:        view_a = drawn_a; view_b = clone(drawn_a);
   clean_identical: view_a = clean;   view_b = clone(clean);
   ```

   Both ordinary weak draws occur for every policy before selection. The two
   existing encoder forwards and, when global VICReg is active, the two
   existing projector forwards remain separate. No shortcut, fused forward,
   or altered RNG prelude is introduced.

3. **Default-off diagnostics.** Add retained drawn-view data/masks, used-view
   data/masks, branch token masks, branch sample-valid masks, global and
   per-channel pools, projected global tensors, joint validity masks, and
   actual encoder/projector call counters. Tensor value/mask receipts are
   detached; the retained pools and projections stay attached so isolated
   gradient checks can audit the real graph. Counters are exposed only when
   the existing debug-return option is enabled.

4. **Output plumbing.** Copy the fields in item 3 from the internal VICReg
   branch result into the public representation output. With debug return off,
   they remain undefined/zero and do not alter production outputs.

5. **Manifest plumbing outside the header.** The canonical configuration
   manifest emits `vicreg_view_pairing_policy` only for a non-default policy.
   Consequently, the default production manifest remains byte-equivalent while
   experimental V1/V2 caches are explicitly versioned.

## Negative audit

The audit found no change to model dimensions, parameter registration or
initialization, tokenizer, encoder, global-pool formula, nonlinear projector,
VICReg component formulas or weights, validity thresholds, gradient clipping,
Adam behavior, EMA behavior, serving readout policy, or production default
selection. The isolated contract proves default-vs-explicit-independent exact
output/RNG equivalence; both-draw consumption; post-draw V1/V2 substitution;
separate call counts; branch-mask identity for identical views; pooled/projected
identity; and zero invariance loss/gradient for identical views.

Any post-audit header hash other than the one above, any failure of the isolated
contract, or any change outside this inventory is a pre-optimizer STOP.
