# Ujcamei Source Retrieval

Source retrieval declares how Ujcamei samples source evidence for a request. Its
active channel config is `src/config/ujcamei.source.retrieval.channels.dsl`,
with grammar
`src/config/grammar/ujcamei.source.retrieval.channels.dsl.bnf`.

Retrieval channels own active channel selection, input/future window lengths,
normalization policy, and reserved channel weighting. They do not define source
existence or graph topology.

Repeated channels with shared settings can be authored with `CHANNEL_SET`. The
decoder expands each interval into the same `channel_form_t` rows as the table,
so explicit rows can remain for channels with distinct window lengths.

Nested retrieval modules:

- `dataloader/`: edge-local samples, graph-anchor edge batches, and runtime
  source cursors.
- `storage/`: retrieval backends such as memory-mapped CSV/binary datasets.
