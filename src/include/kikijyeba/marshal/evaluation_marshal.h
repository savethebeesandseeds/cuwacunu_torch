// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "kikijyeba/marshal/digest.h"

namespace cuwacunu::kikijyeba::marshal {

inline constexpr const char *k_marshal_evaluation_request_schema_v1 =
    "kikijyeba.marshal.evaluation_request.v1";
inline constexpr const char *k_marshal_evaluation_plan_schema_v1 =
    "kikijyeba.marshal.evaluation_plan.v1";
inline constexpr const char *k_marshal_evaluation_runtime_handoff_schema_v1 =
    "kikijyeba.marshal.evaluation_runtime_handoff.v1";
inline constexpr const char *k_marshal_evaluation_receipt_schema_v1 =
    "kikijyeba.marshal.evaluation_receipt.v1";

inline constexpr const char *k_marshal_evaluation_non_authority_statement =
    "Marshal evaluation prepares bounded Runtime replay experiments and "
    "records audit receipts; Runtime executes and Lattice proves target "
    "satisfaction.";

struct marshal_evaluation_anchor_range_t {
  std::string split_id{};
  std::int64_t anchor_index_begin{0};
  std::int64_t anchor_index_end{0};
};

struct marshal_evaluation_request_t {
  std::string evaluation_id{};
  std::string target_id{"channel_mdn_validation_eval_ready"};
  std::string protocol_id{"cwu_02v"};
  std::string target_component_family_id{
      "wikimyei.inference.expected_value.mdn"};
  std::string wave_id{};
  std::string wave_mode{"run|debug"};
  std::string source_order{"sequential"};

  std::filesystem::path runtime_exec_path{
      "/cuwacunu/.build/exec/cuwacunu_exec"};
  std::filesystem::path evaluation_config_path{};
  std::filesystem::path input_mdn_checkpoint{};
  std::filesystem::path input_representation_checkpoint{};

  marshal_evaluation_anchor_range_t trained_range{};
  marshal_evaluation_anchor_range_t validation_range{};
  std::string trained_range_evidence_digest{};
  std::string validation_split_policy_digest{};

  std::string base_reserve_node_id{};
  std::vector<std::string> risky_node_ids{};
  bool include_base_reserve_policy{true};
  bool include_equal_weight_policy{true};
  bool include_current_weight_policy{true};
  bool include_spot_distributional_utility_policy{true};
  std::int64_t max_replay_steps{0};
  std::int64_t max_parallel_replay_jobs{0};
  int runtime_timeout_seconds{600};
  bool force_rebuild_cache{false};

