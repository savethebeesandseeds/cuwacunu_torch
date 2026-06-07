// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <future>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include "kikijyeba/environment/replay/bundle_source.h"
#include "kikijyeba/environment/replay/source.h"
#include "kikijyeba/environment/run/episode_runner.h"

namespace cuwacunu::kikijyeba::environment {

struct replay_policy_factory_t {
  std::string policy_id{};
  policy_kind_t policy_kind{policy_kind_t::external};
  std::function<std::unique_ptr<policy_adapter_iface_t>(
      const replay::replay_episode_bundle_t &bundle)>
      make_policy{};
};

struct replay_experiment_options_t {
  episode_runner_options_t episode_options{};
  bool continue_on_failure{false};
  std::size_t max_parallel_jobs{1}; // 0 selects hardware_concurrency.
};

namespace experiment_runner_detail {

inline void
validate_episode_step_report_identity(const episode_report_t &episode_report) {
  if (episode_report.step_reports.size() != episode_report.transition_count) {
    throw std::runtime_error(
        "[experiment_runner] step report count must match transition count");
  }
  if (episode_report.accepted_anchor_index_begin < 0 ||
      episode_report.accepted_anchor_index_end <=
          episode_report.accepted_anchor_index_begin) {
    throw std::runtime_error(
        "[experiment_runner] episode report accepted anchor range is invalid");
  }
  const auto accepted_anchor_count =
      static_cast<std::size_t>(episode_report.accepted_anchor_index_end -
                               episode_report.accepted_anchor_index_begin);
  if (episode_report.step_reports.size() > accepted_anchor_count) {
    throw std::runtime_error(
        "[experiment_runner] step reports exceed accepted cursor range");
  }
  const auto accepted_anchor_keys =
      episode_runner_detail::split_tokens(episode_report.accepted_anchor_keys);
  if (accepted_anchor_keys.size() != accepted_anchor_count) {
    throw std::runtime_error(
        "[experiment_runner] accepted anchor key count must match accepted "
        "cursor range");
  }
  for (const auto &accepted_anchor_key : accepted_anchor_keys) {
    if (detail::blank(accepted_anchor_key)) {
      throw std::runtime_error(
          "[experiment_runner] accepted anchor keys must be nonempty");
    }
  }
  const auto target_node_ids =
      episode_runner_detail::split_tokens(episode_report.target_node_ids);
  const bool has_projection_evidence =
      !episode_report.realized_return_truth_id.empty() ||
      !episode_report.realized_return_projection_reference_node_id.empty() ||
      !episode_report.realized_return_projection_edge_ids.empty() ||
      !episode_report.realized_return_projection_signs.empty();
  if (has_projection_evidence) {
    const auto projection_edge_ids = episode_runner_detail::split_tokens(
        episode_report.realized_return_projection_edge_ids);
    const auto projection_signs = episode_runner_detail::split_tokens(
        episode_report.realized_return_projection_signs);
    if (episode_report.realized_return_truth_id !=
            kDirectEdgeRealizedReturnTruthV1 ||
        detail::blank(
            episode_report.realized_return_projection_reference_node_id) ||
        projection_edge_ids.size() != target_node_ids.size() ||
        projection_signs.size() != target_node_ids.size()) {
      throw std::runtime_error(
          "[experiment_runner] realized-return projection report evidence must "
          "match target node count");
    }
    for (std::size_t i = 0; i < projection_edge_ids.size(); ++i) {
      const bool is_reference_node =
          target_node_ids[i] ==
          episode_report.realized_return_projection_reference_node_id;
      if ((!is_reference_node && detail::blank(projection_edge_ids[i])) ||
          (is_reference_node && !detail::blank(projection_edge_ids[i])) ||
          projection_signs[i].rfind(target_node_ids[i] + ":", 0) != 0) {
        throw std::runtime_error(
            "[experiment_runner] realized-return projection report evidence "
            "must preserve target node order");
      }
    }
  }
  for (std::size_t i = 0; i < episode_report.step_reports.size(); ++i) {
    const auto &step = episode_report.step_reports[i];
    if (step.step_index != i) {
      throw std::runtime_error(
          "[experiment_runner] step report index must match position");
    }
    if (step.episode_id != episode_report.episode_id ||
        step.protocol_id != episode_report.protocol_id ||
        step.runtime_run_id != episode_report.runtime_run_id ||
        step.environment_run_id != episode_report.environment_run_id ||
        step.policy_id != episode_report.policy_id ||
        step.method_id != episode_report.method_id ||
        step.policy_kind != episode_report.policy_kind ||
        step.world_mode != episode_report.world_mode) {
      throw std::runtime_error(
          "[experiment_runner] step report episode/run/policy/method identity "
          "must match parent episode report");
    }
    if (step.accepted_cursor_wave_id !=
            episode_report.accepted_cursor_wave_id ||
        step.accepted_cursor_wave_mode !=
            episode_report.accepted_cursor_wave_mode ||
        step.accepted_cursor_kind != episode_report.accepted_cursor_kind ||
        step.accepted_cursor_scope != episode_report.accepted_cursor_scope ||
        step.accepted_source_range_policy !=
            episode_report.accepted_source_range_policy ||
        step.accepted_source_order_policy !=
            episode_report.accepted_source_order_policy ||
        step.accepted_batch_cursor_token !=
            episode_report.accepted_batch_cursor_token ||
        step.accepted_anchor_index_begin !=
            episode_report.accepted_anchor_index_begin ||
        step.accepted_anchor_index_end !=
            episode_report.accepted_anchor_index_end) {
      throw std::runtime_error(
          "[experiment_runner] step report accepted cursor identity must match "
          "parent episode report");
    }
    const auto expected_offset = static_cast<std::int64_t>(i);
    const auto expected_anchor_index =
        episode_report.accepted_anchor_index_begin + expected_offset;
    if (step.accepted_cursor_offset != expected_offset ||
        step.observation_anchor_index != expected_anchor_index) {
      throw std::runtime_error(
          "[experiment_runner] step report cursor offset must match ordered "
          "accepted anchor range");
    }
    if (step.anchor_key != accepted_anchor_keys[i]) {
      throw std::runtime_error(
          "[experiment_runner] step report anchor_key must match accepted "
          "anchor key at cursor offset");
    }
    if (step.next_realization_anchor_index !=
        step.observation_anchor_index + 1) {
      throw std::runtime_error(
          "[experiment_runner] step report next-realization anchor must be "
          "immediate");
    }
    if (step.knowledge_timestamp_ms <= 0 ||
        step.realization_available_after_timestamp_ms <=
            step.knowledge_timestamp_ms) {
      throw std::runtime_error(
          "[experiment_runner] step report realization timestamp must be after "
          "knowledge timestamp");
    }
    if (step.action_decision_timestamp_ms < step.knowledge_timestamp_ms ||
        step.action_decision_timestamp_ms >=
            step.realization_available_after_timestamp_ms) {
      throw std::runtime_error(
          "[experiment_runner] step report action decision timestamp must be "
          "after knowledge and before realization availability");
    }
  }
}

inline void validate_policy_factory(const replay_policy_factory_t &factory) {
  if (detail::blank(factory.policy_id)) {
    throw std::runtime_error("[experiment_runner] policy_id is required");
  }
  if (!factory.make_policy) {
    throw std::runtime_error("[experiment_runner] policy factory is required");
  }
}

inline void validate_policy_factories(
    const std::vector<replay_policy_factory_t> &policy_factories) {
  if (policy_factories.empty()) {
    throw std::runtime_error(
        "[experiment_runner] at least one policy factory is required");
  }
  std::unordered_set<std::string> seen_policy_ids;
  seen_policy_ids.reserve(policy_factories.size());
  for (const auto &factory : policy_factories) {
    validate_policy_factory(factory);
    if (!seen_policy_ids.insert(factory.policy_id).second) {
      throw std::runtime_error(
          "[experiment_runner] duplicate policy factory id: " +
          factory.policy_id);
    }
  }
}

template <typename MetricFn>
[[nodiscard]] double
mean_episode_metric(const std::vector<episode_report_t> &reports,
                    MetricFn metric) {
  std::vector<double> values;
  values.reserve(reports.size());
  for (const auto &report : reports) {
    const double value = metric(report);
    if (std::isfinite(value)) {
      values.push_back(value);
    }
  }
  return episode_runner_detail::mean_or_nan(values);
}

template <typename MetricFn>
[[nodiscard]] std::uint64_t
sum_episode_count(const std::vector<episode_report_t> &reports,
                  MetricFn metric) {
  std::uint64_t out = 0;
  for (const auto &report : reports) {
    out += metric(report);
  }
  return out;
}

template <typename MetricFn>
[[nodiscard]] double
sum_episode_metric(const std::vector<episode_report_t> &reports,
                   MetricFn metric) {
  double out = 0.0;
  for (const auto &report : reports) {
    const double value = metric(report);
    if (std::isfinite(value)) {
      out += value;
    }
  }
  return out;
}

template <typename MetricFn>
[[nodiscard]] double
max_episode_metric(const std::vector<episode_report_t> &reports,
                   MetricFn metric) {
  double out = 0.0;
  for (const auto &report : reports) {
    const double value = metric(report);
    if (std::isfinite(value)) {
      out = std::max(out, value);
    }
  }
  return out;
}

[[nodiscard]] inline std::size_t
resolve_parallelism(std::size_t requested_max_parallel_jobs) {
  if (requested_max_parallel_jobs == 0) {
    return std::max<std::size_t>(1, std::thread::hardware_concurrency());
  }
  return std::max<std::size_t>(1, requested_max_parallel_jobs);
}

struct replay_experiment_task_t {
  std::size_t task_index{0};
  std::size_t bundle_index{0};
  std::size_t policy_index{0};
};

struct replay_experiment_task_result_t {
  std::size_t task_index{0};
  std::size_t bundle_index{0};
  std::string policy_id{};
  bool completed{false};
  episode_report_t report{};
  std::string failure{};
};

[[nodiscard]] inline replay_experiment_task_result_t
run_task(replay_experiment_task_t task, replay::replay_episode_bundle_t bundle,
         replay_policy_factory_t factory,
         episode_runner_options_t episode_options) {
  replay_experiment_task_result_t result{};
  result.task_index = task.task_index;
  result.bundle_index = task.bundle_index;
  result.policy_id = factory.policy_id;
  try {
    replay::validate_replay_episode_bundle_ready_for_world(bundle);
    auto spec = bundle.spec;
    auto policy = factory.make_policy(bundle);
    if (!policy) {
      throw std::runtime_error(
          "[experiment_runner] policy factory returned null");
    }
    if (policy->policy_id() != factory.policy_id) {
      throw std::runtime_error(
          "[experiment_runner] policy adapter id does not match factory id: " +
          policy->policy_id() + " != " + factory.policy_id);
    }
    if (policy->policy_kind() != factory.policy_kind) {
      throw std::runtime_error(
          "[experiment_runner] policy adapter kind does not match factory "
          "kind: " +
          std::string(policy_kind_name(policy->policy_kind())) +
          " != " + policy_kind_name(factory.policy_kind));
    }
    episode_options.reward_options = bundle.world_options.reward_options;
    auto world = replay::spawn_replay_world(std::move(bundle));
    result.report = run_episode(*world, *policy, spec, episode_options);
    result.report.experiment_task_index =
        static_cast<std::int64_t>(task.task_index);
    result.report.experiment_bundle_index =
        static_cast<std::int64_t>(task.bundle_index);
    result.report.experiment_policy_index =
        static_cast<std::int64_t>(task.policy_index);
    validate_episode_step_report_identity(result.report);
    result.completed = true;
  } catch (const std::exception &ex) {
    result.failure = "task=" + std::to_string(task.task_index) +
                     "|bundle=" + std::to_string(task.bundle_index) +
                     "|policy_index=" + std::to_string(task.policy_index) +
                     "|policy=" + factory.policy_id +
                     "|policy_kind=" + policy_kind_name(factory.policy_kind) +
                     "|" + ex.what();
  }
  return result;
}

} // namespace experiment_runner_detail

struct replay_policy_summary_t {
  std::string policy_summary_schema_id{kReplayPolicyComparisonArtifactSchema};
  std::string policy_id{};
  policy_kind_t policy_kind{policy_kind_t::external};

