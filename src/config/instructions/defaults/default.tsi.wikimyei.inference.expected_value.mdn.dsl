/*
  default.tsi.wikimyei.inference.expected_value.mdn.dsl
  =============================
  Purpose:
    Typed key-value profile for the MDN that models p(F|X) and exposes
    the expected value, i.e. the conditional expectation E[F|X] in the
    configured selected-future-feature space.
    Values are parsed as: key(domain):type = value.

  Format:
    <key>(domain):<type> = <value>   # optional inline comment
    Supported type examples used here:
      int, float, bool, str, arr[int], arr[float]

  Key groups:
    - Runtime/checkpoint: model_path, verbose_train, telemetry_every, device, dtype
    - Required architecture payload binding: network_design_dsl_file
    - Required jkimyei payload binding: jkimyei_dsl_file
    - Training budget: wave-owned (not configured in this file)
    - Target weighting: target_weights
    - Optimization controls: grad_clip, optimizer_threshold_reset

  Semantics:
    - Defines component-local runtime placement, checkpoint path, and payload bindings.
    - ExpectedValue architecture is authored only in `network_design_dsl_file`.
    - The MDN represents a predictive distribution over configured future
      targets conditioned on the VICReg representation X.
    - The primary exposed statistic of this module is E[F|X] in selected future
      space. Under identity preprocessing this is an expected raw future
      feature; under `log_returns` on price fields this is an expected
      log-return, not an arithmetic return or future price.
    - Native/raw-space expectations are separate derived statistics from the
      full MDN distribution and are not the default runtime export.
    - ExpectedValue training policy is authored in its own
      `jkimyei_dsl_file`; it must not point at the VICReg jkimyei profile.
    - Current `target_weights` are applied inside the per-feature component
      log-probability before mixture aggregation. This is pseudo-likelihood /
      tempered-score semantics, not ordinary post-NLL per-feature weighting.
    - Checkpoints are strict format v6 and carry
      `expected_value.head_manifest.v1` compatibility metadata for positional
      target feature indices, future feature schema, MDN architecture, reducer
      policy, loss semantics, and the current representation-frozen value-head
      training recipe.
    - Does not define topology wiring, source range, or wave mode selection.
*/

# Expected-value / expectation profile: key(domain):type = value # optional comment
TAG:str = expected_value.mdn.default
model_path:str = /tmp/expected_value.ckpt # Path to save/load the model
verbose_train:bool = true # Print progress messages
telemetry_every(-1,+inf):int = -1 # Print telemetry cadence (-1 disables)
target_weights:arr[float] = % __future_target_weights ? 0.5 % # Pseudo-likelihood weights for target feature indices
grad_clip(0,1000):float = 1.0 # Gradient clipping value
optimizer_threshold_reset(-1,+inf):int = 500 # Adam/AdamW step-counter clamp threshold (-1 disables)
dtype[kFloat16|kFloat32|kFloat64]:str = kFloat32 # torch dtype token
device[cpu|gpu|cuda:0]:str = gpu # cpu explicit; gpu/cuda strict and must not fall back to CPU
network_design_dsl_file:str = /cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.inference.expected_value.mdn.network_design.dsl
jkimyei_dsl_file:str = /cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.inference.expected_value.mdn.jkimyei.dsl
