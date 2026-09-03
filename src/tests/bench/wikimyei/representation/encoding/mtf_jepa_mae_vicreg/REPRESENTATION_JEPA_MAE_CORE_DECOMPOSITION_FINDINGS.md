# JEPA/MAE Core Decomposition (JMCD-1) findings

Date completed: 2026-08-26

Terminal classification:
`core_component_marginal_harm_not_localized`

## Human conclusion

The isolated representation module did not learn a better clean sequence
representation from either JEPA or MAE at the frozen 32-update horizon. JEPA
alone and MAE alone each produced a statistically clear loss of clean macro
probe AULC relative to the unchanged factorial-null encoder.

The two objectives are not damaging because of a harmful JEPA-by-MAE
interaction. The interaction residual points strongly in the opposite
direction: using both objectives together partially cancels the damage seen
when either objective trains alone. Consequently, removing JEPA or removing
MAE from the combined objective is not a rescue and is not supported as the
next repair.

The formal classification says that the combined arm's marginal harm was not
localized to one conditionally removable branch. It does **not** say that the
singleton objectives were harmless. Both singletons were harmful versus the
unchanged null under the primary AULC endpoint.

Most importantly, the unchanged served encoder itself remained well below the
equal-width raw-history control. That directs the next investigation toward
where the representation surface loses accessible sequence information:
token construction, encoder processing, or the current all-token serving mean.

## Primary representation endpoint

All values are fixed-seed-mean clean macro probe AULC over labeled sample sizes
`32,64,128,256`.

| arm | step 0 | step 16 | step 32 | step 32 minus null |
| --- | ---: | ---: | ---: | ---: |
| core-objective null | 0.5192625 | 0.5192625 | 0.5192625 | 0.0000000 |
| JEPA + MAE | 0.5192625 | 0.5122693 | 0.5155433 | -0.0037192 |
| JEPA only | 0.5192625 | 0.5122617 | 0.5102738 | -0.0089887 |
| MAE only | 0.5192625 | 0.5137818 | 0.5110813 | -0.0081812 |
| equal-width raw control | 0.6022866 | n/a | n/a | +0.0830241 versus null |

The raw control exceeds the final combined representation by `0.0867433`
AULC. This is not a small objective-selection effect: accessible information
or sample-efficient geometry is already weaker at the served representation
surface before these 32 training updates improve it.

## Frozen paired contrasts

Intervals are the preregistered 512-replicate held-out-group bootstrap with
common resampling across arms. They describe group uncertainty for the fixed
three model seeds, not training-seed uncertainty.

| oriented contrast | point | paired 95% interval | positive seeds | result |
| --- | ---: | ---: | ---: | --- |
| JEPA - null | -0.0089887 | [-0.0145504, -0.0037221] | 1/3 | JEPA alone is worse |
| MAE - null | -0.0081812 | [-0.0143306, -0.0014262] | 1/3 | MAE alone is worse |
| MAE - combined (remove JEPA) | -0.0044620 | [-0.0096404, +0.0005528] | 1/3 | no JEPA-removal rescue |
| JEPA - combined (remove MAE) | -0.0052695 | [-0.0110751, +0.0001980] | 1/3 | no MAE-removal rescue |
| combined - null | -0.0037192 | [-0.0092747, +0.0021201] | 2/3 | point decline, not resolved from zero |
| JEPA + MAE - null - combined | -0.0134507 | [-0.0194416, -0.0073943] | 0/3 | opposite of harmful interaction |

The combined-minus-null mean is negative because seed 31 declined by
`-0.0180438`; seeds 17 and 47 improved by `+0.0054052` and `+0.0014809`.
Therefore the combined decline is not a seed-general claim.

The factorial residual was negative in every seed. Under its frozen
orientation, positive would mean harmful interaction. The observed negative
interval means the combined objective performs better than the additive
expectation from the two harmful singleton trajectories.

## Geometry and safety

