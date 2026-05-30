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
    vicreg_ready
      Checks the channel-preserving VICReg representation target.

    mtf_representation_ready
      Checks the experimental MTF-JEPA-MAE-VICReg representation target as a
      separate representation producer. It does not satisfy VICReg identity.

    channel_mdn_ready
      Checks the strict channel-context ExpectedValue MDN target. V0 treats
      UPSTREAM_TARGET_ID as a prerequisite; if VICReg readiness is not
      satisfied, the MDN target plans the upstream channel
      representation wave.

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
    clauses to a TARGET_ID. V0 still lowers these into the same evaluator
    fields. Use dependency/requirement/forbid clauses for the parts of the
    target that read like proof: upstream dependency, metric/artifact/coverage
    requirements, and forbidden exposure. Use LATTICE_PLAN for suggested wave
    policy only.
    The compiler now keeps both views: a lowered v0 target for evaluation and a
    compiled proof object with clause provenance plus a target fingerprint over
    the proof obligation. Advisory plan values remain visible, but are excluded
    from that fingerprint. Non-blocking warning clauses remain visible too, but
    they are also excluded from proof-obligation identity. Use
    `hero.lattice.explain_target` to inspect that proof object without scanning
    runtime evidence.

  MODEL-STATE INPUTS:
    EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:<target_id>
      Requires a run/evaluation target to prove it loaded the exact output
      checkpoint selected by another satisfied target. This is intentionally
      runtime model-state evidence, not protocol contract identity.
      latest_satisfying is a deterministic readiness selector over the
      referenced satisfied target's newest checkpoint candidate. It is not a
      best-model, Pareto, performance, or deployment selector.

    PLAN_INPUT_MDN_CHECKPOINT and PLAN_INPUT_REPRESENTATION_CHECKPOINT
      Add model-state inputs to the suggested wave. These are plan annotations
      only; Runtime Hero still performs execution policy checks and the runtime
      report must prove what was actually loaded. They carry symbolic
      latest_satisfying source-target references, not proof evidence. Their
      values do not alter the target_spec_fingerprint.

    WAVE_MODE, PLAN_MODE, PLAN_MAX_ATTEMPTS, and MAX_WAVES
      Shape suggested waves and planning budgets only. Their values do not alter
      the target_spec_fingerprint.

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
      interval. This is for run/evaluation targets and requires non-mutating
      evaluation evidence with zero optimizer steps, exact evaluated
      checkpoint input edges, and no output checkpoint.
      Use it for validation evaluation readiness, not for training readiness or
      model-performance acceptance. Hero JSON includes
      validation_performance_scope_policy_vocabulary to record that evaluation
      coverage proves clean evaluation happened, not that NLL, calibration, or
      utility is acceptable.

    CURSOR_EPOCHS = 0.95
      Gentler authoring alias inside LATTICE_REQUIRES exposure_coverage. It
      means the same v0 proof as MIN_COVERAGE: the selected exposure use covers
      this fraction of the named Ujcamei graph-anchor cursor split.

    Unit checks
      The compiler treats coverage values and fractions as [0,1], repeated
      exposure load as non-negative cursor epochs, optimizer effort density as a
      non-negative density, node/count thresholds as non-negative counts, and
      representation condition-number thresholds as finite values >= 1.
      Representation-health warning metrics also enforce their one-sided bad
      direction: high-bad metrics use ABOVE and low-bad metrics use BELOW.
      Opt-in representation-geometry gates use VALUE with OP=ge for low-bad
      metrics and OP=le for high-bad metrics.
      Anchor-domain warning clauses carry exactly one metric threshold.
      This is intentionally small dimensional analysis: it prevents common
      authoring mistakes without changing runtime evidence or target semantics.
      Exposure summaries may also report Wilson 95% intervals for valid-target
      support when runtime reports include both valid_target_count and
      valid_target_fraction; those intervals are visibility, not gates.
      Hero JSON includes valid_target_uncertainty_vocabulary for the Wilson
      method, field bindings, and no-trials policy.
      Hero JSON also includes valid_target_uncertainty_summary: the self-check
      for the three support scopes, Wilson-95/binomial method,
      success/opportunity and interval bindings, no-trials non-claiming policy,
      and support-visibility-only boundary.
      Hero JSON also includes target_numeric_dimension_summary: the compact
      read-side check that the numeric table has declared units/kinds,
      well-formed bounds, known threshold directions, key V1 rows, and separate
      coverage/load units.

    LATTICE_WARN
      Non-blocking warning clauses over evidence summaries. Supported v0 warning
      kinds are exposure_load, effort_density, anchor_domain_health,
      node_support_balance, node_support_floor, representation_health, and
      runtime_health.
      Warnings never change target status, plan readiness, suggested waves, or
      PLAN_MAX_ATTEMPTS accounting. They only report suspicious repeated
      exposure, high optimizer effort density, source-domain health issues,
      weak/imbalanced MDN node support, low normalized support entropy, low
      aggregate Wilson lower-bound support, or VICReg representation-health
      metrics including optional geometry summaries such as effective-rank
      fraction and condition number. KIND=mdn_distribution_calibration can
      warn on aggregate, channel, target-feature, channel/target-feature, and
      per-node MDN NLL diagnostics;
      PIT KS statistic, predictive interval coverage error, tail coverage
      error, and calibration slope error are future non-gating calibration
      metrics until runtime emits samples and uncertainty.
      KIND=runtime_health reads Runtime terminal facts when present and falls
      back to report-derived exposure fields for older jobs. It covers
      correctness-adjacent checks such as finite_parameter_check,
      nonfinite_output_count, checkpoint_written, model_state_mutated,
      representation_checkpoint_loaded, mdn_checkpoint_loaded, and health
      observations such as grad_norm_max_pre_clip, sigma_min/sigma_max,
      mixture_entropy, and valid_target_fraction.
      Any warning kind may declare SPLIT or explicit ANCHOR_INDEX_BEGIN/END.
      Measurements are scoped to that warning interval, defaulting to the
      target range. If trusted completed coverage is unavailable, warning
      overlap can use anchor-range visibility without making that evidence
      satisfy readiness coverage. Hero JSON exposes the resolved interval as
      warning_results[].warning_anchor_range; hero.lattice.explain_target
      exposes the same pre-evaluation scope as
      warning_scope_previews[].resolved_warning_anchor_range; and
      warning_anchor_scope_policy_vocabulary documents the visibility-only
      fallback for both surfaces. Node-support warnings derive a warning-range
      support matrix before applying USE/EFFECT filters, so narrow warning
      windows do not reuse target-wide node-support counts.
      Hero JSON includes node_support_scope_policy_vocabulary: V1 node-support
      rows are MDN-head support visibility only, future shared-representation
      node support must come from NodeLift or representation facts, and
      synthetic backfill from MDN rows is not allowed.
      Hero JSON includes node_support_scope_policy_summary as the compact
      self-check for that no-VICReg-readiness/no-backfill boundary.
      Node-support warnings filter support rows by USE and EFFECT before
      measuring support-balance or floor metrics; the default is
      USE=target_supervision and EFFECT=mutated_component.
      Warning results expose measurement_available so unavailable warning
      measurements can be counted without parsing message text. They also
      expose threshold_direction, threshold_triggered, and threshold_relation so
      the threshold comparison and trigger state are structured. Hero JSON
      includes warning_threshold_relation_vocabulary for the allowed relation
      values and warning_summary for aggregate total, triggered, unavailable,
      clear measured, and per-relation counts.
      Evidence-order JSON includes dimension_vocabulary from the same C++
      vocabulary used by the Pareto comparator. warning_count is a compatibility
      alias; comparison uses triggered_warning_count.
      Hero JSON also includes exposure_measure_algebra_vocabulary: unique
      coverage is idempotent interval union, repeated exposure load is additive
      interval measure.
      Hero JSON also includes exposure_measure_algebra_summary: exactly two
      measures, one idempotent coverage measure, one additive load measure, and
      distinct units.
      Hero JSON also includes source_key_coordinate_policy_vocabulary:
      row-index intervals are authoritative for coverage/leakage; source-key
      windows are audit-only order-preserving and affine map checks.
      Hero JSON also includes source_key_coordinate_policy_summary: exactly one
      row-index coverage/leakage authority row, three audit-only source-key rows,
      declared order-preserving/affine fields, and no source-key audit row with
      coverage or leakage authority.
      Hero JSON also includes leakage_rule_vocabulary: leakage is protected
      split dilation plus forbidden-use intersection over complete checkpoint
      closure.
      Hero JSON also includes leakage_rule_summary: the compact self-check for
      the single V1 dilation rule, closure completeness, and causal witnesses.
      Hero JSON also includes causal_edge_vocabulary so closure can be read as
      a labeled causal graph over source-row uses, checkpoint load/create edges,
      and component mutation.
      Hero JSON also includes causal_edge_summary: 7 V1 edge labels, 4
      source-row use edges, 2 checkpoint reachability edges, 1 mutation edge,
      and zero checkpoint/leakage edge overlap.
      Hero JSON also includes selection_signal_policy_vocabulary: V1 treats
      selection_signal as a forbidden causal use over the causal exposure anchor
      footprint, while first-class selector event streams remain future
      provenance work.
      Hero JSON also includes selection_signal_policy_summary: the compact
      self-check for the four-row boundary, closure-complete policy rows,
      deferred selector-event/path proofs, and no Lattice executor authority.
      Hero JSON also includes derived_query_rule_vocabulary: read-only
      Datalog-style derived relations for identity, dependency, coverage,
      closure, leakage, warning, planning, and satisfaction proofs. These are
      query/cache metadata only; Runtime Hero remains the only executor.
      Hero JSON also includes db_cache_policy_vocabulary: runtime files are the
      evidence source of truth, cache rows are rebuildable read models, cached
      plans are not evidence, checkpoint ids/digests are preview cache keys
      until v2, and Runtime Hero remains the only executor.
      Hero JSON also includes evidence_abstraction_vocabulary: concrete
      runtime/fact inputs mapped into abstract proof outputs with soundness,
      conservative, and join semantics that future indexes must preserve,
      including source-receipt audit as a non-claiming audit abstraction.
      Hero JSON also includes product_evidence_state_vocabulary: the product
      proof-space factors, partial orders, identity-scoped join semantics, proof
      fields, clean-growth rules, and target-status effects, including a
      source-receipt audit set that is not a readiness-strength dimension.
      Hero JSON also includes join_law_vocabulary: the algebraic laws for
      identity-scoped joins, including idempotent coverage/closure/leakage
      witness joins, additive exposure-load and node-support joins, duplicate
      handling, cache requirements, and fail-closed unsafe joins.
      Hero JSON also includes node_support_scope_policy_vocabulary: the MDN-only
      node-support scope and future shared-representation support boundary.
      Hero JSON also includes node_support_scope_policy_summary: four policy
      rows, no VICReg per-node readiness authority, no synthetic backfill, and
      no runtime executor authority.
      Hero JSON also includes representation_geometry_vocabulary: VICReg
      representation-health and embedding-geometry warning metrics, units,
      threshold directions, and V1 non-blocking scope.
      Hero JSON also includes representation_geometry_summary: 18 V1-visible
      representation_health warning metrics, 7 embedding-geometry metrics, 11
      future hard-gate candidates, and 0 active performance gates.
      Hero JSON also includes representation_geometry_gate_review_summary:
      observed VICReg geometry distributions, 0 promoted default thresholds,
      opt-in LATTICE_REQUIRES KIND=representation_geometry syntax, and
      fail-closed missing geometry facts for that explicit gate.
      Hero JSON also includes evidence_retention_policy_vocabulary,
      evidence_retention_audit_scenario_vocabulary, and
      evidence_retention_policy_summary: reports, sidecars, checkpoints, source
      receipts, selection signals, proof certificates, cache rows, human
      receipts, and archive manifests classified for replay-safe compaction.
      Hero JSON also includes benchmark_regression_budget_vocabulary and
      benchmark_regression_budget_summary: finite performance rows split
      library-function, long-lived MCP, and direct CLI timing layers while
      naming proof modes header_only, watched_file_manifest,
      full_runtime_metadata_digest, live_scan, and live_parity. Header-only
      fast audit rows forbid live scans and metadata digests; cache rows remain
      non-authoritative for target satisfaction.
      Hero JSON also includes performance_uncertainty_policy_vocabulary:
      future performance gates must use declared uncertainty methods and
      conservative confidence bounds, not raw point estimates; selection-leakage
      policy is required before performance acceptance.
      Hero JSON also includes performance_uncertainty_policy_summary: one V1
      support-visibility row, five deferred future performance-gate rows, no V1
      performance-gate authority, no point-estimate gates, confidence bounds
      required, and selection-leakage policy required.
      Hero JSON also includes validation_performance_evidence_policy_vocabulary:
      V3-C validation performance evidence rows name metric family, split,
      checkpoint identity, evaluated checkpoint binding, sample count, point
      estimate, uncertainty method, confidence bounds, target syntax,
      selection-leakage policy, and fail-closed cases.
      Hero JSON also includes validation_performance_evidence_policy_summary:
      V3-C has no active performance gate, available point estimates are bounded
      or warning-only, mean_nll stays warning-only until uncertainty is emitted,
      and missing uncertainty, wrong checkpoint binding, stale identity, and
      selector leakage fail closed.
      Hero JSON also includes mathematical_readiness_v1_vocabulary: a crosswalk
      from the mathematical strengthening points to their concrete Hero
      surfaces, read-only V1 status, scope boundaries, and future work.
      mathematical_readiness_v1_summary self-checks that the crosswalk has 16
      included read-only items, no runtime-executor/performance/DB authority, and
      no empty Hero-surface rows.
      Hero JSON also includes mdn_distribution_calibration_vocabulary: the V1
      mean_nll warning metric and future non-gating distribution calibration
      metrics.
      Hero JSON also includes mdn_distribution_calibration_summary: the
      warning-only metric policy keeps mean_nll V1-visible, future calibration
      metrics deferred, all rows non-blocking, and no performance gates.
      Hero JSON also includes
      mdn_distribution_calibration_diagnostic_vocabulary and
      mdn_distribution_calibration_diagnostic_summary: V3-D diagnostics bind
      aggregate/channel/target-feature/channel-target-feature/per-node NLL
      warnings and future calibration rows to exact evaluated MDN checkpoint,
      representation checkpoint, split policy, active identity, validation
      split, sample count, uncertainty method, and warning-only/non-performance
      effect.
      Hero JSON also includes validation_performance_scope_policy_vocabulary:
      validation eval readiness is clean evaluation evidence, not a performance
      gate; future performance targets require explicit metrics, uncertainty,
      and selection-leakage policy.
      Hero JSON also includes validation_performance_scope_policy_summary:
      three V1 readiness/visibility rows, one deferred future performance row,
      no V1 performance-gate authority, uncertainty required for future gates,
      and no runtime executor authority.
      Hero JSON also includes checkpoint_selection_policy_vocabulary:
      latest_satisfying resolves readiness-selected checkpoint artifacts and
      carries symbolic plan-input hints without becoming a Pareto optimizer,
      performance ranker, contract identity input, or executor.
      Hero JSON also includes checkpoint_selection_policy_summary: selector
      policy counts and zero performance, Pareto, identity, or executor authority.
	      Hero JSON also includes plan_advice_scope_policy_vocabulary: plan_basis,
	      suggested_wave, and PLAN_INPUT_* are advisory deficit projections only;
	      PLAN_MAX_ATTEMPTS/MAX_WAVES only guard recommendation availability;
	      Hero JSON also includes deficit_vector_planning_summary: 12 ordered
	      deficit priority classes, 5 plan-advice policy rows, zero evidence or
	      contract-identity authority, and Runtime Hero as the only executor;
	      missing active contract/component/graph/source identity inputs
	      and stale runtime evidence with mismatched identity surface as
	      identity:* deficits;
	      scalar readiness threshold failures surface as metric:* deficits;
	      missing manifests, reports, job state files, checkpoint files,
	      exposure ledgers, closure facts, active exposure facts,
	      target-component-local exposure facts, producer exposure facts, split
	      policies, and non-completed job status surface as artifact:* deficits;
	      cyclic target dependency graphs surface as
	      dependency:target_cycle deficits;
	      malformed proof envelopes surface as
	      certificate:proof_certificate_check deficits;
	      read-only validation input, mutation, or checkpoint-write violations
	      surface as model_state:input_checkpoint,
	      model_state:mutated_component, or model_state:output_checkpoint
	      deficits;
	      Runtime Hero remains the executor.
      Hero JSON also includes operational_readiness_v1_scope_vocabulary: the
      included V1 read-only evidence-authority claims and the intentionally
      deferred DB/index, performance, checkpoint identity, structured source
      receipt, selection-event, and VICReg node-support work.
      operational_readiness_v1_scope_summary self-checks that those 12 rows
      partition into 6 included read-only claims and 6 deferred items, with zero
      runtime-executor/performance/DB authority and no empty claim-boundary
      fields.
      Hero JSON also includes operational_readiness_v1_gate_vocabulary: the
      concrete V1 acceptance gates for identity, train-core readiness, leakage
      guards, validation checkpoint binding, warnings/node-support visibility,
      and Hero read/plan/closure inspection.
      operational_readiness_v1_gate_summary self-checks that the 10 gates are
      required, read-only, non-executing, non-performance, non-DB-source-of-truth,
      have pass/fail conditions and Hero surfaces, and reference all five
      required V1 targets.
      Hero JSON also includes contract_identity_boundary_vocabulary:
      protocol/graph/component active identity and target proof identity are
      separate from runtime model-state checkpoint paths, advisory plan inputs,
      warning visibility, and audit-only source receipt/key-window metadata.
      contract_identity_boundary_summary self-checks that model-state, planning
      advice, warning visibility, and audit metadata have zero overlap with
      protocol contract or target proof identity while active identity surfaces
      are present.
      Hero JSON also includes target_numeric_dimension_vocabulary: the
      unit/bounds/integrality/threshold-direction table used by the target
      compiler for numeric target and warning fields.
      Hero JSON also includes target_numeric_dimension_summary: the self-check
      for table completeness, bounds, directions, duplicate context/field
      pairs, and coverage/load separation.
      Hero JSON also includes monotonicity_invariant_vocabulary: clean evidence
      growth is monotone for target satisfaction, while forbidden overlaps,
      deficits, warnings, and Pareto tradeoffs are explicit weakening or
      incomparable dimensions.
      Hero JSON also includes proof_obligation_vocabulary: certificate fields
      mapped to derived relations, required scopes, status effects, deficit
      kinds, non-claiming semantics, digest participation, and planner
      relevance.
      Hero JSON also includes proof_certificate_digest_policy_vocabulary: the
      certificate digest binds canonical proof content, while warnings,
      deficits, plans, evidence-order projections, policy vocabularies, and the
      digest field itself remain outside the digest.
      Hero JSON also includes proof_certificate_digest_policy_summary: the
      compact self-check for digest-participation counts and advisory/report
      exclusion.
      Hero JSON also includes checkpoint_identity_policy_vocabulary: V1 closure
      authority remains path/exposure-fact based; checkpoint ids/file digests
      are audit previews; exact loaded-checkpoint bindings are runtime-state
      proof fields; non-mutating evaluation_metric coverage witnesses bind their
      input checkpoint edges exactly to a satisfied EVALUATED_CHECKPOINT_SOURCE
      dependency, reject extra checkpoint input edges, and must not carry output
      checkpoint edges; id/digest promotion is future DB/cache work.
      Warning clauses do not alter the target_spec_fingerprint.

      Example:

        LATTICE_WARN {
          TARGET_ID = vicreg_train_core_ready;
          WARNING_ID = high_observed_input_load;
          KIND = exposure_load;
          USE = observed_input;
          SPLIT = train_core;
          SCOPE = target_component_family_id;
          EFFECT = mutated_component;
          CURSOR_EPOCHS_ABOVE = 3.0;
        };

    FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN / END + FORBID_EXPOSURE_USES
      Reject readiness if the checkpoint exposure closure overlaps the forbidden
      source-row footprint interval for any listed use. Forbidden-overlap checks
      inspect the full checkpoint closure, including upstream components. Uses
      are separated by `|`:
      observed_input | target_supervision | evaluation_metric |
      selection_signal.
      In V1, selection_signal is a forbidden causal use label, not a
      first-class selector event stream.
      selection_signal_policy_summary is the Hero-side self-check for that
      boundary and for the deferred selector-event/path proofs.
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
      source-row footprints. Lattice scans warn when numeric source-key
      endpoints fail to preserve row-endpoint ordering or when the inferred
      regular key step does not match the observed/target key windows. Manifests
      also carry active source-file receipts for source audit; v0 target
      evaluation records them but does not interpret source-file paths.
      source_receipt_policy_vocabulary exposes that compact receipts are audit
      metadata only: row-index footprints remain coverage/leakage authority,
      receipt strings do not alter contract identity, and structured source
      receipt facts remain outside V1.
      source_receipt_policy_summary self-checks that boundary with one
      row-index coverage/leakage authority row, three audit-only receipt rows,
      no contract identity authority, no structured receipt facts in V1, and
      declared compact/future structured receipt fields.

    IDENTITY AND CLOSURE:
      Graph-anchor ranged targets and exposure checks require the active graph
      order fingerprint and source cursor token. Checkpoint closure resolution is
      fail-closed: unresolved input checkpoint lineage returns exposure_failed.
      MDN exposure readiness also verifies that the exact representation
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
        it is not a best-model or performance-ranking selector.
        if the source target is unsatisfied, this target blocks and forwards the
        source target's suggested wave.

      Active definitions below are channel-context only. Historical node-MDN
      artifacts belong to frozen receipts or migration notes, not to this active
      target DSL.
