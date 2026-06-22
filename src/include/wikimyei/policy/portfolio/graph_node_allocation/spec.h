// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::wikimyei::policy::portfolio::graph_node_allocation {

inline constexpr std::int64_t k_graph_node_allocation_node_feature_dim_v1 = 28;
inline constexpr std::int64_t k_graph_node_allocation_global_feature_dim_v1 = 6;
inline constexpr std::int64_t k_graph_node_allocation_risk_feature_dim_v1 = 10;

struct graph_node_allocation_spec_t {
  std::string version_token{
      "wikimyei.policy.portfolio.graph_node_allocation.v1"};
  std::string component_assembly_id{"graph_node_allocation_v1"};
  std::string policy_kind{"trainable_graph_node_allocation"};
  std::string policy_input_schema{"kikijyeba.environment.policy_input.v1"};
  std::string action_adapter{"target_node_weights_simplex.v1"};
  std::string action_distribution{"masked_dirichlet_simplex.v1"};
  std::string action_schema{
      "kikijyeba.environment.action.target_node_weights.v1"};
  std::string reward_contract{"kikijyeba.environment.reward.post_execution_"
                              "ledger_log_growth_cost_drawdown.v1"};
  std::string graph_node_universe_policy{"fixed_ordered_target_node_universe"};
  std::string scenario_input_policy{
      "allocation_belief_distributional_summaries_v1"};
  bool raw_mdn_input_allowed{false};
  bool ppo_implemented{true};
  bool live_capital_allowed{false};
};

struct graph_node_allocation_net_spec_t {
  std::string version_token{
      "wikimyei.policy.portfolio.graph_node_allocation.net.v1"};
  std::string policy_input_schema{"kikijyeba.environment.policy_input.v1"};
  std::string policy_input_feature_manifest{
      "wikimyei.policy.portfolio.graph_node_allocation.features.v1"};
  std::int64_t input_node_feature_dim{
      k_graph_node_allocation_node_feature_dim_v1};
  std::int64_t input_global_feature_dim{
      k_graph_node_allocation_global_feature_dim_v1};
  std::int64_t input_risk_feature_dim{
      k_graph_node_allocation_risk_feature_dim_v1};
  std::int64_t node_encoder_layers{2};
  std::int64_t node_encoder_hidden_dim{128};
  std::int64_t global_encoder_layers{2};
  std::int64_t global_encoder_hidden_dim{64};
  std::int64_t risk_encoder_layers{1};
  std::int64_t risk_encoder_hidden_dim{64};
  std::string pooling{"masked_mean_max"};
  std::int64_t fusion_hidden_dim{128};
  std::int64_t fusion_layers{2};
  std::string activation{"silu"};
  std::string normalization{"layer_norm"};
  double dropout{0.0};
  bool share_encoder{true};
  bool separate_actor_critic_heads{true};
  std::int64_t policy_head_hidden_dim{128};
  std::int64_t value_head_hidden_dim{128};
  std::string output_head{"node_weight_logits"};
  std::string value_head{"state_value"};
  std::string action_adapter{"target_node_weights_simplex.v1"};
  std::string action_distribution{"masked_dirichlet_simplex.v1"};
  std::string dirichlet_concentration_head{"scalar_total_concentration"};
  double dirichlet_alpha_floor{1.0e-4};
  double dirichlet_total_concentration_min{0.25};
  double dirichlet_total_concentration_max{256.0};
  std::string logistic_normal_candidate{"masked_logistic_normal_simplex.v1"};
  std::string logistic_normal_std_head{"scalar_log_std"};
  double logistic_normal_log_std_min{-5.0};
  double logistic_normal_log_std_max{1.0};
  std::string logistic_normal_coordinate_policy{"active_additive_log_ratio"};
  std::string logistic_normal_entropy_kind{"latent_normal_entropy"};
  bool ppo_execution_allowed{true};
};

