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
- `network_design_analytics_to_pretty_text(...)`
- `write_network_analytics_file(...)`
- `write_network_analytics_sidecar_for_checkpoint(...)`

The sidecar writer emits a key/value text file next to `.pt` checkpoints
(extension `.network_analytics.kv`) with compact global parameter diagnostics:

- finite/non-finite ratios (NaN/Inf counts)
- scale stats (`mean`, `stddev`, `l1_mean_abs`, `l2_rms`, `max_abs`)
- sparsity/sign balance (`near_zero_ratio`, positive/negative/zero ratios)
- entropy proxies (`abs_energy_entropy`, `log10_abs_histogram_entropy`)
- layer-scale stability proxy (`layer_rms_cv`, min/max spread)

The network-design analytics API provides topology-level diagnostics directly
from `network_design_instruction_t`:

- graph size/shape (`node_count`, internal/export edge counts, density)
- graph structure (`weakly_connected_components`, `isolated_node_count`,
  cycle detection, topological coverage, DAG longest path)
- complexity proxies (`branching_factor`, `cyclomatic_complexity`)
- entropy proxies over structure and semantics (`node_kind_entropy`,
  degree entropies, edge transition entropy, parameter key/token entropy)

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
