#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unistd.h>

#include "hero/hashimyei_hero/hero_hashimyei_tools.h"
#include "hero/lattice_hero/hero_lattice_tools.h"
#include "hero/mcp_server_observability.h"
#include "piaabo/dlogs.h"

namespace {

constexpr const char *kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char *kServerName = "hero_hashimyei_mcp";

__attribute__((constructor(101))) void disable_terminal_logs_pre_main() {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
}

void write_stdout_text(std::string_view text) {
  const char *data = text.data();
  std::size_t remaining = text.size();
  while (remaining > 0) {
    const ssize_t wrote = ::write(STDOUT_FILENO, data, remaining);
    if (wrote < 0) {
      if (errno == EINTR)
        continue;
      break;
    }
    if (wrote == 0)
      break;
    const auto wrote_size = static_cast<std::size_t>(wrote);
    data += wrote_size;
    remaining -= wrote_size;
  }
}

void write_cli_error_text(std::string_view text) {
  const char *data = text.data();
  std::size_t remaining = text.size();
  while (remaining > 0) {
    const ssize_t wrote = ::write(STDERR_FILENO, data, remaining);
    if (wrote < 0) {
      if (errno == EINTR)
        continue;
      break;
    }
    if (wrote == 0)
      break;
    const auto wrote_size = static_cast<std::size_t>(wrote);
    data += wrote_size;
    remaining -= wrote_size;
  }
}

void write_stderr_text(std::string_view text) {
  if (text.empty())
    return;
  cuwacunu::hero::mcp_observability::mirror_stderr_text(text);
  log_err("%.*s", static_cast<int>(text.size()), text.data());
}

void print_help(const char *argv0) {
  std::string help =
      std::string("Usage: ") + argv0 + " [options]\n" +
      "Options:\n"
      "  --global-config <path>   Global .config used to resolve "
      "[REAL_HERO].hashimyei_hero_dsl_filename\n"
      "  --tool <name>            Execute one MCP tool and exit\n"
      "  --args-json <json>       Tool arguments JSON object (default: {})\n"
      "  --list-tools             Human-readable tool list\n"
      "  --list-tools-json        Print MCP tools/list JSON and exit\n"
      "  --hero-config <path>     Explicit Hashimyei HERO defaults DSL\n"
      "  --store-root <path>      Override store_root from HERO defaults DSL\n"
      "  --catalog <path>         Override catalog_path from HERO defaults "
      "DSL\n"
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

} // namespace

int main(int argc, char **argv) {
  cuwacunu::hero::mcp_observability::clear_log_paths();
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

    write_cli_error_text("Unknown argument: " + arg + "\n");
    print_help(argv[0]);
    return 2;
  }

  if (direct_tool_args_overridden && !direct_tool_mode) {
    write_cli_error_text("--args-json requires --tool\n");
    return 2;
  }
  if (list_tools && list_tools_json) {
    write_cli_error_text(
        "--list-tools and --list-tools-json are mutually exclusive\n");
    return 2;
  }
  if ((list_tools || list_tools_json) && direct_tool_mode) {
    write_cli_error_text("--list-tools/--list-tools-json cannot be combined "
                         "with --tool\n");
    return 2;
  }
  if (direct_tool_mode) {
    direct_tool_name = trim_ascii(direct_tool_name);
    if (direct_tool_name.empty()) {
      write_cli_error_text("--tool requires a non-empty name\n");
      return 2;
    }
    direct_tool_args_json = trim_ascii(direct_tool_args_json);
    if (direct_tool_args_json.empty())
      direct_tool_args_json = "{}";
    if (direct_tool_args_json.front() != '{') {
      write_cli_error_text("--args-json must be a JSON object\n");
      return 2;
    }
  }

  if (list_tools_json) {
    write_stdout_text(
        cuwacunu::hero::hashimyei_mcp::build_tools_list_result_json() + "\n");
    return 0;
  }
  if (list_tools) {
    write_stdout_text(
        cuwacunu::hero::hashimyei_mcp::build_tools_list_human_text());
    return 0;
  }

  cuwacunu::hero::hashimyei_mcp::app_context_t app{};

  if (hero_config_path.empty()) {
    hero_config_path =
        cuwacunu::hero::hashimyei_mcp::resolve_hashimyei_hero_dsl_path(
            global_config_path);
  }
  if (hero_config_path.empty()) {
    write_stderr_text(std::string("[") + kServerName +
                      "] missing [REAL_HERO].hashimyei_hero_dsl_filename in " +
                      global_config_path.string() + "\n");
    return 2;
  }

  cuwacunu::hero::hashimyei_mcp::hashimyei_runtime_defaults_t defaults{};
  std::string defaults_error{};
  if (!cuwacunu::hero::hashimyei_mcp::load_hashimyei_runtime_defaults(
          hero_config_path, global_config_path, &defaults, &defaults_error)) {
    write_stderr_text(std::string("[") + kServerName + "] " + defaults_error +
                      "\n");
    return 2;
  }

  if (!store_root_overridden) {
    store_root = defaults.store_root;
  }
  if (!catalog_overridden) {
    catalog_path = defaults.catalog_path;
  }
  if (store_root.empty()) {
    write_stderr_text(std::string("[") + kServerName +
                      "] resolved empty store_root\n");
    return 2;
  }
  if (catalog_path.empty()) {
    write_stderr_text(std::string("[") + kServerName +
                      "] resolved empty catalog_path\n");
    return 2;
  }