struct graph_node_allocation_feature_manifest_spec_t {
  std::string version_token{
      "wikimyei.policy.portfolio.graph_node_allocation.features.v1"};
  std::string policy_input_schema{"kikijyeba.environment.policy_input.v1"};
  std::int64_t node_feature_dim{k_graph_node_allocation_node_feature_dim_v1};
  std::vector<std::string> node_feature_names{};
  std::vector<std::string> node_feature_transforms{};
  std::string node_feature_default_policy{
      "neutral_allocation_belief_fallbacks.v1"};
  std::string node_feature_clipping_policy{
      "producer_validated_no_actor_clip.v1"};
  std::int64_t global_feature_dim{
      k_graph_node_allocation_global_feature_dim_v1};
  std::vector<std::string> global_feature_names{};
  std::vector<std::string> global_feature_transforms{};
  std::string global_feature_default_policy{"portfolio_state_required.v1"};
  std::string global_feature_clipping_policy{
      "producer_validated_no_actor_clip.v1"};
  std::string risk_feature_block{"compact_cross_node_risk_block.v1"};
  std::int64_t risk_feature_dim{k_graph_node_allocation_risk_feature_dim_v1};
  std::vector<std::string> risk_feature_names{};
  std::vector<std::string> risk_feature_transforms{};
  std::string risk_feature_default_policy{"zero_if_no_allocation_belief.v1"};
  std::string risk_feature_clipping_policy{
      "producer_validated_no_actor_clip.v1"};
  std::vector<std::string> evidence_only_fields{};
  bool raw_mdn_input_allowed{false};
  bool full_scenario_bank_input_allowed{false};
};

[[nodiscard]] inline std::vector<std::string> expected_node_feature_names_v1() {
  return {"current_weight",
          "previous_target_weight",
          "weight_delta_from_previous_target",
          "log_executable_mid",
          "valid_mask",
          "tradable_mask",
          "executable_mask",
          "expected_log_return",
          "expected_arithmetic_return",
          "marginal_variance",
          "marginal_volatility",
          "var_down",
          "cvar_down",
          "adverse_excursion_probability",
          "volatility",
          "confidence",
          "liquidity_score",
          "linear_cost",
          "quadratic_impact",
          "capacity_weight_limit",
          "projection_validation_score",
          "residual_quality_score",
          "surprise",
          "calibration_score",
          "mixture_entropy",
          "component_disagreement",
          "channel_disagreement",
          "is_accounting_numeraire_node"};
}

[[nodiscard]] inline std::vector<std::string>
expected_global_feature_names_v1() {
  return {"log_equity_value_numeraire",
          "drawdown",
          "current_weight_sum",
          "previous_target_weight_sum",
          "current_accounting_numeraire_weight",
          "previous_accounting_numeraire_weight"};
}

[[nodiscard]] inline std::vector<std::string> expected_risk_feature_names_v1() {
  return {"mean_pairwise_correlation",
          "max_pairwise_correlation",
          "top1_correlation_eigenvalue",
          "top2_correlation_eigenvalue",
          "top3_correlation_eigenvalue",
          "top1_covariance_eigenvalue",
          "top2_covariance_eigenvalue",
          "top3_covariance_eigenvalue",
          "current_portfolio_volatility_estimate",
          "current_portfolio_cvar_estimate"};
}

inline void require_exact_list(const std::vector<std::string> &actual,
                               const std::vector<std::string> &expected,
                               const char *field_name) {
  if (actual != expected) {
    throw std::runtime_error(std::string("[graph_node_allocation_spec] ") +
                             field_name + " mismatch");
  }
}

inline void require_same_size(const std::vector<std::string> &left,
                              const std::vector<std::string> &right,
                              const char *field_name) {
  if (left.size() != right.size()) {
    throw std::runtime_error(std::string("[graph_node_allocation_spec] ") +
                             field_name + " count mismatch");
  }
}

inline void
reject_retired_key(const cuwacunu::piaabo::parse::simple_kv::block_t &block,
                   const char *key, const char *context) {
  if (block.values.find(key) != block.values.end()) {
    throw std::runtime_error(std::string(context) + " " + key +
                             " is retired; derive dimensions from feature "
                             "names instead");
  }
}

