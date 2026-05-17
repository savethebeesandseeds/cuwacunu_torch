# Wikimyei

Wikimyei is the worker pillar. The fresh graph-first path keeps only the worker
surfaces that are actively used by the node-centered pipeline:

- `expression/nodelift/`
- `representation/encoding/vicreg/`
- `inference/expected_value/mdn/`

Ujcamei is intentionally not nested under Wikimyei. Wikimyei consumes input or
source contracts; Ujcamei owns that boundary.

NodeLift belongs under `wikimyei.expression.nodelift`: it is not source input,
and it is not a learned representation. It is the deterministic expression that
turns edge evidence into node-state material for workers.

Fresh Wikimyei C++ namespaces mirror the room path:

- `cuwacunu::wikimyei::representation::encoding::vicreg`
- `cuwacunu::wikimyei::inference::expected_value`
- `cuwacunu::wikimyei::inference::expected_value::mdn`

The fresh VICReg surface is intentionally small: node-stream adapter, node
representation stream, rank-4 encoder, and config/spec helpers. The old
`VICReg_Rank4` wrapper, edge-era dataloader wrappers, and network-design bridge
were removed from the active tree; future representation training should add a
clean core instead of reviving those wrappers.

The matching implementation subtree lives under `src/impl/wikimyei` and keeps
Makefiles rather than README files.
