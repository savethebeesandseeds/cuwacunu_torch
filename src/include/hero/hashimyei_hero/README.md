# hashimyei

Identity and artifact dispatch layer for hashimyei-based components.

## Headers
- `hero/hashimyei_hero/hashimyei_identity.h`
- `hero/hashimyei_hero/hashimyei_artifacts.h`
- `hero/hashimyei_hero/hashimyei_driver.h`
- `hero/hashimyei_hero/hashimyei_catalog.h`

## Responsibilities
- Canonical alias identity helpers for DSL/path flows.
- Artifact storage discovery + encrypted metadata helpers.
- Artifact manifest helpers (`manifest.v2.kv`) for canonical type/file inventory.
- Driver registry/dispatch for component-owned `save`/`load` operations.
- Catalog identity/model helpers for:
  - `hashimyei.run.manifest.v2` (`run.manifest.v2.kv`)
  - `hashimyei.component.manifest.v2` (`component.manifest.v2.kv`)
  - `wave_contract_binding_t` (`contract`, `wave`, `binding_alias`, derived binding hash)
  - `component_binding` index rows for transversal binding -> component queries

## Design note
- `hashimyei` should not own network/model structures.
- Components own serialization logic.
- `hashimyei` routes save/load calls to registered component drivers and keeps identity/store concerns centralized.
- Hash-required identities (`CONTRACT`, `WAVE`, `WAVE_CONTRACT_BINDING`) reuse the same alias
  for the same `(kind, hash_sha256_hex)` once observed in catalog history.

## Typical usage
- Canonical identity:
  - `tsi.wikimyei.representation.vicreg.0x0003`
- Artifact root:
  - `<hashimyei_store_root>/tsi.wikimyei/<family>/<model>/<hashimyei>/...`
  - env override: `CUWACUNU_HASHIMYEI_STORE_ROOT`
  - default root fallback: `/cuwacunu/.hashimyei`
- Metadata secret:
  - env override: `CUWACUNU_HASHIMYEI_META_SECRET`
  - config fallback: `GENERAL.hashimyei_metadata_secret`
- Catalog DB path helper:
  - `hashimyei::catalog_db_path()` -> `<store-root>/catalog/hashimyei_catalog.idydb`
- Driver dispatch:
  - register a driver for `canonical_type`
  - call `dispatch_artifact_save(...)` / `dispatch_artifact_load(...)`
