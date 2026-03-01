/*
  jkimyei_specs.dsl
  =================
  Purpose:
    Central policy registry for training/inference behavior of wikimyei
    components. This file defines profile catalogs; wave selection chooses
    which PROFILE_ID is active for a given component instance.

  Design:
    - Component families are split by canonical component id.
    - Each COMPONENT contains one or more PROFILE blocks.
    - PROFILE blocks contain explicit knobs (optimizer, scheduler, loss,
      numerics, gradient policy, reproducibility, checkpoint, metrics, data ref).
    - No implicit hidden defaults: profile rows should be fully explicit.

  Runtime contract:
    - Wave.WIKIMYEI.PROFILE_ID resolves into a profile under matching COMPONENT.
    - Component-level flags in COMPONENT_PARAMS tune backend behavior.
    - REPRODUCIBILITY and NUMERICS sections define determinism and runtime
      execution settings.

  Main section types:
    SELECTORS
      Keys used by runtime selectors and diagnostics.
    COMPONENT
      Logical component family keyed by canonical id and display label.
    PROFILE
      One concrete training/inference policy configuration.
    AUGMENTATIONS
      Named augmentation catalog referenced by profile params.

  Note:
    Runtime schedule budget (epochs/steps/range) is not defined here; it is
    wave-owned.
*/

/*
  jkimyei_specs.dsl (v2)
  Block-and-colon syntax for high readability and straightforward BNF.

  Scope:
    - central training policy registry for wikimyei components
    - explicit values only (no silent defaults)
    - component-type split sections

  Out of scope (wave-owned runtime budget):
    - n_epochs
    - n_iters
    - batches
*/

JKSPEC 2.0

SELECTORS {
  COMPONENT_ID_KEY: "jkimyei_component_id"
  PROFILE_ID_KEY: "jkimyei_profile_id"
}

/* ========================================================================== */
/* VICReg component family                                                    */
/* ========================================================================== */
COMPONENT "tsi.wikimyei.representation.vicreg" "VICReg_representation" {

  PROFILE "stable_pretrain" {
    OPTIMIZER "AdamW" {
      initial_learning_rate: 0.001
      weight_decay: 0.01
      epsilon: 1e-8
      beta1: 0.9
      beta2: 0.999
      amsgrad: false
    }

    LR_SCHEDULER "ConstantLR" {
      lr: 0.001
    }

    LOSS "VICReg" {
      sim_coeff: 25.0
      std_coeff: 25.0
      cov_coeff: 1.0
      huber_delta: 0.03
    }

    COMPONENT_PARAMS {
      vicreg_train: true
      vicreg_use_swa: true
      vicreg_detach_to_cpu: true
      augmentation_set: "vicreg_aug_default"
      swa_start_iter: 1000
      optimizer_threshold_reset: 500
    }

    REPRODUCIBILITY {
      deterministic: true
      seed: 1337
      workers: 0
      sampler: "sequential"
      shuffle: false
    }

    NUMERICS {
      dtype: "kFloat32"
      device: "gpu"
      amp: false
      grad_scaler: false
      detect_anomaly: false
      eps: 1e-8
    }

    GRADIENT {
      accumulate_steps: 1
      clip_norm: 1.0
      clip_value: 0.0
      skip_on_nan: true
      zero_grad_set_to_none: true
    }

    CHECKPOINT {
      save_model: true
      save_optimizer: true
      save_scheduler: true
      save_rng: true
      strict_load: true
      keep_last: 3
      keep_best: 1
    }

    METRICS {
      objective: "min:loss.total"
      early_stop: "none"
      log_terms: ["loss.total", "loss.inv", "loss.var", "loss.cov", "lr"]
    }

    DATA_REF {
      dataset_id: "default_market_dataset"
      split: "train"
      target_mapping: "none"
      sampler_policy: "sequential_epoch"
    }
  }

  PROFILE "eval_payload_only" {
    OPTIMIZER "AdamW" {
      initial_learning_rate: 0.001
      weight_decay: 0.01
      epsilon: 1e-8
      beta1: 0.9
      beta2: 0.999
      amsgrad: false
    }

    LR_SCHEDULER "ConstantLR" {
      lr: 0.001
    }

    LOSS "VICReg" {
      sim_coeff: 25.0
      std_coeff: 25.0
      cov_coeff: 1.0
      huber_delta: 0.03
    }

    COMPONENT_PARAMS {
      vicreg_train: false
      vicreg_use_swa: true
      vicreg_detach_to_cpu: true
      augmentation_set: "vicreg_aug_default"
      swa_start_iter: 1000
      optimizer_threshold_reset: 500
    }

    REPRODUCIBILITY {
      deterministic: true
      seed: 1337
      workers: 0
      sampler: "sequential"
      shuffle: false
    }

    NUMERICS {
      dtype: "kFloat32"
      device: "gpu"
      amp: false
      grad_scaler: false
      detect_anomaly: false
      eps: 1e-8
    }

    GRADIENT {
      accumulate_steps: 1
      clip_norm: 1.0
      clip_value: 0.0
      skip_on_nan: true
      zero_grad_set_to_none: true
    }

    CHECKPOINT {
      save_model: true
      save_optimizer: true
      save_scheduler: true
      save_rng: true
      strict_load: true
      keep_last: 3
      keep_best: 1
    }

    METRICS {
      objective: "none"
      early_stop: "none"
      log_terms: ["payload.norm", "payload.mean"]
    }

    DATA_REF {
      dataset_id: "default_market_dataset"
      split: "train"
      target_mapping: "none"
      sampler_policy: "sequential_epoch"
    }
  }

  AUGMENTATIONS "vicreg_aug_default" {
    CURVE "Linear" {
      curve_param: 0.0
      noise_scale: 0.02
      smoothing_kernel_size: 3
      point_drop_prob: 0.06
      value_jitter_std: 0.015
      time_mask_band_frac: 0.00
      channel_dropout_prob: 0.00
      comment: "Identity plus light warp jitter and point drop"
    }

    CURVE "ChaoticDrift" {
      curve_param: 0.0
      noise_scale: 0.10
      smoothing_kernel_size: 7
      point_drop_prob: 0.08
      value_jitter_std: 0.020
      time_mask_band_frac: 0.03
      channel_dropout_prob: 0.05
      comment: "Noisier structure with smoothing and channel dropout"
    }
  }

  ACTIVE_PROFILE: "stable_pretrain"
}

/* NOTE: value-estimation/MDN policy is intentionally out of active scope.
   This contract file currently focuses on VICReg representation flows. */
