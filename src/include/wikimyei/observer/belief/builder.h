// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/inference/expected_value/mdn/mixture_density_network_types.h"
#include "wikimyei/observer/belief/types.h"
#include "wikimyei/observer/utility/calibration.h"
#include "wikimyei/observer/utility/channel_consensus.h"
#include "wikimyei/observer/utility/confidence.h"
#include "wikimyei/observer/utility/data_quality.h"
#include "wikimyei/observer/utility/feature_semantics.h"
#include "wikimyei/observer/utility/flow_liquidity.h"
#include "wikimyei/observer/utility/mdn_moments.h"
#include "wikimyei/observer/utility/nodelift_potential_surface.h"
#include "wikimyei/observer/utility/nodelift_return_projection.h"
#include "wikimyei/observer/utility/range_risk.h"
#include "wikimyei/observer/utility/scenario_bank.h"
#include "wikimyei/observer/utility/surprise.h"
#include "wikimyei/observer/utility/tail_risk.h"
#include "wikimyei/observer/utility/volatility.h"

namespace cuwacunu::wikimyei::observer::belief {

namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;

struct allocation_belief_builder_options_t {
  std::int64_t anchor_slot{0};
  anchor_key_t anchor_key{};
  timestamp_ms_t timestamp_ms{0};

  std::string graph_order_fingerprint{};
  GraphNodeAxisBinding graph_node_axis{};
  std::vector<node_id_t> graph_node_ids{};
  std::vector<node_id_t> node_ids{};              // [A], risky assets
  std::vector<std::int64_t> node_graph_indices{}; // [A]
  BasePolicy base_policy{};
  std::optional<std::int64_t> projection_reference_graph_index{};

  torch::Tensor channel_mask{}; // optional [B,G,C], bool
  // Ordered as risky node_graph_indices followed by projection_reference.
  torch::Tensor empirical_potential_correlation{}; // [A+1,A+1]
  torch::Tensor tradable_mask{};                   // optional [A], bool
  torch::Tensor realized_variance{};               // optional [A]
  torch::Tensor data_quality_score{};              // optional [A]
  torch::Tensor linear_cost{};                     // optional [A]
  torch::Tensor quadratic_impact{};                // optional [A]
  torch::Tensor capacity_weight_limit{};           // optional [A]

  nodelift_potential_surface_options_t surface_options{};
  range_risk_options_t range_options{};
  nodelift_return_projection_options_t projection_options{};
  scenario_bank_options_t scenario_bank_options{};
  volatility_options_t volatility_options{};
  flow_liquidity_options_t flow_liquidity_options{};

