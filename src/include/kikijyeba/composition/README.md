# Kikijyeba Composition

This room owns cross-worker graph-first composition.

It can load source, representation, inference, and training specs into one
validated bundle, and it can build the runtime objects needed by launchers. It
does not own source graph primitives, NodeLift math, representation workers,
inference heads, or training loops.

- `config_bundle.h` loads and cross-validates Ujcamei, Wikimyei, and Jkimyei
  specs from `.config`.
- `source_dock.h` separates Ujcamei source reality from graph-first dock
  settings. Channels and active graph rows are a Kikijyeba assembly decision;
  Ujcamei sources remain the source universe. It also emits a source-resolution
  report that compares active nodes, selected edges, and available real source
  edges before any dataloader is materialized.
- `pipeline_builder.h` turns that bundle into graph-first runtime objects and
  dry-run reports, including graph-resolution counts such as available directed
  edges, missing ordered pairs, reverse pairs, connected components, and cycle
  dimension.

Jkimyei launchers consume this composition layer for training orchestration.
