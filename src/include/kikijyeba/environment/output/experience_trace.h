// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "kikijyeba/environment/run/experiment_runner.h"

namespace cuwacunu::kikijyeba::environment::output {

inline constexpr const char *kExperienceTraceSchema =
    "kikijyeba.environment.output.experience_trace.v1";
inline constexpr const char *kEpisodeTraceSchema =
    "kikijyeba.environment.output.episode_trace.v1";
inline constexpr const char *kTransitionRecordSchema =
    "kikijyeba.environment.output.transition_record.v1";
inline constexpr const char *kPolicyComparisonRecordSchema =
    "kikijyeba.environment.output.policy_comparison_record.v1";
inline constexpr const char *kFutureCajtucuConsumer =
    "cajtucu.output.evaluation";

struct transition_record_t {
  std::string trace_schema_id{kTransitionRecordSchema};
  std::string source_step_artifact_schema_id{};

  std::uint64_t step_index{0};
  std::string episode_id{};
  std::string protocol_id{};
  std::string runtime_run_id{};
  std::string environment_run_id{};

  std::string anchor_key{};
  std::int64_t observation_anchor_index{-1};
  std::int64_t next_realization_anchor_index{-1};
  portfolio::timestamp_ms_t knowledge_timestamp_ms{0};
  portfolio::timestamp_ms_t realization_available_after_timestamp_ms{0};
  portfolio::timestamp_ms_t action_decision_timestamp_ms{0};
  bool time_law_clean{false};

  std::string policy_id{};
  std::string method_id{};
  policy_kind_t policy_kind{policy_kind_t::external};
  world_mode_t world_mode{world_mode_t::historical_replay};

  std::string action_schema_id{};
  std::string target_risky_node_weights{};
  std::string target_base_reserve_node_id{};
  double target_base_reserve_weight{std::numeric_limits<double>::quiet_NaN()};

  std::string execution_model{};
  std::string rebalance_plan_source{};
  bool rebalance_plan_enforced{false};
  std::uint64_t rebalance_order_count{0};
  std::uint64_t rebalance_skipped_count{0};
  std::uint64_t fill_count{0};
  double fill_gross_notional_base{0.0};
  double fill_fee_base{0.0};

  double portfolio_equity_before{std::numeric_limits<double>::quiet_NaN()};
  double portfolio_equity_after{std::numeric_limits<double>::quiet_NaN()};
  double realized_log_growth{std::numeric_limits<double>::quiet_NaN()};
  double realized_arithmetic_return{std::numeric_limits<double>::quiet_NaN()};
  double transaction_cost_base{std::numeric_limits<double>::quiet_NaN()};
  double turnover{std::numeric_limits<double>::quiet_NaN()};
  bool invalid_action{false};

  double reward_log_growth{std::numeric_limits<double>::quiet_NaN()};
  double reward_drawdown_penalty{std::numeric_limits<double>::quiet_NaN()};
  double reward_transaction_cost_penalty{
      std::numeric_limits<double>::quiet_NaN()};
  double reward_turnover_penalty{std::numeric_limits<double>::quiet_NaN()};
  double reward_invalid_action_penalty{
      std::numeric_limits<double>::quiet_NaN()};
  double reward_total{std::numeric_limits<double>::quiet_NaN()};

  bool projection_validation_available{false};
  double projection_mae{std::numeric_limits<double>::quiet_NaN()};
  double projection_rmse{std::numeric_limits<double>::quiet_NaN()};
  double projection_signed_bias{std::numeric_limits<double>::quiet_NaN()};
  double projection_correlation{std::numeric_limits<double>::quiet_NaN()};
  double projection_directional_accuracy{
      std::numeric_limits<double>::quiet_NaN()};
  double projection_interval_coverage{std::numeric_limits<double>::quiet_NaN()};

  bool residual_quality_available{false};
  bool residual_quality_valid{false};
  double residual_mean_residual_energy{
      std::numeric_limits<double>::quiet_NaN()};
  double residual_max_residual_energy{std::numeric_limits<double>::quiet_NaN()};

  bool risk_gate_evaluated{false};
  bool risk_gate_allow_trading{true};
  bool risk_gate_force_base_reserve_fallback{false};
  std::uint64_t warning_count{0};
  std::uint64_t failure_count{0};
  std::string warnings{};
  std::string failures{};
};

struct episode_trace_t {
  std::string trace_schema_id{kEpisodeTraceSchema};
  std::string source_episode_artifact_schema_id{};
  std::string future_consumer{kFutureCajtucuConsumer};

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

  std::int64_t accepted_anchor_index_begin{-1};
  std::int64_t accepted_anchor_index_end{-1};
  std::string accepted_anchor_keys{};
  std::string accepted_batch_cursor_token{};
  std::string realized_return_truth_id{};

  std::uint64_t transition_count{0};
  std::vector<transition_record_t> transitions{};

  double total_reward{0.0};
  double total_log_growth{0.0};
  double final_equity_base{std::numeric_limits<double>::quiet_NaN()};
  double max_drawdown{0.0};
  double total_turnover{0.0};
  double total_transaction_cost_base{0.0};

  double projection_mae{std::numeric_limits<double>::quiet_NaN()};
  double projection_rmse{std::numeric_limits<double>::quiet_NaN()};
  double projection_signed_bias{std::numeric_limits<double>::quiet_NaN()};
  double projection_correlation{std::numeric_limits<double>::quiet_NaN()};
  double projection_directional_accuracy{
      std::numeric_limits<double>::quiet_NaN()};
  double projection_interval_coverage{std::numeric_limits<double>::quiet_NaN()};

  std::vector<std::string> warnings{};
  std::vector<std::string> failures{};
};

struct policy_comparison_record_t {
  std::string trace_schema_id{kPolicyComparisonRecordSchema};
  std::string source_policy_summary_schema_id{};
  std::string future_consumer{kFutureCajtucuConsumer};

  std::string policy_id{};
  policy_kind_t policy_kind{policy_kind_t::external};
  std::uint64_t attempted_count{0};
  std::uint64_t completed_count{0};
  std::uint64_t failed_count{0};

