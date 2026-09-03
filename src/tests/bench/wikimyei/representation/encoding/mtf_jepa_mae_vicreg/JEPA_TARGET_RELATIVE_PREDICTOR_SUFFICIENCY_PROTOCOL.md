# IMA-4B — Target-Relative Predictor Sufficiency Gate

## Decision question

Can a narrow target-slot-aware predictor recover the small amount of frozen
teacher-latent variation that IMA-4A found to be accessible from legal M2
context, without changing the representation, teacher, masks, data, or
augmentations?

IMA-4A is settled evidence, not a study to repeat.  Its support-separated M2
result was:

- current predictor mean target-identity-centred `R² = -1.412862`;
- categorical target-slot × canonical-context-field oracle C mean
  `R² = +0.04159398` (`0.0739974`, `0.0421979`, `0.00858663` by seed);
- decision `categorical_target_slot_interaction_bottleneck`.

Oracle C is a frozen linear diagnostic with one context-field map per target
slot.  It is a lower bound demonstrated by that diagnostic, not an
information-theoretic ceiling.  Its absolute signal is weak, so recovering it
would establish predictor sufficiency but would not by itself establish a
useful representation repair.

## Frozen boundary and custody

Use the FSPA-4 archives for seeds `17`, `31`, and `47` and only the
`support_separated_pair_v1` (M2) mask contract.  Freeze and exclude from every
optimizer:

- online and target tokenizers;
- online and EMA target encoders;
- all augmentations and normalization;
- data rows, group splits, target/context masks, and mask RNG;
- target count 2, context count 54, latent width 32, and 72 canonical slots.

Use the exact IMA-4A group-disjoint splits: 256 fit groups, 128 validation
groups, and 256 locked development groups.  Keep both targets from a group
together.  Capture each frozen context field, mask, target metadata, target
slot, teacher target, and group ID once per seed.  Retain detached tensors and
feed the identical retained objects to both predictor arms.

Before training, require the sealed IMA-4A log SHA-256
`eebd1e59167aad75bdfce69f5176ee4f4e040400181133e5f8f66dca3afe7606`,
`ima4a.mechanics_pass=true`, and the recorded categorical-bottleneck decision.
Require the existing IMA-3/OCA archive, cache, configuration, schedule, and
receipt pins.  Recompute the three frozen current-predictor M2 development
scores and require agreement with IMA-4A to `2e-6`.

Snapshot the complete frozen model state before capture.  At the end require
bit-exact state equality, no frozen parameter gradients, zero encoder or
representation optimizer steps, zero EMA updates, and unchanged capture
hashes.

## Matched predictor A/B

Both standalone arms reproduce the production predictor and copy every common
parameter from the same seed archive.  Both add the same zero-initialized
`Embedding(72, 32)` table, giving 11,744 common plus 2,304 new parameters:
14,048 trainable predictor parameters per arm.

For selected target slot `r`, let `e_r` be its embedding and `q0` the current
continuous-metadata query.  Both arms execute the same operations:

```text
q_attention = q0 + rho * e_r
attended    = MHA(q_attention, Wk(context), Wv(context), context_mask)
h           = attended + q0
prediction  = predictor_MLP(h) + (1 - rho) * e_r
```

- `TRUE_SLOT_ATTENTION`, `rho=1`: categorical target identity can change
  which legal context tokens are attended to, but cannot act as a direct
  output bias.
- `SLOT_BIAS_CONTROL`, `rho=0`: the same active slot table can learn only a
  context-independent target-slot bias; the production predictor still
  adapts normally.

This control matches topology, parameters, active table rows, optimizer state,
tensor shapes, and arithmetic.  Treatment-minus-control therefore asks whether
target identity must interact with context, rather than whether extra
parameters or predictor-only retraining help.  At step zero, both arms must be
bit-exact with one another and within `2e-6` of the frozen production
predictions.

No production source or checkpoint is modified by this gate.

