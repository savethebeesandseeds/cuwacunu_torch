/*
  wikimyei.representation.vicreg.dsl
  ==================================
  Default graph-first VICReg representation component settings.

  VICReg remains a rank-4 representation worker. The node stream adapter maps
  node-lifted tensors [B,C,H,N,9] into [B*N,C,H,9]. Neural architecture lives in
  wikimyei.representation.vicreg.net.

  Supported v1 settings:
    INPUT_ROUTE = node_stream
      Consume NodeLift node batches through the node-stream adapter.

    INPUT_WIDTH = 9
      Must match kline NodeLift feature width.

    MASK_PROFILE:
      all_9 | price_only | close_only | activity_only | custom

    REQUIRED_FEATURE_COORDS:
      Required only for MASK_PROFILE = custom. For other profiles, the profile
      expands to a fixed coordinate list.

    DTYPE:
      float32 | float64

    DEVICE:
      cpu | cuda | cuda:N
*/
VICREG {
  VERSION = wikimyei.representation.vicreg.v1;
  COMPONENT_ID = node_vicreg_v1;
  INPUT_ROUTE = node_stream;
  INPUT_WIDTH = 9;

  MASK_PROFILE = price_only;
  REQUIRED_FEATURE_COORDS = 0,1,2,3;

  DTYPE = float32;
  DEVICE = cpu;
};