*/
LATTICE_PROFILE {
  PROFILE_ID = vicreg_training_readiness;
  TARGET_KIND = vicreg_ready;
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
  PROFILE_ID = mtf_representation_training_readiness;
  TARGET_KIND = mtf_representation_ready;
  SUBJECT_COMPONENT = wikimyei.representation.encoding.mtf_jepa_mae_vicreg;
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
  PROFILE_ID = channel_mdn_training_readiness;
  TARGET_KIND = channel_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  PROTECT_SPLIT = validation_holdout;
  WAVE_MODE = train|debug;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_TARGET {
  TARGET_ID = vicreg_representation_ready;
  TARGET_KIND = vicreg_ready;
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
  TARGET_ID = channel_mdn_ready;
  TARGET_KIND = channel_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = all;
  UPSTREAM_TARGET_ID = vicreg_representation_ready;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  WAVE_MODE = train|debug;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_TARGET {
  TARGET_ID = vicreg_train_core_ready;
  USE_PROFILE = vicreg_training_readiness;
  OVER_SPLIT = train_core;
};

LATTICE_TARGET {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  USE_PROFILE = mtf_representation_training_readiness;
  OVER_SPLIT = train_core;
};

LATTICE_TARGET {
  TARGET_ID = vicreg_acceptance_smoke_ready;
  USE_PROFILE = vicreg_training_readiness;
  OVER_SPLIT = acceptance_smoke;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_train_core_ready;
  USE_PROFILE = channel_mdn_training_readiness;
  OVER_SPLIT = train_core;
};

/*
  Protocol-scoped readiness aliases.

  These are the explicit protocol variant targets. The older unscoped target
  IDs above remain for compatibility while current training continues on
  cwu_01v.
*/
LATTICE_TARGET {
  TARGET_ID = cwu_01v_representation_train_core_ready;
  USE_PROFILE = vicreg_training_readiness;
  PROTOCOL_ID = cwu_01v;
  OVER_SPLIT = train_core;
};

LATTICE_TARGET {
  TARGET_ID = cwu_02v_representation_train_core_ready;
  USE_PROFILE = mtf_representation_training_readiness;
  PROTOCOL_ID = cwu_02v;
  OVER_SPLIT = train_core;
};

LATTICE_TARGET {
  TARGET_ID = cwu_01v_mdn_train_core_ready;
  USE_PROFILE = channel_mdn_training_readiness;
  PROTOCOL_ID = cwu_01v;
  OVER_SPLIT = train_core;
};

LATTICE_TARGET {
  TARGET_ID = cwu_02v_mdn_train_core_ready;
  USE_PROFILE = channel_mdn_training_readiness;
  PROTOCOL_ID = cwu_02v;
  OVER_SPLIT = train_core;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_train_core_no_validation_leakage;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = channel_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  CHECKPOINT_SOURCE = latest_satisfying:channel_mdn_train_core_ready;
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
  TARGET_ID = channel_mdn_train_core_no_test_leakage;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = channel_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  CHECKPOINT_SOURCE = latest_satisfying:channel_mdn_train_core_no_validation_leakage;
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
  TARGET_ID = channel_mdn_validation_eval_ready;
  TARGET_CLASS = evaluation_readiness;
  TARGET_KIND = channel_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = validation_holdout;
  UPSTREAM_TARGET_ID = channel_mdn_train_core_no_test_leakage;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = false;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 0;
  MIN_VALID_TARGET_FRACTION = 0.05;
  EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:channel_mdn_train_core_no_test_leakage;
  PROTECT_SPLIT = validation_holdout;
  WAVE_MODE = run|debug;
  PLAN_MAX_ATTEMPTS = 1;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_acceptance_smoke_ready;
  USE_PROFILE = channel_mdn_training_readiness;
  OVER_SPLIT = acceptance_smoke;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_acceptance_no_validation_leakage;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = channel_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  CHECKPOINT_SOURCE = latest_satisfying:channel_mdn_acceptance_smoke_ready;
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
  REQUIREMENT_ID = channel_observed_input_train_core_coverage;
  KIND = exposure_coverage;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 0.95;
};

LATTICE_REQUIRES {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  REQUIREMENT_ID = mtf_observed_input_train_core_coverage;
  KIND = exposure_coverage;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 0.95;
};

LATTICE_REQUIRES {
  TARGET_ID = cwu_01v_representation_train_core_ready;
  REQUIREMENT_ID = cwu_01v_observed_input_train_core_coverage;
  KIND = exposure_coverage;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 0.95;
};

LATTICE_REQUIRES {
  TARGET_ID = cwu_02v_representation_train_core_ready;
  REQUIREMENT_ID = cwu_02v_observed_input_train_core_coverage;
  KIND = exposure_coverage;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 0.95;
};

LATTICE_PLAN {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  PLAN_ID = train_mtf_jepa_mae_vicreg_train_core;
  WAVE_TARGET = wikimyei.representation.encoding.mtf_jepa_mae_vicreg;
  WAVE_MODE = train|debug;
  WAVE_RANGE = split:train_core;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_WARN {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  WARNING_ID = mtf_representation_effective_rank_fraction_low;
  KIND = representation_health;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = representation_effective_rank_fraction;
  BELOW = 0.25;
};

LATTICE_WARN {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  WARNING_ID = mtf_representation_condition_number_high;
  KIND = representation_health;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = representation_condition_number;
  ABOVE = 1000.0;
};

LATTICE_WARN {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  WARNING_ID = mtf_context_starvation_seen;
  KIND = representation_health;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = context_starved_sample_count;
  ABOVE = 0.0;
};

LATTICE_WARN {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  WARNING_ID = mtf_targets_reduced_for_context_seen;
  KIND = representation_health;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = reduced_targets_for_context_count;
  ABOVE = 0.0;
};

LATTICE_WARN {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  WARNING_ID = mtf_tf_pair_support_missing;
  KIND = representation_health;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = tf_pair_valid_count;
  BELOW = 1.0;
};

LATTICE_WARN {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  WARNING_ID = mtf_vicreg_global_rows_missing;
  KIND = representation_health;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = vicreg_global_valid_rows;
  BELOW = 1.0;
};

LATTICE_WARN {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  WARNING_ID = mtf_vicreg_channel_rows_missing;
  KIND = representation_health;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = vicreg_channel_valid_rows;
  BELOW = 1.0;
};

LATTICE_REQUIRES {
  TARGET_ID = vicreg_acceptance_smoke_ready;
  REQUIREMENT_ID = channel_observed_input_acceptance_coverage;
  KIND = exposure_coverage;
  USE = observed_input;
  SPLIT = acceptance_smoke;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 1.0;
};

LATTICE_DEPENDS {
  TARGET_ID = channel_mdn_train_core_ready;
  UPSTREAM_TARGET_ID = vicreg_train_core_ready;
  BINDING = loaded_representation_checkpoint;
  REQUIRE_EXACT_LOADED_CHECKPOINT = true;
};

LATTICE_REQUIRES {
  TARGET_ID = channel_mdn_train_core_ready;
  REQUIREMENT_ID = channel_target_supervision_train_core_coverage;
  KIND = exposure_coverage;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 0.95;
};

LATTICE_REQUIRES {
  TARGET_ID = cwu_01v_mdn_train_core_ready;
  REQUIREMENT_ID = cwu_01v_channel_target_supervision_train_core_coverage;
  KIND = exposure_coverage;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 0.95;
};

LATTICE_REQUIRES {
  TARGET_ID = cwu_02v_mdn_train_core_ready;
  REQUIREMENT_ID = cwu_02v_channel_target_supervision_train_core_coverage;
  KIND = exposure_coverage;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 0.95;
};

LATTICE_PLAN {
  TARGET_ID = channel_mdn_train_core_ready;
  PLAN_ID = train_channel_mdn_train_core;
  WAVE_TARGET = wikimyei.inference.expected_value.mdn;
  WAVE_MODE = train|debug;
  WAVE_RANGE = split:train_core;
  PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:vicreg_train_core_ready;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_DEPENDS {
  TARGET_ID = cwu_01v_mdn_train_core_ready;
  UPSTREAM_TARGET_ID = cwu_01v_representation_train_core_ready;
  BINDING = loaded_representation_checkpoint;
  REQUIRE_EXACT_LOADED_CHECKPOINT = true;
};

LATTICE_PLAN {
  TARGET_ID = cwu_01v_mdn_train_core_ready;
  PLAN_ID = train_cwu_01v_channel_mdn_train_core;
  WAVE_TARGET = wikimyei.inference.expected_value.mdn;
  WAVE_MODE = train|debug;
  WAVE_RANGE = split:train_core;
  PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:cwu_01v_representation_train_core_ready;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_DEPENDS {
  TARGET_ID = cwu_02v_mdn_train_core_ready;
  UPSTREAM_TARGET_ID = cwu_02v_representation_train_core_ready;
  BINDING = loaded_representation_checkpoint;
  REQUIRE_EXACT_LOADED_CHECKPOINT = true;
};

LATTICE_PLAN {
  TARGET_ID = cwu_02v_mdn_train_core_ready;
  PLAN_ID = train_cwu_02v_channel_mdn_train_core;
  WAVE_TARGET = wikimyei.inference.expected_value.mdn;
  WAVE_MODE = train|debug;
  WAVE_RANGE = split:train_core;
  PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:cwu_02v_representation_train_core_ready;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = high_channel_target_supervision_train_core_load;
  KIND = exposure_load;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  CURSOR_EPOCHS_ABOVE = 3.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = high_channel_mdn_optimizer_effort_density;
  KIND = effort_density;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = optimizer_steps_per_cursor_epoch;
  ABOVE = 10.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = high_channel_mdn_mean_nll;
  KIND = mdn_distribution_calibration;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = mean_nll;
  ABOVE = 2.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = channel_mdn_train_finite_parameter_check_failed;
  KIND = runtime_health;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = finite_parameter_check;
  BELOW = 1.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = channel_mdn_train_nonfinite_output_count;
  KIND = runtime_health;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = nonfinite_output_count;
  ABOVE = 0.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = channel_mdn_train_checkpoint_missing;
  KIND = runtime_health;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = checkpoint_written;
  BELOW = 1.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = channel_mdn_train_representation_checkpoint_missing;
  KIND = runtime_health;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = representation_checkpoint_loaded;
  BELOW = 1.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = high_channel_mdn_pre_clip_grad_norm;
  KIND = runtime_health;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = grad_norm_max_pre_clip;
  ABOVE = 1000.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = channel_mdn_valid_target_fraction_low;
  KIND = runtime_health;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = valid_target_fraction;
  BELOW = 0.05;
};

LATTICE_DEPENDS {
  TARGET_ID = channel_mdn_acceptance_smoke_ready;
  UPSTREAM_TARGET_ID = vicreg_acceptance_smoke_ready;
  BINDING = loaded_representation_checkpoint;
  REQUIRE_EXACT_LOADED_CHECKPOINT = true;
};

LATTICE_REQUIRES {
  TARGET_ID = channel_mdn_acceptance_smoke_ready;
  REQUIREMENT_ID = channel_target_supervision_acceptance_coverage;
  KIND = exposure_coverage;
  USE = target_supervision;
  SPLIT = acceptance_smoke;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 1.0;
};

LATTICE_PLAN {
  TARGET_ID = channel_mdn_acceptance_smoke_ready;
  PLAN_ID = train_channel_mdn_acceptance_smoke;
  WAVE_TARGET = wikimyei.inference.expected_value.mdn;
  WAVE_MODE = train|debug;
  WAVE_RANGE = split:acceptance_smoke;
  PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:vicreg_acceptance_smoke_ready;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_REQUIRES {
  TARGET_ID = channel_mdn_validation_eval_ready;
  REQUIREMENT_ID = channel_validation_evaluation_metric_coverage;
  KIND = exposure_coverage;
  USE = evaluation_metric;
  SPLIT = validation_holdout;
  SCOPE = target_component_family_id;
  EFFECT = none;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 0.95;
};

LATTICE_PLAN {
  TARGET_ID = channel_mdn_validation_eval_ready;
  PLAN_ID = run_channel_mdn_validation_eval;
  WAVE_TARGET = wikimyei.inference.expected_value.mdn;
  WAVE_MODE = run|debug;
  WAVE_RANGE = split:validation_holdout;
  PLAN_INPUT_MDN_CHECKPOINT = latest_satisfying:channel_mdn_train_core_no_test_leakage;
  PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:vicreg_train_core_ready;
  PLAN_MAX_ATTEMPTS = 1;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_validation_eval_ready;
  WARNING_ID = channel_mdn_validation_eval_mutated_model_state;
  KIND = runtime_health;
  USE = evaluation_metric;
  SPLIT = validation_holdout;
  SCOPE = target_component_family_id;
  EFFECT = any;
  METRIC = model_state_mutated;
  ABOVE = 0.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_validation_eval_ready;
  WARNING_ID = channel_mdn_validation_eval_checkpoint_written;
  KIND = runtime_health;
  USE = evaluation_metric;
  SPLIT = validation_holdout;
  SCOPE = target_component_family_id;
  EFFECT = any;
  METRIC = checkpoint_written;
  ABOVE = 0.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_validation_eval_ready;
  WARNING_ID = channel_mdn_validation_eval_representation_checkpoint_missing;
  KIND = runtime_health;
  USE = evaluation_metric;
  SPLIT = validation_holdout;
  SCOPE = target_component_family_id;
  EFFECT = any;
  METRIC = representation_checkpoint_loaded;
  BELOW = 1.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_validation_eval_ready;
  WARNING_ID = channel_mdn_validation_eval_mdn_checkpoint_missing;
  KIND = runtime_health;
  USE = evaluation_metric;
  SPLIT = validation_holdout;
  SCOPE = target_component_family_id;
  EFFECT = any;
  METRIC = mdn_checkpoint_loaded;
  BELOW = 1.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_validation_eval_ready;
  WARNING_ID = channel_mdn_validation_eval_nonfinite_output_count;
  KIND = runtime_health;
  USE = evaluation_metric;
  SPLIT = validation_holdout;
  SCOPE = target_component_family_id;
  EFFECT = any;
  METRIC = nonfinite_output_count;
  ABOVE = 0.0;
};
