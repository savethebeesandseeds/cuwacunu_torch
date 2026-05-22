# Ujcamei

Ujcamei means input, or source.

This subtree owns the code boundary where external evidence enters the system.
It is not a Wikimyei: workers may consume Ujcamei outputs, but the source itself
has its own name and dignity.

This migration follows the graph/source vocabulary in `doc/sections/`, with
Ujcamei source code organized around source registry and source retrieval:

- `source/registry/instrument_signature.h`
- `source/registry/types/`
- `source/contract/`
- `source/retrieval/dataloader/graph_anchor_edge_batch.h`
- `source/retrieval/dataloader/graph_anchor_edge_dataset.h`
- `source/retrieval/dataloader/edge_sample.h`
- `source/retrieval/storage/memory_mapped/`

Protocol graph topology lives in `kikijyeba/topology/graph`. Ujcamei
retrieval consumes resolved topology when constructing graph-anchor batches, but
Ujcamei does not own active graph topology.

The remaining compatibility boundary is `source/contract`: it still carries the
split-DSL bundle shape needed to dock Ujcamei source registry/retrieval with the
current Kikijyeba protocol topology and Wikimyei consumers.

Fresh Ujcamei C++ namespaces follow the pillar name:

- `cuwacunu::ujcamei::source`
- `cuwacunu::ujcamei::source::contract`
- `cuwacunu::ujcamei::source::contract::syntax`
- `cuwacunu::ujcamei::source::registry`
- `cuwacunu::ujcamei::source::registry::types`
- `cuwacunu::ujcamei::source::retrieval`
- `cuwacunu::ujcamei::source::retrieval::dataloader`
- `cuwacunu::ujcamei::source::retrieval::storage::memory_mapped`

The graph topology namespace consumed by Ujcamei retrieval is
`cuwacunu::kikijyeba::topology::graph`.

Fresh Ujcamei config lives under `src/config/.config` and points to matching
dotted BNF/DSL filenames in `src/config`. Old runtime prefixes are not used in
fresh Ujcamei config paths.

Ujcamei source identity is the source universe. Graph-first channel and graph
activation are Kikijyeba protocol dock settings that resolve against Ujcamei
sources before a dataloader is constructed.

`src/impl/ujcamei` mirrors these rooms with Makefiles. Source split-DSL loading
is wired through the fresh config file; the removed contract-key loader is no
longer part of the Ujcamei runtime surface.