  double mean_total_reward{std::numeric_limits<double>::quiet_NaN()};
  double mean_total_log_growth{std::numeric_limits<double>::quiet_NaN()};
  double mean_final_equity_base{std::numeric_limits<double>::quiet_NaN()};
  double mean_max_drawdown{std::numeric_limits<double>::quiet_NaN()};
  double mean_total_turnover{std::numeric_limits<double>::quiet_NaN()};
  double mean_total_transaction_cost_base{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_projection_mae{std::numeric_limits<double>::quiet_NaN()};
  double mean_projection_rmse{std::numeric_limits<double>::quiet_NaN()};
  double mean_projection_signed_bias{std::numeric_limits<double>::quiet_NaN()};
  double mean_projection_correlation{std::numeric_limits<double>::quiet_NaN()};
  double mean_projection_directional_accuracy{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_projection_interval_coverage{
      std::numeric_limits<double>::quiet_NaN()};
};

struct experience_trace_t {
  std::string trace_schema_id{kExperienceTraceSchema};
  std::string source_experiment_artifact_schema_id{};
  std::string future_consumer{kFutureCajtucuConsumer};

  std::string experiment_id{};
  std::string runtime_run_id{};
  std::string environment_run_id{};
  std::size_t requested_max_parallel_jobs{1};
  std::size_t resolved_parallelism{1};
  std::uint64_t attempted_count{0};
  std::uint64_t completed_count{0};

  std::vector<episode_trace_t> episodes{};
  std::vector<policy_comparison_record_t> policy_comparisons{};
  std::vector<std::string> warnings{};
  std::vector<std::string> failures{};
};

namespace detail {

[[nodiscard]] inline bool has_projection_metrics(const step_report_t &step) {
  return std::isfinite(step.projection_mae) ||
         std::isfinite(step.projection_rmse) ||
         std::isfinite(step.projection_signed_bias) ||
         std::isfinite(step.projection_correlation) ||
         std::isfinite(step.projection_directional_accuracy) ||
         std::isfinite(step.projection_interval_coverage);
}

[[nodiscard]] inline bool time_law_clean(const step_report_t &step) {
  return step.observation_anchor_index >= 0 &&
         step.next_realization_anchor_index > step.observation_anchor_index &&
         step.knowledge_timestamp_ms <= step.action_decision_timestamp_ms &&
         step.action_decision_timestamp_ms <
             step.realization_available_after_timestamp_ms;
}

inline void require_trace_schema(const std::string &actual,
                                 const char *expected, const char *name) {
  if (actual != expected) {
    throw std::runtime_error(std::string("[experience_trace] ") + name +
                             " schema mismatch");
  }
}

inline void require_nonblank(const std::string &value, const char *name) {
  if (cuwacunu::kikijyeba::environment::detail::blank(value)) {
    throw std::runtime_error(std::string("[experience_trace] ") + name +
                             " is required");
  }
}

inline void require_finite(double value, const char *name) {
  if (!std::isfinite(value)) {
    throw std::runtime_error(std::string("[experience_trace] non-finite ") +
                             name);
  }
}

[[nodiscard]] inline char hex_digit(unsigned value) {
  return static_cast<char>(value < 10 ? ('0' + value) : ('A' + value - 10));
}

[[nodiscard]] inline std::string encode_text(const std::string &value) {
  std::string out;
  out.reserve(value.size());
  for (unsigned char ch : value) {
    if (ch == '%' || ch == '\n' || ch == '\r') {
      out.push_back('%');
      out.push_back(hex_digit((ch >> 4U) & 0x0FU));
      out.push_back(hex_digit(ch & 0x0FU));
    } else {
      out.push_back(static_cast<char>(ch));
    }
  }
  return out;
}

[[nodiscard]] inline int from_hex(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'a' && ch <= 'f') {
    return 10 + ch - 'a';
  }
  if (ch >= 'A' && ch <= 'F') {
    return 10 + ch - 'A';
  }
  return -1;
}

[[nodiscard]] inline std::string decode_text(const std::string &value) {
  std::string out;
  out.reserve(value.size());
  for (std::size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '%' && i + 2 < value.size()) {
      const int hi = from_hex(value[i + 1]);
      const int lo = from_hex(value[i + 2]);
      if (hi >= 0 && lo >= 0) {
        out.push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
        continue;
      }
    }
    out.push_back(value[i]);
  }
  return out;
}

inline void write_text_kv(std::ostream &out, const std::string &key,
                          const std::string &value) {
  out << key << '=' << encode_text(value) << '\n';
}

inline void write_bool_kv(std::ostream &out, const std::string &key,
                          bool value) {
  out << key << '=' << (value ? "true" : "false") << '\n';
}

inline void write_double_kv(std::ostream &out, const std::string &key,
                            double value) {
  out << key << '=';
  if (std::isnan(value)) {
    out << "nan";
  } else if (std::isinf(value)) {
    out << (value > 0.0 ? "inf" : "-inf");
  } else {
    out << std::setprecision(17) << value;
  }
  out << '\n';
}

template <typename T>
inline void write_integral_kv(std::ostream &out, const std::string &key,
                              T value) {
  out << key << '=' << value << '\n';
}

inline void write_string_vector(std::ostream &out, const std::string &prefix,
                                const std::vector<std::string> &values) {
  write_integral_kv(out, prefix + "count", values.size());
  for (std::size_t i = 0; i < values.size(); ++i) {
    write_text_kv(out, prefix + std::to_string(i), values[i]);
  }
}

using kv_map_t = std::unordered_map<std::string, std::string>;

[[nodiscard]] inline kv_map_t parse_kv_stream(std::istream &in) {
  kv_map_t values;
  std::string line;
  std::uint64_t line_number = 0;
  while (std::getline(in, line)) {
    ++line_number;
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.empty()) {
      continue;
    }
    const auto split = line.find('=');
    if (split == std::string::npos) {
      throw std::runtime_error("[experience_trace] malformed trace line " +
                               std::to_string(line_number));
    }
    const auto key = line.substr(0, split);
    const auto inserted = values.emplace(key, line.substr(split + 1));
    if (!inserted.second) {
      throw std::runtime_error("[experience_trace] duplicate trace key " + key);
    }
  }
  return values;
}

[[nodiscard]] inline const std::string &required_raw(const kv_map_t &values,
                                                     const std::string &key) {
  const auto it = values.find(key);
  if (it == values.end()) {
    throw std::runtime_error("[experience_trace] missing trace key " + key);
  }
  return it->second;
}

[[nodiscard]] inline std::string read_text(const kv_map_t &values,
                                           const std::string &key) {
  return decode_text(required_raw(values, key));
}

[[nodiscard]] inline bool read_bool(const kv_map_t &values,
                                    const std::string &key) {
  const auto &raw = required_raw(values, key);
  if (raw == "true") {
    return true;
  }
  if (raw == "false") {
    return false;
  }
  throw std::runtime_error("[experience_trace] invalid bool key " + key);
}

[[nodiscard]] inline std::uint64_t read_u64(const kv_map_t &values,
                                            const std::string &key) {
  return static_cast<std::uint64_t>(std::stoull(required_raw(values, key)));
}

[[nodiscard]] inline std::size_t read_size(const kv_map_t &values,
                                           const std::string &key) {
  return static_cast<std::size_t>(std::stoull(required_raw(values, key)));
}

[[nodiscard]] inline std::int64_t read_i64(const kv_map_t &values,
                                           const std::string &key) {
  return static_cast<std::int64_t>(std::stoll(required_raw(values, key)));
}

[[nodiscard]] inline double read_double(const kv_map_t &values,
                                        const std::string &key) {
  const auto &raw = required_raw(values, key);
  if (raw == "nan") {
    return std::numeric_limits<double>::quiet_NaN();
  }
  if (raw == "inf") {
    return std::numeric_limits<double>::infinity();
  }
  if (raw == "-inf") {
    return -std::numeric_limits<double>::infinity();
  }
  return std::stod(raw);
}

[[nodiscard]] inline policy_kind_t read_policy_kind(const kv_map_t &values,
                                                    const std::string &key) {
  const auto raw = read_text(values, key);
  if (raw == policy_kind_name(policy_kind_t::deterministic_allocator)) {
    return policy_kind_t::deterministic_allocator;
  }
  if (raw == policy_kind_name(policy_kind_t::baseline)) {
    return policy_kind_t::baseline;
  }
  if (raw == policy_kind_name(policy_kind_t::reinforcement_learning)) {
    return policy_kind_t::reinforcement_learning;
  }
  if (raw == policy_kind_name(policy_kind_t::external)) {
    return policy_kind_t::external;
  }
  throw std::runtime_error("[experience_trace] invalid policy_kind key " + key);
}

[[nodiscard]] inline world_mode_t read_world_mode(const kv_map_t &values,
                                                  const std::string &key) {
  const auto raw = read_text(values, key);
  if (raw == world_mode_name(world_mode_t::historical_replay)) {
    return world_mode_t::historical_replay;
  }
  throw std::runtime_error("[experience_trace] invalid world_mode key " + key);
}

[[nodiscard]] inline std::vector<std::string>
read_string_vector(const kv_map_t &values, const std::string &prefix) {
  const auto count = read_size(values, prefix + "count");
  std::vector<std::string> out;
  out.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    out.push_back(read_text(values, prefix + std::to_string(i)));
  }
  return out;
}

} // namespace detail

