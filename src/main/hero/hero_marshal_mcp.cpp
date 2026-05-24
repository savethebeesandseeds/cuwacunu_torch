#include "kikijyeba/marshal/tool_handler.h"

#include "hero/lattice_hero/hero_lattice_tools.h"

#include <filesystem>
#include <iostream>
#include <string>

#include <unistd.h>

namespace {

std::filesystem::path g_global_config_path{"/cuwacunu/src/config/.config"};

bool call_lattice_tool(const std::string &tool_name,
                       const std::string &arguments_json,
                       std::string *out_tool_result_json,
                       std::string *out_error_message) {
  static cuwacunu::hero::lattice::lattice_context_t context{};
  static std::filesystem::path initialized_global_config{};
  static bool initialized = false;

  if (!initialized || initialized_global_config != g_global_config_path) {
    context = cuwacunu::hero::lattice::lattice_context_t{};
    context.global_config_path = g_global_config_path;
    context.policy_path =
        cuwacunu::hero::lattice::resolve_lattice_hero_dsl_path(
            g_global_config_path);
    std::string error;
    if (!cuwacunu::hero::lattice::load_lattice_policy(
            context.policy_path, context.global_config_path, &context.policy,
            &error)) {
      if (out_error_message) {
        *out_error_message = error;
      }
      return false;
    }
    initialized_global_config = g_global_config_path;
    initialized = true;
  }

  return cuwacunu::hero::lattice::execute_tool_json(
      tool_name, arguments_json, &context, out_tool_result_json,
      out_error_message);
}

void print_help(const char *argv0) {
  std::cout << "Usage: " << argv0 << " [options]\n"
            << "  --global-config PATH   accepted for Hero CLI symmetry\n"
            << "  --tool NAME            run one Marshal tool directly\n"
            << "  --args-json JSON       arguments for --tool\n"
            << "  --list-tools           print registered Marshal tools\n"
            << "  --list-tools-json      print MCP tools/list result JSON\n"
            << "  --help                 show this help\n";
}

} // namespace

int main(int argc, char **argv) {
  bool direct_tool_mode = false;
  bool direct_tool_args_overridden = false;
  bool list_tools = false;
  bool list_tools_json = false;
  std::string direct_tool_name;
  std::string direct_tool_args_json = "{}";

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--global-config" && i + 1 < argc) {
      g_global_config_path = argv[++i];
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
    if (arg == "--help" || arg == "-h") {
      print_help(argv[0]);
      return 0;
    }
    std::cerr << "Unknown argument: " << arg << "\n";
    print_help(argv[0]);
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
  if ((list_tools || list_tools_json) && direct_tool_mode) {
    std::cerr << "--list-tools cannot be combined with --tool\n";
    return 2;
  }

  cuwacunu::kikijyeba::marshal::set_marshal_lattice_tool_callback(
      call_lattice_tool);

  if (list_tools_json) {
    std::cout
        << cuwacunu::kikijyeba::marshal::build_marshal_tools_list_result_json()
        << "\n";
    return 0;
  }
  if (list_tools) {
    std::cout
        << cuwacunu::kikijyeba::marshal::build_marshal_tools_list_human_text();
    return 0;
  }

  if (direct_tool_mode) {
    std::string result;
    std::string error;
    const bool ok = cuwacunu::kikijyeba::marshal::execute_marshal_tool_json(
        direct_tool_name, direct_tool_args_json, &result, &error);
    if (!result.empty()) {
      std::cout << result << "\n";
    }
    if (!ok && !error.empty()) {
      std::cerr << "tool execution failed: " << error << "\n";
    }
    return ok ? 0 : 1;
  }

  if (::isatty(STDIN_FILENO) != 0) {
    std::cerr << "[hero_marshal_mcp] no --tool provided and stdin is a "
                 "terminal; default mode expects JSON-RPC input on stdin.\n";
    print_help(argv[0]);
    return 2;
  }

  cuwacunu::kikijyeba::marshal::run_marshal_jsonrpc_stdio_loop();
  return 0;
}
