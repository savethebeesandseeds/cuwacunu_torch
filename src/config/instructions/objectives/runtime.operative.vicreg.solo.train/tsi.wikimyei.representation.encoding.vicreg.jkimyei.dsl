/*
  Objective-local VICReg jkimyei policy for vicreg.solo.train.

  This file is intentionally small. Legacy ablation profiles were removed so a
  fresh session starts from one canonical long-horizon train policy plus one
  eval-only policy.

  Keep channel_dropout_prob at 0.00 unless there is explicit evidence to
  revisit it. Dropping a whole channel can invalidate every time position for a
  sample under the all-channel VICReg overlap rule.
*/

JKSPEC 2.0

COMPONENT "tsi.wikimyei.representation.encoding.vicreg" {
ACTIVE_PROFILE: "stable_pretrain_linear_only"

PROFILE "stable_pretrain_linear_only" {
    [OPTIMIZER]
      .type: "AdamW"
      .initial_learning_rate: 0.0002
      .weight_decay: 0.01
      .epsilon: 1e-8
      .beta1: 0.9
      .beta2: 0.999
      .amsgrad: false

    [LR_SCHEDULER]
      .type: "CosineAnnealingLR"
      .T_max: 2048
      .eta_min: 0.00003

    [LOSS]
      .type: "VICReg"
      .sim_coeff: 25.0
      .std_coeff: 25.0
      .cov_coeff: 1.0
      .huber_delta: 0.03

    [COMPONENT_PARAMS]
      .vicreg_use_swa: true
      .vicreg_detach_to_cpu: true
      .swa_start_iter: 1000
      .optimizer_threshold_reset: 500

    [GRADIENT]
      .accumulate_steps: 1
      .clip_norm: 1.0
      .clip_value: 0.0
      .skip_on_nan: true
      .zero_grad_set_to_none: true

    [AUGMENTATIONS]
      /------------------------------------------------------------------------------------------------------------------------------------------------------\\
      | time_warp_curve       | Linear                                                        | ChaoticDrift                                                     |
      | active                | true                                                          | false                                                            |
      | curve_param           | 0.0                                                           | 0.0                                                              |
      | noise_scale           | 0.006                                                         | 0.02                                                             |
      | smoothing_kernel_size | 3                                                             | 5                                                                |
      | point_drop_prob       | 0.00                                                          | 0.01                                                             |
      | value_jitter_std      | 0.006                                                         | 0.006                                                            |
      | time_mask_band_frac   | 0.00                                                          | 0.00                                                             |
      | channel_dropout_prob  | 0.00                                                          | 0.00                                                             |
      | comment               | Conservative continuation pass for the existing 0x00FF working candidate | Retired for this cleaned objective bundle; keep disabled unless fresh evidence justifies revival |
      \\------------------------------------------------------------------------------------------------------------------------------------------------------/
}

PROFILE "stable_pretrain_no_swa_low_lr" {
    [OPTIMIZER]
      .type: "AdamW"
      .initial_learning_rate: 0.00011
      .weight_decay: 0.03
      .epsilon: 1e-8
      .beta1: 0.9
      .beta2: 0.999
      .amsgrad: false

    [LR_SCHEDULER]
      .type: "CosineAnnealingLR"
      .T_max: 4096
      .eta_min: 0.000012

    [LOSS]
      .type: "VICReg"
      .sim_coeff: 25.0
      .std_coeff: 25.0
      .cov_coeff: 1.0
      .huber_delta: 0.03

    [COMPONENT_PARAMS]
      .vicreg_use_swa: false
      .vicreg_detach_to_cpu: true
      .swa_start_iter: 6000
      .optimizer_threshold_reset: 500

    [GRADIENT]
      .accumulate_steps: 1
      .clip_norm: 1.5
      .clip_value: 0.0
      .skip_on_nan: true
      .zero_grad_set_to_none: true

    [AUGMENTATIONS]
      /------------------------------------------------------------------------------------------------------------------------------------------------------\\
      | time_warp_curve       | Linear                                                        | ChaoticDrift                                                     |
      | active                | true                                                          | false                                                            |
      | curve_param           | 0.0                                                           | 0.0                                                              |
      | noise_scale           | 0.008                                                         | 0.02                                                             |
      | smoothing_kernel_size | 4                                                             | 5                                                                |
      | point_drop_prob       | 0.01                                                          | 0.01                                                             |
      | value_jitter_std      | 0.007                                                         | 0.006                                                            |
      | time_mask_band_frac   | 0.01                                                          | 0.00                                                             |
      | channel_dropout_prob  | 0.00                                                          | 0.00                                                             |
      | comment               | Low-LR/no-SWA continuation hypothesis on 0x0100 for optimizer-pressure and regularization tuning | Retired baseline candidate variant kept disabled |
      \\------------------------------------------------------------------------------------------------------------------------------------------------------/
}

PROFILE "eval_payload_only" {
    [OPTIMIZER]
      .type: "AdamW"
      .initial_learning_rate: 0.0005
      .weight_decay: 0.01
      .epsilon: 1e-8
      .beta1: 0.9
      .beta2: 0.999
      .amsgrad: false

    [LR_SCHEDULER]
      .type: "ConstantLR"
      .lr: 0.0005

    [LOSS]
      .type: "VICReg"
      .sim_coeff: 25.0
      .std_coeff: 25.0
      .cov_coeff: 1.0
      .huber_delta: 0.03

    [COMPONENT_PARAMS]
      .vicreg_use_swa: true
      .vicreg_detach_to_cpu: true
      .swa_start_iter: 1000
      .optimizer_threshold_reset: 500

    [GRADIENT]
      .accumulate_steps: 1
      .clip_norm: 1.0
      .clip_value: 0.0
      .skip_on_nan: true
      .zero_grad_set_to_none: true

    [AUGMENTATIONS]
      /------------------------------------------------------------------------------------------------------------------------------\\
      | time_warp_curve       | Linear                                        | ChaoticDrift                                         |
      | active                | true                                          | false                                                |
      | curve_param           | 0.0                                           | 0.0                                                  |
      | noise_scale           | 0.01                                          | 0.10                                                 |
      | smoothing_kernel_size | 3                                             | 7                                                    |
      | point_drop_prob       | 0.00                                          | 0.08                                                 |
      | value_jitter_std      | 0.005                                         | 0.020                                                |
      | time_mask_band_frac   | 0.00                                          | 0.03                                                 |
      | channel_dropout_prob  | 0.00                                          | 0.00                                                 |
      | comment               | Evaluation-safe near-identity perturbation    | Disabled by default for eval payload profile         |
      \\------------------------------------------------------------------------------------------------------------------------------/
}

PROFILE "eval_payload_no_swa_low_lr" {
    [OPTIMIZER]
      .type: "AdamW"
      .initial_learning_rate: 0.0004
      .weight_decay: 0.02
      .epsilon: 1e-8
      .beta1: 0.9
      .beta2: 0.999
      .amsgrad: false

    [LR_SCHEDULER]
      .type: "ConstantLR"
      .lr: 0.0004

    [LOSS]
      .type: "VICReg"
      .sim_coeff: 25.0
      .std_coeff: 25.0
      .cov_coeff: 1.0
      .huber_delta: 0.03

    [COMPONENT_PARAMS]
      .vicreg_use_swa: false
      .vicreg_detach_to_cpu: true
      .swa_start_iter: 6000
      .optimizer_threshold_reset: 500

    [GRADIENT]
      .accumulate_steps: 1
      .clip_norm: 1.5
      .clip_value: 0.0
      .skip_on_nan: true
      .zero_grad_set_to_none: true

    [AUGMENTATIONS]
      /------------------------------------------------------------------------------------------------------------------------------\\
      | time_warp_curve       | Linear                                        | ChaoticDrift                                         |
      | active                | true                                          | false                                                |
      | curve_param           | 0.0                                           | 0.0                                                  |
      | noise_scale           | 0.01                                          | 0.10                                                 |
      | smoothing_kernel_size | 4                                             | 7                                                    |
      | point_drop_prob       | 0.01                                          | 0.08                                                 |
      | value_jitter_std      | 0.006                                         | 0.020                                                |
      | time_mask_band_frac   | 0.01                                          | 0.03                                                 |
      | channel_dropout_prob  | 0.00                                          | 0.00                                                 |
      | comment               | Evaluation profile aligned to no-SWA candidate train regime | Disabled by default for eval payload profile         |
      \\------------------------------------------------------------------------------------------------------------------------------/
}
}