## Fixed training and selection

- Objective: coordinate MSE against the captured frozen EMA target.
- Optimizer: fresh Adam, learning rate `1e-3`, global gradient clip `5`.
- Budget: exactly 512 updates per arm; no sweep and no arm-specific stopping.
- Batch: 32 groups / 64 target rows, deterministic group permutation.
- Interleave arms on every step.  Reset paired dropout RNG before each arm.
- Validate at steps `0, 64, 128, 192, 256, 320, 384, 448, 512`.
- Select the lowest validation NMSE independently per arm; earliest wins ties.
- Restore each selected checkpoint and open development exactly once.

All common inputs, group rows, target rows, optimizer-step counts, validation
times, and RNG receipts must match between arms.  Do not extend the budget in
this protocol: if the final validation edge is still improving by at least
`0.005 R²` over step 384, report `compute_censored` and make no scientific
routing claim.

## Metrics and uncertainty

Use IMA-4A's target-identity-centred metric exactly:

```text
NMSE = sum ||prediction - target||²
       / sum ||target - mean(target | canonical target slot)||²
R²   = 1 - NMSE
```

Report per seed and three-seed means for current, control, treatment, and
sealed C.  Primary contrasts are `TRUE_SLOT_ATTENTION - SLOT_BIAS_CONTROL` and
`C - TRUE_SLOT_ATTENTION`.

Use 4,096 deterministic paired group bootstrap resamples, moving both targets
together and keeping selected checkpoints fixed.  Report percentile 95%
intervals, all three seed signs, and leave-one-seed-out means.  The bootstrap
quantifies held-out-group uncertainty; it does not pretend that three archived
training seeds characterize all seed variability.

## Fail-closed mechanics

Stop with `invalid_mechanics` and make no scientific conclusion if any of the
following occurs:

- custody, source/log hash, cache, mask, target, group, capture-hash, or frozen
  current-score reproduction fails;
- unequal parameter counts, rows, update counts, validation schedule, or RNG;
- non-equal step-zero arm outputs or failure to reproduce the frozen predictor;
- a frozen model mutation, frozen gradient, EMA update, or representation
  optimizer step;
- nonfinite loss, gradient, parameter, prediction, or metric;
- no predictor parameter update, or failure of the synthetic interaction
  self-test;
- final-edge improvement meets the compute-censor threshold.

## Precommitted verdicts

`TRUE_SLOT_ATTENTION` **reaches C** only if:

- its development `R²` is positive in all three seeds;
- mean `C - TRUE_SLOT_ATTENTION <= 0.01`; and
- the one-sided 95% upper bound of `C - TRUE_SLOT_ATTENTION` is below `0.02`.

The target-relative interaction is additionally **causal and material** only
if:

- mean `TRUE_SLOT_ATTENTION - SLOT_BIAS_CONTROL >= 0.02`;
- all three seed signs are positive; and
- the paired 95% interval lower bound is above zero.

Route exactly one conclusion:

1. `target_relative_predictor_sufficient`: treatment reaches C and materially
   beats control.  Lock this predictor mechanism and next run one separate,
   bounded representation-utility gate.
2. `predictor_reoptimization_sufficient_target_identity_not_confirmed`: both
   reach C without a material treatment advantage.
3. `target_relative_query_helpful_but_insufficient`: treatment materially
   beats control but does not reach C.  Next test one explicit low-rank
   target-slot × context-slot attention bias, predictor-only.
4. `narrow_target_relative_predictor_insufficient`: treatment does not
   materially beat control and remains below C.  Do not blame the frozen
   encoder; inspect a richer predictor or redesign the teacher target toward a
   support-permitted abstraction.
5. `linear_C_underestimated_predictability`: treatment materially exceeds C;
   reclassify C as a weak linear lower bound.

No result activates a production change automatically.  No result begins
augmentation attribution.  The representation/teacher boundary remains fixed
until this predictor question is resolved.
