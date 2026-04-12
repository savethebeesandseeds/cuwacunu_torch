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
#include <sys/file.h>
#include <tuple>
#include <unordered_map>
#include <unistd.h>
#include <vector>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"
#include "hero/wave_contract_binding_runtime.h"
#include "hero/runtime_hero/runtime_campaign.h"
#include "hero/runtime_hero/runtime_job.h"
#include "hero/marshal_hero/marshal_session.h"

namespace cuwacunu {
namespace hero {
namespace runtime_mcp {

namespace {

constexpr const char* kDefaultCampaignDslKey = "default_iitepi_campaign_dsl_filename";
constexpr const char* kServerName = "hero_runtime_mcp";
constexpr const char* kServerVersion = "0.1.0";
constexpr const char* kProtocolVersion = "2025-03-26";
constexpr const char* kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.runtime.*.";
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;

bool g_jsonrpc_use_content_length_framing = false;

[[nodiscard]] bool rewrite_campaign_imports_absolute(
    const std::filesystem::path& original_path, std::string_view original_text,
    std::string* out_text, std::string* error);

[[nodiscard]] bool decode_campaign_snapshot(
    const app_context_t& app, const std::string& campaign_dsl_path,
    cuwacunu::camahjucunu::iitepi_campaign_instruction_t* out_instruction,
    std::string* error);

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

[[nodiscard]] bool extract_json_array_field(const std::string& json,
                                            const std::string& key,
                                            std::string* out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  raw = trim_ascii(raw);
  if (raw.empty() || raw.front() != '[') return false;
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

[[nodiscard]] bool split_json_array_items(const std::string& json_array,
                                          std::vector<std::string>* out,
                                          std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "json array output pointer is null";
    return false;
  }
  out->clear();
  const std::string raw = trim_ascii(json_array);
  if (raw.empty() || raw.front() != '[' || raw.back() != ']') {
    if (error) *error = "expected JSON array";
    return false;
  }
  std::size_t pos = skip_json_whitespace(raw, 1);
  while (pos < raw.size() && raw[pos] != ']') {
    std::size_t value_end = pos;
    if (!find_json_value_end(raw, pos, &value_end)) {
      if (error) *error = "cannot parse JSON array item";
      return false;
    }
    out->push_back(trim_ascii(raw.substr(pos, value_end - pos)));
    pos = skip_json_whitespace(raw, value_end);
    if (pos >= raw.size()) {
      if (error) *error = "unterminated JSON array";
      return false;
    }
    if (raw[pos] == ',') {
      pos = skip_json_whitespace(raw, pos + 1);
      continue;
    }
    if (raw[pos] == ']') break;
    if (error) *error = "unexpected token in JSON array";
    return false;
  }
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
      "{\"campaign_cursor\":\"\",\"job_cursor\":\"\",\"error\":" +
          json_quote(message) + "}",
      true);
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
      << "\"global_config_path\":"
      << json_quote(record.global_config_path) << ","
      << "\"source_campaign_dsl_path\":"
      << json_quote(record.source_campaign_dsl_path) << ","
      << "\"source_contract_dsl_path\":"
      << json_quote(record.source_contract_dsl_path) << ","
      << "\"source_wave_dsl_path\":"
      << json_quote(record.source_wave_dsl_path) << ","
      << "\"campaign_dsl_path\":" << json_quote(record.campaign_dsl_path) << ","
      << "\"contract_dsl_path\":" << json_quote(record.contract_dsl_path) << ","
      << "\"wave_dsl_path\":" << json_quote(record.wave_dsl_path) << ","
      << "\"binding_id\":" << json_quote(record.binding_id) << ","
      << "\"requested_device\":"
      << json_quote(record.requested_device) << ","
      << "\"resolved_device\":"
      << json_quote(record.resolved_device) << ","
      << "\"device_source_section\":"
      << json_quote(record.device_source_section) << ","
      << "\"device_contract_hash\":"
      << json_quote(record.device_contract_hash) << ","
      << "\"device_error\":"
      << json_quote(record.device_error) << ","
      << "\"cuda_required\":"
      << (record.cuda_required ? "true" : "false") << ","
      << "\"reset_runtime_state\":"
      << (record.reset_runtime_state ? "true" : "false") << ","
      << "\"stdout_path\":" << json_quote(record.stdout_path) << ","
      << "\"stderr_path\":" << json_quote(record.stderr_path) << ","
      << "\"trace_path\":" << json_quote(record.trace_path) << ","
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
      app.defaults.campaigns_root, job_cursor, out, error);
}

[[nodiscard]] bool list_runtime_jobs(
    const app_context_t& app,
    std::vector<cuwacunu::hero::runtime::runtime_job_record_t>* out,
    std::string* error) {
  return cuwacunu::hero::runtime::scan_runtime_job_records(
      app.defaults.campaigns_root, out, error);
}

[[nodiscard]] bool write_runtime_job(
    const app_context_t& app,
    const cuwacunu::hero::runtime::runtime_job_record_t& record,
    std::string* error) {
  return cuwacunu::hero::runtime::write_runtime_job_record(
      app.defaults.campaigns_root, record, error);
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
  if (stable_state == record->state) {
    if (stable_state == "failed") {
      const std::string expected_detail =
          "worker and target processes are no longer alive";
      const bool needs_finished_at = !record->finished_at_ms.has_value();
      if (record->state_detail != expected_detail || needs_finished_at) {
        record->state_detail = expected_detail;
        if (needs_finished_at) {
          record->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
        }
        record->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
        if (!write_runtime_job(app, *record, error)) return false;
        if (out_changed) *out_changed = true;
      }
    } else if (stable_state == "orphaned") {
      const std::string expected_detail =
          "target process alive but worker process is gone";
      if (record->state_detail != expected_detail) {
        record->state_detail = expected_detail;
        record->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
        if (!write_runtime_job(app, *record, error)) return false;
        if (out_changed) *out_changed = true;
      }
    }
    return true;
  }

  if (stable_state == "orphaned") {
    record->state = "orphaned";
    record->state_detail = "target process alive but worker process is gone";
  } else if (stable_state == "failed") {
    record->state = "failed";
    record->state_detail = "worker and target processes are no longer alive";
    if (!record->finished_at_ms.has_value()) {
      record->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    }
  } else
  if (stable_state == "running") {
    record->state = "running";
    record->state_detail = "target process alive";
  } else {
    record->state = stable_state;
  }
  record->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!write_runtime_job(app, *record, error)) return false;
  if (out_changed) *out_changed = true;
  return true;
}

[[nodiscard]] std::filesystem::path runtime_job_trace_path_for_record(
    const app_context_t& app,
    const cuwacunu::hero::runtime::runtime_job_record_t& record) {
  if (!record.trace_path.empty()) {
    return std::filesystem::path(record.trace_path);
  }
  return cuwacunu::hero::runtime::runtime_job_trace_path(
      app.defaults.campaigns_root, record.job_cursor);
}

[[nodiscard]] std::string runtime_campaign_to_json(
    const cuwacunu::hero::runtime::runtime_campaign_record_t& record) {
  const bool runner_alive =
      record.runner_pid.has_value() &&
      cuwacunu::hero::runtime::process_identity_alive(
          *record.runner_pid, record.runner_start_ticks, record.boot_id);
  std::ostringstream run_bind_ids_json;
  run_bind_ids_json << "[";
  for (std::size_t i = 0; i < record.run_bind_ids.size(); ++i) {
    if (i != 0) run_bind_ids_json << ",";
    run_bind_ids_json << json_quote(record.run_bind_ids[i]);
  }
  run_bind_ids_json << "]";

  std::ostringstream job_cursors_json;
  job_cursors_json << "[";
  for (std::size_t i = 0; i < record.job_cursors.size(); ++i) {
    if (i != 0) job_cursors_json << ",";
    job_cursors_json << json_quote(record.job_cursors[i]);
  }
  job_cursors_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(record.schema) << ","
      << "\"campaign_cursor\":" << json_quote(record.campaign_cursor) << ","
      << "\"boot_id\":" << json_quote(record.boot_id) << ","
      << "\"state\":" << json_quote(record.state) << ","
      << "\"state_detail\":" << json_quote(record.state_detail) << ","
      << "\"marshal_session_id\":" << json_quote(record.marshal_session_id) << ","
      << "\"global_config_path\":"
      << json_quote(record.global_config_path) << ","
      << "\"source_campaign_dsl_path\":"
      << json_quote(record.source_campaign_dsl_path) << ","
      << "\"campaign_dsl_path\":" << json_quote(record.campaign_dsl_path) << ","
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
      << ",\"current_run_index\":"
      << (record.current_run_index.has_value()
              ? std::to_string(*record.current_run_index)
              : "null")
      << ",\"runner_alive\":" << (runner_alive ? "true" : "false")
      << ",\"run_bind_ids\":" << run_bind_ids_json.str()
      << ",\"job_cursors\":" << job_cursors_json.str()
      << "}";
  return out.str();
}

[[nodiscard]] bool read_runtime_campaign(
    const app_context_t& app, std::string_view campaign_cursor,
    cuwacunu::hero::runtime::runtime_campaign_record_t* out,
    std::string* error) {
  return cuwacunu::hero::runtime::read_runtime_campaign_record(
      app.defaults.campaigns_root, campaign_cursor, out, error);
}

[[nodiscard]] bool list_runtime_campaigns(
    const app_context_t& app,
    std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t>* out,
    std::string* error) {
  return cuwacunu::hero::runtime::scan_runtime_campaign_records(
      app.defaults.campaigns_root, out, error);
}

[[nodiscard]] bool write_runtime_campaign(
    const app_context_t& app,
    const cuwacunu::hero::runtime::runtime_campaign_record_t& record,
    std::string* error) {
  return cuwacunu::hero::runtime::write_runtime_campaign_record(
      app.defaults.campaigns_root, record, error);
}

[[nodiscard]] bool read_linked_marshal_session(
    const app_context_t& app, std::string_view marshal_session_id,
    cuwacunu::hero::marshal::marshal_session_record_t* out,
    std::string* error) {
  return cuwacunu::hero::marshal::read_marshal_session_record(
      app.defaults.marshal_root, marshal_session_id, out, error);
}

[[nodiscard]] bool write_linked_marshal_session(
    const app_context_t& app,
    const cuwacunu::hero::marshal::marshal_session_record_t& record,
    std::string* error) {
  return cuwacunu::hero::marshal::write_marshal_session_record(
      app.defaults.marshal_root, record, error);
}

struct campaign_launch_request_t {
  std::string binding_id{};
  bool reset_runtime_state{false};
  std::string campaign_dsl_path{};
  std::string marshal_session_id{};
};

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

[[nodiscard]] bool append_marshal_session_event(
    const cuwacunu::hero::marshal::marshal_session_record_t& loop,
    std::string_view event, std::string_view detail, std::string* error) {
  if (error) error->clear();
  const std::string payload =
      "{\"timestamp_ms\":" +
      std::to_string(cuwacunu::hero::runtime::now_ms_utc()) +
      ",\"marshal_session_id\":" + json_quote(loop.marshal_session_id) + ",\"event\":" +
      json_quote(event) + ",\"detail\":" + json_quote(detail) + "}\n";
  return cuwacunu::hero::runtime::append_text_file(loop.events_path, payload, error);
}

[[nodiscard]] std::filesystem::path resolve_default_campaign_dsl_path(
    const std::filesystem::path& global_config_path, std::string* error) {
  if (error) error->clear();
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "GENERAL", kDefaultCampaignDslKey);
  if (!configured.has_value()) {
    if (error) {
      *error = "missing GENERAL." + std::string(kDefaultCampaignDslKey) + " in " +
               global_config_path.string();
    }
    return {};
  }
  return std::filesystem::path(resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured));
}

