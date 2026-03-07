/*
  tsi.probe.representation.transfer_matrix_evaluation.dsl
  ==============================
  Purpose:
    Runtime diagnostics + simple probe-MDN architecture policy for
    tsi.probe.representation.transfer_matrix_evaluation.
    This probe checks cargo sanity / anti-look-ahead guardrails and trains
    a lightweight MDN from representation encoding to selected future dims.

  Format:
    <key>:<type> = <value>
*/

check_temporal_order:bool = true
validate_vicreg_out:bool = true
report_shapes:bool = false
reset_hashimyei_on_start:bool = false

mdn_mixture_comps:int = 1
mdn_features_hidden:int = 8
mdn_residual_depth:int = 0
mdn_target_dims:arr[int] = 3
dtype:str = kFloat32
device:str = gpu

optimizer_lr:float = 3e-4
optimizer_weight_decay:float = 1e-4
optimizer_beta1:float = 0.9
optimizer_beta2:float = 0.999
optimizer_eps:float = 1e-8
grad_clip:float = 0.5

nll_eps:float = 1e-5
nll_sigma_min:float = 5e-2
nll_sigma_max:float = 0.0

anchor_train_ratio:float = 0.70
anchor_val_ratio:float = 0.15
anchor_test_ratio:float = 0.15
prequential_blocks:int = 5
control_shuffle_block:int = 32
control_shuffle_seed:int = 1337
linear_ridge_lambda:float = 1e-2
gaussian_var_min:float = 1e-4

summary_every_steps:int = 256
