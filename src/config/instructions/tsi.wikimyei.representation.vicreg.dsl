/*
  tsi.wikimyei.representation.vicreg.dsl
  ===================
  Purpose:
    Typed key-value profile for VICReg model/runtime settings used by wikimyei.
    Values are explicit and parsed as: key:type = value.

  Format:
    <key>:<type> = <value>   # optional inline comment
    Supported type examples used here:
      int, bool, str

  Key groups:
    - Runtime placement: device, dtype
    - Training budget: wave-owned (not configured in this file)
    - Model shape: encoding_dims, channel_expansion_dim, fused_feature_dim,
      encoder_hidden_dims, encoder_depth
    - Projector config: projector_mlp_spec, projector_norm, projector_activation,
      projector_hidden_bias, projector_last_bias, projector_bn_in_fp32
    - Runtime behavior: enable_buffer_averaging

  Semantics:
    - This file defines component-local VICReg runtime configuration.
    - Wave + jkimyei selection determine whether and how training is executed.
    - Training enable/disable is controlled by wave `WIKIMYEI ... TRAIN`.
    - Profile policy knobs (`swa_start_iter`, `optimizer_threshold_reset`) live in
      `jkimyei.representation.vicreg.dsl` under `[COMPONENT_PARAMS]`.
*/

# VICReg profile: key:type = value # optional comment
encoding_dims:int = 72 # Encoder output embedding dimensions
channel_expansion_dim:int = 64 # Width multiplier for first conv block
fused_feature_dim:int = 32 # Channel count after fusion block
encoder_hidden_dims:int = 24 # Hidden width inside residual blocks
encoder_depth:int = 10 # Number of residual blocks
projector_mlp_spec:str = 128-256-128 # Projector layer widths
projector_norm:str = LayerNorm # BatchNorm1d | LayerNorm | None
projector_activation:str = SiLU # ReLU | SiLU
projector_hidden_bias:bool = false # Ignored when using BatchNorm1d
projector_last_bias:bool = false
projector_bn_in_fp32:bool = true # Used only with BatchNorm1d
enable_buffer_averaging:bool = false # SWA buffer averaging policy
dtype:str = kFloat32 # torch dtype token
device:str = gpu # cpu | cuda:0 | gpu