[[nodiscard]] std::filesystem::path resolve_requested_campaign_source_path(
    const app_context_t& app, const std::filesystem::path& global_config_path,
    std::string requested_campaign_dsl_path, std::string* error) {
  if (error) error->clear();
  requested_campaign_dsl_path = trim_ascii(std::move(requested_campaign_dsl_path));
  const std::filesystem::path source_campaign_path =
      requested_campaign_dsl_path.empty()
          ? resolve_default_campaign_dsl_path(global_config_path, error)
          : std::filesystem::path(resolve_path_from_base_folder(
                global_config_path.parent_path().string(),
                requested_campaign_dsl_path));
  if (source_campaign_path.empty()) return {};
  std::error_code ec{};
  if (!std::filesystem::exists(source_campaign_path, ec) ||
      !std::filesystem::is_regular_file(source_campaign_path, ec)) {
    if (error) {
      *error = "campaign DSL does not exist: " + source_campaign_path.string();
    }
    return {};
  }
  return source_campaign_path;
}

[[nodiscard]] bool prepare_campaign_dsl_snapshot_from_source(
    const app_context_t& app, const std::string& campaign_cursor,
    const std::filesystem::path& source_campaign_path,
    std::string* out_source_campaign_path,
    std::string* out_campaign_snapshot_path, std::string* error) {
  if (error) error->clear();
  if (out_source_campaign_path) out_source_campaign_path->clear();
  if (out_campaign_snapshot_path) out_campaign_snapshot_path->clear();

  std::string campaign_text{};
  if (!cuwacunu::hero::runtime::read_text_file(source_campaign_path,
                                               &campaign_text, error)) {
    return false;
  }
  std::string snapshot_text{};
  if (!rewrite_campaign_imports_absolute(source_campaign_path, campaign_text,
                                         &snapshot_text, error)) {
    return false;
  }
  const std::filesystem::path snapshot_path =
      cuwacunu::hero::runtime::runtime_campaign_dsl_path(
          app.defaults.campaigns_root, campaign_cursor);
  if (!cuwacunu::hero::runtime::write_text_file_atomic(snapshot_path,
                                                       snapshot_text, error)) {
    return false;
  }
  if (out_source_campaign_path) {
    *out_source_campaign_path = source_campaign_path.string();
  }
  if (out_campaign_snapshot_path) {
    *out_campaign_snapshot_path = snapshot_path.string();
  }
  return true;
}