  bool require_existing_paths{true};
  bool require_non_overlapping_ranges{true};
};

struct marshal_evaluation_plan_t {
  std::string schema_version{k_marshal_evaluation_plan_schema_v1};
  std::string request_digest{};
  std::string evaluation_id{};
  bool accepted{false};
  bool target_satisfaction_claimed{false};
  bool runtime_executor{false};
  bool lattice_proof_authority{false};
  bool validation_range_out_of_sample{false};
  std::string runtime_run_command{};
  std::string runtime_replay_command_template{};
  std::vector<std::string> refusal_reasons{};
  std::string plan_digest{};
  std::string non_authority_statement{
      k_marshal_evaluation_non_authority_statement};
};

struct marshal_evaluation_runtime_handoff_t {
  std::string schema_version{k_marshal_evaluation_runtime_handoff_schema_v1};
  std::string request_digest{};
  std::string plan_digest{};
  std::string evaluation_id{};
  bool accepted{false};
  bool dry_run{true};
  bool confirm_execute{false};
  bool target_satisfaction_claimed{false};
  bool runtime_executor{false};
  bool lattice_proof_authority{false};
  std::string runtime_tool_name{"hero.runtime.execute"};
  std::string runtime_execute_args_json{};
  std::string runtime_execute_args_digest{};
  std::string runtime_replay_command_template{};
  std::vector<std::string> refusal_reasons{};
  std::string handoff_digest{};
  std::string non_authority_statement{
      k_marshal_evaluation_non_authority_statement};
};

struct marshal_evaluation_policy_summary_t {
  std::string policy_id{};
  std::string policy_kind{};
  std::string execution_backend_id{};
  std::int64_t attempted_count{0};
  std::int64_t completed_count{0};
  std::int64_t failed_count{0};
  std::int64_t cajtucu_execution_step_count{0};
  std::int64_t cajtucu_rejected_fill_count{0};
  std::int64_t cajtucu_partial_fill_count{0};
  double mean_total_reward{0.0};
  double mean_total_log_growth{0.0};
  double mean_final_equity_base{0.0};
  double mean_max_drawdown{0.0};
  double mean_total_turnover{0.0};
  double mean_total_transaction_cost_base{0.0};
  double mean_projection_mae{0.0};
  double mean_projection_rmse{0.0};
  double mean_projection_correlation{0.0};
  double mean_projection_directional_accuracy{0.0};
  double mean_projection_interval_coverage{0.0};
  double mean_projection_interval_width{0.0};
  double mean_zero_return_baseline_mae{0.0};
  double mean_zero_return_baseline_rmse{0.0};
  double mean_model_skill_vs_zero_mae{0.0};
  double mean_model_skill_vs_zero_rmse{0.0};
  double mean_cajtucu_order_count{0.0};
  double mean_cajtucu_fill_count{0.0};
  double mean_cajtucu_total_fee_base{0.0};
  double mean_cajtucu_total_spread_cost_base{0.0};
  double mean_cajtucu_total_slippage_base{0.0};
  double mean_cajtucu_total_transaction_cost_base{0.0};
};

struct marshal_evaluation_receipt_t {
  std::string schema_version{k_marshal_evaluation_receipt_schema_v1};
  std::string request_digest{};
  std::string plan_digest{};
  std::string evaluation_id{};
  std::filesystem::path runtime_job_dir{};
  std::filesystem::path replay_report_path{};
  std::string top_level_metric_scope{
      "mean_over_completed_episode_reports_across_policies"};
  std::string policy_metric_scope{
      "mean_over_completed_episode_reports_for_policy"};
  std::int64_t replay_bundle_count{0};
  std::int64_t attempted_count{0};
  std::int64_t completed_count{0};
  std::int64_t failed_count{0};
  std::string execution_backend_id{};
  std::int64_t cajtucu_execution_step_count{0};
  std::int64_t cajtucu_rejected_fill_count{0};
  std::int64_t cajtucu_partial_fill_count{0};
  double mean_projection_mae{0.0};
  double mean_projection_rmse{0.0};
  double mean_projection_correlation{0.0};
  double mean_projection_directional_accuracy{0.0};
  double mean_projection_interval_coverage{0.0};
  double mean_projection_interval_width{0.0};
  double mean_zero_return_baseline_mae{0.0};
  double mean_zero_return_baseline_rmse{0.0};
  double mean_model_skill_vs_zero_mae{0.0};
  double mean_model_skill_vs_zero_rmse{0.0};
  double mean_total_log_growth{0.0};
  double mean_final_equity_base{0.0};
  double mean_total_transaction_cost_base{0.0};
  double mean_cajtucu_order_count{0.0};
  double mean_cajtucu_fill_count{0.0};
  double mean_cajtucu_total_fee_base{0.0};
  double mean_cajtucu_total_spread_cost_base{0.0};
  double mean_cajtucu_total_slippage_base{0.0};
  double mean_cajtucu_total_transaction_cost_base{0.0};
  std::int64_t time_law_future_observation_violation_count{0};
  std::int64_t mixed_future_realization_key_count{0};
  std::int64_t projection_validation_step_count{0};
  std::int64_t policy_summary_count{0};
  std::string best_policy_by_final_equity{};
  std::string best_policy_by_after_cost_return{};
  std::string best_policy_by_drawdown_adjusted_return{};
  std::vector<marshal_evaluation_policy_summary_t> policy_summaries{};
  bool target_satisfaction_claimed{false};
  bool runtime_executor{false};
  bool lattice_proof_authority{false};
  std::string receipt_digest{};
  std::string non_authority_statement{
      k_marshal_evaluation_non_authority_statement};
};

namespace evaluation_marshal_detail {

inline void append_kv(std::ostringstream &out, const std::string &key,
                      const std::string &value) {
  out << key << "=" << value << "\n";
}

inline void append_bool(std::ostringstream &out, const std::string &key,
                        bool value) {
  append_kv(out, key, value ? "true" : "false");
}

inline void append_i64(std::ostringstream &out, const std::string &key,
                       std::int64_t value) {
  append_kv(out, key, std::to_string(value));
}

inline void append_double(std::ostringstream &out, const std::string &key,
                          double value) {
  std::ostringstream number;
  number.precision(17);
  number << value;
  append_kv(out, key, number.str());
}

[[nodiscard]] inline std::string csv(const std::vector<std::string> &values) {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << values[i];
  }
  return out.str();
}

[[nodiscard]] inline std::string shell_quote(const std::string &value) {
  std::string out = "'";
  for (const char c : value) {
    if (c == '\'') {
      out += "'\\''";
    } else {
      out.push_back(c);
    }
  }
  out.push_back('\'');
  return out;
}

[[nodiscard]] inline std::string json_quote(std::string_view value) {
  std::ostringstream out;
  out << '"';
  for (const unsigned char c : value) {
    switch (c) {
    case '"':
      out << "\\\"";
      break;
    case '\\':
      out << "\\\\";
      break;
    case '\b':
      out << "\\b";
      break;
    case '\f':
      out << "\\f";
      break;
    case '\n':
      out << "\\n";
      break;
    case '\r':
      out << "\\r";
      break;
    case '\t':
      out << "\\t";
      break;
    default:
      if (c < 0x20) {
        static constexpr char kHex[] = "0123456789abcdef";
        out << "\\u00" << kHex[(c >> 4) & 0x0f] << kHex[c & 0x0f];
      } else {
        out << static_cast<char>(c);
      }
      break;
    }
  }
  out << '"';
  return out.str();
}

inline void append_json_string_field(std::ostringstream &out,
                                     const std::string &key,
                                     const std::string &value, bool *first) {
  if (!*first) {
    out << ",";
  }
  *first = false;
  out << json_quote(key) << ":" << json_quote(value);
}

inline void append_json_bool_field(std::ostringstream &out,
                                   const std::string &key, bool value,
                                   bool *first) {
  if (!*first) {
    out << ",";
  }
  *first = false;
  out << json_quote(key) << ":" << (value ? "true" : "false");
}

inline void append_json_i64_field(std::ostringstream &out,
                                  const std::string &key, std::int64_t value,
                                  bool *first) {
  if (!*first) {
    out << ",";
  }
  *first = false;
  out << json_quote(key) << ":" << value;
}

[[nodiscard]] inline bool
valid_range(const marshal_evaluation_anchor_range_t &range) {
  return !range.split_id.empty() && range.anchor_index_begin >= 0 &&
         range.anchor_index_end > range.anchor_index_begin;
}

[[nodiscard]] inline bool
ranges_overlap(const marshal_evaluation_anchor_range_t &lhs,
               const marshal_evaluation_anchor_range_t &rhs) {
  return lhs.anchor_index_begin < rhs.anchor_index_end &&
         rhs.anchor_index_begin < lhs.anchor_index_end;
}

inline void append_range(std::ostringstream &out, const std::string &prefix,
                         const marshal_evaluation_anchor_range_t &range) {
  append_kv(out, prefix + "_split_id", range.split_id);
  append_i64(out, prefix + "_anchor_index_begin", range.anchor_index_begin);
  append_i64(out, prefix + "_anchor_index_end", range.anchor_index_end);
}

inline void
append_policy_summary(std::ostringstream &out, const std::string &prefix,
                      const marshal_evaluation_policy_summary_t &summary) {
  append_kv(out, prefix + "id", summary.policy_id);
  append_kv(out, prefix + "kind", summary.policy_kind);
  append_kv(out, prefix + "execution_backend_id", summary.execution_backend_id);
  append_i64(out, prefix + "attempted_count", summary.attempted_count);
  append_i64(out, prefix + "completed_count", summary.completed_count);
  append_i64(out, prefix + "failed_count", summary.failed_count);
  append_i64(out, prefix + "cajtucu_execution_step_count",
             summary.cajtucu_execution_step_count);
  append_i64(out, prefix + "cajtucu_rejected_fill_count",
             summary.cajtucu_rejected_fill_count);
  append_i64(out, prefix + "cajtucu_partial_fill_count",
             summary.cajtucu_partial_fill_count);
  append_double(out, prefix + "mean_total_reward", summary.mean_total_reward);
  append_double(out, prefix + "mean_total_log_growth",
                summary.mean_total_log_growth);
  append_double(out, prefix + "mean_final_equity_base",
                summary.mean_final_equity_base);
  append_double(out, prefix + "mean_max_drawdown", summary.mean_max_drawdown);
  append_double(out, prefix + "mean_total_turnover",
                summary.mean_total_turnover);
  append_double(out, prefix + "mean_total_transaction_cost_base",
                summary.mean_total_transaction_cost_base);
  append_double(out, prefix + "mean_projection_mae",
                summary.mean_projection_mae);
  append_double(out, prefix + "mean_projection_rmse",
                summary.mean_projection_rmse);
  append_double(out, prefix + "mean_projection_correlation",
                summary.mean_projection_correlation);
  append_double(out, prefix + "mean_projection_directional_accuracy",
                summary.mean_projection_directional_accuracy);
  append_double(out, prefix + "mean_projection_interval_coverage",
                summary.mean_projection_interval_coverage);
  append_double(out, prefix + "mean_projection_interval_width",
                summary.mean_projection_interval_width);
  append_double(out, prefix + "mean_zero_return_baseline_mae",
                summary.mean_zero_return_baseline_mae);
  append_double(out, prefix + "mean_zero_return_baseline_rmse",
                summary.mean_zero_return_baseline_rmse);
  append_double(out, prefix + "mean_model_skill_vs_zero_mae",
                summary.mean_model_skill_vs_zero_mae);
  append_double(out, prefix + "mean_model_skill_vs_zero_rmse",
                summary.mean_model_skill_vs_zero_rmse);
  append_double(out, prefix + "mean_cajtucu_order_count",
                summary.mean_cajtucu_order_count);
  append_double(out, prefix + "mean_cajtucu_fill_count",
                summary.mean_cajtucu_fill_count);
  append_double(out, prefix + "mean_cajtucu_total_fee_base",
                summary.mean_cajtucu_total_fee_base);
  append_double(out, prefix + "mean_cajtucu_total_spread_cost_base",
                summary.mean_cajtucu_total_spread_cost_base);
  append_double(out, prefix + "mean_cajtucu_total_slippage_base",
                summary.mean_cajtucu_total_slippage_base);
  append_double(out, prefix + "mean_cajtucu_total_transaction_cost_base",
                summary.mean_cajtucu_total_transaction_cost_base);
}

} // namespace evaluation_marshal_detail

