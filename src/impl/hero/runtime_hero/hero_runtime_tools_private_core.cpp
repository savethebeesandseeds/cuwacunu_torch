#include "hero/runtime_hero/hero_runtime_tools.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"
#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "hero/marshal_hero/marshal_session.h"
#include "hero/runtime_hero/runtime_campaign.h"
#include "hero/runtime_hero/runtime_job.h"
#include "hero/wave_contract_binding_runtime.h"

namespace cuwacunu {
namespace hero {
namespace runtime_mcp {

namespace {

constexpr const char *kDefaultCampaignDslKey =
    "default_iitepi_campaign_dsl_filename";
constexpr const char *kServerName = "hero_runtime_mcp";
constexpr const char *kServerVersion = "0.1.0";
constexpr const char *kProtocolVersion = "2025-03-26";
constexpr const char *kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.runtime.*.";
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;
constexpr std::size_t kDefaultInlineLogLines = 40;
constexpr std::size_t kHardInlineLogLines = 120;
constexpr std::size_t kDefaultInlineLogBytes = 8u << 10;
constexpr std::size_t kHardInlineLogBytes = 32u << 10;

bool g_jsonrpc_use_content_length_framing = false;

[[nodiscard]] bool
rewrite_campaign_imports_absolute(const std::filesystem::path &original_path,
                                  std::string_view original_text,
                                  std::string *out_text, std::string *error);

[[nodiscard]] bool decode_campaign_snapshot(
    const app_context_t &app, const std::string &campaign_dsl_path,
    cuwacunu::camahjucunu::iitepi_campaign_instruction_t *out_instruction,
    std::string *error);

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

[[nodiscard]] bool is_ansi_escape_final_byte(unsigned char c) {
  return c >= 0x40 && c <= 0x7E;
}

[[nodiscard]] std::string sanitize_inline_log_text(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(in[i]);
    if (c == 0x1B) {
      if (i + 1 < in.size() && in[i + 1] == '[') {
        i += 2;
        while (i < in.size() &&
               !is_ansi_escape_final_byte(static_cast<unsigned char>(in[i]))) {
          ++i;
        }
      }
      continue;
    }
    if (c == '\r') {
      if (!out.empty() && out.back() != '\n')
        out.push_back('\n');
      continue;
    }
    if (c < 0x20 && c != '\n' && c != '\t')
      continue;
    out.push_back(static_cast<char>(c));
  }
  return out;
}

[[nodiscard]] std::size_t count_text_lines(std::string_view text) {
  if (text.empty())
    return 0;
  std::size_t lines = 0;
  for (const char c : text) {
    if (c == '\n')
      ++lines;
  }
  if (text.back() != '\n')
    ++lines;
  return lines;
}

[[nodiscard]] std::uintmax_t
file_size_or_zero(const std::filesystem::path &path) {
  std::error_code ec{};
  if (!std::filesystem::exists(path, ec) ||
      !std::filesystem::is_regular_file(path, ec)) {
    return 0;
  }
  const auto size = std::filesystem::file_size(path, ec);
  return ec ? 0 : size;
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
    if (!in_single && !in_double && (c == '#' || c == ';'))
      break;
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

[[nodiscard]] std::optional<std::string>
read_ini_value(const std::filesystem::path &ini_path,
               const std::string &section, const std::string &key) {
  std::ifstream in(ini_path);
  if (!in)
    return std::nullopt;
  std::string line;
  std::string current_section;
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(strip_inline_comment(line));
    if (trimmed.empty())
      continue;
    if (trimmed.size() >= 2 && trimmed.front() == '[' &&
        trimmed.back() == ']') {
      current_section = trim_ascii(trimmed.substr(1, trimmed.size() - 2));
      continue;
    }
    if (current_section != section)
      continue;
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos)
      continue;
    if (trim_ascii(trimmed.substr(0, eq)) != key)
      continue;
    return trim_ascii(trimmed.substr(eq + 1));
  }
  return std::nullopt;
}

[[nodiscard]] std::string resolve_path_from_base_folder(std::string base_folder,
                                                        std::string path) {
  base_folder = trim_ascii(std::move(base_folder));
  path = trim_ascii(std::move(path));
  if (path.empty())
    return {};
  const std::filesystem::path p(path);
  if (p.is_absolute())
    return p.lexically_normal().string();
  if (base_folder.empty())
    return p.lexically_normal().string();
  return (std::filesystem::path(base_folder) / p).lexically_normal().string();
}

[[nodiscard]] bool parse_bool_token(std::string_view raw, bool *out) {
  if (!out)
    return false;
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

[[nodiscard]] bool parse_size_token(std::string_view raw, std::size_t *out) {
  if (!out)
    return false;
  const std::string token = trim_ascii(raw);
  if (token.empty())
    return false;
  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(token.data(), token.data() + token.size(), parsed);
  if (ec != std::errc{} || ptr != token.data() + token.size())
    return false;
  *out = static_cast<std::size_t>(parsed);
  return true;
}

[[nodiscard]] std::string
make_tool_result_json(std::string_view text, std::string_view structured_json,
                      bool is_error) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":" << json_quote(text)
      << "}],\"structuredContent\":" << structured_json
      << ",\"isError\":" << (is_error ? "true" : "false") << "}";
  return out.str();
}

[[nodiscard]] std::size_t skip_json_whitespace(const std::string &json,
                                               std::size_t pos) {
  while (pos < json.size() &&
         std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
    ++pos;
  }
  return pos;
}

[[nodiscard]] bool parse_json_string_at(const std::string &json,
                                        std::size_t pos, std::string *out,
                                        std::size_t *out_end) {
  if (out)
    out->clear();
  if (pos >= json.size() || json[pos] != '"')
    return false;
  ++pos;
  std::string parsed;
  while (pos < json.size()) {
    const char c = json[pos++];
    if (c == '"') {
      if (out)
        *out = parsed;
      if (out_end)
        *out_end = pos;
      return true;
    }
    if (c == '\\') {
      if (pos >= json.size())
        return false;
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

[[nodiscard]] bool find_json_value_end(const std::string &json, std::size_t pos,
                                       std::size_t *out_end) {
  if (pos >= json.size())
    return false;
  const char c = json[pos];
  if (c == '"') {
    std::size_t end = 0;
    if (!parse_json_string_at(json, pos, nullptr, &end))
      return false;
    if (out_end)
      *out_end = end;
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
      if (ch == '{')
        ++depth;
      if (ch == '}') {
        --depth;
        if (depth == 0) {
          if (out_end)
            *out_end = i + 1;
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
  if (out_end)
    *out_end = end;
  return true;
}

[[nodiscard]] bool extract_json_raw_field(const std::string &json,
                                          const std::string &key,
                                          std::string *out) {
  if (out)
    out->clear();
  std::size_t pos = skip_json_whitespace(json, 0);
  if (pos >= json.size() || json[pos] != '{')
    return false;
  ++pos;
  for (;;) {
    pos = skip_json_whitespace(json, pos);
    if (pos >= json.size())
      return false;
    if (json[pos] == '}')
      return false;

    std::string current_key;
    std::size_t after_key = pos;
    if (!parse_json_string_at(json, pos, &current_key, &after_key))
      return false;
    pos = skip_json_whitespace(json, after_key);
    if (pos >= json.size() || json[pos] != ':')
      return false;
    ++pos;
    pos = skip_json_whitespace(json, pos);
    if (pos >= json.size())
      return false;

    std::size_t value_end = pos;
    if (!find_json_value_end(json, pos, &value_end))
      return false;
    if (current_key == key) {
      if (out)
        *out = json.substr(pos, value_end - pos);
      return true;
    }

    pos = skip_json_whitespace(json, value_end);
    if (pos >= json.size())
      return false;
    if (json[pos] == ',') {
      ++pos;
      continue;
    }
    if (json[pos] == '}')
      return false;
    return false;
  }
}

[[nodiscard]] bool extract_json_object_field(const std::string &json,
                                             const std::string &key,
                                             std::string *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw))
    return false;
  raw = trim_ascii(raw);
  if (raw.empty() || raw.front() != '{')
    return false;
  if (out)
    *out = raw;
  return true;
}

[[nodiscard]] bool extract_json_array_field(const std::string &json,
                                            const std::string &key,
                                            std::string *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw))
    return false;
  raw = trim_ascii(raw);
  if (raw.empty() || raw.front() != '[')
    return false;
  if (out)
    *out = raw;
  return true;
}

[[nodiscard]] bool extract_json_string_field(const std::string &json,
                                             const std::string &key,
                                             std::string *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw))
    return false;
  std::size_t end = 0;
  if (!parse_json_string_at(raw, 0, out, &end))
    return false;
  end = skip_json_whitespace(raw, end);
  return end == raw.size();
}

[[nodiscard]] bool extract_json_bool_field(const std::string &json,
                                           const std::string &key, bool *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw))
    return false;
  const std::string lowered = lowercase_copy(trim_ascii(raw));
  if (lowered == "true") {
    if (out)
      *out = true;
    return true;
  }
  if (lowered == "false") {
    if (out)
      *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] bool extract_json_size_field(const std::string &json,
                                           const std::string &key,
                                           std::size_t *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw))
    return false;
  return parse_size_token(raw, out);
}