  cuwacunu::hero::mcp_observability::configure_log_paths(
      cuwacunu::hero::mcp_observability::make_log_paths(
          cuwacunu::hero::mcp_observability::derive_store_backed_mcp_log_dir(
              store_root, kServerName)));

  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t::options_t options{};
  options.catalog_path = catalog_path;
  options.encrypted = false;

  std::filesystem::path lattice_catalog_path{};
  {
    const auto lattice_hero_config_path =
        cuwacunu::hero::lattice_mcp::resolve_lattice_hero_dsl_path(
            global_config_path);
    if (lattice_hero_config_path.empty()) {
      write_stderr_text(std::string("[") + kServerName +
                        "] missing [REAL_HERO].lattice_hero_dsl_filename in " +
                        global_config_path.string() + "\n");
      return 2;
    }
    cuwacunu::hero::lattice_mcp::wave_runtime_defaults_t lattice_defaults{};
    std::string lattice_defaults_error{};
    if (!cuwacunu::hero::lattice_mcp::load_wave_runtime_defaults(
            lattice_hero_config_path, global_config_path, &lattice_defaults,
            &lattice_defaults_error)) {
      write_stderr_text(std::string("[") + kServerName + "] " +
                        lattice_defaults_error + "\n");
      return 2;
    }
    lattice_catalog_path = lattice_defaults.catalog_path;
  }
  if (lattice_catalog_path.empty()) {
    write_stderr_text(std::string("[") + kServerName +
                      "] resolved empty lattice catalog_path\n");
    return 2;
  }

  app.store_root = store_root;
  app.lattice_catalog_path = lattice_catalog_path;
  app.global_config_path = global_config_path;
  app.catalog_options = options;

  {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "mode",
        direct_tool_mode  ? "direct_tool"
        : list_tools      ? "list_tools"
        : list_tools_json ? "list_tools_json"
                          : "server");
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "global_config_path", global_config_path.string());
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "hero_config_path", hero_config_path.string());
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "store_root", store_root.string());
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", catalog_path.string());
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "lattice_catalog_path", lattice_catalog_path.string());
    cuwacunu::hero::mcp_observability::append_json_int_field(
        extra, "pid", static_cast<int>(::getpid()));
    std::string ignored{};
    (void)cuwacunu::hero::mcp_observability::append_event_json(
        kServerName, "startup", extra.str(), &ignored);
  }

  if (direct_tool_mode) {
    {
      std::ostringstream extra;
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "tool_name", direct_tool_name);
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "arguments_json", direct_tool_args_json);
      std::string ignored{};
      (void)cuwacunu::hero::mcp_observability::append_event_json(
          kServerName, "tool_start", extra.str(), &ignored);
    }
    const std::uint64_t tool_started_at_ms =
        cuwacunu::hero::runtime::now_ms_utc();
    std::string tool_result{};
    std::string tool_error{};
    if (!cuwacunu::hero::hashimyei_mcp::execute_tool_json(
            direct_tool_name, direct_tool_args_json, &app, &tool_result,
            &tool_error)) {
      std::ostringstream extra;
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "tool_name", direct_tool_name);
      cuwacunu::hero::mcp_observability::append_json_bool_field(extra, "ok",
                                                                false);
      cuwacunu::hero::mcp_observability::append_json_uint64_field(
          extra, "duration_ms",
          cuwacunu::hero::runtime::now_ms_utc() - tool_started_at_ms);
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "error", tool_error);
      std::string ignored{};
      (void)cuwacunu::hero::mcp_observability::append_event_json(
          kServerName, "tool_finish", extra.str(), &ignored);
      write_stderr_text(std::string("[") + kServerName + "] " + tool_error +
                        "\n");
      return 2;
    }
    {
      std::ostringstream extra;
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "tool_name", direct_tool_name);
      cuwacunu::hero::mcp_observability::append_json_bool_field(extra, "ok",
                                                                true);
      cuwacunu::hero::mcp_observability::append_json_uint64_field(
          extra, "duration_ms",
          cuwacunu::hero::runtime::now_ms_utc() - tool_started_at_ms);
      cuwacunu::hero::mcp_observability::append_json_bool_field(
          extra, "tool_result_is_error",
          cuwacunu::hero::hashimyei_mcp::tool_result_is_error(tool_result));
      std::string ignored{};
      (void)cuwacunu::hero::mcp_observability::append_event_json(
          kServerName, "tool_finish", extra.str(), &ignored);
    }
    write_stdout_text(tool_result + "\n");
    return cuwacunu::hero::hashimyei_mcp::tool_result_is_error(tool_result) ? 1
                                                                            : 0;
  }

  if (::isatty(STDIN_FILENO) != 0) {
    write_cli_error_text(std::string("[") + kServerName +
                         "] no --tool provided and stdin is a terminal; "
                         "server mode expects JSON-RPC input on stdin.\n");
    print_help(argv[0]);
    return 2;
  }

  {
    std::string ignored{};
    (void)cuwacunu::hero::mcp_observability::append_event_json(
        kServerName, "server_loop_enter", {}, &ignored);
  }
  cuwacunu::hero::hashimyei_mcp::run_jsonrpc_stdio_loop(&app);
  return 0;
}