[[nodiscard]] inline std::string
canonical_evaluation_request_text(const marshal_evaluation_request_t &request) {
  namespace detail = evaluation_marshal_detail;
  std::ostringstream out;
  detail::append_kv(out, "schema_version",
                    k_marshal_evaluation_request_schema_v1);
  detail::append_kv(out, "evaluation_id", request.evaluation_id);
  detail::append_kv(out, "target_id", request.target_id);
  detail::append_kv(out, "protocol_id", request.protocol_id);
  detail::append_kv(out, "target_component_family_id",
                    request.target_component_family_id);
  detail::append_kv(out, "wave_id", request.wave_id);
  detail::append_kv(out, "wave_mode", request.wave_mode);
  detail::append_kv(out, "source_order", request.source_order);
  detail::append_kv(out, "runtime_exec_path",
                    request.runtime_exec_path.string());
  detail::append_kv(out, "evaluation_config_path",
                    request.evaluation_config_path.string());
  detail::append_kv(out, "input_mdn_checkpoint",
                    request.input_mdn_checkpoint.string());
  detail::append_kv(out, "input_representation_checkpoint",
                    request.input_representation_checkpoint.string());
  detail::append_range(out, "trained_range", request.trained_range);
  detail::append_range(out, "validation_range", request.validation_range);
  detail::append_kv(out, "trained_range_evidence_digest",
                    request.trained_range_evidence_digest);
  detail::append_kv(out, "validation_split_policy_digest",
                    request.validation_split_policy_digest);
  detail::append_kv(out, "base_reserve_node_id", request.base_reserve_node_id);
  detail::append_kv(out, "risky_node_ids", detail::csv(request.risky_node_ids));
  detail::append_bool(out, "include_base_reserve_policy",
                      request.include_base_reserve_policy);
  detail::append_bool(out, "include_equal_weight_policy",
                      request.include_equal_weight_policy);
  detail::append_bool(out, "include_current_weight_policy",
                      request.include_current_weight_policy);
  detail::append_bool(out, "include_spot_distributional_utility_policy",
                      request.include_spot_distributional_utility_policy);
  detail::append_i64(out, "max_replay_steps", request.max_replay_steps);
  detail::append_i64(out, "max_parallel_replay_jobs",
                     request.max_parallel_replay_jobs);
  detail::append_i64(out, "runtime_timeout_seconds",
                     request.runtime_timeout_seconds);
  detail::append_bool(out, "force_rebuild_cache", request.force_rebuild_cache);
  detail::append_bool(out, "require_existing_paths",
                      request.require_existing_paths);
  detail::append_bool(out, "require_non_overlapping_ranges",
                      request.require_non_overlapping_ranges);
  detail::append_kv(out, "non_authority_statement",
                    k_marshal_evaluation_non_authority_statement);
  return out.str();
}

