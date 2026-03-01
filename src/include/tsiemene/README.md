# tsiemene

`tsiemene` means "thing" (`tsi` for short).

This README is a context guide for humans.  
It explains intent and architecture, but it is not a strict normative spec.

If this README and code disagree, trust:
1. Source code
2. BNF grammars
3. Tests

## Why tsiemene exists
`tsiemene` is the execution layer that lets you wire components (sources, models, sinks)
as a typed graph and run them in a wave-driven loop.

The main goal is architectural clarity:
- clear topology
- clear execution control
- clear training policy selection

## Mental model
Think of it as an event-driven graph:
- nodes are `Tsi` components
- edges are typed hops (`directive + kind`)
- runtime pushes events through the graph
- waves carry execution context (episode/batch/index and optional time span)

## High-level separation

### Circuit
Defines wiring/topology:
- what instances exist
- how they connect

### Wave
Defines how execution runs:
- train/run mode
- epochs
- batch size
- source ranges
- per-wikimyei training switches/profile ids

### Jkimyei
Defines policy catalogs:
- optimizer/scheduler/loss and related options

The practical idea is:
- circuit tells "how it is connected"
- wave tells "how to run now"
- jkimyei tells "which policy to use"

## Core runtime objects (conceptual)
- `Board`: runtime container with one or more contracts
- `BoardContract`: one executable unit (graph + seeds + metadata)
- `Wave`: execution state carried across events
- `BoardContext`: optional host context passed into nodes
- `Emitter`: runtime routing interface for node outputs

## Current behavior themes
- typed lane routing (not positional ports)
- queue-based runtime execution
- optional source continuation for multi-batch flows
- graph-level observability through `@meta` signals

## Current component family in active use
- `tsi.source.dataloader`
- `tsi.wikimyei.representation.vicreg`
- `tsi.sink.null`
- `tsi.sink.log.sys`

## Design direction (non-normative)
The project is moving toward:
- stable layering between topology/orchestration/policy
- explicit runtime intent in wave files
- reproducible default behavior for data-loading/training runs
- less hidden behavior and fewer legacy shortcuts

## Notes on documentation style
This README intentionally avoids over-precise constraints, to reduce doc/code drift.

For exact constraints, use:
- `src/config/dsl/tsiemene_circuit.bnf`
- `src/config/dsl/tsiemene_wave.bnf`
- validator/builder/runtime code under `src/include/tsiemene` and `src/impl`
- bench tests under `src/tests/bench/camahjucunu` and `src/tests/bench/tsiemene`

## Useful reading order
1. `src/include/iitepi/board/board.h`
2. `src/include/iitepi/board/board.contract.h`
3. `src/include/iitepi/board/board.runtime.h`
4. `src/include/iitepi/board/board.builder.h`
5. `src/config/instructions/iitepi_circuit.dsl`
6. `src/config/instructions/iitepi_wave.dsl`

## Reference tests
- `src/tests/bench/camahjucunu/test_dsl_tsiemene_circuit.cpp`
- `src/tests/bench/camahjucunu/test_dsl_tsiemene_wave.cpp`
- `src/tests/bench/iitepi/test_iitepi_board.cpp`
- `src/tests/bench/tsiemene/test_board_contract_init.cpp`
- `src/tests/bench/tsiemene/test_tsi_board_paths.cpp`
