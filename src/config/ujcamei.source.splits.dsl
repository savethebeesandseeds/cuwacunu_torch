/*
  ujcamei.source.splits.dsl
  =========================

  Source-owned graph-anchor split intent. Fraction selectors are materialized
  against the active ordered accepted-anchor domain at runtime; they do not
  define Lattice proof, purge, leakage, or protection policy.
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
  END_FRACTION = 65/100;
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
