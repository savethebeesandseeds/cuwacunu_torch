#!/usr/bin/env python3
"""Stage-A-only evaluator for the sealed SRR-3R sparse activation gate.

The capture/runner owns checkpoint authentication, encoder and MDN call
accounting, state purity, full context-mask coverage, and artifact custody.
This evaluator starts at the four persisted Stage-A CSV files.  It imports the
hash-pinned SRR-3 numerical primitives, fails closed on any drift, validates the
complete paired surface, and computes only the frozen Stage-A endpoints.

It has no development-input interface and never loads, admits, or recomputes
Stage-B evidence.  If the frozen head is incompatible, the report merely marks
the already sealed SRR-4 bounded-head evidence as eligible for later admission
by the custody runner.
"""

from __future__ import annotations

import argparse
import hashlib
import inspect
import math
import sys
from dataclasses import fields as dataclass_fields
from pathlib import Path
from typing import Iterable, Sequence

import numpy as np


_MODULE_DIR = Path(__file__).resolve().parent
if str(_MODULE_DIR) not in sys.path:
    sys.path.insert(0, str(_MODULE_DIR))

import evaluate_structured_readout_activation_compatibility as _srr3  # noqa: E402


SCHEMA = (
    "wikimyei.mtf_jepa_mae_vicreg."
    "sparse_structured_readout_activation_compatibility_evaluator.v1"
)
SELF_TEST_SCHEMA = SCHEMA + ".self_test"
PROTOCOL_SHA256 = (
    "6deee9c2420e205828322cee34b8d5d43a83c98918670ede682d8e36e17de6da"
)
PROTOCOL_SIZE_BYTES = 13_639

IMPORTED_SRR3_FILENAME = "evaluate_structured_readout_activation_compatibility.py"
IMPORTED_SRR3_SIZE_BYTES = 59_031
IMPORTED_SRR3_SHA256 = (
    "94d343284d3ce2d2272d31c6e5d24c8c34820356d5238434a85caecd8c423663"
)
IMPORTED_SRR3_SCHEMA = (
    "wikimyei.mtf_jepa_mae_vicreg."
    "structured_readout_activation_compatibility_evaluator.v1"
)
IMPORTED_SRR3_PROTOCOL_SHA256 = (
    "1c24e92a49bb59b0f0a7db63917428399619a0783216f6f3c9049c5a46cbace3"
)

BASELINE_POLICY = "all_tokens"
CANDIDATE_POLICY = "structured_cdsb_sparse_v1"
ACTIVE_POLICY = BASELINE_POLICY
HEAD_COMPUTATION = "legacy_all_tokens_head_on_sparse_semantics"

STAGE_A_BEGIN = 760
STAGE_A_END = 1088
FINAL_HOLDOUT_BEGIN = 1088
FINAL_HOLDOUT_END = 1170
EDGE_COUNT = 3
CHANNEL_COUNT = 3
FEATURE_COUNT = 96
ROWS_PER_ANCHOR = EDGE_COUNT * CHANNEL_COUNT
STAGE_A_ANCHOR_COUNT = STAGE_A_END - STAGE_A_BEGIN
STAGE_A_ROW_COUNT = STAGE_A_ANCHOR_COUNT * ROWS_PER_ANCHOR
MAX_STAGE_A_ENCODER_BATCHES = 6
MAX_BATCH_SIZE = 64

BOOTSTRAP_RESAMPLES = 4_096
BOOTSTRAP_SEED = 8_387_496_322_364_763_509
BOOTSTRAP_RNG = "numpy.PCG64"
CI_LOWER_Q = 0.025
CI_UPPER_Q = 0.975

STAGE_A_DIRECTION_LOWER_GATE = -0.02
STAGE_A_RANK_LOWER_GATE = -0.02
STAGE_A_RMSE_RATIO_UPPER_GATE = 1.10
MATERIAL_DIRECTION_POINT_GATE = 0.02
MATERIAL_RANK_POINT_GATE = 0.02
MATERIAL_RMSE_RATIO_POINT_GATE = 0.95
MATERIAL_CORRELATION_POINT_GATE = 0.05

BASELINE_FEATURE_SIZE_BYTES = 6_133_066
BASELINE_FEATURE_SHA256 = (
    "8f6f72b78c0708b5f23512ada4ca8536ea8818f8e2d3d9bc501401d8ab0ce3c7"
)
CANDIDATE_FEATURE_SIZE_BYTES = 6_112_783
CANDIDATE_FEATURE_SHA256 = (
    "dfac215b73b08525dcba90d8891c8dede328ed99ec0117e2e2efaea6a5afbd73"
)

SRR4_REPORT_SIZE_BYTES = 21_790
SRR4_REPORT_SHA256 = (
    "47252fc1fc51ca8ab55db570e914a3c2f11d62bc3e6d5dc01359c4512d61fd9f"
)
SRR4_REQUIRED_DECISION = "sparse_structured_repair_qualified"

