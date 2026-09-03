# SRR-2 — Production Structured Readout Parity Findings

## Plain-language answer

**The structured readout repair is now implemented correctly in the real
production serving selector, behind the opt-in policy
`structured_cdsb_v1`.** Across the complete frozen SRR-1 representation
domain, the production path returned exactly the same CUDA values and masks as
the independently implemented shadow path. The independent auditor classified
the result `production_structured_readout_parity_reproduced` with zero errors.

This is an important step beyond SRR-1. SRR-1 proved that averaging all tokens
inside a channel destroys useful sequence information. SRR-2 proves that the
accepted repair is no longer only a test-side idea: the public three-argument
production selector can produce that repaired representation exactly.

The repair is deliberately dormant. The checked-in active policy, production
default, omitted-DSL fallback, and legacy checkpoint behavior all remain
`all_tokens`. No training, augmentation, downstream retraining, end-to-end run,
checkpoint migration, activation, or deployment was performed or authorized.

## What was proven

`P` is the new opt-in production branch. `R` is the sealed SRR-1 shadow. `D` is
the independent canonical CPU-float64 reference. The harness captured each
encoder output once and derived all three readouts from the same retained
object, then repeated every capture to test determinism and purity.

| Proof obligation | Authoritative result | Meaning |
|---|---:|---|
| Production `P` versus shadow `R` on CUDA | byte-identical values and masks; maximum absolute difference `0` | the production algorithm exactly reproduces the accepted repair |
| CPU-float64 `P`, `R`, and canonical `D` | byte-identical; maximum absolute difference `0` | production and shadow independently reach the sealed mathematical reference |
| CUDA-float32 translation versus `D` | maximum absolute error `5.662762241342989e-7` against the frozen `2e-5` limit | device rounding is about 35 times smaller than the allowed bound |
| Frozen scientific coverage | 3 seeds × 6 dataset views = 18 retained captures, plus all 18 repeats | the comparison covers every feature tensor consumed by SRR-1, not a sample |
| Retained surface | 3,840 rows, 368,640 feature values, and 11,520 validity values | the exact quality-bearing input domain was compared in full |
| Repeat surface | another 3,840 rows, 368,640 feature values, and 11,520 validity values | every capture reproduced identically |
| Backward compatibility | all enum, parser, default, DSL, fingerprint, checkpoint, selector, and adapter gates passed | existing policies and identities were preserved |
| Independent audit | 4,541 unique schema keys, zero malformed, duplicate, non-finite, critical-value, or unaccessed fields | the result is complete and independently admissible |

The production branch also passed the exact output-shape, stride,
contiguity, dtype, device, mask, finite-value, invalid-zeroing, parameter,
buffer, RNG, model-mode, input-immutability, and public-selector-sandwich
contracts. Unsupported or malformed representation layouts continue to fail
closed.

## Representation quality carried into production

SRR-2 did not refit probes. Instead, it proved byte identity over every feature
and mask consumed by the already audited deterministic SRR-1 probe stack.
Therefore substituting `P` for `R` leaves every prediction, selected probe
hyperparameter, score, interval, shuffle control, contrast, and classification
unchanged.

The quality result now attached to the opt-in production readout is:

| Readout | Predictive AULC, 95% CI | Reversal AULC, 95% CI | Interpretation |
|---|---:|---:|---|
| Current active channel mean `C` | `0.5193 [0.4873, 0.5426]` | `0.5745 [0.5627, 0.5865]` | much of the tested order signal is hidden |
| Production structured `P` (= shadow `R`) | `0.5931 [0.5716, 0.6080]` | `0.9295 [0.9161, 0.9418]` | material predictive gain and strong order decoding |
| Full encoder surface `E` | `0.5833 [0.5628, 0.5979]` | `0.9569 [0.9467, 0.9667]` | full ordered encoder-token reference |

The exact structured-versus-current predictive contrast remains
`P-C = +0.073799267391755394`, with deterministic paired 95% interval
`[0.055623165307082938, 0.092895816258745281]`; all three seed deltas are
positive. The structured readout remains noninferior to the full encoder,
`P-E = +0.0097632202069557472`, interval
`[0.0010917648738582365, 0.019507133427596648]`. All continuous-target and
order-shuffle controls remain passed.

This transport is stronger than rerunning a nearby score: the production and
shadow probe inputs are the same bytes. It is also narrower than a new
end-to-end experiment: downstream behavior was not measured here.

## How far this moves the diagnosis

Before SRR-1, weak end-to-end behavior could not be cleanly separated between
the encoder, augmentations, and serving readout. SRR-1 established that useful
sequence structure exists in the encoder-token surface and that the current
all-token average discards it. SRR-2 now establishes that the exact repair can
be reached through the production interface without changing the encoder or
breaking legacy behavior.

