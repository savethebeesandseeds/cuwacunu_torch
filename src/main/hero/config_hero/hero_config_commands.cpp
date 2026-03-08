#include "main/hero/config_hero/hero_config_commands.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

#include "HERO/hero_config/hero.config.h"

namespace {
constexpr const char* kDeterministicOnlyMessage =
    "hero.ask and hero.fix are disabled in deterministic mode";
constexpr const char* kMcpServerName = "hero_config_mcp";
constexpr const char* kMcpServerVersion = "0.5.0";
constexpr const char* kDefaultMcpProtocolVersion = "2025-03-26";
bool g_jsonrpc_use_content_length_framing = false;

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

[[nodiscard]] std::string field_escape(std::string_view in) {
  std::string out;
  out.reserve(in.size() + 8);
  for (const char c : in) {
    if (c == '\t') {
      out += "\\t";
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else {
      out.push_back(c);
    }
  }
  return out;
}

void emit_ok(std::string_view command, std::string_view message) {
  std::cout << "ok\t" << command << '\t' << field_escape(message) << '\n';
}

void emit_err(std::string_view command, std::string_view message) {
  std::cout << "err\t" << command << '\t' << field_escape(message) << '\n';
}

void emit_data(std::string_view key, std::string_view value) {
  std::cout << "data\t" << field_escape(key) << '\t' << field_escape(value)
            << '\n';
}

void emit_end() {
  std::cout << "end\n";
  std::cout.flush();
}

[[nodiscard]] std::string json_escape(std::string_view in) {
  std::ostringstream out;
  for (const unsigned char c : in) {
    switch (c) {
      case '\"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
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
        if (c < 0x20) {
          out << "\\u00";
          const char* hex = "0123456789abcdef";
          out << hex[(c >> 4) & 0x0F] << hex[c & 0x0F];
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

[[nodiscard]] bool parse_json_string_at(const std::string& json, std::size_t pos,
                                        std::string* out,
                                        std::size_t* end_pos) {
  if (pos >= json.size() || json[pos] != '"') return false;
  ++pos;
  std::string value;
  while (pos < json.size()) {
    const char c = json[pos++];
    if (c == '"') {
      if (out) *out = value;
      if (end_pos) *end_pos = pos;
      return true;
    }
    if (c != '\\') {
      value.push_back(c);
      continue;
    }
    if (pos >= json.size()) return false;
    const char esc = json[pos++];
    switch (esc) {
      case '"':
      case '\\':
      case '/':
        value.push_back(esc);
        break;
      case 'b':
        value.push_back('\b');
        break;
      case 'f':
        value.push_back('\f');
        break;
      case 'n':
        value.push_back('\n');
        break;
      case 'r':
        value.push_back('\r');
        break;
      case 't':
        value.push_back('\t');
        break;
      default:
        value.push_back(esc);
        break;
    }
  }
  return false;
}

[[nodiscard]] std::size_t skip_json_whitespace(const std::string& json,
                                               std::size_t pos) {
  while (pos < json.size() &&
         std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
    ++pos;
  }
  return pos;
}

[[nodiscard]] bool find_json_value_end(const std::string& json, std::size_t pos,
                                       std::size_t* out_end_pos) {
  pos = skip_json_whitespace(json, pos);
  if (pos >= json.size()) return false;

  if (json[pos] == '"') {
    std::size_t end_pos = pos;
    if (!parse_json_string_at(json, pos, nullptr, &end_pos)) return false;
    if (out_end_pos) *out_end_pos = end_pos;
    return true;
  }

  if (json[pos] == '{' || json[pos] == '[') {
    std::vector<char> stack;
    stack.reserve(8);
    stack.push_back(json[pos] == '{' ? '}' : ']');
    bool in_string = false;
    bool escape = false;
    for (std::size_t i = pos + 1; i < json.size(); ++i) {
      const char c = json[i];
      if (in_string) {
        if (escape) {
          escape = false;
          continue;
        }
        if (c == '\\') {
          escape = true;
          continue;
        }
        if (c == '"') {
          in_string = false;
        }
        continue;
      }

      if (c == '"') {
        in_string = true;
        continue;
      }
      if (c == '{') {
        stack.push_back('}');
        continue;
      }
      if (c == '[') {
        stack.push_back(']');
        continue;
      }
      if (c == '}' || c == ']') {
        if (stack.empty() || stack.back() != c) return false;
        stack.pop_back();
        if (stack.empty()) {
          if (out_end_pos) *out_end_pos = i + 1;
          return true;
        }
      }
    }
    return false;
  }

  std::size_t end = pos;
  while (end < json.size()) {
    const char c = json[end];
    if (c == ',' || c == '}' || c == ']' ||
        std::isspace(static_cast<unsigned char>(c)) != 0) {
      break;
    }
    ++end;
  }
  if (end <= pos) return false;
  if (out_end_pos) *out_end_pos = end;
  return true;
}

[[nodiscard]] bool extract_json_raw_field(const std::string& json,
                                          const std::string& key,
                                          std::string* out) {
  std::size_t pos = skip_json_whitespace(json, 0);
  if (pos >= json.size() || json[pos] != '{') return false;
  ++pos;

  while (true) {
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
  const std::string trimmed = trim_ascii(raw);
  if (trimmed.empty() || trimmed.front() != '{') return false;
  if (out) *out = trimmed;
  return true;
}

[[nodiscard]] bool extract_json_string_field(const std::string& json,
                                             const std::string& key,
                                             std::string* out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  std::size_t end_pos = 0;
  if (!parse_json_string_at(raw, 0, out, &end_pos)) return false;
  end_pos = skip_json_whitespace(raw, end_pos);
  return end_pos == raw.size();
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
    std::size_t end_pos = 0;
    if (!parse_json_string_at(raw_id, 0, &parsed, &end_pos)) return false;
    end_pos = skip_json_whitespace(raw_id, end_pos);
    if (end_pos != raw_id.size()) return false;
    if (out_id_json) *out_id_json = json_quote(parsed);
    return true;
  }

  if (out_id_json) *out_id_json = raw_id;
  return true;
}

[[nodiscard]] bool has_json_field(const std::string& json,
                                  const std::string& key) {
  return extract_json_raw_field(json, key, nullptr);
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
  if (number.empty()) return false;

  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(number.data(), number.data() + number.size(), parsed);
  if (ec != std::errc() || ptr != number.data() + number.size()) return false;
  if (out_length) *out_length = static_cast<std::size_t>(parsed);
  return true;
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
      if (content_length == 0) {
        continue;
      }
      std::string payload(content_length, '\0');
      in->read(payload.data(), static_cast<std::streamsize>(content_length));
      if (in->gcount() != static_cast<std::streamsize>(content_length)) {
        return false;
      }
      if (out_payload) *out_payload = payload;
      if (out_used_content_length) *out_used_content_length = true;
      return true;
    }

    if (trimmed.front() == '{' || trimmed.front() == '[') {
      if (out_payload) *out_payload = trimmed;
      return true;
    }
  }

  return false;
}

void emit_jsonrpc_payload(std::string_view payload_json) {
  if (g_jsonrpc_use_content_length_framing) {
    std::cout << "Content-Length: " << payload_json.size() << "\r\n\r\n"
              << payload_json;
  } else {
    std::cout << payload_json << '\n';
  }
  std::cout.flush();
}

void emit_jsonrpc_result(std::string_view id_json,
                         std::string_view result_object_json) {
  const std::string payload = std::string("{\"jsonrpc\":\"2.0\",\"id\":") +
                              std::string(id_json) + ",\"result\":" +
                              std::string(result_object_json) + "}";
  emit_jsonrpc_payload(payload);
}

void emit_jsonrpc_error(std::string_view id_json, int code,
                        std::string_view message) {
  const std::string payload =
      std::string("{\"jsonrpc\":\"2.0\",\"id\":") + std::string(id_json) +
      ",\"error\":{\"code\":" + std::to_string(code) +
      ",\"message\":" + json_quote(message) + "}}";
  emit_jsonrpc_payload(payload);
}

[[nodiscard]] std::string bool_json(bool v) { return v ? "true" : "false"; }

[[nodiscard]] std::string make_text_content_result_json(std::string_view text,
                                                        bool is_error) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":" << json_quote(text)
      << "}],\"isError\":" << bool_json(is_error) << "}";
  return out.str();
}

[[nodiscard]] std::string make_tool_success_result_json(
    std::string_view tool_name, std::string_view structured_content_json) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":"
      << json_quote(std::string("tool ") + std::string(tool_name) +
                    " executed")
      << "}],\"structuredContent\":" << structured_content_json
      << ",\"isError\":false}";
  return out.str();
}

void print_help() {
  emit_ok("help", "available commands");
  emit_data("help", "ping");
  emit_data("help", "status");
  emit_data("help", "profiles");
  emit_data("help", "targets");
  emit_data("help", "schema");
  emit_data("help", "show");
  emit_data("help", "get <key>");
  emit_data("help", "set <key> <value>");
  emit_data("help", "validate");
  emit_data("help", "diff");
  emit_data("help", "dry_run");
  emit_data("help", "backups");
  emit_data("help", "rollback [backup_filename]");
  emit_data("help", "save");
  emit_data("help", "reload");
  emit_data("help", "quit");
  emit_end();
}

[[nodiscard]] std::string build_status_result(
    const cuwacunu::hero::mcp::hero_config_store_t& store) {
  const auto errors = store.validate();
  std::ostringstream out;
  out << "{";
  out << "\"valid\":" << bool_json(errors.empty()) << ",";
  out << "\"config_path\":" << json_quote(store.config_path()) << ",";
  out << "\"dirty\":" << bool_json(store.dirty()) << ",";
  out << "\"from_template\":" << bool_json(store.from_template()) << ",";
  out << "\"protocol_layer\":"
      << json_quote(store.get_or_default("protocol_layer")) << ",";
  out << "\"backup_enabled\":"
      << json_quote(store.get_or_default("backup_enabled")) << ",";
  out << "\"backup_dir\":" << json_quote(store.get_or_default("backup_dir"))
      << ",";
  out << "\"backup_max_entries\":"
      << json_quote(store.get_or_default("backup_max_entries")) << ",";
  out << "\"errors\":[";
  for (std::size_t i = 0; i < errors.size(); ++i) {
    if (i != 0) out << ",";
    out << json_quote(errors[i]);
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string build_save_preview_result(
    const cuwacunu::hero::mcp::hero_config_store_t::save_preview_t& preview,
    bool include_text) {
  const bool format_only = preview.text_changed && preview.diffs.empty();
  std::ostringstream out;
  out << "{";
  out << "\"file_exists\":" << bool_json(preview.file_exists) << ",";
  out << "\"text_changed\":" << bool_json(preview.text_changed) << ",";
  out << "\"format_only\":" << bool_json(format_only) << ",";
  out << "\"has_changes\":" << bool_json(preview.has_changes) << ",";
  out << "\"diff_count\":" << preview.diff_count << ",";
  out << "\"diffs\":[";
  for (std::size_t i = 0; i < preview.diffs.size(); ++i) {
    const auto& d = preview.diffs[i];
    if (i != 0) out << ",";
    out << "{"
        << "\"key\":" << json_quote(d.key) << ","
        << "\"action\":" << json_quote(d.action) << ","
        << "\"before_type\":" << json_quote(d.before_declared_type) << ","
        << "\"before_value\":" << json_quote(d.before_value) << ","
        << "\"after_type\":" << json_quote(d.after_declared_type) << ","
        << "\"after_value\":" << json_quote(d.after_value) << "}";
  }
  out << "]";
  if (include_text) {
    out << ",\"current_text\":" << json_quote(preview.current_text);
    out << ",\"proposed_text\":" << json_quote(preview.proposed_text);
  }
  out << "}";
  return out.str();
}

[[nodiscard]] std::string build_tools_list_result() {
  return R"JSON({"tools":[{"name":"hero.status","description":"Runtime status and validation snapshot","inputSchema":{"type":"object","properties":{},"additionalProperties":false}},{"name":"hero.schema","description":"Runtime key schema","inputSchema":{"type":"object","properties":{},"additionalProperties":false}},{"name":"hero.show","description":"Current key-value entries","inputSchema":{"type":"object","properties":{},"additionalProperties":false}},{"name":"hero.get","description":"Get one key","inputSchema":{"type":"object","properties":{"key":{"type":"string"}},"required":["key"],"additionalProperties":false}},{"name":"hero.set","description":"Set one key","inputSchema":{"type":"object","properties":{"key":{"type":"string"},"value":{"type":"string"}},"required":["key","value"],"additionalProperties":false}},{"name":"hero.validate","description":"Validate runtime config","inputSchema":{"type":"object","properties":{},"additionalProperties":false}},{"name":"hero.diff","description":"Preview config changes before save","inputSchema":{"type":"object","properties":{"include_text":{"type":"boolean"}},"additionalProperties":false}},{"name":"hero.dry_run","description":"Alias of hero.diff","inputSchema":{"type":"object","properties":{"include_text":{"type":"boolean"}},"additionalProperties":false}},{"name":"hero.backups","description":"List available config backups","inputSchema":{"type":"object","properties":{},"additionalProperties":false}},{"name":"hero.rollback","description":"Rollback config to a backup snapshot","inputSchema":{"type":"object","properties":{"backup":{"type":"string"}},"additionalProperties":false}},{"name":"hero.save","description":"Save runtime config to disk","inputSchema":{"type":"object","properties":{},"additionalProperties":false}},{"name":"hero.reload","description":"Reload runtime config from disk","inputSchema":{"type":"object","properties":{},"additionalProperties":false}}]})JSON";
}

[[nodiscard]] bool dispatch_tool_jsonrpc(
    const std::string& tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message) {
  if (out_error_code) *out_error_code = -32603;
  if (out_error_message) *out_error_message = "internal error";

  if (tool_name == "hero.status") {
    if (out_result_json) *out_result_json = build_status_result(*store);
    return true;
  }
  if (tool_name == "hero.schema") {
    std::ostringstream out;
    out << "{\"keys\":[";
    for (std::size_t i = 0; i < cuwacunu::hero::config::kRuntimeKeyDescriptors.size();
         ++i) {
      const auto& d = cuwacunu::hero::config::kRuntimeKeyDescriptors[i];
      if (i != 0) out << ",";
      out << "{"
          << "\"key\":" << json_quote(d.key) << ","
          << "\"type\":" << json_quote(d.type) << ","
          << "\"required\":" << bool_json(d.required) << ","
          << "\"default\":" << json_quote(d.default_value) << ","
          << "\"range\":" << json_quote(d.range) << ","
          << "\"allowed\":" << json_quote(d.allowed) << "}";
    }
    out << "]}";
    if (out_result_json) *out_result_json = out.str();
    return true;
  }
  if (tool_name == "hero.show") {
    const auto entries = store->entries_snapshot();
    std::ostringstream out;
    out << "{\"entries\":[";
    for (std::size_t i = 0; i < entries.size(); ++i) {
      if (i != 0) out << ",";
      out << "{"
          << "\"key\":" << json_quote(entries[i].key) << ","
          << "\"type\":" << json_quote(entries[i].declared_type) << ","
          << "\"value\":" << json_quote(entries[i].value) << "}";
    }
    out << "]}";
    if (out_result_json) *out_result_json = out.str();
    return true;
  }
  if (tool_name == "hero.get") {
    std::string key;
    if (!extract_json_string_field(request_json, "key", &key) || key.empty()) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) *out_error_message = "hero.get requires argument key";
      return false;
    }
    const auto value = store->get_value(key);
    if (!value.has_value()) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) *out_error_message = "key not found: " + key;
      return false;
    }
    if (out_result_json) {
      *out_result_json = "{\"key\":" + json_quote(key) + ",\"value\":" +
                         json_quote(*value) + "}";
    }
    return true;
  }
  if (tool_name == "hero.set") {
    std::string key;
    std::string value;
    if (!extract_json_string_field(request_json, "key", &key) || key.empty()) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) *out_error_message = "hero.set requires argument key";
      return false;
    }
    if (!extract_json_string_field(request_json, "value", &value)) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) *out_error_message = "hero.set requires argument value";
      return false;
    }
    std::string err;
    if (!store->set_value(key, value, &err)) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) *out_error_message = err;
      return false;
    }
    if (out_result_json) {
      *out_result_json = "{\"updated\":true,\"key\":" + json_quote(key) +
                         ",\"value\":" + json_quote(value) + "}";
    }
    return true;
  }
  if (tool_name == "hero.validate") {
    const auto errors = store->validate();
    std::ostringstream out;
    out << "{\"valid\":" << bool_json(errors.empty()) << ",\"errors\":[";
    for (std::size_t i = 0; i < errors.size(); ++i) {
      if (i != 0) out << ",";
      out << json_quote(errors[i]);
    }
    out << "]}";
    if (out_result_json) *out_result_json = out.str();
    return true;
  }
  if (tool_name == "hero.diff" || tool_name == "hero.dry_run") {
    bool include_text = false;
    bool parsed_include_text = false;
    if (extract_json_raw_field(request_json, "include_text", nullptr)) {
      if (!extract_json_bool_field(request_json, "include_text", &include_text)) {
        if (out_error_code) *out_error_code = -32602;
        if (out_error_message) {
          *out_error_message = tool_name + " include_text must be boolean";
        }
        return false;
      }
      parsed_include_text = true;
    }
    if (!parsed_include_text &&
        extract_json_bool_field(request_json, "includeText", &include_text)) {
      parsed_include_text = true;
    }
    (void)parsed_include_text;

    cuwacunu::hero::mcp::hero_config_store_t::save_preview_t preview;
    std::string err;
    if (!store->preview_save(&preview, &err)) {
      if (out_error_code) *out_error_code = -32603;
      if (out_error_message) *out_error_message = err;
      return false;
    }
    if (out_result_json) {
      *out_result_json = build_save_preview_result(preview, include_text);
    }
    return true;
  }
  if (tool_name == "hero.backups") {
    std::vector<cuwacunu::hero::mcp::hero_config_store_t::backup_entry_t>
        backups;
    std::string err;
    if (!store->list_backups(&backups, &err)) {
      if (out_error_code) *out_error_code = -32603;
      if (out_error_message) *out_error_message = err;
      return false;
    }
    std::ostringstream out;
    out << "{\"count\":" << backups.size() << ",\"backups\":[";
    for (std::size_t i = 0; i < backups.size(); ++i) {
      if (i != 0) out << ",";
      out << "{"
          << "\"index\":" << i << ","
          << "\"filename\":" << json_quote(backups[i].filename) << ","
          << "\"path\":" << json_quote(backups[i].path) << "}";
    }
    out << "]}";
    if (out_result_json) *out_result_json = out.str();
    return true;
  }
  if (tool_name == "hero.rollback") {
    std::string selector;
    if (extract_json_raw_field(request_json, "backup", nullptr)) {
      if (!extract_json_string_field(request_json, "backup", &selector)) {
        if (out_error_code) *out_error_code = -32602;
        if (out_error_message) {
          *out_error_message = "hero.rollback backup must be string";
        }
        return false;
      }
    }

    std::string selected;
    std::string err;
    if (!store->rollback_to_backup(selector, &selected, &err)) {
      if (out_error_code) *out_error_code = -32603;
      if (out_error_message) *out_error_message = err;
      return false;
    }
    if (out_result_json) {
      *out_result_json = "{\"rolled_back\":true,\"path\":" +
                         json_quote(store->config_path()) +
                         ",\"selected_backup\":" + json_quote(selected) + "}";
    }
    return true;
  }
  if (tool_name == "hero.save") {
    std::string err;
    if (!store->save(&err)) {
      if (out_error_code) *out_error_code = -32603;
      if (out_error_message) *out_error_message = err;
      return false;
    }
    if (out_result_json) {
      *out_result_json =
          "{\"saved\":true,\"path\":" + json_quote(store->config_path()) + "}";
    }
    return true;
  }
  if (tool_name == "hero.reload") {
    std::string err;
    if (!store->load(&err)) {
      if (out_error_code) *out_error_code = -32603;
      if (out_error_message) *out_error_message = err;
      return false;
    }
    if (out_result_json) {
      *out_result_json = "{\"reloaded\":true,\"path\":" +
                         json_quote(store->config_path()) + "}";
    }
    return true;
  }
  if (tool_name == "hero.ask" || tool_name == "hero.fix") {
    if (out_error_code) *out_error_code = -32601;
    if (out_error_message) *out_error_message = kDeterministicOnlyMessage;
    return false;
  }

  if (out_error_code) *out_error_code = -32601;
  if (out_error_message) *out_error_message = "unknown tool: " + tool_name;
  return false;
}

}  // namespace

