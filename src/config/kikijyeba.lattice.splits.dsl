/*
  kikijyeba.lattice.splits.dsl
  ============================

  Named graph-anchor cursor ranges for lattice target evaluation.

  This file is not a scheduler and does not execute waves. It gives lattice
  targets stable names for train/validation/test ranges so target declarations
  do not carry raw holdout math.

  CURSOR_DOMAIN:
    ujcamei.graph_anchor
      V1 split intervals are graph-wide accepted-anchor index ranges. They are
      meaningful only with the same graph_order_fingerprint and
      source_cursor_token recorded in runtime exposure facts.

  PURGE_LEFT_CONTEXT / PURGE_RIGHT_FUTURE:
    auto_from_Hx / auto_from_Hf expand protected split footprints during
    forbidden-overlap checks. The evaluator derives the left context and right
    future windows from runtime exposure facts, preferring source_input_length
    and source_future_length when available and falling back to observed/target
    source-row footprints. This means PROTECT_SPLIT = validation_holdout guards
    the validation interval plus the temporal context/future rows that could be
    consumed by nearby training anchors.

  SPLIT ROLE:
    train | validation | test | unknown

  SPLIT PROTECTION:
    Validation and test splits can declare default protection policy with:

      ALLOW_USES = evaluation_metric;
      PROTECT_FROM_USES = observed_input|target_supervision|selection_signal;
      PROTECT_REQUIRES_MUTATED_COMPONENT = true;

    Targets can then say PROTECT_SPLIT = validation_holdout and let the target
    compiler/evaluator lower the split's protection defaults into the existing
    forbidden-exposure proof. This keeps holdout policy near the split instead
    of repeating low-level forbidden-use lists inside every target.

  Usage:
    A target may use TRAIN_SPLIT = train_core to resolve its SOURCE_RANGE to
    anchor_index with that split's begin/end. A target may use PROTECT_SPLIT =
    validation_holdout to reject checkpoint closures whose protected exposure
    footprints overlap the validation split.

    The acceptance_smoke split is intentionally tiny. It exists to prove the
    runtime -> exposure fact -> checkpoint fact -> lattice target loop without
    changing the real train/validation/test ranges.
*/
LATTICE_SPLIT_POLICY {
  POLICY_ID = graph_anchor_holdout_v1;
  CURSOR_DOMAIN = ujcamei.graph_anchor;
  PURGE_LEFT_CONTEXT = auto_from_Hx;
  PURGE_RIGHT_FUTURE = auto_from_Hf;
};

LATTICE_SPLIT {
  SPLIT_ID = train_core;
  ROLE = train;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 1600;
};

LATTICE_SPLIT {
  SPLIT_ID = acceptance_smoke;
  ROLE = train;
  ANCHOR_INDEX_BEGIN = 29;
  ANCHOR_INDEX_END = 31;
};

LATTICE_SPLIT {
  SPLIT_ID = validation_holdout;
  ROLE = validation;
  ANCHOR_INDEX_BEGIN = 1800;
  ANCHOR_INDEX_END = 2050;
  ALLOW_USES = evaluation_metric;
  PROTECT_FROM_USES = observed_input|target_supervision|selection_signal;
  PROTECT_REQUIRES_MUTATED_COMPONENT = true;
};

LATTICE_SPLIT {
  SPLIT_ID = test_holdout;
  ROLE = test;
  ANCHOR_INDEX_BEGIN = 2100;
  ANCHOR_INDEX_END = 2247;
  ALLOW_USES = evaluation_metric;
  PROTECT_FROM_USES = observed_input|target_supervision|selection_signal;
  PROTECT_REQUIRES_MUTATED_COMPONENT = true;
};
