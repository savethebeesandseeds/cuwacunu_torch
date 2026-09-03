# IMA-5 — Support-Permitted Teacher Target Alignment Protocol

## Question and bounded stop

IMA-5 asks whether JEPA becomes usable when its teacher target contains only
sample-varying information that the legal support-separated context can
predict.  It does not reopen mask design, predictor-capacity search,
augmentation attribution, or the accepted structured serving readout.

The experiment stops in exactly one of two places:

1. IMA-5A rejects the candidate and closes the present JEPA branch without an
   encoder update; or
2. IMA-5A admits one candidate, IMA-5B runs one paired 512-update
   representation A/B, and the result is classified as mechanism confirmed,
   objective made safe, representation rescue, or rejected.

No nearby predictor, ridge, EMA, pooling, or target variant may be added after
the protocol hash is frozen.

## Settled inputs

The following are evidence inputs and are not re-estimated:

- M2 `support_separated_pair_v1` is the legal context boundary.
- IMA-4A oracle C is the smallest tested map with positive held-out exact-target
  signal.
- IMA-4B and IMA-4C exhausted the bounded nearby predictor-capacity line.
- FSPA-4 seeds `17`, `31`, and `47` are the frozen representation anchors.
- The production input normalizer, tokenizer, online encoder, EMA target
  encoder, augmentations, target selection, representation probes, serving
  readout, seeds, and metrics remain fixed.

The inherited production input normalizer was fitted on SSL groups `[0,256)`
as part of the frozen FSPA/RMC checkpoint contract.  It is not re-fitted.
"Calibration-only normalization" below refers to every new IMA-5 teacher
statistic: slot means, feature means, feature variances, categorical frequency
weights, intercept, coefficients, and ridge selection.

## Frozen group manifest

| purpose | absolute group range |
|---|---:|
| representation SSL / frozen input normalizer | `[0,256)` |
| predictor/probe fit | `[1000000,1000256)` |
| predictor/probe validation | `[2000000,2000128)` |
| development | `[3000000,3000256)` |
| teacher calibration fit | `[6000000,6000256)` |
| teacher ridge selection only | `[7000000,7000128)` |
| final IMA-5B confirmation, initially sealed | `[8000000,8000256)` |

The teacher calibration rows never enter predictor fitting, representation
training, representation probes, development decisions, or confirmation.
The 8M confirmation is not generated or encoded during IMA-5A.  It is opened
once, only if an admitted IMA-5B arm passes every development gate.

## Candidate definitions

All definitions are seed-coordinate-specific and use the frozen FSPA-4 anchor.
The target slots and M2 context mask are identical across candidates.

### E — exact full EMA latent

`E_s = z_s`.  This is the already-rejected reference target.  It receives only
custody and shape smoke checks and is never re-promoted by IMA-5.

### D — support-zeroed target-slot latent

Zero the target slot and every same-channel cross-scale/domain token whose raw
interval overlaps it, retain the original token-validity mask, run the frozen
EMA target encoder, and select slot `s`.  D is a deletion sanity control and is
never eligible for representation training.

### L — slot-conditioned legal-context teacher, version 1

For each target slot `s`, compute on teacher calibration-fit rows only:

```
mu_s = mean(z_s | target slot s)
r_s  = z_s - mu_s
```

Build the exact canonical M2 legal field used by IMA-4A C: all 72 online
context latents in production order, zero in non-context positions, followed
by all 72 explicit context-mask bits.  Prepend the unchanged six-coordinate
target metadata query.  Standardize continuous features from calibration-fit
rows only.  Apply the C categorical target-slot-by-field dual kernel with
fit-only inverse-frequency weights.  Select alpha on the 7M calibration
selection rows using the frozen IMA-4A grid and target-slot-centered NMSE; do
not refit after selection.  Fit a calibration-only residual intercept.

The immutable teacher target is

```
L_s = C_s(Phi_legal)
```

not `z_s`, not `z_s - C_s(Phi_legal)`, and not `mu_s + C_s(Phi_legal)`.
It remains 32-dimensional.  The teacher coefficients and every associated
statistic are frozen before any admission row is scored.

## IMA-5A — zero-update admission gate

IMA-5A performs zero representation optimizer steps and zero EMA updates.

### A. Mechanical and intervention certificate

Before reading target values, certify that all 72 target identities occur in
the 6M calibration-fit capture for every seed.  Record exact group, target,
context-mask, and candidate-field hashes.

For a fixed legal M2 context, replace every hidden target-support/alias token in
turn with zeros, a fixed permutation, large finite deterministic noise, and the
corresponding token from another sample.  The masked encoder input, legal
field, and L target must be byte-identical to the unmodified result.  The
target-support closure must have zero intersection with M2 context.  Any
difference rejects L as leakage.

All model parameters and buffers must be bit-identical before and after the
stage; gradients are cleared.  Non-finite data or an unresolved ridge-grid edge
fails closed.

### B. Non-collapse and slot-lookup gates

Compute on development rows after subtracting development per-slot means for
the spectral quantities.  Report total variance, within-slot variance,
slot-mean variance share, effective rank, participation rank, largest-PC share,
time/frequency variance, each channel's within-slot variance, and the fraction
of within-slot variance carried by each channel.

L passes only when, in every seed:

- total and within-slot coordinate variance exceed `1e-8`;
- effective rank is at least `4.0`;
- participation rank is at least `4.0`;
- largest-PC share is below `0.75`;
- slot-mean variance is below `0.50` of total variance;
- time, frequency, and every channel have within-slot variance above `1e-8`;
- every channel carries at least `0.01` of total within-slot variance.