namespace cuwacunu {
namespace hero {
namespace mcp {

void emit_startup_banner(const hero_config_store_t& store) {
  emit_ok("startup", "hero_config_mcp ready; run help for commands");
  emit_data("config_path", store.config_path());
  emit_data("from_template", store.from_template() ? "true" : "false");
  emit_end();
}

bool execute_command_line(std::string line, hero_config_store_t* store,
                          bool* should_exit) {
  line = trim_ascii(line);
  if (line.empty()) return true;

  std::string command;
  std::string args;
  {
    const std::size_t space = line.find_first_of(" \t");
    if (space == std::string::npos) {
      command = lowercase_copy(line);
    } else {
      command = lowercase_copy(line.substr(0, space));
      args = trim_ascii(line.substr(space + 1));
    }
  }

  if (command == "quit" || command == "exit") {
    emit_ok(command, "bye");
    emit_end();
    if (should_exit) *should_exit = true;
    return true;
  }
  if (command == "help") {
    print_help();
    return true;
  }
  if (command == "ping") {
    emit_ok(command, "pong");
    emit_end();
    return true;
  }
  if (command == "status") {
    const auto errors = store->validate();
    emit_ok(command, errors.empty() ? "valid" : "invalid");
    emit_data("config_path", store->config_path());
    emit_data("dirty", store->dirty() ? "true" : "false");
    emit_data("from_template", store->from_template() ? "true" : "false");
    emit_data("protocol_layer", store->get_or_default("protocol_layer"));
    emit_data("backup_enabled", store->get_or_default("backup_enabled"));
    emit_data("backup_dir", store->get_or_default("backup_dir"));
    emit_data("backup_max_entries", store->get_or_default("backup_max_entries"));
    emit_data("error_count", std::to_string(errors.size()));
    emit_end();
    return true;
  }
  if (command == "profiles") {
    emit_ok(command, "profile manifest");
    for (const auto& p : cuwacunu::hero::config::kProfileDescriptors) {
      emit_data(std::string("profile.") + std::string(p.profile_name),
                std::string("model=") + std::string(p.model) + ",summary=" +
                    std::string(p.summary));
    }
    emit_end();
    return true;
  }
  if (command == "targets") {
    emit_ok(command, "future learning targets");
    for (const auto& t : cuwacunu::hero::config::kFutureLearningTargets) {
      emit_data(std::string(t.include_path), std::string(t.summary));
    }
    emit_end();
    return true;
  }
  if (command == "schema") {
    emit_ok(command, "runtime key schema");
    for (const auto& d : cuwacunu::hero::config::kRuntimeKeyDescriptors) {
      std::ostringstream row;
      row << "type=" << d.type << ",required=" << (d.required ? "true" : "false")
          << ",default=" << d.default_value << ",range=" << d.range
          << ",allowed=" << d.allowed;
      emit_data(d.key, row.str());
    }
    emit_end();
    return true;
  }
  if (command == "show") {
    emit_ok(command, "current values");
    for (const auto& e : store->entries_snapshot()) {
      emit_data(e.key, e.value + " (type=" + e.declared_type + ")");
    }
    emit_end();
    return true;
  }
  if (command == "get") {
    if (args.empty()) {
      emit_err(command, "usage: get <key>");
      emit_end();
      return false;
    }
    const auto value = store->get_value(args);
    if (!value.has_value()) {
      emit_err(command, "key not found: " + args);
      emit_end();
      return false;
    }
    emit_ok(command, "value");
    emit_data(args, *value);
    emit_end();
    return true;
  }
  if (command == "set") {
    const std::size_t split = args.find_first_of(" \t");
    if (args.empty() || split == std::string::npos) {
      emit_err(command, "usage: set <key> <value>");
      emit_end();
      return false;
    }
    const std::string key = trim_ascii(args.substr(0, split));
    std::string value = trim_ascii(args.substr(split + 1));
    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))) {
      value = value.substr(1, value.size() - 2);
    }
    std::string err;
    if (!store->set_value(key, value, &err)) {
      emit_err(command, err);
      emit_end();
      return false;
    }
    emit_ok(command, "updated");
    emit_data(key, value);
    emit_end();
    return true;
  }
  if (command == "validate") {
    const auto errors = store->validate();
    if (errors.empty()) {
      emit_ok(command, "valid");
    } else {
      emit_err(command, "invalid");
      for (const auto& e : errors) emit_data("error", e);
    }
    emit_end();
    return errors.empty();
  }
  if (command == "diff" || command == "dry_run") {
    hero_config_store_t::save_preview_t preview;
    std::string err;
    if (!store->preview_save(&preview, &err)) {
      emit_err(command, err);
      emit_end();
      return false;
    }
    const bool format_only = preview.text_changed && preview.diffs.empty();
    if (!preview.has_changes) {
      emit_ok(command, "no_changes");
    } else if (format_only) {
      emit_ok(command, "format_changes_only");
    } else {
      emit_ok(command, "changes_detected");
    }
    emit_data("file_exists", preview.file_exists ? "true" : "false");
    emit_data("text_changed", preview.text_changed ? "true" : "false");
    emit_data("format_only", format_only ? "true" : "false");
    emit_data("diff_count", std::to_string(preview.diff_count));
    for (const auto& d : preview.diffs) {
      std::ostringstream row;
      row << "action=" << d.action << ",before_type=" << d.before_declared_type
          << ",before_value=" << d.before_value
          << ",after_type=" << d.after_declared_type
          << ",after_value=" << d.after_value;
      emit_data(std::string("diff.") + d.key, row.str());
    }
    emit_end();
    return true;
  }
  if (command == "backups") {
    std::vector<hero_config_store_t::backup_entry_t> backups;
    std::string err;
    if (!store->list_backups(&backups, &err)) {
      emit_err(command, err);
      emit_end();
      return false;
    }
    emit_ok(command, backups.empty() ? "no_backups" : "backup_list");
    emit_data("count", std::to_string(backups.size()));
    for (std::size_t i = 0; i < backups.size(); ++i) {
      emit_data(std::string("backup.") + std::to_string(i),
                backups[i].filename + " -> " + backups[i].path);
    }
    emit_end();
    return true;
  }
  if (command == "rollback") {
    const std::string selector = trim_ascii(args);
    std::string selected;
    std::string err;
    if (!store->rollback_to_backup(selector, &selected, &err)) {
      emit_err(command, err);
      emit_end();
      return false;
    }
    emit_ok(command, "rolled_back");
    emit_data("path", store->config_path());
    emit_data("selected_backup", selected);
    emit_end();
    return true;
  }
  if (command == "save") {
    std::string err;
    if (!store->save(&err)) {
      emit_err(command, err);
      emit_end();
      return false;
    }
    emit_ok(command, "saved");
    emit_data("path", store->config_path());
    emit_end();
    return true;
  }
  if (command == "reload") {
    std::string err;
    if (!store->load(&err)) {
      emit_err(command, err);
      emit_end();
      return false;
    }
    emit_ok(command, "reloaded");
    emit_data("path", store->config_path());
    emit_end();
    return true;
  }
  if (command == "ask" || command == "fix") {
    emit_err(command, kDeterministicOnlyMessage);
    emit_end();
    return true;
  }

