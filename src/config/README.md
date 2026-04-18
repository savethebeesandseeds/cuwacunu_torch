# Config Folder Layout

This folder is organized by role:

- `./.config`: global runtime settings (exchange, seeds, UI/system knobs).
- `./instructions/defaults/`: canonical example/default DSL bundle.
- `./instructions/defaults/tsodao.dsl`: TSODAO hidden-surface policy; current first surface is `./instructions/optim/`.
- `./instructions/objectives/`: objective-scoped experiment bundles.
- `./instructions/optim/`: local plaintext workspace for sensitive optim DSL files.
- `./instructions/optim.tar.gpg`: tracked encrypted backup for `./instructions/optim/` when sealed.
- `./bnf/`: grammar files (`*.bnf`).
- `./secrets/real/`: real exchange secret material.
- `./secrets/test/`: test exchange secret material.

## Current file map

- `bnf/tsi.source.dataloader.sources.bnf`
- `bnf/tsi.source.dataloader.channels.bnf`
- `bnf/jkimyei.bnf`
- `bnf/iitepi.campaign.bnf`
- `bnf/iitepi.contract.bnf`
- `bnf/iitepi.circuit.bnf`
- `bnf/iitepi.wave.bnf`
- `bnf/objective.marshal.bnf`
- `bnf/network_design.bnf`
- `bnf/canonical_path.bnf`
- `bnf/latent_lineage_state.authored.bnf`
- `bnf/latent_lineage_state.runtime.bnf`
- `.config`
- `instructions/defaults/default.iitepi.contract.dsl`
- `instructions/defaults/default.tsi.source.dataloader.sources.dsl`
- `instructions/defaults/default.tsi.source.dataloader.channels.dsl`
- `instructions/defaults/default.tsi.wikimyei.representation.vicreg.jkimyei.dsl`
- `instructions/defaults/default.iitepi.campaign.dsl`
- `instructions/defaults/default.iitepi.circuit.dsl`
- `instructions/defaults/default.iitepi.wave.dsl` (canonical wave payload set)
- `instructions/defaults/default.tsi.wikimyei.representation.vicreg.dsl`
- `instructions/defaults/default.tsi.wikimyei.representation.vicreg.network_design.dsl`
- `instructions/defaults/default.tsi.wikimyei.inference.mdn.expected_value.dsl`
- `instructions/defaults/default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl`
- `instructions/defaults/default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl` (shared transfer-matrix evaluator knobs; VICReg summaries may compare null, stats-only, raw-surface, and learned-embedding forecast probes when the active build supports them)
- `instructions/defaults/default.hero.config.dsl`
- `instructions/defaults/default.hero.hashimyei.dsl`
- `instructions/defaults/default.hero.lattice.dsl`
- `instructions/defaults/default.hero.human.dsl`
- `instructions/defaults/default.hero.marshal.dsl`
- `instructions/defaults/default.hero.runtime.dsl`
- `instructions/defaults/tsodao.dsl` (TSODAO hidden-surface policy; first surface currently maps to `instructions/optim/`)
- `instructions/defaults/default.marshal.objective.dsl`
- `instructions/defaults/default.marshal.objective.md`
- `instructions/defaults/default.marshal.guidance.md` (shared Marshal Hero planning guidance; objectives may further tighten validation/test and baseline-comparison rules)
- `instructions/objectives/vicreg.solo/iitepi.contract.base.dsl`
- `instructions/objectives/vicreg.solo/iitepi.waves.dsl`
- `instructions/objectives/vicreg.solo/iitepi.campaign.dsl`
- `instructions/objectives/vicreg.solo/vicreg.solo.marshal.dsl`
- `instructions/objectives/vicreg.solo/vicreg.solo.objective.md`
- `instructions/objectives/vicreg.solo/tsi.source.dataloader.channels.dsl` (objective-owned big-span observation profile)
- `instructions/objectives/vicreg.solo/tsi.wikimyei.representation.vicreg.dsl` (objective-local VICReg runtime wrapper over objective-owned network design and jkimyei payload bindings)
- `instructions/optim/README.md` (public note for the local-only plaintext optim workspace)
- `instructions/optim.tar.gpg` (optional tracked encrypted backup for `instructions/optim/`)
- `secrets/real/ed25519key.pem` (expected, may be absent locally)
- `secrets/real/exchange.key` (expected, may be absent locally)
- `secrets/real/openai.key` (expected for HERO/OpenAI, may be absent locally)
- `secrets/test/ed25519key.pem`
- `secrets/test/ed25519pub.pem`
- `secrets/test/exchange.key`

## Default runtime roots

- Runtime campaign state defaults to `/cuwacunu/.runtime/.campaigns`.
- Hashimyei and Lattice default to the shared runtime store
  `/cuwacunu/.runtime/.hashimyei`.
- Hashimyei and Lattice MCP tools automatically rebuild their catalogs when the
  catalog-visible store surface changes.

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

## TSODAO Hidden Surface

Recommended workflow:

```bash
make -C /cuwacunu/src/main/tools -j12 build-tsodao
/cuwacunu/.build/tools/tsodao status
/cuwacunu/.build/tools/tsodao init
```

Or as a built tool:

```bash
make -C /cuwacunu/src/main/tools -j12 build-tsodao
/cuwacunu/.build/tools/tsodao init
/cuwacunu/.build/tools/tsodao sync
```

`src/config/instructions/defaults/tsodao.dsl` is the source of truth for the
first TSODAO component: a hidden/public surface rule. Right now it declares:
- `instructions/optim/` as the hidden plaintext root
- `instructions/optim.tar.gpg` as the tracked encrypted archive
- `instructions/optim/README.md` as the public keep path

After that, normal `git add`, `git commit`, and `git push` are guarded by the
repo hooks in `.githooks/`:
- `pre-commit` reads `tsodao.dsl`, refreshes and stages the hidden archive when
  `visibility_mode=recipient` is configured and plaintext hidden content is newer.
- `pre-commit` rejects staged plaintext files under the hidden root other than
  the configured public keep path.
- `pre-push` rejects pushes that would publish hidden plaintext from the commit
  range being pushed.

Practical day-to-day commands:
- to bootstrap or repair the recipient key setup, run `tsodao init`
- before a push, run `tsodao sync`; when plaintext exists and is the safe source,
  it refreshes the encrypted archive and stages it if git is available
- on a fresh machine where plaintext is absent but the encrypted archive is
  present, `tsodao sync` restores plaintext automatically
- `sync` is safe-by-default: if both plaintext and archive exist but TSODAO
  cannot prove which side this machine last synced against, it refuses instead
  of overwriting either side
- explicit direction is available only when you really mean it:
  `tsodao sync --from-plaintext` means local plaintext is the source of truth
  and the encrypted archive should be resealed, while
  `tsodao sync --from-archive` means the encrypted archive is the source of
  truth and the plaintext hidden root should be restored
- restoring over existing plaintext is treated as destructive and asks for
  confirmation unless you pass `--yes`
