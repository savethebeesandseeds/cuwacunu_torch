/*
  default.tsi.wikimyei.inference.mdn.value_estimation.dsl
  =============================
  Purpose:
    Typed key-value profile for expected-value / mixture-density model settings.
    Values are parsed as: key(domain):type = value.

  Format:
    <key>(domain):<type> = <value>   # optional inline comment
    Supported type examples used here:
      int, float, bool, str, arr[int], arr[float]

  Key groups:
    - Artifact/runtime: model_path, verbose_train, telemetry_every, device, dtype
    - Training budget: wave-owned (not configured in this file)
    - Target definition: target_dims, target_weights
    - Model structure: mixture_comps, features_hidden, residual_depth
    - Optimization controls: grad_clip, optimizer_threshold_reset

  Semantics:
    - Defines model-local numeric and optimization policy.
    - Does not define topology wiring, source range, or wave mode selection.
*/

# Value estimation profile: key(domain):type = value # optional comment
model_path:str = /tmp/value_estimation.ckpt # Path to save/load the model
verbose_train:bool = true # Print progress messages
telemetry_every(-1,+inf):int = -1 # Print telemetry cadence (-1 disables)
target_dims:arr[int] = 3 # Feature dims to track
target_weights:arr[float] = 0.5 # Weights for target_dims
mixture_comps(1,1024):int = 10 # Mixture components K
features_hidden(1,8192):int = 12 # Hidden width
residual_depth(0,512):int = 3 # Residual depth
grad_clip(0,1000):float = 1.0 # Gradient clipping value
optimizer_threshold_reset(-1,+inf):int = 500 # Adam/AdamW step-counter clamp threshold (-1 disables)
dtype[kFloat16|kFloat32|kFloat64]:str = kFloat32 # torch dtype token
device[cpu|gpu|cuda:0]:str = gpu # cpu | cuda:0 | gpu