[[nodiscard]] inline transition_record_t
make_transition_record(const step_report_t &step) {
  transition_record_t out{};
  out.source_step_artifact_schema_id = step.step_artifact_schema_id;
  out.step_index = step.step_index;
  out.episode_id = step.episode_id;
  out.protocol_id = step.protocol_id;
  out.runtime_run_id = step.runtime_run_id;
  out.environment_run_id = step.environment_run_id;
  out.anchor_key = step.anchor_key;
  out.observation_anchor_index = step.observation_anchor_index;
  out.next_realization_anchor_index = step.next_realization_anchor_index;
  out.knowledge_timestamp_ms = step.knowledge_timestamp_ms;
  out.realization_available_after_timestamp_ms =
      step.realization_available_after_timestamp_ms;
  out.action_decision_timestamp_ms = step.action_decision_timestamp_ms;
  out.time_law_clean = detail::time_law_clean(step);
  out.policy_id = step.policy_id;
  out.method_id = step.method_id;
  out.policy_kind = step.policy_kind;
  out.world_mode = step.world_mode;
  out.action_schema_id = step.action_schema_id;
  out.target_risky_node_weights = step.target_risky_node_weights;
  out.target_base_reserve_node_id = step.target_base_reserve_node_id;
  out.target_base_reserve_weight = step.target_base_reserve_weight;
  out.execution_model = step.execution_model;
  out.rebalance_plan_source = step.rebalance_plan_source;
  out.rebalance_plan_enforced = step.rebalance_plan_enforced;
  out.rebalance_order_count = step.rebalance_order_count;
  out.rebalance_skipped_count = step.rebalance_skipped_count;
  out.fill_count = step.fill_count;
  out.fill_gross_notional_base = step.fill_gross_notional_base;
  out.fill_fee_base = step.fill_fee_base;
  out.portfolio_equity_before = step.portfolio_equity_before;
  out.portfolio_equity_after = step.portfolio_equity_after;
  out.realized_log_growth = step.realized_log_growth;
  out.realized_arithmetic_return = step.realized_arithmetic_return;
  out.transaction_cost_base = step.transaction_cost_base;
  out.turnover = step.turnover;
  out.invalid_action = step.invalid_action;
  out.reward_log_growth = step.reward_log_growth;
  out.reward_drawdown_penalty = step.reward_drawdown_penalty;
  out.reward_transaction_cost_penalty = step.reward_transaction_cost_penalty;
  out.reward_turnover_penalty = step.reward_turnover_penalty;
  out.reward_invalid_action_penalty = step.reward_invalid_action_penalty;
  out.reward_total = step.reward_total;
  out.projection_validation_available = detail::has_projection_metrics(step);
  out.projection_mae = step.projection_mae;
  out.projection_rmse = step.projection_rmse;
  out.projection_signed_bias = step.projection_signed_bias;
  out.projection_correlation = step.projection_correlation;
  out.projection_directional_accuracy = step.projection_directional_accuracy;
  out.projection_interval_coverage = step.projection_interval_coverage;
  out.residual_quality_available = step.residual_quality_available;
  out.residual_quality_valid = step.residual_quality_valid;
  out.residual_mean_residual_energy = step.residual_mean_residual_energy;
  out.residual_max_residual_energy = step.residual_max_residual_energy;
  out.risk_gate_evaluated = step.risk_gate_evaluated;
  out.risk_gate_allow_trading = step.risk_gate_allow_trading;
  out.risk_gate_force_base_reserve_fallback =
      step.risk_gate_force_base_reserve_fallback;
  out.warning_count = step.warning_count;
  out.failure_count = step.failure_count;
  out.warnings = step.warnings;
  out.failures = step.failures;
  return out;
}

inline void validate_transition_record(const transition_record_t &record) {
  detail::require_trace_schema(record.trace_schema_id, kTransitionRecordSchema,
                               "transition record");
  detail::require_trace_schema(record.source_step_artifact_schema_id,
                               kReplayStepArtifactSchema,
                               "source step artifact");
  detail::require_nonblank(record.episode_id, "episode_id");
  detail::require_nonblank(record.policy_id, "policy_id");
  detail::require_nonblank(record.action_schema_id, "action_schema_id");
  detail::require_nonblank(record.anchor_key, "anchor_key");
  if (!record.time_law_clean) {
    throw std::runtime_error(
        "[experience_trace] transition record time law is not clean");
  }
  detail::require_finite(record.portfolio_equity_before,
                         "portfolio_equity_before");
  detail::require_finite(record.portfolio_equity_after,
                         "portfolio_equity_after");
  detail::require_finite(record.reward_total, "reward_total");
  detail::require_finite(record.transaction_cost_base, "transaction_cost_base");
  detail::require_finite(record.turnover, "turnover");
}