[[nodiscard]] inline std::string
evaluation_request_digest(const marshal_evaluation_request_t &request) {
  return marshal_digest_for_text("kikijyeba.marshal.evaluation_request.v1",
                                 canonical_evaluation_request_text(request));
}

[[nodiscard]] inline std::vector<std::string>
validate_evaluation_request(const marshal_evaluation_request_t &request) {
  namespace detail = evaluation_marshal_detail;
  std::vector<std::string> refusals;
  if (request.evaluation_id.empty()) {
    refusals.emplace_back("missing_evaluation_id");
  }
  if (request.target_id.empty()) {
    refusals.emplace_back("missing_target_id");
  }
  if (request.protocol_id.empty()) {
    refusals.emplace_back("missing_protocol_id");
  }
  if (request.target_component_family_id !=
      "wikimyei.inference.expected_value.mdn") {
    refusals.emplace_back("unsupported_target_component_family_id");
  }
  if (request.wave_id.empty()) {
    refusals.emplace_back("missing_wave_id");
  }
  if (request.wave_mode.empty()) {
    refusals.emplace_back("missing_wave_mode");
  }
  if (request.source_order.empty()) {
    refusals.emplace_back("missing_source_order");
  }
  if (!detail::valid_range(request.trained_range)) {
    refusals.emplace_back("invalid_trained_range");
  }
  if (!detail::valid_range(request.validation_range)) {
    refusals.emplace_back("invalid_validation_range");
  }
  if (request.require_non_overlapping_ranges &&
      detail::valid_range(request.trained_range) &&
      detail::valid_range(request.validation_range) &&
      detail::ranges_overlap(request.trained_range, request.validation_range)) {
    refusals.emplace_back("trained_validation_range_overlap");
  }
  if (request.trained_range_evidence_digest.empty()) {
    refusals.emplace_back("missing_trained_range_evidence_digest");
  } else if (!marshal_digest_is_strong_hex(
                 request.trained_range_evidence_digest)) {
    refusals.emplace_back("malformed_trained_range_evidence_digest");
  }
  if (request.validation_split_policy_digest.empty()) {
    refusals.emplace_back("missing_validation_split_policy_digest");
  } else if (!marshal_digest_is_strong_hex(
                 request.validation_split_policy_digest)) {
    refusals.emplace_back("malformed_validation_split_policy_digest");
  }
  if (request.base_reserve_node_id.empty()) {
    refusals.emplace_back("missing_base_reserve_node_id");
  }
  if (request.risky_node_ids.empty()) {
    refusals.emplace_back("missing_risky_node_ids");
  }
  std::unordered_set<std::string> seen_nodes;
  for (const auto &node_id : request.risky_node_ids) {
    if (node_id.empty()) {
      refusals.emplace_back("empty_risky_node_id");
      continue;
    }
    if (node_id == request.base_reserve_node_id) {
      refusals.emplace_back("base_reserve_node_duplicated_in_risky_nodes");
    }
    if (!seen_nodes.insert(node_id).second) {
      refusals.emplace_back("duplicate_risky_node_id");
    }
  }
  if (request.max_replay_steps < 0) {
    refusals.emplace_back("invalid_max_replay_steps");
  }
  if (request.max_parallel_replay_jobs < 0) {
    refusals.emplace_back("invalid_max_parallel_replay_jobs");
  }
  if (request.runtime_timeout_seconds <= 0) {
    refusals.emplace_back("invalid_runtime_timeout_seconds");
  }
  if (request.require_existing_paths) {
    if (request.runtime_exec_path.empty() ||
        !std::filesystem::exists(request.runtime_exec_path)) {
      refusals.emplace_back("runtime_exec_path_missing");
    }
    if (request.evaluation_config_path.empty() ||
        !std::filesystem::exists(request.evaluation_config_path)) {
      refusals.emplace_back("evaluation_config_path_missing");
    }
    if (request.input_mdn_checkpoint.empty() ||
        !std::filesystem::exists(request.input_mdn_checkpoint)) {
      refusals.emplace_back("input_mdn_checkpoint_missing");
    }
    if (request.input_representation_checkpoint.empty() ||
        !std::filesystem::exists(request.input_representation_checkpoint)) {
      refusals.emplace_back("input_representation_checkpoint_missing");
    }
  }
  return refusals;
}

