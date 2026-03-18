# Runtime `.lls` Schema Registry

This registry covers the runtime `.lls` surfaces standardized by this pass.

Status values:

- `conforming now`: matches the standardized profile for its class today
- `artifact-only`: runtime artifact retained outside the current Lattice ingest set
- `reader-only`: accepted by readers, but not produced as a current runtime artifact

## Produced Runtime Payloads

| Schema / payload | Class | Canonical path subject | Required keys now | Hero/Lattice status | Status |
| --- | --- | --- | --- | --- | --- |
| `wave.source.runtime.projection.v2` | standalone artifact | `tsi.source.dataloader` | `schema`, context header (`canonical_path`, `binding_id`, `wave_cursor`), `source_runtime_cursor`; no `semantic_taxon` required in v1 | ingestable | conforming now |
| `wave.projection.lls.v2` | catalog-only payload | Hero/Lattice | `schema`, `projection_version`, `projector_build_id` | stored in lattice cells, not a report fragment file | conforming now |
| `piaabo.torch_compat.data_analytics.v2` | standalone artifact | `tsi.source.dataloader` | `schema`, `semantic_taxon`, context header (`canonical_path`, `binding_id`, `wave_cursor`), optional `source_runtime_cursor` | ingestable | conforming now |
| `piaabo.torch_compat.data_analytics_symbolic.v2` | standalone artifact | `tsi.source.dataloader` | `schema`, `semantic_taxon`, context header (`canonical_path`, `binding_id`, `wave_cursor`), optional `source_runtime_cursor` | ingestable | conforming now |
| `piaabo.torch_compat.embedding_sequence_analytics.v2` | standalone artifact | `tsi.wikimyei.representation.vicreg.0x<hashimyei>` | `schema`, `semantic_taxon`, context header (`canonical_path`, `binding_id`, `wave_cursor`), optional `source_runtime_cursor` | ingestable | conforming now |
| `piaabo.torch_compat.embedding_sequence_analytics_symbolic.v2` | standalone artifact | `tsi.wikimyei.representation.vicreg.0x<hashimyei>` | `schema`, `semantic_taxon`, context header (`canonical_path`, `binding_id`, `wave_cursor`), optional `source_runtime_cursor` | ingestable | conforming now |
| `piaabo.torch_compat.network_analytics.v5` | standalone artifact | `tsi.wikimyei.representation.vicreg.0x<hashimyei>` | `schema`, `semantic_taxon`, context header (`canonical_path`, `binding_id`, `wave_cursor`), optional `source_runtime_cursor` | ingestable | conforming now |
| `tsi.wikimyei.representation.vicreg.transfer_matrix_evaluation.run.v1` | standalone artifact | `tsi.wikimyei.representation.vicreg.0x<hashimyei>` | `schema`, `semantic_taxon`, context header (`canonical_path`, `binding_id`, `wave_cursor`), optional `source_runtime_cursor` | ingestable | conforming now |
| `tsi.wikimyei.representation.vicreg.transfer_matrix_evaluation.matrix.v1` | artifact-only payload | `tsi.wikimyei.representation.vicreg.0x<hashimyei>` | `schema`, `semantic_taxon`, context header (`canonical_path`, `binding_id`, `wave_cursor`), optional `source_runtime_cursor` | not on current ingest allow-list | artifact-only |
| `tsi.wikimyei.representation.vicreg.status.v1` | standalone artifact | `tsi.wikimyei.representation.vicreg.0x<hashimyei>` | `schema`, context header (`canonical_path`, `binding_id`, `wave_cursor`), optional `source_runtime_cursor`, status fields | ingestable | conforming now |

## Reader-Only or Non-Artifact Runtime Schemas

| Schema | Current role | Notes | Status |
| --- | --- | --- | --- |
| `piaabo.torch_compat.entropic_capacity_comparison.v1` | legacy helper / reader compatibility | query-time comparison now lives in Lattice; no standard component producer or ingest path | reader-only |
| `piaabo.torch_compat.network_design_analytics.v4` | API serializer only | emitted by helper APIs, not persisted as a standard runtime artifact | reader-only |

## Canonical Required Keys by Class

Standalone artifact:

- `schema`
- `semantic_taxon` for analytics/evaluation reports
- `canonical_path`
- `binding_id`
- `wave_cursor`
- optional `source_runtime_cursor` for source-selected reports
- payload keys

Interpretation:

- standalone runtime artifacts are latest semantic facts by default
- `wave_cursor` is the explicit historical selector when a non-latest context is requested
- campaign/job lineage belongs to Runtime Hero state and run manifests, not to the shared report header

Catalog-only payload:

- `schema`
- catalog-owned metadata keys
- deterministic sorted payload body

Artifact-only payload:

- `schema`
- schema-local report keys
- no current ingest requirement

## Canonicalization Targets

The following are the main gaps between live payloads and the canonical emitted form:

- API-only network-design analytics remains on the broader latent-lineage path
- joined Hero `report_lls` remains a separate synthetic transport, not a strict
  standalone runtime `.lls` document
- derived Lattice views such as `entropic_capacity_comparison` remain query-time
  transports and are intentionally not listed as persisted runtime schemas
- synthetic joined transports may use `/* ... */` comment boundaries for human
  readability only

Those deltas are mapped file-by-file in `MIGRATION_BACKLOG.md`.
