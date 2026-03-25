// SPDX-License-Identifier: MIT
#include "hero/human_hero/hero_human_tools.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "hero/human_hero/human_attestation.h"
#include "hero/super_hero/super_loop.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"

namespace cuwacunu {
namespace hero {
namespace human_mcp {

namespace {

constexpr const char* kServerName = "hero_human_mcp";
constexpr const char* kServerVersion = "0.1.0";
constexpr const char* kProtocolVersion = "2025-03-26";
constexpr const char* kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.human.*.";
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;
constexpr std::size_t kSuperToolTimeoutSec = 30;
constexpr const char* kNcursesInitErrorPrefix = "ncurses init failed: ";

using cuwacunu::hero::human::human_response_record_t;

using human_tool_handler_t = bool (*)(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error);

struct human_tool_descriptor_t {
  std::string_view name;
  std::string_view description;
  std::string_view input_schema_json;
  human_tool_handler_t handler;
};

struct interactive_response_input_t {
  std::string control_kind{};
  std::string next_action_kind{};
  std::string target_binding_id{};
  bool reset_runtime_state = false;
  std::string reason{};
  std::string memory_note{};
};

[[nodiscard]] bool collect_pending_requests(
    const app_context_t& app,
    std::vector<cuwacunu::hero::super::super_loop_record_t>* out,
    std::string* error);

[[nodiscard]] bool build_response_and_resume(
    app_context_t* app,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::string control_kind, std::string next_action_kind,
    std::string target_binding_id, bool reset_runtime_state, std::string reason,
    std::string memory_note, std::string* out_structured,
    std::string* out_error);

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  return cuwacunu::hero::runtime::trim_ascii(in);
}

[[nodiscard]] std::string lowercase_copy(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] std::string strip_inline_comment(std::string_view in) {
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

[[nodiscard]] std::string json_escape(std::string_view in) {
  std::ostringstream out;
  for (const unsigned char ch : in) {
    switch (ch) {
      case '\\':
        out << "\\\\";
        break;
      case '"':
        out << "\\\"";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (ch < 0x20) {
          out << "\\u00";
          constexpr char kHex[] = "0123456789abcdef";
          out << kHex[(ch >> 4) & 0x0f] << kHex[ch & 0x0f];
        } else {
          out << static_cast<char>(ch);
        }
        break;
    }
  }
  return out.str();
}

[[nodiscard]] std::string json_quote(std::string_view in) {
  return "\"" + json_escape(in) + "\"";
}

[[nodiscard]] std::string bool_json(bool value) {
  return value ? "true" : "false";
}

[[nodiscard]] std::string json_array_from_strings(
    const std::vector<std::string>& values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) out << ",";
    out << json_quote(values[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string unquote_if_wrapped(std::string s) {
  s = trim_ascii(std::move(s));
  if (s.size() >= 2) {
    const char a = s.front();
    const char b = s.back();
    if ((a == '"' && b == '"') || (a == '\'' && b == '\'')) {
      return s.substr(1, s.size() - 2);
    }
  }
  return s;
}

[[nodiscard]] std::string resolve_path_from_base_folder(std::string base_folder,
                                                        std::string path) {
  base_folder = trim_ascii(std::move(base_folder));
  path = trim_ascii(std::move(path));
  if (path.empty()) return {};
  const std::filesystem::path p(path);
  if (p.is_absolute()) return p.lexically_normal().string();
  if (base_folder.empty()) return p.lexically_normal().string();
  return (std::filesystem::path(base_folder) / p).lexically_normal().string();
}

[[nodiscard]] std::optional<std::string> read_ini_value(
    const std::filesystem::path& ini_path, const std::string& section,
    const std::string& key) {
  std::ifstream in(ini_path);
  if (!in) return std::nullopt;
  std::string line;
  std::string current_section;
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(strip_inline_comment(line));
    if (trimmed.empty()) continue;
    if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']') {
      current_section = trim_ascii(trimmed.substr(1, trimmed.size() - 2));
      continue;
    }
    if (current_section != section) continue;
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos) continue;
    if (trim_ascii(trimmed.substr(0, eq)) != key) continue;
    return trim_ascii(trimmed.substr(eq + 1));
  }
  return std::nullopt;
}

[[nodiscard]] bool parse_size_token(std::string_view raw, std::size_t* out) {
  if (!out) return false;
  const std::string token = trim_ascii(raw);
  if (token.empty()) return false;
  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(token.data(), token.data() + token.size(), parsed);
  if (ec != std::errc{} || ptr != token.data() + token.size()) return false;
  *out = static_cast<std::size_t>(parsed);
  return true;
}

[[nodiscard]] std::size_t skip_json_whitespace(const std::string& json,
                                               std::size_t pos) {
  while (pos < json.size() &&
         std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
    ++pos;
  }
  return pos;
}

[[nodiscard]] bool parse_json_string_at(const std::string& json, std::size_t pos,
                                        std::string* out,
                                        std::size_t* out_end) {
  if (out) out->clear();
  if (pos >= json.size() || json[pos] != '"') return false;
  ++pos;
  std::string parsed;
  while (pos < json.size()) {
    const char c = json[pos++];
    if (c == '"') {
      if (out) *out = parsed;
      if (out_end) *out_end = pos;
      return true;
    }
    if (c == '\\') {
      if (pos >= json.size()) return false;
      const char esc = json[pos++];
      switch (esc) {
        case '"':
        case '\\':
        case '/':
          parsed.push_back(esc);
          break;
        case 'b':
          parsed.push_back('\b');
          break;
        case 'f':
          parsed.push_back('\f');
          break;
        case 'n':
          parsed.push_back('\n');
          break;
        case 'r':
          parsed.push_back('\r');
          break;
        case 't':
          parsed.push_back('\t');
          break;
        case 'u':
          if (pos + 4 > json.size()) return false;
          pos += 4;
          parsed.push_back('?');
          break;
        default:
          return false;
      }
      continue;
    }
    parsed.push_back(c);
  }
  return false;
}

[[nodiscard]] bool find_json_value_end(const std::string& json, std::size_t pos,
                                       std::size_t* out_end) {
  pos = skip_json_whitespace(json, pos);
  if (pos >= json.size()) return false;
  const char first = json[pos];
  if (first == '"') return parse_json_string_at(json, pos, nullptr, out_end);
  if (first == '{' || first == '[') {
    int depth = 0;
    bool in_string = false;
    for (std::size_t i = pos; i < json.size(); ++i) {
      const char c = json[i];
      if (in_string) {
        if (c == '\\') {
          ++i;
          continue;
        }
        if (c == '"') in_string = false;
        continue;
      }
      if (c == '"') {
        in_string = true;
        continue;
      }
      if (c == '{' || c == '[') ++depth;
      if (c == '}' || c == ']') {
        --depth;
        if (depth == 0) {
          if (out_end) *out_end = i + 1;
          return true;
        }
      }
    }
    return false;
  }
  std::size_t i = pos;
  while (i < json.size()) {
    const char c = json[i];
    if (c == ',' || c == '}' || c == ']') break;
    ++i;
  }
  while (i > pos &&
         std::isspace(static_cast<unsigned char>(json[i - 1])) != 0) {
    --i;
  }
  if (i == pos) return false;
  if (out_end) *out_end = i;
  return true;
}

[[nodiscard]] bool extract_json_field_raw(const std::string& json,
                                          std::string_view key,
                                          std::string* out_raw) {
  if (out_raw) out_raw->clear();
  const std::string needle = "\"" + std::string(key) + "\"";
  std::size_t pos = 0;
  while (true) {
    pos = json.find(needle, pos);
    if (pos == std::string::npos) return false;
    pos += needle.size();
    pos = skip_json_whitespace(json, pos);
    if (pos >= json.size() || json[pos] != ':') continue;
    pos = skip_json_whitespace(json, pos + 1);
    std::size_t end = 0;
    if (!find_json_value_end(json, pos, &end)) return false;
    if (out_raw) *out_raw = json.substr(pos, end - pos);
    return true;
  }
}

[[nodiscard]] bool extract_json_string_field(const std::string& json,
                                             std::string_view key,
                                             std::string* out) {
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw)) return false;
  std::size_t end = 0;
  return parse_json_string_at(raw, 0, out, &end) &&
         skip_json_whitespace(raw, end) == raw.size();
}

[[nodiscard]] bool extract_json_bool_field(const std::string& json,
                                           std::string_view key, bool* out) {
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw)) return false;
  if (raw == "true") {
    if (out) *out = true;
    return true;
  }
  if (raw == "false") {
    if (out) *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] bool extract_json_size_field(const std::string& json,
                                           std::string_view key,
                                           std::size_t* out) {
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw)) return false;
  return parse_size_token(raw, out);
}