  std::uint64_t attempted_count{0};
  std::uint64_t completed_count{0};
  std::uint64_t failed_count{0};

  double mean_total_reward{std::numeric_limits<double>::quiet_NaN()};
  double mean_total_log_growth{std::numeric_limits<double>::quiet_NaN()};
  double mean_final_equity_numeraire{std::numeric_limits<double>::quiet_NaN()};
  double mean_max_drawdown{std::numeric_limits<double>::quiet_NaN()};
  double mean_total_turnover{std::numeric_limits<double>::quiet_NaN()};
  double mean_total_transaction_cost_numeraire{
      std::numeric_limits<double>::quiet_NaN()};
  std::uint64_t cajtucu_valid_trace_count{0};
  std::uint64_t cajtucu_invalid_trace_count{0};
  std::uint64_t cajtucu_missing_direct_pair_count{0};
  std::uint64_t cajtucu_numeraire_fallback_pair_count{0};
  std::uint64_t cajtucu_nontradable_edge_reject_count{0};
  std::uint64_t cajtucu_below_min_notional_reject_count{0};
  std::uint64_t cajtucu_above_max_notional_reject_count{0};
  std::uint64_t cajtucu_insufficient_sell_units_reject_count{0};
  std::uint64_t cajtucu_insufficient_units_reject_count{0};
  std::uint64_t cajtucu_invalid_sell_price_count{0};
  std::uint64_t cajtucu_large_equity_mismatch_count{0};
  std::uint64_t cajtucu_synthetic_market_step_count{0};
  std::uint64_t requested_order_count{0};
  std::uint64_t executed_order_count{0};
  std::uint64_t rejected_order_count{0};
  std::uint64_t partial_order_count{0};
  double requested_notional_numeraire{0.0};
  double executed_notional_numeraire{0.0};
  double rejected_notional_numeraire{0.0};
  double partial_notional_numeraire{0.0};
  double fill_ratio{std::numeric_limits<double>::quiet_NaN()};
  double fee_cost_numeraire{0.0};
  double spread_cost_numeraire{0.0};
  double slippage_cost_numeraire{0.0};
  double cost_as_fraction_of_equity{std::numeric_limits<double>::quiet_NaN()};
  double cost_as_fraction_of_gross_return{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_target_weight_error_l1{std::numeric_limits<double>::quiet_NaN()};
  double mean_target_weight_error_linf{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_post_execution_target_weight_sum{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_post_execution_numeraire_weight{
      std::numeric_limits<double>::quiet_NaN()};
  std::uint64_t numeraire_shortfall_count{0};
  double mean_ledger_before_equity{std::numeric_limits<double>::quiet_NaN()};
  double mean_ledger_after_execution_equity{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_ledger_after_realization_equity{
      std::numeric_limits<double>::quiet_NaN()};
  double max_ledger_equity_reconciliation_error{0.0};
  double numeraire_units_before{std::numeric_limits<double>::quiet_NaN()};
  double numeraire_units_after{std::numeric_limits<double>::quiet_NaN()};
  std::uint64_t unit_nonnegativity_violation_count{0};
  double mean_projection_mae{std::numeric_limits<double>::quiet_NaN()};
  double mean_projection_rmse{std::numeric_limits<double>::quiet_NaN()};
  double mean_projection_signed_bias{std::numeric_limits<double>::quiet_NaN()};
  double mean_projection_correlation{std::numeric_limits<double>::quiet_NaN()};
  double mean_projection_directional_accuracy{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_projection_interval_coverage{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_projection_interval_width{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_zero_return_baseline_mae{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_zero_return_baseline_rmse{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_model_skill_vs_zero_mae{std::numeric_limits<double>::quiet_NaN()};
  double mean_model_skill_vs_zero_rmse{
      std::numeric_limits<double>::quiet_NaN()};
};

struct replay_experiment_report_t {
  std::string experiment_artifact_schema_id{kReplayExperimentArtifactSchema};
  std::string experiment_id{};
  std::string runtime_run_id{};
  std::string environment_run_id{};
  std::string execution_profile_digest{};
  std::string policy_set_digest{};
  std::vector<episode_report_t> episode_reports{};
  std::vector<replay_policy_summary_t> policy_summaries{};
  std::vector<std::string> warnings{};
  std::vector<std::string> failures{};

  std::size_t requested_max_parallel_jobs{1};
  std::size_t resolved_parallelism{1};

  std::uint64_t attempted_count{0};
  std::uint64_t completed_count{0};

  [[nodiscard]] double mean_total_reward() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports,
        [](const episode_report_t &report) { return report.total_reward; });
  }

  [[nodiscard]] double mean_total_log_growth() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports,
        [](const episode_report_t &report) { return report.total_log_growth; });
  }

  [[nodiscard]] double mean_final_equity_numeraire() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.final_equity_numeraire;
        });
  }

  [[nodiscard]] double mean_max_drawdown() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports,
        [](const episode_report_t &report) { return report.max_drawdown; });
  }

