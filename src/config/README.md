# Config Folder Layout

This folder is organized by role:

- `./.config`: global runtime settings (exchange, seeds, UI/system knobs).
- `./default.board.contract.config`: board contract config file.
- `./bnf/`: grammar files (`*.bnf`).
- `./instructions/`: DSL payload files (`*.dsl`).
- `./secrets/real/`: real exchange secret material.
- `./secrets/test/`: test exchange secret material.

## Current file map

- `bnf/tsi.source.dataloader.sources.bnf`
- `bnf/tsi.source.dataloader.channels.bnf`
- `bnf/jkimyei.bnf`
- `bnf/iitepi.contract.circuit.bnf`
- `bnf/iitepi.wave.bnf`
- `bnf/network_design.bnf`
- `bnf/canonical_path.bnf`
- `bnf/key_value.bnf`
- `.config`
- `default.board.contract.config`
- `instructions/tsi.source.dataloader.sources.dsl`
- `instructions/tsi.source.dataloader.channels.dsl`
- `instructions/jkimyei.representation.vicreg.dsl`
- `instructions/iitepi.board.dsl`
- `instructions/iitepi.contract.circuit.example.dsl`
- `instructions/iitepi.wave.example.dsl` (canonical wave payload set)
- `instructions/tsi.wikimyei.representation.vicreg.dsl`
- `instructions/tsi.wikimyei.representation.vicreg.network_design.dsl`
- `instructions/tsi.wikimyei.inference.mdn.value_estimation.dsl`
- `instructions/tsi.probe.representation.transfer_matrix_evaluation.dsl`
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

Build/install:

```bash
make -C /cuwacunu/src/main/hero -j12 install-hero-mcp
```

Run as MCP server (Codex/stdio):

```bash
/cuwacunu/src/config/hero_config_mcp
```

Find/install in Codex CLI:

```bash
# Check what MCP servers are already configured
codex mcp list

# Register HERO MCP in Codex (stdio transport)
codex mcp add hero-config -- \
  /cuwacunu/src/config/hero_config_mcp \
  --config /cuwacunu/src/config/instructions/hero.config.dsl

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
  "You must call MCP tool hero.status and return only structuredContent JSON."

codex exec -C /cuwacunu \
  "Call hero.diff with include_text=false and return structuredContent JSON."
```

Required runtime mode:
- `instructions/hero.config.dsl` must keep `protocol_layer:str = STDIO`.

Run legacy human REPL:

```bash
/cuwacunu/src/config/hero_config_mcp --repl
```

Useful MCP tools:
- `hero.status`
- `hero.schema`
- `hero.show`
- `hero.get`
- `hero.set`
- `hero.validate`
- `hero.diff` / `hero.dry_run` (preview changes before save)
- `hero.backups` (list snapshots)
- `hero.rollback` (restore latest or selected snapshot)
- `hero.save`
- `hero.reload`

Deterministic policy:
- `hero.ask` and `hero.fix` are disabled by design in current runtime mode.

## Hashimyei Catalog MCP

Build/install:

```bash
make -C /cuwacunu/src/main/hero -j12 install-hashimyei-tools
```

Installed binaries:
- `/cuwacunu/src/config/hero_hashimyei_ingest`
- `/cuwacunu/src/config/hero_hashimyei_mcp`

Initialize catalog from artifacts:

```bash
/cuwacunu/src/config/hero_hashimyei_ingest \
  --store-root /cuwacunu/.hashimyei
```

Run MCP:

```bash
/cuwacunu/src/config/hero_hashimyei_mcp \
  --store-root /cuwacunu/.hashimyei
```

Supported MCP tools:
- `hero.hashimyei.scan`
- `hero.hashimyei.latest`
- `hero.hashimyei.history`
- `hero.hashimyei.performance`
- `hero.hashimyei.provenance`

## Split Policy

- Global settings live in `./.config`: `[GENERAL]`, `[BNF]`, `[REAL_EXCHANGE]`, `[TEST_EXCHANGE]`, `[REAL_HERO]`.
  `GENERAL.board_config_filename` points to the board config file and
  `GENERAL.board_binding_id` selects the active `BIND` entry.
- HERO runtime settings for deterministic MCP live in
  `./instructions/hero.config.dsl` (`protocol_layer`, local read/write policy,
  backup policy).
- `[REAL_HERO]` remains as legacy smart-mode wiring and is currently not used by
  deterministic HERO MCP execution.
