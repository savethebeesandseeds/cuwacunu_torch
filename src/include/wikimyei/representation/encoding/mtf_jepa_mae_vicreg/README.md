# MTF-JEPA-MAE-VICReg

This directory contains a separate Wikimyei representation family. It combines
multi-scale time-domain tokens, windowed frequency-magnitude tokens,
context-attentive JEPA latent target prediction, a small auxiliary MAE decoder
over fixed raw time/frequency descriptors, and VICReg-style stabilization
implemented locally in this representation family.

The JEPA loss is the primary pretraining objective. MAE reconstruction is a
small auxiliary branch. The encoder exposes pooled global and per-channel
representations for later downstream use. Downstream forecasting and MDN heads
are intentionally not implemented here. The production VICReg representation
path is intentionally not included or called by this module.

The local bench directory also contains a synthetic pretraining smoke executable
that trains briefly on sinusoid, regime-switching, correlated-channel, and
missing-feature toy data. It is intended only as a training-readiness check, not
as a market-data benchmark.

## VICReg weak views

When `USE_VICREG_LOSS = true`, the encoder creates two independently augmented
views for the local VICReg branch. `VICREG_VIEW_GAUSSIAN_JITTER_STD` controls
masked Gaussian noise and defaults to `0.005`.
`VICREG_VIEW_TIME_DROPOUT_SCALE` defaults to `0.10`; its effective probability
is `min(0.10, MASK_RATIO_TIME * scale)`. These controls are separate from the
launcher-owned outer augmentation stack. Weak-view feature drop also follows
`MASK_RATIO_CHANNEL`.

Setting the Gaussian standard deviation and time-drop scale to zero makes those
two effects identities while still consuming their legacy time-uniform and
Gaussian random tensors. This preserves the random-number schedule for matched
ablation runs; it does not mean that all augmentation is disabled.
