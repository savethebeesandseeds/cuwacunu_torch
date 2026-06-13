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

#include "hero/marshal_hero/marshal/digest.h"

namespace cuwacunu::hero::marshal {

inline constexpr const char *k_marshal_rollout_request_schema_v1 =
    "kikijyeba.marshal.rollout_request.v1";
inline constexpr const char *k_marshal_rollout_execution_profile_schema_v1 =
    "kikijyeba.marshal.rollout_execution_profile.v1";
inline constexpr const char *k_marshal_rollout_plan_schema_v1 =
    "kikijyeba.marshal.rollout_plan.v1";
inline constexpr const char *k_marshal_rollout_asset_universe_schema_v1 =
    "kikijyeba.marshal.rollout_asset_universe.v1";

inline constexpr const char *k_marshal_rollout_non_authority_statement =
    "Marshal rollout prepares bounded environment rollout plans. Runtime "
    "executes, Kikijyeba produces trajectory evidence, Cajtucu simulates paper "
    "execution, and Lattice proves readiness only from durable evidence.";

inline constexpr const char *k_rollout_policy_numeraire_only =
    "numeraire_only.v1";
inline constexpr const char *k_rollout_policy_current_weight =
    "current_weight_no_trade.v1";
inline constexpr const char *k_rollout_policy_equal_weight = "equal_weight.v1";
inline constexpr const char *k_rollout_policy_sdu =
    "kikijyeba.environment.policy.spot_distributional_utility.v1";

struct marshal_rollout_execution_profile_t {
  std::string schema_version{k_marshal_rollout_execution_profile_schema_v1};
  std::string execution_backend_id{"cajtucu.execution.paper.v1"};
  std::string cost_model_id{"linear_transaction_cost_rate.v1"};
  bool allow_synthetic_direct_edges{false};
  std::string synthetic_edge_research_reason{};
  double linear_transaction_cost_rate{0.0};
  bool allow_partial_fills{false};
  double equity_mismatch_tolerance{1.0e-6};
  double equity_mismatch_fail_tolerance{0.01};
  bool live_execution_allowed{false};
};

struct marshal_rollout_request_t {
  std::string schema_version{k_marshal_rollout_request_schema_v1};
  std::string rollout_id{};
  std::string rollout_attempt_id{};
  std::string idempotency_key{};
  std::string experiment_id{};

  std::filesystem::path config_path{"/cuwacunu/src/config/.config"};
  std::filesystem::path runtime_job_dir{};
  std::filesystem::path replay_batch_index_path{};
  std::filesystem::path runtime_exec_path{
      "/cuwacunu/.build/exec/cuwacunu_exec"};
  std::filesystem::path report_path{};
  std::string requested_mode{"plan"};

  std::string environment_mode{"historical_replay"};
  std::string environment_assembly_id{"kikijyeba.environment.replay.v1"};
  std::string graph_order_fingerprint{};
  std::string asset_universe_digest{};
  std::string accounting_numeraire_node_id{};
  std::vector<std::string> target_node_ids{};
  std::vector<std::string> policy_tokens{};

  std::int64_t max_steps{0};
  std::int64_t max_parallel_jobs{1};
  std::int64_t timeout_seconds{600};
  marshal_rollout_execution_profile_t execution_profile{};

  bool require_existing_runtime_job_dir{true};
  bool require_completed_runtime_job{true};
  bool require_replay_artifacts{true};
};

struct marshal_rollout_plan_t {
  std::string schema_version{k_marshal_rollout_plan_schema_v1};
  std::string request_digest{};
  std::string plan_digest{};
  std::string rollout_id{};
  std::string rollout_attempt_id{};
  std::string idempotency_key{};
  std::string experiment_id{};
  bool accepted{false};
  std::vector<std::string> refusal_reasons{};

