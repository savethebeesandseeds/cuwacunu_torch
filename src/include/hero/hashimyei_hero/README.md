# hashimyei

Identity and report_fragment dispatch layer for hashimyei-based components.

## Headers
- `hero/hashimyei_hero/hashimyei_identity.h`
- `hero/hashimyei_hero/hashimyei_report_fragments.h`
- `hero/hashimyei_hero/hashimyei_driver.h`
- `hero/hashimyei_hero/hashimyei_catalog.h`
- `hero/hashimyei_hero/hero_hashimyei_tools.h`

- Hard cutoff: use `hashimyei_report_fragments.h` directly.

## Responsibilities
- Canonical alias identity helpers for DSL/path flows.
- Report fragment storage discovery + encrypted metadata helpers.
- Report fragment manifest helpers (`manifest.v2.kv`) for canonical type/file inventory.
- Driver registry/dispatch for component-owned `save`/`load` operations.
- Catalog identity/model helpers for:
  - `hashimyei.run.manifest.v2` (`run.manifest.v2.kv`)
    with `wave_contract_binding_t` (`contract`, `wave`, `binding_id`,
    derived binding hash)
  - `hashimyei.component.manifest.v2` (`component.manifest.v2.kv`)
    with founding DSL provenance metadata
    (`founding_dsl_provenance_path`, `founding_dsl_provenance_sha256_hex`),
    explicit contract docking compatibility digest
    (`docking_signature_sha256_hex`), component `lineage_state`, and
    contract-scoped component lineage via the structured `contract_identity`
    object (`contract_hash` remains the scalar digest term)
  - immutable founding DSL bundle snapshots under
    `.hashimyei/founding_dsl_bundles/<component_id>/...`
    with `founding_dsl_bundle.manifest.v1.kv`

## Design note
- `hashimyei` should not own network/model structures.
- Components own serialization logic.
- `hashimyei` routes save/load calls to registered component drivers and keeps identity/store concerns centralized.
- `default.hero.hashimyei.dsl` configures Hashimyei HERO runtime defaults only; it is
  distinct from a component's founding DSL bundle.
- component revisions dump a stored founding DSL bundle snapshot under
  `.hashimyei`; `hero.hashimyei.get_founding_dsl_bundle` reads that stored
  bundle snapshot as the canonical founding-bundle surface.
- catalog `component_lineage` edges now distinguish component founding
  provenance from stored founding-bundle snapshots through payload kind tags.
- component hashimyei lineage is contract-scoped; reusing the same component
  hashimyei across contracts is invalid.
- component manifests are contract-scoped revisions; run manifests keep the
  operational `wave_contract_binding_t` linkage.
- active component pointers are also contract-scoped; "active" means active for
  a given `(canonical_path/family, contract_hash)` scope, not globally.
- Hash-required identities (`CONTRACT`, `WAVE`, `WAVE_CONTRACT_BINDING`) reuse the same alias
  for the same `(kind, hash_sha256_hex)` once observed in catalog records.

## Typical usage
- Canonical identity:
  - `tsi.wikimyei.representation.vicreg.0x0003`
- Report fragment root:
  - `<hashimyei_store_root>/tsi.wikimyei/<family>/<model>/<hashimyei>/...`
  - env override: `CUWACUNU_HASHIMYEI_STORE_ROOT`
  - config source: `GENERAL.hashimyei_store_root`
- Metadata secret:
  - env override: `CUWACUNU_HASHIMYEI_META_SECRET`
  - config fallback: `GENERAL.hashimyei_metadata_secret`
- Catalog DB path helper:
  - `hashimyei::catalog_db_path()` -> `<store-root>/catalog/hashimyei_catalog.idydb`
- Driver dispatch:
  - register a driver for the component family canonical path (`family_canonical_path`)
  - call `dispatch_report_fragment_save(...)` / `dispatch_report_fragment_load(...)`
