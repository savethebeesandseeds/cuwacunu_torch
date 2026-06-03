// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include <torch/torch.h>

#include "wikimyei/observer/belief/types.h"
#include "wikimyei/observer/utility/calibration.h"
#include "wikimyei/observer/utility/surprise.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/types.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/utility/portfolio_ledger.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/utility/spot_rebalance_router.h"

namespace cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::
    monitoring::belief_reporter {

namespace accounting = cuwacunu::wikimyei::policy::portfolio::
    spot_distributional_utility::accounting::portfolio_ledger;
namespace execution = cuwacunu::wikimyei::policy::portfolio::
    spot_distributional_utility::execution::spot_rebalance_router;
namespace portfolio = cuwacunu::wikimyei::policy::portfolio;
namespace belief = cuwacunu::wikimyei::observer::belief;
namespace observer = cuwacunu::wikimyei::observer;

struct belief_report_inputs_t {
  const belief::AllocationBelief *belief_state{nullptr};
  const portfolio::PortfolioState *portfolio_state{nullptr};
  const portfolio::TargetPortfolio *target{nullptr};
  const execution::spot_rebalance_plan_t *rebalance_plan{nullptr};
  const accounting::portfolio_ledger_t *ledger{nullptr};
  const observer::surprise_t *surprise_result{nullptr};
  const observer::calibration_summary_t *calibration_summary{nullptr};
};

struct belief_decision_report_t {
  std::string schema_id{
      "wikimyei.policy.portfolio.spot_distributional_utility.monitoring."
      "belief_reporter.v1"};
  std::int64_t schema_version{1};
  std::string text{};
};

namespace detail {

[[nodiscard]] inline double tensor_mean_or_nan(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  auto x = tensor.detach().to(torch::kFloat64);
  auto finite = torch::isfinite(x);
  if (!finite.any().item<bool>()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return x.index({finite}).mean().item<double>();
}

[[nodiscard]] inline double tensor_min_or_nan(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  auto x = tensor.detach().to(torch::kFloat64);
  auto finite = torch::isfinite(x);
  if (!finite.any().item<bool>()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return x.index({finite}).min().item<double>();
}

[[nodiscard]] inline double tensor_max_or_nan(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  auto x = tensor.detach().to(torch::kFloat64);
  auto finite = torch::isfinite(x);
  if (!finite.any().item<bool>()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return x.index({finite}).max().item<double>();
}

inline void append_double(std::ostringstream &oss, const std::string &key,
                          double value) {
  oss << key << "=";
  if (std::isfinite(value)) {
    oss << std::setprecision(17) << value;
  } else {
    oss << "nan";
  }
  oss << "\n";
}

inline void append_tensor_summary(std::ostringstream &oss,
                                  const std::string &prefix,
                                  const torch::Tensor &tensor) {
  oss << prefix << "_defined=" << (tensor.defined() ? "true" : "false") << "\n";
  if (!tensor.defined()) {
    return;
  }
  oss << prefix << "_numel=" << tensor.numel() << "\n";
  append_double(oss, prefix + "_mean", tensor_mean_or_nan(tensor));
  append_double(oss, prefix + "_min", tensor_min_or_nan(tensor));
  append_double(oss, prefix + "_max", tensor_max_or_nan(tensor));
}

inline void append_diagnostics(std::ostringstream &oss,
                               const std::string &prefix,
                               const portfolio::decision_diagnostics_t &diag) {
  oss << prefix << "_notes=" << diag.notes.size() << "\n";
  oss << prefix << "_warnings=" << diag.warnings.size() << "\n";
  oss << prefix << "_failures=" << diag.failures.size() << "\n";
}

} // namespace detail

[[nodiscard]] inline belief_decision_report_t
make_report(const belief_report_inputs_t &inputs) {
  std::ostringstream oss;
  belief_decision_report_t report{};
  oss << "report_schema_id=" << report.schema_id << "\n";
  oss << "report_schema_version=" << report.schema_version << "\n";

  if (inputs.belief_state != nullptr) {
    const auto &b = *inputs.belief_state;
    oss << "belief_present=true\n";
    oss << "anchor_key=" << b.anchor_key << "\n";
    oss << "timestamp_ms=" << b.timestamp_ms << "\n";
    oss << "accounting_numeraire_id=" << b.base_policy.accounting_numeraire_id
        << "\n";
    oss << "settlement_asset_id=" << b.base_policy.settlement_asset_id << "\n";
    oss << "reserve_asset_id=" << b.base_policy.reserve_asset_id << "\n";
    oss << "projection_reference_node_id="
        << b.base_policy.projection_reference_node_id << "\n";
    oss << "risky_asset_count=" << b.node_ids.size() << "\n";
    oss << "scenario_count=" << belief::scenario_count(b) << "\n";
    oss << "belief_valid=" << (b.valid ? "true" : "false") << "\n";
    oss << "belief_projection_validation_required="
        << (b.projection_validation_required ? "true" : "false") << "\n";
    oss << "belief_projection_validated="
        << (b.projection_validated ? "true" : "false") << "\n";
    oss << "belief_live_capital_allowed="
        << (b.live_capital_allowed ? "true" : "false") << "\n";
    oss << "graph_order_fingerprint=" << b.graph_order_fingerprint << "\n";
    oss << "graph_node_axis_name=" << b.graph_node_axis.axis_name << "\n";
    oss << "graph_node_axis_coordinate_space="
        << b.graph_node_axis.coordinate_space << "\n";
    oss << "graph_node_axis_node_count=" << b.graph_node_axis.node_ids.size()
        << "\n";
    detail::append_tensor_summary(oss, "belief_expected_arithmetic_return",
                                  b.expected_arithmetic_return);
    detail::append_tensor_summary(oss, "belief_confidence", b.confidence);
    detail::append_tensor_summary(oss, "belief_marginal_volatility",
                                  b.marginal_volatility);
    detail::append_tensor_summary(oss, "belief_residual_quality_score",
                                  b.residual_quality_score);
    detail::append_tensor_summary(oss, "belief_projection_validation_score",
                                  b.projection_validation_score);
    detail::append_tensor_summary(oss, "belief_scenarios", b.scenarios);
  } else {
    oss << "belief_present=false\n";
  }

  if (inputs.portfolio_state != nullptr) {
    const auto &p = *inputs.portfolio_state;
    oss << "portfolio_present=true\n";
    oss << "portfolio_accounting_node_id=" << p.accounting_node_id << "\n";
    oss << "portfolio_reserve_node_id=" << p.reserve_node_id << "\n";
    detail::append_double(oss, "portfolio_base_reserve_weight",
                          p.base_reserve_weight);
    detail::append_double(oss, "portfolio_equity_value_base",
                          p.equity_value_base);
    detail::append_double(oss, "portfolio_drawdown", p.drawdown);
    detail::append_tensor_summary(oss, "portfolio_current_weights",
                                  p.current_weights);
    detail::append_tensor_summary(oss, "portfolio_unrealized_pnl",
                                  p.unrealized_pnl);
  } else {
    oss << "portfolio_present=false\n";
  }

  if (inputs.target != nullptr) {
    const auto &t = *inputs.target;
    oss << "target_present=true\n";
    oss << "target_valid=" << (t.valid ? "true" : "false") << "\n";
    oss << "target_asset_count=" << t.node_ids.size() << "\n";
    oss << "target_base_reserve_node_id=" << t.base_reserve_node_id << "\n";
    detail::append_double(oss, "target_base_reserve_weight",
                          t.target_base_reserve_weight);
    detail::append_double(oss, "target_expected_log_growth",
                          t.expected_log_growth);
    detail::append_double(oss, "target_expected_arithmetic_return",
                          t.expected_arithmetic_return);
    detail::append_double(oss, "target_cvar_loss", t.cvar_loss);
    detail::append_double(oss, "target_turnover", t.turnover);
    detail::append_double(oss, "target_estimated_transaction_cost",
                          t.estimated_transaction_cost);
    detail::append_tensor_summary(oss, "target_weights", t.target_weights);
    detail::append_tensor_summary(oss, "target_delta_weights", t.delta_weights);
    detail::append_diagnostics(oss, "target_diagnostics", t.diagnostics);
  } else {
    oss << "target_present=false\n";
  }

  if (inputs.rebalance_plan != nullptr) {
    const auto &r = *inputs.rebalance_plan;
    oss << "rebalance_present=true\n";
    oss << "rebalance_valid=" << (r.valid ? "true" : "false") << "\n";
    oss << "rebalance_order_count=" << r.orders.size() << "\n";
    oss << "rebalance_skipped_count=" << r.skipped.size() << "\n";
    detail::append_double(oss, "rebalance_requested_turnover_weight",
                          r.requested_turnover_weight);
    detail::append_double(oss, "rebalance_routed_turnover_weight",
                          r.routed_turnover_weight);
    detail::append_double(oss, "rebalance_requested_notional_base",
                          r.requested_notional_base);
    detail::append_double(oss, "rebalance_routed_notional_base",
                          r.routed_notional_base);
    detail::append_double(oss, "rebalance_estimated_transaction_cost_base",
                          r.estimated_transaction_cost_base);
    detail::append_diagnostics(oss, "rebalance_diagnostics", r.diagnostics);
  } else {
    oss << "rebalance_present=false\n";
  }

  if (inputs.ledger != nullptr) {
    const auto &l = *inputs.ledger;
    oss << "ledger_present=true\n";
    oss << "ledger_base_reserve_node_id=" << l.base_reserve_node_id << "\n";
    oss << "ledger_fill_count=" << l.fill_count << "\n";
    detail::append_double(oss, "ledger_base_reserve_units",
                          l.base_reserve_units);
    detail::append_double(oss, "ledger_cumulative_fees_base",
                          l.cumulative_fees_base);
    detail::append_tensor_summary(oss, "ledger_units", l.units);
    detail::append_tensor_summary(oss, "ledger_realized_pnl",
                                  l.realized_pnl_base);
  } else {
    oss << "ledger_present=false\n";
  }

  if (inputs.surprise_result != nullptr) {
    const auto &s = *inputs.surprise_result;
    oss << "surprise_present=true\n";
    detail::append_tensor_summary(oss, "surprise_nll", s.surprise);
    detail::append_tensor_summary(oss, "surprise_score", s.score);
  } else {
    oss << "surprise_present=false\n";
  }

  if (inputs.calibration_summary != nullptr) {
    const auto &c = *inputs.calibration_summary;
    oss << "calibration_present=true\n";
    detail::append_tensor_summary(oss, "calibration_observation_count",
                                  c.observation_count);
    detail::append_tensor_summary(oss, "calibration_mean_nll", c.mean_nll);
    detail::append_tensor_summary(oss, "calibration_pit_mean", c.pit_mean);
    detail::append_tensor_summary(oss, "calibration_score", c.score);
  } else {
    oss << "calibration_present=false\n";
  }

  report.text = oss.str();
  return report;
}

} // namespace
  // cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::monitoring::belief_reporter
