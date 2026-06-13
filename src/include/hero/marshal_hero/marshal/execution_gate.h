// SPDX-License-Identifier: MIT
#pragma once

#include <sstream>
#include <string>

#include "hero/marshal_hero/marshal/dispatch_operation.h"

namespace cuwacunu::hero::marshal {

struct marshal_prior_dry_run_evidence_t {
  bool present{false};
  bool accepted{false};
  std::string advice_digest{};
  std::string dispatch_request_identity_digest{};
  std::string runtime_preview_request_digest{};
  std::string suggested_wave_digest{};
  std::string plan_input_digest{};
  std::string runtime_policy_digest{};
  std::string runtime_wave_digest{};
  std::string runtime_response_digest{};
  std::string dry_run_response_digest{};
  std::string evidence_digest{};
};

struct marshal_execution_gate_input_t {
  marshal_dispatch_advice_t advice{};
  marshal_dispatch_request_t request{};
  marshal_dispatch_validation_context_t validation_context{};
  marshal_runtime_policy_snapshot_t policy{};
  marshal_runtime_wave_snapshot_t active_wave{};
  marshal_prior_dry_run_evidence_t prior_dry_run{};
};

struct marshal_execution_gate_result_t {
  bool accepted{false};
  std::string expected_confirmation_token{};
  std::vector<marshal_refusal_reason_t> refusal_reasons{};
  std::vector<std::string> messages{};
  marshal_dispatch_decision_t decision{};
  std::string non_authority_statement{
      k_marshal_dispatch_non_authority_statement};

