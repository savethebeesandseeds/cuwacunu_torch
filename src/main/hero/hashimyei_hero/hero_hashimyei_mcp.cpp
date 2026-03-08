#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <openssl/evp.h>

#include "HERO/hashimyei/hashimyei_catalog.h"
#include "piaabo/dlogs.h"

namespace {

constexpr const char* kDefaultPassEnv = "CUWACUNU_HASHIMYEI_META_SECRET";
constexpr const char* kStoreRootEnv = "CUWACUNU_HASHIMYEI_STORE_ROOT";
constexpr const char* kDefaultStoreRoot = "/cuwacunu/.hashimyei";
constexpr const char* kServerName = "hero_hashimyei_mcp";
constexpr const char* kServerVersion = "0.1.0";
constexpr const char* kProtocolVersion = "2025-03-26";

bool g_jsonrpc_use_content_length_framing = false;

struct app_context_t {
  std::filesystem::path store_root{};
  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t catalog{};
};

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  std::size_t b = 0;
  while (b < in.size() && std::isspace(static_cast<unsigned char>(in[b])) != 0) {
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

[[nodiscard]] std::string hex_lower(const unsigned char* bytes, std::size_t n) {
  static constexpr std::array<char, 16> kHex{
      '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  std::string out;
  out.resize(n * 2);
  for (std::size_t i = 0; i < n; ++i) {
    const unsigned char b = bytes[i];
    out[2 * i + 0] = kHex[(b >> 4) & 0x0F];
    out[2 * i + 1] = kHex[b & 0x0F];
  }
  return out;
}

[[nodiscard]] bool sha256_hex_bytes(std::string_view payload, std::string* out_hex) {
  if (!out_hex) return false;
  out_hex->clear();
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) return false;

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  const bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
                  EVP_DigestUpdate(ctx, payload.data(), payload.size()) == 1 &&
                  EVP_DigestFinal_ex(ctx, digest, &digest_len) == 1;
  EVP_MD_CTX_free(ctx);
  if (!ok) return false;
  *out_hex = hex_lower(digest, static_cast<std::size_t>(digest_len));
  return true;
}

[[nodiscard]] bool read_text_file(const std::filesystem::path& path,
                                  std::string* out,
                                  std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "output pointer is null";
    return false;
  }
  out->clear();
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error) *error = "cannot open file: " + path.string();
    return false;
  }
  out->assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  if (!in.eof() && in.fail()) {
    if (error) *error = "cannot read file: " + path.string();
    return false;
  }
  return true;
}