[[nodiscard]] bool extract_json_object_field(const std::string& json,
                                             std::string_view key,
                                             std::string* out) {
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw)) return false;
  raw = trim_ascii(raw);
  if (raw.empty() || raw.front() != '{' || raw.back() != '}') return false;
  if (out) *out = std::move(raw);
  return true;
}

[[nodiscard]] bool tool_result_is_error_impl(std::string_view tool_result_json) {
  return tool_result_json.find("\"isError\":true") != std::string_view::npos ||
         tool_result_json.find("\"isError\": true") != std::string_view::npos;
}

[[nodiscard]] bool tool_result_structured_content(std::string_view tool_result_json,
                                                  std::string* out_structured,
                                                  std::string* out_error) {
  if (out_error) out_error->clear();
  if (!out_structured) {
    if (out_error) *out_error = "structuredContent output pointer is null";
    return false;
  }
  out_structured->clear();
  const std::string json(tool_result_json);
  if (tool_result_is_error_impl(json)) {
    std::string structured{};
    if (extract_json_object_field(json, "structuredContent", &structured)) {
      std::string error_value{};
      if (extract_json_string_field(structured, "error", &error_value) &&
          !error_value.empty()) {
        if (out_error) *out_error = error_value;
        return false;
      }
    }
    std::string text{};
    if (extract_json_string_field(json, "text", &text) && !text.empty()) {
      if (out_error) *out_error = text;
      return false;
    }
    if (out_error) *out_error = "tool returned error";
    return false;
  }
  if (!extract_json_object_field(json, "structuredContent", out_structured)) {
    if (out_error) *out_error = "tool result missing structuredContent";
    return false;
  }
  return true;
}

[[nodiscard]] std::string make_tool_result_json(std::string_view text,
                                                std::string_view structured_json,
                                                bool is_error) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":" << json_quote(text)
      << "}],\"structuredContent\":" << structured_json
      << ",\"isError\":" << (is_error ? "true" : "false") << "}";
  return out.str();
}

[[nodiscard]] std::string tool_result_json_for_error(std::string_view message) {
  return make_tool_result_json(
      message,
      "{\"loop_id\":\"\",\"error\":" + json_quote(message) + "}", true);
}

[[nodiscard]] std::filesystem::path resolve_runtime_root_from_global_config(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "GENERAL", "runtime_root");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved).lexically_normal();
}

[[nodiscard]] std::filesystem::path resolve_command_path(
    const std::filesystem::path& base_dir, std::string_view configured_value) {
  const std::string configured = trim_ascii(configured_value);
  if (configured.empty()) return {};
  const std::filesystem::path raw(configured);
  if (configured.find('/') != std::string::npos ||
      configured.find('\\') != std::string::npos) {
    if (raw.is_absolute()) return raw.lexically_normal();
    return (base_dir / raw).lexically_normal();
  }
  if (const char* env_path = std::getenv("PATH"); env_path != nullptr) {
    std::stringstream in(env_path);
    std::string entry{};
    while (std::getline(in, entry, ':')) {
      const std::filesystem::path candidate =
          (std::filesystem::path(entry) / configured).lexically_normal();
      if (::access(candidate.c_str(), X_OK) == 0) return candidate;
    }
  }
  return std::filesystem::path(configured);
}

