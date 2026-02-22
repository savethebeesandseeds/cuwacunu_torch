# Config Folder Layout

This folder is now organized by role:

- `./.config`: main runtime config file.
- `./bnf/`: grammar files (`*.bnf`).
- `./instructions/`: instruction samples (`*.instruction`).
- `./secrets/real/`: real exchange secret material.
- `./secrets/test/`: test exchange secret material.

## Current file map

- `bnf/observation_pipeline.bnf`
- `bnf/training_components.bnf`
- `bnf/tsiemene_board.bnf`
- `bnf/canonical_path.bnf`
- `bnf/test_bnf.bnf` (scratch/experimental)
- `instructions/observation_pipeline.instruction`
- `instructions/training_components.instruction`
- `instructions/tsiemene_board.instruction`
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
