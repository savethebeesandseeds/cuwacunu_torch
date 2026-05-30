/*
  wikimyei.representation.mtf_jepa_mae_vicreg.dsl
  =================================================
  Separate experimental MTF-JEPA-MAE-VICReg representation component settings.

  This path is representation-only. It does not replace the production
  channel-preserving VICReg path and does not emit downstream forecast claims.
*/
MTF_JEPA_MAE_VICREG {
  VERSION = wikimyei.representation.mtf_jepa_mae_vicreg.v1;
  COMPONENT_ASSEMBLY_ID = mtf_jepa_mae_vicreg_v1;
  INPUT_ROUTE = channel_node_stream;
  CHANNEL_COUNT = 3;
  HISTORY_LENGTH = 30;
  INPUT_WIDTH = 9;

  DTYPE = float32;
  DEVICE = cuda;
};
