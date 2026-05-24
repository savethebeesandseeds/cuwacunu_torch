# Lattice Operational Readiness V3-M Checkpoint Closure Hardening

Generated: `2026-05-24T00:32:03Z`

## Scope

V3-M resolves the checkpoint/closure proof-authority findings from the final
review. It keeps the existing read-only lattice boundary:

```text
runtime files and sidecars remain source of truth
cache rows remain rebuildable read models
Lattice Hero remains read-only
Runtime Hero remains the executor
```

## Patched Behaviors

Checkpoint byte verification now returns explicit status:

```text
ok = true only when checkpoint bytes can be opened, read, and at least one byte
     is present
open_failed -> checkpoint_identity_failed
read_failed -> checkpoint_identity_failed
empty_file  -> checkpoint_identity_failed
```

Checkpoint closure completeness now requires:

```text
root_producer_found = true
unresolved_input_checkpoints = []
identity_mismatches = []
```

Unknown root checkpoint paths now fail closed:

```text
complete = false
fact_count = 0
resolution_authority = unresolved_lineage
unresolved_input_checkpoints includes requested root checkpoint
```

Checkpoint identity lookup now preserves the requested identity:

```text
checkpoint_closure_result_for_identity(id, digest)
  resolves candidate path
  computes closure through producer evidence
  verifies finalized root_checkpoint_id == requested id
  verifies finalized root_checkpoint_file_digest == requested digest
```

If a stale checkpoint fact shares a path but does not match the current
producer identity, the lookup fails closed:

```text
resolution_authority = checkpoint_identity_failed
identity_mismatches includes resolved root checkpoint identity mismatch
```

Runtime metadata digests are now documented as metadata freshness, not content
proof:

```text
runtime_metadata_digest = path + size + mtime records
watched_file_metadata_digest = watched path + size + mtime records
cache rows remain non-authoritative for target satisfaction
```

Atomic text writes now use unique temp paths and surface rename failures with
source/destination paths.

## Regression Coverage

Focused lattice exposure regressions cover:

```text
missing checkpoint bytes fail checkpoint_identity_failed
empty checkpoint bytes fail checkpoint_identity_failed
stale checkpoint_id/file_digest lookup fails if finalized root identity differs
unknown root checkpoint path is unresolved_lineage, not complete zero-fact
```

Hero smoke for missing root checkpoint:

```text
tool = hero.lattice.checkpoint_closure
checkpoint_path = /cuwacunu/.runtime/cuwacunu_exec/missing.pt
complete = false
fact_count = 0
resolution_authority = unresolved_lineage
identity_mismatches = []
unresolved_input_checkpoints = [/cuwacunu/.runtime/cuwacunu_exec/missing.pt]
```

Hero derived-query smoke:

```text
tool = hero.lattice.derived_query
relation = unresolved_lineage
checkpoint_path = /cuwacunu/.runtime/cuwacunu_exec/missing.pt
value = true
fail_closed = true
witness_count = 1
```

## Verification

```text
make -C src/tests/bench/kikijyeba/lattice_exposure run-test_kikijyeba_lattice_exposure -j12
  passed

make -C src/tests/bench/kikijyeba/lattice_exposure run-test_kikijyeba_lattice_performance_budget -j12
  passed

make -C src/tests/bench/kikijyeba/lattice_target run-test_kikijyeba_lattice_target -j12
  passed

make -C src/main/hero build-lattice-hero -j12
  passed

make -C src/tests/bench/kikijyeba/runtime run-test_hero_mcp_schema_compat -j12
  passed
```

## Verdict

V3-M is satisfied:

```text
- missing/unreadable/empty checkpoint bytes fail closed
- requested checkpoint id/digest remains bound through identity lookup
- unknown root checkpoint paths are unresolved lineage
- metadata digest semantics are documented as freshness-only
- atomic text writes have unique temp names and rename diagnostics
- closure and derived-query surfaces expose fail-closed witnesses
```
