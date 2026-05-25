/*
  wikimyei.inference.expected_value.mdn.dsl
  =================================================
  Strict channel-context ExpectedValue MDN settings.

  This MDN consumes channel node representations [B,N,C,De] and predicts one
  independent one-step mixture distribution per anchor/node/channel/target
  feature slot: log_pi/mu/sigma [B,N,C,Df,K]. Any explicit global context branch
  must use a separate component identity.
*/
MDN {
  VERSION = wikimyei.inference.expected_value.mdn.v1;
  COMPONENT_ASSEMBLY_ID = mdn_v1;
  INPUT_REPRESENTATION_ASSEMBLY_ID = vicreg_v1;
  CONTEXT_MODE = channel_context_strict;
  TARGET_DOMAIN = channel_node_future;
  TARGET_COORDS = 0,1,2,3,4,5,6,7,8;
  TARGET_MASK_POLICY = per_target_feature_valid;
  ACTIVITY_TARGET = node_feature_support_mean;
  SIGMA_MIN = 0.001;
  SIGMA_MAX = 0.0;
  EPS = 0.000001;
};
