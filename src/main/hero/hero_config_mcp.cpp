#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <unistd.h>

#include "hero/config_hero/hero.config.h"
#include "hero/config_hero/hero_config_store.h"
#include "hero/config_hero/hero_config_tools.h"
#include "piaabo/dlogs.h"

namespace {

constexpr const char* kDefaultHeroConfigPath =
    "/cuwacunu/src/config/instructions/default.hero.config.dsl";
constexpr const char* kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";

__attribute__((constructor(101))) void disable_terminal_logs_pre_main() {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
}

void print_cli_help(const char* argv0) {
  std::cerr << "Usage: " << argv0
            << " [--global-config <path>] [--config <hero_config_dsl>]"
               " [--tool <name>] [--args-json <json>]"
               " [--list-tools] [--list-tools-json]\n"
            << "  default mode: JSON-RPC over stdio\n"
            << "  default config path: [REAL_HERO].config_hero_dsl_filename"
               " in --global-config (default /cuwacunu/src/config/.config)\n"
            << "  --tool/--args-json: execute one MCP tool call and exit\n"
            << "  --list-tools: human-readable tool list\n"
            << "  --list-tools-json: print MCP tools/list JSON and exit\n";
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

[[nodiscard]] std::string lowercase_copy(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] std::string normalize_protocol_layer(std::string_view in) {
  return lowercase_copy(trim_ascii(in));
}

[[nodiscard]] std::string strip_ini_comment(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  bool in_single = false;
  bool in_double = false;
  for (const char c : in) {
    if (c == '\'' && !in_double) {
      in_single = !in_single;
      out.push_back(c);
      continue;
    }
    if (c == '"' && !in_single) {
      in_double = !in_double;
      out.push_back(c);
      continue;
    }
    if (!in_single && !in_double && (c == '#' || c == ';')) break;
    out.push_back(c);
  }
  return out;
}

[[nodiscard]] std::string resolve_path_from_folder(std::string folder,
                                                   std::string path) {
  folder = trim_ascii(folder);
  path = trim_ascii(path);
  if (path.empty()) return {};
  const std::filesystem::path p(path);
  if (p.is_absolute()) return p.string();
  if (folder.empty()) return p.string();
  return (std::filesystem::path(folder) / p).string();
}

[[nodiscard]] std::optional<std::string> read_ini_value(
    const std::string& ini_path, const std::string& section, const std::string& key) {
  std::ifstream in(ini_path);
  if (!in) return std::nullopt;

  std::string line;
  std::string current_section;
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(strip_ini_comment(line));
    if (trimmed.empty()) continue;
    if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']') {
      current_section = trim_ascii(trimmed.substr(1, trimmed.size() - 2));
      continue;
    }
    if (current_section != section) continue;
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos) continue;
    const std::string lhs = trim_ascii(trimmed.substr(0, eq));
    if (lhs != key) continue;
    return trim_ascii(trimmed.substr(eq + 1));
  }
  return std::nullopt;
}

[[nodiscard]] std::string default_config_path_from_real_hero(
    const std::filesystem::path& global_cfg) {
  const std::optional<std::string> configured = read_ini_value(
      global_cfg.string(), "REAL_HERO", "config_hero_dsl_filename");
  if (!configured.has_value()) return kDefaultHeroConfigPath;
  const std::string resolved = resolve_path_from_folder(
      global_cfg.parent_path().string(), *configured);
  if (resolved.empty()) return kDefaultHeroConfigPath;
  return resolved;
}

}  // namespace

int main(int argc, char** argv) {
  std::filesystem::path global_config_path = kDefaultGlobalConfigPath;
  std::string config_path;
  bool config_overridden = false;
  std::string direct_tool_name;
  std::string direct_tool_args_json = "{}";
  bool direct_tool_mode = false;
  bool direct_tool_args_overridden = false;
  bool list_tools = false;
  bool list_tools_json = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--global-config" && i + 1 < argc) {
      global_config_path = argv[++i];
      continue;
    }
    if (arg == "--config" && i + 1 < argc) {
      config_path = argv[++i];
      config_overridden = true;
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

  if (!config_overridden) {
    config_path = default_config_path_from_real_hero(global_config_path);
  }

  if (list_tools_json) {
    std::cout << cuwacunu::hero::mcp::build_tools_list_result_json() << "\n";
    return 0;
  }
  if (list_tools) {
    std::cout << cuwacunu::hero::mcp::build_tools_list_human_text();
    return 0;
  }

  cuwacunu::hero::mcp::hero_config_store_t store(config_path,
                                                 global_config_path.string());
  std::string load_error;
  if (!store.load(&load_error)) {
    std::cout << "err\tstartup\t" << load_error << "\n";
    std::cout << "end\n";
    std::cout.flush();
    return 2;
  }

  const std::string protocol_layer =
      normalize_protocol_layer(store.get_or_default("protocol_layer"));
  if (protocol_layer == "https/sse") {
    std::cerr << cuwacunu::hero::config::kProtocolLayerHttpsSseFailFastMessage
              << "\n";
    return 2;
  }
  if (!protocol_layer.empty() && protocol_layer != "stdio") {
    std::cerr << "Unsupported protocol_layer: " << protocol_layer
              << " (allowed: STDIO|HTTPS/SSE)\n";
    return 2;
  }

  if (direct_tool_mode) {
    std::string tool_result_json;
    std::string tool_error;
    const bool ok = cuwacunu::hero::mcp::execute_tool_json(
        direct_tool_name, direct_tool_args_json, &store, &tool_result_json,
        &tool_error);
    if (!tool_result_json.empty()) {
      std::cout << tool_result_json << "\n";
      std::cout.flush();
    }
    if (!ok && !tool_error.empty()) {
      std::cerr << "tool execution failed: " << tool_error << "\n";
    }
    return ok ? 0 : 1;
  }

  int exit_code = 0;
  if (::isatty(STDIN_FILENO) != 0) {
    std::cerr << "[hero_config_mcp] no --tool provided and stdin is a terminal; "
                 "default mode expects JSON-RPC input on stdin.\n";
    print_cli_help(argv[0]);
    return 2;
  }
  cuwacunu::hero::mcp::run_jsonrpc_stdio_loop(&store);
  return exit_code;
}
