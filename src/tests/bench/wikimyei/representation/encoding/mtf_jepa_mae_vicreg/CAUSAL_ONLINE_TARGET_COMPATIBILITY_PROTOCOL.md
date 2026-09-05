# LWM-0 — Causal Online-Target Compatibility Audit

Registered 2026-09-05 before new measurements. Finite goal: implement and audit
a genuine future-target contract, then make a training-admission decision.
Encoder optimizer updates, EMA updates, checkpoint writes: zero.

## Correction to the proposed roadmap

LeWM applies SIGReg to encoder representations, not prediction errors.
TC-LeWM's residual means a temporally centered representation. Gaussianizing
prediction errors would reward error variance and need not prevent encoder
collapse. That unimplemented roadmap suggestion is replaced here before testing.

Sources: [LeWM training](https://github.com/lucas-maes/le-wm/blob/main/train.py),
[SIGReg implementation](https://github.com/lucas-maes/le-wm/blob/main/module.py),
[default coefficient](https://github.com/lucas-maes/le-wm/blob/main/config/train/lewm.yaml),
[TC-LeWM](https://arxiv.org/html/2607.26924v3).
These inform the contract, not evidence of suitability for this dataset.

## Causal data and encoder boundary

Reuse the isolated synthetic process and nine feature definitions. Generate
observed raw values once with explicit `(group, channel, absolute time)` identity.
Three 30-row windows start at absolute times 0, 37, 74. The eight-row rolling
features give raw supports `[-7,29]`, `[30,66]`, `[67,103]`, respectively.
The first two windows are history; the third is the future target. The 37-row
start-to-start horizon includes seven rows needed to keep rolling support disjoint.
Sharing the process's latent factors is temporal dependence, not raw reuse.

Build features only through their declared raw window. Reject malformed,
out-of-order, overlapping or out-of-bounds plans before encoding. Adversarially
alter all future raw support and require exact history features, latents and
predictions; alter all history raw support and require an exact future target.
Also reject a plan with a one-observation support overlap. Verify window zero
reproduces the existing feature generator exactly before normalization.

Normalization is the existing SSL-only fit. Predictor fit uses groups 0–255;
compatibility evaluation uses fresh groups 6000000–6000127. Do not fit any
normalizer, predictor, or regularizer on evaluation rows. Original RMC development
rows/probes are only protected diagnostics; confirmation stays unopened.
Authenticate the three FSPA-4 archives, fixed dataset, current encoder header and
the existing VVA-1B seam audit. Use seeds 17,31,47 and sparse `[B,3,32]` readout.
Direct online `encode` calls bypass augmentation, masked JEPA, EMA teacher,
reconstruction, VICReg, and TF alignment. All encoder passes use evaluation mode.

## Predictor and branch audit

Concatenate the two ordered history representations (192 values), then use
`Linear(192,128) -> GELU -> Linear(128,96)`, reshaped to `[B,3,32]`.
One common predictor per seed is warmed for exactly 512 Adam updates, LR 0.001,
batch 64, on detached cached SSL embeddings. Each step draws a seeded permutation
of the 256 fit rows and takes 64. There is no encoder graph in warm-up and no
predictor refit between objective arms. Keep the predictor fixed during the audit.

On the first 64 SSL groups compare prediction MSE with detached target versus
full target gradient through the SAME online encoder. Require exact scalar and
prediction-value parity. Separately differentiate context-only, target-only, and
full prediction losses. Require finite nonzero branch/trunk gradients and full
gradient reconstruction (max residual <=5e-5, relative L2 <=1e-4). Require full
norm / (context norm + target norm) >=0.001. Report norms and cosines.

Held-out compatibility uses R² against the fit-set mean future target. Every seed
must achieve overall R² >=0.05 and every channel R² >=0. Whole-history batch
permutation must reduce overall R² by >=0.05; swapping the two history slots must
reduce it by >=0.005. These fixed controls test use of sample identity and order.

## Two fixed noncollapse candidates

Retain a genuine tensor `[B,W=3,C=3,D=32]`. Average per-channel SIGReg; never pool
channels or time into the batch. A0 regularizes raw encoder states. A1 subtracts
the per-sample temporal mean over W only, then regularizes the centered states.
This per-channel extension and W=3 are project choices, not a literal TC-LeWM port.
Use MSE + 0.09 * SIGReg, the reference coefficient, with no weight search.

Use the reference-code characteristic-function convention: 1024 unit Gaussian
projection directions; 17 frequencies `t_j=3j/16`; Gaussian target
`phi_j=exp(-t_j²/2)`; quadrature weights `(3/16)*phi_j` at endpoints and twice
that internally. For each time/channel/projection, sum weighted squared real and
imaginary empirical-CF discrepancies; multiply by batch size, then average over
time, channels and projections. Fix one seeded direction matrix per seed across
all comparisons (an audit-specific departure from per-call resampling).

Fixtures must distinguish an all-zero collapse from unit-Gaussian data (higher
penalty for collapse), show that A1 penalizes temporally static Gaussian states
more than A0, and verify A1 invariance to adding a per-sample/channel constant
across time. Verify batch and channel permutation invariance of the regularizers.
Report the zero gradient at exact zero collapse; a positive penalty alone is not
a guarantee of escape or prevention. At the retained states require positive
energy, temporal/total centered energy ratio >=1e-4, and participation >=0.10
for each time/channel. These are preliminary noncollapse floors, not certification.

## Protected directions and admission

On disposable copies, inspect context-only, target-only, full prediction,
A0 regularizer, A1 regularizer, full+A0 and full+A1 encoder directions. Subtract
each normalized joint trunk gradient at relative parameter radii 0.0005 and 0.001.
Verify realized displacement and unchanged nontrunk parameters/buffers.
Measure the LSA-0 participation/order-separation/channel-contrast diagnostics on
the original development set. Report all directions; each candidate's regularizer
alone AND its full combined direction must preserve every diagnostic within a
2% relative decrease at both radii, in all three seeds.

At the larger radius also run the existing RMC probes for both combined candidates
and their same-seed baseline. Each seed must retain all channel geometry gates,
lose no more than 0.005 AULC in aggregate, order, or any family, and increase neither
shuffled-probe score by more than 0.005. These local-direction safeguards cannot
establish training quality. All numerical failures are `invalid`, not rejections.

Admit only if all mechanical, support, noncollapse, held-out predictor and candidate
direction safeguards pass in 3/3 seeds. Prefer A1 if both qualify; otherwise choose
the sole qualifier. Classify `admitted_temporal_centered`, `admitted_raw`,
`not_admitted_predictor`, or `not_admitted_regularizer` (predictor failure takes
precedence), and list every failed gate. No training begins under this protocol.

Preserve all reference states, gradient slots, mode and CPU/CUDA RNG. Predictor
warm-up changes only the new predictor; no archived optimizer state is present.
Retain separate source, build/run log, protocol digest, results and concise findings.
Stop this goal when the tested contract and admission decision are recorded.
