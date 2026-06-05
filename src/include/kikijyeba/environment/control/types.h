// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "cajtucu/execution/assembly.h"
#include "kikijyeba/protocol/component_stream.h"
#include "wikimyei/observer/belief/types.h"
#include "wikimyei/observer/utility/nodelift_residual_quality.h"
#include "wikimyei/observer/utility/projection_validation.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/types.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/utility/belief_reporter.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/utility/portfolio_ledger.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/utility/risk_gate.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/utility/spot_rebalance_router.h"

namespace cuwacunu::kikijyeba::environment {

namespace belief = cuwacunu::wikimyei::observer::belief;
namespace observer = cuwacunu::wikimyei::observer;
namespace portfolio = cuwacunu::wikimyei::policy::portfolio;
namespace cajtucu_execution = cuwacunu::cajtucu::execution;
namespace sdu =
    cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility;
namespace execution = sdu::execution::spot_rebalance_router;
namespace accounting = sdu::accounting::portfolio_ledger;
namespace monitoring = sdu::monitoring::belief_reporter;
namespace risk = sdu::risk::risk_gate;

inline constexpr const char *kReplayEnvironmentFamily =
    "kikijyeba.environment.replay";
inline constexpr const char *kReplayEnvironmentAssemblyId =
    "kikijyeba.environment.replay.v1";
inline constexpr const char *kReplayStepArtifactSchema =
    "kikijyeba.environment.replay.step_artifact.v1";
inline constexpr const char *kReplayEpisodeArtifactSchema =
    "kikijyeba.environment.replay.episode_artifact.v1";
inline constexpr const char *kReplayExperimentArtifactSchema =
    "kikijyeba.environment.replay.cajtucu_ready_experiment_artifact.v1";
inline constexpr const char *kReplayPolicyComparisonArtifactSchema =
    "kikijyeba.environment.replay.policy_comparison_artifact.v1";
inline constexpr const char *kTargetWeightsActionSchema =
    "kikijyeba.environment.action.target_weights.v1";
inline constexpr const char *kDirectEdgeRealizedReturnTruthV1 =
    "direct_edge_realized_return_truth_v1";

enum class world_mode_t {
  historical_replay,
};

[[nodiscard]] inline const char *world_mode_name(world_mode_t mode) {
  switch (mode) {
  case world_mode_t::historical_replay:
    return "historical_replay";
  }
  return "unknown";
}

enum class policy_kind_t {
  deterministic_allocator,
  baseline,
  reinforcement_learning,
  external,
};

[[nodiscard]] inline const char *policy_kind_name(policy_kind_t kind) {
  switch (kind) {
  case policy_kind_t::deterministic_allocator:
    return "deterministic_allocator";
  case policy_kind_t::baseline:
    return "baseline";
  case policy_kind_t::reinforcement_learning:
    return "reinforcement_learning";
  case policy_kind_t::external:
    return "external";
  }
  return "unknown";
}

struct artifact_ref_t {
  std::string artifact_id{};
  std::string artifact_path{};
  std::string artifact_digest{};
  std::string schema_id{};
};

struct requested_episode_range_t {
  std::optional<std::int64_t> anchor_index_begin{std::nullopt};
  std::optional<std::int64_t> anchor_index_end{std::nullopt}; // exclusive
  std::optional<std::int64_t> source_key_begin{std::nullopt};
  std::optional<std::int64_t> source_key_end{std::nullopt};
};

struct accepted_episode_range_t {
  protocol::component_stream_cursor_t cursor{};
  std::int64_t anchor_index_begin{-1};
  std::int64_t anchor_index_end{-1}; // exclusive
  std::vector<std::string> anchor_keys{};
};

struct realized_return_projection_spec_t {
  std::string truth_id{};
  std::string reference_node_id{};
  std::vector<std::string> edge_ids{};
  torch::Tensor signs{}; // [A], +1 for asset/reference, -1 for reference/asset
};

struct episode_spec_t {
  std::string episode_id{};
  std::string protocol_id{};
  std::string runtime_run_id{};
  std::string environment_run_id{};
  std::uint64_t episode_index{0};

  protocol::component_stream_wave_t requested_wave{};
  requested_episode_range_t requested_range{};
  accepted_episode_range_t accepted_range{};
  world_mode_t world_mode{world_mode_t::historical_replay};

  std::string graph_order_fingerprint{};
  std::vector<std::string> graph_node_ids{};
  std::vector<std::string> risky_node_ids{};
  belief::BasePolicy base_policy{};
  realized_return_projection_spec_t realized_return_projection{};
  portfolio::PortfolioConstraints constraints{};

  double initial_equity_base{1.0};
  bool simulate_fees{true};
  bool simulate_spread{true};
  bool simulate_slippage{true};
  bool require_projection_validation{true};
};

struct observation_t {
  std::string anchor_key{};
  portfolio::timestamp_ms_t timestamp_ms{0};

  std::int64_t observation_anchor_index{-1};
  std::int64_t next_realization_anchor_index{-1};
  portfolio::timestamp_ms_t knowledge_timestamp_ms{0};
  portfolio::timestamp_ms_t realization_available_after_timestamp_ms{0};

  std::optional<artifact_ref_t> mdn_artifact{};
  std::optional<belief::NodeLiftPotentialBelief> nodelift_potential_belief{};
  std::optional<belief::AllocationBelief> allocation_belief{};

  portfolio::PortfolioState portfolio_state{};
  portfolio::MarketState market_state{};
  execution::spot_edge_market_state_t edge_market_state{};
};

struct action_t {
  std::string policy_id{};
  std::string method_id{};
  policy_kind_t policy_kind{policy_kind_t::external};
  portfolio::timestamp_ms_t decision_timestamp_ms{0};
  std::vector<std::string> node_ids{};
  portfolio::node_id_t base_reserve_node_id{};

  torch::Tensor target_weights{}; // [A], risky assets only
  double target_base_reserve_weight{1.0};
  double expected_log_growth{std::numeric_limits<double>::quiet_NaN()};
  double expected_arithmetic_return{std::numeric_limits<double>::quiet_NaN()};
  double cvar_loss{std::numeric_limits<double>::quiet_NaN()};
  double estimated_transaction_cost{std::numeric_limits<double>::quiet_NaN()};
  std::vector<std::string> diagnostics_notes{};
  std::vector<std::string> diagnostics_warnings{};
  std::vector<std::string> diagnostics_failures{};