[[nodiscard]] inline std::string
canonical_evaluation_plan_text(const marshal_evaluation_plan_t &plan) {
  namespace detail = evaluation_marshal_detail;
  std::ostringstream out;
  detail::append_kv(out, "schema_version", plan.schema_version);
  detail::append_kv(out, "request_digest", plan.request_digest);
  detail::append_kv(out, "evaluation_id", plan.evaluation_id);
  detail::append_bool(out, "accepted", plan.accepted);
  detail::append_bool(out, "target_satisfaction_claimed",
                      plan.target_satisfaction_claimed);
  detail::append_bool(out, "runtime_executor", plan.runtime_executor);
  detail::append_bool(out, "lattice_proof_authority",
                      plan.lattice_proof_authority);
  detail::append_bool(out, "validation_range_out_of_sample",
                      plan.validation_range_out_of_sample);
  detail::append_kv(out, "runtime_run_command", plan.runtime_run_command);
  detail::append_kv(out, "runtime_replay_command_template",
                    plan.runtime_replay_command_template);
  detail::append_kv(out, "refusal_reasons", detail::csv(plan.refusal_reasons));
  detail::append_kv(out, "non_authority_statement",
                    k_marshal_evaluation_non_authority_statement);
  return out.str();
}

[[nodiscard]] inline std::string
evaluation_plan_digest(const marshal_evaluation_plan_t &plan) {
  return marshal_digest_for_text("kikijyeba.marshal.evaluation_plan.v1",
                                 canonical_evaluation_plan_text(plan));
}

