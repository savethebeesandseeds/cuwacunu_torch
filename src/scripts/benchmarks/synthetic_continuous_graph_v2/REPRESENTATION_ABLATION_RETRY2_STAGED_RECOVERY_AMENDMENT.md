# Representation Ablation Retry2 Staged Recovery Amendment

## Status and scope

This amendment defines the only permitted operational recovery from the
interrupted
`synthetic_v2_representation_ablation_isolated_v2_retry1` development
attempt.

The new schema is:

`synthetic_v2_representation_ablation_isolated_v2_retry2`

This amendment changes orchestration and provenance only. It does not change
the preregistered representation policies, network substitutions, training
seed, optimizer-step target, source ranges, feature captures, affine
evaluator, comparator, selection order, gate thresholds, or certified
evaluation. The retry2 runner must preserve the retry1 scientific command
blocks except for the checkpoint-authority branch required by the endpoint
import described below.

Retry1 remains immutable and terminal. Retry2 must not resume, relaunch, edit,
chmod, hard-link, or publish an in-place receipt under the retry1 runtime.

## Retry1 interruption evidence

The retry1 boundary is the independently sealed sibling closure:

`synthetic_v2_representation_ablation_isolated_v2_retry1_interruption_closure_v1`

Its immutable closure receipt SHA-256 is:

`e6c845233f3f434a9c46bead1b9fb825217492a5da7ae0a95174fc10b15e1117`

The sealed retry1 content-inventory SHA-256 is:

`6a677ec3c7f5da7907cfc624ab280ad93b703a06da6c7febb1b8c8a80e97ef05`

The closure records 49 regular files, 64,939,302 regular-file bytes, 25
directories, no symbolic links, no special entries, and two identical
snapshots taken while the retry1 development lock was held.

The closure is operational evidence, not scientific evidence. The observed
Docker VM I/O failure and runaway `/tmp` log are not Runtime-emitted model
evidence and do not establish an exact process stop, exit status, model
failure, or causal attribution.

The closure establishes:

- retry1 re-entry is not authorized;
- partial-artifact reuse and checkpoint resume are not authorized;
- endpoint-scale completed 3,000 optimizer steps and has a complete immutable
  training authority;
- time-only is incomplete, has no training status or Runtime result, and its
  observed 2,880-step partial state is terminal;
- no-TF-alignment was not started;
- no challenger capture, affine result, cross-arm selection, development
  receipt, certified artifact, or final result exists.

## Endpoint-scale import boundary

The retry1 endpoint checkpoint is not a direct retry2 input.

The independently audited sibling endpoint-bundle sealer has published:

`synthetic_v2_representation_ablation_isolated_v2_retry1_endpoint_bundle_for_retry2_v1`

The exact immutable authorities are:

- sealer SHA-256:
  `b2edac9ef89d2ff630a5dbf33c041f2d3016c3fffbe74a66f5a5c38975d01a77`;
- amendment SHA-256:
  `c94c282d93844563f83abf3e1826111e14d640370d38e778bee04070aa1303ad`;
- receipt SHA-256:
  `ff675afc779b106f628f3ea65fe3409314bf6ea29a531100e73dfa1a3cca9f96`;
- regular-file inventory SHA-256:
  `78171e0900f9e034642a85320dbcabb2b52f57eed40a73ca55a973c5f65efc6d`;
- directory inventory SHA-256:
  `8e23a668bad459c9effdd89d45d4c6c461f546daf544d06a7e4ae9b653c9e6ec`;
- endpoint content-inventory SHA-256:
  `bd2f8d55b4e3e3a3a06bf14749b28ea0bec01ea9c07aaa1c1628e9ed4f59e13f`;
- checkpoint SHA-256:
  `09c286c5374e4769feb19644c3efa26aa081e37620f1eb5acf3bd9cf534b26ec`;
- policy SHA-256:
  `c1898f3a7aaa5183a8e6e0341f8dbbdc087456ee7f8701175ea70720d682f4d8`;
