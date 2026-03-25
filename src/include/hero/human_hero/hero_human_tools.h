// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <string>

namespace cuwacunu {
namespace hero {
namespace human_mcp {

struct human_defaults_t {
  std::filesystem::path super_root{};
  std::filesystem::path super_hero_binary{};
  std::string operator_id{};
  std::filesystem::path operator_signing_ssh_identity{};
};

struct app_context_t {
  std::filesystem::path global_config_path{};
  std::filesystem::path hero_config_path{};
  std::filesystem::path self_binary_path{};
  human_defaults_t defaults{};
};

[[nodiscard]] std::filesystem::path resolve_human_hero_dsl_path(
    const std::filesystem::path& global_config_path);
[[nodiscard]] bool load_human_defaults(
    const std::filesystem::path& hero_dsl_path,
    const std::filesystem::path& global_config_path, human_defaults_t* out,
    std::string* error);
[[nodiscard]] std::filesystem::path current_executable_path();

[[nodiscard]] bool execute_tool_json(const std::string& tool_name,
                                     std::string arguments_json,
                                     app_context_t* app,
                                     std::string* out_tool_result_json,
                                     std::string* out_error_message);
[[nodiscard]] bool tool_result_is_error(std::string_view tool_result_json);

[[nodiscard]] std::string build_tools_list_result_json();
[[nodiscard]] std::string build_tools_list_human_text();

[[nodiscard]] bool run_interactive_request_responder(app_context_t* app,
                                                     std::string* error);

void run_jsonrpc_stdio_loop(app_context_t* app);

}  // namespace human_mcp
}  // namespace hero
}  // namespace cuwacunu
