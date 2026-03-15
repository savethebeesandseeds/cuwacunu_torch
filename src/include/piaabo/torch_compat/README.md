# torch_compat Specification

`torch_compat` defines the project-level torch runtime contract and distribution shims.

Namespaces:

- `cuwacunu::piaabo::torch_compat`
- `cuwacunu::piaabo::torch_compat::distributions`

Public include form: `#include "piaabo/torch_compat/..."`

## Core Contract

`torch_compat` standardizes:

- runtime device and dtype selection
- tensor/module validation helpers
- compact tensor diagnostics
- missing distribution surfaces used by project code
- config-to-torch bridge functions in `cuwacunu::iitepi`

## Runtime Defaults and Validation

From implementation behavior:

- `kDevice` defaults to CUDA if available, otherwise CPU
- `kType` defaults to `torch::kFloat32`
- `validate_tensor` and `assert_tensor_shape` fail fast through `log_fatal`
- `validate_module_parameters` enforces defined/non-empty/non-NaN parameters
- `WARM_UP_CUDA()` performs actual CUDA warm-up + sync side effects

## Network Analytics Sidecar

`torch_compat` now exposes a dedicated network-analytics component in:

- `include/piaabo/torch_compat/network_analytics.h`
- `impl/piaabo/torch_compat/network_analytics.cpp`

Public API:

- `summarize_module_network_analytics(...)`
- `summarize_network_design_analytics(...)`
- `network_analytics_to_pretty_text(...)`
- `network_design_analytics_to_pretty_text(...)`
- `write_network_analytics_file(...)`
- `write_network_analytics_sidecar_for_checkpoint(...)`
- `extract_analytics_kv_schema(...)`
- `is_supported_network_analytics_schema(...)`
- `is_supported_network_design_analytics_schema(...)`

Runtime `.lls` note:

- the sidecar/report payloads in this module are now documented under
  `src/include/piaabo/latent_lineage_state/`
- the shared strict validator/canonical emitter in `piaabo` is now the emitted
  runtime `.lls` path for data analytics, network analytics, and entropic-capacity
  sidecars

The checkpoint report writer emits a key/value text file next to `.pt`
checkpoints (extension `.network_analytics.lls`) with compact global parameter
diagnostics.
Every component report can also carry a `tsiemene::component_report_identity_t`
envelope (`report_kind`, `canonical_path`, `tsi_type`, optional
`hashimyei/contract_hash/wave_hash/binding_id`).
Current schema is v4 and keeps canonical-only keys:

- finite/non-finite ratios (NaN/Inf counts)
- scale stats (`stddev`, `l1_mean_abs`, `l2_rms`, `min`, `max`, `max_abs`)
- sparsity (`near_zero_ratio`, `exact_zero_ratio`)
- entropy proxies (`abs_energy_entropy`, `log10_abs_histogram_entropy`)
- tensor-scale stability (`tensor_rms_*`)
- robust magnitude quantiles (`abs_p50`, `abs_p90`, `abs_p99`, `log10_abs_iqr`)
- optional buffer-state diagnostics (`finite_buffer_ratio`, `max_abs_buffer_name`)
- guarded spectral diagnostics for matrix-like tensors (`spectral_norm_*`,
  `stable_rank_*`, `effective_rank_*`, row/col norm CV means)
- global entropic capacity proxy from effective-rank distribution
  (`network_global_entropic_capacity`, `network_entropic_bottleneck_min`,
  `network_effective_rank_p50`, `network_effective_rank_p90`)
- top-k anomaly tables (`top_nonfinite_ratio_*`,
  `top_max_abs_over_rms_*`, `top_near_zero_ratio_*`,
  `top_low_stable_rank_*`, `top_low_effective_rank_*`,
  `top_high_spectral_norm_*`)

## Data Analytics Report

`torch_compat` now includes source-window data analytics in:

- `include/piaabo/torch_compat/data_analytics.h`
- `impl/piaabo/torch_compat/data_analytics.cpp`

The module is now split into:

- generic `sequence_*` analytics/report APIs for any ordered temporal tensor
- source-facing `data_*` wrappers that preserve the existing raw-data report
  schemas and file contracts

Generic sequence entry points operate on canonical sequence layouts already
accepted by the core normalizer (`[T,D]`, `[C,T,D]`, `[B,C,T,D]`) and emit:

- `piaabo.torch_compat.sequence_analytics.v1`
- `piaabo.torch_compat.sequence_analytics_symbolic.v1`

These generic reports are intended for latent/embedding sequences and other
non-source temporal tensors.
For large generalized stream sets, the generic symbolic serializer now
automatically reduces the emitted per-stream section to a representative subset
when `stream_count > 32`, keeping at most `16` streams in the artifact while
preserving full aggregate metrics over the original stream population.
Canonical `.lls` output records:

- `reported_stream_count`
- `omitted_stream_count`
- `stream_report_reduced`
- `stream_report_reduction_mode`

Pretty/debug text adds `/* ... */` comments that explain the generalized-stream
context and any representative-stream reduction.

The VICReg representation runtime now reuses those generic sequence reports for
its latent output and writes representation-owned sidecars in the report
fragment directory:

- `embedding_sequence_analytics.latest.lls`
- `embedding_sequence_analytics.symbolic.latest.lls`

Those runtime sidecars use distinct embedding-facing schemas:

- `piaabo.torch_compat.embedding_sequence_analytics.v1`
- `piaabo.torch_compat.embedding_sequence_analytics_symbolic.v1`

