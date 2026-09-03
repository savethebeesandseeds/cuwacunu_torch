# SRR-2 protocol amendment A3 — pre-attempt compatibility recovery

**Sealed:** 2026-08-28, after preservation and review of the failed A2
dispatch, before any SRR-2 authoritative scientific capture or attempt-ledger
creation  
**Scope:** preserve the A2 incident envelope, repair only the pre-attempt CUDA
compatibility receipt and its target-free coverage, and authorize exactly one
replacement dispatch into a distinct fail-closed log envelope

## Bound protocol and incident custody

A3 binds the exact base protocol, A1, A2, and every existing sidecar:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL.md` | 20,061 | `742def90993850ab7ed381e860d60f5adbf1a258c2d9a7de0568bc0067af985e` |
| `PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL.sha256` | 115 | `258ef89c09ef6db995281e1c40c681a537e1fe9f985c88c6644bc285136ff2e0` |
| `PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A1.md` | 2,313 | `03ba84fe2fa318594c2da9812aebda1d9370008e0b37ad24159388e1213c0d73` |
| `PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A1.sha256` | 128 | `abb2d48fffefac920056b91fa6d2a112e56aa7a87bfbd30aa1840b179d4679a4` |
| `PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A2.md` | 5,031 | `5cc7e519c25899d309b76df32bd15e5a24cb731a3eafc9f269ea3905eea84f11` |
| `PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A2.sha256` | 128 | `1d8091c1b538d63cb15dd90f1f5d7543f57d9685474cd52d867e03a59778981d` |

The exact A2 pre-run manifest and the reserved A2 authoritative log are
preserved under distinct immutable paths and become mandatory A3 custody
inputs:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `.build/tests/representation_srr2_v1_prerun_a2.sha256` | 12,737 | `339edec05bd1d5ae686532bf9c44a1c27e3b2e664ea6df1d682059b6157ab613` |
| `.build/tests/representation_srr2_v1_authoritative.log` | 647 | `c60de68c496bd43a73ea7a327264a605eb3d4755df9af97b713ffe0916846768` |

The preserved manifest snapshot is an exact byte copy of the A2 manifest that
authorized the failed dispatch. Neither preserved artifact may be deleted,
truncated, rewritten, renamed, or treated as an A3 result. The replacement
pre-run manifest, candidate patch, binaries, receipts, mechanics log, and
preflight log must be regenerated after the narrow A3 repair and must bind both
preserved artifacts as mandatory entries.

## A2 incident classification

The reserved A2 log records exactly these controlling facts:

```text
srr2.attempt.consumed=false
authoritative_attempt_count=0
failure_reason=invalid_mechanics
terminal_result=invalid_mechanics
SRR-2 production structured readout parity failure: outer augmentation model device is not cuda:0
training_authorized=false
augmentation_change_authorized=false
long_run_authorized=false
active_policy_change_authorized=false
checkpoint_migration_authorized=false
downstream_retraining_authorized=false
end_to_end_authorized=false
deployment_authorized=false
```

At A3 seal time, the durable attempt ledger
`.build/tests/representation_srr2_v1_attempt.lock` is absent. The A2 failure
occurred while constructing the pre-attempt compatibility receipt, before the
attempt boundary and before the first accepted scientific capture request.
Consequently A2 consumed zero scientific attempts, captures, rows, feature
values, validity values, targets, probes, fits, predictions, permutations, or
bootstrap samples. It is an `invalid_mechanics` harness-closure incident, not
evidence for or against production structured-readout parity.

The A2 authoritative command remains permanently spent as a dispatch
envelope. It must never be invoked again, even though its failure preceded the
scientific-attempt marker.

## Narrow CUDA compatibility-alias repair

Review traced the failure to a compatibility-only path: the checked-in active
DSL explicitly declares `DEVICE = cuda`, which the parser represents as the
unindexed CUDA current-device alias with device index `-1`. The receipt passed
that parsed object to the experiment's canonical-manifest helper, which
requires explicit device `cuda:0`. This produced the reported `outer
augmentation model device is not cuda:0` failure before any scientific work.

A3 requires the following narrow repair:

1. Preserve the parsed active configuration, checked-in DSL, production
   defaults, public fingerprint implementation, checkpoint identities, and
   serving policy behavior byte-for-byte except for other already sealed SRR-2
   candidate changes.
2. Require the parsed active device to be CUDA with device index exactly `-1`
   or `0`. Reject CPU, a nonzero CUDA index, and every other device before
   constructing a compatibility alias.
3. Inside the compatibility receipt only, derive a local active alias from the
   parsed active configuration and normalize its outer experiment device to
   explicit `cuda:0` before passing it to the canonical-manifest helper.
4. Derive the local structured alias from that normalized active alias and
   change only its serving policy to `structured_cdsb_v1`; all other compared
   fields, including explicit device `cuda:0`, must remain identical.
5. Require the resulting fingerprints to differ, the active DSL to remain
   `all_tokens`, and every original enum/parser/default/fingerprint/checkpoint/
   adapter compatibility fact to remain true.

These aliases are non-production, local, immutable after construction, and
used only to close the experiment's compatibility proof. The repair does not
authorize changing the active DSL's exact `DEVICE = cuda` declaration, a
production device, an augmentation, a model, a dataset, or any scientific
parameter.

## Target-free exact compatibility preflight

Before the replacement pre-run manifest is sealed, a rebuilt candidate must
run the exact same compatibility-receipt routine, against the same checked-in
DSL and mechanics evidence, that authoritative setup will run before the
attempt ledger. The target-free preflight must emit primitive evidence that:

- the parsed active device is CUDA with index exactly `-1` or `0`, never CPU or
  a nonzero CUDA index;
- both compatibility aliases presented to the canonical-manifest helper are
  exactly `cuda:0`;
- they differ only in serving policy;
- every compatibility receipt fact is true; and
- the complete authoritative compatibility setup returns successfully before
  any attempt-boundary operation.

The independent auditor must parse and recompute this closure rather than
trusting one aggregate boolean. This preflight remains target-free: it may not
generate or inspect targets, construct or fit a probe, select a
hyperparameter, predict, permute, bootstrap, request an authoritative dataset
capture, or create the attempt ledger. It does not consume an authoritative
attempt.

The exact preflight command remains:

```text
docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && CUBLAS_WORKSPACE_CONFIG=:4096:8 ./.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity --experiment production-structured-readout-parity-preflight --device cuda > .build/tests/representation_srr2_v1_preflight.log 2>&1'
```

## One replacement authorization and exact command

A3 authorizes exactly one replacement invocation, and only if every one of the
following facts is independently established immediately before dispatch:

- the six bound protocol/amendment artifacts and their hashes match;
- the preserved A2 manifest snapshot and failed A2 log match the exact path,
  bytes, hash, and zero-attempt semantics above;
- the A2 attempt ledger remains absent, including any regular file,
  directory, or symbolic link at that path;
- the distinct A3 authoritative-log path is absent, including any regular
  file, directory, or symbolic link;
- focused mechanics and the target-free exact compatibility preflight pass on
  rebuilt, sealed binaries;
- the replacement pre-run manifest validates every required source, binary,
  receipt, log, command, environment, candidate-delta, parent-evidence, A2
  incident, and A3 amendment binding; and
- all scientific counters and forbidden authorization flags remain zero or
  false as required by the base protocol.

The exact replacement authoritative command is:

```text
docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && set -o noclobber && CUBLAS_WORKSPACE_CONFIG=:4096:8 ./.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity --experiment production-structured-readout-parity --device cuda > .build/tests/representation_srr2_v1_authoritative_a3.log 2>&1'
```

Its canonical UTF-8 command string, excluding any trailing newline, is exactly
327 bytes with SHA-256
`ebde8d9a011d5bdcb55768e857613d827358cd9d7f778afdd141d2518d32805d`.
The replacement authoritative-log path is exactly:

```text
.build/tests/representation_srr2_v1_authoritative_a3.log
```

`set -o noclobber` gives this new regular-file evidence envelope the same
fail-closed shell boundary as A2. Creation of the A3 path spends the sole A3
replacement dispatch, even if the log is empty or partial and even if no
attempt ledger is subsequently created. It must never be deleted, truncated,
replaced, appended to, or automatically retried. Any later recovery requires
a new independently reviewed and sealed amendment; A3 itself authorizes none.

The durable attempt ledger remains the base protocol's scientific-attempt
marker. If it is created, any later failure is the single authoritative
scientific result and no replacement is permissible.

## Reclosure, invalidation, and unchanged science

The replacement pre-run manifest, parity harness, and independent auditor must
authenticate this amendment and its sidecar, the exact A3 command bytes and
hash, the distinct A3 log path, both preserved A2 incident artifacts, and the
pre-dispatch absence conditions. The auditor must consume the A3 log as the
sole candidate scientific result while retaining the A2 log only as incident
custody evidence. No Make target may launch authoritative mode.

Any custody mismatch, non-absence fact, compatibility-preflight failure,
command-byte change, use of the old A2 command or log as a new result, or
second A3 invocation invalidates authorization and is `invalid_mechanics`.
Deleting or rewriting evidence cannot restore eligibility.

A3 changes no seed, dataset, row, tensor, projection, architecture, threshold,
parity rule, quality-transport premise, terminal precedence, exclusion, or
authorization boundary in the base protocol. A1 remains authoritative for the
deterministic environment and preflight command; A2 remains authoritative for
fail-closed log-envelope semantics; A3 supersedes only the A2 authoritative
command and adds the narrow pre-attempt recovery closure described here. The
checked-in active serving policy remains `all_tokens`, and every final
authorization flag remains false.