At step 32, the null retained effective-rank ratio `0.1209`, participation-rank
ratio `0.0832`, and worst-channel top-eigenvalue share `0.6537`. The combined
arm moved these to `0.0983`, `0.0678`, and `0.7513`. JEPA-only retained more
rank than the combined arm (`0.1100`, `0.0761`, `0.7208`) but still failed the
frozen top-share retention and gap-closure clauses. MAE-only was worse
(`0.0924`, `0.0625`, `0.7691`) and failed every rank/gap repair clause.

All arms retained an active-dimension fraction of `1.0`. The failure is thus
anisotropic concentration and poorer sample-efficient decoding, not literal
dead dimensions. Neither singleton passed safety or replacement support.

## Mechanical and historical validity

- The pure gate fixture and historical representation contracts passed.
- The frozen protocol SHA-256 is
  `7339a313b003f5c0e53a5b413aaa674ad15f334b923bf010c535011cc585b668`.
- The CUDA preflight took zero optimizer steps and passed all four arms.
- Every seed paired exact initialization, clean rows, input hashes, masks, weak
  views, and pre/post CPU/CUDA generator states.
- Branch-specific parameters changed only where their optimizer coefficient
  was active. The null's non-target trainable parameters, served output,
  probes, and geometry remained exact to step 0.
- All checkpoint gradient decompositions and state-restoration checks passed;
  no clipping or non-finite update occurred.
- The post-run neutral-reference audit selected exactly 2,046 unique accepted
  JEPA/MAE keys, found no missing/extra/duplicate key, and reproduced both the
  frozen key-set and key/value hashes exactly.
- The authoritative log is 1,050,475 bytes with SHA-256
  `269665e337730d5d3085848904d2aa6217fdbc14aa65e78393723567d818f1bd`.

## What this changes

1. Do not advance JEPA-only or MAE-only as a repair. Each is worse than doing
   no core-objective update, and each fails geometry safety.
2. Do not blame a harmful JEPA/MAE interaction. The measured interaction is
   reliably beneficial relative to the harmful singleton trajectories.
3. Do not return to outer-augmentation tuning as the leading explanation.
   JMCD used clean launcher inputs, while previous matched augmentation arms
   had no material clean-representation effect. Internal weak views remain
   active and are still part of the module, so this does not exonerate their
   semantics independently.
4. Do not treat lower self-supervised loss as representation improvement. The
   decision endpoint is clean sequence-information decoding, where no trained
   core arm beat the unchanged representation.
5. Localize the large raw-to-served gap before designing another objective.

## Recommended next plan: RSSM-1

**Representation Surface Sufficiency Map (RSSM-1)** is a no-optimizer,
module-only localization of where the `0.0830` raw-to-null AULC gap appears.
It should reuse existing frozen data, normalization, model artifacts, and
probe code; it authorizes no new training by itself.

1. Freeze the question, surfaces, split identities, projections, and decision
   rules before reading results. Record source/artifact hashes.
2. Capture, in one deterministic encoder pass, the equal-width raw control,
   tokenizer outputs before the transformer, encoder tokens before serving
   pooling separated by channel/domain/scale, and the current served
   `all_tokens` mean.
3. Evaluate two fair tracks with the same disjoint ridge probes: native-surface
   decoding and a frozen 96-wide orthonormal projection for dimensional
   control. Use the same `32,64,128,256` AULC ladder and four sequence families.
4. Add shuffled-target and repeated-capture identity controls, tokenizer
   collision counts, centered geometry, nuisance sensitivity, and exact
   sample/group pairing. No optimizer or augmentation sweep belongs here.
5. Apply a preregistered localization tree:
   - raw strong, tokenizer weak: token construction loses information;
   - tokenizer strong, encoder pre-pool weak: encoder processing is the loss;
   - pre-pool strong, served mean weak: serving aggregation is the loss;
   - all internal surfaces weak: the architecture/state descriptors need
     redesign before another training-objective experiment.
6. Stop after the map and choose at most one mechanism-specific repair. Any
   training, longer horizon, architecture edit, production change, or
   end-to-end test requires a separately frozen protocol and authorization.

## Authorization

`next_experiment_authorized=false`

`long_run_authorized=false`

`production_or_end_to_end_authorized=false`

The sole JMCD-1 scientific attempt has been consumed. It must not be rerun or
extended.
