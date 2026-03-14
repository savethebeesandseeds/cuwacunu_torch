#include "hero/runtime_hero/hero_runtime_tools.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cctype>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <sys/wait.h>
#include <tuple>
#include <unordered_map>
#include <unistd.h>
#include <vector>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "hero/wave_contract_binding_runtime.h"
#include "hero/runtime_hero/runtime_job.h"

namespace cuwacunu {
namespace hero {
namespace runtime_mcp {

namespace {

constexpr const char* kDefaultRuntimeHeroDslPath =
    "/cuwacunu/src/config/instructions/default.hero.runtime.dsl";
constexpr const char* kDefaultConfigFilename = ".config";
constexpr const char* kDefaultBoardDslKey = "default_board_dsl_filename";
constexpr const char* kServerName = "hero_runtime_mcp";
constexpr const char* kServerVersion = "0.1.0";
constexpr const char* kProtocolVersion = "2025-03-26";
constexpr const char* kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.runtime.*.";
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;

bool g_jsonrpc_use_content_length_framing = false;

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

[[nodiscard]] std::string json_escape(std::string_view in) {
  std::ostringstream out;
  for (const unsigned char c : in) {
    switch (c) {
      case '"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
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
        if (c < 0x20) {
          static constexpr char kHex[] = "0123456789abcdef";
          out << "\\u00" << kHex[(c >> 4) & 0x0F] << kHex[c & 0x0F];
        } else {
          out << static_cast<char>(c);
        }
        break;
    }
  }
  return out.str();
}

[[nodiscard]] std::string json_quote(std::string_view in) {
  return "\"" + json_escape(in) + "\"";
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

[[nodiscard]] bool parse_bool_token(std::string_view raw, bool* out) {
  if (!out) return false;
  const std::string lowered = lowercase_copy(trim_ascii(raw));
  if (lowered == "true" || lowered == "1" || lowered == "yes" ||
      lowered == "y" || lowered == "on") {
    *out = true;
    return true;
  }
  if (lowered == "false" || lowered == "0" || lowered == "no" ||
      lowered == "n" || lowered == "off") {
    *out = false;
    return true;
  }
  return false;
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

[[nodiscard]] std::string make_tool_result_json(std::string_view text,
                                                std::string_view structured_json,
                                                bool is_error) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":" << json_quote(text)
      << "}],\"structuredContent\":" << structured_json
      << ",\"isError\":" << (is_error ? "true" : "false") << "}";
  return out.str();
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
  if (pos >= json.size()) return false;
  const char c = json[pos];
  if (c == '"') {
    std::size_t end = 0;
    if (!parse_json_string_at(json, pos, nullptr, &end)) return false;
    if (out_end) *out_end = end;
    return true;
  }
  if (c == '{') {
    int depth = 0;
    bool in_string = false;
    bool escape = false;
    for (std::size_t i = pos; i < json.size(); ++i) {
      const char ch = json[i];
      if (in_string) {
        if (escape) {
          escape = false;
        } else if (ch == '\\') {
          escape = true;
        } else if (ch == '"') {
          in_string = false;
        }
        continue;
      }
      if (ch == '"') {
        in_string = true;
        continue;
      }
      if (ch == '{') ++depth;
      if (ch == '}') {
        --depth;
        if (depth == 0) {
          if (out_end) *out_end = i + 1;
          return true;
        }
      }
    }
    return false;
  }
  std::size_t end = pos;
  while (end < json.size() && json[end] != ',' && json[end] != '}' &&
         json[end] != ']') {
    ++end;
  }
  if (out_end) *out_end = end;
  return true;
}

[[nodiscard]] bool extract_json_raw_field(const std::string& json,
                                          const std::string& key,
                                          std::string* out) {
  if (out) out->clear();
  std::size_t pos = skip_json_whitespace(json, 0);
  if (pos >= json.size() || json[pos] != '{') return false;
  ++pos;
  for (;;) {
    pos = skip_json_whitespace(json, pos);
    if (pos >= json.size()) return false;
    if (json[pos] == '}') return false;

    std::string current_key;
    std::size_t after_key = pos;
    if (!parse_json_string_at(json, pos, &current_key, &after_key)) return false;
    pos = skip_json_whitespace(json, after_key);
    if (pos >= json.size() || json[pos] != ':') return false;
    ++pos;
    pos = skip_json_whitespace(json, pos);
    if (pos >= json.size()) return false;

    std::size_t value_end = pos;
    if (!find_json_value_end(json, pos, &value_end)) return false;
    if (current_key == key) {
      if (out) *out = json.substr(pos, value_end - pos);
      return true;
    }

    pos = skip_json_whitespace(json, value_end);
    if (pos >= json.size()) return false;
    if (json[pos] == ',') {
      ++pos;
      continue;
    }
    if (json[pos] == '}') return false;
    return false;
  }
}

[[nodiscard]] bool extract_json_object_field(const std::string& json,
                                             const std::string& key,
                                             std::string* out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  raw = trim_ascii(raw);
  if (raw.empty() || raw.front() != '{') return false;
  if (out) *out = raw;
  return true;
}

[[nodiscard]] bool extract_json_string_field(const std::string& json,
                                             const std::string& key,
                                             std::string* out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  std::size_t end = 0;
  if (!parse_json_string_at(raw, 0, out, &end)) return false;
  end = skip_json_whitespace(raw, end);
  return end == raw.size();
}

[[nodiscard]] bool extract_json_bool_field(const std::string& json,
                                           const std::string& key, bool* out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  const std::string lowered = lowercase_copy(trim_ascii(raw));
  if (lowered == "true") {
    if (out) *out = true;
    return true;
  }
  if (lowered == "false") {
    if (out) *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] bool extract_json_size_field(const std::string& json,
                                           const std::string& key,
                                           std::size_t* out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  return parse_size_token(raw, out);
}

[[nodiscard]] bool extract_json_id_field(const std::string& json,
                                         std::string* out_id_json) {
  std::string raw_id;
  if (!extract_json_raw_field(json, "id", &raw_id)) {
    if (out_id_json) *out_id_json = "null";
    return true;
  }

  raw_id = trim_ascii(raw_id);
  if (raw_id.empty()) return false;
  if (raw_id.front() == '"') {
    std::string parsed;
    std::size_t end = 0;
    if (!parse_json_string_at(raw_id, 0, &parsed, &end)) return false;
    end = skip_json_whitespace(raw_id, end);
    if (end != raw_id.size()) return false;
    if (out_id_json) *out_id_json = json_quote(parsed);
    return true;
  }
  if (out_id_json) *out_id_json = raw_id;
  return true;
}

[[nodiscard]] bool starts_with_case_insensitive(std::string_view value,
                                                std::string_view prefix) {
  if (value.size() < prefix.size()) return false;
  for (std::size_t i = 0; i < prefix.size(); ++i) {
    const char a = static_cast<char>(
        std::tolower(static_cast<unsigned char>(value[i])));
    const char b = static_cast<char>(
        std::tolower(static_cast<unsigned char>(prefix[i])));
    if (a != b) return false;
  }
  return true;
}

[[nodiscard]] bool parse_content_length_header(std::string_view header_line,
                                               std::size_t* out_length) {
  constexpr std::string_view kPrefix = "content-length:";
  const std::string line = trim_ascii(header_line);
  if (!starts_with_case_insensitive(line, kPrefix)) return false;
  const std::string number = trim_ascii(line.substr(kPrefix.size()));
  return parse_size_token(number, out_length);
}

[[nodiscard]] bool read_next_jsonrpc_message(std::istream* in,
                                             std::string* out_payload,
                                             bool* out_used_content_length) {
  if (out_used_content_length) *out_used_content_length = false;

  std::string first_line;
  while (std::getline(*in, first_line)) {
    const std::string trimmed = trim_ascii(first_line);
    if (trimmed.empty()) continue;

    std::size_t content_length = 0;
    bool saw_header_block = false;
    if (parse_content_length_header(trimmed, &content_length)) {
      saw_header_block = true;
    } else if (trimmed.front() != '{' && trimmed.front() != '[' &&
               trimmed.find(':') != std::string::npos) {
      saw_header_block = true;
    }

    if (saw_header_block) {
      std::string header_line;
      while (std::getline(*in, header_line)) {
        const std::string header_trimmed = trim_ascii(header_line);
        if (header_trimmed.empty()) break;
        std::size_t parsed_length = 0;
        if (parse_content_length_header(header_trimmed, &parsed_length)) {
          content_length = parsed_length;
        }
      }
      if (content_length == 0) continue;
      if (content_length > kMaxJsonRpcPayloadBytes) return false;

      std::string payload(content_length, '\0');
      in->read(payload.data(), static_cast<std::streamsize>(content_length));
      if (in->gcount() != static_cast<std::streamsize>(content_length)) {
        return false;
      }
      if (out_payload) *out_payload = std::move(payload);
      if (out_used_content_length) *out_used_content_length = true;
      return true;
    }

    if (out_payload) *out_payload = trimmed;
    return true;
  }
  return false;
}

void emit_jsonrpc_payload(std::string_view payload_json) {
  if (g_jsonrpc_use_content_length_framing) {
    std::cout << "Content-Length: " << payload_json.size() << "\r\n\r\n"
              << payload_json;
  } else {
    std::cout << payload_json << "\n";
  }
  std::cout.flush();
}

void write_jsonrpc_result(std::string_view id_json, std::string_view result_json) {
  emit_jsonrpc_payload("{\"jsonrpc\":\"2.0\",\"id\":" + std::string(id_json) +
                       ",\"result\":" + std::string(result_json) + "}");
}

void write_jsonrpc_error(std::string_view id_json, int code,
                         std::string_view message) {
  emit_jsonrpc_payload(
      "{\"jsonrpc\":\"2.0\",\"id\":" + std::string(id_json) +
      ",\"error\":{\"code\":" + std::to_string(code) +
      ",\"message\":" + json_quote(message) + "}}");
}

[[nodiscard]] std::string tool_result_json_for_error(std::string_view message) {
  return make_tool_result_json(
      message,
      "{\"job_cursor\":\"\",\"error\":" + json_quote(message) + "}", true);
}

[[nodiscard]] std::string runtime_job_to_json(
    const cuwacunu::hero::runtime::runtime_job_record_t& record,
    const cuwacunu::hero::runtime::runtime_job_observation_t& observation) {
  const std::string observed_state =
      cuwacunu::hero::runtime::stable_state_for_observation(record, observation);
  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(record.schema) << ","
      << "\"job_cursor\":" << json_quote(record.job_cursor) << ","
      << "\"job_kind\":" << json_quote(record.job_kind) << ","
      << "\"boot_id\":" << json_quote(record.boot_id) << ","
      << "\"state\":" << json_quote(record.state) << ","
      << "\"observed_state\":" << json_quote(observed_state) << ","
      << "\"state_detail\":" << json_quote(record.state_detail) << ","
      << "\"worker_binary\":" << json_quote(record.worker_binary) << ","
      << "\"worker_command\":" << json_quote(record.worker_command) << ","
      << "\"config_folder\":" << json_quote(record.config_folder) << ","
      << "\"source_board_dsl_path\":"
      << json_quote(record.source_board_dsl_path) << ","
      << "\"source_contract_dsl_path\":"
      << json_quote(record.source_contract_dsl_path) << ","
      << "\"source_wave_dsl_path\":"
      << json_quote(record.source_wave_dsl_path) << ","
      << "\"board_dsl_path\":" << json_quote(record.board_dsl_path) << ","
      << "\"contract_dsl_path\":" << json_quote(record.contract_dsl_path) << ","
      << "\"wave_dsl_path\":" << json_quote(record.wave_dsl_path) << ","
      << "\"binding_id\":" << json_quote(record.binding_id) << ","
      << "\"reset_runtime_state\":"
      << (record.reset_runtime_state ? "true" : "false") << ","
      << "\"stdout_path\":" << json_quote(record.stdout_path) << ","
      << "\"stderr_path\":" << json_quote(record.stderr_path) << ","
      << "\"started_at_ms\":" << record.started_at_ms << ","
      << "\"updated_at_ms\":" << record.updated_at_ms << ","
      << "\"finished_at_ms\":"
      << (record.finished_at_ms.has_value()
              ? std::to_string(*record.finished_at_ms)
              : "null")
      << ",\"runner_pid\":"
      << (record.runner_pid.has_value() ? std::to_string(*record.runner_pid)
                                        : "null")
      << ",\"runner_start_ticks\":"
      << (record.runner_start_ticks.has_value()
              ? std::to_string(*record.runner_start_ticks)
              : "null")
      << ",\"target_pid\":"
      << (record.target_pid.has_value() ? std::to_string(*record.target_pid)
                                        : "null")
      << ",\"target_pgid\":"
      << (record.target_pgid.has_value() ? std::to_string(*record.target_pgid)
                                         : "null")
      << ",\"target_start_ticks\":"
      << (record.target_start_ticks.has_value()
              ? std::to_string(*record.target_start_ticks)
              : "null")
      << ",\"exit_code\":"
      << (record.exit_code.has_value() ? std::to_string(*record.exit_code)
                                       : "null")
      << ",\"term_signal\":"
      << (record.term_signal.has_value() ? std::to_string(*record.term_signal)
                                         : "null")
      << ",\"runner_alive\":"
      << (observation.runner_alive ? "true" : "false")
      << ",\"target_alive\":"
      << (observation.target_alive ? "true" : "false")
      << "}";
  return out.str();
}

[[nodiscard]] bool read_runtime_job(
    const app_context_t& app, std::string_view job_cursor,
    cuwacunu::hero::runtime::runtime_job_record_t* out, std::string* error) {
  return cuwacunu::hero::runtime::read_runtime_job_record(
      app.defaults.jobs_root, job_cursor, out, error);
}

[[nodiscard]] bool list_runtime_jobs(
    const app_context_t& app,
    std::vector<cuwacunu::hero::runtime::runtime_job_record_t>* out,
    std::string* error) {
  return cuwacunu::hero::runtime::scan_runtime_job_records(app.defaults.jobs_root,
                                                           out, error);
}

[[nodiscard]] bool write_runtime_job(
    const app_context_t& app,
    const cuwacunu::hero::runtime::runtime_job_record_t& record,
    std::string* error) {
  return cuwacunu::hero::runtime::write_runtime_job_record(
      app.defaults.jobs_root, record, error);
}

[[nodiscard]] bool reconcile_job_record(
    const app_context_t& app,
    cuwacunu::hero::runtime::runtime_job_record_t* record, bool* out_changed,
    std::string* error) {
  if (error) error->clear();
  if (out_changed) *out_changed = false;
  if (!record) {
    if (error) *error = "job record pointer is null";
    return false;
  }

  const auto observation = cuwacunu::hero::runtime::observe_runtime_job(*record);
  const std::string stable_state =
      cuwacunu::hero::runtime::stable_state_for_observation(*record, observation);
  if (stable_state == record->state) return true;

  if (stable_state == "running") {
    record->state = "running";
    record->state_detail = "target process alive";
  } else if (stable_state == "orphaned") {
    record->state = "orphaned";
    record->state_detail = "target process alive but runner is missing";
  } else if (stable_state == "lost") {
    record->state = "lost";
    record->state_detail = "no live runner or target process";
    if (!record->finished_at_ms.has_value()) {
      record->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    }
  } else {
    record->state = stable_state;
  }
  record->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!write_runtime_job(app, *record, error)) return false;
  if (out_changed) *out_changed = true;
  return true;
}

using runtime_tool_handler_t = bool (*)(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error);

struct runtime_tool_descriptor_t {
  std::string_view name;
  std::string_view description;
  std::string_view input_schema_json;
  runtime_tool_handler_t handler;
};

[[nodiscard]] bool handle_tool_launch_default_board(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error);
[[nodiscard]] bool handle_tool_list_jobs(app_context_t* app,
                                         const std::string& arguments_json,
                                         std::string* out_structured,
                                         std::string* out_error);
[[nodiscard]] bool handle_tool_get_job(app_context_t* app,
                                       const std::string& arguments_json,
                                       std::string* out_structured,
                                       std::string* out_error);
[[nodiscard]] bool handle_tool_stop_job(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error);
[[nodiscard]] bool handle_tool_tail_log(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error);
[[nodiscard]] bool handle_tool_reconcile(app_context_t* app,
                                         const std::string& arguments_json,
                                         std::string* out_structured,
                                         std::string* out_error);

constexpr runtime_tool_descriptor_t kRuntimeTools[] = {
#define HERO_RUNTIME_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER) \
  runtime_tool_descriptor_t{NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
#include "hero/runtime_hero/hero_runtime_tools.def"
#undef HERO_RUNTIME_TOOL
};

[[nodiscard]] const runtime_tool_descriptor_t* find_runtime_tool_descriptor(
    std::string_view name) {
  for (const auto& descriptor : kRuntimeTools) {
    if (descriptor.name == name) return &descriptor;
  }
  return nullptr;
}

[[nodiscard]] std::string initialize_result_json() {
  std::ostringstream out;
  out << "{"
      << "\"protocolVersion\":" << json_quote(kProtocolVersion) << ","
      << "\"serverInfo\":{\"name\":" << json_quote(kServerName)
      << ",\"version\":" << json_quote(kServerVersion) << "},"
      << "\"capabilities\":{\"tools\":{\"listChanged\":false}},"
      << "\"instructions\":" << json_quote(kInitializeInstructions)
      << "}";
  return out.str();
}

void write_child_errno_noexcept(int fd, int child_errno) {
  const char* ptr = reinterpret_cast<const char*>(&child_errno);
  std::size_t remaining = sizeof(child_errno);
  while (remaining > 0) {
    const ssize_t written = ::write(fd, ptr, remaining);
    if (written > 0) {
      ptr += static_cast<std::size_t>(written);
      remaining -= static_cast<std::size_t>(written);
      continue;
    }
    if (written < 0 && errno == EINTR) continue;
    break;
  }
}

[[nodiscard]] bool prepare_job_board_dsl_snapshot(
    const std::filesystem::path& jobs_root, std::string_view job_cursor,
    const std::filesystem::path& config_folder, std::string_view binding_id,
    std::string* out_resolved_binding_id, std::string* out_source_board_path,
    std::string* out_job_board_path,
    std::string* out_source_contract_path, std::string* out_source_wave_path,
    std::string* out_job_contract_path, std::string* out_job_wave_path,
    std::string* error) {
  if (error) error->clear();
  if (out_resolved_binding_id) out_resolved_binding_id->clear();
  if (out_source_board_path) out_source_board_path->clear();
  if (out_job_board_path) out_job_board_path->clear();
  if (out_source_contract_path) out_source_contract_path->clear();
  if (out_source_wave_path) out_source_wave_path->clear();
  if (out_job_contract_path) out_job_contract_path->clear();
  if (out_job_wave_path) out_job_wave_path->clear();

  const std::filesystem::path config_path = config_folder / kDefaultConfigFilename;
  const std::optional<std::string> configured =
      read_ini_value(config_path, "GENERAL", kDefaultBoardDslKey);
  if (!configured.has_value()) {
    if (error) {
      *error = "missing GENERAL." + std::string(kDefaultBoardDslKey) +
               " in " + config_path.string();
    }
    return false;
  }

  const std::filesystem::path source_board_path(resolve_path_from_base_folder(
      config_folder.string(), *configured));
  std::error_code ec{};
  if (!std::filesystem::exists(source_board_path, ec) ||
      !std::filesystem::is_regular_file(source_board_path, ec)) {
    if (error) {
      *error = "configured default board DSL does not exist: " +
               source_board_path.string();
    }
    return false;
  }

  cuwacunu::hero::wave_contract_binding_runtime::
      resolved_wave_contract_binding_snapshot_t snapshot{};
  if (!cuwacunu::hero::wave_contract_binding_runtime::
          prepare_board_binding_snapshot(
              source_board_path, binding_id,
              cuwacunu::hero::runtime::runtime_job_dir(jobs_root, job_cursor),
              &snapshot, error)) {
    return false;
  }
  if (out_source_board_path) {
    *out_source_board_path = snapshot.source_scope_dsl_path;
  }
  if (out_resolved_binding_id) *out_resolved_binding_id = snapshot.binding_id;
  if (out_job_board_path) *out_job_board_path = snapshot.board_dsl_path;
  if (out_source_contract_path) {
    *out_source_contract_path = snapshot.source_contract_dsl_path;
  }
  if (out_source_wave_path) {
    *out_source_wave_path = snapshot.source_wave_dsl_path;
  }
  if (out_job_contract_path) *out_job_contract_path = snapshot.contract_dsl_path;
  if (out_job_wave_path) *out_job_wave_path = snapshot.wave_dsl_path;
  return true;
}

[[nodiscard]] bool launch_runner_process(
    const app_context_t& app,
    const cuwacunu::hero::runtime::runtime_job_record_t& seed_record,
    std::string* error) {
  if (error) error->clear();

  int pipe_fds[2]{-1, -1};
#ifdef O_CLOEXEC
  if (::pipe2(pipe_fds, O_CLOEXEC) != 0) {
    if (error) *error = "pipe2 failed";
    return false;
  }
#else
  if (::pipe(pipe_fds) != 0) {
    if (error) *error = "pipe failed";
    return false;
  }
  (void)::fcntl(pipe_fds[0], F_SETFD, FD_CLOEXEC);
  (void)::fcntl(pipe_fds[1], F_SETFD, FD_CLOEXEC);
#endif

  const pid_t child = ::fork();
  if (child < 0) {
    const std::string msg = "fork failed";
    (void)::close(pipe_fds[0]);
    (void)::close(pipe_fds[1]);
    if (error) *error = msg;
    return false;
  }
  if (child == 0) {
    (void)::close(pipe_fds[0]);
    if (::setsid() < 0) {
      const int child_errno = errno;
      write_child_errno_noexcept(pipe_fds[1], child_errno);
      _exit(127);
    }

    const int devnull = ::open("/dev/null", O_RDWR);
    if (devnull >= 0) {
      (void)::dup2(devnull, STDIN_FILENO);
      (void)::dup2(devnull, STDOUT_FILENO);
      (void)::dup2(devnull, STDERR_FILENO);
      if (devnull > STDERR_FILENO) (void)::close(devnull);
    }

    std::vector<std::string> args{};
    args.push_back(app.self_binary_path.string());
    args.push_back("--job-runner");
    args.push_back("--jobs-root");
    args.push_back(app.defaults.jobs_root.string());
    args.push_back("--job-cursor");
    args.push_back(seed_record.job_cursor);
    args.push_back("--worker-binary");
    args.push_back(seed_record.worker_binary);
    args.push_back("--job-kind");
    args.push_back(seed_record.job_kind);
    args.push_back("--config-folder");
    args.push_back(seed_record.config_folder);
    if (!seed_record.board_dsl_path.empty()) {
      args.push_back("--board-dsl");
      args.push_back(seed_record.board_dsl_path);
    }
    if (!seed_record.binding_id.empty()) {
      args.push_back("--binding");
      args.push_back(seed_record.binding_id);
    }
    if (seed_record.reset_runtime_state) {
      args.push_back("--reset-runtime-state");
    }

    std::vector<char*> argv{};
    argv.reserve(args.size() + 1);
    for (auto& arg : args) argv.push_back(arg.data());
    argv.push_back(nullptr);
    ::execv(argv[0], argv.data());

    const int child_errno = errno;
    write_child_errno_noexcept(pipe_fds[1], child_errno);
    _exit(127);
  }

  (void)::close(pipe_fds[1]);
  int exec_errno = 0;
  const ssize_t n = ::read(pipe_fds[0], &exec_errno, sizeof(exec_errno));
  (void)::close(pipe_fds[0]);
  if (n > 0) {
    if (error) {
      *error = "cannot exec detached runtime job runner: errno=" +
               std::to_string(exec_errno);
    }
    return false;
  }

  cuwacunu::hero::runtime::runtime_job_record_t record = seed_record;
  record.runner_pid = static_cast<std::uint64_t>(child);
  std::uint64_t runner_start_ticks = 0;
  if (!cuwacunu::hero::runtime::read_process_start_ticks(record.runner_pid.value(),
                                                         &runner_start_ticks)) {
    (void)::kill(child, SIGKILL);
    int ignored_status = 0;
    while (::waitpid(child, &ignored_status, 0) < 0 && errno == EINTR) {
    }
    if (error) {
      *error = "cannot determine detached runtime job runner identity";
    }
    return false;
  }
  record.runner_start_ticks = runner_start_ticks;
  record.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!write_runtime_job(app, record, error)) return false;
  return true;
}

[[nodiscard]] bool handle_tool_launch_default_board(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string binding{};
  std::string config_folder = app->defaults.config_folder.string();
  bool reset_runtime_state = false;
  (void)extract_json_string_field(arguments_json, "binding", &binding);
  (void)extract_json_string_field(arguments_json, "config_folder", &config_folder);
  (void)extract_json_bool_field(arguments_json, "reset_runtime_state",
                                &reset_runtime_state);
  config_folder = trim_ascii(config_folder);
  if (config_folder.empty()) {
    config_folder = app->defaults.config_folder.string();
  }

  std::vector<cuwacunu::hero::runtime::runtime_job_record_t> jobs{};
  if (!list_runtime_jobs(*app, &jobs, out_error)) return false;
  std::size_t active_jobs = 0;
  for (auto& job : jobs) {
    bool changed = false;
    if (!reconcile_job_record(*app, &job, &changed, out_error)) return false;
    const auto observation = cuwacunu::hero::runtime::observe_runtime_job(job);
    const std::string stable =
        cuwacunu::hero::runtime::stable_state_for_observation(job, observation);
    if (stable == "launching" || stable == "running" || stable == "stopping" ||
        stable == "orphaned") {
      ++active_jobs;
    }
  }
  if (app->defaults.max_active_jobs > 0 &&
      active_jobs >= app->defaults.max_active_jobs) {
    *out_error = "max_active_jobs reached";
    return false;
  }

  const std::string job_cursor =
      cuwacunu::hero::runtime::make_job_cursor(app->defaults.jobs_root);
  std::string resolved_binding_id{};
  std::string source_board_dsl_path{};
  std::string source_contract_dsl_path{};
  std::string source_wave_dsl_path{};
  std::string job_board_dsl_path{};
  std::string job_contract_dsl_path{};
  std::string job_wave_dsl_path{};
  if (!prepare_job_board_dsl_snapshot(app->defaults.jobs_root, job_cursor,
                                      std::filesystem::path(config_folder), binding,
                                      &resolved_binding_id,
                                      &source_board_dsl_path,
                                      &job_board_dsl_path,
                                      &source_contract_dsl_path,
                                      &source_wave_dsl_path,
                                      &job_contract_dsl_path,
                                      &job_wave_dsl_path, out_error)) {
    return false;
  }
  const std::vector<std::string> worker_argv = [&]() {
    std::vector<std::string> args{};
    args.push_back(app->defaults.main_board_binary.string());
    args.push_back("--config-folder");
    args.push_back(config_folder);
    if (!job_board_dsl_path.empty()) {
      args.push_back("--board-dsl");
      args.push_back(job_board_dsl_path);
    }
    if (!resolved_binding_id.empty()) {
      args.push_back("--binding");
      args.push_back(resolved_binding_id);
    }
    if (reset_runtime_state) {
      args.push_back("--reset-runtime-state");
    }
    return args;
  }();

  cuwacunu::hero::runtime::runtime_job_record_t record{};
  record.job_cursor = job_cursor;
  record.job_kind = "default_board";
  record.boot_id = cuwacunu::hero::runtime::current_boot_id();
  record.state = "launching";
  record.state_detail = "detached runner requested";
  record.worker_binary = app->defaults.main_board_binary.string();
  record.worker_command =
      cuwacunu::hero::runtime::shell_join_command(worker_argv);
  record.config_folder = config_folder;
  record.source_board_dsl_path = source_board_dsl_path;
  record.source_contract_dsl_path = source_contract_dsl_path;
  record.source_wave_dsl_path = source_wave_dsl_path;
  record.board_dsl_path = job_board_dsl_path;
  record.contract_dsl_path = job_contract_dsl_path;
  record.wave_dsl_path = job_wave_dsl_path;
  record.binding_id = resolved_binding_id;
  record.reset_runtime_state = reset_runtime_state;
  record.stdout_path =
      cuwacunu::hero::runtime::runtime_job_stdout_path(app->defaults.jobs_root,
                                                       job_cursor)
          .string();
  record.stderr_path =
      cuwacunu::hero::runtime::runtime_job_stderr_path(app->defaults.jobs_root,
                                                       job_cursor)
          .string();
  record.started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  record.updated_at_ms = record.started_at_ms;

  if (!write_runtime_job(*app, record, out_error)) return false;
  if (!launch_runner_process(*app, record, out_error)) {
    record.state = "failed_to_start";
    record.state_detail = *out_error;
    record.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    record.updated_at_ms = *record.finished_at_ms;
    record.exit_code = 127;
    std::string ignored{};
    (void)write_runtime_job(*app, record, &ignored);
    return false;
  }

  for (int attempt = 0; attempt < 80; ++attempt) {
    std::string poll_error{};
    if (read_runtime_job(*app, job_cursor, &record, &poll_error)) {
      if (record.target_pgid.has_value() || record.target_pid.has_value() ||
          record.state != "launching") {
        break;
      }
    }
    ::usleep(25 * 1000);
  }
  if (!read_runtime_job(*app, job_cursor, &record, out_error)) return false;
  const auto observation = cuwacunu::hero::runtime::observe_runtime_job(record);
  *out_structured =
      "{\"job_cursor\":" + json_quote(job_cursor) + ",\"job\":" +
      runtime_job_to_json(record, observation) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_list_jobs(app_context_t* app,
                                         const std::string& arguments_json,
                                         std::string* out_structured,
                                         std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string state_filter{};
  std::size_t limit = 0;
  std::size_t offset = 0;
  bool newest_first = true;
  (void)extract_json_string_field(arguments_json, "state", &state_filter);
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);
  (void)extract_json_bool_field(arguments_json, "newest_first", &newest_first);
  state_filter = trim_ascii(state_filter);

  std::vector<cuwacunu::hero::runtime::runtime_job_record_t> jobs{};
  if (!list_runtime_jobs(*app, &jobs, out_error)) return false;

  struct row_t {
    cuwacunu::hero::runtime::runtime_job_record_t record{};
    cuwacunu::hero::runtime::runtime_job_observation_t observation{};
    std::string stable_state{};
  };
  std::vector<row_t> rows{};
  rows.reserve(jobs.size());
  for (auto& job : jobs) {
    bool changed = false;
    if (!reconcile_job_record(*app, &job, &changed, out_error)) return false;
    row_t row{};
    row.record = job;
    row.observation = cuwacunu::hero::runtime::observe_runtime_job(job);
    row.stable_state =
        cuwacunu::hero::runtime::stable_state_for_observation(job,
                                                              row.observation);
    if (!state_filter.empty() && row.stable_state != state_filter &&
        row.record.state != state_filter) {
      continue;
    }
    rows.push_back(std::move(row));
  }

  std::sort(rows.begin(), rows.end(), [newest_first](const row_t& a,
                                                     const row_t& b) {
    if (a.record.started_at_ms != b.record.started_at_ms) {
      return newest_first ? (a.record.started_at_ms > b.record.started_at_ms)
                          : (a.record.started_at_ms < b.record.started_at_ms);
    }
    return newest_first ? (a.record.job_cursor > b.record.job_cursor)
                        : (a.record.job_cursor < b.record.job_cursor);
  });

  const std::size_t total = rows.size();
  const std::size_t off = std::min(offset, rows.size());
  std::size_t count = limit;
  if (count == 0) count = rows.size() - off;
  count = std::min(count, rows.size() - off);
  rows.assign(rows.begin() + static_cast<std::ptrdiff_t>(off),
              rows.begin() + static_cast<std::ptrdiff_t>(off + count));

  std::ostringstream jobs_json;
  jobs_json << "[";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i != 0) jobs_json << ",";
    jobs_json << runtime_job_to_json(rows[i].record, rows[i].observation);
  }
  jobs_json << "]";