DECISION_MIGRATION = "activation_requires_versioned_head_checkpoint_migration"
DECISION_UNRESOLVED = "downstream_bottleneck_remains_unresolved"
DECISION_SAFE_DIRECT = "safe_direct_activation"

CLASS_COMPATIBLE_USEFUL = "frozen_head_compatible_and_useful"
CLASS_COMPATIBLE_NO_GAIN = "compatible_no_downstream_gain"
CLASS_INCOMPATIBLE = "frozen_head_incompatible"
CLASS_INVALID = "invalid"

PREDICTION_HEADER = (
    "record_schema",
    "anchor_key",
    "anchor_index",
    "anchor_local_index",
    "edge_index",
    "edge_id",
    "base_node_id",
    "quote_node_id",
    "channel_index",
    "target_edge_close_return",
    "predicted_edge_close_return",
    "valid",
)
PREDICTION_OPTIONAL_HEADER = PREDICTION_HEADER + ("sigma_finite",)
FEATURE_HEADER = (
    "record_schema",
    "anchor_key",
    "anchor_index",
    "anchor_local_index",
    "edge_index",
    "edge_id",
    "base_node_id",
    "quote_node_id",
    "channel_index",
    "target_edge_close_return",
    "feature_count",
    "feature_values",
)
GRAPH_IDENTITIES = {
    0: ("SYNALPHASYNUSD", "SYNALPHA", "SYNUSD"),
    1: ("SYNBETASYNUSD", "SYNBETA", "SYNUSD"),
    2: ("SYNGAMMASYNUSD", "SYNGAMMA", "SYNUSD"),
}
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

EvaluationInputError = _srr3.EvaluationInputError
Interval = _srr3.Interval
Comparison = _srr3.Comparison


def _bool(value: bool) -> str:
    return _srr3._bool(value)


def _float(value: float) -> str:
    return _srr3._float(value)


def _comparison(
    *,
    direction_point: float = 0.0,
    direction_lower: float = STAGE_A_DIRECTION_LOWER_GATE,
    direction_upper: float = 0.01,
    rank_point: float = 0.0,
    rank_lower: float = STAGE_A_RANK_LOWER_GATE,
    rank_upper: float = 0.01,
    rmse_point: float = 1.0,
    rmse_lower: float = 0.99,
    rmse_upper: float = STAGE_A_RMSE_RATIO_UPPER_GATE,
    correlation_point: float = 0.0,
    correlation_lower: float = -0.01,
    correlation_upper: float = 0.01,
) -> Comparison:
    return Comparison(
        direction_delta=Interval(
            direction_point, direction_lower, direction_upper
        ),
        rank_delta=Interval(rank_point, rank_lower, rank_upper),
        rmse_ratio=Interval(rmse_point, rmse_lower, rmse_upper),
        correlation_delta=Interval(
            correlation_point, correlation_lower, correlation_upper
        ),
        best_asset_delta=Interval(0.0, -0.01, 0.01),
    )


