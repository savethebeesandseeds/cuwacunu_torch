# Hero Catalog Schema (Unified Model v1)

This document defines the minimal shared representation used across
Hashimyei Hero and Lattice Hero catalogs.

## Unified Logical Tables

| Logical Table | Meaning |
| --- | --- |
| `entity` | Stable identified objects (run, component, cell, runtime run). |
| `edge` | Relationships between entities (provenance, binding pointers). |
| `event` | Time-ordered state transitions or control rows (trial, revision, header/counter). |
| `metric_num` | Numeric scalar features and metrics. |
| `metric_txt` | Text scalar features and metrics. |
| `blob` | Content-addressed payload-bearing rows (artifacts, ledgers). |

## Shared Vocabulary Source

- Logical tables: `src/include/hero/hero_catalog_logical_table.def`
- Record kinds and mapping:
  `src/include/hero/hero_catalog_record_kind.def`
- Runtime helpers:
  `src/include/hero/hero_catalog_schema.h`

## Record Kind Mapping (Current)

| Record Kind | Logical Table |
| --- | --- |
| `header` | `event` |
| `ledger` | `blob` |
| `counter` | `event` |
| `run` | `entity` |
| `provenance` | `edge` |
| `component` | `entity` |
| `component_provenance` | `edge` |
| `component_revision` | `event` |
| `component_binding` | `edge` |
| `component_active` | `edge` |
| `artifact` | `blob` |
| `metric_num` | `metric_num` |
| `metric_txt` | `metric_txt` |
| `cell` | `entity` |
| `trial` | `event` |
| `projection_num` | `metric_num` |
| `projection_txt` | `metric_txt` |
| `projection_meta` | `event` |
| `runtime_run` | `entity` |
| `runtime_provenance` | `edge` |
| `runtime_artifact` | `blob` |
| `runtime_ledger` | `blob` |

## Physical Catalogs (Current)

### Hashimyei catalog

- File schema: `hashimyei.catalog.v2`
- Single wide row table with columns:
  `record_kind, record_id, run_id, canonical_path, hashimyei, schema, metric_key,
  metric_num, metric_txt, artifact_sha256, path, ts_ms, payload_json`.

### Lattice catalog

- File schema: `lattice.catalog.v2`
- Single wide row table with columns:
  `record_kind, record_id, cell_id, contract_hash, wave_hash, profile_id,
  execution_profile_json, state_txt, metric_num, text_a, text_b, projection_version,
  ts_ms, payload_json, projection_key, projection_num, projection_txt,
  projection_key_aux, projection_txt_aux,
  started_at_ms, finished_at_ms, ok_txt, total_steps, board_hash, run_id`.
- `projection_meta.payload_json` stores canonical projection payload as
  latent-lineage-state text (`projection_lls`).

## Implementation Status

- Phase 1 complete: shared logical schema and record-kind vocabulary are centralized
  in `.def` files and enforced by both catalog writers.
- Phase 2 (next): reduce duplicated physical columns by moving toward a compact
  table family (`h_entity/h_edge/h_event/h_metric/h_blob`) with typed identity refs.

## Phase 2 Physical Schema Proposal

The next cut should use five narrow physical tables, shared by Hashimyei and Lattice.

### `h_entity`

| Column | Type | Notes |
| --- | --- | --- |
| `entity_id` | text | Primary key (stable id). |
| `entity_kind` | text | `run/component/cell/runtime_run/...` |
| `canonical_path` | text | Optional canonical path. |
| `identity_json` | text | Typed `hashimyei_t` envelope or empty. |
| `owner_scope` | text | `hashimyei` or `lattice`. |
| `schema` | text | Entity payload schema id. |
| `created_at_ms` | int64 | Creation time. |
| `updated_at_ms` | int64 | Last update time. |
| `payload_json` | text | Optional normalized payload snapshot. |

Key/index plan:
- `PK(entity_id)`
- `IDX(entity_kind, canonical_path)`
- `IDX(owner_scope, updated_at_ms)`

### `h_edge`

| Column | Type | Notes |
| --- | --- | --- |
| `edge_id` | text | Primary key. |
| `edge_kind` | text | `provenance/binding/active/...` |
| `src_entity_id` | text | Source entity id. |
| `dst_entity_id` | text | Destination entity id (optional). |
| `owner_scope` | text | `hashimyei` or `lattice`. |
| `schema` | text | Edge schema id. |
| `ts_ms` | int64 | Event/order timestamp. |
| `payload_json` | text | Extra edge attributes. |

Key/index plan:
- `PK(edge_id)`
- `IDX(src_entity_id, edge_kind, ts_ms)`
- `IDX(dst_entity_id, edge_kind, ts_ms)`

### `h_event`

| Column | Type | Notes |
| --- | --- | --- |
| `event_id` | text | Primary key. |
| `event_kind` | text | `trial/revision/header/counter/...` |
| `entity_id` | text | Owning entity id. |
| `owner_scope` | text | `hashimyei` or `lattice`. |
| `schema` | text | Event schema id. |
| `ts_ms` | int64 | Event timestamp. |
| `status_txt` | text | Optional state/status field. |
| `payload_json` | text | Event payload. |

Key/index plan:
- `PK(event_id)`
- `IDX(entity_id, event_kind, ts_ms)`
- `IDX(owner_scope, event_kind, ts_ms)`

### `h_metric`

| Column | Type | Notes |
| --- | --- | --- |
| `metric_id` | text | Primary key. |
| `metric_kind` | text | `metric_num/metric_txt/projection_num/projection_txt/...` |
| `entity_id` | text | Owning entity id. |
| `owner_scope` | text | `hashimyei` or `lattice`. |
| `metric_key` | text | Sparse dimension key. |
| `num_value` | float64 | Nullable numeric value. |
| `txt_value` | text | Nullable text value. |
| `schema` | text | Metric schema id. |
| `ts_ms` | int64 | Metric timestamp. |

Key/index plan:
- `PK(metric_id)`
- `IDX(entity_id, metric_key, ts_ms)`
- `IDX(owner_scope, metric_kind, metric_key)`

### `h_blob`

| Column | Type | Notes |
| --- | --- | --- |
| `blob_id` | text | Primary key (`sha256` or synthetic id). |
| `blob_kind` | text | `artifact/ledger/report/...` |
| `entity_id` | text | Owning entity id (optional). |
| `owner_scope` | text | `hashimyei` or `lattice`. |
| `path` | text | Filesystem/virtual path. |
| `sha256` | text | Content hash when available. |
| `schema` | text | Blob schema id. |
| `ts_ms` | int64 | Blob timestamp. |
| `payload_json` | text | Embedded payload when needed. |

Key/index plan:
- `PK(blob_id)`
- `IDX(entity_id, blob_kind, ts_ms)`
- `IDX(path)`
- `IDX(sha256)`
