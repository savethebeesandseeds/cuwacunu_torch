/*
  wikimyei.representation.mtf_jepa_mae_vicreg.dsl
  =================================================
  Active cwu_02v MTF-JEPA-MAE-VICReg representation component settings.

  SERVING_POOL_POLICY changes only the reduction exposed to downstream
  inference. It does not change representation weights or training objectives.
*/
MTF_JEPA_MAE_VICREG {
  VERSION = wikimyei.representation.mtf_jepa_mae_vicreg.v1;
  COMPONENT_ASSEMBLY_ID = mtf_jepa_mae_vicreg_v1;
  INPUT_ROUTE = channel_node_stream;
  CHANNEL_COUNT = 3;
  HISTORY_LENGTH = 30;
  INPUT_WIDTH = 9;
  SERVING_POOL_POLICY = all_tokens;

  DTYPE = float32;
  DEVICE = cuda;
};
