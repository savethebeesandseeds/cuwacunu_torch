#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <unistd.h>

#include "hero/jkimyei_hero/hero_jkimyei_tools.h"
#include "piaabo/dlogs.h"

namespace {

constexpr const char* kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char* kServerName = "hero_jkimyei_mcp";

__attribute__((constructor(101))) void disable_terminal_logs_pre_main() {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
}

void print_help(const char* argv0) {
  std::cout << "Usage: " << argv0 << " [options]\n"
            << "Options:\n"
            << "  --global-config <path>   Global .config used to resolve [REAL_HERO].jkimyei_hero_dsl_filename\n"
            << "  --hero-config <path>     Explicit Jkimyei HERO defaults DSL\n"
            << "  --campaigns-root <path>  Override campaigns_root from HERO defaults DSL\n"
            << "  --config-folder <path>   Override config_folder from HERO defaults DSL\n"
            << "  --tool <name>            Execute one MCP tool and exit\n"
            << "  --args-json <json>       Tool arguments JSON object (default: {})\n"
            << "  --list-tools             Human-readable tool list\n"
            << "  --list-tools-json        Print MCP tools/list JSON and exit\n"
            << "  (without --tool, server mode reads JSON-RPC messages from stdin)\n"
            << "  --help                   Show this help\n";
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
  cuwacunu::hero::jkimyei_mcp::app_context_t app{};
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);

  std::filesystem::path global_config_path = kDefaultGlobalConfigPath;
  std::filesystem::path hero_config_path{};
  std::filesystem::path campaigns_root{};
  std::filesystem::path config_folder{};
  std::string direct_tool_name{};
  std::string direct_tool_args_json = "{}";
  bool direct_tool_mode = false;
  bool direct_tool_args_overridden = false;
  bool list_tools = false;
  bool list_tools_json = false;
  bool campaigns_root_overridden = false;
  bool config_folder_overridden = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--hero-config" && i + 1 < argc) {
      hero_config_path = argv[++i];
      continue;
    }
    if (arg == "--global-config" && i + 1 < argc) {
      global_config_path = argv[++i];
      continue;
    }
    if (arg == "--campaigns-root" && i + 1 < argc) {
      campaigns_root = argv[++i];
      campaigns_root_overridden = true;
      continue;
    }
    if (arg == "--config-folder" && i + 1 < argc) {
      config_folder = argv[++i];
      config_folder_overridden = true;
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

  if (list_tools_json) {
    std::cout << cuwacunu::hero::jkimyei_mcp::build_tools_list_result_json()
              << "\n";
    return 0;
  }
  if (list_tools) {
    std::cout << cuwacunu::hero::jkimyei_mcp::build_tools_list_human_text();
    return 0;
  }

  if (hero_config_path.empty()) {
    hero_config_path =
        cuwacunu::hero::jkimyei_mcp::resolve_jkimyei_hero_dsl_path(
            global_config_path);
  }

  cuwacunu::hero::jkimyei_mcp::jkimyei_runtime_defaults_t defaults{};
  std::string defaults_error{};
  if (!cuwacunu::hero::jkimyei_mcp::load_jkimyei_runtime_defaults(
          hero_config_path, &defaults, &defaults_error)) {
    std::cerr << "[" << kServerName << "] " << defaults_error << "\n";
    return 2;
  }

  if (campaigns_root_overridden) defaults.campaigns_root = campaigns_root;
  if (config_folder_overridden) defaults.config_folder = config_folder;

  app.global_config_path = global_config_path;
  app.hero_config_path = hero_config_path;
  app.defaults = defaults;

  if (direct_tool_mode) {
    std::string tool_result{};
    std::string tool_error{};
    if (!cuwacunu::hero::jkimyei_mcp::execute_tool_json(
            direct_tool_name, direct_tool_args_json, &app, &tool_result,
            &tool_error)) {
      std::cerr << "[" << kServerName << "] " << tool_error << "\n";
      return 2;
    }
    std::cout << tool_result << "\n";
    const bool is_error =
        cuwacunu::hero::jkimyei_mcp::tool_result_is_error(tool_result);
    return is_error ? 1 : 0;
  }

  if (::isatty(STDIN_FILENO) != 0) {
    std::cerr << "[" << kServerName
              << "] no --tool provided and stdin is a terminal; server mode "
                 "expects JSON-RPC input on stdin.\n";
    print_help(argv[0]);
    return 2;
  }

  cuwacunu::hero::jkimyei_mcp::run_jsonrpc_stdio_loop(&app);
  return 0;
}
