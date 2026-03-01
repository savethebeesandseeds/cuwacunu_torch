# jkimyei

Training orchestration API for wikimyei components.

Batch-1 layout:
- `specs/jkimyei_schema.def`: canonical registry of component/optimizer/scheduler/loss surfaces.
- `specs/jkimyei_schema.h`: descriptor arrays generated from `.def`.
- `api/schema_catalog.h`: schema-backed runtime catalog helpers.
- `optim/optimizer_utils.h`: optimizer runtime utilities (`clamp_adam_step`).
- `training_setup/jk_optimizers.h`: optimizer builders from decoded `jkimyei_specs`.
- `training_setup/jk_lr_schedulers.h`: scheduler builders from decoded `jkimyei_specs`.
- `training_setup/jk_losses.h`: loss-row validation.
- `training_setup/jk_setup.h`: component-scoped setup registry and lazy builders.
- `jkimyei.h`: umbrella include for batch-1 API.

Design goals:
- schema-first definitions (`.def` is source of truth)
- strict allow-lists and no silent defaults
- runtime component setup by `contract_hash + component_name`
