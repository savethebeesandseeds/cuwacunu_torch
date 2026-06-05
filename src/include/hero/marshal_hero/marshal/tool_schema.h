// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "hero/marshal_hero/hero_marshal.h"
#include "hero/mcp_schema_compat.h"

namespace cuwacunu::hero::marshal {

inline constexpr std::size_t kMarshalToolCount =
    cuwacunu::hero::marshal::kMarshalToolDescriptorCount;

inline constexpr std::array<marshal_tool_descriptor_t, kMarshalToolCount>
    kMarshalTools = {{
#define HERO_MARSHAL_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON)                \
  marshal_tool_descriptor_t{NAME, DESCRIPTION, INPUT_SCHEMA_JSON},
#include "hero/marshal_hero/hero_marshal.def"
#undef HERO_MARSHAL_TOOL
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

} // namespace cuwacunu::hero::marshal