[[nodiscard]] inline bool
contains_string(const std::vector<std::string> &values,
                const std::string &needle) {
  return std::find(values.begin(), values.end(), needle) != values.end();
}

[[nodiscard]] inline bool
supported_action_distribution(const std::string &distribution) {
  return distribution == "masked_dirichlet_simplex.v1" ||
         distribution == "masked_logistic_normal_simplex.v1";
}

inline void
validate_graph_node_allocation_spec(const graph_node_allocation_spec_t &spec) {
  if (spec.version_token !=
      "wikimyei.policy.portfolio.graph_node_allocation.v1") {
    throw std::runtime_error("[graph_node_allocation_spec] unsupported "
                             "VERSION");
  }
  if (spec.component_assembly_id.empty()) {
    throw std::runtime_error("[graph_node_allocation_spec] "
                             "COMPONENT_ASSEMBLY_ID is required");
  }
  if (spec.policy_kind != "trainable_graph_node_allocation") {
    throw std::runtime_error(
        "[graph_node_allocation_spec] unsupported POLICY_KIND");
  }
  if (spec.policy_input_schema != "kikijyeba.environment.policy_input.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_spec] unsupported POLICY_INPUT_SCHEMA");
  }
  if (spec.action_adapter != "target_node_weights_simplex.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_spec] unsupported ACTION_ADAPTER");
  }
  if (!supported_action_distribution(spec.action_distribution)) {
    throw std::runtime_error(
        "[graph_node_allocation_spec] unsupported ACTION_DISTRIBUTION");
  }
  if (spec.action_schema !=
      "kikijyeba.environment.action.target_node_weights.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_spec] unsupported ACTION_SCHEMA");
  }
  if (spec.reward_contract != "kikijyeba.environment.reward.post_execution_"
                              "ledger_log_growth_cost_drawdown.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_spec] unsupported REWARD_CONTRACT");
  }
  if (spec.graph_node_universe_policy != "fixed_ordered_target_node_universe") {
    throw std::runtime_error(
        "[graph_node_allocation_spec] unsupported graph universe policy");
  }
  if (spec.scenario_input_policy !=
      "allocation_belief_distributional_summaries_v1") {
    throw std::runtime_error(
        "[graph_node_allocation_spec] V1 does not expose raw scenario "
        "banks or raw MDN tensors by default");
  }
  if (spec.raw_mdn_input_allowed || !spec.ppo_implemented ||
      spec.live_capital_allowed) {
    throw std::runtime_error(
        "[graph_node_allocation_spec] V1 requires PPO V0 enabled while raw "
        "MDN and live capital remain disabled");
  }
}

