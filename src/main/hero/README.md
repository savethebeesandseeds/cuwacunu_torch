# HERO MCP Runtime Guide

This directory builds four stdio MCP servers:
- `/cuwacunu/.build/hero/hero_config_mcp`
- `/cuwacunu/.build/hero/hero_hashimyei_mcp`
- `/cuwacunu/.build/hero/hero_lattice_mcp`
- `/cuwacunu/.build/hero/hero_runtime_mcp`

## Build

```bash
make -C /cuwacunu/src/main/hero -j12 all
```

## Defaults (No Extra Flags Needed)

All four MCP binaries already default to:
- global config path: `/cuwacunu/src/config/.config`
- unencrypted catalog mode for Hashimyei/Lattice

Hashimyei/Lattice/Runtime runtime paths are loaded from their HERO defaults DSL
files via `[REAL_HERO]` pointers in that default global config.

## Direct One-Shot Tool Calls

Each HERO MCP binary supports direct tool invocation without manual JSON-RPC framing:

```bash
/cuwacunu/.build/hero/hero_config_mcp \
  --tool hero.config.status \
  --args-json '{}'

/cuwacunu/.build/hero/hero_hashimyei_mcp \
  --tool hero.hashimyei.list \
  --args-json '{}'

/cuwacunu/.build/hero/hero_lattice_mcp \
  --tool hero.lattice.get_runs \
  --args-json '{}'

/cuwacunu/.build/hero/hero_runtime_mcp \
  --tool hero.runtime.list_jobs \
  --args-json '{}'
```

The output is the same JSON object shape returned by MCP `tools/call`
(`content`, `structuredContent`, `isError`).

MCP `initialize` responses from all four HERO servers now also include
an `instructions` string with concise startup/preflight guidance.

Optional override flags when needed:
- `--global-config <path>`
- `hero_runtime_mcp` also supports `--hero-config <path>` and `--jobs-root <path>`

## Startup Notes

No passphrase export is required. Hashimyei/Lattice always open unencrypted
catalogs in MCP mode.

Typical startup errors:
- `[hero_hashimyei_mcp] catalog open failed: ...`
- `[hero_lattice_mcp] cannot open catalog: ...`

## Ownership Boundaries

- `Hashimyei Hero` owns identity resolution and canonical founding references.
- `Lattice Hero` owns execution/runtime report fragment streams, exposure, and report queries.
- `Runtime Hero` owns detached process launch, job tracking, log tails, and stop/reconcile controls.
- `Config Hero` owns developer reset/cutover utilities that operate from the saved config view.
- `main_board` remains a worker entrypoint; Runtime Hero is the detached process supervisor keyed by `job_cursor`.
- Hashimyei APIs must remain identity-oriented; runtime report fragment/report endpoints belong to Lattice.

## Naming Conventions

- `report_fragment`: atomic `.lls` payload emitted by one runtime producer (`runtime_report_fragment` rows).
- `report`: joined `.lls` payload at one lattice intersection (`runtime_report` rows and cell `report_lls`).
- `projection`: derived sparse dimensions for indexing/distance, persisted as canonical `projection_lls` with `projection_num`, `projection_txt`, and `projection_meta` sections.
- Joined report sections use `source_report_fragment_*` keys (for example `source_report_fragment_id`, `source_report_fragment_count`).

## Hashimyei Identity v2

Hashimyei MCP uses strict v2 identity payloads for run/component manifests and
exposes an identity-focused public API.

Manifest files:
- `run.manifest.v2.kv` with `schema=hashimyei.run.manifest.v2`
- `component.manifest.v2.kv` with `schema=hashimyei.component.manifest.v2`

Core identity object:
- `schema=hashimyei.identity.v2`
- `kind` in `{BOARD, CONTRACT, WAVE, WAVE_CONTRACT_BINDING, TSIEMENE}`
- `name=0xNNNN` with matching `ordinal`
- `hash_sha256_hex` required only for `{CONTRACT, WAVE, WAVE_CONTRACT_BINDING}`