- net SHA-256:
  `42a078766e0dfdb8f0074b69d3dc1eacb63f52ec806dd3b4355b3b280c02593e`;
- training-config SHA-256:
  `c517ef409c1829413d18536851aecc48bb94e7f7ef2ed1386106511ee1e3ef28`;
- capture-config SHA-256:
  `63e042f47bbdbc2970cd8afbfcc639fcec5a7a980aacbf7a59d8db592f97f821`.

The sibling bundle must:

- bind the interruption closure and its retry1 content inventory;
- verify the complete retry1 endpoint training status, Runtime result,
  representation report, job manifest, checkpoint, and 3,000 optimizer steps;
- copy, rather than hard-link, its payload;
- preserve imported text byte-for-byte, including embedded retry1 paths;
- prove distinct source/copy inode identity, link count one, and byte identity;
- authorize consumption only through a further retry2-local import copy.

The retry2 runner verifies this authority itself. It must not invoke the
sibling sealer, take a retry1 lock, or reopen the original retry1 endpoint
checkpoint or payload. It verifies an exact tree of 26 regular files and 11
directories, including an exact allowlisted historical snapshot of 21 regular
files, nine directories, and 32,731,999 regular-file bytes rooted at
`source_snapshot`. The regular and directory inventory hashes are fixed as
above. No symbolic link or special entry is permitted. Every file must be
owned by the executing user, mode `0444`, link count one, and on the bundle
device; every directory must have the same owner and mode `0555`. Each of the
21 receipt-listed relative paths must be safe and unique and must reproduce
its recorded SHA-256 and byte count.

The sealed inventories and receipt carry the historical source identity
transitively, including its checkpoint inode/device tuple. Retry2 consumes
only the immutable bundle receipt and checkpoint as local copies; the copied
configs are read-only equivalence authorities and are not replayed as
retry2-native Runtime output.

Retry2 stage 02 copies only from that sibling bundle into:

`.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_ablation_isolated_v2_retry2/imports/retry1_endpoint_v1`

Both the sibling receipt and checkpoint are copied with
`cp --reflink=never`. Retry2 verifies that the imported checkpoint:

- is byte-identical to the sibling bundle checkpoint and carries the pinned
  historical checkpoint hash;
- has link count one;
- has a different inode/device identity from the bundle copy and from the
  historical source tuple recorded in the sealed inventory.

Retry2 also proves endpoint scientific/config equivalence. Policy and net are
byte-compared. Training and capture configs are compared after normalizing
only their endpoint-local policy and net path values; all other authored
content must remain identical. The existing reverse-substitution verifier
continues to prove that the retry2 endpoint arm changes only
`TIME_SCALES=8,16,32,64` to `TIME_SCALES=8,16,32,1`.

The retry2 endpoint import receipt must report:

- historical source optimizer steps: 3,000;
- sibling bundle copy optimizer steps: 0;
- retry2 import optimizer steps: 0;
- retry2 training job created: false;
- retry2 training status created: false;
- retry2 Runtime result created: false;
- retry2 checkpoint resume: false.

No endpoint training job, training status, Runtime result, or synthetic
training history may be fabricated under retry2. All downstream endpoint
capture, selection, and certification must resolve the checkpoint through the
retry2-local import receipt.

## Fixed development stages

`--run-development` is disabled. Each `--advance-development` invocation may
execute exactly one stage and must then exit.

The fixed order is:

1. `00 initialize`
2. `01 canonical_import`
3. `02 endpoint_import`
4. `03 time_only_training`
5. `04 no_tf_alignment_training`
6. `05 endpoint_scale_capture`
7. `06 time_only_capture`
8. `07 no_tf_alignment_capture`
9. `08 endpoint_scale_affine`
10. `09 time_only_affine`
11. `10 no_tf_alignment_affine`
12. `11 selection_and_development`

