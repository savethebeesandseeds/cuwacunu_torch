# OAA-1 — Outer Augmentation Causal Attribution Findings

Date executed: 2026-08-29

## Decision

The launcher-owned outer augmentations do **not** repair either the JEPA-only
or VICReg-only representation integration.

- For JEPA, Gaussian jitter, amplitude scaling, and their safe stack made the
  representation measurably worse than JEPA with no outer augmentation.
  Frequency-gain jitter was indistinguishable from doing nothing.
- For VICReg, amplitude scaling produced a small and reproducible improvement
  over VICReg with no outer augmentation, but the resulting representation was
  still substantially worse than the certified FSPA-4 anchor and still failed
  the protected representation-geometry contract. It therefore mitigated a
  little harm; it did not rescue VICReg.
- The other VICReg profiles were unsupported: none demonstrated a reliable
  improvement over VICReg without outer augmentation, and all remained worse
  than the certified anchor.

No rescue candidate existed. The untouched confirmation rows remained closed,
no recipe was promoted, and no production default changed.

## Plain-language answer

There are two different things in this codebase that can be called
"augmentation":

```text
clean sequence
  -> launcher-owned outer preprocessing
  -> module-owned JEPA masks or VICReg paired views
  -> encoder/objective
  -> structured representation
```

OAA-1 tested the first boundary only. It added one safe outer transformation at
a time while freezing the encoder start, loss, internal masks/views, rows,
random schedules, optimizer, readout, and representation-quality tests.

This distinction matters because OCA-1's harmful JEPA and VICReg results used
exactly zero launcher-owned outer-augmentation calls. Therefore those outer
augmentations could not have caused the original harm. OAA-1 asked the only
remaining causal question about them: can adding a safe outer augmentation
repair or reduce that harm? The answer is no for JEPA and only a small,
incomplete reduction for one VICReg treatment.

This is not evidence that JEPA or VICReg are bad general methods. It is evidence
that the remaining fault is in our integration after the outer-preprocessing
boundary. The next highest-value suspects are the module-owned JEPA
context/target mask construction and VICReg paired-view dropout/jitter.

## Isolated experiment

The tested path was exactly:

```text
clean sequence rows
  -> exact production outer-augmentation helper
  -> isolated MTF encoder plus one objective
  -> structured_cdsb_sparse_v1
  -> frozen lightweight representation probes
```

No downstream head, graph, NodeLift, MDN, observer, policy, execution system,
or end-to-end model was constructed. Probe labels were evaluation-only and did
not participate in representation training.

The two objective strata were JEPA-only and VICReg-only. Each was tested with:

1. Gaussian jitter, standard deviation `0.001`;
2. amplitude scaling in `[0.98,1.02)`;
3. frequency-gain jitter, standard deviation `0.01`;
4. the safe stack of those three transformations.

Identity models were reused from the completed OCA-1 cache rather than
retrained. The eight new objective/profile arms used seeds `17`, `31`, and
`47`, with 512 updates per arm: 12,288 new updates in total. Evaluation used
clean, unaugmented inputs after training.

## Primary results

The primary endpoint is clean macro probe AULC. Positive deltas are better.
"Identity" means the same objective trained with no launcher-owned outer
augmentation. "Anchor" means the certified FSPA-4 representation before
adding JEPA or VICReg.

| objective | outer treatment | delta from identity, paired 95% interval | delta from anchor, paired 95% interval | protocol verdict |
|---|---|---:|---:|---|
| JEPA | Gaussian | `-0.000219 [-0.000298,-0.000144]` | `-0.009346 [-0.019525,+0.000595]` | worsens objective |
| JEPA | amplitude | `-0.001366 [-0.002114,-0.000627]` | `-0.010493 [-0.020811,-0.000533]` | worsens objective |
| JEPA | frequency gain | `-0.000019 [-0.000532,+0.000519]` | `-0.009147 [-0.019378,+0.000920]` | not supported |
| JEPA | safe stack | `-0.000611 [-0.001211,-0.000028]` | `-0.009738 [-0.019800,+0.000137]` | worsens objective |
| VICReg | Gaussian | `+0.000066 [-0.000233,+0.000357]` | `-0.030034 [-0.042642,-0.017439]` | not supported |
| VICReg | amplitude | `+0.002853 [+0.000514,+0.005309]` | `-0.027247 [-0.039869,-0.014602]` | mitigates harm only |
| VICReg | frequency gain | `+0.000424 [-0.000391,+0.001292]` | `-0.029676 [-0.042205,-0.017258]` | not supported |
| VICReg | safe stack | `-0.000842 [-0.002187,+0.000511]` | `-0.030942 [-0.043685,-0.018157]` | not supported |