  std::ostringstream out;
  out << "{\"job_cursor\":\"\""
      << ",\"count\":" << rows.size()
      << ",\"total\":" << total
      << ",\"state\":" << json_quote(state_filter)
      << ",\"jobs\":" << jobs_json.str() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_job(app_context_t* app,
                                       const std::string& arguments_json,
                                       std::string* out_structured,
                                       std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string job_cursor{};
  (void)extract_json_string_field(arguments_json, "job_cursor", &job_cursor);
  job_cursor = trim_ascii(job_cursor);
  if (job_cursor.empty()) {
    *out_error = "get_job requires arguments.job_cursor";
    return false;
  }

  cuwacunu::hero::runtime::runtime_job_record_t record{};
  if (!read_runtime_job(*app, job_cursor, &record, out_error)) return false;
  bool changed = false;
  if (!reconcile_job_record(*app, &record, &changed, out_error)) return false;
  const auto observation = cuwacunu::hero::runtime::observe_runtime_job(record);
  *out_structured =
      "{\"job_cursor\":" + json_quote(job_cursor) + ",\"job\":" +
      runtime_job_to_json(record, observation) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_stop_job(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string job_cursor{};
  bool force = false;
  (void)extract_json_string_field(arguments_json, "job_cursor", &job_cursor);
  (void)extract_json_bool_field(arguments_json, "force", &force);
  job_cursor = trim_ascii(job_cursor);
  if (job_cursor.empty()) {
    *out_error = "stop_job requires arguments.job_cursor";
    return false;
  }

  cuwacunu::hero::runtime::runtime_job_record_t record{};
  if (!read_runtime_job(*app, job_cursor, &record, out_error)) return false;

  int rc = -1;
  const int sig = force ? SIGKILL : SIGTERM;
  bool use_process_group = false;
  bool used_runner_fallback = false;
  if (record.target_pgid.has_value() &&
      cuwacunu::hero::runtime::process_group_alive(
          *record.target_pgid, record.target_start_ticks, record.boot_id)) {
    use_process_group = true;
    rc = ::kill(-static_cast<pid_t>(*record.target_pgid), sig);
  } else if (record.target_pid.has_value() &&
             cuwacunu::hero::runtime::process_identity_alive(
                 *record.target_pid, record.target_start_ticks,
                 record.boot_id)) {
    rc = ::kill(static_cast<pid_t>(*record.target_pid), sig);
  } else if (record.runner_pid.has_value() &&
             cuwacunu::hero::runtime::process_identity_alive(
                 *record.runner_pid, record.runner_start_ticks,
                 record.boot_id) &&
             (!record.target_pid.has_value() || !record.target_pgid.has_value() ||
              record.state == "launching")) {
    used_runner_fallback = true;
    rc = ::kill(static_cast<pid_t>(*record.runner_pid), sig);
  } else {
    *out_error = "job has no live process identity metadata";
    return false;
  }
  if (rc != 0 && errno != ESRCH) {
    *out_error = "failed to signal job process";
    return false;
  }

  record.state = "stopping";
  if (use_process_group) {
    record.state_detail = force ? "sent SIGKILL to target process group"
                                : "sent SIGTERM to target process group";
  } else if (used_runner_fallback) {
    record.state_detail =
        force ? "sent SIGKILL to job runner before target publication"
              : "sent SIGTERM to job runner before target publication";
  } else {
    record.state_detail =
        force ? "sent SIGKILL to target process"
              : "sent SIGTERM to target process";
  }
  record.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!write_runtime_job(*app, record, out_error)) return false;

  const auto observation = cuwacunu::hero::runtime::observe_runtime_job(record);
  *out_structured =
      "{\"job_cursor\":" + json_quote(job_cursor) + ",\"job\":" +
      runtime_job_to_json(record, observation) + "}";
  return true;
}

[[nodiscard]] bool tail_file_lines(const std::filesystem::path& path,
                                   std::size_t lines, std::string* out,
                                   std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "tail output pointer is null";
    return false;
  }
  *out = "";
  std::error_code ec{};
  if (!std::filesystem::exists(path, ec)) {
    if (ec) {
      if (error) *error = "cannot access log file: " + path.string();
      return false;
    }
    return true;
  }
  std::string text{};
  if (!cuwacunu::hero::runtime::read_text_file(path, &text, error)) return false;
  if (lines == 0) {
    *out = text;
    return true;
  }