[[nodiscard]] inline episode_trace_t
make_episode_trace(const episode_report_t &episode) {
  episode_trace_t out{};
  out.source_episode_artifact_schema_id = episode.episode_artifact_schema_id;
  out.episode_id = episode.episode_id;
  out.protocol_id = episode.protocol_id;
  out.runtime_run_id = episode.runtime_run_id;
  out.environment_run_id = episode.environment_run_id;
  out.policy_id = episode.policy_id;
  out.method_id = episode.method_id;
  out.policy_kind = episode.policy_kind;
  out.world_mode = episode.world_mode;
  out.graph_order_fingerprint = episode.graph_order_fingerprint;
  out.graph_node_ids = episode.graph_node_ids;
  out.risky_node_ids = episode.risky_node_ids;
  out.base_reserve_node_id = episode.base_reserve_node_id;
  out.accepted_anchor_index_begin = episode.accepted_anchor_index_begin;
  out.accepted_anchor_index_end = episode.accepted_anchor_index_end;
  out.accepted_anchor_keys = episode.accepted_anchor_keys;
  out.accepted_batch_cursor_token = episode.accepted_batch_cursor_token;
  out.realized_return_truth_id = episode.realized_return_truth_id;
  out.transition_count = episode.transition_count;
  out.transitions.reserve(episode.step_reports.size());
  for (const auto &step : episode.step_reports) {
    out.transitions.push_back(make_transition_record(step));
  }
  out.total_reward = episode.total_reward;
  out.total_log_growth = episode.total_log_growth;
  out.final_equity_base = episode.final_equity_base;
  out.max_drawdown = episode.max_drawdown;
  out.total_turnover = episode.total_turnover;
  out.total_transaction_cost_base = episode.total_transaction_cost_base;
  out.projection_mae = episode.projection_mae;
  out.projection_rmse = episode.projection_rmse;
  out.projection_signed_bias = episode.projection_signed_bias;
  out.projection_correlation = episode.projection_correlation;
  out.projection_directional_accuracy = episode.projection_directional_accuracy;
  out.projection_interval_coverage = episode.projection_interval_coverage;
  out.warnings = episode.warnings;
  out.failures = episode.failures;
  return out;
}

inline void validate_episode_trace(const episode_trace_t &trace) {
  detail::require_trace_schema(trace.trace_schema_id, kEpisodeTraceSchema,
                               "episode trace");
  detail::require_trace_schema(trace.source_episode_artifact_schema_id,
                               kReplayEpisodeArtifactSchema,
                               "source episode artifact");
  detail::require_nonblank(trace.episode_id, "episode_id");
  detail::require_nonblank(trace.policy_id, "policy_id");
  detail::require_nonblank(trace.base_reserve_node_id, "base_reserve_node_id");
  if (trace.future_consumer != kFutureCajtucuConsumer) {
    throw std::runtime_error(
        "[experience_trace] episode trace future consumer mismatch");
  }
  if (trace.transition_count != trace.transitions.size()) {
    throw std::runtime_error(
        "[experience_trace] episode transition count mismatch");
  }
  for (const auto &transition : trace.transitions) {
    validate_transition_record(transition);
    if (transition.episode_id != trace.episode_id ||
        transition.policy_id != trace.policy_id ||
        transition.world_mode != trace.world_mode) {
      throw std::runtime_error(
          "[experience_trace] transition identity does not match episode");
    }
  }
  detail::require_finite(trace.total_reward, "total_reward");
  detail::require_finite(trace.total_log_growth, "total_log_growth");
  detail::require_finite(trace.final_equity_base, "final_equity_base");
}

[[nodiscard]] inline policy_comparison_record_t
make_policy_comparison_record(const replay_policy_summary_t &summary) {
  policy_comparison_record_t out{};
  out.source_policy_summary_schema_id = summary.policy_summary_schema_id;
  out.policy_id = summary.policy_id;
  out.policy_kind = summary.policy_kind;
  out.attempted_count = summary.attempted_count;
  out.completed_count = summary.completed_count;
  out.failed_count = summary.failed_count;
  out.mean_total_reward = summary.mean_total_reward;
  out.mean_total_log_growth = summary.mean_total_log_growth;
  out.mean_final_equity_base = summary.mean_final_equity_base;
  out.mean_max_drawdown = summary.mean_max_drawdown;
  out.mean_total_turnover = summary.mean_total_turnover;
  out.mean_total_transaction_cost_base =
      summary.mean_total_transaction_cost_base;
  out.mean_projection_mae = summary.mean_projection_mae;
  out.mean_projection_rmse = summary.mean_projection_rmse;
  out.mean_projection_signed_bias = summary.mean_projection_signed_bias;
  out.mean_projection_correlation = summary.mean_projection_correlation;
  out.mean_projection_directional_accuracy =
      summary.mean_projection_directional_accuracy;
  out.mean_projection_interval_coverage =
      summary.mean_projection_interval_coverage;
  return out;
}

inline void
validate_policy_comparison_record(const policy_comparison_record_t &record) {
  detail::require_trace_schema(record.trace_schema_id,
                               kPolicyComparisonRecordSchema,
                               "policy comparison record");
  detail::require_trace_schema(record.source_policy_summary_schema_id,
                               kReplayPolicyComparisonArtifactSchema,
                               "source policy summary");
  detail::require_nonblank(record.policy_id, "policy_id");
  if (record.future_consumer != kFutureCajtucuConsumer) {
    throw std::runtime_error(
        "[experience_trace] policy comparison future consumer mismatch");
  }
  if (record.completed_count + record.failed_count > record.attempted_count) {
    throw std::runtime_error(
        "[experience_trace] policy comparison counts are inconsistent");
  }
}

[[nodiscard]] inline experience_trace_t
make_experience_trace(const replay_experiment_report_t &report) {
  experience_trace_t out{};
  out.source_experiment_artifact_schema_id =
      report.experiment_artifact_schema_id;
  out.experiment_id = report.experiment_id;
  out.runtime_run_id = report.runtime_run_id;
  out.environment_run_id = report.environment_run_id;
  out.requested_max_parallel_jobs = report.requested_max_parallel_jobs;
  out.resolved_parallelism = report.resolved_parallelism;
  out.attempted_count = report.attempted_count;
  out.completed_count = report.completed_count;
  out.episodes.reserve(report.episode_reports.size());
  for (const auto &episode : report.episode_reports) {
    out.episodes.push_back(make_episode_trace(episode));
  }
  out.policy_comparisons.reserve(report.policy_summaries.size());
  for (const auto &summary : report.policy_summaries) {
    out.policy_comparisons.push_back(make_policy_comparison_record(summary));
  }
  out.warnings = report.warnings;
  out.failures = report.failures;
  return out;
}

