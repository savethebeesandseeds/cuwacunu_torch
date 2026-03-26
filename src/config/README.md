# Config Folder Layout

This folder is organized by role:

- `./.config`: global runtime settings (exchange, seeds, UI/system knobs).
- `./instructions/defaults/`: canonical example/default DSL bundle.
- `./instructions/objectives/`: objective-scoped experiment bundles.
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
- `bnf/objective.super.bnf`
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
- `instructions/defaults/default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl`
- `instructions/defaults/default.hero.super.dsl`
- `instructions/defaults/default.hero.runtime.dsl`
- `instructions/defaults/default.super.objective.dsl`
- `instructions/defaults/default.super.objective.md`
- `instructions/defaults/default.super.guidance.md`
- `instructions/objectives/vicreg.solo/iitepi.contract.base.dsl`
- `instructions/objectives/vicreg.solo/iitepi.waves.dsl`
- `instructions/objectives/vicreg.solo/iitepi.campaign.dsl`
- `instructions/objectives/vicreg.solo/vicreg.solo.super.dsl`
- `instructions/objectives/vicreg.solo/vicreg.solo.objective.md`
- `instructions/objectives/vicreg.solo/tsi.source.dataloader.channels.dsl` (objective-owned big-span observation profile)
- `instructions/objectives/vicreg.solo/tsi.wikimyei.representation.vicreg.dsl` (objective-local VICReg runtime wrapper over objective-owned network design and jkimyei payload bindings)
- `instructions/objectives/vicreg.solo/vicreg.solo.man`
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

/cuwacunu/.build/hero/hero_super_mcp \
  --tool hero.super.list_loops \
  --args-json '{}'
```

Human-friendly tool discovery:

```bash
/cuwacunu/.build/hero/hero_config_mcp --list-tools
/cuwacunu/.build/hero/hero_runtime_mcp --list-tools
/cuwacunu/.build/hero/hero_super_mcp --list-tools
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