  [[nodiscard]] double mean_total_turnover() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports,
        [](const episode_report_t &report) { return report.total_turnover; });
  }

  [[nodiscard]] double mean_total_transaction_cost_numeraire() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.total_transaction_cost_numeraire;
        });
  }

  [[nodiscard]] std::uint64_t cajtucu_valid_trace_count() const {
    return experiment_runner_detail::sum_episode_count(
        episode_reports, [](const episode_report_t &report) {
          return report.cajtucu_valid_trace_count;
        });
  }

  [[nodiscard]] std::uint64_t cajtucu_invalid_trace_count() const {
    return experiment_runner_detail::sum_episode_count(
        episode_reports, [](const episode_report_t &report) {
          return report.cajtucu_invalid_trace_count;
        });
  }

  [[nodiscard]] std::uint64_t cajtucu_rejected_order_count() const {
    return experiment_runner_detail::sum_episode_count(
        episode_reports, [](const episode_report_t &report) {
          return report.rejected_order_count;
        });
  }

  [[nodiscard]] std::uint64_t cajtucu_partial_order_count() const {
    return experiment_runner_detail::sum_episode_count(
        episode_reports, [](const episode_report_t &report) {
          return report.partial_order_count;
        });
  }

  [[nodiscard]] std::uint64_t cajtucu_synthetic_market_step_count() const {
    return experiment_runner_detail::sum_episode_count(
        episode_reports, [](const episode_report_t &report) {
          return report.cajtucu_synthetic_market_step_count;
        });
  }

  [[nodiscard]] std::uint64_t cajtucu_numeraire_fallback_pair_count() const {
    return experiment_runner_detail::sum_episode_count(
        episode_reports, [](const episode_report_t &report) {
          return report.cajtucu_numeraire_fallback_pair_count;
        });
  }

  [[nodiscard]] double requested_notional_numeraire() const {
    return experiment_runner_detail::sum_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.requested_notional_numeraire;
        });
  }

  [[nodiscard]] double executed_notional_numeraire() const {
    return experiment_runner_detail::sum_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.executed_notional_numeraire;
        });
  }

  [[nodiscard]] double rejected_notional_numeraire() const {
    return experiment_runner_detail::sum_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.rejected_notional_numeraire;
        });
  }

  [[nodiscard]] double fill_ratio() const {
    const double requested = requested_notional_numeraire();
    return requested > 1.0e-12 ? executed_notional_numeraire() / requested
                               : 1.0;
  }

  [[nodiscard]] double fee_cost_numeraire() const {
    return experiment_runner_detail::sum_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.fee_cost_numeraire;
        });
  }

  [[nodiscard]] double spread_cost_numeraire() const {
    return experiment_runner_detail::sum_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.spread_cost_numeraire;
        });
  }

  [[nodiscard]] double slippage_cost_numeraire() const {
    return experiment_runner_detail::sum_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.slippage_cost_numeraire;
        });
  }

  [[nodiscard]] double mean_target_weight_error_l1() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.mean_target_weight_error_l1;
        });
  }

  [[nodiscard]] double mean_target_weight_error_linf() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.mean_target_weight_error_linf;
        });
  }

  [[nodiscard]] double mean_projection_mae() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports,
        [](const episode_report_t &report) { return report.projection_mae; });
  }

  [[nodiscard]] double mean_projection_rmse() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports,
        [](const episode_report_t &report) { return report.projection_rmse; });
  }

  [[nodiscard]] double mean_projection_signed_bias() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.projection_signed_bias;
        });
  }

  [[nodiscard]] double mean_projection_correlation() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.projection_correlation;
        });
  }

  [[nodiscard]] double mean_projection_directional_accuracy() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.projection_directional_accuracy;
        });
  }

  [[nodiscard]] double mean_projection_interval_coverage() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.projection_interval_coverage;
        });
  }

  [[nodiscard]] double mean_projection_interval_width() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.projection_mean_interval_width;
        });
  }

  [[nodiscard]] double mean_zero_return_baseline_mae() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.zero_return_baseline_mae;
        });
  }

  [[nodiscard]] double mean_zero_return_baseline_rmse() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.zero_return_baseline_rmse;
        });
  }

  [[nodiscard]] double mean_model_skill_vs_zero_mae() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.model_skill_vs_zero_mae;
        });
  }

  [[nodiscard]] double mean_model_skill_vs_zero_rmse() const {
    return experiment_runner_detail::mean_episode_metric(
        episode_reports, [](const episode_report_t &report) {
          return report.model_skill_vs_zero_rmse;
        });
  }
};

