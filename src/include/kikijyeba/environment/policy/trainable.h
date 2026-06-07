// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/environment/control/interfaces.h"

namespace cuwacunu::kikijyeba::environment {

inline constexpr const char *kPolicyInputSchemaV1 =
    "kikijyeba.environment.policy_input.v1";
inline constexpr const char *kTargetNodeWeightsSimplexAdapterV1 =
    "target_node_weights_simplex.v1";
inline constexpr const char
    *kPostExecutionLedgerLogGrowthCostDrawdownRewardContractV1 =
        "kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_"
        "drawdown.v1";
inline constexpr const char *kGraphNodeAllocationPolicyId =
    "wikimyei.policy.portfolio.graph_node_allocation.v1";
inline constexpr std::int64_t kPolicyInputNodeFeatureDimV1 = 27;
inline constexpr std::int64_t kPolicyInputGlobalFeatureDimV1 = 8;

struct policy_input_t {
  std::string schema_id{kPolicyInputSchemaV1};
  std::string environment_assembly_id{kReplayEnvironmentAssemblyId};
  std::string observation_anchor_key{};
  std::int64_t observation_anchor_index{-1};
  portfolio::timestamp_ms_t knowledge_timestamp_ms{0};
  std::string graph_order_fingerprint{};
  std::string allocation_belief_digest{};
  std::string execution_profile_digest{};
  std::string reward_contract_id{
      kPostExecutionLedgerLogGrowthCostDrawdownRewardContractV1};
  std::string accounting_numeraire_node_id{};
  std::string causal_schedule_digest{};
  std::string snapshot_family_digest{};

