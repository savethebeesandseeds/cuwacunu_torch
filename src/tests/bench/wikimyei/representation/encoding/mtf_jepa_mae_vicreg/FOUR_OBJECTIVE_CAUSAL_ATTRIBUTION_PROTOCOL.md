# OCA-1 — Four-Objective Causal Attribution Protocol

Status before execution: **sealed primary design; no OCA-1 quality endpoint
observed**.

## Purpose

Determine, without constructing any downstream model, what TF alignment, JEPA,
MAE, and VICReg do to the served sequence representation:

1. whether each objective is mechanically connected to the served encoder;
2. whether each objective can improve representation quality alone;
3. whether objective interactions are constructive or destructive; and
4. whether any legacy objective improves the already-certified FSPA-4
   representation.

The evidence hierarchy is fixed: compatibility with the certified
representation decides the production recipe; standalone and factorial results
explain mechanism.

## Immutable boundary and controls

Every experiment remains inside:

```text
sequence -> MTF encoder -> structured_cdsb_sparse_v1 -> fixed probes
```

No MDN, graph, observer, policy, execution, checkpoint migration consumer, or
end-to-end model may be constructed. Probe targets are evaluation-only and may
not affect representation training, objective weights, schedules, repair
choices, or checkpoint selection.

Freeze the RMC synthetic rows, normalization, masks, model architecture,
structured readout, three seeds `17,31,47`, Adam `1e-3`, batch size `96`, row
schedule, ridge grid, raw control, reversal probe, shuffle controls, bootstrap
table, and every RMC threshold. Outer augmentation remains neutral with zero
calls. Module-owned JEPA masks and VICReg weak views remain active and replayed
identically across arms so a zero coefficient changes only gradient
contribution, not execution or random draws.

## Phase 0 — certified reference fixture

Reproduce FSPA-4 exactly for each seed: 1,024 frozen-projection teacher updates,
the SSL-only minimal participation spectral repair targeting `0.30`, and 512
served-encoder distillation updates. Require the established teacher, repair,
student mechanics, and RMC development pass.

Save one versioned archive per seed under `.build/tests/oca1/`. Each archive
must contain the complete model state and metadata for seed, certificate id,
FSPA-4 protocol SHA-256, OCA-1 protocol SHA-256, canonical configuration hash,
and readout policy. Load each archive into a freshly constructed model and
require byte-exact parameters, buffers, structured development outputs, masks,
and evaluation predictions. Emit the archive SHA-256. These three archives are
the immutable reference and rollback.

## Phase 1 — wiring and gradient audit

On the same fixed SSL batch and RNG state, decompose TF, JEPA, MAE, and VICReg
gradients at both identical initialization and the certified checkpoint.
Measure raw loss, weighted gradient norms for all trainable, tokenizer,
encoder, served trunk, predictor, MAE decoder, and VICReg head partitions;
all six pairwise served-gradient cosines; summed-gradient reconstruction error;
mask/view replay equality; parameter, EMA, RNG, and optimizer neutrality.

An objective is mechanically disconnected only if its served gradient and
served update are exactly zero under a finite nonzero raw loss. Otherwise it is
connected. Phase 1 makes no representation-quality claim.

## Phases 2–3 — complete equal-budget legacy factorial

Run all `2^4 = 16` subsets of the four legacy objectives from byte-identical
initialization. The enabled coefficients are the existing configured values:

- JEPA `1.0`;
- MAE `0.25`;
- TF alignment `0.10`;
- global VICReg `0.05`, retaining its internal configured component weights.

The zero-objective arm is the unchanged initialization. Every other arm receives
exactly **1,536 updates**, matching the certified recipe's total update budget.
Use gradient clipping at norm `5.0`, as in the production launcher, and report
the unclipped norm and clipping count. Evaluate only after update 1,536; no
intermediate quality endpoint may be inspected or selected.

For every arm require finite losses, gradients, updates, masks, and outputs;
exact row/mask/view schedules across paired arms; exact initialization; expected
parameter-partition activity; and the complete RMC learned-gain, family,
raw-control, reversal, shuffle, and geometry report.

Compute precommitted factorial statistics from the final predictions:

- each objective's main effect, averaging its present-minus-absent contrast
  over the eight matched settings of the other objectives;
- all six two-objective interactions, averaging
  `both - left_only - right_only + neither` over the four settings of the
  remaining objectives;
- leave-one-out contrast `full - full_without_objective`;
- standalone objective versus initialization.

Use the frozen paired row bootstrap and three seeds. Do not infer causality from
raw scalar loss size alone.

## Phase 4 — certified-anchor compatibility challenge

Load each Phase-0 certified checkpoint and run five independent 512-update arms:

- anchor plus TF only;
- anchor plus JEPA only;
- anchor plus MAE only;
- anchor plus VICReg only;
- anchor plus the complete legacy objective.

Each starts from the identical archived anchor, uses a fresh Adam `1e-3`
optimizer, and differs only in enabled objective coefficients. Compare final
representations directly with the pre-update anchor.

An addition qualifies only if all of the following hold on development:

- mean final-minus-anchor AULC at least `+0.005`;
- paired 95% lower bound greater than zero;
- all three seeds improve;
- at least three of four family changes are positive and none below `-0.02`;
- final-minus-raw lower bound at least `-0.01`;
- reversal point/lower/retention, both shuffles, all geometry thresholds, and
  mechanics pass unchanged.

If multiple additions qualify, select the one with greatest mean
final-minus-anchor AULC; ties within `1e-12` resolve in the fixed order TF,
JEPA, MAE, VICReg, full. If none qualifies, the certified FSPA-4 anchor remains
canonical and no confirmation is opened.

## Confirmation and repair control

An OCA-qualified anchor addition opens one new untouched 256-row confirmation
split beginning at synthetic group `5,000,000`, never used by RMC or FSPA.
Apply the identical gate once without retraining or reselection. A confirmation
pass promotes the addition; failure retains the FSPA-4 anchor.

Primary evidence may trigger at most one bounded repair per failed objective,
but no repair is authorized by this primary protocol. Each repair must follow a
specific measured mechanism, be written as a separately checksummed amendment
before its endpoint, and be compared with both the failed objective and the
certified anchor under equal compute. No open-ended weight or checkpoint sweep
is allowed.

## Final component verdicts

Assign every objective exactly one primary verdict:

- `beneficial_to_certified_anchor` — passes the Phase-4 gate and confirmation;
- `standalone_capable_only` — passes standalone RMC but not anchor addition;
- `conditionally_helpful_legacy_interaction` — a positive, confidence-bounded
  factorial interaction or leave-one-out effect, without anchor benefit;
- `neutral_at_certified_boundary` — connected but Phase-4 effect is practically
  below `0.005` without a confidence-bounded harm;
- `harmful_at_certified_boundary` — Phase-4 upper confidence bound is below
  zero or it reproducibly breaks a previously passing safeguard;
- `mechanically_disconnected` — the Phase-1 disconnection condition holds;
- `unresolved` — mechanics or convergence evidence is invalid.

Publish the complete factorial table, gradient map, anchor-challenge table,
verdicts, canonical recipe, checkpoint hashes, explicit rollback, and exact
remaining unknowns in plain language.
