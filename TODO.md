# Architecture TODO

This file is the short source-of-truth for the graph-first migration. It is not
a migration diary. If a detail is compatibility-only, it must be labeled that
way so it does not pull the architecture backward.

## Target Path

The active direction is node-centered representation and node-centered
ExpectedValue:

```text
Ujcamei graph-edge source
  -> SRL NodeLift
  -> shared node representation encoder
  -> MDN ExpectedValue
  -> distribution over target-side future NodeLift tensors
```

Edges remain essential, but their roles are:

```text
source evidence: graph-collated edge features [B,L,C,H,9]
conditioning:    observed NodeLift -> node representation [B,N,D]
targets:         future NodeLift -> future_node_features/future_node_mask
```

The target architecture is not edge-local representation, edge-conditioned
ExpectedValue, or one independent MDN per edge. Edges remain the source
evidence; ExpectedValue is centered on nodes.

## Non-Negotiable Boundaries

- `ujcamei` owns source evidence, graph metadata, source contracts, storage, and
  graph-edge dataloaders.
- `wikimyei` owns deterministic transforms, representation workers, inference
  workers, and model-facing adapters.
- `jkimyei` owns training and evaluation loops.
- `kikijyeba` owns higher-level composition/orchestration.
- `piaabo` owns generic utilities only.

## NodeLift Rules

Observed NodeLift is conditioning evidence:

```text
edge_features [B,L,C,Hx,9]
edge_mask     [B,L,C,Hx]
  -> node_features [B,C,Hx,N,9]
  -> node representation input
```

Future NodeLift is target-side evidence only:

```text
future_edge_features [B,L,C,Hf,9]
future_edge_mask     [B,L,C,Hf]
  -> future_node_features [B,C,Hf,N,9]
  -> target/diagnostic sidecar
```

Future-lifted tensors must never feed the same-anchor representation encoder or
conditioning path.

Required leakage invariant:

```text
Changing only future_edge_features may change future_node_features.
It must not change observed node_features, node_encoding, or MDN context.
```

## Current Working Path

- `ujcamei/source/dataloader/graph_anchor_edge_dataset.h` forms full active
  graph-edge batches.
- `wikimyei/expression/nodelift/stream/node_lifted_stream.h` applies observed
  SRL NodeLift and carries target-side future NodeLift sidecars.
- `wikimyei/representation/encoding/vicreg/node_stream_adapter.h` adapts
  node-lifted tensors to Rank4 VICReg tensor input.
- `wikimyei/representation/encoding/vicreg/stream/node_representation_stream.h`
  emits node encodings without using future sidecars.
- `wikimyei/inference/expected_value/mdn/stream/mdn_adapter.h`
  flattens node encodings and target-side future NodeLift tensors into MDN
  rows.
- `jkimyei/training/inference/mdn_trainer.h` trains per-node MDN heads with
  masked future node targets.
- `kikijyeba/composition` owns graph-first config bundle/build planning.

## MDN Policy

Default node MDN policy:

```text
per_node
```

Target domain:

```text
node_future
```

The target tensor is `future_node_features [B,C,Hf,N,9]` under
`future_node_mask [B,C,Hf,N,9]`. V1 uses all nine future node feature
coordinates and reduces the coordinate mask with `all_target_features_valid`.
Activity targets in `future_node_features` mean support-normalized node
activity intensity; total activity targets must use the explicit future activity
sidecar later.

The old edge-conditioned MDN route is not an active ExpectedValue path. The
fresh Wikimyei representation surface now targets future node tensors directly.

## Fresh Config Surface

The fresh authored config files are component-specific:

```text
ujcamei.sources.*
ujcamei.channels.*
ujcamei.graph.*
wikimyei.representation.vicreg.*
wikimyei.inference.expected_value.mdn.*
jkimyei.representation.vicreg.*
jkimyei.inference.expected_value.mdn.*
```

The old generic `network_design` parser is not connected to fresh VICReg. If a
new design language is restored later, it should be versioned against the
node-centered representation contract instead of reviving the old wrapper.

