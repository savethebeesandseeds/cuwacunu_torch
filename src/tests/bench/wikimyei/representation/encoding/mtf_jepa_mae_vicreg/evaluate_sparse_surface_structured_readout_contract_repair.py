#!/usr/bin/env python3
"""Evaluate the sealed SRR-4 sparse-surface representation-value gate.

This is deliberately a feature-probe boundary evaluator.  The SRR-4 capture
runner owns encoder-call accounting, selector/mask parity, state purity, and
artifact sealing.  This helper authenticates the frozen ``all_tokens`` probes,
validates both paired feature surfaces, and then runs the one precommitted,
equal-compute ridge comparison.  It never opens the forbidden final holdout.

The numerical and CSV primitives are imported from the already audited SRR-3
evaluator.  A compatibility guard prevents silent drift in any reused split,
ridge, bootstrap, or gate constant.
"""

from __future__ import annotations

import argparse
import hashlib
import math
import sys
from pathlib import Path
from typing import Iterable, Sequence

import numpy as np


_MODULE_DIR = Path(__file__).resolve().parent
if str(_MODULE_DIR) not in sys.path:
    sys.path.insert(0, str(_MODULE_DIR))

import evaluate_structured_readout_activation_compatibility as _srr3  # noqa: E402


SCHEMA = (
    "wikimyei.mtf_jepa_mae_vicreg."
    "sparse_surface_structured_readout_contract_repair_evaluator.v1"
)
SELF_TEST_SCHEMA = SCHEMA + ".self_test"
PROTOCOL_SHA256 = (
    "a634a1ae386a5a0bebb10440cf0d45730e2dd652375aa3fb80a4d9277708ed30"
)
PROTOCOL_SIZE_BYTES = 13_186

BASELINE_POLICY = "all_tokens"
CANDIDATE_POLICY = "structured_cdsb_sparse_v1"
ACTIVE_POLICY = BASELINE_POLICY

DEVELOPMENT_BEGIN = 0
SELECTION_FIT_END = 554
PURGE_END = 584
DEVELOPMENT_END = 730
CONFIRMATION_BEGIN = 760
CONFIRMATION_END = 1088
FINAL_HOLDOUT_BEGIN = 1088
FINAL_HOLDOUT_END = 1170

EDGE_COUNT = 3
CHANNEL_COUNT = 3
FEATURE_COUNT = 96
ROWS_PER_ANCHOR = EDGE_COUNT * CHANNEL_COUNT
MAX_DEVELOPMENT_ENCODER_BATCHES = 12
MAX_CONFIRMATION_ENCODER_BATCHES = 6

BASELINE_DEVELOPMENT_SIZE_BYTES = 13_648_442
BASELINE_DEVELOPMENT_SHA256 = (
    "d3465e44ed15647e158b9cabf00f4b1670797fdddc5539c0dd4a067db7b193ed"
)
BASELINE_CONFIRMATION_SIZE_BYTES = 6_133_066
BASELINE_CONFIRMATION_SHA256 = (
    "8f6f72b78c0708b5f23512ada4ca8536ea8818f8e2d3d9bc501401d8ab0ce3c7"
)

BOOTSTRAP_RESAMPLES = 4_096
BOOTSTRAP_SEED = 8_387_496_322_364_763_509
BOOTSTRAP_RNG = "numpy.PCG64"
RIDGE_ALPHAS = (
    1.0e-10,
    1.0e-9,
    1.0e-8,
    1.0e-7,
    1.0e-6,
    1.0e-5,
    1.0e-4,
    1.0e-3,
    1.0e-2,
    1.0e-1,
    1.0,
    10.0,
)

DIRECTION_LOWER_GATE = -0.01
RANK_LOWER_GATE = -0.01
RMSE_RATIO_UPPER_GATE = 1.05
MATERIAL_DIRECTION_POINT_GATE = 0.02
MATERIAL_RANK_POINT_GATE = 0.02
MATERIAL_RMSE_RATIO_POINT_GATE = 0.95
MATERIAL_REQUIRED_COUNT = 2
EXPECTED_SELECTION_SOLVES_PER_ARM = len(RIDGE_ALPHAS) * EDGE_COUNT
EXPECTED_SELECTED_REFIT_SOLVES_PER_ARM = EDGE_COUNT
EXPECTED_COMMON_ALPHA_REFIT_SOLVES_PER_ARM = EDGE_COUNT

DECISION_QUALIFIED = "sparse_structured_repair_qualified"
DECISION_VALUE_NOT_PASSED = "sparse_surface_value_gate_not_passed"
DECISION_INVALID = "invalid_mechanics"

