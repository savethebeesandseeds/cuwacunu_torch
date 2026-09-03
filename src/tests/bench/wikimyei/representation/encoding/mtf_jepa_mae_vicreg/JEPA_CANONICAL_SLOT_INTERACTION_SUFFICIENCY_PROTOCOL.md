# IMA-4C — Canonical Target × Context-Slot Interaction Sufficiency Gate

## Decision question

IMA-4A proved that the frozen M2 context contains a small but real amount of
teacher-target information when a linear diagnostic is allowed a different
canonical-context-field map for every target slot. IMA-4B then showed that
neither a target-slot query nor a target-slot output bias is sufficient for the
production predictor to recover that information.

IMA-4C asks the next narrow question:

> Is the missing mechanism an explicit interaction between the selected target
> slot and the canonical source slot of each legal context token?

This is a predictor-only sufficiency test. It does not retrain, repair, or
evaluate the encoder; repeat a representation probe; alter the teacher target;
or reopen augmentation attribution.

## Settled evidence

The final IMA-4B v1a2 result is evidence to consume, not an experiment to
repeat:

- production predictor mean development `R² = -1.41286194248`;
- slot-output-bias control mean development `R² = -0.127818914349`;
- target-slot-query treatment mean development `R² = -0.161768480115`;
- sealed diagnostic C mean development `R² = +0.0415939752187`;
- treatment minus control `-0.0339495657659`, negative in every seed;
- decision `narrow_target_relative_predictor_insufficient`;
- zero representation optimizer steps and zero EMA updates.

Pin and verify these immutable inputs before opening IMA-4C:

- final IMA-4B source SHA-256
  `53a58a72326b5c3ae070f50f36bc78cc7863d1a94d08ac55107337790e634c76`;
- final IMA-4B authoritative-log SHA-256
  `9ce1be7b00bd9338cf7d640502db3d0d7a9731e1fec163b24158a437525f9f8c`;
- IMA-4B base-protocol SHA-256
  `571120d44de1e1c3843ff440a14a43b27d5157d1505f691776842c3f46827b95`;
- IMA-4B amendment A1 SHA-256
  `6f9879ccd1b862eeca9515b5628b09fa028e56e071f60dbb190e9c6260a236c3`;
- IMA-4B amendment A2 SHA-256
  `5d766e77947c88b370309e281276c05b640609c3ad42b6525240de64bb36aa00`.

The log must also contain the final v1a2 schema, three passing seeds,
`ima4b.mechanics_pass=true`, `ima4b.compute_censored=false`, the decision
above, zero representation/EMA updates, and completed execution. Superseded
v1/v1a1 logs are not evidence for this gate.

## Frozen representation boundary

Use the same FSPA-4 archives for seeds `17`, `31`, and `47` and only the
`support_separated_pair_v1` (M2) masking contract. Freeze and exclude from
every optimizer:

- online and target tokenizers;
- online encoder and EMA target encoder;
- augmentations, normalization, data rows, and mask generation;
- context and target masks, canonical slot order, and metadata;
- target count 2, context count 54, token count 72, and latent width 32.

Use exactly the established group-disjoint splits: 256 fit groups, 128
validation groups, and 256 locked development groups. Both targets from a
group always remain together.

For each seed, deterministically recapture each split once because IMA-4B did
not serialize its captures. Retain detached CPU tensors for context latents,
context mask, target metadata, target slot, EMA target, production prediction,
and absolute group ID. Feed the same retained objects to both IMA-4C arms.

Require the established IMA-3/OCA archives, caches, source/configuration pins,
row schedules, and receipts. Reproduce the three frozen current-predictor and
diagnostic-C development scores within `2e-6`. Snapshot the full frozen model
state before capture and require bit-exact equality afterward, no frozen
gradients, zero representation optimizer steps, zero encoder updates, and zero
EMA updates.

## Minimal interaction mechanism

Both arms start from the same archived production predictor, copy all 11,744
common predictor parameters exactly, and add the same two adapters:

