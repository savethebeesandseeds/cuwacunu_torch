# hashimyei

Simple canonical identity provider for `tsi.wikimyei` artifacts.

## Header
- `hashimyei/hashimyei_identity.h`

## What it provides
- Deterministic hex `hashimyei` assignment from a key.
- Shared catalog of known hash names (`0x0`..`0xf`).
- Helper to split fused model/hash tokens:
  - `vicreg_0x3` -> model `vicreg`, hash `0x3`.

## Typical usage
- Canonical identity:
  - `tsi.wikimyei.representation.vicreg.0x3`
- Train artifact:
  - `tsi.wikimyei.representation.vicreg.0x3.jkimyei`
- Endpoints:
  - `...@payload:tensor`
  - `...@weights:tensor`
