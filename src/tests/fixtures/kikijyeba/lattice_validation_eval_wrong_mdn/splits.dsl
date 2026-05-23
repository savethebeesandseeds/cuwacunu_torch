LATTICE_SPLIT_POLICY {
  POLICY_ID = wrong_mdn_validation_fixture;
  CURSOR_DOMAIN = ujcamei.graph_anchor;
  PURGE_LEFT_CONTEXT = auto_from_Hx;
  PURGE_RIGHT_FUTURE = auto_from_Hf;
};

LATTICE_SPLIT {
  SPLIT_ID = train_core;
  ROLE = train;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 160;
};

LATTICE_SPLIT {
  SPLIT_ID = validation_holdout;
  ROLE = validation;
  ANCHOR_INDEX_BEGIN = 200;
  ANCHOR_INDEX_END = 210;
  ALLOW_USES = evaluation_metric;
  PROTECT_FROM_USES = observed_input|target_supervision|selection_signal;
  PROTECT_REQUIRES_MUTATED_COMPONENT = true;
};
