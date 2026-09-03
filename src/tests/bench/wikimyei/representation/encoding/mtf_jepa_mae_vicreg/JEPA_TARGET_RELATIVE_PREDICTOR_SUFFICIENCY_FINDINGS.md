# IMA-4B — Target-Relative Predictor Sufficiency Findings

## Plain-language result

The narrow target-aware predictor did **not** recover the context signal found
by IMA-4A.  It was consistently worse than its matched control.

This experiment does not say that the frozen encoder is broken.  The encoder
and teacher never changed.  It says something more specific: adding the
canonical target-slot identity to the current predictor's attention query is
not the missing repair.

The accepted decision is:

```text
narrow_target_relative_predictor_insufficient
```

No production change is authorized.

## What was compared

Both test predictors:

- started from the exact archived production predictor;
- had 14,048 trainable parameters;
- saw the same retained FSPA-4 context tensors and frozen EMA targets;
- used the legal support-separated M2 masks;
- received identical group batches, seeds, dropout RNG, optimizer, clipping,
  validation times, and 1,024-update budget;
- left tokenizer, encoder, teacher, masks, augmentations, and data untouched.

The only difference was how the same 72 × 32 target-slot embedding was used:

- `true_slot_attention`: target identity altered the attention query and could
  therefore change which context tokens were used;
- `slot_bias_control`: target identity could only learn a context-independent
  output bias.

Both were exactly equal at update zero.  Treatment minus control therefore
isolates target-relative context interaction rather than extra parameters or
predictor-only retraining.

## Authoritative numbers

All scores are held-out, target-identity-centred `R²`; zero is the
target-slot-specific mean baseline.

| Predictor | Seed 17 | Seed 31 | Seed 47 | Mean |
|---|---:|---:|---:|---:|
| archived current predictor | -1.400168 | -1.556294 | -1.282124 | -1.412862 |
| slot-bias control | -0.107202 | -0.168020 | -0.108235 | -0.127819 |
| target-slot attention | -0.163011 | -0.183087 | -0.139207 | -0.161768 |
| sealed linear C reference | +0.073997 | +0.042198 | +0.008587 | +0.041594 |

The primary treatment-minus-control contrast was:

```text
mean = -0.0339496 R²
paired 95% interval = [-0.0666625, -0.00877385]
seed signs = negative, negative, negative
```

The target-attention candidate was below C by:

```text
mean C - treatment = +0.203362 R²
paired 95% interval = [+0.176030, +0.271442]
```

Therefore:

- target-slot attention did not reach C;
- it did not beat the matched control;
- its disadvantage was directionally consistent across all three seeds and
  excluded zero in the paired group bootstrap;
- even the much-improved control remained below the target-specific mean
  baseline on held-out groups.

## Optimization closure

The first 512-update execution was correctly rejected as `compute_censored`
because the original censor compared the final point with a transient step-384
dip.  That attempt is preserved, not treated as evidence:

```text
.build/tests/representation_ima4b_v1_attempt1_compute_censored.log
SHA-256 627240eb9e5560b0cd49e9b7b9877b48496e0ca9637f73141fee93046b16d7cf
```

Amendment A1 reran both arms from the frozen initialization for 1,024 updates,
with no hyperparameter change or checkpoint reuse.  The selected checkpoints
remained early:

| Seed | Target attention | Bias control |
|---|---:|---:|
| 17 | 320 | 320 |
| 31 | 448 | 448 |
| 47 | 320 | 320 |

Every later checkpoint remained worse than its selected checkpoint.  From step
512 through step 1,024, validation performance then deteriorated further in
every arm.  No selected model touched the final boundary, so the amended
compute-censor gate passed.  More updates at the same settings are not a
credible rescue.

