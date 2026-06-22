/*
  ujcamei.source.cursor.dsl
  =========================
  Shared source cursor catalog for Runtime waves.

  Runtime waves select these entries by SOURCE_CURSOR_ID for source-owned or
  ad hoc windows. Waves that want a named train/validation/test split should use
  SOURCE_SPLIT in hero.runtime.wave.dsl; those bounds are derived from
  ujcamei.source.splits.dsl instead of duplicated here.
*/

UJCAMEI_SOURCE_CURSOR {
  CURSOR_ID = validation_eval_extended_window;
  WINDOW_KIND = ad_hoc_window;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 1600;
  ANCHOR_INDEX_END = 2247;
};

UJCAMEI_SOURCE_CURSOR {
  CURSOR_ID = validation_eval_late_window;
  WINDOW_KIND = ad_hoc_window;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 1630;
  ANCHOR_INDEX_END = 2247;
};

UJCAMEI_SOURCE_CURSOR {
  CURSOR_ID = validation_eval_full_domain;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = all;
};

UJCAMEI_SOURCE_CURSOR {
  CURSOR_ID = train_core_full_domain;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = all;
};

UJCAMEI_SOURCE_CURSOR {
  CURSOR_ID = cwu_01v_train_core_window;
  WINDOW_KIND = ad_hoc_window;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 1600;
};
