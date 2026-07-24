# Representation/readout diagnostic protocol amendment

Status: fixed on 2026-07-16 before the v2 representation training run
completed, before any v2 representation forecast or frozen-feature probe was
produced, and before any v2 production MDN checkpoint existed. The raw-history
isolation result was already known. It did not motivate any metric, ridge,
threshold, model, or split change below.

The supplemental preregistration described one contiguous feature capture for
anchors `[0,3264)`, with purge anchors retained only as an integrity record.
Before executing that capture, implementation review found a stricter
transport: capture only the three authorized metric domains as three explicit,
immutable production jobs:

```text
train       [0,2496)       22,464 edge/channel rows
validation  [2560,2816)     2,304 edge/channel rows
certified   [2880,3264)     3,456 edge/channel rows
```

No purge anchor is captured. Each job binds the same frozen representation and
MDN checkpoints, reports exact production mixture and direct-readout metrics
for its own range, and emits the corresponding frozen context features. The
certified job has an explicit single-attempt receipt; a partial or failed job
directory is never overwritten or retried.

The affine procedure is otherwise unchanged. It fits only the train probe,
selects the predeclared ridge grid only from the validation probe, and opens
the certified probe only after a candidate has been selected. Its source order
enforces that certified input timing. It performs no refit. Main/replay
execution repeats the deterministic affine calculation over the already
captured immutable inputs; it does not invoke or rescore the production model.

This amendment strengthens exposure precision and exact split attribution. It
does not change the scientific hypotheses, candidate set, selection order,
success gates, or interpretation ladder. The final `[3328,4096)` interval
remains absent and unauthorized.