Binding model:
- `wave_contract_binding.identity`
- `wave_contract_binding.contract`
- `wave_contract_binding.wave`
- `wave_contract_binding.binding_alias`

Hashimyei MCP public query tools:
- `hero.hashimyei.list`
- `hero.hashimyei.get_founding_dsl`
- `hero.hashimyei.get_component_manifest`
- `hero.hashimyei.reset_catalog`

Hashimyei MCP no longer exposes runtime/report endpoints.
Those are owned by Lattice MCP.

## Unified Catalog Model

Hashimyei and Lattice now share a `.def`-driven logical schema vocabulary:

- `entity`
- `edge`
- `event`
- `metric_num`
- `metric_txt`
- `blob`

Canonical mapping details (record kinds, physical columns, and migration notes)
are documented in:

- `src/main/hero/CATALOG_SCHEMA.md`

## Lattice Projection v2

`hero_lattice_mcp` now emits source-runtime lattice axes with explicit
`request.*` vs `effective.*` semantics.

Lattice MCP runtime/report query tools:
- `hero.lattice.get_runs`
- `hero.lattice.list_report_fragments`
- `hero.lattice.get_latest_report_fragment`
- `hero.lattice.get_report_fragment`
- `hero.lattice.list_report_schemas`
- `hero.lattice.get_report_lls`
- `hero.lattice.reset_catalog`

Config MCP developer/runtime utility tools:
- `hero.config.dev_nuke_reset`

Runtime MCP job-control tools:
- `hero.runtime.launch_default_board`
- `hero.runtime.list_jobs`
- `hero.runtime.get_job`
- `hero.runtime.stop_job`
- `hero.runtime.tail_log`
- `hero.runtime.reconcile`

Runtime Hero job model:
- `job_cursor` is the only public job identity
- `pid`, `pgid`, and future `run_id` are job metadata, not public identifiers
- each job persists under `jobs_root/<job_cursor>/`
- job manifests use schema `hero.runtime.job.v1`
- launched workers run as separate Linux processes supervised by a detached runner

`hero.lattice.get_report_lls` returns a joined `report_lls` payload.
For `tsi.source.*` cursors, `get_report_lls` joins both source data analytics
and source-runtime projection report fragments when both are available for the same
wave/hashimyei intersection scope.
The joined payload is persisted in the lattice catalog as a
`runtime_report` row keyed by `intersection_cursor`, so each intersection can
hold one coherent report view while raw component report fragments remain queryable.
Runtime report fragment ingest in Lattice is `.lls`-only.
The effective standalone/synthetic ingest surface today is:
- `wave.source.runtime.projection.v2`
- `piaabo.torch_compat.data_analytics.v1`
- `tsi.wikimyei.representation.vicreg.status.v1`
- `tsi.wikimyei.representation.vicreg.transfer_matrix_evaluation.run.v1`
- `piaabo.torch_compat.entropic_capacity_comparison.v1`
- `piaabo.torch_compat.network_analytics.v4`

Notes:

- `wave.source.runtime.projection.v2` is surfaced synthetically from joined
  report payloads; it is not scanned from disk as a standalone `.lls` file.
- `tsi.wikimyei.representation.vicreg.status.v1` is now persisted as
  `status.latest.lls`, so it is part of the `.lls` file-scan surface.

All ingested standalone runtime `.lls` payloads must include
`run_id:str = <...>`.

Runtime report fragment queries now support two primary cursors:
- `wave_cursor`: packed numeric cursor `[run_id|epoch_k|batch_j]` used for
  mask-based filtering (`wave_cursor_mask`).
- `wave_cursor_view`: readable cursor view `<run>.<epoch>,<batch>` exposed in
  Lattice MCP responses; `wave_cursor` and `intersection_cursor` inputs may use
  either packed integer or this readable form.
- `wave_cursor_resolution`: per-report fragment scope (`run|episode|batch`) used so
  coarse reports can still match finer query cursors (for example: run-level
  report queried with batch-level cursor).