  bool risk_gate_evaluated{false};
  risk::risk_gate_result_t risk_gate{};
  bool rebalance_plan_available{false};
  execution::spot_rebalance_plan_t rebalance_plan{};
  std::string rebalance_plan_source{};
  bool decision_report_available{false};
  monitoring::belief_decision_report_t decision_report{};

  std::string action_schema_id{kTargetWeightsActionSchema};
};

struct reward_options_t {
  double lambda_drawdown{0.0};
  double lambda_transaction_cost{1.0};
  double lambda_turnover{0.0};
  double invalid_action_penalty{1.0};
  double eps{1.0e-12};
};

struct reward_t {
  double log_growth_reward{0.0};
  double drawdown_penalty{0.0};
  double transaction_cost_penalty{0.0};
  double turnover_penalty{0.0};
  double invalid_action_penalty{0.0};
  double total{0.0};
};

inline void validate_reward_options(const reward_options_t &options) {
  if (!std::isfinite(options.lambda_drawdown) ||
      !std::isfinite(options.lambda_transaction_cost) ||
      !std::isfinite(options.lambda_turnover) ||
      !std::isfinite(options.invalid_action_penalty) ||
      !std::isfinite(options.eps)) {
    throw std::runtime_error("[environment] reward options must be finite");
  }
  if (options.lambda_drawdown < 0.0 || options.lambda_transaction_cost < 0.0 ||
      options.lambda_turnover < 0.0 || options.invalid_action_penalty < 0.0) {
    throw std::runtime_error(
        "[environment] reward penalty coefficients must be nonnegative");
  }
  if (options.eps <= 0.0) {
    throw std::runtime_error("[environment] reward eps must be positive");
  }
}

struct step_info_t {
  std::string step_artifact_schema_id{kReplayStepArtifactSchema};
  std::string anchor_key{};
  std::int64_t anchor_index{-1};

  portfolio::PortfolioState portfolio_before{};
  portfolio::TargetPortfolio target{};
  risk::risk_gate_result_t risk_gate{};
  execution::spot_rebalance_plan_t rebalance_plan{};
  std::vector<accounting::executed_fill_t> fills{};
  cajtucu_execution::execution_trace_t execution_trace{};
  portfolio::PortfolioState portfolio_after{};

  observer::projection_validation_t projection_validation{};
  observer::nodelift_residual_quality_t residual_quality{};
  monitoring::belief_decision_report_t decision_report{};
  std::string rebalance_plan_source{};
  bool rebalance_plan_enforced{false};
  std::string execution_model{};
  bool cajtucu_execution_trace_available{false};

  double realized_log_growth{0.0};
  double realized_arithmetic_return{0.0};
  double transaction_cost_base{0.0};
  double turnover{0.0};
  bool invalid_action{false};

  bool risk_gate_evaluated{false};
  std::vector<std::string> warnings{};
  std::vector<std::string> failures{};
};

using info_t = step_info_t;
using evidence_t = step_info_t;

struct transition_t {
  observation_t next_observation{};
  reward_t reward{};
  bool done{false};
  step_info_t info{};
};

struct step_report_t {
  std::string step_artifact_schema_id{kReplayStepArtifactSchema};
  std::uint64_t step_index{0};
  std::string episode_id{};
  std::string protocol_id{};
  std::string runtime_run_id{};
  std::string environment_run_id{};
  std::string policy_id{};
  std::string method_id{};
  policy_kind_t policy_kind{policy_kind_t::external};
  world_mode_t world_mode{world_mode_t::historical_replay};
  portfolio::timestamp_ms_t action_decision_timestamp_ms{0};
  std::string anchor_key{};
  std::int64_t observation_anchor_index{-1};
  std::int64_t next_realization_anchor_index{-1};
  portfolio::timestamp_ms_t knowledge_timestamp_ms{0};
  portfolio::timestamp_ms_t realization_available_after_timestamp_ms{0};
  std::string accepted_cursor_wave_id{};
  std::string accepted_cursor_wave_mode{};
  std::string accepted_cursor_kind{};
  std::string accepted_cursor_scope{};
  std::string accepted_source_range_policy{};
  std::string accepted_source_order_policy{};
  std::string accepted_batch_cursor_token{};
  std::int64_t accepted_anchor_index_begin{-1};
  std::int64_t accepted_anchor_index_end{-1};
  std::int64_t accepted_cursor_offset{-1};

  std::string action_schema_id{};
  std::string target_risky_node_weights{};
  std::string target_base_reserve_node_id{};
  double target_base_reserve_weight{std::numeric_limits<double>::quiet_NaN()};

  double portfolio_equity_before{std::numeric_limits<double>::quiet_NaN()};
  double portfolio_equity_after{std::numeric_limits<double>::quiet_NaN()};
  double realized_log_growth{std::numeric_limits<double>::quiet_NaN()};
  double realized_arithmetic_return{std::numeric_limits<double>::quiet_NaN()};
  double transaction_cost_base{std::numeric_limits<double>::quiet_NaN()};
  double turnover{std::numeric_limits<double>::quiet_NaN()};
  bool invalid_action{false};

  bool risk_gate_evaluated{false};
  bool risk_gate_allow_trading{true};
  bool risk_gate_force_base_reserve_fallback{false};
  std::uint64_t risk_gate_reason_count{0};
  std::uint64_t risk_gate_warning_count{0};
  std::string risk_gate_reasons{};
  std::string risk_gate_warnings{};

  bool rebalance_plan_valid{false};
  std::string rebalance_plan_source{};
  bool rebalance_plan_enforced{false};
  std::string execution_model{};
  std::uint64_t rebalance_order_count{0};
  std::uint64_t rebalance_skipped_count{0};
  double rebalance_requested_turnover_weight{
      std::numeric_limits<double>::quiet_NaN()};
  double rebalance_routed_turnover_weight{
      std::numeric_limits<double>::quiet_NaN()};
  double rebalance_estimated_transaction_cost_base{
      std::numeric_limits<double>::quiet_NaN()};