- TSODAO keeps that memory in the local-only `local_state_path` declared by
  `tsodao.dsl`, currently under `.runtime/.tsodao/`
- that local state records both the last synced archive sha and a fingerprint of
  the last synced plaintext hidden surface, so `tsodao sync` and
  `tsodao prompt-mark` can notice additions, deletions, and renames instead of
  only newer surviving files

If you already have a personal GPG public key and want to seal manually against
that identity instead of using the setup script:

```bash
/cuwacunu/.build/tools/tsodao seal --recipient YOUR_GPG_KEY
```

Manual symmetric sealing still works when you want an offline passphrase flow:

```bash
/cuwacunu/.build/tools/tsodao seal --symmetric
/cuwacunu/.build/tools/tsodao scrub
```

Restore later with:

```bash
/cuwacunu/.build/tools/tsodao open
```

Operational notes:
- `.gitignore` keeps the plaintext `instructions/optim/*` payload local-only and leaves `instructions/optim/README.md` public.
- `seal` excludes the configured `public_keep_path` from the encrypted tarball.
- `GENERAL.tsodao_dsl_filename` in `src/config/.config` points at the TSODAO policy DSL that hooks and scripts read.
- `open` refuses to overwrite an existing plaintext payload; run `scrub` first if you want to restore from the encrypted backup.
- `src/scripts/tsodao_sync_mark.sh` can be added to your shell status marks; it prints `[!]` whenever TSODAO wants a safe `sync` or when sync direction is ambiguous.
- If the plaintext `optim/*` files were already pushed to a public remote before this workflow, sealing protects future revisions only; it does not erase prior history.

## Secure terminal key setup utility

Use the built-in C++ utility instead of a shell script for key enrollment:

```bash
make -C /cuwacunu/src/main/tools -j12 install-secure-key-setup
```

This finalizes the standalone executable in place at:

```bash
/cuwacunu/.build/tools/secure_key_setup
```

Default interactive flow (all known keys):

```bash
/cuwacunu/.build/tools/secure_key_setup
```

Only OpenAI key:

```bash
/cuwacunu/.build/tools/secure_key_setup --only openai_real
```

Exchange keys:

```bash
/cuwacunu/.build/tools/secure_key_setup --only exchange_real
/cuwacunu/.build/tools/secure_key_setup --only exchange_test
```

Skip specific targets or existing files:

```bash
/cuwacunu/.build/tools/secure_key_setup --skip exchange_test
/cuwacunu/.build/tools/secure_key_setup --skip-existing
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
/cuwacunu/.build/hero/hero_config.mcp
```

Defaults:
- global config path defaults to `/cuwacunu/src/config/.config`
- config MCP does not require a passphrase at runtime

Direct one-shot tool call (no manual JSON-RPC framing):

```bash
/cuwacunu/.build/hero/hero_config.mcp \
  --tool hero.config.status \
  --args-json '{}'

/cuwacunu/.build/hero/hero_runtime.mcp \
  --tool hero.runtime.list_jobs \
  --args-json '{}'

/cuwacunu/.build/hero/hero_marshal.mcp \
  --tool hero.marshal.list_sessions \
  --args-json '{}'
```

Human-friendly tool discovery:

```bash
/cuwacunu/.build/hero/hero_config.mcp --list-tools
/cuwacunu/.build/hero/hero_runtime.mcp --list-tools
/cuwacunu/.build/hero/hero_marshal.mcp --list-tools
```

MCP `initialize` responses from HERO servers include an `instructions` string
with concise runtime prerequisites.

Find/install in Codex CLI:

