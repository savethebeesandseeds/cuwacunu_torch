# Script Utilities

`src/scripts/sync_binance_klines.sh` is the data-ingest entry point for Binance
spot monthly kline archives. It replaces the old one-off download, unzip,
missing-file, and CSV-join helpers that used to live in `.data/scripts/`.

What it does:
- downloads monthly `.zip` archives and `.CHECKSUM` files into
  `.data/raw/<SYMBOL>/<INTERVAL>/<YYYY>/<MM>/`
- verifies SHA-256 checksums before extraction
- extracts monthly CSV files in place
- rebuilds yearly rollups and the loader-facing
  `<SYMBOL>-<INTERVAL>-all-years.csv` files

Useful examples:
```bash
bash src/scripts/sync_binance_klines.sh \
  --symbol BTCUSDT \
  --vicreg-solo \
  --from 2020-01 \
  --to 2024-08
```

```bash
bash src/scripts/sync_binance_klines.sh \
  --symbol ETHUSDT \
  --interval 1w,3d,1d \
  --from 2020-01 \
  --to 2024-08
```

```bash
bash src/scripts/sync_binance_klines.sh \
  --symbol BTCUSDT \
  --rebuild-only
```

`src/scripts/list_binance_spot_symbols.sh` is the lightweight discovery helper
that remains from the old script set. It queries Binance spot exchange info and
supports basic filtering without needing `jq`.

Examples:
```bash
bash src/scripts/list_binance_spot_symbols.sh --quote USDT
bash src/scripts/list_binance_spot_symbols.sh --base BTC --symbols-only
```

`src/scripts/setup_human_operator.sh` bootstraps or validates the Human Hero
operator signing surface. It works with the current runtime contract:
- `operator_id` in `default.hero.human.dsl`
- unencrypted OpenSSH `ssh-ed25519` private key at `operator_signing_ssh_identity`
- matching public-key registration in `human_operator_identities`

Default setup:
```bash
bash /cuwacunu/setup.sh
bash src/scripts/setup_human_operator.sh
```

Explicit operator id:
```bash
bash src/scripts/setup_human_operator.sh --operator-id alice@workstation
```

Force replacement prompts to yes:
```bash
bash src/scripts/setup_human_operator.sh --yes
```

Validate only:
```bash
bash src/scripts/setup_human_operator.sh --validate
```

One important note: this script currently manages an unencrypted OpenSSH
identity because that is what Human Hero signs with today. If we later move the
operator identity into encrypted-at-rest secret storage, this script should
evolve with that runtime contract.

Dependency note:
- `ssh-keygen` is required.
- `/cuwacunu/setup.sh` now installs it through the `openssh-client` package.
- If an operator identity already exists, the script asks whether to replace it
  with a fresh keypair; use `--yes` to auto-accept that replacement prompt.
