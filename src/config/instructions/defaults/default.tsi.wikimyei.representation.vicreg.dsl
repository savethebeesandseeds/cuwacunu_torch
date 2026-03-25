/*
  default.tsi.wikimyei.representation.vicreg.dsl
  ===================
  Purpose:
    Typed key-value profile for VICReg model/runtime settings used by wikimyei.
    Values are explicit and parsed as: key(domain):type = value.

  Format:
    <key>(domain):<type> = <value>   # optional inline comment
    Supported type examples used here:
      int, bool, str

  Key groups:
    - Runtime placement: device, dtype
    - Optional architecture override payload: network_design_dsl_file
    - Required jkimyei payload binding: jkimyei_dsl_file
    - Training budget: wave-owned (not configured in this file)
    - Model shape: encoding_dims, channel_expansion_dim, fused_feature_dim,
      encoder_hidden_dims, encoder_depth
    - Projector config: projector_mlp_spec, projector_norm, projector_activation,
      projector_hidden_bias, projector_last_bias, projector_bn_in_fp32
    - Runtime behavior: enable_buffer_averaging

  Semantics:
    - This file defines component-local VICReg runtime configuration.
    - Contract-scoped `__variables` own only the public docking widths here:
      input shape (`__obs_channels`, `__obs_seq_length`, `__obs_feature_dim`)
      and encoder output width (`__embedding_dims`).
    - Internal encoder/projector widths in this file remain module-local
      implementation defaults.
    - Wave + jkimyei profile selection determine whether and how training is executed.
    - Training enable/disable is controlled by wave root `MODE` + `WIKIMYEI ... JKIMYEI.HALT_TRAIN`.
    - Profile policy knobs (`swa_start_iter`, `optimizer_threshold_reset`) are
      loaded from `jkimyei_dsl_file`.
*/

# VICReg profile: key(domain):type = value # optional comment
encoding_dims(1,4096):int = % __embedding_dims ? 72 % # Encoder output embedding dimensions
channel_expansion_dim(1,4096):int = 64 # Width multiplier for first conv block
fused_feature_dim(1,4096):int = 32 # Channel count after fusion block
encoder_hidden_dims(1,4096):int = 24 # Hidden width inside residual blocks
encoder_depth(1,512):int = 10 # Number of residual blocks
projector_mlp_spec:str = 128-256-128 # Projector layer widths
projector_norm[BatchNorm1d|LayerNorm|None]:str = LayerNorm # BatchNorm1d | LayerNorm | None
projector_activation[ReLU|SiLU]:str = SiLU # ReLU | SiLU
projector_hidden_bias:bool = false # Ignored when using BatchNorm1d
projector_last_bias:bool = false
projector_bn_in_fp32:bool = true # Used only with BatchNorm1d
enable_buffer_averaging:bool = false # SWA buffer averaging policy
dtype[kFloat16|kFloat32|kFloat64]:str = kFloat32 # torch dtype token
device[cpu|gpu|cuda:0]:str = gpu # cpu | cuda:0 | gpu
network_design_dsl_file:str = /cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.representation.vicreg.network_design.dsl
jkimyei_dsl_file:str = /cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.representation.vicreg.jkimyei.dsl
