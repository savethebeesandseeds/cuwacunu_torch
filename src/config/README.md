# Config Folder Layout

This folder is organized by role:

- `./.config`: global runtime settings (exchange, seeds, UI/system knobs).
- `./instructions/default.iitepi.contract.dsl`: campaign contract defaults file.
- `./bnf/`: grammar files (`*.bnf`).
- `./instructions/`: DSL payload files (`*.dsl`).
- `./secrets/real/`: real exchange secret material.
- `./secrets/test/`: test exchange secret material.

## Current file map

- `bnf/tsi.source.dataloader.sources.bnf`
- `bnf/tsi.source.dataloader.channels.bnf`
- `bnf/jkimyei.bnf`
- `bnf/iitepi.campaign.bnf`
- `bnf/iitepi.contract.bnf`
- `bnf/iitepi.contract.circuit.bnf`
- `bnf/iitepi.wave.bnf`
- `bnf/network_design.bnf`
- `bnf/canonical_path.bnf`
- `bnf/latent_lineage_state.bnf`
- `bnf/runtime_lls.bnf`
- `.config`
- `instructions/default.iitepi.contract.dsl`
- `instructions/default.tsi.source.dataloader.sources.dsl`
- `instructions/default.tsi.source.dataloader.channels.dsl`
- `instructions/default.tsi.wikimyei.representation.vicreg.jkimyei.dsl`
- `instructions/default.iitepi.campaign.dsl`
- `instructions/default.iitepi.contract.circuit.dsl`
- `instructions/default.iitepi.wave.dsl` (canonical wave payload set)
- `instructions/default.tsi.wikimyei.representation.vicreg.dsl`
- `instructions/default.tsi.wikimyei.representation.vicreg.network_design.dsl`
- `instructions/default.tsi.wikimyei.inference.mdn.value_estimation.dsl`
- `instructions/default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl`
- `instructions/default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl`
- `instructions/default.hero.runtime.dsl`
- `secrets/real/ed25519key.pem` (expected, may be absent locally)
- `secrets/real/exchange.key` (expected, may be absent locally)
- `secrets/real/openai.key` (expected for HERO/OpenAI, may be absent locally)
- `secrets/test/ed25519key.pem`
- `secrets/test/ed25519pub.pem`
- `secrets/test/exchange.key`

## Generate Ed25519 keys

```bash
apt update
apt install openssl --no-install-recommends
openssl version
```

```bash
cd /cuwacunu/src/config/secrets/real
openssl genpkey -algorithm Ed25519 -out ed25519key.pem -aes-256-cbc
openssl pkey -in ed25519key.pem -out ed25519pub.pem -pubout
```

Use `ed25519pub.pem` when registering API access at the exchange.

## Configure API key material

1. Put plaintext API key into `secrets/real/exchange.key`.
2. Confirm secret/key paths in `./.config` under `[REAL_EXCHANGE]`, `[TEST_EXCHANGE]`, and `[REAL_HERO]`.

## Secure terminal key setup utility

Use the built-in C++ utility instead of a shell script for key enrollment:

```bash
make -C /cuwacunu/src/main/tools -j12 install-secure-key-setup
```

This installs a standalone executable at:

```bash
/cuwacunu/src/config/secure_key_setup
```

Default interactive flow (all known keys):

```bash
/cuwacunu/src/config/secure_key_setup
```

Only OpenAI key:

```bash
/cuwacunu/src/config/secure_key_setup --only openai_real
```

Exchange keys:

```bash
/cuwacunu/src/config/secure_key_setup --only exchange_real
/cuwacunu/src/config/secure_key_setup --only exchange_test
```

Skip specific targets or existing files:

```bash
/cuwacunu/src/config/secure_key_setup --skip exchange_test
/cuwacunu/src/config/secure_key_setup --skip-existing
```

The utility:
- prompts passphrase + key in hidden terminal mode,
- encrypts key material as AEAD blob,
- writes atomically with `0600` permissions.

## HERO local MCP utility

Build:

```bash
make -C /cuwacunu/src/main/hero -j12 all
```