```bash
# Check what MCP servers are already configured
codex mcp list

# Register HERO MCP in Codex (stdio transport)
codex mcp add hero-config -- \
  /cuwacunu/.build/hero/hero_config.mcp

codex mcp add hero-runtime -- \
  /cuwacunu/.build/hero/hero_runtime.mcp

codex mcp add hero-marshal -- \
  /cuwacunu/.build/hero/hero_marshal.mcp

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
- `instructions/defaults/default.hero.config.dsl` must keep
  `protocol_layer[STDIO|HTTPS/SSE]:str = STDIO`.

Run one-shot CLI:

```bash
/cuwacunu/.build/hero/hero_config.mcp --tool hero.config.status
```

Useful MCP tools:
- `hero.config.status`
- `hero.config.schema`
- `hero.config.show`
- `hero.config.get`
- `hero.config.set` (updates in-memory runtime config only; use `hero.config.save` to persist)
- `hero.config.default.list` (list all allowed files under configured `default_roots`; `include_man=true` also returns associated `.man` content when available, and files without a matching `.man` carry a warning)
- `hero.config.default.read` (read one whole file under configured `default_roots` with an allowed extension, returning content and `sha256`; `include_man=true` also returns associated `.man` content when available, and missing `.man` files surface a warning)
- `hero.config.default.create` (create one whole supported file under configured `default_roots` after decoder and file-specific validation; requires `allow_local_write=true`, target path within `write_roots`, and returns a warning because defaults are shared truth)
- `hero.config.default.replace` (replace one whole supported file under configured `default_roots` after decoder and file-specific validation; optional `expected_sha256` guards against stale overwrite; requires `allow_local_write=true`, target path within `write_roots`, and returns a warning because defaults are shared truth)
- `hero.config.default.delete` (delete one whole file under configured `default_roots`; optional `expected_sha256` guards against stale delete; requires `allow_local_write=true` and target path within `write_roots`)
- `hero.config.temp.list` (list all allowed files under configured `temp_roots`; `include_man=true` also returns associated `.man` content when available, and files without a matching `.man` carry a warning)
- `hero.config.temp.read` (read one whole file under configured `temp_roots` with an allowed extension, returning content and `sha256`; `include_man=true` also returns associated `.man` content when available, and missing `.man` files surface a warning)
- `hero.config.temp.create` (create one whole file under configured `temp_roots` with an allowed extension; `.md` scratch files are accepted directly while supported `.dsl` families still go through decoder validation; requires `allow_local_write=true`, target path within `write_roots`, and intentionally skips `backup_dir` snapshots)
- `hero.config.temp.replace` (replace one whole file under configured `temp_roots` with an allowed extension; `.md` scratch files are accepted directly while supported `.dsl` families still go through decoder validation; optional `expected_sha256` guards against stale overwrite; requires `allow_local_write=true`, target path within `write_roots`, and intentionally skips `backup_dir` snapshots)
- `hero.config.temp.delete` (delete one whole file under configured `temp_roots` with an allowed extension after optional `expected_sha256` check; requires `allow_local_write=true`, target path within `write_roots`, and intentionally skips `backup_dir` snapshots)
- `hero.config.objective.list` (list all allowed files under `objective_root`; `include_man=true` also returns associated `.man` content when available, and files without a matching `.man` carry a warning)
- `hero.config.objective.read` (read one whole objective file under `objective_root` with an allowed extension, including the objective-local campaign file named by `campaign_dsl_path` when it lives there, returning content and `sha256`; `include_man=true` also returns associated `.man` content when available, and missing `.man` files surface a warning)
- `hero.config.objective.create` (create one whole objective file under `objective_root` with an allowed extension after decoder validation; requires `allow_local_write=true` and target path within `write_roots`)
- `hero.config.objective.replace` (replace one whole objective file under `objective_root` with an allowed extension after decoder validation; optional `expected_sha256` guards against stale overwrite; requires `allow_local_write=true` and target path within `write_roots`)
- `hero.config.objective.delete` (delete one whole objective file under `objective_root` with an allowed extension, including the objective-local campaign file named by `campaign_dsl_path` when appropriate; optional `expected_sha256` guards against stale delete; requires `allow_local_write=true` and target path within `write_roots`)
- `hero.config.optim.list` (list all allowed files under the TSODAO hidden `optim` root; `include_man=true` also returns associated `.man` content when available, and the response notes when the plaintext surface is currently scrubbed)
- `hero.config.optim.read` (read one whole optim file under the TSODAO hidden root with an allowed extension, returning content and `sha256`; `include_man=true` also returns associated `.man` content when available; if the plaintext surface is scrubbed, restore it first with `tsodao sync`)
- `hero.config.optim.create` (create one whole optim file under the TSODAO hidden root with an allowed extension after decoder validation; requires `allow_local_write=true`, target path within `write_roots`, and creates an encrypted TSODAO archive checkpoint before mutation)
- `hero.config.optim.replace` (replace one whole optim file under the TSODAO hidden root with an allowed extension after decoder validation; optional `expected_sha256` guards against stale overwrite; requires `allow_local_write=true`, target path within `write_roots`, and creates an encrypted TSODAO archive checkpoint before mutation)
- `hero.config.optim.delete` (delete one whole optim file under the TSODAO hidden root with an allowed extension after optional sha256 check; requires `allow_local_write=true`, target path within `write_roots`, and creates an encrypted TSODAO archive checkpoint before mutation)
- `hero.config.optim.backups` (list encrypted TSODAO archive checkpoints for the optim surface)
- `hero.config.optim.rollback` (restore the optim surface from the latest or selected encrypted TSODAO archive checkpoint; requires `allow_local_write=true`)
- `hero.config.validate`
- `hero.config.diff` / `hero.config.dry_run` (preview changes before save)
- `hero.config.backups` (list snapshots)
- `hero.config.rollback` (restore latest or selected snapshot; requires `allow_local_write=true`)
- `hero.config.save` (persists config with deterministic atomic cutover metadata, requires `allow_local_write=true`)
- `hero.config.reload`
- `hero.config.dev_nuke_reset` (developer reset of runtime dump roots, including `.campaigns`, `.marshal_hero`, `.human_hero`, plus Hero catalogs resolved from the saved global config; when `dev_nuke_reset_backup_enabled=true` it first archives those targets under `<runtime_root>/../.backups/hero.runtime_reset/<runtime_root-name>/<stamp>/`; it uses the saved global-config runtime root instead of `write_roots`, and refuses reset while active runtime jobs exist)

Extension nuance:
- `allowed_extensions` now commonly includes `.dsl,.md`, so markdown files can be discovered and read through the `default/temp/objective/optim` file surfaces; `hero.config.temp.create/replace` also accepts `.md` scratch files directly.
- Mutation support is still validation-family dependent. Today that means `.md` files may be visible/readable without automatically being replace-safe through Config Hero.

Runtime MCP tools:
- `hero.runtime.start_campaign`
- `hero.runtime.explain_binding_selection`
- `hero.runtime.list_campaigns`
- `hero.runtime.get_campaign`
- `hero.runtime.stop_campaign`
- `hero.runtime.list_jobs`
- `hero.runtime.get_job`
- `hero.runtime.stop_job`
- `hero.runtime.tail_log`
- `hero.runtime.tail_trace`
- `hero.runtime.reconcile`

Launch-time `reset_runtime_state=true` is a runtime-owned cold reset only: it clears Runtime campaign/hashimyei state and runtime catalogs, but it preserves `.marshal_hero` and `.human_hero`. Use `hero.config.dev_nuke_reset` for the broader developer wipe that also clears those control-plane ledgers.

Marshal MCP tools:
- `hero.marshal.start_session`
- `hero.marshal.list_sessions`
- `hero.marshal.get_session`
- `hero.marshal.reconcile_session`
- `hero.marshal.pause_session`
- `hero.marshal.resume_session`
- `hero.marshal.message_session`
- `hero.marshal.archive_session`
- `hero.marshal.terminate_session`

Human MCP tools:
- `hero.human.list_requests`
- `hero.human.get_request`
- `hero.human.answer_request`
- `hero.human.resolve_governance`
- `hero.human.list_summaries`
- `hero.human.get_summary`
- `hero.human.ack_summary`
- `hero.human.archive_summary`

`hero_human.mcp` without `--tool` on a tty opens the Human Hero operator UI instead of waiting for JSON-RPC. On ncurses-capable terminals this is an all-session cockpit: the default sessions view shows working, campaign-active, blocked, review-ready, and terminal Marshal sessions with state-aware colors, filter cycling, and scrollable detail panes, while `Tab` cycles into focused requests and summaries views. Request rows correspond to sessions blocked for ordinary clarification or signed governance. Review-ready session summaries are messageable from that console with fresh operator guidance, while terminal session summaries remain informational. Session summaries begin with an effort summary that foregrounds elapsed wall time, checkpoint count, and campaign-launch usage. On unsupported terminals such as `TERM=dumb` it falls back to the line-prompt responder. On non-tty stdin it still serves stdio MCP.
`hero.human.answer_request` is the ordinary clarification path and auto-resumes the blocked session. `hero.human.resolve_governance` is governance-only: operators provide `marshal_session_id` carrying the marshal cursor, `resolution_kind = grant | deny | clarify | terminate`, and `reason`, while Marshal Hero retains responsibility for choosing the next Runtime action. `hero.human.ack_summary` is intentionally non-final: it records a required signed acknowledgment message and clears the session summary from the Human Hero inbox after review without releasing the session. `hero.human.archive_summary` is the final review disposition: it records a required signed archive note and asks Marshal Hero to archive the underlying review-ready session so the objective can be launched again. Session inspection and lifecycle control live under `hero.marshal.*`.

`hero.runtime.start_campaign` accepts optional:
- `binding_id` to run one declared `BIND` from the staged campaign snapshot instead of the default `RUN` plan
- `campaign_dsl_path` to override the configured source campaign bundle for that launch
- `marshal_session_id` to link the launched campaign to an existing Marshal Hero session ledger; this is the field Marshal Hero uses when it asks Runtime Hero to launch the next campaign

`hero.runtime.explain_binding_selection` accepts:
- required `binding_id` to name the declared `BIND` whose `MOUNT` selectors should be resolved
- optional `campaign_dsl_path` to inspect a non-default source campaign without launching it

It is the read-only explanation companion to `hero.runtime.start_campaign`: it
resolves `BIND.MOUNT` against the selected contract `DOCK`, reports the exact
hashimyei chosen for each selector, and returns structured failure details when
selection cannot be resolved. Use it first when a launch fails before
meaningful train/eval work starts, such as missing staged `MOUNT`, family
mismatch, or dock-selection failure.

`hero.marshal.start_session` is the primary session entrypoint. It accepts optional:
- `marshal_objective_dsl_path` to override the configured default supervision root for that launch
- `marshal_codex_model` to override `default.hero.marshal.dsl` for this session's Codex model
- `marshal_codex_reasoning_effort` to override `default.hero.marshal.dsl` for this session's Codex reasoning effort

Marshal Hero starts from `marshal.objective.dsl`, not from a campaign-declared `MARSHAL` edge. It resolves the selected `marshal.objective.dsl`, reads its `campaign_dsl_path`, `objective_md_path`, and `guidance_md_path`, resolves the launch-time Codex model/effort as `start_session override > default.hero.marshal.dsl`, copies the authored objective files into `<runtime_root>/.marshal_hero/<marshal_cursor>/`, writes a generated session-local `hero.marshal.dsl` plus `config.hero.policy.dsl`, builds `marshal.briefing.md`, stages an initial bootstrap input checkpoint before any Runtime campaign exists, and returns once the detached session runner is launched. The persisted session snapshot is now centered on `lifecycle`, `work_gate`, observational `activity`, `campaign_status`, `campaign_cursor`, `current_thread_id`, `codex_continuity`, stable pending operator-message entities, and an exact turns ledger under `marshal.session.turns.jsonl`, while checkpoints remain durability artifacts rather than the top-level session identity. `complete` now parks the session as review-ready (`lifecycle=live`, `activity=review`), and `hero.marshal.message_session` re-enters that same live session with a fresh operator message. Review-ready sessions still wake through a planning checkpoint, while other live sessions now prefer direct runner-owned delivery into the current Codex thread, record the exact turn in the turns ledger, and use `memory.md` only as a distilled continuity layer, falling back to queued safe-point handling only when live thread continuity is unavailable. When that direct path finishes during the tool call, `hero.marshal.message_session` now returns `delivery=\"delivered\"` plus `reply_text`; degraded in-band delivery returns `delivery=\"failed\"` with a warning so operator UIs can surface it immediately. `hero.marshal.archive_session` is the final ownership-release path for review-ready sessions: it marks the session `terminal` without overloading summary acknowledgment so a fresh Marshal can be launched for the same objective. The resolved Codex binary/model/reasoning values are pinned in the session manifest and reused for fresh and resumed checkpoints instead of rereading mutable defaults, and replacement-thread recovery now rebuilds context from the durable session snapshot plus recent exact turns and warnings. If a planning checkpoint already produced an intent artifact and later fails during mutation bookkeeping or validation, Marshal Hero preserves the attempted checkpoint, parks the session as review-ready with `finish_reason=failed`, and lets the operator message it again once the issue is fixed. If a sudden interruption or reboot leaves Runtime and Marshal out of sync, `hero.marshal.get_session`, `hero.marshal.list_sessions`, and `hero.marshal.reconcile_session` best-effort repair the session by parking it as review-ready with `finish_reason=interrupted` plus recovery detail so the operator can inspect runtime evidence and message the same session. Later no-op retries of that same checkpoint preserve any existing `mutation.<checkpoint>.json` record instead of erasing mutation history. Runtime Hero still snapshots from that truth source on every launch, so campaign execution stays immutable while Marshal Hero mutations survive `dev_nuke_reset`. Those truth-source mutations are backed by Config Hero backups under `.backups/hero.marshal/<objective_name>/`, and per-checkpoint mutation summaries are written when Codex changed objective files. Only one nonterminal Marshal session may own a given `marshal.objective.dsl` at a time, so review-ready sessions keep ownership until the operator messages them forward, terminates them, or archives them explicitly.

