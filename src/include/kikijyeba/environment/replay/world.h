// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/environment/control/interfaces.h"

namespace cuwacunu::kikijyeba::environment::replay {

namespace graph = cuwacunu::kikijyeba::topology::graph;

struct replay_world_options_t {
  reward_options_t reward_options{};
  double linear_transaction_cost_rate{0.0};
  cajtucu_execution::paper_execution_options_t paper_execution_options{};
  double eps{1.0e-12};
  double realized_return_consistency_tolerance{1.0e-8};
  double rebalance_plan_consistency_tolerance{1.0e-8};
  bool require_projection_validation{true};
  bool require_realized_log_return{true};
};

inline void
validate_replay_world_options(const replay_world_options_t &options) {
  validate_reward_options(options.reward_options);
  cajtucu_execution::validate_paper_execution_options(
      options.paper_execution_options);
  if (!std::isfinite(options.linear_transaction_cost_rate) ||
      !std::isfinite(options.eps) ||
      !std::isfinite(options.realized_return_consistency_tolerance) ||
      !std::isfinite(options.rebalance_plan_consistency_tolerance)) {
    throw std::runtime_error(
        "[replay_world] replay_world_options numeric fields must be finite");
  }
  if (options.linear_transaction_cost_rate < 0.0 ||
      options.realized_return_consistency_tolerance < 0.0 ||
      options.rebalance_plan_consistency_tolerance < 0.0) {
    throw std::runtime_error(
        "[replay_world] replay_world_options rates and tolerances must be "
        "nonnegative");
  }
  if (options.eps <= 0.0) {
    throw std::runtime_error("[replay_world] eps must be positive");
  }
}

struct replay_frame_t {
  observation_t observation{};

  // Realized asset/base returns revealed only after the action at this frame.
  torch::Tensor realized_log_return{};        // [A]
  torch::Tensor realized_arithmetic_return{}; // optional [A]

  // Optional belief-side log-return scenarios used to validate the projection
  // bridge against the realized return after the action.
  torch::Tensor projected_log_return_scenarios{}; // optional [S,A]
  torch::Tensor active_projection_mask{};         // optional [A], bool

