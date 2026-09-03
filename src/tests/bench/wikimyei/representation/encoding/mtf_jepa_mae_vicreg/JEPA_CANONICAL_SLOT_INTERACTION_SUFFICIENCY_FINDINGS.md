# IMA-4C — Canonical Target × Context-Slot Interaction Findings

## Human conclusion

The frozen encoder is not the immediate failure exposed by this experiment.
Its legal context contains a small measurable target signal, but the present
JEPA predictor/teacher objective still cannot use it reliably.

Giving the predictor an explicit canonical target-slot × source-slot
interaction helped in the expected direction in all three seeds. The gain was
not statistically stable, both trained arms still scored below the target-slot
mean baseline, and both remained far below diagnostic C. Under the frozen
protocol, the narrow predictor-repair line is exhausted.

The precommitted decision is:

```text
predictor_capacity_exhausted_teacher_redesign
```

There is no production candidate and no representation-utility candidate from
IMA-4C. The next localized intervention is a support-permitted teacher-target
redesign, not another nearby predictor variant and not augmentation
attribution.

## What was tested

IMA-4C retained the stronger IMA-4B target-slot output-bias baseline in both
arms and added the same rank-4, per-head target/source pair adapter. Each arm
had 16,352 trainable predictor parameters.

- `canonical_source_labels` received the true canonical source-slot labels.
- `permuted_source_labels` received one deterministic group-specific
  active/inactive-preserving derangement, fixed for that group.

Only the pair adapter's label lookup differed. Context latents, context order,
keys, values, masks, target metadata, teacher targets, batches, optimizer,
dropout RNG, update count, and checkpoint schedule were matched. Each frozen
split owned and hashed its maps once; maps were never regenerated during
training and no tensor carrying representation content was permuted.

Both arms copied the archived production predictor exactly. The output table
and target factor started at zero; the source factor started identically and
nonzero. Therefore both arms were bit-identical at step zero and reproduced
production before learning.

## Mechanical validity

The authoritative audit is valid:

- protocol SHA-256:
  `885c16d3f40cb57d291a2ba4481ed264166765e318e1189b67f8267b7f72c667`;
- source SHA-256:
  `38a8792ea48ecb5aa319921c7402fab2bdca426eb29239d3d027e274fc4b95ef`;
- compiled binary SHA-256:
  `4791a8b825182214d4898723ec1c06b46d630dfb488707281f169d6f97839bbd`;
- authoritative log SHA-256:
  `dd145f8f2b5a314e9850bb4c1860e276740e3e0aaeac4dad897adba539eb9196`;
- the log checksum marker matches;
- final IMA-4B v1a2 source, log, and all three protocol pins matched;
- the standalone CPU mechanics test passed every invariant;
- its matched synthetic A/B reduced held-out canonical interaction loss from
  `0.00339657` to `2.54e-15`, while the independently optimized shuffled arm
  remained at `0.00338842` on unseen group maps;
- all three archived seeds passed custody, component equivalence, capture/map
  hashes, frozen-state equality, frozen-gradient, step-zero production parity,
  paired RNG, optimizer-layout, finiteness, and snapshot checks;
- both arms completed exactly 1,024 predictor updates per seed;
- selected checkpoints were before the 1,024-step cap in every seed, so the
  result is not compute-censored;
- representation optimizer steps: `0`;
- EMA updates: `0`.

The accepted selected steps were:

| Seed | Canonical treatment | Shuffled control |
|---:|---:|---:|
| 17 | 320 | 320 |
| 31 | 768 | 448 |
| 47 | 640 | 320 |

## Held-out representation-target result

The metric is target-slot-centred held-out `R²`; zero is the target-slot mean
baseline.

