# Current Runtime `.lls` Audit

This audit records what is deployed today. It intentionally separates actual runtime
behavior from desired future standard behavior.

## Runtime Artifact Inventory

| Payload | Producer | Main consumers | Surface | Ingest class | Current notes |
| --- | --- | --- | --- | --- | --- |
| `wave.source.runtime.projection.v2` | `src/main/tools/cuwacunu_campaign.cpp` via `src/include/hero/lattice_hero/source_runtime_projection.h` | Hero Lattice ingest, assembled fact bundles, humans / direct artifact readers | `source_runtime_projection.latest.lls` under the canonical source leaf `ujcamei/source/retrieval/contracts/<contract_token>/bindings/<binding>/runs/<run_id>/` | standalone, ingestable | strict runtime `.lls`, top-level `schema`, context header (`canonical_path`, `binding_id`, `wave_cursor`), optional context extension `source_runtime_cursor`, explicit `contract_hash`, fixed 12 digits |
| `wave.projection.lls.v2` | `src/impl/hero/lattice_hero/lattice_catalog.cpp` | lattice catalog cells / MCP report joins | stored in catalog `projection_lls`, no standalone file | catalog-only | top-level `schema`, `projection_version`, `projector_build_id`, sorted numeric/text sections, fixed 12 digits |
| `jkimyei.evaluation.data_analytics.v2` | `src/impl/jkimyei/evaluation/source/data_analytics_public.cpp` | Lattice entropic-capacity view inputs, Hero Hashimyei/Lattice ingest | `.runtime/.hashimyei/.../ujcamei/source/retrieval/contracts/<contract_token>/contexts/<source_runtime_cursor>/data_analytics.v2.latest.lls` | standalone, ingestable | strict runtime `.lls`, fixed 12 digits, current `evaluation_report_identity_t` header, explicit `contract_hash`, `source_entropic_load`, and invalid positions excluded from numeric statistics |
| `jkimyei.evaluation.data_analytics_symbolic.v2` | `src/impl/jkimyei/evaluation/source/data_analytics_public.cpp` | Hero Hashimyei/Lattice ingest, humans / direct artifact readers | `.runtime/.hashimyei/.../ujcamei/source/retrieval/contracts/<contract_token>/contexts/<source_runtime_cursor>/data_analytics.symbolic.v2.latest.lls` | standalone, ingestable | strict runtime `.lls`, fixed 12 digits, flattened `channel_<n>_*` keys, current `evaluation_report_identity_t` header, explicit `contract_hash`, and comment-free canonical emission |
| `jkimyei.evaluation.embedding_sequence_analytics.v2` | `src/impl/jkimyei/evaluation/source/data_analytics_public.cpp` | Hero Hashimyei/Lattice ingest, humans / direct artifact readers | `embedding_sequence_analytics.v2.latest.lls` in the component report directory | standalone, ingestable | strict runtime `.lls`, fixed 12 digits, current `evaluation_report_identity_t` header, and generic sequence envelope over latent encodings reshaped to per-dimension streams |
| `jkimyei.evaluation.embedding_sequence_analytics_symbolic.v2` | `src/impl/jkimyei/evaluation/source/data_analytics_public.cpp` | Hero Hashimyei/Lattice ingest, humans / direct artifact readers | `embedding_sequence_analytics.symbolic.v2.latest.lls` in the component report directory | standalone, ingestable | strict runtime `.lls`, fixed 12 digits, current `evaluation_report_identity_t` header, flattened `stream_<n>_*` keys, representative-stream reduction when latent width is large |
| `jkimyei.evaluation.network_analytics.v5` | `src/impl/jkimyei/evaluation/representation/network_analytics_public.cpp` | Lattice entropic-capacity view inputs, Hero Hashimyei/Lattice ingest | `<checkpoint>.network_analytics.lls` | standalone, ingestable | strict runtime `.lls`, fixed 12 digits, current `evaluation_report_identity_t` header, `network_global_entropic_capacity`, and top-k collection flattening uses `_count`, `_1_name`, `_1_value` |

## Reader Surfaces In Use Today

Grammar-backed parser:

- `src/impl/camahjucunu/dsl/latent_lineage_state/latent_lineage_state.cpp`
- used for config/instruction `.dsl` decoding, not as the common runtime `.lls` reader

Standardized runtime validator/emitter foundation:

