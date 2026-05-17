/*
  wikimyei.inference.expected_value.mdn.dsl
  ==============================================
  Default per-node ExpectedValue MDN for graph-first smoke tests.
*/
MDN {
  VERSION = wikimyei.inference.expected_value.mdn.v1;
  COMPONENT_ID = mdn_v1;
  INPUT_REPRESENTATION_ID = node_vicreg_v1;
  TARGET_DOMAIN = node_future;
  TARGET_COORDS = 0,1,2,3,4,5,6,7,8;
  TARGET_MASK_POLICY = all_target_features_valid;
  ACTIVITY_TARGET = node_feature_support_mean;
  HEAD_POLICY = per_node;
  CONTEXT_REDUCTION = last;
  MIXTURE_COUNT = 2;
  HIDDEN_WIDTH = 16;
  RESIDUAL_DEPTH = 1;
  SIGMA_MIN = 0.001;
  SIGMA_MAX = 0.0;
  EPS = 0.000001;
};
