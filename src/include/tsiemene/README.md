tsiemene means "thing" (tsi for short)

Mental model: Kahn-style process network, driven by `Wave` events.

## Core model
- `Wave` is board-owned state (`tsiemene/board.wave.h`), not a `Tsi` component.
  - carries `{id, episode, batch, i}` and optional generic time-span `[span_begin_ms, span_end_ms]`
- A `Tsi` consumes one ingress token per `step(wave, ingress, ctx, emitter)`.
- Routing is defined by `Hop { from(tsi@directive) -> to(tsi@directive) }`.
- Compatibility is based on `@directive + :kind` (not numeric ports).
- `Signal` kinds are currently:
  - `:str`
  - `:tensor`

## Board model (current)
- `Board` is now the high-order runtime container (`tsiemene/board.h`).
- A board owns `BoardContract` entries (`tsiemene/board.contract.h`); each contract owns:
  - `spec` (`instrument`, `sample_type`, `source_type`, `representation_type`, `representation_hashimyei`, `component_types`, dims/hints, vicreg runtime flags)
  - one executable circuit payload (`contract.circuit()` / `BoardContractCircuit`)
- Circuit payload owns:
  - `nodes` (`std::vector<std::unique_ptr<Tsi>>`)
  - `hops` (`std::vector<Hop>`)
  - start execution state (`wave0`, `ingress0`)
  - metadata (`name`, `invoke_name`, `invoke_payload`)
- Helpers available in core:
  - `run_contract(...)`
  - `validate_circuit(...)`
  - `validate_board(...)`
  - `run_circuit(...)`
  - `run_board(...)`
  - `pick_start_directive(...)`

Example contract (runtime view):
```txt
contract[0]:
  name: circuit_1
  invoke: circuit_1("BTCUSDT[01.01.2009,31.12.2009]")
  spec:
    instrument: BTCUSDT
    sample_type: kline
    source_type: tsi.source.dataloader
    representation_type: tsi.wikimyei.representation.vicreg
    representation_hashimyei: 0x8
    component_types:
      - tsi.source.dataloader
      - tsi.wikimyei.representation.vicreg
      - tsi.sink.null
      - tsi.sink.log.sys
    vicreg:
      train: true
      use_swa: true
      detach_to_cpu: true
    shape_hint:
      B: 64
      C: 3
      T: 30
      D: 9
      Tf: 0
```

## Canonical directives
- Use shared ids from `tsiemene::directive_id`:
  - `directive_id::Step` (`@step`)
  - `directive_id::Payload` (`@payload`)
  - `directive_id::Future` (`@future`)
  - `directive_id::Loss` (`@loss`)
  - `directive_id::Meta` (`@meta`)
  - `directive_id::Info` (`@info`)
  - `directive_id::Warn` (`@warn`)
  - `directive_id::Debug` (`@debug`)
  - `directive_id::Error` (`@error`)
- Board/control ids from `tsiemene/board.paths.def` are also canonical:
  - `directive_id::Init` (`@init`)
  - `directive_id::Jkimyei` (`@jkimyei`)
  - `directive_id::Weights` (`@weights`)

## Canonical component schema
- `tsiemene/tsi.paths.def` is now the central manifest for:
  - TSI dataflow directive tokens and component callable method tokens
  - tsi component catalog (`canonical`, `domain`, `instance policy`)
  - per-component lane schema (`in/out`, directive id, cast kind, summary)
- `tsiemene/board.paths.def` is the board-control manifest for:
  - board/control directives and methods (`@init`, `@jkimyei`, `@weights`, `init`, `jkimyei`)
  - canonical board actions (`board.contract@init`)
  - canonical board contract DSL segment keys (`board.contract.*@DSL:str`)
- `tsiemene/tsi.type.registry.h` is generated from this manifest and remains the typed API used by builder/DSL validation.
- `BoardContract::Spec` type metadata is also sourced from this registry (no hardcoded canonical type strings in builder flow).
- Component construction in board builder consumes contract spec fields (for example VICReg runtime flags and shape hints), so runtime instances are seeded from contract state.

## Runtime behavior (current)
- `run_wave` pushes a start event to the `from.tsi` of the first hop.
- One queue event triggers exactly one `tsi.step(...)`.
- An emitted output can fan out to multiple matching hops.
- Pull-style sources can request runtime continuation after queue drain
  (used for single-batch episode stepping).
- Runtime continuation advances wave dispatch cursor with `advance_wave_batch(...)`
  (increments both `batch` and `i`; `episode` stays stable unless reseeded).
- Circuit validation now enforces a single-root DAG with full reachability.
- Empty/null-start circuits are ignored safely (`run_wave` returns `0`).
- Board execution is now first-class:
  - `run_contract(board.contracts[i], ctx)` runs one contract.
  - `run_circuit(board.circuits[i], ctx)` remains as compatibility alias.
  - `run_board(board, ctx)` runs all board contracts in sequence.

## Validation contract (current)
- Runtime graph validation (`tsiemene::validate` in `tsiemene/board.contract.circuit.h`) enforces:
  - non-empty circuit and non-null hop pointer
  - per-hop directive existence and `@directive/:kind` compatibility
  - exactly one root (`in_degree==0`)
  - first hop source must be that root
  - acyclic graph
  - every node reachable from root
  - terminal nodes must be sinks
