# LWM-0 — Causal Online-Target Compatibility Findings

Executed 2026-09-05. The bounded implementation/audit goal is complete.
**Mechanics/custody: PASS. Decision: `not_admitted_regularizer`.**

The genuinely temporal contract is legal, differentiable, and predictable on
held-out synthetic groups. Neither tested combined objective preserves every
protected semantic-family floor, so encoder training is not admitted.

## What was implemented

The online encoder processes two ordered history windows and one genuinely later
target window. Their raw supports are `[-7,29]`, `[30,66]`, and `[67,103]`.
The seven-row gaps between feature windows account for rolling-feature lookback.
The action-free predictor maps two structured `[3,32]` states through
`192 -> 128 -> 96` with one GELU. There is no masked-token or EMA-teacher target.

We also corrected the unimplemented roadmap's regularizer definition before
registration. The [LeWM reference](https://github.com/lucas-maes/le-wm/blob/main/train.py)
regularizes encoder states, not prediction errors. [TC-LeWM](https://arxiv.org/html/2607.26924v3)
centers those states over time. The audit retains `[B,W=3,C=3,D=32]`, centers only
W for A1, and averages SIGReg independently over channels. This channel-preserving
extension is a project choice. A0 uses raw states. Both use the documented fixed
coefficient 0.09 and reference-code quadrature, with common fixed directions for
this audit. No projector or coefficient search was introduced.

## The new target is predictably connected to history

Each seed used exactly 512 predictor-only Adam updates on detached embeddings
from 256 SSL groups. Evaluation used 128 new groups, IDs 6000000–6000127. These
are now consumed audit rows, not an untouched confirmation split.

| Seed | Held-out future-state R² | Permuted history R² | Swapped history slots R² | Lowest channel R² |
|---:|---:|---:|---:|---:|
| 17 | 0.749458 | -0.758550 | -0.661621 | 0.714029 |
| 31 | 0.740535 | -0.789742 | -0.773371 | 0.687831 |
| 47 | 0.763528 | -0.774094 | -0.747598 | 0.706367 |

All three seeds pass the precommitted compatibility thresholds. The permutation
and slot-swap controls show that the fitted predictor uses the sample's ordered
history. This R² concerns a new future-embedding task; it is not comparable to
previous representation AULCs or the legacy same-sample target's R².

Future-support interventions left history features, online history latents and
predictions exactly unchanged. History-support interventions left future target
features and latents exactly unchanged. The interventions changed the permitted
side, so the checks were not vacuous. Overlap by one observation, out-of-order
windows and out-of-bounds windows were rejected. Window-zero features reproduced
the original generator exactly both before and after the frozen normalization.

Detached and full-target MSE values were exactly equal. Context-only plus
target-only encoder gradients reconstructed the full gradient in all seeds:
maximum absolute residual `3.638e-9`, maximum relative L2 residual `1.016e-7`.
Full-gradient norm divided by the sum of branch norms was `0.632–0.693`, above
the `0.001` floor. Both branches were finite and nonzero.

## Why training was not admitted

All preliminary temporal-noncollapse, SIGReg fixture, and label-free virtual
structure checks passed. Both candidates also retained aggregate/order AULC,
shuffle limits and every channel geometry gate. The complete failures were these
four semantic-family changes, at the larger virtual radius (0.001 of trunk norm):

| Seed | Combined objective | Failed family | Baseline AULC | Virtual AULC | Change | Required minimum |
|---:|---|---|---:|---:|---:|---:|
| 17 | Prediction + raw SIGReg | multiscale_state | 0.572767 | 0.564893 | -0.007874 | -0.005 |
| 17 | Prediction + centered SIGReg | multiscale_state | 0.572767 | 0.564253 | -0.008514 | -0.005 |
| 31 | Prediction + raw SIGReg | order_regime | 0.537030 | 0.526259 | -0.010771 | -0.005 |
| 31 | Prediction + centered SIGReg | cross_channel | 0.553134 | 0.545318 | -0.007816 | -0.005 |

Seed 47 passed both candidate gates. Each candidate therefore passed in only
1/3 seeds; the protocol requires 3/3. There were no other admission-gate failures.
The scalar aggregate alone would have missed these failures:

| Seed | Baseline AULC | Prediction + raw | Prediction + centered |
|---:|---:|---:|---:|
| 17 | 0.628304 | 0.626814 | 0.625043 |
| 31 | 0.647031 | 0.644264 | 0.645100 |
| 47 | 0.649311 | 0.648516 | 0.648371 |

At the reference coefficient, weighted raw-regularizer trunk gradients had norms
`4.23–4.59`, centered-regularizer gradients `1.39–1.53`, and prediction gradients
`0.137–0.189`. This makes regularizer pressure a useful next hypothesis; gradient
magnitude does not establish the cause of a semantic-family change.

Crucially, full family probes were run on the two **combined** directions. The
prediction-only and regularizer-only directions received the label-free structural
audit, not separate full family probes. Thus `not_admitted_regularizer` names the
failed choice/admission stage; it does not causally attribute the family failures
to SIGReg alone. Neither encoder training nor an Adam trajectory was tested.

## Noncollapse and state checks

Temporal/total centered energy ratios were `0.547–0.559`; all nine time/channel
participation values per seed passed the preliminary 0.10 floor. All 210 protected
virtual metric records are retained, including context-only and target-only
directions. Candidate regularizers alone and combined directions stayed within
the 2% relative structural tolerance at both radii.

SIGReg assigned zero collapse a penalty `25.7310`, versus about `1.086–1.099` for
unit-Gaussian fixtures. Temporally static Gaussian states had raw penalty about
`0.970–1.114`, and centered penalty `25.7310`. Temporal-offset, batch-permutation
and channel-permutation fixtures passed. The exact zero-collapse gradient was
zero: positive collapse penalty is not a guarantee of escape or prevention.

All virtual displacements matched the prescribed norms. Reference encoder,
target/EMA state, other parameters, buffers, mode, gradient slots, and CPU/CUDA
RNG remained unchanged. The shared predictor was fixed after its warm-up.
Restoring each disposable trunk recovered the full original model state.

Total actual updates: **1,536 new-predictor updates; zero encoder optimizer
updates; zero EMA updates.** No checkpoint was written, no production default
changed, and the original confirmation split remained unopened. The targeted
build and one complete audit run passed; preliminary builds were compile-only.

## Stop decision and next unanswered question

LWM-1 stays unopened. Retain this temporal contract and its positive compatibility
evidence; do not revert to the legacy masked same-sample teacher or repeat this
predictability test. The smallest next attribution question is which component
of the combined direction causes the observed family drift, using prediction-only
and regularizer-only family probes at the already fixed states/radius. That would
be a separate zero-update protocol, not a weight sweep or training launch.

The FSPA-4 sparse-readout anchor is preserved. This audit does not establish a
working training recipe, real-data generalization, or downstream benefit.

## Reproduction and evidence

From `/cuwacunu` in the existing `unnamed_taoist` container:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -f Makefile.lwm0 -j12 lwm0
.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_causal_online_target
```

- [Protocol](CAUSAL_ONLINE_TARGET_COMPATIBILITY_PROTOCOL.md):
  `7d011eb373e9eee2927fdf55347a43883c3092db72df04c332effda8095ae3c3`.
- [All virtual directions](CAUSAL_ONLINE_TARGET_COMPATIBILITY_DIRECTIONS.csv):
  `5f29130f83381cf2cf3d6f462337606efa9b81447d622561f84b77a60e17b1aa`.
- [Other measurements](CAUSAL_ONLINE_TARGET_COMPATIBILITY_MEASUREMENTS.csv):
  `843c92b2a729be3340c913064826f6fa6d270bb0093a7416115c33f93859af8d`.
- Machine-local log `.build/tests/representation_lwm0_v1.log`:
  `674ff96eade7bcbaa201492f741d3a3098b010f26d2e083abe209734bf754912`.
- Executed source: `16ffb9acdb144f5be7e415ccb5e0ce9f78db7305acf1ab28b791d5adfe6925c1`.
- SIGReg header: `c0219e6e294dfe5cbb9675266ad6963f6f2800dcf52bab68ad79ac7cd373f09f`.
- Executed binary: `63afe55bc86d9c231ed7fbf299d5b86566c8b0928069ccddab9f9099c2e30090`.

The source, header, protocol, findings and CSV files are portable; `.build` is ignored.