Run as MCP server (Codex/stdio):

```bash
/cuwacunu/.build/hero/hero_config_mcp
```

Defaults:
- global config path defaults to `/cuwacunu/src/config/.config`
- config MCP does not require a passphrase at runtime

Direct one-shot tool call (no manual JSON-RPC framing):

```bash
/cuwacunu/.build/hero/hero_config_mcp \
  --tool hero.config.status \
  --args-json '{}'

/cuwacunu/.build/hero/hero_runtime_mcp \
  --tool hero.runtime.list_jobs \
  --args-json '{}'
```

Human-friendly tool discovery:

```bash
/cuwacunu/.build/hero/hero_config_mcp --list-tools
/cuwacunu/.build/hero/hero_runtime_mcp --list-tools
```

MCP `initialize` responses from HERO servers include an `instructions` string
with concise runtime prerequisites.

Find/install in Codex CLI:

```bash
# Check what MCP servers are already configured
codex mcp list

# Register HERO MCP in Codex (stdio transport)
codex mcp add hero-config -- \
  /cuwacunu/.build/hero/hero_config_mcp

codex mcp add hero-runtime -- \
  /cuwacunu/.build/hero/hero_runtime_mcp

# Inspect the registered server configuration
codex mcp get hero-config --json
```

Remove from Codex CLI:

```bash
codex mcp remove hero-config
```

Codex test calls:

```bash
codex exec -C /cuwacunu \
  "You must call MCP tool hero.config.status and return only structuredContent JSON."

codex exec -C /cuwacunu \
  "Call hero.config.diff with include_text=false and return structuredContent JSON."
```

Required runtime mode:
- `instructions/default.hero.config.dsl` must keep
  `protocol_layer[STDIO|HTTPS/SSE]:str = STDIO`.

Run legacy human REPL:

```bash
/cuwacunu/.build/hero/hero_config_mcp --tool hero.config.status
```

Useful MCP tools:
- `hero.config.status`
- `hero.config.schema`
- `hero.config.show`
- `hero.config.get`
- `hero.config.set`
- `hero.config.dsl.get` (read one key from `instructions/default.*.dsl`)
- `hero.config.dsl.set` (set one key in `instructions/default.*.dsl`)
- `hero.config.validate`
- `hero.config.diff` / `hero.config.dry_run` (preview changes before save)
- `hero.config.backups` (list snapshots)
- `hero.config.rollback` (restore latest or selected snapshot)
- `hero.config.save` (persists config, takes shared runtime lock, returns deterministic `cutover` metadata)
- `hero.config.reload`
- `hero.config.dev_nuke_reset` (developer reset of runtime dump roots, Runtime HERO jobs root, and Hero catalogs resolved from the saved global config; refuses reset while active runtime jobs exist)

Runtime MCP tools:
- `hero.runtime.start_campaign`
- `hero.runtime.list_campaigns`
- `hero.runtime.get_campaign`
- `hero.runtime.stop_campaign`
- `hero.runtime.list_jobs`
- `hero.runtime.get_job`
- `hero.runtime.stop_job`
- `hero.runtime.tail_log`
- `hero.runtime.reconcile`

Runtime HERO defaults:
- loaded from `instructions/default.hero.runtime.dsl`
- resolved through `[REAL_HERO].runtime_hero_dsl_filename`

Runtime Hero campaigns persist under `campaigns_root` by `campaign_cursor`, with
`campaign.lls`, `campaign.dsl`, and campaign-level stdout/stderr logs. Child jobs
persist under `campaigns_root/<campaign_cursor>/jobs/<job_cursor>/`, where
`job_cursor` is derived from the parent `campaign_cursor`, with `job.lls`,
`campaign.dsl`, `binding.contract.dsl`, `binding.wave.dsl`, and worker
stdout/stderr logs.
Runtime liveness is reconciled using boot id + process start ticks, not bare pid reuse.

