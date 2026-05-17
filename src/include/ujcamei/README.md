# Ujcamei

Ujcamei means input, or source.

This subtree owns the code boundary where external evidence enters the system.
It is not a Wikimyei: workers may consume Ujcamei outputs, but the source itself
has its own name and dignity.

This migration follows the graph/source vocabulary in `doc/sections/`:

- `source/dataloader/graph_anchor_edge_batch.h`
- `source/dataloader/graph_anchor_edge_dataset.h`
- `source/dataloader/edge_sample.h`
- `source/instrument_signature.h`
- `graph/node.h`
- `graph/edge.h`
- `graph/graph.h`
- `source/contract/`
- `source/types/`
- `source/storage/memory_mapped/`

The migrated headers still carry legacy dependencies. They are here as fertile
ground, not as a final runtime contract.

Fresh Ujcamei C++ namespaces follow the pillar name:

- `cuwacunu::ujcamei::source::dataloader`
- `cuwacunu::ujcamei::graph`
- `cuwacunu::ujcamei::source`
- `cuwacunu::ujcamei::source::contract`
- `cuwacunu::ujcamei::source::contract::dsl`
- `cuwacunu::ujcamei::source::storage::memory_mapped`
- `cuwacunu::ujcamei::source::types`

Fresh Ujcamei config lives under `src/config/.config` and points to matching
dotted BNF/DSL filenames in `src/config`. Old runtime prefixes are not used in
fresh Ujcamei config paths.

Ujcamei source identity is the source universe. Graph-first channel and graph
activation are Kikijyeba composition dock settings that resolve against Ujcamei
sources before a dataloader is constructed.

`src/impl/ujcamei` mirrors these rooms with Makefiles. Source split-DSL loading
is wired through the fresh config file; the removed contract-key loader is no
longer part of the Ujcamei runtime surface.
