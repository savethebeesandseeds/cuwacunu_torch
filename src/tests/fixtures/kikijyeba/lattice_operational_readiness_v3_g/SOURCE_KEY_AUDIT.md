# Lattice Operational Readiness V3-G Source-Key Map Audit

Generated: 2026-05-23

## Scope

V3-G strengthens source-key and row-to-source-key audits while preserving the
V1/V2 boundary:

```text
row-index intervals remain coverage and leakage authority
source-key windows remain audit-only metadata
source receipts remain provenance/audit facts
```

## Implemented Audit Fields

`source_key_window_audit` now reports:

```text
complete
numeric
internally_monotone
order_preserving
affine_step_available
reference_key_step
affine_consistent
missing_endpoint_pair_count
irregular_key_warning_count
source_key_gap_warning_count
row_source_key_mismatch_count
source_key_gap_found
row_source_key_mismatch_found
issues
endpoints
```

The gap/mismatch counters make these cases visible without parsing warning text:

```text
- incomplete source-key endpoint pairs
- irregular anchor key step
- nonpositive anchor key step
- row/source-key affine mismatch
- row/source-key affine overflow
```

## Summary Surface

`hero.lattice.scan_exposure` now emits:

```text
source_key_map_audit_summary
```

The summary records:

```text
- exposure fact count
- source receipt fact count
- source-receipt parent binding count
- available/unavailable audit counts
- monotone/order-preserving/affine counts
- gap, irregular-key, and row/source-key mismatch counts
- graph-order and source-cursor binding counts
- source-receipt binding counts
- audit-only authority flags
- explicit-target-rule-required-for-blocking flag
```

## Active Runtime Smoke

Runtime root:

```text
/cuwacunu/.runtime/cuwacunu_exec
```

Direct Lattice Hero scan:

```text
hero.lattice.scan_exposure
```

Observed source-key map summary:

```text
exposure_fact_count = 6
source_receipt_fact_count = 54
source_receipt_bound_parent_count = 6
available_audit_count = 6
unavailable_audit_count = 0
complete_count = 6
numeric_count = 6
internally_monotone_count = 6
order_preserving_count = 6
affine_step_available_count = 6
affine_consistent_count = 6
source_key_gap_warning_count = 0
irregular_key_warning_count = 0
missing_endpoint_pair_count = 0
row_source_key_mismatch_count = 0
audits_with_graph_order_count = 6
audits_with_source_cursor_count = 6
audits_with_source_receipts_count = 6
unique_graph_order_count = 1
unique_source_cursor_count = 1
audit_only = true
coverage_authority = false
leakage_authority = false
row_index_authority_preserved = true
explicit_target_rule_required_for_blocking = true
summary_self_check_passed = true
summary_issue_count = 0
```

## Regression Coverage

Focused exposure regression:

```text
make -C src/tests/bench/kikijyeba/lattice_exposure run-test_kikijyeba_lattice_exposure -j12
```

Covered cases:

```text
- monotone affine source-key windows
- source-key summary binding to graph/source identity and source receipts
- nonmonotone key intervals
- row-order inversion
- affine row/source-key mismatch
- irregular monotone anchor key step
- row-index authority preserved for all source-key audit summaries
```

Focused target regression:

```text
make -C src/tests/bench/kikijyeba/lattice_target run-test_kikijyeba_lattice_target -j12
```

Covered cases:

```text
- source-key coordinate policy vocabulary has one row-index authority row
- source-key coordinate policy has four audit-only rows
- gap/irregular-key audit fields are declared
- certificate self-check recomputes source-key window audits
```

Hero build smoke:

```text
make -C src/main/hero build-lattice-hero -j12
```

## Status

```text
V3-G complete.
```
