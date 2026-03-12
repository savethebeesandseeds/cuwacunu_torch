#pragma once

#include <string>

#include "hero/config_hero/hero_config_store.h"

namespace cuwacunu {
namespace hero {
namespace mcp {

void emit_startup_banner(const hero_config_store_t& store);

[[nodiscard]] bool execute_command_line(std::string line,
                                        hero_config_store_t* store,
                                        bool* should_exit);

[[nodiscard]] bool execute_tool_json(const std::string& tool_name,
                                     std::string arguments_json,
                                     hero_config_store_t* store,
                                     std::string* out_tool_result_json,
                                     std::string* out_error_message);

void run_jsonrpc_stdio_loop(hero_config_store_t* store);

}  // namespace mcp
}  // namespace hero
}  // namespace cuwacunu
