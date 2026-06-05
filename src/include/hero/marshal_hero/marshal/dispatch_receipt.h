// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "hero/marshal_hero/marshal/execution_gate.h"

namespace cuwacunu::hero::marshal {

inline constexpr const char *k_marshal_dispatch_receipt_schema_v1 =
    "kikijyeba.marshal.dispatch_receipt.v1";

struct marshal_dispatch_receipt_t {
  std::string schema_version{k_marshal_dispatch_receipt_schema_v1};
  std::string retention_class{"full"};
  std::string receipt_id{};
  std::string receipt_kind{};
  std::string timestamp{};
  std::string target_id{};
  marshal_active_identity_t lattice_proof_context{};
  std::string plan_basis_digest{};
  std::string suggested_wave_digest{};
  std::string advice_digest{};
  std::string request_digest{};
  std::string decision_digest{};
  std::string runtime_request_digest{};
  std::string runtime_preview_request_digest{};
  std::string runtime_execution_request_digest{};
  std::string runtime_handoff_arguments_digest{};
  std::string runtime_response_digest{};
  std::string policy_decision{};
  std::string confirmation_status{};
  std::vector<std::string> refusal_reasons{};
  std::vector<std::string> compacted_fields{};
  std::string runtime_tool_result_json{};
  std::string post_run_verification{};
  std::string non_authority_statement{
      k_marshal_dispatch_non_authority_statement};
};

struct marshal_receipt_replay_audit_t {
  bool accepted{false};
  bool stale{false};
  bool non_authoritative{true};
  std::vector<std::string> issues{};
  std::string explanation{};
};

[[nodiscard]] inline std::string
canonical_dispatch_receipt_text(const marshal_dispatch_receipt_t &receipt) {
  std::ostringstream out;
  detail::append_kv(out, "schema_version", receipt.schema_version);
  detail::append_kv(out, "retention_class", receipt.retention_class);
  detail::append_kv(out, "receipt_kind", receipt.receipt_kind);
  detail::append_kv(out, "timestamp", receipt.timestamp);
  detail::append_kv(out, "target_id", receipt.target_id);
  detail::append_kv(
      out, "lattice_proof_context",
      canonical_active_identity_text(receipt.lattice_proof_context));
  detail::append_kv(out, "plan_basis_digest", receipt.plan_basis_digest);
  detail::append_kv(out, "suggested_wave_digest",
                    receipt.suggested_wave_digest);
  detail::append_kv(out, "advice_digest", receipt.advice_digest);
  detail::append_kv(out, "request_digest", receipt.request_digest);
  detail::append_kv(out, "decision_digest", receipt.decision_digest);
  detail::append_kv(out, "runtime_request_digest",
                    receipt.runtime_request_digest);
  detail::append_kv(out, "runtime_preview_request_digest",
                    receipt.runtime_preview_request_digest);
  detail::append_kv(out, "runtime_execution_request_digest",
                    receipt.runtime_execution_request_digest);
  detail::append_kv(out, "runtime_handoff_arguments_digest",
                    receipt.runtime_handoff_arguments_digest);
  detail::append_kv(out, "runtime_response_digest",
                    receipt.runtime_response_digest);
  detail::append_kv(out, "policy_decision", receipt.policy_decision);
  detail::append_kv(out, "confirmation_status", receipt.confirmation_status);
  detail::append_string_vector(out, "refusal_reasons", receipt.refusal_reasons);
  detail::append_string_vector(out, "compacted_fields",
                               receipt.compacted_fields);
  detail::append_kv(out, "runtime_tool_result_json",
                    receipt.runtime_tool_result_json);
  detail::append_kv(out, "post_run_verification",
                    receipt.post_run_verification);
  detail::append_kv(out, "non_authority_statement",
                    receipt.non_authority_statement);
  return out.str();
}

[[nodiscard]] inline std::string
dispatch_receipt_digest(const marshal_dispatch_receipt_t &receipt) {
  return marshal_digest_for_text("kikijyeba.marshal.dispatch_receipt.v1",
                                 canonical_dispatch_receipt_text(receipt));
}

inline void finalize_receipt_id(marshal_dispatch_receipt_t *receipt) {
  if (!receipt) {
    return;
  }
  receipt->receipt_id =
      marshal_digest_for_text("kikijyeba.marshal.dispatch_receipt_id.v1",
                              canonical_dispatch_receipt_text(*receipt));
}

[[nodiscard]] inline std::vector<std::string>
refusal_reason_texts(const std::vector<marshal_refusal_reason_t> &reasons) {
  std::vector<std::string> out;
  out.reserve(reasons.size());
  for (const auto reason : reasons) {
    out.push_back(to_string(reason));
  }
  return out;
}

[[nodiscard]] inline marshal_dispatch_receipt_t make_dry_run_dispatch_receipt(
    const marshal_dispatch_advice_t &advice,
    const marshal_dry_run_dispatch_response_t &response,
    const std::string &timestamp) {
  marshal_dispatch_receipt_t receipt{};
  receipt.receipt_kind = "dry_run";
  receipt.timestamp = timestamp;
  receipt.target_id = advice.target_id;
  receipt.lattice_proof_context = advice.active_identity;
  receipt.plan_basis_digest = plan_basis_digest(advice.plan_basis);
  receipt.suggested_wave_digest = suggested_wave_digest(advice.suggested_wave);
  receipt.advice_digest = response.advice_digest;
  receipt.request_digest = response.request_digest;
  receipt.decision_digest = dry_run_dispatch_response_digest(response);
  receipt.runtime_request_digest = response.runtime_request_digest;
  receipt.runtime_preview_request_digest = response.runtime_request_digest;
  receipt.runtime_handoff_arguments_digest =
      response.runtime_handoff.arguments_digest;
  receipt.runtime_response_digest =
      marshal_digest_for_text("kikijyeba.marshal.runtime_response.v1",
                              response.runtime_handoff.tool_result_json);
  receipt.policy_decision = response.accepted ? "accepted" : "refused";
  receipt.confirmation_status = "not_required";
  receipt.refusal_reasons =
      refusal_reason_texts(response.decision.refusal_reasons);
  receipt.runtime_tool_result_json = response.runtime_handoff.tool_result_json;
  receipt.post_run_verification =
      "ask Lattice Hero to re-read runtime evidence; this receipt is not "
      "target satisfaction evidence";
  finalize_receipt_id(&receipt);
  return receipt;
}

[[nodiscard]] inline marshal_dispatch_receipt_t make_execution_dispatch_receipt(
    const marshal_dispatch_advice_t &advice,
    const marshal_execution_gate_result_t &gate,
    const marshal_runtime_hero_handoff_result_t &handoff,
    const std::string &timestamp) {
  marshal_dispatch_receipt_t receipt{};
  receipt.receipt_kind = "execute";
  receipt.timestamp = timestamp;
  receipt.target_id = advice.target_id;
  receipt.lattice_proof_context = advice.active_identity;
  receipt.plan_basis_digest = plan_basis_digest(advice.plan_basis);
  receipt.suggested_wave_digest = suggested_wave_digest(advice.suggested_wave);
  receipt.advice_digest = gate.decision.advice_validation.advice_digest;
  receipt.request_digest = gate.decision.advice_validation.request_digest;
  receipt.decision_digest =
      marshal_digest_for_text("kikijyeba.marshal.execution_gate.v1",
                              gate.expected_confirmation_token + "|" +
                                  detail::bool_text(gate.accepted));
  receipt.runtime_preview_request_digest =
      runtime_dry_run_request_digest(gate.decision.runtime_request);
  receipt.runtime_execution_request_digest =
      runtime_execution_request_digest(gate.decision.runtime_request);
  receipt.runtime_handoff_arguments_digest = handoff.arguments_digest;
  receipt.runtime_request_digest = receipt.runtime_preview_request_digest;
  receipt.runtime_response_digest = marshal_digest_for_text(
      "kikijyeba.marshal.runtime_response.v1", handoff.tool_result_json);
  receipt.policy_decision =
      gate.accepted && handoff.ok ? "accepted" : "refused";
  receipt.confirmation_status =
      gate.has(marshal_refusal_reason_t::missing_confirmation)
          ? "missing"
          : (gate.has(marshal_refusal_reason_t::confirmation_mismatch)
                 ? "mismatch"
                 : "accepted");
  receipt.refusal_reasons = refusal_reason_texts(gate.refusal_reasons);
  receipt.runtime_tool_result_json = handoff.tool_result_json;
  receipt.post_run_verification =
      "ask Lattice Hero to re-read runtime evidence after Runtime Hero writes "
      "artifacts";
  finalize_receipt_id(&receipt);
  return receipt;
}

[[nodiscard]] inline marshal_receipt_replay_audit_t
replay_dispatch_receipt(const marshal_dispatch_receipt_t &receipt,
                        const marshal_active_identity_t &active_identity) {
  marshal_receipt_replay_audit_t audit{};
  audit.accepted = true;
  audit.explanation =
      "dispatch receipt explains Marshal/Runtime handoff only; target "
      "satisfaction requires Lattice Hero replay over runtime evidence";
  if (receipt.schema_version != k_marshal_dispatch_receipt_schema_v1) {
    audit.accepted = false;
    audit.stale = true;
    audit.issues.push_back("unsupported_schema_version");
  }
  if (receipt.non_authority_statement.find("target satisfaction remains") ==
      std::string::npos) {
    audit.accepted = false;
    audit.issues.push_back("missing_non_authority_statement");
  }
  if (receipt.lattice_proof_context.protocol_contract_fingerprint !=
      active_identity.protocol_contract_fingerprint) {
    audit.accepted = false;
    audit.stale = true;
    audit.issues.push_back("stale_protocol_contract_fingerprint");
  }
  if (receipt.lattice_proof_context.target_spec_fingerprint !=
      active_identity.target_spec_fingerprint) {
    audit.accepted = false;
    audit.stale = true;
    audit.issues.push_back("stale_target_spec_fingerprint");
  }
  if (receipt.lattice_proof_context.split_policy_fingerprint !=
      active_identity.split_policy_fingerprint) {
    audit.accepted = false;
    audit.stale = true;
    audit.issues.push_back("stale_split_policy_fingerprint");
  }
  if (receipt.receipt_id.empty()) {
    audit.accepted = false;
    audit.issues.push_back("missing_receipt_id");
  } else {
    auto expected_receipt = receipt;
    finalize_receipt_id(&expected_receipt);
    if (expected_receipt.receipt_id != receipt.receipt_id) {
      audit.accepted = false;
      audit.issues.push_back("receipt_id_mismatch");
    }
  }
  if (receipt.request_digest.empty()) {
    audit.accepted = false;
    audit.issues.push_back("missing_request_digest");
  }
  if (receipt.advice_digest.empty()) {
    audit.accepted = false;
    audit.issues.push_back("missing_advice_digest");
  }
  if (receipt.decision_digest.empty()) {
    audit.accepted = false;
    audit.issues.push_back("missing_decision_digest");
  }
  if (receipt.runtime_request_digest.empty() &&
      receipt.runtime_preview_request_digest.empty()) {
    audit.accepted = false;
    audit.issues.push_back("missing_runtime_request_digest");
  }
  if (receipt.runtime_response_digest.empty()) {
    audit.accepted = false;
    audit.issues.push_back("missing_runtime_response_digest");
  }
  if (receipt.retention_class != "full" &&
      receipt.retention_class != "compact" &&
      receipt.retention_class != "tombstone") {
    audit.accepted = false;
    audit.issues.push_back("unknown_retention_class");
  }
  return audit;
}

[[nodiscard]] inline marshal_dispatch_receipt_t
compact_dispatch_receipt(const marshal_dispatch_receipt_t &receipt) {
  marshal_dispatch_receipt_t compact = receipt;
  compact.retention_class = "compact";
  compact.runtime_tool_result_json.clear();
  compact.compacted_fields.push_back("runtime_tool_result_json");
  compact.post_run_verification =
      "compact receipt: consult runtime artifacts and Lattice Hero for proof; "
      "retained digests are audit metadata only";
  finalize_receipt_id(&compact);
  return compact;
}

inline void write_dispatch_receipt(const std::filesystem::path &path,
                                   const marshal_dispatch_receipt_t &receipt) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream out(path, std::ios::trunc | std::ios::binary);
  if (!out) {
    throw std::runtime_error("failed to open Marshal dispatch receipt: " +
                             path.string());
  }
  out << canonical_dispatch_receipt_text(receipt)
      << "receipt_id=" << receipt.receipt_id << "\n";
  if (!out) {
    throw std::runtime_error("failed to write Marshal dispatch receipt: " +
                             path.string());
  }
}

} // namespace cuwacunu::hero::marshal
