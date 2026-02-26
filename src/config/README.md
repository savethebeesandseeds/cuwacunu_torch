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
- `dsl/canonical_path.bnf`
- `dsl/test_dsl.bnf` (scratch/experimental)
- `.config`
- `default.board.contract.config`
- `instructions/observation_sources.dsl`
- `instructions/observation_channels.dsl`
- `instructions/jkimyei_specs.dsl`
- `instructions/tsiemene_circuit.dsl`
- `instructions/wikimyei_vicreg.ini`
- `instructions/wikimyei_value_estimation.ini`
- `secrets/real/aes_salt.enc`
- `secrets/real/ed25519key.pem` (expected, may be absent locally)
- `secrets/real/exchange.key` (expected, may be absent locally)
- `secrets/test/aes_salt.enc`
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

## Configure API key and salt

1. Ensure `secrets/real/aes_salt.enc` exists.
2. Put plaintext API key into `secrets/real/exchange.key`.
3. Confirm paths in `./.config` under `[REAL_EXCHANGE]` and `[TEST_EXCHANGE]`.

## Split Policy

- Global settings live in `./.config`: `[GENERAL]`, `[DATA_LOADER]`, `[REAL_EXCHANGE]`, `[TEST_EXCHANGE]`.
  `GENERAL.board_contract_config_filename` points to the board contract file.
- Board contract settings live in `./default.board.contract.config`: `[SPECS]`, `[DSL]`.
  `SPECS.vicreg_config_filename` and `SPECS.value_estimation_config_filename`
  point to module INI files under `./instructions/`.

## Runtime Contract Lock

- The board contract is loaded into an immutable runtime snapshot at process bootstrap.
- Editing contract files on disk during runtime is allowed, but changes are not reloaded mid-run.
- A restart is required for contract edits to take effect.
- Reload/refresh boundaries perform integrity checks; if contract dependencies changed on disk,
  runtime fails fast to prevent mixed contract state.
