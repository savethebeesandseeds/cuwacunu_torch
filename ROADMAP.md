# Active Roadmap

This is the short operator-facing roadmap for the current cleanup thread. It is
intentionally small: completed Lattice, Marshal, Runtime wave-ownership, stable
source-range, dataloader, and legacy-cleanup work should not stay here as active
dispatch items.

Subsystem roadmap files may keep design history and receipts. This file names
only the work we still intend to reason about next.

## Active Item

No active cleanup item is currently queued in this file.

## Completed In This Pass

### MDN Semantics And Tensor Shape

Status: implemented for the active channel-context MDN.

Settled contract:

- Input context is `[B,N,C,De]`.
- Input target is one-step `[B,N,C,Df]`.
- Output distributions are `log_pi/mu/sigma [B,N,C,Df,K]`.
- `B` is anchor batch, `N` is graph node, `C` is channel, `Df` is selected
  target feature, and `K` is mixture component.
- The trunk is shared across all slots.
- Heads are channel-specific, not node-specific.
- Target features are independent univariate mixture slots.
- There is no active horizon output axis; `FUTURE_HORIZON` must be `1`.

Acceptance completed:

- A short design note describes the active MDN tensor contract.
- Code names match that contract.
- Tests assert representative input/output ranks and mask behavior.
- Any redesign is deliberately scoped instead of mixed with cleanup.
- Runtime/Lattice report fields continue to describe MDN evidence using the
  settled contract.

## Not Active Here

The following have been resolved or moved out of this active queue and should
not be redispatched from this file:

- Source loading, dataloader behavior, and training stochasticity.
- Stable key-based source ranges.
- Runtime wave ownership.
- Lattice `latest_satisfying` plan-input resolution.
- Marshal target-dispatch simplification.
- Active MDN/VICReg legacy cleanup.