inline void validate_graph_node_allocation_net_spec(
    const graph_node_allocation_net_spec_t &spec) {
  if (spec.version_token !=
      "wikimyei.policy.portfolio.graph_node_allocation.net.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] unsupported VERSION");
  }
  if (spec.policy_input_schema != "kikijyeba.environment.policy_input.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] unsupported POLICY_INPUT_SCHEMA");
  }
  if (spec.policy_input_feature_manifest !=
      "wikimyei.policy.portfolio.graph_node_allocation.features.v1") {
    throw std::runtime_error("[graph_node_allocation_net_spec] unsupported "
                             "POLICY_INPUT_FEATURE_MANIFEST");
  }
  if (spec.input_node_feature_dim !=
          k_graph_node_allocation_node_feature_dim_v1 ||
      spec.input_global_feature_dim !=
          k_graph_node_allocation_global_feature_dim_v1 ||
      spec.input_risk_feature_dim !=
          k_graph_node_allocation_risk_feature_dim_v1) {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] input feature dimensions mismatch "
        "V1 manifest");
  }
  if (spec.node_encoder_layers <= 0 || spec.node_encoder_hidden_dim <= 0 ||
      spec.global_encoder_layers <= 0 || spec.global_encoder_hidden_dim <= 0 ||
      spec.risk_encoder_layers <= 0 || spec.risk_encoder_hidden_dim <= 0 ||
      spec.fusion_layers <= 0 || spec.fusion_hidden_dim <= 0 ||
      spec.policy_head_hidden_dim <= 0 || spec.value_head_hidden_dim <= 0) {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] layer counts and dimensions must be "
        "positive");
  }
  if (spec.pooling != "masked_mean_max") {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] POOLING must be masked_mean_max");
  }
  if (spec.activation != "silu") {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] ACTIVATION must be silu");
  }
  if (spec.normalization != "layer_norm") {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] NORMALIZATION must be layer_norm");
  }
  if (!std::isfinite(spec.dropout) || spec.dropout < 0.0 ||
      spec.dropout >= 1.0) {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] DROPOUT must be finite in [0,1)");
  }
  if (!spec.share_encoder || !spec.separate_actor_critic_heads) {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] V1 requires shared encoders with "
        "separate actor/critic heads");
  }
  if (spec.output_head != "node_weight_logits") {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] OUTPUT_HEAD must be "
        "node_weight_logits");
  }
  if (spec.value_head != "state_value") {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] VALUE_HEAD must be state_value");
  }
  if (spec.action_adapter != "target_node_weights_simplex.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] ACTION_ADAPTER mismatch");
  }
  if (!supported_action_distribution(spec.action_distribution)) {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] ACTION_DISTRIBUTION mismatch");
  }
  if (spec.action_distribution != "masked_dirichlet_simplex.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] V1 default must be "
        "masked_dirichlet_simplex.v1");
  }
  if (spec.dirichlet_concentration_head != "scalar_total_concentration" ||
      !std::isfinite(spec.dirichlet_alpha_floor) ||
      !std::isfinite(spec.dirichlet_total_concentration_min) ||
      !std::isfinite(spec.dirichlet_total_concentration_max) ||
      spec.dirichlet_alpha_floor <= 0.0 ||
      spec.dirichlet_total_concentration_min <= 0.0 ||
      spec.dirichlet_total_concentration_max <=
          spec.dirichlet_total_concentration_min) {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] invalid Dirichlet distribution "
        "configuration");
  }
  if (spec.logistic_normal_candidate != "masked_logistic_normal_simplex.v1" ||
      spec.logistic_normal_std_head != "scalar_log_std" ||
      spec.logistic_normal_coordinate_policy != "active_additive_log_ratio" ||
      spec.logistic_normal_entropy_kind != "latent_normal_entropy" ||
      !std::isfinite(spec.logistic_normal_log_std_min) ||
      !std::isfinite(spec.logistic_normal_log_std_max) ||
      spec.logistic_normal_log_std_max <= spec.logistic_normal_log_std_min) {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] invalid logistic-normal candidate "
        "configuration");
  }
  if (!spec.ppo_execution_allowed) {
    throw std::runtime_error("[graph_node_allocation_net_spec] PPO V0 "
                             "execution must be allowed for this policy");
  }
}

