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
  SOURCE_CURSOR_ID = train_core.all;
  SOURCE_ORDER = random_per_epoch;
};

WAVE_SETTINGS {
  WAVE_ID = train_core_mtf_jepa_mae_vicreg;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.representation.encoding;
  MODE = train|debug;
  SOURCE_CURSOR_ID = train_core.all;
  SOURCE_ORDER = random_per_epoch;
};

WAVE_SETTINGS {
  WAVE_ID = train_core_channel_mdn;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.inference.expected_value;
  MODE = train|debug;
  SOURCE_CURSOR_ID = train_core.all;
  SOURCE_ORDER = random_per_epoch;
};

WAVE_SETTINGS {
  WAVE_ID = policy_training_pre_ppo_noop;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.policy.portfolio;
  MODE = train;
  JOB_KIND = policy_training;
  POLICY_ID = wikimyei.policy.portfolio.graph_node_allocation.noop_pre_ppo;
  POLICY_KIND = noop_policy_training.v1;
  TRAINING_SCHEDULE_MODE = causal_walk_forward_training.v1;
  LIVE_EXECUTION_ALLOWED = false;
  SOURCE_CURSOR_ID = policy_training.all;
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
  LIVE_EXECUTION_ALLOWED = false;
  SOURCE_CURSOR_ID = policy_training.all;
  SOURCE_ORDER = random_per_epoch;
};