[[nodiscard]] bool run_command_with_stdio_and_timeout(
    const std::vector<std::string>& argv, const std::filesystem::path& stdin_path,
    const std::filesystem::path& stdout_path,
    const std::filesystem::path& stderr_path, std::size_t timeout_sec,
    int* out_exit_code, std::string* error) {
  if (error) error->clear();
  if (argv.empty()) {
    if (error) *error = "command argv is empty";
    return false;
  }
  if (out_exit_code) *out_exit_code = -1;

  std::error_code ec{};
  if (!std::filesystem::exists(stdin_path, ec) ||
      !std::filesystem::is_regular_file(stdin_path, ec)) {
    if (error) *error = "command stdin path does not exist: " + stdin_path.string();
    return false;
  }
  const int stdout_probe =
      ::open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (stdout_probe < 0) {
    if (error) *error = "cannot open stdout path for command: " + stdout_path.string();
    return false;
  }
  (void)::close(stdout_probe);
  const int stderr_probe =
      ::open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (stderr_probe < 0) {
    if (error) *error = "cannot open stderr path for command: " + stderr_path.string();
    return false;
  }
  (void)::close(stderr_probe);

  const pid_t child = ::fork();
  if (child < 0) {
    if (error) *error = "fork failed for command execution";
    return false;
  }
  if (child == 0) {
    (void)::setpgid(0, 0);
    const int stdin_fd = ::open(stdin_path.c_str(), O_RDONLY);
    if (stdin_fd < 0) _exit(126);
    (void)::dup2(stdin_fd, STDIN_FILENO);
    if (stdin_fd > STDERR_FILENO) (void)::close(stdin_fd);
    const int stdout_fd =
        ::open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stdout_fd < 0) _exit(126);
    (void)::dup2(stdout_fd, STDOUT_FILENO);
    if (stdout_fd > STDERR_FILENO) (void)::close(stdout_fd);
    const int stderr_fd =
        ::open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stderr_fd < 0) _exit(126);
    (void)::dup2(stderr_fd, STDERR_FILENO);
    if (stderr_fd > STDERR_FILENO) (void)::close(stderr_fd);
    std::vector<char*> exec_argv{};
    exec_argv.reserve(argv.size() + 1);
    for (const std::string& arg : argv) {
      exec_argv.push_back(const_cast<char*>(arg.c_str()));
    }
    exec_argv.push_back(nullptr);
    ::execvp(exec_argv[0], exec_argv.data());
    _exit(127);
  }

  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);
  int status = 0;
  for (;;) {
    const pid_t waited = ::waitpid(child, &status, WNOHANG);
    if (waited == child) break;
    if (waited < 0 && errno != EINTR) {
      if (error) *error = "waitpid failed for command execution";
      (void)::kill(-child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      return false;
    }
    if (std::chrono::steady_clock::now() >= deadline) {
      (void)::kill(-child, SIGKILL);
      (void)::kill(child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      if (error) *error = "command timed out";
      return false;
    }
    ::usleep(100 * 1000);
  }

  if (WIFEXITED(status)) {
    if (out_exit_code) *out_exit_code = WEXITSTATUS(status);
    return true;
  }
  if (WIFSIGNALED(status)) {
    if (out_exit_code) *out_exit_code = 128 + WTERMSIG(status);
    if (error) *error = "command terminated by signal";
    return false;
  }
  if (error) *error = "command ended in unknown state";
  return false;
}

[[nodiscard]] std::filesystem::path human_tool_io_dir(const app_context_t& app) {
  return app.defaults.super_root / ".human_tool_io";
}

[[nodiscard]] std::filesystem::path make_human_tool_io_path(
    const app_context_t& app, std::string_view stem) {
  const std::uint64_t salt =
      static_cast<std::uint64_t>(
          std::chrono::steady_clock::now().time_since_epoch().count());
  return human_tool_io_dir(app) /
         ("human_tool." + std::to_string(cuwacunu::hero::runtime::now_ms_utc()) +
          "." + std::to_string(static_cast<unsigned long long>(::getpid())) + "." +
          cuwacunu::hero::runtime::hex_lower_u64(salt).substr(0, 8) + "." +
          std::string(stem));
}

void cleanup_human_tool_io(const std::filesystem::path& stdin_path,
                           const std::filesystem::path& stdout_path,
                           const std::filesystem::path& stderr_path) {
  std::error_code ec{};
  (void)std::filesystem::remove(stdin_path, ec);
  (void)std::filesystem::remove(stdout_path, ec);
  (void)std::filesystem::remove(stderr_path, ec);
}

[[nodiscard]] bool call_super_tool(const app_context_t& app,
                                   const std::string& tool_name,
                                   std::string arguments_json,
                                   std::string* out_structured,
                                   std::string* error) {
  if (error) error->clear();
  if (!out_structured) {
    if (error) *error = "super structured output pointer is null";
    return false;
  }
  *out_structured = "";

  std::error_code ec{};
  std::filesystem::create_directories(human_tool_io_dir(app), ec);
  if (ec) {
    if (error) {
      *error = "cannot create human tool io dir: " +
               human_tool_io_dir(app).string();
    }
    return false;
  }

  const std::filesystem::path stdin_path =
      make_human_tool_io_path(app, "stdin.json");
  const std::filesystem::path stdout_path =
      make_human_tool_io_path(app, "stdout.json");
  const std::filesystem::path stderr_path =
      make_human_tool_io_path(app, "stderr.log");
  if (!cuwacunu::hero::runtime::write_text_file_atomic(stdin_path, "", error)) {
    return false;
  }

  const std::vector<std::string> argv{
      app.defaults.super_hero_binary.string(), "--global-config",
      app.global_config_path.string(), "--tool", tool_name, "--args-json",
      arguments_json};
  int exit_code = -1;
  std::string invoke_error{};
  const bool invoked = run_command_with_stdio_and_timeout(
      argv, stdin_path, stdout_path, stderr_path, kSuperToolTimeoutSec,
      &exit_code, &invoke_error);

  std::string stdout_text{};
  std::string stderr_text{};
  std::string ignored{};
  (void)cuwacunu::hero::runtime::read_text_file(stdout_path, &stdout_text, &ignored);
  (void)cuwacunu::hero::runtime::read_text_file(stderr_path, &stderr_text, &ignored);
  cleanup_human_tool_io(stdin_path, stdout_path, stderr_path);

  if (!invoked && stdout_text.empty()) {
    if (error) {
      *error = !trim_ascii(stderr_text).empty() ? trim_ascii(stderr_text)
                                                : invoke_error;
    }
    return false;
  }
  if (stdout_text.empty()) {
    if (error) {
      *error = !trim_ascii(stderr_text).empty()
                   ? trim_ascii(stderr_text)
                   : ("super tool call produced no stdout: " + tool_name);
    }
    return false;
  }
  if (!tool_result_structured_content(stdout_text, out_structured, error)) {
    return false;
  }
  if (exit_code != 0) {
    if (error) {
      *error = !trim_ascii(stderr_text).empty()
                   ? trim_ascii(stderr_text)
                   : ("super tool exited non-zero: " + tool_name);
    }
    return false;
  }
  return true;
}

[[nodiscard]] std::string request_excerpt(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  std::string text{};
  std::string ignored{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.human_request_path, &text,
                                               &ignored)) {
    return {};
  }
  std::istringstream in(text);
  std::ostringstream out;
  std::string line{};
  std::size_t count = 0;
  while (count < 8 && std::getline(in, line)) {
    if (count != 0) out << "\n";
    out << line;
    ++count;
  }
  return out.str();
}

[[nodiscard]] std::string ellipsize_text(std::string_view in, int width) {
  if (width <= 0) return {};
  if (static_cast<int>(in.size()) <= width) return std::string(in);
  if (width <= 3) return std::string(in.substr(0, static_cast<std::size_t>(width)));
  return std::string(in.substr(0, static_cast<std::size_t>(width - 3))) + "...";
}

[[nodiscard]] std::vector<std::string> wrap_text_lines(std::string_view text,
                                                       int width) {
  std::vector<std::string> lines{};
  if (width <= 1) {
    lines.emplace_back();
    return lines;
  }
  std::istringstream input{std::string(text)};
  std::string raw{};
  while (std::getline(input, raw)) {
    if (raw.empty()) {
      lines.emplace_back();
      continue;
    }
    std::istringstream words(raw);
    std::string word{};
    std::string current{};
    while (words >> word) {
      while (static_cast<int>(word.size()) > width) {
        if (!current.empty()) {
          lines.push_back(current);
          current.clear();
        }
        lines.push_back(word.substr(0, static_cast<std::size_t>(width)));
        word.erase(0, static_cast<std::size_t>(width));
      }
      if (current.empty()) {
        current = word;
        continue;
      }
      if (static_cast<int>(current.size() + 1 + word.size()) <= width) {
        current.append(" ").append(word);
      } else {
        lines.push_back(current);
        current = word;
      }
    }
    if (!current.empty()) lines.push_back(current);
  }
  if (lines.empty()) lines.emplace_back();
  return lines;
}

void init_human_ncurses_theme() {
  if (!has_colors()) return;
  init_pair(1, COLOR_CYAN, -1);
  init_pair(2, COLOR_BLACK, COLOR_CYAN);
  init_pair(3, COLOR_YELLOW, -1);
  init_pair(4, COLOR_RED, -1);
  init_pair(5, COLOR_GREEN, -1);
}

void draw_boxed_window(WINDOW* win, std::string_view title) {
  if (!win) return;
  werase(win);
  box(win, 0, 0);
  if (!title.empty()) {
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    mvwaddnstr(win, 0, 2, title.data(), static_cast<int>(title.size()));
    wattroff(win, COLOR_PAIR(1) | A_BOLD);
  }
}

void draw_wrapped_block(WINDOW* win, int y, int x, int width, int height,
                        std::string_view text) {
  if (!win || width <= 0 || height <= 0) return;
  const auto lines = wrap_text_lines(text, width);
  for (int i = 0; i < height; ++i) {
    mvwaddnstr(win, y + i, x, std::string(width, ' ').c_str(), width);
  }
  for (int i = 0; i < height && i < static_cast<int>(lines.size()); ++i) {
    mvwaddnstr(win, y + i, x, lines[static_cast<std::size_t>(i)].c_str(), width);
  }
}

[[nodiscard]] bool prompt_text_dialog(const std::string& title,
                                      const std::string& label,
                                      std::string* out_value, bool secret,
                                      bool allow_empty, bool* cancelled) {
  if (cancelled) *cancelled = false;
  if (!out_value) return false;
  std::string buffer = *out_value;
  const int height = 8;
  const int width = std::max(24, std::min(std::max(24, COLS - 2), 96));
  const int starty = std::max(0, (LINES - height) / 2);
  const int startx = std::max(0, (COLS - width) / 2);
  WINDOW* win = newwin(height, width, starty, startx);
  if (!win) return false;
  keypad(win, TRUE);
  curs_set(1);

  for (;;) {
    draw_boxed_window(win, title);
    mvwaddnstr(win, 2, 2, label.c_str(), width - 4);
    mvwaddnstr(win, height - 2, 2, "Enter=accept  Esc=cancel  Backspace=edit",
               width - 4);
    const int field_width = width - 4;
    std::string shown = secret ? std::string(buffer.size(), '*') : buffer;
    shown = ellipsize_text(shown, field_width - 1);
    mvwaddnstr(win, 4, 2, std::string(field_width, ' ').c_str(), field_width);
    mvwaddnstr(win, 4, 2, shown.c_str(), field_width - 1);
    wmove(win, 4, 2 + std::min<int>(static_cast<int>(shown.size()),
                                    std::max(0, field_width - 1)));
    wrefresh(win);

    const int ch = wgetch(win);
    if (ch == 27) {
      if (cancelled) *cancelled = true;
      delwin(win);
      curs_set(0);
      return true;
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      if (!allow_empty && trim_ascii(buffer).empty()) continue;
      *out_value = trim_ascii(buffer);
      delwin(win);
      curs_set(0);
      return true;
    }
    if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
      if (!buffer.empty()) buffer.pop_back();
      continue;
    }
    if (ch >= 32 && ch <= 126) {
      buffer.push_back(static_cast<char>(ch));
    }
  }
}

