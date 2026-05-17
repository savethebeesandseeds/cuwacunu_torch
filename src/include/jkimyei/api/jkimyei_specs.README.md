# jkimyei_specs (v2)

`jkimyei_specs` now parses the `JKSPEC` block syntax and materializes:

- Input requires explicit `COMPONENT "<canonical_type>" { ... }`
  blocks (`component_id` is derived from canonical type; one DSL file per component
  is supported by contract-space merging).
- Duplicate component canonical types are rejected across merged jkimyei DSL sources.

- Legacy compatibility tables consumed by existing runtime code:
  - `components_table`
  - `optimizers_table`
  - `lr_schedulers_table`
  - `loss_functions_table`
  - `vicreg_augmentations`
- Additional profile policy tables (materialized only when the corresponding
  profile section is present):
  - `component_profiles_table`
  - `component_numerics_table`
  - `component_gradient_table`
  - `component_checkpoint_table`
  - `component_metrics_table`
  - `component_data_ref_table`

`REPRODUCIBILITY` is intentionally not part of `jkimyei_specs`; those controls are sourced from wave/config policy.

The previous table-frame parser implementation was moved to:
`src/.deprecated/2026-02-27_camahjucunu_jkimyei_specs_reset/`.