Runtime HERO defaults:
- loaded from `instructions/defaults/default.hero.runtime.dsl`
- resolved through `[REAL_HERO].runtime_hero_dsl_filename`
- campaigns root derived from `[GENERAL].runtime_root` as
  `<runtime_root>/.campaigns`
- marshal-session root derived from `[GENERAL].runtime_root` as
  `<runtime_root>/.marshal_hero`
- campaign grammar loaded from `[BNF].iitepi_campaign_grammar_filename`

Marshal HERO defaults:
- loaded from `instructions/defaults/default.hero.marshal.dsl`
- resolved through `[REAL_HERO].marshal_hero_dsl_filename`
- marshal-session root derived from `[GENERAL].runtime_root` as `<runtime_root>/.marshal_hero`
- repo root taken from `[GENERAL].repo_root`
- config scope root taken from `config_scope_root` in `default.hero.marshal.dsl` and baked into the session-local Config Hero policy
- Marshal Hero truth-source mutations also enable Config Hero backups under `.backups/hero.marshal/<objective_name>/`
- campaign grammar loaded from `[BNF].iitepi_campaign_grammar_filename`
- marshal-objective grammar loaded from `[BNF].marshal_objective_grammar_filename`
- `runtime_hero_binary`, `config_hero_binary`, and `lattice_hero_binary` select the MCP binaries Marshal Hero attaches to each planning checkpoint
- `human_hero_binary` selects the MCP binary Human Hero uses against the same session root
- `human_operator_identities` selects the operator identities file Marshal Hero uses to bind `operator_id` values to OpenSSH `ssh-ed25519` public keys before applying a signed governance resolution
- `marshal_codex_binary` selects the Codex executable or command name used for the durable planning session; bare `codex` first resolves through `PATH`, then falls back to known VS Code Server ChatGPT extension install locations when available
- `marshal_codex_model` selects the default Codex model slug used when `hero.marshal.start_session` does not override it, defaulting to `gpt-5.3-codex-spark`
- `marshal_codex_reasoning_effort` selects the default Codex `model_reasoning_effort` used when `hero.marshal.start_session` does not override it, defaulting to `xhigh` (Extra High)
- `marshal_codex_timeout_sec` bounds one `codex exec` or `codex exec resume` checkpoint call
- `marshal_max_campaign_launches` bounds the number of Runtime campaign launches in one session
- `poll_interval_ms` controls detached session-runner polling cadence while waiting for terminal campaign state

Human HERO defaults:
- loaded from `instructions/defaults/default.hero.human.dsl`
- resolved through `[REAL_HERO].human_hero_dsl_filename`
- `marshal_hero_binary` selects the MCP binary Human Hero uses to inspect sessions and resume them with human input or governance
- `operator_id` is recorded into every signed governance-resolution/summary-ack artifact; if it is still `CHANGE_ME_OPERATOR` or empty, the first Human Hero use auto-initializes it to `<user>@<hostname>` and persists that value back into `default.hero.human.dsl`
- `operator_signing_ssh_identity` selects the unencrypted OpenSSH `ssh-ed25519` identity Human Hero uses for signed governance/summary acknowledgments, and its public key must be the one registered for `operator_id` in Marshal Hero `human_operator_identities`