[[nodiscard]] bool extract_json_id_field(const std::string &json,
                                         std::string *out_id_json) {
  std::string raw_id;
  if (!extract_json_raw_field(json, "id", &raw_id)) {
    if (out_id_json)
      *out_id_json = "null";
    return true;
  }

  raw_id = trim_ascii(raw_id);
  if (raw_id.empty())
    return false;
  if (raw_id.front() == '"') {
    std::string parsed;
    std::size_t end = 0;
    if (!parse_json_string_at(raw_id, 0, &parsed, &end))
      return false;
    end = skip_json_whitespace(raw_id, end);
    if (end != raw_id.size())
      return false;
    if (out_id_json)
      *out_id_json = json_quote(parsed);
    return true;
  }
  if (out_id_json)
    *out_id_json = raw_id;
  return true;
}

[[nodiscard]] bool split_json_array_items(const std::string &json_array,
                                          std::vector<std::string> *out,
                                          std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "json array output pointer is null";
    return false;
  }
  out->clear();
  const std::string raw = trim_ascii(json_array);
  if (raw.empty() || raw.front() != '[' || raw.back() != ']') {
    if (error)
      *error = "expected JSON array";
    return false;
  }
  std::size_t pos = skip_json_whitespace(raw, 1);
  while (pos < raw.size() && raw[pos] != ']') {
    std::size_t value_end = pos;
    if (!find_json_value_end(raw, pos, &value_end)) {
      if (error)
        *error = "cannot parse JSON array item";
      return false;
    }
    out->push_back(trim_ascii(raw.substr(pos, value_end - pos)));
    pos = skip_json_whitespace(raw, value_end);
    if (pos >= raw.size()) {
      if (error)
        *error = "unterminated JSON array";
      return false;
    }
    if (raw[pos] == ',') {
      pos = skip_json_whitespace(raw, pos + 1);
      continue;
    }
    if (raw[pos] == ']')
      break;
    if (error)
      *error = "unexpected token in JSON array";
    return false;
  }
  return true;
}

