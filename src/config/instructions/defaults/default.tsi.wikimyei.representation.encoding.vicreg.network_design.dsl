/*
  default.tsi.wikimyei.representation.encoding.vicreg.network_design.dsl
  =====================================================
  Purpose:
    Framework-agnostic network design payload for VICReg.
    Decoder output is neutral IR (no torch types).
    Semantic validation and binding to VICReg runtime are done by Wikimyei.

  Notes:
    - This file is intentionally node-oriented (no edge syntax).
    - This file is the sole authored source for VICReg architecture.
    - `ASSEMBLY_TAG = vicreg_encoder_projector` declares the VICReg
      encoder-plus-projector assembly used by this implementation:
        INPUT -> VICREG_RANK4_ENCODER -> MLP
      exports:
        embedding -> encoder node
        projected -> projector node
    - The thin `default.tsi.wikimyei.representation.encoding.vicreg.dsl` wrapper owns
      runtime placement, SWA policy, and payload bindings only.
    - Contract-scoped `__variables` own the public docking widths:
      INPUT.C/Hx/Dx (implemented as C/T/D) and encoder `encoding_dims`.
    - The canonical default bundle also exposes projector option defaults
      through contract-scoped `__vicreg_projector_*` variables so objective
      bundles and defaults share the same architecture-control surface.
    - Contract registration fails fast if decoded observation_channels DSL does
      not resolve to values compatible with these public docking widths.
    - Internal encoder/projector widths below remain module-local defaults.
*/

NETWORK "tsi.wikimyei.representation.encoding.vicreg" {
  ASSEMBLY_TAG = vicreg_encoder_projector;

  node obs@INPUT {
    C:int = % __obs_channels ? 3 %;
    T:int = % __obs_seq_length ? 30 %;
    D:int = % __obs_feature_dim ? 9 %;
  }

  node enc@VICREG_RANK4_ENCODER {
    encoding_dims:int = % __embedding_dims ? 72 %;
    channel_expansion_dim:int = 64;
    fused_feature_dim:int = 32;
    hidden_dims:int = 24;
    depth:int = 10;
  }

  node proj@MLP {
    dims:arr[int] = 72,128,256,128;
    norm:str = % __vicreg_projector_norm ? LayerNorm %;
    activation:str = % __vicreg_projector_activation ? SiLU %;
    hidden_bias:bool = % __vicreg_projector_hidden_bias ? false %;
    last_bias:bool = % __vicreg_projector_last_bias ? false %;
    bn_in_fp32:bool = true;
  }

  node analytics@NETWORK_ANALYTICS_POLICY {
    near_zero_epsilon:float = 1e-8;
    log10_abs_histogram_bins:int = 72;
    log10_abs_histogram_min:float = -12.0;
    log10_abs_histogram_max:float = 6.0;
    include_buffers:bool = true;
    enable_spectral_metrics:bool = true;
    spectral_max_elements:int = 1048576;
    anomaly_top_k:int = 5;
  }

  export embedding = enc;
  export projected = proj;
}
