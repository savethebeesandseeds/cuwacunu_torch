// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>

#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::jkimyei::training {

enum class training_task_t {
  mdn_expected_value_inference,
  vicreg_representation,
  mtf_jepa_mae_vicreg_representation,
  policy_graph_node_allocation_ppo_v0,
};

enum class training_optimizer_t {
  adam,
  ppo_clip,
};

inline constexpr const char *k_training_visibility_policy_prior_generation_v1 =
    "prior_generation_per_slice";
inline constexpr const char *k_generation_lane_policy_readiness_v1 =
    "readiness_grade_bootstrap_frozen_init_only";
inline constexpr const char *k_valid_from_policy_fit_end_v1 =
    "valid_from_anchor_gte_fit_end";
inline constexpr const char *k_artifact_provenance_policy_transitive_v1 =
    "transitive_influence_required";

struct training_contract_defaults_t {
  std::string component_assembly_id{};
  std::string no_lookahead_contract_id{};
  std::string no_lookahead_contract_digest{};
  std::string component_order_contract_id{};
  std::string component_order_contract_digest{};
};

struct mtf_jepa_mae_vicreg_training_options_t {
  bool configured{false};
  std::string augmentation_profile{};

  double dropout{0.0};
  double mask_ratio_time{0.0};
  double mask_ratio_frequency{0.0};
  double mask_ratio_channel{0.0};
  double min_context_ratio{1.0};

  double lambda_jepa{1.0};
  double lambda_mae{0.0};
  double lambda_tf_align{0.0};
  double lambda_vicreg{0.0};
  double lambda_global_vicreg{0.0};
  double lambda_channel_vicreg{0.0};

  double vicreg_sim_weight{25.0};
  double vicreg_var_weight{25.0};
  double vicreg_cov_weight{1.0};
  double vicreg_variance_floor{1.0};
  double vicreg_variance_epsilon{1e-4};

  double target_ema_tau{0.996};
  bool use_target_ema{true};
  bool stop_gradient_target{true};
  bool return_diagnostics{true};

  bool use_mae_decoder{true};
  bool use_jepa_loss{true};
  bool use_tf_align_loss{true};
  bool use_vicreg_loss{true};
  bool use_global_vicreg{true};
  bool use_channel_vicreg{false};
  bool use_raw_reconstruction_targets{true};
  bool strict_finite_loss{true};
  bool couple_time_frequency_masks{false};
  bool mask_same_window_across_domains{false};
  bool mask_same_channel_block{false};
  double max_context_target_time_overlap{0.0};

  double gaussian_jitter_std{0.0};
  double feature_dropout_prob{0.0};
  double history_dropout_prob{0.0};
  int64_t time_crop_jitter_max{0};
  double time_dilation_min{1.0};
  double time_dilation_max{1.0};
  double time_warp_max{0.0};
  double amplitude_scale_min{1.0};
  double amplitude_scale_max{1.0};
  double amplitude_shift_std{0.0};
  double frequency_mask_ratio{0.0};
  double frequency_jitter_std{0.0};
  double phase_jitter_max{0.0};
  double channel_dropout_prob{0.0};
  double cross_channel_dropout_prob{0.0};
  double node_dropout_prob{0.0};
  double edge_dropout_prob{0.0};
  double magnitude_normalization_noise_std{0.0};
};

struct training_run_spec_t {
  std::string version_token{"wikimyei.inference.expected_value.mdn.jkimyei.v1"};
  std::string training_id{};
  training_task_t task{training_task_t::mdn_expected_value_inference};
  std::string component_assembly_id{};
  std::string no_lookahead_contract_id{};
  std::string no_lookahead_contract_digest{};
  std::string component_order_contract_id{};
  std::string component_order_contract_digest{};
  std::string component_role{};
  int64_t serving_order_index{-1};
  training_optimizer_t optimizer{training_optimizer_t::adam};
  double learning_rate{0.0};
  int64_t max_steps{0};
  int64_t batch_size{0};
  double grad_clip_norm{0.0};
  double ppo_gamma{0.0};
  double ppo_gae_lambda{0.0};
  double ppo_clip_epsilon{0.0};
  double ppo_target_kl{0.0};
  double ppo_entropy_coeff{0.0};
  double ppo_value_loss_coeff{0.0};
  int64_t ppo_epochs_per_rollout{0};
  int64_t checkpoint_every{0};
  int64_t report_every{1};
  int64_t validation_every{0};
  int64_t seed{0};
  double mdn_edge_return_auxiliary_loss_weight{0.0};
  double mdn_edge_return_auxiliary_direction_weight{0.0};
  double mdn_edge_return_auxiliary_rank_weight{0.0};
  double mdn_edge_return_auxiliary_huber_beta{0.01};
  double mdn_edge_return_auxiliary_logit_scale{50.0};
  bool mdn_direct_edge_return_readout_enabled{false};
  double mdn_direct_edge_return_readout_loss_weight{0.0};
  double mdn_direct_edge_return_readout_direction_weight{0.0};
  double mdn_direct_edge_return_readout_rank_weight{0.0};
  double mdn_direct_edge_return_readout_huber_beta{0.01};
  double mdn_direct_edge_return_readout_logit_scale{50.0};
  bool freeze_representation{true};
  std::string input_representation_checkpoint_path{};
  std::string input_mdn_checkpoint_path{};
  bool allow_untrained_representation{false};
  std::string training_schedule_mode{};
  bool require_causal_schedule{false};
  bool ppo_execution_allowed{false};
  std::string checkpoint_kind{};
  std::string training_visibility_policy{};
  std::string generation_lane_policy{};
  std::string valid_from_policy{};
  std::string artifact_provenance_policy{};
  mtf_jepa_mae_vicreg_training_options_t mtf_jepa_mae_vicreg{};
};