[[nodiscard]] inline marshal_evaluation_plan_t
prepare_evaluation_plan(const marshal_evaluation_request_t &request) {
  namespace detail = evaluation_marshal_detail;
  marshal_evaluation_plan_t plan{};
  plan.request_digest = evaluation_request_digest(request);
  plan.evaluation_id = request.evaluation_id;
  plan.refusal_reasons = validate_evaluation_request(request);
  plan.accepted = plan.refusal_reasons.empty();
  plan.validation_range_out_of_sample = plan.accepted;

  if (plan.accepted) {
    std::ostringstream run;
    run << detail::shell_quote(request.runtime_exec_path.string())
        << " --config "
        << detail::shell_quote(request.evaluation_config_path.string())
        << " --source-range anchor_index"
        << " --anchor-index-begin "
        << request.validation_range.anchor_index_begin << " --anchor-index-end "
        << request.validation_range.anchor_index_end;
    if (!request.base_reserve_node_id.empty()) {
      run << " --replay-base-reserve-node "
          << detail::shell_quote(request.base_reserve_node_id);
    }
    if (!request.risky_node_ids.empty()) {
      run << " --replay-risky-nodes "
          << detail::shell_quote(detail::csv(request.risky_node_ids));
    }
    plan.runtime_run_command = run.str();

    std::ostringstream replay;
    replay << detail::shell_quote(request.runtime_exec_path.string())
           << " --config "
           << detail::shell_quote(request.evaluation_config_path.string())
           << " --replay-from-job-dir <completed_runtime_job_dir>";
    if (!request.evaluation_id.empty()) {
      replay << " --replay-experiment-id "
             << detail::shell_quote(request.evaluation_id);
    }
    if (request.max_replay_steps > 0) {
      replay << " --replay-max-steps " << request.max_replay_steps;
    }
    if (request.max_parallel_replay_jobs > 0) {
      replay << " --replay-max-parallel-jobs "
             << request.max_parallel_replay_jobs;
    }
    if (request.include_equal_weight_policy) {
      replay << " --replay-include-equal-weight";
    }
    if (request.include_current_weight_policy) {
      replay << " --replay-include-current-weight";
    }
    if (!request.include_base_reserve_policy) {
      replay << " --replay-no-base-reserve-policy";
    }
    if (!request.include_spot_distributional_utility_policy) {
      replay << " --replay-no-sdu-policy";
    }
    plan.runtime_replay_command_template = replay.str();
  }

  plan.plan_digest = evaluation_plan_digest(plan);
  return plan;
}

[[nodiscard]] inline std::string canonical_evaluation_runtime_handoff_text(
    const marshal_evaluation_runtime_handoff_t &handoff) {
  namespace detail = evaluation_marshal_detail;
  std::ostringstream out;
  detail::append_kv(out, "schema_version", handoff.schema_version);
  detail::append_kv(out, "request_digest", handoff.request_digest);
  detail::append_kv(out, "plan_digest", handoff.plan_digest);
  detail::append_kv(out, "evaluation_id", handoff.evaluation_id);
  detail::append_bool(out, "accepted", handoff.accepted);
  detail::append_bool(out, "dry_run", handoff.dry_run);
  detail::append_bool(out, "confirm_execute", handoff.confirm_execute);
  detail::append_bool(out, "target_satisfaction_claimed",
                      handoff.target_satisfaction_claimed);
  detail::append_bool(out, "runtime_executor", handoff.runtime_executor);
  detail::append_bool(out, "lattice_proof_authority",
                      handoff.lattice_proof_authority);
  detail::append_kv(out, "runtime_tool_name", handoff.runtime_tool_name);
  detail::append_kv(out, "runtime_execute_args_json",
                    handoff.runtime_execute_args_json);
  detail::append_kv(out, "runtime_execute_args_digest",
                    handoff.runtime_execute_args_digest);
  detail::append_kv(out, "runtime_replay_command_template",
                    handoff.runtime_replay_command_template);
  detail::append_kv(out, "refusal_reasons",
                    detail::csv(handoff.refusal_reasons));
  detail::append_kv(out, "non_authority_statement",
                    k_marshal_evaluation_non_authority_statement);
  return out.str();
}

[[nodiscard]] inline std::string evaluation_runtime_handoff_digest(
    const marshal_evaluation_runtime_handoff_t &handoff) {
  return marshal_digest_for_text(
      "kikijyeba.marshal.evaluation_runtime_handoff.v1",
      canonical_evaluation_runtime_handoff_text(handoff));
}