Human Hero operator setup:
1. Run `bash src/scripts/setup_human_operator.sh`.
2. That script bootstraps `operator_id` if needed, creates or validates the unencrypted OpenSSH `ssh-ed25519` identity at `operator_signing_ssh_identity`, and updates `human_operator_identities`.
3. Use `bash src/scripts/setup_human_operator.sh --validate` to re-check the setup later.
4. Only after that will `hero.human.resolve_governance`, `hero.human.ack_summary`, and `hero.human.archive_summary` be able to sign artifacts that Marshal Hero or Human Hero can verify.

Runtime Hero campaigns persist under the derived campaigns root by `campaign_cursor`, with
`runtime.campaign.manifest.lls`, `campaign.dsl`, and campaign-level stdout/stderr logs. Child jobs
persist under `<runtime_root>/.campaigns/<campaign_cursor>/jobs/<job_cursor>/`, where
`job_cursor` is derived from the parent `campaign_cursor` using the compact
`<campaign_cursor>.j.<base36_index>` form, with `runtime.job.manifest.lls`,
`campaign.dsl`, `binding.contract.dsl`, `binding.wave.dsl`, and worker
stdout/stderr logs plus `job.trace.jsonl`.
`job.trace.jsonl` is a structured append-only execution trace intended for
long-running campaigns; `hero.runtime.tail_trace` exposes the latest phases
without scraping stdout/stderr.
Runtime liveness is reconciled using boot id + process start ticks, not bare pid reuse.

Long-lived STDIO Hero servers also persist their own diagnostics without
touching stdout transport payloads:
- Config Hero `<runtime_root>/.config_hero/mcp/hero_config_mcp/stderr.log` plus `events.jsonl`
- Hashimyei Hero `<runtime_root>/.hashimyei/_meta/mcp/hero_hashimyei_mcp/stderr.log` plus `events.jsonl`
- Lattice Hero `<runtime_root>/.hashimyei/_meta/mcp/hero_lattice_mcp/stderr.log` plus `events.jsonl`
`stderr.log` mirrors emitted diagnostics while `events.jsonl` keeps a small
append-only startup/direct-tool audit trail.

Marshal sessions persist under `<runtime_root>/.marshal_hero/<marshal_cursor>/` with:
- `marshal.session.manifest.lls`
- `marshal.objective.dsl`
- `marshal.objective.md`
- `marshal.guidance.md`
- `hero.marshal.dsl`
- `config.hero.policy.dsl`
- `marshal.session.memory.md`
- `marshal.session.turns.jsonl`
- `marshal.briefing.md`
- `logs/codex.session.stdout.jsonl`
- `logs/codex.session.stderr.jsonl`
- `marshal.session.events.jsonl`
- `human/request.latest.md` when a pause is pending
- `human/summary.latest.md` when a session reaches `review-ready` or `terminal`
- `human/governance_resolution.latest.json` when signed governance has answered
- `human/governance_resolution.latest.sig` when signed governance has answered
- `human/clarification_answer.latest.json` when ordinary clarification has answered
- `human/summary_ack.latest.json` when Human Hero has acknowledged or archived a session summary
- `human/summary_ack.latest.sig` when Human Hero has acknowledged or archived a session summary

`logs/codex.session.stdout.jsonl` stores the raw `codex exec --json` stream for
the latest checkpoint attempt. `logs/codex.session.stderr.jsonl` mirrors stderr
as line-wrapped JSONL entries with `stream`, `line_index`, and `text`.
- `human/governance_resolutions/governance_resolution.0001.json` plus matching `.sig`
- `checkpoints/input.0001.json` plus `checkpoints/input.latest.json`
- `checkpoints/intent.0001.json` plus `checkpoints/intent.latest.json`
- `checkpoints/mutation.0001.json` plus `checkpoints/mutation.latest.json` when a planning checkpoint changed truth-source files

Current marshal-session schemas:
- `hero.marshal.session.v6`
- `hero.marshal.input_checkpoint.v3`
- `hero.marshal.intent_checkpoint.v3`
- `hero.marshal.mutation_checkpoint.v3`
- `hero.human.governance_resolution.v3`
- `hero.human.clarification_answer.v3`
- `hero.human.summary_ack.v3`

The v6 Marshal session keeps Codex shell access read-only, but gives each planning checkpoint session-scoped `hero.config.objective.*`, session-scoped `hero.config.temp.*` for temporary authored surfaces, conditionally `hero.config.default.*`, bounded Runtime read/tail tools, and bounded Lattice read tools against truth-source config roots and persisted runtime evidence. Objective files include the objective-local campaign file named by `campaign_dsl_path` when it lives under the selected objective root, so launch-graph changes go through the same `hero.config.objective.*` surface instead of a special campaign tool family. The generated Marshal briefing names that exact relative campaign file path and tells Codex to use `hero.config.objective.list` rather than assuming a literal `campaign.dsl` filename. Each session also carries a generated `hero.marshal.dsl` that records the resolved launch settings, including any `hero.marshal.start_session` model/effort overrides. Codex now returns `intent = launch_campaign | pause_for_clarification | request_governance | complete | terminate`, and the session event ledger records typed facts such as `session.started`, `operator.message_received`, `operator.message_delivered`, `operator.message_handled`, `codex.action_requested`, `work.blocked`, `work.unblocked`, `campaign.started`, `campaign.start_failed`, `session.finished`, and loud `codex.resume_failed` degradation notices. `launch_campaign` carries `mode = run_plan | binding`, optional `binding_id`, `reset_runtime_state`, and `requires_objective_mutation`; `pause_for_clarification` carries `clarification_request.request`; `request_governance` carries a typed `kind`, operator-facing `request`, and optional typed `delta`. If `launch.requires_objective_mutation=true` but the planning checkpoint produced no same-checkpoint mutation and no recorded mutation checkpoint exists for that checkpoint index, Marshal Hero rejects the launch instead of silently rerunning unchanged truth sources. No-op retries of the same checkpoint preserve any existing mutation checkpoint summary instead of clearing `mutation.latest.json`. If a sudden interruption or reboot leaves the detached Marshal runner gone, read-side reconciliation can park the session as `review-ready/interrupted` with recovery detail instead of leaving a stale live ledger behind. Launch-time `reset_runtime_state=true` is scoped to runtime-owned state and must not delete the owning Marshal/Human ledgers. Human Hero answers ordinary clarification with `hero.human.answer_request`, resolves signed governance with `hero.human.resolve_governance`, and leaves ordinary Runtime routing to Marshal Hero.

The short design constitution for that session model lives in [MARSHAL_SESSION.md](/cuwacunu/src/main/hero/MARSHAL_SESSION.md).

