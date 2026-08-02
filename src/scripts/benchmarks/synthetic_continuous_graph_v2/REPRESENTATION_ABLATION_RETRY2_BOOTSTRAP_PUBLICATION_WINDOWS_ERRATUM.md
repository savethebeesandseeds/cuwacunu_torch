# Representation Ablation Retry2 Bootstrap Publication Windows Erratum

## Status and scope

This erratum is an additive operational authority for the retry2 bootstrap
publication failure observed before any retry2 stage-attempt receipt or
scientific payload existed. It does not replace or weaken the scientific
contract in
`REPRESENTATION_ABLATION_RETRY2_STAGED_RECOVERY_AMENDMENT.md`.

The sealed retry2 runner covered by the failed bootstrap has SHA-256

`84ce29197961a232887290f045050fa06316652cde31651f6e930b302aec69ba`.

The sealed staged-recovery amendment has SHA-256

`414211345e95965f52d8a0ceb672b5efff74b2c495d67619ca2b3ac788060591`.

The observed failure is transcribed, without stream or redirection
attribution, in
`REPRESENTATION_ABLATION_RETRY2_BOOTSTRAP_PUBLICATION_FAILURE_OBSERVATION.txt`.

## Failure classification

The complete authoritative read-only stage-00 preflight finished. The runner
then created the prescribed private bootstrap scratch directory and candidate
runtime root containing only the empty `.development.lock`. The directory
rename returned `Permission denied` on the Windows-backed `/cuwacunu` mount
while that descendant lock file had an open descriptor. Those facts were
observed together; this erratum does not claim an independently established
operating-system cause beyond that observation.

The process exited. The canonical retry2 runtime root remained absent. No
stage-00 attempt or completion receipt, runtime scratch, configuration input,
arm, import, selection, certified artifact, result, Runtime job, optimizer
state, checkpoint, probe, or model metric was created. Consequently:

- `scientific_attempt_consumed=false`;
- `optimizer_steps=0`;
- `candidate_adopted=false`.

The remaining private candidate is operational failure evidence. Consistent
with the staged-recovery amendment, it is contamination and must never be
renamed into, copied into, or otherwise adopted as the canonical retry2
runtime.

## One-shot quarantine authority

The only mutation authorized by this erratum is execution of the separately
sealed
`seal_and_quarantine_representation_ablation_retry2_bootstrap_publication_failure_v1.sh`
against the exact observed residue.

Before mutation, that sealer must acquire an exclusive nonblocking `flock` on
the old sealed runner file and prove the descriptor still identifies that
file. It must bind its own process-start hash and metadata and continuously
recheck its identity. It must verify the pinned old runner and amendment and
the pinned erratum and observation. It must then prove all of the following:

- the canonical runtime root and every named retry2 stage or payload path are
  operationally absent;
- the bootstrap scratch, candidate root, and empty lock are canonical,
  nonsymbolic paths on device `66` with their pinned types, modes, owner,
  link counts, sizes, and inodes;
- the exact residue tree consists of two directories and one empty regular
  file, with no symlink, mount crossing, or special entry;
- the candidate and benchmark runtime parent share device `66`;
- no process visible in the container holds a file descriptor, working
  directory, root, or executable reference at or below the residue path.

While the old-runner bootstrap lock remains held, the sealer may create the
private sibling candidate for
`synthetic_v2_representation_ablation_isolated_v2_retry2_bootstrap_publication_failure_closure_v1`.
It must atomically move the whole bootstrap scratch directory with
`mv -T -n` to that closure candidate's `residue` path. It must prove source
absence and inode/device continuity. This move is quarantine, not scientific
adoption.

The closure must contain deterministic regular-file and directory inventories,
an immutable `failure.status`, frozen copies of the sealer and all four bound
authority/evidence sources, and the quarantined residue. The receipt must bind
the before-state, quarantine transition, sealed after-state, and the explicit
facts above. Every closure regular file is sealed mode `0444`; every closure
directory is sealed mode `0555`. The private closure candidate is published
to its canonical sibling path by an atomic, no-clobber directory rename with
source-absence and inode/device postconditions.

No recursive deletion, cleanup, resume, partial adoption, scientific runtime
publication, or stage-attempt publication is authorized. Any mismatch or
interruption is terminal and must leave evidence in place for a new explicit
review.

## Subsequent runner correction

This quarantine authority does not make the old retry2 runner executable
again. A revised runner requires a separate immutable authority and fresh
independent audits. That revision must preserve every scientific command,
seed, range, comparator, endpoint import, and twelve-stage invariant while
changing only bootstrap publication and its transitive evidence bindings.

The revised operational sequence must serialize every non-plan mode with the
bootstrap runner lock. It may verify and lock a fresh private candidate, close
only the candidate lock descriptor while retaining the bootstrap lock, perform
the same-device no-clobber rename, immediately reopen and exclusively lock the
canonical `.development.lock`, and prove descriptor/path inode and device
identity before publishing stage 00. If canonical lock reacquisition fails,
the exact root-plus-lock state remains pre-attempt and no receipt or payload
may be published. The complete authoritative preflight must be rerun with a
fresh candidate after this failure closure is sealed.

Before closing the candidate descriptor, the revised runner must publish a
fixed, owned, link-count-one, mode-`0600`, nonempty publication-in-progress
guard in bootstrap scratch. The guard binds the revised runner, candidate and
canonical paths, and the captured root/lock inode and device tuples. It remains
present through rename, canonical read-only reopen and `flock`, exact
root-plus-lock enumeration, descriptor/path verification, and inode/device
continuity verification. It is removed only as the final successful
publication action, after which bootstrap scratch must be empty. Any residual
guard is terminal contamination requiring a new explicit closure; it is never
automatically recovered, removed, or adopted. This makes a failed continuity
or reacquisition check durable across process boundaries.