[[nodiscard]] bool prompt_choice_dialog(const std::string& title,
                                        const std::string& prompt,
                                        const std::vector<std::string>& options,
                                        std::size_t* out_index,
                                        bool* cancelled) {
  if (cancelled) *cancelled = false;
  if (!out_index || options.empty()) return false;
  std::size_t selected = std::min<std::size_t>(*out_index, options.size() - 1);
  const int height = std::max(8, static_cast<int>(options.size()) + 6);
  const int width = std::max(24, std::min(std::max(24, COLS - 2), 84));
  const int starty = std::max(0, (LINES - height) / 2);
  const int startx = std::max(0, (COLS - width) / 2);
  WINDOW* win = newwin(height, width, starty, startx);
  if (!win) return false;
  keypad(win, TRUE);

  for (;;) {
    draw_boxed_window(win, title);
    draw_wrapped_block(win, 2, 2, width - 4, 2, prompt);
    for (std::size_t i = 0; i < options.size(); ++i) {
      const int row = 4 + static_cast<int>(i);
      if (i == selected) wattron(win, COLOR_PAIR(2) | A_BOLD);
      mvwaddnstr(win, row, 2, std::string(width - 4, ' ').c_str(), width - 4);
      const std::string line = (i == selected ? "> " : "  ") + options[i];
      mvwaddnstr(win, row, 2, line.c_str(), width - 4);
      if (i == selected) wattroff(win, COLOR_PAIR(2) | A_BOLD);
    }
    mvwaddnstr(win, height - 2, 2,
               "Up/Down=move  Enter=accept  Esc=cancel",
               width - 4);
    wrefresh(win);

    const int ch = wgetch(win);
    if (ch == 27) {
      if (cancelled) *cancelled = true;
      delwin(win);
      return true;
    }
    if (ch == KEY_UP || ch == 'k') {
      if (selected > 0) --selected;
      continue;
    }
    if (ch == KEY_DOWN || ch == 'j') {
      if (selected + 1 < options.size()) ++selected;
      continue;
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      *out_index = selected;
      delwin(win);
      return true;
    }
  }
}

[[nodiscard]] bool prompt_yes_no_dialog(const std::string& title,
                                        const std::string& prompt,
                                        bool default_yes, bool* out_value,
                                        bool* cancelled) {
  if (!out_value) return false;
  std::size_t idx = default_yes ? 1u : 0u;
  if (!prompt_choice_dialog(title, prompt, {"No", "Yes"}, &idx, cancelled)) {
    return false;
  }
  if (cancelled && *cancelled) return true;
  *out_value = (idx == 1u);
  return true;
}

[[nodiscard]] bool read_request_text_for_loop(
    const cuwacunu::hero::super::super_loop_record_t& loop, std::string* out,
    std::string* error) {
  if (!out) {
    if (error) *error = "request text output pointer is null";
    return false;
  }
  return cuwacunu::hero::runtime::read_text_file(loop.human_request_path, out, error);
}

void render_human_requests_screen(
    const std::vector<cuwacunu::hero::super::super_loop_record_t>& pending,
    std::size_t selected, std::string_view request_text,
    std::string_view status_line, bool status_is_error) {
  erase();
  const int W = COLS;
  const int H = LINES;
  if (W < 24 || H < 10) {
    mvaddnstr(0, 0, "Human Hero: terminal too small. Resize or use hero.human.* tools.",
              std::max(0, W - 1));
    refresh();
    return;
  }
  const int header_h = 3;
  const int footer_h = 3;
  const int body_h = std::max(5, H - header_h - footer_h);
  const int left_w = std::max(28, std::min(W / 3, 42));
  const int right_w = std::max(20, W - left_w);

  WINDOW* header = newwin(header_h, W, 0, 0);
  WINDOW* left = newwin(body_h, left_w, header_h, 0);
  WINDOW* right = newwin(body_h, right_w, header_h, left_w);
  WINDOW* footer = newwin(footer_h, W, header_h + body_h, 0);
  if (!header || !left || !right || !footer) {
    if (header) delwin(header);
    if (left) delwin(left);
    if (right) delwin(right);
    if (footer) delwin(footer);
    mvaddnstr(0, 0, "Human Hero: unable to allocate ncurses windows.",
              std::max(0, W - 1));
    refresh();
    return;
  }

  draw_boxed_window(header, " Human Hero ");
  wattron(header, COLOR_PAIR(1) | A_BOLD);
  mvwaddnstr(header, 1, 2,
             "Pending human requests for Super Hero loops",
             W - 4);
  wattroff(header, COLOR_PAIR(1) | A_BOLD);

  draw_boxed_window(left, " Requests ");
  if (pending.empty()) {
    mvwaddnstr(left, 2, 2, "No pending requests.", left_w - 4);
    mvwaddnstr(left, 3, 2, "Press r to refresh or q to exit.", left_w - 4);
  } else {
    const int max_rows = body_h - 2;
    std::size_t top = 0;
    if (selected >= static_cast<std::size_t>(max_rows)) {
      top = selected - static_cast<std::size_t>(max_rows) + 1;
    }
    for (int row = 0; row < max_rows; ++row) {
      const std::size_t idx = top + static_cast<std::size_t>(row);
      if (idx >= pending.size()) break;
      const auto& loop = pending[idx];
      const std::string label = loop.loop_id + " | " + loop.objective_name;
      if (idx == selected) wattron(left, COLOR_PAIR(2) | A_BOLD);
      mvwaddnstr(left, 1 + row, 1, std::string(left_w - 2, ' ').c_str(), left_w - 2);
      mvwaddnstr(left, 1 + row, 2, ellipsize_text(label, left_w - 4).c_str(),
                 left_w - 4);
      if (idx == selected) wattroff(left, COLOR_PAIR(2) | A_BOLD);
    }
  }

  draw_boxed_window(right, " Request ");
  if (!pending.empty()) {
    const auto& loop = pending[selected];
    std::string meta = "loop=" + loop.loop_id + "  review=" +
                       std::to_string(loop.review_count);
    mvwaddnstr(right, 1, 2, ellipsize_text(meta, right_w - 4).c_str(), right_w - 4);
    mvwaddnstr(right, 2, 2,
               ellipsize_text(loop.state_detail.empty() ? loop.objective_name
                                                        : loop.state_detail,
                              right_w - 4)
                   .c_str(),
               right_w - 4);
    draw_wrapped_block(right, 4, 2, right_w - 4, body_h - 6, request_text);
  }

  draw_boxed_window(footer, " Controls ");
  mvwaddnstr(footer, 1, 2,
             "Up/Down select  Enter/c continue  s stop  r refresh  q quit",
             W - 4);
  if (!status_line.empty()) {
    wattron(footer, COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);
    mvwaddnstr(footer, 1, std::max(2, W / 2),
               ellipsize_text(status_line, std::max(8, W / 2 - 3)).c_str(),
               std::max(8, W / 2 - 3));
    wattroff(footer, COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);
  }

  wrefresh(header);
  wrefresh(left);
  wrefresh(right);
  wrefresh(footer);
  delwin(header);
  delwin(left);
  delwin(right);
  delwin(footer);
}