namespace experiment_runner_detail {

inline void
bind_experiment_identity_from_spec(replay_experiment_report_t &report,
                                   const episode_spec_t &spec) {
  if (detail::blank(spec.runtime_run_id) ||
      detail::blank(spec.environment_run_id)) {
    throw std::runtime_error(
        "[experiment_runner] bundle spec runtime/environment identity is "
        "required");
  }
  if (report.runtime_run_id.empty()) {
    report.runtime_run_id = spec.runtime_run_id;
  } else if (report.runtime_run_id != spec.runtime_run_id) {
    throw std::runtime_error(
        "[experiment_runner] experiment cannot mix runtime_run_id values");
  }
  if (report.environment_run_id.empty()) {
    report.environment_run_id = spec.environment_run_id;
  } else if (report.environment_run_id != spec.environment_run_id) {
    throw std::runtime_error(
        "[experiment_runner] experiment cannot mix environment_run_id values");
  }
}

[[nodiscard]] inline bool
failure_belongs_to_policy(const std::string &failure,
                          const std::string &policy_id) {
  return failure.find("|policy=" + policy_id + "|") != std::string::npos;
}

[[nodiscard]] inline std::string
require_failure_field(const std::string &failure, const std::string &key) {
  const std::string prefix = key + "=";
  const std::string infix = "|" + key + "=";
  std::size_t value_begin = std::string::npos;
  if (failure.rfind(prefix, 0) == 0) {
    value_begin = prefix.size();
  } else {
    const auto found = failure.find(infix);
    if (found != std::string::npos) {
      value_begin = found + infix.size();
    }
  }
  if (value_begin == std::string::npos) {
    throw std::runtime_error("[experiment_runner] failure record missing " +
                             key + " field");
  }
  const auto value_end = failure.find('|', value_begin);
  auto value = failure.substr(value_begin, value_end == std::string::npos
                                               ? std::string::npos
                                               : value_end - value_begin);
  if (detail::blank(value)) {
    throw std::runtime_error("[experiment_runner] failure record " + key +
                             " field is empty");
  }
  return value;
}

[[nodiscard]] inline std::int64_t
require_failure_i64(const std::string &failure, const std::string &key) {
  const auto value = require_failure_field(failure, key);
  std::size_t parsed = 0;
  long long out = 0;
  try {
    out = std::stoll(value, &parsed, 10);
  } catch (const std::exception &) {
    throw std::runtime_error("[experiment_runner] failure record " + key +
                             " field must be an integer");
  }
  if (parsed != value.size() || out < 0) {
    throw std::runtime_error("[experiment_runner] failure record " + key +
                             " field must be a nonnegative integer");
  }
  return static_cast<std::int64_t>(out);
}

inline void
bind_common_experiment_identity(replay_experiment_report_t &report) {
  for (const auto &episode_report : report.episode_reports) {
    validate_episode_step_report_identity(episode_report);
  }
  for (const auto &episode_report : report.episode_reports) {
    if (report.runtime_run_id.empty()) {
      report.runtime_run_id = episode_report.runtime_run_id;
    } else if (!episode_report.runtime_run_id.empty() &&
               episode_report.runtime_run_id != report.runtime_run_id) {
      throw std::runtime_error(
          "[experiment_runner] experiment cannot mix runtime_run_id values");
    }
  }
  for (const auto &episode_report : report.episode_reports) {
    if (report.environment_run_id.empty()) {
      report.environment_run_id = episode_report.environment_run_id;
    } else if (!episode_report.environment_run_id.empty() &&
               episode_report.environment_run_id != report.environment_run_id) {
      throw std::runtime_error(
          "[experiment_runner] experiment cannot mix environment_run_id "
          "values");
    }
  }
}

inline void build_policy_summaries(
    replay_experiment_report_t &report,
    const std::vector<replay_policy_factory_t> &policy_factories,
    const std::vector<std::uint64_t> &attempted_by_policy) {
  report.policy_summaries.clear();
  report.policy_summaries.reserve(policy_factories.size());
  for (std::size_t i = 0; i < policy_factories.size(); ++i) {
    const auto &factory = policy_factories[i];
    std::vector<episode_report_t> reports;
    reports.reserve(report.episode_reports.size());
    for (const auto &episode_report : report.episode_reports) {
      if (episode_report.policy_id == factory.policy_id) {
        reports.push_back(episode_report);
      }
    }
    std::uint64_t failures = 0;
    for (const auto &failure : report.failures) {
      if (failure_belongs_to_policy(failure, factory.policy_id)) {
        ++failures;
      }
    }

    replay_policy_summary_t summary{};
    summary.policy_id = factory.policy_id;
    summary.policy_kind = factory.policy_kind;
    summary.attempted_count =
        i < attempted_by_policy.size() ? attempted_by_policy[i] : 0;
    summary.completed_count = static_cast<std::uint64_t>(reports.size());
    summary.failed_count = failures;
    summary.mean_total_reward = mean_episode_metric(
        reports, [](const episode_report_t &r) { return r.total_reward; });
    summary.mean_total_log_growth = mean_episode_metric(
        reports, [](const episode_report_t &r) { return r.total_log_growth; });
    summary.mean_final_equity_numeraire =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.final_equity_numeraire;
        });
    summary.mean_max_drawdown = mean_episode_metric(
        reports, [](const episode_report_t &r) { return r.max_drawdown; });
    summary.mean_total_turnover = mean_episode_metric(
        reports, [](const episode_report_t &r) { return r.total_turnover; });
    summary.mean_total_transaction_cost_numeraire =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.total_transaction_cost_numeraire;
        });
    summary.cajtucu_valid_trace_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.cajtucu_valid_trace_count;
        });
    summary.cajtucu_invalid_trace_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.cajtucu_invalid_trace_count;
        });
    summary.cajtucu_missing_direct_pair_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.cajtucu_missing_direct_pair_count;
        });
    summary.cajtucu_numeraire_fallback_pair_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.cajtucu_numeraire_fallback_pair_count;
        });
    summary.cajtucu_nontradable_edge_reject_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.cajtucu_nontradable_edge_reject_count;
        });
    summary.cajtucu_below_min_notional_reject_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.cajtucu_below_min_notional_reject_count;
        });
    summary.cajtucu_above_max_notional_reject_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.cajtucu_above_max_notional_reject_count;
        });
    summary.cajtucu_insufficient_sell_units_reject_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.cajtucu_insufficient_sell_units_reject_count;
        });
    summary.cajtucu_insufficient_units_reject_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.cajtucu_insufficient_units_reject_count;
        });
    summary.cajtucu_invalid_sell_price_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.cajtucu_invalid_sell_price_count;
        });
    summary.cajtucu_large_equity_mismatch_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.cajtucu_large_equity_mismatch_count;
        });
    summary.cajtucu_synthetic_market_step_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.cajtucu_synthetic_market_step_count;
        });
    summary.requested_order_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.requested_order_count;
        });
    summary.executed_order_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.executed_order_count;
        });
    summary.rejected_order_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.rejected_order_count;
        });
    summary.partial_order_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.partial_order_count;
        });
    summary.requested_notional_numeraire =
        sum_episode_metric(reports, [](const episode_report_t &r) {
          return r.requested_notional_numeraire;
        });
    summary.executed_notional_numeraire =
        sum_episode_metric(reports, [](const episode_report_t &r) {
          return r.executed_notional_numeraire;
        });
    summary.rejected_notional_numeraire =
        sum_episode_metric(reports, [](const episode_report_t &r) {
          return r.rejected_notional_numeraire;
        });
    summary.partial_notional_numeraire =
        sum_episode_metric(reports, [](const episode_report_t &r) {
          return r.partial_notional_numeraire;
        });
    summary.fill_ratio = summary.requested_notional_numeraire > 1.0e-12
                             ? summary.executed_notional_numeraire /
                                   summary.requested_notional_numeraire
                             : 1.0;
    summary.fee_cost_numeraire =
        sum_episode_metric(reports, [](const episode_report_t &r) {
          return r.fee_cost_numeraire;
        });
    summary.spread_cost_numeraire =
        sum_episode_metric(reports, [](const episode_report_t &r) {
          return r.spread_cost_numeraire;
        });
    summary.slippage_cost_numeraire =
        sum_episode_metric(reports, [](const episode_report_t &r) {
          return r.slippage_cost_numeraire;
        });
    summary.cost_as_fraction_of_equity =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.cost_as_fraction_of_equity;
        });
    summary.cost_as_fraction_of_gross_return =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.cost_as_fraction_of_gross_return;
        });
    summary.mean_target_weight_error_l1 =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.mean_target_weight_error_l1;
        });
    summary.mean_target_weight_error_linf =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.mean_target_weight_error_linf;
        });
    summary.mean_post_execution_target_weight_sum =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.mean_post_execution_target_weight_sum;
        });
    summary.mean_post_execution_numeraire_weight =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.mean_post_execution_numeraire_weight;
        });
    summary.numeraire_shortfall_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.numeraire_shortfall_count;
        });
    summary.mean_ledger_before_equity =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.mean_ledger_before_equity;
        });
    summary.mean_ledger_after_execution_equity =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.mean_ledger_after_execution_equity;
        });
    summary.mean_ledger_after_realization_equity =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.mean_ledger_after_realization_equity;
        });
    summary.max_ledger_equity_reconciliation_error =
        max_episode_metric(reports, [](const episode_report_t &r) {
          return r.max_ledger_equity_reconciliation_error;
        });
    summary.numeraire_units_before =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.numeraire_units_before;
        });
    summary.numeraire_units_after =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.numeraire_units_after;
        });
    summary.unit_nonnegativity_violation_count =
        sum_episode_count(reports, [](const episode_report_t &r) {
          return r.unit_nonnegativity_violation_count;
        });
    summary.mean_projection_mae = mean_episode_metric(
        reports, [](const episode_report_t &r) { return r.projection_mae; });
    summary.mean_projection_rmse = mean_episode_metric(
        reports, [](const episode_report_t &r) { return r.projection_rmse; });
    summary.mean_projection_signed_bias =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.projection_signed_bias;
        });
    summary.mean_projection_correlation =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.projection_correlation;
        });
    summary.mean_projection_directional_accuracy =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.projection_directional_accuracy;
        });
    summary.mean_projection_interval_coverage =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.projection_interval_coverage;
        });
    summary.mean_projection_interval_width =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.projection_mean_interval_width;
        });
    summary.mean_zero_return_baseline_mae =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.zero_return_baseline_mae;
        });
    summary.mean_zero_return_baseline_rmse =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.zero_return_baseline_rmse;
        });
    summary.mean_model_skill_vs_zero_mae =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.model_skill_vs_zero_mae;
        });
    summary.mean_model_skill_vs_zero_rmse =
        mean_episode_metric(reports, [](const episode_report_t &r) {
          return r.model_skill_vs_zero_rmse;
        });
    report.policy_summaries.push_back(std::move(summary));
  }
}

