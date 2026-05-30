# Wikimyei

Wikimyei is the worker pillar. The fresh graph-first path keeps only the worker
surfaces that are actively used by the node-centered pipeline:

- `assembly.h`
- `expression/nodelift/`
- `representation/encoding/vicreg/`
- `representation/encoding/mtf_jepa_mae_vicreg/`
- `representation/time_mae/`
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
- `cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg`
- `cuwacunu::wikimyei::inference::expected_value`
- `cuwacunu::wikimyei::inference::expected_value::mdn`

The active VICReg surface is the strict channel-preserving representation path:
feature-level NodeLift masks enter the channel node adapter, the encoder keeps
`C` through `[M,C,Hx,De]`, temporal reduction is mask-aware inside the
representation boundary, and graph export emits `[B,N,C,De]`. Legacy fused/node
VICReg headers have been removed from the active source tree. Historical lattice
receipts that mention the old node representation job remain audit data only.
The legacy node MDN path has been removed; the active MDN surface is the
channel-context MDN.

`representation/encoding/mtf_jepa_mae_vicreg/` is a separate experimental
representation family for multi-scale time/frequency JEPA-MAE pretraining with
VICReg-style stabilization. It does not replace the active VICReg production
path, and it keeps its stability head/loss local rather than including the
production VICReg implementation.
`representation/time_mae/` remains reserved for a standalone TimeMAE family.

The matching implementation subtree lives under `src/impl/wikimyei` and keeps
Makefiles rather than README files.