  emit_err(command, "unknown command; run help");
  emit_end();
  return false;
}

void run_jsonrpc_stdio_loop(hero_config_store_t* store) {
  std::string payload;
  while (true) {
    bool used_content_length = false;
    if (!read_next_jsonrpc_message(&std::cin, &payload,
                                   &used_content_length)) {
      break;
    }
    g_jsonrpc_use_content_length_framing = used_content_length;

    const std::string trimmed = trim_ascii(payload);
    if (trimmed.empty()) continue;

    const bool has_id = has_json_field(trimmed, "id");

    std::string id_json;
    if (!extract_json_id_field(trimmed, &id_json)) {
      if (has_id) {
        emit_jsonrpc_error("null", -32700, "invalid request: unable to parse id");
      }
      continue;
    }

    std::string method;
    if (!extract_json_string_field(trimmed, "method", &method) ||
        method.empty()) {
      if (has_id) {
        emit_jsonrpc_error(id_json, -32600, "invalid request: missing method");
      }
      continue;
    }

    if (method.rfind("notifications/", 0) == 0) {
      continue;
    }

    if (method == "initialize") {
      std::string protocol_version = kDefaultMcpProtocolVersion;
      std::string params_json;
      if (extract_json_object_field(trimmed, "params", &params_json)) {
        std::string protocol_candidate;
        if (extract_json_string_field(params_json, "protocolVersion",
                                      &protocol_candidate) &&
            !protocol_candidate.empty()) {
          protocol_version = protocol_candidate;
        }
      } else {
        // Compatibility path for legacy initialize shape.
        std::string protocol_candidate;
        if (extract_json_string_field(trimmed, "protocolVersion",
                                      &protocol_candidate) &&
            !protocol_candidate.empty()) {
          protocol_version = protocol_candidate;
        }
      }
      if (has_id) {
        emit_jsonrpc_result(
            id_json, std::string("{\"protocolVersion\":") +
                         json_quote(protocol_version) +
                         ",\"capabilities\":{\"tools\":{\"listChanged\":false}},"
                         "\"serverInfo\":{\"name\":" +
                         json_quote(kMcpServerName) +
                         ",\"version\":" + json_quote(kMcpServerVersion) + "}}");
      }
      continue;
    }
    if (method == "ping") {
      if (has_id) {
        emit_jsonrpc_result(id_json, "{}");
      }
      continue;
    }
    if (method == "tools/list") {
      if (has_id) {
        emit_jsonrpc_result(id_json, build_tools_list_result());
      }
      continue;
    }

    if (method == "tools/call" || method.rfind("hero.", 0) == 0) {
      std::string tool_name = method;
      std::string dispatch_json = trimmed;
      if (method == "tools/call") {
        std::string params_json;
        if (!extract_json_object_field(trimmed, "params", &params_json)) {
          if (has_id) {
            emit_jsonrpc_error(id_json, -32602,
                               "tools/call requires params object");
          }
          continue;
        }
        if (!extract_json_string_field(params_json, "name", &tool_name) ||
            tool_name.empty()) {
          if (has_id) {
            emit_jsonrpc_error(id_json, -32602,
                               "tools/call requires params.name");
          }
          continue;
        }

        std::string arguments_json;
        if (extract_json_raw_field(params_json, "arguments", &arguments_json)) {
          arguments_json = trim_ascii(arguments_json);
          if (arguments_json.empty() || arguments_json.front() != '{') {
            if (has_id) {
              emit_jsonrpc_error(id_json, -32602,
                                 "tools/call requires params.arguments object");
            }
            continue;
          }
          dispatch_json = arguments_json;
        } else {
          dispatch_json = "{}";
        }
      } else {
        std::string params_json;
        if (extract_json_object_field(trimmed, "params", &params_json)) {
          dispatch_json = params_json;
        }
      }

      std::string result_json;
      int error_code = -32603;
      std::string error_message;
      if (!dispatch_tool_jsonrpc(tool_name, dispatch_json, store, &result_json,
                                 &error_code, &error_message)) {
        if (has_id) {
          if (method == "tools/call") {
            emit_jsonrpc_result(
                id_json,
                make_text_content_result_json(
                    std::string("tool error: ") + error_message, true));
          } else {
            emit_jsonrpc_error(id_json, error_code, error_message);
          }
        }
        continue;
      }
      if (has_id) {
        if (method == "tools/call") {
          emit_jsonrpc_result(
              id_json, make_tool_success_result_json(tool_name, result_json));
        } else {
          emit_jsonrpc_result(id_json, result_json);
        }
      }
      continue;
    }

    if (has_id) {
      emit_jsonrpc_error(id_json, -32601, "method not found: " + method);
    }
  }
}

}  // namespace mcp
}  // namespace hero
}  // namespace cuwacunu