  std::uint64_t fill_count{0};
  double fill_gross_notional_base{0.0};
  double fill_fee_base{0.0};
  bool cajtucu_execution_trace_available{false};
  std::string cajtucu_backend_id{};
  std::string cajtucu_trace_id{};
  std::string cajtucu_market_source_id{};
  bool cajtucu_synthetic_direct_edges{false};
  bool cajtucu_trace_valid{true};
  std::uint64_t cajtucu_failure_count{0};
  double cajtucu_ledger_intent_equity_difference_base{0.0};
  bool cajtucu_ledger_intent_equity_mismatch{false};
  std::uint64_t cajtucu_order_count{0};
  std::uint64_t cajtucu_executed_order_count{0};
  std::uint64_t cajtucu_fill_count{0};
  std::uint64_t cajtucu_rejected_fill_count{0};
  std::uint64_t cajtucu_partial_fill_count{0};
  std::uint64_t cajtucu_missing_direct_reserve_edge_count{0};
  std::uint64_t cajtucu_nontradable_edge_reject_count{0};
  std::uint64_t cajtucu_below_min_notional_reject_count{0};
  std::uint64_t cajtucu_above_max_notional_reject_count{0};
  std::uint64_t cajtucu_insufficient_reserve_reject_count{0};
  std::uint64_t cajtucu_insufficient_units_reject_count{0};
  std::uint64_t cajtucu_invalid_sell_price_count{0};
  std::uint64_t cajtucu_large_equity_mismatch_count{0};
  double cajtucu_requested_notional_base{0.0};
  double cajtucu_executed_notional_base{0.0};
  double cajtucu_rejected_notional_base{0.0};
  double cajtucu_partial_notional_base{0.0};
  double cajtucu_fill_ratio{std::numeric_limits<double>::quiet_NaN()};
  double cajtucu_total_fee_base{0.0};
  double cajtucu_total_spread_cost_base{0.0};
  double cajtucu_total_slippage_base{0.0};
  double cajtucu_total_transaction_cost_base{0.0};
  double target_weight_error_l1{std::numeric_limits<double>::quiet_NaN()};
  double target_weight_error_linf{std::numeric_limits<double>::quiet_NaN()};
  double post_execution_risky_weight_sum{
      std::numeric_limits<double>::quiet_NaN()};
  double post_execution_base_reserve_weight{
      std::numeric_limits<double>::quiet_NaN()};
  std::uint64_t reserve_shortfall_count{0};
  double ledger_before_equity{std::numeric_limits<double>::quiet_NaN()};
  double ledger_after_execution_equity{
      std::numeric_limits<double>::quiet_NaN()};
  double ledger_after_realization_equity{
      std::numeric_limits<double>::quiet_NaN()};
  double ledger_equity_reconciliation_error{
      std::numeric_limits<double>::quiet_NaN()};
  double base_reserve_units_before{std::numeric_limits<double>::quiet_NaN()};
  double base_reserve_units_after{std::numeric_limits<double>::quiet_NaN()};
  std::uint64_t unit_nonnegativity_violation_count{0};

  double reward_log_growth{std::numeric_limits<double>::quiet_NaN()};
  double reward_drawdown_penalty{std::numeric_limits<double>::quiet_NaN()};
  double reward_transaction_cost_penalty{
      std::numeric_limits<double>::quiet_NaN()};
  double reward_turnover_penalty{std::numeric_limits<double>::quiet_NaN()};
  double reward_invalid_action_penalty{
      std::numeric_limits<double>::quiet_NaN()};
  double reward_total{std::numeric_limits<double>::quiet_NaN()};

  double projection_mae{std::numeric_limits<double>::quiet_NaN()};
  double projection_rmse{std::numeric_limits<double>::quiet_NaN()};
  double projection_signed_bias{std::numeric_limits<double>::quiet_NaN()};
  double projection_correlation{std::numeric_limits<double>::quiet_NaN()};
  double projection_directional_accuracy{
      std::numeric_limits<double>::quiet_NaN()};
  double projection_interval_coverage{std::numeric_limits<double>::quiet_NaN()};
  double projection_mean_interval_width{
      std::numeric_limits<double>::quiet_NaN()};
  double zero_return_baseline_mae{std::numeric_limits<double>::quiet_NaN()};
  double zero_return_baseline_rmse{std::numeric_limits<double>::quiet_NaN()};
  double zero_return_baseline_signed_bias{
      std::numeric_limits<double>::quiet_NaN()};
  double zero_return_baseline_directional_accuracy{
      std::numeric_limits<double>::quiet_NaN()};
  double model_skill_vs_zero_mae{std::numeric_limits<double>::quiet_NaN()};
  double model_skill_vs_zero_rmse{std::numeric_limits<double>::quiet_NaN()};

  bool residual_quality_available{false};
  bool residual_quality_valid{false};
  double residual_mean_residual_energy{
      std::numeric_limits<double>::quiet_NaN()};
  double residual_max_residual_energy{std::numeric_limits<double>::quiet_NaN()};

  std::string decision_report_schema_id{};
  std::uint64_t decision_report_text_bytes{0};

  std::uint64_t warning_count{0};
  std::uint64_t failure_count{0};
  std::string warnings{};
  std::string failures{};
};

struct episode_report_t {
  std::string episode_artifact_schema_id{kReplayEpisodeArtifactSchema};
  std::string episode_id{};
  std::string protocol_id{};
  std::string runtime_run_id{};
  std::string environment_run_id{};
  std::string policy_id{};
  std::string method_id{};
  policy_kind_t policy_kind{policy_kind_t::external};
  world_mode_t world_mode{world_mode_t::historical_replay};
  std::string graph_order_fingerprint{};
  std::string graph_node_ids{};
  std::string risky_node_ids{};
  std::string base_reserve_node_id{};
  std::int64_t experiment_task_index{-1};
  std::int64_t experiment_bundle_index{-1};
  std::int64_t experiment_policy_index{-1};

  std::int64_t requested_anchor_index_begin{-1};
  std::int64_t requested_anchor_index_end{-1};
  std::int64_t requested_source_key_begin{-1};
  std::int64_t requested_source_key_end{-1};

  std::string accepted_cursor_wave_id{};
  std::string accepted_cursor_wave_mode{};
  std::string accepted_cursor_kind{};
  std::string accepted_cursor_scope{};
  std::string accepted_source_range_policy{};
  std::string accepted_source_order_policy{};
  std::string accepted_batch_cursor_token{};
  std::int64_t accepted_anchor_index_begin{-1};
  std::int64_t accepted_anchor_index_end{-1};
  std::string accepted_anchor_keys{};
  std::string realized_return_truth_id{};
  std::string realized_return_projection_reference_node_id{};
  std::string realized_return_projection_edge_ids{};
  std::string realized_return_projection_signs{};

