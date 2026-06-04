// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "kikijyeba/environment/control/interfaces.h"

namespace cuwacunu::kikijyeba::environment {

struct episode_runner_options_t {
  std::uint64_t max_steps{0}; // 0 means derive from EpisodeSpec range.
  bool validate_policy_actions{true};
  bool require_full_accepted_range{true};
  reward_options_t reward_options{};
};

namespace episode_runner_detail {

[[nodiscard]] inline double mean_or_nan(const std::vector<double> &values) {
  if (values.empty()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  double sum = 0.0;
  for (const double value : values) {
    sum += value;
  }
  return sum / static_cast<double>(values.size());
}

[[nodiscard]] inline double rmse_or_nan(const std::vector<double> &values) {
  if (values.empty()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  double sum = 0.0;
  for (const double value : values) {
    sum += value * value;
  }
  return std::sqrt(sum / static_cast<double>(values.size()));
}

[[nodiscard]] inline double
population_std_or_nan(const std::vector<double> &values) {
  if (values.empty()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const double mean = mean_or_nan(values);
  double sum = 0.0;
  for (const double value : values) {
    const double centered = value - mean;
    sum += centered * centered;
  }
  return std::sqrt(sum / static_cast<double>(values.size()));
}

[[nodiscard]] inline double
correlation_or_zero(const std::vector<double> &lhs,
                    const std::vector<double> &rhs) {
  constexpr double kEps = 1.0e-12;
  if (lhs.size() != rhs.size() || lhs.size() < 2U) {
    return 0.0;
  }
  const double lhs_mean = mean_or_nan(lhs);
  const double rhs_mean = mean_or_nan(rhs);
  double numerator = 0.0;
  double lhs_ss = 0.0;
  double rhs_ss = 0.0;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    const double lx = lhs[i] - lhs_mean;
    const double ry = rhs[i] - rhs_mean;
    numerator += lx * ry;
    lhs_ss += lx * lx;
    rhs_ss += ry * ry;
  }
  const double denominator = std::sqrt(lhs_ss * rhs_ss);
  if (denominator <= kEps) {
    return 0.0;
  }
  return numerator / denominator;
}

[[nodiscard]] inline int sign_of(double value) {
  if (value > 0.0) {
    return 1;
  }
  if (value < 0.0) {
    return -1;
  }
  return 0;
}

[[nodiscard]] inline double
directional_accuracy_or_nan(const std::vector<double> &predicted,
                            const std::vector<double> &realized) {
  if (predicted.size() != realized.size() || predicted.empty()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  double correct = 0.0;
  for (std::size_t i = 0; i < predicted.size(); ++i) {
    correct += sign_of(predicted[i]) == sign_of(realized[i]) ? 1.0 : 0.0;
  }
  return correct / static_cast<double>(predicted.size());
}

[[nodiscard]] inline double safe_skill(double model_error,
                                       double baseline_error) {
  constexpr double kEps = 1.0e-12;
  if (!std::isfinite(model_error) || !std::isfinite(baseline_error) ||
      baseline_error <= kEps) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return 1.0 - (model_error / baseline_error);
}

inline void merge_failures(episode_report_t &report,
                           const std::vector<std::string> &failures) {
  report.failures.insert(report.failures.end(), failures.begin(),
                         failures.end());
}

inline void merge_warnings(episode_report_t &report,
                           const std::vector<std::string> &warnings) {
  report.warnings.insert(report.warnings.end(), warnings.begin(),
                         warnings.end());
}

[[nodiscard]] inline std::string
join_diagnostics(const std::vector<std::string> &values) {
  if (values.empty()) {
    return "none";
  }
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << values[i];
  }
  return out.str();
}

[[nodiscard]] inline std::string
join_tokens(const std::vector<std::string> &values) {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << values[i];
  }
  return out.str();
}

[[nodiscard]] inline std::vector<std::string>
split_tokens(const std::string &text) {
  std::vector<std::string> out;
  if (text.empty()) {
    return out;
  }
  std::string current;
  for (const char ch : text) {
    if (ch == ',') {
      out.push_back(current);
      current.clear();
      continue;
    }
    current.push_back(ch);
  }
  out.push_back(current);
  return out;
}

[[nodiscard]] inline std::string
format_node_weights(const std::vector<std::string> &node_ids,
                    const torch::Tensor &weights) {
  if (!weights.defined()) {
    return {};
  }
  auto values = weights.to(torch::kFloat64).contiguous().view({-1});
  std::ostringstream out;
  for (std::int64_t i = 0; i < values.numel(); ++i) {
    if (i != 0) {
      out << ",";
    }
    if (static_cast<std::size_t>(i) < node_ids.size()) {
      out << node_ids[static_cast<std::size_t>(i)];
    } else {
      out << "asset_" << i;
    }
    out << ":" << values.index({i}).item<double>();
  }
  return out.str();
}

[[nodiscard]] inline std::string
format_node_scalars(const std::vector<std::string> &node_ids,
                    const torch::Tensor &values) {
  if (!values.defined()) {
    return {};
  }
  auto scalars = values.to(torch::kFloat64).contiguous().view({-1});
  std::ostringstream out;
  for (std::int64_t i = 0; i < scalars.numel(); ++i) {
    if (i != 0) {
      out << ",";
    }
    if (static_cast<std::size_t>(i) < node_ids.size()) {
      out << node_ids[static_cast<std::size_t>(i)];
    } else {
      out << "asset_" << i;
    }
    out << ":" << scalars.index({i}).item<double>();
  }
  return out.str();
}

[[nodiscard]] inline std::string
format_node_metric_vector(const std::vector<std::string> &node_ids,
                          const std::vector<double> &values) {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    if (i < node_ids.size()) {
      out << node_ids[i];
    } else {
      out << "asset_" << i;
    }
    out << ":" << values[i];
  }
  return out.str();
}

[[nodiscard]] inline bool starts_with(const std::string &text,
                                      const std::string &prefix) {
  return text.rfind(prefix, 0) == 0;
}

[[nodiscard]] inline std::uint64_t
count_rejected_fill_reason(const cajtucu_execution::execution_trace_t &trace,
                           const std::string &reason) {
  std::uint64_t out = 0;
  for (const auto &fill : trace.fills) {
    if (fill.status == cajtucu_execution::fill_status_t::rejected &&
        fill.reject_reason == reason) {
      ++out;
    }
  }
  return out;
}

[[nodiscard]] inline std::uint64_t
count_failure_prefix(const cajtucu_execution::execution_trace_t &trace,
                     const std::string &prefix) {
  std::uint64_t out = 0;
  for (const auto &failure : trace.failures) {
    if (starts_with(failure, prefix)) {
      ++out;
    }
  }
  return out;
}

[[nodiscard]] inline double
rejected_notional_base(const cajtucu_execution::execution_trace_t &trace) {
  double out = 0.0;
  const auto count = std::min(trace.orders.size(), trace.fills.size());
  for (std::size_t i = 0; i < count; ++i) {
    if (trace.fills[i].status == cajtucu_execution::fill_status_t::rejected) {
      out += std::max(0.0, trace.orders[i].requested_notional_base);
    }
  }
  return out;
}

[[nodiscard]] inline double
partial_notional_base(const cajtucu_execution::execution_trace_t &trace) {
  double out = 0.0;
  for (const auto &fill : trace.fills) {
    if (fill.status == cajtucu_execution::fill_status_t::partially_filled) {
      out += std::max(0.0, fill.gross_notional_base);
    }
  }
  return out;
}

[[nodiscard]] inline std::uint64_t
executed_order_count(const cajtucu_execution::execution_trace_t &trace) {
  std::uint64_t out = 0;
  for (const auto &fill : trace.fills) {
    if (fill.status == cajtucu_execution::fill_status_t::filled ||
        fill.status == cajtucu_execution::fill_status_t::partially_filled) {
      ++out;
    }
  }
  return out;
}

[[nodiscard]] inline std::uint64_t
negative_unit_count(const torch::Tensor &units, double tolerance = 1.0e-10) {
  if (!units.defined() || units.numel() == 0) {
    return 0;
  }
  auto flat = units.to(torch::kFloat64).cpu().contiguous().view({-1});
  std::uint64_t out = 0;
  for (std::int64_t i = 0; i < flat.numel(); ++i) {
    if (flat.index({i}).item<double>() < -tolerance) {
      ++out;
    }
  }
  return out;
}

struct node_projection_series_t {
  std::vector<double> predicted{};
  std::vector<double> realized{};
  std::vector<double> error{};
  std::vector<double> abs_error{};
  std::vector<double> squared_error{};
  std::vector<double> interval_hit{};
  std::vector<double> interval_width{};
};

inline void append_node_projection_samples(
    std::vector<node_projection_series_t> &series,
    const observer::projection_validation_t &projection_validation) {
  if (!projection_validation.available ||
      !projection_validation.predicted_log_return.defined() ||
      !projection_validation.realized_log_return.defined() ||
      !projection_validation.error.defined() ||
      !projection_validation.abs_error.defined()) {
    return;
  }
  const auto A = static_cast<std::int64_t>(series.size());
  if (A <= 0) {
    return;
  }
  auto predicted =
      projection_validation.predicted_log_return.to(torch::kFloat64)
          .cpu()
          .contiguous()
          .view({-1});
  auto realized = projection_validation.realized_log_return.to(torch::kFloat64)
                      .cpu()
                      .contiguous()
                      .view({-1});
  auto error = projection_validation.error.to(torch::kFloat64)
                   .cpu()
                   .contiguous()
                   .view({-1});
  auto abs_error = projection_validation.abs_error.to(torch::kFloat64)
                       .cpu()
                       .contiguous()
                       .view({-1});
  auto squared_error =
      projection_validation.squared_error.defined()
          ? projection_validation.squared_error.to(torch::kFloat64)
                .cpu()
                .contiguous()
                .view({-1})
          : error.pow(2);
  auto active =
      projection_validation.active_mask.defined()
          ? projection_validation.active_mask.to(torch::kBool)
                .cpu()
                .contiguous()
                .view({-1})
          : torch::ones({A}, torch::TensorOptions().dtype(torch::kBool));
  auto interval_hit =
      projection_validation.interval_hit.defined()
          ? projection_validation.interval_hit.to(torch::kFloat64)
                .cpu()
                .contiguous()
                .view({-1})
          : torch::full({A}, std::numeric_limits<double>::quiet_NaN(),
                        torch::TensorOptions().dtype(torch::kFloat64));
  auto interval_width =
      projection_validation.interval_width.defined()
          ? projection_validation.interval_width.to(torch::kFloat64)
                .cpu()
                .contiguous()
                .view({-1})
          : torch::full({A}, std::numeric_limits<double>::quiet_NaN(),
                        torch::TensorOptions().dtype(torch::kFloat64));

  const auto count = std::min<std::int64_t>(
      {A, predicted.numel(), realized.numel(), error.numel(), abs_error.numel(),
       squared_error.numel(), active.numel()});
  for (std::int64_t i = 0; i < count; ++i) {
    if (!active.index({i}).item<bool>()) {
      continue;
    }
    const double p = predicted.index({i}).item<double>();
    const double r = realized.index({i}).item<double>();
    const double e = error.index({i}).item<double>();
    const double ae = abs_error.index({i}).item<double>();
    const double se = squared_error.index({i}).item<double>();
    if (!std::isfinite(p) || !std::isfinite(r) || !std::isfinite(e) ||
        !std::isfinite(ae) || !std::isfinite(se)) {
      continue;
    }
    auto &node = series[static_cast<std::size_t>(i)];
    node.predicted.push_back(p);
    node.realized.push_back(r);
    node.error.push_back(e);
    node.abs_error.push_back(ae);
    node.squared_error.push_back(se);
    if (i < interval_hit.numel()) {
      const double hit = interval_hit.index({i}).item<double>();
      if (std::isfinite(hit)) {
        node.interval_hit.push_back(hit);
      }
    }
    if (i < interval_width.numel()) {
      const double width = interval_width.index({i}).item<double>();
      if (std::isfinite(width)) {
        node.interval_width.push_back(width);
      }
    }
  }
}

[[nodiscard]] inline bool approx_equal(double actual, double expected,
                                       double tolerance = 1.0e-8) {
  return std::abs(actual - expected) <=
         tolerance * std::max(1.0, std::abs(expected));
}

inline void validate_reward_evidence(const reward_t &reward) {
  if (!std::isfinite(reward.log_growth_reward) ||
      !std::isfinite(reward.drawdown_penalty) ||
      !std::isfinite(reward.transaction_cost_penalty) ||
      !std::isfinite(reward.turnover_penalty) ||
      !std::isfinite(reward.invalid_action_penalty) ||
      !std::isfinite(reward.total)) {
    throw std::runtime_error(
        "[episode_runner] transition reward fields must be finite");
  }
  const double expected_total =
      reward.log_growth_reward - reward.drawdown_penalty -
      reward.transaction_cost_penalty - reward.turnover_penalty -
      reward.invalid_action_penalty;
  if (!approx_equal(reward.total, expected_total)) {
    throw std::runtime_error(
        "[episode_runner] transition reward total does not match reward "
        "components");
  }
}

inline void validate_reward_evidence_matches_transition(
    const reward_t &reward, const step_info_t &info,
    const reward_options_t &reward_options) {
  validate_reward_evidence(reward);
  const auto expected =
      compute_reward(info.portfolio_before.equity_value_base,
                     info.portfolio_after.equity_value_base,
                     info.portfolio_after.drawdown, info.transaction_cost_base,
                     info.turnover, info.invalid_action, reward_options);
  if (!approx_equal(reward.log_growth_reward, expected.log_growth_reward) ||
      !approx_equal(reward.drawdown_penalty, expected.drawdown_penalty) ||
      !approx_equal(reward.transaction_cost_penalty,
                    expected.transaction_cost_penalty) ||
      !approx_equal(reward.turnover_penalty, expected.turnover_penalty) ||
      !approx_equal(reward.invalid_action_penalty,
                    expected.invalid_action_penalty) ||
      !approx_equal(reward.total, expected.total)) {
    throw std::runtime_error(
        "[episode_runner] transition reward does not match portfolio, cost, "
        "turnover, drawdown, and invalid-action evidence");
  }
}

inline void
validate_policy_action_identity(const action_t &action,
                                const policy_adapter_iface_t &policy) {
  if (action.policy_id != policy.policy_id()) {
    throw std::runtime_error(
        "[episode_runner] action policy_id must match policy adapter id");
  }
  if (action.policy_kind != policy.policy_kind()) {
    throw std::runtime_error(
        "[episode_runner] action policy_kind must match policy adapter kind");
  }
}

inline void validate_projection_validation_evidence(
    const observer::projection_validation_t &projection_validation,
    const episode_spec_t &spec) {
  constexpr double kMetricTolerance = 1.0e-8;
  if (!projection_validation.available) {
    if (spec.require_projection_validation) {
      throw std::runtime_error(
          "[episode_runner] EpisodeSpec requires projection validation "
          "evidence");
    }
    return;
  }
  if (!std::isfinite(projection_validation.signed_bias) ||
      !std::isfinite(projection_validation.mae) ||
      !std::isfinite(projection_validation.rmse) ||
      !std::isfinite(projection_validation.directional_accuracy) ||
      !std::isfinite(projection_validation.correlation) ||
      !std::isfinite(projection_validation.interval_coverage) ||
      !std::isfinite(projection_validation.mean_interval_width) ||
      !std::isfinite(projection_validation.zero_baseline_signed_bias) ||
      !std::isfinite(projection_validation.zero_baseline_mae) ||
      !std::isfinite(projection_validation.zero_baseline_rmse) ||
      !std::isfinite(
          projection_validation.zero_baseline_directional_accuracy) ||
      !std::isfinite(projection_validation.model_skill_vs_zero_mae) ||
      !std::isfinite(projection_validation.model_skill_vs_zero_rmse)) {
    throw std::runtime_error(
        "[episode_runner] projection validation metrics must be finite");
  }
  if (projection_validation.mae < -kMetricTolerance ||
      projection_validation.rmse < -kMetricTolerance ||
      projection_validation.mean_interval_width < -kMetricTolerance ||
      projection_validation.zero_baseline_mae < -kMetricTolerance ||
      projection_validation.zero_baseline_rmse < -kMetricTolerance) {
    throw std::runtime_error(
        "[episode_runner] projection validation error metrics must be "
        "nonnegative");
  }
  if (projection_validation.rmse + kMetricTolerance <
          std::abs(projection_validation.signed_bias) ||
      projection_validation.rmse + kMetricTolerance <
          projection_validation.mae) {
    throw std::runtime_error(
        "[episode_runner] projection validation RMSE must be at least MAE and "
        "absolute signed bias");
  }
  if (projection_validation.directional_accuracy < -kMetricTolerance ||
      projection_validation.directional_accuracy > 1.0 + kMetricTolerance ||
      projection_validation.interval_coverage < -kMetricTolerance ||
      projection_validation.interval_coverage > 1.0 + kMetricTolerance ||
      projection_validation.zero_baseline_directional_accuracy <
          -kMetricTolerance ||
      projection_validation.zero_baseline_directional_accuracy >
          1.0 + kMetricTolerance) {
    throw std::runtime_error(
        "[episode_runner] projection validation probability metrics must be "
        "within [0,1]");
  }
  if (projection_validation.correlation < -1.0 - kMetricTolerance ||
      projection_validation.correlation > 1.0 + kMetricTolerance) {
    throw std::runtime_error(
        "[episode_runner] projection validation correlation must be within "
        "[-1,1]");
  }
  if (spec.require_projection_validation && !projection_validation.valid) {
    throw std::runtime_error(
        "[episode_runner] required projection validation evidence is invalid");
  }
}

inline void
validate_portfolio_state_for_episode(const portfolio::PortfolioState &state,
                                     const episode_spec_t &spec,
                                     const char *context) {
  const auto A = static_cast<std::int64_t>(spec.risky_node_ids.size());
  portfolio::validate_portfolio_state(state, A);
  if (state.accounting_node_id != spec.base_policy.accounting_numeraire_id) {
    throw std::runtime_error(std::string("[episode_runner] ") + context +
                             " accounting node does not match EpisodeSpec "
                             "BasePolicy");
  }
  if (state.reserve_node_id != spec.base_policy.reserve_asset_id) {
    throw std::runtime_error(std::string("[episode_runner] ") + context +
                             " reserve node does not match EpisodeSpec "
                             "BasePolicy");
  }
}

inline void
validate_observation_state_timestamps(const observation_t &observation) {
  if (observation.portfolio_state.timestamp_ms > 0 &&
      observation.portfolio_state.timestamp_ms >
          observation.knowledge_timestamp_ms) {
    throw std::runtime_error(
        "[episode_runner] observation portfolio_state timestamp is after "
        "knowledge timestamp");
  }
  if (observation.market_state.timestamp_ms > 0 &&
      observation.market_state.timestamp_ms >
          observation.knowledge_timestamp_ms) {
    throw std::runtime_error(
        "[episode_runner] observation market_state timestamp is after "
        "knowledge timestamp");
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

inline void validate_edge_market_state_for_episode(
    const execution::spot_edge_market_state_t &market_state,
    const episode_spec_t &spec) {
  if (edge_market_graph_is_empty(market_state)) {
    return;
  }
  execution::validate_spot_edge_market_state(market_state);
  for (const auto &node_id : spec.risky_node_ids) {
    if (!edge_market_graph_contains_node(market_state, node_id)) {
      throw std::runtime_error("[episode_runner] observation edge market graph "
                               "missing risky node: " +
                               node_id);
    }
  }
  const auto &reserve_node_id = spec.base_policy.reserve_asset_id;
  if (!edge_market_graph_contains_node(market_state, reserve_node_id)) {
    throw std::runtime_error("[episode_runner] observation edge market graph "
                             "missing base reserve node: " +
                             reserve_node_id);
  }
}

[[nodiscard]] inline bool
rebalance_plan_has_payload(const execution::spot_rebalance_plan_t &plan) {
  return plan.valid || !plan.base_reserve_node_id.empty() ||
         !plan.node_ids.empty() || !plan.orders.empty() ||
         !plan.skipped.empty() || plan.requested_turnover_weight != 0.0 ||
         plan.routed_turnover_weight != 0.0 ||
         plan.requested_notional_base != 0.0 ||
         plan.routed_notional_base != 0.0 ||
         plan.estimated_transaction_cost_base != 0.0 ||
         !plan.diagnostics.notes.empty() ||
         !plan.diagnostics.warnings.empty() ||
         !plan.diagnostics.failures.empty();
}

inline void validate_execution_timestamp(portfolio::timestamp_ms_t timestamp_ms,
                                         const observation_t &observation,
                                         const char *context,
                                         bool required = true) {
  if (timestamp_ms <= 0) {
    if (required) {
      throw std::runtime_error(std::string("[episode_runner] ") + context +
                               " timestamp is required");
    }
    return;
  }
  if (timestamp_ms < observation.knowledge_timestamp_ms ||
      timestamp_ms >= observation.realization_available_after_timestamp_ms) {
    throw std::runtime_error(std::string("[episode_runner] ") + context +
                             " timestamp must be after knowledge and before "
                             "realization availability");
  }
}

inline void validate_rebalance_plan_evidence(
    const execution::spot_rebalance_plan_t &plan,
    const std::string &rebalance_plan_source, bool rebalance_plan_enforced,
    const episode_spec_t &spec, const observation_t &observation,
    bool allow_invalid_rebalance_plan = false) {
  constexpr double kTolerance = 1.0e-8;
  const bool has_payload = rebalance_plan_has_payload(plan);
  if (!has_payload && !rebalance_plan_enforced &&
      rebalance_plan_source.empty()) {
    return;
  }
  if (rebalance_plan_source.empty()) {
    throw std::runtime_error(
        "[episode_runner] transition rebalance_plan_source is required when "
        "rebalance evidence is present");
  }
  if (!plan.valid && !allow_invalid_rebalance_plan) {
    throw std::runtime_error(
        "[episode_runner] transition rebalance plan must be valid when "
        "rebalance evidence is present");
  }
  validate_execution_timestamp(plan.timestamp_ms, observation,
                               "transition rebalance plan");
  if (plan.node_ids != spec.risky_node_ids) {
    throw std::runtime_error(
        "[episode_runner] transition rebalance plan node_ids mismatch");
  }
  if (plan.base_reserve_node_id != spec.base_policy.reserve_asset_id) {
    throw std::runtime_error(
        "[episode_runner] transition rebalance plan reserve node mismatch");
  }
  if (!std::isfinite(plan.requested_turnover_weight) ||
      !std::isfinite(plan.routed_turnover_weight) ||
      !std::isfinite(plan.requested_notional_base) ||
      !std::isfinite(plan.routed_notional_base) ||
      !std::isfinite(plan.estimated_transaction_cost_base)) {
    throw std::runtime_error(
        "[episode_runner] transition rebalance plan numeric fields must be "
        "finite");
  }
  if (plan.requested_turnover_weight < -kTolerance ||
      plan.routed_turnover_weight < -kTolerance ||
      plan.requested_notional_base < -kTolerance ||
      plan.routed_notional_base < -kTolerance ||
      plan.estimated_transaction_cost_base < -kTolerance) {
    throw std::runtime_error(
        "[episode_runner] transition rebalance plan numeric fields must be "
        "nonnegative");
  }
  if (plan.routed_turnover_weight >
          plan.requested_turnover_weight + kTolerance ||
      plan.routed_notional_base > plan.requested_notional_base + kTolerance) {
    throw std::runtime_error(
        "[episode_runner] transition rebalance plan routed values must not "
        "exceed requested values");
  }

  double order_requested_notional = 0.0;
  double skipped_requested_notional = 0.0;
  double order_routed_notional = 0.0;
  double order_estimated_cost = 0.0;
  for (const auto &order : plan.orders) {
    if (detail::blank(order.node_id) ||
        !std::count(spec.risky_node_ids.begin(), spec.risky_node_ids.end(),
                    order.node_id)) {
      throw std::runtime_error(
          "[episode_runner] transition rebalance order node_id mismatch");
    }
    if (detail::blank(order.edge_id)) {
      throw std::runtime_error(
          "[episode_runner] transition rebalance order edge_id is required");
    }
    if (!std::isfinite(order.delta_weight) ||
        !std::isfinite(order.requested_notional_base) ||
        !std::isfinite(order.routed_notional_base) ||
        !std::isfinite(order.estimated_cost_base)) {
      throw std::runtime_error(
          "[episode_runner] transition rebalance order numeric fields must be "
          "finite");
    }
    if (order.requested_notional_base < -kTolerance ||
        order.routed_notional_base < -kTolerance ||
        order.estimated_cost_base < -kTolerance) {
      throw std::runtime_error(
          "[episode_runner] transition rebalance order notionals/cost must be "
          "nonnegative");
    }
    if (order.routed_notional_base >
        order.requested_notional_base + kTolerance) {
      throw std::runtime_error(
          "[episode_runner] transition rebalance order routed notional must "
          "not exceed requested notional");
    }
    order_requested_notional += order.requested_notional_base;
    order_routed_notional += order.routed_notional_base;
    order_estimated_cost += order.estimated_cost_base;
  }
  for (const auto &skipped : plan.skipped) {
    if (detail::blank(skipped.node_id) ||
        !std::count(spec.risky_node_ids.begin(), spec.risky_node_ids.end(),
                    skipped.node_id)) {
      throw std::runtime_error(
          "[episode_runner] transition rebalance skipped node_id mismatch");
    }
    if (!std::isfinite(skipped.delta_weight) ||
        !std::isfinite(skipped.requested_notional_base)) {
      throw std::runtime_error(
          "[episode_runner] transition rebalance skipped numeric fields must "
          "be finite");
    }
    if (skipped.requested_notional_base < -kTolerance) {
      throw std::runtime_error(
          "[episode_runner] transition rebalance skipped requested notional "
          "must be nonnegative");
    }
    if (detail::blank(skipped.reason)) {
      throw std::runtime_error(
          "[episode_runner] transition rebalance skipped reason is required");
    }
    skipped_requested_notional += skipped.requested_notional_base;
  }
  const bool has_route_breakdown =
      !plan.orders.empty() || !plan.skipped.empty();
  if (has_route_breakdown) {
    if (!approx_equal(plan.requested_notional_base,
                      order_requested_notional + skipped_requested_notional)) {
      throw std::runtime_error(
          "[episode_runner] transition rebalance requested notional does not "
          "match order plus skipped requested notional");
    }
    if (!approx_equal(plan.routed_notional_base, order_routed_notional)) {
      throw std::runtime_error(
          "[episode_runner] transition rebalance routed notional does not "
          "match order routed notional");
    }
    if (!approx_equal(plan.estimated_transaction_cost_base,
                      order_estimated_cost)) {
      throw std::runtime_error(
          "[episode_runner] transition rebalance estimated cost does not "
          "match order estimated cost");
    }
  }
}

inline void
validate_fill_evidence(const std::vector<accounting::executed_fill_t> &fills,
                       double transaction_cost_base, const episode_spec_t &spec,
                       const observation_t &observation) {
  constexpr double kTolerance = 1.0e-8;
  double fill_fee_base = 0.0;
  for (const auto &fill : fills) {
    validate_execution_timestamp(fill.timestamp_ms, observation,
                                 "transition fill");
    if (detail::blank(fill.node_id) ||
        !std::count(spec.risky_node_ids.begin(), spec.risky_node_ids.end(),
                    fill.node_id)) {
      throw std::runtime_error(
          "[episode_runner] transition fill node_id mismatch");
    }
    if (detail::blank(fill.edge_id)) {
      throw std::runtime_error(
          "[episode_runner] transition fill edge_id is required");
    }
    if (!std::isfinite(fill.quantity_asset) ||
        !std::isfinite(fill.gross_notional_base) ||
        !std::isfinite(fill.fee_base)) {
      throw std::runtime_error(
          "[episode_runner] transition fill numeric fields must be finite");
    }
    if (fill.quantity_asset < -kTolerance ||
        fill.gross_notional_base < -kTolerance || fill.fee_base < -kTolerance) {
      throw std::runtime_error(
          "[episode_runner] transition fill quantity/notional/fee must be "
          "nonnegative");
    }
    fill_fee_base += fill.fee_base;
  }
  if (fill_fee_base > transaction_cost_base + kTolerance) {
    throw std::runtime_error(
        "[episode_runner] transition fill fees exceed transaction cost");
  }
}

inline void validate_cajtucu_execution_trace_evidence(
    const cajtucu_execution::execution_trace_t &trace, const step_info_t &info,
    const episode_spec_t &spec, const observation_t &observation) {
  cajtucu_execution::validate_execution_trace(trace);
  if (trace.intent.environment_run_id != spec.environment_run_id ||
      trace.intent.episode_id != spec.episode_id ||
      trace.intent.anchor_key != observation.anchor_key ||
      trace.intent.node_ids != spec.risky_node_ids ||
      trace.intent.base_reserve_node_id != spec.base_policy.reserve_asset_id) {
    throw std::runtime_error(
        "[episode_runner] Cajtucu execution trace identity mismatch");
  }
  if (!approx_equal(trace.total_transaction_cost_base,
                    info.transaction_cost_base)) {
    throw std::runtime_error(
        "[episode_runner] Cajtucu execution trace cost does not match "
        "transition cost");
  }
  if (trace.ledger_before.node_ids != spec.risky_node_ids ||
      trace.ledger_after.node_ids != spec.risky_node_ids) {
    throw std::runtime_error(
        "[episode_runner] Cajtucu ledger universe does not match episode");
  }
  if (trace.market_source_id.empty()) {
    throw std::runtime_error(
        "[episode_runner] Cajtucu market source id is required");
  }
  if (trace.synthetic_direct_edges &&
      !std::count(trace.warnings.begin(), trace.warnings.end(),
                  "synthetic_direct_edges_market_source")) {
    throw std::runtime_error(
        "[episode_runner] synthetic Cajtucu market source must be warned");
  }
  if (trace.ledger_intent_equity_mismatch &&
      !std::count(trace.warnings.begin(), trace.warnings.end(),
                  "ledger_intent_equity_mismatch")) {
    throw std::runtime_error(
        "[episode_runner] Cajtucu equity mismatch must be warned");
  }
  for (const auto &fill : trace.fills) {
    if (fill.status == cajtucu_execution::fill_status_t::rejected) {
      continue;
    }
    if (fill.node_id.empty() ||
        !std::count(spec.risky_node_ids.begin(), spec.risky_node_ids.end(),
                    fill.node_id)) {
      throw std::runtime_error(
          "[episode_runner] Cajtucu fill node_id mismatch");
    }
    if (fill.edge_id.empty()) {
      throw std::runtime_error(
          "[episode_runner] Cajtucu fill edge_id is required");
    }
  }
}

inline void
validate_observation_evidence_for_policy(const observation_t &observation,
                                         const episode_spec_t &spec) {
  validate_observation_time_boundary(observation);
  if (observation.observation_anchor_index <
          spec.accepted_range.anchor_index_begin ||
      observation.observation_anchor_index >=
          spec.accepted_range.anchor_index_end) {
    throw std::runtime_error(
        "[episode_runner] policy observation is outside accepted cursor "
        "range");
  }
  const auto offset =
      static_cast<std::size_t>(observation.observation_anchor_index -
                               spec.accepted_range.anchor_index_begin);
  if (offset >= spec.accepted_range.anchor_keys.size() ||
      observation.anchor_key != spec.accepted_range.anchor_keys[offset]) {
    throw std::runtime_error(
        "[episode_runner] policy observation anchor_key does not match "
        "accepted cursor range");
  }
  if (observation.next_realization_anchor_index !=
      observation.observation_anchor_index + 1) {
    throw std::runtime_error(
        "[episode_runner] policy observation next-realization anchor must be "
        "the immediate next accepted anchor");
  }
  validate_portfolio_state_for_episode(observation.portfolio_state, spec,
                                       "observation portfolio_state");
  validate_observation_state_timestamps(observation);
  const auto A = static_cast<std::int64_t>(spec.risky_node_ids.size());
  portfolio::validate_market_state(observation.market_state, A);
  validate_edge_market_state_for_episode(observation.edge_market_state, spec);
}

inline void validate_transition_evidence_for_report(
    const observation_t &observation, const transition_t &transition,
    const episode_spec_t &spec, const reward_options_t &reward_options) {
  validate_transition_time_boundary(observation, transition);
  validate_portfolio_state_for_episode(transition.info.portfolio_before, spec,
                                       "transition portfolio_before");
  validate_portfolio_state_for_episode(transition.info.portfolio_after, spec,
                                       "transition portfolio_after");
  if (!approx_equal(transition.info.portfolio_before.equity_value_base,
                    observation.portfolio_state.equity_value_base)) {
    throw std::runtime_error(
        "[episode_runner] transition portfolio_before equity must match "
        "policy observation equity");
  }
  if (!std::isfinite(transition.info.realized_log_growth) ||
      !std::isfinite(transition.info.realized_arithmetic_return) ||
      !std::isfinite(transition.info.transaction_cost_base) ||
      !std::isfinite(transition.info.turnover)) {
    throw std::runtime_error(
        "[episode_runner] transition accounting fields must be finite");
  }
  if (transition.info.transaction_cost_base < 0.0 ||
      transition.info.turnover < 0.0) {
    throw std::runtime_error(
        "[episode_runner] transition cost and turnover must be nonnegative");
  }
  const double equity_before =
      transition.info.portfolio_before.equity_value_base;
  const double equity_after = transition.info.portfolio_after.equity_value_base;
  const double expected_log_growth = std::log(equity_after / equity_before);
  const double expected_arithmetic_return = equity_after / equity_before - 1.0;
  if (!approx_equal(transition.info.realized_log_growth, expected_log_growth) ||
      !approx_equal(transition.info.realized_arithmetic_return,
                    expected_arithmetic_return)) {
    throw std::runtime_error(
        "[episode_runner] transition realized growth does not match "
        "portfolio_before/portfolio_after equity");
  }
  if (!transition.done) {
    validate_observation_evidence_for_policy(transition.next_observation, spec);
    if (!approx_equal(
            transition.next_observation.portfolio_state.equity_value_base,
            transition.info.portfolio_after.equity_value_base)) {
      throw std::runtime_error(
          "[episode_runner] next observation equity must match transition "
          "portfolio_after equity");
    }
  }
  validate_projection_validation_evidence(transition.info.projection_validation,
                                          spec);
  const bool allow_invalid_rebalance_plan =
      transition.info.invalid_action &&
      transition.info.cajtucu_execution_trace_available &&
      !transition.info.execution_trace.valid;
  validate_rebalance_plan_evidence(
      transition.info.rebalance_plan, transition.info.rebalance_plan_source,
      transition.info.rebalance_plan_enforced, spec, observation,
      allow_invalid_rebalance_plan);
  validate_fill_evidence(transition.info.fills,
                         transition.info.transaction_cost_base, spec,
                         observation);
  if (transition.info.cajtucu_execution_trace_available) {
    validate_cajtucu_execution_trace_evidence(
        transition.info.execution_trace, transition.info, spec, observation);
  }
  const auto A = static_cast<std::int64_t>(spec.risky_node_ids.size());
  if (!transition.info.target.node_ids.empty() ||
      transition.info.target.target_weights.defined()) {
    validate_execution_timestamp(transition.info.target.timestamp_ms,
                                 observation, "transition target");
    if (transition.info.target.node_ids != spec.risky_node_ids) {
      throw std::runtime_error(
          "[episode_runner] transition target node_ids mismatch");
    }
    if (transition.info.target.base_reserve_node_id !=
        spec.base_policy.reserve_asset_id) {
      throw std::runtime_error(
          "[episode_runner] transition target reserve node mismatch");
    }
    portfolio::validate_target_portfolio(
        transition.info.target, A,
        transition.info.portfolio_before.current_weights);
  }
  validate_reward_evidence_matches_transition(transition.reward,
                                              transition.info, reward_options);
}

inline void
record_allocation_evidence(episode_report_t &report,
                           const portfolio::TargetPortfolio &target) {
  report.allocation_target_risky_node_weights =
      format_node_weights(target.node_ids, target.target_weights);
  report.allocation_reserve_node_id = target.base_reserve_node_id;
  report.allocation_reserve_weight = target.target_base_reserve_weight;
  report.allocation_turnover = target.turnover;
  report.allocation_cvar_loss = target.cvar_loss;
  report.allocation_transaction_cost_estimate =
      target.estimated_transaction_cost;

  std::ostringstream objective;
  objective << "expected_log_growth=" << target.expected_log_growth
            << ";expected_arithmetic_return="
            << target.expected_arithmetic_return
            << ";cvar_loss=" << target.cvar_loss
            << ";turnover=" << target.turnover;
  report.allocation_objective_terms = objective.str();

  report.allocation_constraint_diagnostics =
      target.valid ? "target_valid=true" : "target_valid=false";
  report.allocation_cap_diagnostics =
      "reserve_weight=" + std::to_string(target.target_base_reserve_weight);
  report.allocation_scenario_growth_floor_status =
      target.diagnostics.ok() ? "not_triggered" : "diagnostic_failure";
  report.allocation_fallback_reasons =
      join_diagnostics(target.diagnostics.failures);
  report.allocation_derisk_reasons =
      join_diagnostics(target.diagnostics.warnings);
}

[[nodiscard]] inline step_report_t
make_step_report(std::uint64_t step_index, const observation_t &observation,
                 const action_t &action, const transition_t &transition,
                 const episode_spec_t &spec) {
  step_report_t out{};
  out.step_index = step_index;
  out.episode_id = spec.episode_id;
  out.protocol_id = spec.protocol_id;
  out.runtime_run_id = spec.runtime_run_id;
  out.environment_run_id = spec.environment_run_id;
  out.policy_id = action.policy_id;
  out.method_id = action.method_id;
  out.policy_kind = action.policy_kind;
  out.world_mode = spec.world_mode;
  out.action_decision_timestamp_ms =
      resolve_action_decision_timestamp(action, observation);
  out.anchor_key = transition.info.anchor_key;
  out.observation_anchor_index = observation.observation_anchor_index;
  out.next_realization_anchor_index = observation.next_realization_anchor_index;
  out.knowledge_timestamp_ms = observation.knowledge_timestamp_ms;
  out.realization_available_after_timestamp_ms =
      observation.realization_available_after_timestamp_ms;
  out.accepted_cursor_wave_id = spec.accepted_range.cursor.wave_id;
  out.accepted_cursor_wave_mode = spec.accepted_range.cursor.wave_mode;
  out.accepted_cursor_kind = spec.accepted_range.cursor.source_cursor_kind;
  out.accepted_cursor_scope = spec.accepted_range.cursor.source_cursor_scope;
  out.accepted_source_range_policy =
      spec.accepted_range.cursor.source_range_policy;
  out.accepted_source_order_policy =
      spec.accepted_range.cursor.source_order_policy;
  out.accepted_batch_cursor_token =
      spec.accepted_range.cursor.batch_cursor_token;
  out.accepted_anchor_index_begin = spec.accepted_range.anchor_index_begin;
  out.accepted_anchor_index_end = spec.accepted_range.anchor_index_end;
  out.accepted_cursor_offset = observation.observation_anchor_index -
                               spec.accepted_range.anchor_index_begin;
  out.action_schema_id = action.action_schema_id;
  const auto &target = transition.info.target;
  const auto &node_ids =
      target.node_ids.empty() ? action.node_ids : target.node_ids;
  const auto &target_weights = target.target_weights.defined()
                                   ? target.target_weights
                                   : action.target_weights;
  out.target_risky_node_weights = format_node_weights(node_ids, target_weights);
  out.target_base_reserve_node_id = target.base_reserve_node_id.empty()
                                        ? action.base_reserve_node_id
                                        : target.base_reserve_node_id;
  out.target_base_reserve_weight =
      std::isfinite(target.target_base_reserve_weight)
          ? target.target_base_reserve_weight
          : action.target_base_reserve_weight;
  out.portfolio_equity_before =
      transition.info.portfolio_before.equity_value_base;
  out.portfolio_equity_after =
      transition.info.portfolio_after.equity_value_base;
  out.realized_log_growth = transition.info.realized_log_growth;
  out.realized_arithmetic_return = transition.info.realized_arithmetic_return;
  out.transaction_cost_base = transition.info.transaction_cost_base;
  out.turnover = transition.info.turnover;
  out.invalid_action = transition.info.invalid_action;
  out.risk_gate_evaluated = transition.info.risk_gate_evaluated;
  out.risk_gate_allow_trading = transition.info.risk_gate.allow_trading;
  out.risk_gate_force_base_reserve_fallback =
      transition.info.risk_gate.force_base_reserve_fallback;
  out.risk_gate_reason_count =
      static_cast<std::uint64_t>(transition.info.risk_gate.reasons.size());
  out.risk_gate_warning_count =
      static_cast<std::uint64_t>(transition.info.risk_gate.warnings.size());
  out.risk_gate_reasons = join_diagnostics(transition.info.risk_gate.reasons);
  out.risk_gate_warnings = join_diagnostics(transition.info.risk_gate.warnings);
  out.rebalance_plan_valid = transition.info.rebalance_plan.valid;
  out.rebalance_plan_source = transition.info.rebalance_plan_source;
  out.rebalance_plan_enforced = transition.info.rebalance_plan_enforced;
  out.execution_model = transition.info.execution_model;
  out.rebalance_order_count =
      static_cast<std::uint64_t>(transition.info.rebalance_plan.orders.size());
  out.rebalance_skipped_count =
      static_cast<std::uint64_t>(transition.info.rebalance_plan.skipped.size());
  out.rebalance_requested_turnover_weight =
      transition.info.rebalance_plan.requested_turnover_weight;
  out.rebalance_routed_turnover_weight =
      transition.info.rebalance_plan.routed_turnover_weight;
  out.rebalance_estimated_transaction_cost_base =
      transition.info.rebalance_plan.estimated_transaction_cost_base;
  out.fill_count = static_cast<std::uint64_t>(transition.info.fills.size());
  for (const auto &fill : transition.info.fills) {
    out.fill_gross_notional_base += fill.gross_notional_base;
    out.fill_fee_base += fill.fee_base;
  }
  out.cajtucu_execution_trace_available =
      transition.info.cajtucu_execution_trace_available;
  if (transition.info.cajtucu_execution_trace_available) {
    const auto &trace = transition.info.execution_trace;
    out.cajtucu_backend_id = trace.backend_id;
    out.cajtucu_trace_id = trace.trace_id;
    out.cajtucu_market_source_id = trace.market_source_id;
    out.cajtucu_synthetic_direct_edges = trace.synthetic_direct_edges;
    out.cajtucu_trace_valid = trace.valid;
    out.cajtucu_failure_count =
        static_cast<std::uint64_t>(trace.failures.size());
    out.cajtucu_ledger_intent_equity_difference_base =
        trace.ledger_intent_equity_difference_base;
    out.cajtucu_ledger_intent_equity_mismatch =
        trace.ledger_intent_equity_mismatch;
    out.cajtucu_order_count = static_cast<std::uint64_t>(trace.orders.size());
    out.cajtucu_executed_order_count = executed_order_count(trace);
    out.cajtucu_fill_count = static_cast<std::uint64_t>(trace.fills.size());
    out.cajtucu_rejected_fill_count =
        static_cast<std::uint64_t>(trace.rejected_fill_count);
    out.cajtucu_partial_fill_count =
        static_cast<std::uint64_t>(trace.partial_fill_count);
    out.cajtucu_missing_direct_reserve_edge_count =
        count_rejected_fill_reason(trace, "missing_direct_reserve_edge");
    out.cajtucu_nontradable_edge_reject_count =
        count_rejected_fill_reason(trace, "edge_not_tradable");
    out.cajtucu_below_min_notional_reject_count =
        count_rejected_fill_reason(trace, "below_min_notional");
    out.cajtucu_above_max_notional_reject_count =
        count_rejected_fill_reason(trace, "above_max_notional");
    out.cajtucu_insufficient_reserve_reject_count =
        count_rejected_fill_reason(trace, "insufficient_base_reserve");
    out.cajtucu_insufficient_units_reject_count =
        count_rejected_fill_reason(trace, "insufficient_asset_units");
    out.cajtucu_invalid_sell_price_count = count_rejected_fill_reason(
        trace, "invalid_sell_price_after_spread_slippage");
    out.cajtucu_large_equity_mismatch_count =
        count_failure_prefix(trace, "ledger_intent_equity_mismatch_exceeds_"
                                    "limit");
    out.cajtucu_requested_notional_base = trace.requested_notional_base;
    out.cajtucu_executed_notional_base = trace.routed_notional_base;
    out.cajtucu_rejected_notional_base = rejected_notional_base(trace);
    out.cajtucu_partial_notional_base = partial_notional_base(trace);
    out.cajtucu_fill_ratio =
        trace.requested_notional_base > 1.0e-12
            ? trace.routed_notional_base / trace.requested_notional_base
            : 1.0;
    out.cajtucu_total_fee_base = trace.total_fee_base;
    out.cajtucu_total_spread_cost_base = trace.total_spread_cost_base;
    out.cajtucu_total_slippage_base = trace.total_slippage_base;
    out.cajtucu_total_transaction_cost_base = trace.total_transaction_cost_base;
    if (trace.ledger_after.weights.defined() &&
        trace.intent.target_weights.defined()) {
      auto executed_weights = trace.ledger_after.weights.to(torch::kFloat64)
                                  .contiguous()
                                  .view({-1});
      auto target_weights = trace.intent.target_weights.to(torch::kFloat64)
                                .contiguous()
                                .view({-1});
      const auto n = std::min(executed_weights.numel(), target_weights.numel());
      if (n > 0) {
        auto diff =
            (executed_weights.slice(0, 0, n) - target_weights.slice(0, 0, n))
                .abs();
        out.target_weight_error_l1 = diff.sum().item<double>();
        out.target_weight_error_linf = diff.max().item<double>();
        out.post_execution_risky_weight_sum =
            executed_weights.slice(0, 0, n).sum().item<double>();
      }
    }
    out.post_execution_base_reserve_weight =
        trace.ledger_after.base_reserve_weight;
    out.reserve_shortfall_count =
        (trace.ledger_after.base_reserve_units < -1.0e-10 ||
         out.cajtucu_insufficient_reserve_reject_count > 0)
            ? 1U
            : 0U;
    out.ledger_before_equity = trace.ledger_before.equity_value_base;
    out.ledger_after_execution_equity = trace.ledger_after.equity_value_base;
    out.ledger_after_realization_equity =
        transition.info.portfolio_after.equity_value_base;
    out.ledger_equity_reconciliation_error =
        std::isfinite(out.ledger_after_execution_equity)
            ? std::abs(out.ledger_after_execution_equity -
                       trace.ledger_after.equity_value_base)
            : std::numeric_limits<double>::quiet_NaN();
    out.base_reserve_units_before = trace.ledger_before.base_reserve_units;
    out.base_reserve_units_after = trace.ledger_after.base_reserve_units;
    out.unit_nonnegativity_violation_count =
        negative_unit_count(trace.ledger_before.units) +
        negative_unit_count(trace.ledger_after.units);
  }
  out.reward_log_growth = transition.reward.log_growth_reward;
  out.reward_drawdown_penalty = transition.reward.drawdown_penalty;
  out.reward_transaction_cost_penalty =
      transition.reward.transaction_cost_penalty;
  out.reward_turnover_penalty = transition.reward.turnover_penalty;
  out.reward_invalid_action_penalty = transition.reward.invalid_action_penalty;
  out.reward_total = transition.reward.total;
  if (transition.info.projection_validation.available) {
    out.projection_mae = transition.info.projection_validation.mae;
    out.projection_rmse = transition.info.projection_validation.rmse;
    out.projection_signed_bias =
        transition.info.projection_validation.signed_bias;
    out.projection_correlation =
        transition.info.projection_validation.correlation;
    out.projection_directional_accuracy =
        transition.info.projection_validation.directional_accuracy;
    out.projection_interval_coverage =
        transition.info.projection_validation.interval_coverage;
    out.projection_mean_interval_width =
        transition.info.projection_validation.mean_interval_width;
    out.zero_return_baseline_mae =
        transition.info.projection_validation.zero_baseline_mae;
    out.zero_return_baseline_rmse =
        transition.info.projection_validation.zero_baseline_rmse;
    out.zero_return_baseline_signed_bias =
        transition.info.projection_validation.zero_baseline_signed_bias;
    out.zero_return_baseline_directional_accuracy =
        transition.info.projection_validation
            .zero_baseline_directional_accuracy;
    out.model_skill_vs_zero_mae =
        transition.info.projection_validation.model_skill_vs_zero_mae;
    out.model_skill_vs_zero_rmse =
        transition.info.projection_validation.model_skill_vs_zero_rmse;
  }
  out.residual_quality_available = transition.info.residual_quality.available;
  out.residual_quality_valid = transition.info.residual_quality.valid;
  if (transition.info.residual_quality.available) {
    out.residual_mean_residual_energy =
        transition.info.residual_quality.mean_residual_energy;
    out.residual_max_residual_energy =
        transition.info.residual_quality.max_residual_energy;
  }
  out.decision_report_schema_id = transition.info.decision_report.schema_id;
  out.decision_report_text_bytes =
      static_cast<std::uint64_t>(transition.info.decision_report.text.size());
  out.warning_count =
      static_cast<std::uint64_t>(transition.info.warnings.size());
  out.failure_count =
      static_cast<std::uint64_t>(transition.info.failures.size());
  out.warnings = join_diagnostics(transition.info.warnings);
  out.failures = join_diagnostics(transition.info.failures);
  return out;
}

} // namespace episode_runner_detail

[[nodiscard]] inline episode_report_t
run_episode(world_iface_t &world, policy_adapter_iface_t &policy,
            const episode_spec_t &spec,
            const episode_runner_options_t &options = {}) {
  validate_episode_spec(spec);
  validate_reward_options(options.reward_options);

  episode_report_t report{};
  report.episode_id = spec.episode_id;
  report.protocol_id = spec.protocol_id;
  report.runtime_run_id = spec.runtime_run_id;
  report.environment_run_id = spec.environment_run_id;
  report.policy_id = policy.policy_id();
  report.policy_kind = policy.policy_kind();
  report.world_mode = spec.world_mode;
  report.graph_order_fingerprint = spec.graph_order_fingerprint;
  report.graph_node_ids =
      episode_runner_detail::join_tokens(spec.graph_node_ids);
  report.risky_node_ids =
      episode_runner_detail::join_tokens(spec.risky_node_ids);
  report.base_reserve_node_id = spec.base_policy.reserve_asset_id;
  report.requested_anchor_index_begin =
      spec.requested_range.anchor_index_begin.value_or(-1);
  report.requested_anchor_index_end =
      spec.requested_range.anchor_index_end.value_or(-1);
  report.requested_source_key_begin =
      spec.requested_range.source_key_begin.value_or(-1);
  report.requested_source_key_end =
      spec.requested_range.source_key_end.value_or(-1);
  report.accepted_cursor_wave_id = spec.accepted_range.cursor.wave_id;
  report.accepted_cursor_wave_mode = spec.accepted_range.cursor.wave_mode;
  report.accepted_cursor_kind = spec.accepted_range.cursor.source_cursor_kind;
  report.accepted_cursor_scope = spec.accepted_range.cursor.source_cursor_scope;
  report.accepted_source_range_policy =
      spec.accepted_range.cursor.source_range_policy;
  report.accepted_source_order_policy =
      spec.accepted_range.cursor.source_order_policy;
  report.accepted_batch_cursor_token =
      spec.accepted_range.cursor.batch_cursor_token;
  report.accepted_anchor_index_begin = spec.accepted_range.anchor_index_begin;
  report.accepted_anchor_index_end = spec.accepted_range.anchor_index_end;
  report.accepted_anchor_keys =
      episode_runner_detail::join_tokens(spec.accepted_range.anchor_keys);
  if (detail::has_realized_return_projection(spec.realized_return_projection)) {
    report.realized_return_truth_id = spec.realized_return_projection.truth_id;
    report.realized_return_projection_reference_node_id =
        spec.realized_return_projection.reference_node_id;
    report.realized_return_projection_edge_ids =
        episode_runner_detail::join_tokens(
            spec.realized_return_projection.edge_ids);
    report.realized_return_projection_signs =
        episode_runner_detail::format_node_scalars(
            spec.risky_node_ids, spec.realized_return_projection.signs);
  }

  auto observation = world.reset(spec);
  episode_runner_detail::validate_observation_evidence_for_policy(observation,
                                                                  spec);
  const auto range_steps = static_cast<std::uint64_t>(
      std::max<std::int64_t>(0, spec.accepted_range.anchor_index_end -
                                    spec.accepted_range.anchor_index_begin));
  if (options.require_full_accepted_range && options.max_steps != 0 &&
      options.max_steps < range_steps) {
    throw std::runtime_error(
        "[episode_runner] max_steps cannot truncate required accepted range");
  }
  const std::uint64_t max_steps =
      options.require_full_accepted_range
          ? range_steps
          : (options.max_steps == 0 ? range_steps : options.max_steps);

  std::vector<double> projection_mae;
  std::vector<double> projection_rmse;
  std::vector<double> projection_bias;
  std::vector<double> projection_correlation;
  std::vector<double> projection_directional_accuracy;
  std::vector<double> projection_coverage;
  std::vector<double> projection_interval_width;
  std::vector<double> zero_return_baseline_mae;
  std::vector<double> zero_return_baseline_rmse;
  std::vector<double> zero_return_baseline_bias;
  std::vector<double> zero_return_baseline_directional_accuracy;
  std::vector<double> model_skill_vs_zero_mae;
  std::vector<double> model_skill_vs_zero_rmse;
  std::vector<episode_runner_detail::node_projection_series_t>
      node_projection_series(spec.risky_node_ids.size());
  std::vector<double> target_weight_error_l1;
  std::vector<double> target_weight_error_linf;
  std::vector<double> post_execution_risky_weight_sum;
  std::vector<double> post_execution_base_reserve_weight;
  std::vector<double> ledger_before_equity;
  std::vector<double> ledger_after_execution_equity;
  std::vector<double> ledger_after_realization_equity;

  bool done = false;
  while (!done && report.transition_count < max_steps) {
    auto action = policy.act(observation);
    episode_runner_detail::validate_policy_action_identity(action, policy);
    if (options.validate_policy_actions) {
      validate_episode_action(action, spec);
    }
    validate_action_time_boundary(action, observation);
    auto transition = world.step(action);
    episode_runner_detail::validate_transition_evidence_for_report(
        observation, transition, spec, options.reward_options);
    if (report.method_id.empty()) {
      report.method_id = action.method_id;
    } else if (report.method_id != action.method_id) {
      throw std::runtime_error(
          "[episode_runner] action method_id changed within episode");
    }
    ++report.transition_count;
    auto step_report = episode_runner_detail::make_step_report(
        report.transition_count - 1, observation, action, transition, spec);
    if (step_report.cajtucu_execution_trace_available) {
      report.cajtucu_valid_trace_count +=
          step_report.cajtucu_trace_valid ? 1U : 0U;
      report.cajtucu_invalid_trace_count +=
          step_report.cajtucu_trace_valid ? 0U : 1U;
      report.cajtucu_missing_direct_reserve_edge_count +=
          step_report.cajtucu_missing_direct_reserve_edge_count;
      report.cajtucu_nontradable_edge_reject_count +=
          step_report.cajtucu_nontradable_edge_reject_count;
      report.cajtucu_below_min_notional_reject_count +=
          step_report.cajtucu_below_min_notional_reject_count;
      report.cajtucu_above_max_notional_reject_count +=
          step_report.cajtucu_above_max_notional_reject_count;
      report.cajtucu_insufficient_reserve_reject_count +=
          step_report.cajtucu_insufficient_reserve_reject_count;
      report.cajtucu_insufficient_units_reject_count +=
          step_report.cajtucu_insufficient_units_reject_count;
      report.cajtucu_invalid_sell_price_count +=
          step_report.cajtucu_invalid_sell_price_count;
      report.cajtucu_large_equity_mismatch_count +=
          step_report.cajtucu_large_equity_mismatch_count;
      report.cajtucu_synthetic_market_step_count +=
          step_report.cajtucu_synthetic_direct_edges ? 1U : 0U;
      report.requested_order_count += step_report.cajtucu_order_count;
      report.executed_order_count += step_report.cajtucu_executed_order_count;
      report.rejected_order_count += step_report.cajtucu_rejected_fill_count;
      report.partial_order_count += step_report.cajtucu_partial_fill_count;
      report.requested_notional_base +=
          step_report.cajtucu_requested_notional_base;
      report.executed_notional_base +=
          step_report.cajtucu_executed_notional_base;
      report.rejected_notional_base +=
          step_report.cajtucu_rejected_notional_base;
      report.partial_notional_base += step_report.cajtucu_partial_notional_base;
      report.fee_cost_base += step_report.cajtucu_total_fee_base;
      report.spread_cost_base += step_report.cajtucu_total_spread_cost_base;
      report.slippage_cost_base += step_report.cajtucu_total_slippage_base;
      report.reserve_shortfall_count += step_report.reserve_shortfall_count;
      report.unit_nonnegativity_violation_count +=
          step_report.unit_nonnegativity_violation_count;
      report.max_ledger_equity_reconciliation_error =
          std::max(report.max_ledger_equity_reconciliation_error,
                   std::isfinite(step_report.ledger_equity_reconciliation_error)
                       ? step_report.ledger_equity_reconciliation_error
                       : 0.0);
      if (std::isfinite(step_report.target_weight_error_l1)) {
        target_weight_error_l1.push_back(step_report.target_weight_error_l1);
      }
      if (std::isfinite(step_report.target_weight_error_linf)) {
        target_weight_error_linf.push_back(
            step_report.target_weight_error_linf);
      }
      if (std::isfinite(step_report.post_execution_risky_weight_sum)) {
        post_execution_risky_weight_sum.push_back(
            step_report.post_execution_risky_weight_sum);
      }
      if (std::isfinite(step_report.post_execution_base_reserve_weight)) {
        post_execution_base_reserve_weight.push_back(
            step_report.post_execution_base_reserve_weight);
      }
      if (std::isfinite(step_report.ledger_before_equity)) {
        ledger_before_equity.push_back(step_report.ledger_before_equity);
      }
      if (std::isfinite(step_report.ledger_after_execution_equity)) {
        ledger_after_execution_equity.push_back(
            step_report.ledger_after_execution_equity);
      }
      if (std::isfinite(step_report.ledger_after_realization_equity)) {
        ledger_after_realization_equity.push_back(
            step_report.ledger_after_realization_equity);
      }
      if (std::isfinite(step_report.base_reserve_units_before)) {
        report.base_reserve_units_before =
            step_report.base_reserve_units_before;
      }
      if (std::isfinite(step_report.base_reserve_units_after)) {
        report.base_reserve_units_after = step_report.base_reserve_units_after;
      }
    }
    report.step_reports.push_back(std::move(step_report));
    episode_runner_detail::record_allocation_evidence(report,
                                                      transition.info.target);

    report.total_reward += transition.reward.total;
    report.total_log_growth += transition.info.realized_log_growth;
    report.total_turnover += std::max(0.0, transition.info.turnover);
    report.total_transaction_cost_base +=
        std::max(0.0, transition.info.transaction_cost_base);
    report.max_drawdown =
        std::max(report.max_drawdown, transition.info.portfolio_after.drawdown);
    if (std::isfinite(transition.info.portfolio_after.equity_value_base)) {
      report.final_equity_base =
          transition.info.portfolio_after.equity_value_base;
    }

    if (transition.info.projection_validation.available) {
      projection_mae.push_back(transition.info.projection_validation.mae);
      projection_rmse.push_back(transition.info.projection_validation.rmse);
      projection_bias.push_back(
          transition.info.projection_validation.signed_bias);
      projection_correlation.push_back(
          transition.info.projection_validation.correlation);
      projection_directional_accuracy.push_back(
          transition.info.projection_validation.directional_accuracy);
      projection_coverage.push_back(
          transition.info.projection_validation.interval_coverage);
      projection_interval_width.push_back(
          transition.info.projection_validation.mean_interval_width);
      zero_return_baseline_mae.push_back(
          transition.info.projection_validation.zero_baseline_mae);
      zero_return_baseline_rmse.push_back(
          transition.info.projection_validation.zero_baseline_rmse);
      zero_return_baseline_bias.push_back(
          transition.info.projection_validation.zero_baseline_signed_bias);
      zero_return_baseline_directional_accuracy.push_back(
          transition.info.projection_validation
              .zero_baseline_directional_accuracy);
      model_skill_vs_zero_mae.push_back(
          transition.info.projection_validation.model_skill_vs_zero_mae);
      model_skill_vs_zero_rmse.push_back(
          transition.info.projection_validation.model_skill_vs_zero_rmse);
      episode_runner_detail::append_node_projection_samples(
          node_projection_series, transition.info.projection_validation);
    }

    episode_runner_detail::merge_warnings(report, transition.info.warnings);
    episode_runner_detail::merge_failures(report, transition.info.failures);

    done = transition.done;
    observation = std::move(transition.next_observation);
  }

  report.projection_mae = episode_runner_detail::mean_or_nan(projection_mae);
  report.projection_rmse = episode_runner_detail::mean_or_nan(projection_rmse);
  report.projection_signed_bias =
      episode_runner_detail::mean_or_nan(projection_bias);
  report.projection_correlation =
      episode_runner_detail::mean_or_nan(projection_correlation);
  report.projection_directional_accuracy =
      episode_runner_detail::mean_or_nan(projection_directional_accuracy);
  report.projection_interval_coverage =
      episode_runner_detail::mean_or_nan(projection_coverage);
  report.projection_mean_interval_width =
      episode_runner_detail::mean_or_nan(projection_interval_width);
  report.zero_return_baseline_mae =
      episode_runner_detail::mean_or_nan(zero_return_baseline_mae);
  report.zero_return_baseline_rmse =
      episode_runner_detail::mean_or_nan(zero_return_baseline_rmse);
  report.zero_return_baseline_signed_bias =
      episode_runner_detail::mean_or_nan(zero_return_baseline_bias);
  report.zero_return_baseline_directional_accuracy =
      episode_runner_detail::mean_or_nan(
          zero_return_baseline_directional_accuracy);
  report.model_skill_vs_zero_mae =
      episode_runner_detail::mean_or_nan(model_skill_vs_zero_mae);
  report.model_skill_vs_zero_rmse =
      episode_runner_detail::mean_or_nan(model_skill_vs_zero_rmse);
  report.fill_ratio =
      report.requested_notional_base > 1.0e-12
          ? report.executed_notional_base / report.requested_notional_base
          : 1.0;
  report.cost_as_fraction_of_equity =
      std::isfinite(report.final_equity_base) && report.final_equity_base > 0.0
          ? report.total_transaction_cost_base / report.final_equity_base
          : std::numeric_limits<double>::quiet_NaN();
  report.cost_as_fraction_of_gross_return =
      std::abs(report.total_log_growth) > 1.0e-12
          ? report.total_transaction_cost_base /
                std::abs(report.total_log_growth)
          : std::numeric_limits<double>::quiet_NaN();
  report.mean_target_weight_error_l1 =
      episode_runner_detail::mean_or_nan(target_weight_error_l1);
  report.mean_target_weight_error_linf =
      episode_runner_detail::mean_or_nan(target_weight_error_linf);
  report.mean_post_execution_risky_weight_sum =
      episode_runner_detail::mean_or_nan(post_execution_risky_weight_sum);
  report.mean_post_execution_base_reserve_weight =
      episode_runner_detail::mean_or_nan(post_execution_base_reserve_weight);
  report.mean_ledger_before_equity =
      episode_runner_detail::mean_or_nan(ledger_before_equity);
  report.mean_ledger_after_execution_equity =
      episode_runner_detail::mean_or_nan(ledger_after_execution_equity);
  report.mean_ledger_after_realization_equity =
      episode_runner_detail::mean_or_nan(ledger_after_realization_equity);

  std::vector<double> per_node_projection_mae;
  std::vector<double> per_node_projection_rmse;
  std::vector<double> per_node_projection_signed_bias;
  std::vector<double> per_node_projection_correlation;
  std::vector<double> per_node_projection_directional_accuracy;
  std::vector<double> per_node_projection_interval_coverage;
  std::vector<double> per_node_projection_mean_interval_width;
  std::vector<double> per_node_realized_volatility;
  std::vector<double> per_node_normalized_projection_mae;
  std::vector<double> per_node_normalized_projection_rmse;
  std::vector<double> per_node_zero_return_baseline_mae;
  std::vector<double> per_node_zero_return_baseline_rmse;
  std::vector<double> per_node_model_skill_vs_zero_mae;
  std::vector<double> per_node_model_skill_vs_zero_rmse;
  per_node_projection_mae.reserve(node_projection_series.size());
  per_node_projection_rmse.reserve(node_projection_series.size());
  per_node_projection_signed_bias.reserve(node_projection_series.size());
  per_node_projection_correlation.reserve(node_projection_series.size());
  per_node_projection_directional_accuracy.reserve(
      node_projection_series.size());
  per_node_projection_interval_coverage.reserve(node_projection_series.size());
  per_node_projection_mean_interval_width.reserve(
      node_projection_series.size());
  per_node_realized_volatility.reserve(node_projection_series.size());
  per_node_normalized_projection_mae.reserve(node_projection_series.size());
  per_node_normalized_projection_rmse.reserve(node_projection_series.size());
  per_node_zero_return_baseline_mae.reserve(node_projection_series.size());
  per_node_zero_return_baseline_rmse.reserve(node_projection_series.size());
  per_node_model_skill_vs_zero_mae.reserve(node_projection_series.size());
  per_node_model_skill_vs_zero_rmse.reserve(node_projection_series.size());
  for (const auto &node_series : node_projection_series) {
    const double mae =
        episode_runner_detail::mean_or_nan(node_series.abs_error);
    const double rmse = std::sqrt(
        episode_runner_detail::mean_or_nan(node_series.squared_error));
    const double zero_mae = episode_runner_detail::mean_or_nan([&]() {
      std::vector<double> values;
      values.reserve(node_series.realized.size());
      for (const double realized : node_series.realized) {
        values.push_back(std::abs(realized));
      }
      return values;
    }());
    const double zero_rmse =
        episode_runner_detail::rmse_or_nan(node_series.realized);
    const double realized_volatility =
        episode_runner_detail::population_std_or_nan(node_series.realized);
    per_node_projection_mae.push_back(mae);
    per_node_projection_rmse.push_back(rmse);
    per_node_projection_signed_bias.push_back(
        episode_runner_detail::mean_or_nan(node_series.error));
    per_node_projection_correlation.push_back(
        episode_runner_detail::correlation_or_zero(node_series.predicted,
                                                   node_series.realized));
    per_node_projection_directional_accuracy.push_back(
        episode_runner_detail::directional_accuracy_or_nan(
            node_series.predicted, node_series.realized));
    per_node_projection_interval_coverage.push_back(
        episode_runner_detail::mean_or_nan(node_series.interval_hit));
    per_node_projection_mean_interval_width.push_back(
        episode_runner_detail::mean_or_nan(node_series.interval_width));
    per_node_realized_volatility.push_back(realized_volatility);
    per_node_normalized_projection_mae.push_back(
        std::isfinite(realized_volatility) && realized_volatility > 1.0e-12
            ? mae / realized_volatility
            : std::numeric_limits<double>::quiet_NaN());
    per_node_normalized_projection_rmse.push_back(
        std::isfinite(realized_volatility) && realized_volatility > 1.0e-12
            ? rmse / realized_volatility
            : std::numeric_limits<double>::quiet_NaN());
    per_node_zero_return_baseline_mae.push_back(zero_mae);
    per_node_zero_return_baseline_rmse.push_back(zero_rmse);
    per_node_model_skill_vs_zero_mae.push_back(
        episode_runner_detail::safe_skill(mae, zero_mae));
    per_node_model_skill_vs_zero_rmse.push_back(
        episode_runner_detail::safe_skill(rmse, zero_rmse));
  }
  report.per_node_projection_mae =
      episode_runner_detail::format_node_metric_vector(spec.risky_node_ids,
                                                       per_node_projection_mae);
  report.per_node_projection_rmse =
      episode_runner_detail::format_node_metric_vector(
          spec.risky_node_ids, per_node_projection_rmse);
  report.per_node_projection_signed_bias =
      episode_runner_detail::format_node_metric_vector(
          spec.risky_node_ids, per_node_projection_signed_bias);
  report.per_node_projection_correlation =
      episode_runner_detail::format_node_metric_vector(
          spec.risky_node_ids, per_node_projection_correlation);
  report.per_node_projection_directional_accuracy =
      episode_runner_detail::format_node_metric_vector(
          spec.risky_node_ids, per_node_projection_directional_accuracy);
  report.per_node_projection_interval_coverage =
      episode_runner_detail::format_node_metric_vector(
          spec.risky_node_ids, per_node_projection_interval_coverage);
  report.per_node_projection_mean_interval_width =
      episode_runner_detail::format_node_metric_vector(
          spec.risky_node_ids, per_node_projection_mean_interval_width);
  report.per_node_realized_volatility =
      episode_runner_detail::format_node_metric_vector(
          spec.risky_node_ids, per_node_realized_volatility);
  report.per_node_normalized_projection_mae =
      episode_runner_detail::format_node_metric_vector(
          spec.risky_node_ids, per_node_normalized_projection_mae);
  report.per_node_normalized_projection_rmse =
      episode_runner_detail::format_node_metric_vector(
          spec.risky_node_ids, per_node_normalized_projection_rmse);
  report.per_node_zero_return_baseline_mae =
      episode_runner_detail::format_node_metric_vector(
          spec.risky_node_ids, per_node_zero_return_baseline_mae);
  report.per_node_zero_return_baseline_rmse =
      episode_runner_detail::format_node_metric_vector(
          spec.risky_node_ids, per_node_zero_return_baseline_rmse);
  report.per_node_model_skill_vs_zero_mae =
      episode_runner_detail::format_node_metric_vector(
          spec.risky_node_ids, per_node_model_skill_vs_zero_mae);
  report.per_node_model_skill_vs_zero_rmse =
      episode_runner_detail::format_node_metric_vector(
          spec.risky_node_ids, per_node_model_skill_vs_zero_rmse);

  if (options.require_full_accepted_range) {
    if (report.transition_count != range_steps) {
      throw std::runtime_error(
          "[episode_runner] episode ended before accepted range completed");
    }
    if (!done) {
      throw std::runtime_error(
          "[episode_runner] world did not terminate at accepted range end");
    }
  } else if (!done && report.transition_count >= max_steps) {
    report.warnings.push_back("episode_runner.max_steps_reached");
  }
  return report;
}

} // namespace cuwacunu::kikijyeba::environment