namespace training_spec_detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline training_task_t parse_task(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "mdn_expected_value_inference" ||
      value == "channel_mdn_expected_value_inference") {
    return training_task_t::mdn_expected_value_inference;
  }
  if (value == "vicreg_representation") {
    return training_task_t::vicreg_representation;
  }
  if (value == "mtf_jepa_mae_vicreg_representation" ||
      value == "mtf_jvmae_representation") {
    return training_task_t::mtf_jepa_mae_vicreg_representation;
  }
  if (value == "policy_graph_node_allocation_ppo_v0") {
    return training_task_t::policy_graph_node_allocation_ppo_v0;
  }
  throw std::runtime_error("[training_spec] invalid TASK: " + value);
}

[[nodiscard]] inline training_optimizer_t parse_optimizer(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "adam") {
    return training_optimizer_t::adam;
  }
  if (value == "ppo_clip") {
    return training_optimizer_t::ppo_clip;
  }
  throw std::runtime_error("[training_spec] invalid OPTIMIZER: " + value);
}

[[nodiscard]] inline training_optimizer_t
expected_optimizer(training_task_t task) {
  switch (task) {
  case training_task_t::mdn_expected_value_inference:
  case training_task_t::vicreg_representation:
  case training_task_t::mtf_jepa_mae_vicreg_representation:
    return training_optimizer_t::adam;
  case training_task_t::policy_graph_node_allocation_ppo_v0:
    return training_optimizer_t::ppo_clip;
  }
  throw std::runtime_error("[training_spec] unknown training task");
}

[[nodiscard]] inline const char *expected_version_token(training_task_t task) {
  switch (task) {
  case training_task_t::mdn_expected_value_inference:
    return "wikimyei.inference.expected_value.mdn.jkimyei.v1";
  case training_task_t::vicreg_representation:
    return "wikimyei.representation.vicreg.jkimyei.v1";
  case training_task_t::mtf_jepa_mae_vicreg_representation:
    return "wikimyei.representation.mtf_jepa_mae_vicreg.jkimyei.v1";
  case training_task_t::policy_graph_node_allocation_ppo_v0:
    return "wikimyei.policy.portfolio.graph_node_allocation.jkimyei.v1";
  }
  throw std::runtime_error("[training_spec] unknown training task");
}

[[nodiscard]] inline const char *expected_component_role(training_task_t task) {
  switch (task) {
  case training_task_t::mdn_expected_value_inference:
    return "mdn";
  case training_task_t::vicreg_representation:
  case training_task_t::mtf_jepa_mae_vicreg_representation:
    return "representation";
  case training_task_t::policy_graph_node_allocation_ppo_v0:
    return "policy";
  }
  throw std::runtime_error("[training_spec] unknown training task");
}

[[nodiscard]] inline int64_t
expected_serving_order_index(training_task_t task) {
  switch (task) {
  case training_task_t::vicreg_representation:
  case training_task_t::mtf_jepa_mae_vicreg_representation:
    return 0;
  case training_task_t::mdn_expected_value_inference:
    return 1;
  case training_task_t::policy_graph_node_allocation_ppo_v0:
    return 2;
  }
  throw std::runtime_error("[training_spec] unknown training task");
}

[[nodiscard]] inline bool expected_freeze_representation(training_task_t task) {
  switch (task) {
  case training_task_t::mdn_expected_value_inference:
  case training_task_t::policy_graph_node_allocation_ppo_v0:
    return true;
  case training_task_t::vicreg_representation:
  case training_task_t::mtf_jepa_mae_vicreg_representation:
    return false;
  }
  throw std::runtime_error("[training_spec] unknown training task");
}

[[nodiscard]] inline const char *
expected_training_schedule_mode(training_task_t task) {
  switch (task) {
  case training_task_t::policy_graph_node_allocation_ppo_v0:
    return "causal_walk_forward_training.v1";
  case training_task_t::mdn_expected_value_inference:
  case training_task_t::vicreg_representation:
  case training_task_t::mtf_jepa_mae_vicreg_representation:
    return "";
  }
  throw std::runtime_error("[training_spec] unknown training task");
}

[[nodiscard]] inline bool
expected_require_causal_schedule(training_task_t task) {
  switch (task) {
  case training_task_t::policy_graph_node_allocation_ppo_v0:
    return true;
  case training_task_t::mdn_expected_value_inference:
  case training_task_t::vicreg_representation:
  case training_task_t::mtf_jepa_mae_vicreg_representation:
    return false;
  }
  throw std::runtime_error("[training_spec] unknown training task");
}

[[nodiscard]] inline bool expected_ppo_execution_allowed(training_task_t task) {
  switch (task) {
  case training_task_t::policy_graph_node_allocation_ppo_v0:
    return true;
  case training_task_t::mdn_expected_value_inference:
  case training_task_t::vicreg_representation:
  case training_task_t::mtf_jepa_mae_vicreg_representation:
    return false;
  }
  throw std::runtime_error("[training_spec] unknown training task");
}

[[nodiscard]] inline const char *
expected_checkpoint_kind(training_task_t task) {
  switch (task) {
  case training_task_t::policy_graph_node_allocation_ppo_v0:
    return "ppo_v0_policy_adapter";
  case training_task_t::mdn_expected_value_inference:
  case training_task_t::vicreg_representation:
  case training_task_t::mtf_jepa_mae_vicreg_representation:
    return "";
  }
  throw std::runtime_error("[training_spec] unknown training task");
}

inline void validate_probability(double value, const char *name) {
  if (!std::isfinite(value) || value < 0.0 || value > 1.0) {
    throw std::runtime_error(std::string("[training_spec] invalid ") + name);
  }
}

