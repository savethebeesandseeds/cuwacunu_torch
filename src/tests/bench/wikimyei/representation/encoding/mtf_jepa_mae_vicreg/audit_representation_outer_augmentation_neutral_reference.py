#!/usr/bin/env python3
"""Read-only audit of the frozen neutral JEPA/MAE reference namespace."""

from __future__ import annotations

import argparse
import hashlib
import re
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence


AUDIT_SCHEMA = (
    "wikimyei.mtf_jepa_mae_vicreg."
    "outer_augmentation_neutral_reference_audit.v1"
)
SELF_TEST_SCHEMA = AUDIT_SCHEMA + ".self_test"
PROTOCOL_SHA256 = (
    "35d0e1e508ce8bf982942d0256fd3db553d3317e98bce60431a4e318a8284e99"
)

REFERENCE_SIZE_BYTES = 753_029
REFERENCE_SHA256 = (
    "bd382eb9d638bfc9ce42257eaca0ab5cb2cd4f44a96f2c4c946eb27e9e7dd038"
)
REFERENCE_SCHEMA = (
    "wikimyei.mtf_jepa_mae_vicreg.vicreg_variance_necessity.v1"
)
SELECTOR_TEXT = (
    r"^(seed_(17|31|47)\.arm\.jepa_mae_only\.|"
    r"summary\.arm\.jepa_mae_only\.)"
)
SELECTOR = re.compile(SELECTOR_TEXT)
EXPECTED_SELECTED_KEY_COUNT = 2_046
EXPECTED_KEY_SET_SHA256 = (
    "7998b81e3aa42e585c75a6ddcc9a3e00e2bc09819d8595add52570ea8b168864"
)
EXPECTED_KEY_VALUE_SHA256 = (
    "f3696f996bccd1dd7485a7959fdb5415817e1f7bce2357dbbbf9e2e9654ff5fc"
)


class AuditInputError(RuntimeError):
    """An input could not be inspected without changing it."""

    def __init__(self, code: str) -> None:
        super().__init__(code)
        self.code = code


@dataclass(frozen=True)
class ExpectedContract:
    size_bytes: int
    file_sha256: str
    schema: str
    selected_key_count: int
    key_set_sha256: str
    key_value_sha256: str


@dataclass(frozen=True)
class LogSummary:
    size_bytes: int
    file_sha256: str
    schema_values: tuple[str, ...]
    selected_records: tuple[tuple[str, str], ...]
    selected_keys: frozenset[str]
    selected_line_count: int
    selected_unique_key_count: int
    selected_duplicate_key_count: int
    selected_duplicate_line_count: int
    selected_key_set_sha256: str
    selected_key_value_sha256: str


@dataclass(frozen=True)
class AuditResult:
    reference: LogSummary
    candidate: LogSummary
    reference_contract_pass: bool
    candidate_contract_pass: bool
    candidate_key_set_exact: bool
    candidate_missing_key_count: int
    candidate_extra_key_count: int
    passed: bool


FROZEN_CONTRACT = ExpectedContract(
    size_bytes=REFERENCE_SIZE_BYTES,
    file_sha256=REFERENCE_SHA256,
    schema=REFERENCE_SCHEMA,
    selected_key_count=EXPECTED_SELECTED_KEY_COUNT,
    key_set_sha256=EXPECTED_KEY_SET_SHA256,
    key_value_sha256=EXPECTED_KEY_VALUE_SHA256,
)


def _sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _canonical_key_bytes(keys: Iterable[str]) -> bytes:
    ordered = sorted(set(keys), key=lambda key: key.encode("utf-8"))
    return b"".join(key.encode("utf-8") + b"\n" for key in ordered)


def _canonical_record_bytes(records: Iterable[tuple[str, str]]) -> bytes:
    ordered = sorted(records, key=lambda item: item[0].encode("utf-8"))
    return b"".join(
        key.encode("utf-8") + b"=" + value.encode("utf-8") + b"\n"
        for key, value in ordered
    )