Deterministic policy:
- `hero.config.ask` and `hero.config.fix` are disabled by design in current runtime mode.
- Config HERO edits defaults only. Existing hashimyei instances are not mutated
  by `hero.config.dsl.set`; instance revisions are handled by Hashimyei HERO lineage.
- `hero.config.dev_nuke_reset` uses the saved global config on disk, not dirty
  unsaved in-memory edits.
- `hero.config.dev_nuke_reset` fails fast while active Runtime HERO jobs or campaigns still exist under `campaigns_root`.

## Hashimyei Catalog MCP

Build:

```bash
make -C /cuwacunu/src/main/hero -j12 all
```

Installed binary:
- `/cuwacunu/.build/hero/hero_hashimyei_mcp`

Run MCP:

```bash
/cuwacunu/.build/hero/hero_hashimyei_mcp
```

Defaults:
- global config path defaults to `/cuwacunu/src/config/.config`
- catalog mode is unencrypted in MCP runtime

Direct one-shot tool call:

```bash
/cuwacunu/.build/hero/hero_hashimyei_mcp \
  --tool hero.hashimyei.list \
  --args-json '{}'
```

`store_root` and `catalog_path` are loaded from
`default.hero.hashimyei.dsl` through
`[REAL_HERO].hashimyei_hero_dsl_filename` in the global config.
Catalog encryption mode is fixed to unencrypted in MCP runtime.

MCP ingest behavior:
- startup performs an initial ingest into catalog.
- each read tool call auto-refreshes ingest (debounced).

Register in Codex:

```bash
codex mcp add hero-hashimyei -- \
  /cuwacunu/.build/hero/hero_hashimyei_mcp
```

Supported MCP tools:
- `hero.hashimyei.list`
- `hero.hashimyei.get_component_manifest`
- `hero.hashimyei.get_founding_dsl_bundle`
  - returns the stored `.hashimyei` founding DSL bundle snapshot for a
    component revision
- `hero.hashimyei.update_rank`
- `hero.hashimyei.reset_catalog`

## Lattice MCP

Build:

```bash
make -C /cuwacunu/src/main/hero -j12 all
```

Run MCP:

```bash
/cuwacunu/.build/hero/hero_lattice_mcp
```

Defaults:
- global config path defaults to `/cuwacunu/src/config/.config`
- catalog mode is unencrypted in MCP runtime

Direct one-shot tool call:

```bash
/cuwacunu/.build/hero/hero_lattice_mcp \
  --tool hero.lattice.list_views \
  --args-json '{}'
```

`store_root` and `catalog_path`
are loaded from `default.hero.lattice.dsl` through
`[REAL_HERO].lattice_hero_dsl_filename` in the global config.
Catalog encryption mode is fixed to unencrypted in MCP runtime.

If you also use lattice MCP, register it the same way:

```bash
codex mcp add hero-lattice -- \
  /cuwacunu/.build/hero/hero_lattice_mcp
```

Supported MCP tools:
- `hero.lattice.list_facts`
- `hero.lattice.get_fact`
- `hero.lattice.list_views`
- `hero.lattice.get_view`
- `hero.lattice.refresh`

## Split Policy

- Global settings live in `./.config`: `[GENERAL]`, `[GUI]`, `[BNF]`, `[REAL_EXCHANGE]`, `[TEST_EXCHANGE]`, `[REAL_HERO]`.
  `GENERAL.default_iitepi_campaign_dsl_filename` points to the default
  runtime campaign DSL file.
  Runtime reset is intentionally explicit and can be invoked through
  `/cuwacunu/.build/hero/runtime_reset` or `make -C /cuwacunu/src/main reset-runtime`.
- `[GUI]` holds iinuji defaults, currently:
  `iinuji_logs_buffer_capacity`, `iinuji_logs_show_date`,
  `iinuji_logs_show_thread`, `iinuji_logs_show_metadata`,
  `iinuji_logs_metadata_filter`, `iinuji_logs_show_color`,
  `iinuji_logs_auto_follow`, `iinuji_logs_mouse_capture`.
