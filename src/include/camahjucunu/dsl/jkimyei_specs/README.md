# jkimyei_specs (v2)

`jkimyei_specs` now parses the `JKSPEC` block syntax and materializes:

- Legacy compatibility tables consumed by existing runtime code:
  - `components_table`
  - `optimizers_table`
  - `lr_schedulers_table`
  - `loss_functions_table`
  - `vicreg_augmentations`
- Additional profile policy tables for future expansion:
  - `component_profiles_table`
  - `component_reproducibility_table`
  - `component_numerics_table`
  - `component_gradient_table`
  - `component_checkpoint_table`
  - `component_metrics_table`
  - `component_data_ref_table`
  - `selectors_table`

The previous table-frame parser implementation was moved to:
`src/.deprecated/2026-02-27_camahjucunu_jkimyei_specs_reset/`.
