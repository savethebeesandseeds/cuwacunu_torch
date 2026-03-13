# Current Runtime `.lls` Audit

This audit records what is deployed today. It intentionally separates actual runtime
behavior from desired future standard behavior.

## Runtime Artifact Inventory

| Payload | Producer | Main consumers | Surface | Ingest class | Current notes |
| --- | --- | --- | --- | --- | --- |
| `wave.source.runtime.projection.v2` | `src/include/hero/lattice_hero/source_runtime_projection.h` | `src/impl/hero/lattice_hero/lattice_catalog.cpp` synthetic fragment queries | embedded inside joined `report_lls`, no standalone file | embedded synthetic | strict runtime `.lls`, namespaced `source.runtime.projection.schema`, fixed 12 digits |
| `wave.projection.lls.v2` | `src/impl/hero/lattice_hero/lattice_catalog.cpp` | lattice catalog cells / MCP report joins | stored in catalog `projection_lls`, no standalone file | catalog-only | top-level `schema`, `projection_version`, `projector_build_id`, sorted numeric/text sections, fixed 12 digits |
| `piaabo.torch_compat.data_analytics.v1` | `src/impl/piaabo/torch_compat/data_analytics.cpp` | entropic-capacity comparison, Hero Hashimyei/Lattice ingest | `.hashimyei/.../data_analytics.latest.lls` | standalone, ingestable | strict runtime `.lls`, fixed 12 digits, common identity envelope when provided |
| `piaabo.torch_compat.network_analytics.v4` | `src/impl/piaabo/torch_compat/network_analytics.cpp` | entropic-capacity comparison, Hero Hashimyei/Lattice ingest | `<checkpoint>.network_analytics.lls` | standalone, ingestable | strict runtime `.lls`, fixed 12 digits, top-k collection flattening uses `_count`, `_1_name`, `_1_value` |
| `piaabo.torch_compat.entropic_capacity_comparison.v1` | `src/impl/piaabo/torch_compat/entropic_capacity_comparison.cpp` | Hero Hashimyei/Lattice ingest | `<checkpoint>.entropic_capacity.lls` | standalone, ingestable | strict runtime `.lls`, fixed 12 digits, common identity envelope when provided |
| `tsi.wikimyei.representation.vicreg.transfer_matrix_evaluation.run.v1` | `src/include/wikimyei/evaluation/transfer_evaluation_matrix/transfer_matrix_evaluation.h` wrapped by `src/include/tsiemene/tsi.wikimyei.representation.vicreg.h` | Hero Hashimyei/Lattice ingest | `transfer_matrix_evaluation.summary.latest.lls` | standalone, ingestable | strict runtime `.lls`, fixed 12 digits, canonical `canonical_path` envelope |
| `tsi.wikimyei.representation.vicreg.transfer_matrix_evaluation.matrix.v1` | same as above | humans / direct artifact readers | `transfer_matrix_evaluation.matrix.latest.lls` | artifact-only | strict runtime `.lls`, fixed 12 digits, remains outside the current Lattice ingest allow-list |
| `tsi.wikimyei.representation.vicreg.status.v1` | `src/include/tsiemene/tsi.wikimyei.representation.vicreg.h` | known-schema allow-lists in Hero | `status.latest.lls` | standalone, ingestable | strict runtime `.lls`, canonical `canonical_path`, deterministic fallback `run_id` when no live run id is available |

## Reader Surfaces In Use Today

Grammar-backed parser:

- `src/impl/camahjucunu/dsl/latent_lineage_state/latent_lineage_state.cpp`
- used for config/instruction `.dsl` decoding, not as the common runtime `.lls` reader

Standardized runtime validator/emitter foundation:

- `src/config/bnf/runtime_lls.bnf`
- `src/include/piaabo/latent_lineage_state/runtime_lls.h`
- `src/impl/piaabo/latent_lineage_state/runtime_lls.cpp`
- shared strict reader/emitter for the persisted runtime `.lls` surfaces listed above

Legacy line readers still in tree:

