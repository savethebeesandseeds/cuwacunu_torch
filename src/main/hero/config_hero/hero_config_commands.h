#pragma once

#include <string>

#include "main/hero/config_hero/hero_config_store.h"

namespace cuwacunu {
namespace hero {
namespace mcp {

void emit_startup_banner(const hero_config_store_t& store);

[[nodiscard]] bool execute_command_line(std::string line,
                                        hero_config_store_t* store,
                                        bool* should_exit);

void run_jsonrpc_stdio_loop(hero_config_store_t* store);

}  // namespace mcp
}  // namespace hero
}  // namespace cuwacunu