EvaluationInputError = _srr3.EvaluationInputError
FeatureSurface = _srr3.FeatureSurface
Comparison = _srr3.Comparison
Interval = _srr3.Interval


def _bool(value: bool) -> str:
    return _srr3._bool(value)


def _float(value: float) -> str:
    return _srr3._float(value)


def _assert_reused_primitive_contract() -> None:
    """Fail closed if the imported audited primitive contract has drifted."""

    expected = {
        "development_begin": (_srr3.DEVELOPMENT_BEGIN, DEVELOPMENT_BEGIN),
        "selection_fit_end": (_srr3.SELECTION_FIT_END, SELECTION_FIT_END),
        "purge_end": (_srr3.PURGE_END, PURGE_END),
        "development_end": (_srr3.DEVELOPMENT_END, DEVELOPMENT_END),
        "confirmation_begin": (_srr3.STAGE_A_BEGIN, CONFIRMATION_BEGIN),
        "confirmation_end": (_srr3.STAGE_A_END, CONFIRMATION_END),
        "final_holdout_begin": (_srr3.FINAL_HOLDOUT_BEGIN, FINAL_HOLDOUT_BEGIN),
        "edge_count": (_srr3.EDGE_COUNT, EDGE_COUNT),
        "channel_count": (_srr3.CHANNEL_COUNT, CHANNEL_COUNT),
        "feature_count": (_srr3.FEATURE_COUNT, FEATURE_COUNT),
        "rows_per_anchor": (_srr3.ROWS_PER_ANCHOR, ROWS_PER_ANCHOR),
        "bootstrap_resamples": (_srr3.BOOTSTRAP_RESAMPLES, BOOTSTRAP_RESAMPLES),
        "bootstrap_seed": (_srr3.BOOTSTRAP_SEED, BOOTSTRAP_SEED),
        "direction_gate": (
            _srr3.STAGE_B_DIRECTION_LOWER_GATE,
            DIRECTION_LOWER_GATE,
        ),
        "rank_gate": (_srr3.STAGE_B_RANK_LOWER_GATE, RANK_LOWER_GATE),
        "rmse_gate": (
            _srr3.STAGE_B_RMSE_RATIO_UPPER_GATE,
            RMSE_RATIO_UPPER_GATE,
        ),
        "material_direction": (
            _srr3.MATERIAL_DIRECTION_POINT_GATE,
            MATERIAL_DIRECTION_POINT_GATE,
        ),
        "material_rank": (
            _srr3.MATERIAL_RANK_POINT_GATE,
            MATERIAL_RANK_POINT_GATE,
        ),
        "material_rmse": (
            _srr3.MATERIAL_RMSE_RATIO_POINT_GATE,
            MATERIAL_RMSE_RATIO_POINT_GATE,
        ),
    }
    for name, (actual, frozen) in expected.items():
        if actual != frozen:
            raise EvaluationInputError(f"reused_primitive_{name}_drift")
    if tuple(_srr3.RIDGE_ALPHAS) != RIDGE_ALPHAS:
        raise EvaluationInputError("reused_primitive_ridge_alpha_grid_drift")
    if _srr3.BOOTSTRAP_RNG != BOOTSTRAP_RNG:
        raise EvaluationInputError("reused_primitive_bootstrap_rng_drift")


def _require_frozen_baseline(
    surface: FeatureSurface,
    *,
    role: str,
    expected_size: int,
    expected_sha256: str,
) -> None:
    if surface.size_bytes != expected_size:
        raise EvaluationInputError(f"{role}_frozen_size_mismatch")
    if surface.sha256 != expected_sha256:
        raise EvaluationInputError(f"{role}_frozen_sha256_mismatch")


def _validate_pair(
    baseline: FeatureSurface,
    candidate: FeatureSurface,
    *,
    role: str,
) -> None:
    _srr3._require_paired(
        baseline.identities,
        candidate.identities,
        baseline.target_tokens,
        candidate.target_tokens,
        role,
    )
    if not np.array_equal(baseline.targets, candidate.targets):
        raise EvaluationInputError(f"{role}_parsed_target_mismatch")
    if baseline.features.shape != candidate.features.shape:
        raise EvaluationInputError(f"{role}_feature_shape_mismatch")
    if baseline.features.dtype != np.float64 or candidate.features.dtype != np.float64:
        raise EvaluationInputError(f"{role}_feature_dtype_mismatch")
    if not bool(np.all(np.isfinite(baseline.features))):
        raise EvaluationInputError(f"{role}_baseline_non_finite_feature")
    if not bool(np.all(np.isfinite(candidate.features))):
        raise EvaluationInputError(f"{role}_candidate_non_finite_feature")