inline void validate_experience_trace(const experience_trace_t &trace) {
  detail::require_trace_schema(trace.trace_schema_id, kExperienceTraceSchema,
                               "experience trace");
  detail::require_trace_schema(trace.source_experiment_artifact_schema_id,
                               kReplayExperimentArtifactSchema,
                               "source experiment artifact");
  if (trace.future_consumer != kFutureCajtucuConsumer) {
    throw std::runtime_error(
        "[experience_trace] experience trace future consumer mismatch");
  }
  if (trace.completed_count != trace.episodes.size()) {
    throw std::runtime_error(
        "[experience_trace] completed count must match episode traces");
  }
  if (trace.resolved_parallelism == 0 ||
      (trace.requested_max_parallel_jobs != 0 &&
       trace.resolved_parallelism > trace.requested_max_parallel_jobs)) {
    throw std::runtime_error(
        "[experience_trace] resolved parallelism is inconsistent");
  }
  for (const auto &episode : trace.episodes) {
    validate_episode_trace(episode);
    if ((!trace.runtime_run_id.empty() &&
         episode.runtime_run_id != trace.runtime_run_id) ||
        (!trace.environment_run_id.empty() &&
         episode.environment_run_id != trace.environment_run_id)) {
      throw std::runtime_error(
          "[experience_trace] episode identity does not match experience");
    }
  }
  for (const auto &record : trace.policy_comparisons) {
    validate_policy_comparison_record(record);
  }
}