## Compatibility-Only Code

These live outside the active graph-first path and must not define the fresh
architecture:

- legacy `network_design` parser/adapters outside the fresh VICReg surface.
- old `iitepi` and `tsiemene` runtime-binding/cargo language.
- future edge observers, if reintroduced, must not define the active
  ExpectedValue target path.

Do not add new fresh-path dependencies on `source_runtime_t::update(contract_key)`.
Use config-backed decoding.

## Completed Recently

- Future NodeLift sidecars added:
  - `future_node_features [B,C,Hf,N,9]`
  - `future_node_mask [B,C,Hf,N,9]`
  - future price residuals/masks
  - future activity sidecars
  - future diagnostics
- Future NodeLift shape validation added.
- No-leakage test added for future-only changes.
- Future residual reconstruction test added.
- Masked-future behavior test added.
- NodeLift timing/complexity bench added.
- Future NodeLift tensors are now the intended ExpectedValue targets.
- `mdn` config/spec surface added with `TARGET_DOMAIN=node_future`.
- `mdn_adapter` added for `[B,N,D] + [B,C,Hf,N,9]`.
- `mdn_trainer` added with per-node heads and global valid-target loss
  normalization.
- graph-first config bundle no longer loads edge-conditioned context config for
  MDN
  inference.
- edge-conditioned context adapter/spec/config/tests removed from fresh Wikimyei
  representation.
- old `ExpectedValue` wrapper and expected-value network-design bridge removed
  from the active tree.
- Config, composition-builder, and graph-first inference launcher smoke tests
  pass against the node-centered MDN path.
- Memory-mapped source storage hardening completed for the graph-training path:
  storage/cache/dataset failures now use catchable exceptions and strict anchor
  probes; source enum conversion failures no longer terminate the process.
- CSV-backed normalized-cache graph-first launcher smoke passes through
  source cache materialization, NodeLift, node representation, and MDN training.
- MDN launcher report/checkpoint cadence is observable in reports:
  checkpoint/report write counts and last write steps are recorded and tested.
- Fresh VICReg cleanup completed:
  - removed old `VICReg_Rank4` wrapper and implementation,
  - removed edge-era VICReg dataloader/dataset adapters,
  - removed VICReg network-design bridge,
  - removed unused projector/loss/augmentation/SWA wrapper surfaces from active
    VICReg.

## Open Boundaries To Remember

These are intentional limits of the current milestone, not reasons to move back
to edge inference:

1. We have not run a large real training job yet.
2. VICReg currently has an inference/encoding core, not a fresh self-supervised
   training core. A clean node-stream `train_one_batch` implementation still
   needs to be added before real VICReg training.
3. MDN v1 uses per-node heads. A shared node MDN with node identity embeddings
   remains a later design option.
4. Future NodeLift is stepwise. It is not a cumulative node trajectory unless a
   separate cumulative observer is explicitly added.
5. Edge-side observers may be reintroduced later, but not as the active
   ExpectedValue route.

## Remaining Work

1. Add stable component/gauge policy if training future node targets across
   horizons.
2. Sweep any remaining non-storage source/exchange fatal exits before
   long-running/live jobs.
3. Add real training jobs with report/checkpoint emission:
   - node representation training after the clean VICReg training core exists,
   - frozen-representation MDN training.
4. Harden production launcher resume beyond smoke tests.
5. Update external/user-facing config docs if a fresh `.man` surface is restored.
6. Quarantine or remove compatibility wrappers only after graph-first real jobs
   pass.

## Do Not Do

- Do not move the architecture back to edge-local representation training.
- Do not treat `edge_specialized` MDN heads as the target design.
- Do not feed future NodeLift sidecars into same-anchor representation or MDN
  conditioning context.
- Do not silently synthesize or remove reverse edges.
- Do not revive `iitepi` or `tsiemene` as fresh architecture pillars.
- Do not build a cache before NodeLift masks, gauges, and target policies are
  stable.