[[nodiscard]] std::optional<std::filesystem::path> resolve_dsl_path(
    const app_context_t& app, std::string_view dsl_canonical_path) {
  namespace fs = std::filesystem;
  const fs::path input = fs::path(std::string(dsl_canonical_path));
  if (dsl_canonical_path.empty()) return std::nullopt;

  std::vector<fs::path> candidates{};
  candidates.reserve(6);
  if (input.is_absolute()) {
    candidates.push_back(input);
  } else {
    std::error_code ec;
    candidates.push_back(fs::current_path(ec) / input);
    candidates.push_back(app.store_root / input);
    if (!app.store_root.empty()) {
      const auto parent = app.store_root.parent_path();
      if (!parent.empty()) {
        candidates.push_back(parent / input);
        const auto grand = parent.parent_path();
        if (!grand.empty()) candidates.push_back(grand / input);
      }
    }
  }

  std::unordered_set<std::string> seen{};
  for (const auto& c : candidates) {
    const std::string key = c.generic_string();
    if (seen.find(key) != seen.end()) continue;
    seen.insert(key);
    std::error_code ec;
    if (!fs::exists(c, ec) || !fs::is_regular_file(c, ec)) continue;
    const auto canonical = fs::weakly_canonical(c, ec);
    if (!ec) return canonical;
    return c;
  }
  return std::nullopt;
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
          const char* hex = "0123456789abcdef";
          out << "\\u00";
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

[[nodiscard]] std::filesystem::path default_store_root() {
  if (const char* env = std::getenv(kStoreRootEnv); env && env[0] != '\0') {
    return std::filesystem::path(env);
  }
  return std::filesystem::path(kDefaultStoreRoot);
}

[[nodiscard]] std::filesystem::path default_catalog_path(
    const std::filesystem::path& store_root) {
  return store_root / "catalog" / "hashimyei_catalog.idydb";
}

void write_jsonrpc_payload(std::string_view payload) {
  if (g_jsonrpc_use_content_length_framing) {
    std::cout << "Content-Length: " << payload.size() << "\r\n\r\n" << payload;
  } else {
    std::cout << payload << "\n";
  }
  std::cout.flush();
}

void write_jsonrpc_result(std::string_view id_json, std::string_view result_json) {
  std::ostringstream out;
  out << "{\"jsonrpc\":\"2.0\",\"id\":" << id_json
      << ",\"result\":" << result_json << "}";
  write_jsonrpc_payload(out.str());
}

void write_jsonrpc_error(std::string_view id_json, int code, std::string_view message) {
  std::ostringstream out;
  out << "{\"jsonrpc\":\"2.0\",\"id\":" << id_json << ",\"error\":{"
      << "\"code\":" << code << ",\"message\":" << json_quote(message) << "}}";
  write_jsonrpc_payload(out.str());
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

[[nodiscard]] bool extract_json_size_field(const std::string& json,
                                           const std::string& key,
                                           std::size_t* out) {
  if (!out) return false;
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  raw = trim_ascii(raw);
  if (raw.empty()) return false;
  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(raw.data(), raw.data() + raw.size(), parsed);
  if (ec != std::errc() || ptr != raw.data() + raw.size()) return false;
  *out = static_cast<std::size_t>(parsed);
  return true;
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
      if (content_length == 0) continue;

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

[[nodiscard]] std::string artifact_to_json(
    const cuwacunu::hero::hashimyei::artifact_entry_t& artifact) {
  std::ostringstream out;
  out << "{"
      << "\"artifact_id\":" << json_quote(artifact.artifact_id) << ","
      << "\"run_id\":" << json_quote(artifact.run_id) << ","
      << "\"canonical_path\":" << json_quote(artifact.canonical_path) << ","
      << "\"hashimyei\":" << json_quote(artifact.hashimyei) << ","
      << "\"schema\":" << json_quote(artifact.schema) << ","
      << "\"artifact_sha256\":" << json_quote(artifact.artifact_sha256) << ","
      << "\"path\":" << json_quote(artifact.path) << ","
      << "\"ts_ms\":" << artifact.ts_ms
      << "}";
  return out.str();
}

[[nodiscard]] std::string num_metrics_to_json(
    const std::vector<std::pair<std::string, double>>& metrics) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < metrics.size(); ++i) {
    if (i != 0) out << ",";
    out << "{\"key\":" << json_quote(metrics[i].first) << ",\"value\":"
        << metrics[i].second << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string txt_metrics_to_json(
    const std::vector<std::pair<std::string, std::string>>& metrics) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < metrics.size(); ++i) {
    if (i != 0) out << ",";
    out << "{\"key\":" << json_quote(metrics[i].first) << ",\"value\":"
        << json_quote(metrics[i].second) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string provenance_to_json(
    const std::vector<cuwacunu::hero::hashimyei::dependency_file_t>& deps) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < deps.size(); ++i) {
    if (i != 0) out << ",";
    out << "{\"canonical_path\":" << json_quote(deps[i].canonical_path)
        << ",\"sha256\":" << json_quote(deps[i].sha256_hex) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string component_state_to_json(
    const cuwacunu::hero::hashimyei::component_state_t& component) {
  const auto& m = component.manifest;
  std::ostringstream out;
  out << "{"
      << "\"component_id\":" << json_quote(component.component_id) << ","
      << "\"ts_ms\":" << component.ts_ms << ","
      << "\"manifest_path\":" << json_quote(component.manifest_path) << ","
      << "\"artifact_sha256\":" << json_quote(component.artifact_sha256) << ","
      << "\"manifest\":{"
      << "\"schema\":" << json_quote(m.schema) << ","
      << "\"canonical_path\":" << json_quote(m.canonical_path) << ","
      << "\"tsi_type\":" << json_quote(m.tsi_type) << ","
      << "\"hashimyei\":" << json_quote(m.hashimyei) << ","
      << "\"contract_hash\":" << json_quote(m.contract_hash) << ","
      << "\"wave_hash\":" << json_quote(m.wave_hash) << ","
      << "\"binding_id\":" << json_quote(m.binding_id) << ","
      << "\"dsl_canonical_path\":" << json_quote(m.dsl_canonical_path) << ","
      << "\"dsl_sha256_hex\":" << json_quote(m.dsl_sha256_hex) << ","
      << "\"status\":" << json_quote(m.status) << ","
      << "\"replaced_by\":" << json_quote(m.replaced_by) << ","
      << "\"created_at_ms\":" << m.created_at_ms << ","
      << "\"updated_at_ms\":" << m.updated_at_ms
      << "}"
      << "}";
  return out.str();
}

[[nodiscard]] std::string ensure_trailing_newline(std::string value) {
  if (!value.empty() && value.back() != '\n') value.push_back('\n');
  return value;
}

[[nodiscard]] std::string joined_kv_report(
    std::string_view canonical_path,
    const std::vector<cuwacunu::hero::hashimyei::artifact_entry_t>& artifacts) {
  std::ostringstream out;
  out << "# hashimyei.joined_report.v1\n";
  out << "report_schema=hashimyei.joined_report.v1\n";
  out << "canonical_path=" << canonical_path << "\n";
  out << "artifact_count=" << artifacts.size() << "\n";
  for (std::size_t i = 0; i < artifacts.size(); ++i) {
    const auto& a = artifacts[i];
    out << "\n# --- artifact[" << i << "] ---\n";
    out << "artifact_index=" << i << "\n";
    out << "artifact_id=" << a.artifact_id << "\n";
    out << "schema=" << a.schema << "\n";
    out << "run_id=" << a.run_id << "\n";
    out << "hashimyei=" << a.hashimyei << "\n";
    out << "ts_ms=" << a.ts_ms << "\n";
    out << "path=" << a.path << "\n";
    out << "artifact_sha256=" << a.artifact_sha256 << "\n";
    out << "payload_begin=1\n";
    out << ensure_trailing_newline(a.payload_json);
    out << "payload_end=1\n";
  }
  return out.str();
}

[[nodiscard]] std::string tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":["
      << "{\"name\":\"hero.hashimyei.scan\",\"description\":\"Ingest .hashimyei artifacts into catalog\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"store_root\":{\"type\":\"string\"},\"backfill_legacy\":{\"type\":\"boolean\"}}}},"
      << "{\"name\":\"hero.hashimyei.latest\",\"description\":\"Fetch latest artifact for canonical path\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"canonical_path\":{\"type\":\"string\"},\"schema\":{\"type\":\"string\"}},\"required\":[\"canonical_path\"]}},"
      << "{\"name\":\"hero.hashimyei.history\",\"description\":\"List artifact history for canonical path\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"canonical_path\":{\"type\":\"string\"},\"schema\":{\"type\":\"string\"},\"limit\":{\"type\":\"integer\"},\"offset\":{\"type\":\"integer\"}},\"required\":[\"canonical_path\"]}},"
      << "{\"name\":\"hero.hashimyei.performance\",\"description\":\"Fetch latest performance snapshot for canonical path\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"canonical_path\":{\"type\":\"string\"},\"run_id\":{\"type\":\"string\"}},\"required\":[\"canonical_path\"]}},"
      << "{\"name\":\"hero.hashimyei.provenance\",\"description\":\"Resolve provenance chain by artifact id\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"artifact_id\":{\"type\":\"string\"}},\"required\":[\"artifact_id\"]}},"
      << "{\"name\":\"hero.hashimyei.component\",\"description\":\"Resolve hashimyei component identity state (independent of runs)\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"canonical_path\":{\"type\":\"string\"},\"hashimyei\":{\"type\":\"string\"}},\"anyOf\":[{\"required\":[\"canonical_path\"]},{\"required\":[\"hashimyei\"]}]}},"
      << "{\"name\":\"hero.hashimyei.founding_dsl\",\"description\":\"Fetch founding component DSL for a hashimyei/canonical component\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"canonical_path\":{\"type\":\"string\"},\"hashimyei\":{\"type\":\"string\"},\"include_content\":{\"type\":\"boolean\"}},\"anyOf\":[{\"required\":[\"canonical_path\"]},{\"required\":[\"hashimyei\"]}]}},"
      << "{\"name\":\"hero.hashimyei.report\",\"description\":\"Join multiple .kv artifact reports with deterministic headers\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"canonical_path\":{\"type\":\"string\"},\"run_id\":{\"type\":\"string\"},\"schema\":{\"type\":\"string\"},\"limit\":{\"type\":\"integer\"},\"offset\":{\"type\":\"integer\"},\"newest_first\":{\"type\":\"boolean\"}},\"required\":[\"canonical_path\"]}}"
      << "]}";
  return out.str();
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

[[nodiscard]] bool handle_tool_scan(app_context_t* app,
                                    const std::string& arguments_json,
                                    std::string* out_structured,
                                    std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::filesystem::path root = app->store_root;
  std::string store_root_override;
  if (extract_json_string_field(arguments_json, "store_root", &store_root_override) &&
      !store_root_override.empty()) {
    root = store_root_override;
  }

  bool backfill_legacy = true;
  (void)extract_json_bool_field(arguments_json, "backfill_legacy", &backfill_legacy);

  if (!app->catalog.ingest_filesystem(root, backfill_legacy, out_error)) return false;

  std::vector<cuwacunu::hero::hashimyei::run_manifest_t> runs{};
  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> artifacts{};
  if (!app->catalog.list_runs_by_binding("", "", "", &runs, out_error)) return false;
  if (!app->catalog.list_artifacts("", "", 0, 0, true, &artifacts, out_error)) return false;

  std::ostringstream out;
  out << "{"
      << "\"canonical_path\":\"\","  // required field for all tool payloads
      << "\"store_root\":" << json_quote(root.string()) << ","
      << "\"run_count\":" << runs.size() << ","
      << "\"artifact_count\":" << artifacts.size() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_latest(app_context_t* app,
                                      const std::string& arguments_json,
                                      std::string* out_structured,
                                      std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string canonical_path;
  if (!extract_json_string_field(arguments_json, "canonical_path", &canonical_path) ||
      canonical_path.empty()) {
    *out_error = "latest requires arguments.canonical_path";
    return false;
  }

  std::string schema;
  (void)extract_json_string_field(arguments_json, "schema", &schema);

  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> list{};
  if (!app->catalog.list_artifacts(canonical_path, schema, 1, 0, true, &list,
                                   out_error)) {
    return false;
  }
  if (list.empty()) {
    *out_error = "no artifact found for canonical_path/schema";
    return false;
  }

  const auto& artifact = list.front();
  std::vector<std::pair<std::string, double>> num{};
  std::vector<std::pair<std::string, std::string>> txt{};
  if (!app->catalog.artifact_metrics(artifact.artifact_id, &num, &txt, out_error)) {
    return false;
  }

  std::ostringstream out;
  out << "{\"canonical_path\":" << json_quote(artifact.canonical_path)
      << ",\"artifact\":" << artifact_to_json(artifact)
      << ",\"metrics_num\":" << num_metrics_to_json(num)
      << ",\"metrics_txt\":" << txt_metrics_to_json(txt) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_history(app_context_t* app,
                                       const std::string& arguments_json,
                                       std::string* out_structured,
                                       std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string canonical_path;
  if (!extract_json_string_field(arguments_json, "canonical_path", &canonical_path) ||
      canonical_path.empty()) {
    *out_error = "history requires arguments.canonical_path";
    return false;
  }

  std::string schema;
  (void)extract_json_string_field(arguments_json, "schema", &schema);

  std::size_t limit = 20;
  std::size_t offset = 0;
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);

  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> list{};
  if (!app->catalog.list_artifacts(canonical_path, schema, limit, offset, true,
                                   &list, out_error)) {
    return false;
  }

  std::ostringstream history;
  history << "[";
  for (std::size_t i = 0; i < list.size(); ++i) {
    if (i != 0) history << ",";
    history << artifact_to_json(list[i]);
  }
  history << "]";

  std::ostringstream out;
  out << "{\"canonical_path\":" << json_quote(canonical_path)
      << ",\"count\":" << list.size() << ",\"history\":"
      << history.str() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_performance(app_context_t* app,
                                           const std::string& arguments_json,
                                           std::string* out_structured,
                                           std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string canonical_path;
  if (!extract_json_string_field(arguments_json, "canonical_path", &canonical_path) ||
      canonical_path.empty()) {
    *out_error = "performance requires arguments.canonical_path";
    return false;
  }

  std::string run_id;
  (void)extract_json_string_field(arguments_json, "run_id", &run_id);

  cuwacunu::hero::hashimyei::performance_snapshot_t snapshot{};
  if (!app->catalog.performance_snapshot(canonical_path, run_id, &snapshot,
                                         out_error)) {
    return false;
  }

  std::ostringstream out;
  out << "{\"canonical_path\":" << json_quote(snapshot.artifact.canonical_path)
      << ",\"artifact\":" << artifact_to_json(snapshot.artifact)
      << ",\"metrics_num\":" << num_metrics_to_json(snapshot.numeric_metrics)
      << ",\"metrics_txt\":" << txt_metrics_to_json(snapshot.text_metrics)
      << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_provenance(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string artifact_id;
  if (!extract_json_string_field(arguments_json, "artifact_id", &artifact_id) ||
      artifact_id.empty()) {
    *out_error = "provenance requires arguments.artifact_id";
    return false;
  }

  cuwacunu::hero::hashimyei::artifact_entry_t artifact{};
  if (!app->catalog.get_artifact(artifact_id, &artifact, out_error)) return false;

  std::vector<cuwacunu::hero::hashimyei::dependency_file_t> trace{};
  if (!app->catalog.provenance_trace(artifact_id, &trace, out_error)) return false;

  std::ostringstream out;
  out << "{\"canonical_path\":" << json_quote(artifact.canonical_path)
      << ",\"artifact\":" << artifact_to_json(artifact)
      << ",\"provenance\":" << provenance_to_json(trace) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_component(app_context_t* app,
                                         const std::string& arguments_json,
                                         std::string* out_structured,
                                         std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string canonical_path;
  std::string hashimyei;
  (void)extract_json_string_field(arguments_json, "canonical_path", &canonical_path);
  (void)extract_json_string_field(arguments_json, "hashimyei", &hashimyei);
  if (canonical_path.empty() && hashimyei.empty()) {
    *out_error = "component requires arguments.canonical_path or arguments.hashimyei";
    return false;
  }

  cuwacunu::hero::hashimyei::component_state_t component{};
  if (!app->catalog.resolve_component(canonical_path, hashimyei, &component,
                                      out_error)) {
    return false;
  }

  std::ostringstream out;
  out << "{\"canonical_path\":"
      << json_quote(component.manifest.canonical_path)
      << ",\"hashimyei\":" << json_quote(component.manifest.hashimyei)
      << ",\"component\":" << component_state_to_json(component) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_founding_dsl(app_context_t* app,
                                            const std::string& arguments_json,
                                            std::string* out_structured,
                                            std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string canonical_path;
  std::string hashimyei;
  (void)extract_json_string_field(arguments_json, "canonical_path", &canonical_path);
  (void)extract_json_string_field(arguments_json, "hashimyei", &hashimyei);
  if (canonical_path.empty() && hashimyei.empty()) {
    *out_error =
        "founding_dsl requires arguments.canonical_path or arguments.hashimyei";
    return false;
  }

  bool include_content = true;
  (void)extract_json_bool_field(arguments_json, "include_content", &include_content);

  cuwacunu::hero::hashimyei::component_state_t component{};
  if (!app->catalog.resolve_component(canonical_path, hashimyei, &component,
                                      out_error)) {
    return false;
  }

  const auto resolved =
      resolve_dsl_path(*app, component.manifest.dsl_canonical_path);
  if (!resolved.has_value()) {
    *out_error = "founding DSL file does not exist: " +
                 component.manifest.dsl_canonical_path;
    return false;
  }

  std::string dsl_text;
  if (!read_text_file(*resolved, &dsl_text, out_error)) return false;

  std::string dsl_sha_actual;
  if (!sha256_hex_bytes(dsl_text, &dsl_sha_actual)) {
    *out_error = "cannot compute sha256 for founding DSL";
    return false;
  }
  const bool dsl_sha_match =
      lowercase_copy(dsl_sha_actual) ==
      lowercase_copy(component.manifest.dsl_sha256_hex);

  std::ostringstream out;
  out << "{\"canonical_path\":"
      << json_quote(component.manifest.canonical_path)
      << ",\"hashimyei\":" << json_quote(component.manifest.hashimyei)
      << ",\"dsl_canonical_path\":"
      << json_quote(component.manifest.dsl_canonical_path)
      << ",\"dsl_resolved_path\":" << json_quote(resolved->string())
      << ",\"dsl_sha256_expected\":"
      << json_quote(component.manifest.dsl_sha256_hex)
      << ",\"dsl_sha256_actual\":" << json_quote(dsl_sha_actual)
      << ",\"dsl_sha256_match\":" << (dsl_sha_match ? "true" : "false")
      << ",\"component\":" << component_state_to_json(component);
  if (include_content) {
    out << ",\"dsl_text\":" << json_quote(dsl_text);
  }
  out << "}";

  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_report(app_context_t* app,
                                      const std::string& arguments_json,
                                      std::string* out_structured,
                                      std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string canonical_path;
  if (!extract_json_string_field(arguments_json, "canonical_path", &canonical_path) ||
      canonical_path.empty()) {
    *out_error = "report requires arguments.canonical_path";
    return false;
  }

  std::string schema;
  std::string run_id;
  (void)extract_json_string_field(arguments_json, "schema", &schema);
  (void)extract_json_string_field(arguments_json, "run_id", &run_id);

  std::size_t limit = 0;
  std::size_t offset = 0;
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);

  bool newest_first = false;
  (void)extract_json_bool_field(arguments_json, "newest_first", &newest_first);

  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> rows{};
  if (!app->catalog.list_artifacts(canonical_path, schema, 0, 0, newest_first, &rows,
                                   out_error)) {
    return false;
  }

  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> filtered{};
  filtered.reserve(rows.size());
  for (const auto& row : rows) {
    if (!run_id.empty() && row.run_id != run_id) continue;
    filtered.push_back(row);
  }

  const std::size_t off = std::min(offset, filtered.size());
  std::size_t count = limit;
  if (count == 0) count = filtered.size() - off;
  count = std::min(count, filtered.size() - off);
  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> selected(
      filtered.begin() + static_cast<std::ptrdiff_t>(off),
      filtered.begin() + static_cast<std::ptrdiff_t>(off + count));

  std::ostringstream artifacts_json;
  artifacts_json << "[";
  for (std::size_t i = 0; i < selected.size(); ++i) {
    if (i != 0) artifacts_json << ",";
    artifacts_json << artifact_to_json(selected[i]);
  }
  artifacts_json << "]";

  const std::string report_text = joined_kv_report(canonical_path, selected);
  std::ostringstream out;
  out << "{\"canonical_path\":" << json_quote(canonical_path)
      << ",\"run_id\":" << json_quote(run_id)
      << ",\"schema\":" << json_quote(schema)
      << ",\"count\":" << selected.size()
      << ",\"artifacts\":" << artifacts_json.str()
      << ",\"report_kv\":" << json_quote(report_text) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_call(app_context_t* app,
                                    const std::string& tool_name,
                                    const std::string& arguments_json,
                                    std::string* out_result_json,
                                    std::string* out_error_json) {
  if (!app || !out_result_json || !out_error_json) return false;
  out_result_json->clear();
  out_error_json->clear();

  std::string structured;
  std::string err;
  bool ok = false;
  if (tool_name == "hero.hashimyei.scan") {
    ok = handle_tool_scan(app, arguments_json, &structured, &err);
  } else if (tool_name == "hero.hashimyei.latest") {
    ok = handle_tool_latest(app, arguments_json, &structured, &err);
  } else if (tool_name == "hero.hashimyei.history") {
    ok = handle_tool_history(app, arguments_json, &structured, &err);
  } else if (tool_name == "hero.hashimyei.performance") {
    ok = handle_tool_performance(app, arguments_json, &structured, &err);
  } else if (tool_name == "hero.hashimyei.provenance") {
    ok = handle_tool_provenance(app, arguments_json, &structured, &err);
  } else if (tool_name == "hero.hashimyei.component") {
    ok = handle_tool_component(app, arguments_json, &structured, &err);
  } else if (tool_name == "hero.hashimyei.founding_dsl") {
    ok = handle_tool_founding_dsl(app, arguments_json, &structured, &err);
  } else if (tool_name == "hero.hashimyei.report") {
    ok = handle_tool_report(app, arguments_json, &structured, &err);
  } else {
    *out_error_json = "unknown tool: " + tool_name;
    return false;
  }

  if (!ok) {
    const std::string fallback =
        "{\"canonical_path\":\"\",\"error\":" + json_quote(err) + "}";
    *out_result_json = make_tool_result_json(err, fallback, true);
    return true;
  }

  *out_result_json = make_tool_result_json("ok", structured, false);
  return true;
}

[[nodiscard]] std::string initialize_result_json() {
  std::ostringstream out;
  out << "{"
      << "\"protocolVersion\":" << json_quote(kProtocolVersion) << ","
      << "\"serverInfo\":{\"name\":" << json_quote(kServerName)
      << ",\"version\":" << json_quote(kServerVersion) << "},"
      << "\"capabilities\":{\"tools\":{}}"
      << "}";
  return out.str();
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

    if (method == "notifications/initialized") {
      continue;
    }
    if (method == "initialize") {
      write_jsonrpc_result(id_json, initialize_result_json());
      continue;
    }
    if (method == "ping") {
      write_jsonrpc_result(id_json, "{\"ok\":true}");
      continue;
    }
    if (method == "tools/list") {
      write_jsonrpc_result(id_json, tools_list_result_json());
      continue;
    }
    if (method != "tools/call") {
      write_jsonrpc_error(id_json, -32601, "method not found");
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
    if (!handle_tool_call(app, name, arguments, &tool_result, &tool_error)) {
      write_jsonrpc_error(id_json, -32601, tool_error);
      continue;
    }
    write_jsonrpc_result(id_json, tool_result);
  }
}

void print_help(const char* argv0) {
  std::cout << "Usage: " << argv0 << " [options]\n"
            << "Options:\n"
            << "  --store-root <path>      Hashimyei root (default: CUWACUNU_HASHIMYEI_STORE_ROOT or /cuwacunu/.hashimyei)\n"
            << "  --catalog <path>         Catalog DB path (default: <store-root>/catalog/hashimyei_catalog.idydb)\n"
            << "  --passphrase <value>     Encryption passphrase override\n"
            << "  --passphrase-env <name>  Read passphrase from env var (default: CUWACUNU_HASHIMYEI_META_SECRET)\n"
            << "  --unencrypted            Open plaintext catalog (no passphrase required)\n"
            << "  --help                   Show this help\n";
}

}  // namespace

int main(int argc, char** argv) {
  app_context_t app{};
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);

  std::filesystem::path store_root = default_store_root();

  std::filesystem::path catalog_path{};
  std::string passphrase{};
  std::string passphrase_env = kDefaultPassEnv;
  bool encrypted = true;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--store-root" && i + 1 < argc) {
      store_root = argv[++i];
      continue;
    }
    if (arg == "--catalog" && i + 1 < argc) {
      catalog_path = argv[++i];
      continue;
    }
    if (arg == "--passphrase" && i + 1 < argc) {
      passphrase = argv[++i];
      continue;
    }
    if (arg == "--passphrase-env" && i + 1 < argc) {
      passphrase_env = argv[++i];
      continue;
    }
    if (arg == "--unencrypted") {
      encrypted = false;
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

  if (catalog_path.empty()) {
    catalog_path = default_catalog_path(store_root);
  }

  if (passphrase.empty() && !passphrase_env.empty()) {
    if (const char* env = std::getenv(passphrase_env.c_str()); env && env[0] != '\0') {
      passphrase = env;
    }
  }
  if (encrypted && passphrase.empty()) {
    write_jsonrpc_error("null", -32000,
                        "missing passphrase: set --passphrase or --passphrase-env");
    return 2;
  }

  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t::options_t options{};
  options.catalog_path = catalog_path;
  options.encrypted = encrypted;
  options.passphrase = passphrase;

  std::string error;
  if (!app.catalog.open(options, &error)) {
    write_jsonrpc_error("null", -32000, "catalog open failed: " + error);
    return 2;
  }

  app.store_root = store_root;
  run_jsonrpc_stdio_loop(&app);
  return 0;
}
