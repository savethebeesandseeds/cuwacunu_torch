/*
  default.tsi.wikimyei.representation.vicreg.jkimyei.dsl
  =================================
  Purpose:
    Declarative training policy for Wikimyei profiles used by the default
    Tsiemene circuit.
    Component identity is explicit:
      canonical_type = "tsi.wikimyei.representation.vicreg"
      canonical_type = "tsi.wikimyei.inference.mdn"
      component_id    = canonical_type (derived by decoder)

  Syntax:
    - Top-level:
        COMPONENT "<canonical_type>" {
          ACTIVE_PROFILE: "<profile_name>"
          PROFILE "<profile_name>" { ... }
        }
    - Section style inside PROFILE:
        [SECTION]
          .key: value
    - AUGMENTATIONS supports row-wise or transposed ascii tables.
    - No implicit defaults in profiles: keys should be explicit.

  Settable keys by section:
    [OPTIMIZER]
      .type + optimizer-specific keys from jkimyei_schema.def
      (for AdamW: .initial_learning_rate .weight_decay .epsilon .beta1 .beta2 .amsgrad)
    [LR_SCHEDULER]
      .type + scheduler-specific keys (for ConstantLR: .lr)
    [LOSS]
      .type + loss-specific keys (for VICReg: .sim_coeff .std_coeff .cov_coeff .huber_delta)
    [COMPONENT_PARAMS] (VICReg)
      .vicreg_use_swa .vicreg_detach_to_cpu .swa_start_iter .optimizer_threshold_reset
    [GRADIENT]
      .accumulate_steps .clip_norm .clip_value .skip_on_nan .zero_grad_set_to_none
    [AUGMENTATIONS]
      columns/fields: time_warp_curve active curve_param noise_scale smoothing_kernel_size
                      point_drop_prob value_jitter_std time_mask_band_frac
                      channel_dropout_prob comment
      time_warp_curve selects the base temporal warp; curve_param,
      noise_scale, and smoothing_kernel_size control how strongly time is
      dilated/compressed before the masking/dropout steps.

  Out of scope (owned by wave/.config, not set here):
    wave TRAIN enable/disable + sampler mode + deterministic seed/workers and runtime budget
    n_epochs, n_iters, batches
    numerics/checkpoint/metrics/data_ref policy families are intentionally not used by VICReg
*/

JKSPEC 2.0

/* ========================================================================== */
/* VICReg component family                                                    */
/* ========================================================================== */
COMPONENT "tsi.wikimyei.representation.vicreg" {
ACTIVE_PROFILE: "stable_pretrain"

PROFILE "stable_pretrain" {
    [OPTIMIZER]
      .type: "AdamW"
      .initial_learning_rate: 0.001
      .weight_decay: 0.01
      .epsilon: 1e-8
      .beta1: 0.9
      .beta2: 0.999
      .amsgrad: false

    [LR_SCHEDULER]
      .type: "ConstantLR"
      .lr: 0.001

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
      /---------------------------------------------------------------------------------------------------------------------------------\
      | time_warp_curve       | Linear                                           | ChaoticDrift                                         |
      | active                | true                                             | true                                                 |
      | curve_param           | 0.0                                              | 0.0                                                  |
      | noise_scale           | 0.02                                             | 0.10                                                 |
      | smoothing_kernel_size | 3                                                | 7                                                    |
      | point_drop_prob       | 0.06                                             | 0.08                                                 |
      | value_jitter_std      | 0.015                                            | 0.020                                                |
      | time_mask_band_frac   | 0.00                                             | 0.03                                                 |
      | channel_dropout_prob  | 0.00                                             | 0.05                                                 |
      | comment               | Identity plus light warp jitter and point drop   | Noisier structure with smoothing and channel dropout |
      \---------------------------------------------------------------------------------------------------------------------------------/
}

PROFILE "eval_payload_only" {
    [OPTIMIZER]
      .type: "AdamW"
      .initial_learning_rate: 0.001
      .weight_decay: 0.01
      .epsilon: 1e-8
      .beta1: 0.9
      .beta2: 0.999
      .amsgrad: false

    [LR_SCHEDULER]
      .type: "ConstantLR"
      .lr: 0.001

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
      /------------------------------------------------------------------------------------------------------------------------------\
      | time_warp_curve       | Linear                                        | ChaoticDrift                                         |
      | active                | true                                          | false                                                |
      | curve_param           | 0.0                                           | 0.0                                                  |
      | noise_scale           | 0.01                                          | 0.10                                                 |
      | smoothing_kernel_size | 3                                             | 7                                                    |
      | point_drop_prob       | 0.00                                          | 0.08                                                 |
      | value_jitter_std      | 0.005                                         | 0.020                                                |
      | time_mask_band_frac   | 0.00                                          | 0.03                                                 |
      | channel_dropout_prob  | 0.00                                          | 0.05                                                 |
      | comment               | Evaluation-safe near-identity perturbation    | Disabled by default for eval payload profile         |
      \------------------------------------------------------------------------------------------------------------------------------/
}
}

/* ========================================================================== */
/* ExpectedValue / MDN inference family                                       */
/* ========================================================================== */
COMPONENT "tsi.wikimyei.inference.mdn" {
ACTIVE_PROFILE: "stable_expectation"

PROFILE "stable_expectation" {
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
      .type: "NLLLoss"
      .eps: 1e-8
      .sigma_min: 1e-4
      .sigma_max: 10.0
      .reduction: "mean"

    [COMPONENT_PARAMS]
      .target_dims: [3]
      /* target_weights are pseudo-likelihood / tempered-score weights inside
         mixture aggregation, not ordinary post-NLL per-target weights. */
      .target_weights: [0.5]
      .grad_clip: 1.0
      .optimizer_threshold_reset: 500

    [NUMERICS]
      .dtype: "kFloat32"
      .device: "gpu"
      .amp: false
      .grad_scaler: false
      .detect_anomaly: false
      .eps: 1e-8

    [GRADIENT]
      .accumulate_steps: 1
      .clip_norm: 1.0
      .clip_value: 0.0
      .skip_on_nan: true
      .zero_grad_set_to_none: true

    [CHECKPOINT]
      .save_model: true
      .save_optimizer: true
      .save_scheduler: true
      .save_rng: false
      .strict_load: true
      .keep_last: 2
      .keep_best: 1

    [METRICS]
      .objective: "future_target_dims_nll"
      .early_stop: "none"
      .log_terms: [nll, expectation]

    [DATA_REF]
      .dataset_id: "runtime_source"
      .split: "train"
      .target_mapping: "future_target_dims"
      .sampler_policy: "wave"
}
}
