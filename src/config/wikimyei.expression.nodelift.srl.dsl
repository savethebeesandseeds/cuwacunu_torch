/*
  wikimyei.expression.nodelift.srl.dsl
  ====================================
  Default deterministic NodeLift component settings for Synthetic Reference
  Lift. This component has no .net file and no .jkimyei file because it is not a
  trainable neural network.

  Supported v1 settings:
    VERSION = wikimyei.expression.nodelift.srl.v1
    FEATURE_WIDTH = 9
    PRICE_COORDS = kline_price
    ACTIVITY_COORDS = kline_activity
    GAUGE_POLICY = uniform_per_component
    PRECISION_POLICY = identity
    ACTIVITY_MODE = support_mean

    LIFT_FUTURE:
      target_side
        Apply the same deterministic SRL/NodeLift transform to future edge
        tensors and emit future_node_features/future_node_mask as supervised
        target-side evidence only.

      disabled
        Do not lift future edge tensors.

    RETURN_ACTIVITY_TOTAL / SUPPORT / COVERAGE:
      true | false sidecar emission switches.

    RETURN_COARSE_MASKS:
      true | false coarse mask sidecar emission switch.
*/
NODELIFT_SRL {
  VERSION = wikimyei.expression.nodelift.srl.v1;
  COMPONENT_ID = nodelift_srl_v1;
  FEATURE_WIDTH = 9;
  PRICE_COORDS = kline_price;
  ACTIVITY_COORDS = kline_activity;
  GAUGE_POLICY = uniform_per_component;
  PRECISION_POLICY = identity;
  ACTIVITY_MODE = support_mean;
  LIFT_FUTURE = target_side;
  RETURN_ACTIVITY_TOTAL = true;
  RETURN_ACTIVITY_SUPPORT = true;
  RETURN_ACTIVITY_COVERAGE = true;
  RETURN_COARSE_MASKS = true;
  EPS = 0.000000000001;
  ACTIVITY_MAX_EXP_ARG = 40.0;
};
