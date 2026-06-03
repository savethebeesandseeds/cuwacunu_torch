// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "kikijyeba/marshal/digest.h"

namespace cuwacunu::kikijyeba::marshal {

inline constexpr const char *k_marshal_rollout_request_schema_v1 =
    "kikijyeba.marshal.rollout_request.v1";
inline constexpr const char *k_marshal_rollout_execution_profile_schema_v1 =
    "kikijyeba.marshal.rollout_execution_profile.v1";
inline constexpr const char *k_marshal_rollout_plan_schema_v1 =
    "kikijyeba.marshal.rollout_plan.v1";

inline constexpr const char *k_marshal_rollout_non_authority_statement =
    "Marshal rollout prepares bounded environment rollout plans. Runtime "
    "executes, Kikijyeba produces trajectory evidence, Cajtucu simulates paper "
    "execution, and Lattice proves readiness only from durable evidence.";

inline constexpr const char *k_rollout_policy_base_reserve =
    "base_reserve_only.v1";
inline constexpr const char *k_rollout_policy_current_weight =
    "current_weight_no_trade.v1";
inline constexpr const char *k_rollout_policy_equal_weight = "equal_weight.v1";
inline constexpr const char *k_rollout_policy_sdu =
    "kikijyeba.environment.policy.spot_distributional_utility.v1";

struct marshal_rollout_execution_profile_t {
  std::string schema_version{k_marshal_rollout_execution_profile_schema_v1};
  std::string execution_backend_id{"cajtucu.execution.paper.v1"};
  bool allow_synthetic_direct_edges{false};
  std::string synthetic_edge_research_reason{};
  double linear_transaction_cost_rate{0.0};
  bool allow_partial_fills{false};
  bool allow_negative_base_reserve{false};
  double equity_mismatch_tolerance{1.0e-6};
  double equity_mismatch_fail_tolerance{0.01};
  bool live_execution_allowed{false};
};

struct marshal_rollout_request_t {
  std::string schema_version{k_marshal_rollout_request_schema_v1};
  std::string rollout_id{};
  std::string experiment_id{};

  std::filesystem::path runtime_job_dir{};
  std::filesystem::path runtime_exec_path{
      "/cuwacunu/.build/exec/cuwacunu_exec"};
  std::filesystem::path report_path{};

  std::string environment_mode{"historical_replay"};
  std::string environment_assembly_id{"kikijyeba.environment.replay.v1"};
  std::string base_reserve_node_id{};
  std::vector<std::string> risky_node_ids{};
  std::vector<std::string> policy_tokens{};

  std::int64_t max_steps{0};
  std::int64_t max_parallel_jobs{1};
  marshal_rollout_execution_profile_t execution_profile{};

  bool require_existing_runtime_job_dir{true};
  bool require_completed_runtime_job{true};
  bool require_replay_artifacts{true};
  bool prepare_only{true};
};

struct marshal_rollout_plan_t {
  std::string schema_version{k_marshal_rollout_plan_schema_v1};
  std::string request_digest{};
  std::string plan_digest{};
  std::string rollout_id{};
  std::string experiment_id{};
  bool accepted{false};
  std::vector<std::string> refusal_reasons{};

  std::filesystem::path runtime_job_dir{};
  std::filesystem::path expected_report_path{};
  std::string environment_assembly_id{};
  std::string execution_backend_id{};
  std::vector<std::string> requested_policy_tokens{};
  std::vector<std::string> resolved_policy_ids{};
  marshal_rollout_execution_profile_t execution_profile{};
  std::string replay_command_template{};

  bool target_satisfaction_claimed{false};
  bool runtime_executor{false};
  bool lattice_proof_authority{false};
  bool policy_training_authority{false};
  bool live_execution_authority{false};
  std::string non_authority_statement{
      k_marshal_rollout_non_authority_statement};
};

namespace rollout_marshal_detail {

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

[[nodiscard]] inline std::string trim_ascii(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() &&
         std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1U])) != 0) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

