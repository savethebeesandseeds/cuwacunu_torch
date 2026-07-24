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

An independent sibling endpoint-bundle sealer must first publish:

`synthetic_v2_representation_ablation_isolated_v2_retry1_endpoint_bundle_for_retry2_v1`

The candidate retry2 runner deliberately contains unresolved, clearly named
SHA-256 constants for that not-yet-final sibling receipt and its checkpoint,
policy, net, training config, and capture config. No non-plan retry2 mode may
run until those constants are replaced with audited lowercase SHA-256 values.

The sibling bundle must:

- bind the interruption closure and its retry1 content inventory;
- verify the complete retry1 endpoint training status, Runtime result,
  representation report, job manifest, checkpoint, and 3,000 optimizer steps;
- copy, rather than hard-link, its payload;
- preserve imported text byte-for-byte, including embedded retry1 paths;
- prove distinct source/copy inode identity, link count one, and byte identity;
- authorize consumption only through a further retry2-local import copy.

Its exact allowlisted historical snapshot contains the 21 regular files and
nine directories of retry1 `arms/endpoint_scale`, rooted at
`source_snapshot`. Retry2 consumes only the immutable bundle receipt and
checkpoint as local copies; the copied configs are read-only equivalence
authorities and are not replayed as retry2-native Runtime output.

Retry2 stage 02 copies only from that sibling bundle into:

`.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_ablation_isolated_v2_retry2/imports/retry1_endpoint_v1`

Both the sibling receipt and checkpoint are copied with
`cp --reflink=never`. Retry2 verifies that the imported checkpoint:

- is byte-identical to the sibling bundle and historical retry1 checkpoint;
- has link count one;
- has a different inode/device identity from both earlier copies.

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
attempt path and SHA-256.

On every invocation, all completed prefix stages are reproduced, verified,
and skipped. An attempt without its completion is terminal. Payload without
its preceding attempt is contamination. Neither state may be adopted,
resumed, repaired, or replayed by the runner. A later stage marker before the
current stage is contamination.

The training stages assert an absent retry2 job, status, result, checkpoint,
and log before publishing their attempt. Time-only and no-TF-alignment each
start from optimizer step zero and execute the original 3,000-step Runtime
training command once.

## Stage 00 preflight and lock boundary

The complete sealed preflight is intentionally expensive and is run once,
before the first stage-00 attempt or scientific retry2 payload.

The preflight may use only the bounded retry2-owned bootstrap scratch
directory immediately under the benchmark runtime parent. The directory must
be canonical, mode `0700`, owned by the executing user, and empty before and
after preflight. An interruption during this read-only preflight does not
consume the scientific retry2 attempt.

The runner file descriptor is the bootstrap lock while no retry2 runtime
exists. After successful preflight, the runner may create exactly:

- the retry2 runtime root; and
- the empty `.development.lock` file.

That root-plus-lock state is the sole permitted re-runnable pre-attempt
bootstrap mutation. If interrupted at that boundary, the next invocation must
prove there is no other descendant and rerun the complete read-only preflight.
The stage-00 attempt is then published immediately before the runtime scratch
directory or any scientific initialization artifact is created.

Every lock verifies its open file descriptor against the canonical lock path
by inode and device while held. Subsequent development invocations use the
retry2 runtime lock. Certified evaluation uses a separate certified lock with
the same descriptor/path identity check.

## Storage safety gates

Every development stage has a resource gate before its attempt and after its
operation, completion, and verification. Certified execution has equivalent
pre/post gates.

The fixed minimums are:

- `/cuwacunu` available bytes: `17179869184`;
- `/` available bytes: `68719476736`.

If `/tmp` shares the `/` filesystem, any regular file larger than
`1073741824` bytes causes a hard failure. The gate reports and rejects; it
does not delete or truncate anything.

After initialization, all retry2 temporary candidates and `TMPDIR` activity
are rooted in the retry2 runtime scratch directory. Read-only stage-00
preflight is the sole bootstrap-scratch exception described above.

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
selection, and development receipt.

## Candidate finalization

This file and the retry2 runner are mutable candidates until independent
scientific, provenance, configuration, and interruption-boundary audits pass.
Candidate validation is limited to syntax, plan output, and static command
block comparison. No preflight, development stage, certified access, freeze,
or publication is authorized by creating this amendment.