  std::size_t count = 0;
  std::size_t pos = text.size();
  while (pos > 0) {
    --pos;
    if (text[pos] == '\n') {
      ++count;
      if (count > lines) {
        ++pos;
        break;
      }
    }
  }
  if (count <= lines) pos = 0;
  *out = text.substr(pos);
  return true;
}

[[nodiscard]] bool handle_tool_tail_log(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string job_cursor{};
  std::string stream{"stdout"};
  std::size_t lines = app->defaults.tail_default_lines;
  (void)extract_json_string_field(arguments_json, "job_cursor", &job_cursor);
  (void)extract_json_string_field(arguments_json, "stream", &stream);
  (void)extract_json_size_field(arguments_json, "lines", &lines);
  job_cursor = trim_ascii(job_cursor);
  stream = lowercase_copy(trim_ascii(stream));
  if (job_cursor.empty()) {
    *out_error = "tail_log requires arguments.job_cursor";
    return false;
  }
  if (stream.empty()) stream = "stdout";
  if (stream != "stdout" && stream != "stderr") {
    *out_error = "tail_log stream must be stdout or stderr";
    return false;
  }

  cuwacunu::hero::runtime::runtime_job_record_t record{};
  if (!read_runtime_job(*app, job_cursor, &record, out_error)) return false;

  const std::filesystem::path log_path =
      (stream == "stderr") ? std::filesystem::path(record.stderr_path)
                           : std::filesystem::path(record.stdout_path);
  std::string text{};
  if (!tail_file_lines(log_path, lines, &text, out_error)) return false;

  std::ostringstream out;
  out << "{\"job_cursor\":" << json_quote(job_cursor)
      << ",\"stream\":" << json_quote(stream)
      << ",\"lines\":" << lines
      << ",\"path\":" << json_quote(log_path.string())
      << ",\"text\":" << json_quote(text) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_reconcile(app_context_t* app,
                                         const std::string& arguments_json,
                                         std::string* out_structured,
                                         std::string* out_error) {
  (void)arguments_json;
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::vector<cuwacunu::hero::runtime::runtime_job_record_t> jobs{};
  if (!list_runtime_jobs(*app, &jobs, out_error)) return false;

  std::size_t changed = 0;
  std::ostringstream jobs_json;
  jobs_json << "[";
  bool first = true;
  for (auto& job : jobs) {
    bool updated = false;
    if (!reconcile_job_record(*app, &job, &updated, out_error)) return false;
    if (updated) ++changed;
    const auto observation = cuwacunu::hero::runtime::observe_runtime_job(job);
    if (!first) jobs_json << ",";
    first = false;
    jobs_json << runtime_job_to_json(job, observation);
  }
  jobs_json << "]";

  std::ostringstream out;
  out << "{\"job_cursor\":\"\""
      << ",\"count\":" << jobs.size()
      << ",\"changed\":" << changed
      << ",\"jobs\":" << jobs_json.str() << "}";
  *out_structured = out.str();
  return true;
}

}  // namespace

std::filesystem::path resolve_runtime_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "REAL_HERO", "runtime_hero_dsl_filename");
  if (!configured.has_value()) {
    return std::filesystem::path(kDefaultRuntimeHeroDslPath);
  }
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) {
    return std::filesystem::path(kDefaultRuntimeHeroDslPath);
  }
  return std::filesystem::path(resolved);
}

