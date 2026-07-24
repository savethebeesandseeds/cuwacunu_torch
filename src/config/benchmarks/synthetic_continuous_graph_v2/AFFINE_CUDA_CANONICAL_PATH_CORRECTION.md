# Affine CUDA Canonical-Path Correction

Status: preregistered operational correction; no affine execution has been
authorized by this record.

## Scope

The frozen affine scientific program is unchanged.  This correction creates a
new operational runner whose only permitted behavioral difference is a narrow,
exactly verified exception for the two CUDA 12.4 compatibility symlinks during
preflight.  The frozen compiler, linker, and runtime-path arguments remain
byte-for-byte unchanged after those aliases are proven to resolve to the
canonical directories.

The following scientific authorities remain fixed:

- original affine runner:
  `/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/run_frozen_representation_affine_probe_isolated_v2.sh`
  with SHA-256
  `ebdb5b52bd291c40d8d4742b65c6781351223d9e1dcfd51a8036638bf0bc0173`;
- capture-frozen copy of that runner with the same SHA-256;
- live and capture-frozen affine helper with SHA-256
  `45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939`;
- sealed feature-capture development receipt
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/development.status`
  with SHA-256
  `fce8c2383b5040d11ddc1ea9618d52316ea65803442b900d349327d431623fe6`.

No model, feature, target, row, split, seed, candidate grid, metric, threshold,
route rule, selection lock, or evidence boundary is changed.

## Failure and path facts

The original runner requires every component of its CUDA include and library
paths to be non-symlinked.  In the `unnamed_taoist` image these compatibility
paths are symbolic links:

- `/usr/local/cuda-12.4/include` -> `targets/x86_64-linux/include`;
- `/usr/local/cuda-12.4/lib64` -> `targets/x86_64-linux/lib`.

Their exact resolved directories are:

- `/usr/local/cuda-12.4/targets/x86_64-linux/include`;
- `/usr/local/cuda-12.4/targets/x86_64-linux/lib`.

The failed path check therefore diagnosed an operational alias mismatch, not a
missing CUDA installation and not a scientific failure.

## Corrected operational contract

The new runner must:

1. verify the original live runner and capture-frozen runner are byte-identical
   and have the frozen runner SHA-256 above;
2. verify the live and capture-frozen helper are byte-identical and have the
   frozen helper SHA-256 above;
3. verify the sealed feature-capture development receipt has the exact path,
   hash, schema, status, data ranges, probe counts, and no-access declarations
   already required by the original runner;
4. require both compatibility paths to be symlinks and require `realpath -e`
   to resolve them to the exact canonical directories above;
5. require the canonical include and library directories themselves to contain
   no symlink component;
6. retain the original compile helper byte-for-byte, including the compatibility
   include, library, and ELF runtime-path arguments, and verify the alias
   contract immediately before and immediately after every compile;
7. add the operational runner, this correction, both aliases, both resolved
   canonical directories, and the compile/link/runtime paths to development,
   route-trigger, certified execution-contract, and certified result
   provenance; the existing execution-contract checksum carries those bindings
   transitively into each fixed input manifest without changing its inventory;
8. retain every pre-existing receipt field and value needed by the sealed
   feature-capture verifier.
9. snapshot the operational runner's SHA-256, inode, device, byte size, and
   owner at process start; require the runner to be canonical, mode `0555`,
   single-linked, and owned by the executing uid for every non-plan mode; bind
   only that fixed start identity into receipts; and recheck it before and after
   compilation, before contract/status/trigger publication, before manifests,
   sealing, atomic publication, and successful return.

The correction record itself must be mode `0444`, single-linked, owned by the
executing uid, and hash-pinned before any non-plan work begins.  Frozen runner,
helper, capture runner, and capture receipt authorities retain exact owner/link
guards in addition to their content hashes.

The original runner is scientific identity only after this correction.  It is
not rewritten, and the new runner must never represent its own bytes as the
capture-frozen scientific runner.

## Evidence boundaries

This correction does not authorize canonical `data/raw`, final-holdout, policy,
or independent-final-evidence access.  Development remains first, certified
scoring remains conditional on the frozen validation rule, and the untrained
representation control remains development-only.
