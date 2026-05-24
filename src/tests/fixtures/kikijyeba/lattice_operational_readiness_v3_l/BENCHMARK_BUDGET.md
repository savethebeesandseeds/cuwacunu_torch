# Lattice Operational Readiness V3-L Benchmark Budget

Generated: `2026-05-23T23:39:56Z`

## Scope

V3-L records finite lattice performance guardrails. It does not optimize
indefinitely, does not make cache rows evidence authority, and does not change
target satisfaction.

The measured timing layers are intentionally separate:

```text
library_function: in-process lattice calls without Hero process startup
long_lived_mcp: one Hero MCP process with repeated tool calls
direct_cli: one Hero process per tool call
```

The measured proof modes are:

```text
header_only
watched_file_manifest
full_runtime_metadata_digest
live_scan
live_parity
```

`runtime_metadata_digest` and `watched_file_metadata_digest` are metadata
freshness digests over path, size, and mtime records. They are not file-content
proofs and do not make cache rows target-satisfaction authority.

## Runtime Evidence

```text
runtime_root = /cuwacunu/.runtime/cuwacunu_exec
split_policy_fingerprint = 72367e40daf6375a
protocol_contract_fingerprint = 8aaff747e54c7c4f
graph_order_fingerprint = 507df53170ffab6a
source_cursor_token = version=ujcamei.graph_anchor_cursor_report.v1|graph=507df53170ffab6a|reference_edge=BTCUSDT|Hx=30|Hf=1|edges=3|accepted=2247|candidates=2248|skipped=1|first=1580687999999|last=1774742399999
vicreg_assembly_fingerprint = 759931dbbd341fcf
mdn_assembly_fingerprint = a4d5971c5c4cbb01
configured_target_count = 20
```

Runtime index cache:

```text
cache_file_exists = true
validation_strength = header_only
result_source = cache
row_count = 78
row_set_digest = 04233814d6ac4293
fact_count = 6
node_exposure_fact_count = 12
checkpoint_fact_count = 4
source_receipt_fact_count = 54
selection_signal_fact_count = 0
representation_support_fact_count = 2
relation_counts =
  checkpoint = 4
  exposure = 6
  node_exposure = 12
  representation_support = 2
  source_receipt = 54
```

Readiness batch used for the benchmark:

```text
vicreg_train_core_ready = satisfied
channel_mdn_train_core_ready = satisfied
channel_mdn_train_core_no_validation_leakage = satisfied
channel_mdn_train_core_no_test_leakage = satisfied
channel_mdn_validation_eval_ready = satisfied
single_runtime_scan = true
read_only = true
```

## Benchmark Budget Surface

Hero exposes:

```text
benchmark_regression_budget_vocabulary
benchmark_regression_budget_summary
```

The summary self-check passed with:

```text
budget_count = 9
library_layer_count = 4
long_lived_mcp_layer_count = 2
direct_cli_layer_count = 3
header_only_mode_count = 3
watched_manifest_mode_count = 1
full_metadata_mode_count = 1
live_scan_mode_count = 3
live_parity_mode_count = 1
cache_target_authority_count = 0
summary_self_check_passed = true
summary_issue_count = 0
```

## Baseline Timings

Library benchmark command:

```text
CUWACUNU_LATTICE_BENCHMARK_RUNTIME_ROOT=/cuwacunu/.runtime/cuwacunu_exec \
make -C src/tests/bench/kikijyeba/lattice_exposure \
  run-test_kikijyeba_lattice_performance_budget -j12
```

Library rows:

```text
library_read_runtime_index_cache          header_only                   mean 6.794 ms
library_header_only_index_query          header_only                   mean 7.002 ms
library_watched_manifest_index_query     watched_file_manifest         mean 237.144 ms
library_full_metadata_index_status       full_runtime_metadata_digest  mean 226.263 ms
library_live_scan_index_build            live_scan                     mean 783.139 ms
```

Long-lived MCP rows:

```text
long_lived_mcp_header_only_index_query   header_only  mean 67.600 ms   runs 10
long_lived_mcp_evaluate_targets_batch    live_scan    mean 1665.600 ms runs 5
```

Direct CLI rows:

```text
direct_cli_header_only_index_query       header_only  mean 107.600 ms  runs 5
direct_cli_live_parity_index_query       live_parity  mean 1298.667 ms runs 3
direct_cli_evaluate_targets_batch        live_scan    mean 1913.000 ms runs 3
```

## Regression Smoke

Header-only fast audit query:

```text
tool = hero.lattice.index_query
validation_strength = header_only
compare_live_scan = false
allow_unproven_cache = true
result_source = cache
cache_valid_for_audit_query = false
cache_used_unproven_for_audit_query = true
cache_used_for_target_satisfaction = false
metadata_checked = false
watched_metadata_checked = false
selected_answer_parity.checked = false
```

Parity proof mode remains available and explicitly slower:

```text
tool = hero.lattice.index_query
validation_strength = full_runtime_metadata_digest
compare_live_scan = true
result_source = cache
cache_valid_for_audit_query = true
cache_used_unproven_for_audit_query = false
selected_answer_parity.checked = true
selected_answer_parity.passed = true
```

The library-level regression executable also asserts:

```text
header_only validation does not compute metadata_checked
header_only validation does not compute watched_metadata_checked
header_only validation still checks row_set_digest
header_only validation still checks relation_counts
watched_file_manifest validation does not compute full metadata digest
```

## Verdict

V3-L is satisfied:

```text
- library-level timings are recorded without Hero process startup
- long-lived MCP timings are recorded separately from direct CLI
- direct CLI timings are recorded separately and include process startup
- every timing row names its proof mode
- runtime root, active identity, row counts, target counts, relation counts,
  and baseline timings are recorded
- header-only fast audit query avoids metadata/live-parity proof work
- proof/parity modes remain available and their cost is named
- cache rows remain non-authoritative for target satisfaction
```
