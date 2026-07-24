# Development-prefix cursor-alignment metadata erratum

Status: fixed on 2026-07-16 immediately after the isolated-development-source
v1 closure was published and independently inspected, but before any clean
representation or MDN training, feature capture, affine evaluation, route
selection, or forecast metric. The immutable v1 closure is retained rather
than rewritten.

## Corrected interpretation

`DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_CORRECTION.md` and the v1 closure label
master day 3,290 as `source_cursor_last_master_day_index` and describe it as
the last candidate day. That description is off by one.

The prefix bar-open boundaries remain:

```text
1d last bar open master day = 3293
3d last bar open master day = 3291
1w last bar open master day = 3290
```

Day 3,290 is the coarsest interval boundary needed by the cursor's one-step
future requirement (`Hx=30`, `Hf=1`). The actual last candidate key is
`2177711999999`, the close of master day 3,289. The first candidate key is
`1896047999999`, the close of master day 29. Therefore:

```text
source_cursor_first_anchor_master_day_index=29
source_cursor_last_anchor_master_day_index=3289
source_cursor_last_required_coarse_boundary_master_day_index=3290
accepted_anchor_count=3289-29+1=3261
candidate_anchor_count=3261
maximum_anchor_index=3260
```

The accepted domain, train range, validation range, shortened certified-
development range `[2880,3261)`, and 3,429 certified probe rows are unchanged.
No source byte, cache, checkpoint, model choice, or metric is changed.

## Required provenance layer

The existing v1 closure's source hashes, cache hashes, accepted/candidate
counts, maximum anchor index, skip counters, and duplicate count remain valid.
Its `source_cursor_last_master_day_index=3290` field is superseded and must be
interpreted only as the last required coarse boundary. Clean downstream work
must additionally verify and bind an immutable cursor-erratum receipt that
binds the v1 closure, its original correction document, and this erratum.

V2 final evidence remains operationally exposed and unavailable. This erratum
does not authorize canonical `data/raw`, an omitted tail anchor, or independent
final evaluation.
