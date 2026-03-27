/*
  vicreg.solo objective-local network design.

  This file is the sole authored source for VICReg architecture in the
  vicreg.solo objective.
  Public docking stays frozen to big_span_v1 through the contract-owned
  __obs_* and __embedding_dims variables. Private encoder/projector widths and
  projector options are intentionally contract-scoped here so imported
  contract variants can express alternative architecture candidates without
  changing the observation profile.
*/

NETWORK "tsi.wikimyei.representation.vicreg" {
  ASSEMBLY_TAG = vicreg_encoder_projector;

  node obs@INPUT {
    C:int = % __obs_channels ? 3 %;
    T:int = % __obs_seq_length ? 30 %;
    D:int = % __obs_feature_dim ? 9 %;
  }

  node enc@VICREG_4D_ENCODER {
    encoding_dims:int = % __embedding_dims ? 72 %;
    channel_expansion_dim:int = % __vicreg_channel_expansion_dim ? 64 %;
    fused_feature_dim:int = % __vicreg_fused_feature_dim ? 32 %;
    hidden_dims:int = % __vicreg_hidden_dims ? 24 %;
    depth:int = % __vicreg_depth ? 10 %;
  }

  node proj@MLP {
    projector_mlp_spec:str = % __vicreg_projector_mlp_spec ? 72-128-256-128 %;
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