def _identity_sha256(surface: FeatureSurface) -> str:
    digest = hashlib.sha256()
    for identity in surface.identities:
        fields = (
            str(identity.anchor_key),
            str(identity.anchor_index),
            str(identity.anchor_local_index),
            str(identity.edge_index),
            identity.edge_id,
            identity.base_node_id,
            identity.quote_node_id,
            str(identity.channel_index),
        )
        digest.update("\x1f".join(fields).encode("utf-8"))
        digest.update(b"\n")
    return digest.hexdigest()


def _target_token_sha256(surface: FeatureSurface) -> str:
    digest = hashlib.sha256()
    for token in surface.target_tokens:
        digest.update(token.encode("ascii", errors="strict"))
        digest.update(b"\n")
    return digest.hexdigest()


def _input_fields(prefix: str, surface: FeatureSurface) -> list[tuple[str, object]]:
    fields = _srr3._input_fields(prefix, surface)
    fields.extend(
        (
            (f"{prefix}.identity_sha256", _identity_sha256(surface)),
            (f"{prefix}.target_token_sha256", _target_token_sha256(surface)),
            (f"{prefix}.feature_shape", f"{surface.features.shape[0]}x{FEATURE_COUNT}"),
            (f"{prefix}.feature_dtype", str(surface.features.dtype)),
            (f"{prefix}.finite", _bool(bool(np.all(np.isfinite(surface.features))))),
        )
    )
    return fields


def _surface_diagnostics(
    prefix: str,
    surface: FeatureSurface,
    *,
    begin: int,
    end: int,
) -> list[tuple[str, object]]:
    features = surface.features
    shaped = _srr3._reshape_feature_surface(surface, begin, end)
    row_norm = np.linalg.norm(features, axis=1)
    fields: list[tuple[str, object]] = [
        (f"{prefix}.feature_mean", _float(float(np.mean(features)))),
        (f"{prefix}.feature_std", _float(float(np.std(features, ddof=0)))),
        (f"{prefix}.feature_min", _float(float(np.min(features)))),
        (f"{prefix}.feature_max", _float(float(np.max(features)))),
        (f"{prefix}.feature_l2_norm", _float(float(np.linalg.norm(features)))),
        (f"{prefix}.row_l2_mean", _float(float(np.mean(row_norm)))),
        (f"{prefix}.row_l2_std", _float(float(np.std(row_norm, ddof=0)))),
        (f"{prefix}.row_l2_min", _float(float(np.min(row_norm)))),
        (f"{prefix}.row_l2_max", _float(float(np.max(row_norm)))),
    ]
    for channel in range(CHANNEL_COUNT):
        values = shaped[:, :, channel, :]
        fields.extend(
            (
                (
                    f"{prefix}.channel_{channel}.feature_mean",
                    _float(float(np.mean(values))),
                ),
                (
                    f"{prefix}.channel_{channel}.feature_std",
                    _float(float(np.std(values, ddof=0))),
                ),
                (
                    f"{prefix}.channel_{channel}.feature_variance",
                    _float(float(np.var(values, ddof=0))),
                ),
            )
        )
    return fields


def _paired_feature_diagnostics(
    prefix: str,
    baseline: FeatureSurface,
    candidate: FeatureSurface,
) -> list[tuple[str, object]]:
    left = baseline.features
    right = candidate.features
    difference = right - left
    denominator = float(np.linalg.norm(left) * np.linalg.norm(right))
    global_cosine = (
        float(np.sum(left * right)) / denominator if denominator > 0.0 else 0.0
    )
    row_denominator = np.linalg.norm(left, axis=1) * np.linalg.norm(right, axis=1)
    row_cosines = np.divide(
        np.sum(left * right, axis=1),
        row_denominator,
        out=np.zeros_like(row_denominator),
        where=row_denominator > 0.0,
    )
    return [
        (f"{prefix}.global_cosine", _float(global_cosine)),
        (f"{prefix}.mean_row_cosine", _float(float(np.mean(row_cosines)))),
        (f"{prefix}.minimum_row_cosine", _float(float(np.min(row_cosines)))),
        (f"{prefix}.maximum_row_cosine", _float(float(np.max(row_cosines)))),
        (f"{prefix}.difference_l2_norm", _float(float(np.linalg.norm(difference)))),
        (f"{prefix}.difference_mean_abs", _float(float(np.mean(np.abs(difference))))),
        (f"{prefix}.difference_max_abs", _float(float(np.max(np.abs(difference))))),
        (f"{prefix}.feature_values_exact_equal", _bool(np.array_equal(left, right))),
    ]