Every stage has one immutable attempt receipt and one immutable completion
receipt. Each receipt contains the exact two-digit ordinal and stage name.
Stage 00 records no predecessor. Every later attempt and completion records
the exact previous completion path and SHA-256. Each completion also binds its
attempt path and SHA-256. Before publishing a completion, the runner must
semantically reproduce and verify that stage's complete payload and record
`pre_completion_semantic_verification_pass=true`. It must then reproduce the
same verification after publication. A stage is not complete merely because
its command returned successfully.

On every invocation, all completed prefix stages are reproduced, verified,
and skipped. An attempt without its completion is terminal. Payload without
its preceding attempt is contamination. Neither state may be adopted,
resumed, repaired, or replayed by the runner. A later stage marker before the
current stage is contamination.

Completed-prefix reproduction executes in the main shell, not a
command-substitution subshell. Bash `errexit` is inherited by every remaining
command substitution, and runtime/bootstrap scratch is proved empty
immediately after prefix reproduction and before any next-stage attempt is
published.

The training stages assert an absent retry2 job, status, result, checkpoint,
and log before publishing their attempt. Time-only and no-TF-alignment each
start from optimizer step zero and execute the original 3,000-step Runtime
training command once.

Stage 01 treats the canonical arm root and its affine directory as explicit
payload: both must be absent before the attempt, are created only afterward,
and are semantically reverified as executing-user-owned mode-`0700`
directories with the imported immutable reports and receipt.
Stage 02 applies the same rule to the retry2 `imports` parent and endpoint
import root before publishing the two local immutable copies and import
receipt.

The immutable chain is deliberately noncircular. The development receipt
published by stage 11 binds completion receipts 00 through 10 and the stage-11
attempt receipt, but not its own not-yet-existing completion. The stage-11
completion then binds that development receipt in its semantically verified
payload. A later certified attempt binds all 12 completion paths and hashes,
the count 12, and the stage-11 completion as the development-chain head.

## Stage 00 preflight and lock boundary

The complete authoritative preflight is intentionally expensive and is run
exactly once inside any invocation that is permitted to publish the first
stage-00 attempt, before that attempt or any scientific retry2 payload.
`--preflight` is an optional read-only rehearsal: it is unrecorded,
repeatable, creates no authority, and is never adopted in place of the
authoritative preflight. Therefore a later consuming stage-00 invocation must
run the authoritative preflight even after a successful rehearsal.

The preflight may use only the bounded retry2-owned bootstrap scratch
directory immediately under the benchmark runtime parent. The directory must
be canonical, mode `0700`, owned by the executing user, and empty before and
after preflight. An interruption during this read-only preflight does not
consume the scientific retry2 attempt.

The runner file descriptor is the bootstrap lock while no retry2 runtime
exists. After successful preflight, it creates a mode-`0700` candidate runtime
root containing only an empty mode-`0600`, link-count-one `.development.lock`
inside bootstrap scratch. It opens and exclusively locks that candidate file,
verifies its metadata, and atomically renames the root into its final canonical
path while retaining the open descriptor. It then proves descriptor/path
inode and device identity. No separately visible empty runtime-root window is
permitted.

That exact root-plus-lock state is the sole permitted re-runnable pre-attempt
bootstrap mutation. If interrupted at that boundary, the next invocation must
prove the canonical root and lock metadata, prove there is no other
descendant, and rerun the complete read-only preflight. A residual candidate
root or any extra descendant is contamination. The stage-00 attempt is then
published immediately before the runtime scratch directory or any scientific
initialization artifact is created.

Every lock verifies its open file descriptor against the canonical lock path
by inode and device while held. Subsequent development invocations use the
retry2 runtime lock. Every verification or certified mode that shares runtime
scratch also holds that development lock for its full invocation. Certified
evaluation acquires its separate certified lock only after the development
lock, and applies the same descriptor/path identity check; that certified lock
must also be empty, executing-user-owned, mode `0600`, and link count one.
Lock metadata and descriptor identity are rechecked after child execution and
before immutable stage-completion or certified-result publication.

## Storage safety gates

