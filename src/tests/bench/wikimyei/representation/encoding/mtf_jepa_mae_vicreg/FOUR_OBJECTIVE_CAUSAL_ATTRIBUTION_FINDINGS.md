# OCA-1 — Four-Objective Causal Attribution Findings

Date executed: 2026-08-29

## Decision

All four legacy objectives are mechanically connected to the encoder, but none
is safe to add to the certified repaired representation under its current loss
definition. The retained isolated representation recipe is:

```text
fspa4_minimal_spectral_repair_v1
```

No anchor addition qualified, so the precommitted stop gate correctly kept the
untouched confirmation split closed. The explicit `all_tokens` operational
rollback remains available, but it is not the recommended scientific readout.

## Plain-language answer

The name `MTF-JEPA-MAE-VICReg` describes the four objective mechanisms that the
module implements. It does not mean that all four should be active in the
certified training recipe.

The encoder is capable of learning useful sequence representations. The
certified FSPA-4 encoder through `structured_cdsb_sparse_v1` reproduced its
accepted representation result exactly and passed every frozen gate. The
problem is the legacy objective recipe: TF-alignment and VICReg are strongly
damaging, JEPA is context-dependent and unsafe, and MAE contains useful signal
but trades away protected representation properties when appended to the
repaired encoder.

## Isolation and frozen controls

OCA-1 remained inside the boundary:

```text
sequence -> encoder -> structured_cdsb_sparse_v1 -> fixed lightweight probes
```

- downstream models constructed: `0`;
- training labels used: `false`;
- outer augmentation calls: `0`;
- factorial seeds: `17`, `31`, `47`;
- updates per nonzero factorial arm: `1,536`;
- updates per certified-anchor challenge arm: `512`;
- optimizer: fresh Adam, learning rate `1e-3`;
- gradient clipping threshold: `5.0`;
- identical initialization, rows, masks, weak views, metrics, thresholds, and
  compute budget across paired arms.

The certified FSPA-4 archive hashes passed custody checks. Its revalidated mean
AULC was exactly `0.64154862079148123`; its gain over identical initialization
was `+0.048486854520974267`, with paired 95% interval
`[+0.034668541855527181,+0.062732366623430957]` and three of three seeds
positive. Every family, raw-control, reversal, shuffle, geometry, and mechanics
gate passed before attribution opened.

## Mechanical objective map

The initialization and archived-anchor audits replayed identical masks and weak
views and left the reference models unchanged. Every branch produced a nonzero
served gradient or served update with the expected parameter partitions:

| objective | mechanically connected | expected partitions | interpretation |
|---|---:|---:|---|
| JEPA | yes | pass | the loss reaches the served encoder and predictor |
| MAE | yes | pass | the loss reaches the served encoder and MAE decoder |
| TF-alignment | yes | pass | the loss reaches the served encoder |
| VICReg | yes | pass | the loss reaches the served encoder and VICReg head |

Therefore, poor quality cannot be explained by a disconnected loss, a missing
gradient, or an inactive head.

## Equal-budget factorial from identical initialization

All 15 trained arms completed with finite losses, nonzero gradients and served
updates, exact initialization, exact paired schedules, and expected head
activity. The untrained mean was `0.593061766270507` AULC.

| objective | factorial main effect | paired 95% interval | standalone effect | standalone interval | seeds positive, main | finding |
|---|---:|---:|---:|---:|---:|---|
| JEPA | `+0.022764` | `[+0.018732,+0.026967]` | `-0.035279` | `[-0.048295,-0.023511]` | 3/3 | helps only in some already-damaged combinations; harmful alone |
| MAE | `+0.081127` | `[+0.074379,+0.089282]` | `+0.067290` | `[+0.057425,+0.078348]` | 3/3 | the only strong, reproducible standalone learning signal |
| TF-alignment | `-0.050164` | `[-0.057349,-0.043759]` | `-0.186112` | `[-0.205565,-0.169004]` | 0/3 | reproducibly damaging |
| VICReg | `-0.021951` | `[-0.027753,-0.016691]` | `-0.173327` | `[-0.191032,-0.157228]` | 0/3 | reproducibly damaging |

Selected complete-arm means make the interaction clear:

| arm | mean AULC | effect versus untrained | complete RMC gate |
|---|---:|---:|---:|
| MAE only | `0.660352` | `+0.067290` | fail: geometry |
| JEPA + MAE | `0.651506` | `+0.058444` | fail |
| untrained | `0.593062` | `0` | fail |
| JEPA only | `0.557783` | `-0.035279` | fail |
| all four | `0.539214` | `-0.053847` | fail |
| VICReg only | `0.419735` | `-0.173327` | fail |
| TF-alignment only | `0.406950` | `-0.186112` | fail |

The all-four arm's loss fell and every intended parameter partition changed,
yet representation quality was significantly below initialization: its paired
95% interval was `[-0.066489,-0.041580]`. This is direct evidence that the old
joint optimization target trains successfully in the mechanical sense while
training the representation in the wrong direction.

JEPA's positive averaged main effect must not be read as a standalone success.
Its standalone result is confidence-bounded negative, and its interaction with
MAE is also negative (`-0.076275`). In damaged combinations JEPA sometimes
reduces other objectives' damage; that does not make it a safe encoder goal.

## Certified-anchor challenge