namespace detail {

inline void write_transition_record(std::ostream &out,
                                    const std::string &prefix,
                                    const transition_record_t &record) {
  write_text_kv(out, prefix + "schema", record.trace_schema_id);
  write_text_kv(out, prefix + "source_step_artifact_schema_id",
                record.source_step_artifact_schema_id);
  write_integral_kv(out, prefix + "step_index", record.step_index);
  write_text_kv(out, prefix + "episode_id", record.episode_id);
  write_text_kv(out, prefix + "protocol_id", record.protocol_id);
  write_text_kv(out, prefix + "runtime_run_id", record.runtime_run_id);
  write_text_kv(out, prefix + "environment_run_id", record.environment_run_id);
  write_text_kv(out, prefix + "anchor_key", record.anchor_key);
  write_integral_kv(out, prefix + "observation_anchor_index",
                    record.observation_anchor_index);
  write_integral_kv(out, prefix + "next_realization_anchor_index",
                    record.next_realization_anchor_index);
  write_integral_kv(out, prefix + "knowledge_timestamp_ms",
                    record.knowledge_timestamp_ms);
  write_integral_kv(out, prefix + "realization_available_after_timestamp_ms",
                    record.realization_available_after_timestamp_ms);
  write_integral_kv(out, prefix + "action_decision_timestamp_ms",
                    record.action_decision_timestamp_ms);
  write_bool_kv(out, prefix + "time_law_clean", record.time_law_clean);
  write_text_kv(out, prefix + "policy_id", record.policy_id);
  write_text_kv(out, prefix + "method_id", record.method_id);
  write_text_kv(out, prefix + "policy_kind",
                policy_kind_name(record.policy_kind));
  write_text_kv(out, prefix + "world_mode", world_mode_name(record.world_mode));
  write_text_kv(out, prefix + "action_schema_id", record.action_schema_id);
  write_text_kv(out, prefix + "target_risky_node_weights",
                record.target_risky_node_weights);
  write_text_kv(out, prefix + "target_base_reserve_node_id",
                record.target_base_reserve_node_id);
  write_double_kv(out, prefix + "target_base_reserve_weight",
                  record.target_base_reserve_weight);
  write_text_kv(out, prefix + "execution_model", record.execution_model);
  write_text_kv(out, prefix + "rebalance_plan_source",
                record.rebalance_plan_source);
  write_bool_kv(out, prefix + "rebalance_plan_enforced",
                record.rebalance_plan_enforced);
  write_integral_kv(out, prefix + "rebalance_order_count",
                    record.rebalance_order_count);
  write_integral_kv(out, prefix + "rebalance_skipped_count",
                    record.rebalance_skipped_count);
  write_integral_kv(out, prefix + "fill_count", record.fill_count);
  write_double_kv(out, prefix + "fill_gross_notional_base",
                  record.fill_gross_notional_base);
  write_double_kv(out, prefix + "fill_fee_base", record.fill_fee_base);
  write_double_kv(out, prefix + "portfolio_equity_before",
                  record.portfolio_equity_before);
  write_double_kv(out, prefix + "portfolio_equity_after",
                  record.portfolio_equity_after);
  write_double_kv(out, prefix + "realized_log_growth",
                  record.realized_log_growth);
  write_double_kv(out, prefix + "realized_arithmetic_return",
                  record.realized_arithmetic_return);
  write_double_kv(out, prefix + "transaction_cost_base",
                  record.transaction_cost_base);
  write_double_kv(out, prefix + "turnover", record.turnover);
  write_bool_kv(out, prefix + "invalid_action", record.invalid_action);
  write_double_kv(out, prefix + "reward_log_growth", record.reward_log_growth);
  write_double_kv(out, prefix + "reward_drawdown_penalty",
                  record.reward_drawdown_penalty);
  write_double_kv(out, prefix + "reward_transaction_cost_penalty",
                  record.reward_transaction_cost_penalty);
  write_double_kv(out, prefix + "reward_turnover_penalty",
                  record.reward_turnover_penalty);
  write_double_kv(out, prefix + "reward_invalid_action_penalty",
                  record.reward_invalid_action_penalty);
  write_double_kv(out, prefix + "reward_total", record.reward_total);
  write_bool_kv(out, prefix + "projection_validation_available",
                record.projection_validation_available);
  write_double_kv(out, prefix + "projection_mae", record.projection_mae);
  write_double_kv(out, prefix + "projection_rmse", record.projection_rmse);
  write_double_kv(out, prefix + "projection_signed_bias",
                  record.projection_signed_bias);
  write_double_kv(out, prefix + "projection_correlation",
                  record.projection_correlation);
  write_double_kv(out, prefix + "projection_directional_accuracy",
                  record.projection_directional_accuracy);
  write_double_kv(out, prefix + "projection_interval_coverage",
                  record.projection_interval_coverage);
  write_bool_kv(out, prefix + "residual_quality_available",
                record.residual_quality_available);
  write_bool_kv(out, prefix + "residual_quality_valid",
                record.residual_quality_valid);
  write_double_kv(out, prefix + "residual_mean_residual_energy",
                  record.residual_mean_residual_energy);
  write_double_kv(out, prefix + "residual_max_residual_energy",
                  record.residual_max_residual_energy);
  write_bool_kv(out, prefix + "risk_gate_evaluated",
                record.risk_gate_evaluated);
  write_bool_kv(out, prefix + "risk_gate_allow_trading",
                record.risk_gate_allow_trading);
  write_bool_kv(out, prefix + "risk_gate_force_base_reserve_fallback",
                record.risk_gate_force_base_reserve_fallback);
  write_integral_kv(out, prefix + "warning_count", record.warning_count);
  write_integral_kv(out, prefix + "failure_count", record.failure_count);
  write_text_kv(out, prefix + "warnings", record.warnings);
  write_text_kv(out, prefix + "failures", record.failures);
}

[[nodiscard]] inline transition_record_t
read_transition_record(const kv_map_t &values, const std::string &prefix) {
  transition_record_t record{};
  record.trace_schema_id = read_text(values, prefix + "schema");
  record.source_step_artifact_schema_id =
      read_text(values, prefix + "source_step_artifact_schema_id");
  record.step_index = read_u64(values, prefix + "step_index");
  record.episode_id = read_text(values, prefix + "episode_id");
  record.protocol_id = read_text(values, prefix + "protocol_id");
  record.runtime_run_id = read_text(values, prefix + "runtime_run_id");
  record.environment_run_id = read_text(values, prefix + "environment_run_id");
  record.anchor_key = read_text(values, prefix + "anchor_key");
  record.observation_anchor_index =
      read_i64(values, prefix + "observation_anchor_index");
  record.next_realization_anchor_index =
      read_i64(values, prefix + "next_realization_anchor_index");
  record.knowledge_timestamp_ms =
      read_i64(values, prefix + "knowledge_timestamp_ms");
  record.realization_available_after_timestamp_ms =
      read_i64(values, prefix + "realization_available_after_timestamp_ms");
  record.action_decision_timestamp_ms =
      read_i64(values, prefix + "action_decision_timestamp_ms");
  record.time_law_clean = read_bool(values, prefix + "time_law_clean");
  record.policy_id = read_text(values, prefix + "policy_id");
  record.method_id = read_text(values, prefix + "method_id");
  record.policy_kind = read_policy_kind(values, prefix + "policy_kind");
  record.world_mode = read_world_mode(values, prefix + "world_mode");
  record.action_schema_id = read_text(values, prefix + "action_schema_id");
  record.target_risky_node_weights =
      read_text(values, prefix + "target_risky_node_weights");
  record.target_base_reserve_node_id =
      read_text(values, prefix + "target_base_reserve_node_id");
  record.target_base_reserve_weight =
      read_double(values, prefix + "target_base_reserve_weight");
  record.execution_model = read_text(values, prefix + "execution_model");
  record.rebalance_plan_source =
      read_text(values, prefix + "rebalance_plan_source");
  record.rebalance_plan_enforced =
      read_bool(values, prefix + "rebalance_plan_enforced");
  record.rebalance_order_count =
      read_u64(values, prefix + "rebalance_order_count");
  record.rebalance_skipped_count =
      read_u64(values, prefix + "rebalance_skipped_count");
  record.fill_count = read_u64(values, prefix + "fill_count");
  record.fill_gross_notional_base =
      read_double(values, prefix + "fill_gross_notional_base");
  record.fill_fee_base = read_double(values, prefix + "fill_fee_base");
  record.portfolio_equity_before =
      read_double(values, prefix + "portfolio_equity_before");
  record.portfolio_equity_after =
      read_double(values, prefix + "portfolio_equity_after");
  record.realized_log_growth =
      read_double(values, prefix + "realized_log_growth");
  record.realized_arithmetic_return =
      read_double(values, prefix + "realized_arithmetic_return");
  record.transaction_cost_base =
      read_double(values, prefix + "transaction_cost_base");
  record.turnover = read_double(values, prefix + "turnover");
  record.invalid_action = read_bool(values, prefix + "invalid_action");
  record.reward_log_growth = read_double(values, prefix + "reward_log_growth");
  record.reward_drawdown_penalty =
      read_double(values, prefix + "reward_drawdown_penalty");
  record.reward_transaction_cost_penalty =
      read_double(values, prefix + "reward_transaction_cost_penalty");
  record.reward_turnover_penalty =
      read_double(values, prefix + "reward_turnover_penalty");
  record.reward_invalid_action_penalty =
      read_double(values, prefix + "reward_invalid_action_penalty");
  record.reward_total = read_double(values, prefix + "reward_total");
  record.projection_validation_available =
      read_bool(values, prefix + "projection_validation_available");
  record.projection_mae = read_double(values, prefix + "projection_mae");
  record.projection_rmse = read_double(values, prefix + "projection_rmse");
  record.projection_signed_bias =
      read_double(values, prefix + "projection_signed_bias");
  record.projection_correlation =
      read_double(values, prefix + "projection_correlation");
  record.projection_directional_accuracy =
      read_double(values, prefix + "projection_directional_accuracy");
  record.projection_interval_coverage =
      read_double(values, prefix + "projection_interval_coverage");
  record.residual_quality_available =
      read_bool(values, prefix + "residual_quality_available");
  record.residual_quality_valid =
      read_bool(values, prefix + "residual_quality_valid");
  record.residual_mean_residual_energy =
      read_double(values, prefix + "residual_mean_residual_energy");
  record.residual_max_residual_energy =
      read_double(values, prefix + "residual_max_residual_energy");
  record.risk_gate_evaluated =
      read_bool(values, prefix + "risk_gate_evaluated");
  record.risk_gate_allow_trading =
      read_bool(values, prefix + "risk_gate_allow_trading");
  record.risk_gate_force_base_reserve_fallback =
      read_bool(values, prefix + "risk_gate_force_base_reserve_fallback");
  record.warning_count = read_u64(values, prefix + "warning_count");
  record.failure_count = read_u64(values, prefix + "failure_count");
  record.warnings = read_text(values, prefix + "warnings");
  record.failures = read_text(values, prefix + "failures");
  return record;
}

inline void write_episode_trace(std::ostream &out, const std::string &prefix,
                                const episode_trace_t &trace) {
  write_text_kv(out, prefix + "schema", trace.trace_schema_id);
  write_text_kv(out, prefix + "source_episode_artifact_schema_id",
                trace.source_episode_artifact_schema_id);
  write_text_kv(out, prefix + "future_consumer", trace.future_consumer);
  write_text_kv(out, prefix + "episode_id", trace.episode_id);
  write_text_kv(out, prefix + "protocol_id", trace.protocol_id);
  write_text_kv(out, prefix + "runtime_run_id", trace.runtime_run_id);
  write_text_kv(out, prefix + "environment_run_id", trace.environment_run_id);
  write_text_kv(out, prefix + "policy_id", trace.policy_id);
  write_text_kv(out, prefix + "method_id", trace.method_id);
  write_text_kv(out, prefix + "policy_kind",
                policy_kind_name(trace.policy_kind));
  write_text_kv(out, prefix + "world_mode", world_mode_name(trace.world_mode));
  write_text_kv(out, prefix + "graph_order_fingerprint",
                trace.graph_order_fingerprint);
  write_text_kv(out, prefix + "graph_node_ids", trace.graph_node_ids);
  write_text_kv(out, prefix + "risky_node_ids", trace.risky_node_ids);
  write_text_kv(out, prefix + "base_reserve_node_id",
                trace.base_reserve_node_id);
  write_integral_kv(out, prefix + "accepted_anchor_index_begin",
                    trace.accepted_anchor_index_begin);
  write_integral_kv(out, prefix + "accepted_anchor_index_end",
                    trace.accepted_anchor_index_end);
  write_text_kv(out, prefix + "accepted_anchor_keys",
                trace.accepted_anchor_keys);
  write_text_kv(out, prefix + "accepted_batch_cursor_token",
                trace.accepted_batch_cursor_token);
  write_text_kv(out, prefix + "realized_return_truth_id",
                trace.realized_return_truth_id);
  write_integral_kv(out, prefix + "transition_count", trace.transition_count);
  write_double_kv(out, prefix + "total_reward", trace.total_reward);
  write_double_kv(out, prefix + "total_log_growth", trace.total_log_growth);
  write_double_kv(out, prefix + "final_equity_base", trace.final_equity_base);
  write_double_kv(out, prefix + "max_drawdown", trace.max_drawdown);
  write_double_kv(out, prefix + "total_turnover", trace.total_turnover);
  write_double_kv(out, prefix + "total_transaction_cost_base",
                  trace.total_transaction_cost_base);
  write_double_kv(out, prefix + "projection_mae", trace.projection_mae);
  write_double_kv(out, prefix + "projection_rmse", trace.projection_rmse);
  write_double_kv(out, prefix + "projection_signed_bias",
                  trace.projection_signed_bias);
  write_double_kv(out, prefix + "projection_correlation",
                  trace.projection_correlation);
  write_double_kv(out, prefix + "projection_directional_accuracy",
                  trace.projection_directional_accuracy);
  write_double_kv(out, prefix + "projection_interval_coverage",
                  trace.projection_interval_coverage);
  write_string_vector(out, prefix + "warning_", trace.warnings);
  write_string_vector(out, prefix + "failure_", trace.failures);
  for (std::size_t i = 0; i < trace.transitions.size(); ++i) {
    write_transition_record(out,
                            prefix + "transition_" + std::to_string(i) + "_",
                            trace.transitions[i]);
  }
}

[[nodiscard]] inline episode_trace_t
read_episode_trace(const kv_map_t &values, const std::string &prefix) {
  episode_trace_t trace{};
  trace.trace_schema_id = read_text(values, prefix + "schema");
  trace.source_episode_artifact_schema_id =
      read_text(values, prefix + "source_episode_artifact_schema_id");
  trace.future_consumer = read_text(values, prefix + "future_consumer");
  trace.episode_id = read_text(values, prefix + "episode_id");
  trace.protocol_id = read_text(values, prefix + "protocol_id");
  trace.runtime_run_id = read_text(values, prefix + "runtime_run_id");
  trace.environment_run_id = read_text(values, prefix + "environment_run_id");
  trace.policy_id = read_text(values, prefix + "policy_id");
  trace.method_id = read_text(values, prefix + "method_id");
  trace.policy_kind = read_policy_kind(values, prefix + "policy_kind");
  trace.world_mode = read_world_mode(values, prefix + "world_mode");
  trace.graph_order_fingerprint =
      read_text(values, prefix + "graph_order_fingerprint");
  trace.graph_node_ids = read_text(values, prefix + "graph_node_ids");
  trace.risky_node_ids = read_text(values, prefix + "risky_node_ids");
  trace.base_reserve_node_id =
      read_text(values, prefix + "base_reserve_node_id");
  trace.accepted_anchor_index_begin =
      read_i64(values, prefix + "accepted_anchor_index_begin");
  trace.accepted_anchor_index_end =
      read_i64(values, prefix + "accepted_anchor_index_end");
  trace.accepted_anchor_keys =
      read_text(values, prefix + "accepted_anchor_keys");
  trace.accepted_batch_cursor_token =
      read_text(values, prefix + "accepted_batch_cursor_token");
  trace.realized_return_truth_id =
      read_text(values, prefix + "realized_return_truth_id");
  trace.transition_count = read_u64(values, prefix + "transition_count");
  trace.total_reward = read_double(values, prefix + "total_reward");
  trace.total_log_growth = read_double(values, prefix + "total_log_growth");
  trace.final_equity_base = read_double(values, prefix + "final_equity_base");
  trace.max_drawdown = read_double(values, prefix + "max_drawdown");
  trace.total_turnover = read_double(values, prefix + "total_turnover");
  trace.total_transaction_cost_base =
      read_double(values, prefix + "total_transaction_cost_base");
  trace.projection_mae = read_double(values, prefix + "projection_mae");
  trace.projection_rmse = read_double(values, prefix + "projection_rmse");
  trace.projection_signed_bias =
      read_double(values, prefix + "projection_signed_bias");
  trace.projection_correlation =
      read_double(values, prefix + "projection_correlation");
  trace.projection_directional_accuracy =
      read_double(values, prefix + "projection_directional_accuracy");
  trace.projection_interval_coverage =
      read_double(values, prefix + "projection_interval_coverage");
  trace.warnings = read_string_vector(values, prefix + "warning_");
  trace.failures = read_string_vector(values, prefix + "failure_");
  trace.transitions.reserve(trace.transition_count);
  for (std::uint64_t i = 0; i < trace.transition_count; ++i) {
    trace.transitions.push_back(read_transition_record(
        values, prefix + "transition_" + std::to_string(i) + "_"));
  }
  return trace;
}

inline void
write_policy_comparison_record(std::ostream &out, const std::string &prefix,
                               const policy_comparison_record_t &record) {
  write_text_kv(out, prefix + "schema", record.trace_schema_id);
  write_text_kv(out, prefix + "source_policy_summary_schema_id",
                record.source_policy_summary_schema_id);
  write_text_kv(out, prefix + "future_consumer", record.future_consumer);
  write_text_kv(out, prefix + "policy_id", record.policy_id);
  write_text_kv(out, prefix + "policy_kind",
                policy_kind_name(record.policy_kind));
  write_integral_kv(out, prefix + "attempted_count", record.attempted_count);
  write_integral_kv(out, prefix + "completed_count", record.completed_count);
  write_integral_kv(out, prefix + "failed_count", record.failed_count);
  write_double_kv(out, prefix + "mean_total_reward", record.mean_total_reward);
  write_double_kv(out, prefix + "mean_total_log_growth",
                  record.mean_total_log_growth);
  write_double_kv(out, prefix + "mean_final_equity_base",
                  record.mean_final_equity_base);
  write_double_kv(out, prefix + "mean_max_drawdown", record.mean_max_drawdown);
  write_double_kv(out, prefix + "mean_total_turnover",
                  record.mean_total_turnover);
  write_double_kv(out, prefix + "mean_total_transaction_cost_base",
                  record.mean_total_transaction_cost_base);
  write_double_kv(out, prefix + "mean_projection_mae",
                  record.mean_projection_mae);
  write_double_kv(out, prefix + "mean_projection_rmse",
                  record.mean_projection_rmse);
  write_double_kv(out, prefix + "mean_projection_signed_bias",
                  record.mean_projection_signed_bias);
  write_double_kv(out, prefix + "mean_projection_correlation",
                  record.mean_projection_correlation);
  write_double_kv(out, prefix + "mean_projection_directional_accuracy",
                  record.mean_projection_directional_accuracy);
  write_double_kv(out, prefix + "mean_projection_interval_coverage",
                  record.mean_projection_interval_coverage);
}

[[nodiscard]] inline policy_comparison_record_t
read_policy_comparison_record(const kv_map_t &values,
                              const std::string &prefix) {
  policy_comparison_record_t record{};
  record.trace_schema_id = read_text(values, prefix + "schema");
  record.source_policy_summary_schema_id =
      read_text(values, prefix + "source_policy_summary_schema_id");
  record.future_consumer = read_text(values, prefix + "future_consumer");
  record.policy_id = read_text(values, prefix + "policy_id");
  record.policy_kind = read_policy_kind(values, prefix + "policy_kind");
  record.attempted_count = read_u64(values, prefix + "attempted_count");
  record.completed_count = read_u64(values, prefix + "completed_count");
  record.failed_count = read_u64(values, prefix + "failed_count");
  record.mean_total_reward = read_double(values, prefix + "mean_total_reward");
  record.mean_total_log_growth =
      read_double(values, prefix + "mean_total_log_growth");
  record.mean_final_equity_base =
      read_double(values, prefix + "mean_final_equity_base");
  record.mean_max_drawdown = read_double(values, prefix + "mean_max_drawdown");
  record.mean_total_turnover =
      read_double(values, prefix + "mean_total_turnover");
  record.mean_total_transaction_cost_base =
      read_double(values, prefix + "mean_total_transaction_cost_base");
  record.mean_projection_mae =
      read_double(values, prefix + "mean_projection_mae");
  record.mean_projection_rmse =
      read_double(values, prefix + "mean_projection_rmse");
  record.mean_projection_signed_bias =
      read_double(values, prefix + "mean_projection_signed_bias");
  record.mean_projection_correlation =
      read_double(values, prefix + "mean_projection_correlation");
  record.mean_projection_directional_accuracy =
      read_double(values, prefix + "mean_projection_directional_accuracy");
  record.mean_projection_interval_coverage =
      read_double(values, prefix + "mean_projection_interval_coverage");
  return record;
}

} // namespace detail

