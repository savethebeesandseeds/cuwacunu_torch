/*
  hero.lattice.split_policy.dsl
  =============================

  Lattice-owned proof policy over Ujcamei source split ids. Source split ranges
  live in ujcamei.source.splits.dsl; this file owns only purge and leakage
  protection semantics used by target evaluation.
*/
LATTICE_SPLIT_POLICY {
  POLICY_ID = graph_anchor_holdout_v1;
  SOURCE_SPLIT_CATALOG_ID = graph_anchor_splits_v1;
  PURGE_LEFT_CONTEXT = auto_from_Hx;
  PURGE_RIGHT_FUTURE = auto_from_Hf;
};

LATTICE_SPLIT_PROTECTION {
  SPLIT_ID = validation_holdout;
  ALLOW_USES = evaluation_metric;
  PROTECT_FROM_USES = observed_input|target_supervision|selection_signal;
  PROTECT_REQUIRES_MUTATED_COMPONENT = true;
};

LATTICE_SPLIT_PROTECTION {
  SPLIT_ID = certified_replay_expansion_eval;
  ALLOW_USES = evaluation_metric;
  PROTECT_FROM_USES = observed_input|target_supervision|selection_signal;
  PROTECT_REQUIRES_MUTATED_COMPONENT = true;
};

LATTICE_SPLIT_PROTECTION {
  SPLIT_ID = test_holdout;
  ALLOW_USES = evaluation_metric;
  PROTECT_FROM_USES = observed_input|target_supervision|selection_signal;
  PROTECT_REQUIRES_MUTATED_COMPONENT = true;
};
