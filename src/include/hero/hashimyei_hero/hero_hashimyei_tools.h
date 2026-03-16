#pragma once

#include <chrono>
#include <filesystem>
#include <string>

#include "hero/hashimyei_hero/hashimyei_catalog.h"

namespace cuwacunu {
namespace hero {
namespace hashimyei_mcp {

struct hashimyei_runtime_defaults_t {
  std::filesystem::path store_root{};
  std::filesystem::path catalog_path{};
};

struct app_context_t {
  std::filesystem::path store_root{};
  std::filesystem::path lattice_catalog_path{};
  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t::options_t
      catalog_options{};
  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t catalog{};
  std::chrono::steady_clock::time_point last_auto_ingest_at{};
  bool auto_ingest_ready{false};
};

[[nodiscard]] std::filesystem::path resolve_hashimyei_hero_dsl_path(
    const std::filesystem::path& global_config_path);
[[nodiscard]] bool load_hashimyei_runtime_defaults(
    const std::filesystem::path& hero_dsl_path, hashimyei_runtime_defaults_t* out,
    std::string* error);
[[nodiscard]] std::filesystem::path default_store_root();
[[nodiscard]] std::filesystem::path default_catalog_path(
    const std::filesystem::path& store_root);

[[nodiscard]] bool execute_tool_json(const std::string& tool_name,
                                     std::string arguments_json,
                                     app_context_t* app,
                                     std::string* out_tool_result_json,
                                     std::string* out_error_message);
[[nodiscard]] bool tool_result_is_error(std::string_view tool_result_json);

[[nodiscard]] std::string build_tools_list_result_json();
[[nodiscard]] std::string build_tools_list_human_text();

void run_jsonrpc_stdio_loop(app_context_t* app);

}  // namespace hashimyei_mcp
}  // namespace hero
}  // namespace cuwacunu