[[nodiscard]] inline std::string evaluation_runtime_execute_args_json(
    const marshal_evaluation_request_t &request,
    const marshal_evaluation_plan_t &plan) {
  namespace detail = evaluation_marshal_detail;
  std::ostringstream args;
  args << "{";
  bool first = true;
  detail::append_json_string_field(
      args, "config_path", request.evaluation_config_path.string(), &first);
  detail::append_json_string_field(args, "target_id", request.target_id,
                                   &first);
  detail::append_json_bool_field(args, "dry_run", true, &first);
  detail::append_json_bool_field(args, "confirm_execute", false, &first);
  detail::append_json_i64_field(args, "timeout_seconds",
                                request.runtime_timeout_seconds, &first);
  detail::append_json_bool_field(args, "force_rebuild_cache",
                                 request.force_rebuild_cache, &first);

  if (!first) {
    args << ",";
  }
  first = false;
  args << detail::json_quote("wave_overlay") << ":{";
  bool overlay_first = true;
  detail::append_json_string_field(args, "source_range", "anchor_index",
                                   &overlay_first);
  detail::append_json_i64_field(args, "anchor_index_begin",
                                request.validation_range.anchor_index_begin,
                                &overlay_first);
  detail::append_json_i64_field(args, "anchor_index_end",
                                request.validation_range.anchor_index_end,
                                &overlay_first);
  args << "}";

  args << "," << detail::json_quote("marshal_expected_wave") << ":{";
  bool expected_first = true;
  detail::append_json_string_field(args, "wave_id", request.wave_id,
                                   &expected_first);
  detail::append_json_string_field(args, "target_component_family_id",
                                   request.target_component_family_id,
                                   &expected_first);
  detail::append_json_string_field(args, "mode", request.wave_mode,
                                   &expected_first);
  detail::append_json_string_field(args, "source_range", "anchor_index",
                                   &expected_first);
  detail::append_json_string_field(args, "source_order", request.source_order,
                                   &expected_first);
  detail::append_json_i64_field(args, "anchor_index_begin",
                                request.validation_range.anchor_index_begin,
                                &expected_first);
  detail::append_json_i64_field(args, "anchor_index_end",
                                request.validation_range.anchor_index_end,
                                &expected_first);
  args << "}";

  args << "," << detail::json_quote("runtime_handoff") << ":{";
  bool handoff_first = true;
  detail::append_json_string_field(
      args, "handoff_schema_version",
      k_marshal_evaluation_runtime_handoff_schema_v1, &handoff_first);
  detail::append_json_string_field(
      args, "handoff_id", "evaluation_runtime_handoff_" + plan.plan_digest,
      &handoff_first);
  detail::append_json_string_field(args, "request_digest", plan.request_digest,
                                   &handoff_first);
  detail::append_json_string_field(args, "plan_digest", plan.plan_digest,
                                   &handoff_first);
  detail::append_json_string_field(args, "evaluation_id", request.evaluation_id,
                                   &handoff_first);
  detail::append_json_string_field(args, "runtime_replay_command_template",
                                   plan.runtime_replay_command_template,
                                   &handoff_first);
  detail::append_json_bool_field(args, "target_satisfaction_claimed", false,
                                 &handoff_first);
  detail::append_json_bool_field(args, "runtime_executor", false,
                                 &handoff_first);
  detail::append_json_bool_field(args, "lattice_proof_authority", false,
                                 &handoff_first);
  detail::append_json_string_field(args, "non_authority_statement",
                                   k_marshal_evaluation_non_authority_statement,
                                   &handoff_first);
  args << "}";

  args << "}";
  return args.str();
}

[[nodiscard]] inline marshal_evaluation_runtime_handoff_t
prepare_evaluation_runtime_handoff(const marshal_evaluation_request_t &request,
                                   const marshal_evaluation_plan_t &plan) {
  marshal_evaluation_runtime_handoff_t handoff{};
  handoff.request_digest = plan.request_digest;
  handoff.plan_digest = plan.plan_digest;
  handoff.evaluation_id = request.evaluation_id;
  handoff.runtime_replay_command_template =
      plan.runtime_replay_command_template;

  if (!plan.accepted) {
    handoff.refusal_reasons = plan.refusal_reasons;
  } else {
    handoff.accepted = true;
    handoff.runtime_execute_args_json =
        evaluation_runtime_execute_args_json(request, plan);
    handoff.runtime_execute_args_digest = marshal_digest_for_text(
        "kikijyeba.marshal.evaluation_runtime_execute_args.v1",
        handoff.runtime_execute_args_json);
  }
  handoff.handoff_digest = evaluation_runtime_handoff_digest(handoff);
  return handoff;
}

