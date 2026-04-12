#include <algorithm>
#include <cerrno>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unistd.h>

#include "hero/config_hero/hero.config.h"
#include "hero/config_hero/hero_config_store.h"
#include "hero/config_hero/hero_config_tools.h"
#include "hero/mcp_server_observability.h"
#include "piaabo/dlogs.h"

namespace {

constexpr const char* kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char* kServerName = "hero_config_mcp";

__attribute__((constructor(101))) void disable_terminal_logs_pre_main() {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
}

void write_stdout_text(std::string_view text) {
  const char* data = text.data();
  std::size_t remaining = text.size();
  while (remaining > 0) {
    const ssize_t wrote = ::write(STDOUT_FILENO, data, remaining);
    if (wrote < 0) {
      if (errno == EINTR) continue;
      break;
    }
    if (wrote == 0) break;
    const auto wrote_size = static_cast<std::size_t>(wrote);
    data += wrote_size;
    remaining -= wrote_size;
  }
}

void write_stderr_text(std::string_view text) {
  cuwacunu::hero::mcp_observability::mirror_stderr_text(text);
  const char* data = text.data();
  std::size_t remaining = text.size();
  while (remaining > 0) {
    const ssize_t wrote = ::write(STDERR_FILENO, data, remaining);
    if (wrote < 0) {
      if (errno == EINTR) continue;
      break;
    }
    if (wrote == 0) break;
    const auto wrote_size = static_cast<std::size_t>(wrote);
    data += wrote_size;
    remaining -= wrote_size;
  }
}

void print_cli_help(const char* argv0) {
  std::ostringstream out;
  out << "Usage: " << argv0
      << " [--global-config <path>] [--config <hero_config_dsl>]"
         " [--tool <name>] [--args-json <json>]"
         " [--list-tools] [--list-tools-json]\n"
      << "  default mode: JSON-RPC over stdio\n"
      << "  default config path: [REAL_HERO].config_hero_dsl_filename"
         " in --global-config (default /cuwacunu/src/config/.config)\n"
      << "  --tool/--args-json: execute one MCP tool call and exit\n"
      << "  --list-tools: human-readable tool list\n"
      << "  --list-tools-json: print MCP tools/list JSON and exit\n";
  write_stderr_text(out.str());
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
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_folder(
      global_cfg.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return resolved;
}

}  // namespace

int main(int argc, char** argv) {
  cuwacunu::hero::mcp_observability::clear_log_paths();
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
    write_stderr_text("Unknown argument: " + arg + "\n");
    print_cli_help(argv[0]);
    return 2;
  }

  if (direct_tool_args_overridden && !direct_tool_mode) {
    write_stderr_text("--args-json requires --tool\n");
    return 2;
  }
  if (list_tools && list_tools_json) {
    write_stderr_text(
        "--list-tools and --list-tools-json are mutually exclusive\n");
    return 2;
  }
  if ((list_tools || list_tools_json) && direct_tool_mode) {
    write_stderr_text("--list-tools/--list-tools-json cannot be combined with "
                      "--tool\n");
    return 2;
  }
  if (direct_tool_mode) {
    direct_tool_name = trim_ascii(direct_tool_name);
    if (direct_tool_name.empty()) {
      write_stderr_text("--tool requires a non-empty name\n");
      return 2;
    }
    direct_tool_args_json = trim_ascii(direct_tool_args_json);
    if (direct_tool_args_json.empty()) direct_tool_args_json = "{}";
    if (direct_tool_args_json.front() != '{') {
      write_stderr_text("--args-json must be a JSON object\n");
      return 2;
    }
  }

  {
    std::string observability_error{};
    const auto log_dir =
        cuwacunu::hero::mcp_observability::derive_config_mcp_log_dir(
            global_config_path, &observability_error);
    if (!log_dir.empty()) {
      cuwacunu::hero::mcp_observability::configure_log_paths(
          cuwacunu::hero::mcp_observability::make_log_paths(log_dir));
    }
  }

  if (!config_overridden) {
    config_path = default_config_path_from_real_hero(global_config_path);
  }
  if (config_path.empty()) {
    write_stderr_text("missing [REAL_HERO].config_hero_dsl_filename in " +
                      global_config_path.string() + "\n");
    return 2;
  }

