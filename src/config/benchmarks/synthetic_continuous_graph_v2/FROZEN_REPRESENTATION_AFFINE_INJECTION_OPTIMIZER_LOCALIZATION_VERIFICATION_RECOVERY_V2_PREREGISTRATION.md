# Project Clear Signal — Optimizer-Localization Verification Recovery V2

## Protocol identity and stop condition

- Protocol:
  `synthetic_v2_frozen_representation_affine_injection_optimizer_localization_verification_recovery_development_v2`.
- Authority: development diagnostic only; benchmark acceptance is false.
- This is a mechanical successor to the terminal V1 verification recovery. It
  is complete after one data-free parser self-test and one verification-only
  attempt publish an immutable development receipt.
- There is no build, evaluator or binary execution, model forward, fit,
  optimizer step, data-source or probe read, retry, resume, seed sweep, or
  scientific threshold change.

## Frozen lineage

V2 binds the original optimizer-localization evidence bundle exactly as V1 did,
including its attempt, terminal, rejected report, empty evaluator log, frozen
runner, preregistration, evaluator source, build wrapper, build receipt, binary,
and Phase 2A/2B development authorities.

It additionally binds the failed V1 recovery:

- V1 runner SHA-256
  `2f25692f0c9376da4902194cfabfd0c82b9d7ca72346224873531ffb4a9cdda8`;
- V1 preregistration SHA-256
  `c9a8887aecb4b54e73b262ef50d7133bfce3c2279a202203f4b75221985727b4`;
- V1 verification attempt SHA-256
  `4d2cc6ac4d4be5282fbe596f7eaafe557606c3d37acbd18d6cb874921b1b861c`;
- V1 verification log SHA-256
  `a020f06cc7ad27d46f02a87e96fa95aded90e6787da1487a37eea928d0d370b7`;
- V1 terminal SHA-256
  `660e6c6396828630092243ba1fd569a9b935aa6d2cc46863b3fb3c73b36786db`.

The V1 terminal must show attempt consumption, worker exit code 2, and failure
at `single_pass_report_verification`. Its log must remain the five immutable AWK
syntax-error lines. Both the original runtime and V1 runtime locks must be held
shared and nonblocking throughout V2 preparation and verification.

## Exact mechanical correction

The complete report validator is factored into
`frozen_representation_affine_injection_optimizer_localization_verification_recovery_v2_validator.awk`,
SHA-256 `2b819f54ff1df0d1ccb39dbd5538cecad31ae957f8224ebb588671129807ec76`.
The deployed canonical interpreter is `/usr/bin/mawk`, SHA-256
`301315e7e2e964b4e403824b3f6c7ad8db1023e4ce87e6f6c92bf367e047f311`.

Relative to the frozen V1 AWK program, the only executable validation changes
are:

1. every scalar identifier `split` is renamed to `split_id`; and
2. the first statement of `END` is
   `if (syntax_self_test) exit 0`.

No scientific comparison, field, tolerance, gate, output, ordering rule, or
classification changes. The mixed duplicate-MSE rule remains limited to the
same ten pairs:

`abs(a - b) <= 1.25e-7 + 1.25e-7 * max(abs(a), abs(b))`.

All other tolerances remain frozen, and the inherited first-match
classification remains `float32_conditioning_failure`. Paired-GELU parity and
direct-linear Adam clear-failure remain separately recorded.

## Mandatory data-free exact-program self-test

Before a verification attempt may exist, public command `--prepare` executes
the exact deployed validator and interpreter under a 5-second GNU timeout with
1-second TERM grace:

`/usr/bin/mawk -v syntax_self_test=1 -f VALIDATOR /dev/null /dev/null`.

Because the guard is inside the validator's `END` block, the interpreter must
parse the complete deployed program before it can exit successfully. Both input
files are `/dev/null`; no report, probe, data source, model, or policy artifact
is opened. Success requires exit code 0 and empty stdout/stderr, then atomically
publishes an immutable self-test log and receipt. The scientific attempt binds
that receipt and cannot be emitted without it. A self-test failure terminalizes
V2 without consuming a verification attempt.

## Verification and lifecycle

The report verifier remains single-pass and cached: the Phase 2A report and
rejected report are each parsed once by the exact validator. Its worker remains
bounded by a 30-second timeout with 5-second TERM grace. Fit, optimizer, access,
parity, classification, and receipt semantics are identical to V1.

V2 uses a distinct pristine private runtime. Self-test, attempt, logs, terminal,
verification receipt, and result use atomic no-clobber publication and immutable
metadata. Timeout children are captured, signaled, waited, and reaped. Any
post-attempt error terminalizes the attempt. `--verify-preparation` and
`--verify-development` are read-only and cannot create or chmod runtime state.
Successful output cannot authorize certified/final evaluation, representation
training, or another scientific run.