- `src/config/grammar/kikijyeba.lattice.runtime.bnf`
- `src/include/kikijyeba/lattice/runtime_report/runtime_lls.h`
- `src/impl/kikijyeba/lattice/runtime_report/runtime_lls.cpp`
- shared strict reader/emitter for the persisted runtime `.lls` surfaces listed above

Lattice query-time view surfaces:

- `hero.lattice.list_views`
- `hero.lattice.get_view`
- current view kind: `entropic_capacity_comparison`
- current selector form: required `wave_cursor`, optional `canonical_path` narrowing, optional `contract_hash`
- derived views are not persisted runtime report fragments in this pass
- read tools synchronize the lattice catalog to the current runtime store
  before querying; `hero.lattice.refresh(reingest=true)` remains the explicit
  force-rebuild path

Legacy comparison helper:

- `src/impl/jkimyei/evaluation/entropic_capacity_comparison.cpp`
- still provides payload reducers and canonical text serialization for compatibility tests
- no standard component runtime currently emits or ingests
  `jkimyei.evaluation.entropic_capacity_comparison.v1`

Legacy line readers still in tree:

- `parse_kv_payload(...)` in `src/impl/hero/lattice_hero/lattice_catalog.cpp`
- `parse_latent_lineage_state_payload(...)` in `src/impl/hero/hashimyei_hero/hashimyei_catalog.cpp`
- schema-local scanners in `src/impl/jkimyei/evaluation/data_analytics.cpp`,
  `src/impl/jkimyei/evaluation/network_analytics.cpp`, and
  `src/impl/jkimyei/evaluation/entropic_capacity_comparison.cpp`

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

- the documented runtime report header is now:
  `schema`, `semantic_taxon`, `canonical_path`, `binding_id`, `wave_cursor`
- source-selected reports may also carry `source_runtime_cursor` derived from
  `SOURCE.RUNTIME.{RUNTIME_INSTRUMENT_SIGNATURE,FROM,TO}`
- the default reading is latest fact first; historical retrieval is explicit via
  `wave_cursor`, while campaign/run lineage remains adjacent in runtime state
  and manifests
- analytics/evaluation reports use the simplified taxonomy
  `source.data`, `embedding.data`, `embedding.evaluation`, `embedding.network`
- `evaluation` data/network writers emit only the simplified report header
- entropic-capacity helper payloads can still emit the common simplified header
  when the caller provides it, but that helper is no longer part of the
  standard component runtime artifact set
- operational reports such as status and source runtime projection use the same
  context header and may omit `semantic_taxon` in v1

## Code-vs-Doc Mismatches

1. `src/config/man/latent_lineage_state.man` documents the broader typed
   latent-lineage syntax, including multiple comment forms, while strict runtime
   `.lls` producers now emit comment-free documents and readers should not rely
   on comments in persisted runtime report artifacts.

2. Older VICReg standalone report files can still exist in archived runtime
   stores. They are not current graph-first component runtime `.lls` producers
   and should not be used as authored Wikimyei report surfaces.

3. Hero uses two distinct projection payload shapes:
   `schema=wave.source.runtime.projection.v2` for the standalone source-runtime
   fact, and `schema=wave.projection.lls.v2` for the catalog cell
   `projection_lls`. The embedded projection text inside joined transports still
   uses `source.runtime.projection.schema=wave.source.runtime.projection.v2`.

4. Declared type/domain metadata is part of the shared LHS syntax, but many runtime
   consumers flatten to semantic keys immediately and do not preserve the declared
   metadata after parse.

5. API-only network-design analytics still uses the older normalization path; it is
   intentionally outside the strict persisted runtime-artifact surface.

6. Non-runtime helper surfaces now use `canonical_path` consistently where they
   describe a stable subject identity, matching the strict runtime payload
   surface documented around `schema`, `semantic_taxon`, `canonical_path`,
   `binding_id`, `wave_cursor`, optional `source_runtime_cursor`, and payload.

## Representative Payload Sources

- source projection fixture shape: `src/tests/bench/hashimyei/test_source_runtime_projection.cpp`
- Lattice ingest expectations: `src/tests/bench/hashimyei/test_lattice_catalog.cpp`
- relaxed syntax / duplicate-key behavior: `src/tests/bench/camahjucunu/test_dsl_latent_lineage_state.cpp`
- data/network/entropic compatibility checks:
  `src/tests/bench/iitepi/test_entropic_capacity_analytics.cpp`
