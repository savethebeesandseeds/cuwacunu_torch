#!/usr/bin/env python3
"""Deterministic offline evaluator for the sealed SRR-3 protocol.

The capture/runner owns checkpoint authentication, encoder-call accounting,
state non-mutation, and artifact sealing.  This helper begins at the persisted
CSV boundary: it validates the complete paired row contract before computing
any endpoint.  Development probes are opened only when the Stage-A result is
``frozen_head_incompatible`` and both development paths were supplied.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import io
import math
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence

import numpy as np


SCHEMA = (
    "wikimyei.mtf_jepa_mae_vicreg."
    "structured_readout_activation_compatibility_evaluator.v1"
)
SELF_TEST_SCHEMA = SCHEMA + ".self_test"
PROTOCOL_SHA256 = (
    "1c24e92a49bb59b0f0a7db63917428399619a0783216f6f3c9049c5a46cbace3"
)
PROTOCOL_SIZE_BYTES = 7_306

BASELINE_POLICY = "all_tokens"
CANDIDATE_POLICY = "structured_cdsb_v1"
PREDICTION_RECORD_SCHEMA = (
    "kikijyeba.synthetic.srr3_direct_edge_prediction_probe.v1"
)
FEATURE_RECORD_SCHEMA = (
    "kikijyeba.synthetic.representation_edge_feature_probe.v1"
)

STAGE_A_BEGIN = 760
STAGE_A_END = 1088
DEVELOPMENT_BEGIN = 0
SELECTION_FIT_END = 554
PURGE_END = 584
DEVELOPMENT_END = 730
FINAL_HOLDOUT_BEGIN = 1088
EDGE_COUNT = 3
CHANNEL_COUNT = 3
FEATURE_COUNT = 96
ROWS_PER_ANCHOR = EDGE_COUNT * CHANNEL_COUNT
STAGE_A_ROW_COUNT = (STAGE_A_END - STAGE_A_BEGIN) * ROWS_PER_ANCHOR
DEVELOPMENT_ROW_COUNT = (
    (DEVELOPMENT_END - DEVELOPMENT_BEGIN) * ROWS_PER_ANCHOR
)
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
STAGE_B_DIRECTION_LOWER_GATE = -0.01
STAGE_B_RANK_LOWER_GATE = -0.01
STAGE_B_RMSE_RATIO_UPPER_GATE = 1.05
MATERIAL_DIRECTION_POINT_GATE = 0.02
MATERIAL_RANK_POINT_GATE = 0.02
MATERIAL_RMSE_RATIO_POINT_GATE = 0.95
MATERIAL_CORRELATION_POINT_GATE = 0.05

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


class EvaluationInputError(RuntimeError):
    """The persisted input contract is not safe to evaluate."""

    def __init__(self, code: str) -> None:
        super().__init__(code)
        self.code = code


@dataclass(frozen=True)
class RowIdentity:
    anchor_key: int
    anchor_index: int
    anchor_local_index: int
    edge_index: int
    edge_id: str
    base_node_id: str
    quote_node_id: str
    channel_index: int


@dataclass(frozen=True)
class PredictionSurface:
    path: Path
    size_bytes: int
    sha256: str
    identities: tuple[RowIdentity, ...]
    target_tokens: tuple[str, ...]
    targets: np.ndarray
    predictions: np.ndarray
    valid: np.ndarray
    sigma_finite: np.ndarray | None
    batch_count: int


@dataclass(frozen=True)
class FeatureSurface:
    path: Path
    size_bytes: int
    sha256: str
    identities: tuple[RowIdentity, ...]
    target_tokens: tuple[str, ...]
    targets: np.ndarray
    features: np.ndarray
    batch_count: int


@dataclass(frozen=True)
class Metrics:
    valid_count: int
    valid_coverage: float
    direction: float
    rank_count: int
    rank: float
    rmse: float
    correlation: float
    correlation_defined: bool
    best_asset_count: int
    best_asset: float
    prediction_mean: float
    prediction_std: float
    prediction_min: float
    prediction_max: float


@dataclass(frozen=True)
class Interval:
    point: float
    lower: float
    upper: float


@dataclass(frozen=True)
class Comparison:
    direction_delta: Interval
    rank_delta: Interval
    rmse_ratio: Interval
    correlation_delta: Interval
    best_asset_delta: Interval


@dataclass(frozen=True)
class RidgePrediction:
    predictions: np.ndarray
    model_sha256: str
    zero_std_feature_count: int
    solve_count: int


@dataclass(frozen=True)
class RidgeArm:
    selected_alpha: float
    validation_curve: tuple[tuple[float, float], ...]
    selected: RidgePrediction
    common_alpha: RidgePrediction
    selection_solve_count: int


def _bool(value: bool) -> str:
    return "true" if value else "false"


def _float(value: float) -> str:
    if not math.isfinite(value):
        raise EvaluationInputError("non_finite_report_value")
    if value == 0.0:
        value = 0.0
    return format(value, ".17g")


def _sha256(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def _read_regular_file(path: Path, role: str) -> bytes:
    try:
        if path.is_symlink():
            raise EvaluationInputError(f"{role}_is_symlink")
        if not path.is_file():
            raise EvaluationInputError(f"{role}_not_regular_file")
        return path.read_bytes()
    except EvaluationInputError:
        raise
    except FileNotFoundError as error:
        raise EvaluationInputError(f"{role}_not_found") from error
    except OSError as error:
        raise EvaluationInputError(f"{role}_unreadable") from error


def _parse_int(token: str, role: str) -> int:
    if token == "" or token.strip() != token:
        raise EvaluationInputError(f"{role}_invalid_integer")
    try:
        value = int(token, 10)
    except ValueError as error:
        raise EvaluationInputError(f"{role}_invalid_integer") from error
    if str(value) != token:
        raise EvaluationInputError(f"{role}_noncanonical_integer")
    return value


def _parse_float(token: str, role: str) -> float:
    if token == "" or token.strip() != token:
        raise EvaluationInputError(f"{role}_invalid_float")
    try:
        value = float(token)
    except ValueError as error:
        raise EvaluationInputError(f"{role}_invalid_float") from error
    if not math.isfinite(value):
        raise EvaluationInputError(f"{role}_non_finite")
    return value


def _parse_bool(token: str, role: str) -> bool:
    if token == "true":
        return True
    if token == "false":
        return False
    raise EvaluationInputError(f"{role}_invalid_boolean")


def _decode_csv(raw: bytes, role: str) -> list[list[str]]:
    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeDecodeError as error:
        raise EvaluationInputError(f"{role}_not_utf8") from error
    try:
        rows = list(csv.reader(io.StringIO(text, newline=""), strict=True))
    except csv.Error as error:
        raise EvaluationInputError(f"{role}_malformed_csv") from error
    if not rows:
        raise EvaluationInputError(f"{role}_empty")
    if any(not row for row in rows):
        raise EvaluationInputError(f"{role}_blank_record")
    return rows


def _identity(row: Sequence[str], ordinal: int, role: str) -> RowIdentity:
    anchor_key = _parse_int(row[1], f"{role}_row_{ordinal}_anchor_key")
    anchor_index = _parse_int(row[2], f"{role}_row_{ordinal}_anchor_index")
    anchor_local = _parse_int(
        row[3], f"{role}_row_{ordinal}_anchor_local_index"
    )
    edge = _parse_int(row[4], f"{role}_row_{ordinal}_edge_index")
    channel = _parse_int(row[8], f"{role}_row_{ordinal}_channel_index")
    if anchor_key < 0 or anchor_local < 0:
        raise EvaluationInputError(f"{role}_negative_identity")
    if edge not in GRAPH_IDENTITIES or channel not in range(CHANNEL_COUNT):
        raise EvaluationInputError(f"{role}_unexpected_edge_or_channel")
    graph = (row[5], row[6], row[7])
    if graph != GRAPH_IDENTITIES[edge]:
        raise EvaluationInputError(f"{role}_graph_identity_mismatch")
    return RowIdentity(
        anchor_key=anchor_key,
        anchor_index=anchor_index,
        anchor_local_index=anchor_local,
        edge_index=edge,
        edge_id=row[5],
        base_node_id=row[6],
        quote_node_id=row[7],
        channel_index=channel,
    )


def _validate_layout(
    identities: Sequence[RowIdentity],
    *,
    begin: int,
    end: int,
    role: str,
    max_batches: int | None,
) -> int:
    expected_rows = (end - begin) * ROWS_PER_ANCHOR
    if len(identities) != expected_rows:
        raise EvaluationInputError(f"{role}_row_count_mismatch")
    anchor_keys: set[int] = set()
    previous_local = -1
    batch_count = 0
    for ordinal, identity in enumerate(identities):
        anchor_offset, cell = divmod(ordinal, ROWS_PER_ANCHOR)
        expected_anchor = begin + anchor_offset
        expected_edge, expected_channel = divmod(cell, CHANNEL_COUNT)
        if (
            identity.anchor_index != expected_anchor
            or identity.edge_index != expected_edge
            or identity.channel_index != expected_channel
        ):
            raise EvaluationInputError(f"{role}_row_order_mismatch")
        first_row_for_anchor = cell == 0
        if not first_row_for_anchor:
            first = identities[ordinal - cell]
            if (
                identity.anchor_key != first.anchor_key
                or identity.anchor_local_index != first.anchor_local_index
            ):
                raise EvaluationInputError(f"{role}_anchor_identity_inconsistent")
            continue
        if identity.anchor_key in anchor_keys:
            raise EvaluationInputError(f"{role}_duplicate_anchor_key")
        anchor_keys.add(identity.anchor_key)
        local = identity.anchor_local_index
        if local >= MAX_BATCH_SIZE:
            raise EvaluationInputError(f"{role}_batch_size_exceeded")
        if anchor_offset == 0:
            if local != 0:
                raise EvaluationInputError(f"{role}_first_batch_not_local_zero")
            batch_count = 1
        elif local == 0:
            batch_count += 1
        elif local != previous_local + 1:
            raise EvaluationInputError(f"{role}_nonsequential_batch_local_index")
        previous_local = local
    if max_batches is not None and batch_count > max_batches:
        raise EvaluationInputError(f"{role}_encoder_batch_budget_exceeded")
    return batch_count


def _load_predictions(
    path: Path,
    *,
    role: str,
    begin: int,
    end: int,
    max_batches: int | None,
) -> PredictionSurface:
    raw = _read_regular_file(path, role)
    rows = _decode_csv(raw, role)
    header = tuple(rows[0])
    if header not in (PREDICTION_HEADER, PREDICTION_OPTIONAL_HEADER):
        raise EvaluationInputError(f"{role}_unexpected_header")
    has_sigma = header == PREDICTION_OPTIONAL_HEADER
    identities: list[RowIdentity] = []
    target_tokens: list[str] = []
    targets: list[float] = []
    predictions: list[float] = []
    valid: list[bool] = []
    sigma_finite: list[bool] = []
    for ordinal, row in enumerate(rows[1:], start=1):
        if len(row) != len(header):
            raise EvaluationInputError(f"{role}_row_{ordinal}_column_count")
        if row[0] != PREDICTION_RECORD_SCHEMA:
            raise EvaluationInputError(f"{role}_record_schema_mismatch")
        identities.append(_identity(row, ordinal, role))
        target_tokens.append(row[9])
        targets.append(_parse_float(row[9], f"{role}_row_{ordinal}_target"))
        predictions.append(
            _parse_float(row[10], f"{role}_row_{ordinal}_prediction")
        )
        valid.append(_parse_bool(row[11], f"{role}_row_{ordinal}_valid"))
        if has_sigma:
            sigma_finite.append(
                _parse_bool(row[12], f"{role}_row_{ordinal}_sigma_finite")
            )
    batch_count = _validate_layout(
        identities,
        begin=begin,
        end=end,
        role=role,
        max_batches=max_batches,
    )
    prediction_array = np.asarray(predictions, dtype=np.float64)
    valid_array = np.asarray(valid, dtype=np.bool_)
    if np.any(~valid_array & (prediction_array != 0.0)):
        raise EvaluationInputError(f"{role}_invalid_prediction_not_zero")
    sigma_array = (
        np.asarray(sigma_finite, dtype=np.bool_) if has_sigma else None
    )
    if sigma_array is not None and not bool(np.all(sigma_array)):
        raise EvaluationInputError(f"{role}_non_finite_sigma_reported")
    return PredictionSurface(
        path=path,
        size_bytes=len(raw),
        sha256=_sha256(raw),
        identities=tuple(identities),
        target_tokens=tuple(target_tokens),
        targets=np.asarray(targets, dtype=np.float64),
        predictions=prediction_array,
        valid=valid_array,
        sigma_finite=sigma_array,
        batch_count=batch_count,
    )


def _load_features(
    path: Path,
    *,
    role: str,
    begin: int,
    end: int,
    max_batches: int | None,
) -> FeatureSurface:
    raw = _read_regular_file(path, role)
    rows = _decode_csv(raw, role)
    if tuple(rows[0]) != FEATURE_HEADER:
        raise EvaluationInputError(f"{role}_unexpected_header")
    identities: list[RowIdentity] = []
    target_tokens: list[str] = []
    targets: list[float] = []
    features: list[list[float]] = []
    for ordinal, row in enumerate(rows[1:], start=1):
        if len(row) != len(FEATURE_HEADER):
            raise EvaluationInputError(f"{role}_row_{ordinal}_column_count")
        if row[0] != FEATURE_RECORD_SCHEMA:
            raise EvaluationInputError(f"{role}_record_schema_mismatch")
        identities.append(_identity(row, ordinal, role))
        target_tokens.append(row[9])
        targets.append(_parse_float(row[9], f"{role}_row_{ordinal}_target"))
        feature_count = _parse_int(
            row[10], f"{role}_row_{ordinal}_feature_count"
        )
        tokens = row[11].split(";") if row[11] else []
        if feature_count != FEATURE_COUNT or len(tokens) != FEATURE_COUNT:
            raise EvaluationInputError(f"{role}_feature_width_mismatch")
        features.append(
            [
                _parse_float(
                    token, f"{role}_row_{ordinal}_feature_{feature_index}"
                )
                for feature_index, token in enumerate(tokens)
            ]
        )
    batch_count = _validate_layout(
        identities,
        begin=begin,
        end=end,
        role=role,
        max_batches=max_batches,
    )
    feature_array = np.asarray(features, dtype=np.float64)
    if feature_array.shape != ((end - begin) * ROWS_PER_ANCHOR, FEATURE_COUNT):
        raise EvaluationInputError(f"{role}_feature_shape_mismatch")
    if not bool(np.all(np.isfinite(feature_array))):
        raise EvaluationInputError(f"{role}_non_finite_feature")
    return FeatureSurface(
        path=path,
        size_bytes=len(raw),
        sha256=_sha256(raw),
        identities=tuple(identities),
        target_tokens=tuple(target_tokens),
        targets=np.asarray(targets, dtype=np.float64),
        features=feature_array,
        batch_count=batch_count,
    )


def _require_paired(
    left_identities: Sequence[RowIdentity],
    right_identities: Sequence[RowIdentity],
    left_targets: Sequence[str],
    right_targets: Sequence[str],
    role: str,
) -> None:
    if tuple(left_identities) != tuple(right_identities):
        raise EvaluationInputError(f"{role}_identity_or_order_mismatch")
    if tuple(left_targets) != tuple(right_targets):
        raise EvaluationInputError(f"{role}_target_token_mismatch")


def _validate_stage_a_pair(
    baseline_prediction: PredictionSurface,
    candidate_prediction: PredictionSurface,
    baseline_feature: FeatureSurface,
    candidate_feature: FeatureSurface,
) -> None:
    _require_paired(
        baseline_prediction.identities,
        candidate_prediction.identities,
        baseline_prediction.target_tokens,
        candidate_prediction.target_tokens,
        "stage_a_prediction_pair",
    )
    _require_paired(
        baseline_feature.identities,
        candidate_feature.identities,
        baseline_feature.target_tokens,
        candidate_feature.target_tokens,
        "stage_a_feature_pair",
    )
    _require_paired(
        baseline_prediction.identities,
        baseline_feature.identities,
        baseline_prediction.target_tokens,
        baseline_feature.target_tokens,
        "stage_a_baseline_prediction_feature",
    )
    _require_paired(
        candidate_prediction.identities,
        candidate_feature.identities,
        candidate_prediction.target_tokens,
        candidate_feature.target_tokens,
        "stage_a_candidate_prediction_feature",
    )
    if not np.array_equal(baseline_prediction.valid, candidate_prediction.valid):
        raise EvaluationInputError("stage_a_valid_coverage_mismatch")
    if (baseline_prediction.sigma_finite is None) != (
        candidate_prediction.sigma_finite is None
    ):
        raise EvaluationInputError("stage_a_sigma_diagnostic_availability_mismatch")
    invalid = ~baseline_prediction.valid
    if bool(np.any(baseline_feature.features[invalid] != 0.0)):
        raise EvaluationInputError("stage_a_baseline_invalid_features_not_zero")
    if bool(np.any(candidate_feature.features[invalid] != 0.0)):
        raise EvaluationInputError("stage_a_candidate_invalid_features_not_zero")


def _reshape_rows(values: np.ndarray, anchors: int) -> np.ndarray:
    return np.asarray(values).reshape(anchors, EDGE_COUNT, CHANNEL_COUNT, *values.shape[1:])


def _safe_correlation(prediction: np.ndarray, target: np.ndarray) -> tuple[float, bool]:
    if prediction.size <= 1:
        return 0.0, False
    centered_prediction = prediction - float(np.mean(prediction))
    centered_target = target - float(np.mean(target))
    denominator = math.sqrt(
        float(np.dot(centered_prediction, centered_prediction))
        * float(np.dot(centered_target, centered_target))
    )
    if denominator <= 0.0:
        return 0.0, False
    value = float(np.dot(centered_prediction, centered_target)) / denominator
    return max(-1.0, min(1.0, value)), True


def _metrics(
    prediction: np.ndarray, target: np.ndarray, valid: np.ndarray
) -> Metrics:
    if prediction.shape != target.shape or prediction.shape != valid.shape:
        raise EvaluationInputError("metric_surface_shape_mismatch")
    if prediction.ndim != 3 or prediction.shape[1:] != (
        EDGE_COUNT,
        CHANNEL_COUNT,
    ):
        raise EvaluationInputError("metric_surface_contract_mismatch")
    finite = np.isfinite(prediction) & np.isfinite(target)
    if not bool(np.all(finite)):
        raise EvaluationInputError("metric_surface_non_finite")
    valid_count = int(np.count_nonzero(valid))
    if valid_count == 0:
        raise EvaluationInputError("metric_surface_has_no_valid_rows")
    p = prediction[valid]
    y = target[valid]
    direction = float(np.mean(np.sign(p) == np.sign(y)))
    rmse = math.sqrt(float(np.mean(np.square(p - y))))
    correlation, correlation_defined = _safe_correlation(p, y)

    complete = np.all(valid, axis=1)  # [anchor, channel]
    pair_correct = 0
    pair_count = 0
    for lhs in range(EDGE_COUNT - 1):
        for rhs in range(lhs + 1, EDGE_COUNT):
            mask = complete
            predicted_sign = np.sign(prediction[:, lhs, :] - prediction[:, rhs, :])
            target_sign = np.sign(target[:, lhs, :] - target[:, rhs, :])
            pair_correct += int(np.count_nonzero((predicted_sign == target_sign) & mask))
            pair_count += int(np.count_nonzero(mask))
    if pair_count == 0:
        raise EvaluationInputError("metric_surface_has_no_rank_groups")
    rank = pair_correct / pair_count

    prediction_best = np.argmax(prediction, axis=1)
    target_best = np.argmax(target, axis=1)
    best_count = int(np.count_nonzero(complete))
    if best_count == 0:
        raise EvaluationInputError("metric_surface_has_no_best_asset_groups")
    best_asset = float(np.count_nonzero((prediction_best == target_best) & complete)) / best_count
    return Metrics(
        valid_count=valid_count,
        valid_coverage=valid_count / prediction.size,
        direction=direction,
        rank_count=pair_count,
        rank=rank,
        rmse=rmse,
        correlation=correlation,
        correlation_defined=correlation_defined,
        best_asset_count=best_count,
        best_asset=best_asset,
        prediction_mean=float(np.mean(p)),
        prediction_std=float(np.std(p, ddof=0)),
        prediction_min=float(np.min(p)),
        prediction_max=float(np.max(p)),
    )


def _linear_quantile(values: np.ndarray, quantile: float) -> float:
    ordered = np.sort(np.asarray(values, dtype=np.float64))
    if ordered.size == 0:
        raise EvaluationInputError("empty_bootstrap_distribution")
    position = (ordered.size - 1) * quantile
    lower = int(math.floor(position))
    upper = int(math.ceil(position))
    if lower == upper:
        return float(ordered[lower])
    weight = position - lower
    return float(ordered[lower] * (1.0 - weight) + ordered[upper] * weight)


def _interval(point: float, samples: Sequence[float]) -> Interval:
    sample_array = np.asarray(samples, dtype=np.float64)
    if not bool(np.all(np.isfinite(sample_array))):
        raise EvaluationInputError("non_finite_bootstrap_distribution")
    return Interval(
        point=point,
        lower=_linear_quantile(sample_array, CI_LOWER_Q),
        upper=_linear_quantile(sample_array, CI_UPPER_Q),
    )


def _bootstrap_comparison(
    baseline_prediction: np.ndarray,
    candidate_prediction: np.ndarray,
    target: np.ndarray,
    valid: np.ndarray,
) -> tuple[Metrics, Metrics, Comparison]:
    baseline = _metrics(baseline_prediction, target, valid)
    candidate = _metrics(candidate_prediction, target, valid)
    if baseline.rmse <= 0.0:
        raise EvaluationInputError("baseline_rmse_not_positive")
    direction: list[float] = []
    rank: list[float] = []
    rmse_ratio: list[float] = []
    correlation: list[float] = []
    best_asset: list[float] = []
    rng = np.random.Generator(np.random.PCG64(BOOTSTRAP_SEED))
    anchor_count = baseline_prediction.shape[0]
    for _ in range(BOOTSTRAP_RESAMPLES):
        sample = rng.integers(0, anchor_count, size=anchor_count, dtype=np.int64)
        baseline_sample = _metrics(
            baseline_prediction[sample], target[sample], valid[sample]
        )
        candidate_sample = _metrics(
            candidate_prediction[sample], target[sample], valid[sample]
        )
        if baseline_sample.rmse <= 0.0:
            raise EvaluationInputError("bootstrap_baseline_rmse_not_positive")
        direction.append(candidate_sample.direction - baseline_sample.direction)
        rank.append(candidate_sample.rank - baseline_sample.rank)
        rmse_ratio.append(candidate_sample.rmse / baseline_sample.rmse)
        correlation.append(
            candidate_sample.correlation - baseline_sample.correlation
        )
        best_asset.append(candidate_sample.best_asset - baseline_sample.best_asset)
    comparison = Comparison(
        direction_delta=_interval(candidate.direction - baseline.direction, direction),
        rank_delta=_interval(candidate.rank - baseline.rank, rank),
        rmse_ratio=_interval(candidate.rmse / baseline.rmse, rmse_ratio),
        correlation_delta=_interval(
            candidate.correlation - baseline.correlation, correlation
        ),
        best_asset_delta=_interval(
            candidate.best_asset - baseline.best_asset, best_asset
        ),
    )
    return baseline, candidate, comparison


def _material_flags(comparison: Comparison) -> dict[str, bool]:
    return {
        "direction": (
            comparison.direction_delta.point >= MATERIAL_DIRECTION_POINT_GATE
            and comparison.direction_delta.lower > 0.0
        ),
        "rank": (
            comparison.rank_delta.point >= MATERIAL_RANK_POINT_GATE
            and comparison.rank_delta.lower > 0.0
        ),
        "rmse": (
            comparison.rmse_ratio.point <= MATERIAL_RMSE_RATIO_POINT_GATE
            and comparison.rmse_ratio.upper < 1.0
        ),
        "correlation": (
            comparison.correlation_delta.point
            >= MATERIAL_CORRELATION_POINT_GATE
            and comparison.correlation_delta.lower > 0.0
        ),
    }


def _classify_stage_a(
    comparison: Comparison, *, correlation_defined: bool = True
) -> tuple[bool, dict[str, bool], str]:
    compatibility = all(
        (
            comparison.direction_delta.lower >= STAGE_A_DIRECTION_LOWER_GATE,
            comparison.rank_delta.lower >= STAGE_A_RANK_LOWER_GATE,
            comparison.rmse_ratio.upper <= STAGE_A_RMSE_RATIO_UPPER_GATE,
        )
    )
    flags = _material_flags(comparison)
    if not correlation_defined:
        flags["correlation"] = False
    if not compatibility:
        classification = "frozen_head_incompatible"
    elif any(flags.values()):
        classification = "frozen_head_compatible_and_useful"
    else:
        classification = "compatible_no_downstream_gain"
    return compatibility, flags, classification


def _arm_diagnostics(
    prefix: str,
    prediction: PredictionSurface,
    feature: FeatureSurface,
) -> list[tuple[str, object]]:
    valid = prediction.valid
    valid_features = feature.features[valid]
    values: list[tuple[str, object]] = [
        (f"{prefix}.feature_mean", _float(float(np.mean(valid_features)))),
        (f"{prefix}.feature_std", _float(float(np.std(valid_features)))),
        (f"{prefix}.feature_l2_norm", _float(float(np.linalg.norm(valid_features)))),
        (f"{prefix}.valid_count", int(np.count_nonzero(valid))),
        (f"{prefix}.valid_coverage", _float(float(np.mean(valid)))),
        (f"{prefix}.prediction_mean", _float(float(np.mean(prediction.predictions[valid])))),
        (f"{prefix}.prediction_std", _float(float(np.std(prediction.predictions[valid])))),
        (f"{prefix}.prediction_min", _float(float(np.min(prediction.predictions[valid])))),
        (f"{prefix}.prediction_max", _float(float(np.max(prediction.predictions[valid])))),
        (f"{prefix}.sigma_finite.available", _bool(prediction.sigma_finite is not None)),
        (
            f"{prefix}.sigma_finite.all",
            _bool(
                prediction.sigma_finite is not None
                and bool(np.all(prediction.sigma_finite))
            ),
        ),
    ]
    shaped = _reshape_rows(
        feature.features,
        STAGE_A_END - STAGE_A_BEGIN,
    )
    for channel in range(CHANNEL_COUNT):
        values.append(
            (
                f"{prefix}.feature_channel_{channel}_variance",
                _float(float(np.var(shaped[:, :, channel, :]))),
            )
        )
    return values


def _paired_feature_diagnostics(
    baseline: FeatureSurface, candidate: FeatureSurface, valid: np.ndarray
) -> list[tuple[str, object]]:
    left = baseline.features[valid]
    right = candidate.features[valid]
    denominator = float(np.linalg.norm(left) * np.linalg.norm(right))
    global_cosine = float(np.sum(left * right)) / denominator if denominator > 0.0 else 0.0
    row_denominator = np.linalg.norm(left, axis=1) * np.linalg.norm(right, axis=1)
    row_cosines = np.divide(
        np.sum(left * right, axis=1),
        row_denominator,
        out=np.zeros_like(row_denominator),
        where=row_denominator > 0.0,
    )
    return [
        ("stage_a.paired_feature.global_cosine", _float(global_cosine)),
        (
            "stage_a.paired_feature.mean_row_cosine",
            _float(float(np.mean(row_cosines))),
        ),
        (
            "stage_a.paired_feature.minimum_row_cosine",
            _float(float(np.min(row_cosines))),
        ),
    ]


def _metric_fields(prefix: str, metric: Metrics) -> list[tuple[str, object]]:
    return [
        (f"{prefix}.valid_count", metric.valid_count),
        (f"{prefix}.valid_coverage", _float(metric.valid_coverage)),
        (f"{prefix}.directional_accuracy", _float(metric.direction)),
        (f"{prefix}.pairwise_rank_count", metric.rank_count),
        (f"{prefix}.pairwise_rank_accuracy", _float(metric.rank)),
        (f"{prefix}.rmse", _float(metric.rmse)),
        (f"{prefix}.correlation", _float(metric.correlation)),
        (f"{prefix}.correlation_defined", _bool(metric.correlation_defined)),
        (f"{prefix}.best_asset_count", metric.best_asset_count),
        (f"{prefix}.best_asset_agreement", _float(metric.best_asset)),
        (f"{prefix}.prediction_mean", _float(metric.prediction_mean)),
        (f"{prefix}.prediction_std", _float(metric.prediction_std)),
        (f"{prefix}.prediction_min", _float(metric.prediction_min)),
        (f"{prefix}.prediction_max", _float(metric.prediction_max)),
    ]


def _interval_fields(prefix: str, interval: Interval) -> list[tuple[str, object]]:
    return [
        (f"{prefix}.point", _float(interval.point)),
        (f"{prefix}.ci_lower", _float(interval.lower)),
        (f"{prefix}.ci_upper", _float(interval.upper)),
    ]


def _comparison_fields(prefix: str, comparison: Comparison) -> list[tuple[str, object]]:
    fields: list[tuple[str, object]] = []
    fields.extend(_interval_fields(f"{prefix}.direction_delta", comparison.direction_delta))
    fields.extend(_interval_fields(f"{prefix}.rank_delta", comparison.rank_delta))
    fields.extend(_interval_fields(f"{prefix}.rmse_ratio", comparison.rmse_ratio))
    fields.extend(
        _interval_fields(f"{prefix}.correlation_delta", comparison.correlation_delta)
    )
    fields.extend(
        _interval_fields(f"{prefix}.best_asset_delta", comparison.best_asset_delta)
    )
    return fields


def _reshape_feature_surface(surface: FeatureSurface, begin: int, end: int) -> np.ndarray:
    return surface.features.reshape(
        end - begin, EDGE_COUNT, CHANNEL_COUNT, FEATURE_COUNT
    )


def _reshape_target_surface(surface: FeatureSurface, begin: int, end: int) -> np.ndarray:
    return surface.targets.reshape(end - begin, EDGE_COUNT, CHANNEL_COUNT)


def _fit_ridge_predict(
    fit_features: np.ndarray,
    fit_targets: np.ndarray,
    prediction_features: np.ndarray,
    *,
    begin: int,
    end: int,
    alpha: float,
) -> RidgePrediction:
    if fit_features.dtype != np.float64 or prediction_features.dtype != np.float64:
        raise EvaluationInputError("ridge_feature_dtype_not_float64")
    prediction = np.empty(prediction_features.shape[:3], dtype=np.float64)
    digest = hashlib.sha256()
    zero_std_count = 0
    solve_count = 0
    for edge in range(EDGE_COUNT):
        x = fit_features[begin:end, edge, :, :].reshape(-1, FEATURE_COUNT)
        y = fit_targets[begin:end, edge, :].reshape(-1)
        mean = np.mean(x, axis=0, dtype=np.float64)
        std = np.std(x, axis=0, ddof=0, dtype=np.float64)
        zero = std <= 1.0e-12
        zero_std_count += int(np.count_nonzero(zero))
        scale = std.copy()
        scale[zero] = 1.0
        standardized = (x - mean) / scale
        design = np.empty((standardized.shape[0], FEATURE_COUNT + 1), dtype=np.float64)
        design[:, 0] = 1.0
        design[:, 1:] = standardized
        gram = design.T @ design
        rhs = design.T @ y
        gram[1:, 1:] += np.eye(FEATURE_COUNT, dtype=np.float64) * alpha
        try:
            coefficients = np.linalg.solve(gram, rhs)
        except np.linalg.LinAlgError as error:
            raise EvaluationInputError("ridge_linear_solve_failed") from error
        if not bool(np.all(np.isfinite(coefficients))):
            raise EvaluationInputError("ridge_non_finite_coefficients")
        x_predict = prediction_features[:, edge, :, :].reshape(-1, FEATURE_COUNT)
        predicted = coefficients[0] + ((x_predict - mean) / scale) @ coefficients[1:]
        prediction[:, edge, :] = predicted.reshape(
            prediction_features.shape[0], CHANNEL_COUNT
        )
        for tensor in (mean, scale, coefficients):
            digest.update(np.asarray(tensor, dtype="<f8").tobytes(order="C"))
        solve_count += 1
    if not bool(np.all(np.isfinite(prediction))):
        raise EvaluationInputError("ridge_non_finite_prediction")
    return RidgePrediction(
        predictions=prediction,
        model_sha256=digest.hexdigest(),
        zero_std_feature_count=zero_std_count,
        solve_count=solve_count,
    )


def _select_ridge_alpha(
    development: FeatureSurface,
) -> tuple[float, tuple[tuple[float, float], ...], int]:
    dev_features = _reshape_feature_surface(
        development, DEVELOPMENT_BEGIN, DEVELOPMENT_END
    )
    dev_targets = _reshape_target_surface(
        development, DEVELOPMENT_BEGIN, DEVELOPMENT_END
    )
    validation_target = dev_targets[PURGE_END:DEVELOPMENT_END]
    curve: list[tuple[float, float]] = []
    selection_solves = 0
    for alpha in RIDGE_ALPHAS:
        validation = _fit_ridge_predict(
            dev_features,
            dev_targets,
            dev_features[PURGE_END:DEVELOPMENT_END],
            begin=DEVELOPMENT_BEGIN,
            end=SELECTION_FIT_END,
            alpha=alpha,
        )
        selection_solves += validation.solve_count
        rmse = math.sqrt(
            float(np.mean(np.square(validation.predictions - validation_target)))
        )
        curve.append((alpha, rmse))
    # The grid is ascending.  min() therefore implements exact-RMSE ties by
    # choosing the lower alpha, as required by the sealed protocol.
    selected_alpha, _ = min(curve, key=lambda item: (item[1], item[0]))
    return selected_alpha, tuple(curve), selection_solves


def _refit_ridge_arm(
    development: FeatureSurface,
    evaluation: FeatureSurface,
    *,
    selected_alpha: float,
    validation_curve: tuple[tuple[float, float], ...],
    selection_solve_count: int,
    common_alpha: float,
) -> RidgeArm:
    dev_features = _reshape_feature_surface(
        development, DEVELOPMENT_BEGIN, DEVELOPMENT_END
    )
    dev_targets = _reshape_target_surface(
        development, DEVELOPMENT_BEGIN, DEVELOPMENT_END
    )
    eval_features = _reshape_feature_surface(evaluation, STAGE_A_BEGIN, STAGE_A_END)
    selected = _fit_ridge_predict(
        dev_features,
        dev_targets,
        eval_features,
        begin=DEVELOPMENT_BEGIN,
        end=DEVELOPMENT_END,
        alpha=selected_alpha,
    )
    common = _fit_ridge_predict(
        dev_features,
        dev_targets,
        eval_features,
        begin=DEVELOPMENT_BEGIN,
        end=DEVELOPMENT_END,
        alpha=common_alpha,
    )
    return RidgeArm(
        selected_alpha=selected_alpha,
        validation_curve=validation_curve,
        selected=selected,
        common_alpha=common,
        selection_solve_count=selection_solve_count,
    )


def _validate_development_pair(
    baseline: FeatureSurface, candidate: FeatureSurface
) -> None:
    _require_paired(
        baseline.identities,
        candidate.identities,
        baseline.target_tokens,
        candidate.target_tokens,
        "stage_b_development_pair",
    )


def _ridge_arm_fields(prefix: str, arm: RidgeArm) -> list[tuple[str, object]]:
    fields: list[tuple[str, object]] = [
        (f"{prefix}.selected_alpha", _float(arm.selected_alpha)),
        (f"{prefix}.selection_candidate_count", len(arm.validation_curve)),
        (f"{prefix}.selection_solve_count", arm.selection_solve_count),
        (f"{prefix}.selected_refit_solve_count", arm.selected.solve_count),
        (f"{prefix}.common_alpha_refit_solve_count", arm.common_alpha.solve_count),
        (f"{prefix}.selected_model_sha256", arm.selected.model_sha256),
        (f"{prefix}.common_alpha_model_sha256", arm.common_alpha.model_sha256),
        (
            f"{prefix}.selected_zero_std_feature_count",
            arm.selected.zero_std_feature_count,
        ),
        (
            f"{prefix}.common_alpha_zero_std_feature_count",
            arm.common_alpha.zero_std_feature_count,
        ),
    ]
    for index, (alpha, rmse) in enumerate(arm.validation_curve):
        fields.extend(
            (
                (f"{prefix}.selection.alpha_{index}.alpha", _float(alpha)),
                (f"{prefix}.selection.alpha_{index}.validation_rmse", _float(rmse)),
            )
        )
    return fields


def _evaluate_stage_b(
    baseline_development: FeatureSurface,
    candidate_development: FeatureSurface,
    baseline_evaluation: FeatureSurface,
    candidate_evaluation: FeatureSurface,
) -> list[tuple[str, object]]:
    _validate_development_pair(baseline_development, candidate_development)
    # Select each arm exactly once over the same fixed 12-alpha grid, then give
    # each arm one selected-alpha refit and one baseline-common-alpha refit.
    baseline_alpha, baseline_curve, baseline_selection_solves = (
        _select_ridge_alpha(baseline_development)
    )
    candidate_alpha, candidate_curve, candidate_selection_solves = (
        _select_ridge_alpha(candidate_development)
    )
    baseline_arm = _refit_ridge_arm(
        baseline_development,
        baseline_evaluation,
        selected_alpha=baseline_alpha,
        validation_curve=baseline_curve,
        selection_solve_count=baseline_selection_solves,
        common_alpha=baseline_alpha,
    )
    candidate_arm = _refit_ridge_arm(
        candidate_development,
        candidate_evaluation,
        selected_alpha=candidate_alpha,
        validation_curve=candidate_curve,
        selection_solve_count=candidate_selection_solves,
        common_alpha=baseline_alpha,
    )
    if baseline_arm.selection_solve_count != candidate_arm.selection_solve_count:
        raise EvaluationInputError("stage_b_unequal_selection_compute")
    if (
        baseline_arm.selected.solve_count != candidate_arm.selected.solve_count
        or baseline_arm.common_alpha.solve_count
        != candidate_arm.common_alpha.solve_count
    ):
        raise EvaluationInputError("stage_b_unequal_refit_compute")

    target = _reshape_target_surface(
        baseline_evaluation, STAGE_A_BEGIN, STAGE_A_END
    )
    valid = np.ones_like(target, dtype=np.bool_)
    baseline_metric, candidate_metric, comparison = _bootstrap_comparison(
        baseline_arm.selected.predictions,
        candidate_arm.selected.predictions,
        target,
        valid,
    )
    common_baseline_metric, common_candidate_metric, common_comparison = (
        _bootstrap_comparison(
            baseline_arm.common_alpha.predictions,
            candidate_arm.common_alpha.predictions,
            target,
            valid,
        )
    )
    flags = _material_flags(comparison)
    material_count = sum(int(flags[name]) for name in ("direction", "rank", "rmse"))
    direction_gate = comparison.direction_delta.lower >= STAGE_B_DIRECTION_LOWER_GATE
    rank_gate = comparison.rank_delta.lower >= STAGE_B_RANK_LOWER_GATE
    rmse_gate = comparison.rmse_ratio.upper <= STAGE_B_RMSE_RATIO_UPPER_GATE
    material_gate = material_count >= 2
    passed = direction_gate and rank_gate and rmse_gate and material_gate
    decision = (
        "activation_requires_versioned_head_checkpoint_migration"
        if passed
        else "downstream_bottleneck_unresolved"
    )

    fields: list[tuple[str, object]] = [
        ("stage_b.status", "completed"),
        ("stage_b.mechanics.pass", "true"),
        ("stage_b.execution.device", "cpu"),
        ("stage_b.execution.dtype", "float64"),
        ("stage_b.execution.optimizer_steps", 0),
        ("stage_b.execution.rng_used_for_fit", "false"),
        ("stage_b.alpha_grid", ",".join(_float(alpha) for alpha in RIDGE_ALPHAS)),
        ("stage_b.alpha_selection_metric", "lowest_validation_rmse"),
        ("stage_b.alpha_tie_rule", "lower_alpha"),
        ("stage_b.standardization", "per_arm_per_edge_fit_only_population_std"),
        ("stage_b.intercept_penalized", "false"),
        ("stage_b.selection_fit_anchor_range", "[0,554)"),
        ("stage_b.purge_anchor_range", "[554,584)"),
        ("stage_b.validation_anchor_range", "[584,730)"),
        ("stage_b.refit_anchor_range", "[0,730)"),
        ("stage_b.confirmation_anchor_range", "[760,1088)"),
        ("stage_b.common_alpha", _float(baseline_alpha)),
    ]
    fields.extend(_ridge_arm_fields("stage_b.arm.baseline", baseline_arm))
    fields.extend(_ridge_arm_fields("stage_b.arm.candidate", candidate_arm))
    fields.extend(_metric_fields("stage_b.selected.arm.baseline", baseline_metric))
    fields.extend(_metric_fields("stage_b.selected.arm.candidate", candidate_metric))
    fields.extend(_comparison_fields("stage_b.selected.comparison", comparison))
    fields.extend(
        _metric_fields("stage_b.common_alpha.arm.baseline", common_baseline_metric)
    )
    fields.extend(
        _metric_fields("stage_b.common_alpha.arm.candidate", common_candidate_metric)
    )
    fields.extend(
        _comparison_fields("stage_b.common_alpha.comparison", common_comparison)
    )
    fields.extend(
        [
            (
                "stage_b.common_alpha.baseline_prediction_exact_selected",
                _bool(
                    np.array_equal(
                        baseline_arm.selected.predictions,
                        baseline_arm.common_alpha.predictions,
                    )
                ),
            ),
            ("stage_b.gate.direction_lower.minimum", _float(STAGE_B_DIRECTION_LOWER_GATE)),
            ("stage_b.gate.direction_lower.pass", _bool(direction_gate)),
            ("stage_b.gate.rank_lower.minimum", _float(STAGE_B_RANK_LOWER_GATE)),
            ("stage_b.gate.rank_lower.pass", _bool(rank_gate)),
            ("stage_b.gate.rmse_ratio_upper.maximum", _float(STAGE_B_RMSE_RATIO_UPPER_GATE)),
            ("stage_b.gate.rmse_ratio_upper.pass", _bool(rmse_gate)),
            ("stage_b.material.direction", _bool(flags["direction"])),
            ("stage_b.material.rank", _bool(flags["rank"])),
            ("stage_b.material.rmse", _bool(flags["rmse"])),
            ("stage_b.material.required_count", 2),
            ("stage_b.material.actual_count", material_count),
            ("stage_b.material.pass", _bool(material_gate)),
            ("stage_b.pass", _bool(passed)),
            ("final.decision", decision),
        ]
    )
    return fields


def _input_fields(prefix: str, surface: PredictionSurface | FeatureSurface) -> list[tuple[str, object]]:
    return [
        (f"{prefix}.path", surface.path),
        (f"{prefix}.size_bytes", surface.size_bytes),
        (f"{prefix}.sha256", surface.sha256),
        (f"{prefix}.row_count", len(surface.identities)),
        (f"{prefix}.batch_count", surface.batch_count),
    ]


def _evaluate(args: argparse.Namespace) -> list[tuple[str, object]]:
    baseline_prediction = _load_predictions(
        args.baseline_predictions,
        role="baseline_predictions",
        begin=STAGE_A_BEGIN,
        end=STAGE_A_END,
        max_batches=MAX_STAGE_A_ENCODER_BATCHES,
    )
    candidate_prediction = _load_predictions(
        args.candidate_predictions,
        role="candidate_predictions",
        begin=STAGE_A_BEGIN,
        end=STAGE_A_END,
        max_batches=MAX_STAGE_A_ENCODER_BATCHES,
    )
    baseline_feature = _load_features(
        args.baseline_eval_probe,
        role="baseline_eval_probe",
        begin=STAGE_A_BEGIN,
        end=STAGE_A_END,
        max_batches=MAX_STAGE_A_ENCODER_BATCHES,
    )
    candidate_feature = _load_features(
        args.candidate_eval_probe,
        role="candidate_eval_probe",
        begin=STAGE_A_BEGIN,
        end=STAGE_A_END,
        max_batches=MAX_STAGE_A_ENCODER_BATCHES,
    )
    _validate_stage_a_pair(
        baseline_prediction,
        candidate_prediction,
        baseline_feature,
        candidate_feature,
    )

    anchor_count = STAGE_A_END - STAGE_A_BEGIN
    target = baseline_prediction.targets.reshape(
        anchor_count, EDGE_COUNT, CHANNEL_COUNT
    )
    valid = baseline_prediction.valid.reshape(
        anchor_count, EDGE_COUNT, CHANNEL_COUNT
    )
    baseline_values = baseline_prediction.predictions.reshape(
        anchor_count, EDGE_COUNT, CHANNEL_COUNT
    )
    candidate_values = candidate_prediction.predictions.reshape(
        anchor_count, EDGE_COUNT, CHANNEL_COUNT
    )
    baseline_metric, candidate_metric, comparison = _bootstrap_comparison(
        baseline_values, candidate_values, target, valid
    )
    compatibility, flags, classification = _classify_stage_a(
        comparison,
        correlation_defined=(
            baseline_metric.correlation_defined
            and candidate_metric.correlation_defined
        ),
    )

    fields: list[tuple[str, object]] = [
        ("schema", SCHEMA),
        ("protocol_sha256", PROTOCOL_SHA256),
        ("protocol_size_bytes", PROTOCOL_SIZE_BYTES),
        ("status", "completed"),
        ("baseline_policy", BASELINE_POLICY),
        ("candidate_policy", CANDIDATE_POLICY),
        ("mechanics.pass", "true"),
        ("mechanics.endpoint_metrics_inspected", "true"),
        ("stage_a.anchor_range", "[760,1088)"),
        ("stage_a.anchor_count", anchor_count),
        ("stage_a.expected_row_count", STAGE_A_ROW_COUNT),
        ("stage_a.bootstrap.resamples", BOOTSTRAP_RESAMPLES),
        ("stage_a.bootstrap.seed", BOOTSTRAP_SEED),
        ("stage_a.bootstrap.cluster_unit", "anchor"),
        ("stage_a.bootstrap.rng", BOOTSTRAP_RNG),
        ("stage_a.bootstrap.quantile_method", "linear_order_statistic_interpolation"),
    ]
    fields.extend(_input_fields("input.baseline_predictions", baseline_prediction))
    fields.extend(_input_fields("input.candidate_predictions", candidate_prediction))
    fields.extend(_input_fields("input.baseline_eval_probe", baseline_feature))
    fields.extend(_input_fields("input.candidate_eval_probe", candidate_feature))
    fields.extend(
        _arm_diagnostics("stage_a.diagnostic.arm.baseline", baseline_prediction, baseline_feature)
    )
    fields.extend(
        _arm_diagnostics("stage_a.diagnostic.arm.candidate", candidate_prediction, candidate_feature)
    )
    fields.extend(
        _paired_feature_diagnostics(
            baseline_feature, candidate_feature, baseline_prediction.valid
        )
    )
    fields.extend(_metric_fields("stage_a.arm.baseline", baseline_metric))
    fields.extend(_metric_fields("stage_a.arm.candidate", candidate_metric))
    fields.extend(_comparison_fields("stage_a.comparison", comparison))
    fields.append(
        (
            "stage_a.comparison.valid_coverage_delta",
            _float(candidate_metric.valid_coverage - baseline_metric.valid_coverage),
        )
    )
    direction_gate = comparison.direction_delta.lower >= STAGE_A_DIRECTION_LOWER_GATE
    rank_gate = comparison.rank_delta.lower >= STAGE_A_RANK_LOWER_GATE
    rmse_gate = comparison.rmse_ratio.upper <= STAGE_A_RMSE_RATIO_UPPER_GATE
    fields.extend(
        [
            ("stage_a.gate.direction_lower.minimum", _float(STAGE_A_DIRECTION_LOWER_GATE)),
            ("stage_a.gate.direction_lower.pass", _bool(direction_gate)),
            ("stage_a.gate.rank_lower.minimum", _float(STAGE_A_RANK_LOWER_GATE)),
            ("stage_a.gate.rank_lower.pass", _bool(rank_gate)),
            ("stage_a.gate.rmse_ratio_upper.maximum", _float(STAGE_A_RMSE_RATIO_UPPER_GATE)),
            ("stage_a.gate.rmse_ratio_upper.pass", _bool(rmse_gate)),
            ("stage_a.compatibility.pass", _bool(compatibility)),
            ("stage_a.material.direction", _bool(flags["direction"])),
            ("stage_a.material.rank", _bool(flags["rank"])),
            ("stage_a.material.rmse", _bool(flags["rmse"])),
            ("stage_a.material.correlation", _bool(flags["correlation"])),
            ("stage_a.material.count", sum(int(value) for value in flags.values())),
            ("stage_a.classification", classification),
        ]
    )

    development_supplied = (
        args.baseline_dev_probe is not None and args.candidate_dev_probe is not None
    )
    if classification != "frozen_head_incompatible":
        fields.extend(
            [
                ("stage_b.status", "not_authorized"),
                ("stage_b.reason", "stage_a_did_not_classify_frozen_head_incompatible"),
                ("stage_b.development_paths_ignored", _bool(development_supplied)),
            ]
        )
        return fields
    if not development_supplied:
        fields.extend(
            [
                ("stage_b.status", "authorized_not_run"),
                ("stage_b.reason", "development_probes_not_supplied"),
            ]
        )
        return fields

    try:
        baseline_development = _load_features(
            args.baseline_dev_probe,
            role="baseline_dev_probe",
            begin=DEVELOPMENT_BEGIN,
            end=DEVELOPMENT_END,
            max_batches=None,
        )
        candidate_development = _load_features(
            args.candidate_dev_probe,
            role="candidate_dev_probe",
            begin=DEVELOPMENT_BEGIN,
            end=DEVELOPMENT_END,
            max_batches=None,
        )
        fields.extend(_input_fields("input.baseline_dev_probe", baseline_development))
        fields.extend(_input_fields("input.candidate_dev_probe", candidate_development))
        fields.extend(
            _evaluate_stage_b(
                baseline_development,
                candidate_development,
                baseline_feature,
                candidate_feature,
            )
        )
    except EvaluationInputError as error:
        fields.extend(
            [
                ("stage_b.status", "invalid"),
                ("stage_b.mechanics.pass", "false"),
                ("stage_b.mechanics.error", error.code),
                ("stage_b.endpoint_metrics_inspected", "false"),
                ("final.decision", "downstream_bottleneck_unresolved"),
            ]
        )
    return fields


def _synthetic_comparison(
    direction: Interval,
    rank: Interval,
    rmse: Interval,
    correlation: Interval,
) -> Comparison:
    return Comparison(
        direction_delta=direction,
        rank_delta=rank,
        rmse_ratio=rmse,
        correlation_delta=correlation,
        best_asset_delta=Interval(0.0, -0.01, 0.01),
    )


def _self_tests() -> list[tuple[str, object]]:
    tests: list[tuple[str, bool]] = []
    tests.append(
        (
            "linear_quantile",
            _linear_quantile(np.asarray([0.0, 1.0, 2.0, 3.0]), 0.5) == 1.5,
        )
    )
    synthetic_identities: list[RowIdentity] = []
    for anchor in range(2):
        for edge in range(EDGE_COUNT):
            edge_id, base_id, quote_id = GRAPH_IDENTITIES[edge]
            for channel in range(CHANNEL_COUNT):
                synthetic_identities.append(
                    RowIdentity(
                        anchor_key=10_000 + anchor,
                        anchor_index=anchor,
                        anchor_local_index=anchor,
                        edge_index=edge,
                        edge_id=edge_id,
                        base_node_id=base_id,
                        quote_node_id=quote_id,
                        channel_index=channel,
                    )
                )
    tests.append(
        (
            "paired_layout_accepts_exact_grid",
            _validate_layout(
                synthetic_identities,
                begin=0,
                end=2,
                role="self_test_layout",
                max_batches=1,
            )
            == 1,
        )
    )
    corrupted = synthetic_identities.copy()
    corrupted[1], corrupted[2] = corrupted[2], corrupted[1]
    rejected = False
    try:
        _validate_layout(
            corrupted,
            begin=0,
            end=2,
            role="self_test_corrupted_layout",
            max_batches=1,
        )
    except EvaluationInputError:
        rejected = True
    tests.append(("paired_layout_rejects_reordering", rejected))
    anchors = 12
    target = np.empty((anchors, EDGE_COUNT, CHANNEL_COUNT), dtype=np.float64)
    for anchor in range(anchors):
        for edge in range(EDGE_COUNT):
            for channel in range(CHANNEL_COUNT):
                target[anchor, edge, channel] = (
                    0.1 * (edge - 1) + 0.01 * channel + 0.001 * (anchor + 1)
                )
    valid = np.ones_like(target, dtype=np.bool_)
    perfect = _metrics(target.copy(), target, valid)
    tests.append(
        (
            "perfect_metrics",
            perfect.direction == 1.0
            and perfect.rank == 1.0
            and perfect.rmse == 0.0
            and abs(perfect.correlation - 1.0) <= 1.0e-15
            and perfect.best_asset == 1.0,
        )
    )
    useful = _synthetic_comparison(
        Interval(0.03, 0.01, 0.05),
        Interval(0.0, -0.01, 0.01),
        Interval(1.0, 0.98, 1.02),
        Interval(0.0, -0.01, 0.01),
    )
    tests.append(
        (
            "useful_classification",
            _classify_stage_a(useful)[2] == "frozen_head_compatible_and_useful",
        )
    )
    incompatible = _synthetic_comparison(
        Interval(-0.03, -0.04, -0.02),
        Interval(0.0, -0.01, 0.01),
        Interval(1.0, 0.98, 1.02),
        Interval(0.0, -0.01, 0.01),
    )
    tests.append(
        (
            "incompatible_classification",
            _classify_stage_a(incompatible)[2] == "frozen_head_incompatible",
        )
    )
    rng = np.random.Generator(np.random.PCG64(17))
    ridge_features = rng.normal(size=(24, EDGE_COUNT, CHANNEL_COUNT, 4)).astype(
        np.float64
    )
    weights = np.asarray([0.2, -0.4, 0.7, 0.1], dtype=np.float64)
    ridge_target = np.empty((24, EDGE_COUNT, CHANNEL_COUNT), dtype=np.float64)
    for edge in range(EDGE_COUNT):
        ridge_target[:, edge, :] = (
            ridge_features[:, edge, :, :] @ weights + 0.05 * edge
        )
    # Pad to the production width without changing the known linear surface.
    padded = np.zeros((24, EDGE_COUNT, CHANNEL_COUNT, FEATURE_COUNT), dtype=np.float64)
    padded[..., :4] = ridge_features
    ridge = _fit_ridge_predict(
        padded,
        ridge_target,
        padded,
        begin=0,
        end=24,
        alpha=1.0e-10,
    )
    ridge_rmse = math.sqrt(float(np.mean(np.square(ridge.predictions - ridge_target))))
    tests.append(("ridge_recovery", ridge_rmse < 1.0e-8 and ridge.solve_count == 3))
    passed = all(value for _, value in tests)
    fields: list[tuple[str, object]] = [
        ("schema", SELF_TEST_SCHEMA),
        ("protocol_sha256", PROTOCOL_SHA256),
        ("self_test.synthetic_only", "true"),
        ("self_test.endpoint_artifacts_opened", "false"),
        ("self_test.count", len(tests)),
    ]
    for name, value in tests:
        fields.append((f"self_test.{name}.pass", _bool(value)))
    fields.append(("self_test.pass", _bool(passed)))
    return fields


def _render(fields: Iterable[tuple[str, object]]) -> str:
    seen: set[str] = set()
    lines: list[str] = []
    for key, value in fields:
        if key in seen:
            raise EvaluationInputError("duplicate_report_key")
        if "=" in key or "\n" in key or "\r" in key:
            raise EvaluationInputError("invalid_report_key")
        text = str(value)
        if "\n" in text or "\r" in text:
            raise EvaluationInputError("invalid_report_value")
        seen.add(key)
        lines.append(f"{key}={text}")
    return "\n".join(lines) + "\n"


def _invalid_fields(code: str) -> list[tuple[str, object]]:
    return [
        ("schema", SCHEMA),
        ("protocol_sha256", PROTOCOL_SHA256),
        ("protocol_size_bytes", PROTOCOL_SIZE_BYTES),
        ("status", "invalid"),
        ("mechanics.pass", "false"),
        ("mechanics.error", code),
        ("mechanics.endpoint_metrics_inspected", "false"),
        ("stage_a.compatibility.pass", "false"),
        ("stage_a.classification", "invalid"),
    ]


def _parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-predictions", type=Path)
    parser.add_argument("--candidate-predictions", type=Path)
    parser.add_argument("--baseline-eval-probe", type=Path)
    parser.add_argument("--candidate-eval-probe", type=Path)
    parser.add_argument("--baseline-dev-probe", type=Path)
    parser.add_argument("--candidate-dev-probe", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        supplied = any(
            value is not None
            for value in (
                args.baseline_predictions,
                args.candidate_predictions,
                args.baseline_eval_probe,
                args.candidate_eval_probe,
                args.baseline_dev_probe,
                args.candidate_dev_probe,
            )
        )
        if supplied:
            parser.error("--self-test cannot be combined with endpoint inputs")
        return args
    required = (
        "baseline_predictions",
        "candidate_predictions",
        "baseline_eval_probe",
        "candidate_eval_probe",
    )
    missing = [name for name in required if getattr(args, name) is None]
    if missing:
        parser.error("missing required arguments: " + ", ".join(missing))
    if (args.baseline_dev_probe is None) != (args.candidate_dev_probe is None):
        parser.error("development probes must be supplied as a baseline/candidate pair")
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