inline void validate_drop_probability(double value, const char *name) {
  if (!std::isfinite(value) || value < 0.0 || value >= 1.0) {
    throw std::runtime_error(std::string("[training_spec] invalid ") + name);
  }
}

inline void validate_non_negative_finite(double value, const char *name) {
  if (!std::isfinite(value) || value < 0.0) {
    throw std::runtime_error(std::string("[training_spec] invalid ") + name);
  }
}

inline void validate_positive_finite(double value, const char *name) {
  if (!std::isfinite(value) || value <= 0.0) {
    throw std::runtime_error(std::string("[training_spec] invalid ") + name);
  }
}

inline void validate_mtf_jepa_mae_vicreg_training_options(
    const mtf_jepa_mae_vicreg_training_options_t &options) {
  if (!options.configured) {
    throw std::runtime_error(
        "[training_spec] MTF-JEPA-MAE-VICReg training options are required");
  }
  if (options.augmentation_profile.empty()) {
    throw std::runtime_error(
        "[training_spec] AUGMENTATION_PROFILE is required for MTF training");
  }
  if (!std::isfinite(options.dropout) || options.dropout < 0.0 ||
      options.dropout >= 1.0) {
    throw std::runtime_error("[training_spec] invalid MTF DROPOUT");
  }
  validate_probability(options.mask_ratio_time, "MASK_RATIO_TIME");
  validate_probability(options.mask_ratio_frequency, "MASK_RATIO_FREQUENCY");
  validate_probability(options.mask_ratio_channel, "MASK_RATIO_CHANNEL");
  validate_probability(options.min_context_ratio, "MIN_CONTEXT_RATIO");
  validate_probability(options.max_context_target_time_overlap,
                       "MAX_CONTEXT_TARGET_TIME_OVERLAP");
  validate_non_negative_finite(options.lambda_jepa, "LAMBDA_JEPA");
  validate_non_negative_finite(options.lambda_mae, "LAMBDA_MAE");
  validate_non_negative_finite(options.lambda_tf_align, "LAMBDA_TF_ALIGN");
  validate_non_negative_finite(options.lambda_vicreg, "LAMBDA_VICREG");
  validate_non_negative_finite(options.lambda_global_vicreg,
                               "LAMBDA_GLOBAL_VICREG");
  validate_non_negative_finite(options.lambda_channel_vicreg,
                               "LAMBDA_CHANNEL_VICREG");
  validate_non_negative_finite(options.vicreg_sim_weight, "VICREG_SIM_WEIGHT");
  validate_non_negative_finite(options.vicreg_var_weight, "VICREG_VAR_WEIGHT");
  validate_non_negative_finite(options.vicreg_cov_weight, "VICREG_COV_WEIGHT");
  validate_positive_finite(options.vicreg_variance_floor,
                           "VICREG_VARIANCE_FLOOR");
  validate_positive_finite(options.vicreg_variance_epsilon,
                           "VICREG_VARIANCE_EPSILON");
  validate_probability(options.target_ema_tau, "TARGET_EMA_TAU");
  validate_non_negative_finite(options.gaussian_jitter_std,
                               "GAUSSIAN_JITTER_STD");
  validate_drop_probability(options.feature_dropout_prob,
                            "FEATURE_DROPOUT_PROB");
  validate_drop_probability(options.history_dropout_prob,
                            "HISTORY_DROPOUT_PROB");
  if (options.time_crop_jitter_max < 0) {
    throw std::runtime_error(
        "[training_spec] TIME_CROP_JITTER_MAX must be >= 0");
  }
  validate_positive_finite(options.time_dilation_min, "TIME_DILATION_MIN");
  validate_positive_finite(options.time_dilation_max, "TIME_DILATION_MAX");
  if (options.time_dilation_min > options.time_dilation_max) {
    throw std::runtime_error(
        "[training_spec] TIME_DILATION_MIN must be <= TIME_DILATION_MAX");
  }
  validate_probability(options.time_warp_max, "TIME_WARP_MAX");
  validate_positive_finite(options.amplitude_scale_min, "AMPLITUDE_SCALE_MIN");
  validate_positive_finite(options.amplitude_scale_max, "AMPLITUDE_SCALE_MAX");
  if (options.amplitude_scale_min > options.amplitude_scale_max) {
    throw std::runtime_error(
        "[training_spec] AMPLITUDE_SCALE_MIN must be <= AMPLITUDE_SCALE_MAX");
  }
  validate_non_negative_finite(options.amplitude_shift_std,
                               "AMPLITUDE_SHIFT_STD");
  validate_drop_probability(options.frequency_mask_ratio,
                            "FREQUENCY_MASK_RATIO");
  validate_non_negative_finite(options.frequency_jitter_std,
                               "FREQUENCY_JITTER_STD");
  validate_non_negative_finite(options.phase_jitter_max, "PHASE_JITTER_MAX");
  validate_drop_probability(options.channel_dropout_prob,
                            "CHANNEL_DROPOUT_PROB");
  validate_drop_probability(options.cross_channel_dropout_prob,
                            "CROSS_CHANNEL_DROPOUT_PROB");
  validate_drop_probability(options.node_dropout_prob, "NODE_DROPOUT_PROB");
  if (options.edge_dropout_prob != 0.0) {
    throw std::runtime_error(
        "[training_spec] EDGE_DROPOUT_PROB is not supported by the current "
        "MTF channel-node input");
  }
  validate_non_negative_finite(options.magnitude_normalization_noise_std,
                               "MAGNITUDE_NORMALIZATION_NOISE_STD");
}

} // namespace training_spec_detail

[[nodiscard]] inline training_contract_defaults_t
canonical_training_contract_defaults() {
  return {};
}

