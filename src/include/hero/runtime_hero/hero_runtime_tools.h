#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cuwacunu::hero::runtime {

struct runtime_policy_t {
  std::filesystem::path policy_path{};
  std::filesystem::path global_config_path{};
  std::unordered_map<std::string, std::string> values{};
  bool from_template{false};
};

struct runtime_context_t {
  std::filesystem::path global_config_path{};
  std::filesystem::path policy_path{};
  runtime_policy_t policy{};
};

[[nodiscard]] std::filesystem::path
resolve_runtime_hero_dsl_path(const std::filesystem::path &global_config_path);

[[nodiscard]] bool
load_runtime_policy(const std::filesystem::path &policy_path,
                    const std::filesystem::path &global_config,
                    runtime_policy_t *out, std::string *error);

[[nodiscard]] bool execute_tool_json(const std::string &tool_name,
                                     std::string arguments_json,
                                     runtime_context_t *ctx,
                                     std::string *out_tool_result_json,
                                     std::string *out_error_message);

[[nodiscard]] inline bool
tool_result_is_error(std::string_view tool_result_json) {
  return tool_result_json.find("\"isError\":true") != std::string_view::npos ||
         tool_result_json.find("\"isError\": true") != std::string_view::npos;
}

[[nodiscard]] std::string build_tools_list_result_json();
[[nodiscard]] std::string build_tools_list_human_text();

void run_jsonrpc_stdio_loop(runtime_context_t *ctx);

} // namespace cuwacunu::hero::runtime