  {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "mode",
        direct_tool_mode   ? "direct_tool"
        : list_tools       ? "list_tools"
        : list_tools_json  ? "list_tools_json"
                           : "server");
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "global_config_path", global_config_path.string());
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "config_path", config_path);
    cuwacunu::hero::mcp_observability::append_json_int_field(
        extra, "pid", static_cast<int>(::getpid()));
    std::string ignored{};
    (void)cuwacunu::hero::mcp_observability::append_event_json(
        kServerName, "startup", extra.str(), &ignored);
  }

  if (list_tools_json) {
    write_stdout_text(cuwacunu::hero::mcp::build_tools_list_result_json() + "\n");
    return 0;
  }
  if (list_tools) {
    write_stdout_text(cuwacunu::hero::mcp::build_tools_list_human_text());
    return 0;
  }

  cuwacunu::hero::mcp::hero_config_store_t store(config_path,
                                                 global_config_path.string());
  std::string load_error;
  if (!store.load(&load_error)) {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "message", load_error);
    std::string ignored{};
    (void)cuwacunu::hero::mcp_observability::append_event_json(
        kServerName, "startup_failed", extra.str(), &ignored);
    write_stdout_text("err\tstartup\t" + load_error + "\nend\n");
    return 2;
  }

  const std::string protocol_layer =
      normalize_protocol_layer(store.get_or_default("protocol_layer"));
  if (protocol_layer == "https/sse") {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "message",
        std::string(cuwacunu::hero::config::kProtocolLayerHttpsSseFailFastMessage));
    std::string ignored{};
    (void)cuwacunu::hero::mcp_observability::append_event_json(
        kServerName, "startup_failed", extra.str(), &ignored);
    write_stderr_text(
        std::string(cuwacunu::hero::config::kProtocolLayerHttpsSseFailFastMessage) +
        "\n");
    return 2;
  }
  if (!protocol_layer.empty() && protocol_layer != "stdio") {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "message", "Unsupported protocol_layer: " + protocol_layer);
    std::string ignored{};
    (void)cuwacunu::hero::mcp_observability::append_event_json(
        kServerName, "startup_failed", extra.str(), &ignored);
    write_stderr_text("Unsupported protocol_layer: " + protocol_layer +
                      " (allowed: STDIO|HTTPS/SSE)\n");
    return 2;
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
    std::string tool_result_json;
    std::string tool_error;
    const bool ok = cuwacunu::hero::mcp::execute_tool_json(
        direct_tool_name, direct_tool_args_json, &store, &tool_result_json,
        &tool_error);
    {
      std::ostringstream extra;
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "tool_name", direct_tool_name);
      cuwacunu::hero::mcp_observability::append_json_bool_field(extra, "ok", ok);
      cuwacunu::hero::mcp_observability::append_json_uint64_field(
          extra, "duration_ms",
          cuwacunu::hero::runtime::now_ms_utc() - tool_started_at_ms);
      if (!tool_error.empty()) {
        cuwacunu::hero::mcp_observability::append_json_string_field(
            extra, "error", tool_error);
      }
      std::string ignored{};
      (void)cuwacunu::hero::mcp_observability::append_event_json(
          kServerName, "tool_finish", extra.str(), &ignored);
    }
    if (!tool_result_json.empty()) {
      write_stdout_text(tool_result_json + "\n");
    }
    if (!ok && !tool_error.empty()) {
      write_stderr_text("tool execution failed: " + tool_error + "\n");
    }
    return ok ? 0 : 1;
  }

  int exit_code = 0;
  if (::isatty(STDIN_FILENO) != 0) {
    write_stderr_text(
        "[hero_config_mcp] no --tool provided and stdin is a terminal; "
        "default mode expects JSON-RPC input on stdin.\n");
    print_cli_help(argv[0]);
    return 2;
  }
  {
    std::string ignored{};
    (void)cuwacunu::hero::mcp_observability::append_event_json(
        kServerName, "server_loop_enter", {}, &ignored);
  }
  cuwacunu::hero::mcp::run_jsonrpc_stdio_loop(&store);
  return exit_code;
}
