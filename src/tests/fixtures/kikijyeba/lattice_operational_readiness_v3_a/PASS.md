# Lattice Operational Readiness V3-A PASS

Generated: `2026-05-23T19:58:48Z`

V3-A promotes the V2 rebuildable index/cache from status-only visibility to a
queryable read-only audit surface. The source of truth remains runtime files,
reports, sidecars, and checkpoint facts. Lattice Hero does not write index
files, evidence, runtime state, or target satisfaction.

## Scope

Implemented V3-A:

```text
hero.lattice.index_query
```

The tool filters runtime-index rows by:

```text
relation
key
key_contains
digest
digest_prefix
```

It reports:

```text
cache_validation
stored_cache_parity
selected_answer_parity
result_source
cache_valid_for_audit_query
cache_used_unproven_for_audit_query
cache_used_for_target_satisfaction = false
index_written = false
```

## Active Runtime Evidence

Active runtime root:

```text
/cuwacunu/.runtime/cuwacunu_exec
```

Temporary V3-A audit cache used for smoke proof:

```text
/tmp/hero_mcp_smoke/v3_a/lattice_runtime_index.v1.lls
```

The cache was built outside Hero from runtime files:

```text
rows = 78
runtime_metadata_digest = 790d88b7c922d424
```

`hero.lattice.index_query` over the active runtime and the temporary cache:

```text
relation = checkpoint
result_source = cache
cache_valid_for_audit_query = true
cache_used_for_target_satisfaction = false
index_written = false
stored_cache_parity.checked = true
stored_cache_parity.passed = true
stored_cache_parity.indexed_row_count = 78
stored_cache_parity.live_row_count = 78
selected_answer_parity.checked = true
selected_answer_parity.passed = true
selected_answer_parity.indexed_row_count = 4
selected_answer_parity.live_row_count = 4
matching_row_count = 4
```

`hero.lattice.index_query` over representation-support rows:

```text
relation = representation_support
result_source = cache
cache_valid_for_audit_query = true
stored_cache_parity.passed = true
selected_answer_parity.passed = true
matching_row_count = 2
```

Default active runtime has no persisted `indexes/lattice_runtime_index.v1.lls`
file, so the tool also
proved the fallback path:

```text
cache_file_exists = false
result_source = live_scan
cache_valid_for_audit_query = false
selected_answer_parity.passed = true
cache_used_for_target_satisfaction = false
index_written = false
```

Stale-cache smoke:

```text
index_path = /tmp/hero_mcp_smoke/v3_a/lattice_runtime_index_stale.v1.lls
result_source = live_scan
cache_valid_for_audit_query = false
metadata_matches = false
cache_valid = false
cache_validation.issues = [runtime_metadata_digest_mismatch]
selected_answer_parity.passed = true
matching_row_count = 4
```

Tampered-cache smoke:

```text
index_path = /tmp/hero_mcp_smoke/v3_a/lattice_runtime_index_tampered.v1.lls
result_source = live_scan
cache_valid_for_audit_query = false
metadata_matches = true
cache_valid = true
stored_cache_parity.passed = false
stored_cache_parity.missing_from_index_count = 1
stored_cache_parity.issues include row_count_mismatch and row_digest_set_mismatch
selected_answer_parity.passed = true
matching_row_count = 4
```

Unproven-cache smoke:

```text
compare_live_scan = false
allow_unproven_cache = false
result_source = live_scan
cache_valid_for_audit_query = false
cache_used_unproven_for_audit_query = false
matching_row_count = 4
```

Fast unproven-cache smoke:

```text
compare_live_scan = false
allow_unproven_cache = true
validation_strength = header_only
metadata_checked = false
result_source = cache
cache_valid_for_audit_query = false
cache_used_unproven_for_audit_query = true
cache_used_for_target_satisfaction = false
matching_row_count = 4
```

Fast header-only index status smoke:

```text
validation_strength = header_only
metadata_checked = false
result_source = cache
row_count = 78
cache_used_for_target_satisfaction = false
```

## Performance Follow-Up

The post-review fast path keeps proof mode intact and adds an explicit unproven
audit-cache lane:

```text
index_status_valid_cache_metadata   mean 165.358 ms
index_status_valid_cache_header     mean  51.924 ms
index_query_valid_cache_parity      mean 380.474 ms
index_query_unproven_live_fallback  mean 367.692 ms
index_query_fast_unproven_header    mean  63.654 ms
```

The fast path is opt-in:

```text
compare_live_scan = false
allow_unproven_cache = true
validation_strength = header_only
```

and remains non-authoritative:

```text
cache_valid_for_audit_query = false
cache_used_unproven_for_audit_query = true
cache_used_for_target_satisfaction = false
```

## Fixture Evidence

Focused fixture tests prove:

```text
- valid runtime index cache validates against runtime-file metadata
- header-only runtime index validation is structural and skips runtime metadata
  digest proof
- relation query returns representation_support rows
- key-substring query isolates one representation-support node row
- exact-key query isolates a stable index key
- exact-digest query returns only matching row digests
- digest-prefix query returns only matching digest prefixes
- valid cache rows match live-scan rows
- query answers match live-scan answers
- tampered cache rows fail parity
- stale runtime metadata fails closed
- header-only validation remains explicit structural validation, not a freshness
  proof
```

## Verification Commands

```bash
make -C src/tests/bench/kikijyeba/lattice_exposure run-test_kikijyeba_lattice_exposure -j12
make -C src/main/hero build-lattice-hero -j12
/cuwacunu/.build/hero/hero_lattice.mcp --global-config /cuwacunu/src/config/.config --list-tools-json
/cuwacunu/.build/hero/hero_lattice.mcp --global-config /cuwacunu/src/config/.config --tool hero.lattice.index_query --args-json '{"index_path":"/tmp/hero_mcp_smoke/v3_a/lattice_runtime_index.v1.lls","relation":"checkpoint","limit":8,"compare_live_scan":true}'
/cuwacunu/.build/hero/hero_lattice.mcp --global-config /cuwacunu/src/config/.config --tool hero.lattice.index_query --args-json '{"index_path":"/tmp/hero_mcp_smoke/v3_a/lattice_runtime_index.v1.lls","relation":"representation_support","limit":8,"compare_live_scan":true}'
/cuwacunu/.build/hero/hero_lattice.mcp --global-config /cuwacunu/src/config/.config --tool hero.lattice.index_query --args-json '{"index_path":"/tmp/hero_mcp_smoke/v3_a/lattice_runtime_index_stale.v1.lls","relation":"checkpoint","limit":4,"compare_live_scan":true}'
/cuwacunu/.build/hero/hero_lattice.mcp --global-config /cuwacunu/src/config/.config --tool hero.lattice.index_query --args-json '{"index_path":"/tmp/hero_mcp_smoke/v3_a/lattice_runtime_index_tampered.v1.lls","relation":"checkpoint","limit":4,"compare_live_scan":true}'
/cuwacunu/.build/hero/hero_lattice.mcp --global-config /cuwacunu/src/config/.config --tool hero.lattice.index_query --args-json '{"index_path":"/tmp/hero_mcp_smoke/v3_a/lattice_runtime_index.v1.lls","relation":"checkpoint","limit":4,"compare_live_scan":false}'
/cuwacunu/.build/hero/hero_lattice.mcp --global-config /cuwacunu/src/config/.config --tool hero.lattice.index_query --args-json '{"index_path":"/tmp/hero_mcp_smoke/v3_a/lattice_runtime_index.v1.lls","relation":"checkpoint","limit":4,"compare_live_scan":false,"allow_unproven_cache":true,"validation_strength":"header_only"}'
/cuwacunu/.build/hero/hero_lattice.mcp --global-config /cuwacunu/src/config/.config --tool hero.lattice.index_status --args-json '{"index_path":"/tmp/hero_mcp_smoke/v3_a/lattice_runtime_index.v1.lls","limit":1}'
/cuwacunu/.build/hero/hero_lattice.mcp --global-config /cuwacunu/src/config/.config --tool hero.lattice.index_status --args-json '{"index_path":"/tmp/hero_mcp_smoke/v3_a/lattice_runtime_index.v1.lls","limit":1,"validation_strength":"header_only"}'
git diff --check -- src/config/hero.lattice.dsl src/config/README.md src/impl/hero/lattice_hero/hero_lattice_tools.cpp src/include/kikijyeba/lattice/README.md src/include/kikijyeba/lattice/ROADMAP.md src/include/kikijyeba/lattice/exposure/exposure_ledger.h src/main/hero/README.md src/main/hero/hero_lattice_mcp.cpp src/tests/bench/kikijyeba/lattice_exposure/test_kikijyeba_lattice_exposure.cpp src/tests/fixtures/kikijyeba/lattice_operational_readiness_v3_a/PASS.md
```

All commands passed.

## Boundary

V3-A does not implement:

```text
- performance gates
- MDN distribution calibration gates
- hard representation-geometry gates
- Datalog-style derived query rules beyond row-query parity
- Pareto/nondominated checkpoint comparison
- source-key/order-preserving map audit authority
```

Those remain V3 candidates outside this bounded goal.
