#pragma once

#include <filesystem>
#include <string>

namespace cuwacunu {
namespace hero {
namespace jkimyei_mcp {

struct jkimyei_runtime_defaults_t {
  std::filesystem::path campaigns_root{};
  std::filesystem::path config_folder{};
  std::filesystem::path campaign_grammar_path{};
};

struct app_context_t {
  std::filesystem::path global_config_path{};
  std::filesystem::path hero_config_path{};
  jkimyei_runtime_defaults_t defaults{};
};

[[nodiscard]] std::filesystem::path resolve_jkimyei_hero_dsl_path(
    const std::filesystem::path& global_config_path);
[[nodiscard]] bool load_jkimyei_runtime_defaults(
    const std::filesystem::path& hero_dsl_path,
    jkimyei_runtime_defaults_t* out, std::string* error);

[[nodiscard]] bool execute_tool_json(const std::string& tool_name,
                                     std::string arguments_json,
                                     app_context_t* app,
                                     std::string* out_tool_result_json,
                                     std::string* out_error_message);
[[nodiscard]] bool tool_result_is_error(std::string_view tool_result_json);

[[nodiscard]] std::string build_tools_list_result_json();
[[nodiscard]] std::string build_tools_list_human_text();

void run_jsonrpc_stdio_loop(app_context_t* app);

}  // namespace jkimyei_mcp
}  // namespace hero
}  // namespace cuwacunu