  // Optional NodeLift residual diagnostics for the same observation.
  torch::Tensor nodelift_residual_energy{}; // optional [...,Df]
  torch::Tensor nodelift_residual_mask{};   // optional same shape, bool
};

namespace detail {

[[nodiscard]] inline torch::Tensor
require_realized_log_return(const replay_frame_t &frame, std::int64_t A,
                            const replay_world_options_t &options) {
  if (!frame.realized_log_return.defined()) {
    if (options.require_realized_log_return) {
      throw std::runtime_error(
          "[replay_world] realized_log_return is required");
    }
    return torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  }
  TORCH_CHECK(frame.realized_log_return.dim() == 1 &&
                  frame.realized_log_return.size(0) == A,
              "[replay_world] realized_log_return must be [A]");
  TORCH_CHECK(frame.realized_log_return.is_floating_point(),
              "[replay_world] realized_log_return must be floating point");
  auto out = frame.realized_log_return.to(torch::kFloat64);
  TORCH_CHECK(torch::isfinite(out).all().item<bool>(),
              "[replay_world] realized_log_return contains non-finite values");
  return out;
}

[[nodiscard]] inline torch::Tensor realized_arithmetic_return(
    const replay_frame_t &frame, const torch::Tensor &realized_log_return,
    std::int64_t A, const replay_world_options_t &options) {
  if (frame.realized_arithmetic_return.defined()) {
    TORCH_CHECK(frame.realized_arithmetic_return.dim() == 1 &&
                    frame.realized_arithmetic_return.size(0) == A,
                "[replay_world] realized_arithmetic_return must be [A]");
    TORCH_CHECK(
        frame.realized_arithmetic_return.is_floating_point(),
        "[replay_world] realized_arithmetic_return must be floating point");
    auto out = frame.realized_arithmetic_return.to(torch::kFloat64);
    TORCH_CHECK(
        torch::isfinite(out).all().item<bool>(),
        "[replay_world] realized_arithmetic_return contains non-finite values");
    TORCH_CHECK((out >= -1.0).all().item<bool>(),
                "[replay_world] realized_arithmetic_return cannot be below "
                "-100%");
    const double tolerance =
        std::max(0.0, options.realized_return_consistency_tolerance);
    const double max_err =
        (out - (realized_log_return.exp() - 1.0)).abs().max().item<double>();
    TORCH_CHECK(max_err <= tolerance,
                "[replay_world] realized_arithmetic_return must match "
                "exp(realized_log_return)-1");
    return out;
  }
  return realized_log_return.exp() - 1.0;
}

[[nodiscard]] inline portfolio::PortfolioState
portfolio_state_or_default(const observation_t &observation,
                           const episode_spec_t &spec) {
  const auto A = static_cast<std::int64_t>(spec.risky_node_ids.size());
  auto state = observation.portfolio_state;
  if (!state.current_weights.defined()) {
    state.current_weights =
        torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  }
  if (!state.current_units.defined()) {
    state.current_units =
        torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  }
  if (state.accounting_node_id.empty()) {
    state.accounting_node_id = spec.base_policy.accounting_numeraire_id;
  }
  if (state.reserve_node_id.empty()) {
    state.reserve_node_id = spec.base_policy.reserve_asset_id;
  }
  if (state.equity_value_base <= 0.0) {
    state.equity_value_base = spec.initial_equity_base;
  }
  if (!std::isfinite(state.equity_value_base)) {
    state.equity_value_base = spec.initial_equity_base;
  }
  const double risky_weight_sum =
      state.current_weights.to(torch::kFloat64).sum().item<double>();
  if (!std::isfinite(state.base_reserve_weight)) {
    state.base_reserve_weight = std::max(0.0, 1.0 - risky_weight_sum);
  }
  return state;
}

inline void
validate_portfolio_state_matches_episode(const portfolio::PortfolioState &state,
                                         const episode_spec_t &spec) {
  if (state.accounting_node_id != spec.base_policy.accounting_numeraire_id) {
    throw std::runtime_error(
        "[replay_world] portfolio accounting node must match EpisodeSpec "
        "BasePolicy accounting numeraire");
  }
  if (state.reserve_node_id != spec.base_policy.reserve_asset_id) {
    throw std::runtime_error(
        "[replay_world] portfolio reserve node must match EpisodeSpec "
        "BasePolicy reserve asset");
  }
}

inline void
validate_observation_state_timestamps(const observation_t &observation) {
  if (observation.portfolio_state.timestamp_ms > 0 &&
      observation.portfolio_state.timestamp_ms >
          observation.knowledge_timestamp_ms) {
    throw std::runtime_error(
        "[replay_world] portfolio state timestamp is after observation "
        "knowledge timestamp");
  }
  if (observation.market_state.timestamp_ms > 0 &&
      observation.market_state.timestamp_ms >
          observation.knowledge_timestamp_ms) {
    throw std::runtime_error(
        "[replay_world] market state timestamp is after observation knowledge "
        "timestamp");
  }
}

[[nodiscard]] inline bool edge_market_graph_is_empty(
    const execution::spot_edge_market_state_t &market_state) {
  return market_state.graph.node_ids.empty() &&
         market_state.graph.edge_ids.empty() &&
         market_state.graph.base_index.empty() &&
         market_state.graph.quote_index.empty();
}

[[nodiscard]] inline bool edge_market_graph_contains_node(
    const execution::spot_edge_market_state_t &market_state,
    const portfolio::node_id_t &node_id) {
  return std::find(market_state.graph.node_ids.begin(),
                   market_state.graph.node_ids.end(),
                   node_id) != market_state.graph.node_ids.end();
}

inline void validate_edge_market_state_matches_episode(
    const execution::spot_edge_market_state_t &market_state,
    const episode_spec_t &spec) {
  if (edge_market_graph_is_empty(market_state)) {
    return;
  }
  execution::validate_spot_edge_market_state(market_state);
  for (const auto &node_id : spec.risky_node_ids) {
    if (!edge_market_graph_contains_node(market_state, node_id)) {
      throw std::runtime_error(
          "[replay_world] edge market graph missing risky node: " + node_id);
    }
  }
  const auto &reserve_node_id = spec.base_policy.reserve_asset_id;
  if (!edge_market_graph_contains_node(market_state, reserve_node_id)) {
    throw std::runtime_error(
        "[replay_world] edge market graph missing base reserve node: " +
        reserve_node_id);
  }
}

inline void validate_frame_diagnostic_tensors(const replay_frame_t &frame,
                                              std::int64_t A,
                                              bool require_projection) {
  if (frame.projected_log_return_scenarios.defined()) {
    TORCH_CHECK(frame.projected_log_return_scenarios.dim() == 2 &&
                    frame.projected_log_return_scenarios.size(1) == A,
                "[replay_world] projected_log_return_scenarios must be [S,A]");
    TORCH_CHECK(frame.projected_log_return_scenarios.is_floating_point(),
                "[replay_world] projected_log_return_scenarios must be "
                "floating point");
    TORCH_CHECK(frame.projected_log_return_scenarios.size(0) > 0,
                "[replay_world] projected_log_return_scenarios must have at "
                "least one scenario");
    const auto scenarios =
        frame.projected_log_return_scenarios.to(torch::kFloat64);
    TORCH_CHECK(torch::isfinite(scenarios).all().item<bool>(),
                "[replay_world] projected_log_return_scenarios contain "
                "non-finite values");
    if (frame.active_projection_mask.defined()) {
      TORCH_CHECK(frame.active_projection_mask.dim() == 1 &&
                      frame.active_projection_mask.size(0) == A,
                  "[replay_world] active_projection_mask must be [A]");
      TORCH_CHECK(frame.active_projection_mask.scalar_type() == torch::kBool,
                  "[replay_world] active_projection_mask must be bool");
      TORCH_CHECK(frame.active_projection_mask.any().item<bool>(),
                  "[replay_world] active_projection_mask must contain at "
                  "least one active asset");
    }
  } else if (require_projection) {
    throw std::runtime_error(
        "[replay_world] projected_log_return_scenarios required");
  } else if (frame.active_projection_mask.defined()) {
    throw std::runtime_error("[replay_world] active_projection_mask requires "
                             "projected_log_return_scenarios");
  }

  if (frame.nodelift_residual_energy.defined()) {
    TORCH_CHECK(frame.nodelift_residual_energy.dim() >= 1 &&
                    frame.nodelift_residual_energy.numel() > 0,
                "[replay_world] nodelift_residual_energy must be nonempty");
    TORCH_CHECK(frame.nodelift_residual_energy.is_floating_point(),
                "[replay_world] nodelift_residual_energy must be floating "
                "point");
    const auto residual_energy =
        frame.nodelift_residual_energy.to(torch::kFloat64);
    TORCH_CHECK(torch::isfinite(residual_energy).all().item<bool>(),
                "[replay_world] nodelift_residual_energy contains non-finite "
                "values");
    if (frame.nodelift_residual_mask.defined()) {
      TORCH_CHECK(frame.nodelift_residual_mask.sizes() ==
                      frame.nodelift_residual_energy.sizes(),
                  "[replay_world] nodelift_residual_mask shape must match "
                  "nodelift_residual_energy");
      TORCH_CHECK(frame.nodelift_residual_mask.scalar_type() == torch::kBool,
                  "[replay_world] nodelift_residual_mask must be bool");
    }
  } else if (frame.nodelift_residual_mask.defined()) {
    throw std::runtime_error(
        "[replay_world] nodelift_residual_mask requires residual energy");
  }
}

inline void validate_allocation_belief_matches_frame(
    const belief::AllocationBelief &belief_state,
    const observation_t &observation, const episode_spec_t &spec) {
  belief::validate_allocation_belief_contract(belief_state);
  if (belief_state.anchor_key != observation.anchor_key) {
    throw std::runtime_error(
        "[replay_world] AllocationBelief anchor_key mismatch");
  }
  if (belief_state.timestamp_ms > observation.knowledge_timestamp_ms) {
    throw std::runtime_error(
        "[replay_world] AllocationBelief timestamp is after observation "
        "knowledge timestamp");
  }
  if (belief_state.graph_order_fingerprint != spec.graph_order_fingerprint) {
    throw std::runtime_error(
        "[replay_world] AllocationBelief graph fingerprint mismatch");
  }
  if (belief_state.graph_node_ids != spec.graph_node_ids) {
    throw std::runtime_error(
        "[replay_world] AllocationBelief graph node order mismatch");
  }
  if (belief_state.node_ids != spec.risky_node_ids) {
    throw std::runtime_error(
        "[replay_world] AllocationBelief risky node order mismatch");
  }
  if (belief_state.base_policy.accounting_numeraire_id !=
          spec.base_policy.accounting_numeraire_id ||
      belief_state.base_policy.settlement_asset_id !=
          spec.base_policy.settlement_asset_id ||
      belief_state.base_policy.reserve_asset_id !=
          spec.base_policy.reserve_asset_id ||
      belief_state.base_policy.projection_reference_node_id !=
          spec.base_policy.projection_reference_node_id) {
    throw std::runtime_error(
        "[replay_world] AllocationBelief BasePolicy mismatch");
  }
}

inline void validate_nodelift_potential_belief_matches_frame(
    const belief::NodeLiftPotentialBelief &belief_state,
    const observation_t &observation, const episode_spec_t &spec) {
  if (belief_state.anchor_key != observation.anchor_key) {
    throw std::runtime_error(
        "[replay_world] NodeLiftPotentialBelief anchor_key mismatch");
  }
  if (belief_state.timestamp_ms > observation.knowledge_timestamp_ms) {
    throw std::runtime_error(
        "[replay_world] NodeLiftPotentialBelief timestamp is after "
        "observation knowledge timestamp");
  }
  if (!belief_state.graph_order_fingerprint.empty() &&
      belief_state.graph_order_fingerprint != spec.graph_order_fingerprint) {
    throw std::runtime_error(
        "[replay_world] NodeLiftPotentialBelief graph fingerprint mismatch");
  }
  if (!belief_state.graph_node_ids.empty() &&
      belief_state.graph_node_ids != spec.graph_node_ids) {
    throw std::runtime_error(
        "[replay_world] NodeLiftPotentialBelief graph node order mismatch");
  }

  const bool has_tensors =
      belief_state.log_weight.defined() || belief_state.mu.defined() ||
      belief_state.sigma.defined() || belief_state.active_mask.defined();
  if (!has_tensors) {
    return;
  }
  const auto G = static_cast<std::int64_t>(spec.graph_node_ids.size());
  TORCH_CHECK(belief_state.log_weight.defined() && belief_state.mu.defined() &&
                  belief_state.sigma.defined() &&
                  belief_state.active_mask.defined(),
              "[replay_world] NodeLiftPotentialBelief tensors must be "
              "complete when present");
  TORCH_CHECK(belief_state.log_weight.dim() == 3,
              "[replay_world] NodeLiftPotentialBelief log_weight must be "
              "[G,Df,Kc]");
  TORCH_CHECK(belief_state.mu.sizes() == belief_state.log_weight.sizes() &&
                  belief_state.sigma.sizes() == belief_state.log_weight.sizes(),
              "[replay_world] NodeLiftPotentialBelief mixture tensor shape "
              "mismatch");
  TORCH_CHECK(belief_state.log_weight.size(0) == G,
              "[replay_world] NodeLiftPotentialBelief graph axis mismatch");
  TORCH_CHECK(belief_state.active_mask.dim() == 2 &&
                  belief_state.active_mask.size(0) == G &&
                  belief_state.active_mask.size(1) ==
                      belief_state.log_weight.size(1),
              "[replay_world] NodeLiftPotentialBelief active_mask must be "
              "[G,Df]");
  TORCH_CHECK(belief_state.active_mask.scalar_type() == torch::kBool,
              "[replay_world] NodeLiftPotentialBelief active_mask must be "
              "bool");
  TORCH_CHECK(torch::isfinite(belief_state.log_weight.to(torch::kFloat64))
                      .all()
                      .item<bool>() &&
                  torch::isfinite(belief_state.mu.to(torch::kFloat64))
                      .all()
                      .item<bool>() &&
                  torch::isfinite(belief_state.sigma.to(torch::kFloat64))
                      .all()
                      .item<bool>(),
              "[replay_world] NodeLiftPotentialBelief mixture tensors contain "
              "non-finite values");
  TORCH_CHECK(
      !belief_state.sigma.to(torch::kFloat64).lt(0.0).any().item<bool>(),
      "[replay_world] NodeLiftPotentialBelief sigma must be "
      "nonnegative");
}

inline void
validate_observation_beliefs_match_frame(const observation_t &observation,
                                         const episode_spec_t &spec) {
  if (observation.allocation_belief.has_value()) {
    validate_allocation_belief_matches_frame(*observation.allocation_belief,
                                             observation, spec);
  }
  if (observation.nodelift_potential_belief.has_value()) {
    validate_nodelift_potential_belief_matches_frame(
        *observation.nodelift_potential_belief, observation, spec);
  }
}

inline void validate_frame(const replay_frame_t &frame,
                           const episode_spec_t &spec, std::size_t offset,
                           const replay_world_options_t &options) {
  const auto expected_anchor = spec.accepted_range.anchor_index_begin +
                               static_cast<std::int64_t>(offset);
  const auto &expected_key = spec.accepted_range.anchor_keys.at(offset);
  validate_observation_time_boundary(frame.observation);
  validate_observation_state_timestamps(frame.observation);
  if (frame.observation.observation_anchor_index != expected_anchor ||
      frame.observation.anchor_key != expected_key) {
    throw std::runtime_error("[replay_world] frame observation does not match "
                             "accepted cursor range");
  }
  if (frame.observation.next_realization_anchor_index != expected_anchor + 1) {
    throw std::runtime_error(
        "[replay_world] frame next-realization anchor must be the immediate "
        "next accepted anchor");
  }
  const auto A = static_cast<std::int64_t>(spec.risky_node_ids.size());
  auto log_return = require_realized_log_return(frame, A, options);
  (void)realized_arithmetic_return(frame, log_return, A, options);
  auto state = portfolio_state_or_default(frame.observation, spec);
  portfolio::validate_portfolio_state(state, A);
  validate_portfolio_state_matches_episode(state, spec);
  portfolio::validate_market_state(frame.observation.market_state, A);
  validate_edge_market_state_matches_episode(
      frame.observation.edge_market_state, spec);
  validate_frame_diagnostic_tensors(frame, A,
                                    options.require_projection_validation);
  validate_observation_beliefs_match_frame(frame.observation, spec);
}

inline void validate_frame_sequence(const replay_frame_t &current,
                                    const replay_frame_t &next) {
  if (next.observation.observation_anchor_index !=
      current.observation.next_realization_anchor_index) {
    throw std::runtime_error(
        "[replay_world] next frame must start at the prior "
        "next-realization anchor");
  }
  if (next.observation.knowledge_timestamp_ms <
      current.observation.realization_available_after_timestamp_ms) {
    throw std::runtime_error(
        "[replay_world] next frame knowledge timestamp cannot precede prior "
        "realization boundary");
  }
}

[[nodiscard]] inline portfolio::TargetPortfolio
make_target_from_action(const action_t &action,
                        const portfolio::PortfolioState &current_state,
                        portfolio::timestamp_ms_t timestamp_ms) {
  const auto A = static_cast<std::int64_t>(action.node_ids.size());
  portfolio::TargetPortfolio target{};
  target.timestamp_ms = timestamp_ms;
  target.node_ids = action.node_ids;
  target.base_reserve_node_id = action.base_reserve_node_id;
  target.target_weights = action.target_weights.to(torch::kFloat64);
  target.target_base_reserve_weight = action.target_base_reserve_weight;
  target.delta_weights =
      target.target_weights - current_state.current_weights.to(torch::kFloat64);
  if (std::isfinite(action.expected_log_growth)) {
    target.expected_log_growth = action.expected_log_growth;
  }
  if (std::isfinite(action.expected_arithmetic_return)) {
    target.expected_arithmetic_return = action.expected_arithmetic_return;
  }
  if (std::isfinite(action.cvar_loss)) {
    target.cvar_loss = action.cvar_loss;
  }
  if (std::isfinite(action.estimated_transaction_cost)) {
    target.estimated_transaction_cost = action.estimated_transaction_cost;
  }
  target.diagnostics.notes = action.diagnostics_notes;
  target.diagnostics.warnings = action.diagnostics_warnings;
  target.diagnostics.failures = action.diagnostics_failures;
  target.turnover = target.delta_weights.abs().sum().item<double>();
  target.valid = true;
  portfolio::validate_target_portfolio(target, A,
                                       current_state.current_weights);
  return target;
}

[[nodiscard]] inline cajtucu_execution::market_execution_state_t
make_cajtucu_market_execution_state(
    const execution::spot_edge_market_state_t &edge_market_state,
    const portfolio::MarketState &market_state, const episode_spec_t &spec,
    const replay_world_options_t &options) {
  cajtucu_execution::market_execution_state_t out{};
  out.timestamp_ms = market_state.timestamp_ms;
  const auto A = static_cast<std::int64_t>(spec.risky_node_ids.size());
  auto tensor_options = torch::TensorOptions().dtype(torch::kFloat64);

  if (!edge_market_graph_is_empty(edge_market_state)) {
    execution::validate_spot_edge_market_state(edge_market_state);
    out.market_source_id = "kikijyeba.environment.replay.edge_market_state";
    out.synthetic_direct_edges = false;
    out.graph = edge_market_state.graph;
    out.edge_mid_price = edge_market_state.edge_mid_price;
    out.edge_fee_rate =
        edge_market_state.edge_fee_rate.defined()
            ? edge_market_state.edge_fee_rate.to(torch::kFloat64) +
                  options.linear_transaction_cost_rate
            : torch::full({out.graph.num_edges()},
                          options.linear_transaction_cost_rate, tensor_options);
    out.edge_spread_rate = edge_market_state.edge_spread_rate;
    out.edge_slippage_rate = edge_market_state.edge_slippage_rate;
    out.min_notional_base = edge_market_state.min_notional_base;
    out.max_notional_base = edge_market_state.max_notional_base;
    out.edge_tradable_mask = edge_market_state.edge_tradable_mask;
    cajtucu_execution::validate_market_execution_state(out);
    return out;
  }

  portfolio::validate_market_state(market_state, A);
  out.market_source_id =
      "kikijyeba.environment.replay.synthetic_node_market_state_direct_edges";
  out.synthetic_direct_edges = true;
  out.graph.node_ids = spec.risky_node_ids;
  out.graph.node_ids.push_back(spec.base_policy.reserve_asset_id);
  const auto reserve_index = static_cast<graph::node_index_t>(A);
  for (std::int64_t i = 0; i < A; ++i) {
    out.graph.edge_ids.push_back(
        spec.risky_node_ids[static_cast<std::size_t>(i)] + "/" +
        spec.base_policy.reserve_asset_id);
    out.graph.base_index.push_back(static_cast<graph::node_index_t>(i));
    out.graph.quote_index.push_back(reserve_index);
  }
  out.edge_mid_price = market_state.executable_mid.defined()
                           ? market_state.executable_mid.to(torch::kFloat64)
                           : torch::ones({A}, tensor_options);
  out.edge_fee_rate =
      market_state.fee_rate.defined()
          ? market_state.fee_rate.to(torch::kFloat64) +
                options.linear_transaction_cost_rate
          : torch::full({A}, options.linear_transaction_cost_rate,
                        tensor_options);
  out.edge_spread_rate = market_state.bid_ask_spread;
  out.edge_slippage_rate = market_state.estimated_slippage;
  out.min_notional_base = market_state.min_notional;
  out.max_notional_base = market_state.max_notional;
  out.edge_tradable_mask = market_state.tradable_mask;
  cajtucu_execution::validate_market_execution_state(out);
  return out;
}

[[nodiscard]] inline cajtucu_execution::execution_ledger_t
make_cajtucu_execution_ledger(
    const portfolio::PortfolioState &portfolio_state,
    const cajtucu_execution::market_execution_state_t &market,
    const episode_spec_t &spec) {
  const auto A = static_cast<std::int64_t>(spec.risky_node_ids.size());
  portfolio::validate_portfolio_state(portfolio_state, A);
  auto weights = portfolio_state.current_weights.to(torch::kFloat64);
  auto prices = cajtucu_execution::asset_price_base_vector(
      market, spec.risky_node_ids, spec.base_policy.reserve_asset_id);
  auto units = portfolio_state.current_units.defined()
                   ? portfolio_state.current_units.to(torch::kFloat64)
                   : (weights * portfolio_state.equity_value_base) / prices;
  cajtucu_execution::execution_ledger_t out{};
  out.timestamp_ms = portfolio_state.timestamp_ms;
  out.base_reserve_node_id = spec.base_policy.reserve_asset_id;
  out.node_ids = spec.risky_node_ids;
  out.units = units.clamp_min(0.0);
  out.weights = weights.clamp_min(0.0);
  out.base_reserve_units = std::max(0.0, portfolio_state.base_reserve_weight) *
                           portfolio_state.equity_value_base;
  out.base_reserve_weight = portfolio_state.base_reserve_weight;
  out.equity_value_base = portfolio_state.equity_value_base;
  cajtucu_execution::validate_execution_ledger(out);
  return out;
}

[[nodiscard]] inline cajtucu_execution::execution_intent_t
make_cajtucu_execution_intent(const action_t &action,
                              const portfolio::TargetPortfolio &target,
                              const portfolio::PortfolioState &current_state,
                              const episode_spec_t &spec,
                              const observation_t &observation) {
  cajtucu_execution::execution_intent_t intent{};
  intent.intent_id = spec.environment_run_id + ":" + spec.episode_id + ":" +
                     observation.anchor_key + ":" + action.policy_id;
  intent.action_id = action.policy_id + ":" + observation.anchor_key;
  intent.policy_id = action.policy_id;
  intent.method_id = action.method_id;
  intent.runtime_run_id = spec.runtime_run_id;
  intent.environment_run_id = spec.environment_run_id;
  intent.episode_id = spec.episode_id;
  intent.anchor_key = observation.anchor_key;
  intent.timestamp_ms = target.timestamp_ms;
  intent.node_ids = target.node_ids;
  intent.base_reserve_node_id = target.base_reserve_node_id;
  intent.current_weights = current_state.current_weights.to(torch::kFloat64);
  intent.target_weights = target.target_weights.to(torch::kFloat64);
  intent.current_base_reserve_weight = current_state.base_reserve_weight;
  intent.target_base_reserve_weight = target.target_base_reserve_weight;
  intent.equity_value_base = current_state.equity_value_base;
  cajtucu_execution::validate_execution_intent(intent);
  return intent;
}

[[nodiscard]] inline execution::order_side_t
to_rebalance_order_side(const cajtucu_execution::execution_order_t &order) {
  const bool asset_is_edge_base = order.edge_base_node_id == order.node_id;
  if (order.side == cajtucu_execution::order_side_t::buy_asset) {
    return asset_is_edge_base ? execution::order_side_t::buy_edge_base
                              : execution::order_side_t::sell_edge_base;
  }
  return asset_is_edge_base ? execution::order_side_t::sell_edge_base
                            : execution::order_side_t::buy_edge_base;
}

[[nodiscard]] inline execution::spot_rebalance_plan_t
make_rebalance_plan_from_cajtucu_trace(
    const cajtucu_execution::execution_trace_t &trace) {
  execution::spot_rebalance_plan_t plan{};
  plan.timestamp_ms = trace.intent.timestamp_ms;
  plan.base_reserve_node_id = trace.intent.base_reserve_node_id;
  plan.node_ids = trace.intent.node_ids;
  plan.requested_turnover_weight = trace.turnover_weight;
  plan.requested_notional_base = trace.requested_notional_base;
  plan.routed_notional_base = trace.routed_notional_base;
  plan.estimated_transaction_cost_base = trace.total_transaction_cost_base;
  plan.valid = trace.valid;
  for (const auto &order : trace.orders) {
    if (!order.valid) {
      plan.skipped.push_back({
          .node_id = order.node_id,
          .delta_weight = order.delta_weight,
          .requested_notional_base = order.requested_notional_base,
          .reason =
              order.reject_reason.empty() ? "rejected" : order.reject_reason,
      });
      continue;
    }
    plan.orders.push_back({
        .node_id = order.node_id,
        .edge_id = order.edge_id,
        .edge_base_node_id = order.edge_base_node_id,
        .edge_quote_node_id = order.edge_quote_node_id,
        .side = to_rebalance_order_side(order),
        .delta_weight = order.delta_weight,
        .requested_notional_base = order.requested_notional_base,
        .routed_notional_base = order.routed_notional_base,
        .estimated_cost_base =
            order.fee_base + order.spread_cost_base + order.slippage_base,
    });
    plan.routed_turnover_weight +=
        order.routed_notional_base /
        std::max(trace.ledger_before.equity_value_base, 1.0e-12);
  }
  plan.diagnostics.notes.push_back(cajtucu_execution::kCajtucuPaperBackendIdV1);
  plan.diagnostics.warnings = trace.warnings;
  plan.diagnostics.failures = trace.failures;
  return plan;
}

[[nodiscard]] inline std::vector<accounting::executed_fill_t>
make_fills_from_cajtucu_trace(
    const cajtucu_execution::execution_trace_t &trace) {
  std::vector<accounting::executed_fill_t> fills;
  for (const auto &fill : trace.fills) {
    if (fill.status == cajtucu_execution::fill_status_t::rejected) {
      continue;
    }
    fills.push_back({
        .timestamp_ms = trace.intent.timestamp_ms,
        .node_id = fill.node_id,
        .edge_id = fill.edge_id,
        .side = fill.side == cajtucu_execution::order_side_t::buy_asset
                    ? accounting::fill_side_t::buy_asset
                    : accounting::fill_side_t::sell_asset,
        .quantity_asset = fill.filled_quantity_asset,
        .gross_notional_base = fill.gross_notional_base,
        .fee_base = fill.fee_base,
    });
  }
  return fills;
}

[[nodiscard]] inline execution::spot_rebalance_plan_t
make_weight_rebalance_plan(const portfolio::TargetPortfolio &target,
                           const portfolio::PortfolioState &current_state,
                           double estimated_transaction_cost_base) {
  execution::spot_rebalance_plan_t plan{};
  plan.timestamp_ms = target.timestamp_ms;
  plan.base_reserve_node_id = target.base_reserve_node_id;
  plan.node_ids = target.node_ids;
  plan.requested_turnover_weight = target.turnover;
  plan.routed_turnover_weight = target.turnover;
  plan.requested_notional_base =
      target.turnover * current_state.equity_value_base;
  plan.routed_notional_base = plan.requested_notional_base;
  plan.estimated_transaction_cost_base = estimated_transaction_cost_base;
  plan.valid = true;
  plan.diagnostics.notes.push_back(
      "kikijyeba.environment.replay.weight_rebalance_plan.v1");
  return plan;
}

[[nodiscard]] inline bool approx_equal(double actual, double expected,
                                       double tolerance) {
  return std::abs(actual - expected) <=
         tolerance * std::max(1.0, std::abs(expected));
}

inline void validate_policy_rebalance_plan_matches_step(
    const execution::spot_rebalance_plan_t &plan,
    const portfolio::TargetPortfolio &target,
    const portfolio::PortfolioState &current_state, double tolerance) {
  if (!plan.valid) {
    throw std::runtime_error(
        "[replay_world] policy rebalance plan must be valid");
  }
  if (!approx_equal(plan.requested_turnover_weight, target.turnover,
                    tolerance)) {
    throw std::runtime_error(
        "[replay_world] policy rebalance plan requested turnover does not "
        "match target action turnover");
  }
  const double expected_requested_notional =
      target.turnover * current_state.equity_value_base;
  if (!approx_equal(plan.requested_notional_base, expected_requested_notional,
                    tolerance)) {
    throw std::runtime_error(
        "[replay_world] policy rebalance plan requested notional does not "
        "match target action turnover/equity");
  }
  if (plan.timestamp_ms <= 0) {
    throw std::runtime_error(
        "[replay_world] policy rebalance plan timestamp is required");
  }
  if (plan.timestamp_ms != target.timestamp_ms) {
    throw std::runtime_error(
        "[replay_world] policy rebalance plan timestamp does not match target "
        "timestamp");
  }
}

[[nodiscard]] inline std::vector<accounting::executed_fill_t>
make_weight_fills(const portfolio::TargetPortfolio &target,
                  const portfolio::PortfolioState &current_state,
                  const portfolio::MarketState &market_state,
                  double total_transaction_cost_base) {
  std::vector<accounting::executed_fill_t> fills;
  const auto A = static_cast<std::int64_t>(target.node_ids.size());
  auto delta = target.delta_weights.to(torch::kFloat64);
  auto mid = market_state.executable_mid.defined()
                 ? market_state.executable_mid.to(torch::kFloat64)
                 : torch::Tensor();
  double routed_notional = target.turnover * current_state.equity_value_base;
  for (std::int64_t i = 0; i < A; ++i) {
    const double d = delta.index({i}).item<double>();
    if (std::abs(d) <= 1.0e-12) {
      continue;
    }
    const double notional = std::abs(d) * current_state.equity_value_base;
    double quantity = 0.0;
    if (mid.defined() && mid.dim() == 1 && mid.size(0) == A) {
      const double price = mid.index({i}).item<double>();
      if (std::isfinite(price) && price > 0.0) {
        quantity = notional / price;
      }
    }
    const double fee = routed_notional > 0.0 ? total_transaction_cost_base *
                                                   (notional / routed_notional)
                                             : 0.0;
    fills.push_back({
        .timestamp_ms = target.timestamp_ms,
        .node_id = target.node_ids[static_cast<std::size_t>(i)],
        .edge_id = std::string("replay_weight_fill:") +
                   target.node_ids[static_cast<std::size_t>(i)] + "/" +
                   target.base_reserve_node_id,
        .side = d >= 0.0 ? accounting::fill_side_t::buy_asset
                         : accounting::fill_side_t::sell_asset,
        .quantity_asset = quantity,
        .gross_notional_base = notional,
        .fee_base = fee,
    });
  }
  return fills;
}

} // namespace detail

class replay_world_t final : public world_iface_t {
public:
  explicit replay_world_t(std::vector<replay_frame_t> frames,
                          replay_world_options_t options = {})
      : frames_(std::move(frames)), options_(options) {}