Every development stage has a resource gate before its attempt and after its
operation, completion, and verification. Certified execution has equivalent
pre/post gates.

The fixed minimums are:

- `/cuwacunu` available bytes: `17179869184`;
- `/` available bytes: `68719476736`.

An unconditional, filesystem-bounded traversal of `/tmp` is required on every
gate. Any regular file larger than `1073741824` bytes causes a hard failure,
regardless of whether `/tmp` shares the `/` filesystem. Failure to traverse is
also terminal. Device identity is recorded as evidence only. The gate reports
and rejects; it does not delete or truncate anything.
`/cuwacunu`, `/`, and `/tmp` must first be canonical, non-symbolic-link
directories so the measured and traversed objects are the named roots.

Before the canonical runtime exists, preflight and the atomic root candidate
use only the retry2-owned bootstrap scratch. After stage 00 is complete, the
runtime scratch is selected before completed-prefix or receipt reproduction.
Each later invocation also re-establishes and verifies its shell-local
canonical input/path bindings before that reproduction; receipt persistence
must never be mistaken for shell-variable persistence.
All later retry2 candidates, comparison files, and `TMPDIR` activity are
contained there. The relevant scratch must be empty at stage boundaries;
residue is contamination and is never adopted or cleaned automatically.
Every non-plan entry and successful exit also requires bootstrap scratch to be
absent or the exact empty mode-`0700`, executing-user-owned directory. All
scratch and root enumeration must propagate traversal failure rather than
treating it as emptiness. Operational absence means neither an existing entry
nor a dangling symbolic link; every component is checked for symbolic links
before directory creation, log redirection, or immutable publication.
For stages with sequential children, the second child's job, report, and log
destinations are rechecked after the first child exits. Every child-produced
file is then proved canonical, regular, executing-user-owned, and link count
one before any `chmod` seals it.

## Scientific invariants

Retry2 retains:

- seed `17`;
- challenger target `3000` optimizer steps;
- endpoint-scale-only substitution:
  `TIME_SCALES:8,16,32,64 -> 8,16,32,1`;
- time-only substitution:
  `USE_FREQUENCY_TOKENS:true -> false`;
- no-TF-alignment substitution:
  `LAMBDA_TF_ALIGN:0.10 -> 0.00`;
- train anchors `[0,2496)`;
- validation anchors `[2560,2816)`;
- certified-development anchors `[2880,3261)`;
- comparator order: validation direction, rank, correlation, RMSE;
- tie tolerance `1e-12`;
- tie preference: canonical, endpoint-scale, time-only, no-TF-alignment;
- main/replay byte identity;
- one immutable selection before certified access;
- one selected-arm certified attempt;
- no policy access.

Development may not read above anchor 2815. Certified-development may not
read above anchor 3260. The independent final range `[3328,4096)` remains
forbidden and unopened.

Before stage 11 completes, every local certified attempt, job, capture,
report, log, and result path must be absent. Certified execution remains a
separate command and requires the entire immutable development stage chain,
selection, and development receipt. While holding the certified lock, the
runner re-verifies all 12 completion receipts and their semantic payloads,
rechecks the lock descriptor against its canonical path, and runs a fresh
resource gate immediately before publishing the single certified attempt.
The certified capture and result bind that attempt transitively. A final
resource gate and full verification are required before success.
After the certified attempt exists, later completed-development reproduction
replaces the historical absence assertion with verification that this
immutable attempt binds all 12 stage completions. Certified payload without
that attempt remains contamination.

## Candidate finalization

This file and the retry2 runner are mutable candidates until independent
scientific, provenance, configuration, and interruption-boundary audits pass.
Candidate validation is limited to syntax, plan output, and static command
block comparison. No preflight, development stage, certified access, freeze,
or publication is authorized by creating this amendment. Once all independent
audits pass, the amendment is frozen mode `0444`, its exact SHA-256 is pinned
by the mode-`0555` runner, and those frozen identities define the executable
retry2 boundary.
