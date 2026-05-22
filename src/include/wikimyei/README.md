# Wikimyei

Wikimyei is the worker pillar. The fresh graph-first path keeps only the worker
surfaces that are actively used by the node-centered pipeline:

- `assembly.h`
- `expression/nodelift/`
- `representation/encoding/vicreg/`
- `inference/expected_value/mdn/`

Ujcamei is intentionally not nested under Wikimyei. Wikimyei consumes input or
source contracts; Ujcamei owns that boundary.

NodeLift belongs under `wikimyei.expression.nodelift`: it is not source input,
and it is not a learned representation. It is the deterministic expression that
turns edge evidence into node-state material for workers.

Every active Wikimyei should expose an assembly: the static compatibility
surface that declares its family, component id, trainability, consumed stream
docks, produced stream docks, and constraints. Assembly is not runtime state and
it is not training policy. Runtime evidence belongs to `.lls`; Jkimyei `.jkimyei`
files describe how a trainable assembly is trained.

Assembly docks may name symbolic dimensions such as `N`, `L`, `C`, `Hx`, `Hf`,
`F`, `De`, `Df`, and `K`. Those are not old global DSL variables. Kikijyeba
topology resolves them into a `dock_binding` from the loaded source, topology,
retrieval, and network specs, then records the resulting binding token in stream
plans and `.lls` runtime reports.

Fresh Wikimyei C++ namespaces mirror the room path:

- `cuwacunu::wikimyei::assembly`
- `cuwacunu::wikimyei::representation::encoding::vicreg`
- `cuwacunu::wikimyei::inference::expected_value`
- `cuwacunu::wikimyei::inference::expected_value::mdn`

The fresh VICReg surface is intentionally small: node-stream adapter, node
representation stream, rank-4 encoder, node training model, and config/spec
helpers. Representation code should stay node-stream centered and keep training
orchestration in Jkimyei.

The matching implementation subtree lives under `src/impl/wikimyei` and keeps
Makefiles rather than README files.