D is labelled deletion-collapsed if it violates any rank, domain, slot-share,
order, or channel-variation gate.  D remains non-promotable even if it does not
collapse.

### C. Unchanged-production-predictor gate

Instantiate the exact production `LatentPredictor`, copy the existing
checkpoint weights, and train only those predictor parameters.  The encoder,
tokenizers, EMA encoder, teacher L, masks, and captures stay frozen.  Reuse the
IMA-4B fit/validation/development rows and its exact schedule:

- Adam, learning rate `1e-3`, clip norm `5.0`;
- `1,024` updates, group batch `32`;
- validation checkpoints
  `{0,64,128,192,256,320,384,448,512,640,768,896,1024}`;
- validation selects one immutable snapshot; no development selection;
- the run is compute-censored if the selected point is 1,024 and the final
  increment is at least `0.005` R2.

R2 uses development target-slot-centered L variance.  L passes the combined
predictor gate only if mean R2 is at least `0.50`, every seed is at least
`0.25`, the paired group-bootstrap 95% lower bound is above zero, and no seed
is compute-censored.  Apply the same thresholds independently to time and
frequency target rows.

Failure closes the present JEPA branch.  It does not imply that the repaired
target lacks signal; it means the unchanged production JEPA boundary cannot
consume the support-permitted target, and IMA-4 already forbids another local
predictor search.

### D. Structural gates

Use the unchanged ridge grid, fit/validation/development rows, and group-level
targets.  A group feature is the deterministic concatenation of its two L
targets in ascending target-slot order.

L must satisfy all of the following across the three frozen seeds:

- original-versus-exact-reversal AULC has mean at least `0.60`, every seed is
  above `0.50`, and its paired 95% lower bound is above `0.50`;
- three-way target-channel discrimination after fit-only per-slot centering has
  mean accuracy at least `0.50` and every seed at least `0.40`;
- the mean development R2 over trend and sine/cosine phase tasks `{0,3,4}` is
  above zero with at least two positive seeds;
- the mean development R2 over lag/coupling tasks `{7,8}` is above zero with at
  least two positive seeds;
- under a fixed simultaneous cyclic channel permutation of the canonical
  field and target identity, permutation-restored L has cosine at least `0.50`
  and normalized RMSE at most `1.0` in every seed.

Shuffled order and shuffled continuous controls are reported and must remain at
chance/baseline: shuffled reversal upper 95% at most `0.60`; shuffled protected
task macro R2 at most `0.05`.

### E. Time/frequency routing without an update

At the selected predictor snapshot, compute online tokenizer-plus-encoder
gradients for L time-target and frequency-target MSE separately on the frozen
96-row diagnostic batch.  No optimizer step follows.  Both domains are
admitted together only if both pass sections B-D and their joint encoder
gradient cosine is at least `-0.05` in every seed.

If time passes and frequency fails any domain gate or the joint cosine gate,
admit a time-only L target for the first IMA-5B treatment.  If time fails, or a
shared mechanical/structural gate fails, close the present JEPA branch.

## IMA-5B — one paired representation A/B, conditional on admission

IMA-5B is forbidden unless IMA-5A emits an admitted joint or time-only L
target.  It contains exactly two new-training arms per seed:

- F0: exact E target with frozen FSPA-4 teacher;
- F1: admitted L target with the same frozen FSPA-4 teacher.

Everything else is paired: M2 masks, rows, seeds, views, initialization,
augmentations, model topology, optimizer, update count, evaluation, and
structured readout.  Run `2 arms x 3 seeds x 512 updates = 3,072` representation
updates.  F0-FSPA's already-cached tau-0.990 result is the separate teacher-
motion contrast; it is not a third new arm.

Use raw coordinate MSE.  At update zero, freeze one scalar F1 loss multiplier
so the median online-encoder gradient norm over the three seeds matches F0.
Do not whiten per dimension.  Report predictor gradients separately.

Development uses the inherited probe battery.  The 8M confirmation is opened
only after a precommitted development verdict passes.  Paired group bootstrap
and all protected family, reversal, shuffle, and geometry gates remain
unchanged.

Verdicts use F1-F0 and F1-FSPA-4 representation macro AULC:

- `mechanism_confirmed`: F1-F0 at least `+0.005`, paired lower bound above
  zero, all three seeds positive, hidden-support fingerprint removed, and
  order/channel damage corrected;
- `objective_made_safe`: F1-FSPA-4 lies in `[-0.005,+0.005]`, F1 is materially
  above F0, and every protected gate passes;
- `representation_rescue`: F1-FSPA-4 at least `+0.005`, paired lower bound
  above zero, all three seeds positive, and every family/geometry gate passes;
- otherwise `support_permitted_teacher_rejected`.

Only `representation_rescue` may reopen a coherent JEPA+MAE treatment.
`objective_made_safe` may justify L as a neutral auxiliary but does not reopen
topology expansion.  `all_tokens` remains the explicit serving-readout
rollback, but serving-readout policy is outside IMA-5.

## Completion evidence

IMA-5 is complete only when the protocol and source are checksummed, the CPU
self-test passes, the authoritative IMA-5A log and checksum exist, all custody
and zero-update receipts pass, and either:

- IMA-5A emits a precommitted branch-closing decision; or
- an admitted IMA-5B run completes, opens confirmation only at its stop gate,
  and records one of the four representation verdicts above.

