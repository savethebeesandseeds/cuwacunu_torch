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

The trunk is shared across all slots. Channel heads remain channel-specific.
There is no per-node head in the active architecture; node identity is carried by
the representation tensor and the explicit `N` axis. Multi-step prediction is a
future rollout/objective-weighting problem, not an active MDN output axis.

`FUTURE_HORIZON` remains in the config identity for source alignment, but the
active MDN requires it to be `1`.
