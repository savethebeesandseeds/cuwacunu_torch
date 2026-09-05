# LWM-0A — Temporal Family Direction Attribution

Completed 2026-09-05. Decision: **`attribution_complete`**.
Training remains **not admitted**. One valid run; zero encoder optimizer or EMA
updates; confirmation remains unopened.

All four retained LWM-0 family-floor failures have the registered classification
**`regularizer_direction`**: the prediction-only direction passes the affected
family floor, while the corresponding SIGReg-only direction fails it. This
narrows the next objective repair to anti-collapse pressure while retaining the
temporal prediction contract.

## Result at the frozen displacement

Each direction uses the same relative trunk displacement, 0.001, on a disposable
copy of the certified FSPA-4 encoder. Values below are family AULC changes from
the unchanged baseline. The required minimum is **-0.005**.

| Seed | Combined candidate | Affected family | Prediction only | Corresponding SIGReg only | Combined replay |
|---:|---|---|---:|---:|---:|
| 17 | Raw | multiscale_state | -0.002429 | -0.007660 | -0.007874 |
| 17 | Centered | multiscale_state | -0.002429 | -0.007974 | -0.008514 |
| 31 | Raw | order_regime | -0.003719 | -0.010719 | -0.010771 |
| 31 | Centered | cross_channel | -0.000040 | -0.007778 | -0.007816 |

The full quality guard includes aggregate/order scores, shuffle controls,
all four semantic families and all three channel geometry checks:

| Direction | Seed 17 | Seed 31 | Seed 47 |
|---|---|---|---|
| Prediction only | PASS | PASS | PASS |
| Raw SIGReg only | FAIL | FAIL | PASS |
| Centered SIGReg only | FAIL | FAIL | PASS |
| Prediction + raw SIGReg | FAIL | FAIL | PASS |
| Prediction + centered SIGReg | FAIL | FAIL | PASS |

Prediction's worst family change is -0.004408 (seed 47, order_regime), still
above the floor. The complete unrounded values, including all safeguards, are
retained in the [measurements](TEMPORAL_FAMILY_DIRECTION_ATTRIBUTION_MEASUREMENTS.csv).

## Replay and preservation

The authenticated LWM-0 implementation supplies the frozen predictor, data,
gradient and evaluation helpers. Its original source and executable were
preserved; the build generates a copy changing only the entry-point name.

Each predictor was reconstructed with the original 512 predictor-only updates
(1,536 total). First/last warm-up summaries replayed exactly. All five loss and
gradient-norm comparisons passed the registered tolerance. All 75 original
label-free virtual metric values replayed within 1e-8; all 15 realized virtual
displacements passed their norm check.

Baseline and both combined full evaluations replayed in every seed. Independent
comparison of the retained logs found **204 numeric fields exactly equal**
(including derived family deltas) and **24 boolean fields identical**. This
complements the executable's replay checks before attribution.

Reference encoder, predictor after reconstruction, gradient slots and CPU/CUDA
RNG passed state-preservation checks in every seed. Restoring each disposable
trunk recovered the complete original model state. FSPA-4 archives remain
unchanged. The targeted build and the single complete audit run passed.

## Interpretation and next bounded question

These are local direction associations at fixed states and radius. Equal-norm
component directions are not additive contributions to the combined step; this
audit does not prove training causality. Prediction passing these guards does
not establish collapse prevention or authorize prediction-only training.

LWM-1 stays closed. Preserve the temporal contract and focus the next design
question on anti-collapse protection that preserves the certified information.
One candidate direction for a separate protocol is a constraint activated by
loss of representation spread or temporal diversity, with explicit collapsed
and near-collapsed fixtures before the same family checks. This is a design
hypothesis, not an admitted objective or a registered experiment. Do not repeat
the temporal predictability audit or launch a coefficient sweep from this result.

## Reproduction and evidence

From `/cuwacunu` in the existing `unnamed_taoist` container:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -f Makefile.lwm0a -j12 lwm0a
.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_family_direction_attribution
```

- [Protocol](TEMPORAL_FAMILY_DIRECTION_ATTRIBUTION_PROTOCOL.md):
  `ff250dfaacdddc120d099d0ed671ea231e39e7a81a18503789ff381eea647d98`.
- [Measurements](TEMPORAL_FAMILY_DIRECTION_ATTRIBUTION_MEASUREMENTS.csv), 504 fields
  including predictor reconstruction summaries:
  `4fd2260ecb3c571a808e203382a22141e3cfefdb40b3dafdab0a0b8765797307`.
- Machine-local log `.build/tests/representation_lwm0a_v1.log`:
  `7d3b92cde331452b656e53c2af04ca229f5645f02f40dac266f97815967e5625`.
- Executed source: `fa1c141c3660fe67310b7811da9fe53d7aa94b57d5082a438009293227d89dd3`.
- Executed binary: `3c22844da6c4951a99e3d3a16a6906aed902e984627e3badd8e8f688c3a89374`.
- Makefile: `e1086b3a2bb0fbe5c4535620c1f0568779d7174b62456de94822db7f3a58c3cc`.

The source, Makefile, protocol, findings and measurements are portable;
reproduction also needs the authenticated LWM-0 authorities and FSPA archives
listed in the driver. `.build` is ignored by Git.
