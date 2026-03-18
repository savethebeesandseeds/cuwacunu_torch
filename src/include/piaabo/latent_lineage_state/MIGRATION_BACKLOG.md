# Runtime `.lls` Remaining Backlog

This backlog now tracks the remaining work after the hard-cut runtime `.lls`
migration landed.

## Shared Layer

- Keep the strict runtime `.lls` API small unless a concrete consumer requires a
  richer neutral document/query helper.
- Decide whether any compatibility parser is still worth adding for one-shot
  offline conversion tooling.

## `piaabo::torch_compat`

`src/impl/piaabo/torch_compat/network_analytics.cpp`

- Decide whether the API-only network-design serializer should also be moved onto
  the strict runtime `.lls` path, or remain explicitly outside the persisted
  runtime-artifact contract.
- Keep legacy v1-v3 schema readers until old artifacts are retired.

## Vicreg / Transfer Matrix

`src/include/tsiemene/tsi.wikimyei.representation.vicreg.h`

- Preserve the current matrix artifact as artifact-only unless a later change
  explicitly promotes it to an ingestable runtime schema.

## Hero / Lattice

`src/impl/hero/lattice_hero/lattice_catalog.cpp`

- Keep `wave.projection.lls.v2` as the catalog-cell schema; `wave.source.runtime.projection.v2`
  is now a standalone runtime fact and should not regress back to synthetic
  reconstruction from joined transports.

`src/impl/hero/hashimyei_hero/hashimyei_catalog.cpp`

- Retire legacy fallback handling in non-runtime manifest helpers only if a separate
  manifest standardization pass warrants it.

## Documentation Follow-Ups

- Keep `src/main/hero/README.md` aligned with the real ingest surface and file
  extensions.
- Keep `src/include/piaabo/torch_compat/README.md` aligned with the current emitter
  and identity-envelope rules.