  std::uint64_t transition_count{0};
  std::vector<step_report_t> step_reports{};
  double total_reward{0.0};
  double total_log_growth{0.0};
  double final_equity_base{std::numeric_limits<double>::quiet_NaN()};
  double max_drawdown{0.0};
  double total_turnover{0.0};
  double total_transaction_cost_base{0.0};
  std::uint64_t cajtucu_valid_trace_count{0};
  std::uint64_t cajtucu_invalid_trace_count{0};
  std::uint64_t cajtucu_missing_direct_reserve_edge_count{0};
  std::uint64_t cajtucu_nontradable_edge_reject_count{0};
  std::uint64_t cajtucu_below_min_notional_reject_count{0};
  std::uint64_t cajtucu_above_max_notional_reject_count{0};
  std::uint64_t cajtucu_insufficient_reserve_reject_count{0};
  std::uint64_t cajtucu_insufficient_units_reject_count{0};
  std::uint64_t cajtucu_invalid_sell_price_count{0};
  std::uint64_t cajtucu_large_equity_mismatch_count{0};
  std::uint64_t cajtucu_synthetic_market_step_count{0};
  std::uint64_t requested_order_count{0};
  std::uint64_t executed_order_count{0};
  std::uint64_t rejected_order_count{0};
  std::uint64_t partial_order_count{0};
  double requested_notional_base{0.0};
  double executed_notional_base{0.0};
  double rejected_notional_base{0.0};
  double partial_notional_base{0.0};
  double fill_ratio{std::numeric_limits<double>::quiet_NaN()};
  double fee_cost_base{0.0};
  double spread_cost_base{0.0};
  double slippage_cost_base{0.0};
  double cost_as_fraction_of_equity{std::numeric_limits<double>::quiet_NaN()};
  double cost_as_fraction_of_gross_return{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_target_weight_error_l1{std::numeric_limits<double>::quiet_NaN()};
  double mean_target_weight_error_linf{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_post_execution_risky_weight_sum{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_post_execution_base_reserve_weight{
      std::numeric_limits<double>::quiet_NaN()};
  std::uint64_t reserve_shortfall_count{0};
  double mean_ledger_before_equity{std::numeric_limits<double>::quiet_NaN()};
  double mean_ledger_after_execution_equity{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_ledger_after_realization_equity{
      std::numeric_limits<double>::quiet_NaN()};
  double max_ledger_equity_reconciliation_error{0.0};
  double base_reserve_units_before{std::numeric_limits<double>::quiet_NaN()};
  double base_reserve_units_after{std::numeric_limits<double>::quiet_NaN()};
  std::uint64_t unit_nonnegativity_violation_count{0};
  std::string allocation_target_risky_node_weights{};
  std::string allocation_reserve_node_id{};
  double allocation_reserve_weight{std::numeric_limits<double>::quiet_NaN()};
  double allocation_turnover{std::numeric_limits<double>::quiet_NaN()};
  std::string allocation_objective_terms{};
  double allocation_cvar_loss{std::numeric_limits<double>::quiet_NaN()};
  double allocation_transaction_cost_estimate{
      std::numeric_limits<double>::quiet_NaN()};
  std::string allocation_constraint_diagnostics{};
  std::string allocation_cap_diagnostics{};
  std::string allocation_scenario_growth_floor_status{};
  std::string allocation_fallback_reasons{};
  std::string allocation_derisk_reasons{};

  double projection_mae{std::numeric_limits<double>::quiet_NaN()};
  double projection_rmse{std::numeric_limits<double>::quiet_NaN()};
  double projection_signed_bias{std::numeric_limits<double>::quiet_NaN()};
  double projection_correlation{std::numeric_limits<double>::quiet_NaN()};
  double projection_directional_accuracy{
      std::numeric_limits<double>::quiet_NaN()};
  double projection_interval_coverage{std::numeric_limits<double>::quiet_NaN()};
  double projection_mean_interval_width{
      std::numeric_limits<double>::quiet_NaN()};
  double zero_return_baseline_mae{std::numeric_limits<double>::quiet_NaN()};
  double zero_return_baseline_rmse{std::numeric_limits<double>::quiet_NaN()};
  double zero_return_baseline_signed_bias{
      std::numeric_limits<double>::quiet_NaN()};
  double zero_return_baseline_directional_accuracy{
      std::numeric_limits<double>::quiet_NaN()};
  double model_skill_vs_zero_mae{std::numeric_limits<double>::quiet_NaN()};
  double model_skill_vs_zero_rmse{std::numeric_limits<double>::quiet_NaN()};
  std::string per_node_projection_mae{};
  std::string per_node_projection_rmse{};
  std::string per_node_projection_signed_bias{};
  std::string per_node_projection_correlation{};
  std::string per_node_projection_directional_accuracy{};
  std::string per_node_projection_interval_coverage{};
  std::string per_node_projection_mean_interval_width{};
  std::string per_node_realized_volatility{};
  std::string per_node_normalized_projection_mae{};
  std::string per_node_normalized_projection_rmse{};
  std::string per_node_zero_return_baseline_mae{};
  std::string per_node_zero_return_baseline_rmse{};
  std::string per_node_model_skill_vs_zero_mae{};
  std::string per_node_model_skill_vs_zero_rmse{};

  std::vector<std::string> warnings{};
  std::vector<std::string> failures{};
};

namespace detail {

[[nodiscard]] inline bool blank(const std::string &text) {
  for (const char ch : text) {
    if (!std::isspace(static_cast<unsigned char>(ch))) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool
path_contains_parent_reference(const std::filesystem::path &path) {
  for (const auto &part : path) {
    if (part == "..") {
      return true;
    }
  }
  return false;
}

inline void require_finite(double value, const char *name) {
  if (!std::isfinite(value)) {
    throw std::runtime_error(std::string("[environment] non-finite ") + name);
  }
}

inline void validate_requested_range(const requested_episode_range_t &range) {
  const bool has_anchor_range = range.anchor_index_begin.has_value() ||
                                range.anchor_index_end.has_value();
  const bool has_source_key_range =
      range.source_key_begin.has_value() || range.source_key_end.has_value();
  if (!has_anchor_range && !has_source_key_range) {
    throw std::runtime_error(
        "[environment] requested anchor-index or source-key range is required");
  }
  if (has_anchor_range &&
      (!range.anchor_index_begin.has_value() ||
       !range.anchor_index_end.has_value() || *range.anchor_index_begin < 0 ||
       *range.anchor_index_end <= *range.anchor_index_begin)) {
    throw std::runtime_error(
        "[environment] requested anchor-index range must be nonempty and "
        "increasing");
  }
  if (has_source_key_range &&
      (!range.source_key_begin.has_value() ||
       !range.source_key_end.has_value() ||
       *range.source_key_end <= *range.source_key_begin)) {
    throw std::runtime_error(
        "[environment] requested source-key range must be nonempty and "
        "increasing");
  }
}

[[nodiscard]] inline std::int64_t size_to_i64_checked(std::size_t value,
                                                      const char *name) {
  if (value >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    throw std::runtime_error(std::string("[environment] ") + name +
                             " exceeds int64 range");
  }
  return static_cast<std::int64_t>(value);
}

inline void require_optional_i64_match(std::optional<std::int64_t> expected,
                                       std::optional<std::int64_t> actual,
                                       const char *name) {
  if (expected.has_value() != actual.has_value() ||
      (expected.has_value() && *expected != *actual)) {
    throw std::runtime_error(std::string("[environment] ") + name +
                             " does not match requested range evidence");
  }
}

inline void
require_optional_size_match_i64(std::optional<std::int64_t> expected,
                                std::optional<std::size_t> actual,
                                const char *name) {
  if (expected.has_value() != actual.has_value()) {
    throw std::runtime_error(std::string("[environment] ") + name +
                             " does not match requested range evidence");
  }
  if (expected.has_value()) {
    const auto actual_i64 = size_to_i64_checked(*actual, name);
    if (*expected != actual_i64) {
      throw std::runtime_error(std::string("[environment] ") + name +
                               " does not match requested range evidence");
    }
  }
}

inline void validate_v1_unified_base_policy(const belief::BasePolicy &policy) {
  if (policy.accounting_numeraire_id.empty() ||
      policy.settlement_asset_id.empty() || policy.reserve_asset_id.empty() ||
      policy.projection_reference_node_id.empty()) {
    throw std::runtime_error("[environment] BasePolicy fields are required");
  }
  if (policy.accounting_numeraire_id != policy.reserve_asset_id ||
      policy.accounting_numeraire_id != policy.settlement_asset_id ||
      policy.accounting_numeraire_id != policy.projection_reference_node_id) {
    throw std::runtime_error(
        "[environment] replay V1 requires accounting, settlement, reserve, "
        "and projection reference to be the same graph node");
  }
}

inline void validate_requested_wave_binding(
    const protocol::component_stream_wave_t &wave,
    const requested_episode_range_t &range,
    const protocol::component_stream_cursor_t &cursor) {
  if (blank(wave.wave_id) || blank(wave.mode_text) ||
      blank(wave.source_cursor_kind) || blank(wave.source_cursor_scope) ||
      blank(wave.source_range_policy) || blank(wave.source_order_policy)) {
    throw std::runtime_error(
        "[environment] requested component-stream wave identity is required");
  }
  if (wave.wave_id != cursor.wave_id || wave.mode_text != cursor.wave_mode ||
      wave.source_cursor_kind != cursor.source_cursor_kind ||
      wave.source_cursor_scope != cursor.source_cursor_scope ||
      wave.source_range_policy != cursor.source_range_policy ||
      wave.source_order_policy != cursor.source_order_policy) {
    throw std::runtime_error(
        "[environment] accepted cursor identity must match requested wave");
  }

  require_optional_size_match_i64(range.anchor_index_begin,
                                  wave.anchor_index_begin,
                                  "requested_wave.anchor_index_begin");
  require_optional_size_match_i64(range.anchor_index_end, wave.anchor_index_end,
                                  "requested_wave.anchor_index_end");
  require_optional_i64_match(range.source_key_begin, wave.source_key_begin,
                             "requested_wave.source_key_begin");
  require_optional_i64_match(range.source_key_end, wave.source_key_end,
                             "requested_wave.source_key_end");
  require_optional_size_match_i64(range.anchor_index_begin,
                                  cursor.anchor_index_begin,
                                  "accepted_cursor.anchor_index_begin");
  require_optional_size_match_i64(range.anchor_index_end,
                                  cursor.anchor_index_end,
                                  "accepted_cursor.anchor_index_end");
  require_optional_i64_match(range.source_key_begin, cursor.source_key_begin,
                             "accepted_cursor.source_key_begin");
  require_optional_i64_match(range.source_key_end, cursor.source_key_end,
                             "accepted_cursor.source_key_end");

  if (cursor.source_range_policy == "anchor_index" &&
      (!range.anchor_index_begin.has_value() ||
       !range.anchor_index_end.has_value())) {
    throw std::runtime_error(
        "[environment] anchor_index cursor requires requested anchor range");
  }
  if (cursor.source_range_policy == "source_key" &&
      (!range.source_key_begin.has_value() ||
       !range.source_key_end.has_value())) {
    throw std::runtime_error(
        "[environment] source_key cursor requires requested source-key range");
  }
  if (cursor.source_order_policy != "sequential") {
    throw std::runtime_error(
        "[environment] replay V1 requires sequential source order policy");
  }
}

inline void validate_accepted_range(const accepted_episode_range_t &range,
                                    const requested_episode_range_t &request) {
  protocol::validate_component_stream_cursor(range.cursor);
  if (range.anchor_index_begin < 0 ||
      range.anchor_index_end <= range.anchor_index_begin) {
    throw std::runtime_error(
        "[environment] accepted anchor range must be nonempty and increasing");
  }
  const auto expected_count = static_cast<std::size_t>(
      range.anchor_index_end - range.anchor_index_begin);
  if (range.anchor_keys.size() != expected_count) {
    throw std::runtime_error(
        "[environment] accepted anchor key count must match accepted range");
  }
  std::unordered_set<std::string> seen_keys;
  for (const auto &anchor_key : range.anchor_keys) {
    if (blank(anchor_key) || !seen_keys.insert(anchor_key).second) {
      throw std::runtime_error(
          "[environment] accepted anchor keys must be unique and nonempty");
    }
  }
  if (request.anchor_index_begin.has_value() &&
      request.anchor_index_end.has_value()) {
    if (range.anchor_index_begin < *request.anchor_index_begin ||
        range.anchor_index_end > *request.anchor_index_end) {
      throw std::runtime_error(
          "[environment] accepted anchor range must stay within requested "
          "anchor-index range");
    }
  }
}

[[nodiscard]] inline bool has_realized_return_projection(
    const realized_return_projection_spec_t &projection) {
  return !projection.truth_id.empty() ||
         !projection.reference_node_id.empty() ||
         !projection.edge_ids.empty() || projection.signs.defined();
}

inline void validate_realized_return_projection(
    const realized_return_projection_spec_t &projection,
    const std::vector<std::string> &risky_node_ids,
    const belief::BasePolicy &base_policy) {
  if (!has_realized_return_projection(projection)) {
    return;
  }
  if (projection.truth_id != kDirectEdgeRealizedReturnTruthV1) {
    throw std::runtime_error("[environment] realized-return truth id must be "
                             "direct_edge_realized_return_truth_v1");
  }
  if (projection.reference_node_id !=
      base_policy.projection_reference_node_id) {
    throw std::runtime_error(
        "[environment] realized-return projection reference node must match "
        "BasePolicy projection reference");
  }
  if (projection.edge_ids.size() != risky_node_ids.size()) {
    throw std::runtime_error(
        "[environment] realized-return projection edge count must match risky "
        "node count");
  }
  std::unordered_set<std::string> seen_edges;
  seen_edges.reserve(projection.edge_ids.size());
  for (const auto &edge_id : projection.edge_ids) {
    if (blank(edge_id) || !seen_edges.insert(edge_id).second) {
      throw std::runtime_error(
          "[environment] realized-return projection edge ids must be unique "
          "and nonempty");
    }
  }
  if (!projection.signs.defined() || projection.signs.dim() != 1 ||
      projection.signs.size(0) !=
          static_cast<std::int64_t>(risky_node_ids.size())) {
    throw std::runtime_error(
        "[environment] realized-return projection signs must be [A]");
  }
  TORCH_CHECK(projection.signs.is_floating_point(),
              "[environment] realized-return projection signs must be "
              "floating point");
  const auto signs = projection.signs.to(torch::kFloat64);
  TORCH_CHECK(torch::isfinite(signs).all().item<bool>(),
              "[environment] realized-return projection signs contain "
              "non-finite values");
  for (std::int64_t i = 0; i < signs.size(0); ++i) {
    const auto sign = signs.index({i}).item<double>();
    if (sign != 1.0 && sign != -1.0) {
      throw std::runtime_error(
          "[environment] realized-return projection signs must be +/-1");
    }
  }
}

} // namespace detail

inline void validate_episode_spec(const episode_spec_t &spec) {
  if (detail::blank(spec.episode_id) || detail::blank(spec.protocol_id) ||
      detail::blank(spec.runtime_run_id) ||
      detail::blank(spec.environment_run_id)) {
    throw std::runtime_error(
        "[environment] episode_id, protocol_id, runtime_run_id, and "
        "environment_run_id are required");
  }
  if (detail::blank(spec.graph_order_fingerprint)) {
    throw std::runtime_error(
        "[environment] graph_order_fingerprint is required");
  }
  if (spec.world_mode != world_mode_t::historical_replay) {
    throw std::runtime_error(
        "[environment] replay V1 requires historical_replay world mode");
  }
  if (spec.graph_node_ids.empty()) {
    throw std::runtime_error("[environment] graph_node_ids is required");
  }
  std::unordered_set<std::string> graph_nodes;
  graph_nodes.reserve(spec.graph_node_ids.size());
  for (const auto &node_id : spec.graph_node_ids) {
    if (detail::blank(node_id) || !graph_nodes.insert(node_id).second) {
      throw std::runtime_error(
          "[environment] graph_node_ids must be unique and nonempty");
    }
  }
  if (spec.risky_node_ids.empty()) {
    throw std::runtime_error("[environment] risky_node_ids is required");
  }
  std::unordered_set<std::string> seen_nodes;
  for (const auto &node_id : spec.risky_node_ids) {
    if (detail::blank(node_id) || !seen_nodes.insert(node_id).second) {
      throw std::runtime_error(
          "[environment] risky_node_ids must be unique and nonempty");
    }
    if (!graph_nodes.contains(node_id)) {
      throw std::runtime_error(
          "[environment] risky_node_ids must be present in graph_node_ids");
    }
  }
  detail::validate_requested_range(spec.requested_range);
  detail::validate_requested_wave_binding(
      spec.requested_wave, spec.requested_range, spec.accepted_range.cursor);
  detail::validate_accepted_range(spec.accepted_range, spec.requested_range);
  if (spec.initial_equity_base <= 0.0 ||
      !std::isfinite(spec.initial_equity_base)) {
    throw std::runtime_error(
        "[environment] initial_equity_base must be positive finite");
  }
  detail::validate_v1_unified_base_policy(spec.base_policy);
  if (!graph_nodes.contains(spec.base_policy.reserve_asset_id)) {
    throw std::runtime_error(
        "[environment] base reserve node must be present in graph_node_ids");
  }
  for (const auto &node_id : spec.risky_node_ids) {
    if (node_id == spec.base_policy.reserve_asset_id) {
      throw std::runtime_error(
          "[environment] reserve asset must not appear in risky_node_ids");
    }
  }
  detail::validate_realized_return_projection(
      spec.realized_return_projection, spec.risky_node_ids, spec.base_policy);
}

inline void validate_action(const action_t &action,
                            const std::vector<std::string> &expected_node_ids,
                            double budget_tolerance = 1.0e-8) {
  if (detail::blank(action.policy_id)) {
    throw std::runtime_error("[environment] action policy_id is required");
  }
  if (action.policy_kind == policy_kind_t::deterministic_allocator &&
      detail::blank(action.method_id)) {
    throw std::runtime_error(
        "[environment] deterministic allocator action method_id is required");
  }
  if (action.action_schema_id != kTargetWeightsActionSchema) {
    throw std::runtime_error("[environment] action_schema_id must be "
                             "kikijyeba.environment.action.target_weights.v1");
  }
  if (action.decision_timestamp_ms < 0) {
    throw std::runtime_error(
        "[environment] action decision_timestamp_ms must be nonnegative");
  }
  if (detail::blank(action.base_reserve_node_id)) {
    throw std::runtime_error(
        "[environment] action base_reserve_node_id is required");
  }
  if (action.node_ids != expected_node_ids) {
    throw std::runtime_error("[environment] action node_ids mismatch");
  }
  for (const auto &node_id : action.node_ids) {
    if (node_id == action.base_reserve_node_id) {
      throw std::runtime_error(
          "[environment] action base_reserve_node_id must not be a risky node");
    }
  }
  const auto A = static_cast<std::int64_t>(expected_node_ids.size());
  if (!action.target_weights.defined() || action.target_weights.dim() != 1 ||
      action.target_weights.size(0) != A) {
    throw std::runtime_error("[environment] action target_weights must be [A]");
  }
  auto weights = action.target_weights.to(torch::kFloat64);
  if (!torch::isfinite(weights).all().item<bool>() ||
      (weights < -budget_tolerance).any().item<bool>()) {
    throw std::runtime_error(
        "[environment] action target_weights must be finite and nonnegative");
  }
  detail::require_finite(action.target_base_reserve_weight,
                         "target_base_reserve_weight");
  if (action.target_base_reserve_weight < -budget_tolerance) {
    throw std::runtime_error(
        "[environment] action target_base_reserve_weight must be nonnegative");
  }
  const double total =
      weights.sum().item<double>() + action.target_base_reserve_weight;
  if (std::abs(total - 1.0) > budget_tolerance) {
    throw std::runtime_error(
        "[environment] action risky weights plus reserve weight must equal 1");
  }
  if (!action.risk_gate_evaluated &&
      (action.risk_gate.force_base_reserve_fallback ||
       !action.risk_gate.reasons.empty() ||
       !action.risk_gate.warnings.empty())) {
    throw std::runtime_error(
        "[environment] action carries risk-gate evidence but "
        "risk_gate_evaluated is false");
  }
  if (action.rebalance_plan_available) {
    if (detail::blank(action.rebalance_plan_source)) {
      throw std::runtime_error(
          "[environment] action rebalance_plan_source is required when "
          "rebalance_plan_available is true");
    }
    if (!action.rebalance_plan.valid) {
      throw std::runtime_error(
          "[environment] action rebalance plan must be valid when "
          "rebalance_plan_available is true");
    }
    if (action.rebalance_plan.timestamp_ms <= 0) {
      throw std::runtime_error(
          "[environment] action rebalance plan timestamp is required when "
          "rebalance_plan_available is true");
    }
    if (action.rebalance_plan.node_ids != action.node_ids) {
      throw std::runtime_error(
          "[environment] action rebalance plan node_ids mismatch");
    }
    if (action.rebalance_plan.base_reserve_node_id !=
        action.base_reserve_node_id) {
      throw std::runtime_error(
          "[environment] action rebalance plan reserve node mismatch");
    }
    detail::require_finite(action.rebalance_plan.requested_turnover_weight,
                           "rebalance_plan.requested_turnover_weight");
    detail::require_finite(action.rebalance_plan.routed_turnover_weight,
                           "rebalance_plan.routed_turnover_weight");
    detail::require_finite(action.rebalance_plan.requested_notional_base,
                           "rebalance_plan.requested_notional_base");
    detail::require_finite(action.rebalance_plan.routed_notional_base,
                           "rebalance_plan.routed_notional_base");
    detail::require_finite(
        action.rebalance_plan.estimated_transaction_cost_base,
        "rebalance_plan.estimated_transaction_cost_base");
    if (action.rebalance_plan.requested_turnover_weight < -budget_tolerance ||
        action.rebalance_plan.routed_turnover_weight < -budget_tolerance ||
        action.rebalance_plan.requested_notional_base < -budget_tolerance ||
        action.rebalance_plan.routed_notional_base < -budget_tolerance ||
        action.rebalance_plan.estimated_transaction_cost_base <
            -budget_tolerance) {
      throw std::runtime_error(
          "[environment] action rebalance plan numeric fields must be "
          "nonnegative");
    }
    if (action.rebalance_plan.routed_turnover_weight >
            action.rebalance_plan.requested_turnover_weight +
                budget_tolerance ||
        action.rebalance_plan.routed_notional_base >
            action.rebalance_plan.requested_notional_base + budget_tolerance) {
      throw std::runtime_error(
          "[environment] action rebalance plan routed values must not exceed "
          "requested values");
    }
    const auto approx_equal = [budget_tolerance](double actual,
                                                 double expected) {
      return std::abs(actual - expected) <=
             budget_tolerance * std::max(1.0, std::abs(expected));
    };
    double order_requested_notional = 0.0;
    double skipped_requested_notional = 0.0;
    double order_routed_notional = 0.0;
    double order_estimated_cost = 0.0;
    for (const auto &order : action.rebalance_plan.orders) {
      if (!std::count(action.node_ids.begin(), action.node_ids.end(),
                      order.node_id)) {
        throw std::runtime_error(
            "[environment] action rebalance plan order node_id mismatch");
      }
      detail::require_finite(order.delta_weight,
                             "rebalance_plan.order.delta_weight");
      detail::require_finite(order.requested_notional_base,
                             "rebalance_plan.order.requested_notional_base");
      detail::require_finite(order.routed_notional_base,
                             "rebalance_plan.order.routed_notional_base");
      detail::require_finite(order.estimated_cost_base,
                             "rebalance_plan.order.estimated_cost_base");
      if (order.requested_notional_base < -budget_tolerance ||
          order.routed_notional_base < -budget_tolerance ||
          order.estimated_cost_base < -budget_tolerance) {
        throw std::runtime_error(
            "[environment] action rebalance plan order notionals/cost must be "
            "nonnegative");
      }
      if (order.routed_notional_base >
          order.requested_notional_base + budget_tolerance) {
        throw std::runtime_error(
            "[environment] action rebalance plan order routed notional must "
            "not exceed requested notional");
      }
      order_requested_notional += order.requested_notional_base;
      order_routed_notional += order.routed_notional_base;
      order_estimated_cost += order.estimated_cost_base;
    }
    for (const auto &skipped : action.rebalance_plan.skipped) {
      if (!std::count(action.node_ids.begin(), action.node_ids.end(),
                      skipped.node_id)) {
        throw std::runtime_error(
            "[environment] action rebalance plan skipped node_id mismatch");
      }
      detail::require_finite(skipped.delta_weight,
                             "rebalance_plan.skipped.delta_weight");
      detail::require_finite(skipped.requested_notional_base,
                             "rebalance_plan.skipped.requested_notional_base");
      if (skipped.requested_notional_base < -budget_tolerance) {
        throw std::runtime_error(
            "[environment] action rebalance plan skipped requested notional "
            "must be nonnegative");
      }
      if (detail::blank(skipped.reason)) {
        throw std::runtime_error(
            "[environment] action rebalance plan skipped reason is required");
      }
      skipped_requested_notional += skipped.requested_notional_base;
    }
    const bool has_route_breakdown = !action.rebalance_plan.orders.empty() ||
                                     !action.rebalance_plan.skipped.empty();
    if (has_route_breakdown) {
      if (!approx_equal(action.rebalance_plan.requested_notional_base,
                        order_requested_notional +
                            skipped_requested_notional)) {
        throw std::runtime_error(
            "[environment] action rebalance plan requested notional does not "
            "match order plus skipped requested notional");
      }
      if (!approx_equal(action.rebalance_plan.routed_notional_base,
                        order_routed_notional)) {
        throw std::runtime_error(
            "[environment] action rebalance plan routed notional does not "
            "match order routed notional");
      }
      if (!approx_equal(action.rebalance_plan.estimated_transaction_cost_base,
                        order_estimated_cost)) {
        throw std::runtime_error(
            "[environment] action rebalance plan estimated cost does not match "
            "order estimated cost");
      }
    }
  }
  if (action.decision_report_available &&
      (detail::blank(action.decision_report.schema_id) ||
       action.decision_report.text.empty())) {
    throw std::runtime_error(
        "[environment] action decision report requires schema and text when "
        "decision_report_available is true");
  }
}

inline void validate_episode_action(const action_t &action,
                                    const episode_spec_t &spec,
                                    double budget_tolerance = 1.0e-8) {
  validate_action(action, spec.risky_node_ids, budget_tolerance);
  if (action.base_reserve_node_id != spec.base_policy.reserve_asset_id) {
    throw std::runtime_error(
        "[environment] action base_reserve_node_id must match EpisodeSpec "
        "BasePolicy reserve asset");
  }
}

[[nodiscard]] inline portfolio::timestamp_ms_t
resolve_action_decision_timestamp(const action_t &action,
                                  const observation_t &observation) {
  return action.decision_timestamp_ms > 0 ? action.decision_timestamp_ms
                                          : observation.knowledge_timestamp_ms;
}

inline void
validate_observation_time_boundary(const observation_t &observation) {
  if (detail::blank(observation.anchor_key)) {
    throw std::runtime_error(
        "[environment] observation anchor_key is required");
  }
  if (observation.timestamp_ms <= 0 ||
      observation.timestamp_ms > observation.knowledge_timestamp_ms) {
    throw std::runtime_error(
        "[environment] observation timestamp must be positive and no later "
        "than the knowledge timestamp");
  }
  if (observation.observation_anchor_index < 0 ||
      observation.next_realization_anchor_index <=
          observation.observation_anchor_index) {
    throw std::runtime_error(
        "[environment] observation must expose time-t anchor and only point to "
        "a later realization anchor");
  }
  if (observation.knowledge_timestamp_ms <= 0 ||
      observation.realization_available_after_timestamp_ms <=
          observation.knowledge_timestamp_ms) {
    throw std::runtime_error(
        "[environment] observation realization timestamp must be after "
        "knowledge timestamp");
  }
}

inline void validate_action_time_boundary(const action_t &action,
                                          const observation_t &observation) {
  validate_observation_time_boundary(observation);
  const auto decision_timestamp_ms =
      resolve_action_decision_timestamp(action, observation);
  if (decision_timestamp_ms < observation.knowledge_timestamp_ms ||
      decision_timestamp_ms >=
          observation.realization_available_after_timestamp_ms) {
    throw std::runtime_error(
        "[environment] action decision timestamp must be after knowledge and "
        "before realization availability");
  }
}

inline void validate_transition_time_boundary(const observation_t &observation,
                                              const transition_t &transition) {
  validate_observation_time_boundary(observation);
  if (transition.info.anchor_key != observation.anchor_key ||
      transition.info.anchor_index != observation.observation_anchor_index) {
    throw std::runtime_error(
        "[environment] transition info must match the policy observation "
        "anchor");
  }
  if (!transition.done) {
    validate_observation_time_boundary(transition.next_observation);
    if (transition.next_observation.observation_anchor_index !=
        observation.next_realization_anchor_index) {
      throw std::runtime_error(
          "[environment] next observation must start at the prior "
          "next-realization anchor");
    }
    if (transition.next_observation.knowledge_timestamp_ms <
        observation.realization_available_after_timestamp_ms) {
      throw std::runtime_error(
          "[environment] next observation knowledge timestamp cannot precede "
          "the prior realization boundary");
    }
  }
}

[[nodiscard]] inline reward_t
compute_reward(double equity_before_base, double equity_after_base,
               double drawdown, double transaction_cost_base, double turnover,
               bool invalid_action,
               const reward_options_t &options = reward_options_t{}) {
  validate_reward_options(options);
  detail::require_finite(equity_before_base, "equity_before_base");
  detail::require_finite(equity_after_base, "equity_after_base");
  detail::require_finite(drawdown, "drawdown");
  detail::require_finite(transaction_cost_base, "transaction_cost_base");
  detail::require_finite(turnover, "turnover");
  if (equity_before_base <= 0.0 || equity_after_base <= 0.0) {
    throw std::runtime_error("[environment] reward equity must be positive");
  }

  reward_t out{};
  out.log_growth_reward = std::log((equity_after_base + options.eps) /
                                   (equity_before_base + options.eps));
  out.drawdown_penalty = options.lambda_drawdown * std::max(0.0, drawdown);
  out.transaction_cost_penalty = options.lambda_transaction_cost *
                                 std::max(0.0, transaction_cost_base) /
                                 std::max(equity_before_base, options.eps);
  out.turnover_penalty = options.lambda_turnover * std::max(0.0, turnover);
  out.invalid_action_penalty =
      invalid_action ? options.invalid_action_penalty : 0.0;
  out.total = out.log_growth_reward - out.drawdown_penalty -
              out.transaction_cost_penalty - out.turnover_penalty -
              out.invalid_action_penalty;
  return out;
}

} // namespace cuwacunu::kikijyeba::environment
