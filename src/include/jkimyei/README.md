#Jkimyei

Training orchestration API for wikimyei components.

This room owns training policy, optimizer/loss/scheduler setup, and decoded
training specs. The `jkimyei_specs` parser was copied out of legacy
Camahjucunu because it describes training, not communication.

Current layout:

- `api/`: support API that is neither a training loop nor an evaluation
  artifact surface.
- `evaluation/`: evaluation/report identity helpers.
- `training/`: concrete trainers and launchers.

Selected files:

- `api/training_spec.h`: fresh graph-first training spec decoder
  shared by the split Jkimyei authored surfaces.
- `training/representation/channel_graph_first_representation_launcher.h`:
  channel-preserving VICReg representation training launcher.
- `training/inference/channel_graph_first_inference_launcher.h`: config-backed
  channel-context MDN inference launcher.
- `api/schema_catalog.h`: schema-backed runtime catalog helpers.
- `api/jkimyei_specs.h`: decoded JKSPEC tables and parser entrypoints.
- `api/jkimyei_schema.def`: canonical registry of component/optimizer/scheduler/loss surfaces.
- `api/jkimyei_schema.h`: descriptor arrays generated from `.def`.
- `api/jkimyei_optimizers.h`: optimizer builders and optimizer runtime
  utilities (`clamp_adam_step`).
- `evaluation/source/source_data_analytics.h`: source-data evaluation artifact/report identity helpers.
- `api/jkimyei_lr_schedulers.h`: scheduler builders from decoded
  `jkimyei_specs`.
- `api/jkimyei_losses.h`: loss-row validation.
- `api/jkimyei_setup.h`: component-scoped setup registry and lazy builders.
- `jkimyei.h`: umbrella include for batch-1 API.

Authored training configs are split by worker surface:

- `wikimyei.representation.vicreg.jkimyei` owns representation training
  policy.
- `wikimyei.inference.expected_value.mdn.jkimyei` owns channel-context
  ExpectedValue inference training policy.
- `wikimyei.policy.portfolio.graph_node_allocation.jkimyei` owns the
  pre-PPO policy contract smoke. It is intentionally `OPTIMIZER = noop`,
  requires `causal_walk_forward_training.v1`, and must keep
  `PPO_EXECUTION_ALLOWED = false`.
- The older generic `jkimyei.*` name is not used by the fresh
  graph-first path.
- Runtime `.lls` emission is wave-owned. `.jkimyei` files describe training policy,
  not latent-lineage report persistence.

Runtime boundary:
- `jkimyei` owns policy/schema selection and component setup.
- Cross-worker graph-first protocol now lives under
  `kikijyeba/protocol`; Jkimyei launchers consume that surface.
- Evaluation helpers live here even when they describe source-data reports, so
  Ujcamei can stay focused on source formation.
- runtime episode budgets such as `n_epochs`, `n_iters`, and `batches` stay wave-owned.
- wikimyei components may expose a small runtime `jkimyei` facet for
  inspection and safe control of pending training state, but component-specific
  train loops remain component-owned.
- campaign/runtime orchestration remains undecided in the fresh source. Do not
  revive the old `iitepi` campaign shape as a hidden dependency.

Design goals:
- schema-first definitions (`.def` is source of truth)
- strict allow-lists and no silent defaults
- runtime component setup by explicit DSL text keyed by `contract_hash + component_name`

The copied `jkimyei_specs` declarations now live under the public
`jkimyei/api` path while keeping namespace
`cuwacunu::jkimyei::specs`.
