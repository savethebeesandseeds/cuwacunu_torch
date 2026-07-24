/*
  Exact graph-anchor split catalog for synthetic_continuous_graph_v2.

  The 4096-anchor domain leaves an unassigned 64-anchor purge before each
  downstream split. acceptance_smoke is wholly contained in train_core.
*/
UJCAMEI_SOURCE_SPLIT_CATALOG {
  CATALOG_ID = graph_anchor_splits_v2;
  CURSOR_DOMAIN = ujcamei.graph_anchor;
};

UJCAMEI_SOURCE_SPLIT {
  SPLIT_ID = train_core;
  ROLE = train;
  SELECTOR = anchor_index_range;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 2496;
  MIN_COUNT = 1;
};

UJCAMEI_SOURCE_SPLIT {
  SPLIT_ID = acceptance_smoke;
  ROLE = train;
  SELECTOR = anchor_index_range;
  ANCHOR_INDEX_BEGIN = 64;
  ANCHOR_INDEX_END = 128;
  MIN_COUNT = 1;
};

UJCAMEI_SOURCE_SPLIT {
  SPLIT_ID = validation_holdout;
  ROLE = validation;
  SELECTOR = anchor_index_range;
  ANCHOR_INDEX_BEGIN = 2560;
  ANCHOR_INDEX_END = 2816;
  MIN_COUNT = 1;
};

UJCAMEI_SOURCE_SPLIT {
  SPLIT_ID = certified_replay_expansion_eval;
  ROLE = validation;
  SELECTOR = anchor_index_range;
  ANCHOR_INDEX_BEGIN = 2880;
  ANCHOR_INDEX_END = 3264;
  MIN_COUNT = 1;
};

UJCAMEI_SOURCE_SPLIT {
  SPLIT_ID = test_holdout;
  ROLE = test;
  SELECTOR = anchor_index_range;
  ANCHOR_INDEX_BEGIN = 3328;
  ANCHOR_INDEX_END = 4096;
  MIN_COUNT = 1;
};
