# Representation ablation retry1 operational-interruption recovery amendment

Status: fixed after the one-shot retry1 development process stopped making
observable progress, after the endpoint-scale arm completed, and before the
time-only arm completed. This is an operational recovery amendment. It changes
no data, model, augmentation, objective, seed, optimizer, range, metric,
threshold, comparator, tie rule, selection rule, or certification rule.

## Retained retry1 authority

The retained runtime is
`/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_ablation_isolated_v2_retry1`.
It is interruption evidence and must not be edited, sealed in place, resumed,
re-entered, or used directly by a later experiment.

The external closure binds the complete retained tree:

- forty-nine regular files;
- twenty-five directories;
- 64,939,302 total regular-file bytes; and
- complete regular-file content inventory SHA-256
  `6a677ec3c7f5da7907cfc624ab280ad93b703a06da6c7febb1b8c8a80e97ef05`.

The inventory digest is computed with
`find . -xdev -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum |
sha256sum`, using `./`-relative names from the retry1 runtime root. The closure
also records exact regular-file and directory metadata inventories and rejects
every cross-device entry. It must acquire the retry1 `.development.lock`
read-only, prove that the locked file descriptor and the canonical lock path
retain the same device and inode, and hold and recheck that identity while
taking two identical exact snapshots. Symbolic links, special entries, and
regular files with a link count other than one are forbidden.

## Observed execution boundary

The retry1 attempt sentinel is consumed and forbids another retry1 development
entry.

The endpoint-scale arm has an immutable `training.status` with `status=complete`,
a Runtime result with `status=completed`, and a representation report with
3,000 completed optimizer steps. Its retained checkpoint is the checkpoint
named by those artifacts and has SHA-256
`09c286c5374e4769feb19644c3efa26aa081e37620f1eb5acf3bd9cf534b26ec`.
This makes the endpoint-scale arm a completed retained arm, not a partial arm.
It does not authorize direct use of the checkpoint or result. Endpoint import
eligibility is
`requires_separate_retry2_import_verifier`: a retry2 design may import it only
through a new verifier that independently authenticates an allowlisted endpoint
bundle, excludes every time-only artifact, and proves exact equivalence to the
retry2 endpoint scientific and configuration contract. Import must make a
byte-identical copy whose device/inode identity differs from the source and
whose link count is one. Retry2 may consume only that retry2-local copy, never
the retained retry1 path or a hard link to it.

The latest retained time-only report records 2,880 attempted, completed, and
optimizer steps. It reports its last checkpoint optimizer step as 2,850. The
requested run length was 3,000 steps, and neither time-only `training.status`
nor a time-only Runtime result exists. The time-only arm is incomplete. Its
report, checkpoint, probes, manifest, log, and all other partial artifacts are
interruption evidence only and are not reusable training or scientific inputs.

The no-TF-alignment arm has only its prepared configuration files; it has no
training directory, training log, training status, Runtime result, or
checkpoint. Challenger captures, affine probes, cross-arm selection,
development completion, certified evaluation, and final result artifacts are
absent.

## External operational evidence

During recovery, the operator observed a runaway `/tmp` log with inode 161829
in the Docker VM and `sdd` I/O errors reported by that VM. Those observations
are external operational evidence. They are not artifacts emitted by retry1,
not model or metric evidence, and not evidence that any representation setting
caused the interruption. They do not establish an exact process stop time,
process exit status, or process-liveness history. The closure may record this
evidence only with that scope.

## External immutable closure

The verifier
`seal_and_verify_representation_ablation_retry1_interruption_closure_v1.sh`
publishes only the separate sibling runtime
`synthetic_v2_representation_ablation_isolated_v2_retry1_interruption_closure_v1`.
It must not chmod, touch, rename, link into, add a receipt to, or otherwise
mutate retry1. Publication uses an atomic no-clobber directory rename. The
closure contains frozen copies of this amendment and its verifier, the two
exact source inventories, and `interruption_closure.status`; closure files are
mode 0444 and closure directories are mode 0555.

An existing closure is accepted only when its exact allowlisted tree, frozen
sources, receipt, inventories, and the unchanged retained retry1 runtime all
verify. Retry1 re-entry and partial-artifact reuse remain forbidden.

## Retry2 rule

A fresh retry2 must use the distinct schema
`synthetic_v2_representation_ablation_isolated_v2_retry2`. The time-only and
no-TF-alignment arms restart from optimizer step zero. No retry1 time-only
checkpoint, report, optimizer state, or other partial artifact may enter
retry2.

The completed endpoint-scale arm may not be consumed directly. If retry2 elects
to avoid recomputing it, a separate retry2 endpoint-import verifier and receipt
must authenticate an explicit endpoint-only bundle, prove the bundle complete
at 3,000 steps, prove that no time-only path or artifact is imported, prove
exact retry2 endpoint scientific/configuration equivalence, make and verify a
byte-identical non-hardlinked copy with a distinct device/inode identity, limit
retry2 consumption to that local copy, and bind that decision before any
retry2 challenger execution. Without that verifier, the endpoint-scale arm
also restarts from step zero.

Canonical raw data, certified inputs, the final holdout, and policy artifacts
remain outside this recovery amendment.
