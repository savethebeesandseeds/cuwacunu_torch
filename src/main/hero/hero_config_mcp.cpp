#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unistd.h>

#include "hero/config_hero/hero_config.h"
#include "hero/config_hero/config/hero_config_store.h"
#include "hero/config_hero/hero_config_tools.h"

namespace {

constexpr const char *kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char *kDefaultHeroConfigPath =
    "/cuwacunu/src/config/hero.config.dsl";

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  std::size_t begin = 0;
  while (begin < in.size() &&
         std::isspace(static_cast<unsigned char>(in[begin])) != 0) {
    ++begin;
  }
  std::size_t end = in.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(in[end - 1])) != 0) {
    --end;
  }
  return std::string(in.substr(begin, end - begin));
}

[[nodiscard]] std::string lowercase_ascii(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
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
    if (!in_single && !in_double && (c == '#' || c == ';')) {
      break;
    }
    out.push_back(c);
  }
  return out;
}

[[nodiscard]] std::optional<std::string>
read_ini_value(const std::filesystem::path &ini_path, std::string_view section,
               std::string_view key) {
  std::ifstream in(ini_path);
  if (!in) {
    return std::nullopt;
  }
  std::string current_section;
  std::string line;
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(strip_ini_comment(line));
    if (trimmed.empty()) {
      continue;
    }
    if (trimmed.size() >= 2 && trimmed.front() == '[' &&
        trimmed.back() == ']') {
      current_section = trim_ascii(trimmed.substr(1, trimmed.size() - 2));
      continue;
    }
    if (current_section != section) {
      continue;
    }
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const std::string lhs = trim_ascii(trimmed.substr(0, eq));
    if (lhs != key) {
      continue;
    }
    return trim_ascii(trimmed.substr(eq + 1));
  }
  return std::nullopt;
}

[[nodiscard]] std::filesystem::path
resolve_near(const std::filesystem::path &base_file,
             std::string_view raw_path) {
  std::filesystem::path path(trim_ascii(raw_path));
  if (path.empty() || path.is_absolute()) {
    return path;
  }
  return base_file.parent_path() / path;
}

[[nodiscard]] std::string default_config_path_from_global(
    const std::filesystem::path &global_config_path) {
  if (const auto fresh =
          read_ini_value(global_config_path, "HERO", "config_hero_dsl_path")) {
    return resolve_near(global_config_path, *fresh).string();
  }
  return kDefaultHeroConfigPath;
}

void print_help(const char *argv0) {
  std::cout
      << "Usage: " << argv0
      << " [--global-config <path>] [--config <hero_config_dsl>]"
         " [--tool <name>] [--args-json <json>]"
         " [--list-tools] [--list-tools-json]\n"
      << "  default mode: JSON-RPC over stdio\n"
      << "  default config path: [HERO].config_hero_dsl_path in "
         "--global-config, then "
      << kDefaultHeroConfigPath << "\n"
      << "\n"
      << "Authority groups:\n"
      << "  Read-only health:\n"
      << "    hero.config.status\n"
      << "  Read-only inspection:\n"
      << "    hero.config.inspect subject=schema|show|value|validate|map\n"
      << "    hero.config.inspect subject=bundle|resolve_path|diff|backups\n"
      << "    hero.config.inspect subject=file_list|file_read\n"
      << "  Config mutation under policy-controlled roots:\n"
      << "    hero.config.apply "
         "operation=set|save|reload|rollback|write|delete\n"
      << "    requested_mode=plan validates/previews without mutation;\n"
      << "    requested_mode=execute performs the Config-scope mutation.\n"
      << "  Boundary:\n"
      << "    Config Hero does not execute Runtime, prove Lattice "
         "targets,\n"
      << "    select checkpoints, or make deployment/allocation "
         "decisions.\n"
      << "    Use requested_mode=plan and expected digests for file "
         "write/delete work.\n";
}

} // namespace

int main(int argc, char **argv) {
  std::filesystem::path global_config_path = kDefaultGlobalConfigPath;
  std::string config_path;
  bool config_overridden = false;
  bool list_tools = false;
  bool list_tools_json = false;
  bool direct_tool_mode = false;
  bool direct_tool_args_overridden = false;
  std::string direct_tool_name;
  std::string direct_tool_args_json = "{}";

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

  if (!config_overridden) {
    config_path = default_config_path_from_global(global_config_path);
  }
  if (trim_ascii(config_path).empty()) {
    std::cerr << "empty Config Hero policy path\n";
    return 2;
  }

  if (list_tools_json) {
    std::cout << cuwacunu::hero::config::build_tools_list_result_json() << "\n";
    return 0;
  }
  if (list_tools) {
    std::cout << cuwacunu::hero::config::build_tools_list_human_text();
    return 0;
  }

  cuwacunu::hero::config::hero_config_store_t store(
      config_path, global_config_path.string());
  std::string load_error;
  if (!store.load(&load_error)) {
    std::cerr << "Config Hero load failed: " << load_error << "\n";
    return 2;
  }

  const std::string protocol_layer =
      lowercase_ascii(trim_ascii(store.get_or_default("protocol_layer")));
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
    direct_tool_name = trim_ascii(direct_tool_name);
    direct_tool_args_json = trim_ascii(direct_tool_args_json);
    if (direct_tool_name.empty()) {
      std::cerr << "--tool requires a non-empty name\n";
      return 2;
    }
    if (direct_tool_args_json.empty()) {
      direct_tool_args_json = "{}";
    }
    std::string result;
    std::string error;
    const bool ok = cuwacunu::hero::config::execute_tool_json(
        direct_tool_name, direct_tool_args_json, &store, &result, &error);
    if (!result.empty()) {
      std::cout << result << "\n";
    }
    if (!ok && !error.empty()) {
      std::cerr << "tool execution failed: " << error << "\n";
    }
    return ok ? 0 : 1;
  }

  if (::isatty(STDIN_FILENO) != 0) {
    std::cerr << "[hero_config_mcp] no --tool provided and stdin is a "
                 "terminal; default mode expects JSON-RPC input on stdin.\n";
    print_help(argv[0]);
    return 2;
  }

  cuwacunu::hero::config::run_jsonrpc_stdio_loop(&store);
  return 0;
}
