// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "hero/marshal_hero/marshal/digest.h"
#include "hero/marshal_hero/marshal/rollout_marshal.h"

namespace cuwacunu::hero::marshal {

inline constexpr const char
    *k_marshal_paper_online_session_handoff_request_schema_v1 =
        "kikijyeba.marshal.paper_online_session_handoff_request.v1";
inline constexpr const char
    *k_marshal_paper_online_session_handoff_packet_schema_v1 =
        "kikijyeba.marshal.paper_online_session_handoff_packet.v1";
inline constexpr const char
    *k_marshal_paper_online_session_handoff_receipt_schema_v1 =
        "kikijyeba.marshal.paper_online_session_handoff_receipt.v1";

struct marshal_paper_online_session_handoff_request_t {
  std::string schema_version{
      k_marshal_paper_online_session_handoff_request_schema_v1};
  std::filesystem::path config_path{"/cuwacunu/src/config/.config"};
  std::filesystem::path runtime_root{"/cuwacunu/.runtime/cuwacunu_exec"};
  std::filesystem::path readiness_job_dir{};
  std::string readiness_target_id{"paper_online_readiness_contract_ready"};
  std::string readiness_proof_certificate_digest{};
  std::string expected_readiness_fact_digest{};
  std::int64_t readiness_proof_checked_at_ms{0};
  std::int64_t max_readiness_proof_age_ms{60000};
  std::string admission_id{};
  std::int64_t admission_requested_at_ms{0};
  std::string session_id{};
  std::filesystem::path session_root{};
  std::int64_t session_requested_at_ms{0};
  std::int64_t max_steps{0};
  std::int64_t step_interval_ms{1000};
  std::int64_t market_data_receive_lag_ms{0};
  std::vector<std::string> target_node_ids{};
  bool recover_persistent_ledger{true};
  std::filesystem::path receipt_path{};
  std::string handoff_id{};
  int timeout_seconds{600};
  std::string source{"handoff_request"};
};

namespace paper_online_session_handoff_detail {

[[nodiscard]] inline std::string json_quote(std::string_view value) {
  return rollout_marshal_detail::json_quote(value);
}

[[nodiscard]] inline std::string
string_array_json(const std::vector<std::string> &values) {
  std::ostringstream out;
  rollout_marshal_detail::append_json_string_array(out, values);
  return out.str();
}

[[nodiscard]] inline bool id_is_path_safe(std::string_view value) {
  return !rollout_marshal_detail::trim_ascii(value).empty() &&
         value.find('/') == std::string_view::npos &&
         value.find('\\') == std::string_view::npos &&
         value.find("..") == std::string_view::npos;
}

[[nodiscard]] inline bool path_exists(const std::filesystem::path &path) {
  std::error_code ec;
  return std::filesystem::exists(path, ec);
}

[[nodiscard]] inline bool path_is_directory(const std::filesystem::path &path) {
  std::error_code ec;
  return std::filesystem::is_directory(path, ec);
}

[[nodiscard]] inline std::filesystem::path
normalized_path_for_compare(std::filesystem::path path) {
  std::error_code ec;
  if (path.is_relative()) {
    path = std::filesystem::absolute(path, ec);
  }
  ec.clear();
  const auto normalized = std::filesystem::weakly_canonical(path, ec);
  return ec ? path.lexically_normal() : normalized;
}

[[nodiscard]] inline bool path_is_within(const std::filesystem::path &root,
                                         const std::filesystem::path &path) {
  const auto normalized_root = normalized_path_for_compare(root);
  const auto normalized_path = normalized_path_for_compare(path);
  auto root_it = normalized_root.begin();
  auto path_it = normalized_path.begin();
  for (; root_it != normalized_root.end(); ++root_it, ++path_it) {
    if (path_it == normalized_path.end() || *root_it != *path_it) {
      return false;
    }
  }
  return true;
}

inline void write_text_file_atomically(const std::filesystem::path &path,
                                       const std::string &text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  const std::filesystem::path tmp_path =
      path.parent_path() / (path.filename().string() + ".tmp");
  {
    std::ofstream out(tmp_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
      throw std::runtime_error("failed to open receipt tmp file: " +
                               tmp_path.string());
    }
    out << text;
    if (!out.good()) {
      throw std::runtime_error("failed to write receipt tmp file: " +
                               tmp_path.string());
    }
  }
  std::error_code ec;
  std::filesystem::remove(path, ec);
  ec.clear();
  std::filesystem::rename(tmp_path, path, ec);
  if (ec) {
    throw std::runtime_error("failed to install receipt file: " +
                             path.string() + ": " + ec.message());
  }
}

} // namespace paper_online_session_handoff_detail

inline void normalize_paper_online_session_handoff_request(
    marshal_paper_online_session_handoff_request_t *request) {
  if (request == nullptr) {
    return;
  }
  if (request->session_requested_at_ms == 0) {
    request->session_requested_at_ms = request->admission_requested_at_ms;
  }
  if (request->handoff_id.empty() && !request->session_id.empty()) {
    request->handoff_id = request->session_id + ".paper_online_session_handoff";
  }
  if (request->receipt_path.empty() && !request->readiness_job_dir.empty() &&
      !request->session_id.empty()) {
    request->receipt_path =
        request->readiness_job_dir / ("marshal.paper_online_session_handoff." +
                                      request->session_id + ".receipt.json");
  }
}

[[nodiscard]] inline std::vector<std::string>
paper_online_session_handoff_local_issues(
    const marshal_paper_online_session_handoff_request_t &request) {
  namespace detail = paper_online_session_handoff_detail;
  std::vector<std::string> issues;
  if (request.config_path.empty()) {
    issues.emplace_back("config_path_missing");
  }
  if (request.runtime_root.empty()) {
    issues.emplace_back("runtime_root_missing");
  }
  if (request.readiness_job_dir.empty()) {
    issues.emplace_back("readiness_job_dir_missing");
  } else if (!detail::path_exists(request.readiness_job_dir /
                                  "lattice.paper_online_readiness.fact")) {
    issues.emplace_back("paper_online_readiness_fact_missing");
  }
  if (request.readiness_target_id != "paper_online_readiness_contract_ready") {
    issues.emplace_back("unexpected_readiness_target_id");
  }
  if (request.readiness_proof_certificate_digest.empty()) {
    issues.emplace_back("readiness_proof_certificate_digest_missing");
  }
  if (request.readiness_proof_checked_at_ms <= 0) {
    issues.emplace_back("readiness_proof_checked_at_ms_invalid");
  }
  if (request.max_readiness_proof_age_ms <= 0) {
    issues.emplace_back("max_readiness_proof_age_ms_invalid");
  }
  if (!detail::id_is_path_safe(request.admission_id)) {
    issues.emplace_back("admission_id_invalid");
  }
  if (request.admission_requested_at_ms <= 0) {
    issues.emplace_back("admission_requested_at_ms_invalid");
  }
  if (!detail::id_is_path_safe(request.session_id)) {
    issues.emplace_back("session_id_invalid");
  }
  if (request.session_root.empty()) {
    issues.emplace_back("session_root_missing");
  }
  if (request.session_requested_at_ms <= 0) {
    issues.emplace_back("session_requested_at_ms_invalid");
  }
  if (request.session_requested_at_ms < request.admission_requested_at_ms) {
    issues.emplace_back("session_requested_before_admission");
  }
  if (request.max_steps <= 0) {
    issues.emplace_back("max_steps_invalid");
  }
  if (request.step_interval_ms <= 0) {
    issues.emplace_back("step_interval_ms_invalid");
  }
  if (request.market_data_receive_lag_ms < 0) {
    issues.emplace_back("market_data_receive_lag_ms_invalid");
  }
  if (request.target_node_ids.empty()) {
    issues.emplace_back("target_node_ids_missing");
  }
  std::set<std::string> seen_nodes;
  for (const auto &node_id : request.target_node_ids) {
    if (!detail::id_is_path_safe(node_id)) {
      issues.emplace_back("target_node_id_invalid");
    }
    if (!seen_nodes.insert(node_id).second) {
      issues.emplace_back("target_node_ids_duplicate");
      break;
    }
  }
  if (!request.recover_persistent_ledger) {
    issues.emplace_back("recover_persistent_ledger_required");
  }
  if (!detail::id_is_path_safe(request.handoff_id)) {
    issues.emplace_back("handoff_id_invalid");
  }
  if (request.receipt_path.empty()) {
    issues.emplace_back("receipt_path_missing");
  } else if (!request.readiness_job_dir.empty() &&
             !detail::path_is_within(request.readiness_job_dir,
                                     request.receipt_path)) {
    issues.emplace_back("receipt_path_outside_readiness_job_dir");
  } else if (!request.readiness_job_dir.empty() &&
             detail::normalized_path_for_compare(request.readiness_job_dir) ==
                 detail::normalized_path_for_compare(request.receipt_path)) {
    issues.emplace_back("receipt_path_must_be_file");
  } else if (detail::path_is_directory(request.receipt_path)) {
    issues.emplace_back("receipt_path_must_be_file");
  }
  if (request.timeout_seconds <= 0) {
    issues.emplace_back("timeout_seconds_invalid");
  }
  std::sort(issues.begin(), issues.end());
  issues.erase(std::unique(issues.begin(), issues.end()), issues.end());
  return issues;
}

[[nodiscard]] inline std::string
canonical_paper_online_session_handoff_request_text(
    const marshal_paper_online_session_handoff_request_t &request) {
  namespace detail = rollout_marshal_detail;
  std::ostringstream out;
  detail::append_kv(out, "schema_version", request.schema_version);
  detail::append_kv(out, "config_path", request.config_path.string());
  detail::append_kv(out, "runtime_root", request.runtime_root.string());
  detail::append_kv(out, "readiness_job_dir",
                    request.readiness_job_dir.string());
  detail::append_kv(out, "readiness_target_id", request.readiness_target_id);
  detail::append_kv(out, "readiness_proof_certificate_digest",
                    request.readiness_proof_certificate_digest);
  detail::append_kv(out, "expected_readiness_fact_digest",
                    request.expected_readiness_fact_digest);
  detail::append_i64(out, "readiness_proof_checked_at_ms",
                     request.readiness_proof_checked_at_ms);
  detail::append_i64(out, "max_readiness_proof_age_ms",
                     request.max_readiness_proof_age_ms);
  detail::append_kv(out, "admission_id", request.admission_id);
  detail::append_i64(out, "admission_requested_at_ms",
                     request.admission_requested_at_ms);
  detail::append_kv(out, "session_id", request.session_id);
  detail::append_kv(out, "session_root", request.session_root.string());
  detail::append_i64(out, "session_requested_at_ms",
                     request.session_requested_at_ms);
  detail::append_i64(out, "max_steps", request.max_steps);
  detail::append_i64(out, "step_interval_ms", request.step_interval_ms);
  detail::append_i64(out, "market_data_receive_lag_ms",
                     request.market_data_receive_lag_ms);
  detail::append_kv(out, "target_node_ids",
                    detail::csv(request.target_node_ids));
  detail::append_bool(out, "recover_persistent_ledger",
                      request.recover_persistent_ledger);
  detail::append_kv(out, "receipt_path", request.receipt_path.string());
  detail::append_kv(out, "handoff_id", request.handoff_id);
  detail::append_i64(out, "timeout_seconds", request.timeout_seconds);
  detail::append_kv(out, "source", request.source);
  detail::append_kv(
      out, "authority",
      "marshal_prepares_only;environment_validates_or_runs;lattice_proves");
  return out.str();
}

[[nodiscard]] inline std::string paper_online_session_handoff_request_digest(
    const marshal_paper_online_session_handoff_request_t &request) {
  return marshal_digest_for_text(
      k_marshal_paper_online_session_handoff_request_schema_v1,
      canonical_paper_online_session_handoff_request_text(request));
}

[[nodiscard]] inline std::string
paper_online_session_handoff_admission_request_json(
    const marshal_paper_online_session_handoff_request_t &request) {
  namespace detail = paper_online_session_handoff_detail;
  std::ostringstream out;
  out << "{\"schema\":\"kikijyeba.environment."
         "paper_online_session_admission_request.v1\""
      << ",\"readiness\":{\"job_dir\":"
      << detail::json_quote(request.readiness_job_dir.string())
      << ",\"target_id\":" << detail::json_quote(request.readiness_target_id)
      << ",\"proof_certificate_digest\":"
      << detail::json_quote(request.readiness_proof_certificate_digest)
      << ",\"proof_checked_at_ms\":" << request.readiness_proof_checked_at_ms
      << ",\"max_proof_age_ms\":" << request.max_readiness_proof_age_ms;
  if (!request.expected_readiness_fact_digest.empty()) {
    out << ",\"readiness_fact_digest\":"
        << detail::json_quote(request.expected_readiness_fact_digest);
  }
  out << "},\"session\":{\"admission_id\":"
      << detail::json_quote(request.admission_id)
      << ",\"requested_at_ms\":" << request.admission_requested_at_ms << "}}";
  return out.str();
}

[[nodiscard]] inline std::string
paper_online_session_handoff_session_request_json(
    const marshal_paper_online_session_handoff_request_t &request) {
  namespace detail = paper_online_session_handoff_detail;
  std::ostringstream out;
  out << "{\"schema\":\"kikijyeba.environment."
         "paper_online_session_run_request.v1\""
      << ",\"admission\":{\"job_dir\":"
      << detail::json_quote(request.readiness_job_dir.string()) << "}"
      << ",\"session\":{\"session_id\":"
      << detail::json_quote(request.session_id) << ",\"session_root\":"
      << detail::json_quote(request.session_root.string())
      << ",\"requested_at_ms\":" << request.session_requested_at_ms
      << ",\"max_steps\":" << request.max_steps
      << ",\"step_interval_ms\":" << request.step_interval_ms
      << ",\"market_data_receive_lag_ms\":"
      << request.market_data_receive_lag_ms << ",\"target_node_ids\":"
      << detail::string_array_json(request.target_node_ids)
      << ",\"recover_persistent_ledger\":"
      << (request.recover_persistent_ledger ? "true" : "false")
      << ",\"operator_abort_at_step\":-1"
      << ",\"kill_switch_at_step\":-1"
      << ",\"duplicate_action_at_step\":-1"
      << ",\"duplicate_execution_intent_at_step\":-1}}";
  return out.str();
}

[[nodiscard]] inline std::string
paper_online_session_handoff_admission_args_json(
    const marshal_paper_online_session_handoff_request_t &request) {
  return "{\"mode\":\"check\",\"admission_request\":" +
         paper_online_session_handoff_admission_request_json(request) + "}";
}

[[nodiscard]] inline std::string
paper_online_session_handoff_session_validate_args_json(
    const marshal_paper_online_session_handoff_request_t &request,
    bool include_machine_payload) {
  return "{\"mode\":\"validate\",\"session_request\":" +
         paper_online_session_handoff_session_request_json(request) +
         ",\"include_machine_payload\":" +
         (include_machine_payload ? "true" : "false") + "}";
}

[[nodiscard]] inline std::string
paper_online_session_handoff_session_run_args_json(
    const marshal_paper_online_session_handoff_request_t &request,
    bool include_machine_payload) {
  return "{\"mode\":\"run\",\"session_request\":" +
         paper_online_session_handoff_session_request_json(request) +
         ",\"include_machine_payload\":" +
         (include_machine_payload ? "true" : "false") + "}";
}

} // namespace cuwacunu::hero::marshal