- `parse_kv_payload(...)` in `src/impl/hero/lattice_hero/lattice_catalog.cpp`
- `parse_latent_lineage_state_payload(...)` in `src/impl/hero/hashimyei_hero/hashimyei_catalog.cpp`
- schema-local scanners in `src/impl/piaabo/torch_compat/data_analytics.cpp`,
  `src/impl/piaabo/torch_compat/network_analytics.cpp`, and
  `src/impl/piaabo/torch_compat/entropic_capacity_comparison.cpp`

Common deployed rule across those remaining legacy readers:

- normalize LHS with `extract_latent_lineage_state_lhs_key(...)`
- drop full-line `#` comments
- keep last assignment for duplicate keys

## Current Producer Behavior

Comments:

- migrated runtime producers now emit canonical comment-free `.lls`
- legacy relaxed readers still ignore full-line `#` comments where they remain in
  non-runtime transport/config payloads

Typed/domain LHS usage:

- migrated runtime/report writers now emit through
  `emit_runtime_lls_canonical(...)`
- migrated runtime/report writers can now use declared domains as
  interpretation/reference ranges instead of only type-level placeholders
- the remaining API-only network-design serializer still uses the broader latent
  lineage normalization path and is out of the strict persisted runtime-artifact
  contract

Duplicate keys:

- strict runtime readers reject duplicate keys as invalid input
- last-write-wins behavior remains only in the legacy relaxed readers listed above

Ordering:

- source projection and lattice cell projection explicitly sort numeric/text axes
- network top-k collections sort deterministically before flattening
- canonical emission now centralizes runtime field ordering across the migrated
  runtime producers

Numeric formatting:

- source projection, lattice cell projection, and transfer-matrix payloads use fixed
  12-digit fractional formatting
- all migrated runtime producers now use fixed 12-digit fractional formatting

Collection encoding:

- network analytics uses flattened keys with a count plus 1-based siblings:
  `top_nonfinite_ratio_count`, `top_nonfinite_ratio_1_name`,
  `top_nonfinite_ratio_1_value`, ...
- transfer-matrix prequential rows use dotted zero-padded namespaces:
  `prequential.0000.method`, `prequential.0000.block_index`, ...

Identity envelope:

- `torch_compat` data/network writers can emit the common
  `component_report_identity_t` envelope (`report_kind`, `tsi_type`,
  `canonical_path`, `hashimyei`, `contract_hash`, `wave_hash`, `binding_id`,
  `run_id`)
- entropic-capacity payloads can now emit the common identity envelope when the
  caller provides it
- transfer-matrix wrapper now emits canonical `canonical_path`
- source projection embedded payloads use the source-runtime namespace rather than
  the common top-level identity envelope

## Code-vs-Doc Mismatches

1. `src/config/man/latent_lineage_state.man` documents `#`, `;`, `//`, and
   `/* ... */` as comments, but deployed runtime readers only guarantee full-line
   `#` comments. The new runtime `.lls` standard narrows this further to `/* ... */`
   only, so producer/reader migration is still required.

2. Earlier Hero docs listed
   `tsi.wikimyei.representation.vicreg.status.v1` among accepted runtime `.lls`
   report schemas before the producer was migrated. The producer now writes
   `status.latest.lls`.

3. Hero uses two distinct projection payload shapes:
   `source.runtime.projection.schema=wave.source.runtime.projection.v2` for the
   embedded source-runtime subdocument, and `schema=wave.projection.lls.v2` for the
   catalog cell `projection_lls`. The earlier documentation treated them as one
   surface.

4. Declared type/domain metadata is part of the shared LHS syntax, but many runtime
   consumers flatten to semantic keys immediately and do not preserve the declared
   metadata after parse.

5. API-only network-design analytics still uses the older normalization path; it is
   intentionally outside the strict persisted runtime-artifact surface.

6. Internal non-runtime data structures still carry some `canonical_base` field
   names, even though the strict runtime payload surface now emits `canonical_path`.

## Representative Payload Sources

- source projection fixture shape: `src/tests/bench/hashimyei/test_source_runtime_projection.cpp`
- Lattice ingest expectations: `src/tests/bench/hashimyei/test_lattice_catalog.cpp`
- relaxed syntax / duplicate-key behavior: `src/tests/bench/camahjucunu/test_dsl_latent_lineage_state.cpp`
- data/network/entropic compatibility checks:
  `src/tests/bench/iitepi/test_entropic_capacity_analytics.cpp`