def _classifier_contract_checks() -> dict[str, bool]:
    epsilon = 1.0e-9
    checks: dict[str, bool] = {}

    boundary = _comparison()
    compatible, flags, classification = _srr3._classify_stage_a(boundary)
    checks["compatibility_boundaries_inclusive"] = (
        compatible
        and not any(flags.values())
        and classification == CLASS_COMPATIBLE_NO_GAIN
    )

    for name, comparison in (
        (
            "direction_failure",
            _comparison(
                direction_lower=STAGE_A_DIRECTION_LOWER_GATE - epsilon
            ),
        ),
        (
            "rank_failure",
            _comparison(rank_lower=STAGE_A_RANK_LOWER_GATE - epsilon),
        ),
        (
            "rmse_failure",
            _comparison(
                rmse_upper=STAGE_A_RMSE_RATIO_UPPER_GATE + epsilon
            ),
        ),
    ):
        checks[name] = (
            _srr3._classify_stage_a(comparison)[2] == CLASS_INCOMPATIBLE
        )

    material_cases = {
        "direction_material": _comparison(
            direction_point=MATERIAL_DIRECTION_POINT_GATE,
            direction_lower=epsilon,
        ),
        "rank_material": _comparison(
            rank_point=MATERIAL_RANK_POINT_GATE,
            rank_lower=epsilon,
        ),
        "rmse_material": _comparison(
            rmse_point=MATERIAL_RMSE_RATIO_POINT_GATE,
            rmse_upper=1.0 - epsilon,
        ),
        "correlation_material": _comparison(
            correlation_point=MATERIAL_CORRELATION_POINT_GATE,
            correlation_lower=epsilon,
        ),
    }
    for name, comparison in material_cases.items():
        compatibility, flags, classification = _srr3._classify_stage_a(
            comparison
        )
        expected_flag = name.removesuffix("_material")
        checks[name] = (
            compatibility
            and flags[expected_flag]
            and sum(int(value) for value in flags.values()) == 1
            and classification == CLASS_COMPATIBLE_USEFUL
        )

    strict_cases = {
        "direction_point_strict": _comparison(
            direction_point=MATERIAL_DIRECTION_POINT_GATE - epsilon,
            direction_lower=epsilon,
        ),
        "direction_lower_strict": _comparison(
            direction_point=MATERIAL_DIRECTION_POINT_GATE,
            direction_lower=0.0,
        ),
        "rank_point_strict": _comparison(
            rank_point=MATERIAL_RANK_POINT_GATE - epsilon,
            rank_lower=epsilon,
        ),
        "rank_lower_strict": _comparison(
            rank_point=MATERIAL_RANK_POINT_GATE,
            rank_lower=0.0,
        ),
        "rmse_point_strict": _comparison(
            rmse_point=MATERIAL_RMSE_RATIO_POINT_GATE + epsilon,
            rmse_upper=1.0 - epsilon,
        ),
        "rmse_upper_strict": _comparison(
            rmse_point=MATERIAL_RMSE_RATIO_POINT_GATE,
            rmse_upper=1.0,
        ),
        "correlation_point_strict": _comparison(
            correlation_point=MATERIAL_CORRELATION_POINT_GATE - epsilon,
            correlation_lower=epsilon,
        ),
        "correlation_lower_strict": _comparison(
            correlation_point=MATERIAL_CORRELATION_POINT_GATE,
            correlation_lower=0.0,
        ),
    }
    for name, comparison in strict_cases.items():
        checks[name] = (
            _srr3._classify_stage_a(comparison)[2]
            == CLASS_COMPATIBLE_NO_GAIN
        )

    correlation_material = material_cases["correlation_material"]
    _, disabled_flags, disabled_classification = _srr3._classify_stage_a(
        correlation_material, correlation_defined=False
    )
    checks["undefined_correlation_cannot_materialize"] = (
        not disabled_flags["correlation"]
        and disabled_classification == CLASS_COMPATIBLE_NO_GAIN
    )

    incompatible_and_material = _comparison(
        direction_point=MATERIAL_DIRECTION_POINT_GATE,
        direction_lower=STAGE_A_DIRECTION_LOWER_GATE - epsilon,
        correlation_point=MATERIAL_CORRELATION_POINT_GATE,
        correlation_lower=epsilon,
    )
    checks["incompatibility_precedes_materiality"] = (
        _srr3._classify_stage_a(incompatible_and_material)[2]
        == CLASS_INCOMPATIBLE
    )
    checks["material_flag_keys_exact"] = set(
        _srr3._material_flags(boundary)
    ) == {"direction", "rank", "rmse", "correlation"}
    return checks


