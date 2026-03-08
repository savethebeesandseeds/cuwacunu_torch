# db_PLAN.md
Last updated: 2026-03-08
Owner: HERO Hashimyei initiative
Status: Living checklist for DB-backed Hashimyei catalog

## 1. Foundation Decisions (Locked)
- [x] Hashimyei is identity for TSI components that persist runtime state.
- [x] Some circuit components do not have hashimyei; DB must still track them.
- [x] Canonical paths are first-class identity and must exist on every record.
- [x] Wave-contract binding is immutable experiment identity.
- [x] Files under `.hashimyei` remain source-of-truth artifacts.
- [x] DB is catalog/index over artifacts, not canonical evidence.
- [x] Backend is IdyDB.
- [x] Default DB mode is encrypted.
- [x] Ingest is post-run job, not hot runtime write path.
- [x] V1 scope is known schemas only.
- [x] Query model is append-log persistence plus in-memory indexes.

## 2. Success Criteria
- [ ] Any artifact can be resolved to immutable run identity (`board_hash`, `contract_hash`, `wave_hash`, `binding_id`).
- [x] Any query can be answered by canonical path, with nullable hashimyei.
- [x] Catalog rebuild from filesystem is deterministic and idempotent.
- [x] MCP queries do not require full filesystem scan in steady-state.
- [x] Catalog encryption and wrong-passphrase failure are validated by tests.

## 3. Data Model and Identity
- [x] Define `run_id = sha256(board_hash|contract_hash|wave_hash|binding_id|started_at_ms)`.
- [x] Create run manifest path `.hashimyei/runs/<run_id>/run.manifest.v1.kv`.
- [x] Store in run manifest: `run_id`, timestamps, hashes, binding, sampler, record_type, seed, device, dtype.
- [x] Store dependency fingerprints: canonical path + sha256 for board/contract/wave inputs.
- [x] Store component instance list: canonical path, tsi type, optional hashimyei.
- [x] Create logical table `runs`.
- [x] Create logical table `artifacts`.
- [x] Create logical table `metrics_num`.
- [x] Create logical table `metrics_txt`.
- [x] Create logical table `provenance_files`.
- [x] Create logical table `ingest_ledger`.
- [ ] Define uniqueness rules: `(artifact_sha256)` in ledger, `(run_id, path, schema, sha256)` in artifacts.
- [ ] Define nullability: `hashimyei` nullable, `canonical_path` non-null.

## 4. Physical Catalog on IdyDB
- [x] Set DB location `.hashimyei/catalog/hashimyei_catalog.idydb`.
- [x] Open with `idydb_open_encrypted` by default.
- [x] Use secret source `GENERAL.hashimyei_metadata_secret`.
- [x] Add catalog header record with `catalog_schema_version=1`.
- [x] Use append-only row records with `record_kind` discriminator.
- [x] Define fixed columns for append rows: `record_kind`, `record_id`, `run_id`, `canonical_path`, `hashimyei`, `schema`, `metric_key`, `metric_num`, `metric_txt`, `artifact_sha256`, `path`, `ts_ms`, `payload_json`.
- [x] Keep file format unchanged for `.hashimyei` artifacts.
- [x] Keep IdyDB on-disk format unchanged in v1.