[[nodiscard]] bool collect_interactive_response_inputs(
    app_context_t* app,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    bool stop_direct, interactive_response_input_t* out, bool* cancelled,
    std::string* error) {
  if (cancelled) *cancelled = false;
  if (error) error->clear();
  if (!app || !out) {
    if (error) *error = "interactive response output pointer is null";
    return false;
  }
  *out = interactive_response_input_t{};
  out->control_kind = stop_direct ? "stop" : "continue";

  if (out->control_kind == "continue") {
    std::size_t action_idx = 0;
    if (!prompt_choice_dialog(" Continue ",
                              "Choose the next runtime action for this loop.",
                              {"default_plan", "binding"}, &action_idx,
                              cancelled)) {
      return false;
    }
    if (cancelled && *cancelled) return true;
    out->next_action_kind = (action_idx == 0) ? "default_plan" : "binding";
    if (out->next_action_kind == "binding") {
      if (!prompt_text_dialog(" Binding target ", "Target binding id",
                              &out->target_binding_id, false, false,
                              cancelled)) {
        return false;
      }
      if (cancelled && *cancelled) return true;
    }
    if (!prompt_yes_no_dialog(" Runtime reset ",
                              "Request a cold Runtime reset before launch?",
                              false, &out->reset_runtime_state, cancelled)) {
      return false;
    }
    if (cancelled && *cancelled) return true;
  }

  if (!prompt_text_dialog(stop_direct ? " Stop reason " : " Continue reason ",
                          "Reason",
                          &out->reason, false, false, cancelled)) {
    return false;
  }
  if (cancelled && *cancelled) return true;

  if (!prompt_text_dialog(" Memory note ",
                          "Memory note (optional)",
                          &out->memory_note, false, true, cancelled)) {
    return false;
  }
  if (cancelled && *cancelled) return true;

  (void)loop;
  return true;
}

[[nodiscard]] bool run_ncurses_request_responder(app_context_t* app,
                                                 std::string* error) {
  if (error) error->clear();
  if (!app) {
    if (error) *error = "human app pointer is null";
    return false;
  }

  try {
    const char* term = std::getenv("TERM");
    if (term == nullptr || trim_ascii(term).empty() ||
        trim_ascii(term) == "dumb") {
      ::setenv("TERM", "xterm-256color", 1);
    }
    cuwacunu::iinuji::NcursesAppOpts opts{};
    opts.input_timeout_ms = -1;
    cuwacunu::iinuji::NcursesApp curses_app(opts);
    init_human_ncurses_theme();

    std::vector<cuwacunu::hero::super::super_loop_record_t> pending{};
    std::string load_error{};
    if (!collect_pending_requests(*app, &pending, &load_error)) {
      if (error) *error = load_error;
      return false;
    }
    std::sort(pending.begin(), pending.end(), [](const auto& a, const auto& b) {
      if (a.updated_at_ms != b.updated_at_ms) return a.updated_at_ms > b.updated_at_ms;
      return a.loop_id > b.loop_id;
    });

    std::size_t selected = 0;
    std::string request_text{};
    std::string status{};
    bool status_is_error = false;

    for (;;) {
      if (!pending.empty()) selected = std::min(selected, pending.size() - 1);
      request_text.clear();
      if (!pending.empty()) {
        std::string request_error{};
        if (!read_request_text_for_loop(pending[selected], &request_text,
                                        &request_error)) {
          request_text = "<failed to read request: " + request_error + ">";
        }
      }
      render_human_requests_screen(pending, selected, request_text, status,
                                   status_is_error);
      const int ch = getch();
      if (ch == 'q' || ch == 'Q' || ch == 27) return true;
      if (ch == KEY_RESIZE) continue;
      if (ch == 'r' || ch == 'R') {
        std::string refresh_error{};
        if (!collect_pending_requests(*app, &pending, &refresh_error)) {
          status = refresh_error;
          status_is_error = true;
        } else {
          std::sort(pending.begin(), pending.end(),
                    [](const auto& a, const auto& b) {
                      if (a.updated_at_ms != b.updated_at_ms) {
                        return a.updated_at_ms > b.updated_at_ms;
                      }
                      return a.loop_id > b.loop_id;
                    });
          if (!pending.empty()) selected = std::min(selected, pending.size() - 1);
          status = pending.empty() ? "No pending requests." : "Refreshed.";
          status_is_error = false;
        }
        continue;
      }
      if (pending.empty()) continue;
      if (ch == KEY_UP || ch == 'k') {
        if (selected > 0) --selected;
        continue;
      }
      if (ch == KEY_DOWN || ch == 'j') {
        if (selected + 1 < pending.size()) ++selected;
        continue;
      }
      if (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == 'c' || ch == 'C' ||
          ch == 's' || ch == 'S') {
        const bool stop_direct = (ch == 's' || ch == 'S');
        interactive_response_input_t input{};
        bool cancelled = false;
        std::string input_error{};
        if (!collect_interactive_response_inputs(app, pending[selected],
                                                 stop_direct, &input,
                                                 &cancelled, &input_error)) {
          status = input_error;
          status_is_error = true;
          continue;
        }
        if (cancelled) {
          status = "Cancelled.";
          status_is_error = false;
          continue;
        }
        std::string structured{};
        std::string response_error{};
        if (!build_response_and_resume(app, pending[selected], input.control_kind,
                                       input.next_action_kind,
                                       input.target_binding_id,
                                       input.reset_runtime_state, input.reason,
                                       input.memory_note, &structured,
                                       &response_error)) {
          status = response_error;
          status_is_error = true;
          continue;
        }
        std::string refresh_error{};
        if (!collect_pending_requests(*app, &pending, &refresh_error)) {
          status = refresh_error;
          status_is_error = true;
          continue;
        }
        std::sort(pending.begin(), pending.end(), [](const auto& a, const auto& b) {
          if (a.updated_at_ms != b.updated_at_ms) return a.updated_at_ms > b.updated_at_ms;
          return a.loop_id > b.loop_id;
        });
        if (!pending.empty()) selected = std::min(selected, pending.size() - 1);
        status = "Applied signed human response.";
        status_is_error = false;
      }
    }
  } catch (const std::exception& ex) {
    if (error) *error = std::string(kNcursesInitErrorPrefix) + ex.what();
    return false;
  }
}

[[nodiscard]] std::string pending_request_row_to_json(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  std::ostringstream out;
  out << "{"
      << "\"loop_id\":" << json_quote(loop.loop_id) << ","
      << "\"objective_name\":" << json_quote(loop.objective_name) << ","
      << "\"state\":" << json_quote(loop.state) << ","
      << "\"state_detail\":" << json_quote(loop.state_detail) << ","
      << "\"review_count\":" << loop.review_count << ","
      << "\"started_at_ms\":" << loop.started_at_ms << ","
      << "\"updated_at_ms\":" << loop.updated_at_ms << ","
      << "\"human_request_path\":" << json_quote(loop.human_request_path) << ","
      << "\"human_response_path\":"
      << json_quote(
             cuwacunu::hero::super::super_loop_human_response_latest_path(
                 std::filesystem::path(loop.loop_root).parent_path(),
                 loop.loop_id)
                 .string())
      << ",\"human_response_sig_path\":"
      << json_quote(
             cuwacunu::hero::super::super_loop_human_response_latest_sig_path(
                 std::filesystem::path(loop.loop_root).parent_path(),
                 loop.loop_id)
                 .string())
      << ",\"request_excerpt\":" << json_quote(request_excerpt(loop))
      << "}";
  return out.str();
}

[[nodiscard]] bool collect_pending_requests(
    const app_context_t& app,
    std::vector<cuwacunu::hero::super::super_loop_record_t>* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "pending requests output pointer is null";
    return false;
  }
  out->clear();
  std::vector<cuwacunu::hero::super::super_loop_record_t> loops{};
  if (!cuwacunu::hero::super::scan_super_loop_records(app.defaults.super_root,
                                                      &loops, error)) {
    return false;
  }
  for (const auto& loop : loops) {
    if (loop.state == "need_human") out->push_back(loop);
  }
  return true;
}

