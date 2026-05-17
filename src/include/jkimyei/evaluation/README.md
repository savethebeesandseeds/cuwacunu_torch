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

Ujcamei forms source tensors. Piaabo owns generic utilities. Jkimyei owns
evaluation/report helper surfaces.
