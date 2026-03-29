#pragma once

#include <filesystem>
#include <string>

namespace cuwacunu {
namespace hero {
namespace super_mcp {

struct super_defaults_t {
  std::filesystem::path super_root{};
  std::filesystem::path repo_root{};
  std::filesystem::path config_scope_root{};
  std::filesystem::path campaign_grammar_path{};
  std::filesystem::path super_objective_grammar_path{};
  std::filesystem::path runtime_hero_binary{};
  std::filesystem::path config_hero_binary{};
  std::filesystem::path lattice_hero_binary{};
  std::filesystem::path human_hero_binary{};
  std::filesystem::path human_operator_identities{};
  std::filesystem::path super_codex_binary{};
  std::size_t tail_default_lines{120};
  std::size_t poll_interval_ms{1000};
  std::size_t super_codex_timeout_sec{900};
  std::size_t super_max_review_turns{8};
  std::size_t super_max_campaign_launches{8};
};

struct app_context_t {
  std::filesystem::path global_config_path{};
  std::filesystem::path hero_config_path{};
  std::filesystem::path self_binary_path{};
  super_defaults_t defaults{};
};

[[nodiscard]] std::filesystem::path resolve_super_hero_dsl_path(
    const std::filesystem::path& global_config_path);
[[nodiscard]] bool load_super_defaults(
    const std::filesystem::path& hero_dsl_path,
    const std::filesystem::path& global_config_path, super_defaults_t* out,
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

[[nodiscard]] bool run_loop_runner(app_context_t* app, std::string_view loop_id,
                                   std::string* error);

void run_jsonrpc_stdio_loop(app_context_t* app);

}  // namespace super_mcp
}  // namespace hero
}  // namespace cuwacunu