def _summarize(raw: bytes, selector: re.Pattern[str]) -> LogSummary:
    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeDecodeError as error:
        raise AuditInputError("input_not_utf8") from error

    all_records: list[tuple[str, str]] = []
    selected_records: list[tuple[str, str]] = []
    for line in text.splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        all_records.append((key, value))
        if selector.match(key):
            selected_records.append((key, value))

    selected_key_counts = Counter(key for key, _ in selected_records)
    duplicate_counts = [count for count in selected_key_counts.values() if count > 1]
    selected_keys = frozenset(selected_key_counts)
    schema_values = tuple(value for key, value in all_records if key == "schema")
    return LogSummary(
        size_bytes=len(raw),
        file_sha256=_sha256_bytes(raw),
        schema_values=schema_values,
        selected_records=tuple(selected_records),
        selected_keys=selected_keys,
        selected_line_count=len(selected_records),
        selected_unique_key_count=len(selected_keys),
        selected_duplicate_key_count=len(duplicate_counts),
        selected_duplicate_line_count=sum(count - 1 for count in duplicate_counts),
        selected_key_set_sha256=_sha256_bytes(_canonical_key_bytes(selected_keys)),
        selected_key_value_sha256=_sha256_bytes(
            _canonical_record_bytes(selected_records)
        ),
    )


def _reference_contract_pass(
    summary: LogSummary, expected: ExpectedContract
) -> bool:
    return all(
        (
            summary.size_bytes == expected.size_bytes,
            summary.file_sha256 == expected.file_sha256,
            summary.schema_values == (expected.schema,),
            summary.selected_line_count == expected.selected_key_count,
            summary.selected_unique_key_count == expected.selected_key_count,
            summary.selected_duplicate_key_count == 0,
            summary.selected_duplicate_line_count == 0,
            summary.selected_key_set_sha256 == expected.key_set_sha256,
            summary.selected_key_value_sha256 == expected.key_value_sha256,
        )
    )


def _audit_bytes(
    reference_raw: bytes,
    candidate_raw: bytes,
    *,
    selector: re.Pattern[str],
    expected: ExpectedContract,
) -> AuditResult:
    reference = _summarize(reference_raw, selector)
    candidate = _summarize(candidate_raw, selector)
    reference_pass = _reference_contract_pass(reference, expected)

    missing_keys = reference.selected_keys - candidate.selected_keys
    extra_keys = candidate.selected_keys - reference.selected_keys
    candidate_key_set_exact = not missing_keys and not extra_keys
    candidate_pass = all(
        (
            candidate.selected_line_count == expected.selected_key_count,
            candidate.selected_unique_key_count == expected.selected_key_count,
            candidate.selected_duplicate_key_count == 0,
            candidate.selected_duplicate_line_count == 0,
            candidate_key_set_exact,
            candidate.selected_key_set_sha256 == expected.key_set_sha256,
            candidate.selected_key_value_sha256 == expected.key_value_sha256,
        )
    )
    return AuditResult(
        reference=reference,
        candidate=candidate,
        reference_contract_pass=reference_pass,
        candidate_contract_pass=candidate_pass,
        candidate_key_set_exact=candidate_key_set_exact,
        candidate_missing_key_count=len(missing_keys),
        candidate_extra_key_count=len(extra_keys),
        passed=reference_pass and candidate_pass,
    )


def _read_only(path: Path, role: str) -> bytes:
    try:
        return path.read_bytes()
    except FileNotFoundError as error:
        raise AuditInputError(f"{role}_not_found") from error
    except IsADirectoryError as error:
        raise AuditInputError(f"{role}_is_directory") from error
    except OSError as error:
        raise AuditInputError(f"{role}_unreadable") from error


def _bool(value: bool) -> str:
    return "true" if value else "false"


def _emit(fields: Sequence[tuple[str, object]]) -> None:
    for key, value in fields:
        print(f"{key}={value}")