  double channel_disagreement_confidence_scale{1.0};
};

struct allocation_belief_build_result_t {
  AllocationBelief allocation_belief{};
  NodeLiftPotentialBelief nodelift_potential_belief{};
  channel_consensus_t channel_consensus{};
  nodelift_potential_surface_t potential_surface{};
  nodelift_return_projection_t return_projection{};
  scenario_bank_t scenario_bank{};
  tail_risk_t tail_risk{};
  range_risk_t range_risk{};
  flow_liquidity_t flow_liquidity{};
  volatility_t volatility{};
};

struct allocation_belief_batch_builder_options_t {
  allocation_belief_builder_options_t common{};
  std::vector<anchor_key_t> anchor_keys{};
  std::vector<timestamp_ms_t> timestamps_ms{};
};

namespace detail {

[[nodiscard]] inline torch::Tensor
graph_index_tensor(const std::vector<std::int64_t> &indices,
                   const torch::Device &device) {
  TORCH_CHECK(!indices.empty(),
              "[allocation_belief_builder] node universe is empty");
  return torch::tensor(
      indices, torch::TensorOptions().dtype(torch::kInt64).device(device));
}

[[nodiscard]] inline torch::Tensor
select_anchor_assets(const torch::Tensor &tensor, std::int64_t anchor_slot,
                     const torch::Tensor &graph_index, const char *name) {
  TORCH_CHECK(tensor.defined(), "[allocation_belief_builder] missing ", name);
  TORCH_CHECK(tensor.dim() >= 2, "[allocation_belief_builder] ", name,
              " must have [B,N,...] shape");
  TORCH_CHECK(anchor_slot >= 0 && anchor_slot < tensor.size(0),
              "[allocation_belief_builder] anchor_slot out of range for ",
              name);
  return tensor.select(/*dim=*/0, anchor_slot)
      .index_select(/*dim=*/0, graph_index);
}

[[nodiscard]] inline torch::Tensor vector_or(const torch::Tensor &tensor,
                                             std::int64_t A,
                                             torch::TensorOptions options,
                                             double value, const char *name) {
  if (tensor.defined()) {
    TORCH_CHECK(tensor.dim() == 1 && tensor.size(0) == A,
                "[allocation_belief_builder] ", name, " must be [A]");
    return tensor.to(options);
  }
  return torch::full({A}, value, options);
}

[[nodiscard]] inline torch::Tensor
bool_vector_or(const torch::Tensor &tensor, std::int64_t A,
               const torch::Device &device, bool value, const char *name) {
  if (tensor.defined()) {
    TORCH_CHECK(tensor.dim() == 1 && tensor.size(0) == A,
                "[allocation_belief_builder] ", name, " must be [A]");
    return tensor.to(torch::TensorOptions().dtype(torch::kBool).device(device));
  }
  return torch::full({A}, value,
                     torch::TensorOptions().dtype(torch::kBool).device(device));
}

[[nodiscard]] inline torch::Tensor
mixture_disagreement(const torch::Tensor &log_weight, const torch::Tensor &mu,
                     const torch::Tensor &mean) {
  auto pi = log_weight.exp();
  return (pi * (mu - mean.unsqueeze(-1)).pow(2)).sum(/*dim=*/-1);
}

[[nodiscard]] inline torch::Tensor
mixture_entropy(const torch::Tensor &log_weight) {
  auto pi = log_weight.exp();
  return -(pi * log_weight).sum(/*dim=*/-1);
}

[[nodiscard]] inline torch::Tensor
normalized_mixture_entropy(const torch::Tensor &log_weight) {
  auto entropy = mixture_entropy(log_weight);
  const auto Kc = log_weight.size(-1);
  if (Kc <= 1) {
    return torch::zeros_like(entropy);
  }
  return (entropy / std::log(static_cast<double>(Kc))).clamp(0.0, 1.0);
}

} // namespace detail

[[nodiscard]] inline allocation_belief_build_result_t
build_single_anchor_allocation_belief(
    const mdn::MdnOut &out,
    const allocation_belief_builder_options_t &options) {
  const auto quality = check_mdn_output(out);
  TORCH_CHECK(quality.valid, "[allocation_belief_builder] invalid MDN output");
  TORCH_CHECK(options.anchor_slot >= 0 &&
                  options.anchor_slot < out.log_pi.size(0),
              "[allocation_belief_builder] anchor_slot out of range");
  TORCH_CHECK(
      options.node_ids.size() == options.node_graph_indices.size(),
      "[allocation_belief_builder] node_ids/node_graph_indices mismatch");
  TORCH_CHECK(!options.node_ids.empty(),
              "[allocation_belief_builder] risky node universe is empty");
  const auto A = static_cast<std::int64_t>(options.node_ids.size());
  detail::require_base_policy(options.base_policy);
  const auto graph_axis = detail::effective_graph_node_axis_binding(
      options.graph_node_axis, options.graph_order_fingerprint,
      options.graph_node_ids, "[allocation_belief_builder]");
  const auto projection_reference_graph_index =
      options.projection_reference_graph_index.has_value()
          ? options.projection_reference_graph_index
          : detail::find_graph_node_index(
                graph_axis.node_ids,
                options.base_policy.projection_reference_node_id);
  TORCH_CHECK(
      projection_reference_graph_index.has_value(),
      "[allocation_belief_builder] projection reference must be provided "
      "by graph index or found in graph_node_ids");
  TORCH_CHECK(
      *projection_reference_graph_index >= 0 &&
          *projection_reference_graph_index <
              static_cast<std::int64_t>(graph_axis.node_ids.size()) &&
          graph_axis.node_ids[static_cast<std::size_t>(
              *projection_reference_graph_index)] ==
              options.base_policy.projection_reference_node_id,
      "[allocation_belief_builder] projection_reference_graph_index does "
      "not match projection_reference_node_id");
  TORCH_CHECK(detail::find_graph_node_index(
                  graph_axis.node_ids, options.base_policy.reserve_asset_id)
                  .has_value(),
              "[allocation_belief_builder] reserve_asset_id must appear in "
              "graph_node_ids");
  TORCH_CHECK(
      options.empirical_potential_correlation.defined() &&
          options.empirical_potential_correlation.dim() == 2 &&
          options.empirical_potential_correlation.size(0) == A + 1 &&
          options.empirical_potential_correlation.size(1) == A + 1,
      "[allocation_belief_builder] empirical_potential_correlation must be "
      "[A+1,A+1] ordered as risky nodes then projection reference");

  auto consensus =
      compute_uniform_valid_channel_consensus(out, options.channel_mask);
  auto surface = from_channel_consensus(consensus, options.surface_options);
  auto graph_index = detail::graph_index_tensor(options.node_graph_indices,
                                                surface.log_weight.device());
  auto projected = project_single_anchor(
      surface, options.anchor_slot, options.node_graph_indices,
      *projection_reference_graph_index,
      options.empirical_potential_correlation, options.projection_options);
  auto scenarios = make_stress_bank(projected.arithmetic_return_scenarios,
                                    options.scenario_bank_options);
  auto tails = compute_node_tail_risk(projected.arithmetic_return_scenarios);
  auto ranges = compute_range_risk(consensus, options.node_graph_indices,
                                   *projection_reference_graph_index,
                                   options.range_options);
  auto flow = compute_flow_liquidity(consensus, options.flow_liquidity_options);

  auto tensor_options =
      torch::TensorOptions()
          .dtype(torch::kFloat64)
          .device(projected.arithmetic_return_scenarios.device());
  auto valid_mask = projected.active_mask.to(
      torch::TensorOptions()
          .dtype(torch::kBool)
          .device(projected.arithmetic_return_scenarios.device()));
  auto tradable_mask = detail::bool_vector_or(
      options.tradable_mask, A, projected.arithmetic_return_scenarios.device(),
      true, "tradable_mask");
  auto expected_log = projected.expected_log_return.to(tensor_options);
  auto expected_arithmetic =
      projected.expected_arithmetic_return.to(tensor_options);
  auto marginal_variance = projected.marginal_variance.to(tensor_options);
  auto marginal_volatility = projected.marginal_volatility.to(tensor_options);

  auto realized_variance =
      options.realized_variance.defined()
          ? detail::vector_or(options.realized_variance, A, tensor_options, 0.0,
                              "realized_variance")
          : marginal_variance;
  auto vol = blend_mdn_and_realized_variance(
      marginal_variance, realized_variance, options.volatility_options);

  TORCH_CHECK(ranges.adverse_excursion_prob.defined() &&
                  ranges.adverse_excursion_prob.dim() == 2 &&
                  ranges.adverse_excursion_prob.size(1) == A,
              "[allocation_belief_builder] adverse_excursion_prob must be "
              "[B,A]");
  auto range_adverse =
      ranges.adverse_excursion_prob.select(/*dim=*/0, options.anchor_slot)
          .to(tensor_options);
  auto flow_liquidity_score =
      detail::select_anchor_assets(flow.liquidity_score, options.anchor_slot,
                                   graph_index, "liquidity_score")
          .to(tensor_options)
          .clamp(0.0, 1.0);
  auto flow_capacity = detail::select_anchor_assets(
                           flow.capacity_weight_limit, options.anchor_slot,
                           graph_index, "capacity_weight_limit")
                           .to(tensor_options)
                           .clamp(0.0, 1.0);
  auto liquidity_score = options.capacity_weight_limit.defined()
                             ? flow_liquidity_score
                             : flow_liquidity_score;
  auto capacity =
      options.capacity_weight_limit.defined()
          ? detail::vector_or(options.capacity_weight_limit, A, tensor_options,
                              1.0, "capacity_weight_limit")
                .clamp(0.0, 1.0)
          : flow_capacity;
  auto linear_cost = detail::vector_or(options.linear_cost, A, tensor_options,
                                       0.0, "linear_cost")
                         .clamp_min(0.0);
  auto quadratic_impact =
      detail::vector_or(options.quadratic_impact, A, tensor_options, 0.0,
                        "quadratic_impact")
          .clamp_min(0.0);

  auto raw_entropy = detail::select_anchor_assets(
                         detail::mixture_entropy(surface.log_weight),
                         options.anchor_slot, graph_index, "mixture_entropy")
                         .to(tensor_options);
  auto normalized_entropy =
      detail::select_anchor_assets(
          detail::normalized_mixture_entropy(surface.log_weight),
          options.anchor_slot, graph_index, "normalized_mixture_entropy")
          .to(tensor_options);
  auto mixture_entropy_score =
      score_from_normalized_entropy(normalized_entropy.to(torch::kCPU))
          .to(tensor_options);
  auto component_disagreement =
      detail::select_anchor_assets(
          detail::mixture_disagreement(surface.log_weight, surface.potential_mu,
                                       surface.expected_potential),
          options.anchor_slot, graph_index, "component_disagreement")
          .to(tensor_options);
  auto channel_disagreement =
      detail::select_anchor_assets(consensus.channel_disagreement.select(
                                       2, options.surface_options.close_coord),
                                   options.anchor_slot, graph_index,
                                   "channel_disagreement")
          .to(tensor_options);
  auto channel_score = score_from_channel_disagreement(
                           channel_disagreement.to(torch::kCPU),
                           options.channel_disagreement_confidence_scale)
                           .to(tensor_options);
  auto data_score = detail::vector_or(options.data_quality_score, A,
                                      tensor_options, 1.0, "data_quality_score")
                        .clamp(0.0, 1.0);
  auto calibration_score = neutral_calibration_score(A, tensor_options);
  auto surprise_score = neutral_surprise_score(A, tensor_options);
  confidence_inputs_t confidence_inputs{};
  confidence_inputs.data_quality_score = data_score;
  confidence_inputs.liquidity_score = liquidity_score;
  confidence_inputs.calibration_score = calibration_score;
  confidence_inputs.entropy_score = mixture_entropy_score;
  confidence_inputs.channel_score = channel_score;
  confidence_inputs.surprise_score = surprise_score;
  auto final_confidence =
      compute_confidence(confidence_inputs, A, tensor_options)
          .masked_fill(valid_mask.logical_not(), 0.0)
          .masked_fill(tradable_mask.logical_not(), 0.0);

  AllocationBelief state{};
  state.anchor_key = options.anchor_key;
  state.timestamp_ms = options.timestamp_ms;
  state.graph_node_axis = graph_axis;
  state.graph_order_fingerprint = graph_axis.graph_order_fingerprint;
  state.source_feature_semantics_id = std::string(kKlineFeatureSemanticsId);
  state.source_feature_semantics_fingerprint =
      kline_feature_semantics_fingerprint();
  state.graph_node_ids = graph_axis.node_ids;
  state.node_ids = options.node_ids;
  state.node_graph_indices = options.node_graph_indices;
  state.base_policy = options.base_policy;
  state.projection_reference_graph_index = projection_reference_graph_index;
  state.horizon = 1;
  state.marginal_unit = return_unit_t::log_return;
  state.scenario_unit = return_unit_t::arithmetic_return;
  state.return_origin = return_origin_t::base_relative_nodelift_projection;
  state.valid_mask = std::move(valid_mask);
  state.tradable_mask = std::move(tradable_mask);
  state.expected_log_return = std::move(expected_log);
  state.expected_arithmetic_return = std::move(expected_arithmetic);
  state.marginal_variance = std::move(marginal_variance);
  state.marginal_volatility = std::move(marginal_volatility);
  state.covariance = projected.covariance.to(tensor_options);
  state.correlation = projected.correlation.to(tensor_options);
  state.scenarios = projected.arithmetic_return_scenarios.to(tensor_options);
  state.var_down = tails.var_down.to(tensor_options);
  state.cvar_down = tails.cvar_down.to(tensor_options);
  state.adverse_excursion_prob = std::move(range_adverse);
  state.volatility = vol.volatility.to(tensor_options);
  state.mixture_entropy = std::move(raw_entropy);
  state.component_disagreement = std::move(component_disagreement);
  state.channel_disagreement = std::move(channel_disagreement);
  state.surprise = torch::zeros({A}, tensor_options);
  state.calibration_score = calibration_score;
  state.liquidity_score = std::move(liquidity_score);
  state.linear_cost = std::move(linear_cost);
  state.quadratic_impact = std::move(quadratic_impact);
  state.capacity_weight_limit = std::move(capacity);
  state.confidence = std::move(final_confidence);
  state.projection_validation_required = true;
  state.projection_validated = false;
  state.live_capital_allowed = false;
  state.valid = true;
  state.diagnostics.notes.push_back("wikimyei.observer.belief.builder.v1");
  state.diagnostics.warnings.push_back(
      "allocation_belief.projection_validation_required_before_live_capital");
  validate_allocation_belief_contract(state);

  NodeLiftPotentialBelief nodelift{};
  nodelift.anchor_key = options.anchor_key;
  nodelift.timestamp_ms = options.timestamp_ms;
  nodelift.graph_node_axis = graph_axis;
  nodelift.graph_order_fingerprint = graph_axis.graph_order_fingerprint;
  nodelift.source_feature_semantics_id = std::string(kKlineFeatureSemanticsId);
  nodelift.source_feature_semantics_fingerprint =
      kline_feature_semantics_fingerprint();
  nodelift.graph_node_ids = graph_axis.node_ids;
  nodelift.horizon = 1;
  nodelift.gauge_policy = gauge_policy_t::uniform_per_component;
  nodelift.price_coord_semantics =
      coordinate_semantics_t::edge_log_return_lifted_potential;
  nodelift.log_weight =
      consensus.log_weight.select(/*dim=*/0, options.anchor_slot).detach();
  nodelift.mu = consensus.mu.select(/*dim=*/0, options.anchor_slot).detach();
  nodelift.sigma =
      consensus.sigma.select(/*dim=*/0, options.anchor_slot).detach();
  nodelift.active_mask =
      consensus.active_mask.select(/*dim=*/0, options.anchor_slot).detach();
  nodelift.valid = true;
  nodelift.diagnostics.notes.push_back(
      "wikimyei.observer.belief.nodelift_potential.v1");

  allocation_belief_build_result_t result{};
  result.allocation_belief = std::move(state);
  result.nodelift_potential_belief = std::move(nodelift);
  result.channel_consensus = std::move(consensus);
  result.potential_surface = std::move(surface);
  result.return_projection = std::move(projected);
  result.scenario_bank = std::move(scenarios);
  result.tail_risk = std::move(tails);
  result.range_risk = std::move(ranges);
  result.flow_liquidity = std::move(flow);
  result.volatility = std::move(vol);
  return result;
}

[[nodiscard]] inline AllocationBeliefBatch build_allocation_belief_batch(
    const mdn::MdnOut &out,
    const allocation_belief_batch_builder_options_t &options) {
  TORCH_CHECK(out.log_pi.defined() && out.log_pi.dim() == 5,
              "[allocation_belief_builder] MdnOut must be [B,N,C,Df,K]");
  const auto B = out.log_pi.size(0);
  TORCH_CHECK(static_cast<std::int64_t>(options.anchor_keys.size()) == B,
              "[allocation_belief_builder] anchor_keys must have B entries");
  TORCH_CHECK(static_cast<std::int64_t>(options.timestamps_ms.size()) == B,
              "[allocation_belief_builder] timestamps_ms must have B entries");

  AllocationBeliefBatch batch{};
  batch.beliefs.reserve(static_cast<std::size_t>(B));
  for (std::int64_t b = 0; b < B; ++b) {
    auto single = options.common;
    single.anchor_slot = b;
    single.anchor_key = options.anchor_keys[static_cast<std::size_t>(b)];
    single.timestamp_ms = options.timestamps_ms[static_cast<std::size_t>(b)];
    batch.beliefs.push_back(
        build_single_anchor_allocation_belief(out, single).allocation_belief);
  }
  return batch;
}

} // namespace cuwacunu::wikimyei::observer::belief
