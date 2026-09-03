# VVA-1B Protocol Amendment A3 — Failed-Cache Recovery and Compute Accounting

Status: frozen after the first failed VVA-1B execution and before any
replacement authoritative optimizer update.

## What happened

The first authoritative execution passed custody, the CPU self-test, and CUDA
Stage 0, then opened training. Seed `17` completed all `512` interleaved steps
for each of `V0_current`, `V1_tied_weak`, and `V2_clean_identical`. The process
then failed before publishing the seed cache or evaluating any trained arm.

The immutable failed log is:

```text
.build/tests/representation_vva1b_v1_authoritative_failed.40698.0.log
sha256=a6e44879660e32c21f08f0f70afc542fa60525459c9cc3531c2ce09b3674367c
sidecar_sha256=70786e44b63b281a7a57860d59b8fc3764891da2e7783154f9049f0f9f0ad96e
```

It records `vva1b.training.seed_17.completed_steps=512` followed by the
LibTorch exception:

```text
Expected all elements of the tensor to have the same scalar type: Long,
but got element of scalar type: Int
```

The failure occurred while constructing the integer `receipt_flags` tensor in
the seed-cache serializer. Its initializer mixed `int64_t` step counters with
ordinary `int` boolean literals. It was not a loss, gradient, optimizer,
augmentation, encoder, readout, or endpoint failure. The cache archive was
never written, the seed-cache directory is empty, and the process exited before
any trained-arm evaluation or contrast was emitted.

The failed execution was bound to these immutable artifacts:

```text
source_sha256=c5e423bf19ba63d424c2f1058df6e58e0e764dfc52357abe89d47c58a94e1501
executable_sha256=fc084d877acad99f879ab030a35c18c95fb179a6747bf6bd1d4758d412131ea4
build_manifest_sha256=f892e1262a6e0ef0e3d735faf5325b84e28fd54cdf39acbcfce358059421ec2b
self_test_sha256=b1e3054e79ba7ed210315fa6aed0dd8fecfaef9bd3c8adbcb5d6d590be9fcbdb
preflight_sha256=cfbe333bd644d15eabc016d6a4866eac63b638b9dc81ddc86ff8214be63055a7
preserved_executable=.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_triad_failed_attempt_v1
preserved_executable_sha256=fc084d877acad99f879ab030a35c18c95fb179a6747bf6bd1d4758d412131ea4
```

## Evidence and compute accounting

The failed seed-17 state cannot be reconstructed or admitted. No trained model,
optimizer state, cache, arm endpoint, contrast, candidate decision,
confirmation result, or production decision from that process is evidence.
The only admissible content is the failure/custody receipt itself.

Accounting before recovery is therefore:

```text
attempt_1.interleaved_seed_steps_executed=512
attempt_1.adam_updates_executed=1536
attempt_1.ema_updates_executed=1536
attempt_1.adam_updates_discarded=1536
attempt_1.ema_updates_discarded=1536
attempt_1.accepted_authoritative_adam_updates=0
attempt_1.accepted_authoritative_ema_updates=0
attempt_1.completed_seed_caches=0
attempt_1.classification=invalid_numeric_or_mechanics
```

Because the frozen estimand requires paired joint training of all three arms at
all three seeds, a valid recovery cannot omit or splice seed `17`. If the user
explicitly authorizes replacement compute, the recovery execution must perform
the original, unchanged `3 seeds x 3 arms x 512 steps = 4608` accepted Adam
updates and `4608` accepted EMA updates. On successful completion the complete
lifetime accounting must be:

```text
physical_attempted_adam_updates=6144
physical_attempted_ema_updates=6144
discarded_failed_attempt_adam_updates=1536
discarded_failed_attempt_ema_updates=1536
accepted_authoritative_adam_updates=4608
accepted_authoritative_ema_updates=4608
```

The discarded attempt must never be averaged, selected, or combined with the
recovery results. This amendment defines the only admissible recovery; it does
not itself authorize spending the replacement updates.

## Authorized implementation repair

Before replacement training, the harness may make only these recovery changes:

1. construct every scalar/list cache integer tensor through an explicitly
   homogeneous `int64_t` container;
2. exercise the exact objective, result-flag, and receipt-flag builders in a
   zero-optimizer-update archive save/load round trip;
3. bump the seed-cache schema from `v2` to `v3`; and
4. publish new versioned build-manifest, self-test, preflight, authoritative-log,
   and failed-log paths without modifying or deleting the v1 artifacts; and
5. harden failure receipts so any recovery failure reports executed,
   discarded, and accepted counts, cache-publication state, an invalid
   classification, sealed confirmation, no promotion, unchanged production,
   and both frozen rollback paths; and
6. keep current-process executions, freshly committed updates, and fully
   validated replacement updates as separate ledgers, with zero-update pure
   arithmetic tests for a resumed-cache completion and a post-resume failure.

No arm, view, augmentation, architecture, parameter, initialization, data row,
seed, schedule, loss, optimizer, learning rate, clipping rule, EMA rule,
readout, endpoint, bootstrap table, threshold, contrast, safeguard,
confirmation rule, or rollback rule may change.

## Mandatory stop gates

Before any replacement authoritative update:

- this amendment and its checksum, the failed v1 log and its checksum, and the
  preserved failed executable and its checksum, and the base
  protocol/A1/A2/seam-audit custody must all pass;
- the recovery source and executable must be bound by a new transitive build
  manifest;
- the CPU self-test must report a passing cache-integer archive round trip and
  passing resumed-cache and post-resume-failure accounting cases, all with zero
  optimizer and EMA updates;
- CUDA preflight must repeat the frozen Stage-0 gates; explicitly report the
  512-replicate table `408205cac33d403d`, drawn-A/B and used-A/B data/mask
  hashes, branch/sample/global-valid mask hashes, CPU/CUDA RNG receipts, and
  default-seam parity at update indices `{0,255,511}`; and finish with
  `authoritative_optimizer_updates=0`, `training.opened=false`, and
  confirmation sealed;
- no completed v3 seed cache may pre-exist; and
- the replacement `4608`-update execution must have explicit user
  authorization.

If any recovery gate or replacement execution fails, stop and preserve the new
failed artifact. A completed seed becomes resumable only after its three-arm
cache has been atomically published, checksummed, reloaded, and fully
reconstructed. Partial or temporary caches are never evidence. Do not launch
another authoritative retry automatically.

All scientific questions, contrasts, and decision rules in the base protocol
and Amendments A1/A2 remain unchanged.