[[nodiscard]] bool reconcile_campaign_record(
    const app_context_t& app,
    cuwacunu::hero::runtime::runtime_campaign_record_t* record,
    bool* out_changed, std::string* error) {
  if (error) error->clear();
  if (out_changed) *out_changed = false;
  if (!record) {
    if (error) *error = "campaign record pointer is null";
    return false;
  }

  const bool runner_alive =
      record->runner_pid.has_value() &&
      cuwacunu::hero::runtime::process_identity_alive(
          *record->runner_pid, record->runner_start_ticks, record->boot_id);
  if (record->state == "launching" || record->state == "running" ||
      record->state == "stopping") {
    std::string next_state = record->state;
    std::string next_detail = record->state_detail;
    if (runner_alive) {
      if (record->state == "launching") {
        next_state = "running";
        next_detail = "campaign runner alive";
      }
    } else {
      next_state = "failed";
      next_detail = (record->state == "stopping")
                        ? "stale forced-stop campaign runner is no longer alive"
                        : "campaign runner is no longer alive";
      if (!record->finished_at_ms.has_value()) {
        record->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      }
    }
    if (next_state != record->state || next_detail != record->state_detail) {
      record->state = std::move(next_state);
      record->state_detail = std::move(next_detail);
      record->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      if (!write_runtime_campaign(app, *record, error)) return false;
      if (out_changed) *out_changed = true;
    }
  }
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

struct campaign_start_lock_guard_t {
  int fd{-1};

  campaign_start_lock_guard_t() = default;
  campaign_start_lock_guard_t(const campaign_start_lock_guard_t&) = delete;
  campaign_start_lock_guard_t& operator=(const campaign_start_lock_guard_t&) =
      delete;

  campaign_start_lock_guard_t(campaign_start_lock_guard_t&& other) noexcept
      : fd(other.fd) {
    other.fd = -1;
  }

  campaign_start_lock_guard_t& operator=(
      campaign_start_lock_guard_t&& other) noexcept {
    if (this == &other) return *this;
    reset();
    fd = other.fd;
    other.fd = -1;
    return *this;
  }

  ~campaign_start_lock_guard_t() { reset(); }

  void reset() {
    if (fd >= 0) {
      (void)::flock(fd, LOCK_UN);
      (void)::close(fd);
      fd = -1;
    }
  }
};

[[nodiscard]] bool is_runtime_campaign_terminal_state(std::string_view state) {
  return state != "launching" && state != "running" && state != "stopping";
}

[[nodiscard]] bool is_runtime_job_terminal_state(std::string_view state) {
  return state != "launching" && state != "running" && state != "stopping" &&
         state != "orphaned";
}

[[nodiscard]] bool handle_tool_start_campaign(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error);
[[nodiscard]] bool handle_tool_list_campaigns(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error);
[[nodiscard]] bool handle_tool_get_campaign(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error);
[[nodiscard]] bool handle_tool_stop_campaign(
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
[[nodiscard]] bool handle_tool_tail_trace(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error);
[[nodiscard]] bool handle_tool_reconcile(app_context_t* app,
                                         const std::string& arguments_json,
                                         std::string* out_structured,
                                         std::string* out_error);
[[nodiscard]] bool tail_file_lines(const std::filesystem::path& path,
                                   std::size_t lines, std::string* out,
                                   std::string* error);
[[nodiscard]] bool launch_campaign_runner_process(
    const app_context_t& app,
    const cuwacunu::hero::runtime::runtime_campaign_record_t& seed_record,
    std::string* error);

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

[[nodiscard]] bool acquire_campaign_start_lock(
    const app_context_t& app, campaign_start_lock_guard_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "campaign start lock output pointer is null";
    return false;
  }
  out->reset();

  std::error_code ec{};
  std::filesystem::create_directories(app.defaults.campaigns_root, ec);
  if (ec) {
    if (error) {
      *error = "cannot create campaigns_root for campaign start lock: " +
               app.defaults.campaigns_root.string();
    }
    return false;
  }

  const std::filesystem::path lock_path =
      cuwacunu::hero::runtime::runtime_campaign_start_lock_path(
          app.defaults.campaigns_root);
  const int fd = ::open(lock_path.c_str(), O_CREAT | O_RDWR, 0600);
  if (fd < 0) {
    if (error) {
      *error = "cannot open campaign start lock: " + lock_path.string();
    }
    return false;
  }

  for (;;) {
    if (::flock(fd, LOCK_EX) == 0) {
      out->fd = fd;
      return true;
    }
    if (errno == EINTR) continue;
    (void)::close(fd);
    if (error) {
      *error = "cannot acquire campaign start lock: " + lock_path.string();
    }
    return false;
  }
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

[[nodiscard]] bool rewrite_campaign_imports_absolute(
    const std::filesystem::path& source_campaign_path,
    std::string_view input_text, std::string* out_text,
    std::string* error) {
  if (error) error->clear();
  if (!out_text) {
    if (error) *error = "campaign snapshot output pointer is null";
    return false;
  }
  *out_text = "";

  bool in_block_comment = false;
  std::ostringstream out;
  std::size_t pos = 0;
  bool first = true;
  while (pos <= input_text.size()) {
    const std::size_t end = input_text.find('\n', pos);
    std::string line = (end == std::string::npos)
                           ? std::string(input_text.substr(pos))
                           : std::string(input_text.substr(pos, end - pos));

    const auto rewrite_import =
        [&](std::string_view token, const std::string& current_line) {
          std::size_t token_pos = current_line.find(token);
          if (token_pos == std::string::npos) return current_line;
          const std::size_t quote_begin =
              current_line.find('"', token_pos + token.size());
          if (quote_begin == std::string::npos) return current_line;
          const std::size_t quote_end = current_line.find('"', quote_begin + 1);
          if (quote_end == std::string::npos) return current_line;
          const std::filesystem::path raw_path(
              current_line.substr(quote_begin + 1, quote_end - quote_begin - 1));
          if (raw_path.is_absolute()) return current_line;
          const std::filesystem::path rewritten =
              (source_campaign_path.parent_path() / raw_path).lexically_normal();
          return current_line.substr(0, quote_begin + 1) + rewritten.string() +
                 current_line.substr(quote_end);
        };

    if (!in_block_comment) {
      line = rewrite_import("IMPORT_CONTRACT", line);
      line = rewrite_import("FROM", line);
      line = rewrite_import("IMPORT_CONTRACT_FILE", line);
      line = rewrite_import("IMPORT_WAVE_FILE", line);
      line = rewrite_import("MARSHAL", line);
    }
    if (!first) out << "\n";
    first = false;
    out << line;
    in_block_comment =
        cuwacunu::hero::wave_contract_binding_runtime::detail::
            next_block_comment_state(line, in_block_comment);
    if (end == std::string::npos) break;
    pos = end + 1;
  }
  *out_text = out.str();
  return true;
}

[[nodiscard]] bool prepare_campaign_dsl_snapshot(
    const app_context_t& app, const std::string& campaign_cursor,
    const std::filesystem::path& global_config_path,
    std::string* out_source_campaign_path,
    std::string* out_campaign_snapshot_path, std::string* error) {
  const std::filesystem::path source_campaign_path =
      resolve_default_campaign_dsl_path(global_config_path, error);
  if (source_campaign_path.empty()) return false;
  return prepare_campaign_dsl_snapshot_from_source(
      app, campaign_cursor, source_campaign_path, out_source_campaign_path,
      out_campaign_snapshot_path, error);
}

[[nodiscard]] bool validate_cuwacunu_campaign_binary_for_launch(
    const app_context_t& app, std::string* error) {
  if (error) error->clear();

  const std::filesystem::path& worker_path =
      app.defaults.cuwacunu_campaign_binary;
  if (worker_path.empty()) {
    if (error) *error = "runtime defaults missing cuwacunu_campaign_binary";
    return false;
  }

  std::error_code ec{};
  if (!std::filesystem::exists(worker_path, ec) ||
      !std::filesystem::is_regular_file(worker_path, ec)) {
    if (error) {
      *error = "configured cuwacunu_campaign_binary does not exist: " +
               worker_path.string();
    }
    return false;
  }

  if (::access(worker_path.c_str(), X_OK) != 0) {
    if (error) {
      *error = "configured cuwacunu_campaign_binary is not executable: " +
               worker_path.string();
    }
    return false;
  }

  return true;
}

[[nodiscard]] bool decode_campaign_snapshot(
    const app_context_t& app, const std::string& campaign_dsl_path,
    cuwacunu::camahjucunu::iitepi_campaign_instruction_t* out_instruction,
    std::string* error) {
  if (error) error->clear();
  if (!out_instruction) {
    if (error) *error = "campaign decode output pointer is null";
    return false;
  }
  std::string grammar_text{};
  if (!cuwacunu::hero::runtime::read_text_file(app.defaults.campaign_grammar_path,
                                               &grammar_text, error)) {
    return false;
  }
  std::string campaign_text{};
  if (!cuwacunu::hero::runtime::read_text_file(campaign_dsl_path, &campaign_text,
                                               error)) {
    return false;
  }
  try {
    *out_instruction =
        cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
            grammar_text, campaign_text);
  } catch (const std::exception& e) {
    if (error) {
      *error = "failed to decode campaign DSL snapshot '" + campaign_dsl_path +
               "': " + e.what();
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool select_campaign_run_bind_ids(
    const cuwacunu::camahjucunu::iitepi_campaign_instruction_t& instruction,
    std::string requested_binding_id, std::vector<std::string>* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "campaign run-bind output pointer is null";
    return false;
  }
  out->clear();

  requested_binding_id = trim_ascii(requested_binding_id);
  if (!requested_binding_id.empty()) {
    const auto bind_it = std::find_if(
        instruction.binds.begin(), instruction.binds.end(),
        [&](const auto& bind) { return bind.id == requested_binding_id; });
    if (bind_it == instruction.binds.end()) {
      if (error) {
        *error = "campaign does not declare BIND '" + requested_binding_id + "'";
      }
      return false;
    }
    out->push_back(requested_binding_id);
    return true;
  }

  if (instruction.runs.empty()) {
    if (error) *error = "campaign does not define any RUN entries";
    return false;
  }
  for (const auto& run : instruction.runs) {
    const std::string bind_ref = trim_ascii(run.bind_ref);
    if (!bind_ref.empty()) out->push_back(bind_ref);
  }
  if (out->empty()) {
    if (error) *error = "campaign RUN list resolved to zero bind ids";
    return false;
  }
  return true;
}

[[nodiscard]] bool launch_campaign_under_lock(
    const app_context_t& app, const campaign_launch_request_t& request,
    cuwacunu::hero::runtime::runtime_campaign_record_t* out_record,
    std::string* error) {
  if (error) error->clear();
  if (out_record) *out_record = cuwacunu::hero::runtime::runtime_campaign_record_t{};

  std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> campaigns{};
  if (!list_runtime_campaigns(app, &campaigns, error)) return false;
  std::size_t active_campaigns = 0;
  for (auto& campaign : campaigns) {
    bool changed = false;
    if (!reconcile_campaign_record(app, &campaign, &changed, error)) return false;
    if (campaign.state == "launching" || campaign.state == "running" ||
        campaign.state == "stopping") {
      ++active_campaigns;
    }
  }
  if (app.defaults.max_active_campaigns > 0 &&
      active_campaigns >= app.defaults.max_active_campaigns) {
    if (error) *error = "max_active_campaigns reached";
    return false;
  }
  if (!validate_cuwacunu_campaign_binary_for_launch(app, error)) return false;

  const std::string campaign_cursor =
      cuwacunu::hero::runtime::make_campaign_cursor(app.defaults.campaigns_root);
  const std::filesystem::path source_campaign_path =
      resolve_requested_campaign_source_path(app, app.global_config_path,
                                            request.campaign_dsl_path, error);
  if (source_campaign_path.empty()) return false;

  std::string source_campaign_dsl_path{};
  std::string campaign_dsl_path{};
  if (!prepare_campaign_dsl_snapshot_from_source(app, campaign_cursor,
                                                 source_campaign_path,
                                                 &source_campaign_dsl_path,
                                                 &campaign_dsl_path, error)) {
    return false;
  }
  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  if (!decode_campaign_snapshot(app, campaign_dsl_path, &instruction, error)) {
    return false;
  }
  std::vector<std::string> run_bind_ids{};
  if (!select_campaign_run_bind_ids(instruction, request.binding_id, &run_bind_ids,
                                    error)) {
    return false;
  }

  cuwacunu::hero::runtime::runtime_campaign_record_t record{};
  record.campaign_cursor = campaign_cursor;
  record.boot_id = cuwacunu::hero::runtime::current_boot_id();
  record.state = "launching";
  record.state_detail = "detached campaign runner requested";
  record.marshal_session_id = request.marshal_session_id;
  record.global_config_path = app.global_config_path.string();
  record.source_campaign_dsl_path = source_campaign_dsl_path;
  record.campaign_dsl_path = campaign_dsl_path;
  record.reset_runtime_state = request.reset_runtime_state;
  record.stdout_path = cuwacunu::hero::runtime::runtime_campaign_stdout_path(
                           app.defaults.campaigns_root, campaign_cursor)
                           .string();
  record.stderr_path = cuwacunu::hero::runtime::runtime_campaign_stderr_path(
                           app.defaults.campaigns_root, campaign_cursor)
                           .string();
  record.started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  record.updated_at_ms = record.started_at_ms;
  record.run_bind_ids = std::move(run_bind_ids);

  if (!write_runtime_campaign(app, record, error)) return false;
  if (!launch_campaign_runner_process(app, record, error)) {
    record.state = "failed_to_start";
    record.state_detail = *error;
    record.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    record.updated_at_ms = *record.finished_at_ms;
    std::string ignored{};
    (void)write_runtime_campaign(app, record, &ignored);
    return false;
  }

  for (int attempt = 0; attempt < 80; ++attempt) {
    std::string poll_error{};
    if (read_runtime_campaign(app, campaign_cursor, &record, &poll_error)) {
      if (record.runner_pid.has_value() || record.state != "launching") {
        break;
      }
    }
    ::usleep(25 * 1000);
  }
  if (!read_runtime_campaign(app, campaign_cursor, &record, error)) return false;
  if (out_record) *out_record = record;
  return true;
}

[[nodiscard]] bool run_command_with_stdio_and_timeout(
    const std::vector<std::string>& argv, const std::filesystem::path& stdin_path,
    const std::filesystem::path& stdout_path,
    const std::filesystem::path& stderr_path, std::size_t timeout_sec,
    int* out_exit_code, std::string* error) {
  if (error) error->clear();
  if (out_exit_code) *out_exit_code = -1;
  if (argv.empty()) {
    if (error) *error = "command argv is empty";
    return false;
  }
  std::error_code ec{};
  if (!std::filesystem::exists(stdin_path, ec) ||
      !std::filesystem::is_regular_file(stdin_path, ec)) {
    if (error) *error = "command stdin path does not exist: " + stdin_path.string();
    return false;
  }
  std::filesystem::create_directories(stdout_path.parent_path(), ec);
  if (ec) {
    if (error) *error = "cannot create command stdout parent: " + stdout_path.string();
    return false;
  }
  std::filesystem::create_directories(stderr_path.parent_path(), ec);
  if (ec) {
    if (error) *error = "cannot create command stderr parent: " + stderr_path.string();
    return false;
  }
  const int stdout_probe =
      ::open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (stdout_probe < 0) {
    if (error) *error = "cannot open command stdout path: " + stdout_path.string();
    return false;
  }
  (void)::close(stdout_probe);
  const int stderr_probe =
      ::open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (stderr_probe < 0) {
    if (error) *error = "cannot open command stderr path: " + stderr_path.string();
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

[[nodiscard]] bool launch_campaign_runner_process(
    const app_context_t& app,
    const cuwacunu::hero::runtime::runtime_campaign_record_t& seed_record,
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

    const pid_t grandchild = ::fork();
    if (grandchild < 0) {
      const int child_errno = errno;
      write_child_errno_noexcept(pipe_fds[1], child_errno);
      _exit(127);
    }
    if (grandchild > 0) {
      _exit(0);
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
    args.push_back("--campaign-runner");
    args.push_back("--campaigns-root");
    args.push_back(app.defaults.campaigns_root.string());
    args.push_back("--campaign-cursor");
    args.push_back(seed_record.campaign_cursor);
    args.push_back("--worker-binary");
    args.push_back(app.defaults.cuwacunu_campaign_binary.string());
    args.push_back("--global-config");
    args.push_back(seed_record.global_config_path);
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
  int ignored_status = 0;
  while (::waitpid(child, &ignored_status, 0) < 0 && errno == EINTR) {
  }
  if (n > 0) {
    if (error) {
      *error = "cannot exec detached runtime campaign runner: errno=" +
               std::to_string(exec_errno);
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool handle_tool_start_campaign(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string binding_id{};
  bool reset_runtime_state = false;
  std::string campaign_dsl_path{};
  std::string marshal_session_id{};
  (void)extract_json_string_field(arguments_json, "binding_id", &binding_id);
  (void)extract_json_bool_field(arguments_json, "reset_runtime_state",
                                &reset_runtime_state);
  (void)extract_json_string_field(arguments_json, "campaign_dsl_path",
                                  &campaign_dsl_path);
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  binding_id = trim_ascii(binding_id);
  campaign_dsl_path = trim_ascii(campaign_dsl_path);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (app->global_config_path.empty()) {
    *out_error = "runtime MCP app missing global_config_path";
    return false;
  }

  const std::filesystem::path source_campaign_path =
      resolve_requested_campaign_source_path(*app, app->global_config_path,
                                            campaign_dsl_path, out_error);
  if (source_campaign_path.empty()) return false;

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t source_instruction{};
  if (!decode_campaign_snapshot(*app, source_campaign_path.string(),
                                &source_instruction, out_error)) {
    return false;
  }

  std::vector<std::string> warnings{};
  if (marshal_session_id.empty() &&
      !trim_ascii(source_instruction.marshal_objective_file).empty()) {
    warnings.push_back(
        "campaign declares MARSHAL \"" +
        trim_ascii(source_instruction.marshal_objective_file) +
        "\" but Runtime Hero direct launch does not run supervision; use "
        "hero.marshal.start_session(...) when you want the MARSHAL objective honored");
  }

  campaign_launch_request_t request{};
  request.binding_id = binding_id;
  request.reset_runtime_state = reset_runtime_state;
  request.campaign_dsl_path = source_campaign_path.string();
  request.marshal_session_id = marshal_session_id;

  campaign_start_lock_guard_t start_lock{};
  if (!acquire_campaign_start_lock(*app, &start_lock, out_error)) return false;
  cuwacunu::hero::runtime::runtime_campaign_record_t record{};
  if (!launch_campaign_under_lock(*app, request, &record, out_error)) {
    return false;
  }

  const std::string warnings_json = json_array_from_strings(warnings);
  *out_structured = "{\"campaign_cursor\":" + json_quote(record.campaign_cursor) +
                    ",\"campaign\":" + runtime_campaign_to_json(record) +
                    ",\"warnings\":" + warnings_json + "}";
  return true;
}

[[nodiscard]] bool handle_tool_list_campaigns(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error) {
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

  std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> campaigns{};
  if (!list_runtime_campaigns(*app, &campaigns, out_error)) return false;

  std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> rows{};
  for (auto& campaign : campaigns) {
    bool changed = false;
    if (!reconcile_campaign_record(*app, &campaign, &changed, out_error)) {
      return false;
    }
    if (!state_filter.empty() && campaign.state != state_filter) continue;
    rows.push_back(std::move(campaign));
  }

  std::sort(rows.begin(), rows.end(), [newest_first](const auto& a,
                                                     const auto& b) {
    if (a.started_at_ms != b.started_at_ms) {
      return newest_first ? (a.started_at_ms > b.started_at_ms)
                          : (a.started_at_ms < b.started_at_ms);
    }
    return newest_first ? (a.campaign_cursor > b.campaign_cursor)
                        : (a.campaign_cursor < b.campaign_cursor);
  });

  const std::size_t total = rows.size();
  const std::size_t off = std::min(offset, rows.size());
  std::size_t count = limit;
  if (count == 0) count = rows.size() - off;
  count = std::min(count, rows.size() - off);

  std::ostringstream campaigns_json;
  campaigns_json << "[";
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0) campaigns_json << ",";
    campaigns_json << runtime_campaign_to_json(rows[off + i]);
  }
  campaigns_json << "]";

  std::ostringstream out;
  out << "{\"campaign_cursor\":\"\""
      << ",\"count\":" << count
      << ",\"total\":" << total
      << ",\"state\":" << json_quote(state_filter)
      << ",\"campaigns\":" << campaigns_json.str() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_tail_trace(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string job_cursor{};
  std::size_t lines = app->defaults.tail_default_lines;
  (void)extract_json_string_field(arguments_json, "job_cursor", &job_cursor);
  (void)extract_json_size_field(arguments_json, "lines", &lines);
  job_cursor = trim_ascii(job_cursor);
  if (job_cursor.empty()) {
    *out_error = "tail_trace requires arguments.job_cursor";
    return false;
  }

  cuwacunu::hero::runtime::runtime_job_record_t record{};
  if (!read_runtime_job(*app, job_cursor, &record, out_error)) return false;

  const auto trace_path = runtime_job_trace_path_for_record(*app, record);
  std::string text{};
  if (!tail_file_lines(trace_path, lines, &text, out_error)) return false;

  std::ostringstream out;
  out << "{\"job_cursor\":" << json_quote(job_cursor)
      << ",\"lines\":" << lines
      << ",\"path\":" << json_quote(trace_path.string())
      << ",\"text\":" << json_quote(text) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_campaign(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string campaign_cursor{};
  (void)extract_json_string_field(arguments_json, "campaign_cursor",
                                  &campaign_cursor);
  campaign_cursor = trim_ascii(campaign_cursor);
  if (campaign_cursor.empty()) {
    *out_error = "get_campaign requires arguments.campaign_cursor";
    return false;
  }

  cuwacunu::hero::runtime::runtime_campaign_record_t record{};
  if (!read_runtime_campaign(*app, campaign_cursor, &record, out_error)) {
    return false;
  }
  bool changed = false;
  if (!reconcile_campaign_record(*app, &record, &changed, out_error)) {
    return false;
  }
  *out_structured =
      "{\"campaign_cursor\":" + json_quote(campaign_cursor) + ",\"campaign\":" +
      runtime_campaign_to_json(record) + "}";
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

[[nodiscard]] bool handle_tool_stop_campaign(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string campaign_cursor{};
  bool force = false;
  (void)extract_json_string_field(arguments_json, "campaign_cursor",
                                  &campaign_cursor);
  (void)extract_json_bool_field(arguments_json, "force", &force);
  campaign_cursor = trim_ascii(campaign_cursor);
  if (campaign_cursor.empty()) {
    *out_error = "stop_campaign requires arguments.campaign_cursor";
    return false;
  }

  cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
  if (!read_runtime_campaign(*app, campaign_cursor, &campaign, out_error)) {
    return false;
  }
  bool campaign_changed = false;
  if (!reconcile_campaign_record(*app, &campaign, &campaign_changed, out_error)) {
    return false;
  }
  if (is_runtime_campaign_terminal_state(campaign.state)) {
    if (!campaign.marshal_session_id.empty()) {
      cuwacunu::hero::marshal::marshal_session_record_t loop{};
      std::string loop_error{};
      if (read_linked_marshal_session(*app, campaign.marshal_session_id, &loop,
                                 &loop_error)) {
        loop.phase = "finished";
        loop.phase_detail = "stop_campaign requested after terminal campaign";
        loop.pause_kind = "none";
        loop.finish_reason = "terminated";
        loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
        loop.updated_at_ms = *loop.finished_at_ms;
        loop.active_campaign_cursor.clear();
        std::string ignored{};
        (void)write_linked_marshal_session(*app, loop, &ignored);
        (void)append_marshal_session_event(loop, "session_terminated",
                                      loop.phase_detail,
                                      &ignored);
      }
    }
    *out_structured =
        "{\"campaign_cursor\":" + json_quote(campaign_cursor) +
        ",\"campaign\":" + runtime_campaign_to_json(campaign) + "}";
    return true;
  }

  bool issued_stop = false;
  if (!campaign.job_cursors.empty()) {
    const std::string latest_job_cursor = campaign.job_cursors.back();
    cuwacunu::hero::runtime::runtime_job_record_t latest_job{};
    if (!read_runtime_job(*app, latest_job_cursor, &latest_job, out_error)) {
      return false;
    }
    bool job_changed = false;
    if (!reconcile_job_record(*app, &latest_job, &job_changed, out_error)) {
      return false;
    }
    const auto observation =
        cuwacunu::hero::runtime::observe_runtime_job(latest_job);
    const std::string stable_state =
        cuwacunu::hero::runtime::stable_state_for_observation(latest_job,
                                                              observation);
    if (!is_runtime_job_terminal_state(stable_state)) {
      std::string ignored{};
      const std::string stop_job_args =
          "{\"job_cursor\":" + json_quote(latest_job_cursor) +
          ",\"force\":" + std::string(force ? "true" : "false") + "}";
      if (!handle_tool_stop_job(app, stop_job_args, &ignored, out_error)) {
        return false;
      }
      issued_stop = true;
    }
  }

  if (campaign.runner_pid.has_value() &&
      cuwacunu::hero::runtime::process_identity_alive(
          *campaign.runner_pid, campaign.runner_start_ticks, campaign.boot_id)) {
    const int sig = force ? SIGKILL : SIGTERM;
    if (::kill(static_cast<pid_t>(*campaign.runner_pid), sig) != 0 &&
        errno != ESRCH) {
      if (out_error->empty()) *out_error = "failed to signal campaign runner";
      return false;
    }
    issued_stop = true;
  }

  if (!issued_stop) {
    if (!reconcile_campaign_record(*app, &campaign, &campaign_changed, out_error)) {
      return false;
    }
    if (is_runtime_campaign_terminal_state(campaign.state)) {
      *out_structured =
          "{\"campaign_cursor\":" + json_quote(campaign_cursor) +
          ",\"campaign\":" + runtime_campaign_to_json(campaign) + "}";
      return true;
    }
    *out_error = "campaign has no live process identity metadata";
    return false;
  }

  campaign.state = "stopping";
  campaign.state_detail = force ? "sent SIGKILL to campaign runner"
                                : "sent SIGTERM to campaign runner";
  campaign.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!write_runtime_campaign(*app, campaign, out_error)) return false;

  if (!campaign.marshal_session_id.empty()) {
    cuwacunu::hero::marshal::marshal_session_record_t loop{};
    std::string loop_error{};
    if (read_linked_marshal_session(*app, campaign.marshal_session_id, &loop,
                               &loop_error)) {
      loop.phase = "finished";
      loop.phase_detail = "stop_campaign requested by operator";
      loop.pause_kind = "none";
      loop.finish_reason = "terminated";
      loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      loop.updated_at_ms = *loop.finished_at_ms;
      loop.active_campaign_cursor.clear();
      std::string ignored{};
      (void)write_linked_marshal_session(*app, loop, &ignored);
      (void)append_marshal_session_event(loop, "session_terminated",
                                    "stop_campaign requested by operator",
                                    &ignored);
    }
  }

  *out_structured =
      "{\"campaign_cursor\":" + json_quote(campaign_cursor) +
      ",\"campaign\":" + runtime_campaign_to_json(campaign) + "}";
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
  std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> campaigns{};
  if (!list_runtime_campaigns(*app, &campaigns, out_error)) return false;

  std::size_t changed_jobs = 0;
  std::size_t changed_campaigns = 0;
  std::ostringstream jobs_json;
  jobs_json << "[";
  bool first = true;
  for (auto& job : jobs) {
    bool updated = false;
    if (!reconcile_job_record(*app, &job, &updated, out_error)) return false;
    if (updated) ++changed_jobs;
    const auto observation = cuwacunu::hero::runtime::observe_runtime_job(job);
    if (!first) jobs_json << ",";
    first = false;
    jobs_json << runtime_job_to_json(job, observation);
  }
  jobs_json << "]";

  std::ostringstream campaigns_json;
  campaigns_json << "[";
  first = true;
  for (auto& campaign : campaigns) {
    bool updated = false;
    if (!reconcile_campaign_record(*app, &campaign, &updated, out_error)) {
      return false;
    }
    if (updated) ++changed_campaigns;
    if (!first) campaigns_json << ",";
    first = false;
    campaigns_json << runtime_campaign_to_json(campaign);
  }
  campaigns_json << "]";

  std::ostringstream out;
  out << "{\"campaign_cursor\":\"\",\"job_cursor\":\"\""
      << ",\"job_count\":" << jobs.size()
      << ",\"campaign_count\":" << campaigns.size()
      << ",\"changed_jobs\":" << changed_jobs
      << ",\"changed_campaigns\":" << changed_campaigns
      << ",\"jobs\":" << jobs_json.str()
      << ",\"campaigns\":" << campaigns_json.str() << "}";
  *out_structured = out.str();
  return true;
}

}  // namespace

std::filesystem::path resolve_runtime_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "REAL_HERO", "runtime_hero_dsl_filename");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved);
}

[[nodiscard]] std::filesystem::path resolve_runtime_root_from_global_config(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "GENERAL", "runtime_root");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved);
}

[[nodiscard]] std::filesystem::path derive_campaigns_root(
    const std::filesystem::path& runtime_root) {
  if (runtime_root.empty()) return {};
  return (runtime_root / ".campaigns").lexically_normal();
}

[[nodiscard]] std::filesystem::path derive_marshal_root(
    const std::filesystem::path& runtime_root) {
  return cuwacunu::hero::marshal::marshal_root(runtime_root);
}

[[nodiscard]] std::filesystem::path resolve_campaign_grammar_from_global_config(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "BNF", "iitepi_campaign_grammar_filename");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved);
}

bool load_runtime_defaults(const std::filesystem::path& hero_dsl_path,
                           const std::filesystem::path& global_config_path,
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

  out->campaigns_root =
      derive_campaigns_root(resolve_runtime_root_from_global_config(global_config_path));
  out->marshal_root =
      derive_marshal_root(resolve_runtime_root_from_global_config(global_config_path));
  bool saw_cuwacunu_campaign_binary = false;
  if (const auto it = values.find("cuwacunu_campaign_binary");
      it != values.end()) {
    saw_cuwacunu_campaign_binary = true;
    out->cuwacunu_campaign_binary = resolve_local_path(it->second);
  }
  out->campaign_grammar_path =
      resolve_campaign_grammar_from_global_config(global_config_path);
  bool saw_tail_default_lines = false;
  if (const auto it = values.find("tail_default_lines"); it != values.end()) {
    saw_tail_default_lines = true;
    if (!parse_size_token(it->second, &out->tail_default_lines)) {
      if (error) *error = "invalid tail_default_lines in " + hero_dsl_path.string();
      return false;
    }
  }
  bool saw_max_active_campaigns = false;
  if (const auto it = values.find("max_active_campaigns"); it != values.end()) {
    saw_max_active_campaigns = true;
    if (!parse_size_token(it->second, &out->max_active_campaigns)) {
      if (error) *error = "invalid max_active_campaigns in " + hero_dsl_path.string();
      return false;
    }
  }

  if (!saw_cuwacunu_campaign_binary ||
      out->cuwacunu_campaign_binary.empty()) {
    if (error) {
      *error =
          "missing cuwacunu_campaign_binary in " + hero_dsl_path.string();
    }
    return false;
  }
  if (out->campaigns_root.empty()) {
    if (error) {
      *error = "missing GENERAL.runtime_root in " + global_config_path.string();
    }
    return false;
  }
  if (out->marshal_root.empty()) {
    if (error) {
      *error = "cannot derive .marshal_hero root from GENERAL.runtime_root in " +
               global_config_path.string();
    }
    return false;
  }
  if (out->campaign_grammar_path.empty()) {
    if (error) {
      *error =
          "missing [BNF].iitepi_campaign_grammar_filename in " +
          global_config_path.string();
    }
    return false;
  }
  if (!saw_tail_default_lines || out->tail_default_lines == 0) {
    if (error) *error = "missing/invalid tail_default_lines in " + hero_dsl_path.string();
    return false;
  }
  if (!saw_max_active_campaigns) {
    if (error) *error = "missing max_active_campaigns in " + hero_dsl_path.string();
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
  const std::size_t tool_count =
      sizeof(kRuntimeTools) / sizeof(kRuntimeTools[0]);
  for (std::size_t i = 0; i < tool_count; ++i) {
    const auto& tool = kRuntimeTools[i];
    if (i != 0) out << ",";
    const bool read_only =
        tool.name == "hero.runtime.list_campaigns" ||
        tool.name == "hero.runtime.get_campaign" ||
        tool.name == "hero.runtime.list_jobs" ||
        tool.name == "hero.runtime.get_job" ||
        tool.name == "hero.runtime.tail_log" ||
        tool.name == "hero.runtime.tail_trace";
    const bool destructive = tool.name == "hero.runtime.stop_campaign" ||
                             tool.name == "hero.runtime.stop_job";
    out << "{\"name\":" << json_quote(tool.name)
        << ",\"description\":" << json_quote(tool.description)
        << ",\"inputSchema\":" << tool.input_schema_json
        << ",\"annotations\":{\"readOnlyHint\":"
        << (read_only ? "true" : "false") << ",\"destructiveHint\":"
        << (destructive ? "true" : "false")
        << ",\"openWorldHint\":false}}";
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