[[nodiscard]] bool build_response_and_resume(
    app_context_t* app,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::string control_kind, std::string next_action_kind,
    std::string target_binding_id, bool reset_runtime_state, std::string reason,
    std::string memory_note, std::string* out_structured,
    std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !out_structured || !out_error) return false;

  control_kind = trim_ascii(control_kind);
  next_action_kind = trim_ascii(next_action_kind);
  target_binding_id = trim_ascii(target_binding_id);
  reason = trim_ascii(reason);
  memory_note = trim_ascii(memory_note);
  if (control_kind != "continue" && control_kind != "stop") {
    *out_error = "human response control_kind must be continue or stop";
    return false;
  }
  if (control_kind == "continue" &&
      next_action_kind != "default_plan" && next_action_kind != "binding") {
    *out_error =
        "human response continue requires next_action.kind = default_plan or binding";
    return false;
  }
  if (control_kind == "continue" && next_action_kind == "binding" &&
      target_binding_id.empty()) {
    *out_error =
        "human response binding requires next_action.target_binding_id";
    return false;
  }
  if (reason.empty()) {
    *out_error = "human response requires a non-empty reason";
    return false;
  }
  if (app->defaults.operator_id.empty() ||
      app->defaults.operator_id == "CHANGE_ME_OPERATOR") {
    *out_error =
        "Human Hero operator_id is not configured; update default.hero.human.dsl";
    return false;
  }
  if (app->defaults.operator_signing_ssh_identity.empty()) {
    *out_error =
        "Human Hero operator_signing_ssh_identity is not configured";
    return false;
  }

  std::string request_sha256_hex{};
  if (!cuwacunu::hero::human::sha256_hex_file(loop.human_request_path,
                                              &request_sha256_hex, out_error)) {
    return false;
  }

  human_response_record_t response{};
  response.loop_id = loop.loop_id;
  response.review_index = loop.review_count;
  response.request_sha256_hex = request_sha256_hex;
  response.operator_id = app->defaults.operator_id;
  response.responded_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  response.control_kind = control_kind;
  response.next_action_kind = (control_kind == "continue") ? next_action_kind : "";
  response.target_binding_id =
      (response.next_action_kind == "binding") ? target_binding_id : "";
  response.reset_runtime_state =
      (control_kind == "continue") ? reset_runtime_state : false;
  response.reason = reason;
  response.memory_note = memory_note;
  response.signer_public_key_fingerprint_sha256_hex =
      std::string(64, '0');

  std::string signature_hex{};
  std::string fingerprint_hex{};
  std::string provisional_json = cuwacunu::hero::human::human_response_to_json(response);
  if (!cuwacunu::hero::human::sign_human_response_json(
          app->defaults.operator_signing_ssh_identity, provisional_json,
          &signature_hex, &fingerprint_hex, out_error)) {
    return false;
  }
  response.signer_public_key_fingerprint_sha256_hex = fingerprint_hex;
  const std::string response_json =
      cuwacunu::hero::human::human_response_to_json(response);
  if (!cuwacunu::hero::human::sign_human_response_json(
          app->defaults.operator_signing_ssh_identity, response_json,
          &signature_hex, &fingerprint_hex, out_error)) {
    return false;
  }
  if (fingerprint_hex != response.signer_public_key_fingerprint_sha256_hex) {
    *out_error = "human response fingerprint changed during signing";
    return false;
  }

  std::error_code ec{};
  const auto response_dir = cuwacunu::hero::super::super_loop_human_responses_dir(
      app->defaults.super_root, loop.loop_id);
  std::filesystem::create_directories(response_dir, ec);
  if (ec) {
    *out_error = "cannot create human response directory: " + response_dir.string();
    return false;
  }
  const auto response_path = cuwacunu::hero::super::super_loop_human_response_path(
      app->defaults.super_root, loop.loop_id, loop.review_count);
  const auto response_sig_path =
      cuwacunu::hero::super::super_loop_human_response_sig_path(
          app->defaults.super_root, loop.loop_id, loop.review_count);
  const auto latest_path =
      cuwacunu::hero::super::super_loop_human_response_latest_path(
          app->defaults.super_root, loop.loop_id);
  const auto latest_sig_path =
      cuwacunu::hero::super::super_loop_human_response_latest_sig_path(
          app->defaults.super_root, loop.loop_id);

  if (!cuwacunu::hero::runtime::write_text_file_atomic(response_path, response_json,
                                                       out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(response_sig_path,
                                                       signature_hex + "\n",
                                                       out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(latest_path, response_json,
                                                       out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(latest_sig_path,
                                                       signature_hex + "\n",
                                                       out_error)) {
    return false;
  }

  std::string super_structured{};
  std::string resume_args =
      "{\"loop_id\":" + json_quote(loop.loop_id) + ",\"human_response_path\":" +
      json_quote(response_path.string()) + ",\"human_response_sig_path\":" +
      json_quote(response_sig_path.string()) + "}";
  if (!call_super_tool(*app, "hero.super.resume_loop", resume_args,
                       &super_structured, out_error)) {
    return false;
  }

  *out_structured = "{\"loop_id\":" + json_quote(loop.loop_id) +
                    ",\"response_path\":" + json_quote(response_path.string()) +
                    ",\"response_sig_path\":" +
                    json_quote(response_sig_path.string()) +
                    ",\"fingerprint_sha256_hex\":" + json_quote(fingerprint_hex) +
                    ",\"super\":" + super_structured + "}";
  return true;
}

[[nodiscard]] bool handle_tool_list_requests(app_context_t* app,
                                             const std::string& arguments_json,
                                             std::string* out_structured,
                                             std::string* out_error);
[[nodiscard]] bool handle_tool_get_request(app_context_t* app,
                                           const std::string& arguments_json,
                                           std::string* out_structured,
                                           std::string* out_error);
[[nodiscard]] bool handle_tool_respond(app_context_t* app,
                                       const std::string& arguments_json,
                                       std::string* out_structured,
                                       std::string* out_error);

#define HERO_HUMAN_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER) \
  {NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
constexpr human_tool_descriptor_t kHumanTools[] = {
#include "hero/human_hero/hero_human_tools.def"
};
#undef HERO_HUMAN_TOOL

[[nodiscard]] const human_tool_descriptor_t* find_human_tool_descriptor(
    std::string_view tool_name) {
  for (const auto& tool : kHumanTools) {
    if (tool.name == tool_name) return &tool;
  }
  return nullptr;
}

[[nodiscard]] bool handle_tool_list_requests(app_context_t* app,
                                             const std::string& arguments_json,
                                             std::string* out_structured,
                                             std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::size_t limit = 0;
  std::size_t offset = 0;
  bool newest_first = true;
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);
  (void)extract_json_bool_field(arguments_json, "newest_first", &newest_first);

  std::vector<cuwacunu::hero::super::super_loop_record_t> loops{};
  if (!collect_pending_requests(*app, &loops, out_error)) return false;
  std::sort(loops.begin(), loops.end(), [newest_first](const auto& a,
                                                       const auto& b) {
    if (a.updated_at_ms != b.updated_at_ms) {
      return newest_first ? (a.updated_at_ms > b.updated_at_ms)
                          : (a.updated_at_ms < b.updated_at_ms);
    }
    return newest_first ? (a.loop_id > b.loop_id) : (a.loop_id < b.loop_id);
  });
  const std::size_t total = loops.size();
  const std::size_t off = std::min(offset, loops.size());
  std::size_t count = limit;
  if (count == 0) count = loops.size() - off;
  count = std::min(count, loops.size() - off);

  std::ostringstream rows;
  rows << "[";
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0) rows << ",";
    rows << pending_request_row_to_json(loops[off + i]);
  }
  rows << "]";

  *out_structured =
      "{\"loop_id\":\"\",\"count\":" + std::to_string(count) +
      ",\"total\":" + std::to_string(total) + ",\"requests\":" + rows.str() +
      "}";
  return true;
}