def _assert_reused_primitive_contract() -> None:
    """Fail closed before endpoint files open if the audited import drifted."""

    source_path = Path(_srr3.__file__)
    if source_path.name != IMPORTED_SRR3_FILENAME:
        raise EvaluationInputError("reused_primitive_filename_drift")
    if source_path.is_symlink() or not source_path.is_file():
        raise EvaluationInputError("reused_primitive_source_not_regular")
    try:
        raw = source_path.read_bytes()
    except OSError as error:
        raise EvaluationInputError("reused_primitive_source_unreadable") from error
    if len(raw) != IMPORTED_SRR3_SIZE_BYTES:
        raise EvaluationInputError("reused_primitive_source_size_drift")
    if hashlib.sha256(raw).hexdigest() != IMPORTED_SRR3_SHA256:
        raise EvaluationInputError("reused_primitive_source_sha256_drift")

    scalar_expected = {
        "SCHEMA": IMPORTED_SRR3_SCHEMA,
        "PROTOCOL_SHA256": IMPORTED_SRR3_PROTOCOL_SHA256,
        "PROTOCOL_SIZE_BYTES": 7_306,
        "BASELINE_POLICY": BASELINE_POLICY,
        "CANDIDATE_POLICY": "structured_cdsb_v1",
        "PREDICTION_RECORD_SCHEMA": (
            "kikijyeba.synthetic.srr3_direct_edge_prediction_probe.v1"
        ),
        "FEATURE_RECORD_SCHEMA": (
            "kikijyeba.synthetic.representation_edge_feature_probe.v1"
        ),
        "STAGE_A_BEGIN": STAGE_A_BEGIN,
        "STAGE_A_END": STAGE_A_END,
        "DEVELOPMENT_BEGIN": 0,
        "SELECTION_FIT_END": 554,
        "PURGE_END": 584,
        "DEVELOPMENT_END": 730,
        "FINAL_HOLDOUT_BEGIN": FINAL_HOLDOUT_BEGIN,
        "EDGE_COUNT": EDGE_COUNT,
        "CHANNEL_COUNT": CHANNEL_COUNT,
        "FEATURE_COUNT": FEATURE_COUNT,
        "ROWS_PER_ANCHOR": ROWS_PER_ANCHOR,
        "STAGE_A_ROW_COUNT": STAGE_A_ROW_COUNT,
        "DEVELOPMENT_ROW_COUNT": 6_570,
        "MAX_STAGE_A_ENCODER_BATCHES": MAX_STAGE_A_ENCODER_BATCHES,
        "MAX_BATCH_SIZE": MAX_BATCH_SIZE,
        "BOOTSTRAP_RESAMPLES": BOOTSTRAP_RESAMPLES,
        "BOOTSTRAP_SEED": BOOTSTRAP_SEED,
        "BOOTSTRAP_RNG": BOOTSTRAP_RNG,
        "CI_LOWER_Q": CI_LOWER_Q,
        "CI_UPPER_Q": CI_UPPER_Q,
        "STAGE_A_DIRECTION_LOWER_GATE": STAGE_A_DIRECTION_LOWER_GATE,
        "STAGE_A_RANK_LOWER_GATE": STAGE_A_RANK_LOWER_GATE,
        "STAGE_A_RMSE_RATIO_UPPER_GATE": STAGE_A_RMSE_RATIO_UPPER_GATE,
        "STAGE_B_DIRECTION_LOWER_GATE": -0.01,
        "STAGE_B_RANK_LOWER_GATE": -0.01,
        "STAGE_B_RMSE_RATIO_UPPER_GATE": 1.05,
        "MATERIAL_DIRECTION_POINT_GATE": MATERIAL_DIRECTION_POINT_GATE,
        "MATERIAL_RANK_POINT_GATE": MATERIAL_RANK_POINT_GATE,
        "MATERIAL_RMSE_RATIO_POINT_GATE": MATERIAL_RMSE_RATIO_POINT_GATE,
        "MATERIAL_CORRELATION_POINT_GATE": MATERIAL_CORRELATION_POINT_GATE,
    }
    for name, expected in scalar_expected.items():
        if not hasattr(_srr3, name) or getattr(_srr3, name) != expected:
            raise EvaluationInputError(
                f"reused_primitive_constant_{name.lower()}_drift"
            )

    tuple_expected = {
        "PREDICTION_HEADER": PREDICTION_HEADER,
        "PREDICTION_OPTIONAL_HEADER": PREDICTION_OPTIONAL_HEADER,
        "FEATURE_HEADER": FEATURE_HEADER,
        "RIDGE_ALPHAS": RIDGE_ALPHAS,
    }
    for name, expected in tuple_expected.items():
        if not hasattr(_srr3, name) or tuple(getattr(_srr3, name)) != expected:
            raise EvaluationInputError(
                f"reused_primitive_constant_{name.lower()}_drift"
            )
    if _srr3.GRAPH_IDENTITIES != GRAPH_IDENTITIES:
        raise EvaluationInputError("reused_primitive_graph_identities_drift")

    dataclass_contracts = {
        "RowIdentity": (
            "anchor_key",
            "anchor_index",
            "anchor_local_index",
            "edge_index",
            "edge_id",
            "base_node_id",
            "quote_node_id",
            "channel_index",
        ),
        "PredictionSurface": (
            "path",
            "size_bytes",
            "sha256",
            "identities",
            "target_tokens",
            "targets",
            "predictions",
            "valid",
            "sigma_finite",
            "batch_count",
        ),
        "FeatureSurface": (
            "path",
            "size_bytes",
            "sha256",
            "identities",
            "target_tokens",
            "targets",
            "features",
            "batch_count",
        ),
        "Metrics": (
            "valid_count",
            "valid_coverage",
            "direction",
            "rank_count",
            "rank",
            "rmse",
            "correlation",
            "correlation_defined",
            "best_asset_count",
            "best_asset",
            "prediction_mean",
            "prediction_std",
            "prediction_min",
            "prediction_max",
        ),
        "Interval": ("point", "lower", "upper"),
        "Comparison": (
            "direction_delta",
            "rank_delta",
            "rmse_ratio",
            "correlation_delta",
            "best_asset_delta",
        ),
    }
    for name, expected_fields in dataclass_contracts.items():
        value = getattr(_srr3, name, None)
        try:
            actual_fields = tuple(field.name for field in dataclass_fields(value))
        except (TypeError, AttributeError) as error:
            raise EvaluationInputError(
                f"reused_primitive_dataclass_{name.lower()}_drift"
            ) from error
        if actual_fields != expected_fields:
            raise EvaluationInputError(
                f"reused_primitive_dataclass_{name.lower()}_drift"
            )

    signatures = {
        "_load_predictions": "(path: 'Path', *, role: 'str', begin: 'int', end: 'int', max_batches: 'int | None') -> 'PredictionSurface'",
        "_load_features": "(path: 'Path', *, role: 'str', begin: 'int', end: 'int', max_batches: 'int | None') -> 'FeatureSurface'",
        "_validate_stage_a_pair": "(baseline_prediction: 'PredictionSurface', candidate_prediction: 'PredictionSurface', baseline_feature: 'FeatureSurface', candidate_feature: 'FeatureSurface') -> 'None'",
        "_metrics": "(prediction: 'np.ndarray', target: 'np.ndarray', valid: 'np.ndarray') -> 'Metrics'",
        "_bootstrap_comparison": "(baseline_prediction: 'np.ndarray', candidate_prediction: 'np.ndarray', target: 'np.ndarray', valid: 'np.ndarray') -> 'tuple[Metrics, Metrics, Comparison]'",
        "_material_flags": "(comparison: 'Comparison') -> 'dict[str, bool]'",
        "_classify_stage_a": "(comparison: 'Comparison', *, correlation_defined: 'bool' = True) -> 'tuple[bool, dict[str, bool], str]'",
    }
    for name, expected_signature in signatures.items():
        value = getattr(_srr3, name, None)
        if not callable(value) or str(inspect.signature(value)) != expected_signature:
            raise EvaluationInputError(
                f"reused_primitive_signature_{name.removeprefix('_')}_drift"
            )

    checks = _classifier_contract_checks()
    failed = [name for name, passed in checks.items() if not passed]
    if failed:
        raise EvaluationInputError(
            "reused_primitive_classifier_behavior_drift_" + failed[0]
        )