inline void validate_graph_node_allocation_feature_manifest_spec(
    const graph_node_allocation_feature_manifest_spec_t &spec) {
  if (spec.version_token !=
      "wikimyei.policy.portfolio.graph_node_allocation.features.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_feature_manifest] unsupported VERSION");
  }
  if (spec.policy_input_schema != "kikijyeba.environment.policy_input.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_feature_manifest] unsupported "
        "POLICY_INPUT_SCHEMA");
  }
  if (spec.node_feature_dim != k_graph_node_allocation_node_feature_dim_v1 ||
      spec.global_feature_dim !=
          k_graph_node_allocation_global_feature_dim_v1 ||
      spec.risk_feature_dim != k_graph_node_allocation_risk_feature_dim_v1) {
    throw std::runtime_error(
        "[graph_node_allocation_feature_manifest] feature dimensions mismatch "
        "V1 contract");
  }
  require_exact_list(spec.node_feature_names, expected_node_feature_names_v1(),
                     "NODE_FEATURE_NAMES");
  require_exact_list(spec.global_feature_names,
                     expected_global_feature_names_v1(),
                     "GLOBAL_FEATURE_NAMES");
  require_exact_list(spec.risk_feature_names, expected_risk_feature_names_v1(),
                     "RISK_FEATURE_NAMES");
  require_same_size(spec.node_feature_names, spec.node_feature_transforms,
                    "NODE_FEATURE_TRANSFORMS");
  require_same_size(spec.global_feature_names, spec.global_feature_transforms,
                    "GLOBAL_FEATURE_TRANSFORMS");
  require_same_size(spec.risk_feature_names, spec.risk_feature_transforms,
                    "RISK_FEATURE_TRANSFORMS");
  if (spec.node_feature_default_policy !=
          "neutral_allocation_belief_fallbacks.v1" ||
      spec.global_feature_default_policy != "portfolio_state_required.v1" ||
      spec.risk_feature_default_policy != "zero_if_no_allocation_belief.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_feature_manifest] unsupported default "
        "policy");
  }
  if (spec.node_feature_clipping_policy !=
          "producer_validated_no_actor_clip.v1" ||
      spec.global_feature_clipping_policy !=
          "producer_validated_no_actor_clip.v1" ||
      spec.risk_feature_clipping_policy !=
          "producer_validated_no_actor_clip.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_feature_manifest] unsupported clipping "
        "policy");
  }
  if (spec.risk_feature_block != "compact_cross_node_risk_block.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_feature_manifest] unsupported "
        "RISK_FEATURE_BLOCK");
  }
  if (!contains_string(spec.evidence_only_fields, "observation_anchor_index") ||
      !contains_string(spec.evidence_only_fields, "knowledge_timestamp_ms") ||
      !contains_string(spec.evidence_only_fields, "policy_input_digest")) {
    throw std::runtime_error(
        "[graph_node_allocation_feature_manifest] evidence-only identity "
        "fields are incomplete");
  }
  if (spec.raw_mdn_input_allowed || spec.full_scenario_bank_input_allowed) {
    throw std::runtime_error(
        "[graph_node_allocation_feature_manifest] raw MDN and full scenario "
        "bank inputs are forbidden for V1");
  }
}

[[nodiscard]] inline graph_node_allocation_spec_t
decode_graph_node_allocation_spec_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(dsl_text, "GRAPH_NODE_ALLOCATION");
  graph_node_allocation_spec_t spec{};
  spec.version_token = kv::required(block, "VERSION");
  spec.component_assembly_id = kv::required(block, "COMPONENT_ASSEMBLY_ID");
  spec.policy_kind = kv::required(block, "POLICY_KIND");
  spec.policy_input_schema = kv::required(block, "POLICY_INPUT_SCHEMA");
  spec.action_adapter = kv::required(block, "ACTION_ADAPTER");
  spec.action_distribution = kv::required(block, "ACTION_DISTRIBUTION");
  spec.action_schema = kv::required(block, "ACTION_SCHEMA");
  spec.reward_contract = kv::required(block, "REWARD_CONTRACT");
  spec.graph_node_universe_policy =
      kv::required(block, "GRAPH_NODE_UNIVERSE_POLICY");
  spec.scenario_input_policy = kv::required(block, "SCENARIO_INPUT_POLICY");
  spec.raw_mdn_input_allowed =
      kv::parse_bool(kv::required(block, "RAW_MDN_INPUT_ALLOWED"));
  spec.ppo_implemented = kv::parse_bool(kv::required(block, "PPO_IMPLEMENTED"));
  spec.live_capital_allowed =
      kv::parse_bool(kv::required(block, "LIVE_CAPITAL_ALLOWED"));
  validate_graph_node_allocation_spec(spec);
  return spec;
}

