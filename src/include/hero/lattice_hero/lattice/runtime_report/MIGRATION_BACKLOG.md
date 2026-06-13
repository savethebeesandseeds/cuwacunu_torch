# Runtime `.lls` Remaining Backlog

This backlog now tracks the remaining work after the hard-cut runtime `.lls`
migration landed.

## Shared Layer

- Keep the strict runtime `.lls` API small unless a concrete consumer requires a
  richer neutral document/query helper.
- Decide whether any compatibility parser is still worth adding for one-shot
  offline conversion tooling.

## `jkimyei::evaluation`

`src/include/jkimyei/evaluation/evaluation_report_identity.h`

- Active evaluation emitters now use the current `evaluation_report_identity_t`
  envelope rather than Tsiemene report identities.
- Keep source-load, network-capacity, and capacity-comparison artifacts on the
  strict runtime `.lls` profile.
- Decide whether the API-only network-design serializer should also be promoted
  to a persisted runtime artifact, or remain outside the runtime-artifact
  contract.

## Archived Vicreg / Transfer Matrix Artifacts

- Preserve the current matrix artifact as artifact-only unless a later change
  explicitly promotes it to an ingestable runtime schema.
- These are archived compatibility surfaces. Do not use them as the authored
  graph-first Wikimyei `.lls` path.

## Hero / Lattice

`src/include/hero/lattice_hero/lattice/exposure/exposure_ledger.h`

- Keep source analytics `.lls` fallback on the strict runtime parser and the
  canonical v2 filename set only.
- Keep runtime report ingest owned by Lattice. Retired catalog/source projection
  source paths must not be reintroduced as compatibility layers.

## Documentation Follow-Ups

- Keep `src/main/hero/README.md` aligned with the real ingest surface and file
  extensions.
- Keep `src/include/jkimyei/evaluation/README.md` aligned with the current emitter
  and identity-envelope rules.
