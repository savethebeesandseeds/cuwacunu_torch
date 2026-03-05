/*
  tsi.wikimyei.representation.vicreg.network_design.dsl
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
*/

NETWORK "tsi.wikimyei.representation.vicreg" {
  JOIN_POLICY = vicreg_default;

  node obs@INPUT {
    C:int = 36;
    T:int = 64;
    D:int = 8;
  }

  node enc@VICREG_4D_ENCODER {
    encoding_dims:int = 72;
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

  export embedding = enc;
  export projected = proj;
}