Deterministic policy:
- Config HERO edits files only inside configured `default_roots`, `temp_roots`, `objective_roots`, and the TSODAO hidden optim root, and only for `allowed_extensions`. Existing hashimyei instances are not mutated by these tools; instance revisions are handled by Hashimyei HERO lineage.
- `hero.config.temp.*` uses the same write policy gates as the other plaintext surfaces, but it deliberately skips `backup_dir` snapshots so temporary instruction churn does not pollute shared config backups.
- `allow_local_write=false` blocks persisted config writes plus default/objective/optim plaintext file mutations.
- `write_roots` constrains persisted config writes, default/objective file mutations,
  and plaintext optim mutation targets when local writes are enabled.
- `hero.config.optim.*` resolves its hidden plaintext root and encrypted archive
  from `GENERAL.tsodao_dsl_filename`; its pre-mutation backups are encrypted
  copies of the TSODAO archive written under `optim_backup_dir`, not plaintext
  file snapshots.
- `dev_nuke_reset_backup_enabled=true` makes `hero.config.dev_nuke_reset` move
  targeted runtime roots/catalogs into
  `<runtime_root>/../.backups/hero.runtime_reset/<runtime_root-name>/<stamp>/`
  before cleanup; this archive path is derived from the saved runtime root, not
  from `write_roots`.
- `hero.config.dev_nuke_reset` uses the saved global config on disk, not dirty
  unsaved in-memory edits.
- `hero.config.dev_nuke_reset` fails fast while active Runtime HERO jobs or campaigns still exist under `<runtime_root>/.campaigns` and only removes paths derived from the saved `<runtime_root>`.

## Hashimyei Catalog MCP

Build:

```bash
make -C /cuwacunu/src/main/hero -j12 all
```

Installed binary:
- `/cuwacunu/.build/hero/hero_hashimyei.mcp`

Run MCP:

```bash
/cuwacunu/.build/hero/hero_hashimyei.mcp
```

Defaults:
- global config path defaults to `/cuwacunu/src/config/.config`
- catalog mode is unencrypted in MCP runtime

Direct one-shot tool call:

```bash
/cuwacunu/.build/hero/hero_hashimyei.mcp \
  --tool hero.hashimyei.list \
  --args-json '{}'
```

Hashimyei HERO is loaded from `default.hero.hashimyei.dsl` through
`[REAL_HERO].hashimyei_hero_dsl_filename` in the global config, while
`store_root` and `catalog_path` are derived from `[GENERAL].runtime_root`.
Catalog encryption mode is fixed to unencrypted in MCP runtime.

MCP ingest behavior:
- discovery-only `hero.hashimyei.list` reads the current catalog snapshot and
  does not force a store-wide ingest rebuild.
- `hero.hashimyei.get_component_manifest`,
  `hero.hashimyei.evaluate_contract_compatibility`, and
  `hero.hashimyei.get_founding_dsl_bundle` read the current store snapshot
  directly so lineage and docking checks do not need the ingest rebuild path.
- `hero.hashimyei.reset_catalog` remains the explicit force-rebuild tool.

Register in Codex:

```bash
codex mcp add hero-hashimyei -- \
  /cuwacunu/.build/hero/hero_hashimyei.mcp
```

Supported MCP tools:
- `hero.hashimyei.list`
- `hero.hashimyei.get_component_manifest`
- `hero.hashimyei.evaluate_contract_compatibility`
  - compares a component revision against a requested contract using the same
    dock-only rule runtime uses for hashimyei reuse
- `hero.hashimyei.get_founding_dsl_bundle`
  - returns the stored `.runtime/.hashimyei` founding DSL bundle snapshot for a
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
/cuwacunu/.build/hero/hero_lattice.mcp
```

Defaults:
- global config path defaults to `/cuwacunu/src/config/.config`
- catalog mode is unencrypted in MCP runtime

Direct one-shot tool call:

```bash
/cuwacunu/.build/hero/hero_lattice.mcp \
  --tool hero.lattice.list_views \
  --args-json '{}'
```

Lattice HERO is loaded from `default.hero.lattice.dsl` through
`[REAL_HERO].lattice_hero_dsl_filename` in the global config, while
`store_root` and `catalog_path` are derived from `[GENERAL].runtime_root`.
Catalog encryption mode is fixed to unencrypted in MCP runtime.

Catalog sync behavior:
- each read tool call fingerprints the catalog-visible store surface and
  rebuilds the catalog when that fingerprint changes.
- `hero.lattice.refresh` remains the explicit force-rebuild tool.

If you also use lattice MCP, register it the same way:

```bash
codex mcp add hero-lattice -- \
  /cuwacunu/.build/hero/hero_lattice.mcp
