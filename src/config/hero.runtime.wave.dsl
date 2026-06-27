/*
  hero.runtime.wave.dsl
  =====================
  Canonical Runtime wave settings catalog.

  A wave is the Runtime instruction surface that says which protocol-bound
  component family to run and which source cursor or named Lattice split to use.
  The active wave is selected by `[HERO].runtime_wave_id` in the caller config.

  Runtime wave settings do not define topology, model architecture, observer
  identity, allocation-policy identity, source cursor ranges, or replay/paper
  execution policy. Runtime probes live in hero.runtime.probes.dsl; attaching an
  enabled probe still requires this wave's MODE to include debug.
*/
WAVE_SETTINGS {
  WAVE_ID = cwu_02v_validation_holdout_eval_mdn;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.inference.expected_value;
  MODE = run|debug;
  SOURCE_SPLIT = validation_holdout;
};

WAVE_SETTINGS {
  WAVE_ID = cwu_02v_ad_hoc_validation_eval_mdn_extended;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.inference.expected_value;
  MODE = run|debug;
  SOURCE_CURSOR_ID = validation_eval_extended_window;
};

WAVE_SETTINGS {
  WAVE_ID = cwu_02v_certified_replay_eval_mdn;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.inference.expected_value;
  MODE = run|debug;
  SOURCE_SPLIT = certified_replay_expansion_eval;
};

WAVE_SETTINGS {
  WAVE_ID = cwu_02v_ad_hoc_validation_eval_mdn_late;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.inference.expected_value;
  MODE = run|debug;
  SOURCE_CURSOR_ID = validation_eval_late_window;
};

WAVE_SETTINGS {
  WAVE_ID = cwu_01v_validation_eval_channel_mdn;
  PROTOCOL = cwu_01v;
  TARGET = wikimyei.inference.expected_value;
  MODE = run|debug;
  SOURCE_CURSOR_ID = validation_eval_full_domain;
};

WAVE_SETTINGS {
  WAVE_ID = cwu_02v_validation_eval_channel_mdn;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.inference.expected_value;
  MODE = run|debug;
  SOURCE_CURSOR_ID = validation_eval_full_domain;
};

WAVE_SETTINGS {
  WAVE_ID = train_core_vicreg;
  PROTOCOL = cwu_01v;
  TARGET = wikimyei.representation.encoding;
  MODE = train|debug;
  SOURCE_CURSOR_ID = cwu_01v_train_core_window;
  SOURCE_ORDER = random_per_epoch;
};

WAVE_SETTINGS {
  WAVE_ID = train_core_mtf_jepa_mae_vicreg;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.representation.encoding;
  MODE = train|debug;
  SOURCE_SPLIT = train_core;
  SOURCE_ORDER = random_per_epoch;
};

WAVE_SETTINGS {
  WAVE_ID = train_core_channel_mdn;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.inference.expected_value;
  MODE = train|debug;
  SOURCE_SPLIT = train_core;
  SOURCE_ORDER = random_per_epoch;
};

WAVE_SETTINGS {
  WAVE_ID = policy_training_ppo_v0;
  PROTOCOL = cwu_02v;
  TARGET = wikimyei.policy.portfolio;
  MODE = train|debug;
  JOB_KIND = policy_training;
  TRAIN_SPLIT = train_core;
  VALIDATION_SPLIT = certified_replay_expansion_eval;
  TEST_SPLIT = test_holdout;
  LIVE_EXECUTION_ALLOWED = false;
  SOURCE_SPLIT = train_core;
  SOURCE_ORDER = random_per_epoch;
};