[[nodiscard]] bool handle_tool_get_request(app_context_t* app,
                                           const std::string& arguments_json,
                                           std::string* out_structured,
                                           std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string loop_id{};
  (void)extract_json_string_field(arguments_json, "loop_id", &loop_id);
  loop_id = trim_ascii(loop_id);
  if (loop_id.empty()) {
    *out_error = "get_request requires arguments.loop_id";
    return false;
  }

  cuwacunu::hero::super::super_loop_record_t loop{};
  if (!cuwacunu::hero::super::read_super_loop_record(app->defaults.super_root, loop_id,
                                                     &loop, out_error)) {
    return false;
  }
  if (loop.state != "need_human") {
    *out_error = "loop is not currently waiting for human input";
    return false;
  }

  std::string request_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.human_request_path,
                                               &request_text, out_error)) {
    return false;
  }
  *out_structured = "{\"loop_id\":" + json_quote(loop_id) + ",\"loop\":" +
                    pending_request_row_to_json(loop) + ",\"request_text\":" +
                    json_quote(request_text) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_respond(app_context_t* app,
                                       const std::string& arguments_json,
                                       std::string* out_structured,
                                       std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string loop_id{};
  std::string control_kind{};
  std::string next_action_json{};
  std::string reason{};
  std::string memory_note{};
  (void)extract_json_string_field(arguments_json, "loop_id", &loop_id);
  (void)extract_json_string_field(arguments_json, "control_kind", &control_kind);
  (void)extract_json_object_field(arguments_json, "next_action", &next_action_json);
  (void)extract_json_string_field(arguments_json, "reason", &reason);
  (void)extract_json_string_field(arguments_json, "memory_note", &memory_note);
  loop_id = trim_ascii(loop_id);
  if (loop_id.empty()) {
    *out_error = "respond requires arguments.loop_id";
    return false;
  }
  if (trim_ascii(control_kind).empty()) {
    *out_error = "respond requires arguments.control_kind";
    return false;
  }

  cuwacunu::hero::super::super_loop_record_t loop{};
  if (!cuwacunu::hero::super::read_super_loop_record(app->defaults.super_root, loop_id,
                                                     &loop, out_error)) {
    return false;
  }
  if (loop.state != "need_human") {
    *out_error = "loop is not currently waiting for human input";
    return false;
  }

  std::string next_action_kind{};
  std::string target_binding_id{};
  bool reset_runtime_state = false;
  if (!trim_ascii(next_action_json).empty()) {
    (void)extract_json_string_field(next_action_json, "kind", &next_action_kind);
    (void)extract_json_string_field(next_action_json, "target_binding_id",
                                    &target_binding_id);
    (void)extract_json_bool_field(next_action_json, "reset_runtime_state",
                                  &reset_runtime_state);
  }

  return build_response_and_resume(app, loop, control_kind, next_action_kind,
                                   target_binding_id, reset_runtime_state, reason,
                                   memory_note, out_structured, out_error);
}

}  // namespace

std::filesystem::path resolve_human_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "REAL_HERO", "human_hero_dsl_filename");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved);
}

bool load_human_defaults(const std::filesystem::path& hero_dsl_path,
                         const std::filesystem::path& global_config_path,
                         human_defaults_t* out, std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "human defaults output pointer is null";
    return false;
  }
  *out = human_defaults_t{};

  std::ifstream in(hero_dsl_path);
  if (!in) {
    if (error) {
      *error = "cannot open Human HERO defaults DSL: " + hero_dsl_path.string();
    }
    return false;
  }

  std::unordered_map<std::string, std::string> values{};
  std::string line{};
  bool in_block_comment = false;
  while (std::getline(in, line)) {
    std::string candidate = line;
    if (in_block_comment) {
      const std::size_t end_block = candidate.find("*/");
      if (end_block == std::string::npos) continue;
      candidate = candidate.substr(end_block + 2);
      in_block_comment = false;
    }
    const std::size_t start_block = candidate.find("/*");
    if (start_block != std::string::npos) {
      const std::size_t end_block = candidate.find("*/", start_block + 2);
      if (end_block == std::string::npos) {
        candidate = candidate.substr(0, start_block);
        in_block_comment = true;
      } else {
        candidate.erase(start_block, end_block - start_block + 2);
      }
    }
    candidate = trim_ascii(strip_inline_comment(candidate));
    if (candidate.empty()) continue;
    const std::size_t eq = candidate.find('=');
    if (eq == std::string::npos) continue;
    std::string lhs = trim_ascii(candidate.substr(0, eq));
    std::string rhs = unquote_if_wrapped(candidate.substr(eq + 1));
    lhs = cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(lhs);
    if (lhs.empty()) continue;
    values[lhs] = trim_ascii(rhs);
  }

  out->super_root = cuwacunu::hero::super::super_root(
      resolve_runtime_root_from_global_config(global_config_path));
  const auto resolve_exec = [&](const char* key, std::filesystem::path* dst) {
    const auto it = values.find(key);
    if (it == values.end()) return false;
    *dst = resolve_command_path(hero_dsl_path.parent_path(), it->second);
    return !dst->empty();
  };
  if (!resolve_exec("super_hero_binary", &out->super_hero_binary)) {
    if (error) *error = "missing/invalid super_hero_binary in " + hero_dsl_path.string();
    return false;
  }
  out->operator_id = trim_ascii(values["operator_id"]);
  const auto ssh_identity_it = values.find("operator_signing_ssh_identity");
  if (ssh_identity_it != values.end() &&
      !trim_ascii(ssh_identity_it->second).empty()) {
    out->operator_signing_ssh_identity = std::filesystem::path(
        resolve_path_from_base_folder(hero_dsl_path.parent_path().string(),
                                      ssh_identity_it->second));
  }
  if (out->super_root.empty()) {
    if (error) {
      *error = "cannot derive .super_hero root from GENERAL.runtime_root in " +
               global_config_path.string();
    }
    return false;
  }
  return true;
}

std::filesystem::path current_executable_path() {
  std::array<char, 4096> buf{};
  const ssize_t n = ::readlink("/proc/self/exe", buf.data(), buf.size() - 1);
  if (n <= 0) return {};
  buf[static_cast<std::size_t>(n)] = '\0';
  return std::filesystem::path(buf.data()).lexically_normal();
}

std::string build_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < std::size(kHumanTools); ++i) {
    const auto& tool = kHumanTools[i];
    if (i != 0) out << ",";
    out << "{\"name\":" << json_quote(tool.name)
        << ",\"description\":" << json_quote(tool.description)
        << ",\"inputSchema\":" << tool.input_schema_json << "}";
  }
  out << "]}";
  return out.str();
}

std::string build_tools_list_human_text() {
  std::ostringstream out;
  for (const auto& tool : kHumanTools) {
    out << tool.name << "\t" << tool.description << "\n";
  }
  return out.str();
}

bool tool_result_is_error(std::string_view tool_result_json) {
  return tool_result_json.find("\"isError\":true") != std::string_view::npos ||
         tool_result_json.find("\"isError\": true") != std::string_view::npos;
}

