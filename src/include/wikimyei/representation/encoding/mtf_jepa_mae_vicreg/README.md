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
