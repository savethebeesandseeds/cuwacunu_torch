# MTF-JEPA-MAE-VICReg

This directory contains a separate Wikimyei representation family. It combines
multi-scale time-domain tokens, windowed frequency-magnitude tokens,
context-attentive JEPA latent target prediction, a small auxiliary MAE decoder
over fixed raw time/frequency descriptors, and VICReg-style stabilization
implemented locally in this representation family.

The JEPA loss is the primary pretraining objective. MAE reconstruction is a
small auxiliary branch. The encoder exposes pooled global and per-channel
representations for later downstream use. Downstream forecasting and MDN heads
are intentionally not implemented here. The active `cwu_02v` protocol docks
this representation; legacy `cwu_01v` retains the older VICReg encoder.

## Serving pool policy

`SERVING_POOL_POLICY` in the component DSL controls which encoded token pool is
exposed to downstream inference without changing the representation weights or
training objectives:

- `all_tokens` preserves the historical token-count-weighted per-channel mean;
- `time_only` averages time-domain tokens per channel;
- `frequency_only` averages frequency-domain tokens per channel;
- `domain_balanced` equally weights the valid time-domain and frequency-domain
  means per channel;
- `structured_cdsb_v1` is an opt-in, versioned deterministic readout that
  preserves channel, domain, scale, and coarse within-scale position, then uses
  the frozen mean-preserving projection to return the unchanged `[B,C,D]`
  serving shape;
- `structured_cdsb_sparse_v1` is a separately versioned sparse-mask candidate.
  It estimates supported compact cells from valid tokens and fills absent
  within-scale positions with the neutral mean from the same domain and scale
  before applying the unchanged projection. Its public mask means computable,
  not fully observed.

The default and checked-in active policy remain `all_tokens`. Frequency-only,
domain-balanced, and structured serving require frequency tokens. Version 1 of
the structured policy fails closed outside the proven 3-channel, 72-token,
32-wide layout or on a partially observed channel. The sparse policy keeps the
same frozen layout, requires support in every domain/scale group, and is exactly
v1 on complete channel blocks. The policy is part of the
protocol-contract fingerprint, and MDN checkpoints bind the policy they were
trained against. Legacy MDN checkpoints are interpreted as `all_tokens` only.

The local bench directory also contains a synthetic pretraining smoke executable
that trains briefly on sinusoid, regime-switching, correlated-channel, and
missing-feature toy data. It is intended only as a training-readiness check, not
as a market-data benchmark.

## JEPA mask policy

`mtf_jepa_mask_policy_t::legacy_soft_overlap` remains the default and preserves
the historical mask behavior. Both paired-target policies consume the complete
legacy mask random schedule and then deterministically substitute one
finest-scale raw window, represented by its exact time/frequency token pair, as
the JEPA target.

`paired_target_legacy_context_v1` retains overlapping context support and is a
dose-matched experimental control. `support_separated_pair_v1` chooses the
identical target pair but applies the repaired context contract below.

Every same-channel token whose raw interval overlaps that target window is hard
excluded from context, across every scale and both domains. The policy retains
as many legacy context tokens as possible, fills the remainder with
metadata-hashed eligible tokens, and returns exactly
`ceil(min_context_ratio * valid_token_count)` contexts. It never relaxes support
exclusions: a sample without a valid target pair or sufficient separated
context throws instead.

On the frozen 3-channel, 72-token layout with `min_context_ratio = 0.75`, this
means exactly 2 target tokens and 54 context tokens. These policies are
currently module-level experimental opt-ins; the leaky variant exists only for
causal attribution. DSL activation and representation training remain separate
gated steps.

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