[[nodiscard]] inline graph_node_allocation_net_spec_t
decode_graph_node_allocation_net_spec_from_dsl(const std::string &net_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(net_text, "GRAPH_NODE_ALLOCATION_NET");
  graph_node_allocation_net_spec_t spec{};
  spec.version_token = kv::required(block, "VERSION");
  spec.policy_input_schema = kv::required(block, "POLICY_INPUT_SCHEMA");
  spec.policy_input_feature_manifest =
      kv::required(block, "POLICY_INPUT_FEATURE_MANIFEST");
  reject_retired_key(block, "INPUT_NODE_FEATURE_DIM",
                     "[graph_node_allocation_net_spec]");
  reject_retired_key(block, "INPUT_GLOBAL_FEATURE_DIM",
                     "[graph_node_allocation_net_spec]");
  reject_retired_key(block, "INPUT_RISK_FEATURE_DIM",
                     "[graph_node_allocation_net_spec]");
  spec.node_encoder_layers =
      kv::parse_i64(kv::required(block, "NODE_ENCODER_LAYERS"));
  spec.node_encoder_hidden_dim =
      kv::parse_i64(kv::required(block, "NODE_ENCODER_HIDDEN_DIM"));
  spec.global_encoder_layers =
      kv::parse_i64(kv::required(block, "GLOBAL_ENCODER_LAYERS"));
  spec.global_encoder_hidden_dim =
      kv::parse_i64(kv::required(block, "GLOBAL_ENCODER_HIDDEN_DIM"));
  spec.risk_encoder_layers =
      kv::parse_i64(kv::required(block, "RISK_ENCODER_LAYERS"));
  spec.risk_encoder_hidden_dim =
      kv::parse_i64(kv::required(block, "RISK_ENCODER_HIDDEN_DIM"));
  spec.pooling = kv::required(block, "POOLING");
  spec.fusion_hidden_dim =
      kv::parse_i64(kv::required(block, "FUSION_HIDDEN_DIM"));
  spec.fusion_layers = kv::parse_i64(kv::required(block, "FUSION_LAYERS"));
  spec.activation = kv::required(block, "ACTIVATION");
  spec.normalization = kv::required(block, "NORMALIZATION");
  spec.dropout = kv::parse_double(kv::required(block, "DROPOUT"));
  spec.share_encoder = kv::parse_bool(kv::required(block, "SHARE_ENCODER"));
  spec.separate_actor_critic_heads =
      kv::parse_bool(kv::required(block, "SEPARATE_ACTOR_CRITIC_HEADS"));
  spec.policy_head_hidden_dim =
      kv::parse_i64(kv::required(block, "POLICY_HEAD_HIDDEN_DIM"));
  spec.value_head_hidden_dim =
      kv::parse_i64(kv::required(block, "VALUE_HEAD_HIDDEN_DIM"));
  spec.output_head = kv::required(block, "OUTPUT_HEAD");
  spec.value_head = kv::required(block, "VALUE_HEAD");
  spec.action_adapter =
      kv::optional(block, "ACTION_ADAPTER", spec.action_adapter);
  spec.action_distribution =
      kv::optional(block, "ACTION_DISTRIBUTION", spec.action_distribution);
  spec.dirichlet_concentration_head =
      kv::required(block, "DIRICHLET_CONCENTRATION_HEAD");
  spec.dirichlet_alpha_floor =
      kv::parse_double(kv::required(block, "DIRICHLET_ALPHA_FLOOR"));
  spec.dirichlet_total_concentration_min = kv::parse_double(
      kv::required(block, "DIRICHLET_TOTAL_CONCENTRATION_MIN"));
  spec.dirichlet_total_concentration_max = kv::parse_double(
      kv::required(block, "DIRICHLET_TOTAL_CONCENTRATION_MAX"));
  spec.logistic_normal_candidate =
      kv::required(block, "LOGISTIC_NORMAL_CANDIDATE");
  spec.logistic_normal_std_head =
      kv::required(block, "LOGISTIC_NORMAL_STD_HEAD");
  spec.logistic_normal_log_std_min =
      kv::parse_double(kv::required(block, "LOGISTIC_NORMAL_LOG_STD_MIN"));
  spec.logistic_normal_log_std_max =
      kv::parse_double(kv::required(block, "LOGISTIC_NORMAL_LOG_STD_MAX"));
  spec.logistic_normal_coordinate_policy =
      kv::required(block, "LOGISTIC_NORMAL_COORDINATE_POLICY");
  spec.logistic_normal_entropy_kind =
      kv::required(block, "LOGISTIC_NORMAL_ENTROPY_KIND");
  spec.ppo_execution_allowed = kv::parse_bool(
      kv::optional(block, "PPO_EXECUTION_ALLOWED",
                   spec.ppo_execution_allowed ? "true" : "false"));
  validate_graph_node_allocation_net_spec(spec);
  return spec;
}

