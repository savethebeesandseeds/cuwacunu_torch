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
