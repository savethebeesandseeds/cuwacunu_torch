#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "hero/lattice_hero/lattice_catalog.h"

namespace cuwacunu {
namespace hero {
namespace lattice_mcp {

struct wave_runtime_defaults_t {
  std::filesystem::path store_root{};
  std::filesystem::path catalog_path{};
  std::filesystem::path config_folder{};
};

struct app_context_t {
  std::filesystem::path store_root{};
  std::filesystem::path config_folder{};
  std::filesystem::path lattice_catalog_path{};
  cuwacunu::hero::wave::lattice_catalog_store_t catalog{};
};

[[nodiscard]] std::filesystem::path default_store_root();
[[nodiscard]] std::filesystem::path default_catalog_path(
    const std::filesystem::path& store_root);
[[nodiscard]] std::filesystem::path resolve_lattice_hero_dsl_path(
    const std::filesystem::path& global_config_path);
[[nodiscard]] bool load_wave_runtime_defaults(
    const std::filesystem::path& hero_dsl_path, wave_runtime_defaults_t* out,
    std::string* error);

[[nodiscard]] bool execute_tool_json(const std::string& tool_name,
                                     std::string arguments_json,
                                     app_context_t* app,
                                     std::string* out_tool_result_json,
                                     std::string* out_error_message);
[[nodiscard]] bool tool_result_is_error(std::string_view tool_result_json);

[[nodiscard]] std::string build_tools_list_result_json();
[[nodiscard]] std::string build_tools_list_human_text();
void write_cli_stdout(std::string_view payload);
void run_jsonrpc_stdio_loop(app_context_t* app);

}  // namespace lattice_mcp
}  // namespace hero
}  // namespace cuwacunu
