# IMA-2 — Feasible Support-Separated Mask Contract

Date: 2026-08-30

## Decision

**The repaired mask contract is feasible.**

`support_separated_pair_v1` passed its zero-update gate. Bounded
representation-only training may now be designed, but no training result or
representation-quality improvement is claimed by IMA-2.

## Repair

IMA-1 proved that six targets plus 54 strictly separated contexts cannot fit in
the active 72-token layout. IMA-2 preserves the informative 54-token context and
changes the target from six unrelated tokens to two representations of one
coherent raw region:

- one finest-scale time token;
- its exact frequency-domain token for the same channel, scale, start, and
  width.

All same-channel tokens whose raw interval overlaps that target region are hard
excluded from context, including other scales and both domains. Different
channels remain eligible because they represent distinct input channels and are
legitimate predictive context.

The policy first executes the complete legacy masker, preserving its random
draw schedule. It then chooses the repaired target and context deterministically
from the consumed legacy result. It retains the maximum available legacy
context, fills only the missing positions with metadata-hashed eligible tokens,
and never relaxes support exclusion. Infeasible samples throw.

`legacy_soft_overlap` remains the default rollback. The repaired policy is an
explicit module-level opt-in and is not activated through the production DSL by
this goal.

## Exhaustive proof

The active layout contains 21 possible finest-scale time/frequency target pairs:
seven windows in each of three channels.

Every candidate was enumerated independently of the production selector:

- target tokens: exactly `2`;
- context tokens required: exactly `54`;
- strict context capacity: minimum `56`, maximum `62`;
- candidates below the required capacity: `0`.

The two-token target therefore leaves a safety margin of at least two eligible
tokens for every possible target window.

## Frozen-schedule proof

The verifier replayed seeds `17`, `31`, and `47` for 512 scheduled masks each:

- samples: `1,536`;
- mask/count/subset/support failures: `0`;
- legacy-versus-repair RNG post-state failures: `0`;
- deterministic replay failures: `0`;
- target candidates exercised: `21 / 21`;
- selections per candidate: minimum `53`, maximum `90`;
- retained legacy contexts per sample: `38` to `51` of `54`;
- replacements per sample: `3` to `16`;
- soft exclusions: `0`;
- relaxed exclusions: `0`;
- target reductions for starvation: `0`;
- context-starved samples: `0`.

A deliberately restricted 24-token sample could not supply its required
separated context and threw the intended geometric-infeasibility error. This
proves the policy fails closed instead of silently leaking target support.

The default legacy path was bit-exact against the explicitly selected legacy
policy. No optimizer or EMA update was constructed.

## What this proves

The masking obstacle found by IMA-1 is repaired at the mechanical and geometric
level. JEPA can now receive a coherent target while preserving 75% visible
context without exposing another token from the same raw target interval.

This does not yet prove that JEPA improves the learned representation. Reducing
the target dose from six tokens to two is necessary for feasibility, but it also
means that a direct comparison against the old six-target treatment combines
target-dose, topology, and support-separation effects.

The next bounded quality experiment should therefore use a dose-matched
three-arm decomposition:

1. cached current six-target legacy control;
2. two-token paired target with a dose-matched legacy/leaky context;
3. two-token paired target with `support_separated_pair_v1` context.

With the cached current control, the two new arms cost
`2 × 3 seeds × 512 updates = 3,072` updates. Encoder architecture, data rows,
outer augmentations, FSPA-4 anchors, structured readout, evaluation gates, and
all non-mask objectives should remain frozen.

## Reproduction

```text
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg -j12 ima2-mask-contract
.build/tests/test_wikimyei_mtf_jepa_mae_vicreg_ima2_mask_contract
```

Evidence SHA-256 values:

- representation module: `b298cb5bace82531fa965d853c99778a5221be3bc17913350d7464b6104a1249`;
- IMA-2 verifier source: `2552aaa7fcc87b4d235831d4214e63202efc3c8df7af47b82e8572b9fa353eac`;
- IMA-2 verifier binary: `b56283edaef9074bfd5818b73e7bee72a3f1e64d1e643f323fa8e0127760af72`.
