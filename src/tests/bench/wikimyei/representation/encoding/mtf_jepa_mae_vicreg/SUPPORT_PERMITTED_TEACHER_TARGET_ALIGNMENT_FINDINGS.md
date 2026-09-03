# IMA-5 — Support-Permitted Teacher Target Alignment Findings

## Human conclusion

IMA-5 did **not** find a usable JEPA repair.  It did establish the answer
without changing or retraining the representation encoder.

The new teacher target was constructed only from legal context and passed the
strongest leakage test: changing every hidden target-support token to zeros,
permuted values, large finite noise, or values from another sample changed
neither the legal field nor the target by even one stored bit.  Therefore the
old unobservable-target problem was genuinely removed.

Removing that problem was not enough.  The resulting target was too weak and
poorly structured in the protected time domain, and the unchanged production
predictor could not learn it.  Predictor R2 remained negative in all three
seeds, meaning the validation-selected snapshots performed worse than the
target-slot mean baseline.  Each predictor completed the full 1,024-update
search budget; the selected snapshots were steps `192`, `192`, and `128`.
Order, channel, and channel-permutation structure also missed their frozen
gates.

The precommitted decision is:

```text
jepa_branch_closed_time_target_noncollapse
```

No target was admitted.  IMA-5B is forbidden and was not run.  This saved the
planned 3,072 representation updates.  The encoder checkpoint is unchanged.

## What was tested

The candidate was the cross-fitted, slot-conditioned legal-context residual
teacher:

```text
L_s = C_s(Phi_legal)
```

`Phi_legal` contained the frozen M2 context latents in canonical slot order,
zeros at unavailable slots, all 72 explicit mask bits, and the six-coordinate
target metadata.  `C_s` used the frozen IMA-4A categorical target-slot-by-field
ridge specification.  Slot means, feature normalization, coefficients,
intercept, and alpha selection came only from the disjoint 6M/7M calibration
rows.  Selection rows were not refitted.

The encoder, target encoder, tokenizers, production predictor initialization,
M2 masks, augmentations, FSPA-4 checkpoints, data rows, seeds, metrics, and
thresholds were frozen.  Only a fresh copy of the existing production
predictor was trained.  No downstream model or end-to-end stack was involved.

The support-zeroed target latent was measured only as the non-promotable
deletion control.  It collapsed as predicted in every seed.

## Decisive results

R2 is centered by target slot.  Zero is the slot-mean baseline.

| Seed | Teacher vs exact residual on development | Production predictor combined R2 | Time R2 | Frequency R2 | Time participation rank |
|---:|---:|---:|---:|---:|---:|
| 17 | +0.115339 | -0.103457 | -0.103831 | -0.100315 | 3.733341 |
| 31 | +0.051508 | -0.127721 | -0.123850 | -0.213132 | 3.678394 |
| 47 | +0.021741 | -0.080105 | -0.081639 | -0.065248 | 3.670654 |
| **Mean** | **+0.062863** | **-0.103761** | **-0.103107** | **-0.126232** | **3.694130** |

The admission floors were mean predictor R2 at least `0.50`, every seed at
least `0.25`, and a positive lower interval bound.  The observed combined
bootstrap interval was `[-0.274917,-0.146225]`.  The time interval was
`[-0.277657,-0.142769]`; the frequency interval was
`[-0.291269,-0.174839]`.  This is not a near miss.

These are percentile intervals from the deterministic generated-group
bootstrap.  Each replicate resamples the 256 development groups, recomputes
the nonlinear slot-centered R2 within each seed, and then averages the three
fixed seeds.  They therefore estimate held-out group-level uncertainty rather
than sampling uncertainty over the three displayed seed values, and need not
contain their arithmetic mean.

The time target failed the frozen participation-rank floor of `4.0` in every
seed, even though its effective rank remained above `5.5`.  More importantly,
independent structural tests also failed:

- combined reversal/order AULC was `0.568848`, below `0.60`;
- time-only reversal/order AULC was `0.553385`, below `0.60`;
- frequency-only reversal/order AULC was `0.560872`, below `0.60`;
- channel accuracy was `0.3320`, `0.3359`, and `0.3594`, near three-way
  chance and below the `0.50` mean gate;
- combined cyclic-channel permutation cosine was only `0.1333`, `0.0116`,
  and `0.1279`, below `0.50`, with normalized RMSE above `1.26`;
- the time/frequency encoder-gradient cosine was `-0.1617` in seed 17, below
  the `-0.05` joint-domain floor.

The cross-fitted teacher itself was unstable across row ranges.  Its 7M
calibration-selection R2 against the exact residual was negative in every
seed (`-0.1386`, `-0.0661`, `-0.1189`), while its separate 3M development R2
was small and positive.  This is consistent with a weak, poorly transporting
context-predictable component rather than a robust training target.

## Why the result is valid

- protocol SHA-256: `a44654c7a1d4499b7884555b985bd502abce4d4ae53a8a6cf323eb7981fc1d17`;
- Amendment A1 SHA-256: `a325c187f57adff9eca184055da9efde2fcc08420c71455cf06f837d6db409ef`;
- Amendment A2 SHA-256: `7b0ad04e927415522396a43e793c437a86991226d850c29f158fe2dbb62f533b`;
- source SHA-256: `feb4debb89e444f90be653a187f7a704585315088f75a18575c17b83071fabb9`;
- compiled binary SHA-256: `c3093d32921d95387db3cdf43fb0881b1e878dff36a7a07b4a34f215e5e3613b`;
- authoritative log SHA-256: `c7c70ff7fe84fad19fca222795c8c4aac09687696e7ebc557fab2088f90ea67c`;
- the CPU-only self-test was rerun against that exact binary, exited `0`, and
  emitted `ima5a.self_test.pass=true`; its retained receipt is
  `.build/tests/representation_ima5a_v1_self_test.log`;