def _require_frozen_feature(
    surface: object,
    *,
    role: str,
    expected_size: int,
    expected_sha256: str,
) -> None:
    if getattr(surface, "size_bytes") != expected_size:
        raise EvaluationInputError(f"{role}_frozen_size_mismatch")
    if getattr(surface, "sha256") != expected_sha256:
        raise EvaluationInputError(f"{role}_frozen_sha256_mismatch")


def _recommendation_fields(classification: str) -> list[tuple[str, object]]:
    common: list[tuple[str, object]] = [
        ("stage_b.evidence.loaded_by_evaluator", "false"),
        ("stage_b.evidence.admitted_by_evaluator", "false"),
        ("stage_b.evidence.recomputed_by_evaluator", "false"),
        ("stage_b.evidence.new_encoder_calls", 0),
        ("stage_b.evidence.new_head_fits", 0),
        ("final.recommendation.safe_direct_activation_authorized", "false"),
        ("final.recommendation.policy_activation_authorized", "false"),
        ("final.recommendation.rollback_policy", BASELINE_POLICY),
    ]
    if classification == CLASS_COMPATIBLE_USEFUL:
        return [
            ("stage_b.status", "not_authorized"),
            ("stage_b.reason", "stage_a_frozen_head_compatible_and_useful"),
            ("stage_b.evidence.eligible_for_runner_admission", "false"),
            *common,
            ("final.recommendation.status", "complete"),
            ("final.recommendation.decision", DECISION_MIGRATION),
            (
                "final.recommendation.head_action",
                "versioned_identity_migration_preserve_frozen_head_tensors",
            ),
            (
                "final.recommendation.requires_authenticated_stage_b_evidence",
                "false",
            ),
        ]
    if classification == CLASS_COMPATIBLE_NO_GAIN:
        return [
            ("stage_b.status", "not_authorized"),
            ("stage_b.reason", "stage_a_compatible_no_downstream_gain"),
            ("stage_b.evidence.eligible_for_runner_admission", "false"),
            *common,
            ("final.recommendation.status", "complete"),
            ("final.recommendation.decision", DECISION_UNRESOLVED),
            ("final.recommendation.head_action", "retain_all_tokens"),
            (
                "final.recommendation.requires_authenticated_stage_b_evidence",
                "false",
            ),
        ]
    if classification == CLASS_INCOMPATIBLE:
        return [
            ("stage_b.status", "eligible_for_authenticated_srr4_admission"),
            ("stage_b.reason", "stage_a_frozen_head_incompatible"),
            ("stage_b.evidence.eligible_for_runner_admission", "true"),
            ("stage_b.evidence.required_report_size_bytes", SRR4_REPORT_SIZE_BYTES),
            ("stage_b.evidence.required_report_sha256", SRR4_REPORT_SHA256),
            ("stage_b.evidence.required_decision", SRR4_REQUIRED_DECISION),
            *common,
            (
                "final.recommendation.status",
                "pending_authenticated_srr4_bounded_head_evidence",
            ),
            ("final.recommendation.decision", "not_emitted"),
            ("final.recommendation.head_action", "pending_stage_b_evidence"),
            (
                "final.recommendation.requires_authenticated_stage_b_evidence",
                "true",
            ),
            ("final.recommendation.if_stage_b_pass", DECISION_MIGRATION),
            ("final.recommendation.if_stage_b_fail", DECISION_UNRESOLVED),
        ]
    raise EvaluationInputError("unexpected_stage_a_classification")