inline void write_experience_trace_report(const experience_trace_t &trace,
                                          const std::filesystem::path &path) {
  if (path.empty()) {
    throw std::runtime_error("[experience_trace] report path is required");
  }
  validate_experience_trace(trace);

  std::ofstream out(path);
  if (!out) {
    throw std::runtime_error("[experience_trace] failed to open report path " +
                             path.string());
  }

  detail::write_text_kv(out, "schema", trace.trace_schema_id);
  detail::write_text_kv(out, "source_experiment_artifact_schema_id",
                        trace.source_experiment_artifact_schema_id);
  detail::write_text_kv(out, "future_consumer", trace.future_consumer);
  detail::write_text_kv(out, "experiment_id", trace.experiment_id);
  detail::write_text_kv(out, "runtime_run_id", trace.runtime_run_id);
  detail::write_text_kv(out, "environment_run_id", trace.environment_run_id);
  detail::write_integral_kv(out, "requested_max_parallel_jobs",
                            trace.requested_max_parallel_jobs);
  detail::write_integral_kv(out, "resolved_parallelism",
                            trace.resolved_parallelism);
  detail::write_integral_kv(out, "attempted_count", trace.attempted_count);
  detail::write_integral_kv(out, "completed_count", trace.completed_count);
  detail::write_integral_kv(out, "episode_count", trace.episodes.size());
  detail::write_integral_kv(out, "policy_comparison_count",
                            trace.policy_comparisons.size());
  detail::write_string_vector(out, "warning_", trace.warnings);
  detail::write_string_vector(out, "failure_", trace.failures);

  for (std::size_t i = 0; i < trace.episodes.size(); ++i) {
    detail::write_episode_trace(out, "episode_" + std::to_string(i) + "_",
                                trace.episodes[i]);
  }
  for (std::size_t i = 0; i < trace.policy_comparisons.size(); ++i) {
    detail::write_policy_comparison_record(
        out, "policy_comparison_" + std::to_string(i) + "_",
        trace.policy_comparisons[i]);
  }
  if (!out) {
    throw std::runtime_error("[experience_trace] failed while writing " +
                             path.string());
  }
}

