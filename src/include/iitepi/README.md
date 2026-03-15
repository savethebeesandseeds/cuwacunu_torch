# iitepi Architecture

`iitepi` is the runtime orchestration layer that executes ordered campaign binds
over contracts and waves.

Namespace: `cuwacunu::iitepi`  
Primary include: `#include "iitepi/iitepi.h"`

## Public Runtime Model

The public dispatcher is now campaign-oriented:

1. Contract
- static machine topology and acknowledgements

2. Wave
- runtime execution/training policy

3. Campaign
- imports contract files and wave files
- declares reusable `BIND` entries
- executes ordered `RUN <bind_id>;` steps

4. Binding
- one selected `contract + wave + bind-local variables`
- this is the execution unit launched by Runtime Hero

## Core Spaces

`iitepi` still manages the same internal runtime registries:

1. `config_space_t`
- loads `.config`
- exposes typed config access
- resolves `GENERAL.default_iitepi_campaign_dsl_filename`

2. `contract_space_t`
- immutable contract registry

3. `wave_space_t`
- immutable wave registry

4. `runtime_binding_space_t`
- private/internal worker join layer
- still materializes the internal runtime join state used by `run_runtime_binding(...)`
- no longer defines the public DSL model

## DSL Layering

1. `default.iitepi.contract.dsl`
- static contract wrapper
- `CIRCUIT_FILE: ...;`
- `AKNOWLEDGE: <alias> = <tsi family>;`

2. `default.iitepi.contract.circuit.dsl`
- TSI instances and hops
- optional `active_circuit = <circuit_name>`

3. `default.iitepi.wave.dsl`
- execution/training policy
- source runtime window and observation file paths
- wave-local `JKIMYEI { HALT_TRAIN, PROFILE_ID }`

4. `default.iitepi.campaign.dsl`
- `CAMPAIGN { ... }`
- `IMPORT_CONTRACT_FILE`
- `IMPORT_WAVE_FILE`
- `BIND <id> { __var = value; CONTRACT = ...; WAVE = ...; }`
- ordered `RUN <bind_id>;`

## Runtime Flow

1. Load config
- `config_space_t::change_config_file(...)`
- `config_space_t::update_config()`

2. Initialize runtime selection
- Runtime Hero or `main_campaign` resolves the configured campaign DSL
- one selected `RUN` becomes one execution binding

3. Internal worker bridge
- the selected campaign bind is materialized into the private internal
  runtime-binding snapshot consumed by existing builder/runtime code

4. Execute
- entry point remains `cuwacunu::iitepi::run_runtime_binding(binding_id, device)`
- the public caller supplies campaign/binding context before that step

## Integrity and Locking

1. Contract, wave, and internal join records remain immutable and hash-addressed.
2. Runtime lock integrity still fails fast on campaign-driven dependency drift.
3. Editing campaign/contract/wave files on disk mid-run is allowed, but changes
   are not reloaded into the active run.
4. Restart is required for edits to take effect.

## Practical Entry Points

1. Registry and lock lifecycle
- `config_space_t::update_config()`
- `config_space_t::effective_campaign_dsl_path()`
- `config_space_t::locked_campaign_hash()`
- `config_space_t::locked_binding_id()`

2. Programmatic execution
- `cuwacunu::iitepi::run_runtime_binding(binding_id[, device])`

3. Runtime dispatch
- `hero.runtime.start_campaign`
- `hero.runtime.get_campaign`
- `hero.runtime.list_campaigns`
- `hero.runtime.stop_campaign`

4. Dev-loop reset
- `main_campaign --reset-runtime-state`
- `.build/hero/runtime_reset`
- `hero.config.dev_nuke_reset`

This document reflects the public runtime model. Internal runtime-binding
machinery still exists as a worker implementation detail until the lower
runtime builder is fully campaign-native.
