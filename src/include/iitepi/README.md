# iitepi Architecture

`iitepi` is the runtime orchestration layer for executing TSI circuits through
explicit board bindings.

Namespace: `cuwacunu::iitepi`  
Primary include: `#include "iitepi/iitepi.h"`

## What iitepi Owns

`iitepi` manages four runtime registries (spaces):

1. `config_space_t`
- Loads global `.config`.
- Exposes typed config access (`get<T>`, `get_arr<T>`).
- Provides the selected board file and binding id used for runtime lock.

2. `contract_space_t`
- Registers immutable contract records (hash-addressed).
- Holds contract assets and decoders:
  `circuit`, `observation`, `jkimyei`, `canonical_path`.

3. `wave_space_t`
- Registers immutable wave records (hash-addressed).
- Holds wave grammar/DSL and decoded wave set.

4. `board_space_t`
- Registers immutable board records (hash-addressed).
- Resolves `BIND` entries to contract/wave hashes.
- Owns runtime lock state for selected board + selected binding.

## Responsibility Split

Architecture is intentionally split into static topology and dynamic execution:

1. Contract (static machine)
- Observation, jkimyei specs, circuit topology.
- No runtime schedule ownership.

2. Wave (dynamic execution policy)
- Mode (`train`/`run`), sampler, epochs, batch size, max batches.
- Component train flags and profile selection.
- Source symbol/range window.
- Dataloader runtime knobs (`WORKERS`, `FORCE_REBUILD_CACHE`,
  `RANGE_WARN_BATCHES`) and observation file ownership
  (`SOURCES_DSL_FILE`, `CHANNELS_DSL_FILE`) are declared per `SOURCE` block.

3. Board (join layer)
- Imports contract files and wave files.
- Declares executable `BIND` entries:
  `CONTRACT = ...`, `WAVE = ...`.

4. Binding (execution choice)
- Picks one contract + one wave for a run.
- This is the runtime selection unit.

## DSL Layering

1. `iitepi.contract.circuit.example.dsl`
- Declares TSI instances and hops.
- Topology only.

2. `iitepi.wave.example.dsl`
- Declares one or more `WAVE` blocks.
- Runtime behavior for execution/training.

3. `iitepi.board.dsl`
- Imports contract/wave files.
- Declares bind ids that join them.

## Runtime Flow

1. Load config
- `config_space_t::change_config_file(...)`
- `config_space_t::update_config()`

2. Initialize board lock
- `board_space_t::init()` reads configured board path + binding id.
- `board_space_t::assert_locked_runtime_intact_or_fail_fast()` checks lock integrity.

3. Resolve selected binding
- Board decode identifies `BIND`.
- `board_space_t::contract_hash_for_binding(...)`
- `board_space_t::wave_hash_for_binding(...)`

4. Build runtime board
- `board.builder` validates and materializes runtime contracts/nodes.
- Observation runtime payload is selected from wave dataloader file paths
  (not from contract default observation DSL text).
- Validations are explicit:
  `validate_contract_definition(...)`,
  `validate_wave_definition(...)`,
  `validate_wave_contract_compatibility(...)`.

5. Execute
- Entry point: `cuwacunu::iitepi::run_binding(binding_id, device)`.
- Returns `board_binding_run_record_t` with `ok/error`, hashes, sampler/type, steps.

## TSI Execution Model

1. Circuit compiles into runtime nodes and route map.
2. Execution is wave-driven:
- `run_contract(...)` iterates epochs from wave execution spec.
- `run_wave_compiled(...)` processes event queue (`Ingress` -> TSI `step` -> emitted signals).
3. Sources emit payload/future/meta; wikimyei consume/emit payload/loss/meta; sinks consume logs/null.
4. Artifact load/save is handled through TSI wikimyei + hashimyei integration in contract runtime.

## Record Model

`contract_record_t`, `wave_record_t`, `board_record_t` expose:

1. Raw grammar/DSL blobs.
2. Lazy-decoded AST accessors (`decoded()`).
3. Typed `get/get_arr` helpers against parsed config sections.
4. Dependency manifest fingerprints for integrity checks.

## Integrity and Locking

1. Every registry record stores dependency fingerprint manifest.
   For waves, this includes wave-selected
   `SOURCE.SOURCES_DSL_FILE` and `SOURCE.CHANNELS_DSL_FILE`.
2. `assert_intact_or_fail_fast(hash)` verifies immutable dependency content.
3. `board_space_t::init(...)` establishes the active runtime lock.
4. Config reload while active runtime is locked must not drift board/binding identity.

## Device Policy

Execution can fall back from GPU to CPU when configured GPU is unavailable or
runtime-unusable. Fallback is intentionally loud via
`[torch_utils][CPU_FALLBACK_ACTIVE] ...` warning logs.

## Practical Entry Points

1. Registry and lock lifecycle
- `config_space_t::update_config()`
- `board_space_t::init()`
- `board_space_t::locked_board_hash()`
- `board_space_t::locked_board_binding_id()`
- `board_space_t::network_analytics([std::ostream*], [bool beautify])`
- `contract_space_t::network_analytics(contract_hash[, std::ostream*], [bool beautify])`

2. Programmatic execution
- `cuwacunu::iitepi::run_binding(binding_id[, device])`

3. Introspection
- `contract_space_t::contract_itself(hash)`
- `wave_space_t::wave_itself(hash)`
- `board_space_t::board_itself(hash)`

This document defines architecture intent and runtime contracts. Source code
remains authoritative for low-level parsing, builder details, and diagnostics.