[[nodiscard]] inline experience_trace_t
read_experience_trace_report(const std::filesystem::path &path) {
  if (path.empty()) {
    throw std::runtime_error("[experience_trace] report path is required");
  }
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("[experience_trace] failed to open report path " +
                             path.string());
  }
  const auto values = detail::parse_kv_stream(in);

  experience_trace_t trace{};
  trace.trace_schema_id = detail::read_text(values, "schema");
  trace.source_experiment_artifact_schema_id =
      detail::read_text(values, "source_experiment_artifact_schema_id");
  trace.future_consumer = detail::read_text(values, "future_consumer");
  trace.experiment_id = detail::read_text(values, "experiment_id");
  trace.runtime_run_id = detail::read_text(values, "runtime_run_id");
  trace.environment_run_id = detail::read_text(values, "environment_run_id");
  trace.requested_max_parallel_jobs =
      detail::read_size(values, "requested_max_parallel_jobs");
  trace.resolved_parallelism =
      detail::read_size(values, "resolved_parallelism");
  trace.attempted_count = detail::read_u64(values, "attempted_count");
  trace.completed_count = detail::read_u64(values, "completed_count");
  const auto episode_count = detail::read_size(values, "episode_count");
  const auto policy_comparison_count =
      detail::read_size(values, "policy_comparison_count");
  trace.warnings = detail::read_string_vector(values, "warning_");
  trace.failures = detail::read_string_vector(values, "failure_");

  trace.episodes.reserve(episode_count);
  for (std::size_t i = 0; i < episode_count; ++i) {
    trace.episodes.push_back(detail::read_episode_trace(
        values, "episode_" + std::to_string(i) + "_"));
  }
  trace.policy_comparisons.reserve(policy_comparison_count);
  for (std::size_t i = 0; i < policy_comparison_count; ++i) {
    trace.policy_comparisons.push_back(detail::read_policy_comparison_record(
        values, "policy_comparison_" + std::to_string(i) + "_"));
  }

  validate_experience_trace(trace);
  return trace;
}

} // namespace cuwacunu::kikijyeba::environment::output