[[nodiscard]] inline std::string
canonical_evaluation_receipt_text(const marshal_evaluation_receipt_t &receipt) {
  namespace detail = evaluation_marshal_detail;
  std::ostringstream out;
  detail::append_kv(out, "schema_version", receipt.schema_version);
  detail::append_kv(out, "request_digest", receipt.request_digest);
  detail::append_kv(out, "plan_digest", receipt.plan_digest);
  detail::append_kv(out, "evaluation_id", receipt.evaluation_id);
  detail::append_kv(out, "runtime_job_dir", receipt.runtime_job_dir.string());
  detail::append_kv(out, "replay_report_path",
                    receipt.replay_report_path.string());
  detail::append_kv(out, "top_level_metric_scope",
                    receipt.top_level_metric_scope);
  detail::append_kv(out, "policy_metric_scope", receipt.policy_metric_scope);
  detail::append_i64(out, "replay_bundle_count", receipt.replay_bundle_count);
  detail::append_i64(out, "attempted_count", receipt.attempted_count);
  detail::append_i64(out, "completed_count", receipt.completed_count);
  detail::append_i64(out, "failed_count", receipt.failed_count);
  detail::append_kv(out, "execution_backend_id", receipt.execution_backend_id);
  detail::append_i64(out, "cajtucu_execution_step_count",
                     receipt.cajtucu_execution_step_count);
  detail::append_i64(out, "cajtucu_rejected_fill_count",
                     receipt.cajtucu_rejected_fill_count);
  detail::append_i64(out, "cajtucu_partial_fill_count",
                     receipt.cajtucu_partial_fill_count);
  detail::append_double(out, "mean_projection_mae",
                        receipt.mean_projection_mae);
  detail::append_double(out, "mean_projection_rmse",
                        receipt.mean_projection_rmse);
  detail::append_double(out, "mean_projection_correlation",
                        receipt.mean_projection_correlation);
  detail::append_double(out, "mean_projection_directional_accuracy",
                        receipt.mean_projection_directional_accuracy);
  detail::append_double(out, "mean_projection_interval_coverage",
                        receipt.mean_projection_interval_coverage);
  detail::append_double(out, "mean_projection_interval_width",
                        receipt.mean_projection_interval_width);
  detail::append_double(out, "mean_zero_return_baseline_mae",
                        receipt.mean_zero_return_baseline_mae);
  detail::append_double(out, "mean_zero_return_baseline_rmse",
                        receipt.mean_zero_return_baseline_rmse);
  detail::append_double(out, "mean_model_skill_vs_zero_mae",
                        receipt.mean_model_skill_vs_zero_mae);
  detail::append_double(out, "mean_model_skill_vs_zero_rmse",
                        receipt.mean_model_skill_vs_zero_rmse);
  detail::append_double(out, "mean_total_log_growth",
                        receipt.mean_total_log_growth);
  detail::append_double(out, "mean_final_equity_base",
                        receipt.mean_final_equity_base);
  detail::append_double(out, "mean_total_transaction_cost_base",
                        receipt.mean_total_transaction_cost_base);
  detail::append_double(out, "mean_cajtucu_order_count",
                        receipt.mean_cajtucu_order_count);
  detail::append_double(out, "mean_cajtucu_fill_count",
                        receipt.mean_cajtucu_fill_count);
  detail::append_double(out, "mean_cajtucu_total_fee_base",
                        receipt.mean_cajtucu_total_fee_base);
  detail::append_double(out, "mean_cajtucu_total_spread_cost_base",
                        receipt.mean_cajtucu_total_spread_cost_base);
  detail::append_double(out, "mean_cajtucu_total_slippage_base",
                        receipt.mean_cajtucu_total_slippage_base);
  detail::append_double(out, "mean_cajtucu_total_transaction_cost_base",
                        receipt.mean_cajtucu_total_transaction_cost_base);
  detail::append_i64(out, "time_law_future_observation_violation_count",
                     receipt.time_law_future_observation_violation_count);
  detail::append_i64(out, "mixed_future_realization_key_count",
                     receipt.mixed_future_realization_key_count);
  detail::append_i64(out, "projection_validation_step_count",
                     receipt.projection_validation_step_count);
  detail::append_i64(out, "policy_summary_count", receipt.policy_summary_count);
  detail::append_kv(out, "best_policy_by_final_equity",
                    receipt.best_policy_by_final_equity);
  detail::append_kv(out, "best_policy_by_after_cost_return",
                    receipt.best_policy_by_after_cost_return);
  detail::append_kv(out, "best_policy_by_drawdown_adjusted_return",
                    receipt.best_policy_by_drawdown_adjusted_return);
  detail::append_i64(
      out, "embedded_policy_summary_count",
      static_cast<std::int64_t>(receipt.policy_summaries.size()));
  for (std::size_t i = 0; i < receipt.policy_summaries.size(); ++i) {
    detail::append_policy_summary(out,
                                  "embedded_policy_" + std::to_string(i) + "_",
                                  receipt.policy_summaries[i]);
  }
  detail::append_bool(out, "target_satisfaction_claimed",
                      receipt.target_satisfaction_claimed);
  detail::append_bool(out, "runtime_executor", receipt.runtime_executor);
  detail::append_bool(out, "lattice_proof_authority",
                      receipt.lattice_proof_authority);
  detail::append_kv(out, "non_authority_statement",
                    k_marshal_evaluation_non_authority_statement);
  return out.str();
}

[[nodiscard]] inline std::string
evaluation_receipt_digest(const marshal_evaluation_receipt_t &receipt) {
  return marshal_digest_for_text("kikijyeba.marshal.evaluation_receipt.v1",
                                 canonical_evaluation_receipt_text(receipt));
}

[[nodiscard]] inline marshal_evaluation_receipt_t
finalize_evaluation_receipt(marshal_evaluation_receipt_t receipt) {
  if (receipt.policy_summary_count == 0 && !receipt.policy_summaries.empty()) {
    receipt.policy_summary_count =
        static_cast<std::int64_t>(receipt.policy_summaries.size());
  }
  receipt.receipt_digest = evaluation_receipt_digest(receipt);
  return receipt;
}

} // namespace cuwacunu::kikijyeba::marshal
