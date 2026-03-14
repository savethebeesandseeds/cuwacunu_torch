#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

namespace cuwacunu {
namespace hero {
namespace runtime_mcp {

struct runtime_defaults_t {
  std::filesystem::path jobs_root{};
  std::filesystem::path main_board_binary{};
  std::filesystem::path config_folder{};
  std::size_t tail_default_lines{120};
  std::size_t max_active_jobs{1};
};

struct app_context_t {
  std::filesystem::path global_config_path{};
  std::filesystem::path hero_config_path{};
  std::filesystem::path self_binary_path{};
  runtime_defaults_t defaults{};
};

[[nodiscard]] std::filesystem::path resolve_runtime_hero_dsl_path(
    const std::filesystem::path& global_config_path);
[[nodiscard]] bool load_runtime_defaults(
    const std::filesystem::path& hero_dsl_path, runtime_defaults_t* out,
    std::string* error);
[[nodiscard]] std::filesystem::path current_executable_path();

[[nodiscard]] bool execute_tool_json(const std::string& tool_name,
                                     std::string arguments_json,
                                     app_context_t* app,
                                     std::string* out_tool_result_json,
                                     std::string* out_error_message);

[[nodiscard]] std::string build_tools_list_result_json();
[[nodiscard]] std::string build_tools_list_human_text();

void run_jsonrpc_stdio_loop(app_context_t* app);

}  // namespace runtime_mcp
}  // namespace hero
}  // namespace cuwacunu