def _evaluate(args: argparse.Namespace) -> list[tuple[str, object]]:
    # The import contract is authenticated before any endpoint path is opened.
    _assert_reused_primitive_contract()

    baseline_prediction = _srr3._load_predictions(
        args.baseline_predictions,
        role="baseline_predictions",
        begin=STAGE_A_BEGIN,
        end=STAGE_A_END,
        max_batches=MAX_STAGE_A_ENCODER_BATCHES,
    )
    candidate_prediction = _srr3._load_predictions(
        args.candidate_predictions,
        role="candidate_predictions",
        begin=STAGE_A_BEGIN,
        end=STAGE_A_END,
        max_batches=MAX_STAGE_A_ENCODER_BATCHES,
    )
    baseline_feature = _srr3._load_features(
        args.baseline_eval_probe,
        role="baseline_eval_probe",
        begin=STAGE_A_BEGIN,
        end=STAGE_A_END,
        max_batches=MAX_STAGE_A_ENCODER_BATCHES,
    )
    candidate_feature = _srr3._load_features(
        args.candidate_eval_probe,
        role="candidate_eval_probe",
        begin=STAGE_A_BEGIN,
        end=STAGE_A_END,
        max_batches=MAX_STAGE_A_ENCODER_BATCHES,
    )
    _srr3._validate_stage_a_pair(
        baseline_prediction,
        candidate_prediction,
        baseline_feature,
        candidate_feature,
    )
    _require_frozen_feature(
        baseline_feature,
        role="baseline_eval_probe",
        expected_size=BASELINE_FEATURE_SIZE_BYTES,
        expected_sha256=BASELINE_FEATURE_SHA256,
    )
    _require_frozen_feature(
        candidate_feature,
        role="candidate_eval_probe",
        expected_size=CANDIDATE_FEATURE_SIZE_BYTES,
        expected_sha256=CANDIDATE_FEATURE_SHA256,
    )
    surfaces = (
        baseline_prediction,
        candidate_prediction,
        baseline_feature,
        candidate_feature,
    )
    if any(surface.batch_count != MAX_STAGE_A_ENCODER_BATCHES for surface in surfaces):
        raise EvaluationInputError("stage_a_source_batch_count_not_exact")
    if baseline_prediction.sigma_finite is None or candidate_prediction.sigma_finite is None:
        raise EvaluationInputError("stage_a_sigma_diagnostic_missing")
    if not bool(np.all(baseline_prediction.sigma_finite)) or not bool(
        np.all(candidate_prediction.sigma_finite)
    ):
        raise EvaluationInputError("stage_a_sigma_not_finite")

    target = baseline_prediction.targets.reshape(
        STAGE_A_ANCHOR_COUNT, EDGE_COUNT, CHANNEL_COUNT
    )
    valid = baseline_prediction.valid.reshape(
        STAGE_A_ANCHOR_COUNT, EDGE_COUNT, CHANNEL_COUNT
    )
    baseline_values = baseline_prediction.predictions.reshape(
        STAGE_A_ANCHOR_COUNT, EDGE_COUNT, CHANNEL_COUNT
    )
    candidate_values = candidate_prediction.predictions.reshape(
        STAGE_A_ANCHOR_COUNT, EDGE_COUNT, CHANNEL_COUNT
    )
    baseline_metric, candidate_metric, comparison = _srr3._bootstrap_comparison(
        baseline_values, candidate_values, target, valid
    )
    compatibility, material, classification = _srr3._classify_stage_a(
        comparison,
        correlation_defined=(
            baseline_metric.correlation_defined
            and candidate_metric.correlation_defined
        ),
    )
    direction_gate = (
        comparison.direction_delta.lower >= STAGE_A_DIRECTION_LOWER_GATE
    )
    rank_gate = comparison.rank_delta.lower >= STAGE_A_RANK_LOWER_GATE
    rmse_gate = (
        comparison.rmse_ratio.upper <= STAGE_A_RMSE_RATIO_UPPER_GATE
    )
    if compatibility != (direction_gate and rank_gate and rmse_gate):
        raise EvaluationInputError("stage_a_classifier_gate_inconsistent")

    fields: list[tuple[str, object]] = [
        ("schema", SCHEMA),
        ("protocol_sha256", PROTOCOL_SHA256),
        ("protocol_size_bytes", PROTOCOL_SIZE_BYTES),
        ("status", "completed"),
        ("scope", "stage_a_only"),
        ("baseline_policy", BASELINE_POLICY),
        ("candidate_policy", CANDIDATE_POLICY),
        ("active_policy", ACTIVE_POLICY),
        ("head.saved_checkpoint_policy", BASELINE_POLICY),
        ("head.counterfactual_computation", HEAD_COMPUTATION),
        ("reused_srr3.source_sha256", IMPORTED_SRR3_SHA256),
        ("reused_srr3.contract_guard.pass", "true"),
        ("mechanics.pass", "true"),
        ("mechanics.endpoint_metrics_inspected", "true"),
        ("mechanics.development_inputs_supported", "false"),
        ("mechanics.stage_b_evidence_opened", "false"),
        ("stage_a.anchor_range", "[760,1088)"),
        ("stage_a.anchor_count", STAGE_A_ANCHOR_COUNT),
        ("stage_a.expected_row_count", STAGE_A_ROW_COUNT),
        ("stage_a.source_batch_count", baseline_prediction.batch_count),
        ("stage_a.source_batch_ceiling", MAX_STAGE_A_ENCODER_BATCHES),
        ("stage_a.bootstrap.resamples", BOOTSTRAP_RESAMPLES),
        ("stage_a.bootstrap.seed", BOOTSTRAP_SEED),
        ("stage_a.bootstrap.cluster_unit", "anchor"),
        ("stage_a.bootstrap.rng", BOOTSTRAP_RNG),
        (
            "stage_a.bootstrap.quantile_method",
            "linear_order_statistic_interpolation",
        ),
    ]
    fields.extend(
        _srr3._input_fields("input.baseline_predictions", baseline_prediction)
    )
    fields.extend(
        _srr3._input_fields("input.candidate_predictions", candidate_prediction)
    )
    fields.extend(_srr3._input_fields("input.baseline_eval_probe", baseline_feature))
    fields.extend(_srr3._input_fields("input.candidate_eval_probe", candidate_feature))
    fields.extend(
        [
            ("input.baseline_eval_probe.frozen_size_bytes", BASELINE_FEATURE_SIZE_BYTES),
            ("input.baseline_eval_probe.frozen_sha256", BASELINE_FEATURE_SHA256),
            ("input.baseline_eval_probe.frozen_match", "true"),
            ("input.candidate_eval_probe.frozen_size_bytes", CANDIDATE_FEATURE_SIZE_BYTES),
            ("input.candidate_eval_probe.frozen_sha256", CANDIDATE_FEATURE_SHA256),
            ("input.candidate_eval_probe.frozen_match", "true"),
        ]
    )
    fields.extend(
        _srr3._arm_diagnostics(
            "stage_a.diagnostic.arm.baseline",
            baseline_prediction,
            baseline_feature,
        )
    )
    fields.extend(
        _srr3._arm_diagnostics(
            "stage_a.diagnostic.arm.candidate",
            candidate_prediction,
            candidate_feature,
        )
    )
    fields.extend(
        _srr3._paired_feature_diagnostics(
            baseline_feature, candidate_feature, baseline_prediction.valid
        )
    )
    fields.extend(_srr3._metric_fields("stage_a.arm.baseline", baseline_metric))
    fields.extend(_srr3._metric_fields("stage_a.arm.candidate", candidate_metric))
    fields.extend(_srr3._comparison_fields("stage_a.comparison", comparison))
    fields.extend(
        [
            (
                "stage_a.comparison.valid_coverage_delta",
                _float(candidate_metric.valid_coverage - baseline_metric.valid_coverage),
            ),
            ("stage_a.gate.valid_coverage_exact.pass", "true"),
            ("stage_a.gate.finite_outputs.pass", "true"),
            (
                "stage_a.gate.direction_lower.minimum",
                _float(STAGE_A_DIRECTION_LOWER_GATE),
            ),
            ("stage_a.gate.direction_lower.pass", _bool(direction_gate)),
            (
                "stage_a.gate.rank_lower.minimum",
                _float(STAGE_A_RANK_LOWER_GATE),
            ),
            ("stage_a.gate.rank_lower.pass", _bool(rank_gate)),
            (
                "stage_a.gate.rmse_ratio_upper.maximum",
                _float(STAGE_A_RMSE_RATIO_UPPER_GATE),
            ),
            ("stage_a.gate.rmse_ratio_upper.pass", _bool(rmse_gate)),
            ("stage_a.compatibility.pass", _bool(compatibility)),
            ("stage_a.material.direction", _bool(material["direction"])),
            ("stage_a.material.rank", _bool(material["rank"])),
            ("stage_a.material.rmse", _bool(material["rmse"])),
            ("stage_a.material.correlation", _bool(material["correlation"])),
            (
                "stage_a.material.count",
                sum(int(value) for value in material.values()),
            ),
            ("stage_a.classification", classification),
        ]
    )
    fields.extend(_recommendation_fields(classification))
    return fields


