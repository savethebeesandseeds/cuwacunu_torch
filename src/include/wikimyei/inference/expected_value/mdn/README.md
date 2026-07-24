# Channel-Context MDN Contract

Active MDN prediction is one-step and slot-based.

The model consumes channel node representations:

```text
context      [B,N,C,De]
context_mask [B,N,C]
future       [B,N,C,Df]
future_mask  [B,N,C,Df]
```

It emits one independent univariate mixture distribution for each
anchor/node/channel/target-feature slot:

```text
log_pi [B,N,C,Df,K]
mu     [B,N,C,Df,K]
sigma  [B,N,C,Df,K]
```

The active architecture is:

```text
shared slot trunk
+ low-rank channel adapters
+ shared feature-conditioned MDN head
```

There are no full per-channel heads and no per-node heads. Node identity is
carried by the representation tensor and the explicit `N` axis. Channel
specialization is constrained to residual low-rank adapters, while target
features use learned feature identities inside one shared head. Multi-step
prediction is a future rollout/objective-weighting problem, not an active MDN
output axis.

Feature identities are keyed by the original `TARGET_COORDS` source feature
ids, not by transient output ordinal. The output order is still `[Df]`, but the
embedding table and checkpoint metadata keep the source-coordinate semantics
visible.

`sigma` is emitted with the configured smooth floor already applied:

```text
sigma = sigma_floor + softplus(raw_sigma)
```

The NLL path may still apply an emergency cap/floor for numerical protection,
but Runtime reports should treat emitted `sigma` as the model-space positive
scale.

The active loss is a balanced channel/target-feature NLL: each supported
channel-feature cell contributes through its own masked mean before global
aggregation, so dense validity patterns do not silently dominate sparse ones.

`FUTURE_HORIZON` remains in the config identity for source alignment, but the
active MDN requires it to be `1`.

The single-embedding `[B,De]` MDN wrapper has been removed. The graph-first
production front door is `ChannelContextMdn`.

## Diagnostic Affine Calibration

`PerEdgeAffineReturnHead` is a separate diagnostic-only readout. It is not
wired into `ChannelContextMdn`, its checkpoints, or policy execution. It stores
development-fit float64 normalization and frozen per-edge affine coefficients,
and accepts either post-direct-adapter context `[B,E+1,C,H]` or exact readout
features `[B,E,C,F]`.

The v1 readout contract consumes the first `3H` features as
`[base, quote, base - quote]` and ignores any configured suffix. Its archive
uses strict canonical metadata, a nested model state, and a semantic SHA-256
over metadata plus canonical big-endian tensor bytes. Use it as a frozen truth
oracle for readout diagnostics; do not treat it as a production forecast head
or benchmark-acceptance artifact.