def _normal_fields(
    reference_path: Path, candidate_path: Path, result: AuditResult
) -> list[tuple[str, object]]:
    reference = result.reference
    candidate = result.candidate
    invalidity_override = not result.passed
    return [
        ("schema", AUDIT_SCHEMA),
        ("protocol_sha256", PROTOCOL_SHA256),
        ("selector_regex", SELECTOR_TEXT),
        ("expected.reference_size_bytes", REFERENCE_SIZE_BYTES),
        ("expected.reference_sha256", REFERENCE_SHA256),
        ("expected.reference_schema", REFERENCE_SCHEMA),
        ("expected.selected_key_count", EXPECTED_SELECTED_KEY_COUNT),
        ("expected.selected_key_set_sha256", EXPECTED_KEY_SET_SHA256),
        ("expected.selected_key_value_sha256", EXPECTED_KEY_VALUE_SHA256),
        ("reference.path", reference_path),
        ("reference.size_bytes", reference.size_bytes),
        ("reference.sha256", reference.file_sha256),
        ("reference.size_exact", _bool(reference.size_bytes == REFERENCE_SIZE_BYTES)),
        ("reference.sha256_exact", _bool(reference.file_sha256 == REFERENCE_SHA256)),
        ("reference.schema_count", len(reference.schema_values)),
        (
            "reference.schema_exact",
            _bool(reference.schema_values == (REFERENCE_SCHEMA,)),
        ),
        ("reference.selected_line_count", reference.selected_line_count),
        (
            "reference.selected_unique_key_count",
            reference.selected_unique_key_count,
        ),
        (
            "reference.selected_duplicate_key_count",
            reference.selected_duplicate_key_count,
        ),
        (
            "reference.selected_duplicate_line_count",
            reference.selected_duplicate_line_count,
        ),
        (
            "reference.selected_count_exact",
            _bool(
                reference.selected_line_count == EXPECTED_SELECTED_KEY_COUNT
                and reference.selected_unique_key_count
                == EXPECTED_SELECTED_KEY_COUNT
            ),
        ),
        (
            "reference.selected_no_duplicates",
            _bool(
                reference.selected_duplicate_key_count == 0
                and reference.selected_duplicate_line_count == 0
            ),
        ),
        ("reference.selected_key_set_sha256", reference.selected_key_set_sha256),
        (
            "reference.selected_key_set_sha256_exact",
            _bool(reference.selected_key_set_sha256 == EXPECTED_KEY_SET_SHA256),
        ),
        (
            "reference.selected_key_value_sha256",
            reference.selected_key_value_sha256,
        ),
        (
            "reference.selected_key_value_sha256_exact",
            _bool(
                reference.selected_key_value_sha256
                == EXPECTED_KEY_VALUE_SHA256
            ),
        ),
        ("reference.contract_pass", _bool(result.reference_contract_pass)),
        ("candidate.path", candidate_path),
        ("candidate.size_bytes", candidate.size_bytes),
        ("candidate.sha256", candidate.file_sha256),
        ("candidate.selected_line_count", candidate.selected_line_count),
        (
            "candidate.selected_unique_key_count",
            candidate.selected_unique_key_count,
        ),
        (
            "candidate.selected_duplicate_key_count",
            candidate.selected_duplicate_key_count,
        ),
        (
            "candidate.selected_duplicate_line_count",
            candidate.selected_duplicate_line_count,
        ),
        (
            "candidate.selected_count_exact",
            _bool(
                candidate.selected_line_count == EXPECTED_SELECTED_KEY_COUNT
                and candidate.selected_unique_key_count
                == EXPECTED_SELECTED_KEY_COUNT
            ),
        ),
        (
            "candidate.selected_no_duplicates",
            _bool(
                candidate.selected_duplicate_key_count == 0
                and candidate.selected_duplicate_line_count == 0
            ),
        ),
        ("candidate.selected_missing_key_count", result.candidate_missing_key_count),
        ("candidate.selected_extra_key_count", result.candidate_extra_key_count),
        ("candidate.selected_key_set_exact", _bool(result.candidate_key_set_exact)),
        ("candidate.selected_key_set_sha256", candidate.selected_key_set_sha256),
        (
            "candidate.selected_key_set_sha256_exact",
            _bool(candidate.selected_key_set_sha256 == EXPECTED_KEY_SET_SHA256),
        ),
        (
            "candidate.selected_key_value_sha256",
            candidate.selected_key_value_sha256,
        ),
        (
            "candidate.selected_key_value_sha256_exact",
            _bool(
                candidate.selected_key_value_sha256
                == EXPECTED_KEY_VALUE_SHA256
            ),
        ),
        ("candidate.contract_pass", _bool(result.candidate_contract_pass)),
        ("audit.pass", _bool(result.passed)),
        ("audit.invalidity_override_applied", _bool(invalidity_override)),
        (
            "audit.invalidity_override",
            "invalid_numeric_or_mechanics" if invalidity_override else "none",
        ),
        (
            "audit.classification_override",
            "invalid_numeric_or_mechanics" if invalidity_override else "none",
        ),
        ("result", "PASS" if result.passed else "FAIL"),
    ]


def _error_fields(code: str) -> list[tuple[str, object]]:
    return [
        ("schema", AUDIT_SCHEMA),
        ("protocol_sha256", PROTOCOL_SHA256),
        ("audit.error", code),
        ("audit.pass", "false"),
        ("audit.invalidity_override_applied", "true"),
        ("audit.invalidity_override", "invalid_numeric_or_mechanics"),
        ("audit.classification_override", "invalid_numeric_or_mechanics"),
        ("result", "FAIL"),
    ]


def _fixture_contract(raw: bytes, selector: re.Pattern[str]) -> ExpectedContract:
    summary = _summarize(raw, selector)
    if len(summary.schema_values) != 1:
        raise AssertionError("self-test reference schema fixture is invalid")
    return ExpectedContract(
        size_bytes=summary.size_bytes,
        file_sha256=summary.file_sha256,
        schema=summary.schema_values[0],
        selected_key_count=summary.selected_unique_key_count,
        key_set_sha256=summary.selected_key_set_sha256,
        key_value_sha256=summary.selected_key_value_sha256,
    )