The immediate representation bottleneck is therefore both **identified** and
**implemented**, but not yet **activated**. This substantially reduces the
uncertainty around the representation layer. It does not clear or condemn the
training augmentations; testing them while the destructive mean remains active
would still mix two different causes.

## A2 incident and A3 recovery

The first A2 dispatch stopped before the scientific attempt boundary because
the experiment's compatibility receipt passed the checked-in CUDA current-
device alias (`cuda`, index `-1`) to a helper that requires explicit `cuda:0`.
Its preserved log records `srr2.attempt.consumed=false`,
`authoritative_attempt_count=0`, no captures, and
`terminal_result=invalid_mechanics`; it is not evidence for or against the
readout.

A3 repaired only that local compatibility receipt. It left the parsed active
configuration and production device behavior unchanged, reran target-free
preflight, resealed every artifact, and authorized one distinct replacement
dispatch. That dispatch consumed exactly one attempt and completed. The A2 log
and manifest remain preserved as incident evidence, and the A3 attempt ledger
is durable and read-only.

## What this does not establish

- It does not prove that `structured_cdsb_v1` is globally optimal or that its
  sixteen cells are minimal.
- It does not estimate performance on unseen real-world data; the scientific
  boundary remains the frozen deterministic sequence family and three encoder
  seeds.
- It does not show that an existing downstream head can consume the changed
  feature semantics without adaptation, even though shape `[B,3,32]` is
  unchanged.
- It does not test runtime speed or latency; timing had no pass/fail role.
- It does not test or change training objectives, augmentations, optimizers, or
  encoder weights.
- It does not authorize changing the active policy, migrating checkpoints,
  retraining downstream components, running end to end, or deploying.

## One next recommendation

Proceed with **SRR-3 — Controlled Readout Activation and Downstream
Compatibility**. This should remain a separately frozen, low-cost stage whose
first question is whether the existing downstream path can use the repaired
representation—not whether a long training run eventually compensates for a
bad interface.

The ordered SRR-3 plan should be:

1. Freeze the current `all_tokens` baseline, the opt-in
   `structured_cdsb_v1` candidate, exact checkpoints, evaluation rows, seeds,
   metrics, decision thresholds, and a strict compute budget before observing
   results.
2. Run a no-training dual-readout compatibility test from one retained encoder
   capture. Compare downstream input distributions, masks, finite values,
   predictions, and task metrics while every weight and augmentation remains
   frozen.
3. If the frozen downstream head is compatible, require precommitted
   noninferiority plus evidence that at least one sequence-sensitive endpoint
   improves before proposing activation.
4. If the head is incompatible because its checkpoint was fitted to the old
   feature semantics, stop and authorize only a bounded **head-only** adaptation
   comparison: encoder frozen, augmentations frozen, equal seeds/data/compute,
   old versus structured readout. Do not interpret an incompatible old head as
   a failure of the repaired representation.
5. Approve a versioned policy activation and checkpoint-migration plan only if
   the appropriate frozen-head or head-only gate passes. Preserve an explicit
   rollback to `all_tokens`.
6. Only after the readout is activated or otherwise held fixed should a new
   augmentation-attribution experiment begin. That later experiment can then
   compare augmentations without the known destructive averaging confound.

## Evidence and authorization boundary

- A3 pre-run manifest: 14,188 bytes, SHA-256
  `eda88225b7726fad9117432b8a2b08654732d1e7bfe6eff05cf4fefd5b0bcbde`.
- A3 authoritative log: 313,122 bytes, SHA-256
  `ae9b0bfc9dc0046f42cbc2f5423955c7b28f8aea39907b4fd32e83a3d0052080`.
- Durable attempt ledger: 547 bytes, SHA-256
  `97ee76a21e0dbfaad3344090f8e753776dab19552bc74a3ee2355dd81149e865`.
- Independent audit log: 3,745 bytes, SHA-256
  `e65912db8f1037519c84a1e7c921794ebd7156859b86d69c5c76648b538fa4b0`.
- Terminal classification:
  `production_structured_readout_parity_reproduced`.
- Audit result: `audit_pass=true`, `audit_error_count=0`,
  `srr2.audit.scientific_success=true`.

The authoritative run and independent audit both end with:

```text
training_authorized=false
augmentation_change_authorized=false
long_run_authorized=false
active_policy_change_authorized=false
checkpoint_migration_authorized=false
downstream_retraining_authorized=false
end_to_end_authorized=false
deployment_authorized=false
```
