# Lattice Operational Readiness V3-B PASS

Generated: `2026-05-23T18:10:00Z`

V3-B closes the lattice performance pass without changing the read-only
authority boundary. Runtime files, reports, sidecars, and checkpoint facts remain
the source of truth. Lattice Hero still does not write evidence, execute waves,
or use cache rows for target satisfaction.

## Implemented

```text
hero.lattice.evaluate_targets
watched-file-guarded long-lived scan reuse
runtime index validation_strength = header_only | watched_file_manifest | full_runtime_metadata_digest
runtime index row_set_digest
runtime index relation_counts
checkpoint closure lookup indexes
streaming checkpoint file digest
exposure_scan_options_t
per-job lattice artifact bundle
```

## Benchmark

Direct CLI benchmark against `/cuwacunu/.runtime/cuwacunu_exec`:

```text
status                         mean 355.600 ms
evaluate_target_channel_mdn    mean 491.600 ms
evaluate_targets_batch_5       mean 982.600 ms
index_status_full              mean 152.600 ms
index_status_watched           mean 162.200 ms
index_status_header            mean  40.000 ms
index_query_parity             mean 789.600 ms
index_query_fast_watched       mean 175.000 ms
index_query_fast_header        mean  36.400 ms
checkpoint_closure             mean 342.000 ms
```

Interpretation:

```text
- direct CLI includes process startup and policy/context setup
- parity proof remains intentionally slower because it rebuilds and compares live evidence
- the explicit interactive fast path is header_only + allow_unproven_cache + compare_live_scan=false
- batch evaluation avoids repeated scans when multiple readiness targets are needed
- watched_file_manifest is a bounded freshness check, not the fastest unproven lane
```

## Verification

```bash
make -C src/tests/bench/kikijyeba/lattice_exposure run-test_kikijyeba_lattice_exposure -j12
make -C src/main/hero build-lattice-hero -j12
/cuwacunu/.build/hero/hero_lattice.mcp --global-config /cuwacunu/src/config/.config --list-tools-json
/cuwacunu/.build/hero/hero_lattice.mcp --global-config /cuwacunu/src/config/.config --tool hero.lattice.evaluate_targets --args-json '{"target_ids":["vicreg_train_core_ready","channel_mdn_train_core_ready"],"limit":2}'
/cuwacunu/.build/hero/hero_lattice.mcp --global-config /cuwacunu/src/config/.config --tool hero.lattice.index_status --args-json '{"validation_strength":"watched_file_manifest","limit":2}'
```

