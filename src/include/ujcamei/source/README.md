# Ujcamei Source

Source-owned records, storage, cursors, and dataloading live here.

- `registry/`: source registry DSL decoder for rows that declare what source
  evidence exists and where it lives. It also owns source instrument identity
  and source record schemas.
- `retrieval/`: retrieval-channel DSL decoder for rows that declare how source
  evidence is windowed, normalized, fetched, and reported for a request.
- `contract/`: compatibility data model and runtime decode surface while graph
  topology rows remain composed at the Kikijyeba boundary. Shared parser syntax
  support lives here while the compatibility decoder surface still exists.

The source registry decoder namespace is
`cuwacunu::ujcamei::source::registry`. The retrieval channel decoder namespace
is `cuwacunu::ujcamei::source::retrieval`. Shared parsing syntax support lives
under `cuwacunu::ujcamei::source::contract::syntax`. Source record schemas live
under `cuwacunu::ujcamei::source::registry::types`. Dataloading and storage live
under `cuwacunu::ujcamei::source::retrieval::dataloader` and
`cuwacunu::ujcamei::source::retrieval::storage`. The merged compatibility
contract remains under `cuwacunu::ujcamei::source::contract`.

Source rows also carry `source_kind` provenance (`real`, `synthetic`, or
`derived`) so graph/NodeLift validation can distinguish real instruments from
derived evidence. Ujcamei channel rows declare active temporal sampling and
window lengths. Kikijyeba graph topology rows declare active nodes and directed
instrument edges, resolve them against the Ujcamei source/channel universe, then
pass an explicit source materialization request into storage.

Runtime cursors are code-level retrieval/reporting metadata, not authored DSL
schema. `retrieval/dataloader/source_cursor.h` records source-range tokens,
graph-anchor batch cursors, and dataset-level anchor-domain reports. Ujcamei
retrieval should be reported through these cursors: they name the requested
source section, the available anchor domain, the yielded batch anchors, and the
anchors skipped by source coverage checks. Runtime `.lls` reports use those
cursors to attach Wikimyei work back to the exact source range it observed.

Split-DSL parsing through `src/config/.config` remains active. Ujcamei decodes
the source registry as its primary runtime shape; graph-first protocol loads
Ujcamei retrieval-channel rows and Kikijyeba graph topology, then composes them
with the source registry only at the pipeline boundary.
