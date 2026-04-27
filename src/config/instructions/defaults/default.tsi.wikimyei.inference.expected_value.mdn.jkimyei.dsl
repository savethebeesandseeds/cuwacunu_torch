/*
  default.tsi.wikimyei.inference.expected_value.mdn.jkimyei.dsl
  ============================================================
  Purpose:
    Declarative training policy for the default ExpectedValue / MDN inference
    profile. This file is intentionally separate from the VICReg jkimyei
    policy: VICReg owns representation pretraining controls; ExpectedValue
    owns value-head optimizer, loss, numerics, checkpoint, metrics, and
    data-reference policy. The contract DOCK owns target feature selection.

  Component identity is explicit:
      canonical_type = "tsi.wikimyei.inference.expected_value.mdn"
      component_id    = canonical_type (derived by decoder)

  Out of scope:
    - topology wiring, source range, and wave train/eval selection
    - MDN architecture, which is authored in the network_design DSL
    - representation checkpoint identity, which will be bound by the future
      head routing / compatibility-manifest slice
*/

JKSPEC 2.0

/* ========================================================================== */
/* ExpectedValue / MDN inference family                                       */
/* ========================================================================== */
COMPONENT "tsi.wikimyei.inference.expected_value.mdn" {
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
      .objective: "future_target_feature_indices_nll"
      .early_stop: "none"
      .log_terms: [nll, expectation]

    [DATA_REF]
      .dataset_id: "runtime_source"
      .split: "train"
      .target_mapping: "future_target_feature_indices"
      .sampler_policy: "wave"
}
}
