# Jkimyei Evaluation

Evaluation helpers and report artifact surfaces live here.

This is not a Torch compatibility layer. These files describe and serialize
evaluation/report identities used by training, inference, and runtime audit
flows. Evaluation headers are split by the thing being evaluated:

- `source/data_analytics.h`: source, sequence, and embedding sequence
  analytics.
- `source/source_data_analytics.h`: source-data analytics adapter helpers.
- `representation/network_analytics.h`: network and network-design analytics
  reports used by representation/inference models.
- `inference/entropic_capacity_comparison.h`: reader/helper surface for
  entropic-capacity comparisons.
- `evaluation_report_identity.h`: current runtime `.lls` identity envelope used
  by evaluation emitters. Active evaluation code should not depend on Tsiemene
  report identities.

The old load/capacity vocabulary is still useful, but it now travels through the
strict runtime `.lls` surface:

- source reports emit `source_entropic_load`
- network reports emit `network_global_entropic_capacity`
- capacity-comparison reports derive `capacity_margin`, `capacity_ratio`, and
  `capacity_regime`

Source analytics artifact paths default under the canonical runtime root at
`.runtime/cuwacunu_exec/components/jkimyei.evaluation.source.data_analytics/spawns/standalone_runtime/artifacts/retrieval/ujcamei/source/retrieval`;
set `CUWACUNU_EVALUATION_STORE_ROOT` to redirect that store.

Ujcamei forms source tensors. Piaabo owns generic utilities. Jkimyei owns
evaluation/report helper surfaces.
