# HERO MCP Runtime Guide

This directory builds three stdio MCP servers:
- `/cuwacunu/.build/hero/hero_config_mcp`
- `/cuwacunu/.build/hero/hero_hashimyei_mcp`
- `/cuwacunu/.build/hero/hero_lattice_mcp`

## Build

```bash
make -C /cuwacunu/src/main/hero -j12 all
```

## Defaults (No Extra Flags Needed)

All three MCP binaries already default to:
- global config path: `/cuwacunu/src/config/.config`
- unencrypted catalog mode for Hashimyei/Lattice

Hashimyei/Lattice runtime paths (`store_root`, `catalog_path`, etc.) are loaded from
their HERO defaults DSL files via `[REAL_HERO]` pointers in that default global config.

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
```

The output is the same JSON object shape returned by MCP `tools/call`
(`content`, `structuredContent`, `isError`).

MCP `initialize` responses from all three HERO servers now also include
an `instructions` string with concise startup/preflight guidance.

Optional override flags when needed:
- `--global-config <path>`

## Startup Notes

No passphrase export is required. Hashimyei/Lattice always open unencrypted
catalogs in MCP mode.

Typical startup errors:
- `[hero_hashimyei_mcp] catalog open failed: ...`
- `[hero_lattice_mcp] cannot open catalog: ...`

## Ownership Boundaries

- `Hashimyei Hero` owns identity resolution and canonical founding references.
- `Lattice Hero` owns execution/runtime history, exposure, and report queries.
- Hashimyei APIs must remain identity-oriented; report/history endpoints belong to Lattice.

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
- `hero.lattice.get_history`
- `hero.lattice.get_report_lls`
- `hero.lattice.reset_catalog`

`hero.lattice.get_report_lls` returns a joined `report_lls` payload.
For `tsi.source.*` cursors, `get_report_lls` joins both source data analytics
and source-runtime projection artifacts when both are available for the same
wave/hashimyei intersection scope.
Runtime report ingest in Lattice is `.lls`-only and accepts:
- `wave.source.runtime.projection.v2`
- `piaabo.torch_compat.data_analytics.v1`
- `tsi.wikimyei.representation.vicreg.status.v1`
- `tsi.wikimyei.representation.vicreg.transfer_matrix_evaluation.run.v1`
- `piaabo.torch_compat.entropic_capacity_comparison.v1`
- `piaabo.torch_compat.network_analytics.v4`

All ingested runtime `.lls` payloads must include `run_id=<...>`.

Runtime artifact queries now support two primary cursors:
- `wave_cursor`: packed numeric cursor `[run_id|episode_k|batch_j]` used for
  mask-based filtering (`wave_cursor_mask`).
- `wave_cursor_resolution`: per-artifact scope (`run|episode|batch`) used so
  coarse reports can still match finer query cursors (for example: run-level
  report queried with batch-level cursor).
- `hashimyei_cursor`: canonical component path alias (same value as
  `canonical_path`) for hashimyei-oriented lookups.
- `intersection_cursor`: `hashimyei_cursor|wave_cursor` convenience key
  representing one lattice intersection.

`hero.lattice.get_history` and `hero.lattice.get_report_lls` accept
`intersection_cursor` directly. When
provided, it drives cursor filtering for the selected lattice intersection.

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

Projection `.lls` payload (`projection_lls`) includes:
- numeric dimensions (former `axis_num`), including all source-runtime ratios/flags
- text dimensions (former `axis_txt`), for example `source.runtime.symbol`
- tags (former `tags`), for example `source.runtime.range_basis`
  and `source.runtime.interval_semantics`

Projection metadata:
- `projection_version = 2`
- `projector_build_id = wave.projector.v2`

Raw epoch-ms fields are audit-only and are appended to joined provenance payloads
(`joined_kv_report`) as latent-lineage formatted text, not to `axis_num`.

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
