/*
  default.tsi.wikimyei.representation.vicreg.network_design.dsl
  =====================================================
  Purpose:
    Framework-agnostic network design payload for VICReg.
    Decoder output is neutral IR (no torch types).
    Semantic validation and binding to VICReg runtime are done by Wikimyei.

  Notes:
    - This file is intentionally node-oriented (no edge syntax).
    - `JOIN_POLICY = vicreg_default` delegates join semantics to VICReg code:
        INPUT -> VICREG_4D_ENCODER -> MLP
      exports:
        embedding -> encoder node
        projected -> projector node
    - Contract-scoped `__variables` own the public docking widths:
      INPUT.C/T/D and encoder `encoding_dims`.
    - Contract registration fails fast if decoded observation_channels DSL does
      not resolve to values compatible with these public docking widths.
    - Internal encoder/projector widths below remain module-local defaults.
*/

NETWORK "tsi.wikimyei.representation.vicreg" {
  JOIN_POLICY = vicreg_default;

  node obs@INPUT {
    C:int = % __obs_channels ? 3 %;
    T:int = % __obs_seq_length ? 30 %;
    D:int = % __obs_feature_dim ? 9 %;
  }

  node enc@VICREG_4D_ENCODER {
    encoding_dims:int = % __embedding_dims ? 72 %;
    channel_expansion_dim:int = 64;
    fused_feature_dim:int = 32;
    hidden_dims:int = 24;
    depth:int = 10;
  }

  node proj@MLP {
    dims:arr[int] = 72,128,256,128;
    norm:str = LayerNorm;
    activation:str = SiLU;
    hidden_bias:bool = false;
    last_bias:bool = false;
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
