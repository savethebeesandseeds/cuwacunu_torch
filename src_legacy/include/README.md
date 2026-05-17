## Include Taxonomy

- `source/dataloader`: source-owned domain/runtime helpers for `tsi.source.dataloader`
- `source/evaluation`: source-owned analytics/report wrappers
- `wikimyei/evaluation`: representation-owned analytics/evaluation wrappers
- `piaabo/torch_compat`: reusable analytics kernels and serializers

Runtime report identity now carries three separate ideas:

- `schema`
- `semantic_taxon`
- context: `canonical_path`, `binding_id`, `wave_cursor`, optional `source_runtime_cursor`
- payload: every remaining report-specific key

Read that surface as latest semantic facts by default. Historical selection is
an explicit `wave_cursor` concern, while campaign/job lineage stays in Runtime
Hero state and run manifests rather than the runtime report header.
