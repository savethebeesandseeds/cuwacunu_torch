#include "hero/jkimyei_hero/hero_jkimyei_tools.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "camahjucunu/dsl/jkimyei_campaign/jkimyei_campaign.h"
#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "hero/jkimyei_hero/jkimyei_campaign_runtime.h"
#include "hero/runtime_hero/runtime_job.h"

namespace {

constexpr const char* kDefaultJkimyeiHeroDslPath =
    "/cuwacunu/src/config/instructions/default.hero.jkimyei.dsl";
constexpr const char* kDefaultJkimyeiCampaignGrammarPath =
    "/cuwacunu/src/config/bnf/jkimyei_campaign.bnf";
constexpr const char* kServerName = "hero_jkimyei_mcp";
constexpr const char* kServerVersion = "0.1.0";
constexpr const char* kProtocolVersion = "2025-03-26";
constexpr const char* kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.jkimyei.*.";
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;

bool g_jsonrpc_use_content_length_framing = false;

using app_context_t = cuwacunu::hero::jkimyei_mcp::app_context_t;
using jkimyei_runtime_defaults_t =
    cuwacunu::hero::jkimyei_mcp::jkimyei_runtime_defaults_t;
using jkimyei_campaign_record_t =
    cuwacunu::hero::jkimyei::jkimyei_campaign_record_t;

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

[[nodiscard]] bool skip_json_compound(const std::string& json, std::size_t pos,
                                      std::size_t* out_end, char open_c,
                                      char close_c) {
  if (pos >= json.size() || json[pos] != open_c) return false;
  int depth = 0;
  while (pos < json.size()) {
    const char c = json[pos];
    if (c == '"') {
      std::size_t end = 0;
      if (!parse_json_string_at(json, pos, nullptr, &end)) return false;
      pos = end;
      continue;
    }
    if (c == open_c) ++depth;
    if (c == close_c) {
      --depth;
      ++pos;
      if (depth == 0) {
        if (out_end) *out_end = pos;
        return true;
      }
      continue;
    }
    ++pos;
  }
  return false;
}

[[nodiscard]] bool extract_json_raw_field(const std::string& json,
                                          const std::string& key,
                                          std::string* out) {
  if (out) out->clear();
  std::size_t pos = skip_json_whitespace(json, 0);
  if (pos >= json.size() || json[pos] != '{') return false;
  ++pos;
  while (true) {
    pos = skip_json_whitespace(json, pos);
    if (pos >= json.size()) return false;
    if (json[pos] == '}') return false;
    std::string parsed_key;
    std::size_t after_key = 0;
    if (!parse_json_string_at(json, pos, &parsed_key, &after_key)) return false;
    pos = skip_json_whitespace(json, after_key);
    if (pos >= json.size() || json[pos] != ':') return false;
    ++pos;
    pos = skip_json_whitespace(json, pos);
    if (pos >= json.size()) return false;

    std::size_t value_end = pos;
    if (json[pos] == '"') {
      if (!parse_json_string_at(json, pos, nullptr, &value_end)) return false;
    } else if (json[pos] == '{') {
      if (!skip_json_compound(json, pos, &value_end, '{', '}')) return false;
    } else if (json[pos] == '[') {
      if (!skip_json_compound(json, pos, &value_end, '[', ']')) return false;
    } else {
      while (value_end < json.size() && json[value_end] != ',' &&
             json[value_end] != '}') {
        ++value_end;
      }
    }

    if (parsed_key == key) {
      if (out) *out = json.substr(pos, value_end - pos);
      return true;
    }

    pos = skip_json_whitespace(json, value_end);
    if (pos < json.size() && json[pos] == ',') {
      ++pos;
      continue;
    }
    if (pos < json.size() && json[pos] == '}') return false;
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

  std::size_t pos = 0;
  if (raw_id[pos] == '-') ++pos;
  bool saw_digit = false;
  while (pos < raw_id.size() &&
         std::isdigit(static_cast<unsigned char>(raw_id[pos])) != 0) {
    saw_digit = true;
    ++pos;
  }
  pos = skip_json_whitespace(raw_id, pos);
  if (!saw_digit || pos != raw_id.size()) return false;
  if (out_id_json) *out_id_json = trim_ascii(raw_id);
  return true;
}

[[nodiscard]] bool read_next_jsonrpc_message(std::istream* in,
                                             std::string* out_json,
                                             bool* out_used_content_length) {
  if (out_json) out_json->clear();
  if (out_used_content_length) *out_used_content_length = false;
  if (!in) return false;

  if (!g_jsonrpc_use_content_length_framing) {
    std::string line;
    while (std::getline(*in, line)) {
      const std::string trimmed = trim_ascii(line);
      if (trimmed.empty()) continue;
      if (out_json) *out_json = trimmed;
      return true;
    }
    return false;
  }

  std::string header_line;
  std::size_t content_length = 0;
  bool saw_content_length = false;
  while (std::getline(*in, header_line)) {
    if (!header_line.empty() && header_line.back() == '\r') {
      header_line.pop_back();
    }
    if (header_line.empty()) break;
    constexpr std::string_view kContentLength = "Content-Length:";
    if (header_line.rfind(kContentLength, 0) == 0) {
      const std::string value = trim_ascii(header_line.substr(kContentLength.size()));
      std::uint64_t parsed = 0;
      const auto [ptr, ec] =
          std::from_chars(value.data(), value.data() + value.size(), parsed);
      if (ec == std::errc{} && ptr == value.data() + value.size()) {
        content_length = static_cast<std::size_t>(parsed);
        saw_content_length = true;
      }
    }
  }

  if (!saw_content_length || content_length == 0 ||
      content_length > kMaxJsonRpcPayloadBytes) {
    return false;
  }

  std::string payload(content_length, '\0');
  in->read(payload.data(), static_cast<std::streamsize>(payload.size()));
  if (static_cast<std::size_t>(in->gcount()) != payload.size()) return false;
  if (out_json) *out_json = payload;
  if (out_used_content_length) *out_used_content_length = true;
  return true;
}

void write_jsonrpc_message(std::string_view payload_json) {
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
  std::ostringstream out;
  out << "{\"jsonrpc\":\"2.0\",\"id\":" << id_json
      << ",\"result\":" << result_json << "}";
  write_jsonrpc_message(out.str());
}

void write_jsonrpc_error(std::string_view id_json, int code,
                         std::string_view message) {
  std::ostringstream out;
  out << "{\"jsonrpc\":\"2.0\",\"id\":" << id_json
      << ",\"error\":{\"code\":" << code
      << ",\"message\":" << json_quote(message) << "}}";
  write_jsonrpc_message(out.str());
}

[[nodiscard]] std::string tool_result_json_for_error(std::string_view message) {
  return make_tool_result_json(
      message, "{\"ok\":false,\"error\":" + json_quote(message) + "}", true);
}

using jkimyei_tool_handler_t = bool (*)(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error);

struct jkimyei_tool_descriptor_t {
  std::string_view name;
  std::string_view description;
  std::string_view input_schema_json;
  jkimyei_tool_handler_t handler;
};

[[nodiscard]] bool handle_tool_create_campaign(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error);
[[nodiscard]] bool handle_tool_list_campaigns(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error);
[[nodiscard]] bool handle_tool_get_campaign(app_context_t* app,
                                            const std::string& arguments_json,
                                            std::string* out_structured,
                                            std::string* out_error);

constexpr jkimyei_tool_descriptor_t kJkimyeiTools[] = {
#define HERO_JKIMYEI_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER) \
  jkimyei_tool_descriptor_t{NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
#include "hero/jkimyei_hero/hero_jkimyei_tools.def"
#undef HERO_JKIMYEI_TOOL
};

[[nodiscard]] const jkimyei_tool_descriptor_t* find_jkimyei_tool_descriptor(
    std::string_view name) {
  for (const auto& descriptor : kJkimyeiTools) {
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

[[nodiscard]] std::string string_vector_json(
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

[[nodiscard]] std::string campaign_record_to_json(
    const jkimyei_campaign_record_t& record) {
  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(record.schema) << ","
      << "\"campaign_cursor\":" << json_quote(record.campaign_cursor) << ","
      << "\"state\":" << json_quote(record.state) << ","
      << "\"state_detail\":" << json_quote(record.state_detail) << ","
      << "\"config_folder\":" << json_quote(record.config_folder) << ","
      << "\"source_campaign_dsl_path\":"
      << json_quote(record.source_campaign_dsl_path) << ","
      << "\"campaign_dsl_path\":" << json_quote(record.campaign_dsl_path) << ","
      << "\"requested_campaign_id\":"
      << json_quote(record.requested_campaign_id) << ","
      << "\"active_campaign_id\":" << json_quote(record.active_campaign_id) << ","
      << "\"resolved_campaign_id\":"
      << json_quote(record.resolved_campaign_id) << ","
      << "\"started_at_ms\":" << record.started_at_ms << ","
      << "\"updated_at_ms\":" << record.updated_at_ms << ","
      << "\"finished_at_ms\":";
  if (record.finished_at_ms.has_value()) {
    out << *record.finished_at_ms;
  } else {
    out << "null";
  }
  out << ",\"bind_ids\":" << string_vector_json(record.bind_ids)
      << ",\"bind_count\":" << record.bind_ids.size() << "}";
  return out.str();
}

[[nodiscard]] const cuwacunu::camahjucunu::jkimyei_campaign_decl_t*
find_campaign_decl(
    const cuwacunu::camahjucunu::jkimyei_campaign_instruction_t& instruction,
    std::string_view campaign_id) {
  for (const auto& campaign : instruction.campaigns) {
    if (campaign.id == campaign_id) return &campaign;
  }
  return nullptr;
}

[[nodiscard]] std::filesystem::path resolve_jkimyei_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "REAL_HERO", "jkimyei_hero_dsl_filename");
  if (!configured.has_value()) {
    return std::filesystem::path(kDefaultJkimyeiHeroDslPath);
  }
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) {
    return std::filesystem::path(kDefaultJkimyeiHeroDslPath);
  }
  return std::filesystem::path(resolved);
}

[[nodiscard]] bool load_jkimyei_runtime_defaults(
    const std::filesystem::path& hero_dsl_path, jkimyei_runtime_defaults_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "jkimyei defaults output pointer is null";
    return false;
  }
  *out = jkimyei_runtime_defaults_t{};

  std::ifstream in(hero_dsl_path);
  if (!in) {
    if (error) {
      *error = "cannot open jkimyei HERO defaults DSL: " + hero_dsl_path.string();
    }
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

  if (const auto it = values.find("campaigns_root"); it != values.end()) {
    out->campaigns_root = resolve_local_path(it->second);
  }
  if (const auto it = values.find("config_folder"); it != values.end()) {
    out->config_folder = resolve_local_path(it->second);
  }
  if (const auto it = values.find("campaign_grammar_filename");
      it != values.end()) {
    out->campaign_grammar_path = resolve_local_path(it->second);
  }

  if (out->campaigns_root.empty()) {
    out->campaigns_root = "/cuwacunu/.runtime/jkimyei_campaigns";
  }
  if (out->config_folder.empty()) {
    out->config_folder = "/cuwacunu/src/config";
  }
  if (out->campaign_grammar_path.empty()) {
    out->campaign_grammar_path = kDefaultJkimyeiCampaignGrammarPath;
  }
  return true;
}

[[nodiscard]] bool handle_tool_create_campaign(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string requested_campaign_id{};
  std::string config_folder = app->defaults.config_folder.string();
  (void)extract_json_string_field(arguments_json, "campaign_id",
                                  &requested_campaign_id);
  (void)extract_json_string_field(arguments_json, "config_folder",
                                  &config_folder);
  requested_campaign_id = trim_ascii(requested_campaign_id);
  config_folder = trim_ascii(config_folder);
  if (config_folder.empty()) {
    *out_error = "jkimyei config_folder is empty";
    return false;
  }

  const std::string campaign_cursor =
      cuwacunu::hero::jkimyei::make_campaign_cursor(app->defaults.campaigns_root);

  std::string source_campaign_path{};
  std::string campaign_snapshot_path{};
  if (!cuwacunu::hero::jkimyei::prepare_campaign_dsl_snapshot(
          app->defaults.campaigns_root, campaign_cursor,
          std::filesystem::path(config_folder), &source_campaign_path,
          &campaign_snapshot_path, out_error)) {
    return false;
  }

  std::string grammar_text{};
  if (!cuwacunu::hero::runtime::read_text_file(app->defaults.campaign_grammar_path,
                                               &grammar_text, out_error)) {
    return false;
  }
  std::string campaign_text{};
  if (!cuwacunu::hero::runtime::read_text_file(campaign_snapshot_path,
                                               &campaign_text, out_error)) {
    return false;
  }

  cuwacunu::camahjucunu::jkimyei_campaign_instruction_t instruction{};
  try {
    instruction = cuwacunu::camahjucunu::dsl::decode_jkimyei_campaign_from_dsl(
        grammar_text, campaign_text);
  } catch (const std::exception& e) {
    *out_error = "failed to decode campaign DSL snapshot: " +
                 std::string(e.what());
    return false;
  }

  const std::string resolved_campaign_id =
      requested_campaign_id.empty() ? instruction.active_campaign_id
                                    : requested_campaign_id;
  const auto* campaign_decl = find_campaign_decl(instruction, resolved_campaign_id);
  if (campaign_decl == nullptr) {
    *out_error = "campaign id not found in snapshot: " + resolved_campaign_id;
    return false;
  }

  jkimyei_campaign_record_t record{};
  record.campaign_cursor = campaign_cursor;
  record.state = "staged";
  record.state_detail = "campaign snapshot created";
  record.config_folder = std::filesystem::path(config_folder).lexically_normal().string();
  record.source_campaign_dsl_path = source_campaign_path;
  record.campaign_dsl_path = campaign_snapshot_path;
  record.requested_campaign_id = requested_campaign_id;
  record.active_campaign_id = instruction.active_campaign_id;
  record.resolved_campaign_id = resolved_campaign_id;
  record.started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  record.updated_at_ms = record.started_at_ms;
  record.bind_ids = campaign_decl->bind_refs;

  if (!cuwacunu::hero::jkimyei::write_jkimyei_campaign_record(
          app->defaults.campaigns_root, record, out_error)) {
    return false;
  }

  *out_structured =
      "{\"campaign_cursor\":" + json_quote(record.campaign_cursor) +
      ",\"campaign\":" + campaign_record_to_json(record) + "}";
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

  std::vector<jkimyei_campaign_record_t> campaigns{};
  if (!cuwacunu::hero::jkimyei::list_jkimyei_campaign_records(
          app->defaults.campaigns_root, &campaigns, out_error)) {
    return false;
  }

  std::vector<jkimyei_campaign_record_t> filtered{};
  filtered.reserve(campaigns.size());
  for (auto& record : campaigns) {
    if (!state_filter.empty() && record.state != state_filter) continue;
    filtered.push_back(std::move(record));
  }

  std::sort(filtered.begin(), filtered.end(),
            [newest_first](const auto& lhs, const auto& rhs) {
              if (lhs.started_at_ms != rhs.started_at_ms) {
                return newest_first ? (lhs.started_at_ms > rhs.started_at_ms)
                                    : (lhs.started_at_ms < rhs.started_at_ms);
              }
              return newest_first ? (lhs.campaign_cursor > rhs.campaign_cursor)
                                  : (lhs.campaign_cursor < rhs.campaign_cursor);
            });

  const std::size_t total = filtered.size();
  const std::size_t off = std::min(offset, filtered.size());
  std::size_t count = limit;
  if (count == 0) count = filtered.size() - off;
  count = std::min(count, filtered.size() - off);

  std::ostringstream campaigns_json;
  campaigns_json << "[";
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0) campaigns_json << ",";
    campaigns_json
        << campaign_record_to_json(filtered[off + i]);
  }
  campaigns_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"campaigns_root\":"
      << json_quote(app->defaults.campaigns_root.string()) << ","
      << "\"total\":" << total << ","
      << "\"offset\":" << off << ","
      << "\"count\":" << count << ","
      << "\"campaigns\":" << campaigns_json.str() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_campaign(app_context_t* app,
                                            const std::string& arguments_json,
                                            std::string* out_structured,
                                            std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string campaign_cursor{};
  (void)extract_json_string_field(arguments_json, "campaign_cursor",
                                  &campaign_cursor);
  campaign_cursor = trim_ascii(campaign_cursor);
  if (campaign_cursor.empty()) {
    *out_error = "campaign_cursor is required";
    return false;
  }

  jkimyei_campaign_record_t record{};
  if (!cuwacunu::hero::jkimyei::read_jkimyei_campaign_record(
          app->defaults.campaigns_root, campaign_cursor, &record, out_error)) {
    return false;
  }

  *out_structured =
      "{\"campaign_cursor\":" + json_quote(campaign_cursor) +
      ",\"campaign\":" + campaign_record_to_json(record) + "}";
  return true;
}

}  // namespace

namespace cuwacunu {
namespace hero {
namespace jkimyei_mcp {

std::filesystem::path resolve_jkimyei_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  return ::resolve_jkimyei_hero_dsl_path(global_config_path);
}

bool load_jkimyei_runtime_defaults(const std::filesystem::path& hero_dsl_path,
                                   jkimyei_runtime_defaults_t* out,
                                   std::string* error) {
  return ::load_jkimyei_runtime_defaults(hero_dsl_path, out, error);
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

  const auto* descriptor = find_jkimyei_tool_descriptor(tool_name);
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

bool tool_result_is_error(std::string_view tool_result_json) {
  bool is_error = false;
  return extract_json_bool_field(std::string(tool_result_json), "isError",
                                 &is_error) &&
         is_error;
}

std::string build_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  const std::size_t tool_count =
      sizeof(kJkimyeiTools) / sizeof(kJkimyeiTools[0]);
  for (std::size_t i = 0; i < tool_count; ++i) {
    const auto& tool = kJkimyeiTools[i];
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
  for (const auto& tool : kJkimyeiTools) {
    out << tool.name << "\t" << tool.description << "\n";
  }
  return out.str();
}

void run_jsonrpc_stdio_loop(app_context_t* app) {
  std::string request{};
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

    std::string method{};
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

    std::string params{};
    if (!extract_json_object_field(request, "params", &params)) {
      write_jsonrpc_error(id_json, -32602, "tools/call requires params object");
      continue;
    }

    std::string name{};
    if (!extract_json_string_field(params, "name", &name) || name.empty()) {
      write_jsonrpc_error(id_json, -32602, "tools/call requires params.name");
      continue;
    }
    std::string arguments = "{}";
    std::string extracted_args{};
    if (extract_json_object_field(params, "arguments", &extracted_args)) {
      arguments = extracted_args;
    }

    std::string tool_result{};
    std::string tool_error{};
    if (!execute_tool_json(name, arguments, app, &tool_result, &tool_error)) {
      write_jsonrpc_error(id_json, -32601, tool_error);
      continue;
    }
    write_jsonrpc_result(id_json, tool_result);
  }
}

}  // namespace jkimyei_mcp
}  // namespace hero
}  // namespace cuwacunu