## 5. Missing IdyDB Query Utilities (Gap Closure)
- [x] Keep `src/include/camahjucunu/db/idydb.h` as the single public DB header and host query utilities in `namespace db::query`.
- [x] Keep `src/impl/camahjucunu/db/idydb.cpp` as the single DB implementation unit (query utilities merged, no behavior change).
- [x] Keep IdyDB C ABI unchanged in v1 while exposing C++ query utilities in `namespace db::query`.
- [x] Add `query_spec_t` with fields: `filters`, `select_columns`, `limit`, `offset`, `newest_first`.
- [x] Add `query_row_t` with fields: `row_id`, `values`.
- [x] Add utility `select_rows(idydb**, const query_spec_t&, std::vector<idydb_column_row_sizing>*)`.
- [x] Add utility `fetch_row(idydb**, idydb_column_row_sizing row, const std::vector<idydb_column_row_sizing>& columns, std::vector<idydb_value>*)`.
- [x] Add utility `fetch_rows(idydb**, const std::vector<idydb_column_row_sizing>& rows, const std::vector<idydb_column_row_sizing>& columns, std::vector<query_row_t>*)`.
- [x] Add utility `count_rows(idydb**, const std::vector<idydb_filter_term>& filters, std::size_t*)`.
- [x] Add utility `latest_row_by_key(idydb**, idydb_column_row_sizing key_column, const std::string& key_value, idydb_column_row_sizing*)`.
- [x] Add utility `exists_row(idydb**, const std::vector<idydb_filter_term>& filters, bool*)`.
- [x] Add deterministic row ordering policy in utilities (row id ascending by default, descending when `newest_first=true`).
- [x] Add memory ownership contract: all returned `idydb_value` buffers freed via `idydb_values_free`.
- [x] Add tests for utility filters on `int`, `float`, `bool`, `char`, null checks.
- [x] Add tests for `limit` and `offset`.
- [x] Add tests for latest-row semantics.
- [x] Add tests for large row counts and stable ordering.
- [x] Add tests for encrypted DB utility queries.

## 6. Ingest Pipeline
- [x] Add ingest binary under HERO tooling (`hero_hashimyei_ingest`).
- [x] Ingest sequence: run manifest -> artifact registration -> schema projection -> ledger commit.
- [x] Add known-schema parser: `transfer_matrix_evaluation.epoch/history/activation/prequential`.
- [x] Add known-schema parser: `piaabo.torch_compat.data_analytics.v1`.
- [x] Add known-schema parser: `piaabo.torch_compat.network_analytics.v1-v4`.
- [x] Add known-schema parser: `piaabo.torch_compat.entropic_capacity_comparison.v1`.
- [x] Store full raw payload for every known file in `payload_json` mirror field.
- [ ] Add idempotency by artifact sha256 and ingest version.
- [x] Add backfill mode for existing `.hashimyei` trees.
- [x] Define legacy fallback run identity for historical files without run manifests.
- [x] Add ingest lock to enforce single-writer catalog updates.

## 7. Query API and MCP Readiness
- [x] Add catalog store service `hashimyei_catalog_store_t`.
- [x] Rebuild in-memory indexes from catalog on startup.
- [x] Add query `get_run(run_id)`.
- [x] Add query `list_runs_by_binding(contract_hash, wave_hash, binding_id)`.
- [x] Add query `latest_artifact(canonical_path, schema)`.
- [x] Add query `artifact_metrics(artifact_id)`.
- [x] Add query `performance_snapshot(canonical_path, run_id)`.
- [x] Add query `provenance_trace(artifact_id)` with dependency fingerprint chain.
- [x] Reserve MCP namespace `hero.hashimyei.*`.
- [x] Define first MCP tools: `scan`, `latest`, `history`, `performance`, `provenance`.
- [x] Ensure every response contains canonical path.
- [x] Ensure non-hashimyei paths are fully supported.

## 8. Testing and Validation
- [x] Unit tests for run manifest parser and validator.
- [x] Unit tests for each known schema projection parser.
- [x] Unit tests for idempotent ingest and duplicate suppression.
- [x] Integration test ingest from sample `.hashimyei` tree.
- [x] Integration test encrypted catalog open/reopen/wrong-passphrase.
- [x] Integration test full catalog rebuild from filesystem.
- [x] Integration test canonical-path query for hashimyei and non-hashimyei components.
- [ ] Performance smoke test: startup rebuild and top query latency for realistic artifact count.

## 9. Rollout Phases
- [x] Phase A: run manifest + ingest ledger + artifact table.
- [x] Phase B: known-schema projection tables.
- [x] Phase C: query utility layer + catalog service.
- [x] Phase D: MCP read-only tools.
- [ ] Phase E: backfill and cutover to DB-first query path.

## 10. Exit Criteria for “DB Ready”
- [x] Rebuild is deterministic and repeatable.
- [x] All v1 known schemas ingest and query correctly.
- [x] Query utilities are covered by tests and documented.
- [x] MCP can answer provenance and latest-performance without filesystem walk.
- [ ] Team checklist has no unresolved blocker in sections 3-9.