1. a target-slot output table `E[72,32]`, initialized to zero; and
2. a rank-4, per-head canonical pair bias with factors
   `U[4,72,4]` and `V[4,72,4]`.

The target-slot output table retains the stronger IMA-4B control baseline in
both arms. It is not the treatment. The only treatment variable is whether the
pair bias receives stable canonical source identity.

For row `b`, head `h`, selected target slot `r`, actual context position `s`,
and source-label map `sigma_b`, add

```text
pair_bias[b,h,r,s]
    = dot(U[h,r,:], V[h,sigma_b(s),:]) / sqrt(4)

attention_logits
    = q(metadata) K(context)^T / sqrt(8) + pair_bias
```

Then apply the production Boolean mask, masked fill, softmax, masked
renormalization, value aggregation, residual, predictor MLP, and dropout path
unchanged. Add `E[r]` only to the final prediction. Never permute context
latents, context order, keys, values, actual masks, targets, or metadata.

The per-arm trainable count is fixed at:

```text
11,744 production predictor parameters
+2,304 target-slot output parameters
+2,304 low-rank pair parameters
=16,352 parameters
```

Initialize `E=0` and `U=0`. Initialize `V` once from a deterministic, small,
nonzero distribution, identically for both arms. This makes both arms exactly
equal to one another at step zero and within `2e-6` of production while giving
`U` a live first-step gradient. Zeroing both factors is forbidden because it
would make the bilinear adapter locally dead.

## Equal-compute causal arms

- `TRUE_CANONICAL_PAIR`: `sigma_b(s)=s`; the pair bias receives the true
  canonical source-slot label.
- `SHUFFLED_PAIR_CONTROL`: only the pair adapter's source labels are
  deterministically permuted. The actual context and mask are unchanged.

For the control, construct one complete 72-slot permutation independently for
each experiment seed and absolute group ID. Construct separate Sattolo
derangements within that group's 54 active and 18 inactive source positions.
This preserves exactly the active and inactive factor-row multisets while
destroying stable source-slot alignment. Use the same map for both targets in
the group, and reuse that group's map unchanged every time the group is seen.
The map is independent of epoch, split role, candidate metrics, and optimizer
state. This avoids giving the control a train/evaluation distribution shift.

A single fixed global permutation is invalid: learned `V` could invert it as a
mere relabeling. Permuting active labels into inactive labels is also invalid:
it changes factor-row exposure instead of only source identity.

Precompute, retain, and hash every map. No permutation RNG may run inside a
forward pass. The treatment must gather through explicit identity maps so both
arms perform the same gathers, contractions, additions, masks, and attention
operations. Require identical parameter counts, active rows, optimizer layout,
batches, target rows, dropout seeds, update counts, and checkpoint schedule.

## Fixed training and selection

- Loss: coordinate MSE against the captured frozen EMA target.
- Optimizer: fresh Adam, learning rate `1e-3`, global gradient clip `5`.
- Batch: 32 groups / 64 target rows.
- Budget: exactly 1,024 interleaved predictor updates per arm.
- Validation checkpoints: `0, 64, 128, 192, 256, 320, 384, 448, 512, 640,
  768, 896, 1024`.
- Selection: independently choose the lowest validation NMSE; earliest wins
  exact ties. Store and restore the selected validation index explicitly.
- After both choices are locked, open development exactly once.

At every update use the same fit-group rows and paired dropout RNG for both
arms. Each retained group's treatment identity map and control derangement are
immutable. There is no sweep, arm-specific stopping, checkpoint reuse,
continuation, or additional training if the result disappoints.

Report `compute_censored` only if an arm selects step 1024 and its validation
`R²(1024) - R²(896) >= 0.005`. A censored experiment makes no scientific
routing claim and receives no automatic budget extension.

## Mechanical self-tests

Before the audit, a standalone CPU self-test must prove:

- all map tensors are full permutations with the requested active/inactive
  preservation, treatment identity, control derangement, group pairing,
  repeated-use stability, group variation, and deterministic hashes;
- step-zero arms are bit-identical and reproduce a copied production
  predictor within `2e-6`;
- `U` has a nonzero finite first-step gradient and both pair factors have
  changed by a selected nonzero checkpoint;
