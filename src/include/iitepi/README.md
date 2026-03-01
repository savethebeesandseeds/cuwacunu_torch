# iitepi Specification

`iitepi` is the runtime registry and configuration space for board/contract/wave execution.

Namespace: `cuwacunu::iitepi`
Public include forms:
- `#include "iitepi/iitepi.h"`
- `#include "piaabo/dconfig.h"` (compat umbrella)

## Core Contract

`iitepi` provides four coupled spaces:

1. `config_space_t`
- global `.config` loading, validation, and typed access
- exchange mode selection (`TEST` / `REAL`)
- board path + binding selection values (`GENERAL.board_config_filename`,
  `GENERAL.board_binding_id`) used by board initialization

2. `contract_space_t`
- immutable contract record registry keyed by manifest hash
- object-level typed access to contract values and module instruction values
- dependency fingerprinting and integrity checks during runtime

3. `wave_space_t`
- immutable wave record registry keyed by manifest hash
- grouped access to wave grammar/DSL/decoded payload

4. `board_space_t`
- immutable board registry keyed by manifest hash
- board DSL join-layer resolution (`CONTRACT`, `WAVE`, `BIND`)
- explicit runtime lock lifecycle via `board_space_t::init(...)`
- helpers to resolve bound contract/wave hashes from selected binding id

## `config_space_t` Contract

Main behavior:

- `change_config_file(...)` selects global config file and triggers reload.
- `update_config()` parses, validates, and enforces runtime invariants.
- `get<T>(section,key,fallback)` and `get_arr<T>(...)` provide typed access.

Runtime invariants:

- `GENERAL.exchange_type` must be valid and cannot change mid-run.
- if board runtime is already initialized, reloads must keep the same locked board path
  and binding id.
- locked board/contract/wave records are re-checked for integrity on updates when board
  runtime is active.

Failure behavior:

- malformed/invalid critical config terminates via fail-fast logging path.
- missing typed keys raise `bad_config_access` unless fallback is provided.

## `contract_space_t` Contract

Main behavior:

- `register_contract_file(path)` canonicalizes path, builds contract record, and returns immutable contract hash.
- `contract_itself(hash)` returns the registered immutable contract record.
- `contract_itself(hash)->get<T>(section,key,fallback)` / `get_arr<T>(...)` provide typed access.
- grouped assets are exposed on record blobs:
  - `contract_itself(hash)->circuit.{grammar,dsl,decoded()}`
  - `contract_itself(hash)->observation.{sources,channels,decoded()}`
  - `contract_itself(hash)->jkimyei.{grammar,dsl,decoded()}`

## `wave_space_t` / `board_space_t` Contract

Main behavior:

- `register_wave_file(path)` / `register_board_file(path)` register immutable records.
- `board_space_t::init()` (or `init(path,binding)`) activates one immutable board+binding
  runtime lock.
- `wave_itself(hash)->wave.{grammar,dsl,decoded()}` exposes wave payload.
- `board_itself(hash)->board.{grammar,dsl,decoded()}` exposes board bind manifest.
- `contract_hash_for_binding(board_hash,binding_id)` and
  `wave_hash_for_binding(board_hash,binding_id)` resolve bound hashes.

Integrity contract:

- `assert_intact_or_fail_fast(hash)` checks dependency files remain intact.
- change in dependency hash/digest is treated as immutable lock violation.
- `assert_registry_intact_or_fail_fast()` validates all registered records.

## Concurrency Contract

- process-global shared state is guarded by space-specific mutexes.
- registries are singleton-style runtime state.

This specification captures runtime contracts and invariants; source remains authoritative for parsing and fingerprinting details.
