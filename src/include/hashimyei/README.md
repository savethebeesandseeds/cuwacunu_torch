# hashimyei

Identity and artifact dispatch layer for hashimyei-based components.

## Headers
- `hashimyei/hashimyei_identity.h`
- `hashimyei/hashimyei_artifacts.h`
- `hashimyei/hashimyei_driver.h`

## Responsibilities
- Canonical alias identity helpers for DSL/path flows.
- Artifact storage discovery + encrypted metadata helpers.
- Artifact manifest helpers (`manifest.txt`) for canonical type/file inventory.
- Driver registry/dispatch for component-owned `save`/`load` operations.

## Design note
- `hashimyei` should not own network/model structures.
- Components own serialization logic.
- `hashimyei` routes save/load calls to registered component drivers and keeps identity/store concerns centralized.

## Typical usage
- Canonical identity:
  - `tsi.wikimyei.representation.vicreg.0x0003`
- Artifact root:
  - `<hashimyei_store_root>/tsi.wikimyei/<family>/<model>/<hashimyei>/...`
- Driver dispatch:
  - register a driver for `canonical_type`
  - call `dispatch_artifact_save(...)` / `dispatch_artifact_load(...)`
