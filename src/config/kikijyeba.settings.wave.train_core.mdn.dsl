/*
  Train-core channel MDN wave profile. Concrete anchor/source-key ranges are
  launch overlays, not profile identity. The MDN jkimyei file must be
  materialized with INPUT_REPRESENTATION_CHECKPOINT before execution.
*/
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
