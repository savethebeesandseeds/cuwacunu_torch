# Lattice Legacy Compatibility Audit

Date: 2026-06-01

Purpose:

```text
Mark legacy and compatibility surfaces before a later dev_nuke cleanup.
```

This file is an audit only. It does not authorize deleting code by itself.

`dev_nuke` is assumed to remove old Lattice `.runtime` records, stale report
stores, cache rows, and historical job artifacts. That makes runtime-data
compatibility removable after validation, but it does not automatically retire
public Hero tools, DSL aliases, target ids, or current trainable target
behavior.

## Dev Nuke Result

Runtime Hero's pre-collapse developer reset surface was run against
`/cuwacunu/.runtime` on 2026-06-01 with `dry_run=false`, `backup=false`, and
`confirm_dev_nuke=true`.

Result:

```text
removed_paths = /cuwacunu/.runtime/cuwacunu_exec
cleared_entry_count = 233
active_jobs = []
backup_snapshot_path = null
```

After the reset, Runtime Hero reported `runtime_root_exists=false` for
`/cuwacunu/.runtime/cuwacunu_exec` and `job_count=0`.

This clears stale runtime data. It does not by itself remove code paths that
still exist for compatibility.

## Not Legacy

Keep these. They are the current Lattice target/training path.

| Surface | Files | Reason |
| --- | --- | --- |
| Active `TARGET_KIND` enum: `vicreg_ready`, `mtf_representation_ready`, `channel_mdn_ready` | `src/include/hero/lattice_hero/lattice/target/lattice_target.h`, `src/config/hero.lattice.targets.dsl` | Core trainable/checkpoint-producing readiness targets. Marshal uses these dispatchable targets to prepare bounded Runtime handoffs. |
| `latest_satisfying:<target_id>` readiness selection | `src/include/hero/lattice_hero/lattice/target/lattice_target.h`, `src/config/hero.lattice.targets.dsl` | Deterministic readiness-based checkpoint source. It is not a best-model selector, but it is part of the current target dependency path. |
| `hero.marshal.prepare` for dispatchable readiness targets | `src/include/hero/marshal_hero/hero_marshal.def`, `src/tests/bench/kikijyeba/marshal/test_kikijyeba_marshal_dispatch.cpp` | Current coordination path from Lattice target advice to Runtime handoff preparation. The retired `hero.marshal.reach_lattice_target` name is kept only in negative tests. |
| Active artifact-readiness targets | `src/config/hero.lattice.targets.dsl`, `src/include/hero/lattice_hero/lattice/target/lattice_target.h` | Current non-dispatchable proof targets over catalog facts. They protect fact expansion from becoming `TARGET_KIND` expansion. |
| Disabled policy-gate reservations | `src/include/hero/lattice_hero/lattice/target/lattice_target.h`, `src/config/hero.lattice.targets.dsl` | Current reserved/fail-closed future-policy syntax. It documents future policy inputs without granting authority. |

## Runtime-Data Legacy

These are candidates for removal after `dev_nuke`, because they primarily keep
old runtime stores or old report shapes readable.

| Surface | Files | Current state | Suggested action after dev_nuke |
| --- | --- | --- | --- |
| Exposure derivation from job manifest/state/report when no exposure sidecar exists | `src/include/hero/lattice_hero/lattice/exposure/exposure_ledger.h` | Scanner can still derive facts from older job directories. Current Runtime is expected to write canonical sidecars. | Verify new jobs always emit sidecars, then remove or quarantine report-derived fallback paths one family at a time. |
| Runtime report relaxed readers | `src/include/hero/lattice_hero/lattice/runtime_report/CURRENT_RUNTIME_AUDIT.md`, `src/impl/hero/lattice_hero/lattice_catalog.cpp`, `src/impl/jkimyei/evaluation/data_analytics.cpp`, `src/impl/jkimyei/evaluation/network_analytics.cpp`, `src/impl/jkimyei/evaluation/entropic_capacity_comparison.cpp` | Legacy readers still accept relaxed line payloads, comments, or last-write-wins behavior outside strict runtime `.lls`. | Remove relaxed runtime-data readers once strict producers and dev_nuked stores are the only supported path. |
| `jkimyei.evaluation.entropic_capacity_comparison.v1` helper | `src/include/hero/lattice_hero/lattice/runtime_report/SCHEMA_REGISTRY.md`, `src/impl/jkimyei/evaluation/entropic_capacity_comparison.cpp` | Reader/helper compatibility; no standard component producer or ingest path. | Delete or quarantine if no tests or tools require it after dev_nuke. |
| Archived VICReg / transfer-matrix runtime artifact compatibility | `src/include/hero/lattice_hero/lattice/runtime_report/MIGRATION_BACKLOG.md`, runtime-report docs/tests | Archived compatibility surfaces, not authored graph-first Wikimyei runtime paths. | Remove compatibility readers/fixtures after archived stores are no longer supported. |
| Rebuildable cache rows from old scan state | `src/include/hero/lattice_hero/lattice/exposure/exposure_ledger.h`, Hero Lattice docs | Cache/index rows are read models, not source of truth. | Safe to drop stale cache compatibility after dev_nuke; keep live scan behavior. |

## API And DSL Compatibility

