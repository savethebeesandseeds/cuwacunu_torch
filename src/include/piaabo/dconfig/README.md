# dconfig Specification

`dconfig` is the configuration and contract-lock subsystem for runtime execution.

Namespace: `cuwacunu::piaabo::dconfig`
Public include form: `#include "piaabo/dconfig.h"`

## Core Contract

`dconfig` provides two coupled spaces:

1. `config_space_t`
- global `.config` loading, validation, and typed access
- exchange mode selection (`TEST` / `REAL`)
- immutable lock to one board-contract path per process run

2. `contract_space_t`
- immutable contract snapshot registry keyed by manifest hash
- typed access to contract values and module instruction values
- dependency fingerprinting and integrity checks during runtime

## `config_space_t` Contract

Main behavior:

- `change_config_file(...)` selects global config file and triggers reload.
- `update_config()` parses, validates, and then enforces runtime invariants.
- `get<T>(section,key,fallback)` and `get_arr<T>(...)` provide typed access.

Runtime invariants:

- `GENERAL.exchange_type` must be valid and cannot change mid-run.
- `GENERAL.board_contract_config_filename` is resolved and locked on first load.
- subsequent reloads must keep the same locked contract path.
- locked contract snapshot is re-checked for integrity on updates.

Failure behavior:

- malformed/invalid critical config terminates via fail-fast logging path.
- missing typed keys raise `bad_config_access` unless fallback is provided.

## `contract_space_t` Contract

Main behavior:

- `register_contract_file(path)` canonicalizes path, builds snapshot, and returns immutable contract hash.
- `snapshot(hash)` returns registered immutable snapshot.
- `get<T>(hash,section,key,fallback)` / `get_arr<T>(...)` provide typed access.
- grammar/dsl helper getters expose resolved contract assets.

Snapshot content (behavioral):

- parsed contract config
- resolved module instruction sections (`VICReg`, optional `VALUE_ESTIMATION`)
- loaded DSL asset text from configured DSL files
- effective inline DSL payloads (with fallback to file payload when inline text is absent)
- dependency fingerprint manifest (`path`, size, mtime, sha256) + aggregate digest

Integrity contract:

- `assert_intact_or_fail_fast(hash)` checks dependency files remain intact.
- change in dependency hash/digest is treated as immutable lock violation.
- `assert_registry_intact_or_fail_fast()` validates all registered snapshots.

Lookup contract:

- `raw(hash, section, key)` first checks contract config section/key.
- if missing, module instruction section values are consulted.
- missing section/key then raises `bad_config_access`.

## Compatibility and Removed-Key Policy

Validation explicitly rejects deprecated split-mode keys (for circuit/wave DSL and related legacy fields) and requires current unified keys.

## Legacy Ghosts

- Removed-key compatibility checks are still embedded in runtime validation and should be retired after migration windows close.
- Module instruction parsing is custom/ad-hoc (instruction-file key parsing), which increases maintenance burden.
- Snapshot hashing uses a local SHA-256 implementation; consolidation with a shared crypto utility is a likely future cleanup.
- Global singleton registries and static locks are intentional but complicate isolated testing and multi-context embedding.

## Concurrency Contract

- process-global shared state is guarded by `config_mutex` and `contract_config_mutex`.
- contract registry is singleton-style runtime state.

This spec defines runtime contracts and invariants; source remains authoritative for parser and fingerprinting implementation details.
