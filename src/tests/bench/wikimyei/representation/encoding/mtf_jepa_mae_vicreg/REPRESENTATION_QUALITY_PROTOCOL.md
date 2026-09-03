# MTF representation-quality protocol

This protocol measures whether self-supervised training makes the frozen
MTF-JEPA-MAE-VICReg encoder retain and expose more useful sequence information.
Runtime latency, training-loss reduction, and projector VICReg statistics are
not representation-quality endpoints.

## Claim and comparison boundary

The primary surface is the active `all_tokens` serving vector with the exact
production input and architecture shape (`C=3`, `H=30`, `F=9`, `D=32`). The
encoder is frozen before any probe is fitted. The primary quantity is the
paired held-out gain

```text
quality gain = probe score(trained encoder) - probe score(same-seed initialization)
```

The following controls answer different questions:

- full raw history: whether the generated task is learnable at all;
- train-only PCA(raw) to 32 dimensions per channel (96 total): whether the
  learned representation beats a capacity-matched linear compression;
- fixed orthogonal random projection(raw) to 32 dimensions per channel (96
  total): whether it beats a capacity-matched untrained compression;
- tokenizer descriptors: whether information was discarded before the
  transformer;
- ordered pre-pool tokens: diagnostic localization only, because this surface
  is wider than the served representation and is not a fair primary control;
- shuffled probe targets: leakage and false-positive control.

## Data isolation

Use deterministic, independently keyed groups for four non-overlapping roles:

1. self-supervised encoder training;
2. probe fitting;
3. probe/ridge selection;
4. final evaluation.

Standardization and PCA are fitted only on their corresponding training split.
The final split is never used for encoder training, probe fitting, ridge
selection, or threshold selection. Statistical resampling treats a complete
generated group as the independent unit; channels, instruments, ridge values,
and horizons are not independent replicates.

## Sequence-information battery

Use fixed low-capacity ridge or logistic probes, selected on the probe
validation split, for four preregistered families:

1. multiscale state: endpoint, local slope, amplitude/energy, oscillation
   frequency and phase components;
2. temporal order and regime: reversal/order labels, lag identity, change-point
   location and pre/post-regime difference;
3. cross-channel dynamics: lead/lag identity, relative state, and coupling;
4. future dynamics: several causal horizons rather than one aggregate return.

Report normalized held-out R2 for regression, chance-corrected balanced
accuracy for classification, per-family macro scores, and the macro score over
a fixed labeled-sample ladder. A matched small nonlinear probe may diagnose
information that is present but not linearly accessible; it does not replace
the primary frozen linear probe.

Paired nuisance views (small observation noise and declared label-preserving
missingness) and one-factor semantic counterfactuals provide a separate
robustness check. Report nuisance-view retrieval and the probability that a
semantic edit moves an embedding farther than a nuisance edit.

## Collapse guards

Compute the eigenspectrum of the centered served activations themselves, not
the VICReg projector and not only marginal coordinate variances. At minimum
report effective-rank fraction, participation-rank fraction, largest-eigenvalue
share, active-dimension fraction, and centered pairwise cosine. These are
necessary collapse guards; good geometry alone is not evidence that sequence
information survived.

## Decision rule

The fast development tier uses three fixed model seeds and a shortened training
budget. It may expose a failure but cannot qualify the production training
recipe. The release tier uses the exact active architecture and canonical
training budget with at least five model seeds.

Declare `representation_improved` only when all preregistered release gates
hold:

- the lower 95% paired group-bootstrap bound for the trained macro
  sample-efficiency score exceeds every equal-width control by at least 0.02;
- at least three of four task families have a positive lower confidence bound,
  and no family has an upper bound below -0.02;
- semantic-versus-nuisance distance AUROC has lower bound at least 0.75 and its
  trained-minus-initialization lower bound is positive;
- every channel has effective-rank fraction at least 0.25, largest-eigenvalue
  share at most 0.80, and active-dimension fraction at least 0.75;
- at least four of five model seeds have positive primary gain;
- raw/task-validity controls pass and shuffled-target controls remain at
  chance.

Otherwise report the narrowest supported diagnosis: tokenizer information
loss, no learned improvement, pooling bottleneck, nonlinear-only information,
robustness failure, or collapse. Never convert objective reduction or a wider
pre-pool probe result into a representation-quality pass.