inline void
validate_replay_experiment_report(const replay_experiment_report_t &report) {
  if (detail::blank(report.experiment_id)) {
    throw std::runtime_error("[experiment_runner] experiment_id is required");
  }
  if (detail::blank(report.runtime_run_id) ||
      detail::blank(report.environment_run_id)) {
    throw std::runtime_error(
        "[experiment_runner] experiment runtime_run_id and environment_run_id "
        "are required");
  }
  if (report.completed_count != report.episode_reports.size()) {
    throw std::runtime_error(
        "[experiment_runner] completed_count must match episode report count");
  }
  if (report.attempted_count !=
      report.completed_count + report.failures.size()) {
    throw std::runtime_error(
        "[experiment_runner] attempted_count must match completed plus failed "
        "tasks");
  }
  if (report.policy_summaries.empty()) {
    throw std::runtime_error(
        "[experiment_runner] at least one policy summary is required");
  }

  std::unordered_set<std::string> seen_policy_ids;
  std::uint64_t attempted_total = 0;
  std::uint64_t completed_total = 0;
  std::uint64_t failed_total = 0;
  for (const auto &summary : report.policy_summaries) {
    if (detail::blank(summary.policy_id) ||
        !seen_policy_ids.insert(summary.policy_id).second) {
      throw std::runtime_error(
          "[experiment_runner] policy summaries must have unique nonempty ids");
    }
    if (summary.attempted_count !=
        summary.completed_count + summary.failed_count) {
      throw std::runtime_error(
          "[experiment_runner] policy summary attempted_count must match "
          "completed plus failed count");
    }
    std::uint64_t observed_completed = 0;
    for (const auto &episode_report : report.episode_reports) {
      validate_episode_step_report_identity(episode_report);
      if (episode_report.policy_id == summary.policy_id) {
        ++observed_completed;
        if (episode_report.policy_kind != summary.policy_kind) {
          throw std::runtime_error(
              "[experiment_runner] episode policy kind must match policy "
              "summary kind");
        }
      }
    }
    if (observed_completed != summary.completed_count) {
      throw std::runtime_error(
          "[experiment_runner] policy summary completed_count must match "
          "episode reports");
    }
    std::uint64_t observed_failed = 0;
    for (const auto &failure : report.failures) {
      if (failure_belongs_to_policy(failure, summary.policy_id)) {
        ++observed_failed;
      }
    }
    if (observed_failed != summary.failed_count) {
      throw std::runtime_error(
          "[experiment_runner] policy summary failed_count must match failure "
          "records");
    }
    attempted_total += summary.attempted_count;
    completed_total += summary.completed_count;
    failed_total += summary.failed_count;
  }

  std::unordered_set<std::int64_t> seen_task_indices;
  seen_task_indices.reserve(report.episode_reports.size());
  for (const auto &episode_report : report.episode_reports) {
    if (episode_report.runtime_run_id != report.runtime_run_id ||
        episode_report.environment_run_id != report.environment_run_id) {
      throw std::runtime_error(
          "[experiment_runner] episode report runtime/environment identity "
          "must match experiment report");
    }
    if (episode_report.experiment_task_index < 0 ||
        episode_report.experiment_bundle_index < 0 ||
        episode_report.experiment_policy_index < 0) {
      throw std::runtime_error(
          "[experiment_runner] completed episode task indices must be "
          "nonnegative");
    }
    if (static_cast<std::uint64_t>(episode_report.experiment_task_index) >=
        report.attempted_count) {
      throw std::runtime_error(
          "[experiment_runner] completed episode task index exceeds attempted "
          "task count");
    }
    if (!seen_task_indices.insert(episode_report.experiment_task_index)
             .second) {
      throw std::runtime_error(
          "[experiment_runner] completed episode task indices must be unique");
    }
    if (static_cast<std::size_t>(episode_report.experiment_policy_index) >=
        report.policy_summaries.size()) {
      throw std::runtime_error(
          "[experiment_runner] completed episode policy index is outside "
          "policy summaries");
    }
    const auto &summary = report.policy_summaries[static_cast<std::size_t>(
        episode_report.experiment_policy_index)];
    if (summary.policy_id != episode_report.policy_id ||
        summary.policy_kind != episode_report.policy_kind) {
      throw std::runtime_error(
          "[experiment_runner] completed episode policy index must identify "
          "the matching policy summary");
    }
    const auto expected_task_index =
        episode_report.experiment_bundle_index *
            static_cast<std::int64_t>(report.policy_summaries.size()) +
        episode_report.experiment_policy_index;
    if (episode_report.experiment_task_index != expected_task_index) {
      throw std::runtime_error(
          "[experiment_runner] completed episode task index must match "
          "bundle/policy task geometry");
    }
  }

  if (attempted_total != report.attempted_count ||
      completed_total != report.completed_count ||
      failed_total != report.failures.size()) {
    throw std::runtime_error(
        "[experiment_runner] policy summary totals must match experiment "
        "counts");
  }
  for (const auto &episode_report : report.episode_reports) {
    if (!seen_policy_ids.contains(episode_report.policy_id)) {
      throw std::runtime_error(
          "[experiment_runner] episode report policy_id missing policy "
          "summary");
    }
  }
  for (const auto &failure : report.failures) {
    const auto task_index = require_failure_i64(failure, "task");
    const auto bundle_index = require_failure_i64(failure, "bundle");
    const auto policy_index = require_failure_i64(failure, "policy_index");
    const auto policy_id = require_failure_field(failure, "policy");
    const auto policy_kind = require_failure_field(failure, "policy_kind");
    if (static_cast<std::uint64_t>(task_index) >= report.attempted_count) {
      throw std::runtime_error(
          "[experiment_runner] failed task index exceeds attempted task count");
    }
    if (!seen_task_indices.insert(task_index).second) {
      throw std::runtime_error(
          "[experiment_runner] completed and failed task indices must be "
          "unique");
    }
    if (static_cast<std::size_t>(policy_index) >=
        report.policy_summaries.size()) {
      throw std::runtime_error(
          "[experiment_runner] failed task policy index is outside policy "
          "summaries");
    }
    const auto &summary =
        report.policy_summaries[static_cast<std::size_t>(policy_index)];
    if (summary.policy_id != policy_id ||
        policy_kind != policy_kind_name(summary.policy_kind) ||
        !failure_belongs_to_policy(failure, summary.policy_id)) {
      throw std::runtime_error(
          "[experiment_runner] failed task policy index must identify the "
          "matching policy summary");
    }
    const auto expected_task_index =
        bundle_index *
            static_cast<std::int64_t>(report.policy_summaries.size()) +
        policy_index;
    if (task_index != expected_task_index) {
      throw std::runtime_error(
          "[experiment_runner] failed task index must match bundle/policy task "
          "geometry");
    }
  }
}

} // namespace experiment_runner_detail