- HERO runtime settings for deterministic MCP live in:
  - `./instructions/default.hero.config.dsl` (Config HERO runtime policy)
  - `./instructions/default.hero.hashimyei.dsl` (Hashimyei HERO catalog defaults)
  - `./instructions/default.hero.lattice.dsl` (Lattice HERO catalog/runtime defaults)
  - `./instructions/default.hero.runtime.dsl` (Runtime HERO campaign/job defaults)
  `default.hero.hashimyei.dsl` is the Hashimyei HERO runtime defaults file. It is
  not the founding DSL bundle for component hashimyei lineage.
- `[REAL_HERO]` owns the canonical pointer paths for those four HERO DSL files:
  - `config_hero_dsl_filename`
  - `hashimyei_hero_dsl_filename`
  - `lattice_hero_dsl_filename`
  - `runtime_hero_dsl_filename`
- All grammar (`*.bnf`) paths are centralized in `[BNF]`:
  - `iitepi_campaign_grammar_filename`
  - `iitepi_runtime_binding_grammar_filename`
  - `iitepi_wave_grammar_filename`
  - `network_design_grammar_filename`
  - `vicreg_grammar_filename`
  - `value_estimation_grammar_filename`
  - `wikimyei_evaluation_embedding_sequence_analytics_grammar_filename`
  - `wikimyei_evaluation_transfer_matrix_evaluation_grammar_filename`
  - `observation_sources_grammar_filename`
  - `observation_channels_grammar_filename`
  - `jkimyei_specs_grammar_filename`
    shared grammar (`bnf/jkimyei.bnf`) for per-component jkimyei DSL files
  - `tsiemene_circuit_grammar_filename`
  - `canonical_path_grammar_filename`
- Unified campaign DSL (`./instructions/default.iitepi.campaign.dsl`) declares:
  - `CAMPAIGN { ... }` root block
  - `IMPORT_CONTRACT_FILE "<contract_defaults_file>";`
  - `IMPORT_WAVE_FILE "<wave_dsl_file>";`
  - `BIND <id> { CONTRACT = <derived_contract_id>; WAVE = <wave_id>; }`
  - ordered `RUN <bind_id>;`
  - optional bind-local variables inside `BIND`, where names must start with `__`
    and are intended for wave-local pre-decode placeholder resolution such as
    `% __sampler ? sequential %`
  - bind-local variables are operational only; they do not override
    contract-defined docking variables
  - bind-local variables may not reuse names already declared by contract
    `__variables`; shadowing is rejected during campaign snapshot staging
- Runtime Hero owns campaign dispatch and persists immutable snapshots under
  `campaigns_root/<campaign_cursor>/`. Each child job receives a staged
  `campaign.dsl`, `binding.contract.dsl`, and `binding.wave.dsl` under
  `campaigns_root/<campaign_cursor>/jobs/<job_cursor>/`.
- Internal runtime-binding snapshots are validated against
  `bnf/iitepi.runtime_binding.bnf` and follow this staged shape:
  - `ACTIVE_BIND <bind_id>;`
  - `RUNTIME_BINDING { ... }`
  - `IMPORT_CONTRACT_FILE "<contract_defaults_file>";`
  - `IMPORT_WAVE_FILE "<wave_dsl_file>";`
  - `BIND <id> { CONTRACT = <derived_contract_id>; WAVE = <wave_id>; }`
- The public dispatcher is campaign-oriented. The top-level runtime DSL is now
  `campaign.dsl`, and `jkimyei` is not a separate Hero.