def _classify(
    comparison: Comparison,
) -> tuple[str, bool, dict[str, bool], dict[str, bool]]:
    gates = {
        "direction_noninferiority": (
            comparison.direction_delta.lower >= DIRECTION_LOWER_GATE
        ),
        "rank_noninferiority": comparison.rank_delta.lower >= RANK_LOWER_GATE,
        "rmse_noninferiority": (
            comparison.rmse_ratio.upper <= RMSE_RATIO_UPPER_GATE
        ),
    }
    material = _srr3._material_flags(comparison)
    material_count = sum(
        int(material[name]) for name in ("direction", "rank", "rmse")
    )
    passed = all(gates.values()) and material_count >= MATERIAL_REQUIRED_COUNT
    decision = DECISION_QUALIFIED if passed else DECISION_VALUE_NOT_PASSED
    return decision, passed, gates, material


def _evaluate(args: argparse.Namespace) -> list[tuple[str, object]]:
    _assert_reused_primitive_contract()

    baseline_development = _srr3._load_features(
        args.baseline_dev_probe,
        role="baseline_development_probe",
        begin=DEVELOPMENT_BEGIN,
        end=DEVELOPMENT_END,
        max_batches=MAX_DEVELOPMENT_ENCODER_BATCHES,
    )
    candidate_development = _srr3._load_features(
        args.candidate_dev_probe,
        role="candidate_development_probe",
        begin=DEVELOPMENT_BEGIN,
        end=DEVELOPMENT_END,
        max_batches=MAX_DEVELOPMENT_ENCODER_BATCHES,
    )
    baseline_confirmation = _srr3._load_features(
        args.baseline_confirmation_probe,
        role="baseline_confirmation_probe",
        begin=CONFIRMATION_BEGIN,
        end=CONFIRMATION_END,
        max_batches=MAX_CONFIRMATION_ENCODER_BATCHES,
    )
    candidate_confirmation = _srr3._load_features(
        args.candidate_confirmation_probe,
        role="candidate_confirmation_probe",
        begin=CONFIRMATION_BEGIN,
        end=CONFIRMATION_END,
        max_batches=MAX_CONFIRMATION_ENCODER_BATCHES,
    )

    _require_frozen_baseline(
        baseline_development,
        role="baseline_development_probe",
        expected_size=BASELINE_DEVELOPMENT_SIZE_BYTES,
        expected_sha256=BASELINE_DEVELOPMENT_SHA256,
    )
    _require_frozen_baseline(
        baseline_confirmation,
        role="baseline_confirmation_probe",
        expected_size=BASELINE_CONFIRMATION_SIZE_BYTES,
        expected_sha256=BASELINE_CONFIRMATION_SHA256,
    )
    _validate_pair(
        baseline_development,
        candidate_development,
        role="development_pair",
    )
    _validate_pair(
        baseline_confirmation,
        candidate_confirmation,
        role="confirmation_pair",
    )

    baseline_alpha, baseline_curve, baseline_selection_solves = (
        _srr3._select_ridge_alpha(baseline_development)
    )
    candidate_alpha, candidate_curve, candidate_selection_solves = (
        _srr3._select_ridge_alpha(candidate_development)
    )
    baseline_arm = _srr3._refit_ridge_arm(
        baseline_development,
        baseline_confirmation,
        selected_alpha=baseline_alpha,
        validation_curve=baseline_curve,
        selection_solve_count=baseline_selection_solves,
        common_alpha=baseline_alpha,
    )
    candidate_arm = _srr3._refit_ridge_arm(
        candidate_development,
        candidate_confirmation,
        selected_alpha=candidate_alpha,
        validation_curve=candidate_curve,
        selection_solve_count=candidate_selection_solves,
        common_alpha=baseline_alpha,
    )

    equal_selection_compute = (
        baseline_arm.selection_solve_count == candidate_arm.selection_solve_count
    )
    equal_selected_refit_compute = (
        baseline_arm.selected.solve_count == candidate_arm.selected.solve_count
    )
    equal_common_refit_compute = (
        baseline_arm.common_alpha.solve_count
        == candidate_arm.common_alpha.solve_count
    )
    if not equal_selection_compute:
        raise EvaluationInputError("unequal_selection_compute")
    if not equal_selected_refit_compute:
        raise EvaluationInputError("unequal_selected_refit_compute")
    if not equal_common_refit_compute:
        raise EvaluationInputError("unequal_common_alpha_refit_compute")
    if (
        baseline_arm.selection_solve_count != EXPECTED_SELECTION_SOLVES_PER_ARM
        or candidate_arm.selection_solve_count != EXPECTED_SELECTION_SOLVES_PER_ARM
    ):
        raise EvaluationInputError("unexpected_selection_solve_count")
    if (
        baseline_arm.selected.solve_count
        != EXPECTED_SELECTED_REFIT_SOLVES_PER_ARM
        or candidate_arm.selected.solve_count
        != EXPECTED_SELECTED_REFIT_SOLVES_PER_ARM
    ):
        raise EvaluationInputError("unexpected_selected_refit_solve_count")
    if (
        baseline_arm.common_alpha.solve_count
        != EXPECTED_COMMON_ALPHA_REFIT_SOLVES_PER_ARM
        or candidate_arm.common_alpha.solve_count
        != EXPECTED_COMMON_ALPHA_REFIT_SOLVES_PER_ARM
    ):
        raise EvaluationInputError("unexpected_common_alpha_refit_solve_count")

    target = _srr3._reshape_target_surface(
        baseline_confirmation,
        CONFIRMATION_BEGIN,
        CONFIRMATION_END,
    )
    valid = np.ones_like(target, dtype=np.bool_)
    baseline_metric, candidate_metric, comparison = _srr3._bootstrap_comparison(
        baseline_arm.selected.predictions,
        candidate_arm.selected.predictions,
        target,
        valid,
    )
    common_baseline_metric, common_candidate_metric, common_comparison = (
        _srr3._bootstrap_comparison(
            baseline_arm.common_alpha.predictions,
            candidate_arm.common_alpha.predictions,
            target,
            valid,
        )
    )
    decision, passed, gates, material = _classify(comparison)
    material_count = sum(
        int(material[name]) for name in ("direction", "rank", "rmse")
    )

    fields: list[tuple[str, object]] = [
        ("schema", SCHEMA),
        ("protocol_sha256", PROTOCOL_SHA256),
        ("protocol_size_bytes", PROTOCOL_SIZE_BYTES),
        ("status", "completed"),
        ("mechanics.feature_probe_boundary.pass", "true"),
        ("mechanics.capture_contract_evaluated_by_this_tool", "false"),
        ("mechanics.capture_contract_required_before_invocation", "true"),
        ("mechanics.endpoint_metrics_inspected", "true"),
        ("policy.baseline", BASELINE_POLICY),
        ("policy.candidate", CANDIDATE_POLICY),
        ("policy.active", ACTIVE_POLICY),
        ("policy.rollback", BASELINE_POLICY),
        ("policy.activation_changed", "false"),
        ("settled.srr1_representation_result_retested", "false"),
        ("settled.srr2_production_parity_retested", "false"),
        ("execution.device", "cpu"),
        ("execution.dtype", "float64"),
        ("execution.optimizer_steps", 0),
        ("execution.backward_calls", 0),
        ("execution.rng_used_for_fit", "false"),
        ("execution.final_holdout_opened", "false"),
        ("execution.final_holdout_anchor_range", "[1088,1170)"),
        ("split.selection_fit_anchor_range", "[0,554)"),
        ("split.purge_anchor_range", "[554,584)"),
        ("split.validation_anchor_range", "[584,730)"),
        ("split.refit_anchor_range", "[0,730)"),
        ("split.confirmation_anchor_range", "[760,1088)"),
        (
            "split.selection_fit_row_count",
            (SELECTION_FIT_END - DEVELOPMENT_BEGIN) * ROWS_PER_ANCHOR,
        ),
        ("split.purge_row_count", (PURGE_END - SELECTION_FIT_END) * ROWS_PER_ANCHOR),
        ("split.validation_row_count", (DEVELOPMENT_END - PURGE_END) * ROWS_PER_ANCHOR),
        ("split.refit_row_count", (DEVELOPMENT_END - DEVELOPMENT_BEGIN) * ROWS_PER_ANCHOR),
        (
            "split.confirmation_row_count",
            (CONFIRMATION_END - CONFIRMATION_BEGIN) * ROWS_PER_ANCHOR,
        ),
        ("pairing.development.identity_order_target.pass", "true"),
        ("pairing.confirmation.identity_order_target.pass", "true"),
        ("pairing.feature_width_equal", "true"),
        ("pairing.full_confirmation_row_coverage", "true"),
        ("ridge.alpha_grid", ",".join(_float(alpha) for alpha in RIDGE_ALPHAS)),
        ("ridge.alpha_selection_metric", "lowest_validation_rmse"),
        ("ridge.alpha_tie_rule", "lower_alpha"),
        ("ridge.standardization", "per_arm_per_edge_fit_only_population_std"),
        ("ridge.intercept_penalized", "false"),
        ("ridge.common_alpha", _float(baseline_alpha)),
        ("compute.equal_selection.pass", _bool(equal_selection_compute)),
        ("compute.equal_selected_refit.pass", _bool(equal_selected_refit_compute)),
        ("compute.equal_common_alpha_refit.pass", _bool(equal_common_refit_compute)),
        ("compute.expected_selection_solves_per_arm", EXPECTED_SELECTION_SOLVES_PER_ARM),
        ("compute.expected_selected_refit_solves_per_arm", EXPECTED_SELECTED_REFIT_SOLVES_PER_ARM),
        (
            "compute.expected_common_alpha_refit_solves_per_arm",
            EXPECTED_COMMON_ALPHA_REFIT_SOLVES_PER_ARM,
        ),
        ("bootstrap.resamples", BOOTSTRAP_RESAMPLES),
        ("bootstrap.seed", BOOTSTRAP_SEED),
        ("bootstrap.rng", BOOTSTRAP_RNG),
        ("bootstrap.unit", "anchor_cluster"),
    ]
    fields.extend(_input_fields("input.development.baseline", baseline_development))
    fields.extend(_input_fields("input.development.candidate", candidate_development))
    fields.extend(_input_fields("input.confirmation.baseline", baseline_confirmation))
    fields.extend(_input_fields("input.confirmation.candidate", candidate_confirmation))
    fields.extend(
        _surface_diagnostics(
            "diagnostic.development.baseline",
            baseline_development,
            begin=DEVELOPMENT_BEGIN,
            end=DEVELOPMENT_END,
        )
    )
    fields.extend(
        _surface_diagnostics(
            "diagnostic.development.candidate",
            candidate_development,
            begin=DEVELOPMENT_BEGIN,
            end=DEVELOPMENT_END,
        )
    )
    fields.extend(
        _paired_feature_diagnostics(
            "diagnostic.development.paired",
            baseline_development,
            candidate_development,
        )
    )
    fields.extend(
        _surface_diagnostics(
            "diagnostic.confirmation.baseline",
            baseline_confirmation,
            begin=CONFIRMATION_BEGIN,
            end=CONFIRMATION_END,
        )
    )
    fields.extend(
        _surface_diagnostics(
            "diagnostic.confirmation.candidate",
            candidate_confirmation,
            begin=CONFIRMATION_BEGIN,
            end=CONFIRMATION_END,
        )
    )
    fields.extend(
        _paired_feature_diagnostics(
            "diagnostic.confirmation.paired",
            baseline_confirmation,
            candidate_confirmation,
        )
    )
    fields.extend(_srr3._ridge_arm_fields("arm.baseline", baseline_arm))
    fields.extend(_srr3._ridge_arm_fields("arm.candidate", candidate_arm))
    fields.extend(_srr3._metric_fields("selected.arm.baseline", baseline_metric))
    fields.extend(_srr3._metric_fields("selected.arm.candidate", candidate_metric))
    fields.extend(_srr3._comparison_fields("selected.comparison", comparison))
    fields.extend(
        _srr3._metric_fields("common_alpha.arm.baseline", common_baseline_metric)
    )
    fields.extend(
        _srr3._metric_fields("common_alpha.arm.candidate", common_candidate_metric)
    )
    fields.extend(
        _srr3._comparison_fields("common_alpha.comparison", common_comparison)
    )
    fields.extend(
        [
            (
                "common_alpha.baseline_prediction_exact_selected",
                _bool(
                    np.array_equal(
                        baseline_arm.selected.predictions,
                        baseline_arm.common_alpha.predictions,
                    )
                ),
            ),
            ("gate.direction_delta_lower.minimum", _float(DIRECTION_LOWER_GATE)),
            ("gate.direction_delta_lower.actual", _float(comparison.direction_delta.lower)),
            ("gate.direction_delta_lower.pass", _bool(gates["direction_noninferiority"])),
            ("gate.rank_delta_lower.minimum", _float(RANK_LOWER_GATE)),
            ("gate.rank_delta_lower.actual", _float(comparison.rank_delta.lower)),
            ("gate.rank_delta_lower.pass", _bool(gates["rank_noninferiority"])),
            ("gate.rmse_ratio_upper.maximum", _float(RMSE_RATIO_UPPER_GATE)),
            ("gate.rmse_ratio_upper.actual", _float(comparison.rmse_ratio.upper)),
            ("gate.rmse_ratio_upper.pass", _bool(gates["rmse_noninferiority"])),
            ("gate.noninferiority.pass", _bool(all(gates.values()))),
            ("material.direction.point_minimum", _float(MATERIAL_DIRECTION_POINT_GATE)),
            ("material.direction.lower_strict_minimum", "0"),
            ("material.direction.pass", _bool(material["direction"])),
            ("material.rank.point_minimum", _float(MATERIAL_RANK_POINT_GATE)),
            ("material.rank.lower_strict_minimum", "0"),
            ("material.rank.pass", _bool(material["rank"])),
            ("material.rmse_ratio.point_maximum", _float(MATERIAL_RMSE_RATIO_POINT_GATE)),
            ("material.rmse_ratio.upper_strict_maximum", "1"),
            ("material.rmse.pass", _bool(material["rmse"])),
            ("material.required_count", MATERIAL_REQUIRED_COUNT),
            ("material.actual_count", material_count),
            ("material.pass", _bool(material_count >= MATERIAL_REQUIRED_COUNT)),
            ("quality_gate.pass", _bool(passed)),
            ("authorization.fresh_srr3_stage_a", _bool(passed)),
            ("authorization.augmentation_attribution", "false"),
            ("final.decision", decision),
        ]
    )
    return fields