[[nodiscard]] inline replay_experiment_report_t run_replay_experiment(
    std::string experiment_id,
    const std::vector<replay::replay_episode_bundle_t> &bundles,
    const std::vector<replay_policy_factory_t> &policy_factories,
    const replay_experiment_options_t &options = {}) {
  if (detail::blank(experiment_id)) {
    throw std::runtime_error("[experiment_runner] experiment_id is required");
  }
  if (bundles.empty()) {
    throw std::runtime_error(
        "[experiment_runner] at least one bundle is required");
  }
  if (policy_factories.empty()) {
    throw std::runtime_error(
        "[experiment_runner] at least one policy factory is required");
  }
  experiment_runner_detail::validate_policy_factories(policy_factories);

  replay_experiment_report_t out{};
  out.experiment_id = std::move(experiment_id);
  out.requested_max_parallel_jobs = options.max_parallel_jobs;
  out.resolved_parallelism =
      experiment_runner_detail::resolve_parallelism(options.max_parallel_jobs);

  std::vector<std::uint64_t> attempted_by_policy(policy_factories.size(), 0);
  std::vector<experiment_runner_detail::replay_experiment_task_t> tasks;
  tasks.reserve(bundles.size() * policy_factories.size());
  for (std::size_t bundle_index = 0; bundle_index < bundles.size();
       ++bundle_index) {
    replay::validate_replay_episode_bundle(bundles[bundle_index]);
    experiment_runner_detail::bind_experiment_identity_from_spec(
        out, bundles[bundle_index].spec);
    for (std::size_t policy_index = 0; policy_index < policy_factories.size();
         ++policy_index) {
      tasks.push_back({
          .task_index = tasks.size(),
          .bundle_index = bundle_index,
          .policy_index = policy_index,
      });
      ++attempted_by_policy[policy_index];
    }
  }
  out.attempted_count = static_cast<std::uint64_t>(tasks.size());

  const auto handle_result =
      [&](experiment_runner_detail::replay_experiment_task_result_t result) {
        if (result.completed) {
          out.episode_reports.push_back(std::move(result.report));
          ++out.completed_count;
          return;
        }
        out.failures.push_back(std::move(result.failure));
        if (!options.continue_on_failure) {
          throw std::runtime_error(out.failures.back());
        }
      };

  const std::size_t parallelism = out.resolved_parallelism;

  for (std::size_t next = 0; next < tasks.size();) {
    if (parallelism == 1) {
      const auto task = tasks[next++];
      handle_result(experiment_runner_detail::run_task(
          task, bundles[task.bundle_index], policy_factories[task.policy_index],
          options.episode_options));
      continue;
    }

    std::vector<
        std::future<experiment_runner_detail::replay_experiment_task_result_t>>
        futures;
    futures.reserve(parallelism);
    for (std::size_t i = 0; i < parallelism && next < tasks.size(); ++i) {
      const auto task = tasks[next++];
      auto bundle = bundles[task.bundle_index];
      auto factory = policy_factories[task.policy_index];
      auto episode_options = options.episode_options;
      futures.push_back(
          std::async(std::launch::async, [task, bundle = std::move(bundle),
                                          factory = std::move(factory),
                                          episode_options]() mutable {
            return experiment_runner_detail::run_task(
                task, std::move(bundle), std::move(factory), episode_options);
          }));
    }
    for (auto &future : futures) {
      handle_result(future.get());
    }
  }

  experiment_runner_detail::build_policy_summaries(out, policy_factories,
                                                   attempted_by_policy);
  experiment_runner_detail::bind_common_experiment_identity(out);
  experiment_runner_detail::validate_replay_experiment_report(out);
  return out;
}