The five precommitted additions started from the identical archived FSPA-4
encoder and differed only in enabled objective coefficients. Qualification
required a gain of at least `+0.005`, a lower bound above zero, all three seeds
improving, preserved family floors, and unchanged reversal, shuffle, geometry,
and mechanics gates.

| addition | mean AULC | delta from FSPA-4 | paired 95% interval | positive seeds | family floor | geometry | qualifies |
|---|---:|---:|---:|---:|---:|---:|---:|
| TF-alignment | `0.622334` | `-0.019215` | `[-0.028864,-0.010175]` | 0/3 | fail | fail | no |
| JEPA | `0.632422` | `-0.009127` | `[-0.019320,+0.000840]` | 1/3 | fail | fail | no |
| MAE | `0.653595` | `+0.012047` | `[+0.003067,+0.021269]` | 1/3 | fail | fail | no |
| VICReg | `0.611449` | `-0.030100` | `[-0.042781,-0.017592]` | 0/3 | fail | fail | no |
| all four | `0.646191` | `+0.004642` | `[-0.006041,+0.015436]` | 2/3 | fail | fail | no |

MAE is the important nuance. Its mean predictive AULC increased, but the gain
was not seed-reproducible and it failed the protected representation contract:
only two of four family changes were positive, the future-family change was
`-0.020165` (below the sealed `-0.02` floor), and the geometry gate failed.
This is why "MAE has useful learning signal" and "current MAE is unsafe to add
to the certified representation" are both true.

The all-four continuation also failed before confirmation: its point gain was
below `+0.005`, its interval crossed zero, only two seeds improved, and its
family and geometry safeguards failed.

## Final objective verdicts

The executable emitted the protocol verdict
`harmful_at_certified_boundary` for all four objectives. In human terms:

| objective | final interpretation |
|---|---|
| JEPA | mechanically real, but harmful alone and not beneficial at the repaired boundary |
| MAE | useful from scratch, but its current loss is not safeguard-preserving at the repaired boundary |
| TF-alignment | consistently harmful from scratch and at the repaired boundary |
| VICReg | consistently harmful from scratch and at the repaired boundary |

No objective qualified; `oca1.anchor_challenge.selected=none`. Confirmation was
not skipped accidentally—it remained closed because the precommitted stop gate
said there was no safe candidate to confirm.

## What this establishes about the encoder

Within the sealed causal sequence benchmark, three fixed seeds, and the frozen
lightweight probes, the evidence now supports all of the following:

1. The encoder and structured sparse readout can represent predictive sequence
   structure; FSPA-4 already certified this with untouched confirmation.
2. Every legacy loss is wired correctly, so wiring is not the explanation for
   the old failure.
3. The simultaneous JEPA + MAE + TF-alignment + VICReg objective is itself a
   representation-quality failure despite successful numerical optimization.
4. MAE identifies a promising mechanism for a future constrained repair, but
   the current MAE loss cannot replace or extend FSPA-4.

This is not a universal guarantee for arbitrary real datasets, unseen regimes,
or downstream tasks. Those are deliberately outside OCA-1. It is a decisive
module-isolated answer on the frozen benchmark that was designed to test
sequence representation rather than velocity or downstream fit.

## Durable artifacts

Primary retained artifacts under `.build/tests/oca1/`:

- full log: `oca1_full_run.log`, SHA-256
  `3ade025b37525c45d376eabaca2c31771ec7d23fbd2f32f26277cec0d9be606d`;
- complete factorial table: `factorial_complete_table.tsv`, SHA-256
  `1a11be29b99f584117b9ba509ed1f62d678f68fd5595dd71039304914235a9ec`;
- factorial seed caches:
  - seed 17: `3ede1b371d37f9cd7e057097970815c878e54d19c289d9b7e590fd534ca68100`;
  - seed 31: `6b4daa8d4b3cc8dd3ccd403551cf5bc554893b32445db1eac1b588b08cf22b31`;
  - seed 47: `ba6a58318eec79e9805543a56fcfd2b338ef339d27478c6d232a1f745cc963c1`;
- anchor-challenge seed caches:
  - seed 17: `5290559894fa6e3f5d2fd57f32a90e97bb0eec924ec22cc9354ae26d0e629c92`;
  - seed 31: `bb5391840ede1ad4aaf8ff760d39b796a7a300918e1aab881fbb5255051fcc39`;
  - seed 47: `aaa7dd4b7638f240d1e940db7ede3bc40eb8ad01000baa90dc043801a29256a6`.

All completed-seed archive checksums exactly matched their commit markers. The
cache round-trip separately proved exact model state, receipt values, row
schedule, structured output, metadata, and checksum recovery.

Protocol SHA-256:
`56bac408b28046e4e014ccd22aab675da9d00b0feb28ed2d25d1debacd57ead2`.

## Retained recipe, rollback, and next question

Retain `fspa4_minimal_spectral_repair_v1` with
`structured_cdsb_sparse_v1`. Do not append the current JEPA, MAE, TF-alignment,
VICReg, or all-four objective to the certified anchor.

Keep `all_tokens` as an explicit operational rollback only. Its information
loss remains established, so rollback availability is not evidence that it is
the preferred representation.

The readout/objective boundary is now resolved. The next isolated question is
augmentation attribution: freeze the certified FSPA-4 recipe and determine
which outer transformations preserve or damage its representation. No
downstream model is needed for that next experiment.

