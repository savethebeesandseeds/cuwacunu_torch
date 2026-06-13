// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/environment/control/interfaces.h"
#include "wikimyei/assembly.h"
#include "wikimyei/policy/portfolio/graph_node_allocation/torch_policy_module.h"

namespace cuwacunu::kikijyeba::environment {

inline constexpr const char *kPolicyInputSchemaV1 =
    "kikijyeba.environment.policy_input.v1";
inline constexpr const char *kPolicyInputTensorPayloadSchemaV1 =
    "kikijyeba.environment.policy_input.tensor_payload.v1";
inline constexpr const char *kTargetNodeWeightsSimplexAdapterV1 =
    "target_node_weights_simplex.v1";
inline constexpr const char *kMaskedDirichletSimplexDistributionV1 =
    "masked_dirichlet_simplex.v1";
inline constexpr const char *kMaskedLogisticNormalSimplexDistributionV1 =
    "masked_logistic_normal_simplex.v1";
inline constexpr const char
    *kPostExecutionLedgerLogGrowthCostDrawdownRewardContractV1 =
        "kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_"
        "drawdown.v1";
inline constexpr const char *kGraphNodeAllocationPolicyId =
    "wikimyei.policy.portfolio.graph_node_allocation.v1";
inline constexpr std::int64_t kPolicyInputNodeFeatureDimV1 = 28;
inline constexpr std::int64_t kPolicyInputGlobalFeatureDimV1 = 6;
inline constexpr std::int64_t kPolicyInputRiskFeatureDimV1 = 10;

[[nodiscard]] inline bool
action_distribution_id_supported(const std::string &distribution_id) {
  return distribution_id == kMaskedDirichletSimplexDistributionV1 ||
         distribution_id == kMaskedLogisticNormalSimplexDistributionV1;
}

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
  torch::Tensor risk_features{};           // [R]
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
  std::string action_distribution_id{kMaskedDirichletSimplexDistributionV1};
  torch::Tensor node_weight_logits{};         // [A]
  torch::Tensor action_distribution_params{}; // distribution-specific scalar.
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

struct action_distribution_options_t {
  double dirichlet_alpha_floor{1.0e-4};
  double dirichlet_total_concentration{16.0};
  double dirichlet_total_concentration_min{0.25};
  double dirichlet_total_concentration_max{256.0};
  double logistic_normal_log_std{-1.0};
  double logistic_normal_log_std_min{-5.0};
  double logistic_normal_log_std_max{1.0};
  std::uint64_t random_seed{0};
  bool random_seed_bound{false};
  torch::Tensor standard_normal_noise{}; // [K-1], optional test fixture.
};

struct action_distribution_evidence_t {
  std::string action_distribution_id{};
  std::string entropy_kind{};
  std::vector<std::int64_t> active_node_indices{};
  std::int64_t active_count{0};
  std::int64_t reference_node_index{-1};
  torch::Tensor target_node_weights{};
  torch::Tensor sampled_active_weights{};
  torch::Tensor alpha_active{};
  torch::Tensor mean_active{};
  torch::Tensor mu_z{};
  torch::Tensor std_z{};
  torch::Tensor latent_z_sample{};
  double total_concentration{std::numeric_limits<double>::quiet_NaN()};
  double log_prob{std::numeric_limits<double>::quiet_NaN()};
  double entropy{std::numeric_limits<double>::quiet_NaN()};
  double value_estimate{std::numeric_limits<double>::quiet_NaN()};
};

struct action_distribution_record_t {
  std::string action_distribution_id{};
  action_distribution_evidence_t evidence{};
};

struct action_sample_t {
  action_t action{};
  action_distribution_evidence_t evidence{};
};

namespace trainable_policy_detail {

inline void mix_i64(std::uint64_t &hash, std::int64_t value) {
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, std::to_string(value));
}

inline void mix_double(std::uint64_t &hash, double value) {
  std::ostringstream out;
  out.precision(17);
  out << value;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(hash,
                                                                 out.str());
}

inline void mix_tensor_summary(std::uint64_t &hash, const torch::Tensor &tensor,
                               std::string_view label) {
  using cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string;
  mix_hash_string(hash, label);
  if (!tensor.defined()) {
    mix_hash_string(hash, "undefined");
    return;
  }
  mix_hash_string(hash, std::to_string(tensor.dim()));
  for (const auto size : tensor.sizes()) {
    mix_hash_string(hash, std::to_string(size));
  }
  const auto flat = tensor.to(torch::kFloat64).contiguous().view({-1});
  const auto *data = flat.data_ptr<double>();
  const auto count = flat.numel();
  for (std::int64_t i = 0; i < count; ++i) {
    mix_double(hash, data[i]);
  }
}

[[nodiscard]] inline std::string
policy_input_digest(const policy_input_t &input) {
  using cuwacunu::wikimyei::assembly::assembly_detail::hash_hex;
  using cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  using cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string;
  std::uint64_t hash = kFnvOffsetBasis;
  mix_hash_string(hash, "kikijyeba.environment.policy_input.digest.v1");
  mix_hash_string(hash, input.schema_id);
  mix_hash_string(hash, input.environment_assembly_id);
  mix_hash_string(hash, input.observation_anchor_key);
  mix_i64(hash, input.observation_anchor_index);
  mix_i64(hash, input.knowledge_timestamp_ms);
  mix_hash_string(hash, input.graph_order_fingerprint);
  mix_hash_string(hash, input.allocation_belief_digest);
  mix_hash_string(hash, input.execution_profile_digest);
  mix_hash_string(hash, input.reward_contract_id);
  mix_hash_string(hash, input.accounting_numeraire_node_id);
  mix_hash_string(hash, input.causal_schedule_digest);
  mix_hash_string(hash, input.snapshot_family_digest);
  for (const auto &node_id : input.node_ids) {
    mix_hash_string(hash, node_id);
  }
  mix_tensor_summary(hash, input.valid_mask, "valid_mask");
  mix_tensor_summary(hash, input.tradable_mask, "tradable_mask");
  mix_tensor_summary(hash, input.executable_mask, "executable_mask");
  mix_tensor_summary(hash, input.current_weights, "current_weights");
  mix_tensor_summary(hash, input.previous_target_weights,
                     "previous_target_weights");
  mix_tensor_summary(hash, input.node_features, "node_features");
  mix_tensor_summary(hash, input.global_features, "global_features");
  mix_tensor_summary(hash, input.risk_features, "risk_features");
  return hash_hex(hash);
}

[[nodiscard]] inline std::string
format_active_node_indices(const std::vector<std::int64_t> &indices) {
  std::ostringstream out;
  for (std::size_t i = 0; i < indices.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << indices[i];
  }
  return out.str();
}

[[nodiscard]] inline std::string tensor_shape_csv(const torch::Tensor &tensor) {
  if (!tensor.defined()) {
    return {};
  }
  std::ostringstream out;
  for (std::int64_t i = 0; i < tensor.dim(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << tensor.size(i);
  }
  return out.str();
}

[[nodiscard]] inline std::string tensor_flat_values_csv(torch::Tensor tensor) {
  if (!tensor.defined()) {
    return {};
  }
  const auto flat = tensor.to(torch::kFloat64).contiguous().view({-1});
  if (!torch::isfinite(flat).all().item<bool>()) {
    return {};
  }
  std::ostringstream out;
  out << std::setprecision(17);
  const auto *data = flat.data_ptr<double>();
  for (std::int64_t i = 0; i < flat.numel(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << data[i];
  }
  return out.str();
}

inline void bind_policy_input_tensor_payload(action_t &action,
                                             const policy_input_t &input) {
  action.policy_input_tensor_payload_schema_id =
      kPolicyInputTensorPayloadSchemaV1;
  action.policy_input_node_features_shape = tensor_shape_csv(input.node_features);
  action.policy_input_node_features = tensor_flat_values_csv(input.node_features);
  action.policy_input_global_features_shape =
      tensor_shape_csv(input.global_features);
  action.policy_input_global_features =
      tensor_flat_values_csv(input.global_features);
  action.policy_input_risk_features_shape = tensor_shape_csv(input.risk_features);
  action.policy_input_risk_features = tensor_flat_values_csv(input.risk_features);
  action.policy_input_executable_mask_shape =
      tensor_shape_csv(input.executable_mask);
  action.policy_input_executable_mask =
      tensor_flat_values_csv(input.executable_mask);
  action.policy_input_tensor_payload_bound =
      !action.policy_input_node_features_shape.empty() &&
      !action.policy_input_node_features.empty() &&
      !action.policy_input_global_features_shape.empty() &&
      !action.policy_input_global_features.empty() &&
      !action.policy_input_risk_features_shape.empty() &&
      !action.policy_input_risk_features.empty() &&
      !action.policy_input_executable_mask_shape.empty() &&
      !action.policy_input_executable_mask.empty();
}

inline void bind_action_distribution_evidence(
    action_t &action, const policy_input_t &input,
    const action_distribution_evidence_t &evidence) {
  action.action_distribution_id = evidence.action_distribution_id;
  action.policy_input_digest = policy_input_digest(input);
  action.active_node_indices =
      format_active_node_indices(evidence.active_node_indices);
  action.active_count = evidence.active_count;
  action.old_log_prob = evidence.log_prob;
  action.old_entropy = evidence.entropy;
  action.old_value_estimate = evidence.value_estimate;
  action.action_distribution_evidence_bound =
      !action.action_distribution_id.empty() &&
      !action.policy_input_digest.empty() && evidence.active_count > 0 &&
      std::isfinite(evidence.log_prob) && std::isfinite(evidence.entropy) &&
      std::isfinite(evidence.value_estimate);
  bind_policy_input_tensor_payload(action, input);
}

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

[[nodiscard]] inline torch::Tensor matrix_or_default(torch::Tensor matrix,
                                                     std::int64_t A,
                                                     double diag_value,
                                                     const char *name) {
  if (!matrix.defined() || matrix.numel() == 0) {
    return torch::eye(A, torch::TensorOptions().dtype(torch::kFloat64)) *
           diag_value;
  }
  matrix = matrix.to(torch::kFloat64).contiguous();
  if (matrix.dim() != 2 || matrix.size(0) != A || matrix.size(1) != A ||
      !torch::isfinite(matrix).all().item<bool>()) {
    throw std::runtime_error(std::string("[policy_input] ") + name +
                             " must be finite [A,A]");
  }
  return matrix;
}

[[nodiscard]] inline std::array<double, 3>
top3_symmetric_eigenvalues(const torch::Tensor &matrix) {
  auto eig = torch::linalg_eigvalsh(matrix.to(torch::kFloat64)).contiguous();
  eig = std::get<0>(eig.sort(/*dim=*/0, /*descending=*/true));
  std::array<double, 3> out{0.0, 0.0, 0.0};
  const auto count = std::min<std::int64_t>(3, eig.numel());
  for (std::int64_t i = 0; i < count; ++i) {
    const double value = eig.index({i}).item<double>();
    out[static_cast<std::size_t>(i)] = std::isfinite(value) ? value : 0.0;
  }
  return out;
}

[[nodiscard]] inline torch::Tensor
compact_risk_features_or_zero(const belief::AllocationBelief *belief_state,
                              const torch::Tensor &current_weights,
                              std::int64_t A) {
  if (belief_state == nullptr) {
    return torch::zeros({kPolicyInputRiskFeatureDimV1},
                        torch::TensorOptions().dtype(torch::kFloat64));
  }
  const auto correlation =
      matrix_or_default(belief_state->correlation, A, 1.0, "correlation");
  const auto covariance =
      matrix_or_default(belief_state->covariance, A, 0.0, "covariance");

  double corr_sum = 0.0;
  double corr_max = 0.0;
  std::int64_t corr_count = 0;
  for (std::int64_t i = 0; i < A; ++i) {
    for (std::int64_t j = i + 1; j < A; ++j) {
      const double value = correlation.index({i, j}).item<double>();
      corr_sum += value;
      corr_max = (corr_count == 0) ? value : std::max(corr_max, value);
      ++corr_count;
    }
  }
  const double mean_corr =
      corr_count > 0 ? corr_sum / static_cast<double>(corr_count) : 0.0;
  if (corr_count == 0) {
    corr_max = 0.0;
  }

  const auto corr_eigs = top3_symmetric_eigenvalues(correlation);
  const auto cov_eigs = top3_symmetric_eigenvalues(covariance);
  const auto weights64 = current_weights.to(torch::kFloat64).view({1, A});
  double variance = torch::matmul(torch::matmul(weights64, covariance),
                                  weights64.transpose(0, 1))
                        .item<double>();
  if (!std::isfinite(variance) || variance < 0.0) {
    variance = 0.0;
  }
  const double portfolio_volatility = std::sqrt(variance);
  const auto cvar_down =
      vector_or_default(belief_state->cvar_down, A, 0.0, "cvar_down");
  const double portfolio_cvar =
      (current_weights.to(torch::kFloat64) * cvar_down).sum().item<double>();

  return torch::tensor({mean_corr, corr_max, corr_eigs[0], corr_eigs[1],
                        corr_eigs[2], cov_eigs[0], cov_eigs[1], cov_eigs[2],
                        portfolio_volatility,
                        std::isfinite(portfolio_cvar) ? portfolio_cvar : 0.0},
                       torch::TensorOptions().dtype(torch::kFloat64));
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
  if (!input.risk_features.defined() || input.risk_features.dim() != 1 ||
      input.risk_features.size(0) != kPolicyInputRiskFeatureDimV1 ||
      !torch::isfinite(input.risk_features.to(torch::kFloat64))
           .all()
           .item<bool>()) {
    throw std::runtime_error("[policy_input] risk_features must be finite [R]");
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
  auto is_accounting_numeraire =
      torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  is_accounting_numeraire.index_put_({numeraire_index}, 1.0);
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
                    channel_disagreement,
                    is_accounting_numeraire},
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
  const double log_equity = std::log(std::max(equity, 1.0e-12));
  out.global_features =
      torch::tensor({log_equity, drawdown, current_sum, previous_sum,
                     current_numeraire_weight, previous_numeraire_weight},
                    torch::TensorOptions().dtype(torch::kFloat64));
  out.risk_features = trainable_policy_detail::compact_risk_features_or_zero(
      belief_state, out.current_weights, A);
  validate_policy_input(out);
  return out;
}

inline void validate_raw_policy_output(const raw_policy_output_t &raw,
                                       std::int64_t A) {
  if (raw.action_adapter_id != kTargetNodeWeightsSimplexAdapterV1) {
    throw std::runtime_error("[raw_policy_output] unsupported action adapter");
  }
  if (!action_distribution_id_supported(raw.action_distribution_id)) {
    throw std::runtime_error(
        "[raw_policy_output] unsupported action distribution");
  }
  if (!raw.node_weight_logits.defined() || raw.node_weight_logits.dim() != 1 ||
      raw.node_weight_logits.size(0) != A ||
      !torch::isfinite(raw.node_weight_logits.to(torch::kFloat64))
           .all()
           .item<bool>()) {
    throw std::runtime_error(
        "[raw_policy_output] node_weight_logits must be finite [A]");
  }
  if (raw.action_distribution_params.defined() &&
      raw.action_distribution_params.numel() > 0 &&
      !torch::isfinite(raw.action_distribution_params.to(torch::kFloat64))
           .all()
           .item<bool>()) {
    throw std::runtime_error(
        "[raw_policy_output] action_distribution_params must be finite");
  }
}

namespace action_distribution_detail {

inline constexpr double kPi = 3.141592653589793238462643383279502884;

[[nodiscard]] inline double clamp_finite(double value, double lo, double hi,
                                         const char *name) {
  if (!std::isfinite(value)) {
    throw std::runtime_error(std::string("[action_distribution] ") + name +
                             " must be finite");
  }
  return std::max(lo, std::min(hi, value));
}

[[nodiscard]] inline double softplus(double value) {
  if (value > 20.0) {
    return value;
  }
  if (value < -20.0) {
    return std::exp(value);
  }
  return std::log1p(std::exp(value));
}

[[nodiscard]] inline std::vector<std::int64_t>
active_indices_from_input(const policy_input_t &input) {
  const auto mask =
      (input.valid_mask & input.tradable_mask & input.executable_mask)
          .contiguous();
  const auto A = static_cast<std::int64_t>(input.node_ids.size());
  std::vector<std::int64_t> active;
  active.reserve(static_cast<std::size_t>(A));
  for (std::int64_t i = 0; i < A; ++i) {
    if (mask.index({i}).item<bool>()) {
      active.push_back(i);
    }
  }
  return active;
}

[[nodiscard]] inline torch::Tensor
masked_softmax_active(const torch::Tensor &logits,
                      const std::vector<std::int64_t> &active) {
  if (active.empty()) {
    throw std::runtime_error(
        "[action_distribution] no executable graph node is available");
  }
  std::vector<double> values(active.size(), 0.0);
  double max_logit = -std::numeric_limits<double>::infinity();
  for (std::size_t offset = 0; offset < active.size(); ++offset) {
    const double logit = logits.index({active[offset]}).item<double>();
    values[offset] = logit;
    max_logit = std::max(max_logit, logit);
  }
  double denom = 0.0;
  for (double &value : values) {
    value = std::exp(value - max_logit);
    denom += value;
  }
  if (!(denom > 0.0) || !std::isfinite(denom)) {
    throw std::runtime_error(
        "[action_distribution] softmax denominator is invalid");
  }
  for (double &value : values) {
    value /= denom;
  }
  return torch::tensor(values, torch::TensorOptions().dtype(torch::kFloat64));
}

[[nodiscard]] inline torch::Tensor
scatter_active_weights(std::int64_t A, const std::vector<std::int64_t> &active,
                       const torch::Tensor &active_weights) {
  auto weights =
      torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto active64 = active_weights.to(torch::kFloat64).contiguous();
  if (active64.dim() != 1 ||
      active64.size(0) != static_cast<std::int64_t>(active.size()) ||
      !torch::isfinite(active64).all().item<bool>()) {
    throw std::runtime_error(
        "[action_distribution] active weights must be finite [K]");
  }
  for (std::size_t offset = 0; offset < active.size(); ++offset) {
    weights.index_put_({active[offset]},
                       active64.index({static_cast<std::int64_t>(offset)}));
  }
  return weights.contiguous();
}

[[nodiscard]] inline action_t make_action_from_target_node_weights(
    const policy_input_t &input, const torch::Tensor &target_node_weights,
    const target_node_weights_adapter_options_t &options) {
  if (detail::blank(options.policy_id) || detail::blank(options.method_id) ||
      detail::blank(options.policy_artifact_digest)) {
    throw std::runtime_error(
        "[target_node_weights_adapter] policy id, method id, and artifact "
        "digest are required");
  }
  auto weights = target_node_weights.to(torch::kFloat64).contiguous();
  const auto A = static_cast<std::int64_t>(input.node_ids.size());
  if (!weights.defined() || weights.dim() != 1 || weights.size(0) != A ||
      !torch::isfinite(weights).all().item<bool>()) {
    throw std::runtime_error(
        "[target_node_weights_adapter] target weights must be finite [A]");
  }
  if ((weights < -1.0e-10).any().item<bool>()) {
    throw std::runtime_error(
        "[target_node_weights_adapter] target weights must be nonnegative");
  }

  action_t action{};
  action.policy_id = options.policy_id;
  action.method_id = options.method_id;
  action.policy_kind = policy_kind_t::reinforcement_learning;
  action.decision_timestamp_ms = options.decision_timestamp_ms > 0
                                     ? options.decision_timestamp_ms
                                     : input.knowledge_timestamp_ms;
  action.node_ids = input.node_ids;
  action.target_weights = weights;
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

[[nodiscard]] inline double
distribution_param_scalar_or_default(const raw_policy_output_t &raw,
                                     double default_value) {
  if (!raw.action_distribution_params.defined() ||
      raw.action_distribution_params.numel() == 0) {
    return default_value;
  }
  return raw.action_distribution_params.to(torch::kFloat64)
      .reshape({-1})
      .index({0})
      .item<double>();
}

[[nodiscard]] inline double
dirichlet_total_concentration(const raw_policy_output_t &raw,
                              const action_distribution_options_t &options) {
  if (!raw.action_distribution_params.defined() ||
      raw.action_distribution_params.numel() == 0) {
    return clamp_finite(options.dirichlet_total_concentration,
                        options.dirichlet_total_concentration_min,
                        options.dirichlet_total_concentration_max,
                        "dirichlet_total_concentration");
  }
  const double raw_value = distribution_param_scalar_or_default(raw, 0.0);
  const double concentration =
      options.dirichlet_total_concentration_min + softplus(raw_value);
  return clamp_finite(concentration, options.dirichlet_total_concentration_min,
                      options.dirichlet_total_concentration_max,
                      "dirichlet_total_concentration");
}

[[nodiscard]] inline double
logistic_normal_std(const raw_policy_output_t &raw,
                    const action_distribution_options_t &options) {
  const double raw_log_std = distribution_param_scalar_or_default(
      raw, options.logistic_normal_log_std);
  const double log_std = clamp_finite(
      raw_log_std, options.logistic_normal_log_std_min,
      options.logistic_normal_log_std_max, "logistic_normal_log_std");
  return std::exp(log_std);
}

[[nodiscard]] inline double
dirichlet_log_prob(const torch::Tensor &alpha_active,
                   const torch::Tensor &active_weights) {
  const auto alpha = alpha_active.to(torch::kFloat64).contiguous();
  const auto weights = active_weights.to(torch::kFloat64).contiguous();
  if (alpha.dim() != 1 || weights.dim() != 1 ||
      alpha.size(0) != weights.size(0) || alpha.size(0) <= 0 ||
      !torch::isfinite(alpha).all().item<bool>() ||
      !torch::isfinite(weights).all().item<bool>() ||
      (alpha <= 0.0).any().item<bool>() ||
      (weights <= 0.0).any().item<bool>()) {
    throw std::runtime_error(
        "[action_distribution] invalid Dirichlet log_prob tensors");
  }
  const double alpha_sum = alpha.sum().item<double>();
  double out = std::lgamma(alpha_sum);
  for (std::int64_t i = 0; i < alpha.size(0); ++i) {
    const double a = alpha.index({i}).item<double>();
    const double w = weights.index({i}).item<double>();
    out -= std::lgamma(a);
    out += (a - 1.0) * std::log(w);
  }
  return out;
}

[[nodiscard]] inline double
dirichlet_entropy(const torch::Tensor &alpha_active) {
  const auto alpha = alpha_active.to(torch::kFloat64).contiguous();
  if (alpha.dim() != 1 || alpha.size(0) <= 0 ||
      !torch::isfinite(alpha).all().item<bool>() ||
      (alpha <= 0.0).any().item<bool>()) {
    throw std::runtime_error(
        "[action_distribution] invalid Dirichlet entropy tensor");
  }
  const double alpha_sum = alpha.sum().item<double>();
  double log_beta = -std::lgamma(alpha_sum);
  for (std::int64_t i = 0; i < alpha.size(0); ++i) {
    log_beta += std::lgamma(alpha.index({i}).item<double>());
  }
  const double digamma_sum =
      torch::digamma(torch::tensor(alpha_sum, torch::TensorOptions().dtype(
                                                  torch::kFloat64)))
          .item<double>();
  double out =
      log_beta + (alpha_sum - static_cast<double>(alpha.size(0))) * digamma_sum;
  const auto digamma_alpha = torch::digamma(alpha);
  for (std::int64_t i = 0; i < alpha.size(0); ++i) {
    out -= (alpha.index({i}).item<double>() - 1.0) *
           digamma_alpha.index({i}).item<double>();
  }
  return out;
}

[[nodiscard]] inline torch::Tensor
sample_dirichlet_active(const torch::Tensor &alpha_active,
                        const action_distribution_options_t &options) {
  const auto alpha = alpha_active.to(torch::kFloat64).contiguous();
  std::mt19937_64 rng(options.random_seed_bound ? options.random_seed
                                                : std::random_device{}());
  std::vector<double> draws(static_cast<std::size_t>(alpha.size(0)), 0.0);
  double total = 0.0;
  for (std::int64_t i = 0; i < alpha.size(0); ++i) {
    std::gamma_distribution<double> gamma(alpha.index({i}).item<double>(), 1.0);
    double value = gamma(rng);
    if (!std::isfinite(value) || value < 0.0) {
      value = 0.0;
    }
    draws[static_cast<std::size_t>(i)] = value;
    total += value;
  }
  if (!(total > 0.0) || !std::isfinite(total)) {
    return alpha / alpha.sum();
  }
  for (double &draw : draws) {
    draw /= total;
  }
  return torch::tensor(draws, torch::TensorOptions().dtype(torch::kFloat64));
}

[[nodiscard]] inline double normal_diag_log_prob(const torch::Tensor &z,
                                                 const torch::Tensor &mu,
                                                 const torch::Tensor &std) {
  const auto z64 = z.to(torch::kFloat64).contiguous();
  const auto mu64 = mu.to(torch::kFloat64).contiguous();
  const auto std64 = std.to(torch::kFloat64).contiguous();
  if (z64.dim() != 1 || mu64.dim() != 1 || std64.dim() != 1 ||
      z64.size(0) != mu64.size(0) || z64.size(0) != std64.size(0) ||
      !torch::isfinite(z64).all().item<bool>() ||
      !torch::isfinite(mu64).all().item<bool>() ||
      !torch::isfinite(std64).all().item<bool>() ||
      (std64 <= 0.0).any().item<bool>()) {
    throw std::runtime_error(
        "[action_distribution] invalid diagonal normal tensors");
  }
  double out = 0.0;
  for (std::int64_t i = 0; i < z64.size(0); ++i) {
    const double s = std64.index({i}).item<double>();
    const double centered =
        (z64.index({i}).item<double>() - mu64.index({i}).item<double>()) / s;
    out += -0.5 * centered * centered - std::log(s) - 0.5 * std::log(2.0 * kPi);
  }
  return out;
}

[[nodiscard]] inline double
logistic_normal_log_prob(const torch::Tensor &active_weights,
                         const torch::Tensor &mu_z,
                         const torch::Tensor &std_z) {
  const auto weights = active_weights.to(torch::kFloat64).contiguous();
  if (weights.dim() != 1 || weights.size(0) < 2 ||
      !torch::isfinite(weights).all().item<bool>() ||
      (weights <= 0.0).any().item<bool>()) {
    throw std::runtime_error(
        "[action_distribution] logistic-normal weights must be positive [K]");
  }
  const std::int64_t K = weights.size(0);
  const double ref_weight = weights.index({K - 1}).item<double>();
  std::vector<double> z_values(static_cast<std::size_t>(K - 1), 0.0);
  double log_jacobian = 0.0;
  for (std::int64_t i = 0; i < K; ++i) {
    const double w = weights.index({i}).item<double>();
    log_jacobian -= std::log(w);
    if (i + 1 < K) {
      z_values[static_cast<std::size_t>(i)] = std::log(w / ref_weight);
    }
  }
  const auto z =
      torch::tensor(z_values, torch::TensorOptions().dtype(torch::kFloat64));
  return normal_diag_log_prob(z, mu_z, std_z) + log_jacobian;
}

[[nodiscard]] inline double latent_normal_entropy(const torch::Tensor &std_z) {
  const auto std64 = std_z.to(torch::kFloat64).contiguous();
  if (std64.dim() != 1 || std64.size(0) <= 0 ||
      !torch::isfinite(std64).all().item<bool>() ||
      (std64 <= 0.0).any().item<bool>()) {
    throw std::runtime_error("[action_distribution] invalid latent std tensor");
  }
  double out = 0.0;
  for (std::int64_t i = 0; i < std64.size(0); ++i) {
    const double s = std64.index({i}).item<double>();
    out += 0.5 * (1.0 + std::log(2.0 * kPi)) + std::log(s);
  }
  return out;
}

[[nodiscard]] inline torch::Tensor
standard_normal_noise(std::int64_t dim,
                      const action_distribution_options_t &options) {
  if (dim <= 0) {
    return torch::empty({0}, torch::TensorOptions().dtype(torch::kFloat64));
  }
  if (options.standard_normal_noise.defined()) {
    const auto noise = options.standard_normal_noise.to(torch::kFloat64)
                           .reshape({-1})
                           .contiguous();
    if (noise.size(0) != dim || !torch::isfinite(noise).all().item<bool>()) {
      throw std::runtime_error(
          "[action_distribution] standard_normal_noise must be finite [K-1]");
    }
    return noise;
  }
  std::mt19937_64 rng(options.random_seed_bound ? options.random_seed
                                                : std::random_device{}());
  std::normal_distribution<double> normal(0.0, 1.0);
  std::vector<double> values(static_cast<std::size_t>(dim), 0.0);
  for (double &value : values) {
    value = normal(rng);
  }
  return torch::tensor(values, torch::TensorOptions().dtype(torch::kFloat64));
}

} // namespace action_distribution_detail

class action_distribution_iface_t {
public:
  virtual ~action_distribution_iface_t() = default;
  [[nodiscard]] virtual std::string distribution_id() const = 0;
  [[nodiscard]] virtual action_sample_t
  sample(const raw_policy_output_t &raw, const policy_input_t &input,
         const target_node_weights_adapter_options_t &adapter_options,
         const action_distribution_options_t &distribution_options) const = 0;
  [[nodiscard]] virtual action_sample_t deterministic_action(
      const raw_policy_output_t &raw, const policy_input_t &input,
      const target_node_weights_adapter_options_t &adapter_options,
      const action_distribution_options_t &distribution_options) const = 0;
  [[nodiscard]] virtual double
  log_prob(const raw_policy_output_t &raw, const policy_input_t &input,
           const action_t &action,
           const action_distribution_evidence_t &evidence,
           const action_distribution_options_t &distribution_options) const = 0;
  [[nodiscard]] virtual double
  entropy(const raw_policy_output_t &raw, const policy_input_t &input,
          const action_distribution_evidence_t &evidence,
          const action_distribution_options_t &distribution_options) const = 0;
  [[nodiscard]] virtual double
  approximate_kl(const action_distribution_record_t &old_record,
                 const action_distribution_record_t &new_record) const {
    if (old_record.action_distribution_id != distribution_id() ||
        new_record.action_distribution_id != distribution_id()) {
      throw std::runtime_error(
          "[action_distribution] KL records use a different distribution");
    }
    if (!std::isfinite(old_record.evidence.log_prob) ||
        !std::isfinite(new_record.evidence.log_prob)) {
      throw std::runtime_error(
          "[action_distribution] KL records require finite log_prob");
    }
    return old_record.evidence.log_prob - new_record.evidence.log_prob;
  }
};

class masked_dirichlet_simplex_distribution_t final
    : public action_distribution_iface_t {
public:
  [[nodiscard]] std::string distribution_id() const override {
    return kMaskedDirichletSimplexDistributionV1;
  }

  [[nodiscard]] action_sample_t
  sample(const raw_policy_output_t &raw, const policy_input_t &input,
         const target_node_weights_adapter_options_t &adapter_options,
         const action_distribution_options_t &distribution_options)
      const override {
    return make(raw, input, adapter_options, distribution_options,
                /*deterministic=*/false);
  }

  [[nodiscard]] action_sample_t deterministic_action(
      const raw_policy_output_t &raw, const policy_input_t &input,
      const target_node_weights_adapter_options_t &adapter_options,
      const action_distribution_options_t &distribution_options)
      const override {
    return make(raw, input, adapter_options, distribution_options,
                /*deterministic=*/true);
  }

  [[nodiscard]] double
  log_prob(const raw_policy_output_t &raw, const policy_input_t &input,
           const action_t &action, const action_distribution_evidence_t &,
           const action_distribution_options_t &distribution_options)
      const override {
    validate_policy_input(input);
    const auto A = static_cast<std::int64_t>(input.node_ids.size());
    validate_raw_policy_output(raw, A);
    if (raw.action_distribution_id != distribution_id()) {
      throw std::runtime_error(
          "[action_distribution] raw output distribution mismatch");
    }
    const auto active =
        action_distribution_detail::active_indices_from_input(input);
    if (active.empty()) {
      throw std::runtime_error(
          "[action_distribution] no executable graph node is available");
    }
    if (active.size() == 1) {
      return 0.0;
    }
    const auto logits = raw.node_weight_logits.to(torch::kFloat64).contiguous();
    const auto mean =
        action_distribution_detail::masked_softmax_active(logits, active);
    const double concentration =
        action_distribution_detail::dirichlet_total_concentration(
            raw, distribution_options);
    const auto alpha =
        mean * concentration + distribution_options.dirichlet_alpha_floor;
    std::vector<double> active_weights;
    active_weights.reserve(active.size());
    const auto weights = action.target_weights.to(torch::kFloat64).contiguous();
    for (const auto index : active) {
      active_weights.push_back(weights.index({index}).item<double>());
    }
    return action_distribution_detail::dirichlet_log_prob(
        alpha, torch::tensor(active_weights,
                             torch::TensorOptions().dtype(torch::kFloat64)));
  }

  [[nodiscard]] double
  entropy(const raw_policy_output_t &, const policy_input_t &,
          const action_distribution_evidence_t &evidence,
          const action_distribution_options_t &) const override {
    if (evidence.active_count <= 1) {
      return 0.0;
    }
    return action_distribution_detail::dirichlet_entropy(evidence.alpha_active);
  }

private:
  [[nodiscard]] action_sample_t
  make(const raw_policy_output_t &raw, const policy_input_t &input,
       const target_node_weights_adapter_options_t &adapter_options,
       const action_distribution_options_t &distribution_options,
       bool deterministic) const {
    validate_policy_input(input);
    const auto A = static_cast<std::int64_t>(input.node_ids.size());
    validate_raw_policy_output(raw, A);
    if (raw.action_distribution_id != distribution_id()) {
      throw std::runtime_error(
          "[action_distribution] raw output distribution mismatch");
    }
    const auto active =
        action_distribution_detail::active_indices_from_input(input);
    if (active.empty()) {
      throw std::runtime_error(
          "[action_distribution] no executable graph node is available");
    }

    action_distribution_evidence_t evidence{};
    evidence.action_distribution_id = distribution_id();
    evidence.active_node_indices = active;
    evidence.active_count = static_cast<std::int64_t>(active.size());
    evidence.value_estimate = raw.value_estimate;

    torch::Tensor active_weights;
    if (active.size() == 1) {
      active_weights =
          torch::ones({1}, torch::TensorOptions().dtype(torch::kFloat64));
      evidence.mean_active = active_weights;
      evidence.alpha_active = active_weights;
      evidence.total_concentration = 1.0;
      evidence.log_prob = 0.0;
      evidence.entropy = 0.0;
    } else {
      const auto logits =
          raw.node_weight_logits.to(torch::kFloat64).contiguous();
      evidence.mean_active =
          action_distribution_detail::masked_softmax_active(logits, active);
      evidence.total_concentration =
          action_distribution_detail::dirichlet_total_concentration(
              raw, distribution_options);
      evidence.alpha_active =
          evidence.mean_active * evidence.total_concentration +
          distribution_options.dirichlet_alpha_floor;
      active_weights =
          deterministic ? evidence.alpha_active / evidence.alpha_active.sum()
                        : action_distribution_detail::sample_dirichlet_active(
                              evidence.alpha_active, distribution_options);
      evidence.log_prob = action_distribution_detail::dirichlet_log_prob(
          evidence.alpha_active, active_weights);
      evidence.entropy =
          action_distribution_detail::dirichlet_entropy(evidence.alpha_active);
    }
    evidence.sampled_active_weights = active_weights.contiguous();
    evidence.target_node_weights =
        action_distribution_detail::scatter_active_weights(A, active,
                                                           active_weights);
    action_sample_t out{};
    out.action =
        action_distribution_detail::make_action_from_target_node_weights(
            input, evidence.target_node_weights, adapter_options);
    trainable_policy_detail::bind_action_distribution_evidence(out.action,
                                                               input, evidence);
    out.evidence = std::move(evidence);
    return out;
  }
};

class masked_logistic_normal_simplex_distribution_t final
    : public action_distribution_iface_t {
public:
  [[nodiscard]] std::string distribution_id() const override {
    return kMaskedLogisticNormalSimplexDistributionV1;
  }

  [[nodiscard]] action_sample_t
  sample(const raw_policy_output_t &raw, const policy_input_t &input,
         const target_node_weights_adapter_options_t &adapter_options,
         const action_distribution_options_t &distribution_options)
      const override {
    return make(raw, input, adapter_options, distribution_options,
                /*deterministic=*/false);
  }

  [[nodiscard]] action_sample_t deterministic_action(
      const raw_policy_output_t &raw, const policy_input_t &input,
      const target_node_weights_adapter_options_t &adapter_options,
      const action_distribution_options_t &distribution_options)
      const override {
    return make(raw, input, adapter_options, distribution_options,
                /*deterministic=*/true);
  }

  [[nodiscard]] double
  log_prob(const raw_policy_output_t &raw, const policy_input_t &input,
           const action_t &action, const action_distribution_evidence_t &,
           const action_distribution_options_t &distribution_options)
      const override {
    validate_policy_input(input);
    const auto A = static_cast<std::int64_t>(input.node_ids.size());
    validate_raw_policy_output(raw, A);
    if (raw.action_distribution_id != distribution_id()) {
      throw std::runtime_error(
          "[action_distribution] raw output distribution mismatch");
    }
    const auto active =
        action_distribution_detail::active_indices_from_input(input);
    if (active.empty()) {
      throw std::runtime_error(
          "[action_distribution] no executable graph node is available");
    }
    if (active.size() == 1) {
      return 0.0;
    }
    const auto params = logistic_params(raw, active, distribution_options);
    std::vector<double> active_weights;
    active_weights.reserve(active.size());
    const auto weights = action.target_weights.to(torch::kFloat64).contiguous();
    for (const auto index : active) {
      active_weights.push_back(weights.index({index}).item<double>());
    }
    return action_distribution_detail::logistic_normal_log_prob(
        torch::tensor(active_weights,
                      torch::TensorOptions().dtype(torch::kFloat64)),
        params.first, params.second);
  }

  [[nodiscard]] double
  entropy(const raw_policy_output_t &, const policy_input_t &,
          const action_distribution_evidence_t &evidence,
          const action_distribution_options_t &) const override {
    if (evidence.active_count <= 1) {
      return 0.0;
    }
    return action_distribution_detail::latent_normal_entropy(evidence.std_z);
  }

private:
  [[nodiscard]] std::pair<torch::Tensor, torch::Tensor> logistic_params(
      const raw_policy_output_t &raw, const std::vector<std::int64_t> &active,
      const action_distribution_options_t &distribution_options) const {
    const auto logits = raw.node_weight_logits.to(torch::kFloat64).contiguous();
    const auto K = static_cast<std::int64_t>(active.size());
    const double ref_logit = logits.index({active.back()}).item<double>();
    std::vector<double> mu_values(static_cast<std::size_t>(K - 1), 0.0);
    for (std::int64_t offset = 0; offset + 1 < K; ++offset) {
      mu_values[static_cast<std::size_t>(offset)] =
          logits.index({active[static_cast<std::size_t>(offset)]})
              .item<double>() -
          ref_logit;
    }
    const double std_value = action_distribution_detail::logistic_normal_std(
        raw, distribution_options);
    return {
        torch::tensor(mu_values, torch::TensorOptions().dtype(torch::kFloat64)),
        torch::full({K - 1}, std_value,
                    torch::TensorOptions().dtype(torch::kFloat64))};
  }

  [[nodiscard]] action_sample_t
  make(const raw_policy_output_t &raw, const policy_input_t &input,
       const target_node_weights_adapter_options_t &adapter_options,
       const action_distribution_options_t &distribution_options,
       bool deterministic) const {
    validate_policy_input(input);
    const auto A = static_cast<std::int64_t>(input.node_ids.size());
    validate_raw_policy_output(raw, A);
    if (raw.action_distribution_id != distribution_id()) {
      throw std::runtime_error(
          "[action_distribution] raw output distribution mismatch");
    }
    const auto active =
        action_distribution_detail::active_indices_from_input(input);
    if (active.empty()) {
      throw std::runtime_error(
          "[action_distribution] no executable graph node is available");
    }

    action_distribution_evidence_t evidence{};
    evidence.action_distribution_id = distribution_id();
    evidence.entropy_kind = "latent_normal_entropy";
    evidence.active_node_indices = active;
    evidence.active_count = static_cast<std::int64_t>(active.size());
    evidence.reference_node_index = active.back();
    evidence.value_estimate = raw.value_estimate;

    torch::Tensor active_weights;
    if (active.size() == 1) {
      active_weights =
          torch::ones({1}, torch::TensorOptions().dtype(torch::kFloat64));
      evidence.log_prob = 0.0;
      evidence.entropy = 0.0;
    } else {
      const auto params = logistic_params(raw, active, distribution_options);
      evidence.mu_z = params.first;
      evidence.std_z = params.second;
      evidence.latent_z_sample =
          deterministic
              ? evidence.mu_z
              : evidence.mu_z +
                    evidence.std_z *
                        action_distribution_detail::standard_normal_noise(
                            evidence.mu_z.size(0), distribution_options);
      std::vector<double> softmax_values(
          static_cast<std::size_t>(evidence.active_count), 0.0);
      double max_value = 0.0;
      for (std::int64_t offset = 0; offset + 1 < evidence.active_count;
           ++offset) {
        const double value =
            evidence.latent_z_sample.index({offset}).item<double>();
        softmax_values[static_cast<std::size_t>(offset)] = value;
        max_value = std::max(max_value, value);
      }
      double denom = std::exp(-max_value);
      for (std::int64_t offset = 0; offset + 1 < evidence.active_count;
           ++offset) {
        const double value =
            std::exp(evidence.latent_z_sample.index({offset}).item<double>() -
                     max_value);
        softmax_values[static_cast<std::size_t>(offset)] = value;
        denom += value;
      }
      softmax_values.back() = std::exp(-max_value);
      if (!(denom > 0.0) || !std::isfinite(denom)) {
        throw std::runtime_error(
            "[action_distribution] logistic-normal softmax denominator is "
            "invalid");
      }
      for (double &value : softmax_values) {
        value /= denom;
      }
      active_weights = torch::tensor(
          softmax_values, torch::TensorOptions().dtype(torch::kFloat64));
      evidence.log_prob = action_distribution_detail::logistic_normal_log_prob(
          active_weights, evidence.mu_z, evidence.std_z);
      evidence.entropy =
          action_distribution_detail::latent_normal_entropy(evidence.std_z);
    }
    evidence.sampled_active_weights = active_weights.contiguous();
    evidence.target_node_weights =
        action_distribution_detail::scatter_active_weights(A, active,
                                                           active_weights);
    action_sample_t out{};
    out.action =
        action_distribution_detail::make_action_from_target_node_weights(
            input, evidence.target_node_weights, adapter_options);
    trainable_policy_detail::bind_action_distribution_evidence(out.action,
                                                               input, evidence);
    out.evidence = std::move(evidence);
    return out;
  }
};

[[nodiscard]] inline const action_distribution_iface_t &
action_distribution_for_id(const std::string &distribution_id) {
  static const masked_dirichlet_simplex_distribution_t dirichlet{};
  static const masked_logistic_normal_simplex_distribution_t logistic_normal{};
  if (distribution_id == kMaskedDirichletSimplexDistributionV1) {
    return dirichlet;
  }
  if (distribution_id == kMaskedLogisticNormalSimplexDistributionV1) {
    return logistic_normal;
  }
  throw std::runtime_error("[action_distribution] unsupported distribution id");
}

[[nodiscard]] inline action_t adapt_raw_output_to_target_node_weights_action(
    const raw_policy_output_t &raw, const policy_input_t &input,
    const target_node_weights_adapter_options_t &options) {
  validate_policy_input(input);
  const auto A = static_cast<std::int64_t>(input.node_ids.size());
  validate_raw_policy_output(raw, A);
  const auto logits = raw.node_weight_logits.to(torch::kFloat64).contiguous();
  const auto active =
      action_distribution_detail::active_indices_from_input(input);
  if (active.empty()) {
    throw std::runtime_error(
        "[target_node_weights_adapter] no executable graph node is available");
  }
  const auto active_weights =
      action_distribution_detail::masked_softmax_active(logits, active);
  const auto weights = action_distribution_detail::scatter_active_weights(
      A, active, active_weights);
  return action_distribution_detail::make_action_from_target_node_weights(
      input, weights, options);
}

class trainable_policy_adapter_iface_t : public policy_adapter_iface_t {
public:
  [[nodiscard]] virtual std::string policy_artifact_digest() const = 0;
  [[nodiscard]] virtual std::string policy_input_schema_id() const = 0;
  [[nodiscard]] virtual std::string action_adapter_id() const = 0;
  [[nodiscard]] virtual std::string action_distribution_id() const = 0;
  [[nodiscard]] virtual std::string reward_contract_id() const = 0;
  [[nodiscard]] virtual policy_input_t
  make_input(const observation_t &observation) const = 0;
  [[nodiscard]] virtual raw_policy_output_t
  forward(const policy_input_t &input) = 0;

  [[nodiscard]] action_sample_t sample_action(
      const observation_t &observation,
      const action_distribution_options_t &distribution_options = {}) {
    auto input = make_input(observation);
    auto raw = forward(input);
    target_node_weights_adapter_options_t options{};
    options.policy_id = policy_id();
    options.method_id = action_adapter_id();
    options.policy_artifact_digest = policy_artifact_digest();
    options.decision_timestamp_ms = input.knowledge_timestamp_ms;
    return action_distribution_for_id(raw.action_distribution_id)
        .sample(raw, input, options, distribution_options);
  }

  [[nodiscard]] action_sample_t deterministic_action_sample(
      const observation_t &observation,
      const action_distribution_options_t &distribution_options = {}) {
    auto input = make_input(observation);
    auto raw = forward(input);
    target_node_weights_adapter_options_t options{};
    options.policy_id = policy_id();
    options.method_id = action_adapter_id();
    options.policy_artifact_digest = policy_artifact_digest();
    options.decision_timestamp_ms = input.knowledge_timestamp_ms;
    return action_distribution_for_id(raw.action_distribution_id)
        .deterministic_action(raw, input, options, distribution_options);
  }

  [[nodiscard]] action_t act(const observation_t &observation) override {
    return deterministic_action_sample(observation).action;
  }

  [[nodiscard]] action_t
  collect_action(const observation_t &observation) override {
    return sample_action(observation).action;
  }
};

struct graph_node_allocation_torch_policy_config_t {
  std::string policy_id{kGraphNodeAllocationPolicyId};
  std::string policy_artifact_digest{
      "graph_node_allocation_torch_policy_module_v0_digest"};
  std::string action_distribution_id{kMaskedDirichletSimplexDistributionV1};
  std::string graph_order_fingerprint{};
  std::string execution_profile_digest{"cajtucu.paper.fixture.digest"};
  std::string accounting_numeraire_node_id{};
  std::string causal_schedule_digest{};
  std::string snapshot_family_digest{"snapshot_family.fixture.digest"};
  std::string policy_checkpoint_path{};
  std::string module_state_path{};
  cuwacunu::wikimyei::policy::portfolio::graph_node_allocation::
      graph_node_allocation_net_spec_t net_spec{};
  std::uint64_t module_seed{17};
  torch::Tensor node_weight_logit_bias{};
  torch::Tensor action_distribution_params{};
  bool action_distribution_params_bound{false};
  double value_estimate_bias{0.0};
  torch::Tensor previous_target_weights{};
};

class graph_node_allocation_torch_policy_t final
    : public trainable_policy_adapter_iface_t {
public:
  explicit graph_node_allocation_torch_policy_t(
      graph_node_allocation_torch_policy_config_t config)
      : config_(std::move(config)) {
    if (detail::blank(config_.policy_id) ||
        detail::blank(config_.policy_artifact_digest)) {
      throw std::runtime_error(
          "[graph_node_allocation_torch_policy] policy id and artifact digest "
          "are required");
    }
    if (!action_distribution_id_supported(config_.action_distribution_id)) {
      throw std::runtime_error(
          "[graph_node_allocation_torch_policy] unsupported action "
          "distribution");
    }
    torch::manual_seed(static_cast<std::int64_t>(config_.module_seed));
    module_ = cuwacunu::wikimyei::policy::portfolio::graph_node_allocation::
        GraphNodeAllocationTorchPolicyModule(
            cuwacunu::wikimyei::policy::portfolio::graph_node_allocation::
                make_graph_node_allocation_torch_policy_options(
                    config_.net_spec));
    if (!detail::blank(config_.module_state_path)) {
      cuwacunu::wikimyei::policy::portfolio::graph_node_allocation::
          load_graph_node_allocation_torch_module_state(
              config_.module_state_path, module_,
              &config_.node_weight_logit_bias,
              &config_.action_distribution_params,
              &config_.action_distribution_params_bound,
              &config_.value_estimate_bias);
    }
    module_->eval();
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

  [[nodiscard]] std::string action_distribution_id() const override {
    return config_.action_distribution_id;
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
    const auto A = static_cast<std::int64_t>(input.node_ids.size());
    const auto active_mask =
        (input.valid_mask & input.tradable_mask & input.executable_mask)
            .contiguous();
    const auto module_out =
        module_->forward(input.node_features, input.global_features,
                         input.risk_features, active_mask);

    raw_policy_output_t out{};
    out.action_distribution_id = action_distribution_id();
    out.node_weight_logits =
        module_out.node_weight_logits.to(torch::kFloat64).contiguous();
    if (config_.node_weight_logit_bias.defined()) {
      const auto bias =
          config_.node_weight_logit_bias.to(torch::kFloat64).contiguous();
      if (bias.dim() != 1 || bias.size(0) != A ||
          !torch::isfinite(bias).all().item<bool>()) {
        throw std::runtime_error(
            "[graph_node_allocation_torch_policy] checkpoint logit bias must "
            "be finite [A]");
      }
      out.node_weight_logits = (out.node_weight_logits + bias).contiguous();
    }

    if (config_.action_distribution_params_bound) {
      out.action_distribution_params =
          config_.action_distribution_params.to(torch::kFloat64).contiguous();
    } else {
      out.action_distribution_params =
          module_out.action_distribution_params.to(torch::kFloat64)
              .contiguous();
    }
    const auto value =
        module_out.state_value.to(torch::kFloat64).reshape({-1}).index({0});
    out.value_estimate = value.item<double>() + config_.value_estimate_bias;
    validate_raw_policy_output(out, A);
    return out;
  }

  [[nodiscard]] std::string module_contract_id() const {
    return cuwacunu::wikimyei::policy::portfolio::graph_node_allocation::
        k_graph_node_allocation_torch_policy_module_v0;
  }

  [[nodiscard]] std::string architecture_digest() const {
    return module_->architecture_digest();
  }

private:
  graph_node_allocation_torch_policy_config_t config_{};
  cuwacunu::wikimyei::policy::portfolio::graph_node_allocation::
      GraphNodeAllocationTorchPolicyModule module_{nullptr};
};

struct fake_trainable_policy_config_t {
  std::string policy_id{kGraphNodeAllocationPolicyId};
  std::string policy_artifact_digest{"fake_trainable_policy_fixture_digest"};
  std::string action_distribution_id{kMaskedDirichletSimplexDistributionV1};
  std::string graph_order_fingerprint{};
  std::string execution_profile_digest{"cajtucu.paper.fixture.digest"};
  std::string accounting_numeraire_node_id{};
  std::string causal_schedule_digest{};
  std::string snapshot_family_digest{"snapshot_family.fixture.digest"};
  std::string policy_checkpoint_path{};
  torch::Tensor logits{};
  torch::Tensor action_distribution_params{};
  double value_estimate{0.0};
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

  [[nodiscard]] std::string action_distribution_id() const override {
    return config_.action_distribution_id;
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
    if (config_.action_distribution_params.defined()) {
      out.action_distribution_params =
          config_.action_distribution_params.to(torch::kFloat64).contiguous();
    }
    out.action_distribution_id = action_distribution_id();
    out.value_estimate = config_.value_estimate;
    return out;
  }

private:
  fake_trainable_policy_config_t config_{};
};

} // namespace cuwacunu::kikijyeba::environment