def _self_tests() -> list[tuple[str, object]]:
    _assert_reused_primitive_contract()
    tests: list[tuple[str, bool]] = []

    imported = dict(_srr3._self_tests())
    tests.append(("imported_srr3_self_test", imported.get("self_test.pass") == "true"))
    for name, passed in _classifier_contract_checks().items():
        tests.append((f"classifier_{name}", passed))

    useful = dict(_recommendation_fields(CLASS_COMPATIBLE_USEFUL))
    tests.append(
        (
            "useful_maps_to_versioned_identity_migration",
            useful["stage_b.status"] == "not_authorized"
            and useful["final.recommendation.status"] == "complete"
            and useful["final.recommendation.decision"] == DECISION_MIGRATION
            and useful[
                "final.recommendation.requires_authenticated_stage_b_evidence"
            ]
            == "false",
        )
    )
    no_gain = dict(_recommendation_fields(CLASS_COMPATIBLE_NO_GAIN))
    tests.append(
        (
            "no_gain_maps_to_canonical_unresolved",
            no_gain["stage_b.status"] == "not_authorized"
            and no_gain["final.recommendation.decision"] == DECISION_UNRESOLVED,
        )
    )
    incompatible = dict(_recommendation_fields(CLASS_INCOMPATIBLE))
    tests.append(
        (
            "incompatible_only_marks_srr4_evidence_eligible",
            incompatible["stage_b.status"]
            == "eligible_for_authenticated_srr4_admission"
            and incompatible["stage_b.evidence.eligible_for_runner_admission"]
            == "true"
            and incompatible["stage_b.evidence.loaded_by_evaluator"] == "false"
            and incompatible["stage_b.evidence.recomputed_by_evaluator"] == "false"
            and incompatible["final.recommendation.decision"] == "not_emitted"
            and incompatible["final.recommendation.if_stage_b_pass"]
            == DECISION_MIGRATION
            and incompatible["final.recommendation.if_stage_b_fail"]
            == DECISION_UNRESOLVED,
        )
    )
    tests.append(
        (
            "safe_direct_never_emitted",
            all(
                DECISION_SAFE_DIRECT not in str(value)
                for classification in (
                    CLASS_COMPATIBLE_USEFUL,
                    CLASS_COMPATIBLE_NO_GAIN,
                    CLASS_INCOMPATIBLE,
                )
                for _, value in _recommendation_fields(classification)
            ),
        )
    )
    tests.append(
        (
            "canonical_unresolved_spelling",
            DECISION_UNRESOLVED == "downstream_bottleneck_remains_unresolved",
        )
    )

    passed = all(value for _, value in tests)
    fields: list[tuple[str, object]] = [
        ("schema", SELF_TEST_SCHEMA),
        ("protocol_sha256", PROTOCOL_SHA256),
        ("protocol_size_bytes", PROTOCOL_SIZE_BYTES),
        ("reused_srr3.source_sha256", IMPORTED_SRR3_SHA256),
        ("self_test.synthetic_only", "true"),
        ("self_test.endpoint_artifacts_opened", "false"),
        ("self_test.development_artifacts_opened", "false"),
        ("self_test.srr4_evidence_opened", "false"),
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
        ("scope", "stage_a_only"),
        ("baseline_policy", BASELINE_POLICY),
        ("candidate_policy", CANDIDATE_POLICY),
        ("mechanics.pass", "false"),
        ("mechanics.error", code),
        ("mechanics.endpoint_metrics_inspected", "false"),
        ("mechanics.development_inputs_supported", "false"),
        ("mechanics.stage_b_evidence_opened", "false"),
        ("stage_a.compatibility.pass", "false"),
        ("stage_a.classification", CLASS_INVALID),
        ("stage_b.status", "not_authorized"),
        ("stage_b.evidence.eligible_for_runner_admission", "false"),
        ("stage_b.evidence.loaded_by_evaluator", "false"),
        ("stage_b.evidence.recomputed_by_evaluator", "false"),
        ("final.recommendation.status", "invalid_mechanics"),
        ("final.recommendation.decision", "not_emitted"),
        ("final.recommendation.policy_activation_authorized", "false"),
        ("final.recommendation.rollback_policy", BASELINE_POLICY),
    ]


def _parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-predictions", type=Path)
    parser.add_argument("--candidate-predictions", type=Path)
    parser.add_argument("--baseline-eval-probe", type=Path)
    parser.add_argument("--candidate-eval-probe", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    inputs = (
        args.baseline_predictions,
        args.candidate_predictions,
        args.baseline_eval_probe,
        args.candidate_eval_probe,
    )
    if args.self_test:
        if any(value is not None for value in inputs):
            parser.error("--self-test cannot be combined with endpoint inputs")
        return args
    names = (
        "baseline_predictions",
        "candidate_predictions",
        "baseline_eval_probe",
        "candidate_eval_probe",
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