| Seed | Production current | Diagnostic C | Shuffled control | Canonical treatment | Treatment − control |
|---:|---:|---:|---:|---:|---:|
| 17 | -1.400168 | +0.073997 | -0.105580 | -0.095456 | +0.010124 |
| 31 | -1.556294 | +0.042198 | -0.182179 | -0.125750 | +0.056429 |
| 47 | -1.282124 | +0.008587 | -0.112041 | -0.104446 | +0.007595 |
| **Mean** | **-1.412862** | **+0.041594** | **-0.133267** | **-0.108551** | **+0.024716** |

The canonical arm's advantage had the correct sign in all three seeds, but its
paired 95% interval was `[-0.008929, +0.062574]`. It therefore failed the
precommitted material-effect gate even though the point estimate exceeded the
`+0.02` threshold. Seed 31 supplied most of the magnitude; leaving it out
reduced the mean advantage to `+0.008859`.

The canonical arm did not approach diagnostic C:

- mean `C - treatment = +0.150145`;
- paired 95% interval `[+0.116561, +0.211127]`;
- one-sided upper 95% bound `+0.203920`;
- treatment `R²` was negative in every seed.

The shuffled arm also failed to reach C:

- mean `C - control = +0.174861`;
- paired 95% interval `[+0.146393, +0.236649]`;
- one-sided upper 95% bound `+0.229044`;
- control `R²` was negative in every seed.

Thus neither arm reached C, treatment was not materially superior, and route 6
applies exactly.

## What we learned

Stable canonical source identity is a plausible small design clue: its effect
was positive in every seed under a tightly matched control. It is not a proven
repair. The uncertainty interval crosses zero and the resulting predictions
remain worse than a target-slot mean.

Together, IMA-4B and IMA-4C now rule out the bounded production-shaped family
that was justified by diagnostic C:

- moving categorical target identity into the attention query did not help;
- a target-slot output bias alone did not recover C;
- adding an explicit low-rank target-slot × canonical-source-slot attention
  bias did not recover C.

Continuing with nearby rank changes or adapter variants would be post-result
architecture searching. A full-rank or flattened-field predictor might still
fit diagnostic C, but IMA-4C did not test it, and C itself is only a small
linear diagnostic signal whose representation utility has never been shown.

The most localized explanation now is an objective mismatch: the EMA target
contains target-specific detail that legal support-separated context is not
allowed to observe. Increasing a narrow predictor's routing capacity does not
repair that causal mismatch.

## What this does not prove

IMA-4C does not prove that:

- every possible predictor architecture must fail;
- the encoder is universally adequate or fully repaired;
- diagnostic C's small signal is useful to a downstream task;
- VICReg, TF, JEPA, or MAE are correct in every implementation detail;
- augmentations are harmless;
- any particular replacement teacher target will work.

It does prove that the frozen encoder is not information-empty under legal M2
context and that another narrow target-routing change is not justified by the
current evidence.

## Required next route

Name the next goal:

```text
IMA-5 — Support-Permitted Teacher Target Alignment
```

First run a no-encoder-training target audit on the same frozen, group-disjoint
captures. Candidate teacher abstractions must remove hidden own-support detail,
remain noncollapsed, and be demonstrably predictable from legal context in all
three seeds. Freeze candidates and gates before observing their locked endpoint.

Only a candidate that passes that predictability gate may receive one bounded,
isolated representation-training and utility test. Keep the encoder design,
readout, masks, data rows, seeds, and augmentations fixed during the causal
teacher comparison. If no candidate passes, redesign the JEPA target family
rather than spending more compute on predictor capacity.

Outer augmentation attribution remains closed until the teacher/representation
boundary is resolved and held fixed.

## Evidence files

- Protocol: `JEPA_CANONICAL_SLOT_INTERACTION_SUFFICIENCY_PROTOCOL.md`
- Harness: `quality_wikimyei_mtf_jepa_mae_vicreg_canonical_slot_interaction_sufficiency.cpp`
- Authoritative log: `.build/tests/representation_ima4c_v1_authoritative.log`
- Log checksum: `.build/tests/representation_ima4c_v1_authoritative.log.sha256`
