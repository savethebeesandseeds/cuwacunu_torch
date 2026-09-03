# GPV-1 cache-codec recovery v2

Date: 2026-08-31

Status: sealed mechanical erratum to
`GLOBAL_POOL_PROJECTOR_VARIANCE_CAUSAL_DECOMPOSITION_PROTOCOL.md`.

This erratum supersedes only the cache mechanics in original protocol lines
249–267. It changes only the resumable-cache codec and evidence filenames. It
does not change the representation module, factor definitions, initial states,
data, rows, views, seeds, optimizer, update counts, metrics, thresholds,
classification, confirmation gate, or rollback.

## Failed v1 attempt

The first authoritative attempt trained seed 17, mask 1 for all 512 updates and
atomically committed this pair:

- archive:
  `.build/tests/gpv1/seed_17_mask_1_v1.complete.pt`
- archive size: `449779` bytes
- archive SHA-256:
  `202e0cc06372624884673ebd821f3dd05eb04141f6b93c688431ef1b73182ffd`
- marker-file SHA-256:
  `476c247ceffb2ddf500bce7d9f3cfd13c0d16adf6013bafd7af1c1fa120f4c79`

The archive and marker have valid byte integrity and the archive contains all
declared keys. Mandatory immediate reload stopped before semantic receipt
validation with:

```text
setStorage: sizes [4096], strides [1], storage offset 0, and itemsize 8
requiring a storage size of 32768 are out of bounds for storage of size 4096
```

The attempt used these bound artifacts:

- protocol SHA-256:
  `01c6b1d9fcc95a0c831426a481c866cb196f413030d2f3c195b5219d84d57a2a`
- harness SHA-256:
  `65a596001a71bd06ac5aa0fd3e6154e661fdcf956dbe74f9a4cebcf8f6ce11fd`
- executable SHA-256:
  `11eb5d1f8f07529af11ef45b28122b9c96f53b6b92e9f069be27f27ece6d06ac`
- Makefile SHA-256:
  `b736c445701098229dbda38481817b13325dc1d3e75d1753a981a253a912f702`
- build-receipt SHA-256:
  `dbc3c7aba89680900a0f29425b38dae160c823586330a54c538bfad161ca3913`
- full-dependency-file SHA-256:
  `658d59d4117e65a528d71ce0ac801bc1a78d9d83d3a7b0fa2365da336c179849`

No representation-quality endpoint was evaluated, no candidate was selected,
and no authoritative v1 result log or checksum was committed. The v1 pair is
forensic failure evidence only. It must remain untouched and must never be
loaded, converted, migrated, counted, or used by the recovered experiment.

## Root cause

The loader reused one already-defined `torch::Tensor` destination across
incompatible archive dtypes. LibTorch's `InputArchive` uses `set_` when that
destination is already defined on the requested device. The destination kept
its previous Float64 scalar type while receiving UInt8 storage for the first
4096-byte hash vector, producing the exact 32768-versus-4096-byte exception.
Later UInt8/Int64/Float64 transitions were unsafe as well.

This is a loader-only defect. It occurred after training and archive commit,
but before mandatory semantic replay; therefore the v1 cell is not accepted as
scientific evidence.

## Recovery contract

1. Preserve the original protocol and its SHA-256 unchanged.
2. Bind this erratum and its checksum as required scientific custody.
3. Use a fresh undefined tensor for every `InputArchive` tensor read. Check
   each archived dtype and shape exactly before converting it to host vectors:
   Float64 `[512]` for each loss/gradient/component vector, UInt8 `[4096]`
   for each 512-entry hash vector, Int64 `[512]` for step flags, Float64
   `[26]` for statistics, Int64 `[4]` for counts, Int64 `[5]` for flags, and
   the exact declared parameter-inventory length for parameter deltas. String
   and scalar metadata also require their exact stored dtypes and shapes.
4. Add an in-memory OutputArchive/InputArchive round-trip test using the same
   reader in this exact transition order: Float64, UInt8, Int64, Float64,
   Int64. Require exact dtype, shape, and value recovery.
5. Use only cache schema `gpv1.cell_cache.v2`, implementation
   `retained_oca_views_single_graph_atomic_cell_fresh_tensor_codec_v2`, and
   filenames
   `.build/tests/gpv1/seed_<seed>_mask_<mask>_v2.complete.pt`.
6. Start every one of the 21 trainable cells from its certified frozen anchor.
   Do not reuse the v1 payload. The current mask-0 evidence remains the same
   frozen reference specified by the original protocol.
7. Use authoritative log
   `.build/tests/representation_gpv1_v2_authoritative.log` and its SHA-256
   marker. A failed run may preserve a distinctly named diagnostic log, but it
   has no scientific authority and must not use the authoritative filename.
8. Keep the original 10,752-new-update ceiling. The failed v1 cell's 512
   updates are excluded from results and are not represented as resumed or new
   v2 updates.

## Stop gates before v2 training

- The preserved v1 archive and marker must match the exact hashes above.
- Static checks, a clean rebuild, the mixed-dtype archive round-trip, all prior
  CPU mechanics checks, and the complete 24-update disposable CUDA preflight
  must pass against one exact v2 executable and manifest.
- Preflight must report zero authoritative optimizer updates and must create no
  v2 cell archive or authoritative result log.
- Before any resumed update, globally inventory and fully validate every
  existing v2 committed pair; do not train an earlier missing cell before a
  later invalid committed pair has been rejected.
- An independent audit must confirm the loader uses a fresh destination for
  every archive tensor, v1 is excluded, v2 custody is fail-closed, and the
  scientific design is otherwise unchanged.

Only after all gates pass may the full seven-mask by three-seed attribution be
restarted. Production defaults remain unchanged throughout. The operational
rollback remains `all_tokens`; the certified representation rollback remains
`fspa4_structured_cdsb_sparse_v1`.
