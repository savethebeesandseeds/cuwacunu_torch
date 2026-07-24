# Representation conditional-certification amendment

Status: fixed on 2026-07-16 while the canonical production MDN was still
training on train core. Intermediate train-only checkpoints and running train
metrics existed. No canonical representation validation or certified
forecast, frozen-feature capture, affine result, ablation result, or
certified-development feature row existed, and `[3328,4096)` was unopened.

This amendment resolves the ordering between the canonical localization
procedure and the later conditional ablation preregistration. Earlier capture
documents describe canonical certified capture, but it must not occur
unconditionally: the raw-encoder validation result is the predeclared trigger
for whether representation challengers must be screened first.

## Stage 1: canonical development only

The canonical frozen representation and production MDN first produce exact
train `[0,2496)` and validation `[2560,2816)` probes only. The seed-17
untrained representation control uses the same two ranges. No purge,
certified, or final anchor is captured. The evaluator source and runner are
frozen and hashed before these jobs.

The fixed affine evaluator then runs without a certified input on:

1. the trained raw 96-value encoder interface;
2. the trained post-MDN 384-value dynamic interface; and
3. the untrained raw 96-value encoder control.

Every arm uses train-only standardization and fitting, the declared ridge
grid, the declared validation selection order, and no refit. Main and replay
must be byte-identical. The resulting immutable development receipt records
all validation metrics and the selected ridge for each arm.

## Immutable trigger

The route is a mechanical function of the trained raw-encoder validation
report only:

- If direction, pairwise rank, and correlation are each at least `0.95` and
  RMSE is at most `0.25` target RMS, publish
  `route=canonical_certification`.
- Otherwise publish `route=representation_ablation_screen`.

The trigger receipt binds the development capture, evaluator binary and
sources, report hashes, exact boolean gate fields, and maximum anchor read
2,815. It cannot be edited or recomputed under different code.

## Stage 2A: canonical certification

Only `route=canonical_certification` authorizes the canonical capture runner
to create its certified-attempt receipt and invoke Runtime on `[2880,3264)`.
The attempt is published before invocation and cannot be retried or
overwritten. The two already selected trained affine arms may then parse the
same immutable certified probe and score one selected ridge each. They do not
refit or reselect. The untrained control remains validation-only.

## Stage 2B: representation ablation screen

`route=representation_ablation_screen` permanently forbids canonical
certified capture. The fixed
`REPRESENTATION_ABLATION_PREREGISTRATION.md` procedure instead captures
train and validation raw-encoder probes for canonical, `endpoint_scale`,
`time_only`, and `no_tf_alignment`; selects across those four validation
reports in the declared order; and gives the sole certified attempt to the
selected representation arm. No non-selected arm may capture certified
development.

This staging changes no model, feature arm, seed, step count, ridge value,
metric, threshold, tie rule, split boundary, or success interpretation. It
only prevents certified development from being opened before the already
declared conditional model-selection decision. No route authorizes the final
`[3328,4096)` interval.
