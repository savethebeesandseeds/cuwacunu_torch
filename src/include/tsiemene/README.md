tsiemene means "thing" (tsi for short)

Mental model: Kahn-style process network, driven by `Wave` events.

## Core model
- A `Tsi` consumes one ingress token per `step(wave, ingress, ctx, emitter)`.
- Routing is defined by `Hop { from(tsi@directive) -> to(tsi@directive) }`.
- Compatibility is based on `@directive + :kind` (not numeric ports).
- `Signal` kinds are currently:
  - `:str`
  - `:tensor`

## Board model (current)
- `Board` is now the high-order runtime container (`tsiemene/utils/board.h`).
- A board owns `BoardCircuit` entries; each circuit owns:
  - `nodes` (`std::vector<std::unique_ptr<Tsi>>`)
  - `hops` (`std::vector<Hop>`)
  - start execution state (`wave0`, `ingress0`)
  - metadata (`name`, `invoke_name`, `invoke_payload`)
- Helpers available in core:
  - `validate_circuit(...)`
  - `validate_board(...)`
  - `run_circuit(...)`
  - `run_board(...)`
  - `pick_start_directive(...)`

## Canonical directives
- Use shared ids from `tsiemene::directive_id`:
  - `directive_id::Payload` (`@payload`)
  - `directive_id::Loss` (`@loss`)
  - `directive_id::Meta` (`@meta`)

## Runtime behavior (current)
- `run_wave` pushes a start event to the `from.tsi` of the first hop.
- One queue event triggers exactly one `tsi.step(...)`.
- An emitted output can fan out to multiple matching hops.
- Circuit validation now enforces a single-root DAG with full reachability.
- Empty/null-start circuits are ignored safely (`run_wave` returns `0`).
- Board execution is now first-class:
  - `run_circuit(board.circuits[i], ctx)` runs one circuit.
  - `run_board(board, ctx)` runs all board circuits in sequence.

## Validation contract (current)
- Runtime graph validation (`tsiemene::validate` in `utils/circuits.h`) enforces:
  - non-empty circuit and non-null hop pointer
  - per-hop directive existence and `@directive/:kind` compatibility
  - exactly one root (`in_degree==0`)
  - first hop source must be that root
  - acyclic graph
  - every node reachable from root
  - terminal nodes must be sinks
- Board validation (`validate_board` in `utils/board.h`) additionally enforces:
  - non-empty `name`, `invoke_name`, `invoke_payload`
  - all hop endpoints are owned by `BoardCircuit::nodes`
  - no orphan nodes (every node appears in at least one hop)
  - unique `TsiId` within each circuit
  - `ingress0.directive` exists on root input and matches `ingress0.signal.kind`
- DSL semantic validation (`validate_circuit_decl` / `validate_board_instruction` in `tsiemene_board_runtime.h`) enforces:
  - non-empty circuit identity/invoke fields
  - non-empty `instances` and `hops`
  - unique instance aliases
  - hop endpoints reference declared aliases
  - directive/kind tokens resolve (`@payload/@loss/@meta`, `:str/:tensor`)
  - single-root, acyclic, fully reachable instance graph
  - terminal DSL instances must be sink types (`tsi.sink.*`)
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
- `tsi.sink.log.sys` now routes through `piaabo/dlogs.h` (`log_info/log_warn`), so logs are both terminal-visible and available in the in-memory dlogs buffer (used by iinuji `F3` logs screen).

## Implemented nodes
- `tsi.wikimyei.wave.generator`
- `tsi.wikimyei.source.dataloader`
- `tsi.wikimyei.representation.vicreg`
- `tsi.sink.null`
- `tsi.sink.log.sys`

## Reference executable
- `src/tests/bench/tsiemene/test_tsi_basic.cpp`
- `src/tests/bench/tsiemene/test_tsi_instruction.cpp`
- Runs one `circuit_1` approximation with instruction:
  - `BTCUSDT[01.01.2009,31.12.2009]`

Current circuit_1 wiring:
- `w_wave@payload:str -> w_source@payload:str`
- `w_source@payload:tensor -> w_rep@payload:tensor`
- `w_rep@payload:tensor -> w_null@payload:tensor`
- `w_rep@loss:tensor -> w_log@loss:tensor`
- `w_wave@meta:str -> w_log@meta:str`
- `w_source@meta:str -> w_log@meta:str`
- `w_rep@meta:str -> w_log@meta:str`
- `w_null@meta:str -> w_log@meta:str`

## Dataloader command behavior (current)
- Range mode: `SYMBOL[dd.mm.yyyy,dd.mm.yyyy]`
- Batch mode: `batches=N` (or legacy bare-number command in non-range mode)
- Important: range mode does **not** infer `batches` from date digits; only explicit `batches=` limits range emission.

## Board DSL status (current)
- A first board parser is now available under camahjucunu BNF:
  - Grammar: `src/config/bnf/tsiemene_board.bnf`
  - Instruction sample: `src/config/instructions/tsiemene_board.instruction`
  - Decoder: `BNF::tsiemeneBoard`
  - Runtime resolution helpers: `camahjucunu/BNF/implementations/tsiemene_board/tsiemene_board_runtime.h`
  - DSL -> `tsiemene::Board` bridge helpers: `tsiemene/utils/board.tsiemene_board.h`
  - Bench test: `src/tests/bench/camahjucunu/bnf/test_bnf_tsiemene_board.cpp`
- Implemented circuit syntax includes:
  - Circuit header/body/close
  - Instance declarations (`alias = tsi.type.path`)
  - Hop declarations (`from@directive:kind -> to@directive:kind`)
  - Circuit invoke payload (`circuit_name(payload);`)
- This remains a minimal routing subset, but now with baseline semantic validation aligned with runtime circuit rules.

## Scope boundary
- Richer DSL features (params, node-local configs, compile-time type registry, multi-wave scheduling policies) remain a later step.
