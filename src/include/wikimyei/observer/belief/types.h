// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/observer/utility/feature_semantics.h"

namespace cuwacunu::wikimyei::observer::belief {

using anchor_key_t = std::string;
using node_id_t = std::string;
using timestamp_ms_t = std::int64_t;

enum class return_unit_t {
  normalized_log_return,
  log_return,
  arithmetic_return,
};

enum class coordinate_semantics_t {
  edge_log_return_lifted_potential,
  edge_log_price_lifted_potential,
  activity_lifted_intensity,
  unknown,
};

enum class gauge_policy_t {
  uniform_per_component,
};

enum class return_origin_t {
  base_relative_nodelift_projection,
};

[[nodiscard]] inline const char *return_unit_name(return_unit_t unit) {
  switch (unit) {
  case return_unit_t::normalized_log_return:
    return "normalized_log_return";
  case return_unit_t::log_return:
    return "log_return";
  case return_unit_t::arithmetic_return:
    return "arithmetic_return";
  }
  return "unknown";
}

struct BeliefDiagnostics {
  std::vector<std::string> notes{};
  std::vector<std::string> warnings{};
  std::vector<std::string> failures{};

  [[nodiscard]] bool ok() const { return failures.empty(); }
};

struct BasePolicy {
  node_id_t accounting_numeraire_id{};
  node_id_t settlement_asset_id{};
  node_id_t reserve_asset_id{};
  node_id_t projection_reference_node_id{};
};

// Runtime binding for the graph-node axis.
//
// A wikimyei dock describes that a tensor has a graph-node axis; this object
// binds concrete node identifiers to that axis for one graph order.
struct GraphNodeAxisBinding {
  std::string dock_name{};
  std::string axis_name{"G"};
  std::string coordinate_space{std::string(kGraphOrderKlineCoordinateSpace)};
  std::string graph_order_fingerprint{};
  std::vector<node_id_t> node_ids{}; // index -> node id
};

[[nodiscard]] inline GraphNodeAxisBinding make_graph_node_axis_binding(
    std::string graph_order_fingerprint, std::vector<node_id_t> node_ids,
    std::string dock_name = {},
    std::string coordinate_space = std::string(kGraphOrderKlineCoordinateSpace),
    std::string axis_name = "G") {
  GraphNodeAxisBinding out{};
  out.dock_name = std::move(dock_name);
  out.axis_name = std::move(axis_name);
  out.coordinate_space = std::move(coordinate_space);
  out.graph_order_fingerprint = std::move(graph_order_fingerprint);
  out.node_ids = std::move(node_ids);
  return out;
}

[[nodiscard]] inline bool
graph_node_axis_binding_has_payload(const GraphNodeAxisBinding &binding) {
  return !binding.graph_order_fingerprint.empty() ||
         !binding.node_ids.empty() || !binding.dock_name.empty();
}

struct NodeLiftPotentialBelief {
  anchor_key_t anchor_key{};
  timestamp_ms_t timestamp_ms{0};

  GraphNodeAxisBinding graph_node_axis{};
  std::string graph_order_fingerprint{};
  std::string source_feature_semantics_id{};
  std::string source_feature_semantics_fingerprint{};
  std::vector<node_id_t> graph_node_ids{}; // [G]

  int horizon{1};
  gauge_policy_t gauge_policy{gauge_policy_t::uniform_per_component};
  coordinate_semantics_t price_coord_semantics{
      coordinate_semantics_t::edge_log_return_lifted_potential};

  torch::Tensor log_weight{};  // [G,Df,Kc]
  torch::Tensor mu{};          // [G,Df,Kc]
  torch::Tensor sigma{};       // [G,Df,Kc]
  torch::Tensor active_mask{}; // [G,Df], bool

  torch::Tensor mixture_entropy{};        // optional [G,Df]
  torch::Tensor channel_disagreement{};   // optional [G,Df]
  torch::Tensor component_disagreement{}; // optional [G,Df]