- Contract settings live in `./instructions/default.iitepi.contract.dsl` with
  marker format:
  - `-----BEGIN IITEPI CONTRACT-----`
  - optional contract `__variables` for hard-static docking compatibility
  - `CIRCUIT_FILE: <path>;`
  - `AKNOWLEDGE: <alias> = <tsi family>;` (one per circuit alias)
  - `-----END IITEPI CONTRACT-----`
  `AKNOWLEDGE` values must be family tokens (no hashimyei suffix). This keeps
  contract static while wave owns runtime hashimyei selection. Component
  hashimyei lineage is contract-scoped; reusing the same component hashimyei
  across contracts is invalid. Active component selection is also
  contract-scoped, so "active" is never a global family-wide pointer.
  Family reranking is a runtime artifact concern, not a contract parameter:
  `hero.hashimyei.update_rank` persists contract-scoped
  `hero.family.rank.v1` overlays keyed by `(family, contract_hash)`, and
  `hero.lattice.get_view(view_kind=family_evaluation_report, ...)`
  serializes the family evidence used by client-owned ranking logic. Ranking
  stays inert until an explicit overlay is written; it does not bootstrap a
  default order and does not alter runtime component selection in this phase.
  Runtime derives module configuration defaults from colocated files:
  `default.tsi.wikimyei.representation.vicreg.dsl`,
  `default.tsi.wikimyei.inference.mdn.value_estimation.dsl`,
  `default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl`,
  `default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl`.
  Contract `__variables` are resolved across that contract-local DSL graph, so
  docking-critical values such as input tensor shape and embedding dimensions
  can be defined once by the contract. Changing those values changes contract
  identity and therefore hashimyei compatibility lineage.
  Runtime also derives an explicit contract docking signature from the
  compatible circuit set, contract `__variables`, and docking-bearing contract
  DSL surfaces (circuit, VICReg, VICReg network design, and contract-owned
  observation/channel DSLs when present). Component manifests persist that
  digest as `docking_signature_sha256_hex`, alongside a contract-scoped
  `lineage_state`.
  When wave reuses an existing component hashimyei, runtime validates that the
  selected component manifest belongs to the same contract and that its
  `docking_signature_sha256_hex` matches the current contract before accepting
  the load.
  When contract-owned observation/channel DSL paths are present together with a
  VICReg network design, contract validation also checks that observation
  active-channel count and max sequence length match `INPUT.C` and `INPUT.T`,
  and that contract `__obs_feature_dim` matches `INPUT.D` when declared.
  Contract circuit payload declares one or more compatible named circuits, and
  the operational selector is wave-local `CIRCUIT: <circuit_name>;`.
  If a contract exposes multiple circuits and wave omits `CIRCUIT`, runtime
  now rejects the binding instead of relying on a contract-local default.
  Circuit grammar accepts multiline hop expressions and comments:
  `/* ... */` and `# ...`.
- Wave settings are authored directly in `./instructions/default.iitepi.wave.dsl`.
  split train/run keys are removed and rejected by validation.
  wave owns operational circuit selection via `CIRCUIT: <circuit_name>;`.
  runtime dataloader ownership is wave-local via root `WAVE` keys:
    `SAMPLER`, `BATCH_SIZE`, `MAX_BATCHES_PER_EPOCH`,
    with source-owned fields split inside each `SOURCE { ... }` block:
    `SETTINGS { WORKERS, FORCE_REBUILD_CACHE, RANGE_WARN_BATCHES }`,
    `RUNTIME { SYMBOL, FROM, TO }`.
  - observation/channel DSL selection is contract-owned through
    `__observation_sources_dsl_file` and
    `__observation_channels_dsl_file`.
  - wave `WIKIMYEI { ... }` is profile policy only:
    `JKIMYEI { HALT_TRAIN, PROFILE_ID }`; jkimyei DSL payload file ownership is contract-local, and `PROFILE_ID` is only required when the contract exposes multiple compatible profiles.
  - runtime probe policy is wave-local via `PROBE { ... }` blocks:
    `TRAINING_WINDOW=incoming_batch`,
    `REPORT_POLICY=epoch_end_log`,
    `OBJECTIVE=future_target_dims_nll`.
    Probe path parity with circuit probe nodes is strict.
  - runtime sink ownership is wave-local via `SINK { ... }` blocks:
    `PATH=<canonical_sink_node_path>`.
    Sink path parity with circuit sink nodes is strict.
  - `tsi.probe.log` circuit instance mode accepts `batch` or `epoch`
    (keyed `mode/log_mode/cadence` or positional token); `event` mode is removed.
  - contract `__observation_sources_dsl_file` and
    `__observation_channels_dsl_file` now own static observation/channel
    policy selection. This keeps source symbol and date range wave-local while
    moving docking-critical observation payload selection fully into contract.
