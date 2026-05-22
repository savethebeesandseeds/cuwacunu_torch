/*
  kikijyeba.lattice.targets.dsl
  =============================

  Read-only target declarations for the current graph-first contract.

  A lattice target is not a wave and not a scheduler. It is a desired state over
  contract-scoped evidence: job manifests, job state, component reports,
  checkpoints, and later strict runtime `.lls` facts. The v0 evaluator only
  reads/evaluates/plans. It never executes.

  Runtime report mode:
    Lattice target v0 does not require waves to run with MODE=...|debug. It can
    evaluate normal job manifests, job.state, component reports, and checkpoint
    artifacts. Debug `.lls` facts are richer lineage evidence and should become
    first-class lattice evidence later, especially for cursor coverage and data
    exposure accounting.

  DEV_WARNING:
    The current protocol contract fingerprint still includes a small set of
    Jkimyei training selectors. Runtime model-state fields such as
    INPUT_REPRESENTATION_CHECKPOINT are not contract identity; they are proven
    through job reports, exposure facts, and checkpoint lineage.

  TARGET_KIND:
    representation_ready
      Checks the VICReg node representation target.

    node_mdn_ready
      Checks the node-centered ExpectedValue MDN target. V0 treats
      UPSTREAM_TARGET_ID as a prerequisite; if representation readiness is not
      satisfied, the node MDN target plans the upstream representation wave.

  PLAN_MODE:
    train | debug
      The suggested wave mode when the target is unsatisfied. The planner emits
      this as the next wave recommendation; Runtime Hero still performs all
      execution policy checks.

  SOURCE_RANGE:
    all
      Match full graph-anchor source-range evidence.

    anchor_index
      Match anchor-index ranged evidence. Requires ANCHOR_INDEX_BEGIN and
      ANCHOR_INDEX_END so the target is tied to the same graph-wide wave cursor
      range as the report/checkpoint evidence.

  SPLIT REFERENCES:
    OVER_SPLIT = train_core
      Resolve the target's SOURCE_RANGE to the named graph-anchor range from
      `kikijyeba.lattice.splits.dsl`. V1 requires SOURCE_RANGE=anchor_index and
      forbids also setting ANCHOR_INDEX_BEGIN/END directly.

    PROTECT_SPLIT = validation_holdout
      Apply the default holdout protection from `kikijyeba.lattice.splits.dsl`.
      The split declares which exposure uses it protects from, and the compiler
      lowers that into the existing forbidden-exposure proof. This is preferred
      over repeating FORBID_SPLIT + FORBID_EXPOSURE_USES in each target.

  PROFILES AND GUARDS:
    LATTICE_PROFILE names reusable readiness requirements, while LATTICE_GUARD
    remains available for low-level reusable forbidden exposure policy. Prefer
    split-level protection for validation/test holdouts. A target can use:

      USE_PROFILE = vicreg_training_readiness;
      PROTECT_SPLIT = validation_holdout;

    The parser lowers those forms back into the same v0 evaluator fields. This
    is the first step toward a clause-oriented proof language without changing
    the runtime meaning of existing targets.

  CLAUSE BLOCKS:
    LATTICE_DEPENDS, LATTICE_REQUIRES, LATTICE_FORBIDS, and LATTICE_PLAN attach
    proof clauses to a TARGET_ID. V0 still lowers these into the same evaluator
    fields. Use clauses for the parts of the target that read like proof:
    upstream dependency, metric/artifact/coverage requirements, forbidden
    exposure, and suggested wave policy.
    The compiler now keeps both views: a lowered v0 target for evaluation and a
    compiled proof object with clause provenance plus a target fingerprint. Use
    `hero.lattice.explain_target` to inspect that proof object without scanning
    runtime evidence.

  MODEL-STATE INPUTS:
    EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:<target_id>
      Requires a run/evaluation target to prove it loaded the exact output
      checkpoint selected by another satisfied target. This is intentionally
      runtime model-state evidence, not protocol contract identity.

    PLAN_INPUT_MDN_CHECKPOINT and PLAN_INPUT_REPRESENTATION_CHECKPOINT
      Add model-state inputs to the suggested wave. These are plan annotations
      only; Runtime Hero still performs execution policy checks and the runtime
      report must prove what was actually loaded.

  NAMING ALIASES:
    SUBJECT_COMPONENT is the preferred spelling for COMPONENT.
    OVER_SPLIT is the preferred spelling for TRAIN_SPLIT.
    WAVE_MODE is the preferred spelling for PLAN_MODE.
    PLAN_MAX_ATTEMPTS is the preferred spelling for MAX_WAVES.
    The old names still load for compatibility; mixing old and new names inside
    one block produces validation warnings unless the values conflict, in which
    case decoding fails.

  PLAN_MAX_ATTEMPTS:
    Counts active non-dry-run train attempts for this target/source range,
    contract, component assembly, graph order, and source cursor. Stale
    contract jobs, wrong graph-order jobs, dry-runs, and missing job.state files
    do not consume the active planning budget. When the count is exhausted, the
    evaluator can still report readiness from existing evidence, but it will
    stop recommending another wave.

  EXPOSURE REQUIREMENTS:
    These are optional v0 checks over the lattice exposure ledger. They require
    SOURCE_RANGE = anchor_index because v0 coverage is measured over explicit
    graph-anchor cursor intervals. The evaluator can auto-build the ledger from
    normal runtime job directories; explicit ledger injection is only needed for
    specialized tools or tests.

    MIN_OBSERVED_INPUT_COVERAGE = 0.95
      Require the checkpoint exposure closure to cover at least this fraction of
      the target anchor interval with observed_input evidence that mutated a
      component. Readiness coverage is target-component-local; an MDN target
      cannot satisfy its own observed_input coverage from an upstream VICReg
      checkpoint.

    MIN_TARGET_SUPERVISION_COVERAGE = 0.95
      Require the checkpoint exposure closure to cover at least this fraction of
      the target anchor interval with target_supervision evidence that mutated a
      component. This is usually for node-centered MDN training. Coverage is
      measured in graph-anchor coordinates, not shifted future source-row
      coordinates.

    MIN_EVALUATION_METRIC_COVERAGE = 0.95
      Require completed evaluation_metric evidence over the target anchor
      interval. This is for run/evaluation targets and does not require
      optimizer mutation. Use it for validation evaluation readiness, not for
      training readiness.

    CURSOR_EPOCHS = 0.95
      Gentler authoring alias inside LATTICE_REQUIRES exposure_coverage. It
      means the same v0 proof as MIN_COVERAGE: the selected exposure use covers
      this fraction of the named Ujcamei graph-anchor cursor split.

    LATTICE_WARN
      Non-blocking warning clauses over evidence summaries. Supported v0 warning
      kinds are exposure_load, effort_density, and anchor_domain_health.
      Warnings never change target status, plan readiness, suggested waves, or
      PLAN_MAX_ATTEMPTS accounting. They only report suspicious repeated
      exposure, high optimizer effort density, or source-domain health issues.

      Example:

        LATTICE_WARN {
          TARGET_ID = vicreg_train_core_ready;
          WARNING_ID = high_observed_input_load;
          KIND = exposure_load;
          USE = observed_input;
          SPLIT = train_core;
          SCOPE = target_component;
          EFFECT = mutated_component;
          CURSOR_EPOCHS_ABOVE = 3.0;
        };

    REQUIRE_TRAINED_NODE_HEAD_COUNT
      Training-readiness check for node-centered MDN waves. Use this for train
      targets. REQUIRE_EVALUATED_NODE_HEAD_COUNT is reserved for run/evaluation
      targets where heads are evaluated without optimizer mutation.

    FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN / END + FORBID_EXPOSURE_USES
      Reject readiness if the checkpoint exposure closure overlaps the forbidden
      source-row footprint interval for any listed use. Forbidden-overlap checks
      inspect the full checkpoint closure, including upstream components. Uses
      are separated by `|`:
      observed_input | target_supervision | evaluation_metric |
      selection_signal.
      When the range comes from PROTECT_SPLIT, the evaluator expands the split's
      protected source-row footprint using the split policy purge settings:
      auto_from_Hx expands left by observed context and auto_from_Hf expands
      right by future supervision context.

    FORBID_EXPOSURE_REQUIRES_MUTATED_COMPONENT
      Defaults to true. When true, forbidden-overlap checks only consider facts
      that mutated a component; set false later for stricter validation/test
      leakage policies.

    FOOTPRINT COORDINATE:
      New runtime manifests emit graph_anchor_row_index_v1 footprints. Observed
      input covers [anchor_begin - (Hx - 1), anchor_end), and future supervision
      covers [anchor_begin + 1, anchor_end + Hf). Older artifacts without these
      fields fall back to anchor_range_v0 and are weaker near split boundaries.
      Manifests may also emit graph_anchor_key_window_v1 source-key windows when
      the graph cursor has a regular reference key step. Those key windows are
      audit metadata for source-key/timestamp review; v0 target coverage and
      forbidden-overlap checks still use row-index intervals. Coverage uses the
      anchor coverage interval; forbidden-overlap checks use observed/target
      source-row footprints. Manifests also carry active source-file receipts
      for source audit; v0 target evaluation records them but does not interpret
      source-file paths.

    IDENTITY AND CLOSURE:
      Graph-anchor ranged targets and exposure checks require the active graph
      order fingerprint and source cursor token. Checkpoint closure resolution is
      fail-closed: unresolved input checkpoint lineage returns exposure_failed.
      Node MDN exposure readiness also verifies that the exact representation
      checkpoint loaded by the MDN report has an active compatible VICReg
      producer fact.

    CHECKPOINT_SOURCE:
      output_checkpoint
        Default. Evaluate the checkpoint written by the candidate job for this
        target.

      latest_satisfying:<target_id>
        Evaluate the checkpoint from another satisfied target. This is for
        read-only guard targets such as "the latest train-core MDN checkpoint
        has no validation leakage". It does not execute or schedule anything;
        if the source target is unsatisfied, this target blocks and forwards the
        source target's suggested wave.
*/
LATTICE_PROFILE {
  PROFILE_ID = vicreg_training_readiness;
  TARGET_KIND = representation_ready;
  SUBJECT_COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  PROTECT_SPLIT = validation_holdout;
  WAVE_MODE = train|debug;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_PROFILE {
  PROFILE_ID = node_mdn_training_readiness;
  TARGET_KIND = node_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  REQUIRE_ACTIVE_NODE_HEAD_COUNT = 1;
  REQUIRE_TRAINED_NODE_HEAD_COUNT = 1;
  PROTECT_SPLIT = validation_holdout;
  WAVE_MODE = train|debug;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_TARGET {
  TARGET_ID = vicreg_representation_ready;
  TARGET_KIND = representation_ready;
  SUBJECT_COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  WAVE_MODE = train|debug;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_TARGET {
  TARGET_ID = node_mdn_ready;
  TARGET_KIND = node_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = all;
  UPSTREAM_TARGET_ID = vicreg_representation_ready;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  REQUIRE_ACTIVE_NODE_HEAD_COUNT = 1;
  REQUIRE_EVALUATED_NODE_HEAD_COUNT = 1;
  WAVE_MODE = train|debug;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_TARGET {
  TARGET_ID = vicreg_train_core_ready;
  USE_PROFILE = vicreg_training_readiness;
  OVER_SPLIT = train_core;
};

LATTICE_TARGET {
  TARGET_ID = vicreg_acceptance_smoke_ready;
  USE_PROFILE = vicreg_training_readiness;
  OVER_SPLIT = acceptance_smoke;
};

LATTICE_TARGET {
  TARGET_ID = node_mdn_train_core_ready;
  USE_PROFILE = node_mdn_training_readiness;
  OVER_SPLIT = train_core;
};

LATTICE_TARGET {
  TARGET_ID = node_mdn_train_core_no_validation_leakage;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = node_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  CHECKPOINT_SOURCE = latest_satisfying:node_mdn_train_core_ready;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  PROTECT_SPLIT = validation_holdout;
  WAVE_MODE = run|debug;
  PLAN_MAX_ATTEMPTS = 0;
};

LATTICE_TARGET {
  TARGET_ID = node_mdn_train_core_no_test_leakage;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = node_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  CHECKPOINT_SOURCE = latest_satisfying:node_mdn_train_core_no_validation_leakage;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  PROTECT_SPLIT = test_holdout;
  WAVE_MODE = run|debug;
  PLAN_MAX_ATTEMPTS = 0;
};

LATTICE_TARGET {
  TARGET_ID = node_mdn_validation_eval_ready;
  TARGET_CLASS = evaluation_readiness;
  TARGET_KIND = node_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = validation_holdout;
  UPSTREAM_TARGET_ID = node_mdn_train_core_no_test_leakage;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = false;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 0;
  MIN_VALID_TARGET_FRACTION = 0.05;
  REQUIRE_EVALUATED_NODE_HEAD_COUNT = 1;
  EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:node_mdn_train_core_no_test_leakage;
  PROTECT_SPLIT = validation_holdout;
  WAVE_MODE = run|debug;
  PLAN_MAX_ATTEMPTS = 1;
};

LATTICE_TARGET {
  TARGET_ID = node_mdn_acceptance_smoke_ready;
  USE_PROFILE = node_mdn_training_readiness;
  OVER_SPLIT = acceptance_smoke;
};

LATTICE_TARGET {
  TARGET_ID = node_mdn_acceptance_no_validation_leakage;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = node_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  CHECKPOINT_SOURCE = latest_satisfying:node_mdn_acceptance_smoke_ready;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  PROTECT_SPLIT = validation_holdout;
  WAVE_MODE = run|debug;
  PLAN_MAX_ATTEMPTS = 0;
};

LATTICE_REQUIRES {
  TARGET_ID = vicreg_train_core_ready;
  REQUIREMENT_ID = observed_input_train_core_coverage;
  KIND = exposure_coverage;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 0.95;
};

LATTICE_WARN {
  TARGET_ID = vicreg_train_core_ready;
  WARNING_ID = high_observed_input_train_core_load;
  KIND = exposure_load;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  CURSOR_EPOCHS_ABOVE = 3.0;
};

LATTICE_WARN {
  TARGET_ID = vicreg_train_core_ready;
  WARNING_ID = low_vicreg_anchor_acceptance_fraction;
  KIND = anchor_domain_health;
  SCOPE = target_component;
  EFFECT = any;
  ACCEPTED_FRACTION_BELOW = 0.80;
};

LATTICE_WARN {
  TARGET_ID = vicreg_train_core_ready;
  WARNING_ID = vicreg_fetch_probe_skips_present;
  KIND = anchor_domain_health;
  SCOPE = target_component;
  EFFECT = any;
  SKIPPED_FAILED_FETCH_PROBE_ABOVE = 0;
};

LATTICE_REQUIRES {
  TARGET_ID = vicreg_acceptance_smoke_ready;
  REQUIREMENT_ID = observed_input_acceptance_coverage;
  KIND = exposure_coverage;
  USE = observed_input;
  SPLIT = acceptance_smoke;
  SCOPE = target_component;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 1.0;
};

LATTICE_DEPENDS {
  TARGET_ID = node_mdn_train_core_ready;
  UPSTREAM_TARGET_ID = vicreg_train_core_ready;
  BINDING = loaded_representation_checkpoint;
  REQUIRE_EXACT_LOADED_CHECKPOINT = true;
};

LATTICE_REQUIRES {
  TARGET_ID = node_mdn_train_core_ready;
  REQUIREMENT_ID = target_supervision_train_core_coverage;
  KIND = exposure_coverage;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 0.95;
};

LATTICE_WARN {
  TARGET_ID = node_mdn_train_core_ready;
  WARNING_ID = high_target_supervision_train_core_load;
  KIND = exposure_load;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  CURSOR_EPOCHS_ABOVE = 3.0;
};

LATTICE_WARN {
  TARGET_ID = node_mdn_train_core_ready;
  WARNING_ID = high_mdn_optimizer_effort_density;
  KIND = effort_density;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  METRIC = optimizer_steps_per_cursor_epoch;
  ABOVE = 10.0;
};

LATTICE_WARN {
  TARGET_ID = node_mdn_train_core_ready;
  WARNING_ID = low_mdn_anchor_acceptance_fraction;
  KIND = anchor_domain_health;
  SCOPE = target_component;
  EFFECT = any;
  ACCEPTED_FRACTION_BELOW = 0.80;
};

LATTICE_WARN {
  TARGET_ID = node_mdn_train_core_ready;
  WARNING_ID = mdn_fetch_probe_skips_present;
  KIND = anchor_domain_health;
  SCOPE = target_component;
  EFFECT = any;
  SKIPPED_FAILED_FETCH_PROBE_ABOVE = 0;
};

LATTICE_DEPENDS {
  TARGET_ID = node_mdn_acceptance_smoke_ready;
  UPSTREAM_TARGET_ID = vicreg_acceptance_smoke_ready;
  BINDING = loaded_representation_checkpoint;
  REQUIRE_EXACT_LOADED_CHECKPOINT = true;
};

LATTICE_REQUIRES {
  TARGET_ID = node_mdn_acceptance_smoke_ready;
  REQUIREMENT_ID = target_supervision_acceptance_coverage;
  KIND = exposure_coverage;
  USE = target_supervision;
  SPLIT = acceptance_smoke;
  SCOPE = target_component;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 1.0;
};

LATTICE_REQUIRES {
  TARGET_ID = node_mdn_validation_eval_ready;
  REQUIREMENT_ID = validation_evaluation_metric_coverage;
  KIND = exposure_coverage;
  USE = evaluation_metric;
  SPLIT = validation_holdout;
  SCOPE = target_component;
  EFFECT = none;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 0.95;
};

LATTICE_PLAN {
  TARGET_ID = node_mdn_validation_eval_ready;
  PLAN_ID = run_mdn_validation_eval;
  WAVE_TARGET = wikimyei.inference.expected_value.mdn;
  WAVE_MODE = run|debug;
  WAVE_RANGE = split:validation_holdout;
  PLAN_INPUT_MDN_CHECKPOINT = latest_satisfying:node_mdn_train_core_no_test_leakage;
  PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:vicreg_train_core_ready;
  PLAN_MAX_ATTEMPTS = 1;
};
