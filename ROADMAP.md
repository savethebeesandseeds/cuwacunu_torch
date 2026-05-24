# Active Roadmap

This file is the short cross-cutting roadmap for the current Kikijyeba,
Wikimyei, Jkimyei, Lattice, Marshal, and Runtime cleanup thread. It is meant to
keep the open questions finite and visible before work is dispatched into
subsystem-specific goals.

Subsystem roadmap files remain useful for detailed history, but this file is
the current operator-facing index of what we still need to reason about.

## Current Themes

### 1. MDN Semantics And Shape

Goal: make the active MDN shape obvious and defensible.

Questions to settle:

- What exactly is the active channel-context MDN input rank?
- What exactly is the output rank?
- Are heads shared per node, per channel, per node-channel pair, or globally?
- Does the model predict the correct target domain for each anchor, node,
  channel, horizon, and target coordinate?
- Are mask semantics correct for missing source rows, lifted node values, and
  future targets?

Acceptance:

- A short design note describes the active MDN tensor contract.
- Code names match the tensor contract.
- Tests assert representative input/output ranks and mask behavior.
- Any redesign is intentionally scoped instead of mixed with cleanup.

### 2. Source Loading, Dataloader, And Training Stochasticity

Goal: clarify how edge/source data reaches NodeLift and define train/eval
sampling behavior as one dataloader contract.

Questions to settle:

- Which layer materializes memory-mapped source data?
- Does `graph_anchor_edge_dataset_t` read from memory-mapped source stores,
  generated caches, or direct source files?
- Does train mode iterate anchors sequentially today?
- Should train mode use a sampler/shuffle policy distinct from run/eval mode?
- If random sampling is added, where should the seed and reproducibility
  contract live?
- Does VICReg train from sequential wave pulses plus stochastic augmentations,
  or from randomized anchor batches?
- Does MDN train sequentially over the same cursor stream, or with shuffled
  examples?
- Should samplers operate before NodeLift, after NodeLift, or inside each
  component trainer?
- What must be recorded so Lattice can audit training exposure?

Acceptance:

- A source-to-NodeLift data path note names the concrete classes involved.
- Runtime reports expose enough cursor/sampler information to audit a wave.
- Train/eval sampling behavior is explicit and tested.
- Seeds and sampler identities are emitted in reports when randomness affects
  exposure or optimizer order.

### 3. Stable Source Ranges

Goal: evaluate replacing or supplementing unstable anchor-index wave ranges
with key-based ranges.

Questions to settle:

- Are current `anchor_index` ranges stable enough for evidence replay?
- For kline sources, should wave ranges support millisecond timestamp keys?
- How should key ranges map to graph anchors when multiple edges/sources are
  synchronized?
- How should Lattice splits represent protected regions in key space and row
  space?

Acceptance:

- A design decision states whether `anchor_index` remains v1-only or stays as a
  supported range type.
- If key ranges are adopted, wave DSL, split DSL, Runtime reports, and Lattice
  evidence all name the same key-space semantics.
- Leakage checks remain row-space sound even when the operator authored a
  key-space range.

### 4. Legacy Cleanup

Goal: remove active legacy MDN/VICReg implementation paths without losing
historical audit readability.

Decision:

- Active target DSL exposes `channel_mdn_*` readiness only.
- The old target kind is named `legacy_node_mdn_ready` and is accepted only for
  explicit historical fixtures.
- The old `node_mdn_ready` spelling fails with a migration error instead of
  silently selecting compatibility behavior.
- Historical `inference_mdn` facts remain readable where scanners/tests need to
  audit old runtime artifacts.
- Active VICReg is the channel-preserving representation path
  (`channel_representation_vicreg` runtime jobs, `vicreg_representation`
  training task, channel graph-first builder/launcher).
- Legacy node-VICReg headers, trainers, old graph-first representation
  launcher, and non-channel protocol/builder routes are removed from the active
  source tree.
- Historical `representation_vicreg` facts remain readable only where old
  fixtures/receipts must be replayed; Runtime no longer emits that job kind.

Acceptance:

- Active Lattice targets no longer suggest executable legacy MDN waves.
- Active Runtime/Protocol/Jkimyei paths cannot execute node-VICReg.
- Historical evidence parsing is clearly marked compatibility-only, if kept.
- Tests distinguish active channel MDN targets from legacy fixture support.

### 5. Runtime Wave Ownership

Status: implemented for the current cleanup pass.

Goal: make target, source, mode, and model-state selection easy to explain.

Working rule:

- `TARGET` chooses the component family to run.
- `MODE` chooses run/eval versus train mutation.
- `SOURCE_RANGE` chooses the graph-anchor region.
- Source dock/topology chooses the graph and edge/source universe.
- `.jkimyei` chooses training policy and runtime model-state inputs.
- Runtime executes; Lattice proves; Marshal prepares handoff.

Acceptance:

- Runtime docs describe the rule above without legacy MDN ambiguity.
- Runtime preview output exposes target, mode, source range, job kind, and model
  state inputs in one place.
- Runtime/Marshal handoff uses the same canonical active-wave vocabulary:
  `target_component_family_id`, `mode`, `source_range`, anchor/source-key bounds,
  `source_order`, `job_kind`, `train_target`, and `model_state_inputs`.
- `wave_target` and `wave_mode` remain compatibility aliases only.

### 6. Marshal Target Dispatch Simplification

Goal: make pursuing a Lattice target operationally simple without giving Marshal
proof authority.

Target shape:

```text
hero.marshal.prepare_target_dispatch
  input: target_id
  output: operator packet + optional machine payload
  default: no execution, no config edit, no proof claim
```

Acceptance:

- Marshal can prepare one target at a time.
- Lattice owns proof and `latest_satisfying` checkpoint resolution.
- Marshal validates advice and Runtime alignment.
- Runtime remains the only executor.

### 7. Lattice Plan Input Resolution

Goal: give Marshal a read-only way to materialize symbolic model-state hints.

Questions to settle:

- Should this be `hero.lattice.resolve_plan_inputs` or a flag on
  `hero.lattice.plan_target`?
- What proof/certificate should accompany a resolved checkpoint path?
- How fresh must resolver output be for Marshal dry-run preparation?

Acceptance:

- Marshal never chooses checkpoints by filesystem time or metric.
- Resolved checkpoint paths include Lattice-owned proof/closure receipts.
- Unresolved symbolic inputs block Runtime dry-run preparation.

## Suggested Dispatch Order

1. MDN semantics and shape.
2. Source loading, dataloader behavior, and training stochasticity.
3. Stable source ranges.
4. Runtime wave ownership docs.
5. Lattice plan input resolution.
6. Marshal target dispatch simplification.

The first two are coupled: MDN shape and the source/dataloader/sampler contract
define what data the system actually trains on. Stable source ranges should
follow once that data path is clear. Legacy MDN/VICReg cleanup is now
constrained to active-channel runtime paths plus explicit historical evidence
replay support.