```

Supported MCP tools:
- `hero.lattice.list_facts`
- `hero.lattice.get_fact`
- `hero.lattice.list_views`
- `hero.lattice.get_view`
- `hero.lattice.refresh`

## Split Policy

- Global settings live in `./.config`: `[GENERAL]`, `[GUI]`, `[BNF]`, `[TSI_RUNTIME]`, `[REAL_EXCHANGE]`, `[TEST_EXCHANGE]`, `[REAL_HERO]`.
  `GENERAL.default_iitepi_campaign_dsl_filename` points to the checked-in
  default top-level campaign, currently
  `./instructions/defaults/default.iitepi.campaign.dsl`.
  `GENERAL.repo_root` pins the repository/worktree root that Marshal Hero uses
  for `codex exec -C` during session planning checkpoints.
  Runtime reset is intentionally explicit and can be invoked through
  `/cuwacunu/.build/tools/runtime_reset` or `make -C /cuwacunu/src/main reset-runtime`.
- `./instructions/defaults/` holds the canonical example/default payloads,
  including the `instructions/defaults/*.dsl` files that Config HERO is allowed to manage.
- `./instructions/objectives/` holds coherent experiment bundles. The first
  bundle is `vicreg.solo/`, which keeps contract, waves, campaign binds, and
  only the objective-local wrappers that differ from `./instructions/defaults/`.
- `[GUI]` holds iinuji defaults, currently:
  `iinuji_logs_buffer_capacity`, `iinuji_logs_show_date`,
  `iinuji_logs_show_thread`, `iinuji_logs_show_metadata`,
  `iinuji_logs_metadata_filter`, `iinuji_logs_show_color`,
  `iinuji_logs_auto_follow`, `iinuji_logs_mouse_capture`,
  `iinuji_loading_logo_path`, `iinuji_closing_logo_path`,
  `iinuji_home_animation_path`.
  `iinuji_closing_logo_path` defaults to
  `/cuwacunu/src/resources/waajacamaya.png`; invalid paths fall back to
  `iinuji_loading_logo_path` during shutdown.
  `iinuji_home_animation_path` defaults to
  `/cuwacunu/src/resources/waajacamaya.apng`; invalid paths fall back to the
  static `iinuji_loading_logo_path` asset on F1 Home.
- `[TSI_RUNTIME]` holds runtime execution defaults:
  `runtime_trace_level`, `runtime_max_queue_size`.
- HERO runtime settings for deterministic MCP live in:
  - `./instructions/defaults/default.hero.config.dsl` (Config HERO runtime policy)
  - `./instructions/defaults/default.hero.hashimyei.dsl` (Hashimyei HERO catalog defaults)
  - `./instructions/defaults/default.hero.lattice.dsl` (Lattice HERO catalog/runtime defaults)
  - `./instructions/defaults/default.hero.human.dsl` (Human HERO operator attestation defaults)
  - `./instructions/defaults/default.hero.marshal.dsl` (Marshal HERO session/orchestration defaults)
  - `./instructions/defaults/default.hero.runtime.dsl` (Runtime HERO campaign/job defaults)
  `default.hero.hashimyei.dsl` is the Hashimyei HERO runtime defaults file. It is
  not the founding DSL bundle for component revision lineage.
- `[REAL_HERO]` owns the canonical pointer paths for those six HERO DSL files:
  - `config_hero_dsl_filename`
  - `hashimyei_hero_dsl_filename`
  - `human_hero_dsl_filename`
  - `lattice_hero_dsl_filename`
  - `marshal_hero_dsl_filename`
  - `runtime_hero_dsl_filename`
- All grammar (`*.bnf`) paths are centralized in `[BNF]`:
  - `iitepi_campaign_grammar_filename`
  - `iitepi_runtime_binding_grammar_filename`
  - `iitepi_wave_grammar_filename`
  - `marshal_objective_grammar_filename`
  - `network_design_grammar_filename`
  - `vicreg_grammar_filename`
  - `expected_value_grammar_filename`
  - `wikimyei_evaluation_embedding_sequence_analytics_grammar_filename`
  - `wikimyei_evaluation_transfer_matrix_evaluation_grammar_filename`
  - `observation_sources_grammar_filename`
  - `observation_channels_grammar_filename`
  - `jkimyei_specs_grammar_filename`
    shared grammar (`bnf/jkimyei.bnf`) for per-component jkimyei DSL files
  - `tsiemene_circuit_grammar_filename`
  - `canonical_path_grammar_filename`
- Unified campaign DSL (`./instructions/defaults/default.iitepi.campaign.dsl`) declares:
  - `CAMPAIGN { ... }` root block
  - `IMPORT_CONTRACT "<contract_defaults_file>" AS <contract_alias>;`
  - `FROM "<wave_dsl_file>" IMPORT_WAVE <wave_id>;`
  - `BIND <id> { MOUNT { <wave_binding_id> = EXACT 0x...; | <wave_binding_id> = RANK <n>; } CONTRACT = <imported_contract_alias>; WAVE = <imported_wave_id>; }`
  - ordered `RUN <bind_id>;`
  - optional bind-local variables inside `BIND`, where names must start with `__`
    and are intended for wave-local pre-decode placeholder resolution such as
    `% __sampler ? sequential %`
  - bind-local variables are operational only; they do not override
    contract-defined docking variables
  - bind-local variables may not reuse names already declared by contract
    `__variables`; shadowing is rejected during campaign snapshot staging
  - Runtime Hero may also narrow launch to one declared `BIND` through
    `hero.runtime.start_campaign(binding_id=...)`; when omitted, the declared
    `RUN` sequence remains the default plan
  - Marshal Hero is the primary persistent post-campaign planning session through
    `hero.marshal.start_session(...)`; it starts from `marshal.objective.dsl`, which
    declares the campaign plus separate human-authored objective and guidance markdown
  - `marshal.objective.dsl` is its own DSL family, validated by
    `./bnf/objective.marshal.bnf`, and currently declares
    `campaign_dsl_path:path`, `objective_md_path:path`, `guidance_md_path:path`,
    and optional
    `objective_name:str` and `marshal_session_id:str`
  The same grammar is used by objective-local files such as
  `./instructions/objectives/vicreg.solo/iitepi.campaign.dsl`.
  The defaults bundle also ships sample `./instructions/defaults/default.marshal.objective.dsl`
  plus `./instructions/defaults/default.marshal.objective.md` and
  `./instructions/defaults/default.marshal.guidance.md`.
- Vocabulary used below:
  - `component revision`: one stored loadable family member
  - `hashimyei`: the exact revision token, such as `0x00FF`, and the name of
    the identity/catalog subsystem
  - `DOCK`: the public compatibility interface declared by a contract
  - `ASSEMBLY`: the contract-owned realization and DSL graph
  - `WAVE`: the logical runtime policy and required component slots
  - `BIND.MOUNT`: the run-local selector that chooses which compatible
    component revision fills a wave slot
- Runtime Hero owns campaign dispatch and persists immutable snapshots under
  `<runtime_root>/.campaigns/<campaign_cursor>/`. Each child job receives a staged
  `campaign.dsl`, `binding.contract.dsl`, `binding.wave.dsl`, and
  `job.trace.jsonl` under
  `<runtime_root>/.campaigns/<campaign_cursor>/jobs/<job_cursor>/`.
- The staged per-job `campaign.dsl` keeps the selected bind's `MOUNT` block,
  while the staged `binding.wave.dsl` records the resolved exact component
  revision paths produced by that mount selection.
- `hero.runtime.explain_binding_selection` exposes that same dock-based
  `BIND.MOUNT` resolution path before launch, so operators can inspect which
  concrete component revision would be mounted, including the exact selected
  hashimyei token, without creating a campaign snapshot.
- Marshal Hero owns the long-lived session ledgers under
  `<runtime_root>/.marshal_hero/<marshal_cursor>/`. The session ledger is adjacent to
  campaigns rather than nested inside them because one session may span many
  sequential campaigns.
- Internal runtime-binding snapshots are validated against
  `bnf/iitepi.runtime_binding.bnf` and follow this staged shape:
  - `ACTIVE_BIND <bind_id>;`
  - `RUNTIME_BINDING { ... }`
  - `IMPORT_CONTRACT "<contract_defaults_file>" AS <contract_alias>;`
  - `FROM "<wave_dsl_file>" IMPORT_WAVE <wave_id>;`
  - `BIND <id> { MOUNT { <wave_binding_id> = EXACT 0x...; | <wave_binding_id> = RANK <n>; } CONTRACT = <imported_contract_alias>; WAVE = <imported_wave_id>; }`
- The public dispatcher is campaign-oriented. The top-level runtime DSL is now
  `campaign.dsl`, and `jkimyei` is not a separate Hero.
- Contract settings live in the checked-in defaults example
  `./instructions/defaults/default.iitepi.contract.dsl` and in objective-local
  contract bundles such as `./instructions/objectives/vicreg.solo/iitepi.contract.base.dsl`, with
  marker format:
  - `-----BEGIN IITEPI CONTRACT-----`
  - `DOCK { ... }`
  - `ASSEMBLY { ... }`
  - `-----END IITEPI CONTRACT-----`
  `DOCK` is the public semantic compatibility surface.
  `ASSEMBLY` is the concrete realization and contract-owned DSL graph.
  `AKNOWLEDGE` values live inside `ASSEMBLY` and must be family tokens (no
  hashimyei suffix). This keeps contract static while campaign `BIND.MOUNT`
  owns runtime component-revision selection. Component revision lineage is
  contract-scoped; reusing the same exact revision token across contracts is
  invalid. Active component selection is also contract-scoped, so "active" is
  never a global family-wide pointer.
  Family reranking is a runtime artifact concern, not a contract parameter:
  `hero.hashimyei.update_rank` persists dock-scoped
  `hero.family.rank.v2` overlays keyed by `(family, dock_hash)`, and
  `hero.lattice.get_view(view_kind=family_evaluation_report, ...)`
  serializes dock-compatible family evidence used by client-owned ranking
  logic. Ranking stays inert until an explicit overlay is written; it does not
  bootstrap a default order and does not alter runtime component selection in
  this phase.
  Assembly-owned module configuration is explicit through path-bearing
  `ASSEMBLY` variables such as:
  `__vicreg_config_dsl_file`,
  `__expected_value_config_dsl_file`,
  `__embedding_sequence_analytics_config_dsl_file`,
  `__transfer_matrix_evaluation_config_dsl_file`,
  together with assembly-owned observation/channel DSL selectors.
  In the checked-in `vicreg.solo` objective, VICReg remains objective-local
  while the expected-value and evaluation sidecars explicitly point to the
  shared `../../defaults` payloads.
  Contract `DOCK` and `ASSEMBLY` variables are both resolved across the
  contract-local DSL graph. `DOCK` holds public values such as input tensor
  shape, embedding dimensions, and future-target dimensionality, while
  `ASSEMBLY` holds private encoder/projector widths, weights, and file
  ownership. Changing dock values changes compatibility lineage.
  Runtime derives an explicit docking signature from the compatible circuit
  set, all `DOCK` assignments, and dock-bearing assembly surfaces
  (currently the circuit and observation-channel DSL when present).
  Assembly-owned observation source registries still affect exact contract
  identity through the full dependency/signature graph, but they are not part
  of the public docking digest because unrelated source-row additions should
  not invalidate compatible component weights. Component manifests persist that digest as
  `docking_signature_sha256_hex`, alongside a contract-scoped
  `lineage_state`. Identity is keyed by stable surface ids plus resolved
  content hashes; local checkout-root path spellings are retained for runtime
  diagnostics, but do not change the digest by themselves.
  When wave reuses an existing component revision, runtime validates that the
  selected component manifest has a compatible public docking signature before
  accepting the load. The founding contract hash remains stored as provenance
  in the manifest, but it is no longer the hard runtime acceptance gate.
  Compatible revisions therefore need the same public `DOCK`, not the same
  `ASSEMBLY`. The `hero.hashimyei.evaluate_contract_compatibility` tool exposes
  that exact docking decision with a human-readable explanation.
  When assembly-owned observation/channel DSL paths are present together with a
  VICReg network design, contract validation also checks that observation
  active-channel count matches dock `__obs_channels` and `INPUT.C`, that
  max active `seq_length` matches dock `__obs_seq_length` and `INPUT.T`,
  and that dock `__obs_feature_dim` matches `INPUT.D` when declared.
  In the current model, the observation-channel table is the source of truth
  for loader-derived docking values:
  `C = count(active == true)` and
  `T = max(seq_length) over active rows`.
  `__embedding_dims` remains a dock-owned width on the VICReg
  output side for downstream component compatibility.
  Runtime load now allows VICReg checkpoint-private topology to differ from
  the current default constructor shape as long as the public docking widths
  remain compatible. In practice, that means private encoder/projector widths
  may vary across component revisions, while `__obs_channels`,
  `__obs_seq_length`, `__obs_feature_dim`, `__embedding_dims`, and
  `__future_target_dims` remain the enforced public docking boundary.
  Assembly `CIRCUIT_FILE` declares one or more compatible named circuits, and
  the operational selector is wave-local `CIRCUIT: <circuit_name>;`.
  If a contract exposes multiple circuits and wave omits `CIRCUIT`, runtime
  now rejects the binding instead of relying on a contract-local default.
  Circuit grammar accepts multiline hop expressions and comments:
  `/* ... */` and `# ...`.