- `instructions/default.tsi.source.dataloader.sources.dsl` owns CSV lattice policy via required:
  `CSV_POLICY { CSV_BOOTSTRAP_DELTAS, CSV_STEP_ABS_TOL, CSV_STEP_REL_TOL }`.
  It also owns required source analytics policy:
  `DATA_ANALYTICS_POLICY { MAX_SAMPLES, MAX_FEATURES, MASK_EPSILON, STANDARDIZE_EPSILON }`.
  `MASK_EPSILON` is the minimum accepted valid-timestep ratio for a sample;
  accepted samples exclude invalid positions from the numeric analytics.

## Runtime Contract Lock

- Campaign, contract, and wave files are loaded into immutable runtime snapshots keyed by manifest SHA-256 hash.
- `runtime_binding.contract@init` is binding driven (`CONTRACT` + `WAVE`).
- Editing campaign/contract/wave dependencies on disk during runtime is allowed, but changes are not reloaded mid-run.
- A restart is required for edits to take effect.
- Reload/refresh boundaries perform integrity checks across all registered campaign/contract/wave snapshots; if dependencies changed on disk,
  runtime fails fast to prevent mixed state.

## Value Estimation Runtime Notes

- `VALUE_ESTIMATION.optimizer_threshold_reset` is a step-counter clamp for Adam/AdamW (not a grad-norm reset trigger).
- `ExpectedValue` scheduler stepping is driven by scheduler mode:
  - `PerBatch`: step each batch.
  - `PerEpoch`: step once per epoch.
  - `PerEpochWithMetric`: step once per epoch with epoch loss metric.
- `ExpectedValue` checkpoints are strict format v2 only and require:
  - `format_version = 2`
  - `meta/contract_hash`
  - `meta/component_name`
  - `meta/scheduler_mode`
  - `meta/scheduler_batch_steps`
  - `meta/scheduler_epoch_steps`

## VICReg Runtime Notes

- `jkimyei.*.dsl` files use explicit `COMPONENT "<canonical_type>" { ... }` blocks; implicit component identity is removed and `component_id` is derived from canonical type.
- Contract-owned VICReg jkimyei payload is bound via `jkimyei_dsl_file` inside
  `default.tsi.wikimyei.representation.vicreg.dsl`.
- `VICReg.swa_start_iter` and `VICReg.optimizer_threshold_reset` are profile policy keys owned by `default.tsi.wikimyei.representation.vicreg.jkimyei.dsl` (`[COMPONENT_PARAMS]`), not by `default.tsi.wikimyei.representation.vicreg.dsl`.
- VICReg train/eval enable is owned by wave `WIKIMYEI ... JKIMYEI.HALT_TRAIN` together with root `WAVE.MODE` train bit; `default.tsi.wikimyei.representation.vicreg.jkimyei.dsl` no longer defines a `vicreg_train` key.
- Embedding-sequence sidecars are now owned by
  `default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl`, not by
  silent `piaabo` defaults. The module config controls
  `max_samples`, `max_features`, `mask_epsilon`, and `standardize_epsilon`
  while runtime `debug_enabled` still gates whether the sidecars are emitted.
- `network_design` is the naming for graph/node architecture payloads.
  Optional path binding is configured by `network_design_dsl_file` inside
  `default.tsi.wikimyei.representation.vicreg.dsl`.
  Decoder layer is framework-agnostic; semantic validation is Wikimyei-owned;
  LibTorch-facing mapping is performed in `piaabo/torch_compat`.
  For checkpoint-side network analytics sidecars, declare exactly one
  `node ...@NETWORK_ANALYTICS_POLICY` in `network_design.dsl` with strict keys:
  `near_zero_epsilon`, `log10_abs_histogram_bins`, `log10_abs_histogram_min`,
  `log10_abs_histogram_max`, `include_buffers`, `enable_spectral_metrics`,
  `spectral_max_elements`, `anomaly_top_k`.