[[nodiscard]] bool starts_with_case_insensitive(std::string_view value,
                                                std::string_view prefix) {
  if (value.size() < prefix.size())
    return false;
  for (std::size_t i = 0; i < prefix.size(); ++i) {
    const char a =
        static_cast<char>(std::tolower(static_cast<unsigned char>(value[i])));
    const char b =
        static_cast<char>(std::tolower(static_cast<unsigned char>(prefix[i])));
    if (a != b)
      return false;
  }
  return true;
}

[[nodiscard]] bool parse_content_length_header(std::string_view header_line,
                                               std::size_t *out_length) {
  constexpr std::string_view kPrefix = "content-length:";
  const std::string line = trim_ascii(header_line);
  if (!starts_with_case_insensitive(line, kPrefix))
    return false;
  const std::string number = trim_ascii(line.substr(kPrefix.size()));
  return parse_size_token(number, out_length);
}

[[nodiscard]] bool read_next_jsonrpc_message(std::istream *in,
                                             std::string *out_payload,
                                             bool *out_used_content_length) {
  if (out_used_content_length)
    *out_used_content_length = false;

  std::string first_line;
  while (std::getline(*in, first_line)) {
    const std::string trimmed = trim_ascii(first_line);
    if (trimmed.empty())
      continue;

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
        if (header_trimmed.empty())
          break;
        std::size_t parsed_length = 0;
        if (parse_content_length_header(header_trimmed, &parsed_length)) {
          content_length = parsed_length;
        }
      }
      if (content_length == 0)
        continue;
      if (content_length > kMaxJsonRpcPayloadBytes)
        return false;

      std::string payload(content_length, '\0');
      in->read(payload.data(), static_cast<std::streamsize>(content_length));
      if (in->gcount() != static_cast<std::streamsize>(content_length)) {
        return false;
      }
      if (out_payload)
        *out_payload = std::move(payload);
      if (out_used_content_length)
        *out_used_content_length = true;
      return true;
    }

    if (out_payload)
      *out_payload = trimmed;
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

