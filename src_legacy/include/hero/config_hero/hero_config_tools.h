#pragma once

#include <string>

#include "hero/config_hero/hero_config_store.h"

namespace cuwacunu {
namespace hero {
namespace mcp {

[[nodiscard]] bool execute_tool_json(const std::string& tool_name,
                                     std::string arguments_json,
                                     hero_config_store_t* store,
                                     std::string* out_tool_result_json,
                                     std::string* out_error_message);

[[nodiscard]] std::string build_tools_list_result_json();
[[nodiscard]] std::string build_tools_list_human_text();

void run_jsonrpc_stdio_loop(hero_config_store_t* store);

}  // namespace mcp
}  // namespace hero
}  // namespace cuwacunu
