# Development-prefix production-cursor alignment correction

Status: fixed on 2026-07-16 after the first isolated-source zero-step dry run
constructed its production graph cursor, but before an isolated-source closure,
clean representation checkpoint, clean MDN checkpoint, frozen feature row,
affine result, route trigger, or forecast metric existed. The failed preparation
attempt performed zero optimizer steps, wrote no checkpoint, and published no
completion receipt.

## Observed production domain

The sealed development-prefix inputs contain 3,294 `1d` rows, 1,098 `3d`
rows, and 471 `1w` rows. Relative to the fixed master-day origin, their last
bar open indices are respectively 3,293, 3,291, and 3,290. The production
graph cursor takes the common causally usable interval boundary. Its last
candidate key is therefore the close of master day 3,290, not the close of
master day 3,293.

The isolated Runtime dry run reported:

```text
accepted_anchor_count=3261
candidate_anchor_count=3261
skipped_outside_common_range=0
skipped_missing_edge_coverage=0
skipped_failed_fetch_probe=0
duplicate_anchor_count=0
source_cursor_first_master_day_index=29
source_cursor_last_master_day_index=3290
```

Thus the production anchor indices are exactly `[0,3261)`. The generator
manifest's earlier `max_visible_anchor_exclusive=3264` described the intended
daily-prefix boundary, but did not account for the coarser interval's common
cursor boundary. It is not the authoritative production-domain count.

## Corrected clean-development contract

The isolated closure and every clean downstream runner require 3,261 accepted
and candidate anchors and maximum anchor index 3,260. The already fixed train
and validation ranges remain unchanged:

```text
train=[0,2496)
validation=[2560,2816)
```

The certified-development range is shortened without inspecting any model
metric:

```text
certified_development=[2880,3261)
certified_development_anchor_count=381
certified_development_probe_rows=3429
```

No missing row is synthesized, copied from canonical `data/raw`, or recovered
from a later interval. The three unavailable tail anchors are excluded. The
V2 final interval remains operationally exposed and unavailable, and this
correction creates no independent final evidence.

This correction changes only the physically available production cursor count
and the certified-development end. It does not change source bytes, train or
validation rows, architecture, augmentation, seed, optimizer, step count,
candidate model, selection rule, gate, or interpretation ladder. Every clean
receipt binds this document in addition to the source-isolation and staged-
evaluation documents.