def _run_self_test() -> int:
    selector = re.compile(r"^selected\.")
    baseline = (
        b"schema=self_test.reference.v1\n"
        b"selected.alpha=1\n"
        b"selected.beta=2\n"
        b"selected.gamma=3\n"
        b"unselected.note=ignored\n"
    )
    expected = _fixture_contract(baseline, selector)

    duplicate = baseline + b"selected.alpha=1\n"
    missing = baseline.replace(b"selected.beta=2\n", b"")
    extra = baseline + b"selected.delta=4\n"
    value_mismatch = baseline.replace(b"selected.gamma=3", b"selected.gamma=9")

    baseline_pass = _audit_bytes(
        baseline, baseline, selector=selector, expected=expected
    ).passed
    duplicate_result = _audit_bytes(
        baseline, duplicate, selector=selector, expected=expected
    )
    missing_result = _audit_bytes(
        baseline, missing, selector=selector, expected=expected
    )
    extra_result = _audit_bytes(
        baseline, extra, selector=selector, expected=expected
    )
    value_result = _audit_bytes(
        baseline, value_mismatch, selector=selector, expected=expected
    )

    duplicate_rejected = (
        not duplicate_result.passed
        and duplicate_result.candidate.selected_duplicate_key_count == 1
    )
    missing_rejected = (
        not missing_result.passed and missing_result.candidate_missing_key_count == 1
    )
    extra_rejected = (
        not extra_result.passed and extra_result.candidate_extra_key_count == 1
    )
    value_mismatch_rejected = (
        not value_result.passed
        and value_result.candidate_key_set_exact
        and value_result.candidate.selected_key_value_sha256
        != expected.key_value_sha256
    )
    passed = all(
        (
            baseline_pass,
            duplicate_rejected,
            missing_rejected,
            extra_rejected,
            value_mismatch_rejected,
        )
    )
    _emit(
        [
            ("schema", SELF_TEST_SCHEMA),
            ("protocol_sha256", PROTOCOL_SHA256),
            ("self_test.fixture_storage", "in_memory"),
            ("self_test.training_logs_read", "false"),
            ("self_test.training_logs_mutated", "false"),
            ("self_test.baseline_pass", _bool(baseline_pass)),
            ("self_test.duplicate_rejected", _bool(duplicate_rejected)),
            ("self_test.missing_rejected", _bool(missing_rejected)),
            ("self_test.extra_rejected", _bool(extra_rejected)),
            (
                "self_test.value_mismatch_rejected",
                _bool(value_mismatch_rejected),
            ),
            ("self_test.pass", _bool(passed)),
            ("result", "PASS" if passed else "FAIL"),
        ]
    )
    return 0 if passed else 1


class _StdoutArgumentParser(argparse.ArgumentParser):
    def error(self, message: str) -> None:
        _emit(_error_fields("invalid_arguments"))
        raise SystemExit(2)


def _parser(default_reference: Path, default_candidate: Path) -> argparse.ArgumentParser:
    parser = _StdoutArgumentParser(
        description=(
            "Read-only exact-key audit for the frozen neutral JEPA/MAE namespace."
        )
    )
    parser.add_argument("--reference", type=Path, default=default_reference)
    parser.add_argument("--candidate", type=Path, default=default_candidate)
    parser.add_argument(
        "--self-test",
        action="store_true",
        help="run only in-memory negative fixtures; read no training log",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    torch_root = Path(__file__).resolve().parents[7]
    tests_root = torch_root / ".build" / "tests"
    default_reference = (
        tests_root / "representation_vicreg_variance_necessity_v1_authoritative.log"
    )
    default_candidate = (
        tests_root / "representation_outer_augmentation_training_v1_authoritative.log"
    )
    args = _parser(default_reference, default_candidate).parse_args(argv)
    if args.self_test:
        return _run_self_test()

    reference_path = args.reference.resolve(strict=False)
    candidate_path = args.candidate.resolve(strict=False)
    try:
        result = _audit_bytes(
            _read_only(reference_path, "reference"),
            _read_only(candidate_path, "candidate"),
            selector=SELECTOR,
            expected=FROZEN_CONTRACT,
        )
    except AuditInputError as error:
        _emit(_error_fields(error.code))
        return 1

    _emit(_normal_fields(reference_path, candidate_path, result))
    return 0 if result.passed else 1


if __name__ == "__main__":
    sys.exit(main())
