# Config Folder Layout

This folder is organized by role:

- `./.config`: global runtime settings (exchange, seeds, UI/system knobs).
- `./instructions/default.iitepi.contract.dsl`: board contract defaults file.
- `./bnf/`: grammar files (`*.bnf`).
- `./instructions/`: DSL payload files (`*.dsl`).
- `./secrets/real/`: real exchange secret material.
- `./secrets/test/`: test exchange secret material.

## Current file map

- `bnf/tsi.source.dataloader.sources.bnf`
- `bnf/tsi.source.dataloader.channels.bnf`
- `bnf/jkimyei.bnf`
- `bnf/iitepi.contract.bnf`
- `bnf/iitepi.contract.circuit.bnf`
- `bnf/iitepi.wave.bnf`
- `bnf/network_design.bnf`
- `bnf/canonical_path.bnf`
- `bnf/latent_lineage_state.bnf`
- `.config`
- `instructions/default.iitepi.contract.dsl`
- `instructions/default.tsi.source.dataloader.sources.dsl`
- `instructions/default.tsi.source.dataloader.channels.dsl`
- `instructions/default.tsi.wikimyei.representation.vicreg.jkimyei.dsl`
- `instructions/default.iitepi.board.dsl`
- `instructions/default.iitepi.contract.circuit.dsl`
- `instructions/default.iitepi.wave.dsl` (canonical wave payload set)
- `instructions/default.tsi.wikimyei.representation.vicreg.dsl`
- `instructions/default.tsi.wikimyei.representation.vicreg.network_design.dsl`
- `instructions/default.tsi.wikimyei.inference.mdn.value_estimation.dsl`
- `instructions/default.tsi.wikimyei.inference.transfer_matrix_evaluation.dsl`
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
/cuwacunu/.build/hero/hero_config_mcp --repl
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

Deterministic policy:
- `hero.config.ask` and `hero.config.fix` are disabled by design in current runtime mode.
- Config HERO edits defaults only. Existing hashimyei instances are not mutated
  by `hero.config.dsl.set`; instance revisions are handled by Hashimyei HERO lineage.

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
- `hero.hashimyei.get_founding_dsl`
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
  --tool hero.lattice.get_runs \
  --args-json '{}'
```

`store_root`, `catalog_path`, `hashimyei_catalog_path`, and `config_folder`
are loaded from `default.hero.lattice.dsl` through
`[REAL_HERO].lattice_hero_dsl_filename` in the global config.
Catalog encryption mode is fixed to unencrypted in MCP runtime.

If you also use lattice MCP, register it the same way:

```bash
codex mcp add hero-lattice -- \
  /cuwacunu/.build/hero/hero_lattice_mcp
