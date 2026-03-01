# Config Folder Layout

This folder is organized by role:

- `./.config`: global runtime settings (exchange, seeds, UI/system knobs).
- `./default.board.contract.config`: board contract config file.
- `./dsl/`: grammar files (`*.bnf`).
- `./instructions/`: DSL and module config files (`*.dsl`, `*.ini`).
- `./secrets/real/`: real exchange secret material.
- `./secrets/test/`: test exchange secret material.

## Current file map

- `dsl/observation_sources.bnf`
- `dsl/observation_channels.bnf`
- `dsl/jkimyei_specs.bnf`
- `dsl/tsiemene_circuit.bnf`
- `dsl/tsiemene_wave.bnf`
- `dsl/canonical_path.bnf`
- `dsl/test_dsl.bnf` (scratch/experimental)
- `.config`
- `default.board.contract.config`
- `instructions/observation_sources.dsl`
- `instructions/observation_channels.dsl`
- `instructions/jkimyei_specs.dsl`
- `instructions/iitepi_board.dsl`
- `instructions/iitepi_circuit.dsl`
- `instructions/iitepi_wave.dsl` (canonical wave payload set)
- `instructions/wikimyei_vicreg.ini`
- `instructions/wikimyei_value_estimation.ini`
- `secrets/real/ed25519key.pem` (expected, may be absent locally)
- `secrets/real/exchange.key` (expected, may be absent locally)
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
2. Confirm secret/key paths in `./.config` under `[REAL_EXCHANGE]` and `[TEST_EXCHANGE]`.

## Split Policy

- Global settings live in `./.config`: `[GENERAL]`, `[DATA_LOADER]`, `[REAL_EXCHANGE]`, `[TEST_EXCHANGE]`.
  `GENERAL.board_config_filename` points to the board config file and
  `GENERAL.board_binding_id` selects the active `BIND` entry.
- Board settings live in `./default.board.config`: `[DSL]`.
  - `tsiemene_board_grammar_filename`
  - `tsiemene_board_dsl_filename`
- Board DSL (`./instructions/iitepi_board.dsl`) declares:
  - `BOARD { ... }` root block
  - `IMPORT_CONTRACT_FILE "<contract_config_file>";`
  - `IMPORT_WAVE_FILE "<wave_config_file>";`
  - `BIND <id> { CONTRACT = <derived_contract_id>; WAVE = <wave_id>; }`
- Contract settings live in `./default.board.contract.config`: `[SPECS]`, `[DSL]`.
  `SPECS.vicreg_config_filename` points to the VICReg module INI file.
  `SPECS.value_estimation_config_filename` is optional while value-estimation
  is not part of the active contract.
  `[DSL]` should provide:
  - canonical circuit payload: `tsiemene_circuit_dsl_filename`
  - canonical path grammar: `canonical_path_grammar_filename`
- Wave settings live in `./default.wave.config`: `[DSL]`.
  - `tsiemene_wave_grammar_filename`
  - `tsiemene_wave_dsl_filename`
  - split train/run keys are removed and rejected by validation.

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
