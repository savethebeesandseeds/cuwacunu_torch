/*
  Train-core MTF-JEPA-MAE-VICReg wave profile.
  Concrete anchor/source-key ranges are launch overlays, not profile identity.
  This is an integration profile only. Production defaults remain on VICReg/MDN,
  and graph-first execution for this target is intentionally not enabled yet.
*/
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