[[nodiscard]] inline graph_node_allocation_feature_manifest_spec_t
decode_graph_node_allocation_feature_manifest_from_dsl(
    const std::string &features_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block =
      kv::single_block(features_text, "GRAPH_NODE_ALLOCATION_FEATURES");
  graph_node_allocation_feature_manifest_spec_t spec{};
  spec.version_token = kv::required(block, "VERSION");
  spec.policy_input_schema = kv::required(block, "POLICY_INPUT_SCHEMA");
  reject_retired_key(block, "NODE_FEATURE_DIM",
                     "[graph_node_allocation_feature_manifest_spec]");
  reject_retired_key(block, "GLOBAL_FEATURE_DIM",
                     "[graph_node_allocation_feature_manifest_spec]");
  reject_retired_key(block, "RISK_FEATURE_DIM",
                     "[graph_node_allocation_feature_manifest_spec]");
  spec.node_feature_names =
      kv::parse_list(kv::required(block, "NODE_FEATURE_NAMES"));
  spec.node_feature_dim =
      static_cast<std::int64_t>(spec.node_feature_names.size());
  spec.node_feature_transforms =
      kv::parse_list(kv::required(block, "NODE_FEATURE_TRANSFORMS"));
  spec.node_feature_default_policy =
      kv::required(block, "NODE_FEATURE_DEFAULT_POLICY");
  spec.node_feature_clipping_policy =
      kv::required(block, "NODE_FEATURE_CLIPPING_POLICY");
  spec.global_feature_names =
      kv::parse_list(kv::required(block, "GLOBAL_FEATURE_NAMES"));
  spec.global_feature_dim =
      static_cast<std::int64_t>(spec.global_feature_names.size());
  spec.global_feature_transforms =
      kv::parse_list(kv::required(block, "GLOBAL_FEATURE_TRANSFORMS"));
  spec.global_feature_default_policy =
      kv::required(block, "GLOBAL_FEATURE_DEFAULT_POLICY");
  spec.global_feature_clipping_policy =
      kv::required(block, "GLOBAL_FEATURE_CLIPPING_POLICY");
  spec.risk_feature_block = kv::required(block, "RISK_FEATURE_BLOCK");
  spec.risk_feature_names =
      kv::parse_list(kv::required(block, "RISK_FEATURE_NAMES"));
  spec.risk_feature_dim =
      static_cast<std::int64_t>(spec.risk_feature_names.size());
  spec.risk_feature_transforms =
      kv::parse_list(kv::required(block, "RISK_FEATURE_TRANSFORMS"));
  spec.risk_feature_default_policy =
      kv::required(block, "RISK_FEATURE_DEFAULT_POLICY");
  spec.risk_feature_clipping_policy =
      kv::required(block, "RISK_FEATURE_CLIPPING_POLICY");
  spec.evidence_only_fields =
      kv::parse_list(kv::required(block, "EVIDENCE_ONLY_FIELDS"));
  spec.raw_mdn_input_allowed =
      kv::parse_bool(kv::required(block, "RAW_MDN_INPUT_ALLOWED"));
  spec.full_scenario_bank_input_allowed =
      kv::parse_bool(kv::required(block, "FULL_SCENARIO_BANK_INPUT_ALLOWED"));
  validate_graph_node_allocation_feature_manifest_spec(spec);
  return spec;
}

} // namespace cuwacunu::wikimyei::policy::portfolio::graph_node_allocation
