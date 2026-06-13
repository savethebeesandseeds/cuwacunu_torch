/*
  ujcamei.source.cursor.dsl
  =========================
  Shared source cursor catalog for Runtime waves.

  Runtime waves select these entries by SOURCE_CURSOR_ID. Cursor identities are
  source-level and protocol-neutral; protocol-specific component choices live in
  the Runtime wave and Kikijyeba protocol DSLs.
*/
UJCAMEI_SOURCE_CURSOR {
  CURSOR_ID = validation_eval.1800_2050;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 1800;
  ANCHOR_INDEX_END = 2050;
};

UJCAMEI_SOURCE_CURSOR {
  CURSOR_ID = validation_eval.all;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = all;
};

UJCAMEI_SOURCE_CURSOR {
  CURSOR_ID = train_core.all;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = all;
};

UJCAMEI_SOURCE_CURSOR {
  CURSOR_ID = policy_training.all;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = all;
};
