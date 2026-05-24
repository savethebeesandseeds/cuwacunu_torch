// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "hero/mcp_schema_compat.h"

namespace cuwacunu::kikijyeba::marshal {

struct marshal_tool_descriptor_t {
  std::string_view name;
  std::string_view description;
  std::string_view input_schema_json;
};

inline constexpr std::array<marshal_tool_descriptor_t, 8> kMarshalTools = {{
    {"hero.marshal.status",
     "Summarize Marshal receipts, policy surface, and advice "
     "availability.",
     R"({"type":"object","properties":{"receipt_root":{"type":"string"},"limit":{"type":"integer"},"receipts":{"type":"array","items":{"type":"object"}},"runtime_policy":{"type":"object"},"lattice_advice_surface_available":{"type":"boolean"}},"additionalProperties":false})"},
    {"hero.marshal.lookup_target_advice",
     "Ask Lattice Hero for one target plan and materialize "
     "explicit Marshal "
     "dispatch advice without dispatching.",
     R"({"type":"object","properties":{"target_id":{"type":"string"},"config_path":{"type":"string"},"runtime_root":{"type":"string"},"requested_mode":{"type":"string"},"source_lattice_timestamp":{"type":"string"},"max_waves":{"type":"integer"},"recommendation_attempt_count":{"type":"integer"},"context":{"type":"object"},"protocol_contract_fingerprint":{"type":"string"},"graph_order_fingerprint":{"type":"string"},"source_cursor_token":{"type":"string"},"vicreg_assembly_fingerprint":{"type":"string"},"mdn_assembly_fingerprint":{"type":"string"}},"required":["target_id"],"additionalProperties":false})"},
    {"hero.marshal.prepare_target_dispatch",
     "Prepare one target-first operator packet from a Lattice plan, "
     "read-only latest_satisfying resolution, Marshal validation, "
     "and Runtime wave/policy inspection without execution by "
     "default.",
     R"({"type":"object","properties":{"target_id":{"type":"string"},"config_path":{"type":"string"},"runtime_root":{"type":"string"},"requested_mode":{"type":"string"},"source_lattice_timestamp":{"type":"string"},"max_waves":{"type":"integer"},"recommendation_attempt_count":{"type":"integer"},"context":{"type":"object"},"protocol_contract_fingerprint":{"type":"string"},"graph_order_fingerprint":{"type":"string"},"source_cursor_token":{"type":"string"},"vicreg_assembly_fingerprint":{"type":"string"},"mdn_assembly_fingerprint":{"type":"string"},"materialize_plan_inputs":{"type":"boolean"},"include_runtime_dry_run":{"type":"boolean"},"include_machine_payload":{"type":"boolean"},"runtime_policy":{"type":"object"},"runtime_wave":{"type":"object"},"timeout_seconds":{"type":"integer"}},"required":["target_id"],"additionalProperties":false})"},
    {"hero.marshal.validate_advice",
     "Validate explicit Lattice advice without dispatching Runtime "
     "Hero.",
     R"({"type":"object","properties":{"advice":{"type":"object"},"request":{"type":"object"},"context":{"type":"object"}},"required":["advice","request"],"additionalProperties":false})"},
    {"hero.marshal.dry_run_dispatch",
     "Run deterministic Marshal dry-run dispatch for explicit "
     "Lattice advice.",
     R"({"type":"object","properties":{"advice":{"type":"object"},"request":{"type":"object"},"context":{"type":"object"},"runtime_policy":{"type":"object"},"runtime_wave":{"type":"object"},"timeout_seconds":{"type":"integer"}},"required":["advice","request"],"additionalProperties":false})"},
    {"hero.marshal.execution_gate",
     "Validate execution policy and confirmation token without "
     "scheduling.",
     R"({"type":"object","properties":{"advice":{"type":"object"},"request":{"type":"object"},"context":{"type":"object"},"prior_dry_run":{"type":"object"},"runtime_policy":{"type":"object"},"runtime_wave":{"type":"object"}},"required":["advice","request","prior_dry_run"],"additionalProperties":false})"},
    {"hero.marshal.replay_receipt",
     "Replay a Marshal dispatch receipt as audit metadata.",
     R"({"type":"object","properties":{"receipt":{"type":"object"},"active_identity":{"type":"object"}},"required":["receipt","active_identity"],"additionalProperties":false})"},
    {"hero.marshal.batch_preview",
     "Preview independent dry-run dispatches without execution "
     "order "
     "inference.",
     R"({"type":"object","properties":{"items":{"type":"array","items":{"type":"object"}},"context":{"type":"object"},"runtime_policy":{"type":"object"},"timeout_seconds":{"type":"integer"}},"required":["items"],"additionalProperties":false})"},
}};

[[nodiscard]] inline std::vector<std::string> marshal_tool_names() {
  std::vector<std::string> out;
  out.reserve(kMarshalTools.size());
  for (const auto &tool : kMarshalTools) {
    out.emplace_back(tool.name);
  }
  return out;
}

[[nodiscard]] inline std::vector<std::string> marshal_cli_tool_names() {
  return marshal_tool_names();
}

[[nodiscard]] inline std::vector<std::string> marshal_mcp_tool_names() {
  return marshal_tool_names();
}

[[nodiscard]] inline std::vector<
    cuwacunu::hero::mcp_schema_compat::mcp_schema_compat_issue_t>
validate_marshal_tool_schemas() {
  std::vector<cuwacunu::hero::mcp_schema_compat::mcp_schema_compat_issue_t>
      issues;
  for (const auto &tool : kMarshalTools) {
    const auto tool_issues =
        cuwacunu::hero::mcp_schema_compat::validate_tool_input_schema(
            tool.name, tool.input_schema_json);
    issues.insert(issues.end(), tool_issues.begin(), tool_issues.end());
  }
  return issues;
}

[[nodiscard]] inline bool direct_cli_and_mcp_expose_same_primitives() {
  auto cli = marshal_cli_tool_names();
  auto mcp = marshal_mcp_tool_names();
  std::sort(cli.begin(), cli.end());
  std::sort(mcp.begin(), mcp.end());
  return cli == mcp;
}

} // namespace cuwacunu::kikijyeba::marshal