- the local bias-aware attention path equals production attention exactly when
  the pair bias is zero;
- a synthetic target × canonical-source interaction is learnable by treatment
  and not by the shuffled control under the same retained inputs;
- capture hashes, row schedules, validation indices, snapshot restore,
  parameter counts, optimizer states, RNG receipts, and all finiteness checks
  fail closed.

## Metrics and uncertainty

Use the unchanged target-identity-centred metric:

```text
NMSE = sum ||prediction - target||²
       / sum ||target - mean(target | canonical target slot)||²
R²   = 1 - NMSE
```

Report each seed and the three-seed mean for production current, shuffled
control, true-canonical treatment, and sealed diagnostic C. Report the primary
contrasts:

- `TRUE_CANONICAL_PAIR - SHUFFLED_PAIR_CONTROL`;
- `C - TRUE_CANONICAL_PAIR`;
- `C - SHUFFLED_PAIR_CONTROL`;
- `TRUE_CANONICAL_PAIR - C`.

Use 4,096 deterministic paired group bootstraps. Move both targets together,
keep selected checkpoints and predictions fixed, score within each seed's
target-slot coordinate system, and then average contrasts across seeds. Report
two-sided percentile 95% intervals, one-sided 95% upper bounds for both
`C - treatment` and `C - control`, every seed sign, and leave-one-seed-out
means.

## Fail-closed gates

Stop with `invalid_mechanics` and make no scientific conclusion if any
custody, hash, capture, parity, permutation, frozen-state, gradient, optimizer,
schedule, RNG, finiteness, or self-test check fails. A `compute_censored`
result is also non-routable.

The treatment **reaches C** only when all are true:

- treatment development `R²` is positive in all three seeds;
- mean `C - treatment <= 0.01`; and
- the one-sided 95% upper bound of `C - treatment` is below `0.02`.

Apply the same reach definition to the shuffled control, using its separately
reported `C - control` paired bootstrap and one-sided 95% upper bound.

The canonical interaction is **causal and material** only when all are true:

- mean `treatment - control >= 0.02`;
- all three seed contrasts are positive; and
- the paired two-sided 95% interval lower bound is above zero.

Treatment **materially exceeds C** only when all are true:

- mean `treatment - C >= 0.02`;
- all three seed contrasts are positive; and
- the paired two-sided 95% interval lower bound is above zero.

## Precommitted routing

Route exactly one conclusion after valid, uncensored mechanics:

1. `linear_C_underestimated_predictability`: treatment materially exceeds C.
   Stop predictor expansion and run one separate bounded representation-utility
   gate.
2. `canonical_pair_interaction_sufficient`: treatment reaches C and materially
   beats control. Lock this mechanism, stop predictor expansion, and run the
   same utility gate.
3. `predictor_sufficient_pair_interaction_not_confirmed`: treatment reaches C
   without a material advantage. Stable canonical identity is not confirmed,
   but only the canonical treatment is a meaningful deployable candidate; stop
   expansion and take treatment to the utility gate.
4. `canonical_identity_unnecessary_mechanism_unresolved`: treatment fails to
   reach C but the shuffled control reaches it. The arbitrary group-specific
   control is not deployable and must not proceed to utility. Stop here and
   report the predictor mechanism unresolved; a clean pair-disabled reference
   would require a separately precommitted experiment.
5. `canonical_pair_interaction_helpful_but_insufficient`: neither arm reaches
   C, but treatment materially beats control. Permanently stop narrow
   predictor-capacity experiments and redesign the EMA teacher target toward a
   support-permitted abstraction.
6. `predictor_capacity_exhausted_teacher_redesign`: neither arm reaches C and
   treatment is not materially better. Permanently stop narrow
   predictor-capacity experiments and redesign the teacher target.

Any arm that reaches C still proves only target-prediction sufficiency. If the
single later representation-utility gate fails, route directly to teacher
target redesign. No IMA-4C result automatically changes production, and outer
augmentation attribution remains closed until this representation/teacher
boundary is resolved and held fixed.