- all three seeds passed mechanics, checkpoint custody, component parity,
  frozen-state equality, finiteness, and capture receipts;
- the exact production target policy exposes 42 eligible finest-width slots;
  all 42 occurred in every calibration fit and had the same eligibility-mask
  hash across seeds;
- every hidden-support intervention produced exactly zero field difference
  and exactly zero target difference;
- each predictor completed exactly 1,024 updates;
- selected checkpoints were steps `192`, `192`, and `128`, so no seed was
  compute-censored;
- confirmation group 8M was not generated or opened;
- representation optimizer steps: `0`;
- EMA updates: `0`;
- the authoritative log checksum matches.

Two transparent protocol amendments corrected pre-result mechanics only:

1. A1 changed the unindexed CUDA device to the checkpoint-required `cuda:0`.
2. A2 corrected the impossible demand that all 72 context slots appear as
   targets.  The production mask can target only minimum-width paired slots,
   so the corrected guard requires complete coverage of all 42 mask-eligible
   slots and rejects every ineligible query.

Neither amendment changed data rows, target values, seeds, thresholds,
metrics, model definitions, or compute budgets.

## Post-run independent audit

Three independent read-only audits separated the validity of this measured
rejection from the reusability of the generic harness.

The measured decision passes.  Time participation rank is below the frozen
`4.0` floor in every seed, so the registered decision tree stops at
`jepa_branch_closed_time_target_noncollapse`.  The negative predictor R2,
failed reversal/channel/permutation gates, and one antagonistic domain-gradient
seed independently reinforce rejection.  None of the issues below can turn
that rejection into an admission, and no IMA-5B update is permitted.

The audit did find limitations that must remain visible:

1. The log does not print the shuffled protected-task macro R2 or the
   time/frequency per-channel variance details, although the harness computes
   and gates them.  Those particular conjuncts therefore cannot be audited
   from the retained log alone.
2. The deletion-control summary uses an unregistered per-seed `0.55` order
   cutoff and does not print its component measurements.  D is non-promotable
   and is not part of the actual stop decision.
3. The unused final time-only fallback does not explicitly reject every
   possible shared mechanics/structure failure.  The actual run never reaches
   that fallback: it closes earlier on time noncollapse.  Therefore this v1
   harness is retired with the result and must not be reused to authorize a
   hypothetical time-only candidate without correcting and re-auditing that
   route.
4. The Make target did not enforce the CPU self-test as an automatic
   prerequisite.  The missing durable evidence was repaired after the audit
   by rerunning the CPU-only self-test against the checksummed binary and
   retaining its zero-exit receipt.  No training or scientific measurement
   was repeated.

These are reporting and counterfactual-routing defects, not evidence that the
observed IMA-5 rejection is wrong.  Recording them prevents the authoritative
log from being overstated as a universally reusable harness result.

## What we learned

IMA-4 showed that the exact EMA target contains detail unavailable to legal
context.  IMA-5 removed that unavailable detail completely.  The remaining
legal-context component is still not a good JEPA objective at the current
boundary:

1. it transports weakly across calibration partitions;
2. it loses protected time/channel structure;
3. the existing predictor cannot consume it;
4. time and frequency gradients are not consistently compatible.

Thus the failure is no longer explained only by hidden target support.  The
specific C-derived residual teacher is rejected, and further nearby predictor
changes are already forbidden by IMA-4.  Continuing to tune this JEPA branch
would be post-result search rather than a justified repair.

## What this does not prove

IMA-5 does not prove that JEPA is a bad representation method, that every
possible JEPA target must fail, or that the frozen encoder is universally
correct.  It proves that this implementation's exact target is unobservable
and that the smallest evidence-backed support-permitted replacement still
fails the frozen admission requirements.

It also does not test representation training with L: that run was
intentionally withheld because the zero-update admission gate failed.

## Recommended next isolated goal

Do not run IMA-5B and do not reopen local JEPA predictor search.  OAA-1 already
showed that launcher-owned outer augmentations did not cause or repair the
objective harm.  The next unclosed augmentation boundary is the module-owned
VICReg paired-view construction.

Name the next goal:

```text
VVA-1 — VICReg Paired-View Augmentation Causal Attribution
```

Keep the accepted structured readout, frozen encoder start, rows, seeds,
metrics, and VICReg loss fixed.  Attribute the current internal paired-view
dropout/jitter components one at a time against an identity-view control
before allowing any representation-training repair.  This returns directly
to the user's augmentation hypothesis without bringing back the downstream or
end-to-end stack.

## Evidence files

- Protocol: `SUPPORT_PERMITTED_TEACHER_TARGET_ALIGNMENT_PROTOCOL.md`
- Amendment A1: `SUPPORT_PERMITTED_TEACHER_TARGET_ALIGNMENT_PROTOCOL_AMENDMENT_A1.md`
- Amendment A2: `SUPPORT_PERMITTED_TEACHER_TARGET_ALIGNMENT_PROTOCOL_AMENDMENT_A2.md`
- Harness: `quality_wikimyei_mtf_jepa_mae_vicreg_support_permitted_teacher_target_alignment.cpp`
- Authoritative log: `.build/tests/representation_ima5a_v1_authoritative.log`
- Log checksum: `.build/tests/representation_ima5a_v1_authoritative.log.sha256`
- CPU self-test receipt: `.build/tests/representation_ima5a_v1_self_test.log`
- CPU self-test receipt checksum:
  `.build/tests/representation_ima5a_v1_self_test.log.sha256`