This module computes source entropic load from mask-aware flattened past windows
and writes deterministic key/value reports (`data_analytics.latest.lls`) under
the hashimyei data root.
Schema: `piaabo.torch_compat.data_analytics.v1`.

The same module also emits a compact symbolic sidecar
(`data_analytics.symbolic.latest.lls`) with per-channel:

- `label`, `record_type`, `anchor_feature`, `feature_names`
- `valid_count`, `observed_symbol_count`, `eligible`
- `lz76_complexity`, `lz76_normalized`
- `entropy_rate_bits`
- `information_density`
- `compression_ratio` on the ternary symbolic stream
- `autocorrelation_decay_lag` on the anchor series
- `power_spectrum_entropy` on the anchor series
- `hurst_exponent` via a dyadic R/S estimate

Top-level symbolic summaries emit mean/min/max plus channel labels for:

- `lz76_normalized`
- `information_density`
- `compression_ratio`
- `autocorrelation_decay_lag`
- `power_spectrum_entropy`
- `hurst_exponent`

Schema: `piaabo.torch_compat.data_analytics_symbolic.v1`.
The symbolic sidecar stays comment-free in canonical `.lls`; human-facing pretty
output includes `/* ... */` channel annotations.

Runtime config source: `default.tsi.source.dataloader.sources.dsl` via required block
`DATA_ANALYTICS_POLICY { MAX_SAMPLES, MAX_FEATURES, MASK_EPSILON, STANDARDIZE_EPSILON }`.
No silent fallback defaults are used by `tsi.source.dataloader`.

## Entropic Capacity Comparison Helper

`torch_compat` now includes a comparison sidecar in:

- `include/piaabo/torch_compat/entropic_capacity_comparison.h`
- `impl/piaabo/torch_compat/entropic_capacity_comparison.cpp`

It joins source and network analytics into headline metrics:

- `source_entropic_load`
- `network_entropic_capacity`
- `capacity_margin`
- `capacity_ratio`
- `capacity_regime`

Schema: `piaabo.torch_compat.entropic_capacity_comparison.v1`.

This module is now a compatibility/helper surface, not a standard component-owned
runtime report. The preferred cross-report comparison surface is the query-time
Lattice view
`hero.lattice.get_view_lls(view_kind=entropic_capacity_comparison, ...)`,
which derives directly from the source/network fact reports instead of reading a
component-emitted comparison sidecar back as an intermediate artifact.

The helper still supports payload reducers and standalone writers for tests or
ad hoc tooling, but Hashimyei/Lattice no longer treat
`piaabo.torch_compat.entropic_capacity_comparison.v1` as a first-class ingested
runtime fact.

The network-design analytics API provides topology-level diagnostics directly
from `network_design_instruction_t`. Current schema is v3 and emits only canonical
topology metrics (no legacy aliases):

- graph size/shape (`node_count`, internal/export edge counts, density)
- graph structure (`weakly_connected_components`, `isolated_node_count`,
  cycle detection, topological order count/ratio, DAG longest path)
- directionality (`source_count`, `internal_sink_count`,
  `export_reachable_ratio`, `dead_end_node_count`, `orphan_node_count`)
- path geometry (`longest_source_to_export_path_nodes`,
  `median_source_to_export_path_nodes`, `skip_edge_count`, `mean_skip_span`)
- cycle burden (`scc_count`, `largest_scc_size`, `cyclic_node_ratio`)
- complexity (`active_fanout_mean`, `edge_surplus`)
- edge evidence transparency (`explicit_edge_count`, `inferred_edge_count`,
  unresolved/self-reference token counters)
- entropy proxies over structure (`node_kind_entropy`,
  degree entropies, edge transition entropy)

Network checkpoint-side analytics options are resolved from
`network_design.dsl` through required node kind `NETWORK_ANALYTICS_POLICY`
with strict key validation (unknown/missing keys fail option resolution and
sidecar generation is explicitly skipped with warning).

## Network Design Adapter

`torch_compat` now also exposes an adapter boundary for network-design payloads:

- `include/piaabo/torch_compat/network_design_adapter.h`
- `impl/piaabo/torch_compat/network_design_adapter.cpp`

Boundary contract:

- DSL decoding remains framework-agnostic (`camahjucunu::dsl::network_design`)
- semantic validation remains Wikimyei-owned
- adapter maps validated semantic spec to projector/module options used by LibTorch-facing code

## Config Bridge Contract

Declared in `torch_utils.h` under namespace `cuwacunu::iitepi`:

- `config_dtype(contract_hash, section)`
- `config_device(contract_hash, section)`

Behavior contract:

- reads configured values from config/contract spaces
- invalid dtype/device values raise runtime errors
- CUDA-required config on non-CUDA runtime raises runtime error

## Distribution Surface

Public classes:

- `distributions::Categorical`
- `distributions::Beta`
- `distributions::Gamma`

These classes are the compatibility layer for distribution behavior expected by higher modules.

## Dependency Contract

- requires libtorch (`<torch/torch.h>`)
- diagnostics rely on `piaabo/dlogs.h` through `piaabo/dutils.h`

## Legacy Ghosts

- Runtime warning flags unresolved RNG-seeding policy for libtorch workflows.
- Numeric precision policy is inconsistent (`float` defaults vs noted `double` migration concerns).
- Distribution implementations (`Categorical`, `Beta`) are flagged in source as needing stronger test coverage.
- Distribution precision cleanup (`float` to `double` consistency) is still pending in multiple distribution sources.

This spec defines behavior-level contracts; source remains authoritative for numeric details.
