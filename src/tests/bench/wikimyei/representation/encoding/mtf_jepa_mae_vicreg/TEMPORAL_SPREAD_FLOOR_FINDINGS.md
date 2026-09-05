# LWM-0B — Reference-relative temporal spread floor

Completed 2026-09-05. Decision: **`not_admitted`**. The candidate passes the
collapse fixtures and is inactive at the healthy encoder, but its active
directions violate protected semantic-family floors in seeds 17 and 31.
One valid run; no threshold, coefficient or seed search.

## Implemented candidate

The [spread-floor implementation](lwm0b_spread_floor.h) replaces Gaussian shape
matching with lower bounds on projected batch variance, separately for raw
states and states centered over the true temporal axis. Bounds use half the
reference standard deviation, calibrated on detached fit256 states. Directions,
calibration and coefficient 0.09 stay fixed. The temporal prediction contract,
encoder and production defaults are unchanged.

The regularizer loss and encoder gradient are exactly zero at the healthy
first64 states in all seeds. Consequently, the healthy combined direction
replays prediction alone and passes all guards. That is a reconstruction
control, not independent evidence about active anti-collapse pressure.

## What passed

All representation fixtures passed in all three seeds:

| Fixture | Total penalty range |
|---|---:|
| Healthy | 0 |
| Uniform contraction to 0.1 scale | 0.638604–0.639380 |
| Static over time | 0.498299–0.498306 |
| Temporal residual contracted to 0.1 scale | 0.319279–0.319953 |
| Rank one, original total batch energy retained | 0.109027–0.127234 |
| Near rank one, original total batch energy retained | 0.108495–0.126761 |
| All zero | 0.996004 |

Negative gradients increase batch energy under uniform contraction and temporal
energy under near-static contraction; one small step on representation copies
reduces the respective total penalties. Batch reversal and rank-fixture energy
matching checks pass. Exact zero collapse still has zero gradient: neither
escape from exact collapse nor restoration of rank is established.

## Why the candidate failed

The two mandatory active-direction stress checks differentiate through uniform
or temporal contraction of encoder outputs, then evaluate a normalized step on
a disposable encoder using its original uncontracted readout. At relative trunk
displacement 0.001, the complete family-floor failures are:

| Seed | Active loss intervention | Family | AULC change | Required minimum |
|---:|---|---|---:|---:|
| 17 | Uniform contraction | multiscale_state | -0.008490 | -0.005 |
| 17 | Temporal contraction | multiscale_state | -0.009058 | -0.005 |
| 31 | Uniform contraction | cross_channel | -0.006989 | -0.005 |
| 31 | Temporal contraction | cross_channel | -0.007881 | -0.005 |

Seed 47 passes both active directions. Every aggregate/order score, shuffle,
channel geometry and label-free structural guard passes in all directions.
The candidate therefore passes the complete checks in only **1/3 seeds**.

Removing Gaussian shape matching is insufficient under these local stress
checks. The fixtures show useful corrective pressure in representation space;
the encoder steps still alter protected semantic information. Focus the next
repair on preserving feature relationships during active corrections.
This is a proposed direction, not a new registered experiment.

These synthetic loss interventions are not collapsed encoder checkpoints or
optimizer trajectories. The result rejects this candidate under the registered
stress conditions; it does not prove that all variance floors fail, nor that an
ordinary training trajectory would encounter these exact interventions.

## Validation and retained evidence

The targeted build and one complete audit run passed. All **144 shared replay
fields** (132 numeric and 12 boolean) match LWM-0A exactly. Prediction loss/norm
reconstruction and all realized displacements pass their registered tolerances.
Encoder/copy, fixed predictor, gradient slots and CPU/CUDA RNG preservation pass
in all seeds. Actual updates: **1,536 predictor reconstruction updates, zero
encoder optimizer updates, zero EMA updates**. No checkpoints were written and
confirmation remains unopened. LWM-1 remains closed.

- [Protocol](TEMPORAL_SPREAD_FLOOR_PROTOCOL.md):
  `959d2b7aca6d46311d224a61c3a4b01a94022477a694a013141aa2208353cfd6`.
- [All measurements](TEMPORAL_SPREAD_FLOOR_MEASUREMENTS.csv), 819 fields:
  `a84dc597c9bddc18f7d9a24747022c9e9285ffb7bbf11d524f8548ad0ba8b868`.
- Machine-local log `.build/tests/representation_lwm0b_v1.log`:
  `dc11cf57bda5ab33adc224b4e0686f33600ae11001e24e3f7d46aabecc554400`.
- Executed driver: `ae01bedbc54bee0a8065be004ff83fcd4bb5397a15eb8179d9a2b57440cc43b9`.
- Executed header: `0a8597b60cb403d39267259eb2b3c70afefc7dc19ecdeeebaf85b412836fb186`.
- Executed binary: `3cf4e46500b3b8fa828a0e346ba5aad1e37e836e3fb1ea0df2bb0a50730b105a`.

Reproduce from `/cuwacunu` in the existing `unnamed_taoist` container, with the
authenticated retained authorities and FSPA-4 archives available:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -f Makefile.lwm0b -j12 lwm0b
.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_temporal_spread_floor
```