  [[nodiscard]] observation_t reset(const episode_spec_t &spec) override {
    validate_episode_spec(spec);
    validate_replay_world_options(options_);
    if (spec.world_mode != world_mode_t::historical_replay) {
      throw std::runtime_error(
          "[replay_world] V1 requires historical_replay world mode");
    }
    if (spec.require_projection_validation &&
        !options_.require_projection_validation) {
      throw std::runtime_error(
          "[replay_world] EpisodeSpec requires projection validation but "
          "replay_world_options disabled it");
    }
    const auto expected_frame_count =
        static_cast<std::size_t>(spec.accepted_range.anchor_index_end -
                                 spec.accepted_range.anchor_index_begin);
    if (frames_.size() != expected_frame_count) {
      throw std::runtime_error(
          "[replay_world] frame count must match accepted anchor range");
    }
    spec_ = spec;
    step_index_ = 0;
    active_ = true;
    for (std::size_t i = 0; i < frames_.size(); ++i) {
      detail::validate_frame(frames_[i], spec_, i, options_);
      if (i + 1 < frames_.size()) {
        detail::validate_frame_sequence(frames_[i], frames_[i + 1]);
      }
    }
    current_portfolio_ =
        detail::portfolio_state_or_default(frames_.front().observation, spec_);
    portfolio::validate_portfolio_state(
        current_portfolio_,
        static_cast<std::int64_t>(spec_.risky_node_ids.size()));
    peak_equity_base_ = current_portfolio_.equity_value_base;
    auto observation = frames_.front().observation;
    observation.portfolio_state = current_portfolio_;
    validate_observation_time_boundary(observation);
    return observation;
  }

