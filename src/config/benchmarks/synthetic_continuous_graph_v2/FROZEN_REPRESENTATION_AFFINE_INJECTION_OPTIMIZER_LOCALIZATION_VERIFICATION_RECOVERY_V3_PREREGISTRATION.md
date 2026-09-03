# Project Clear Signal — Optimizer-Localization Verification Recovery V3

## Protocol identity and stop condition

- Protocol:
  `synthetic_v2_frozen_representation_affine_injection_optimizer_localization_verification_recovery_development_v3`.
- Authority: development diagnostic only; benchmark acceptance is false.
- V3 is a mechanical successor to the terminal V2 parser preparation. It is
  complete after one data-free parser self-test and one verification-only
  attempt publish an immutable development receipt.
- There is no build, evaluator or binary execution, model forward, fit,
  optimizer step, report/probe access during preparation, retry, resume, seed
  sweep, or scientific threshold change.

## Frozen lineage

V3 retains the original optimizer-localization evidence and V1 recovery lineage
bound by V2. It additionally binds the failed V2 preparation:

- V2 preregistration SHA-256
  `fbde44e0dfc3eb3758744b8a96e4760e4a7c00e159bbda904c80c9614441766a`;
- V2 validator SHA-256
  `2b819f54ff1df0d1ccb39dbd5538cecad31ae957f8224ebb588671129807ec76`;
- V2 runner SHA-256
  `56a775862c565894edc12a8c13d8ee63263e7812ed270dcb8124e89800bb2bce`;
- V2 self-test log SHA-256
  `79f66ac41242f347215434706b0d866176b9b924657592f89337444b58bd465f`;
- V2 terminal SHA-256
  `eb94e73269eff8e9e7398e8c4377983813479417abb845ed2418f55dfab0daa8`.

The V2 terminal must show `failure_stage=validator_syntax_self_test`, child and
exit code 2, no self-test receipt, and
`verification_attempt_consumed=false`. Its log remains the five immutable mawk
parse-error lines. No V2 attempt, self-test receipt, verification receipt, or
result may exist. The original, V1, and V2 runtime locks are held shared and
nonblocking before any corresponding lineage hash/read.

## Exact mawk-portability correction

The V3 validator is
`frozen_representation_affine_injection_optimizer_localization_verification_recovery_v3_validator.awk`,
SHA-256 `a93a4c6315c0c89f6b747490d3cb6112dbf506675ad188ed656d10693dbedbb2`.
The interpreter remains canonical `/usr/bin/mawk`, SHA-256
`301315e7e2e964b4e403824b3f6c7ad8db1023e4ce87e6f6c92bf367e047f311`.

Relative to V2, only mawk-incompatible line-broken expressions are rewritten:

- multiline lower/upper-bound ternaries become explicit nested `if/else`;
- the multiline metric-field and mixed-tolerance fraction ternaries become
  explicit `if/else` assignments;
- float32/GELU parity and recovery/clear-failure boolean presentations become
  explicit assignments and `if/else` branches;
- line-broken recovery metric comparisons use named scalar operands and
  single-line comparisons; and
- the line-broken clear-failure conjunction becomes nested `if` statements.

Single-line ternaries remain unchanged. No field, arithmetic expression,
threshold, tolerance, gate, classification rule, output key, or output value is
changed. The mixed duplicate-MSE rule remains exactly

`abs(a - b) <= 1.25e-7 + 1.25e-7 * max(abs(a), abs(b))`.

Before this V3 identity was frozen or any V3 runtime existed, the exact
validator bytes above were directly parsed by the deployed interpreter using
`-v syntax_self_test=1 -f VALIDATOR /dev/null /dev/null`; the command exited 0
with empty output.

## Mandatory runtime self-test and verification

V3 retains the public `--prepare` self-test. It executes the exact frozen
validator and `/usr/bin/mawk` against `/dev/null /dev/null` under a 5-second GNU
timeout with 1-second TERM grace. The first `END` statement remains
`if (syntax_self_test) exit 0`. Exit 0 and empty output publish an immutable
self-test receipt before any verification attempt can exist.

Full verification remains the same single-pass cached import, bounded by a
30-second timeout with 5-second TERM grace. It reads the Phase 2A and rejected
development reports only after preparation and attempt authorization. Every
scientific comparison, ten-pair tolerance, gate, classification, and receipt
semantic remains unchanged from V2.

V3 uses a distinct pristine private runtime. Outputs are atomic, no-clobber,
immutable, and timeout children are captured, signaled, waited, and reaped.
`--verify-preparation` and `--verify-development` are read-only. Successful V3
output cannot authorize certified/final evaluation, representation training, or
another scientific run.