codex mcp add hero-super -- \
  /cuwacunu/.build/hero/hero_super_mcp

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
/cuwacunu/.build/hero/hero_config_mcp --tool hero.config.status
```

Useful MCP tools:
- `hero.config.status`
- `hero.config.schema`
- `hero.config.show`
- `hero.config.get`
- `hero.config.set` (updates in-memory runtime config only; use `hero.config.save` to persist)
- `hero.config.default.read` (read one whole `instructions/defaults/*.dsl`, returning content plus `sha256`)
- `hero.config.default.replace` (replace one whole supported `instructions/defaults/*.dsl` after decoder and file-specific validation; optional `expected_sha256` guards against stale overwrite; requires `allow_local_write=true` and target path within `write_roots`)
- `hero.config.objective_dsl.read` (read one whole mutable objective-local `.dsl` under `objective_root`, excluding `campaign.dsl` and the source super-objective constitution, returning content plus `sha256`)
- `hero.config.objective_dsl.replace` (create or replace one whole mutable objective-local `.dsl` under `objective_root` after decoder validation; optional `expected_sha256` guards against stale overwrite; excludes `campaign.dsl` and the source super-objective constitution; requires `allow_local_write=true` and target path within `write_roots`)
- `hero.config.objective_campaign.read` (read one whole objective-local `campaign.dsl` under `objective_root`, returning content plus `sha256`)
- `hero.config.objective_campaign.replace` (create or replace one whole objective-local `campaign.dsl` under `objective_root` after campaign decoder validation; optional `expected_sha256` guards against stale overwrite; requires `allow_local_write=true` and target path within `write_roots`)
- `hero.config.validate`
- `hero.config.diff` / `hero.config.dry_run` (preview changes before save)
- `hero.config.backups` (list snapshots)
- `hero.config.rollback` (restore latest or selected snapshot; requires `allow_local_write=true`)
- `hero.config.save` (persists config, takes shared runtime lock, requires `allow_local_write=true`, returns deterministic `cutover` metadata)
- `hero.config.reload`
- `hero.config.dev_nuke_reset` (developer reset of runtime dump roots, including `.campaigns` and `.super_hero`, plus Hero catalogs resolved from the saved global config; requires `allow_local_write=true`, requires all reset targets to stay within `write_roots`, and refuses reset while active runtime jobs exist)

Runtime MCP tools:
- `hero.runtime.start_campaign`
- `hero.runtime.list_campaigns`
- `hero.runtime.get_campaign`
- `hero.runtime.stop_campaign`
- `hero.runtime.list_jobs`
- `hero.runtime.get_job`
- `hero.runtime.stop_job`
- `hero.runtime.tail_log`
- `hero.runtime.tail_trace`
- `hero.runtime.reconcile`

Super MCP tools:
- `hero.super.start_loop`
- `hero.super.list_loops`
- `hero.super.get_loop`
- `hero.super.resume_loop`
- `hero.super.stop_loop`

Human MCP tools:
- `hero.human.list_requests`
- `hero.human.get_request`
- `hero.human.respond`

`hero_human_mcp` without `--tool` on a tty now opens a simple ncurses operator UI for pending human requests; on non-tty stdin it still serves stdio MCP.

`hero.runtime.start_campaign` accepts optional:
- `binding_id` to run one declared `BIND` from the staged campaign snapshot instead of the default `RUN` plan
- `campaign_dsl_path` to override the configured source campaign bundle for that launch
- `super_loop_id` to link the launched campaign to an existing Super Hero loop ledger; this is the field Super Hero uses when it asks Runtime Hero to launch the next campaign

`hero.super.start_loop` is the primary loop entrypoint. It accepts optional:
- `super_objective_dsl_path` to override the configured default supervision root for that launch

Super Hero now starts from `super.objective.dsl`, not from a campaign-declared `SUPER` edge. It resolves the selected `super.objective.dsl`, reads its `campaign_dsl_path`, `objective_md_path`, and `guidance_md_path`, copies those authored files into `<runtime_root>/.super_hero/<loop_id>/`, points its mutable `objective_root` at the truth-source objective bundle under `src/config/instructions/objectives/...`, writes a loop-local Config Hero policy at `config.hero.policy.dsl`, builds `super.briefing.md`, runs an initial Codex review before any Runtime campaign exists, and only then decides whether to stop, request human review, or launch the first Runtime campaign. Later reviews follow the same control contract after terminal campaign states. Runtime Hero still snapshots from that truth source on every launch, so campaign execution stays immutable while Super Hero mutations survive `dev_nuke_reset`. Those truth-source mutations are now backed by Config Hero backups under `.backups/hero.super/<objective_name>/`. Only one non-terminal Super loop may own a given `super.objective.dsl` at a time.

Runtime HERO defaults:
- loaded from `instructions/defaults/default.hero.runtime.dsl`
- resolved through `[REAL_HERO].runtime_hero_dsl_filename`
- campaigns root derived from `[GENERAL].runtime_root` as
  `<runtime_root>/.campaigns`
- super-loop root derived from `[GENERAL].runtime_root` as
  `<runtime_root>/.super_hero`
- campaign grammar loaded from `[BNF].iitepi_campaign_grammar_filename`

Super HERO defaults:
- loaded from `instructions/defaults/default.hero.super.dsl`
- resolved through `[REAL_HERO].super_hero_dsl_filename`
- super-loop root derived from `[GENERAL].runtime_root` as `<runtime_root>/.super_hero`
- repo root taken from `[GENERAL].repo_root`
- config scope root taken from `config_scope_root` in `default.hero.super.dsl` and baked into the loop-local Config Hero policy
- Super Hero truth-source mutations also enable Config Hero backups under `.backups/hero.super/<objective_name>/`
- campaign grammar loaded from `[BNF].iitepi_campaign_grammar_filename`
- super-objective grammar loaded from `[BNF].super_objective_grammar_filename`
- `runtime_hero_binary`, `config_hero_binary`, and `lattice_hero_binary` select the MCP binaries Super Hero attaches to a review session
- `human_operator_identities` selects the operator identities file Super Hero uses to bind `operator_id` values to OpenSSH `ssh-ed25519` public keys before resuming a paused loop
- `super_codex_binary` selects the Codex executable or command name used for review
- `super_codex_timeout_sec` bounds one `codex exec` review call
- `super_max_reviews` bounds the number of review turns in one loop, including the initial prelaunch review
- `poll_interval_ms` controls detached loop-runner polling cadence while waiting for terminal campaign state

Human HERO defaults:
- loaded from `instructions/defaults/default.hero.human.dsl`
- resolved through `[REAL_HERO].human_hero_dsl_filename`
- `super_hero_binary` selects the MCP binary Human Hero uses to inspect and resume Super loops
- `operator_id` is recorded into every signed human response artifact
- `operator_signing_ssh_identity` selects the unencrypted OpenSSH `ssh-ed25519` identity Human Hero uses for response signatures

Runtime Hero campaigns persist under the derived campaigns root by `campaign_cursor`, with
`campaign.lls`, `campaign.dsl`, and campaign-level stdout/stderr logs. Child jobs
persist under `<runtime_root>/.campaigns/<campaign_cursor>/jobs/<job_cursor>/`, where
`job_cursor` is derived from the parent `campaign_cursor`, with `job.lls`,
`campaign.dsl`, `binding.contract.dsl`, `binding.wave.dsl`, and worker
stdout/stderr logs plus `job.trace.jsonl`.
`job.trace.jsonl` is a structured append-only execution trace intended for
long-running campaigns; `hero.runtime.tail_trace` exposes the latest phases
without scraping stdout/stderr.
Runtime liveness is reconciled using boot id + process start ticks, not bare pid reuse.

Super loops persist under `<runtime_root>/.super_hero/<loop_id>/` with:
- `loop.lls`
- `super.objective.dsl`
- `super.objective.md`
- `super.guidance.md`
- `config.hero.policy.dsl`
- `memory.md`
- `super.briefing.md`
- `logs/codex.session.log`
- `events.jsonl`
- `human/request.latest.md` when needed
- `human/response.latest.json` when Human Hero has answered
- `human/response.latest.sig` when Human Hero has answered
- `human/responses/human_response.0001.json` plus matching `.sig`
- `reviews/review_packet.0001.json` plus `reviews/latest.json`
- `decisions/decision.0001.json` plus `decisions/latest.json`

Current super-loop schemas:
- `hero.super.loop.v1`
- `hero.super.review_packet.v1`
- `hero.super.decision.v1`

The primary v1 super loop keeps Codex shell access read-only, but gives the review session loop-scoped `hero.config.objective_dsl.read/replace` and `hero.config.objective_campaign.read/replace` tools against the truth-source objective bundle plus bounded Runtime/Lattice read tools for evidence lookup. `read` returns the whole file plus `sha256`, and `replace` atomically writes a whole-file replacement only after the appropriate decoder validation succeeds. Codex returns `control_kind = continue | stop | need_human` plus a bounded `next_action` object with `kind = none | default_plan | binding`, always includes `next_action.reset_runtime_state`, and uses `kind = none` for `stop` or `need_human`. It records actual objective `.dsl` changes in `memory_note` and leaves Super Hero as the only process allowed to request the next campaign launch or stop from Runtime Hero. Review packets now carry `lattice_recommendations`, and the intended evidence order is: review packet first, then `hero.lattice.get_view/get_fact` for semantic judgments, then Runtime tails for operational debugging, with direct file reads as fallback.

When Codex returns `need_human`, Super Hero pauses the loop in `need_human` state, writes `human/request.latest.md`, and waits for Human Hero. Human Hero can be used through MCP tools or run with no arguments on a tty to answer pending requests interactively. It writes a signed `human_response*.json` artifact plus detached `.sig`, then asks `hero.super.resume_loop` to verify the signature and continue or stop the loop.

The short design constitution for that loop lives in [SUPER_LOOP.md](/cuwacunu/src/main/hero/SUPER_LOOP.md).

Deterministic policy:
- Config HERO edits defaults, bounded mutable objective-local `.dsl` files, and explicit objective-local `campaign.dsl` files only. It still excludes the source super-objective constitution. Existing hashimyei instances are not mutated by these tools; instance revisions are handled by Hashimyei HERO lineage.
- `allow_local_write=false` blocks filesystem-mutating Config HERO tools.
- `write_roots` constrains persisted config writes, `instructions/defaults/*.dsl` writes,
  mutable objective-local `.dsl` writes, explicit objective-local `campaign.dsl` writes,
  and `hero.config.dev_nuke_reset` target paths when local writes are enabled.
- `hero.config.dev_nuke_reset` uses the saved global config on disk, not dirty
  unsaved in-memory edits.
- `hero.config.dev_nuke_reset` fails fast while active Runtime HERO jobs or campaigns still exist under `<runtime_root>/.campaigns`.

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

Hashimyei HERO is loaded from `default.hero.hashimyei.dsl` through
`[REAL_HERO].hashimyei_hero_dsl_filename` in the global config, while
`store_root` and `catalog_path` are derived from `[GENERAL].runtime_root`.
Catalog encryption mode is fixed to unencrypted in MCP runtime.

MCP ingest behavior:
- each read tool call fingerprints the catalog-visible store surface and
  rebuilds the catalog when that fingerprint changes.
- `hero.hashimyei.reset_catalog` remains the explicit force-rebuild tool.

Register in Codex:

```bash
codex mcp add hero-hashimyei -- \
  /cuwacunu/.build/hero/hero_hashimyei_mcp
```

Supported MCP tools:
- `hero.hashimyei.list`
- `hero.hashimyei.get_component_manifest`
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
  `GENERAL.default_iitepi_campaign_dsl_filename` points to the checked-in
  default top-level campaign, currently
  `./instructions/defaults/default.iitepi.campaign.dsl`.
  `GENERAL.repo_root` pins the repository/worktree root that Runtime Hero uses
  for `codex exec -C` during super-loop review.
  Runtime reset is intentionally explicit and can be invoked through
  `/cuwacunu/.build/hero/runtime_reset` or `make -C /cuwacunu/src/main reset-runtime`.
- `./instructions/defaults/` holds the canonical example/default payloads,
  including the `instructions/defaults/*.dsl` files that Config HERO is allowed to manage.
- `./instructions/objectives/` holds coherent experiment bundles. The first
  bundle is `vicreg.solo/`, which keeps contract, waves, campaign binds, and
  only the objective-local wrappers that differ from `./instructions/defaults/`.
- `[GUI]` holds iinuji defaults, currently:
  `iinuji_logs_buffer_capacity`, `iinuji_logs_show_date`,
  `iinuji_logs_show_thread`, `iinuji_logs_show_metadata`,
  `iinuji_logs_metadata_filter`, `iinuji_logs_show_color`,
  `iinuji_logs_auto_follow`, `iinuji_logs_mouse_capture`.
- HERO runtime settings for deterministic MCP live in:
  - `./instructions/defaults/default.hero.config.dsl` (Config HERO runtime policy)
  - `./instructions/defaults/default.hero.hashimyei.dsl` (Hashimyei HERO catalog defaults)
  - `./instructions/defaults/default.hero.lattice.dsl` (Lattice HERO catalog/runtime defaults)
  - `./instructions/defaults/default.hero.super.dsl` (Super HERO loop/orchestration defaults)
  - `./instructions/defaults/default.hero.runtime.dsl` (Runtime HERO campaign/job defaults)
  `default.hero.hashimyei.dsl` is the Hashimyei HERO runtime defaults file. It is
  not the founding DSL bundle for component hashimyei lineage.
- `[REAL_HERO]` owns the canonical pointer paths for those five HERO DSL files:
  - `config_hero_dsl_filename`
  - `hashimyei_hero_dsl_filename`
  - `lattice_hero_dsl_filename`
  - `super_hero_dsl_filename`
  - `runtime_hero_dsl_filename`
- All grammar (`*.bnf`) paths are centralized in `[BNF]`:
  - `iitepi_campaign_grammar_filename`
  - `iitepi_runtime_binding_grammar_filename`
  - `iitepi_wave_grammar_filename`
  - `super_objective_grammar_filename`
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
  - `BIND <id> { CONTRACT = <imported_contract_alias>; WAVE = <imported_wave_id>; }`
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
  - Super Hero is the primary persistent post-campaign review loop through
    `hero.super.start_loop(...)`; it starts from `super.objective.dsl`, which
    declares the campaign plus separate human-authored objective and guidance markdown
  - `super.objective.dsl` is its own DSL family, validated by
    `./bnf/objective.super.bnf`, and currently declares
    `campaign_dsl_path:path`, `objective_md_path:path`, `guidance_md_path:path`,
    and optional
    `objective_name:str` and `loop_id:str`
  The same grammar is used by objective-local files such as
  `./instructions/objectives/vicreg.solo/iitepi.campaign.dsl`.
  The defaults bundle also ships sample `./instructions/defaults/default.super.objective.dsl`
  plus `./instructions/defaults/default.super.objective.md` and
  `./instructions/defaults/default.super.guidance.md`.
- Runtime Hero owns campaign dispatch and persists immutable snapshots under
  `<runtime_root>/.campaigns/<campaign_cursor>/`. Each child job receives a staged
  `campaign.dsl`, `binding.contract.dsl`, `binding.wave.dsl`, and
  `job.trace.jsonl` under
  `<runtime_root>/.campaigns/<campaign_cursor>/jobs/<job_cursor>/`.
- Super Hero owns the long-lived loop ledgers under
  `<runtime_root>/.super_hero/<loop_id>/`. The loop ledger is adjacent to
  campaigns rather than nested inside them because one loop may span many
  sequential campaigns.
- Internal runtime-binding snapshots are validated against
  `bnf/iitepi.runtime_binding.bnf` and follow this staged shape:
  - `ACTIVE_BIND <bind_id>;`
  - `RUNTIME_BINDING { ... }`
  - `IMPORT_CONTRACT "<contract_defaults_file>" AS <contract_alias>;`
  - `FROM "<wave_dsl_file>" IMPORT_WAVE <wave_id>;`
  - `BIND <id> { CONTRACT = <imported_contract_alias>; WAVE = <imported_wave_id>; }`
- The public dispatcher is campaign-oriented. The top-level runtime DSL is now
  `campaign.dsl`, and `jkimyei` is not a separate Hero.
- Contract settings live in the checked-in defaults example
  `./instructions/defaults/default.iitepi.contract.dsl` and in objective-local
  contract bundles such as `./instructions/objectives/vicreg.solo/iitepi.contract.base.dsl`, with
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
  Contract-owned module configuration is now explicit through path-bearing
  `__variables` such as:
  `__vicreg_config_dsl_file`,
  `__expected_value_config_dsl_file`,
  `__embedding_sequence_analytics_config_dsl_file`,
  `__transfer_matrix_evaluation_config_dsl_file`,
  together with contract-owned observation/channel DSL selectors.
  In the checked-in `vicreg.solo` objective, VICReg remains objective-local
  while the expected-value and evaluation sidecars explicitly point to the
  shared `../../defaults` payloads.
  Contract `__variables` are resolved across that contract-local DSL graph, so
  public docking values such as input tensor shape and embedding dimensions
  can be defined once by the contract. In the checked-in VICReg defaults,
  private encoder/projector widths now live in the VICReg-owned DSLs rather
  than the contract wrapper. Changing contract-owned docking values still
  changes contract identity and therefore hashimyei compatibility lineage.
  Runtime also derives an explicit contract docking signature from the
  compatible circuit set, contract public docking `__variables`, and
  contract-owned docking surfaces (circuit, VICReg module/network-design,
  and contract-owned observation-channel DSLs when present). Contract-owned
  observation source registries still affect exact contract identity through
  the full dependency/signature graph, but they are not part of the public
  docking digest because unrelated source-row additions should not invalidate
  compatible component weights. Component manifests persist that digest as
  `docking_signature_sha256_hex`, alongside a contract-scoped
  `lineage_state`. Identity is keyed by stable surface ids plus resolved
  content hashes; local checkout-root path spellings are retained for runtime
  diagnostics, but do not change the digest by themselves.
  When wave reuses an existing component hashimyei, runtime validates that the
  selected component manifest has a compatible public docking signature before
  accepting the load. The founding contract hash remains stored as provenance
  in the manifest, but it is no longer the hard runtime acceptance gate.
  When contract-owned observation/channel DSL paths are present together with a
  VICReg network design, contract validation also checks that observation
  active-channel count matches contract `__obs_channels` and `INPUT.C`, that
  max active `seq_length` matches contract `__obs_seq_length` and `INPUT.T`,
  and that contract `__obs_feature_dim` matches `INPUT.D` when declared.
  In the current model, the observation-channel table is the source of truth
  for loader-derived docking values:
  `C = count(active == true)` and
  `T = max(seq_length) over active rows`.
  `__embedding_dims` remains a contract-owned docking width on the VICReg
  output side for downstream component compatibility.
  Runtime load now allows VICReg checkpoint-private topology to differ from
  the current default constructor shape as long as the public docking widths
  remain compatible. In practice, that means private encoder/projector widths
  may vary across component revisions, while `__obs_channels`,
  `__obs_seq_length`, `__obs_feature_dim`, and `__embedding_dims` remain the
  enforced public docking boundary.
  Contract circuit payload declares one or more compatible named circuits, and
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