bool execute_tool_json(const std::string& tool_name, std::string arguments_json,
                       app_context_t* app, std::string* out_tool_result_json,
                       std::string* out_error_message) {
  if (out_tool_result_json) out_tool_result_json->clear();
  if (out_error_message) out_error_message->clear();
  if (!app) {
    if (out_error_message) *out_error_message = "app context pointer is null";
    return false;
  }
  if (!out_tool_result_json || !out_error_message) return false;

  arguments_json = trim_ascii(arguments_json);
  if (arguments_json.empty()) arguments_json = "{}";

  const auto* descriptor = find_human_tool_descriptor(tool_name);
  if (descriptor == nullptr) {
    *out_error_message = "unknown tool: " + tool_name;
    return false;
  }

  std::string structured{};
  std::string err{};
  const bool ok = descriptor->handler(app, arguments_json, &structured, &err);
  if (!ok) {
    *out_tool_result_json = tool_result_json_for_error(err);
    return true;
  }
  *out_tool_result_json = make_tool_result_json("ok", structured, false);
  return true;
}

[[nodiscard]] bool run_line_prompt_request_responder(app_context_t* app,
                                                     std::string* error) {
  if (error) error->clear();
  if (!app) {
    if (error) *error = "human app pointer is null";
    return false;
  }

  std::vector<cuwacunu::hero::super::super_loop_record_t> pending{};
  if (!collect_pending_requests(*app, &pending, error)) return false;
  if (pending.empty()) {
    std::cout << "No pending human requests.\n";
    return true;
  }

  std::sort(pending.begin(), pending.end(), [](const auto& a, const auto& b) {
    if (a.updated_at_ms != b.updated_at_ms) return a.updated_at_ms > b.updated_at_ms;
    return a.loop_id > b.loop_id;
  });

  std::size_t selected = 0;
  if (pending.size() > 1) {
    std::cout << "Pending human requests:\n";
    for (std::size_t i = 0; i < pending.size(); ++i) {
      std::cout << "  [" << i + 1 << "] " << pending[i].loop_id << "  "
                << pending[i].objective_name << "  "
                << pending[i].state_detail << "\n";
    }
    std::cout << "Select request number (blank to cancel): " << std::flush;
    std::string line{};
    if (!std::getline(std::cin, line)) return false;
    line = trim_ascii(line);
    if (line.empty()) return true;
    std::size_t choice = 0;
    if (!parse_size_token(line, &choice) || choice == 0 || choice > pending.size()) {
      if (error) *error = "invalid request selection";
      return false;
    }
    selected = choice - 1;
  }
  const auto& loop = pending[selected];
  std::string request_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.human_request_path,
                                               &request_text, error)) {
    return false;
  }
  std::cout << "\n=== Human Request ===\n" << request_text << "\n";

  std::cout << "Respond with [c]ontinue, [s]top, or blank to cancel: " << std::flush;
  std::string control{};
  if (!std::getline(std::cin, control)) return false;
  control = lowercase_copy(trim_ascii(control));
  if (control.empty()) return true;
  std::string control_kind{};
  if (control == "c" || control == "continue") {
    control_kind = "continue";
  } else if (control == "s" || control == "stop") {
    control_kind = "stop";
  } else {
    if (error) *error = "invalid control selection";
    return false;
  }

  std::string next_action_kind{};
  std::string target_binding_id{};
  bool reset_runtime_state = false;
  if (control_kind == "continue") {
    std::cout << "Next action [d]efault_plan or [b]inding: " << std::flush;
    std::string action{};
    if (!std::getline(std::cin, action)) return false;
    action = lowercase_copy(trim_ascii(action));
    if (action == "d" || action == "default_plan") {
      next_action_kind = "default_plan";
    } else if (action == "b" || action == "binding") {
      next_action_kind = "binding";
      std::cout << "Target binding id: " << std::flush;
      if (!std::getline(std::cin, target_binding_id)) return false;
      target_binding_id = trim_ascii(target_binding_id);
      if (target_binding_id.empty()) {
        if (error) *error = "binding response requires target binding id";
        return false;
      }
    } else {
      if (error) *error = "invalid next action selection";
      return false;
    }
    std::cout << "Reset Runtime state? [y/N]: " << std::flush;
    std::string reset{};
    if (!std::getline(std::cin, reset)) return false;
    reset = lowercase_copy(trim_ascii(reset));
    reset_runtime_state = (reset == "y" || reset == "yes" || reset == "true");
  }

  std::cout << "Reason: " << std::flush;
  std::string reason{};
  if (!std::getline(std::cin, reason)) return false;
  reason = trim_ascii(reason);
  if (reason.empty()) {
    if (error) *error = "reason is required";
    return false;
  }
  std::cout << "Memory note (optional, single line): " << std::flush;
  std::string memory_note{};
  if (!std::getline(std::cin, memory_note)) return false;
  memory_note = trim_ascii(memory_note);

  std::string structured{};
  if (!build_response_and_resume(app, loop, control_kind, next_action_kind,
                                 target_binding_id, reset_runtime_state, reason,
                                 memory_note, &structured, error)) {
    return false;
  }
  std::cout << "\nHuman response applied.\n" << structured << "\n";
  return true;
}

bool run_interactive_request_responder(app_context_t* app, std::string* error) {
  if (error) error->clear();
  std::string ncurses_error{};
  if (run_ncurses_request_responder(app, &ncurses_error)) return true;
  if (!ncurses_error.empty() &&
      ncurses_error.rfind(kNcursesInitErrorPrefix, 0) == 0) {
    return run_line_prompt_request_responder(app, error);
  }
  if (error) *error = ncurses_error.empty() ? "interactive human responder failed"
                                            : ncurses_error;
  return false;
}

void run_jsonrpc_stdio_loop(app_context_t* app) {
  std::string line;
  while (std::getline(std::cin, line)) {
    const std::string request_json = trim_ascii(line);
    if (request_json.empty()) continue;
    if (request_json.size() > kMaxJsonRpcPayloadBytes) continue;

    std::string id_raw{"null"};
    std::string method{};
    std::string params_json{"{}"};
    if (extract_json_field_raw(request_json, "id", &id_raw)) {
      id_raw = trim_ascii(id_raw);
      if (id_raw.empty()) id_raw = "null";
    }
    if (!extract_json_string_field(request_json, "method", &method) ||
        method.empty()) {
      std::cout
          << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
          << ",\"error\":{\"code\":-32600,\"message\":\"invalid request\"}}\n";
      std::cout.flush();
      continue;
    }
    (void)extract_json_object_field(request_json, "params", &params_json);

    if (method == "initialize") {
      std::cout
          << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
          << ",\"result\":{\"protocolVersion\":" << json_quote(kProtocolVersion)
          << ",\"serverInfo\":{\"name\":" << json_quote(kServerName)
          << ",\"version\":" << json_quote(kServerVersion)
          << "},\"instructions\":" << json_quote(kInitializeInstructions)
          << "}}\n";
      std::cout.flush();
      continue;
    }
    if (method == "notifications/initialized") continue;
    if (method == "ping") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":{}}\n";
      std::cout.flush();
      continue;
    }
    if (method == "tools/list") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":" << build_tools_list_result_json() << "}\n";
      std::cout.flush();
      continue;
    }
    if (method == "tools/call") {
      std::string name{};
      std::string arguments{"{}"};
      if (!extract_json_string_field(params_json, "name", &name) ||
          name.empty()) {
        std::cout
            << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
            << ",\"error\":{\"code\":-32602,\"message\":\"missing tool name\"}}\n";
        std::cout.flush();
        continue;
      }
      std::string args_object{};
      if (extract_json_object_field(params_json, "arguments", &args_object)) {
        arguments = std::move(args_object);
      }
      std::string tool_result{};
      std::string tool_error{};
      if (!execute_tool_json(name, arguments, app, &tool_result, &tool_error)) {
        std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                  << ",\"error\":{\"code\":-32603,\"message\":"
                  << json_quote(tool_error) << "}}\n";
        std::cout.flush();
        continue;
      }
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":" << tool_result << "}\n";
      std::cout.flush();
      continue;
    }
    std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
              << ",\"error\":{\"code\":-32601,\"message\":\"method not found\"}}\n";
    std::cout.flush();
  }
}

}  // namespace human_mcp
}  // namespace hero
}  // namespace cuwacunu
