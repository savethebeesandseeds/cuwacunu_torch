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

`src/scripts/cuwacunu_bashrc.sh` is the repo-owned bashrc extension. It keeps
the cuwacunu shell behavior centralized by:
- adding `/cuwacunu/.build/hero` and `/cuwacunu/.build/tools` to `PATH`
- rendering the shell status marks `[*]` and `[!]`

`src/scripts/hero_human_prompt_mark.sh` is the tiny shell status-mark helper for
the Human Hero pending marker. It reads the hard-coded runtime file
`/cuwacunu/.runtime/.human_hero/awaiting_human.pending.count` and also checks
`/cuwacunu/.runtime/.human_hero/finished.pending.count`. It prints `[*]` when
at least one Super Hero loop is paused in `awaiting_human` or at least one
finished-loop summary report still needs Human Hero acknowledgment.

`src/scripts/tsodao_sync_mark.sh` is the matching shell status-mark helper for
TSODAO sync attention. It asks the TSODAO binary for `prompt-mark` and prints
`[!]` when the hidden surface needs a safe `tsodao sync` or when TSODAO refuses
to guess direction.

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

`src/main/tools/tsodao.cpp` builds the canonical TSODAO surface guard. Its first
component is the hidden/public policy declared in
`src/config/instructions/defaults/tsodao.dsl`, which currently points at
`src/config/instructions/optim/` as the hidden plaintext surface and
`src/config/instructions/optim.tar.gpg` as the tracked encrypted archive. TSODAO
also writes a local sync-state file under `.runtime/.tsodao/` so `sync` can be
safe-by-default instead of guessing direction.

Examples:
```bash
/cuwacunu/.build/tools/tsodao status
/cuwacunu/.build/tools/tsodao init
/cuwacunu/.build/tools/tsodao sync
/cuwacunu/.build/tools/tsodao sync --to-archive
/cuwacunu/.build/tools/tsodao sync --to-hidden
/cuwacunu/.build/tools/tsodao prompt-mark
bash src/scripts/setup_tsodao_key.sh
bash src/scripts/install_git_hooks.sh
/cuwacunu/.build/tools/tsodao seal --symmetric
/cuwacunu/.build/tools/tsodao seal --recipient alice@example.com
/cuwacunu/.build/tools/tsodao scrub
/cuwacunu/.build/tools/tsodao open
```

Dependency note:
- `gpg`, `tar`, and `sha256sum` are required.
- `seal` defaults to symmetric AES-256 encryption and can instead target a
  specific GPG recipient with `--recipient`.
- `tsodao init` is the preferred workflow when you want commits to
  auto-refresh the hidden archive through repo git hooks.
- `sync` is the normal operator command. With no flags it chooses a direction
  only when that direction is provably safe.
- `sync --to-archive` means plaintext hidden root -> encrypted archive.
- `sync --to-hidden` means encrypted archive -> plaintext hidden root.
- `sync` is safety-first: if both plaintext and archive exist but TSODAO cannot
  prove which one this machine last synced against, it refuses instead of
  overwriting either side.
- archive-to-plaintext restore over existing hidden plaintext asks for
  confirmation unless `--yes` is passed.

`src/scripts/setup_tsodao_key.sh` is a compatibility wrapper to `tsodao init`.
The binary provisions or validates the GPG recipient used by `tsodao.dsl`,
writes the recipient into that DSL, and installs repo git hooks unless
`--skip-hooks` is set.

Examples:
```bash
bash src/scripts/setup_tsodao_key.sh
bash src/scripts/setup_tsodao_key.sh --uid "Cuwacunu TSODAO <alice@example.com>"
bash src/scripts/setup_tsodao_key.sh --validate
```

`src/scripts/install_git_hooks.sh` enables the tracked `.githooks/` directory
as the repository hook path so commit and push use the TSODAO guardrails.

Build/install:

```bash
make -C /cuwacunu/src/main/tools -j12 build-tsodao
/cuwacunu/.build/tools/tsodao init
/cuwacunu/.build/tools/tsodao sync
```

Optional in-place finalize:
```bash
make -C /cuwacunu/src/main/tools -j12 install-tsodao
/cuwacunu/.build/tools/tsodao sync
```

Aggregator note:
- Daily build use should stay on `lib`, `link`, and `install`:
  `cd /cuwacunu && make -j12 lib`, `cd /cuwacunu && make -j12 link`,
  `cd /cuwacunu && make -j12 install`, `make -C /cuwacunu/src lib`,
  `make -C /cuwacunu/src link`, `make -C /cuwacunu/src install`.
- `make -C /cuwacunu/src/main all` is an advanced/internal direct-main
  entrypoint; it also finalizes the installable HERO and tool surfaces.
- TSODAO install is quiet when the installed binary is already identical;
  `[TOOL: INSTALLED]` only appears when the `.build/tools` binary actually
  changes mode into its finalized `700` state.
- HERO `install-*-hero` targets under `src/main/hero` are specialized
  internal surfaces, not the normal daily build entrypoints.

Compatibility note:
- `src/scripts/setup_tsodao_key.sh` remains as a thin compatibility wrapper to `tsodao init`.
- `src/scripts/optim_bundle.sh` and `src/scripts/setup_optim_bundle_key.sh`
  remain as thin wrappers to the TSODAO entry points.
