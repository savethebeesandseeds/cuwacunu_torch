// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace cuwacunu::hero::environment {

struct environment_policy_t {
  std::filesystem::path policy_path{};
  std::filesystem::path global_config_path{};
  std::string profile_id{"operator_default"};
  std::unordered_map<std::string, std::string> values{};
  bool from_template{false};
};

struct environment_context_t {
  std::filesystem::path global_config_path{};
  std::filesystem::path policy_path{};
  environment_policy_t policy{};
};

[[nodiscard]] std::filesystem::path resolve_environment_hero_dsl_path(
    const std::filesystem::path &global_config_path);

[[nodiscard]] bool
load_environment_policy(const std::filesystem::path &policy_path,
                        const std::filesystem::path &global_config,
                        environment_policy_t *out, std::string *error,
                        std::string_view profile_override = {});

[[nodiscard]] bool execute_tool_json(const std::string &tool_name,
                                     std::string arguments_json,
                                     environment_context_t *ctx,
                                     std::string *out_tool_result_json,
                                     std::string *out_error_message);

[[nodiscard]] inline bool
tool_result_is_error(std::string_view tool_result_json) {
  return tool_result_json.find("\"isError\":true") != std::string_view::npos ||
         tool_result_json.find("\"isError\": true") != std::string_view::npos;
}

[[nodiscard]] std::string build_tools_list_result_json();
[[nodiscard]] std::string build_tools_list_human_text();

void run_jsonrpc_stdio_loop(environment_context_t *ctx);

} // namespace cuwacunu::hero::environment
