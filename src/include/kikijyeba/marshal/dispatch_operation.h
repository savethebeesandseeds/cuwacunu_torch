// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <sstream>
#include <string>

#include "kikijyeba/marshal/runtime_hero_handoff.h"

namespace cuwacunu::kikijyeba::marshal {

struct marshal_dry_run_dispatch_response_t {
  bool accepted{false};
  bool target_satisfaction_claimed{false};
  bool runtime_dry_run_output_included{false};
  std::string advice_digest{};
  std::string request_digest{};
  std::string runtime_request_digest{};
  marshal_dispatch_decision_t decision{};
  marshal_runtime_hero_handoff_result_t runtime_handoff{};
  std::string response_digest{};
  std::string non_authority_statement{
      k_marshal_dispatch_non_authority_statement};
};

[[nodiscard]] inline std::string canonical_dry_run_dispatch_response_text(
    const marshal_dry_run_dispatch_response_t &response) {
  std::ostringstream out;
  detail::append_kv(out, "accepted", detail::bool_text(response.accepted));
  detail::append_kv(out, "target_satisfaction_claimed",
                    detail::bool_text(response.target_satisfaction_claimed));
  detail::append_kv(
      out, "runtime_dry_run_output_included",
      detail::bool_text(response.runtime_dry_run_output_included));
  detail::append_kv(out, "advice_digest", response.advice_digest);
  detail::append_kv(out, "request_digest", response.request_digest);
  detail::append_kv(out, "runtime_request_digest",
                    response.runtime_request_digest);
  detail::append_kv(out, "decision_accepted",
                    detail::bool_text(response.decision.accepted));
  detail::append_kv(
      out, "decision_runtime_handoff_available",
      detail::bool_text(response.decision.runtime_handoff_available));
  std::vector<std::string> refusal_text;
  refusal_text.reserve(response.decision.refusal_reasons.size());
  for (const auto reason : response.decision.refusal_reasons) {
    refusal_text.push_back(to_string(reason));
  }
  detail::append_string_vector(out, "refusal_reasons", refusal_text);
  detail::append_string_vector(out, "messages", response.decision.messages);
  detail::append_kv(out, "runtime_handoff_attempted",
                    detail::bool_text(response.runtime_handoff.attempted));
  detail::append_kv(out, "runtime_handoff_ok",
                    detail::bool_text(response.runtime_handoff.ok));
  detail::append_kv(out, "runtime_tool_name",
                    response.runtime_handoff.tool_name);
  detail::append_kv(out, "runtime_arguments_json",
                    response.runtime_handoff.arguments_json);
  detail::append_kv(out, "runtime_tool_result_json",
                    response.runtime_handoff.tool_result_json);
  detail::append_kv(out, "runtime_error_message",
                    response.runtime_handoff.error_message);
  detail::append_kv(out, "non_authority_statement",
                    k_marshal_dispatch_non_authority_statement);
  return out.str();
}

[[nodiscard]] inline std::string dry_run_dispatch_response_digest(
    const marshal_dry_run_dispatch_response_t &response) {
  return marshal_digest_for_text(
      "kikijyeba.marshal.dry_run_response.v1",
      canonical_dry_run_dispatch_response_text(response));
}

[[nodiscard]] inline marshal_dry_run_dispatch_response_t
run_marshal_dry_run_dispatch(
    const marshal_dispatch_advice_t &advice,
    const marshal_dispatch_request_t &request,
    const marshal_dispatch_validation_context_t &context,
    const marshal_runtime_policy_snapshot_t &policy,
    const marshal_runtime_wave_snapshot_t &active_wave,
    const marshal_runtime_hero_handoff_options_t &handoff_options = {}) {
  marshal_dry_run_dispatch_response_t response{};
  response.advice_digest = dispatch_advice_digest(advice);
  response.request_digest = dispatch_request_digest(request);
  response.decision = build_runtime_dry_run_dispatch_preview(
      advice, request, context, policy, active_wave);
  response.runtime_request_digest =
      runtime_dry_run_request_digest(response.decision.runtime_request);

  if (response.decision.accepted &&
      response.decision.runtime_handoff_available) {
    response.runtime_handoff =
        call_runtime_hero_dry_run(response.decision, handoff_options);
    response.runtime_dry_run_output_included =
        !response.runtime_handoff.tool_result_json.empty();
    if (!response.runtime_handoff.ok) {
      add_refusal(response.decision,
                  marshal_refusal_reason_t::runtime_policy_refused,
                  "Runtime Hero dry-run handoff did not return ok=true");
      response.decision.accepted = false;
      response.decision.runtime_handoff_available = false;
    }
  } else {
    response.runtime_handoff =
        call_runtime_hero_dry_run(response.decision, handoff_options);
  }

  response.accepted = response.decision.accepted &&
                      response.runtime_handoff.ok &&
                      response.runtime_dry_run_output_included &&
                      !response.target_satisfaction_claimed;
  response.response_digest = dry_run_dispatch_response_digest(response);
  return response;
}

} // namespace cuwacunu::kikijyeba::marshal
