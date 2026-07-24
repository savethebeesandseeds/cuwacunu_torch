# Representation/readout diagnostic preregistration

Status: fixed on 2026-07-16 after the preregistered raw AR(24) gate passed,
but before completion or downstream evaluation of the v2 representation,
before running the raw-history production-MDN isolation, and before any v2
MDN or affine-readout result existed. Intermediate representation training
health fields (loss, latent spread, and effective rank) had been inspected;
no forecast label metric from that representation had been computed.

This supplement does not alter the fresh-seed data contract or expose the
final holdout. It fixes the remaining diagnostic choices that the original
fresh-seed preregistration intentionally described only as an evidence
ordering.

## Raw-history production-MDN isolation

The production `ChannelContextMdn` is tested with representation encoding
absent. Each example contains the preceding 30 completed daily close returns
for all three base/quote edges. Anchor zero is excluded because its complete
lag history is unavailable, giving a fitting range of `[1,2496)`.

Two arms use identical examples and seed:

1. `K=1` with NLL only. This controls basic MDN likelihood capability.
2. `K=3` with the current direct-return objective: regression/direction/rank
   weights `100/5/5`, target scale `36`, 800 direct-head-only warmup steps,
   and `edge_embedding_per_edge` identity.

The K=3 arm reports both mixture-expectation and direct-head forecasts. A
closed-form linear raw-history control is reported from the same examples.
No hyperparameter is selected on certified development. The primary
capability claim belongs to the unchanged K=3 direct output. It passes only
if both validation and certified development attain direction and pairwise
rank accuracy of at least `0.95`, correlation of at least `0.95`, and RMSE no
greater than `0.25` target RMS.

## Frozen representation affine decoder

One production run captures the frozen post-representation MDN context probe
for anchors `[0,3264)`. Only `[0,2496)` is eligible for fitting,
`[2560,2816)` for ridge selection, and `[2880,3264)` for one unchanged
confirmation. Purge anchors are captured for a contiguous integrity record
but excluded from every fit and metric.

The primary decoder uses the first 384 probe values: base representation,
quote representation, and their difference, each of width 128. The 16
learned edge-identity values are excluded. Features are standardized using
train-core statistics only. One affine coefficient row and bias are fitted
per edge while pooling the three channels, matching the deployable
conditioned-affine readout structure used in the v1 diagnosis.

The fixed ridge grid is:

```text
1e-12, 1e-10, 1e-8, 1e-6, 1e-4, 1e-2
```

For each candidate, weights are solved in float64 from train core only.
Selection is lexicographic on validation: highest direction accuracy, then
highest pairwise-rank accuracy, then highest correlation, then lowest RMSE,
then smallest ridge value. Only the selected candidate is scored on certified
development. No refit occurs.

The decoder shows strong information preservation only if validation and
certified development both meet the original raw-data gate: direction,
pairwise rank, and correlation at least `0.95`, and RMSE at most `0.25`
target RMS. Direction at least `0.80` and rank at least `0.78` is reported as
partial preservation comparable to the v1 affine ceiling, not success on the
fresh-seed target. Anything lower is a representation/interface failure for
this benchmark.

## Production learned readout and localization

The canonical v2 production MDN consumes the single frozen representation
checkpoint and uses the already fixed direct-return objective. Its direct
output is primary; mixture expectation is a diagnostic. The same strong gate
above applies.

Interpretation is fixed as follows:

- Raw production MDN fails: the failure is not localized to representation;
  inspect target plumbing, objective scaling, or optimizer behavior first.
- Raw production MDN succeeds and affine decoder fails: usable sequential
  information was lost in representation or its exposed interface.
- Affine decoder succeeds and learned readout fails: the representation is
  decodable; the learned head/objective/optimization path is faulty.
- Both representation arms succeed but remain below raw history: the
  representation is an upstream accuracy ceiling even if the learned head is
  functioning.
- All arms succeed: v1 failure was primarily benchmark construction or an
  already-corrected implementation defect; final evidence still requires a
  separate exactly-once ledger.

No result in this supplement authorizes reading, scoring, fitting, or tuning
on `[3328,4096)`.
