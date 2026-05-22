/*
  wikimyei.inference.expected_value.mdn.dsl
  =========================================
  Default per-node ExpectedValue MDN component settings. Neural architecture
  lives in wikimyei.inference.expected_value.mdn.net.

  This is the active node-centered ExpectedValue surface. It predicts
  future_node_features from target-side future NodeLift, not future edge
  features.

  Supported v1 settings:
    TARGET_DOMAIN = node_future
    TARGET_COORDS = comma-separated zero-based feature coordinates.
      The default 0,1,2,3,4,5,6,7,8 means all kline node features.

    TARGET_MASK_POLICY = all_target_features_valid
      A node target at [row,C,Hf] is valid only when every selected target
      coordinate is valid in future_node_mask.

    ACTIVITY_TARGET = node_feature_support_mean
      Activity coordinates inside future_node_features are support-normalized
      intensities. Total activity targets would need an explicit future
      activity sidecar policy.

    HEAD_POLICY = per_node
      One MDN head per stable node slot. Checkpoints are tied to node_ids and
      graph_order_fingerprint.

    CONTEXT_REDUCTION = last | mean
      How sequence-shaped node encodings [B,N,H,D] are reduced before MDN
      input. [B,N,D] encodings pass through directly.
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
  SIGMA_MIN = 0.001;
  SIGMA_MAX = 0.0;
  EPS = 0.000001;
};