void write_jsonrpc_result(std::string_view id_json,
                          std::string_view result_json) {
  emit_jsonrpc_payload("{\"jsonrpc\":\"2.0\",\"id\":" + std::string(id_json) +
                       ",\"result\":" + std::string(result_json) + "}");
}

void write_jsonrpc_error(std::string_view id_json, int code,
                         std::string_view message) {
  emit_jsonrpc_payload("{\"jsonrpc\":\"2.0\",\"id\":" + std::string(id_json) +
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
    const cuwacunu::hero::runtime::runtime_job_record_t &record,
    const cuwacunu::hero::runtime::runtime_job_observation_t &observation) {
  const std::string observed_state =
      cuwacunu::hero::runtime::stable_state_for_observation(record,
                                                            observation);
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
      << "\"global_config_path\":" << json_quote(record.global_config_path)
      << ","
      << "\"source_campaign_dsl_path\":"
      << json_quote(record.source_campaign_dsl_path) << ","
      << "\"source_contract_dsl_path\":"
      << json_quote(record.source_contract_dsl_path) << ","
      << "\"source_wave_dsl_path\":" << json_quote(record.source_wave_dsl_path)
      << ","
      << "\"campaign_dsl_path\":" << json_quote(record.campaign_dsl_path) << ","
      << "\"contract_dsl_path\":" << json_quote(record.contract_dsl_path) << ","
      << "\"wave_dsl_path\":" << json_quote(record.wave_dsl_path) << ","
      << "\"binding_id\":" << json_quote(record.binding_id) << ","
      << "\"requested_device\":" << json_quote(record.requested_device) << ","
      << "\"resolved_device\":" << json_quote(record.resolved_device) << ","
      << "\"device_source_section\":"
      << json_quote(record.device_source_section) << ","
      << "\"device_contract_hash\":" << json_quote(record.device_contract_hash)
      << ","
      << "\"device_error\":" << json_quote(record.device_error) << ","
      << "\"cuda_required\":" << (record.cuda_required ? "true" : "false")
      << ","
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
      << ",\"runner_alive\":" << (observation.runner_alive ? "true" : "false")
      << ",\"target_alive\":" << (observation.target_alive ? "true" : "false")
      << "}";
  return out.str();
}

[[nodiscard]] std::string runtime_job_summary_to_json(
    const cuwacunu::hero::runtime::runtime_job_record_t &record,
    const cuwacunu::hero::runtime::runtime_job_observation_t &observation) {
  const std::string observed_state =
      cuwacunu::hero::runtime::stable_state_for_observation(record,
                                                            observation);
  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(record.schema) << ","
      << "\"job_cursor\":" << json_quote(record.job_cursor) << ","
      << "\"job_kind\":" << json_quote(record.job_kind) << ","
      << "\"state\":" << json_quote(record.state) << ","
      << "\"observed_state\":" << json_quote(observed_state) << ","
      << "\"state_detail\":" << json_quote(record.state_detail) << ","
      << "\"binding_id\":" << json_quote(record.binding_id) << ","
      << "\"requested_device\":" << json_quote(record.requested_device) << ","
      << "\"resolved_device\":" << json_quote(record.resolved_device) << ","
      << "\"device_error\":" << json_quote(record.device_error) << ","
      << "\"stdout_path\":" << json_quote(record.stdout_path) << ","
      << "\"stderr_path\":" << json_quote(record.stderr_path) << ","
      << "\"trace_path\":" << json_quote(record.trace_path) << ","
      << "\"started_at_ms\":" << record.started_at_ms << ","
      << "\"updated_at_ms\":" << record.updated_at_ms << ",\"finished_at_ms\":"
      << (record.finished_at_ms.has_value()
              ? std::to_string(*record.finished_at_ms)
              : "null")
      << ",\"exit_code\":"
      << (record.exit_code.has_value() ? std::to_string(*record.exit_code)
                                       : "null")
      << ",\"term_signal\":"
      << (record.term_signal.has_value() ? std::to_string(*record.term_signal)
                                         : "null")
      << ",\"runner_alive\":" << (observation.runner_alive ? "true" : "false")
      << ",\"target_alive\":" << (observation.target_alive ? "true" : "false")
      << "}";
  return out.str();
}

[[nodiscard]] bool
read_runtime_job(const app_context_t &app, std::string_view job_cursor,
                 cuwacunu::hero::runtime::runtime_job_record_t *out,
                 std::string *error) {
  return cuwacunu::hero::runtime::read_runtime_job_record(
      app.defaults.campaigns_root, job_cursor, out, error);
}

[[nodiscard]] bool list_runtime_jobs(
    const app_context_t &app,
    std::vector<cuwacunu::hero::runtime::runtime_job_record_t> *out,
    std::string *error) {
  return cuwacunu::hero::runtime::scan_runtime_job_records(
      app.defaults.campaigns_root, out, error);
}

[[nodiscard]] bool
write_runtime_job(const app_context_t &app,
                  const cuwacunu::hero::runtime::runtime_job_record_t &record,
                  std::string *error) {
  return cuwacunu::hero::runtime::write_runtime_job_record(
      app.defaults.campaigns_root, record, error);
}

[[nodiscard]] bool
reconcile_job_record(const app_context_t &app,
                     cuwacunu::hero::runtime::runtime_job_record_t *record,
                     bool *out_changed, std::string *error) {
  if (error)
    error->clear();
  if (out_changed)
    *out_changed = false;
  if (!record) {
    if (error)
      *error = "job record pointer is null";
    return false;
  }

  const auto observation =
      cuwacunu::hero::runtime::observe_runtime_job(*record);
  const std::string stable_state =
      cuwacunu::hero::runtime::stable_state_for_observation(*record,
                                                            observation);
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
        if (!write_runtime_job(app, *record, error))
          return false;
        if (out_changed)
          *out_changed = true;
      }
    } else if (stable_state == "orphaned") {
      const std::string expected_detail =
          "target process alive but worker process is gone";
      if (record->state_detail != expected_detail) {
        record->state_detail = expected_detail;
        record->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
        if (!write_runtime_job(app, *record, error))
          return false;
        if (out_changed)
          *out_changed = true;
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
  } else if (stable_state == "running") {
    record->state = "running";
    record->state_detail = "target process alive";
  } else {
    record->state = stable_state;
  }
  record->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!write_runtime_job(app, *record, error))
    return false;
  if (out_changed)
    *out_changed = true;
  return true;
}