- Board validation (`validate_board` in `tsiemene/board.h`) additionally enforces:
  - non-empty `name`, `invoke_name`, `invoke_payload`
  - all hop endpoints are owned by contract circuit nodes
  - no orphan nodes (every node appears in at least one hop)
  - unique `TsiId` within each contract
  - `ingress0.directive` exists on root input and matches `ingress0.signal.kind`
- DSL semantic validation (`validate_circuit_decl` / `validate_circuit_instruction` in `tsiemene_circuit_runtime.h`) enforces:
  - non-empty circuit identity/invoke fields
  - non-empty `instances` and `hops`
  - unique instance aliases
  - hop endpoints reference declared aliases
  - hop source must match a declared output lane of source tsi type
  - hop target must match a declared input lane of target tsi type
  - hop target must declare inbound directive explicitly (`-> alias@directive`)
  - hop target kind cast is forbidden (RHS kind is inferred from LHS kind)
  - directive/kind tokens resolve (`@step/@payload/@loss/@meta/@info/@warn/@debug/@error`, `:str/:tensor`)
  - single-root, acyclic, fully reachable instance graph
  - terminal DSL instances must resolve to registered sink `TsiTypeId` values
  - unique circuit names and invoke names at board scope

## Observability model
- No fixed printf logging inside processing TSIs.
- Runtime emits structured trace events via `@meta:str`:
  - `step`
  - `route`
  - `drop`
  - `step.done`
- TSIs may emit their own `@meta` facts for domain context (for example dataloader mode/emit summary, vicreg input/output shape summary).
- Logging is circuit-defined: you only see traces if `@meta` is wired to `tsi.sink.log.sys`.
- `tsi.sink.log.sys` now routes through `piaabo/dlogs.h` (`log_info/log_warn`), so logs are both terminal-visible and available in the in-memory dlogs buffer (used by iinuji `F8` logs screen).

## Implemented nodes
- `tsi.source.dataloader`
- `tsi.wikimyei.representation.vicreg`
- `tsi.sink.null`
- `tsi.sink.log.sys`

## Reference executable
- `src/tests/bench/tsiemene/test_tsi_basic.cpp`
- `src/tests/bench/tsiemene/test_tsi_instruction.cpp`
- Runs one `circuit_1` approximation with instruction:
  - `BTCUSDT[01.01.2009,31.12.2009]`

Current circuit_1 wiring:
- `w_source@payload:tensor -> w_rep@step`
- `w_rep@payload:tensor -> w_null@step`
- `w_rep@loss:tensor -> w_log@info`
- `w_source@meta:str -> w_log@warn`
- `w_rep@meta:str -> w_log@debug`
- `w_null@meta:str -> w_log@debug`

## Dataloader command behavior (current)
- Range mode: `SYMBOL[dd.mm.yyyy,dd.mm.yyyy]`
- Batch mode: `batches=N`
- Important: range mode does **not** infer `batches` from date digits; only explicit `batches=` limits range emission.
- Wave-seeded range mode is also supported: if wave has time-span and command omits `[dd.mm.yyyy,...]`,
  source consumes wave span for key-range dispatch.
- One source step emits one batch (`@payload`) and optionally one future batch (`@future`).
- Runtime continuation drives the next source step automatically while the current episode has pending batches.
- Init action (iinuji): `iinuji.tsi.dataloader.init()` is contract-backed.
  - No dataloader init artifacts are persisted on disk.
  - Runtime state is derived from current `contract_space_t` observation DSLs.
  - The dataloader init id is a stable virtual id (`0x0`) used only for command routing/UI selection.

## Board DSL status (current)
- A first board parser is now available under camahjucunu dsl modules (powered by the parser core):
  - Grammar: `src/config/dsl/tsiemene_circuit.bnf`
  - DSL sample: `src/config/instructions/tsiemene_circuit.dsl`
  - Decoder: `dsl::tsiemeneCircuits`
  - Runtime resolution helpers: `camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h`
  - DSL -> `tsiemene::Board` bridge helpers: `tsiemene/board.builder.h`
  - Bench test: `src/tests/bench/camahjucunu/dsl/test_dsl_tsiemene_circuit.cpp`
- Implemented circuit syntax includes:
  - Circuit header/body/close
  - Instance declarations (`alias = tsi.type.path`)
  - Hop declarations (`from@directive:kind -> to@directive`)
  - Circuit invoke payload (`circuit_name(payload);`)
- This remains a minimal routing subset, but now with baseline semantic validation aligned with runtime circuit rules.

## Scope boundary
- Richer DSL features (params, node-local configs, multi-wave scheduling policies) remain a later step.

## Invoke envelope (wave/source split)
- Compact invoke payload is still valid:
  - `circuit_1(BTCUSDT[01.01.2009,31.12.2009]);`
- Optional generalized wave envelope:
  - `circuit_1(wave@symbol:BTCUSDT,episode:2,batch:0,from:01.01.2009,to:31.12.2009@batches=16);`
- Semantics:
  - left side of second `@` (`wave@...`) seeds `wave0` dispatch metadata.
  - right side (after second `@`) is the source command ingress.
  - `symbol:` in wave metadata can provide instrument identity even when source command is generic (for example `batches=16`).
