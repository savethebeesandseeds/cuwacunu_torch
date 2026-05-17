# Ujcamei Source

Source-owned records, storage, cursors, and dataloading live here.

- `instrument_signature.h`: source instrument identity shared by contracts,
  dataloaders, and graph edge adapters.
- `contract/`: source DSL specs plus compatibility parser helpers while
  channel/graph dock rows finish moving to Kikijyeba composition.
- `types/`: market/source record schemas and exchange data value types.
- `dataloader/`: edge-local samples and graph-anchor edge source batches.
- `storage/`: source storage backends such as memory-mapped CSV/binary data.

The source contract namespace is `cuwacunu::ujcamei::source::contract`, with
parser/decoder helpers under `cuwacunu::ujcamei::source::contract::dsl`.
Dataloading lives under `cuwacunu::ujcamei::source::dataloader`.

Source rows also carry `source_kind` provenance (`real`, `synthetic`, or
`derived`) so graph/NodeLift validation can distinguish real instruments from
derived evidence. Graph-first dock rows declare active channel sampling and
directed instrument edges. Kikijyeba resolves those rows against the Ujcamei
source universe, then passes an explicit source materialization request into
storage.

Runtime cursors are code-level retrieval/reporting metadata, not authored DSL
schema. `dataloader/source_cursor.h` records source-range tokens,
graph-anchor batch cursors, and dataset-level anchor-domain reports so training
reports can name the exact source range and anchor coverage used by a run.

Split-DSL parsing through `src/config/.config` remains active. Ujcamei decodes
only the source universe as its primary runtime shape; graph-first channel and
graph dock rows are decoded by Kikijyeba composition and composed with the
source universe only at the pipeline boundary.
