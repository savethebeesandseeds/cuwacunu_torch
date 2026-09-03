# IMA-5 Protocol Amendment A2 — Target-Eligible Slot Coverage

## Incident

The A1-corrected invocation stopped at the calibration support guard with:

```text
support_permitted_teacher_target_alignment_error=IMA-5A calibration omits a target slot
```

The temporary failure log has SHA-256
`7550d56a12a8331db6cd322207615f88690df3d2b4eb10842d72328546dd9b93`.
No authoritative IMA-5A log, fitted teacher, candidate measurement, predictor
update, encoder update, or EMA update was produced.

## Source fact exposed by the guard

The original protocol incorrectly required the calibration capture to contain
all 72 representation slots as target identities.  The frozen production
paired-target masker does not permit that.  It first finds matching
time/frequency pairs with identical channel, scale, start, and width, then
retains only pairs at the minimum available width.  Consequently, coarser
slots are legal context slots but can never be JEPA target slots under the
frozen M2 policy.

Requiring those structurally ineligible slots is impossible and does not make
the teacher more support-permitted.

## Bounded correction

Replace only the impossible `all 72 target identities` coverage clause with:

1. Derive the target-eligible slot set from frozen token metadata using the
   exact production paired-target predicate and minimum-width rule.
2. Require every target identity observed in calibration to belong to that
   eligible set.
3. Require every eligible target identity to occur at least once in the fixed
   6M calibration-fit capture for every seed.
4. Record the eligible-slot count, mask hash, and coverage receipt.
5. Define `mu_s` only for eligible target slots.  Ineligible rows in the
   72-slot storage remain zero and are forbidden from teacher queries.

The calibration range remains `[6000000,6000256)`.  Candidate construction,
data rows, seeds, ridge grid, metrics, thresholds, predictor schedule,
confirmation seal, and all scientific stop gates remain unchanged.  The
corrected source must bind A1 and A2, pass the CPU self-test, receive a new
source checksum, and may then rerun IMA-5A.