  [[nodiscard]] bool has(marshal_refusal_reason_t reason) const {
    return std::find(refusal_reasons.begin(), refusal_reasons.end(), reason) !=
           refusal_reasons.end();
  }
};

inline void add_refusal(marshal_execution_gate_result_t &result,
                        marshal_refusal_reason_t reason,
                        const std::string &message) {
  if (!result.has(reason)) {
    result.refusal_reasons.push_back(reason);
  }
  result.messages.push_back(std::string(to_string(reason)) + ": " + message);
}

[[nodiscard]] inline std::string canonical_prior_dry_run_evidence_text(
    const marshal_prior_dry_run_evidence_t &evidence) {
  std::ostringstream out;
  detail::append_kv(out, "present", detail::bool_text(evidence.present));
  detail::append_kv(out, "accepted", detail::bool_text(evidence.accepted));
  detail::append_kv(out, "advice_digest", evidence.advice_digest);
  detail::append_kv(out, "dispatch_request_identity_digest",
                    evidence.dispatch_request_identity_digest);
  detail::append_kv(out, "runtime_preview_request_digest",
                    evidence.runtime_preview_request_digest);
  detail::append_kv(out, "suggested_wave_digest",
                    evidence.suggested_wave_digest);
  detail::append_kv(out, "plan_input_digest", evidence.plan_input_digest);
  detail::append_kv(out, "runtime_policy_digest",
                    evidence.runtime_policy_digest);
  detail::append_kv(out, "runtime_wave_digest", evidence.runtime_wave_digest);
  detail::append_kv(out, "runtime_response_digest",
                    evidence.runtime_response_digest);
  detail::append_kv(out, "dry_run_response_digest",
                    evidence.dry_run_response_digest);
  return out.str();
}

[[nodiscard]] inline std::string prior_dry_run_evidence_digest(
    const marshal_prior_dry_run_evidence_t &evidence) {
  return marshal_digest_for_text(
      "kikijyeba.marshal.prior_dry_run_evidence.v1",
      canonical_prior_dry_run_evidence_text(evidence));
}

[[nodiscard]] inline marshal_prior_dry_run_evidence_t
make_prior_dry_run_evidence(
    const marshal_dispatch_advice_t &advice,
    const marshal_dispatch_request_t &dry_run_request,
    const marshal_dry_run_dispatch_response_t &response,
    const marshal_runtime_policy_snapshot_t &policy,
    const marshal_runtime_wave_snapshot_t &active_wave) {
  marshal_prior_dry_run_evidence_t evidence{};
  evidence.present = true;
  evidence.accepted = response.accepted;
  evidence.advice_digest = response.advice_digest.empty()
                               ? dispatch_advice_digest(advice)
                               : response.advice_digest;
  evidence.dispatch_request_identity_digest =
      dispatch_request_identity_digest(dry_run_request);
  evidence.runtime_preview_request_digest = response.runtime_request_digest;
  evidence.suggested_wave_digest = suggested_wave_digest(advice.suggested_wave);
  evidence.plan_input_digest =
      plan_input_digest(advice.suggested_wave.plan_inputs);
  evidence.runtime_policy_digest = runtime_policy_snapshot_digest(policy);
  evidence.runtime_wave_digest = runtime_wave_snapshot_digest(active_wave);
  evidence.runtime_response_digest =
      marshal_digest_for_text("kikijyeba.marshal.runtime_response.v1",
                              response.runtime_handoff.tool_result_json);
  evidence.dry_run_response_digest =
      response.response_digest.empty()
          ? dry_run_dispatch_response_digest(response)
          : response.response_digest;
  evidence.evidence_digest = prior_dry_run_evidence_digest(evidence);
  return evidence;
}

[[nodiscard]] inline std::string
confirmation_token_basis_text(const marshal_dispatch_advice_t &advice,
                              const marshal_dispatch_request_t &request) {
  std::ostringstream out;
  detail::append_kv(out, "domain", "marshal_confirmation_v1");
  detail::append_kv(out, "target_id", advice.target_id);
  detail::append_kv(out, "runtime_root",
                    detail::normalize_path_text(advice.runtime_root));
  detail::append_kv(out, "protocol_contract_fingerprint",
                    advice.active_identity.protocol_contract_fingerprint);
  detail::append_kv(out, "target_spec_fingerprint",
                    advice.active_identity.target_spec_fingerprint);
  detail::append_kv(out, "split_policy_fingerprint",
                    advice.active_identity.split_policy_fingerprint);
  detail::append_kv(out, "suggested_wave_digest",
                    suggested_wave_digest(advice.suggested_wave));
  detail::append_kv(out, "plan_input_digest",
                    plan_input_digest(advice.suggested_wave.plan_inputs));
  detail::append_kv(out, "requested_mode", to_string(request.requested_mode));
  return out.str();
}

[[nodiscard]] inline std::string
confirmation_token(const marshal_dispatch_advice_t &advice,
                   const marshal_dispatch_request_t &request) {
  return marshal_digest_for_text(
      "kikijyeba.marshal.confirmation.v1",
      confirmation_token_basis_text(advice, request));
}

[[nodiscard]] inline marshal_execution_gate_result_t
validate_execution_gate(const marshal_execution_gate_input_t &input) {
  marshal_execution_gate_result_t result{};
  result.expected_confirmation_token =
      confirmation_token(input.advice, input.request);

  auto context = input.validation_context;
  context.allow_execute_mode = true;
  result.decision = build_runtime_dry_run_dispatch_preview(
      input.advice, input.request, context, input.policy, input.active_wave);

  for (const auto reason : result.decision.refusal_reasons) {
    add_refusal(result, reason,
                "M1/M2 validation refused execution gate input");
  }

  if (input.request.requested_mode != marshal_dispatch_mode_t::execute) {
    add_refusal(result, marshal_refusal_reason_t::forbidden_dispatch_mode,
                "M3 execution gate requires requested_mode=execute");
  }
  if (!input.policy.allow_execute) {
    add_refusal(result, marshal_refusal_reason_t::runtime_policy_refused,
                "Runtime Hero allow_execute is false");
  }
  if (input.active_wave.train_target && !input.policy.allow_train_execute) {
    add_refusal(result, marshal_refusal_reason_t::runtime_policy_refused,
                "Runtime Hero allow_train_execute is false for train wave");
  }
  if (input.request.operator_confirmation_token.empty()) {
    add_refusal(result, marshal_refusal_reason_t::missing_confirmation,
                "execution requires a confirmation token");
  } else if (input.request.operator_confirmation_token !=
             result.expected_confirmation_token) {
    add_refusal(result, marshal_refusal_reason_t::confirmation_mismatch,
                "confirmation token does not bind this exact dispatch request");
  }
  const auto expected_runtime_preview_digest =
      runtime_dry_run_request_digest(result.decision.runtime_request);
  const auto expected_request_identity_digest =
      dispatch_request_identity_digest(input.request);
  const auto expected_suggested_wave_digest =
      suggested_wave_digest(input.advice.suggested_wave);
  const auto expected_plan_input_digest =
      plan_input_digest(input.advice.suggested_wave.plan_inputs);
  const auto expected_policy_digest =
      runtime_policy_snapshot_digest(input.policy);
  const auto expected_wave_digest =
      runtime_wave_snapshot_digest(input.active_wave);

  if (!input.prior_dry_run.present) {
    add_refusal(result, marshal_refusal_reason_t::dry_run_receipt_missing,
                "execution requires structured prior dry-run evidence");
  } else {
    if (!input.prior_dry_run.accepted) {
      add_refusal(result, marshal_refusal_reason_t::dry_run_receipt_mismatch,
                  "prior dry-run evidence was not accepted");
    }
    if (input.prior_dry_run.advice_digest !=
        result.decision.advice_validation.advice_digest) {
      add_refusal(result, marshal_refusal_reason_t::dry_run_receipt_mismatch,
                  "prior dry-run advice digest differs from execution advice");
    }
    if (input.prior_dry_run.dispatch_request_identity_digest !=
        expected_request_identity_digest) {
      add_refusal(result, marshal_refusal_reason_t::dry_run_receipt_mismatch,
                  "prior dry-run request identity differs from execution "
                  "request");
    }
    if (input.prior_dry_run.runtime_preview_request_digest !=
        expected_runtime_preview_digest) {
      add_refusal(result, marshal_refusal_reason_t::dry_run_receipt_mismatch,
                  "prior dry-run runtime request differs from execution "
                  "preview request");
    }
    if (input.prior_dry_run.suggested_wave_digest !=
        expected_suggested_wave_digest) {
      add_refusal(result, marshal_refusal_reason_t::dry_run_receipt_mismatch,
                  "prior dry-run suggested wave differs from execution wave");
    }
    if (input.prior_dry_run.plan_input_digest != expected_plan_input_digest) {
      add_refusal(result, marshal_refusal_reason_t::dry_run_receipt_mismatch,
                  "prior dry-run plan inputs differ from execution inputs");
    }
    if (input.prior_dry_run.runtime_policy_digest != expected_policy_digest) {
      add_refusal(result, marshal_refusal_reason_t::dry_run_receipt_mismatch,
                  "prior dry-run Runtime Hero policy snapshot differs");
    }
    if (input.prior_dry_run.runtime_wave_digest != expected_wave_digest) {
      add_refusal(result, marshal_refusal_reason_t::dry_run_receipt_mismatch,
                  "prior dry-run Runtime Hero active wave snapshot differs");
    }
    if (input.prior_dry_run.runtime_response_digest.empty() ||
        input.prior_dry_run.dry_run_response_digest.empty()) {
      add_refusal(result, marshal_refusal_reason_t::dry_run_receipt_mismatch,
                  "prior dry-run evidence is missing response digests");
    }
    if (!marshal_digest_is_strong_hex(
            input.prior_dry_run.runtime_response_digest) ||
        !marshal_digest_is_strong_hex(
            input.prior_dry_run.dry_run_response_digest) ||
        !marshal_digest_is_strong_hex(input.prior_dry_run.evidence_digest)) {
      add_refusal(result, marshal_refusal_reason_t::dry_run_receipt_mismatch,
                  "prior dry-run evidence digests must be SHA-256 hex digests");
    } else if (input.prior_dry_run.evidence_digest !=
               prior_dry_run_evidence_digest(input.prior_dry_run)) {
      add_refusal(result, marshal_refusal_reason_t::dry_run_receipt_mismatch,
                  "prior dry-run evidence digest does not match the structured "
                  "dry-run evidence");
    }
  }

  result.accepted = result.refusal_reasons.empty();
  return result;
}

[[nodiscard]] inline marshal_runtime_hero_handoff_result_t
call_runtime_hero_execution(
    const marshal_execution_gate_result_t &gate,
    const marshal_runtime_hero_handoff_options_t &options = {}) {
  if (!gate.accepted || !gate.decision.accepted ||
      !gate.decision.runtime_handoff_available) {
    marshal_runtime_hero_handoff_result_t out{};
    out.arguments_json = runtime_hero_execute_args_json(
        gate.decision.runtime_request, false);
    out.arguments_digest = marshal_digest_for_text(
        "kikijyeba.marshal.runtime_handoff_arguments.v1", out.arguments_json);
    out.error_message =
        "Marshal execution gate is not accepted for Runtime Hero execution";
    return out;
  }
  return detail::call_runtime_hero_execute_request(
      gate.decision.runtime_request, options, false);
}

} // namespace cuwacunu::hero::marshal
