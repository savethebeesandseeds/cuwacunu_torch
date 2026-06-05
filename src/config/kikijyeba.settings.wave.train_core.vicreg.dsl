/*
  Train-core VICReg wave profile.
  Concrete anchor/source-key ranges are launch overlays, not profile identity.
  Default Runtime Hero profile remains locked; use runtime_hero_profile =
  train_operator when intentionally executing this profile.
*/
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