bool load_runtime_defaults(const std::filesystem::path& hero_dsl_path,
                           runtime_defaults_t* out, std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "runtime defaults output pointer is null";
    return false;
  }
  *out = runtime_defaults_t{};

  std::ifstream in(hero_dsl_path);
  if (!in) {
    if (error) *error = "cannot open runtime HERO defaults DSL: " + hero_dsl_path.string();
    return false;
  }

  std::unordered_map<std::string, std::string> values{};
  std::string line;
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

  const auto resolve_local_path = [&](const std::string& value) {
    return std::filesystem::path(resolve_path_from_base_folder(
        hero_dsl_path.parent_path().string(), value));
  };

  if (const auto it = values.find("jobs_root"); it != values.end()) {
    out->jobs_root = resolve_local_path(it->second);
  }
  if (const auto it = values.find("main_board_binary"); it != values.end()) {
    out->main_board_binary = resolve_local_path(it->second);
  }
  if (const auto it = values.find("config_folder"); it != values.end()) {
    out->config_folder = resolve_local_path(it->second);
  }
  if (const auto it = values.find("tail_default_lines"); it != values.end()) {
    if (!parse_size_token(it->second, &out->tail_default_lines)) {
      if (error) *error = "invalid tail_default_lines in " + hero_dsl_path.string();
      return false;
    }
  }
  if (const auto it = values.find("max_active_jobs"); it != values.end()) {
    if (!parse_size_token(it->second, &out->max_active_jobs)) {
      if (error) *error = "invalid max_active_jobs in " + hero_dsl_path.string();
      return false;
    }
  }

  if (out->jobs_root.empty()) {
    out->jobs_root = "/cuwacunu/.runtime/jobs";
  }
  if (out->main_board_binary.empty()) {
    out->main_board_binary = "/cuwacunu/.build/hero/main_board";
  }
  if (out->config_folder.empty()) {
    out->config_folder = "/cuwacunu/src/config";
  }
  if (out->tail_default_lines == 0) out->tail_default_lines = 120;
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
  const std::size_t tool_count =
      sizeof(kRuntimeTools) / sizeof(kRuntimeTools[0]);
  for (std::size_t i = 0; i < tool_count; ++i) {
    const auto& tool = kRuntimeTools[i];
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
  for (const auto& tool : kRuntimeTools) {
    out << tool.name << "\t" << tool.description << "\n";
  }
  return out.str();
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

  const auto* descriptor = find_runtime_tool_descriptor(tool_name);
  if (descriptor == nullptr) {
    *out_error_message = "unknown tool: " + tool_name;
    return false;
  }

  std::string structured;
  std::string err;
  const bool ok =
      descriptor->handler(app, arguments_json, &structured, &err);
  if (!ok) {
    *out_tool_result_json = tool_result_json_for_error(err);
    return true;
  }
  *out_tool_result_json = make_tool_result_json("ok", structured, false);
  return true;
}

