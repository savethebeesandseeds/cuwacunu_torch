/*
  Validation-holdout channel MDN evaluation profile. Concrete anchor/source-key
  ranges are launch overlays, not profile identity. The MDN jkimyei file must be
  materialized with both INPUT_REPRESENTATION_CHECKPOINT and INPUT_MDN_CHECKPOINT
  before execution.
*/
WAVE_SETTINGS {
  WAVE_ID = validation_eval_channel_mdn;
  COMPATIBLE_PROTOCOLS = cwu_01v,cwu_02v;
  TARGET = wikimyei.inference.expected_value.mdn;
  MODE = run|debug;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = all;
};