  BeliefDiagnostics diagnostics{};
  bool valid{false};
};

// Single-anchor portfolio-ready belief.
//
// Shape convention:
//   G = graph node count from upstream inference
//   A = allocatable risky asset count
//   S = scenario count
//
// node_ids contains only risky allocatable assets. BasePolicy defines the
// accounting numeraire, settlement asset, reserve asset, and NodeLift
// projection reference. V1 normally binds all four to the same USD-like node,
// but the fields stay separate because they are different contracts.
struct AllocationBelief {
  anchor_key_t anchor_key{};
  timestamp_ms_t timestamp_ms{0};

  GraphNodeAxisBinding graph_node_axis{};
  std::string graph_order_fingerprint{};
  std::string source_feature_semantics_id{};
  std::string source_feature_semantics_fingerprint{};
  std::vector<node_id_t> graph_node_ids{}; // [G]

  std::vector<node_id_t> node_ids{};              // [A], risky assets only
  std::vector<std::int64_t> node_graph_indices{}; // [A], node -> graph slot

  BasePolicy base_policy{};
  std::optional<std::int64_t> projection_reference_graph_index{};

  int horizon{1};

  return_unit_t marginal_unit{return_unit_t::log_return};
  return_unit_t scenario_unit{return_unit_t::arithmetic_return};
  return_origin_t return_origin{
      return_origin_t::base_relative_nodelift_projection};

  torch::Tensor valid_mask{};    // [A], bool
  torch::Tensor tradable_mask{}; // [A], bool

  torch::Tensor expected_log_return{};        // [A]
  torch::Tensor expected_arithmetic_return{}; // [A]

  torch::Tensor marginal_variance{};   // [A], arithmetic-return variance
  torch::Tensor marginal_volatility{}; // [A]

  torch::Tensor covariance{};  // [A,A], scenario-derived
  torch::Tensor correlation{}; // [A,A]
  torch::Tensor scenarios{};   // [S,A], arithmetic returns

  torch::Tensor var_down{};               // [A]
  torch::Tensor cvar_down{};              // [A]
  torch::Tensor adverse_excursion_prob{}; // [A]
  torch::Tensor volatility{};             // [A]

  torch::Tensor mixture_entropy{};             // [A]
  torch::Tensor component_disagreement{};      // [A]
  torch::Tensor channel_disagreement{};        // [A]
  torch::Tensor surprise{};                    // [A], neutral until persistence
  torch::Tensor calibration_score{};           // [A], neutral until persistence
  torch::Tensor residual_quality_score{};      // optional [A], higher is better
  torch::Tensor projection_validation_score{}; // optional [A], higher is better

  torch::Tensor liquidity_score{};       // [A], [0,1]
  torch::Tensor linear_cost{};           // [A]
  torch::Tensor quadratic_impact{};      // [A]
  torch::Tensor capacity_weight_limit{}; // [A]

  torch::Tensor confidence{}; // [A], [0,1]

  bool projection_validation_required{true};
  bool projection_validated{false};
  bool live_capital_allowed{false};

  BeliefDiagnostics diagnostics{};
  bool valid{false};
};

struct AllocationBeliefBatch {
  std::vector<AllocationBelief> beliefs{};
};

struct CollatedAllocationBeliefBatch {
  std::vector<anchor_key_t> anchor_keys{};
  std::vector<timestamp_ms_t> timestamps_ms{};

  std::string graph_order_fingerprint{};
  GraphNodeAxisBinding graph_node_axis{};
  std::string source_feature_semantics_id{};
  std::string source_feature_semantics_fingerprint{};
  std::vector<node_id_t> graph_node_ids{};
  std::vector<node_id_t> node_ids{};
  std::vector<std::int64_t> node_graph_indices{};
  BasePolicy base_policy{};
  std::optional<std::int64_t> projection_reference_graph_index{};

