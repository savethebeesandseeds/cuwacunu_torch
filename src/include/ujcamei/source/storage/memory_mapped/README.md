# Memory-Mapped Ujcamei Source Storage

Memory-mapped CSV/binary tensor loading lives here by Ujcamei source
ownership.
The fresh namespace is `cuwacunu::ujcamei::source::storage::memory_mapped`.

This is usable as the edge-local backing store for graph-collated Ujcamei. The
API still exposes edge-local channel aggregation assumptions. Public
constructors should prefer `source_materialization_request_t`, which contains
only source rows, channel sampling rows, and CSV/cache policy needed to
materialize storage. `source_spec_t` overloads remain as compatibility wrappers;
source-contract loading and Kikijyeba graph-first dock resolution are handled
above this storage folder.