[[nodiscard]] inline replay_experiment_report_t run_replay_experiment(
    std::string experiment_id, replay_bundle_source_iface_t &bundle_source,
    const std::vector<replay_policy_factory_t> &policy_factories,
    const replay_experiment_options_t &options = {}) {
  if (detail::blank(experiment_id)) {
    throw std::runtime_error("[experiment_runner] experiment_id is required");
  }
  if (policy_factories.empty()) {
    throw std::runtime_error(
        "[experiment_runner] at least one policy factory is required");
  }
  experiment_runner_detail::validate_policy_factories(policy_factories);

  replay_experiment_report_t out{};
  out.experiment_id = std::move(experiment_id);
  out.requested_max_parallel_jobs = options.max_parallel_jobs;
  out.resolved_parallelism =
      experiment_runner_detail::resolve_parallelism(options.max_parallel_jobs);
  std::vector<std::uint64_t> attempted_by_policy(policy_factories.size(), 0);

  const std::size_t parallelism = out.resolved_parallelism;

  const auto handle_result =
      [&](experiment_runner_detail::replay_experiment_task_result_t result) {
        if (result.completed) {
          out.episode_reports.push_back(std::move(result.report));
          ++out.completed_count;
          return;
        }
        out.failures.push_back(std::move(result.failure));
        if (!options.continue_on_failure) {
          throw std::runtime_error(out.failures.back());
        }
      };

  std::vector<
      std::future<experiment_runner_detail::replay_experiment_task_result_t>>
      futures;
  futures.reserve(parallelism);
  auto drain_futures = [&]() {
    for (auto &future : futures) {
      handle_result(future.get());
    }
    futures.clear();
  };

  std::size_t bundle_index = 0;
  std::size_t task_index = 0;
  while (auto bundle = bundle_source.next_bundle()) {
    replay::validate_replay_episode_bundle(*bundle);
    experiment_runner_detail::bind_experiment_identity_from_spec(out,
                                                                 bundle->spec);
    for (std::size_t policy_index = 0; policy_index < policy_factories.size();
         ++policy_index) {
      experiment_runner_detail::replay_experiment_task_t task{
          .task_index = task_index++,
          .bundle_index = bundle_index,
          .policy_index = policy_index,
      };
      ++out.attempted_count;
      ++attempted_by_policy[policy_index];
      if (parallelism == 1) {
        handle_result(experiment_runner_detail::run_task(
            task, *bundle, policy_factories[policy_index],
            options.episode_options));
        continue;
      }
      auto task_bundle = *bundle;
      auto factory = policy_factories[policy_index];
      auto episode_options = options.episode_options;
      futures.push_back(
          std::async(std::launch::async,
                     [task, task_bundle = std::move(task_bundle),
                      factory = std::move(factory), episode_options]() mutable {
                       return experiment_runner_detail::run_task(
                           task, std::move(task_bundle), std::move(factory),
                           episode_options);
                     }));
      if (futures.size() >= parallelism) {
        drain_futures();
      }
    }
    ++bundle_index;
  }
  drain_futures();

  if (out.attempted_count == 0) {
    throw std::runtime_error(
        "[experiment_runner] bundle source produced no bundles: " +
        bundle_source.source_id());
  }
  experiment_runner_detail::build_policy_summaries(out, policy_factories,
                                                   attempted_by_policy);
  experiment_runner_detail::bind_common_experiment_identity(out);
  experiment_runner_detail::validate_replay_experiment_report(out);
  return out;
}

} // namespace cuwacunu::kikijyeba::environment