The VICReg amplitude result was positive against identity on all three seeds,
which is why the gate calls it mitigation. Against the anchor, however, all
three seeds were negative. Its four-family floor and geometry gates also
failed. The improvement is therefore real but insufficient and unsafe to
promote.

No arm passed the full representation contract. Every arm failed the absolute
structured-geometry gate and every arm failed the family-floor comparison
against the certified anchor. Lower training loss and nonzero parameter updates
did not substitute for representation quality.

## Mechanical and causal validity

The executable completed with
`execution_status=oaa1_measurements_complete` and
`oaa1.development.experiment_valid=true`.

Before training, all four eligible profiles passed deterministic replay,
support and terminal-anchor preservation, actual forward-input binding, finite
values, and internal-mask-mechanics checks. Custody passed for the sealed
protocol, OCA evidence, FSPA archives, semantic qualifier, production helper,
and identity caches.

During training:

- all 24 seed/objective/profile runs completed their 512 updates;
- each served augmented object was shared exactly between the JEPA and VICReg
  strata for the corresponding seed, profile, and update;
- rows, masks, forward RNG schedules, optimizer, EMA order, and compute budget
  remained paired;
- every arm had finite losses, gradients, updates, and outputs;
- every intended encoder/objective partition changed and every inactive head
  remained unchanged;
- the OCA-equivalent gradient norm and clipping mechanism was retained.

This validates the causal comparison: the outer input values were the intended
treatment, and no downstream or readout change explains the result.

## Stop gate, rollback, and next experiment

The development gate selected `none`, so confirmation correctly remained
unopened (`rows=0`). Promotion is `none`.

Retain the canonical representation recipe:

```text
fspa4_minimal_spectral_repair_v1
  -> structured_cdsb_sparse_v1
```

Keep `all_tokens` only as the explicit operational rollback. Its known sequence
information loss means it is not the scientific recommendation.

The next isolated goal is **IMA-1 — Intrinsic Mask/View Causal Attribution**:

1. freeze the certified encoder boundary and all evaluation machinery;
2. test JEPA context selection and target-mask construction independently;
3. test VICReg paired-view dropout and paired-view jitter independently;
4. use identity/no-perturbation controls and the smallest equal-compute
   factorial that can identify individual and interaction effects;
5. stop if an intrinsic treatment repairs the objective and passes the full
   family, reversal, shuffle, and geometry contract;
6. do not reintroduce downstream or end-to-end components.

This next stage is authorized by the OAA-1 stop gate. It should not begin by
assuming the published JEPA or VICReg ideas are defective; it should determine
whether our masks, paired views, or their interaction with our sequence layout
train the encoder in the wrong direction.

## Durable evidence

- full run log: `.build/tests/oaa1/oaa1_full_run.log`, SHA-256
  `c40fa30c72c2e048a27483827c530e4bda865dc46bac61713795057dd89239c5`;
- executable SHA-256:
  `607f38c8920f955f81fd04ba416b677b5cb5bb1d76bd8f3a35ceca813901d0cb`;
- completed seed-17 cache:
  `40e56f35aab11898edeb8a90de9a2f0ddf73c87fcd7deae13e9d2b6745efc4a7`;
- completed seed-31 cache:
  `0f0eb38b0572e48a2ec41999f12c72d8b3259c2e389b99eadc9c459dccc2e658`;
- completed seed-47 cache:
  `37ba1f33f656bc3d8ca0f03862270983fecb0d05eca4ed884ec46165c769c534`;
- protocol SHA-256:
  `3370817d8e81686961ce87ab8cd99157616e4bc2cee3a9d262f674b1f2f3b4a2`.

Each completed cache has a matching SHA-256 commit marker. The cache smoke test
also passed exact model-state, receipt, row schedule, structured-output,
metadata, and checksum recovery.
