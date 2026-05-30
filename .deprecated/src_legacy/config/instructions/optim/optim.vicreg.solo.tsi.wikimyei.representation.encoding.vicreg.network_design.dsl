/*
  Frozen optimized network design for vicreg.solo.

  Promoted from the winning baseline medium-dock candidate:
    channel_expansion_dim = 64
    fused_feature_dim = 32
    hidden_dims = 24
    depth = 10
    projector = 72-128-256-128 with LayerNorm + SiLU and bias disabled
*/

NETWORK "tsi.wikimyei.representation.encoding.vicreg" {
  ASSEMBLY_TAG = vicreg_encoder_projector;

  node obs@INPUT {
    C:int = 3;
    T:int = 30;
    D:int = 9;
  }

  node enc@VICREG_4D_ENCODER {
    encoding_dims:int = 72;
    channel_expansion_dim:int = 64;
    fused_feature_dim:int = 32;
    hidden_dims:int = 24;
    depth:int = 10;
  }

  node proj@MLP {
    projector_mlp_spec:str = 72-128-256-128;
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