def _synthetic_comparison(
    direction: Interval,
    rank: Interval,
    rmse: Interval,
) -> Comparison:
    neutral = Interval(0.0, -0.01, 0.01)
    return Comparison(
        direction_delta=direction,
        rank_delta=rank,
        rmse_ratio=rmse,
        correlation_delta=neutral,
        best_asset_delta=neutral,
    )


def _self_tests() -> list[tuple[str, object]]:
    tests: list[tuple[str, bool]] = []

    primitive_guard = True
    try:
        _assert_reused_primitive_contract()
    except EvaluationInputError:
        primitive_guard = False
    tests.append(("reused_primitive_contract", primitive_guard))

    qualified = _synthetic_comparison(
        Interval(0.03, 0.01, 0.05),
        Interval(0.03, 0.01, 0.05),
        Interval(1.0, 0.98, 1.02),
    )
    tests.append(
        (
            "qualification_requires_two_material_flags",
            _classify(qualified)[0] == DECISION_QUALIFIED
            and _classify(qualified)[1],
        )
    )
    one_material = _synthetic_comparison(
        Interval(0.03, 0.01, 0.05),
        Interval(0.01, 0.001, 0.02),
        Interval(1.0, 0.98, 1.02),
    )
    tests.append(
        (
            "one_material_flag_does_not_qualify",
            _classify(one_material)[0] == DECISION_VALUE_NOT_PASSED
            and not _classify(one_material)[1],
        )
    )
    inferior = _synthetic_comparison(
        Interval(0.03, 0.01, 0.05),
        Interval(0.03, 0.01, 0.05),
        Interval(1.06, 1.01, 1.06),
    )
    tests.append(
        (
            "noninferiority_failure_does_not_qualify",
            _classify(inferior)[0] == DECISION_VALUE_NOT_PASSED
            and not _classify(inferior)[1],
        )
    )
    exact_boundaries = _synthetic_comparison(
        Interval(0.02, 0.0, 0.03),
        Interval(0.02, 0.0, 0.03),
        Interval(0.95, 0.90, 1.0),
    )
    tests.append(
        (
            "material_strict_confidence_boundaries",
            _classify(exact_boundaries)[3]["direction"] is False
            and _classify(exact_boundaries)[3]["rank"] is False
            and _classify(exact_boundaries)[3]["rmse"] is False,
        )
    )

    left = np.asarray([[1.0, 0.0], [0.0, 2.0]], dtype=np.float64)
    fake = type("SyntheticSurface", (), {})()
    fake.features = left
    paired = dict(_paired_feature_diagnostics("paired", fake, fake))
    tests.append(
        (
            "paired_feature_identity",
            abs(float(paired["paired.global_cosine"]) - 1.0) <= 1.0e-15
            and paired["paired.difference_l2_norm"] == "0"
            and paired["paired.feature_values_exact_equal"] == "true",
        )
    )

    target_tokens = ("0.1", "0.2")
    identity = _srr3.RowIdentity(
        anchor_key=1,
        anchor_index=0,
        anchor_local_index=0,
        edge_index=0,
        edge_id="SYNALPHASYNUSD",
        base_node_id="SYNALPHA",
        quote_node_id="SYNUSD",
        channel_index=0,
    )
    paired_left = type("PairedSurface", (), {})()
    paired_left.identities = (identity, identity)
    paired_left.target_tokens = target_tokens
    paired_left.targets = np.asarray([0.1, 0.2], dtype=np.float64)
    paired_left.features = np.zeros((2, FEATURE_COUNT), dtype=np.float64)
    paired_right = type("PairedSurface", (), {})()
    paired_right.identities = paired_left.identities
    paired_right.target_tokens = target_tokens
    paired_right.targets = paired_left.targets.copy()
    paired_right.features = paired_left.features.copy()
    pair_accepts = True
    try:
        _validate_pair(paired_left, paired_right, role="self_test_pair")
    except EvaluationInputError:
        pair_accepts = False
    tests.append(("paired_identity_target_accepts", pair_accepts))
    paired_right.target_tokens = ("0.1", "0.3")
    pair_rejects = False
    try:
        _validate_pair(paired_left, paired_right, role="self_test_pair_corrupt")
    except EvaluationInputError:
        pair_rejects = True
    tests.append(("paired_identity_target_rejects_corruption", pair_rejects))

    passed = all(value for _, value in tests)
    fields: list[tuple[str, object]] = [
        ("schema", SELF_TEST_SCHEMA),
        ("protocol_sha256", PROTOCOL_SHA256),
        ("protocol_size_bytes", PROTOCOL_SIZE_BYTES),
        ("self_test.synthetic_only", "true"),
        ("self_test.endpoint_artifacts_opened", "false"),
        ("self_test.count", len(tests)),
    ]
    for name, value in tests:
        fields.append((f"self_test.{name}.pass", _bool(value)))
    fields.append(("self_test.pass", _bool(passed)))
    return fields