[[nodiscard]] std::filesystem::path runtime_job_trace_path_for_record(
    const app_context_t &app,
    const cuwacunu::hero::runtime::runtime_job_record_t &record) {
  if (!record.trace_path.empty()) {
    return std::filesystem::path(record.trace_path);
  }
  return cuwacunu::hero::runtime::runtime_job_trace_path(
      app.defaults.campaigns_root, record.job_cursor);
}

[[nodiscard]] std::string runtime_campaign_to_json(
    const cuwacunu::hero::runtime::runtime_campaign_record_t &record) {
  const bool runner_alive =
      record.runner_pid.has_value() &&
      cuwacunu::hero::runtime::process_identity_alive(
          *record.runner_pid, record.runner_start_ticks, record.boot_id);
  std::ostringstream run_bind_ids_json;
  run_bind_ids_json << "[";
  for (std::size_t i = 0; i < record.run_bind_ids.size(); ++i) {
    if (i != 0)
      run_bind_ids_json << ",";
    run_bind_ids_json << json_quote(record.run_bind_ids[i]);
  }
  run_bind_ids_json << "]";

  std::ostringstream job_cursors_json;
  job_cursors_json << "[";
  for (std::size_t i = 0; i < record.job_cursors.size(); ++i) {
    if (i != 0)
      job_cursors_json << ",";
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
      << "\"marshal_session_id\":" << json_quote(record.marshal_session_id)
      << ","
      << "\"global_config_path\":" << json_quote(record.global_config_path)
      << ","
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
      << ",\"job_cursors\":" << job_cursors_json.str() << "}";
  return out.str();
}

[[nodiscard]] bool
read_runtime_campaign(const app_context_t &app,
                      std::string_view campaign_cursor,
                      cuwacunu::hero::runtime::runtime_campaign_record_t *out,
                      std::string *error) {
  return cuwacunu::hero::runtime::read_runtime_campaign_record(
      app.defaults.campaigns_root, campaign_cursor, out, error);
}

[[nodiscard]] bool list_runtime_campaigns(
    const app_context_t &app,
    std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> *out,
    std::string *error) {
  return cuwacunu::hero::runtime::scan_runtime_campaign_records(
      app.defaults.campaigns_root, out, error);
}

[[nodiscard]] bool write_runtime_campaign(
    const app_context_t &app,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &record,
    std::string *error) {
  return cuwacunu::hero::runtime::write_runtime_campaign_record(
      app.defaults.campaigns_root, record, error);
}

struct campaign_launch_request_t {
  std::string binding_id{};
  bool reset_runtime_state{false};
  std::string campaign_dsl_path{};
  std::string marshal_session_id{};
};

[[nodiscard]] std::string
json_array_from_strings(const std::vector<std::string> &values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0)
      out << ",";
    out << json_quote(values[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string component_selection_explanation_to_json(
    const cuwacunu::hero::wave_contract_binding_runtime::
        resolved_component_selection_explanation_t &explanation) {
  std::ostringstream out;
  out << "{"
      << "\"wave_binding_id\":" << json_quote(explanation.wave_binding_id)
      << ","
      << "\"component_kind\":" << json_quote(explanation.component_kind) << ","
      << "\"family\":" << json_quote(explanation.family) << ","
      << "\"component_compatibility_sha256_hex\":"
      << json_quote(explanation.component_compatibility_sha256_hex) << ","
      << "\"component_tag\":" << json_quote(explanation.component_tag) << ","
      << "\"resolved\":" << (explanation.resolved ? "true" : "false") << ","
      << "\"derived_hashimyei\":" << json_quote(explanation.derived_hashimyei)
      << ","
      << "\"derived_canonical_path\":"
      << json_quote(explanation.derived_canonical_path) << ","
      << "\"summary\":" << json_quote(explanation.summary) << ","
      << "\"details\":" << json_quote(explanation.details) << "}";
  return out.str();
}

[[nodiscard]] std::string binding_selection_explanation_to_json(
    const cuwacunu::hero::wave_contract_binding_runtime::
        resolved_binding_selection_explanation_t &explanation) {
  std::ostringstream components_json;
  components_json << "[";
  for (std::size_t i = 0; i < explanation.components.size(); ++i) {
    if (i != 0)
      components_json << ",";
    components_json << component_selection_explanation_to_json(
        explanation.components[i]);
  }
  components_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"binding_id\":" << json_quote(explanation.binding_id) << ","
      << "\"resolved\":" << (explanation.resolved ? "true" : "false") << ","
      << "\"summary\":" << json_quote(explanation.summary) << ","
      << "\"details\":" << json_quote(explanation.details) << ","
      << "\"compatibility_basis\":\"component_compatibility\","
      << "\"campaign_dsl_path\":"
      << json_quote(explanation.source_campaign_dsl_path) << ","
      << "\"source_contract_dsl_path\":"
      << json_quote(explanation.source_contract_dsl_path) << ","
      << "\"source_wave_dsl_path\":"
      << json_quote(explanation.source_wave_dsl_path) << ","
      << "\"contract_ref\":" << json_quote(explanation.contract_ref) << ","
      << "\"wave_ref\":" << json_quote(explanation.wave_ref) << ","
      << "\"contract_hash\":" << json_quote(explanation.contract_hash) << ","
      << "\"docking_signature_sha256_hex\":"
      << json_quote(explanation.docking_signature_sha256_hex) << ","
      << "\"component_count\":" << explanation.components.size() << ","
      << "\"components\":" << components_json.str() << "}";
  return out.str();
}

[[nodiscard]] std::filesystem::path resolve_default_campaign_dsl_path(
    const std::filesystem::path &global_config_path, std::string *error) {
  if (error)
    error->clear();
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "GENERAL", kDefaultCampaignDslKey);
  if (!configured.has_value()) {
    if (error) {
      *error = "missing GENERAL." + std::string(kDefaultCampaignDslKey) +
               " in " + global_config_path.string();
    }
    return {};
  }
  return std::filesystem::path(resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured));
}

[[nodiscard]] std::filesystem::path resolve_requested_campaign_source_path(
    const app_context_t &app, const std::filesystem::path &global_config_path,
    std::string requested_campaign_dsl_path, std::string *error) {
  if (error)
    error->clear();
  requested_campaign_dsl_path =
      trim_ascii(std::move(requested_campaign_dsl_path));
  const std::filesystem::path source_campaign_path =
      requested_campaign_dsl_path.empty()
          ? resolve_default_campaign_dsl_path(global_config_path, error)
          : std::filesystem::path(resolve_path_from_base_folder(
                global_config_path.parent_path().string(),
                requested_campaign_dsl_path));
  if (source_campaign_path.empty())
    return {};
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
    const app_context_t &app, const std::string &campaign_cursor,
    const std::filesystem::path &source_campaign_path,
    std::string *out_source_campaign_path,
    std::string *out_campaign_snapshot_path, std::string *error) {
  if (error)
    error->clear();
  if (out_source_campaign_path)
    out_source_campaign_path->clear();
  if (out_campaign_snapshot_path)
    out_campaign_snapshot_path->clear();

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
    const app_context_t &app,
    cuwacunu::hero::runtime::runtime_campaign_record_t *record,
    bool *out_changed, std::string *error) {
  if (error)
    error->clear();
  if (out_changed)
    *out_changed = false;
  if (!record) {
    if (error)
      *error = "campaign record pointer is null";
    return false;
  }

  // Older deliberate-stop reconciliations used `failed` once the runner
  // vanished, which made operator-requested campaign stops look erroneous in
  // downstream UIs. Normalize those legacy records into the explicit terminal
  // `stopped` state on the next reconciliation pass.
  if (record->state == "failed" &&
      record->state_detail == "stale forced-stop campaign runner is no longer "
                              "alive") {
    record->state = "stopped";
    record->state_detail = "campaign stopped by request";
    record->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    if (!record->finished_at_ms.has_value()) {
      record->finished_at_ms = record->updated_at_ms;
    }
    if (!write_runtime_campaign(app, *record, error))
      return false;
    if (out_changed)
      *out_changed = true;
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
      if (record->state == "stopping") {
        next_state = "stopped";
        next_detail = "campaign stopped by request";
      } else {
        next_state = "failed";
        next_detail = "campaign runner is no longer alive";
      }
      if (!record->finished_at_ms.has_value()) {
        record->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      }
    }
    if (next_state != record->state || next_detail != record->state_detail) {
      record->state = std::move(next_state);
      record->state_detail = std::move(next_detail);
      record->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      if (!write_runtime_campaign(app, *record, error))
        return false;
      if (out_changed)
        *out_changed = true;
    }
  }
  return true;
}

using runtime_tool_handler_t = bool (*)(app_context_t *app,
                                        const std::string &arguments_json,
                                        std::string *out_structured,
                                        std::string *out_error);

struct runtime_tool_descriptor_t {
  std::string_view name;
  std::string_view description;
  std::string_view input_schema_json;
  runtime_tool_handler_t handler;
};

struct campaign_start_lock_guard_t {
  int fd{-1};

  campaign_start_lock_guard_t() = default;
  campaign_start_lock_guard_t(const campaign_start_lock_guard_t &) = delete;
  campaign_start_lock_guard_t &
  operator=(const campaign_start_lock_guard_t &) = delete;

  campaign_start_lock_guard_t(campaign_start_lock_guard_t &&other) noexcept
      : fd(other.fd) {
    other.fd = -1;
  }

  campaign_start_lock_guard_t &
  operator=(campaign_start_lock_guard_t &&other) noexcept {
    if (this == &other)
      return *this;
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
