# iitepi Architecture

`iitepi` is the runtime orchestration layer that executes ordered campaign binds
over contracts and waves.

Namespace: `cuwacunu::iitepi`  
Primary include: `#include "iitepi/iitepi.h"`

## Public Runtime Model

The public dispatcher is now campaign-oriented:

1. Contract
- authored compatibility plus realization package
- contains `DOCK` and `ASSEMBLY`

2. Wave
- logical runtime policy plus required component slots

3. Campaign
- imports named contract ids and named wave ids from files
- declares reusable `BIND` entries
- declares the default ordered `RUN <bind_id>;` plan

4. Binding
- one selected `contract + wave + bind-local variables`
- derives concrete component ids from the selected contract and wave slots
- this is the execution unit launched by Runtime Hero

## Vocabulary

Human-facing docs use these terms deliberately:

1. Component revision
- one stored loadable family member
- this is the normal human-facing term for what gets reused or compared

2. Hashimyei
- the exact revision token and the name of the identity/catalog subsystem
- examples: `0x0000`, `0x00FF`
- use this word when you mean the stored token or the subsystem itself, not as
  a catch-all for contracts, waves, and selection policy

3. DOCK
- the public compatibility interface declared by a contract
- component compatibility is derived from the relevant DOCK surfaces plus the
  component instrument signature

4. ASSEMBLY
- the concrete realization owned by the contract
- includes path-bearing DSL ownership and private realization knobs

5. WAVE
- the logical run shape and policy
- declares which component slots are needed, but not which exact revision token
  should fill them

6. Derived Component
- the contract-derived exact component revision for a wave slot
- carries the concrete hashimyei used by runtime load/save surfaces

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

4. `runtime_binding/runtime_binding_space_t.h`
- private/internal worker join layer
- still materializes the internal runtime join state used by `run_runtime_binding(...)`
- no longer defines the public DSL model

## DSL Layering

1. `default.iitepi.contract.dsl`
- static contract wrapper
- `DOCK { ... }`
- `ASSEMBLY { CIRCUIT_FILE: ...; AKNOWLEDGE: <alias> = <tsi family>; ... }`

2. `default.iitepi.circuit.dsl`
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
- authored wave files do not select concrete component revisions

4. `default.iitepi.campaign.dsl`
- `CAMPAIGN { ... }`
- `IMPORT_CONTRACT "<contract_file>" AS <contract_alias>;`
- `FROM "<wave_file>" IMPORT_WAVE <wave_id>;`
- `BIND <id> { __var = value; CONTRACT = ...; WAVE = ...; }`
- ordered `RUN <bind_id>;`

## Runtime Flow

1. Load config
- `config_space_t::change_config_file("/path/to/.config")`
- `config_space_t::update_config()`

2. Initialize runtime selection
- Runtime Hero or `cuwacunu_campaign` resolves the configured campaign DSL
- the default `RUN` list becomes the execution plan unless an explicit
  binding override narrows launch to one declared `BIND`

3. Internal worker bridge
- the selected campaign bind is materialized into the private internal
  runtime-binding snapshot consumed by existing builder/runtime code
- component-compatible component revisions are derived for the selected wave
  before the internal runtime-binding snapshot is materialized
- the staged per-job `campaign.dsl` preserves the selected bind, while the
  staged `binding.wave.dsl` records the derived exact component revision paths
  selected for the run
- contract DOCK/ASSEMBLY `__variables` are resolved into the staged contract
  DSL graph while bind-local `__variables` remain wave-scoped operational
  overrides
- bind-local `__variables` may not shadow names already declared by the
  contract; overlapping names are rejected during campaign snapshot staging
- observation/channel DSL selection is assembly-owned through contract
  `ASSEMBLY` variables, while source runtime instrument signature and date range
  remain wave-local runtime scope
- the checked-in defaults keep public docking widths in `DOCK`
  (`__obs_channels`, `__obs_seq_length`, `__obs_feature_dim`,
  `__embedding_dims`, `__future_target_feature_indices`) and keep selected private
  `__vicreg_*` realization knobs in `ASSEMBLY`
- contract snapshots derive an explicit docking signature from the compatible
  circuit set, all `DOCK` assignments, all per-component
  `INSTRUMENT_SIGNATURE` declarations, and dock-bearing assembly surfaces; the
  public docking digest includes the circuit and contract-owned
  observation-channel DSL, while the observation source registry still affects
  exact contract identity but is intentionally excluded from the docking digest
  so unrelated source-row additions do not invalidate compatible component
  weights
- runtime reuse/load of an existing component revision validates the selected
  component manifest against the current component compatibility signature;
  founding contract hash and full docking signature remain provenance
- the same component compatibility rule is available to operators through
  `hero.hashimyei.evaluate_contract_compatibility`
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
- `hero.runtime.explain_binding_selection`
- `hero.runtime.get_campaign`
- `hero.runtime.list_campaigns`
- `hero.runtime.stop_campaign`

`hero.runtime.explain_binding_selection` is the read-only inspection surface for
contract-derived component selection. It uses the same component compatibility
path as launch-time snapshot staging and reports the exact component revision
selected for each wikimyei slot, including its exact hashimyei token, plus the
`contract_hash`, `docking_signature_sha256_hex`, and
`component_compatibility_sha256_hex` that governed compatibility. Use it first
when a launch fails before meaningful train/eval work starts.

4. Dev-loop reset
- `cuwacunu_campaign --reset-runtime-state`
- `.build/tools/runtime_reset`
- `hero.config.dev_nuke_reset`

The first two clear runtime-owned state only. `hero.config.dev_nuke_reset`
is the broader developer wipe that also removes Marshal/Human session ledgers.

This document reflects the public runtime model. Internal runtime-binding
machinery still exists as a worker implementation detail until the lower
runtime builder is fully campaign-native.