  std::filesystem::path runtime_job_dir{};
  std::filesystem::path replay_batch_index_path{};
  std::filesystem::path expected_report_path{};
  std::string requested_mode{"plan"};
  std::string environment_assembly_id{};
  std::string graph_order_fingerprint{};
  std::string asset_universe_digest{};
  std::string execution_backend_id{};
  std::string execution_profile_digest{};
  std::string policy_set_digest{};
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

[[nodiscard]] inline std::string read_accounting_numeraire_node_id_from_config(
    const std::filesystem::path &config_path) {
  std::ifstream input(config_path);
  if (!input.is_open()) {
    return {};
  }
  std::string current_section;
  std::string line;
  while (std::getline(input, line)) {
    const auto comment = line.find('#');
    if (comment != std::string::npos) {
      line.resize(comment);
    }
    line = trim_ascii(line);
    if (line.empty()) {
      continue;
    }
    if (line.front() == '[' && line.back() == ']') {
      current_section =
          trim_ascii(std::string_view(line).substr(1U, line.size() - 2U));
      continue;
    }
    if (current_section != "ACCOUNTING") {
      continue;
    }
    const auto pos = line.find('=');
    if (pos == std::string::npos) {
      continue;
    }
    const auto key = trim_ascii(std::string_view(line).substr(0, pos));
    if (key == "accounting_numeraire_node_id") {
      return trim_ascii(std::string_view(line).substr(pos + 1U));
    }
  }
  return {};
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

[[nodiscard]] inline std::string
normalize_policy_token(const std::string &token) {
  if (token == "numeraire" || token == "numeraire_only" ||
      token == k_rollout_policy_numeraire_only) {
    return k_rollout_policy_numeraire_only;
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
  for (const auto &token : policy_tokens) {
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

[[nodiscard]] inline std::string
rollout_asset_universe_text(const std::string &accounting_numeraire_node_id,
                            const std::vector<std::string> &target_node_ids) {
  namespace detail = rollout_marshal_detail;
  std::ostringstream out;
  detail::append_kv(out, "schema_version",
                    k_marshal_rollout_asset_universe_schema_v1);
  detail::append_kv(out, "accounting_numeraire_node_id",
                    accounting_numeraire_node_id);
  detail::append_kv(out, "target_node_ids", detail::csv(target_node_ids));
  return out.str();
}

[[nodiscard]] inline std::string
rollout_asset_universe_digest(const std::string &accounting_numeraire_node_id,
                              const std::vector<std::string> &target_node_ids) {
  return marshal_digest_for_text(
      k_marshal_rollout_asset_universe_schema_v1,
      rollout_asset_universe_text(accounting_numeraire_node_id,
                                  target_node_ids));
}

[[nodiscard]] inline std::string
rollout_policy_set_text(const std::vector<std::string> &resolved_policy_ids) {
  namespace detail = rollout_marshal_detail;
  std::ostringstream out;
  detail::append_kv(out, "schema_version",
                    "kikijyeba.marshal.rollout_policy_set.v1");
  detail::append_kv(out, "resolved_policy_ids",
                    detail::csv(resolved_policy_ids));
  return out.str();
}

[[nodiscard]] inline std::string
rollout_policy_set_digest(const std::vector<std::string> &resolved_policy_ids) {
  return marshal_digest_for_text("kikijyeba.marshal.rollout_policy_set.v1",
                                 rollout_policy_set_text(resolved_policy_ids));
}

[[nodiscard]] inline std::string canonical_rollout_execution_profile_text(
    const marshal_rollout_execution_profile_t &profile) {
  namespace detail = rollout_marshal_detail;
  std::ostringstream out;
  detail::append_kv(out, "schema_version", profile.schema_version);
  detail::append_kv(out, "execution_backend_id", profile.execution_backend_id);
  detail::append_kv(out, "cost_model_id", profile.cost_model_id);
  detail::append_bool(out, "allow_synthetic_direct_edges",
                      profile.allow_synthetic_direct_edges);
  detail::append_kv(out, "synthetic_edge_research_reason",
                    profile.synthetic_edge_research_reason);
  detail::append_double(out, "linear_transaction_cost_rate",
                        profile.linear_transaction_cost_rate);
  detail::append_bool(out, "allow_partial_fills", profile.allow_partial_fills);
  detail::append_double(out, "equity_mismatch_tolerance",
                        profile.equity_mismatch_tolerance);
  detail::append_double(out, "equity_mismatch_fail_tolerance",
                        profile.equity_mismatch_fail_tolerance);
  detail::append_bool(out, "live_execution_allowed",
                      profile.live_execution_allowed);
  return out.str();
}

[[nodiscard]] inline std::string rollout_execution_profile_digest(
    const marshal_rollout_execution_profile_t &profile) {
  return marshal_digest_for_text(
      k_marshal_rollout_execution_profile_schema_v1,
      canonical_rollout_execution_profile_text(profile));
}

[[nodiscard]] inline std::string
canonical_rollout_request_text(const marshal_rollout_request_t &request) {
  namespace detail = rollout_marshal_detail;
  std::ostringstream out;
  detail::append_kv(out, "schema_version", request.schema_version);
  detail::append_kv(out, "rollout_id", request.rollout_id);
  detail::append_kv(out, "rollout_attempt_id", request.rollout_attempt_id);
  detail::append_kv(out, "idempotency_key", request.idempotency_key);
  detail::append_kv(out, "experiment_id", request.experiment_id);
  detail::append_kv(out, "config_path", request.config_path.string());
  detail::append_kv(out, "runtime_job_dir", request.runtime_job_dir.string());
  detail::append_kv(out, "replay_batch_index_path",
                    request.replay_batch_index_path.string());
  detail::append_kv(out, "runtime_exec_path",
                    request.runtime_exec_path.string());
  detail::append_kv(out, "report_path", request.report_path.string());
  detail::append_kv(out, "requested_mode", request.requested_mode);
  detail::append_kv(out, "environment_mode", request.environment_mode);
  detail::append_kv(out, "environment_assembly_id",
                    request.environment_assembly_id);
  detail::append_kv(out, "graph_order_fingerprint",
                    request.graph_order_fingerprint);
  detail::append_kv(out, "asset_universe_digest",
                    request.asset_universe_digest);
  detail::append_kv(out, "accounting_numeraire_node_id",
                    request.accounting_numeraire_node_id);
  detail::append_kv(out, "target_node_ids",
                    detail::csv(request.target_node_ids));
  detail::append_kv(out, "policy_tokens", detail::csv(request.policy_tokens));
  detail::append_i64(out, "max_steps", request.max_steps);
  detail::append_i64(out, "max_parallel_jobs", request.max_parallel_jobs);
  detail::append_i64(out, "timeout_seconds", request.timeout_seconds);
  detail::append_kv(
      out, "execution_profile",
      canonical_rollout_execution_profile_text(request.execution_profile));
  detail::append_bool(out, "require_existing_runtime_job_dir",
                      request.require_existing_runtime_job_dir);
  detail::append_bool(out, "require_completed_runtime_job",
                      request.require_completed_runtime_job);
  detail::append_bool(out, "require_replay_artifacts",
                      request.require_replay_artifacts);
  detail::append_kv(out, "non_authority_statement",
                    k_marshal_rollout_non_authority_statement);
  return out.str();
}

[[nodiscard]] inline std::string
args_digest(const marshal_rollout_request_t &request) {
  return marshal_digest_for_text(k_marshal_rollout_request_schema_v1,
                                 canonical_rollout_request_text(request));
}

[[nodiscard]] inline marshal_rollout_request_t
normalize_rollout_request_defaults(marshal_rollout_request_t request) {
  if (request.accounting_numeraire_node_id.empty()) {
    request.accounting_numeraire_node_id =
        rollout_marshal_detail::read_accounting_numeraire_node_id_from_config(
            request.config_path);
  }
  return request;
}

[[nodiscard]] inline std::vector<std::string>
validate_rollout_request(const marshal_rollout_request_t &request) {
  namespace detail = rollout_marshal_detail;
  std::vector<std::string> refusals;

  if (request.rollout_id.empty()) {
    refusals.emplace_back("missing_rollout_id");
  }
  if (request.rollout_attempt_id.empty()) {
    refusals.emplace_back("missing_rollout_attempt_id");
  }
  if (request.idempotency_key.empty()) {
    refusals.emplace_back("missing_idempotency_key");
  }
  if (request.runtime_job_dir.empty()) {
    refusals.emplace_back("missing_runtime_job_dir");
  }
  if (request.replay_batch_index_path.empty()) {
    refusals.emplace_back("missing_replay_batch_index_path");
  }
  if (request.requested_mode != "plan" && request.requested_mode != "execute") {
    refusals.emplace_back("unsupported_requested_mode");
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
  if (request.graph_order_fingerprint.empty()) {
    refusals.emplace_back("missing_graph_order_fingerprint");
  }
  if (request.asset_universe_digest.empty()) {
    refusals.emplace_back("missing_asset_universe_digest");
  }
  if (request.accounting_numeraire_node_id.empty()) {
    refusals.emplace_back("missing_config_accounting_numeraire_node_id");
  }
  if (request.target_node_ids.empty()) {
    refusals.emplace_back("missing_target_node_ids");
  }

  bool contains_numeraire = false;
  std::unordered_set<std::string> seen_nodes;
  for (const auto &node_id : request.target_node_ids) {
    if (node_id.empty()) {
      refusals.emplace_back("empty_target_node_id");
      continue;
    }
    if (node_id == request.accounting_numeraire_node_id) {
      contains_numeraire = true;
    }
    if (!seen_nodes.insert(node_id).second) {
      refusals.emplace_back("duplicate_target_node_id");
    }
  }
  if (!request.accounting_numeraire_node_id.empty() && !contains_numeraire) {
    refusals.emplace_back(
        "accounting_numeraire_node_missing_from_target_nodes");
  }

  const auto computed_asset_universe_digest = rollout_asset_universe_digest(
      request.accounting_numeraire_node_id, request.target_node_ids);
  if (!request.asset_universe_digest.empty() &&
      request.asset_universe_digest != computed_asset_universe_digest) {
    refusals.emplace_back("asset_universe_digest_mismatch");
  }

  if (request.policy_tokens.empty()) {
    refusals.emplace_back("missing_policy_set");
    refusals.emplace_back("empty_policy_set");
  }
  for (const auto &token : request.policy_tokens) {
    if (detail::normalize_policy_token(token).empty()) {
      refusals.emplace_back("unsupported_policy_token:" + token);
    }
  }
  if (request.max_steps == 0) {
    refusals.emplace_back("missing_max_steps");
  }
  if (request.max_steps <= 0) {
    refusals.emplace_back("nonpositive_max_steps");
  }
  if (request.max_parallel_jobs <= 0) {
    refusals.emplace_back("invalid_max_parallel_jobs");
  }
  if (request.timeout_seconds <= 0) {
    refusals.emplace_back("invalid_timeout_seconds");
  }

  const auto &profile = request.execution_profile;
  if (profile.cost_model_id != "linear_transaction_cost_rate.v1") {
    refusals.emplace_back("unsupported_cost_model_id");
  }
  if (!std::isfinite(profile.linear_transaction_cost_rate) ||
      profile.linear_transaction_cost_rate < 0.0) {
    refusals.emplace_back("invalid_linear_transaction_cost_rate");
  }
  if (std::isfinite(profile.linear_transaction_cost_rate) &&
      profile.linear_transaction_cost_rate == 0.0) {
    refusals.emplace_back("nonzero_cost_required_for_validation_rollout");
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
  if (profile.allow_synthetic_direct_edges) {
    refusals.emplace_back(
        "synthetic_direct_edges_forbidden_for_validation_rollout");
  }
  if (profile.allow_partial_fills) {
    refusals.emplace_back("partial_fills_not_supported_by_runtime_replay_v1");
  }
  if (profile.live_execution_allowed) {
    refusals.emplace_back("live_execution_forbidden_v1");
  }

  const auto job_state_path = request.runtime_job_dir / "job.state";
  const auto job_manifest_path = request.runtime_job_dir / "job.manifest";
  if (!request.runtime_job_dir.empty() &&
      std::filesystem::exists(job_manifest_path)) {
    const auto job_manifest = detail::read_kv_file(job_manifest_path);
    const auto manifest_graph =
        detail::kv_string(job_manifest, "graph_order_fingerprint");
    if (manifest_graph.empty()) {
      refusals.emplace_back("runtime_job_graph_order_fingerprint_missing");
    } else if (!request.graph_order_fingerprint.empty() &&
               manifest_graph != request.graph_order_fingerprint) {
      refusals.emplace_back("runtime_job_graph_order_fingerprint_mismatch");
    }
  } else if (request.require_existing_runtime_job_dir &&
             !request.runtime_job_dir.empty() &&
             std::filesystem::exists(request.runtime_job_dir)) {
    refusals.emplace_back("runtime_job_manifest_missing");
  }
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
      if (batch_index.empty()) {
        refusals.emplace_back("replay_batch_index_missing");
      } else if (!request.replay_batch_index_path.empty() &&
                 std::filesystem::path(batch_index) !=
                     request.replay_batch_index_path) {
        refusals.emplace_back("replay_batch_index_path_mismatch");
      }
      if (request.replay_batch_index_path.empty() ||
          !std::filesystem::exists(request.replay_batch_index_path)) {
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
  detail::append_kv(out, "rollout_attempt_id", plan.rollout_attempt_id);
  detail::append_kv(out, "idempotency_key", plan.idempotency_key);
  detail::append_kv(out, "experiment_id", plan.experiment_id);
  detail::append_bool(out, "accepted", plan.accepted);
  detail::append_kv(out, "runtime_job_dir", plan.runtime_job_dir.string());
  detail::append_kv(out, "replay_batch_index_path",
                    plan.replay_batch_index_path.string());
  detail::append_kv(out, "expected_report_path",
                    plan.expected_report_path.string());
  detail::append_kv(out, "requested_mode", plan.requested_mode);
  detail::append_kv(out, "environment_assembly_id",
                    plan.environment_assembly_id);
  detail::append_kv(out, "graph_order_fingerprint",
                    plan.graph_order_fingerprint);
  detail::append_kv(out, "asset_universe_digest", plan.asset_universe_digest);
  detail::append_kv(out, "execution_backend_id", plan.execution_backend_id);
  detail::append_kv(out, "execution_profile_digest",
                    plan.execution_profile_digest);
  detail::append_kv(out, "policy_set_digest", plan.policy_set_digest);
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
prepare_rollout_plan(marshal_rollout_request_t request) {
  namespace detail = rollout_marshal_detail;
  request = normalize_rollout_request_defaults(std::move(request));
  marshal_rollout_plan_t plan{};
  plan.request_digest = args_digest(request);
  plan.rollout_id = request.rollout_id;
  plan.rollout_attempt_id = request.rollout_attempt_id;
  plan.idempotency_key = request.idempotency_key;
  plan.experiment_id = request.experiment_id.empty() ? request.rollout_id
                                                     : request.experiment_id;
  plan.runtime_job_dir = request.runtime_job_dir;
  plan.replay_batch_index_path = request.replay_batch_index_path;
  plan.expected_report_path = detail::default_report_path(request);
  plan.requested_mode = request.requested_mode;
  plan.environment_assembly_id = request.environment_assembly_id;
  plan.graph_order_fingerprint = request.graph_order_fingerprint;
  plan.asset_universe_digest = request.asset_universe_digest;
  plan.execution_backend_id = request.execution_profile.execution_backend_id;
  plan.execution_profile_digest =
      rollout_execution_profile_digest(request.execution_profile);
  plan.requested_policy_tokens = request.policy_tokens;
  plan.resolved_policy_ids = detail::resolve_policy_ids(request.policy_tokens);
  plan.policy_set_digest = rollout_policy_set_digest(plan.resolved_policy_ids);
  plan.execution_profile = request.execution_profile;
  plan.refusal_reasons = validate_rollout_request(request);
  plan.accepted = plan.refusal_reasons.empty();

  if (plan.accepted) {
    std::ostringstream command;
    command << detail::shell_quote(request.runtime_exec_path.string())
            << " --config " << detail::shell_quote(request.config_path.string())
            << " --replay-from-job-dir "
            << detail::shell_quote(request.runtime_job_dir.string())
            << " --replay-experiment-id "
            << detail::shell_quote(plan.experiment_id)
            << " --replay-report-path "
            << detail::shell_quote(plan.expected_report_path.string())
            << " --replay-accounting-numeraire-node "
            << detail::shell_quote(request.accounting_numeraire_node_id)
            << " --replay-target-nodes "
            << detail::shell_quote(detail::csv(request.target_node_ids));
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
                                 k_rollout_policy_numeraire_only)) {
      command << " --replay-no-numeraire-only-policy";
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
    if (!plan.execution_profile_digest.empty()) {
      command << " --replay-execution-profile-digest "
              << detail::shell_quote(plan.execution_profile_digest);
    }
    if (!plan.policy_set_digest.empty()) {
      command << " --replay-policy-set-digest "
              << detail::shell_quote(plan.policy_set_digest);
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
  detail::append_json_string_field(out, "cost_model_id", profile.cost_model_id,
                                   &first);
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
  detail::append_json_string_field(out, "rollout_attempt_id",
                                   plan.rollout_attempt_id, &first);
  detail::append_json_string_field(out, "idempotency_key", plan.idempotency_key,
                                   &first);
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
  detail::append_json_string_field(out, "replay_batch_index_path",
                                   plan.replay_batch_index_path.string(),
                                   &first);
  detail::append_json_string_field(out, "expected_report_path",
                                   plan.expected_report_path.string(), &first);
  detail::append_json_string_field(out, "requested_mode", plan.requested_mode,
                                   &first);
  detail::append_json_string_field(out, "environment_assembly_id",
                                   plan.environment_assembly_id, &first);
  detail::append_json_string_field(out, "graph_order_fingerprint",
                                   plan.graph_order_fingerprint, &first);
  detail::append_json_string_field(out, "asset_universe_digest",
                                   plan.asset_universe_digest, &first);
  detail::append_json_string_field(out, "execution_backend_id",
                                   plan.execution_backend_id, &first);
  detail::append_json_string_field(out, "execution_profile_digest",
                                   plan.execution_profile_digest, &first);
  detail::append_json_string_field(out, "policy_set_digest",
                                   plan.policy_set_digest, &first);

  out << "," << detail::json_quote("requested_policy_tokens") << ":";
  detail::append_json_string_array(out, plan.requested_policy_tokens);
  out << "," << detail::json_quote("resolved_policy_ids") << ":";
  detail::append_json_string_array(out, plan.resolved_policy_ids);

  out << "," << detail::json_quote("execution_profile") << ":"
      << rollout_execution_profile_json(plan.execution_profile);
  out << "," << detail::json_quote("idempotency")
      << ":{\"scope\":\"request_digest_binding\","
         "\"durable_duplicate_handoff_ledger\":false,"
         "\"duplicate_execution_prevented_by_marshal\":false}";
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

[[nodiscard]] inline std::string rollout_runtime_replay_execution_request_text(
    const marshal_rollout_request_t &request,
    const marshal_rollout_plan_t &plan) {
  namespace detail = rollout_marshal_detail;
  std::ostringstream out;
  detail::append_kv(out, "job_dir", request.runtime_job_dir.string());
  detail::append_kv(out, "accounting_numeraire_node_id",
                    request.accounting_numeraire_node_id);
  detail::append_kv(out, "target_node_ids",
                    detail::csv(request.target_node_ids));
  detail::append_kv(out, "experiment_id", plan.experiment_id);
  detail::append_kv(out, "report_path", plan.expected_report_path.string());
  detail::append_kv(out, "execution_profile_digest",
                    plan.execution_profile_digest);
  detail::append_kv(out, "policy_set_digest", plan.policy_set_digest);
  if (request.max_steps > 0) {
    detail::append_i64(out, "max_steps", request.max_steps);
  }
  if (request.max_parallel_jobs > 0) {
    detail::append_i64(out, "max_parallel_jobs", request.max_parallel_jobs);
  }
  detail::append_bool(out, "include_equal_weight",
                      detail::contains_policy(plan.resolved_policy_ids,
                                              k_rollout_policy_equal_weight));
  detail::append_bool(out, "include_current_weight",
                      detail::contains_policy(plan.resolved_policy_ids,
                                              k_rollout_policy_current_weight));
  detail::append_bool(out, "include_numeraire_only_policy",
                      detail::contains_policy(plan.resolved_policy_ids,
                                              k_rollout_policy_numeraire_only));
  detail::append_bool(
      out, "include_spot_distributional_utility_policy",
      detail::contains_policy(plan.resolved_policy_ids, k_rollout_policy_sdu));
  detail::append_bool(out, "allow_synthetic_direct_edges",
                      request.execution_profile.allow_synthetic_direct_edges);
  detail::append_bool(out, "validation_rollout", true);
  if (request.execution_profile.linear_transaction_cost_rate > 0.0) {
    detail::append_double(
        out, "linear_transaction_cost_rate",
        request.execution_profile.linear_transaction_cost_rate);
  }
  return out.str();
}

[[nodiscard]] inline std::filesystem::path
materialize_rollout_runtime_replay_execution_request(
    const marshal_rollout_request_t &request,
    const marshal_rollout_plan_t &plan, std::string *request_digest) {
  const std::string text =
      rollout_runtime_replay_execution_request_text(request, plan);
  const std::string digest = marshal_digest_for_text(
      "kikijyeba.runtime.policy_training_execution_request.v1", text);
  if (request_digest != nullptr) {
    *request_digest = digest;
  }
  const std::filesystem::path dir =
      std::filesystem::temp_directory_path() / "cuwacunu_hero_runtime_requests";
  std::filesystem::create_directories(dir);
  const std::filesystem::path path = dir / ("replay_" + digest + ".kv");
  std::ofstream out(path, std::ios::trunc);
  out << text;
  return path;
}

[[nodiscard]] inline std::string
rollout_runtime_replay_args_json(const marshal_rollout_request_t &request,
                                 const marshal_rollout_plan_t &plan,
                                 bool dry_run) {
  namespace detail = rollout_marshal_detail;
  std::string execution_request_digest;
  const std::filesystem::path execution_request_path =
      materialize_rollout_runtime_replay_execution_request(
          request, plan, &execution_request_digest);
  std::ostringstream run_request_text;
  run_request_text << "config_path=" << request.config_path.string() << "\n"
                   << "execution_request_path="
                   << execution_request_path.string() << "\n"
                   << "execution_request_digest=" << execution_request_digest
                   << "\n";
  const std::string args_digest = marshal_digest_for_text(
      "kikijyeba.runtime.run_request.replay.v1", run_request_text.str());
  const std::filesystem::path run_request_dir =
      std::filesystem::temp_directory_path() / "cuwacunu_hero_runtime_requests";
  std::filesystem::create_directories(run_request_dir);
  const std::filesystem::path args_path =
      run_request_dir / ("replay_run_" + args_digest + ".kv");
  std::ofstream run_request_out(args_path, std::ios::trunc);
  run_request_out << run_request_text.str();

  std::ostringstream out;
  out << "{";
  bool first = true;
  detail::append_json_string_field(out, "mode", dry_run ? "dry_run" : "execute",
                                   &first);
  detail::append_json_string_field(out, "args_path", args_path.string(),
                                   &first);
  detail::append_json_string_field(out, "args_digest", args_digest, &first);
  out << "}";
  return out.str();
}

} // namespace cuwacunu::hero::marshal
