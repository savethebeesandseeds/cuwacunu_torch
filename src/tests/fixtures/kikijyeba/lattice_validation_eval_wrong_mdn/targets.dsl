LATTICE_TARGET {
  TARGET_ID = fixture_vicreg_train_core_ready;
  TARGET_KIND = legacy_node_vicreg_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_OBSERVED_INPUT_COVERAGE = 1.0;
  PROTECT_SPLIT = validation_holdout;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};

LATTICE_TARGET {
  TARGET_ID = fixture_legacy_node_mdn_train_core_ready;
  TARGET_KIND = legacy_node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
  UPSTREAM_TARGET_ID = fixture_vicreg_train_core_ready;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  REQUIRE_ACTIVE_NODE_HEAD_COUNT = 1;
  REQUIRE_TRAINED_NODE_HEAD_COUNT = 1;
  MIN_TARGET_SUPERVISION_COVERAGE = 1.0;
  PROTECT_SPLIT = validation_holdout;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};

LATTICE_TARGET {
  TARGET_ID = fixture_legacy_node_mdn_no_validation_leakage;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = legacy_node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  CHECKPOINT_SOURCE = latest_satisfying:fixture_legacy_node_mdn_train_core_ready;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  PROTECT_SPLIT = validation_holdout;
  PLAN_MODE = run|debug;
  MAX_WAVES = 0;
};

LATTICE_TARGET {
  TARGET_ID = fixture_legacy_node_mdn_validation_eval_ready;
  TARGET_CLASS = evaluation_readiness;
  TARGET_KIND = legacy_node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = validation_holdout;
  UPSTREAM_TARGET_ID = fixture_legacy_node_mdn_no_validation_leakage;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = false;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 0;
  MIN_VALID_TARGET_FRACTION = 0.05;
  REQUIRE_EVALUATED_NODE_HEAD_COUNT = 1;
  EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:fixture_legacy_node_mdn_no_validation_leakage;
  MIN_EVALUATION_METRIC_COVERAGE = 1.0;
  PROTECT_SPLIT = validation_holdout;
  PLAN_MODE = run|debug;
  MAX_WAVES = 1;
};
