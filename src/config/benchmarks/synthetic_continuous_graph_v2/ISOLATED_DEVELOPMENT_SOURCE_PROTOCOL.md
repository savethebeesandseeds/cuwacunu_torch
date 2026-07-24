# Isolated development-source implementation protocol

Status: fixed on 2026-07-16 after the physical-source exposure was found and
`DEVELOPMENT_SOURCE_ISOLATION_AMENDMENT.md` was sealed, but before any clean
mirror, mirror-local cache, clean checkpoint, clean feature probe, or clean
validation result existed. The quarantined raw-source runs cannot alter this
transport contract.

## Fixed artifact identities

The isolated source runtime is:

```text
.runtime/benchmarks/synthetic_continuous_graph_v2/
  synthetic_v2_isolated_development_source_v1/
```

Within it, `source/` is the only production source root,
`config/ujcamei.source.registry.development_prefix.dsl` is the only source
registry, `config/synthetic_benchmark.isolated_development.config` is the
base production config, and `development_source_closure.status` is the
immutable closure receipt. Its schema is
`synthetic_v2_isolated_development_source_v1`.

Clean downstream receipts use these fixed runtime identities:

```text
synthetic_v2_representation_train_isolated_v2
synthetic_v2_mdn_train_isolated_v2
synthetic_v2_frozen_feature_capture_isolated_v2
synthetic_v2_frozen_affine_development_isolated_v2
synthetic_v2_frozen_representation_affine_probe_isolated_v2
synthetic_v2_representation_ablation_isolated_v2
```

No clean runner may fall back to an earlier result path.

## Allowed source construction

Exactly nine regular, non-symlinked CSV inputs are allowed: the three fixed
instruments by `1d`, `3d`, and `1w` under the sealed benchmark
`data/development_prefix`. Their byte hashes and sizes must match the prefix
entries already recorded in `fresh_seed_data_closure.v2.report`. The old
closure report may be read and hash-bound as a document, but its verifier must
not be invoked: that verifier follows and rehashes canonical raw entries,
including the operationally exposed final interval.

The preparation runner copies only those nine verified CSVs into an atomic
runtime candidate. It rejects every symlink, extra CSV/cache, canonical
`data/raw` string, source outside the sealed prefix, and destination outside
the fixed runtime root. The local registry is derived from the canonical v2
registry by changing only `SOURCE_ROOT` to the absolute candidate `source/`
path. The base config is derived from the already fixed v2 production config
by changing only its registry path to that local registry and making authored
paths absolute.

Runtime then performs one zero-optimizer-step, no-checkpoint, no-replay,
force-cache-rebuild production dry run on train `[0,2496)`. It must report
exactly 3,264 accepted and candidate anchors, maximum available anchor 3,263,
and source receipts exclusively under the candidate mirror. Any `data/raw`
receipt or accepted count 4,096 is fatal.

The resulting nine raw and nine causal `log_returns` caches must pass strict
CSV/raw/normalized freshness. There must be exactly nine CSVs and eighteen
caches. The final source tree, registry, and config are made read-only before
atomic installation. The immutable receipt binds every input prefix CSV,
mirror CSV, mirror cache, registry, config, Runtime executable, cache guard,
dry-run manifest/result, both source-isolation documents, and this protocol.

The isolated verifier is read-only and verifies only the sealed prefix inputs
and isolated runtime artifacts named by its own receipt. It must reject paths
containing `/data/raw/` and must never invoke the old full-source closure
verifier.

## Downstream enforcement

Every clean representation, MDN, capture, affine, and ablation runner binds
the isolated closure and this protocol. Production job validators require
`accepted_anchor_count=3264`, exact authorized cursor bounds, and all
`source_file_receipts` under the isolated `source/` root. They reject
canonical registry/config paths, raw-source receipts, symlinks, an anchor end
above 3,264, and every old representation or MDN receipt.

The canonical representation and MDN restart at optimizer step zero with all
scientific choices unchanged. Development capture remains train/validation
first, and the already fixed raw-96 gate remains the sole conditional route.
No clean V2 operation authorizes independent final evidence.