  [[nodiscard]] transition_t step(const action_t &action) override {
    if (!active_) {
      throw std::runtime_error(
          "[replay_world] reset must be called before step");
    }
    if (step_index_ >= frames_.size()) {
      throw std::runtime_error("[replay_world] episode already exhausted");
    }
    validate_episode_action(action, spec_);

    const auto A = static_cast<std::int64_t>(spec_.risky_node_ids.size());
    const auto &frame = frames_[step_index_];
    validate_action_time_boundary(action, frame.observation);
    const auto decision_timestamp_ms =
        resolve_action_decision_timestamp(action, frame.observation);
    auto log_return = detail::require_realized_log_return(frame, A, options_);
    auto arithmetic_return =
        detail::realized_arithmetic_return(frame, log_return, A, options_);

    transition_t transition{};
    transition.info.anchor_key = frame.observation.anchor_key;
    transition.info.anchor_index = frame.observation.observation_anchor_index;
    transition.info.portfolio_before = current_portfolio_;

    auto target = detail::make_target_from_action(action, current_portfolio_,
                                                  decision_timestamp_ms);
    transition.info.target = target;
    transition.info.turnover = target.turnover;
    if (action.risk_gate_evaluated) {
      transition.info.risk_gate_evaluated = true;
      transition.info.risk_gate = action.risk_gate;
    }
    if (action.decision_report_available) {
      transition.info.decision_report = action.decision_report;
    }

    const double equity_before = current_portfolio_.equity_value_base;
    const auto cajtucu_market = detail::make_cajtucu_market_execution_state(
        frame.observation.edge_market_state, frame.observation.market_state,
        spec_, options_);
    const auto cajtucu_ledger_before = detail::make_cajtucu_execution_ledger(
        current_portfolio_, cajtucu_market, spec_);
    const auto cajtucu_intent = detail::make_cajtucu_execution_intent(
        action, target, current_portfolio_, spec_, frame.observation);
    cajtucu_execution::paper_execution_backend_t paper_backend(
        options_.paper_execution_options);
    auto execution_trace = paper_backend.execute(cajtucu_intent, cajtucu_market,
                                                 cajtucu_ledger_before);
    transition.info.warnings.insert(transition.info.warnings.end(),
                                    execution_trace.warnings.begin(),
                                    execution_trace.warnings.end());
    transition.info.failures.insert(transition.info.failures.end(),
                                    execution_trace.failures.begin(),
                                    execution_trace.failures.end());
    const double transaction_cost_base =
        execution_trace.total_transaction_cost_base;

    auto execution_units =
        execution_trace.ledger_after.units.to(torch::kFloat64);
    auto execution_prices = cajtucu_execution::asset_price_base_vector(
        cajtucu_market, spec_.risky_node_ids,
        spec_.base_policy.reserve_asset_id);
    auto risky_value_after =
        execution_units * execution_prices * (1.0 + arithmetic_return);
    double reserve_value_after =
        execution_trace.ledger_after.base_reserve_units;
    double equity_after =
        risky_value_after.sum().item<double>() + reserve_value_after;
    if (!std::isfinite(equity_after) || equity_after <= options_.eps) {
      throw std::runtime_error(
          "[replay_world] action leads to nonpositive realized equity");
    }

    auto risky_weight_after = risky_value_after / equity_after;

    portfolio::PortfolioState after = current_portfolio_;
    after.timestamp_ms =
        frame.observation.realization_available_after_timestamp_ms;
    after.current_weights = risky_weight_after.clamp_min(0.0);
    after.base_reserve_weight = reserve_value_after / equity_after;
    after.equity_value_base = equity_after;
    after.current_units = execution_units.clone();
    peak_equity_base_ = std::max(peak_equity_base_, equity_after);
    after.drawdown = peak_equity_base_ > options_.eps
                         ? std::max(0.0, (peak_equity_base_ - equity_after) /
                                             peak_equity_base_)
                         : 0.0;

    portfolio::validate_portfolio_state(after, A);

    transition.info.portfolio_after = after;
    transition.info.realized_log_growth =
        std::log(equity_after / equity_before);
    transition.info.realized_arithmetic_return =
        (equity_after / equity_before) - 1.0;
    transition.info.transaction_cost_base = transaction_cost_base;
    transition.info.execution_trace = execution_trace;
    transition.info.cajtucu_execution_trace_available = true;
    if (action.rebalance_plan_available) {
      detail::validate_policy_rebalance_plan_matches_step(
          action.rebalance_plan, target, current_portfolio_,
          options_.rebalance_plan_consistency_tolerance);
      transition.info.warnings.push_back(
          "replay_world.policy_rebalance_plan_validated_but_cajtucu_executed");
    }
    transition.info.rebalance_plan =
        detail::make_rebalance_plan_from_cajtucu_trace(execution_trace);
    transition.info.rebalance_plan_source =
        cajtucu_execution::kCajtucuPaperBackendIdV1;
    transition.info.rebalance_plan_enforced = true;
    transition.info.execution_model =
        cajtucu_execution::kCajtucuPaperBackendIdV1;
    transition.info.fills =
        detail::make_fills_from_cajtucu_trace(execution_trace);
    if (!execution_trace.valid || execution_trace.rejected_fill_count > 0) {
      transition.info.invalid_action = true;
      transition.info.warnings.push_back(execution_trace.valid
                                             ? "cajtucu.paper.rejected_fills"
                                             : "cajtucu.paper.invalid_trace");
    }

    if (frame.projected_log_return_scenarios.defined()) {
      transition.info.projection_validation =
          observer::validate_projected_log_return(
              frame.projected_log_return_scenarios, log_return,
              frame.active_projection_mask);
    }
    if (frame.nodelift_residual_energy.defined()) {
      transition.info.residual_quality =
          observer::compute_nodelift_residual_quality(
              frame.nodelift_residual_energy, frame.nodelift_residual_mask);
    }

    transition.reward =
        compute_reward(equity_before, equity_after, after.drawdown,
                       transaction_cost_base, target.turnover,
                       transition.info.invalid_action, options_.reward_options);

    ++step_index_;
    current_portfolio_ = after;
    transition.done = step_index_ >= frames_.size();
    if (!transition.done) {
      transition.next_observation = frames_[step_index_].observation;
      transition.next_observation.portfolio_state = current_portfolio_;
      validate_observation_time_boundary(transition.next_observation);
      detail::validate_frame_sequence(frame, frames_[step_index_]);
    }
    return transition;
  }

  [[nodiscard]] std::size_t step_index() const { return step_index_; }

private:
  std::vector<replay_frame_t> frames_{};
  replay_world_options_t options_{};
  episode_spec_t spec_{};
  portfolio::PortfolioState current_portfolio_{};
  double peak_equity_base_{0.0};
  std::size_t step_index_{0};
  bool active_{false};
};

} // namespace cuwacunu::kikijyeba::environment::replay
