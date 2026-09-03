# Project Clear Signal — Optimizer-Localization Verification Recovery

## Protocol identity and stop condition

- Protocol:
  `synthetic_v2_frozen_representation_affine_injection_optimizer_localization_verification_recovery_development_v1`.
- Authority: development diagnostic only; benchmark acceptance is false.
- The protocol is complete after one bounded verification-only attempt validates
  the immutable predecessor evidence and publishes one immutable recovery receipt.
- There is no scientific rerun, build, evaluator or binary execution, model
  forward, fit, optimizer step, retry, resume, seed sweep, or follow-on rung.

The predecessor evaluator completed successfully and emitted a complete report.
Its runner terminalized during report validation. This protocol imports that
already-produced report; it does not produce or alter scientific measurements.

## Frozen predecessor evidence

The verifier binds these exact immutable files:

- predecessor attempt, SHA-256
  `ec49afdec429f8937fcd6099c8be222ccdd3bb7870003ec48d55a725c51bf7a6`;
- predecessor terminal, SHA-256
  `fd0bcbe4cdda0ac8b77ccd0c30b409704d40a6a7c4cf8fa88108b7e830d395a2`;
- rejected development report, SHA-256
  `5b1ebcc7af65792074e653406a1a6f4120dc9ad4105adca5cbca90f6c5815f30`;
- empty evaluator log, SHA-256
  `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`;
- predecessor runner, SHA-256
  `97eaebc2b0dc609acc75ffa09fcd4569bd83fcdfe8c3255e6eab67b77e314afa`;
- predecessor preregistration, SHA-256
  `5c86fcb55b10e52ab322d271c0117f6184402a7d5234a32e13048237d7056b09`;
- evaluator source, SHA-256
  `7a55325e6f291e2355d1d5944c9fb00e94dadebe702d48dab3b9af349a0b871b`;
- build wrapper, SHA-256
  `2e5b39981302d55f8785389c4b01cb6dd4b38036d6d45ca10a798c69567004fd`;
- compile-only build receipt, SHA-256
  `973f7dd179e76c915f0554c3980e658d4229125a81fbfe1f936cfad25b310e3c`;
- compiled evaluator binary, SHA-256
  `b04f380db3cec472ae2ef589664d38a632b92779ccfcbbfd6c45005dbe20f801`.

The predecessor terminal must say `failure_stage=report_validation`,
`evaluator_started=1`, and `evaluator_exit_code=0`, and must bind the attempt,
empty log, and rejected report hashes above. The predecessor execution lock must
be available for a shared nonblocking lock throughout verification.

## Frozen development authorities

The verifier also binds:

- Phase 2A development receipt, SHA-256
  `b611d3d3d9e2d1e198a2764b928886b647d5ee95211a89e584a49c4e05b7fbe5`;
- byte-identical Phase 2A main and replay reports, each SHA-256
  `2bb817bc5649c895e5fde2079fffb9505d2a42ac90b6f7ded55ef8b4946fe38a`;
- recovered Phase 2B development result, SHA-256
  `cdf50f2a827b0eeb99bb92dca4821dd255b4c0e8d102db7daee759c662d79418`;
- Phase 2B nonlinear report, SHA-256
  `34d51b3226278f8fe43a79e09c1c9063fe1aab31db042bd6a224ef06445fbc36`.

The development train and validation probe hashes are bound transitively by the
predecessor attempt and Phase 2A receipt. The recovery verifier does not open
probe bytes. It reads no raw source, certified input, final holdout, policy,
MDN, or checkpoint data.

## Corrected duplicate-MSE validation rule

The only predecessor validator defect is its demand for absolute equality within
`1e-12` between ten duplicate MSE presentations:

- one Torch float32 `final_full_train_standardized_mse` and the aggregate route
  MSE recomputed from emitted predictions in float64; and
- nine Torch float32 per-head MSE fields and the corresponding route per-head
  MSE values recomputed in float64.

The scientific preregistration required the MSE values to be positive and finite;
it did not require bit-identical or `1e-12`-identical float32 and float64 duplicate
presentations. For exactly these ten pairs, the recovery verifier uses

`abs(a - b) <= 1.25e-7 + 1.25e-7 * max(abs(a), abs(b))`.

Both values must still be positive and finite. The absolute and relative
tolerances are each approximately one IEEE-754 float32 machine epsilon. The
verifier records the pair count, strict-`1e-12` failure count, mixed-tolerance
pass count, maximum absolute discrepancy, maximum scale-relative discrepancy,
and maximum fraction of the allowed mixed tolerance. No source value is rounded,
rewritten, normalized, or substituted.

Every other equality and numerical tolerance remains exactly as frozen in the
predecessor runner. In particular, Phase 2A oracle reproduction remains at
absolute tolerance `1e-12`; parity thresholds remain `1e-3` and `1e-5`; and all
optimizer recovery and clear-failure thresholds remain unchanged.

## Single-pass full-report verification

The worker reads the rejected report once into a duplicate-rejecting key cache
and reads the Phase 2A main report once into a second cache. It then applies the
predecessor's complete report contract to the original immutable values:

- schema, source identity, row counts, anchor bounds, route identities, dtypes,
  deterministic settings, standardization, and architecture;
- every aggregate and per-channel train/validation metric, internal metric
  identity, and Phase 2A oracle reproduction;
- parity deltas and gates, aggregate and nine-head standardized MSE ratios,
  recovery and clear-failure gates, and first-match classification;
- seed, fit and step counts, optimizer schedule and fingerprint, protected-access
  flags, and absence of trainer validation access or checkpoint writes; and
- the ten duplicate MSE pairs under only the corrected rule above.

The inherited first-match classification must remain
`float32_conditioning_failure`. The receipt separately preserves that direct
float32 aggregate forecast metrics remain close to the float64 oracle, that the
paired-GELU parity gate passes, and that the distinct direct-linear Adam
clear-failure gate passes. The latter evidence must not replace or reorder the
frozen classification.

## Lifecycle and protected boundary

The verifier has one import attempt and one verification worker, bounded by a
30-second GNU timeout with a 5-second TERM grace. Runtime outputs use a distinct
pristine private root, atomic no-clobber publication, immutable file metadata,
and a captured/reaped timeout child. Any signal, timeout, malformed source,
failed validation, incomplete receipt, or post-attempt error terminalizes the
attempt. `--verify-development` is read-only. Successful output cannot authorize
certified/final evaluation, representation training, or another scientific run.
