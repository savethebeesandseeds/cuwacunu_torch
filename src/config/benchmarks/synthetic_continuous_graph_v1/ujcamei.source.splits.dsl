/*
  ujcamei.source.splits.dsl
  =========================

  Synthetic benchmark source-owned graph-anchor split intent.

  The main repository split catalog keeps train_core adjacent to evaluation.
  This benchmark catalog leaves an Hx/Hf purge gap before
  certified_replay_expansion_eval so no-lookahead protected-split checks can
  certify replay evidence without weakening Lattice policy.
*/
UJCAMEI_SOURCE_SPLIT_CATALOG {
  CATALOG_ID = graph_anchor_splits_v1;
  CURSOR_DOMAIN = ujcamei.graph_anchor;
};

UJCAMEI_SOURCE_SPLIT {
  SPLIT_ID = train_core;
  ROLE = train;
  SELECTOR = fraction_range;
  BEGIN_FRACTION = 0/1;
  END_FRACTION = 73/117;
  MIN_COUNT = 1;
};

UJCAMEI_SOURCE_SPLIT {
  SPLIT_ID = acceptance_smoke;
  ROLE = train;
  SELECTOR = fraction_range;
  BEGIN_FRACTION = 1/100;
  END_FRACTION = 2/100;
  MIN_COUNT = 1;
};

UJCAMEI_SOURCE_SPLIT {
  SPLIT_ID = validation_holdout;
  ROLE = validation;
  SELECTOR = fraction_range;
  BEGIN_FRACTION = 80/100;
  END_FRACTION = 92/100;
  MIN_COUNT = 1;
};

UJCAMEI_SOURCE_SPLIT {
  SPLIT_ID = certified_replay_expansion_eval;
  ROLE = validation;
  SELECTOR = fraction_range;
  BEGIN_FRACTION = 65/100;
  END_FRACTION = 93/100;
  MIN_COUNT = 1;
};

UJCAMEI_SOURCE_SPLIT {
  SPLIT_ID = test_holdout;
  ROLE = test;
  SELECTOR = fraction_range;
  BEGIN_FRACTION = 93/100;
  END_FRACTION = 100/100;
  MIN_COUNT = 1;
};