inline void apply_training_run_spec_canonical_defaults(
    training_run_spec_t &spec, const training_contract_defaults_t &defaults) {
  if (spec.component_assembly_id.empty()) {
    spec.component_assembly_id = defaults.component_assembly_id;
  }
  if (spec.no_lookahead_contract_id.empty()) {
    spec.no_lookahead_contract_id = defaults.no_lookahead_contract_id;
  }
  if (spec.no_lookahead_contract_digest.empty()) {
    spec.no_lookahead_contract_digest = defaults.no_lookahead_contract_digest;
  }
  if (spec.component_order_contract_id.empty()) {
    spec.component_order_contract_id = defaults.component_order_contract_id;
  }
  if (spec.component_order_contract_digest.empty()) {
    spec.component_order_contract_digest =
        defaults.component_order_contract_digest;
  }
  if (spec.component_role.empty()) {
    spec.component_role =
        training_spec_detail::expected_component_role(spec.task);
  }
  if (spec.serving_order_index < 0) {
    spec.serving_order_index =
        training_spec_detail::expected_serving_order_index(spec.task);
  }
  if (spec.training_visibility_policy.empty()) {
    spec.training_visibility_policy =
        k_training_visibility_policy_prior_generation_v1;
  }
  if (spec.generation_lane_policy.empty()) {
    spec.generation_lane_policy = k_generation_lane_policy_readiness_v1;
  }
  if (spec.valid_from_policy.empty()) {
    spec.valid_from_policy = k_valid_from_policy_fit_end_v1;
  }
  if (spec.artifact_provenance_policy.empty()) {
    spec.artifact_provenance_policy =
        k_artifact_provenance_policy_transitive_v1;
  }
}

inline void
apply_training_run_spec_canonical_defaults(training_run_spec_t &spec) {
  apply_training_run_spec_canonical_defaults(
      spec, canonical_training_contract_defaults());
}

