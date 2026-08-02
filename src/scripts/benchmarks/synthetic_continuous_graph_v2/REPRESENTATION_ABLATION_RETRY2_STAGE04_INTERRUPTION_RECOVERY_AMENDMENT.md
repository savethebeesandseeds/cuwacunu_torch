# Retry2 stage-04 interruption recovery amendment

## Scope

This amendment closes the interrupted stage-04 `no_tf_alignment_training`
attempt in
`synthetic_v2_representation_ablation_isolated_v2_retry2`. It does not repair,
resume, complete, or reinterpret that attempt.

Stages 00 through 03 have immutable attempt and completion receipts. Stage 04
has an immutable attempt receipt with `status=consumed`, but no stage-04
completion receipt. The attempt receipt explicitly declares an attempt without
completion terminal and forbids both partial-payload adoption and checkpoint
resume. No stage-05 or later marker exists.

## Observed boundary

The last complete representation report and event stream record 2,160 completed
optimizer steps out of the requested 3,000. The last verified checkpoint was
written at optimizer step 2,150. Parameters and gradients were finite, no batch
was skipped, and the event stream contains only informational records through
step 2,160. These facts prove that a scientific training attempt was consumed;
they do not make its partial checkpoint a completed scientific artifact.

The host-side status `1073807364` is `0x40010004`, the Windows status
`DBG_TERMINATE_PROCESS`. It is not a Linux child exit status. Together with the
absence of a model error, a Runtime completion fact, an OOM event, or a live
process, this supports the classification
`external_host_or_controller_termination_supported_not_actor_attributed`.
Neither the exact controller nor the reason for termination is asserted.

## Closure authority

The companion sealer:

- locks the operational Retry2 runner before locking Retry2's
  `.development.lock`, with both descriptors opened read-only;
- verifies the exact runner, runtime-root, lock, stage-receipt, partial-training,
  and access-boundary identities;
- rejects any process, file-descriptor, working-directory, executable, memory
  mapping, or competing lock reference to the Retry2 runtime other than its own
  two lock descriptors;
- takes two identical, exact inventories of the whole live Retry2 runtime;
- creates an immutable, copy-based whole-runtime `source_snapshot/`, with
  byte-identity, distinct-inode, and single-link proof for every regular file;
- publishes only a separate sibling closure using a no-clobber candidate rename;
  and
- leaves the Retry2 runtime byte-for-byte and inode-for-inode unchanged.

The closure is evidentiary. It authorizes no resume, adoption, same-runtime
re-entry, certified-input access, final-holdout access, or policy access.

## Recovery rule

Any later recovery must use a new schema and a new runtime. It may import only a
separately verified completed prefix. The interrupted stage-04 payload and its
checkpoint must remain excluded from scientific inputs, and
`no_tf_alignment_training` must restart from optimizer step zero. Any such
prefix import requires its own immutable authority; this amendment and its
closure do not create that authority.