[[nodiscard]] inline std::map<std::string, std::string>
read_kv_file(const std::filesystem::path &path) {
  std::map<std::string, std::string> out;
  std::ifstream input(path);
  if (!input.is_open()) {
    return out;
  }
  std::string line;
  while (std::getline(input, line)) {
    const auto pos = line.find('=');
    if (pos == std::string::npos) {
      continue;
    }
    out[trim_ascii(std::string_view(line).substr(0, pos))] =
        trim_ascii(std::string_view(line).substr(pos + 1U));
  }
  return out;
}

[[nodiscard]] inline bool kv_true(const std::map<std::string, std::string> &kv,
                                  const std::string &key) {
  const auto found = kv.find(key);
  return found != kv.end() && found->second == "true";
}

[[nodiscard]] inline std::string
kv_string(const std::map<std::string, std::string> &kv,
          const std::string &key) {
  const auto found = kv.find(key);
  return found == kv.end() ? std::string{} : found->second;
}

[[nodiscard]] inline std::string
default_report_path(const marshal_rollout_request_t &request) {
  if (!request.report_path.empty()) {
    return request.report_path.string();
  }
  return (request.runtime_job_dir / "artifacts" /
          (request.rollout_id + ".replay.report"))
      .string();
}

[[nodiscard]] inline std::vector<std::string> default_policy_tokens() {
  return {"base_reserve", "current_weight", "equal_weight", "sdu"};
}

[[nodiscard]] inline std::string
normalize_policy_token(const std::string &token) {
  if (token == "reserve" || token == "base_reserve" ||
      token == k_rollout_policy_base_reserve) {
    return k_rollout_policy_base_reserve;
  }
  if (token == "current" || token == "current_weight" ||
      token == k_rollout_policy_current_weight) {
    return k_rollout_policy_current_weight;
  }
  if (token == "equal" || token == "equal_weight" ||
      token == k_rollout_policy_equal_weight) {
    return k_rollout_policy_equal_weight;
  }
  if (token == "sdu" || token == "spot_distributional_utility" ||
      token == k_rollout_policy_sdu) {
    return k_rollout_policy_sdu;
  }
  return {};
}

[[nodiscard]] inline std::vector<std::string>
resolve_policy_ids(const std::vector<std::string> &policy_tokens) {
  std::vector<std::string> out;
  std::unordered_set<std::string> seen;
  const auto tokens =
      policy_tokens.empty() ? default_policy_tokens() : policy_tokens;
  for (const auto &token : tokens) {
    const auto resolved = normalize_policy_token(token);
    if (!resolved.empty() && seen.insert(resolved).second) {
      out.push_back(resolved);
    }
  }
  return out;
}

[[nodiscard]] inline bool contains_policy(const std::vector<std::string> &ids,
                                          std::string_view id) {
  return std::find(ids.begin(), ids.end(), id) != ids.end();
}

inline void append_json_string_array(std::ostringstream &out,
                                     const std::vector<std::string> &values) {
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(values[i]);
  }
  out << "]";
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

inline void append_json_double_field(std::ostringstream &out,
                                     const std::string &key, double value,
                                     bool *first) {
  if (!*first) {
    out << ",";
  }
  *first = false;
  out << json_quote(key) << ":";
  out << std::setprecision(17) << value;
}

} // namespace rollout_marshal_detail

[[nodiscard]] inline std::string canonical_rollout_execution_profile_text(
    const marshal_rollout_execution_profile_t &profile) {
  namespace detail = rollout_marshal_detail;
  std::ostringstream out;
  detail::append_kv(out, "schema_version", profile.schema_version);
  detail::append_kv(out, "execution_backend_id", profile.execution_backend_id);
  detail::append_bool(out, "allow_synthetic_direct_edges",
                      profile.allow_synthetic_direct_edges);
  detail::append_kv(out, "synthetic_edge_research_reason",
                    profile.synthetic_edge_research_reason);
  detail::append_double(out, "linear_transaction_cost_rate",
                        profile.linear_transaction_cost_rate);
  detail::append_bool(out, "allow_partial_fills", profile.allow_partial_fills);
  detail::append_bool(out, "allow_negative_base_reserve",
                      profile.allow_negative_base_reserve);
  detail::append_double(out, "equity_mismatch_tolerance",
                        profile.equity_mismatch_tolerance);
  detail::append_double(out, "equity_mismatch_fail_tolerance",
                        profile.equity_mismatch_fail_tolerance);
  detail::append_bool(out, "live_execution_allowed",
                      profile.live_execution_allowed);
  return out.str();
}

