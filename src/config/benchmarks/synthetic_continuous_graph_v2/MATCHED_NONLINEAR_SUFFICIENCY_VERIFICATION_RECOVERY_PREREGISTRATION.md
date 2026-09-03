# Project Clear Signal — Immutable Verification Recovery Preregistration

## Protocol identity

- Protocol: `synthetic_v2_matched_nonlinear_sufficiency_verification_recovery_development_v1`
- Diagnostic authority: development only.
- Benchmark-acceptance authority: false.
- This protocol performs no capture, model forward, training fit, optimizer step,
  refit, checkpoint write, or policy action.

## Why this successor exists

The immediately preceding protocol completed and validated both raw captures and
the single matched nonlinear evaluator before publishing its development receipt.
It then retired that receipt and sealed a terminal receipt because the redundant
post-publication verifier exceeded its fixed 30-second timeout while repeating
probe-identity validation on the current machine.

The predecessor remains terminal and is never resumed or retried. This successor
does not recreate its science. It only revalidates the immutable predecessor
bundle with a conservative timeout and, if the complete frozen-validator pass,
immutable verification-receipt validation, and result-candidate validation all
pass, imports the already-computed development classification through the final
atomic result commit.

## Frozen predecessor authority

The recovery runner must bind and verify these exact immutable inputs:

- predecessor runner:
  `run_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_v1.sh`,
  SHA-256 `ce4c0374254cd85b0689f1338885b2d4f9b816c5f77ed207a6d48374414b9cc3`;
- predecessor preregistration SHA-256
  `cd1460b3e1af0d9f2e719c8638b41f2c5ed8a4d6b65c82b34f6a1343ff645785`;
- predecessor attempt SHA-256
  `8cb75b9b18b4f21938c46075446099e874bd5ad9ab1223fa0822237e0323eeb9`;
- predecessor terminal SHA-256
  `2cec20e3d7ab6a8848e2a387c87a255c74a99f64145573b257fc9840cecea902`;
- predecessor rejected development receipt SHA-256
  `49ea95c58e521f846d70b54fb45671fcd7d8fadc1532da75249cb672c3fad99d`;
- predecessor artifact manifest SHA-256
  `d3640df0982946cc4b7071e0a1a9dc048b33f1bc2f4519a2d1b49a113d2b630f`;
- the exact nonlinear report SHA-256 recorded by the predecessor manifest.

The terminal receipt must attest two captures started/completed/validated, one
evaluator started/completed/validated, six completed fits, 21,000 optimizer
steps, maximum anchor 2815, no certified/final/policy access, no checkpoint
write, and failure only at `success_sealing_or_verification` ordinal 4 with exit
124. The rejected development receipt must retain `status=complete` and bind the
same attempt and manifest.

## Exact verification operation

The successor sources the hash-pinned predecessor runner only as a validation
library; its guarded `main` is not invoked. It calls the predecessor's frozen
read-only checks in this order:

1. verify the predecessor self-test receipt;
2. re-run the complete scientific-authority preflight;
3. require the rejected receipt to be immutable and hash-exact;
4. validate the attempt, build receipts, two capture reports, both complete
   coordinate/target identity joins, and the nonlinear report;
5. verify all 22 fixed manifest path/hash bindings and all nine stage receipts;
6. validate the rejected development receipt against the report classification;
7. prove no predecessor capture or evaluator process remains.

The complete verification pass is bounded by 300 seconds with a 10-second TERM
grace. Its exact output is sealed in an immutable log and verification receipt.
Only after that pass, receipt validation, and successor-result candidate
validation may the final development receipt be atomically committed. A later
`--verify-development` is metadata/hash-only and cannot repeat scientific scans
or turn an incomplete attempt into success. No verifier output is interpreted as
science; the exact predecessor artifacts remain the sole scientific evidence.

## Success and failure rules

The successor has one verification attempt and no resume or retry. It publishes
an immutable attempt receipt before validation. A failed, interrupted, or timed
out pass publishes an immutable terminal-invalid receipt and no success receipt.

Success requires the one complete frozen-validator pass. The successor
development receipt must bind
its runner and this preregistration, the predecessor runner/attempt/terminal/
rejected receipt/manifest/nonlinear report, the verification log, all inherited
science counters, the exact inherited classification, both arms' four
gate-driving validation medians, and the representation arm's four corresponding
training medians. It must separately state that successor capture/evaluator/fit/
optimizer counts are all zero.

On success, the inherited classification becomes an authoritative development
diagnostic under this recovery protocol. It remains ineligible for benchmark
acceptance and grants no certified-evaluation authority.

## Protected boundary

The maximum allowed scientific anchor remains 2815. Certified, final holdout,
policy, refit, checkpoint-write, and real-data paths are forbidden. The recovery
may read only the already-authorized development inputs needed by the frozen
validator and the sealed predecessor runtime bundle.
