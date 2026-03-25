#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

#include "hero/super_hero/hero_super_tools.h"
#include "piaabo/dlogs.h"

namespace {

constexpr const char* kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";

__attribute__((constructor(101))) void disable_terminal_logs_pre_main() {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
}

void print_cli_help(const char* argv0) {
  std::cerr << "Usage: " << argv0
            << " [--global-config <path>] [--hero-config <path>]"
               " [--tool <name>] [--args-json <json>]"
               " [--list-tools] [--list-tools-json]"
               " [--loop-runner --loop-id <id>] [--help]\n";
}

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  std::size_t b = 0;
  while (b < in.size() &&
         std::isspace(static_cast<unsigned char>(in[b])) != 0) {
    ++b;
  }
  std::size_t e = in.size();
  while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1])) != 0) {
    --e;
  }
  return std::string(in.substr(b, e - b));
}

}  // namespace

int main(int argc, char** argv) {
  std::filesystem::path global_config_path = kDefaultGlobalConfigPath;
  std::filesystem::path hero_config_path{};
  bool hero_config_overridden = false;
  std::string direct_tool_name{};
  std::string direct_tool_args_json{"{}"};
  bool direct_tool_mode = false;
  bool direct_tool_args_overridden = false;
  bool list_tools = false;
  bool list_tools_json = false;
  bool loop_runner_mode = false;
  std::string loop_id{};

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--global-config" && i + 1 < argc) {
      global_config_path = argv[++i];
      continue;
    }
    if (arg == "--hero-config" && i + 1 < argc) {
      hero_config_path = argv[++i];
      hero_config_overridden = true;
      continue;
    }
    if (arg == "--tool" && i + 1 < argc) {
      direct_tool_name = argv[++i];
      direct_tool_mode = true;
      continue;
    }
    if (arg == "--args-json" && i + 1 < argc) {
      direct_tool_args_json = argv[++i];
      direct_tool_args_overridden = true;
      continue;
    }
    if (arg == "--list-tools") {
      list_tools = true;
      continue;
    }
    if (arg == "--list-tools-json") {
      list_tools_json = true;
      continue;
    }
    if (arg == "--loop-runner") {
      loop_runner_mode = true;
      continue;
    }
    if (arg == "--loop-id" && i + 1 < argc) {
      loop_id = argv[++i];
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      print_cli_help(argv[0]);
      return 0;
    }
    std::cerr << "Unknown argument: " << arg << "\n";
    print_cli_help(argv[0]);
    return 2;
  }

  if (direct_tool_args_overridden && !direct_tool_mode) {
    std::cerr << "--args-json requires --tool\n";
    return 2;
  }
  if (list_tools && list_tools_json) {
    std::cerr << "--list-tools and --list-tools-json are mutually exclusive\n";
    return 2;
  }
  if ((list_tools || list_tools_json || direct_tool_mode) && loop_runner_mode) {
    std::cerr << "--loop-runner cannot be combined with tool/list modes\n";
    return 2;
  }
  if ((list_tools || list_tools_json) && direct_tool_mode) {
    std::cerr << "--list-tools/--list-tools-json cannot be combined with --tool\n";
    return 2;
  }
  if (direct_tool_mode) {
    direct_tool_name = trim_ascii(direct_tool_name);
    if (direct_tool_name.empty()) {
      std::cerr << "--tool requires a non-empty name\n";
      return 2;
    }
    direct_tool_args_json = trim_ascii(direct_tool_args_json);
    if (direct_tool_args_json.empty()) direct_tool_args_json = "{}";
    if (direct_tool_args_json.front() != '{') {
      std::cerr << "--args-json must be a JSON object\n";
      return 2;
    }
  }
  if (loop_runner_mode) {
    loop_id = trim_ascii(loop_id);
    if (loop_id.empty()) {
      std::cerr << "--loop-runner requires --loop-id\n";
      return 2;
    }
  }

  if (!hero_config_overridden) {
    hero_config_path =
        cuwacunu::hero::super_mcp::resolve_super_hero_dsl_path(global_config_path);
  }
  if (hero_config_path.empty()) {
    std::cerr << "missing [REAL_HERO].super_hero_dsl_filename in "
              << global_config_path.string() << "\n";
    return 2;
  }

  cuwacunu::hero::super_mcp::app_context_t app{};
  app.global_config_path = global_config_path;
  app.hero_config_path = hero_config_path;
  app.self_binary_path = cuwacunu::hero::super_mcp::current_executable_path();
  std::string load_error{};
  if (!cuwacunu::hero::super_mcp::load_super_defaults(
          hero_config_path, global_config_path, &app.defaults, &load_error)) {
    std::cerr << load_error << "\n";
    return 2;
  }

  if (loop_runner_mode) {
    std::string runner_error{};
    if (!cuwacunu::hero::super_mcp::run_loop_runner(&app, loop_id, &runner_error)) {
      if (!runner_error.empty()) std::cerr << runner_error << "\n";
      return 1;
    }
    return 0;
  }

  if (list_tools_json) {
    std::cout << cuwacunu::hero::super_mcp::build_tools_list_result_json() << "\n";
    return 0;
  }
  if (list_tools) {
    std::cout << cuwacunu::hero::super_mcp::build_tools_list_human_text();
    return 0;
  }

  if (direct_tool_mode) {
    std::string tool_result_json{};
    std::string tool_error{};
    if (!cuwacunu::hero::super_mcp::execute_tool_json(
            direct_tool_name, direct_tool_args_json, &app, &tool_result_json,
            &tool_error)) {
      if (!tool_error.empty()) std::cerr << "tool execution failed: " << tool_error << "\n";
      return 1;
    }
    if (!tool_result_json.empty()) {
      std::cout << tool_result_json << "\n";
      std::cout.flush();
    }
    return cuwacunu::hero::super_mcp::tool_result_is_error(tool_result_json) ? 1
                                                                              : 0;
  }

  cuwacunu::hero::super_mcp::run_jsonrpc_stdio_loop(&app);
  return 0;
}
