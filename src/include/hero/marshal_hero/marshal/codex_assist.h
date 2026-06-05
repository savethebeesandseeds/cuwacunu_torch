// SPDX-License-Identifier: MIT
#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "hero/marshal_hero/marshal/dispatch_operation.h"
#include "hero/marshal_hero/marshal/execution_gate.h"

namespace cuwacunu::hero::marshal {

struct marshal_codex_assist_response_t {
  bool accepted{false};
  std::string prompt_text{};
  std::vector<std::string> operator_options{};
  marshal_dry_run_dispatch_response_t dry_run_response{};
  marshal_execution_gate_result_t execution_gate{};
  std::string deterministic_primitive{};
  std::string non_authority_statement{
      k_marshal_dispatch_non_authority_statement};
};

[[nodiscard]] inline marshal_codex_assist_response_t
codex_assisted_dry_run_explanation(
    const std::string &prompt_text,
    const std::vector<std::string> &operator_options,
    const marshal_dispatch_advice_t &advice,
    const marshal_dispatch_request_t &request,
    const marshal_dispatch_validation_context_t &context,
    const marshal_runtime_policy_snapshot_t &policy,
    const marshal_runtime_wave_snapshot_t &active_wave,
    const marshal_runtime_hero_handoff_options_t &handoff_options = {}) {
  marshal_codex_assist_response_t out{};
  out.prompt_text = prompt_text;
  out.operator_options = operator_options;
  out.deterministic_primitive = "run_marshal_dry_run_dispatch";
  out.dry_run_response = run_marshal_dry_run_dispatch(
      advice, request, context, policy, active_wave, handoff_options);
  out.accepted = out.dry_run_response.accepted;
  return out;
}

[[nodiscard]] inline marshal_codex_assist_response_t
codex_assisted_execution_confirmation_prompt(
    const std::string &prompt_text,
    const std::vector<std::string> &operator_options,
    const marshal_execution_gate_input_t &input) {
  marshal_codex_assist_response_t out{};
  out.prompt_text = prompt_text;
  out.operator_options = operator_options;
  out.deterministic_primitive = "validate_execution_gate";
  out.execution_gate = validate_execution_gate(input);
  out.accepted = out.execution_gate.accepted;
  return out;
}

[[nodiscard]] inline std::string canonical_codex_assist_response_text(
    const marshal_codex_assist_response_t &response) {
  std::ostringstream out;
  detail::append_kv(out, "accepted", detail::bool_text(response.accepted));
  detail::append_kv(out, "prompt_text", response.prompt_text);
  detail::append_string_vector(out, "operator_options",
                               response.operator_options);
  detail::append_kv(out, "deterministic_primitive",
                    response.deterministic_primitive);
  detail::append_kv(out, "non_authority_statement",
                    response.non_authority_statement);
  detail::append_kv(out, "dry_run_response_digest",
                    response.dry_run_response.response_digest);
  detail::append_kv(out, "execution_expected_confirmation",
                    response.execution_gate.expected_confirmation_token);
  return out.str();
}

} // namespace cuwacunu::hero::marshal