Independent review then found a dormant checkpoint-index implementation bug in
v1a1: a post-512 selected step would have been mapped to the wrong validation
array entry.  No observed selection used that path, so v1a1's scientific lines
were unaffected, but it was superseded rather than accepted.  Amendment A2
stored the validation index explicitly and reran the complete experiment from
the frozen initialization.  The final v1a2 scientific lines reproduced v1a1
exactly.  The superseded log remains available at:

```text
.build/tests/representation_ima4b_v1a1_superseded_selection_index.log
SHA-256 106e77b544952ee12f39aa378ea7ce18caab7489c070832669371ed3478ac5a7
```

## Mechanical validity

For every seed, all of the following passed:

- sealed IMA-4A log/source/protocol and IMA-3/header custody;
- FSPA-4 archive and IMA-3 cache validation;
- exact current-predictor and C-score reproduction;
- production-component equivalence;
- identical treatment/control initialization and optimizer layout;
- paired RNG and identical data schedule;
- finite losses, gradients, parameters, predictions, and metrics;
- exactly 1,024 Adam updates per predictor arm;
- unchanged capture hashes;
- bit-exact frozen model state and no frozen gradients;
- zero representation optimizer steps and zero EMA updates.

The CPU mechanism self-test also passed.

## What we learned

### 1. Predictor adaptation plus slot bias helps greatly, but not enough

The control moved from mean `R²=-1.412862` to `-0.127819` while the encoder and
teacher stayed fixed.  That control combined ordinary predictor reoptimization
with a learned categorical output-bias table, so this experiment cannot assign
the gain between those two ingredients.  It does show that the old error was
not caused by absence of information in the encoder alone.

However, negative held-out `R²` means the retrained control still could not
beat a target-slot-specific mean.  This is improvement, not successful target
prediction.

### 2. C did not validate a simple target-query embedding

IMA-4A's C diagnostic assigns a distinct linear map of the complete canonical
context field to each target slot.  The tested candidate is much narrower: it
uses target identity only to modify a shared attention query, while context
keys/values still have no explicit canonical source-slot identity.

The negative treatment-minus-control result shows that the useful part of C is
not reproduced by this query mechanism.  It does **not** prove that all richer
target × context-slot predictors would fail.

### 3. The recoverable signal remains small

C explains only about 4.16% of held-out target-centred variation on average.
Even if a richer predictor reaches it, that may still be too weak to protect or
improve sequence representation during JEPA training.  A later utility gate
must distinguish “predictable at all” from “useful enough to train against.”

## Recommended next boundary

If one final predictor-only architecture test is desired, make it
**IMA-4C — Canonical Target × Context-Slot Interaction Sufficiency Gate**:

- keep the exact frozen captures and all IMA-4B custody;
- add a low-rank categorical target-slot × canonical context-slot attention
  bias, so both sides of the interaction exposed by C are explicit;
- compare it with a parameter- and compute-matched shuffled-slot control;
- require it to reach C under the same held-out metric;
- stop the predictor-capacity line permanently if it fails.

Do not reopen augmentation attribution yet.  If IMA-4C fails—or if it reaches C
but the recovered ~4% signal cannot improve a bounded representation-utility
test—the more credible repair is to redesign the EMA teacher target into a
support-permitted abstraction rather than continue enlarging the predictor.

## Evidence custody

- Base protocol SHA-256:
  `571120d44de1e1c3843ff440a14a43b27d5157d1505f691776842c3f46827b95`
- Amendment A1 SHA-256:
  `6f9879ccd1b862eeca9515b5628b09fa028e56e071f60dbb190e9c6260a236c3`
- Amendment A2 SHA-256:
  `5d766e77947c88b370309e281276c05b640609c3ad42b6525240de64bb36aa00`
- Harness source SHA-256:
  `53a58a72326b5c3ae070f50f36bc78cc7863d1a94d08ac55107337790e634c76`
- Authoritative log SHA-256:
  `9ce1be7b00bd9338cf7d640502db3d0d7a9731e1fec163b24158a437525f9f8c`