- `hashimyei_cursor`: canonical component path alias (same value as
  `canonical_path`) for hashimyei-oriented lookups.
- `intersection_cursor`: `hashimyei_cursor|wave_cursor` convenience key
  representing one lattice intersection.
- `intersection_cursor_view`: readable `hashimyei_cursor|<run>.<epoch>,<batch>`
  mirror exposed in Lattice MCP responses.

`hero.lattice.list_report_fragments` and `hero.lattice.get_report_lls` accept
`intersection_cursor` directly. When
provided, it drives cursor filtering for the selected lattice intersection.
`hero.lattice.get_latest_report_fragment` accepts the same scope resolution and
returns one fragment body with payload.

`hero.config.dev_nuke_reset` removes runtime dump folders, Runtime Hero jobs_root,
and Hero catalog files resolved from the saved global config, then reloads
Config Hero state. It does not use unsaved in-memory config edits.

For `tsi.source.*`, symbol tokens are normalized out of path identity:
- canonical/hashimyei cursor uses source family identity (for example
  `tsi.source.dataloader`)
- symbol remains in lattice/report features (`source.runtime.symbol`)

Core numeric axes:
- `source.runtime.request.from_ratio`
- `source.runtime.request.to_ratio`
- `source.runtime.request.span_ratio`
- `source.runtime.effective.coverage_ratio`
- `source.runtime.flags.clipped_left`
- `source.runtime.flags.clipped_right`
- `source.channels.active_count`
- `source.channels.total_count`
- `source.channels.active_ratio`
- `source.channel.<interval>.active`

Two projection-flavored runtime payloads exist today:

- embedded source-runtime projection payload in joined `report_lls`
  - schema key: `source.runtime.projection.schema=wave.source.runtime.projection.v2`
  - sections: `projection_num`, `projection_txt`, `audit`
  - audit-only raw epoch-ms fields remain in this embedded payload
- catalog cell `projection_lls`
  - schema key: `schema=wave.projection.lls.v2`
  - metadata keys: `projection_version`, `projector_build_id`
  - sorted numeric and text projection sections only

The current runtime `.lls` profile and migration backlog are documented under
`src/include/piaabo/latent_lineage_state/`.

## Smoke Tests

Quick config MCP smoke:

```bash
/cuwacunu/.build/hero/hero_config_mcp --once status
```

Hashimyei/Lattice smoke (uses temp catalogs):

```bash
mkdir -p /tmp/hero_mcp_smoke/hash /tmp/hero_mcp_smoke/lat

req1='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}'
req2='{"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}}'

{ printf 'Content-Length: %d\r\n\r\n%s' "${#req1}" "$req1"; \
  printf 'Content-Length: %d\r\n\r\n%s' "${#req2}" "$req2"; } \
| timeout 8s /cuwacunu/.build/hero/hero_hashimyei_mcp \
    --global-config /cuwacunu/src/config/.config \
    --store-root /tmp/hero_mcp_smoke/hash \
    --catalog /tmp/hero_mcp_smoke/hash/hashimyei_catalog.idydb

{ printf 'Content-Length: %d\r\n\r\n%s' "${#req1}" "$req1"; \
  printf 'Content-Length: %d\r\n\r\n%s' "${#req2}" "$req2"; } \
| timeout 12s /cuwacunu/.build/hero/hero_lattice_mcp \
    --global-config /cuwacunu/src/config/.config \
    --store-root /tmp/hero_mcp_smoke/lat \
    --catalog /tmp/hero_mcp_smoke/lat/lattice_catalog.idydb \
    --hashimyei-catalog /tmp/hero_mcp_smoke/lat/hashimyei_catalog.idydb
```

Runtime Hero smoke:

```bash
mkdir -p /tmp/hero_mcp_smoke/runtime_jobs

/cuwacunu/.build/hero/hero_runtime_mcp \
  --global-config /cuwacunu/src/config/.config \
  --jobs-root /tmp/hero_mcp_smoke/runtime_jobs \
  --tool hero.runtime.list_jobs \
  --args-json '{}'
```
