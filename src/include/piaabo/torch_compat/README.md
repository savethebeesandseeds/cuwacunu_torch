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
- config-to-torch bridge functions in `cuwacunu::piaabo::dconfig`

## Runtime Defaults and Validation

From implementation behavior:

- `kDevice` defaults to CUDA if available, otherwise CPU
- `kType` defaults to `torch::kFloat32`
- `validate_tensor` and `assert_tensor_shape` fail fast through `log_fatal`
- `validate_module_parameters` enforces defined/non-empty/non-NaN parameters
- `WARM_UP_CUDA()` performs actual CUDA warm-up + sync side effects

## Config Bridge Contract

Declared in `torch_utils.h` under namespace `cuwacunu::piaabo::dconfig`:

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