These are legacy-shaped, but not automatically removable by `dev_nuke`, because
they affect config files, clients, tests, or public Hero JSON.

| Surface | Files | Current state | Cleanup rule |
| --- | --- | --- | --- |
| Target DSL old/new alias pairs: `COMPONENT`/`SUBJECT_COMPONENT`, `TRAIN_SPLIT`/`OVER_SPLIT`, `PLAN_MODE`/`WAVE_MODE`, `MAX_WAVES`/`PLAN_MAX_ATTEMPTS`, `FACT_FAMILY`/`SUBJECT_FACT_FAMILY`, `APPLY_SPLIT_PROTECTION`/`PROTECT_SPLIT` | `src/include/hero/lattice_hero/lattice/target/lattice_target.h`, `src/config/hero.lattice.targets.dsl`, `src/include/hero/lattice_hero/lattice/README.md` | Old names still load; mixed equal values warn; conflicting values fail. | Retire only after configs/docs/tests use preferred spellings and a deliberate API break is accepted. |
| `mtf_jepa_mae_vicreg_ready` spelling alias | `src/include/hero/lattice_hero/lattice/target/lattice_target.h` | Parses as `mtf_representation_ready`. | Candidate for later DSL cleanup, not runtime-data cleanup. |
| Removed target-kind tombstones: `representation_ready`, `node_mdn_ready` | `src/include/hero/lattice_hero/lattice/target/lattice_target.h`, target tests | Explicit migration errors, not active support. | Keep until migration diagnostics are no longer useful; deletion is optional. |
| Protocol-scoped readiness aliases and older unscoped target ids | `src/config/hero.lattice.targets.dsl` | Current DSL says unscoped ids remain for compatibility while training continues on `cwu_01v`. | Do not delete as part of dev_nuke. Retire only after active configs and Marshal flows use protocol-scoped ids. |
| Hero JSON compatibility aliases such as `warning_count` | `src/include/hero/lattice_hero/lattice/target/lattice_target.h`, `src/include/hero/lattice_hero/lattice/README.md`, `src/main/hero/README.md`, `src/config/man/hero.lattice.man` | Kept as public output compatibility while structured fields such as `triggered_warning_count` carry the current semantics. | Retire only after downstream clients stop reading aliases. |
| V2/V3 vocabulary labels in docs and policy summaries | Lattice README, Config README, Hero README, `.man` docs, target vocabulary code | Mostly historical names for read-only policy/vocabulary surfaces. | Rename only if the public tool output changes too; otherwise treat as documentation/API label compatibility. |

## Quarantined Or Parked Surfaces

These should stay out of active proof authority.

| Surface | Current boundary |
| --- | --- |
| `replay_environment` | Parked future environment evidence. Audit-only when explicitly inspected; default scans do not derive it; not an active artifact-readiness proof template; not Runtime execution, allocation, market-readiness, or deployment authority. Do not delete it during runtime-data cleanup. |
| Performance/calibration/benchmark V3 vocabularies | Read-only visibility or policy summaries; no active performance-gate authority. |
| DB/cache/index surfaces | Rebuildable read models; target evaluation still uses durable runtime evidence/live scan as source of truth. |

## Cleanup Order After `dev_nuke`

Use small deletion milestones. Do not delete everything at once.

1. Confirm `dev_nuke` removes stale Lattice `.runtime` records, report stores,
   scan caches, and historical job artifacts.
2. Prove current Runtime emits canonical sidecars or strict `.lls` payloads for
   active facts needed by readiness and artifact-readiness targets.
3. Remove runtime-data compatibility readers and stale cache/report fallback
   paths one family at a time.
4. Leave `replay_environment` parked and non-authoritative until a bounded
   environment Lattice roadmap promotes or replaces it.
5. Only after runtime-data cleanup, decide whether to retire public DSL/API
   compatibility aliases.
6. Keep the trainable target path and Marshal reachability tests green
   throughout.

First code-removal candidate:

```text
Runtime Report Relaxed Reader Cleanup
```

Relaxed report readers are the clearest runtime-data legacy candidates after
`dev_nuke`, provided strict producers and canonical sidecars are confirmed.
Clean them one schema family at a time, then run exposure, Runtime schema,
Marshal, and target validations. Do not include parked environment evidence in
generic legacy deletion.

## Required Validation For Cleanup Milestones

Run the smallest relevant group after each deletion. A full cleanup review
should run:

```text
make -C src/tests/bench/kikijyeba/lattice_exposure -j12 run-test_kikijyeba_lattice_exposure
make -C src/tests/bench/kikijyeba/lattice_target -j12 run-test_kikijyeba_lattice_target
make -C src/tests/bench/kikijyeba/runtime -j12 run-test_hero_mcp_schema_compat
make -C src/tests/bench/kikijyeba/marshal -j12 run-test_kikijyeba_marshal_dispatch
git diff --check
```

## Do Not Delete In Legacy Cleanup

- active `TARGET_KIND` readiness targets
- `latest_satisfying` readiness checkpoint source semantics
- `hero.marshal.prepare` for dispatchable readiness targets
- artifact-readiness proof templates for active fact families
- disabled policy-gate reservations and fail-closed checks
- warning non-blocking behavior and authority flags
- parked `replay_environment` future environment evidence
