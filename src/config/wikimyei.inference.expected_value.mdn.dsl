/*
  wikimyei.inference.expected_value.mdn.dsl
  =================================================
  Strict channel-context ExpectedValue MDN settings.

  This MDN consumes channel node representations [B,N,C,De]. Each target
  channel receives its own context row. Any explicit global context branch must
  use a separate component identity.
*/
MDN {
  VERSION = wikimyei.inference.expected_value.mdn.v1;
  COMPONENT_ID = channel_context_mdn_v1;
  INPUT_REPRESENTATION_ID = vicreg_v1;
  CONTEXT_MODE = channel_context_strict;
  TARGET_DOMAIN = channel_node_future;
  TARGET_COORDS = 0,1,2,3,4,5,6,7,8;
  TARGET_MASK_POLICY = all_target_features_valid;
  ACTIVITY_TARGET = node_feature_support_mean;
  SIGMA_MIN = 0.001;
  SIGMA_MAX = 0.0;
  EPS = 0.000001;
};