[[nodiscard]] inline std::string
canonical_rollout_request_text(const marshal_rollout_request_t &request) {
  namespace detail = rollout_marshal_detail;
  std::ostringstream out;
  detail::append_kv(out, "schema_version", request.schema_version);
  detail::append_kv(out, "rollout_id", request.rollout_id);
  detail::append_kv(out, "experiment_id", request.experiment_id);
  detail::append_kv(out, "runtime_job_dir", request.runtime_job_dir.string());
  detail::append_kv(out, "runtime_exec_path",
                    request.runtime_exec_path.string());
  detail::append_kv(out, "report_path", request.report_path.string());
  detail::append_kv(out, "environment_mode", request.environment_mode);
  detail::append_kv(out, "environment_assembly_id",
                    request.environment_assembly_id);
  detail::append_kv(out, "base_reserve_node_id", request.base_reserve_node_id);
  detail::append_kv(out, "risky_node_ids", detail::csv(request.risky_node_ids));
  detail::append_kv(out, "policy_tokens", detail::csv(request.policy_tokens));
  detail::append_i64(out, "max_steps", request.max_steps);
  detail::append_i64(out, "max_parallel_jobs", request.max_parallel_jobs);
  detail::append_kv(
      out, "execution_profile",
      canonical_rollout_execution_profile_text(request.execution_profile));
  detail::append_bool(out, "require_existing_runtime_job_dir",
                      request.require_existing_runtime_job_dir);
  detail::append_bool(out, "require_completed_runtime_job",
                      request.require_completed_runtime_job);
  detail::append_bool(out, "require_replay_artifacts",
                      request.require_replay_artifacts);
  detail::append_bool(out, "prepare_only", request.prepare_only);
  detail::append_kv(out, "non_authority_statement",
                    k_marshal_rollout_non_authority_statement);
  return out.str();
}

[[nodiscard]] inline std::string
rollout_request_digest(const marshal_rollout_request_t &request) {
  return marshal_digest_for_text(k_marshal_rollout_request_schema_v1,
                                 canonical_rollout_request_text(request));
}

