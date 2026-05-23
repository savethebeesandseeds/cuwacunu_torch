/*
  wikimyei.representation.vicreg.dsl
  ==========================================
  Channel-preserving VICReg representation component settings.

  This path consumes NodeLift node batches [B,C,Hx,N,9], preserves
  feature-level masks, encodes rows as [M,C,Hx,De], and exports the primary
  channel representation as [B,N,C,De].
*/
VICREG {
  VERSION = wikimyei.representation.vicreg.v1;
  COMPONENT_ID = vicreg_v1;
  INPUT_ROUTE = channel_node_stream;
  CHANNEL_COUNT = 3;
  HISTORY_LENGTH = 30;
  INPUT_WIDTH = 9;

  CELL_VALID_POLICY = required_features;
  REQUIRED_FEATURE_COORDS = 0,1,2,3;
  MIN_VALID_FRACTION = 1.0;
  USE_MISSINGNESS_INDICATORS = true;

  DTYPE = float32;
  DEVICE = cpu;
};