void run_jsonrpc_stdio_loop(app_context_t* app) {
  std::string request;
  while (true) {
    bool used_content_length = false;
    if (!read_next_jsonrpc_message(&std::cin, &request, &used_content_length)) {
      return;
    }
    if (used_content_length) g_jsonrpc_use_content_length_framing = true;

    std::string id_json = "null";
    if (!extract_json_id_field(request, &id_json)) {
      write_jsonrpc_error("null", -32700, "invalid request id");
      continue;
    }

    std::string method;
    if (!extract_json_string_field(request, "method", &method)) {
      write_jsonrpc_error(id_json, -32600, "missing method");
      continue;
    }

    if (method.rfind("notifications/", 0) == 0) continue;
    if (method == "initialize") {
      write_jsonrpc_result(id_json, initialize_result_json());
      continue;
    }
    if (method == "tools/list") {
      write_jsonrpc_result(id_json, build_tools_list_result_json());
      continue;
    }
    if (method != "tools/call") {
      write_jsonrpc_error(id_json, -32601, "unsupported method");
      continue;
    }

    std::string params;
    if (!extract_json_object_field(request, "params", &params)) {
      write_jsonrpc_error(id_json, -32602, "tools/call requires params object");
      continue;
    }
    std::string name;
    if (!extract_json_string_field(params, "name", &name) || name.empty()) {
      write_jsonrpc_error(id_json, -32602, "tools/call requires params.name");
      continue;
    }
    std::string arguments = "{}";
    std::string extracted_args;
    if (extract_json_object_field(params, "arguments", &extracted_args)) {
      arguments = extracted_args;
    }

    std::string tool_result;
    std::string tool_error;
    if (!execute_tool_json(name, arguments, app, &tool_result, &tool_error)) {
      write_jsonrpc_error(id_json, -32601, tool_error);
      continue;
    }
    write_jsonrpc_result(id_json, tool_result);
  }
}

}  // namespace runtime_mcp
}  // namespace hero
}  // namespace cuwacunu