[[nodiscard]] inline std::vector<std::string>
validate_rollout_request(const marshal_rollout_request_t &request) {
  namespace detail = rollout_marshal_detail;
  std::vector<std::string> refusals;

  if (request.rollout_id.empty()) {
    refusals.emplace_back("missing_rollout_id");
  }
  if (request.runtime_job_dir.empty()) {
    refusals.emplace_back("missing_runtime_job_dir");
  }
  if (request.require_existing_runtime_job_dir) {
    if (request.runtime_job_dir.empty() ||
        !std::filesystem::exists(request.runtime_job_dir)) {
      refusals.emplace_back("runtime_job_dir_missing");
    } else if (!std::filesystem::is_directory(request.runtime_job_dir)) {
      refusals.emplace_back("runtime_job_dir_not_directory");
    }
  }
  if (!request.runtime_exec_path.empty() &&
      !std::filesystem::exists(request.runtime_exec_path)) {
    refusals.emplace_back("runtime_exec_path_missing");
  }
  if (request.environment_mode != "historical_replay") {
    refusals.emplace_back("unsupported_environment_mode");
  }
  if (request.environment_assembly_id != "kikijyeba.environment.replay.v1") {
    refusals.emplace_back("unsupported_environment_assembly_id");
  }
  if (request.execution_profile.execution_backend_id !=
      "cajtucu.execution.paper.v1") {
    refusals.emplace_back("unsupported_execution_backend_id");
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

  const auto tokens = request.policy_tokens.empty()
                          ? detail::default_policy_tokens()
                          : request.policy_tokens;
  for (const auto &token : tokens) {
    if (detail::normalize_policy_token(token).empty()) {
      refusals.emplace_back("unsupported_policy_token:" + token);
    }
  }
  if (request.max_steps < 0) {
    refusals.emplace_back("invalid_max_steps");
  }
  if (request.max_parallel_jobs <= 0) {
    refusals.emplace_back("invalid_max_parallel_jobs");
  }

  const auto &profile = request.execution_profile;
  if (!std::isfinite(profile.linear_transaction_cost_rate) ||
      profile.linear_transaction_cost_rate < 0.0) {
    refusals.emplace_back("invalid_linear_transaction_cost_rate");
  }
  if (!std::isfinite(profile.equity_mismatch_tolerance) ||
      profile.equity_mismatch_tolerance < 0.0) {
    refusals.emplace_back("invalid_equity_mismatch_tolerance");
  }
  if (!std::isfinite(profile.equity_mismatch_fail_tolerance) ||
      profile.equity_mismatch_fail_tolerance < 0.0) {
    refusals.emplace_back("invalid_equity_mismatch_fail_tolerance");
  }
  if (profile.equity_mismatch_fail_tolerance <
      profile.equity_mismatch_tolerance) {
    refusals.emplace_back("equity_mismatch_fail_tolerance_below_warn");
  }
  if (profile.allow_synthetic_direct_edges &&
      profile.synthetic_edge_research_reason.empty()) {
    refusals.emplace_back("missing_synthetic_edge_research_reason");
  }
  if (profile.allow_negative_base_reserve) {
    refusals.emplace_back("negative_base_reserve_forbidden_v1");
  }
  if (profile.live_execution_allowed) {
    refusals.emplace_back("live_execution_forbidden_v1");
  }

  const auto job_state_path = request.runtime_job_dir / "job.state";
  if (!request.runtime_job_dir.empty() &&
      std::filesystem::exists(job_state_path)) {
    const auto job_state = detail::read_kv_file(job_state_path);
    if (request.require_completed_runtime_job) {
      const auto status = detail::kv_string(job_state, "status");
      const auto terminal_status =
          detail::kv_string(job_state, "terminal_status");
      if (!status.empty() && status != "completed" && status != "success") {
        refusals.emplace_back("runtime_job_not_completed");
      }
      if (status.empty() && !terminal_status.empty() &&
          terminal_status != "completed" && terminal_status != "success") {
        refusals.emplace_back("runtime_job_not_completed");
      }
    }
    if (request.require_replay_artifacts) {
      const bool replay_written =
          detail::kv_true(job_state, "replay_artifacts_written");
      const auto batch_index =
          detail::kv_string(job_state, "replay_batch_index_path");
      if (!replay_written) {
        refusals.emplace_back("replay_artifacts_not_written");
      }
      if (batch_index.empty() || !std::filesystem::exists(batch_index)) {
        refusals.emplace_back("replay_batch_index_missing");
      }
    }
  } else if (request.require_completed_runtime_job ||
             request.require_replay_artifacts) {
    refusals.emplace_back("runtime_job_state_missing");
  }

  return refusals;
}

[[nodiscard]] inline std::string
canonical_rollout_plan_text(const marshal_rollout_plan_t &plan) {
  namespace detail = rollout_marshal_detail;
  std::ostringstream out;
  detail::append_kv(out, "schema_version", plan.schema_version);
  detail::append_kv(out, "request_digest", plan.request_digest);
  detail::append_kv(out, "rollout_id", plan.rollout_id);
  detail::append_kv(out, "experiment_id", plan.experiment_id);
  detail::append_bool(out, "accepted", plan.accepted);
  detail::append_kv(out, "runtime_job_dir", plan.runtime_job_dir.string());
  detail::append_kv(out, "expected_report_path",
                    plan.expected_report_path.string());
  detail::append_kv(out, "environment_assembly_id",
                    plan.environment_assembly_id);
  detail::append_kv(out, "execution_backend_id", plan.execution_backend_id);
  detail::append_kv(out, "requested_policy_tokens",
                    detail::csv(plan.requested_policy_tokens));
  detail::append_kv(out, "resolved_policy_ids",
                    detail::csv(plan.resolved_policy_ids));
  detail::append_kv(
      out, "execution_profile",
      canonical_rollout_execution_profile_text(plan.execution_profile));
  detail::append_kv(out, "replay_command_template",
                    plan.replay_command_template);
  detail::append_kv(out, "refusal_reasons", detail::csv(plan.refusal_reasons));
  detail::append_bool(out, "target_satisfaction_claimed",
                      plan.target_satisfaction_claimed);
  detail::append_bool(out, "runtime_executor", plan.runtime_executor);
  detail::append_bool(out, "lattice_proof_authority",
                      plan.lattice_proof_authority);
  detail::append_bool(out, "policy_training_authority",
                      plan.policy_training_authority);
  detail::append_bool(out, "live_execution_authority",
                      plan.live_execution_authority);
  detail::append_kv(out, "non_authority_statement",
                    k_marshal_rollout_non_authority_statement);
  return out.str();
}

[[nodiscard]] inline std::string
rollout_plan_digest(const marshal_rollout_plan_t &plan) {
  return marshal_digest_for_text(k_marshal_rollout_plan_schema_v1,
                                 canonical_rollout_plan_text(plan));
}

[[nodiscard]] inline marshal_rollout_plan_t
prepare_rollout_plan(const marshal_rollout_request_t &request) {
  namespace detail = rollout_marshal_detail;
  marshal_rollout_plan_t plan{};
  plan.request_digest = rollout_request_digest(request);
  plan.rollout_id = request.rollout_id;
  plan.experiment_id = request.experiment_id.empty() ? request.rollout_id
                                                     : request.experiment_id;
  plan.runtime_job_dir = request.runtime_job_dir;
  plan.expected_report_path = detail::default_report_path(request);
  plan.environment_assembly_id = request.environment_assembly_id;
  plan.execution_backend_id = request.execution_profile.execution_backend_id;
  plan.requested_policy_tokens = request.policy_tokens.empty()
                                     ? detail::default_policy_tokens()
                                     : request.policy_tokens;
  plan.resolved_policy_ids = detail::resolve_policy_ids(request.policy_tokens);
  plan.execution_profile = request.execution_profile;
  plan.refusal_reasons = validate_rollout_request(request);
  plan.accepted = plan.refusal_reasons.empty();

  if (plan.accepted) {
    std::ostringstream command;
    command << detail::shell_quote(request.runtime_exec_path.string())
            << " --replay-from-job-dir "
            << detail::shell_quote(request.runtime_job_dir.string())
            << " --replay-experiment-id "
            << detail::shell_quote(plan.experiment_id)
            << " --replay-report-path "
            << detail::shell_quote(plan.expected_report_path.string())
            << " --replay-base-reserve-node "
            << detail::shell_quote(request.base_reserve_node_id)
            << " --replay-risky-nodes "
            << detail::shell_quote(detail::csv(request.risky_node_ids));
    if (request.max_steps > 0) {
      command << " --replay-max-steps " << request.max_steps;
    }
    if (request.max_parallel_jobs > 0) {
      command << " --replay-max-parallel-jobs " << request.max_parallel_jobs;
    }
    if (detail::contains_policy(plan.resolved_policy_ids,
                                k_rollout_policy_equal_weight)) {
      command << " --replay-include-equal-weight";
    }
    if (detail::contains_policy(plan.resolved_policy_ids,
                                k_rollout_policy_current_weight)) {
      command << " --replay-include-current-weight";
    }
    if (!detail::contains_policy(plan.resolved_policy_ids,
                                 k_rollout_policy_base_reserve)) {
      command << " --replay-no-base-reserve-policy";
    }
    if (!detail::contains_policy(plan.resolved_policy_ids,
                                 k_rollout_policy_sdu)) {
      command << " --replay-no-sdu-policy";
    }
    if (request.execution_profile.allow_synthetic_direct_edges) {
      command << " --replay-allow-synthetic-direct-edges";
    }
    if (request.execution_profile.linear_transaction_cost_rate > 0.0) {
      command << " --replay-linear-transaction-cost-rate "
              << std::setprecision(17)
              << request.execution_profile.linear_transaction_cost_rate;
    }
    plan.replay_command_template = command.str();
  }

  plan.plan_digest = rollout_plan_digest(plan);
  return plan;
}

[[nodiscard]] inline std::string rollout_execution_profile_json(
    const marshal_rollout_execution_profile_t &profile) {
  namespace detail = rollout_marshal_detail;
  std::ostringstream out;
  out << "{";
  bool first = true;
  detail::append_json_string_field(out, "schema_version",
                                   profile.schema_version, &first);
  detail::append_json_string_field(out, "execution_backend_id",
                                   profile.execution_backend_id, &first);
  detail::append_json_bool_field(out, "allow_synthetic_direct_edges",
                                 profile.allow_synthetic_direct_edges, &first);
  detail::append_json_string_field(out, "synthetic_edge_research_reason",
                                   profile.synthetic_edge_research_reason,
                                   &first);
  detail::append_json_double_field(out, "linear_transaction_cost_rate",
                                   profile.linear_transaction_cost_rate,
                                   &first);
  detail::append_json_bool_field(out, "allow_partial_fills",
                                 profile.allow_partial_fills, &first);
  detail::append_json_bool_field(out, "allow_negative_base_reserve",
                                 profile.allow_negative_base_reserve, &first);
  detail::append_json_double_field(out, "equity_mismatch_tolerance",
                                   profile.equity_mismatch_tolerance, &first);
  detail::append_json_double_field(out, "equity_mismatch_fail_tolerance",
                                   profile.equity_mismatch_fail_tolerance,
                                   &first);
  detail::append_json_bool_field(out, "live_execution_allowed",
                                 profile.live_execution_allowed, &first);
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::string
rollout_plan_json(const marshal_rollout_plan_t &plan) {
  namespace detail = rollout_marshal_detail;
  std::ostringstream out;
  out << "{";
  bool first = true;
  detail::append_json_string_field(out, "schema_version", plan.schema_version,
                                   &first);
  detail::append_json_string_field(out, "request_digest", plan.request_digest,
                                   &first);
  detail::append_json_string_field(out, "plan_digest", plan.plan_digest,
                                   &first);
  detail::append_json_string_field(out, "rollout_id", plan.rollout_id, &first);
  detail::append_json_string_field(out, "experiment_id", plan.experiment_id,
                                   &first);
  detail::append_json_bool_field(out, "accepted", plan.accepted, &first);

  if (!first) {
    out << ",";
  }
  first = false;
  out << detail::json_quote("refusal_reasons") << ":";
  detail::append_json_string_array(out, plan.refusal_reasons);

  detail::append_json_string_field(out, "runtime_job_dir",
                                   plan.runtime_job_dir.string(), &first);
  detail::append_json_string_field(out, "expected_report_path",
                                   plan.expected_report_path.string(), &first);
  detail::append_json_string_field(out, "environment_assembly_id",
                                   plan.environment_assembly_id, &first);
  detail::append_json_string_field(out, "execution_backend_id",
                                   plan.execution_backend_id, &first);

  out << "," << detail::json_quote("requested_policy_tokens") << ":";
  detail::append_json_string_array(out, plan.requested_policy_tokens);
  out << "," << detail::json_quote("resolved_policy_ids") << ":";
  detail::append_json_string_array(out, plan.resolved_policy_ids);

  out << "," << detail::json_quote("execution_profile") << ":"
      << rollout_execution_profile_json(plan.execution_profile);
  detail::append_json_string_field(out, "replay_command_template",
                                   plan.replay_command_template, &first);
  detail::append_json_bool_field(out, "target_satisfaction_claimed",
                                 plan.target_satisfaction_claimed, &first);
  detail::append_json_bool_field(out, "runtime_executor", plan.runtime_executor,
                                 &first);
  detail::append_json_bool_field(out, "lattice_proof_authority",
                                 plan.lattice_proof_authority, &first);
  detail::append_json_bool_field(out, "policy_training_authority",
                                 plan.policy_training_authority, &first);
  detail::append_json_bool_field(out, "live_execution_authority",
                                 plan.live_execution_authority, &first);
  detail::append_json_string_field(out, "non_authority_statement",
                                   plan.non_authority_statement, &first);
  out << "}";
  return out.str();
}

} // namespace cuwacunu::kikijyeba::marshal