```

Supported MCP tools:
- `hero.lattice.get_runs`
- `hero.lattice.list_report_fragments`
- `hero.lattice.get_report_lls`
- `hero.lattice.reset_catalog`

## Split Policy

- Global settings live in `./.config`: `[GENERAL]`, `[BNF]`, `[REAL_EXCHANGE]`, `[TEST_EXCHANGE]`, `[REAL_HERO]`.
  `GENERAL.board_config_filename` points to the board DSL file and
  `GENERAL.board_binding_id` selects the active `BIND` entry.
- HERO runtime settings for deterministic MCP live in:
  - `./instructions/default.hero.config.dsl` (Config HERO runtime policy)
  - `./instructions/default.hero.hashimyei.dsl` (Hashimyei HERO catalog defaults)
  - `./instructions/default.hero.lattice.dsl` (Lattice HERO catalog/runtime defaults)
- `[REAL_HERO]` owns the canonical pointer paths for those three HERO DSL files:
  - `config_hero_dsl_filename`
  - `hashimyei_hero_dsl_filename`
  - `lattice_hero_dsl_filename`
- All grammar (`*.bnf`) paths are centralized in `[BNF]`:
  - `iitepi_board_grammar_filename`
  - `iitepi_wave_grammar_filename`
  - `network_design_grammar_filename`
  - `vicreg_grammar_filename`
  - `value_estimation_grammar_filename`
  - `wikimyei_inference_transfer_matrix_evaluation_grammar_filename`
  - `observation_sources_grammar_filename`
  - `observation_channels_grammar_filename`
  - `jkimyei_specs_grammar_filename`
    shared grammar (`bnf/jkimyei.bnf`) for per-component jkimyei DSL files
  - `tsiemene_circuit_grammar_filename`
  - `canonical_path_grammar_filename`
- Board DSL (`./instructions/default.iitepi.board.dsl`) declares:
  - `BOARD { ... }` root block
  - `IMPORT_CONTRACT_FILE "<contract_defaults_file>";`
  - `IMPORT_WAVE_FILE "<wave_dsl_file>";`
  - `BIND <id> { CONTRACT = <derived_contract_id>; WAVE = <wave_id>; }`
- Legacy wrapper files `default.board.config` and `default.wave.config` are removed.
  Use direct DSL paths in `GENERAL.board_config_filename` and board `IMPORT_WAVE_FILE`.
- Contract settings live in `./instructions/default.iitepi.contract.dsl` with
  marker format:
  - `-----BEGIN IITEPI CONTRACT-----`
  - `CIRCUIT_FILE: <path>;`
  - `AKNOWLEDGE: <alias> = <tsi family>;` (one per circuit alias)
  - `-----END IITEPI CONTRACT-----`
  `AKNOWLEDGE` values must be family tokens (no hashimyei suffix). This keeps
  contract static while wave owns runtime hashimyei selection.
  Runtime derives module configuration defaults from colocated files:
  `default.tsi.wikimyei.representation.vicreg.dsl`,
  `default.tsi.wikimyei.inference.mdn.value_estimation.dsl`,
  `default.tsi.wikimyei.inference.transfer_matrix_evaluation.dsl`.
  Circuit payload can declare optional `active_circuit = <circuit_name>`;
  when set, runtime builds/runs only that circuit. Circuit grammar accepts
  multiline hop expressions and comments: `/* ... */` and `# ...`.
- Wave settings are authored directly in `./instructions/default.iitepi.wave.dsl`.
  split train/run keys are removed and rejected by validation.
  runtime dataloader ownership is wave-local via root `WAVE` keys:
    `SAMPLER`, `BATCH_SIZE`, `MAX_BATCHES_PER_EPOCH`,
    with source-owned fields split inside each `SOURCE { ... }` block:
    `SETTINGS { WORKERS, SAMPLER, FORCE_REBUILD_CACHE, RANGE_WARN_BATCHES }`,
    `RUNTIME { SYMBOL, FROM, TO, SOURCES_DSL_FILE, CHANNELS_DSL_FILE }`.
  - wave `WIKIMYEI { ... }` is profile policy only:
    `JKIMYEI { HALT_TRAIN, PROFILE_ID }`; jkimyei DSL payload file ownership is contract-local.
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
  - `SOURCE.RUNTIME.SOURCES_DSL_FILE` and `SOURCE.RUNTIME.CHANNELS_DSL_FILE` have no contract fallback.
- `instructions/default.tsi.source.dataloader.sources.dsl` owns CSV lattice policy via required:
  `CSV_POLICY { CSV_BOOTSTRAP_DELTAS, CSV_STEP_ABS_TOL, CSV_STEP_REL_TOL }`.
  It also owns required source analytics policy:
  `DATA_ANALYTICS_POLICY { MAX_SAMPLES, MAX_FEATURES, MASK_EPSILON, STANDARDIZE_EPSILON }`.

## Runtime Contract Lock

- Board, contract, and wave files are loaded into immutable runtime snapshots keyed by manifest SHA-256 hash.
- `board.contract@init` is board-binding driven (`CONTRACT` + `WAVE`).
- Editing board/contract/wave dependencies on disk during runtime is allowed, but changes are not reloaded mid-run.
- A restart is required for edits to take effect.
- Reload/refresh boundaries perform integrity checks across all registered board/contract/wave snapshots; if dependencies changed on disk,
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
