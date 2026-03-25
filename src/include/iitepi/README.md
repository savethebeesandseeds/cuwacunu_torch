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
- declares the default ordered `RUN <bind_id>;` plan

4. Binding
- one selected `contract + wave + bind-local variables`
- this is the execution unit launched by Runtime Hero

## Core Spaces

`iitepi` still manages the same internal runtime registries:

1. `config_space_t`
- loads `.config`
- exposes typed config access
- resolves `GENERAL.default_iitepi_campaign_dsl_filename`
  (checked in today as the `vicreg.solo` objective bundle)

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
- one or more named compatible circuits

3. `default.iitepi.wave.dsl`
- execution/training policy
- operational `CIRCUIT = <circuit_name>` selector when the contract declares
  more than one circuit
- source runtime window
- wave-local `JKIMYEI { HALT_TRAIN, PROFILE_ID }`
  where `PROFILE_ID` is only needed when the contract exposes multiple
  compatible profiles

4. `default.iitepi.campaign.dsl`
- `CAMPAIGN { ... }`
- `IMPORT_CONTRACT_FILE`
- `IMPORT_WAVE_FILE`
- `BIND <id> { __var = value; CONTRACT = ...; WAVE = ...; }`
- ordered `RUN <bind_id>;`

## Runtime Flow

1. Load config
- `config_space_t::change_config_file("/path/to/.config")`
- `config_space_t::update_config()`

2. Initialize runtime selection
- Runtime Hero or `main_campaign` resolves the configured campaign DSL
- the default `RUN` list becomes the execution plan unless an explicit
  binding override narrows launch to one declared `BIND`

3. Internal worker bridge
- the selected campaign bind is materialized into the private internal
  runtime-binding snapshot consumed by existing builder/runtime code
- contract-local `__variables` are resolved into the staged contract DSL graph
  while bind-local `__variables` remain wave-scoped operational overrides
- bind-local `__variables` may not shadow names already declared by the
  contract; overlapping names are rejected during campaign snapshot staging
- observation/channel DSL selection is contract-owned through contract
  `__variables`, while source symbol and date range remain wave-local runtime
  scope
- the checked-in defaults treat only public docking widths as contract-owned
  (`__obs_channels`, `__obs_seq_length`, `__obs_feature_dim`,
  `__embedding_dims`); private VICReg encoder/projector widths live in the
  VICReg DSLs
- contract snapshots also derive an explicit docking signature from the
  compatible circuit set, contract `__variables`, and docking-bearing contract
  DSL surfaces; the public docking digest includes the circuit, VICReg
  module/network-design, and contract-owned observation-channel DSL, while the
  observation source registry still affects exact contract identity but is
  intentionally excluded from the docking digest so unrelated source-row
  additions do not invalidate compatible component weights
- runtime reuse/load of an existing component hashimyei validates the selected
  component manifest against the current public docking signature; founding
  contract hash remains provenance, but it is no longer the hard runtime gate
- component manifests describe revision lifecycle through `lineage_state`,
  rather than the older generic `status`

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