inline void
validate_training_run_spec(const training_run_spec_t &spec,
                           const training_contract_defaults_t &defaults) {
  const std::string expected =
      training_spec_detail::expected_version_token(spec.task);
  if (spec.version_token != expected) {
    throw std::runtime_error(
        "[training_spec] unsupported version token for task");
  }
  if (spec.training_id.empty()) {
    throw std::runtime_error("[training_spec] training_id is required");
  }
  if (spec.component_assembly_id.empty()) {
    throw std::runtime_error(
        "[training_spec] component_assembly_id is required");
  }
  if (!defaults.component_assembly_id.empty() &&
      spec.component_assembly_id != defaults.component_assembly_id) {
    throw std::runtime_error("[training_spec] component_assembly_id mismatch");
  }
  const bool is_policy_ppo_v0 =
      spec.task == training_task_t::policy_graph_node_allocation_ppo_v0;
  if (!is_policy_ppo_v0 && spec.optimizer != training_optimizer_t::adam) {
    throw std::runtime_error("[training_spec] v1 supports adam only for "
                             "trainable representation/inference tasks");
  }
  if (is_policy_ppo_v0 && spec.optimizer != training_optimizer_t::ppo_clip) {
    throw std::runtime_error(
        "[training_spec] graph-node PPO V0 requires OPTIMIZER = ppo_clip");
  }
  if (spec.learning_rate <= 0.0) {
    throw std::runtime_error("[training_spec] learning_rate must be > 0");
  }
  if (spec.max_steps < 0 || spec.batch_size <= 0 || spec.grad_clip_norm < 0.0 ||
      spec.checkpoint_every < 0 || spec.report_every <= 0 ||
      spec.validation_every < 0 || spec.seed < 0) {
    throw std::runtime_error(
        "[training_spec] invalid training cadence/options");
  }
  if (spec.validation_every != 0) {
    throw std::runtime_error(
        "[training_spec] VALIDATION_EVERY is not implemented in v1; set it "
        "to 0");
  }
  const bool is_mdn_training =
      spec.task == training_task_t::mdn_expected_value_inference;
  const bool is_representation_training =
      spec.task == training_task_t::vicreg_representation ||
      spec.task == training_task_t::mtf_jepa_mae_vicreg_representation;
  const bool is_mtf_representation_training =
      spec.task == training_task_t::mtf_jepa_mae_vicreg_representation;
  if (is_mdn_training && !spec.freeze_representation) {
    throw std::runtime_error(
        "[training_spec] v1 MDN ExpectedValue training requires frozen "
        "representation");
  }
  if (is_mdn_training) {
    training_spec_detail::validate_non_negative_finite(
        spec.mdn_edge_return_auxiliary_loss_weight,
        "MDN_EDGE_RETURN_AUXILIARY_LOSS_WEIGHT");
    training_spec_detail::validate_non_negative_finite(
        spec.mdn_edge_return_auxiliary_direction_weight,
        "MDN_EDGE_RETURN_AUXILIARY_DIRECTION_WEIGHT");
    training_spec_detail::validate_non_negative_finite(
        spec.mdn_edge_return_auxiliary_rank_weight,
        "MDN_EDGE_RETURN_AUXILIARY_RANK_WEIGHT");
    training_spec_detail::validate_positive_finite(
        spec.mdn_edge_return_auxiliary_huber_beta,
        "MDN_EDGE_RETURN_AUXILIARY_HUBER_BETA");
    training_spec_detail::validate_positive_finite(
        spec.mdn_edge_return_auxiliary_logit_scale,
        "MDN_EDGE_RETURN_AUXILIARY_LOGIT_SCALE");
    training_spec_detail::validate_non_negative_finite(
        spec.mdn_direct_edge_return_readout_loss_weight,
        "MDN_DIRECT_EDGE_RETURN_READOUT_LOSS_WEIGHT");
    training_spec_detail::validate_non_negative_finite(
        spec.mdn_direct_edge_return_readout_direction_weight,
        "MDN_DIRECT_EDGE_RETURN_READOUT_DIRECTION_WEIGHT");
    training_spec_detail::validate_non_negative_finite(
        spec.mdn_direct_edge_return_readout_rank_weight,
        "MDN_DIRECT_EDGE_RETURN_READOUT_RANK_WEIGHT");
    training_spec_detail::validate_positive_finite(
        spec.mdn_direct_edge_return_readout_huber_beta,
        "MDN_DIRECT_EDGE_RETURN_READOUT_HUBER_BETA");
    training_spec_detail::validate_positive_finite(
        spec.mdn_direct_edge_return_readout_logit_scale,
        "MDN_DIRECT_EDGE_RETURN_READOUT_LOGIT_SCALE");
    const bool direct_readout_has_weight =
        spec.mdn_direct_edge_return_readout_loss_weight > 0.0 ||
        spec.mdn_direct_edge_return_readout_direction_weight > 0.0 ||
        spec.mdn_direct_edge_return_readout_rank_weight > 0.0;
    if (direct_readout_has_weight &&
        !spec.mdn_direct_edge_return_readout_enabled) {
      throw std::runtime_error(
          "[training_spec] MDN direct edge-return readout weights require "
          "MDN_DIRECT_EDGE_RETURN_READOUT_ENABLED=true");
    }
  } else if (spec.mdn_edge_return_auxiliary_loss_weight != 0.0 ||
             spec.mdn_edge_return_auxiliary_direction_weight != 0.0 ||
             spec.mdn_edge_return_auxiliary_rank_weight != 0.0 ||
             spec.mdn_direct_edge_return_readout_enabled ||
             spec.mdn_direct_edge_return_readout_loss_weight != 0.0 ||
             spec.mdn_direct_edge_return_readout_direction_weight != 0.0 ||
             spec.mdn_direct_edge_return_readout_rank_weight != 0.0) {
    throw std::runtime_error(
        "[training_spec] MDN edge-return auxiliary/readout controls are only "
        "valid for mdn_expected_value_inference");
  }
  // Checkpoint paths are runtime model-state inputs. They are required by the
  // MDN launcher when the MDN wave actually executes, but must not make an
  // otherwise valid global config bundle unusable for a VICReg-only wave.
  if (is_representation_training &&
      (!spec.input_representation_checkpoint_path.empty() ||
       !spec.input_mdn_checkpoint_path.empty() ||
       spec.allow_untrained_representation)) {
    throw std::runtime_error(
        "[training_spec] representation training does not consume "
        "INPUT_REPRESENTATION_CHECKPOINT, INPUT_MDN_CHECKPOINT, or "
        "ALLOW_UNTRAINED_REPRESENTATION");
  }
  if (is_mtf_representation_training) {
    training_spec_detail::validate_mtf_jepa_mae_vicreg_training_options(
        spec.mtf_jepa_mae_vicreg);
  } else if (spec.mtf_jepa_mae_vicreg.configured) {
    throw std::runtime_error(
        "[training_spec] MTF-JEPA-MAE-VICReg options are only valid for "
        "mtf_jepa_mae_vicreg_representation");
  }
  if (is_policy_ppo_v0) {
    if (spec.max_steps <= 0 || spec.checkpoint_every <= 0) {
      throw std::runtime_error(
          "[training_spec] graph-node PPO V0 requires positive MAX_STEPS and "
          "CHECKPOINT_EVERY");
    }
    if (!std::isfinite(spec.ppo_gamma) || spec.ppo_gamma <= 0.0 ||
        spec.ppo_gamma > 1.0 || !std::isfinite(spec.ppo_gae_lambda) ||
        spec.ppo_gae_lambda < 0.0 || spec.ppo_gae_lambda > 1.0 ||
        !std::isfinite(spec.ppo_clip_epsilon) || spec.ppo_clip_epsilon <= 0.0 ||
        !std::isfinite(spec.ppo_target_kl) || spec.ppo_target_kl <= 0.0 ||
        !std::isfinite(spec.ppo_entropy_coeff) ||
        spec.ppo_entropy_coeff < 0.0 ||
        !std::isfinite(spec.ppo_value_loss_coeff) ||
        spec.ppo_value_loss_coeff < 0.0 || spec.ppo_epochs_per_rollout <= 0) {
      throw std::runtime_error(
          "[training_spec] graph-node PPO V0 requires valid PPO "
          "hyperparameters");
    }
    if (spec.training_schedule_mode !=
            training_spec_detail::expected_training_schedule_mode(spec.task) ||
        spec.require_causal_schedule !=
            training_spec_detail::expected_require_causal_schedule(spec.task)) {
      throw std::runtime_error(
          "[training_spec] graph-node PPO V0 requires causal schedule "
          "identity");
    }
    if (spec.ppo_execution_allowed !=
        training_spec_detail::expected_ppo_execution_allowed(spec.task)) {
      throw std::runtime_error("[training_spec] graph-node PPO V0 requires "
                               "PPO_EXECUTION_ALLOWED = true");
    }
    if (spec.checkpoint_kind !=
        training_spec_detail::expected_checkpoint_kind(spec.task)) {
      throw std::runtime_error(
          "[training_spec] graph-node PPO V0 checkpoint kind mismatch");
    }
    if (!spec.input_representation_checkpoint_path.empty() ||
        !spec.input_mdn_checkpoint_path.empty() ||
        spec.allow_untrained_representation || !spec.freeze_representation) {
      throw std::runtime_error(
          "[training_spec] graph-node PPO V0 consumes AllocationBelief through "
          "policy_input_t and does not own upstream model checkpoint inputs");
    }
  }
  if (spec.no_lookahead_contract_id.empty()) {
    throw std::runtime_error(
        "[training_spec] no_lookahead_contract_id is required");
  }
  if (spec.no_lookahead_contract_digest.empty()) {
    throw std::runtime_error(
        "[training_spec] no_lookahead_contract_digest is required");
  }
  if (spec.component_order_contract_id.empty()) {
    throw std::runtime_error(
        "[training_spec] component_order_contract_id is required");
  }
  if (spec.component_order_contract_digest.empty()) {
    throw std::runtime_error(
        "[training_spec] component_order_contract_digest is required");
  }
  if ((!defaults.no_lookahead_contract_id.empty() &&
       spec.no_lookahead_contract_id != defaults.no_lookahead_contract_id) ||
      (!defaults.no_lookahead_contract_digest.empty() &&
       spec.no_lookahead_contract_digest !=
           defaults.no_lookahead_contract_digest) ||
      (!defaults.component_order_contract_id.empty() &&
       spec.component_order_contract_id !=
           defaults.component_order_contract_id) ||
      (!defaults.component_order_contract_digest.empty() &&
       spec.component_order_contract_digest !=
           defaults.component_order_contract_digest)) {
    throw std::runtime_error(
        "[training_spec] no-lookahead contract reference mismatch");
  }
  if (spec.component_role !=
      training_spec_detail::expected_component_role(spec.task)) {
    throw std::runtime_error("[training_spec] component role mismatch");
  }
  if (spec.serving_order_index !=
      training_spec_detail::expected_serving_order_index(spec.task)) {
    throw std::runtime_error("[training_spec] serving order index mismatch");
  }
  if (spec.freeze_representation !=
      training_spec_detail::expected_freeze_representation(spec.task)) {
    throw std::runtime_error("[training_spec] freeze representation mismatch");
  }
  if (spec.training_visibility_policy !=
      k_training_visibility_policy_prior_generation_v1) {
    throw std::runtime_error(
        "[training_spec] training visibility policy mismatch");
  }
  if (spec.generation_lane_policy != k_generation_lane_policy_readiness_v1) {
    throw std::runtime_error("[training_spec] generation lane policy mismatch");
  }
  if (spec.valid_from_policy != k_valid_from_policy_fit_end_v1) {
    throw std::runtime_error("[training_spec] valid-from policy mismatch");
  }
  if (spec.artifact_provenance_policy !=
      k_artifact_provenance_policy_transitive_v1) {
    throw std::runtime_error(
        "[training_spec] artifact provenance policy mismatch");
  }
}