  int horizon{1};
  return_unit_t marginal_unit{return_unit_t::log_return};
  return_unit_t scenario_unit{return_unit_t::arithmetic_return};
  return_origin_t return_origin{
      return_origin_t::base_relative_nodelift_projection};

  torch::Tensor belief_valid{}; // [B], bool

  torch::Tensor valid_mask{};    // [B,A], bool
  torch::Tensor tradable_mask{}; // [B,A], bool

  torch::Tensor expected_log_return{};        // [B,A]
  torch::Tensor expected_arithmetic_return{}; // [B,A]

  torch::Tensor marginal_variance{};   // [B,A]
  torch::Tensor marginal_volatility{}; // [B,A]

  torch::Tensor covariance{};  // [B,A,A]
  torch::Tensor correlation{}; // [B,A,A]
  torch::Tensor scenarios{};   // [B,S,A]

  torch::Tensor var_down{};               // optional [B,A]
  torch::Tensor cvar_down{};              // optional [B,A]
  torch::Tensor adverse_excursion_prob{}; // optional [B,A]
  torch::Tensor volatility{};             // optional [B,A]

  torch::Tensor mixture_entropy{};             // optional [B,A]
  torch::Tensor component_disagreement{};      // optional [B,A]
  torch::Tensor channel_disagreement{};        // optional [B,A]
  torch::Tensor surprise{};                    // optional [B,A]
  torch::Tensor calibration_score{};           // optional [B,A]
  torch::Tensor residual_quality_score{};      // optional [B,A]
  torch::Tensor projection_validation_score{}; // optional [B,A]

  torch::Tensor liquidity_score{};       // optional [B,A]
  torch::Tensor linear_cost{};           // optional [B,A]
  torch::Tensor quadratic_impact{};      // optional [B,A]
  torch::Tensor capacity_weight_limit{}; // optional [B,A]

  torch::Tensor confidence{}; // [B,A]

  torch::Tensor projection_validation_required{}; // [B], bool
  torch::Tensor projection_validated{};           // [B], bool
  torch::Tensor live_capital_allowed{};           // [B], bool