def _render(fields: Iterable[tuple[str, object]]) -> str:
    return _srr3._render(fields)


def _invalid_fields(code: str) -> list[tuple[str, object]]:
    return [
        ("schema", SCHEMA),
        ("protocol_sha256", PROTOCOL_SHA256),
        ("protocol_size_bytes", PROTOCOL_SIZE_BYTES),
        ("status", "invalid"),
        ("mechanics.feature_probe_boundary.pass", "false"),
        ("mechanics.error", code),
        ("mechanics.endpoint_metrics_inspected", "false"),
        ("policy.active", ACTIVE_POLICY),
        ("policy.rollback", BASELINE_POLICY),
        ("policy.activation_changed", "false"),
        ("execution.final_holdout_opened", "false"),
        ("authorization.fresh_srr3_stage_a", "false"),
        ("authorization.augmentation_attribution", "false"),
        ("final.decision", DECISION_INVALID),
    ]


def _parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-dev-probe", type=Path)
    parser.add_argument("--candidate-dev-probe", type=Path)
    parser.add_argument("--baseline-confirmation-probe", type=Path)
    parser.add_argument("--candidate-confirmation-probe", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    inputs = (
        args.baseline_dev_probe,
        args.candidate_dev_probe,
        args.baseline_confirmation_probe,
        args.candidate_confirmation_probe,
    )
    if args.self_test:
        if any(path is not None for path in inputs):
            parser.error("--self-test cannot be combined with endpoint inputs")
        return args
    names = (
        "baseline_dev_probe",
        "candidate_dev_probe",
        "baseline_confirmation_probe",
        "candidate_confirmation_probe",
    )
    missing = [name for name in names if getattr(args, name) is None]
    if missing:
        parser.error("missing required arguments: " + ", ".join(missing))
    return args


def _write_optional_output(path: Path | None, report: str) -> None:
    if path is None:
        return
    try:
        with path.open("x", encoding="utf-8", newline="\n") as handle:
            handle.write(report)
    except FileExistsError as error:
        raise EvaluationInputError("output_already_exists") from error
    except OSError as error:
        raise EvaluationInputError("output_write_failed") from error


def main(argv: Sequence[str] | None = None) -> int:
    args = _parse_args(sys.argv[1:] if argv is None else argv)
    try:
        fields = _self_tests() if args.self_test else _evaluate(args)
        report = _render(fields)
        _write_optional_output(args.output, report)
        sys.stdout.write(report)
        if args.self_test:
            return 0 if dict(fields)["self_test.pass"] == "true" else 1
        return 0
    except EvaluationInputError as error:
        report = _render(_invalid_fields(error.code))
        sys.stdout.write(report)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
