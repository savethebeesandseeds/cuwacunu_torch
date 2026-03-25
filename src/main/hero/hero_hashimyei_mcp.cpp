#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <unistd.h>

#include "hero/hashimyei_hero/hero_hashimyei_tools.h"
#include "hero/lattice_hero/hero_lattice_tools.h"
#include "piaabo/dlogs.h"

namespace {

constexpr const char* kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char* kServerName = "hero_hashimyei_mcp";

__attribute__((constructor(101))) void disable_terminal_logs_pre_main() {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
}

[[nodiscard]] bool write_all_fd(int fd, const void* bytes, std::size_t size) {
  const char* data = reinterpret_cast<const char*>(bytes);
  std::size_t remaining = size;
  while (remaining > 0) {
    const ssize_t wrote = ::write(fd, data, remaining);
    if (wrote < 0) {
      if (errno == EINTR) continue;
      return false;
    }
    if (wrote == 0) return false;
    const auto wrote_size = static_cast<std::size_t>(wrote);
    data += wrote_size;
    remaining -= wrote_size;
  }
  return true;
}

void write_stdout_text(std::string_view text) {
  (void)write_all_fd(STDOUT_FILENO, text.data(), text.size());
}

void print_help(const char* argv0) {
  std::string help = std::string("Usage: ") + argv0 + " [options]\n" +
                     "Options:\n"
                     "  --global-config <path>   Global .config used to resolve [REAL_HERO].hashimyei_hero_dsl_filename\n"
                     "  --tool <name>            Execute one MCP tool and exit\n"
                     "  --args-json <json>       Tool arguments JSON object (default: {})\n"
                     "  --list-tools             Human-readable tool list\n"
                     "  --list-tools-json        Print MCP tools/list JSON and exit\n"
                     "  --hero-config <path>     Explicit Hashimyei HERO defaults DSL\n"
                     "  --store-root <path>      Override store_root from HERO defaults DSL\n"
                     "  --catalog <path>         Override catalog_path from HERO defaults DSL\n"
                     "  (without --tool, server mode reads JSON-RPC messages from stdin)\n"
                     "  --help                   Show this help\n";
  write_stdout_text(help);
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
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);

  std::filesystem::path global_config_path = kDefaultGlobalConfigPath;
  std::filesystem::path hero_config_path{};
  std::filesystem::path store_root{};
  std::filesystem::path catalog_path{};
  std::string direct_tool_name{};
  std::string direct_tool_args_json = "{}";
  bool direct_tool_mode = false;
  bool direct_tool_args_overridden = false;
  bool list_tools = false;
  bool list_tools_json = false;
  bool store_root_overridden = false;
  bool catalog_overridden = false;

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
    if (arg == "--store-root" && i + 1 < argc) {
      store_root = argv[++i];
      store_root_overridden = true;
      continue;
    }
    if (arg == "--catalog" && i + 1 < argc) {
      catalog_path = argv[++i];
      catalog_overridden = true;
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
    std::cerr << "--list-tools/--list-tools-json cannot be combined with "
                 "--tool\n";
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
    write_stdout_text(
        cuwacunu::hero::hashimyei_mcp::build_tools_list_result_json() + "\n");
    return 0;
  }
  if (list_tools) {
    write_stdout_text(cuwacunu::hero::hashimyei_mcp::build_tools_list_human_text());
    return 0;
  }

  cuwacunu::hero::hashimyei_mcp::app_context_t app{};

  if (hero_config_path.empty()) {
    hero_config_path =
        cuwacunu::hero::hashimyei_mcp::resolve_hashimyei_hero_dsl_path(
            global_config_path);
  }
  if (hero_config_path.empty()) {
    std::cerr << "[" << kServerName
              << "] missing [REAL_HERO].hashimyei_hero_dsl_filename in "
              << global_config_path.string() << "\n";
    return 2;
  }

  cuwacunu::hero::hashimyei_mcp::hashimyei_runtime_defaults_t defaults{};
  std::string defaults_error{};
  if (!cuwacunu::hero::hashimyei_mcp::load_hashimyei_runtime_defaults(
          hero_config_path, global_config_path, &defaults, &defaults_error)) {
    std::cerr << "[" << kServerName << "] " << defaults_error << "\n";
    return 2;
  }

  if (!store_root_overridden) {
    store_root = defaults.store_root;
  }
  if (!catalog_overridden) {
    catalog_path = defaults.catalog_path;
  }
  if (store_root.empty()) {
    std::cerr << "[" << kServerName << "] resolved empty store_root\n";
    return 2;
  }
  if (catalog_path.empty()) {
    std::cerr << "[" << kServerName << "] resolved empty catalog_path\n";
    return 2;
  }

  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t::options_t options{};
  options.catalog_path = catalog_path;
  options.encrypted = false;

  std::filesystem::path lattice_catalog_path{};
  {
    const auto lattice_hero_config_path =
        cuwacunu::hero::lattice_mcp::resolve_lattice_hero_dsl_path(
            global_config_path);
    if (lattice_hero_config_path.empty()) {
      std::cerr << "[" << kServerName
                << "] missing [REAL_HERO].lattice_hero_dsl_filename in "
                << global_config_path.string() << "\n";
      return 2;
    }
    cuwacunu::hero::lattice_mcp::wave_runtime_defaults_t lattice_defaults{};
    std::string lattice_defaults_error{};
    if (!cuwacunu::hero::lattice_mcp::load_wave_runtime_defaults(
            lattice_hero_config_path, global_config_path, &lattice_defaults,
            &lattice_defaults_error)) {
      std::cerr << "[" << kServerName << "] " << lattice_defaults_error << "\n";
      return 2;
    }
    lattice_catalog_path = lattice_defaults.catalog_path;
  }
  if (lattice_catalog_path.empty()) {
    std::cerr << "[" << kServerName << "] resolved empty lattice catalog_path\n";
    return 2;
  }

  app.store_root = store_root;
  app.lattice_catalog_path = lattice_catalog_path;
  app.catalog_options = options;

  if (direct_tool_mode) {
    app.close_catalog_after_execute = false;
    std::string tool_result{};
    std::string tool_error{};
    if (!cuwacunu::hero::hashimyei_mcp::execute_tool_json(
            direct_tool_name, direct_tool_args_json, &app, &tool_result,
            &tool_error)) {
      std::cerr << "[" << kServerName << "] " << tool_error << "\n";
      std::cerr.flush();
      // Direct CLI mode is single-shot; avoid destructor-driven catalog close
      // stalls on process teardown.
      std::_Exit(2);
    }
    write_stdout_text(tool_result + "\n");
    const int exit_code =
        cuwacunu::hero::hashimyei_mcp::tool_result_is_error(tool_result) ? 1
                                                                         : 0;
    // Direct CLI mode is single-shot; avoid destructor-driven catalog close
    // stalls on process teardown.
    std::_Exit(exit_code);
  }

  if (::isatty(STDIN_FILENO) != 0) {
    std::cerr << "[" << kServerName
              << "] no --tool provided and stdin is a terminal; "
                 "server mode expects JSON-RPC input on stdin.\n";
    print_help(argv[0]);
    return 2;
  }

  cuwacunu::hero::hashimyei_mcp::run_jsonrpc_stdio_loop(&app);
  return 0;
}
