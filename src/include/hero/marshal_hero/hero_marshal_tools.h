#pragma once

#include <string>
#include <string_view>

#include "hero/marshal_hero/hero_marshal.h"
#include "hero/marshal_hero/marshal/tool_handler.h"

namespace cuwacunu::hero::marshal {

[[nodiscard]] inline bool execute_tool_json(const std::string &tool_name,
                                            const std::string &arguments_json,
                                            std::string *out_tool_result_json,
                                            std::string *out_error_message) {
  return cuwacunu::hero::marshal::execute_marshal_tool_json(
      tool_name, arguments_json, out_tool_result_json, out_error_message);
}

[[nodiscard]] inline bool tool_result_is_error(std::string_view result_json) {
  return result_json.find("\"isError\":true") != std::string_view::npos ||
         result_json.find("\"isError\": true") != std::string_view::npos;
}

[[nodiscard]] inline std::string build_tools_list_result_json() {
  return cuwacunu::hero::marshal::build_marshal_tools_list_result_json();
}

[[nodiscard]] inline std::string build_tools_list_human_text() {
  return cuwacunu::hero::marshal::build_marshal_tools_list_human_text();
}

inline void run_jsonrpc_stdio_loop() {
  cuwacunu::hero::marshal::run_marshal_jsonrpc_stdio_loop();
}

} // namespace cuwacunu::hero::marshal
