/*
  wikimyei.representation.vicreg.dsl
  ===================
  Default graph-first VICReg representation worker used by smoke tests.

  VICReg remains a rank-4 representation worker. The node stream adapter maps
  node-lifted tensors [B,C,H,N,9] into [B*N,C,H,9].
*/
VICREG {
  VERSION = wikimyei.representation.vicreg.v1;
  COMPONENT_ID = node_vicreg_v1;
  INPUT_ROUTE = node_stream;
  INPUT_WIDTH = 9;

  ENCODING_DIM = 8;
  CHANNEL_EXPANSION_DIM = 4;
  FUSED_FEATURE_DIM = 8;
  ENCODER_HIDDEN_DIM = 16;
  ENCODER_DEPTH = 1;

  MASK_PROFILE = price_only;
  REQUIRED_FEATURE_COORDS = 0,1,2,3;

  DTYPE = float32;
  DEVICE = cpu;
};
