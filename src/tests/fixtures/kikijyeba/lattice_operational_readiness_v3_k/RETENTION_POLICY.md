# Lattice Operational Readiness V3-K Retention Policy

Generated: 2026-05-23

## Verdict

V3-K is complete as a read-only retention and compaction policy. It does not
delete, prune, archive, or compact runtime evidence. It defines the guardrails
that must be satisfied before future compaction is allowed.

## Replay Authority

Source material for replay remains:

```text
- runtime reports
- lattice sidecars
- checkpoint files and checkpoint facts
- selection-signal facts
```

Audit/read-model metadata remains non-authoritative:

```text
- compact source receipts
- source receipt facts for coverage/leakage authority
- proof certificates
- runtime index cache rows
- PASS.md and human receipts
```

## Required Archive Binding

Archive manifests must bind:

```text
protocol_contract_fingerprint
target_spec_fingerprint
split_policy_fingerprint
graph_order_fingerprint
source_cursor_token
checkpoint_path
checkpoint_id
checkpoint_file_digest
```

## Audit Scenarios

The policy vocabulary covers:

```text
complete_archive_replay
missing_lineage_after_pruning
stale_cache_after_archive_movement
compact_receipt_non_authority
```

## Summary Expectations

```text
policy_count = 9
scenario_count = 4
compact_receipt_authority_count = 0
cache_row_count = 1
human_receipt_count = 1
runtime_executor_count = 0
empty_binding_count = 0
reports_preserved_for_replay = true
sidecars_preserved_for_replay = true
checkpoints_preserved_for_lineage = true
selection_signals_preserved_for_leakage = true
proof_certificates_non_authority = true
cache_rows_rebuildable_non_authority = true
human_receipts_non_authority = true
archive_manifest_binds_identity = true
compact_receipts_non_authority = true
pruning_checks_unresolved_lineage = true
stale_cache_after_archive_non_authority = true
complete_archive_replay_scenario_present = true
summary_self_check_passed = true
```

## Verification

```text
make -C src/tests/bench/kikijyeba/lattice_target run-test_kikijyeba_lattice_target -j12
make -C src/main/hero build-lattice-hero -j12
```