  std::vector<BeliefDiagnostics> diagnostics{};
};

namespace detail {

inline void require_unique_nonempty(const std::vector<node_id_t> &ids,
                                    const char *field_name) {
  std::unordered_set<std::string> seen;
  seen.reserve(ids.size());
  for (const auto &id : ids) {
    if (id.empty()) {
      throw std::runtime_error(std::string("[AllocationBelief] empty ") +
                               field_name + " entry");
    }
    if (!seen.insert(id).second) {
      throw std::runtime_error(std::string("[AllocationBelief] duplicate ") +
                               field_name + " entry: " + id);
    }
  }
}

[[nodiscard]] inline std::optional<std::int64_t>
find_graph_node_index(const std::vector<node_id_t> &graph_node_ids,
                      const node_id_t &node_id) {
  for (std::size_t i = 0; i < graph_node_ids.size(); ++i) {
    if (graph_node_ids[i] == node_id) {
      return static_cast<std::int64_t>(i);
    }
  }
  return std::nullopt;
}

inline void
validate_graph_node_axis_binding(const GraphNodeAxisBinding &binding,
                                 const char *owner) {
  if (binding.axis_name.empty()) {
    throw std::runtime_error(std::string(owner) +
                             " graph node axis_name is required");
  }
  if (binding.coordinate_space.empty()) {
    throw std::runtime_error(std::string(owner) +
                             " graph node coordinate_space is required");
  }
  if (binding.graph_order_fingerprint.empty()) {
    throw std::runtime_error(std::string(owner) +
                             " graph_order_fingerprint is required");
  }
  require_unique_nonempty(binding.node_ids, "graph_node_axis.node_ids");
}

[[nodiscard]] inline GraphNodeAxisBinding
effective_graph_node_axis_binding(const GraphNodeAxisBinding &binding,
                                  const std::string &graph_order_fingerprint,
                                  const std::vector<node_id_t> &graph_node_ids,
                                  const char *owner) {
  auto effective = graph_node_axis_binding_has_payload(binding)
                       ? binding
                       : make_graph_node_axis_binding(graph_order_fingerprint,
                                                      graph_node_ids);
  validate_graph_node_axis_binding(effective, owner);
  if (!graph_order_fingerprint.empty() &&
      effective.graph_order_fingerprint != graph_order_fingerprint) {
    throw std::runtime_error(std::string(owner) +
                             " graph_node_axis fingerprint mismatch");
  }
  if (!graph_node_ids.empty() && effective.node_ids != graph_node_ids) {
    throw std::runtime_error(std::string(owner) +
                             " graph_node_axis node_ids mismatch");
  }
  return effective;
}

inline void require_base_policy(const BasePolicy &policy) {
  if (policy.accounting_numeraire_id.empty() ||
      policy.settlement_asset_id.empty() || policy.reserve_asset_id.empty() ||
      policy.projection_reference_node_id.empty()) {
    throw std::runtime_error(
        "[AllocationBelief] BasePolicy fields are required");
  }
}

inline void require_vector_shape(const torch::Tensor &tensor, std::int64_t A,
                                 const char *name, bool required) {
  if (!tensor.defined()) {
    if (required) {
      throw std::runtime_error(std::string("[AllocationBelief] missing ") +
                               name);
    }
    return;
  }
  TORCH_CHECK(tensor.dim() == 1 && tensor.size(0) == A, "[AllocationBelief] ",
              name, " must be [A]");
}

inline void require_bool_vector_shape(const torch::Tensor &tensor,
                                      std::int64_t A, const char *name,
                                      bool required) {
  require_vector_shape(tensor, A, name, required);
  if (tensor.defined()) {
    TORCH_CHECK(tensor.scalar_type() == torch::kBool, "[AllocationBelief] ",
                name, " must be bool");
  }
}

inline void require_matrix_shape(const torch::Tensor &tensor, std::int64_t A,
                                 const char *name, bool required) {
  if (!tensor.defined()) {
    if (required) {
      throw std::runtime_error(std::string("[AllocationBelief] missing ") +
                               name);
    }
    return;
  }
  TORCH_CHECK(tensor.dim() == 2 && tensor.size(0) == A && tensor.size(1) == A,
              "[AllocationBelief] ", name, " must be [A,A]");
}

inline void require_scenario_shape(const torch::Tensor &tensor, std::int64_t A,
                                   bool required) {
  if (!tensor.defined()) {
    if (required) {
      throw std::runtime_error("[AllocationBelief] missing scenarios");
    }
    return;
  }
  TORCH_CHECK(tensor.dim() == 2 && tensor.size(1) == A,
              "[AllocationBelief] scenarios must be [S,A]");
}

inline void
require_batch_metadata_compatible(const AllocationBelief &expected,
                                  const AllocationBelief &candidate) {
  const auto expected_axis = effective_graph_node_axis_binding(
      expected.graph_node_axis, expected.graph_order_fingerprint,
      expected.graph_node_ids, "[AllocationBeliefBatch]");
  const auto candidate_axis = effective_graph_node_axis_binding(
      candidate.graph_node_axis, candidate.graph_order_fingerprint,
      candidate.graph_node_ids, "[AllocationBeliefBatch]");
  if (candidate.graph_order_fingerprint != expected.graph_order_fingerprint) {
    throw std::runtime_error("[AllocationBeliefBatch] graph_order_fingerprint "
                             "mismatch during collate");
  }
  if (candidate_axis.dock_name != expected_axis.dock_name ||
      candidate_axis.axis_name != expected_axis.axis_name ||
      candidate_axis.coordinate_space != expected_axis.coordinate_space ||
      candidate_axis.graph_order_fingerprint !=
          expected_axis.graph_order_fingerprint ||
      candidate_axis.node_ids != expected_axis.node_ids) {
    throw std::runtime_error("[AllocationBeliefBatch] graph node axis binding "
                             "mismatch during collate");
  }
  if (candidate.source_feature_semantics_id !=
          expected.source_feature_semantics_id ||
      candidate.source_feature_semantics_fingerprint !=
          expected.source_feature_semantics_fingerprint) {
    throw std::runtime_error("[AllocationBeliefBatch] feature semantics "
                             "mismatch during collate");
  }
  if (candidate.graph_node_ids != expected.graph_node_ids ||
      candidate.node_ids != expected.node_ids ||
      candidate.node_graph_indices != expected.node_graph_indices) {
    throw std::runtime_error(
        "[AllocationBeliefBatch] node universe mismatch during collate");
  }
  if (candidate.base_policy.accounting_numeraire_id !=
          expected.base_policy.accounting_numeraire_id ||
      candidate.base_policy.settlement_asset_id !=
          expected.base_policy.settlement_asset_id ||
      candidate.base_policy.reserve_asset_id !=
          expected.base_policy.reserve_asset_id ||
      candidate.base_policy.projection_reference_node_id !=
          expected.base_policy.projection_reference_node_id ||
      candidate.projection_reference_graph_index !=
          expected.projection_reference_graph_index) {
    throw std::runtime_error(
        "[AllocationBeliefBatch] base policy mismatch during collate");
  }
  if (candidate.horizon != expected.horizon ||
      candidate.marginal_unit != expected.marginal_unit ||
      candidate.scenario_unit != expected.scenario_unit ||
      candidate.return_origin != expected.return_origin) {
    throw std::runtime_error(
        "[AllocationBeliefBatch] unit/horizon mismatch during "
        "collate");
  }
  if (candidate.scenarios.size(0) != expected.scenarios.size(0)) {
    throw std::runtime_error(
        "[AllocationBeliefBatch] scenario count mismatch during collate");
  }
}

[[nodiscard]] inline torch::Tensor
stack_required_tensor(const std::vector<AllocationBelief> &states,
                      torch::Tensor AllocationBelief::*member,
                      const char *name) {
  std::vector<torch::Tensor> tensors;
  tensors.reserve(states.size());
  for (const auto &state : states) {
    const auto &tensor = state.*member;
    TORCH_CHECK(tensor.defined(), "[AllocationBeliefBatch] missing ", name,
                " during collate");
    tensors.push_back(tensor);
  }
  return torch::stack(tensors, /*dim=*/0);
}

[[nodiscard]] inline torch::Tensor
stack_optional_tensor(const std::vector<AllocationBelief> &states,
                      torch::Tensor AllocationBelief::*member,
                      const char *name) {
  bool any_defined = false;
  bool all_defined = true;
  for (const auto &state : states) {
    const bool defined = (state.*member).defined();
    any_defined = any_defined || defined;
    all_defined = all_defined && defined;
  }
  if (!any_defined) {
    return {};
  }
  TORCH_CHECK(all_defined, "[AllocationBeliefBatch] mixed optional ", name,
              " presence during collate");
  return stack_required_tensor(states, member, name);
}

} // namespace detail

[[nodiscard]] inline std::int64_t asset_count(const AllocationBelief &belief) {
  return static_cast<std::int64_t>(belief.node_ids.size());
}

[[nodiscard]] inline std::int64_t
scenario_count(const AllocationBelief &belief) {
  return belief.scenarios.defined() ? belief.scenarios.size(0) : 0;
}

inline void
validate_allocation_belief_contract(const AllocationBelief &belief,
                                    bool require_portfolio_fields = true) {
  if (belief.horizon != 1) {
    throw std::runtime_error("[AllocationBelief] V1 requires horizon=1");
  }
  if (belief.scenario_unit != return_unit_t::arithmetic_return) {
    throw std::runtime_error(
        "[AllocationBelief] V1 requires arithmetic-return scenarios");
  }
  if (belief.return_origin !=
      return_origin_t::base_relative_nodelift_projection) {
    throw std::runtime_error("[AllocationBelief] V1 requires "
                             "base-relative NodeLift projection returns");
  }
  if (belief.source_feature_semantics_id.empty() ||
      belief.source_feature_semantics_fingerprint.empty()) {
    throw std::runtime_error(
        "[AllocationBelief] source feature semantics are required");
  }
  detail::require_base_policy(belief.base_policy);
  const auto graph_axis = detail::effective_graph_node_axis_binding(
      belief.graph_node_axis, belief.graph_order_fingerprint,
      belief.graph_node_ids, "[AllocationBelief]");
  detail::require_unique_nonempty(belief.node_ids, "node_ids");
  const auto graph_contains = [&](const node_id_t &node_id) {
    return detail::find_graph_node_index(graph_axis.node_ids, node_id)
        .has_value();
  };
  if (!graph_contains(belief.base_policy.reserve_asset_id) ||
      !graph_contains(belief.base_policy.projection_reference_node_id)) {
    throw std::runtime_error(
        "[AllocationBelief] reserve/projection reference nodes must appear "
        "in graph_node_ids");
  }
  if (belief.node_graph_indices.size() != belief.node_ids.size()) {
    throw std::runtime_error(
        "[AllocationBelief] node_graph_indices size must match node_ids");
  }
  for (std::size_t i = 0; i < belief.node_ids.size(); ++i) {
    if (belief.node_ids[i] == belief.base_policy.reserve_asset_id ||
        belief.node_ids[i] == belief.base_policy.projection_reference_node_id) {
      throw std::runtime_error(
          "[AllocationBelief] base/reserve node must not appear in risky "
          "node_ids");
    }
    const auto graph_index = belief.node_graph_indices[i];
    if (graph_index < 0 ||
        graph_index >= static_cast<std::int64_t>(graph_axis.node_ids.size())) {
      throw std::runtime_error(
          "[AllocationBelief] node_graph_indices out of range");
    }
  }
  if (belief.projection_reference_graph_index.has_value() &&
      (*belief.projection_reference_graph_index < 0 ||
       *belief.projection_reference_graph_index >=
           static_cast<std::int64_t>(graph_axis.node_ids.size()))) {
    throw std::runtime_error("[AllocationBelief] "
                             "projection_reference_graph_index out of range");
  }
  if (belief.projection_reference_graph_index.has_value() &&
      graph_axis.node_ids[static_cast<std::size_t>(
          *belief.projection_reference_graph_index)] !=
          belief.base_policy.projection_reference_node_id) {
    throw std::runtime_error(
        "[AllocationBelief] projection_reference_graph_index does not "
        "match BasePolicy projection_reference_node_id");
  }

  const auto A = asset_count(belief);
  const bool required = require_portfolio_fields || belief.valid;
  detail::require_bool_vector_shape(belief.valid_mask, A, "valid_mask",
                                    required);
  detail::require_bool_vector_shape(belief.tradable_mask, A, "tradable_mask",
                                    required);
  detail::require_vector_shape(belief.expected_log_return, A,
                               "expected_log_return", required);
  detail::require_vector_shape(belief.expected_arithmetic_return, A,
                               "expected_arithmetic_return", required);
  detail::require_vector_shape(belief.marginal_variance, A, "marginal_variance",
                               required);
  detail::require_vector_shape(belief.marginal_volatility, A,
                               "marginal_volatility", required);
  detail::require_matrix_shape(belief.covariance, A, "covariance", required);
  detail::require_matrix_shape(belief.correlation, A, "correlation", required);
  detail::require_scenario_shape(belief.scenarios, A, required);
  detail::require_vector_shape(belief.confidence, A, "confidence", required);
  detail::require_vector_shape(belief.residual_quality_score, A,
                               "residual_quality_score", false);
  detail::require_vector_shape(belief.projection_validation_score, A,
                               "projection_validation_score", false);
  detail::require_vector_shape(belief.liquidity_score, A, "liquidity_score",
                               false);
  detail::require_vector_shape(belief.linear_cost, A, "linear_cost", false);
  detail::require_vector_shape(belief.quadratic_impact, A, "quadratic_impact",
                               false);
  detail::require_vector_shape(belief.capacity_weight_limit, A,
                               "capacity_weight_limit", false);

  if (belief.valid && !belief.diagnostics.ok()) {
    throw std::runtime_error(
        "[AllocationBelief] valid state cannot carry diagnostic failures");
  }
  if (belief.live_capital_allowed && !belief.projection_validated) {
    throw std::runtime_error(
        "[AllocationBelief] live capital requires projection validation");
  }
  if (belief.live_capital_allowed && !belief.projection_validation_required) {
    throw std::runtime_error("[AllocationBelief] live capital cannot disable "
                             "projection validation");
  }
}

[[nodiscard]] inline const AllocationBelief &
select_decision_anchor(const AllocationBeliefBatch &batch,
                       const anchor_key_t &decision_anchor_key) {
  for (const auto &state : batch.beliefs) {
    if (state.anchor_key == decision_anchor_key) {
      return state;
    }
  }
  throw std::runtime_error(
      "[AllocationBeliefBatch] decision anchor not found: " +
      decision_anchor_key);
}

[[nodiscard]] inline CollatedAllocationBeliefBatch
collate_allocation_belief_batch(const AllocationBeliefBatch &batch,
                                bool require_portfolio_fields = true) {
  if (batch.beliefs.empty()) {
    throw std::runtime_error(
        "[AllocationBeliefBatch] cannot collate empty batch");
  }
  const auto &first = batch.beliefs.front();
  validate_allocation_belief_contract(first, require_portfolio_fields);

  CollatedAllocationBeliefBatch out{};
  out.anchor_keys.reserve(batch.beliefs.size());
  out.timestamps_ms.reserve(batch.beliefs.size());
  out.diagnostics.reserve(batch.beliefs.size());
  std::vector<torch::Tensor> belief_valid_tensors;
  belief_valid_tensors.reserve(batch.beliefs.size());
  std::vector<torch::Tensor> projection_validation_required_tensors;
  projection_validation_required_tensors.reserve(batch.beliefs.size());
  std::vector<torch::Tensor> projection_validated_tensors;
  projection_validated_tensors.reserve(batch.beliefs.size());
  std::vector<torch::Tensor> live_capital_allowed_tensors;
  live_capital_allowed_tensors.reserve(batch.beliefs.size());

  for (const auto &state : batch.beliefs) {
    validate_allocation_belief_contract(state, require_portfolio_fields);
    detail::require_batch_metadata_compatible(first, state);
    out.anchor_keys.push_back(state.anchor_key);
    out.timestamps_ms.push_back(state.timestamp_ms);
    out.diagnostics.push_back(state.diagnostics);
    belief_valid_tensors.push_back(
        torch::tensor(state.valid, torch::TensorOptions().dtype(torch::kBool)));
    projection_validation_required_tensors.push_back(
        torch::tensor(state.projection_validation_required,
                      torch::TensorOptions().dtype(torch::kBool)));
    projection_validated_tensors.push_back(
        torch::tensor(state.projection_validated,
                      torch::TensorOptions().dtype(torch::kBool)));
    live_capital_allowed_tensors.push_back(
        torch::tensor(state.live_capital_allowed,
                      torch::TensorOptions().dtype(torch::kBool)));
  }

  out.graph_order_fingerprint = first.graph_order_fingerprint;
  out.graph_node_axis = detail::effective_graph_node_axis_binding(
      first.graph_node_axis, first.graph_order_fingerprint,
      first.graph_node_ids, "[AllocationBeliefBatch]");
  out.source_feature_semantics_id = first.source_feature_semantics_id;
  out.source_feature_semantics_fingerprint =
      first.source_feature_semantics_fingerprint;
  out.graph_node_ids = first.graph_node_ids;
  out.node_ids = first.node_ids;
  out.node_graph_indices = first.node_graph_indices;
  out.base_policy = first.base_policy;
  out.projection_reference_graph_index = first.projection_reference_graph_index;
  out.horizon = first.horizon;
  out.marginal_unit = first.marginal_unit;
  out.scenario_unit = first.scenario_unit;
  out.return_origin = first.return_origin;
  out.belief_valid = torch::stack(belief_valid_tensors, /*dim=*/0);
  out.projection_validation_required =
      torch::stack(projection_validation_required_tensors, /*dim=*/0);
  out.projection_validated =
      torch::stack(projection_validated_tensors, /*dim=*/0);
  out.live_capital_allowed =
      torch::stack(live_capital_allowed_tensors, /*dim=*/0);

  out.valid_mask = detail::stack_required_tensor(
      batch.beliefs, &AllocationBelief::valid_mask, "valid_mask");
  out.tradable_mask = detail::stack_required_tensor(
      batch.beliefs, &AllocationBelief::tradable_mask, "tradable_mask");
  out.expected_log_return = detail::stack_required_tensor(
      batch.beliefs, &AllocationBelief::expected_log_return,
      "expected_log_return");
  out.expected_arithmetic_return = detail::stack_required_tensor(
      batch.beliefs, &AllocationBelief::expected_arithmetic_return,
      "expected_arithmetic_return");
  out.marginal_variance = detail::stack_required_tensor(
      batch.beliefs, &AllocationBelief::marginal_variance, "marginal_variance");
  out.marginal_volatility = detail::stack_required_tensor(
      batch.beliefs, &AllocationBelief::marginal_volatility,
      "marginal_volatility");
  out.covariance = detail::stack_required_tensor(
      batch.beliefs, &AllocationBelief::covariance, "covariance");
  out.correlation = detail::stack_required_tensor(
      batch.beliefs, &AllocationBelief::correlation, "correlation");
  out.scenarios = detail::stack_required_tensor(
      batch.beliefs, &AllocationBelief::scenarios, "scenarios");
  out.confidence = detail::stack_required_tensor(
      batch.beliefs, &AllocationBelief::confidence, "confidence");

  out.var_down = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::var_down, "var_down");
  out.cvar_down = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::cvar_down, "cvar_down");
  out.adverse_excursion_prob = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::adverse_excursion_prob,
      "adverse_excursion_prob");
  out.volatility = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::volatility, "volatility");
  out.mixture_entropy = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::mixture_entropy, "mixture_entropy");
  out.component_disagreement = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::component_disagreement,
      "component_disagreement");
  out.channel_disagreement = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::channel_disagreement,
      "channel_disagreement");
  out.surprise = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::surprise, "surprise");
  out.calibration_score = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::calibration_score, "calibration_score");
  out.residual_quality_score = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::residual_quality_score,
      "residual_quality_score");
  out.projection_validation_score = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::projection_validation_score,
      "projection_validation_score");
  out.liquidity_score = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::liquidity_score, "liquidity_score");
  out.linear_cost = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::linear_cost, "linear_cost");
  out.quadratic_impact = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::quadratic_impact, "quadratic_impact");
  out.capacity_weight_limit = detail::stack_optional_tensor(
      batch.beliefs, &AllocationBelief::capacity_weight_limit,
      "capacity_weight_limit");

  return out;
}

} // namespace cuwacunu::wikimyei::observer::belief