inline void validate_training_run_spec(const training_run_spec_t &spec) {
  validate_training_run_spec(spec, canonical_training_contract_defaults());
}

[[nodiscard]] inline training_run_spec_t decode_training_run_spec_from_dsl(
    const std::string &dsl_text,
    const training_contract_defaults_t &contract_defaults) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(dsl_text, "TRAINING");
  training_run_spec_t spec{};
  spec.training_id = kv::required(block, "TRAINING_ID");
  spec.task = training_spec_detail::parse_task(kv::required(block, "TASK"));
  spec.version_token =
      kv::optional(block, "VERSION",
                   training_spec_detail::expected_version_token(spec.task));
  spec.component_assembly_id = kv::optional(block, "COMPONENT_ASSEMBLY_ID", "");
  spec.no_lookahead_contract_id =
      kv::optional(block, "NO_LOOKAHEAD_CONTRACT_ID", "");
  spec.no_lookahead_contract_digest =
      kv::optional(block, "NO_LOOKAHEAD_CONTRACT_DIGEST", "");
  spec.component_order_contract_id =
      kv::optional(block, "COMPONENT_ORDER_CONTRACT_ID", "");
  spec.component_order_contract_digest =
      kv::optional(block, "COMPONENT_ORDER_CONTRACT_DIGEST", "");
  spec.component_role = kv::optional(block, "COMPONENT_ROLE", "");
  spec.serving_order_index =
      kv::parse_i64(kv::optional(block, "SERVING_ORDER_INDEX", "-1"));
  const auto optimizer_value = kv::optional(block, "OPTIMIZER", "");
  spec.optimizer = optimizer_value.empty()
                       ? training_spec_detail::expected_optimizer(spec.task)
                       : training_spec_detail::parse_optimizer(optimizer_value);
  spec.learning_rate = kv::parse_double(kv::required(block, "LEARNING_RATE"));
  spec.max_steps = kv::parse_i64(kv::optional(block, "MAX_STEPS", "0"));
  spec.batch_size = kv::parse_i64(kv::required(block, "BATCH_SIZE"));
  spec.grad_clip_norm =
      kv::parse_double(kv::optional(block, "GRAD_CLIP_NORM", "0.0"));
  const bool is_policy_ppo_v0 =
      spec.task == training_task_t::policy_graph_node_allocation_ppo_v0;
  if (is_policy_ppo_v0) {
    spec.ppo_gamma = kv::parse_double(kv::required(block, "PPO_GAMMA"));
    spec.ppo_gae_lambda =
        kv::parse_double(kv::required(block, "PPO_GAE_LAMBDA"));
    spec.ppo_clip_epsilon =
        kv::parse_double(kv::required(block, "PPO_CLIP_EPSILON"));
    spec.ppo_target_kl = kv::parse_double(kv::required(block, "PPO_TARGET_KL"));
    spec.ppo_entropy_coeff =
        kv::parse_double(kv::required(block, "PPO_ENTROPY_COEFF"));
    spec.ppo_value_loss_coeff =
        kv::parse_double(kv::required(block, "PPO_VALUE_LOSS_COEFF"));
    spec.ppo_epochs_per_rollout =
        kv::parse_i64(kv::required(block, "PPO_EPOCHS_PER_ROLLOUT"));
  }
  spec.checkpoint_every =
      kv::parse_i64(kv::optional(block, "CHECKPOINT_EVERY", "0"));
  spec.report_every = kv::parse_i64(kv::optional(block, "REPORT_EVERY", "1"));
  spec.validation_every =
      kv::parse_i64(kv::optional(block, "VALIDATION_EVERY", "0"));
  spec.seed = kv::parse_i64(kv::optional(block, "SEED", "0"));
  if (spec.task == training_task_t::mdn_expected_value_inference) {
    spec.mdn_edge_return_auxiliary_loss_weight = kv::parse_double(
        kv::required(block, "MDN_EDGE_RETURN_AUXILIARY_LOSS_WEIGHT"));
    spec.mdn_edge_return_auxiliary_direction_weight = kv::parse_double(
        kv::required(block, "MDN_EDGE_RETURN_AUXILIARY_DIRECTION_WEIGHT"));
    spec.mdn_edge_return_auxiliary_rank_weight = kv::parse_double(
        kv::required(block, "MDN_EDGE_RETURN_AUXILIARY_RANK_WEIGHT"));
    spec.mdn_edge_return_auxiliary_huber_beta = kv::parse_double(
        kv::required(block, "MDN_EDGE_RETURN_AUXILIARY_HUBER_BETA"));
    spec.mdn_edge_return_auxiliary_logit_scale = kv::parse_double(
        kv::required(block, "MDN_EDGE_RETURN_AUXILIARY_LOGIT_SCALE"));
    spec.mdn_direct_edge_return_readout_enabled = kv::parse_bool(
        kv::required(block, "MDN_DIRECT_EDGE_RETURN_READOUT_ENABLED"));
    spec.mdn_direct_edge_return_readout_loss_weight = kv::parse_double(
        kv::required(block, "MDN_DIRECT_EDGE_RETURN_READOUT_LOSS_WEIGHT"));
    spec.mdn_direct_edge_return_readout_direction_weight = kv::parse_double(
        kv::required(block, "MDN_DIRECT_EDGE_RETURN_READOUT_DIRECTION_WEIGHT"));
    spec.mdn_direct_edge_return_readout_rank_weight = kv::parse_double(
        kv::required(block, "MDN_DIRECT_EDGE_RETURN_READOUT_RANK_WEIGHT"));
    spec.mdn_direct_edge_return_readout_huber_beta = kv::parse_double(
        kv::required(block, "MDN_DIRECT_EDGE_RETURN_READOUT_HUBER_BETA"));
    spec.mdn_direct_edge_return_readout_logit_scale = kv::parse_double(
        kv::required(block, "MDN_DIRECT_EDGE_RETURN_READOUT_LOGIT_SCALE"));
  }
  spec.freeze_representation = kv::parse_bool(kv::optional(
      block, "FREEZE_REPRESENTATION",
      training_spec_detail::expected_freeze_representation(spec.task)
          ? "true"
          : "false"));
  spec.input_representation_checkpoint_path =
      kv::optional(block, "INPUT_REPRESENTATION_CHECKPOINT", "");
  spec.input_mdn_checkpoint_path =
      kv::optional(block, "INPUT_MDN_CHECKPOINT", "");
  spec.allow_untrained_representation = kv::parse_bool(
      kv::optional(block, "ALLOW_UNTRAINED_REPRESENTATION", "false"));
  spec.training_schedule_mode = kv::optional(
      block, "TRAINING_SCHEDULE_MODE",
      training_spec_detail::expected_training_schedule_mode(spec.task));
  spec.require_causal_schedule = kv::parse_bool(kv::optional(
      block, "REQUIRE_CAUSAL_SCHEDULE",
      training_spec_detail::expected_require_causal_schedule(spec.task)
          ? "true"
          : "false"));
  spec.ppo_execution_allowed = kv::parse_bool(kv::optional(
      block, "PPO_EXECUTION_ALLOWED",
      training_spec_detail::expected_ppo_execution_allowed(spec.task)
          ? "true"
          : "false"));
  spec.checkpoint_kind =
      kv::optional(block, "CHECKPOINT_KIND",
                   training_spec_detail::expected_checkpoint_kind(spec.task));
  spec.training_visibility_policy =
      kv::optional(block, "TRAINING_VISIBILITY_POLICY", "");
  spec.generation_lane_policy =
      kv::optional(block, "GENERATION_LANE_POLICY", "");
  spec.valid_from_policy = kv::optional(block, "VALID_FROM_POLICY", "");
  spec.artifact_provenance_policy =
      kv::optional(block, "ARTIFACT_PROVENANCE_POLICY", "");
  if (spec.task == training_task_t::mtf_jepa_mae_vicreg_representation) {
    auto &mtf = spec.mtf_jepa_mae_vicreg;
    mtf.configured = true;
    mtf.augmentation_profile = kv::required(block, "AUGMENTATION_PROFILE");
    mtf.dropout = kv::parse_double(kv::required(block, "DROPOUT"));
    mtf.mask_ratio_time =
        kv::parse_double(kv::required(block, "MASK_RATIO_TIME"));
    mtf.mask_ratio_frequency =
        kv::parse_double(kv::required(block, "MASK_RATIO_FREQUENCY"));
    mtf.mask_ratio_channel =
        kv::parse_double(kv::required(block, "MASK_RATIO_CHANNEL"));
    mtf.min_context_ratio =
        kv::parse_double(kv::required(block, "MIN_CONTEXT_RATIO"));
    mtf.lambda_jepa = kv::parse_double(kv::required(block, "LAMBDA_JEPA"));
    mtf.lambda_mae = kv::parse_double(kv::required(block, "LAMBDA_MAE"));
    mtf.lambda_tf_align =
        kv::parse_double(kv::required(block, "LAMBDA_TF_ALIGN"));
    mtf.lambda_vicreg = kv::parse_double(kv::required(block, "LAMBDA_VICREG"));
    mtf.lambda_global_vicreg =
        kv::parse_double(kv::required(block, "LAMBDA_GLOBAL_VICREG"));
    mtf.lambda_channel_vicreg =
        kv::parse_double(kv::required(block, "LAMBDA_CHANNEL_VICREG"));
    mtf.vicreg_sim_weight =
        kv::parse_double(kv::required(block, "VICREG_SIM_WEIGHT"));
    mtf.vicreg_var_weight =
        kv::parse_double(kv::required(block, "VICREG_VAR_WEIGHT"));
    mtf.vicreg_cov_weight =
        kv::parse_double(kv::required(block, "VICREG_COV_WEIGHT"));
    mtf.vicreg_variance_floor =
        kv::parse_double(kv::required(block, "VICREG_VARIANCE_FLOOR"));
    mtf.vicreg_variance_epsilon =
        kv::parse_double(kv::required(block, "VICREG_VARIANCE_EPSILON"));
    mtf.target_ema_tau =
        kv::parse_double(kv::required(block, "TARGET_EMA_TAU"));
    mtf.use_target_ema = kv::parse_bool(kv::required(block, "USE_TARGET_EMA"));
    mtf.stop_gradient_target =
        kv::parse_bool(kv::required(block, "STOP_GRADIENT_TARGET"));
    mtf.return_diagnostics =
        kv::parse_bool(kv::required(block, "RETURN_DIAGNOSTICS"));
    mtf.use_mae_decoder =
        kv::parse_bool(kv::required(block, "USE_MAE_DECODER"));
    mtf.use_jepa_loss = kv::parse_bool(kv::required(block, "USE_JEPA_LOSS"));
    mtf.use_tf_align_loss =
        kv::parse_bool(kv::required(block, "USE_TF_ALIGN_LOSS"));
    mtf.use_vicreg_loss =
        kv::parse_bool(kv::required(block, "USE_VICREG_LOSS"));
    mtf.use_global_vicreg =
        kv::parse_bool(kv::required(block, "USE_GLOBAL_VICREG"));
    mtf.use_channel_vicreg =
        kv::parse_bool(kv::required(block, "USE_CHANNEL_VICREG"));
    mtf.use_raw_reconstruction_targets =
        kv::parse_bool(kv::required(block, "USE_RAW_RECONSTRUCTION_TARGETS"));
    mtf.strict_finite_loss =
        kv::parse_bool(kv::required(block, "STRICT_FINITE_LOSS"));
    mtf.couple_time_frequency_masks =
        kv::parse_bool(kv::required(block, "COUPLE_TIME_FREQUENCY_MASKS"));
    mtf.mask_same_window_across_domains =
        kv::parse_bool(kv::required(block, "MASK_SAME_WINDOW_ACROSS_DOMAINS"));
    mtf.mask_same_channel_block =
        kv::parse_bool(kv::required(block, "MASK_SAME_CHANNEL_BLOCK"));
    mtf.max_context_target_time_overlap = kv::parse_double(
        kv::required(block, "MAX_CONTEXT_TARGET_TIME_OVERLAP"));
    mtf.gaussian_jitter_std =
        kv::parse_double(kv::required(block, "GAUSSIAN_JITTER_STD"));
    mtf.feature_dropout_prob =
        kv::parse_double(kv::required(block, "FEATURE_DROPOUT_PROB"));
    mtf.history_dropout_prob =
        kv::parse_double(kv::required(block, "HISTORY_DROPOUT_PROB"));
    mtf.time_crop_jitter_max =
        kv::parse_i64(kv::required(block, "TIME_CROP_JITTER_MAX"));
    mtf.time_dilation_min =
        kv::parse_double(kv::required(block, "TIME_DILATION_MIN"));
    mtf.time_dilation_max =
        kv::parse_double(kv::required(block, "TIME_DILATION_MAX"));
    mtf.time_warp_max = kv::parse_double(kv::required(block, "TIME_WARP_MAX"));
    mtf.amplitude_scale_min =
        kv::parse_double(kv::required(block, "AMPLITUDE_SCALE_MIN"));
    mtf.amplitude_scale_max =
        kv::parse_double(kv::required(block, "AMPLITUDE_SCALE_MAX"));
    mtf.amplitude_shift_std =
        kv::parse_double(kv::required(block, "AMPLITUDE_SHIFT_STD"));
    mtf.frequency_mask_ratio =
        kv::parse_double(kv::required(block, "FREQUENCY_MASK_RATIO"));
    mtf.frequency_jitter_std =
        kv::parse_double(kv::required(block, "FREQUENCY_JITTER_STD"));
    mtf.phase_jitter_max =
        kv::parse_double(kv::required(block, "PHASE_JITTER_MAX"));
    mtf.channel_dropout_prob =
        kv::parse_double(kv::required(block, "CHANNEL_DROPOUT_PROB"));
    mtf.cross_channel_dropout_prob =
        kv::parse_double(kv::required(block, "CROSS_CHANNEL_DROPOUT_PROB"));
    mtf.node_dropout_prob =
        kv::parse_double(kv::required(block, "NODE_DROPOUT_PROB"));
    mtf.edge_dropout_prob =
        kv::parse_double(kv::required(block, "EDGE_DROPOUT_PROB"));
    mtf.magnitude_normalization_noise_std = kv::parse_double(
        kv::required(block, "MAGNITUDE_NORMALIZATION_NOISE_STD"));
  }
  apply_training_run_spec_canonical_defaults(spec, contract_defaults);
  validate_training_run_spec(spec, contract_defaults);
  return spec;
}

[[nodiscard]] inline training_run_spec_t
decode_training_run_spec_from_dsl(const std::string &dsl_text) {
  return decode_training_run_spec_from_dsl(
      dsl_text, canonical_training_contract_defaults());
}

} // namespace cuwacunu::jkimyei::training
