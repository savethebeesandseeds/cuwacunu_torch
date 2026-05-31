// SPDX-License-Identifier: MIT
#pragma once

#include <exception>
#include <string>
#include <utility>

#include "wikimyei/engine/portfolio/spot_distributional_utility/solver.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/types.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/utility/base_reserve_fallback.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/utility/belief_reporter.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/utility/risk_gate.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/utility/spot_rebalance_router.h"
#include "wikimyei/observer/belief/types.h"

namespace cuwacunu::wikimyei::engine::portfolio::spot_distributional_utility::
    decision_step {

namespace belief = cuwacunu::wikimyei::observer::belief;
namespace execution = cuwacunu::wikimyei::engine::portfolio::
    spot_distributional_utility::execution::spot_rebalance_router;
namespace risk_gate = cuwacunu::wikimyei::engine::portfolio::
    spot_distributional_utility::risk::risk_gate;
namespace monitoring = cuwacunu::wikimyei::engine::portfolio::
    spot_distributional_utility::monitoring::belief_reporter;

struct decision_step_options_t {
  risk_gate::risk_gate_thresholds_t risk_thresholds{};
  cuwacunu::wikimyei::engine::portfolio::spot_distributional_utility::
      solver_options_t solver_options{};
  execution::spot_rebalance_router_options_t router_options{};
};

struct decision_step_result_t {
  risk_gate::risk_gate_result_t risk_gate{};
  TargetPortfolio target{};
  execution::spot_rebalance_plan_t rebalance_plan{};
  monitoring::belief_decision_report_t report{};
  bool valid{false};
  decision_diagnostics_t diagnostics{};
};

namespace detail {

inline void
append_gate_diagnostics(TargetPortfolio &target,
                        const risk_gate::risk_gate_result_t &risk_gate) {
  if (risk_gate.force_base_reserve_fallback) {
    target.diagnostics.notes.push_back("risk_gate.force_base_reserve_fallback");
  } else {
    target.diagnostics.notes.push_back("risk_gate.allow_trading");
  }
  target.diagnostics.failures.insert(target.diagnostics.failures.end(),
                                     risk_gate.reasons.begin(),
                                     risk_gate.reasons.end());
  target.diagnostics.warnings.insert(target.diagnostics.warnings.end(),
                                     risk_gate.warnings.begin(),
                                     risk_gate.warnings.end());
}

[[nodiscard]] inline execution::spot_rebalance_plan_t
make_unrouted_plan(const belief::AllocationBelief &belief_state,
                   const PortfolioState &portfolio_state,
                   std::string failure_reason) {
  execution::spot_rebalance_plan_t plan{};
  plan.timestamp_ms = belief_state.timestamp_ms;
  plan.base_reserve_node_id = portfolio_state.reserve_node_id;
  plan.node_ids = belief_state.node_ids;
  plan.valid = false;
  plan.diagnostics.failures.push_back(std::move(failure_reason));
  return plan;
}

inline void build_report(decision_step_result_t &result,
                         const belief::AllocationBelief &belief_state,
                         const PortfolioState &portfolio_state) {
  result.report = monitoring::make_report({
      .belief_state = &belief_state,
      .portfolio_state = &portfolio_state,
      .target = &result.target,
      .rebalance_plan = &result.rebalance_plan,
  });
}

} // namespace detail

[[nodiscard]] inline decision_step_result_t
run(const belief::AllocationBelief &belief_state,
    const PortfolioState &portfolio_state, const MarketState &market_state,
    const PortfolioConstraints &constraints,
    const execution::spot_edge_market_state_t &edge_market_state,
    const decision_step_options_t &options = {}) {
  decision_step_result_t result{};
  result.risk_gate = risk_gate::evaluate(belief_state, portfolio_state,
                                         options.risk_thresholds);

  try {
    if (result.risk_gate.force_base_reserve_fallback) {
      result.target = base_reserve_fallback::solve(
          belief_state, portfolio_state, constraints,
          result.risk_gate.fallback_mode);
    } else {
      result.target =
          cuwacunu::wikimyei::engine::portfolio::spot_distributional_utility::
              solve(belief_state, portfolio_state, market_state, constraints,
                    options.solver_options);
    }
    detail::append_gate_diagnostics(result.target, result.risk_gate);
  } catch (const std::exception &ex) {
    result.target.timestamp_ms = belief_state.timestamp_ms;
    result.target.node_ids = belief_state.node_ids;
    result.target.valid = false;
    result.target.diagnostics.failures.push_back(
        std::string("portfolio_target_failed: ") + ex.what());
    result.diagnostics.failures.insert(
        result.diagnostics.failures.end(),
        result.target.diagnostics.failures.begin(),
        result.target.diagnostics.failures.end());
    result.rebalance_plan = detail::make_unrouted_plan(
        belief_state, portfolio_state, "target_invalid_no_route");
    detail::build_report(result, belief_state, portfolio_state);
    return result;
  }

  try {
    if (!result.target.valid) {
      result.rebalance_plan = detail::make_unrouted_plan(
          belief_state, portfolio_state, "target_invalid_no_route");
    } else {
      result.rebalance_plan =
          execution::plan_rebalance(result.target, portfolio_state,
                                    edge_market_state, options.router_options);
    }
  } catch (const std::exception &ex) {
    result.rebalance_plan = detail::make_unrouted_plan(
        belief_state, portfolio_state,
        std::string("rebalance_route_failed: ") + ex.what());
  }

  result.valid = result.target.valid && result.rebalance_plan.valid;
  if (!result.target.valid) {
    result.diagnostics.failures.push_back("target_invalid");
  }
  if (!result.rebalance_plan.valid) {
    result.diagnostics.failures.push_back("rebalance_plan_invalid");
  }
  if (result.valid) {
    result.diagnostics.notes.push_back(
        "wikimyei.engine.portfolio."
        "spot_distributional_utility.decision_step.v1");
  }

  detail::build_report(result, belief_state, portfolio_state);
  return result;
}

} // namespace
  // cuwacunu::wikimyei::engine::portfolio::spot_distributional_utility::decision_step