  std::vector<std::string> node_ids{};
  torch::Tensor valid_mask{};              // [A], bool
  torch::Tensor tradable_mask{};           // [A], bool
  torch::Tensor executable_mask{};         // [A], bool
  torch::Tensor current_weights{};         // [A], float64
  torch::Tensor previous_target_weights{}; // [A], float64
  torch::Tensor node_features{};           // [A,F]
  torch::Tensor global_features{};         // [G]
};

struct policy_input_builder_options_t {
  std::string graph_order_fingerprint{};
  std::string allocation_belief_digest{};
  std::string execution_profile_digest{};
  std::string reward_contract_id{
      kPostExecutionLedgerLogGrowthCostDrawdownRewardContractV1};
  std::string accounting_numeraire_node_id{};
  std::string causal_schedule_digest{};
  std::string snapshot_family_digest{};
  torch::Tensor previous_target_weights{};
};

struct raw_policy_output_t {
  std::string schema_id{
      "kikijyeba.environment.raw_policy_output.target_node_weights_logits.v1"};
  std::string action_adapter_id{kTargetNodeWeightsSimplexAdapterV1};
  torch::Tensor node_weight_logits{}; // [A]
  double value_estimate{std::numeric_limits<double>::quiet_NaN()};
};

struct target_node_weights_adapter_options_t {
  std::string policy_id{kGraphNodeAllocationPolicyId};
  std::string method_id{"target_node_weights_simplex.v1"};
  std::string policy_artifact_digest{};
  std::string policy_net_digest{};
  std::string policy_dsl_digest{};
  std::string policy_jkimyei_digest{};
  portfolio::timestamp_ms_t decision_timestamp_ms{0};
};

namespace trainable_policy_detail {

[[nodiscard]] inline bool tensor_is_bool_vector(const torch::Tensor &tensor,
                                                std::int64_t size) {
  return tensor.defined() && tensor.dim() == 1 && tensor.size(0) == size &&
         tensor.scalar_type() == torch::kBool;
}

[[nodiscard]] inline torch::Tensor
bool_mask_or_default(torch::Tensor mask, std::int64_t A, bool value) {
  if (!mask.defined()) {
    return torch::full({A}, value, torch::TensorOptions().dtype(torch::kBool));
  }
  if (!tensor_is_bool_vector(mask, A)) {
    throw std::runtime_error("[policy_input] mask must be bool [A]");
  }
  return mask.contiguous();
}

[[nodiscard]] inline torch::Tensor
weights_or_default(torch::Tensor weights, std::int64_t A, double value) {
  if (!weights.defined()) {
    return torch::full({A}, value,
                       torch::TensorOptions().dtype(torch::kFloat64));
  }
  weights = weights.to(torch::kFloat64).contiguous();
  if (weights.dim() != 1 || weights.size(0) != A ||
      !torch::isfinite(weights).all().item<bool>()) {
    throw std::runtime_error("[policy_input] weights must be finite [A]");
  }
  return weights;
}

[[nodiscard]] inline torch::Tensor vector_or_default(torch::Tensor values,
                                                     std::int64_t A,
                                                     double value,
                                                     const char *name) {
  if (!values.defined() || values.numel() == 0) {
    return torch::full({A}, value,
                       torch::TensorOptions().dtype(torch::kFloat64));
  }
  values = values.to(torch::kFloat64).contiguous();
  if (values.dim() != 1 || values.size(0) != A ||
      !torch::isfinite(values).all().item<bool>()) {
    throw std::runtime_error(std::string("[policy_input] ") + name +
                             " must be finite [A]");
  }
  return values;
}

inline void validate_node_axis(const std::vector<std::string> &node_ids) {
  if (node_ids.empty()) {
    throw std::runtime_error("[policy_input] node_ids are required");
  }
  std::unordered_set<std::string> seen;
  seen.reserve(node_ids.size());
  for (const auto &node_id : node_ids) {
    if (detail::blank(node_id) || !seen.insert(node_id).second) {
      throw std::runtime_error(
          "[policy_input] node_ids must be nonempty and unique");
    }
  }
}

[[nodiscard]] inline std::int64_t
require_node_index(const std::vector<std::string> &node_ids,
                   const std::string &node_id, const char *field_name) {
  if (detail::blank(node_id)) {
    throw std::runtime_error(std::string("[policy_input] ") + field_name +
                             " is required");
  }
  const auto it = std::find(node_ids.begin(), node_ids.end(), node_id);
  if (it == node_ids.end()) {
    throw std::runtime_error(std::string("[policy_input] ") + field_name +
                             " must be present in node_ids");
  }
  return static_cast<std::int64_t>(std::distance(node_ids.begin(), it));
}

} // namespace trainable_policy_detail

inline void validate_policy_input(const policy_input_t &input) {
  if (input.schema_id != kPolicyInputSchemaV1) {
    throw std::runtime_error("[policy_input] unsupported schema_id");
  }
  if (input.environment_assembly_id != kReplayEnvironmentAssemblyId) {
    throw std::runtime_error("[policy_input] environment assembly mismatch");
  }
  if (detail::blank(input.observation_anchor_key) ||
      input.observation_anchor_index < 0 || input.knowledge_timestamp_ms <= 0) {
    throw std::runtime_error(
        "[policy_input] observation causal identity is required");
  }
  if (detail::blank(input.graph_order_fingerprint) ||
      detail::blank(input.execution_profile_digest) ||
      detail::blank(input.reward_contract_id) ||
      detail::blank(input.causal_schedule_digest) ||
      detail::blank(input.snapshot_family_digest)) {
    throw std::runtime_error(
        "[policy_input] graph, execution, reward, causal schedule, and "
        "snapshot identity are required");
  }
  if (input.reward_contract_id !=
      kPostExecutionLedgerLogGrowthCostDrawdownRewardContractV1) {
    throw std::runtime_error("[policy_input] unsupported reward_contract_id");
  }
  trainable_policy_detail::validate_node_axis(input.node_ids);
  (void)trainable_policy_detail::require_node_index(
      input.node_ids, input.accounting_numeraire_node_id,
      "accounting_numeraire_node_id");
  const auto A = static_cast<std::int64_t>(input.node_ids.size());
  if (!trainable_policy_detail::tensor_is_bool_vector(input.valid_mask, A) ||
      !trainable_policy_detail::tensor_is_bool_vector(input.tradable_mask, A) ||
      !trainable_policy_detail::tensor_is_bool_vector(input.executable_mask,
                                                      A)) {
    throw std::runtime_error("[policy_input] masks must be bool [A]");
  }
  const auto check_weights = [A](const torch::Tensor &weights,
                                 const char *name) {
    if (!weights.defined() || weights.dim() != 1 || weights.size(0) != A ||
        !torch::isfinite(weights.to(torch::kFloat64)).all().item<bool>()) {
      throw std::runtime_error(std::string("[policy_input] ") + name +
                               " must be finite [A]");
    }
  };
  check_weights(input.current_weights, "current_weights");
  check_weights(input.previous_target_weights, "previous_target_weights");
  if (!input.node_features.defined() || input.node_features.dim() != 2 ||
      input.node_features.size(0) != A ||
      input.node_features.size(1) != kPolicyInputNodeFeatureDimV1 ||
      !torch::isfinite(input.node_features.to(torch::kFloat64))
           .all()
           .item<bool>()) {
    throw std::runtime_error(
        "[policy_input] node_features must be finite [A,F]");
  }
  if (!input.global_features.defined() || input.global_features.dim() != 1 ||
      input.global_features.size(0) != kPolicyInputGlobalFeatureDimV1 ||
      !torch::isfinite(input.global_features.to(torch::kFloat64))
           .all()
           .item<bool>()) {
    throw std::runtime_error(
        "[policy_input] global_features must be finite [G]");
  }
}

[[nodiscard]] inline policy_input_t
make_policy_input(const observation_t &observation,
                  const policy_input_builder_options_t &options) {
  if (observation.portfolio_state.node_ids.empty()) {
    throw std::runtime_error(
        "[policy_input] observation portfolio_state.node_ids are required");
  }
  policy_input_t out{};
  out.observation_anchor_key = observation.anchor_key;
  out.observation_anchor_index = observation.observation_anchor_index;
  out.knowledge_timestamp_ms = observation.knowledge_timestamp_ms;
  out.graph_order_fingerprint = options.graph_order_fingerprint;
  if (out.graph_order_fingerprint.empty() && observation.allocation_belief) {
    out.graph_order_fingerprint =
        observation.allocation_belief->graph_order_fingerprint;
  }
  out.allocation_belief_digest = options.allocation_belief_digest;
  out.execution_profile_digest = options.execution_profile_digest;
  out.reward_contract_id = options.reward_contract_id;
  out.accounting_numeraire_node_id =
      !detail::blank(options.accounting_numeraire_node_id)
          ? options.accounting_numeraire_node_id
          : observation.portfolio_state.accounting_numeraire_node_id;
  if (!detail::blank(options.accounting_numeraire_node_id) &&
      !detail::blank(
          observation.portfolio_state.accounting_numeraire_node_id) &&
      options.accounting_numeraire_node_id !=
          observation.portfolio_state.accounting_numeraire_node_id) {
    throw std::runtime_error(
        "[policy_input] accounting_numeraire_node_id option does not match "
        "observation portfolio state");
  }
  out.causal_schedule_digest = options.causal_schedule_digest;
  out.snapshot_family_digest = options.snapshot_family_digest;
  out.node_ids = observation.portfolio_state.node_ids;
  trainable_policy_detail::validate_node_axis(out.node_ids);
  const belief::AllocationBelief *belief_state =
      observation.allocation_belief ? &*observation.allocation_belief : nullptr;
  if (belief_state != nullptr) {
    belief::validate_allocation_belief_contract(*belief_state);
    if (!belief_state->valid) {
      throw std::runtime_error("[policy_input] AllocationBelief must be valid");
    }
    if (belief_state->timestamp_ms > observation.knowledge_timestamp_ms) {
      throw std::runtime_error(
          "[policy_input] AllocationBelief timestamp is after policy "
          "knowledge timestamp");
    }
    if (belief_state->node_ids != out.node_ids) {
      throw std::runtime_error(
          "[policy_input] AllocationBelief node_ids must match policy input "
          "node_ids");
    }
    if (!belief_state->graph_order_fingerprint.empty() &&
        !out.graph_order_fingerprint.empty() &&
        belief_state->graph_order_fingerprint != out.graph_order_fingerprint) {
      throw std::runtime_error(
          "[policy_input] AllocationBelief graph_order_fingerprint mismatch");
    }
    if (out.graph_order_fingerprint.empty()) {
      out.graph_order_fingerprint = belief_state->graph_order_fingerprint;
    }
  }
  const auto numeraire_index = trainable_policy_detail::require_node_index(
      out.node_ids, out.accounting_numeraire_node_id,
      "accounting_numeraire_node_id");
  const auto A = static_cast<std::int64_t>(out.node_ids.size());
  const auto belief_valid_mask =
      belief_state != nullptr
          ? trainable_policy_detail::bool_mask_or_default(
                belief_state->valid_mask, A, true)
          : torch::ones({A}, torch::TensorOptions().dtype(torch::kBool));
  const auto belief_tradable_mask =
      belief_state != nullptr
          ? trainable_policy_detail::bool_mask_or_default(
                belief_state->tradable_mask, A, true)
          : torch::ones({A}, torch::TensorOptions().dtype(torch::kBool));
  const auto market_tradable_mask =
      trainable_policy_detail::bool_mask_or_default(
          observation.market_state.tradable_mask, A, true);
  out.valid_mask = belief_valid_mask.contiguous();
  out.tradable_mask =
      (market_tradable_mask & belief_tradable_mask).contiguous();
  out.executable_mask = (out.valid_mask & out.tradable_mask).contiguous();
  out.current_weights = trainable_policy_detail::weights_or_default(
      observation.portfolio_state.current_weights, A, 0.0);
  out.previous_target_weights = trainable_policy_detail::weights_or_default(
      options.previous_target_weights, A, 1.0 / static_cast<double>(A));

  auto executable_mid = trainable_policy_detail::weights_or_default(
      observation.market_state.executable_mid, A, 1.0);
  executable_mid = torch::where(executable_mid > 0.0, executable_mid,
                                torch::ones_like(executable_mid));
  const auto expected_log_return =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->expected_log_return, A, 0.0,
                "expected_log_return")
          : torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto expected_arithmetic_return =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->expected_arithmetic_return, A, 0.0,
                "expected_arithmetic_return")
          : torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto marginal_variance =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->marginal_variance, A, 0.0, "marginal_variance")
          : torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto marginal_volatility =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->marginal_volatility, A, 0.0,
                "marginal_volatility")
          : torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto var_down =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(belief_state->var_down,
                                                       A, 0.0, "var_down")
          : torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto cvar_down =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(belief_state->cvar_down,
                                                       A, 0.0, "cvar_down")
          : torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto adverse_excursion_prob =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->adverse_excursion_prob, A, 0.0,
                "adverse_excursion_prob")
          : torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto volatility =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(belief_state->volatility,
                                                       A, 0.0, "volatility")
          : torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto confidence =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(belief_state->confidence,
                                                       A, 1.0, "confidence")
          : torch::ones({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto liquidity_score =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->liquidity_score, A, 1.0, "liquidity_score")
          : torch::ones({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto linear_cost =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->linear_cost, A, 0.0, "linear_cost")
          : torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto quadratic_impact =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->quadratic_impact, A, 0.0, "quadratic_impact")
          : torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto capacity_weight_limit =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->capacity_weight_limit, A, 1.0,
                "capacity_weight_limit")
          : torch::ones({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto projection_validation_score =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->projection_validation_score, A, 1.0,
                "projection_validation_score")
          : torch::ones({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto residual_quality_score =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->residual_quality_score, A, 1.0,
                "residual_quality_score")
          : torch::ones({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto surprise =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(belief_state->surprise,
                                                       A, 0.0, "surprise")
          : torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto calibration_score =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->calibration_score, A, 1.0, "calibration_score")
          : torch::ones({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto mixture_entropy =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->mixture_entropy, A, 0.0, "mixture_entropy")
          : torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto component_disagreement =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->component_disagreement, A, 0.0,
                "component_disagreement")
          : torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto channel_disagreement =
      belief_state != nullptr
          ? trainable_policy_detail::vector_or_default(
                belief_state->channel_disagreement, A, 0.0,
                "channel_disagreement")
          : torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto tradable_float = out.tradable_mask.to(torch::kFloat64);
  const auto executable_float = out.executable_mask.to(torch::kFloat64);
  out.node_features =
      torch::stack({out.current_weights.to(torch::kFloat64),
                    out.previous_target_weights.to(torch::kFloat64),
                    out.current_weights.to(torch::kFloat64) -
                        out.previous_target_weights.to(torch::kFloat64),
                    torch::log(executable_mid.to(torch::kFloat64)),
                    out.valid_mask.to(torch::kFloat64),
                    tradable_float,
                    executable_float,
                    expected_log_return,
                    expected_arithmetic_return,
                    marginal_variance,
                    marginal_volatility,
                    var_down,
                    cvar_down,
                    adverse_excursion_prob,
                    volatility,
                    confidence,
                    liquidity_score,
                    linear_cost,
                    quadratic_impact,
                    capacity_weight_limit,
                    projection_validation_score,
                    residual_quality_score,
                    surprise,
                    calibration_score,
                    mixture_entropy,
                    component_disagreement,
                    channel_disagreement},
                   1)
          .contiguous();

  const double equity = observation.portfolio_state.equity_value_numeraire > 0.0
                            ? observation.portfolio_state.equity_value_numeraire
                            : 0.0;
  const double drawdown = std::isfinite(observation.portfolio_state.drawdown)
                              ? observation.portfolio_state.drawdown
                              : 0.0;
  const double current_sum =
      out.current_weights.to(torch::kFloat64).sum().item<double>();
  const double previous_sum =
      out.previous_target_weights.to(torch::kFloat64).sum().item<double>();
  const double current_numeraire_weight =
      out.current_weights.to(torch::kFloat64)
          .index({numeraire_index})
          .item<double>();
  const double previous_numeraire_weight =
      out.previous_target_weights.to(torch::kFloat64)
          .index({numeraire_index})
          .item<double>();
  out.global_features =
      torch::tensor({equity, drawdown, current_sum, previous_sum,
                     current_numeraire_weight, previous_numeraire_weight,
                     static_cast<double>(observation.observation_anchor_index),
                     static_cast<double>(observation.knowledge_timestamp_ms)},
                    torch::TensorOptions().dtype(torch::kFloat64));
  validate_policy_input(out);
  return out;
}

inline void validate_raw_policy_output(const raw_policy_output_t &raw,
                                       std::int64_t A) {
  if (raw.action_adapter_id != kTargetNodeWeightsSimplexAdapterV1) {
    throw std::runtime_error("[raw_policy_output] unsupported action adapter");
  }
  if (!raw.node_weight_logits.defined() || raw.node_weight_logits.dim() != 1 ||
      raw.node_weight_logits.size(0) != A ||
      !torch::isfinite(raw.node_weight_logits.to(torch::kFloat64))
           .all()
           .item<bool>()) {
    throw std::runtime_error(
        "[raw_policy_output] node_weight_logits must be finite [A]");
  }
}

[[nodiscard]] inline action_t adapt_raw_output_to_target_node_weights_action(
    const raw_policy_output_t &raw, const policy_input_t &input,
    const target_node_weights_adapter_options_t &options) {
  validate_policy_input(input);
  const auto A = static_cast<std::int64_t>(input.node_ids.size());
  validate_raw_policy_output(raw, A);
  if (detail::blank(options.policy_id) || detail::blank(options.method_id) ||
      detail::blank(options.policy_artifact_digest)) {
    throw std::runtime_error(
        "[target_node_weights_adapter] policy id, method id, and artifact "
        "digest are required");
  }

  const auto logits = raw.node_weight_logits.to(torch::kFloat64).contiguous();
  const auto mask =
      (input.valid_mask & input.tradable_mask & input.executable_mask)
          .contiguous();
  if (!mask.any().item<bool>()) {
    throw std::runtime_error(
        "[target_node_weights_adapter] no executable graph node is available");
  }
  double max_logit = -std::numeric_limits<double>::infinity();
  for (std::int64_t i = 0; i < A; ++i) {
    if (mask.index({i}).item<bool>()) {
      max_logit = std::max(max_logit, logits.index({i}).item<double>());
    }
  }
  std::vector<double> weights(static_cast<std::size_t>(A), 0.0);
  double denom = 0.0;
  for (std::int64_t i = 0; i < A; ++i) {
    if (!mask.index({i}).item<bool>()) {
      continue;
    }
    const double value = std::exp(logits.index({i}).item<double>() - max_logit);
    weights[static_cast<std::size_t>(i)] = value;
    denom += value;
  }
  if (!(denom > 0.0) || !std::isfinite(denom)) {
    throw std::runtime_error(
        "[target_node_weights_adapter] softmax denominator is invalid");
  }
  for (auto &weight : weights) {
    weight /= denom;
  }

  action_t action{};
  action.policy_id = options.policy_id;
  action.method_id = options.method_id;
  action.policy_kind = policy_kind_t::reinforcement_learning;
  action.decision_timestamp_ms = options.decision_timestamp_ms > 0
                                     ? options.decision_timestamp_ms
                                     : input.knowledge_timestamp_ms;
  action.node_ids = input.node_ids;
  action.target_weights =
      torch::tensor(weights, torch::TensorOptions().dtype(torch::kFloat64));
  action.policy_input_schema_id = input.schema_id;
  action.action_adapter_id = kTargetNodeWeightsSimplexAdapterV1;
  action.reward_contract_id = input.reward_contract_id;
  action.policy_artifact_digest = options.policy_artifact_digest;
  action.policy_net_digest = options.policy_net_digest;
  action.policy_dsl_digest = options.policy_dsl_digest;
  action.policy_jkimyei_digest = options.policy_jkimyei_digest;
  validate_action(action, input.node_ids);
  return action;
}

class trainable_policy_adapter_iface_t : public policy_adapter_iface_t {
public:
  [[nodiscard]] virtual std::string policy_artifact_digest() const = 0;
  [[nodiscard]] virtual std::string policy_input_schema_id() const = 0;
  [[nodiscard]] virtual std::string action_adapter_id() const = 0;
  [[nodiscard]] virtual std::string reward_contract_id() const = 0;
  [[nodiscard]] virtual policy_input_t
  make_input(const observation_t &observation) const = 0;
  [[nodiscard]] virtual raw_policy_output_t
  forward(const policy_input_t &input) = 0;

  [[nodiscard]] action_t act(const observation_t &observation) override {
    auto input = make_input(observation);
    auto raw = forward(input);
    target_node_weights_adapter_options_t options{};
    options.policy_id = policy_id();
    options.method_id = action_adapter_id();
    options.policy_artifact_digest = policy_artifact_digest();
    options.decision_timestamp_ms = input.knowledge_timestamp_ms;
    return adapt_raw_output_to_target_node_weights_action(raw, input, options);
  }
};

struct fake_trainable_policy_config_t {
  std::string policy_id{kGraphNodeAllocationPolicyId};
  std::string policy_artifact_digest{"fake_trainable_policy_fixture_digest"};
  std::string graph_order_fingerprint{};
  std::string execution_profile_digest{"cajtucu.paper.fixture.digest"};
  std::string accounting_numeraire_node_id{};
  std::string causal_schedule_digest{};
  std::string snapshot_family_digest{"snapshot_family.fixture.digest"};
  torch::Tensor logits{};
  torch::Tensor previous_target_weights{};
};

class fake_trainable_policy_t final : public trainable_policy_adapter_iface_t {
public:
  explicit fake_trainable_policy_t(fake_trainable_policy_config_t config)
      : config_(std::move(config)) {
    if (detail::blank(config_.policy_id) ||
        detail::blank(config_.policy_artifact_digest)) {
      throw std::runtime_error(
          "[fake_trainable_policy] policy id and artifact digest are required");
    }
  }

  [[nodiscard]] std::string policy_id() const override {
    return config_.policy_id;
  }

  [[nodiscard]] policy_kind_t policy_kind() const override {
    return policy_kind_t::reinforcement_learning;
  }

  [[nodiscard]] std::string policy_artifact_digest() const override {
    return config_.policy_artifact_digest;
  }

  [[nodiscard]] std::string policy_input_schema_id() const override {
    return kPolicyInputSchemaV1;
  }

  [[nodiscard]] std::string action_adapter_id() const override {
    return kTargetNodeWeightsSimplexAdapterV1;
  }

  [[nodiscard]] std::string reward_contract_id() const override {
    return kPostExecutionLedgerLogGrowthCostDrawdownRewardContractV1;
  }

  [[nodiscard]] policy_input_t
  make_input(const observation_t &observation) const override {
    policy_input_builder_options_t options{};
    options.graph_order_fingerprint = config_.graph_order_fingerprint;
    options.execution_profile_digest = config_.execution_profile_digest;
    options.accounting_numeraire_node_id = config_.accounting_numeraire_node_id;
    options.causal_schedule_digest = config_.causal_schedule_digest;
    options.reward_contract_id = reward_contract_id();
    options.snapshot_family_digest = config_.snapshot_family_digest;
    options.previous_target_weights = config_.previous_target_weights;
    return make_policy_input(observation, options);
  }

  [[nodiscard]] raw_policy_output_t
  forward(const policy_input_t &input) override {
    validate_policy_input(input);
    raw_policy_output_t out{};
    if (config_.logits.defined()) {
      out.node_weight_logits = config_.logits.to(torch::kFloat64).contiguous();
    } else {
      out.node_weight_logits =
          torch::zeros({static_cast<std::int64_t>(input.node_ids.size())},
                       torch::TensorOptions().dtype(torch::kFloat64));
    }
    return out;
  }

private:
  fake_trainable_policy_config_t config_{};
};

} // namespace cuwacunu::kikijyeba::environment
