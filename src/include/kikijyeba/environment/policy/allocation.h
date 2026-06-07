// SPDX-License-Identifier: MIT
#pragma once

#include <stdexcept>
#include <string>
#include <utility>

#include "kikijyeba/environment/control/interfaces.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/decision_step.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/solver.h"

namespace cuwacunu::kikijyeba::environment {

namespace wikimyei_sdu =
    cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility;

inline constexpr const char *kSpotDistributionalUtilityPolicyId =
    "kikijyeba.environment.policy.spot_distributional_utility.v1";
inline constexpr const char *kSpotDistributionalUtilityMethodId =
    "wikimyei.policy.portfolio.spot_distributional_utility.v1";
inline constexpr const char *kSpotDistributionalUtilityDecisionStepMethodId =
    "wikimyei.policy.portfolio.spot_distributional_utility.decision_step.v1";

struct spot_distributional_utility_policy_config_t {
  std::string policy_id{kSpotDistributionalUtilityPolicyId};
  std::string method_id{kSpotDistributionalUtilityMethodId};
  portfolio::PortfolioConstraints constraints{};
  wikimyei_sdu::solver_options_t solver_options{};
  wikimyei_sdu::decision_step::decision_step_options_t decision_options{};
  bool require_allocation_belief{true};
  bool require_valid_target{true};
  bool use_decision_step_when_edge_market_available{true};
};

class spot_distributional_utility_policy_t final
    : public policy_adapter_iface_t {
public:
  explicit spot_distributional_utility_policy_t(
      spot_distributional_utility_policy_config_t config = {})
      : config_(std::move(config)) {
    if (detail::blank(config_.policy_id)) {
      throw std::runtime_error(
          "[spot_distributional_utility_policy] policy_id is required");
    }
    if (detail::blank(config_.method_id)) {
      throw std::runtime_error(
          "[spot_distributional_utility_policy] method_id is required");
    }
  }

  [[nodiscard]] std::string policy_id() const override {
    return config_.policy_id;
  }

  [[nodiscard]] policy_kind_t policy_kind() const override {
    return policy_kind_t::deterministic_allocator;
  }

  [[nodiscard]] action_t act(const observation_t &observation) override {
    if (!observation.allocation_belief.has_value()) {
      if (config_.require_allocation_belief) {
        throw std::runtime_error(
            "[spot_distributional_utility_policy] AllocationBelief is "
            "required");
      }
      throw std::runtime_error(
          "[spot_distributional_utility_policy] no fallback policy configured");
    }

    last_decision_step_available_ = false;
    const bool edge_market_available =
        observation.edge_market_state.graph.num_edges() > 0;
    if (config_.use_decision_step_when_edge_market_available &&
        edge_market_available) {
      auto decision_options = config_.decision_options;
      decision_options.solver_options = config_.solver_options;
      last_decision_step_ = wikimyei_sdu::decision_step::run(
          *observation.allocation_belief, observation.portfolio_state,
          observation.market_state, config_.constraints,
          observation.edge_market_state, decision_options);
      last_decision_step_available_ = true;
      last_target_ = last_decision_step_.target;
    } else {
      last_target_ = wikimyei_sdu::solve(
          *observation.allocation_belief, observation.portfolio_state,
          observation.market_state, config_.constraints,
          config_.solver_options);
    }
    if (config_.require_valid_target && !last_target_.valid) {
      throw std::runtime_error(
          "[spot_distributional_utility_policy] solver produced invalid "
          "target");
    }

    action_t action{};
    action.policy_id = config_.policy_id;
    action.method_id = config_.method_id;
    action.policy_kind = policy_kind_t::deterministic_allocator;
    action.decision_timestamp_ms = observation.knowledge_timestamp_ms;
    action.node_ids = last_target_.node_ids;
    action.target_weights =
        last_target_.target_weights.to(torch::kFloat64).contiguous();
    action.expected_log_growth = last_target_.expected_log_growth;
    action.expected_arithmetic_return = last_target_.expected_arithmetic_return;
    action.cvar_loss = last_target_.cvar_loss;
    action.estimated_transaction_cost = last_target_.estimated_transaction_cost;
    action.diagnostics_notes = last_target_.diagnostics.notes;
    action.diagnostics_warnings = last_target_.diagnostics.warnings;
    action.diagnostics_failures = last_target_.diagnostics.failures;
    if (last_decision_step_available_) {
      action.risk_gate_evaluated = true;
      action.risk_gate = last_decision_step_.risk_gate;
      action.rebalance_plan_available = true;
      action.rebalance_plan = last_decision_step_.rebalance_plan;
      action.rebalance_plan.timestamp_ms = action.decision_timestamp_ms;
      action.rebalance_plan_source =
          kSpotDistributionalUtilityDecisionStepMethodId;
      action.decision_report_available = true;
      action.decision_report = last_decision_step_.report;
    }
    validate_action(action, action.node_ids);
    return action;
  }

  [[nodiscard]] const portfolio::TargetPortfolio &last_target() const {
    return last_target_;
  }

  [[nodiscard]] bool last_decision_step_available() const {
    return last_decision_step_available_;
  }

private:
  spot_distributional_utility_policy_config_t config_{};
  portfolio::TargetPortfolio last_target_{};
  wikimyei_sdu::decision_step::decision_step_result_t last_decision_step_{};
  bool last_decision_step_available_{false};
};

} // namespace cuwacunu::kikijyeba::environment