- All grammar (`*.bnf`) paths are centralized in `[BNF]`:
  - `iitepi_board_grammar_filename`
  - `iitepi_wave_grammar_filename`
  - `network_design_grammar_filename`
  - `vicreg_grammar_filename`
  - `value_estimation_grammar_filename`
  - `transfer_matrix_evaluation_grammar_filename`
  - `observation_sources_grammar_filename`
  - `observation_channels_grammar_filename`
  - `jkimyei_specs_grammar_filename`
    shared grammar (`bnf/jkimyei.bnf`) for per-component jkimyei DSL files
  - `tsiemene_circuit_grammar_filename`
  - `canonical_path_grammar_filename`
- Board settings live in `./default.board.config`: `[DSL]`.
  - `iitepi_board_dsl_filename`
- Board DSL (`./instructions/iitepi.board.dsl`) declares:
  - `BOARD { ... }` root block
  - `IMPORT_CONTRACT_FILE "<contract_config_file>";`
  - `IMPORT_WAVE_FILE "<wave_config_file>";`
  - `BIND <id> { CONTRACT = <derived_contract_id>; WAVE = <wave_id>; }`
- Contract settings live in `./default.board.contract.config`: `[SPECS]`, `[DSL]`.
  `SPECS.vicreg_config_filename` points to the VICReg module key-value DSL file.
  `SPECS.vicreg_network_design_filename` is optional and carries neutral node/graph design payload.
  `SPECS.value_estimation_config_filename` is optional while value-estimation
  is not part of the active contract.
  `SPECS.transfer_matrix_evaluation_config_filename` is optional and enables
  probe-local transfer-matrix trainer policy (optimizer + NLL numerics + diagnostics).
  `BOARD_CONTRACT_DSL.*_dsl_text` inline payload keys are removed; contract payloads are file-path only.
  `[DSL]` should provide:
  - primary per-component jkimyei payload: `jkimyei_specs_dsl_filename`
  - optional extra per-component jkimyei payloads: `jkimyei_specs_extra_dsl_filenames`
  - canonical circuit payload: `tsiemene_circuit_dsl_filename`
  - circuit payload can declare optional `active_circuit = <circuit_name>`;
    when set, runtime builds/runs only that circuit
  - circuit grammar accepts multiline hop expressions and comments:
    `/* ... */` and `# ...`
- Wave settings live in `./default.wave.config`: `[DSL]`.
  - `iitepi_wave_dsl_filename`
  - split train/run keys are removed and rejected by validation.
  - runtime dataloader ownership is wave-local via root `WAVE` keys:
    `SAMPLER`, `BATCH_SIZE`, `MAX_BATCHES_PER_EPOCH`,
    with source-owned dataloader fields inside each `SOURCE { ... }` block:
    `WORKERS`, `FORCE_REBUILD_CACHE`, `RANGE_WARN_BATCHES`,
    `SOURCES_DSL_FILE`, `CHANNELS_DSL_FILE`.
  - runtime probe policy is wave-local via `PROBE { ... }` blocks:
    `TRAINING_WINDOW=incoming_batch`,
    `REPORT_POLICY=epoch_end_log`,
    `OBJECTIVE=future_target_dims_nll`.
    Probe path parity with circuit probe nodes is strict.
  - `tsi.probe.log` circuit instance mode accepts `batch` or `epoch`
    (keyed `mode/log_mode/cadence` or positional token); `event` mode is removed.
  - `SOURCE.SOURCES_DSL_FILE` and `SOURCE.CHANNELS_DSL_FILE` have no contract fallback.
- `instructions/tsi.source.dataloader.sources.dsl` owns CSV lattice policy via required:
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
- Loaded `jkimyei.*.dsl` files must have unique `canonical_type` values; duplicates are rejected at contract decode time.
- `VICReg.swa_start_iter` and `VICReg.optimizer_threshold_reset` are profile policy keys owned by `jkimyei.representation.vicreg.dsl` (`[COMPONENT_PARAMS]`), not by `tsi.wikimyei.representation.vicreg.dsl`.
- VICReg train/eval enable is owned by wave `WIKIMYEI ... TRAIN`; `jkimyei.representation.vicreg.dsl` no longer defines a `vicreg_train` key.
- `network_design` is the naming for graph/node architecture payloads.
  Decoder layer is framework-agnostic; semantic validation is Wikimyei-owned;
  LibTorch-facing mapping is performed in `piaabo/torch_compat`.
  For checkpoint-side network analytics sidecars, declare exactly one
  `node ...@NETWORK_ANALYTICS_POLICY` in `network_design.dsl` with strict keys:
  `near_zero_epsilon`, `log10_abs_histogram_bins`, `log10_abs_histogram_min`,
  `log10_abs_histogram_max`, `include_buffers`, `enable_spectral_metrics`,
  `spectral_max_elements`, `anomaly_top_k`.