- Wave settings are authored directly in the defaults example
  `./instructions/defaults/default.iitepi.wave.dsl` and in objective-local
  wave bundles such as `./instructions/objectives/vicreg.solo/iitepi.waves.dsl`.
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
  - concrete component revision selection is campaign-local through
  `BIND.MOUNT { <wave_binding_id> = EXACT 0x...; | ... = RANK <n>; }`.
    Authored wave files no longer carry `WIKIMYEI.HASHIMYEI`; wave `WIKIMYEI`
    blocks stay focused on family/path identity and `JKIMYEI` profile policy.
- `instructions/defaults/default.tsi.source.dataloader.sources.dsl` owns CSV lattice policy via required:
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

## Expected Value / Expectation Runtime Notes

- `EXPECTED_VALUE.optimizer_threshold_reset` is a step-counter clamp for Adam/AdamW (not a grad-norm reset trigger).
- `EXPECTED_VALUE` names the MDN-backed module whose primary exposed statistic is
  the conditional expectation `E[Y|X]`.
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
- VICReg `[AUGMENTATIONS]` now uses canonical field `time_warp_curve` for the base temporal warp selector. `curve_param`, `noise_scale`, and `smoothing_kernel_size` are the knobs that shape the actual time warp; legacy field name `name` is still accepted for backward compatibility.
- Embedding-sequence sidecars are now owned by
  `default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl`, not by
  silent `piaabo` defaults. The module config controls
  `max_samples`, `max_features`, `mask_epsilon`, and `standardize_epsilon`
  while runtime `debug_enabled` still gates whether the sidecars are emitted.
- `network_design` is the naming for graph/node architecture payloads.
  Required path binding is configured by `network_design_dsl_file` inside
  `default.tsi.wikimyei.representation.vicreg.dsl`.
  VICReg encoder/projector architecture is authored only in
  `*.representation.vicreg.network_design.dsl`; the thin
  `*.representation.vicreg.dsl` wrapper owns runtime placement,
  `enable_buffer_averaging`, and payload bindings.
  Resolved network-design architecture is normalized back into the runtime
  contract snapshot for downstream compatibility readers of `VICReg.*`.
  Decoder layer is framework-agnostic; semantic validation is Wikimyei-owned;
  LibTorch-facing mapping is performed in `piaabo/torch_compat`.
  For checkpoint-side network analytics sidecars, declare exactly one
  `node ...@NETWORK_ANALYTICS_POLICY` in `network_design.dsl` with strict keys:
  `near_zero_epsilon`, `log10_abs_histogram_bins`, `log10_abs_histogram_min`,
  `log10_abs_histogram_max`, `include_buffers`, `enable_spectral_metrics`,
  `spectral_max_elements`, `anomaly_top_k`.
