# SRR-2 protocol amendment A2 — fail-closed authoritative log envelope

**Sealed:** 2026-08-28, after the target-free preflight and independent
pre-authoritative audit, before any SRR-2 authoritative scientific row
**Scope:** supersede only the A1 authoritative command; preserve every frozen
scientific population, threshold, gate, precedence rule, exclusion, and the
unchanged target-free preflight command

## Bound parent custody

A2 binds the exact parent protocol, A1, and both of their sidecars:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL.md` | 20,061 | `742def90993850ab7ed381e860d60f5adbf1a258c2d9a7de0568bc0067af985e` |
| `PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL.sha256` | 115 | `258ef89c09ef6db995281e1c40c681a537e1fe9f985c88c6644bc285136ff2e0` |
| `PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A1.md` | 2,313 | `03ba84fe2fa318594c2da9812aebda1d9370008e0b37ad24159388e1213c0d73` |
| `PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A1.sha256` | 128 | `abb2d48fffefac920056b91fa6d2a112e56aa7a87bfbd30aa1840b179d4679a4` |

## Reason

Independent pre-authoritative review found that A1's plain `>` redirection
opens and may truncate the canonical authoritative log before the experiment
binary can inspect or exclusively create its durable attempt ledger. A retry or
concurrent launch could therefore destroy or interleave the sole authoritative
log and only then fail on the already-created ledger. That would preserve the
one-attempt scientific boundary while losing its required evidence.

The defect was found after the target-free preflight and an initial pre-run
manifest seal, but before authoritative dispatch. At A2 seal time both
`.build/tests/representation_srr2_v1_authoritative.log` and
`.build/tests/representation_srr2_v1_attempt.lock` were absent, no
authoritative scientific row had been requested, and the authoritative-attempt
count remained zero. The earlier pre-run manifest is superseded and cannot
authorize execution because it binds the clobberable A1 command.

## Narrow authoritative-command replacement

A2 supersedes only the authoritative command in A1. The exact authoritative
command is:

```text
docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && set -o noclobber && CUBLAS_WORKSPACE_CONFIG=:4096:8 ./.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity --experiment production-structured-readout-parity --device cuda > .build/tests/representation_srr2_v1_authoritative.log 2>&1'
```

`set -o noclobber` makes creation of the canonical regular-file log envelope
exclusive at the shell redirection boundary. If that path already exists, the
shell must fail before starting the experiment binary. A retry or concurrent
dispatch must not truncate, append to, or share the canonical log.

The A1 target-free preflight command is unchanged and remains exactly:

```text
docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && CUBLAS_WORKSPACE_CONFIG=:4096:8 ./.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity --experiment production-structured-readout-parity-preflight --device cuda > .build/tests/representation_srr2_v1_preflight.log 2>&1'
```

## Pre-seal absence and reserved-envelope rule

Before the replacement pre-run manifest is sealed, independent checks must
establish all of the following:

- the canonical authoritative-log path is absent, including any regular file,
  directory, or symbolic link;
- the canonical attempt-ledger path is absent, including any regular file,
  directory, or symbolic link;
- the manifest records both absence facts and excludes both paths from its
  entry set; and
- the manifest, experiment binary, and independent auditor bind the exact A2
  command bytes and authenticate this amendment and its sidecar.

Any creation of the canonical authoritative-log path reserves the SRR-2
one-shot evidence envelope, even if the file is empty, partial, or accompanied
by no attempt ledger. That file must not be deleted, truncated, replaced, or
automatically retried. Operators must stop and preserve the log, ledger state,
and dispatch evidence for independent incident review. A recovery or retry, if
scientifically permissible, requires a separately reviewed and sealed protocol
amendment; A2 itself authorizes none.

The attempt ledger remains the durable scientific-attempt consumption marker.
The earlier log-envelope reservation exists solely to make command dispatch and
evidence custody fail closed before that later marker can be created.

## Required reclosure

Before authoritative execution, the replacement closure must bind A2 and its
sidecar, the exact no-clobber command in both independent implementations, the
rebuilt binaries and receipts, the regenerated candidate patch, a fresh run of
the unchanged target-free preflight, and a replacement pre-run manifest. No
Make target may launch authoritative mode, and no deletion or retry path is
added or authorized by this amendment.
