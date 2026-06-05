/*
  hero.runtime.wave.dsl
  ===========================
  Canonical Runtime wave settings catalog.

  A wave is the Runtime instruction surface that says how the graph-first
  procedure is launched. The file may contain many WAVE_SETTINGS blocks. The
  active wave is selected by `[HERO].runtime_wave_id` in the
  caller config; WAVE_SELECTION.ACTIVE_WAVE_ID is only the checked-in fallback.

  Runtime wave settings do not define topology, model architecture, observer
  identity, allocation-policy identity, or replay/paper execution policy. Those
  live in protocol, Wikimyei policy, or Marshal/Runtime rollout contracts.

  Reusable train profiles keep SOURCE_RANGE=all. Concrete anchor/source-key
  ranges are launch overlays, not profile identity.
*/
WAVE_SELECTION {
  ACTIVE_WAVE_ID = cwu_02v_channel_validation_eval_mdn_1800_2050;
};

WAVE_SETTINGS {
  WAVE_ID = cwu_02v_channel_validation_eval_mdn_1800_2050;
  COMPATIBLE_PROTOCOLS = cwu_02v;
  TARGET = wikimyei.inference.expected_value.mdn;
  MODE = run|debug;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 1800;
  ANCHOR_INDEX_END = 2050;
};

WAVE_SETTINGS {
  WAVE_ID = validation_eval_channel_mdn;
  COMPATIBLE_PROTOCOLS = cwu_01v,cwu_02v;
  TARGET = wikimyei.inference.expected_value.mdn;
  MODE = run|debug;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = all;
};

WAVE_SETTINGS {
  WAVE_ID = train_core_vicreg;
  COMPATIBLE_PROTOCOLS = cwu_01v;
  TARGET = wikimyei.representation.encoding.vicreg;
  MODE = train|debug;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = all;
  SOURCE_ORDER = random_per_epoch;
};

WAVE_SETTINGS {
  WAVE_ID = train_core_mtf_jepa_mae_vicreg;
  COMPATIBLE_PROTOCOLS = cwu_02v;
  TARGET = wikimyei.representation.encoding.mtf_jepa_mae_vicreg;
  MODE = train|debug;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = all;
  SOURCE_ORDER = random_per_epoch;
};

WAVE_SETTINGS {
  WAVE_ID = train_core_channel_mdn;
  COMPATIBLE_PROTOCOLS = cwu_01v,cwu_02v;
  TARGET = wikimyei.inference.expected_value.mdn;
  MODE = train|debug;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = all;
  SOURCE_ORDER = random_per_epoch;
};

WAVE_SETTINGS {
  WAVE_ID = policy_training_pre_ppo_noop;
  COMPATIBLE_PROTOCOLS = cwu_02v;
  TARGET = wikimyei.policy.trainable;
  MODE = train;
  JOB_KIND = policy_training;
  POLICY_ID = wikimyei.policy.trainable.noop_pre_ppo;
  POLICY_KIND = noop_policy_training.v1;
  TRAINING_SCHEDULE_MODE = causal_walk_forward_training.v1;
  LIVE_EXECUTION_ALLOWED = false;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = all;
  SOURCE_ORDER = random_per_epoch;
};
