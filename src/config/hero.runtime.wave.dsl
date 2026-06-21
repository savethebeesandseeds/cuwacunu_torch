/*
  hero.runtime.wave.dsl
  =====================
  Canonical Runtime wave settings catalog.

  A wave is the Runtime instruction surface that says which protocol-bound
  component family to run and which predeclared Ujcamei source cursor to use.
  The active wave is selected by `[HERO].runtime_wave_id` in the caller config;
  WAVE_SELECTION.ACTIVE_WAVE_ID is only the checked-in fallback.

  Runtime wave settings do not define topology, model architecture, observer
  identity, allocation-policy identity, source cursor ranges, or replay/paper
  execution policy. Those live in protocol, Wikimyei policy, Ujcamei cursor, or
  Marshal/Runtime rollout contracts.
*/
WAVE_SELECTION {
  ACTIVE_WAVE_ID = cwu_02v_channel_validation_eval_mdn_1800_2050;
};

WAVE_SETTINGS {
  WAVE_ID = cwu_02v_channel_validation_eval_mdn_1800_2050;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.inference.expected_value;
  MODE = run|debug;
  SOURCE_CURSOR_ID = validation_eval.1800_2050;
};

WAVE_SETTINGS {
  WAVE_ID = cwu_02v_channel_validation_eval_mdn_1600_2247;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.inference.expected_value;
  MODE = run|debug;
  SOURCE_CURSOR_ID = validation_eval.1600_2247;
};

WAVE_SETTINGS {
  WAVE_ID = cwu_02v_channel_validation_eval_mdn_1200_2247;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.inference.expected_value;
  MODE = run|debug;
  SOURCE_CURSOR_ID = validation_eval.1200_2247;
};

WAVE_SETTINGS {
  WAVE_ID = cwu_02v_channel_validation_eval_mdn_1630_2247;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.inference.expected_value;
  MODE = run|debug;
  SOURCE_CURSOR_ID = validation_eval.1630_2247;
};

WAVE_SETTINGS {
  WAVE_ID = cwu_01v_validation_eval_channel_mdn;
  PROTOCOL = cwu_01v;
  TARGET = wikimyei.inference.expected_value;
  MODE = run|debug;
  SOURCE_CURSOR_ID = validation_eval.all;
};

WAVE_SETTINGS {
  WAVE_ID = cwu_02v_validation_eval_channel_mdn;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.inference.expected_value;
  MODE = run|debug;
  SOURCE_CURSOR_ID = validation_eval.all;
};

WAVE_SETTINGS {
  WAVE_ID = train_core_vicreg;
  PROTOCOL = cwu_01v;
  TARGET = wikimyei.representation.encoding;
  MODE = train|debug;
  SOURCE_CURSOR_ID = train_core.0_1600;
  SOURCE_ORDER = random_per_epoch;
};

WAVE_SETTINGS {
  WAVE_ID = train_core_mtf_jepa_mae_vicreg;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.representation.encoding;
  MODE = train|debug;
  SOURCE_CURSOR_ID = train_core.0_1170;
  SOURCE_ORDER = random_per_epoch;
};

WAVE_SETTINGS {
  WAVE_ID = train_core_channel_mdn;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.inference.expected_value;
  MODE = train|debug;
  SOURCE_CURSOR_ID = train_core.0_1170;
  SOURCE_ORDER = random_per_epoch;
};

WAVE_SETTINGS {
  WAVE_ID = policy_training_ppo_v0;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.policy.portfolio;
  MODE = train;
  JOB_KIND = policy_training;
  POLICY_ID = wikimyei.policy.portfolio.graph_node_allocation.ppo_v0;
  POLICY_KIND = ppo_policy_adapter.v1;
  TRAINING_SCHEDULE_MODE = causal_walk_forward_training.v1;
  CAUSAL_SCHEDULE_DIGEST = policy_training_causal_walk_forward_all_v1;
  SNAPSHOT_FAMILY_DIGEST = policy_training_causal_snapshots.cwu_02v.graph_anchor.v1;
  EARLY_STOPPING_POLICY_DIGEST = validation_only_patience_ppo_v0_v1;
  HYPERPARAMETER_SELECTION_POLICY_DIGEST = validation_only_grid_ppo_v0_v1;
  SELECTOR_POLICY_DIGEST = validation_selector_no_test_access_ppo_v0_v1;
  TRAIN_SPLIT = train_core;
  VALIDATION_SPLIT = certified_replay_expansion_eval;
  TEST_SPLIT = test_holdout;
  LIVE_EXECUTION_ALLOWED = false;
  SOURCE_CURSOR_ID = policy_training.all;
  SOURCE_ORDER = random_per_epoch;
};
