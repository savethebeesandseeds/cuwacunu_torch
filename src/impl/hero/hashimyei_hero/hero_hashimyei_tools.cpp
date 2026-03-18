#include "hero/hashimyei_hero/hero_hashimyei_tools.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <unistd.h>
#include <vector>

#include <openssl/evp.h>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "hero/lattice_hero/lattice_catalog.h"

namespace {

constexpr const char* kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char* kServerName = "hero_hashimyei_mcp";
constexpr const char* kServerVersion = "0.1.0";
constexpr const char* kProtocolVersion = "2025-03-26";
constexpr const char* kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.hashimyei.*.";
constexpr auto kAutoIngestMinInterval = std::chrono::seconds(2);
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;  // 8 MiB

bool g_jsonrpc_use_content_length_framing = false;

using app_context_t = cuwacunu::hero::hashimyei_mcp::app_context_t;
using hashimyei_runtime_defaults_t =
    cuwacunu::hero::hashimyei_mcp::hashimyei_runtime_defaults_t;

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

[[nodiscard]] std::uint64_t now_ms_utc() {
  const auto now = std::chrono::system_clock::now();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
          .count());
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
  if (p.is_absolute()) return p.string();
  if (base_folder.empty()) return p.string();
  return (std::filesystem::path(base_folder) / p).string();
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
    const std::string lhs = trim_ascii(trimmed.substr(0, eq));
    if (lhs != key) continue;
    return trim_ascii(trimmed.substr(eq + 1));
  }
  return std::nullopt;
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

[[nodiscard]] std::filesystem::path resolve_hashimyei_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "REAL_HERO", "hashimyei_hero_dsl_filename");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved);
}

[[nodiscard]] bool load_hashimyei_runtime_defaults(
    const std::filesystem::path& hero_dsl_path, hashimyei_runtime_defaults_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for hashimyei runtime defaults";
    return false;
  }

  std::error_code ec;
  if (!std::filesystem::exists(hero_dsl_path, ec) ||
      !std::filesystem::is_regular_file(hero_dsl_path, ec)) {
    if (error) *error = "hero hashimyei DSL file not found: " + hero_dsl_path.string();
    return false;
  }
  *out = hashimyei_runtime_defaults_t{};

  std::ifstream in(hero_dsl_path);
  if (!in) {
    if (error) *error = "cannot open hero hashimyei DSL file: " + hero_dsl_path.string();
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
    const std::string value = unquote_if_wrapped(
        trim_ascii(candidate.substr(eq + 1)));
    lhs = cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(lhs);
    if (lhs.empty()) continue;
    values[lhs] = value;
  }

  bool saw_store_root = false;
  const auto it_store_root = values.find("store_root");
  if (it_store_root != values.end()) {
    saw_store_root = true;
    out->store_root = resolve_path_from_base_folder(
        hero_dsl_path.parent_path().string(), it_store_root->second);
  }
  bool saw_catalog_path = false;
  const auto it_catalog_path = values.find("catalog_path");
  if (it_catalog_path != values.end()) {
    saw_catalog_path = true;
    out->catalog_path = resolve_path_from_base_folder(
        hero_dsl_path.parent_path().string(), it_catalog_path->second);
  }
  bool saw_encrypted = false;
  const auto it_encrypted = values.find("encrypted");
  if (it_encrypted != values.end()) {
    saw_encrypted = true;
    bool parsed = false;
    if (!parse_bool_token(it_encrypted->second, &parsed)) {
      if (error) *error = "invalid bool for key encrypted in " + hero_dsl_path.string();
      return false;
    }
    if (parsed) {
      if (error) {
        *error = "encrypted=true is not supported in " + hero_dsl_path.string() +
                 "; set encrypted=false";
      }
      return false;
    }
  }
  if (!saw_store_root || out->store_root.empty()) {
    if (error) *error = "missing store_root in " + hero_dsl_path.string();
    return false;
  }
  if (!saw_catalog_path || out->catalog_path.empty()) {
    if (error) *error = "missing catalog_path in " + hero_dsl_path.string();
    return false;
  }
  if (!saw_encrypted) {
    if (error) *error = "missing encrypted in " + hero_dsl_path.string();
    return false;
  }
  return true;
}

[[nodiscard]] std::string maybe_hashimyei_from_canonical(std::string_view canonical) {
  const std::size_t dot = canonical.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= canonical.size()) return {};
  const std::string_view tail = canonical.substr(dot + 1);
  if (tail.size() < 3) return {};
  if (tail[0] != '0' || (tail[1] != 'x' && tail[1] != 'X')) return {};
  for (std::size_t i = 2; i < tail.size(); ++i) {
    const char c = tail[i];
    const bool digit = c >= '0' && c <= '9';
    const bool lower = c >= 'a' && c <= 'f';
    const bool upper = c >= 'A' && c <= 'F';
    if (!digit && !lower && !upper) return {};
  }
  return std::string(tail);
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

[[nodiscard]] bool is_ingest_lock_held_error(std::string_view error) {
  return error.find("ingest lock already held:") != std::string_view::npos;
}

[[nodiscard]] bool maybe_auto_ingest_catalog(app_context_t* app,
                                             bool force,
                                             std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app) {
    if (out_error) *out_error = "missing app context";
    return false;
  }
  if (app->store_root.empty()) {
    if (out_error) *out_error = "store_root is empty";
    return false;
  }

  if (!force && app->auto_ingest_ready &&
      std::chrono::steady_clock::now() - app->last_auto_ingest_at <
          kAutoIngestMinInterval) {
    return true;
  }

  std::string ingest_error;
  if (!app->catalog.ingest_filesystem(app->store_root, &ingest_error)) {
    if (app->catalog.opened() && is_ingest_lock_held_error(ingest_error)) {
      if (out_error) out_error->clear();
      app->last_auto_ingest_at = std::chrono::steady_clock::now();
      app->auto_ingest_ready = true;
      return true;
    }
    if (out_error) *out_error = std::move(ingest_error);
    return false;
  }

  app->last_auto_ingest_at = std::chrono::steady_clock::now();
  app->auto_ingest_ready = true;
  return true;
}

[[nodiscard]] bool close_catalog_if_open(app_context_t* app,
                                         std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !app->catalog.opened()) return true;
  return app->catalog.close(out_error);
}

[[nodiscard]] bool reset_catalog(app_context_t* app, bool reingest,
                                 std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app) {
    if (out_error) *out_error = "missing app context";
    return false;
  }

  if (app->catalog.opened()) {
    std::string close_error;
    if (!app->catalog.close(&close_error)) {
      if (out_error) *out_error = "catalog close failed: " + close_error;
      return false;
    }
  }

  const std::filesystem::path catalog_path = app->catalog_options.catalog_path;
  if (catalog_path.empty()) {
    if (out_error) *out_error = "catalog_path is empty";
    return false;
  }

  std::error_code ec{};
  std::filesystem::remove(catalog_path, ec);
  if (ec && std::filesystem::exists(catalog_path)) {
    if (out_error) {
      *out_error = "cannot remove catalog file: " + catalog_path.string();
    }
    return false;
  }

  if (!app->catalog.open(app->catalog_options, out_error)) return false;
  app->auto_ingest_ready = false;
  app->last_auto_ingest_at = std::chrono::steady_clock::time_point{};
  if (!reingest) return true;

  return maybe_auto_ingest_catalog(app, true, out_error);
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

[[nodiscard]] bool extract_json_string_array_field(const std::string& json,
                                                   const std::string& key,
                                                   std::vector<std::string>* out) {
  if (!out) return false;
  out->clear();

  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  std::size_t pos = skip_json_whitespace(raw, 0);
  if (pos >= raw.size() || raw[pos] != '[') return false;
  ++pos;
  while (true) {
    pos = skip_json_whitespace(raw, pos);
    if (pos >= raw.size()) return false;
    if (raw[pos] == ']') {
      ++pos;
      pos = skip_json_whitespace(raw, pos);
      return pos == raw.size();
    }
    std::string value{};
    std::size_t end_pos = pos;
    if (!parse_json_string_at(raw, pos, &value, &end_pos)) return false;
    out->push_back(value);
    pos = skip_json_whitespace(raw, end_pos);
    if (pos >= raw.size()) return false;
    if (raw[pos] == ',') {
      ++pos;
      continue;
    }
    if (raw[pos] == ']') {
      ++pos;
      pos = skip_json_whitespace(raw, pos);
      return pos == raw.size();
    }
    return false;
  }
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
  if (parsed > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return false;
  }
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

[[nodiscard]] std::string hashimyei_identity_to_json(
    const cuwacunu::hashimyei::hashimyei_t& id) {
  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(id.schema) << ","
      << "\"kind\":"
      << json_quote(cuwacunu::hashimyei::hashimyei_kind_to_string(id.kind)) << ","
      << "\"name\":" << json_quote(id.name) << ","
      << "\"ordinal\":" << id.ordinal << ","
      << "\"hash_sha256_hex\":" << json_quote(id.hash_sha256_hex) << "}";
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
      << "\"report_fragment_sha256\":" << json_quote(component.report_fragment_sha256) << ","
      << "\"family_rank\":"
      << (component.family_rank.has_value()
              ? std::to_string(*component.family_rank)
              : "null")
      << ","
      << "\"manifest\":{"
      << "\"schema\":" << json_quote(m.schema) << ","
      << "\"canonical_path\":" << json_quote(m.canonical_path) << ","
      << "\"family\":" << json_quote(m.family) << ","
      << "\"hashimyei_identity\":"
      << hashimyei_identity_to_json(m.hashimyei_identity) << ","
      << "\"contract_identity\":"
      << hashimyei_identity_to_json(m.contract_identity) << ","
      << "\"parent_identity\":"
      << (m.parent_identity.has_value()
              ? hashimyei_identity_to_json(*m.parent_identity)
              : "null")
      << ","
      << "\"revision_reason\":" << json_quote(m.revision_reason) << ","
      << "\"founding_revision_id\":"
      << json_quote(m.founding_revision_id) << ","
      << "\"founding_dsl_provenance_path\":"
      << json_quote(m.founding_dsl_provenance_path) << ","
      << "\"founding_dsl_provenance_sha256_hex\":"
      << json_quote(m.founding_dsl_provenance_sha256_hex) << ","
      << "\"docking_signature_sha256_hex\":"
      << json_quote(m.docking_signature_sha256_hex) << ","
      << "\"lineage_state\":" << json_quote(m.lineage_state) << ","
      << "\"replaced_by\":" << json_quote(m.replaced_by) << ","
      << "\"created_at_ms\":" << m.created_at_ms << ","
      << "\"updated_at_ms\":" << m.updated_at_ms
      << "}"
      << "}";
  return out.str();
}

[[nodiscard]] std::string family_rank_state_to_json(
    const cuwacunu::hero::family_rank::state_t& state) {
  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(state.schema) << ","
      << "\"family\":" << json_quote(state.family) << ","
      << "\"contract_hash\":"
      << (state.contract_hash.empty() ? "null" : json_quote(state.contract_hash))
      << ",\"source_view_kind\":"
      << (state.source_view_kind.empty() ? "null"
                                         : json_quote(state.source_view_kind))
      << ",\"source_view_transport_sha256\":"
      << (state.source_view_transport_sha256.empty()
              ? "null"
              : json_quote(state.source_view_transport_sha256))
      << ",\"updated_at_ms\":" << state.updated_at_ms
      << ",\"assignments\":[";
  for (std::size_t i = 0; i < state.assignments.size(); ++i) {
    const auto& assignment = state.assignments[i];
    if (i != 0) out << ",";
    out << "{"
        << "\"rank\":" << assignment.rank << ","
        << "\"hashimyei\":" << json_quote(assignment.hashimyei) << ","
        << "\"canonical_path\":" << json_quote(assignment.canonical_path) << ","
        << "\"component_id\":" << json_quote(assignment.component_id) << "}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string sanitize_family_rank_path_token(std::string_view family) {
  std::string out;
  out.reserve(family.size());
  for (const unsigned char c : family) {
    const bool alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    const bool digit = (c >= '0' && c <= '9');
    if (alpha || digit) {
      out.push_back(static_cast<char>(c));
    } else {
      out.push_back('_');
    }
  }
  if (out.empty()) out = "family";
  return out;
}

[[nodiscard]] std::filesystem::path family_rank_file_path(
    const std::filesystem::path& store_root, std::string_view family,
    std::string_view contract_hash) {
  std::string family_sha{};
  if (!sha256_hex_bytes(std::string(family), &family_sha)) family_sha.clear();
  std::string contract_sha{};
  if (!sha256_hex_bytes(std::string(contract_hash), &contract_sha)) {
    contract_sha.clear();
  }
  const std::string folder =
      sanitize_family_rank_path_token(family) +
      (family_sha.empty() ? std::string{} : ("_" + family_sha.substr(0, 12)));
  const std::string contract_folder =
      sanitize_family_rank_path_token(contract_hash) +
      (contract_sha.empty() ? std::string{} : ("_" + contract_sha.substr(0, 12)));
  return store_root / "hero" / "family_rank" / folder / contract_folder /
         "family.rank.latest.lls";
}

[[nodiscard]] bool write_text_file_atomic(const std::filesystem::path& path,
                                          std::string_view payload,
                                          std::string* error) {
  if (error) error->clear();
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  if (ec) {
    if (error) *error = "cannot create rank directory: " + path.parent_path().string();
    return false;
  }
  const std::filesystem::path tmp_path = path.string() + ".tmp";
  {
    std::ofstream out(tmp_path, std::ios::binary | std::ios::trunc);
    if (!out) {
      if (error) *error = "cannot open temp rank file: " + tmp_path.string();
      return false;
    }
    out << payload;
    if (!out) {
      if (error) *error = "cannot write temp rank file: " + tmp_path.string();
      return false;
    }
  }
  std::filesystem::rename(tmp_path, path, ec);
  if (ec) {
    std::filesystem::remove(path, ec);
    ec.clear();
    std::filesystem::rename(tmp_path, path, ec);
  }
  if (ec) {
    if (error) *error = "cannot finalize rank file: " + path.string();
    return false;
  }
  return true;
}

using hashimyei_tool_handler_t = bool (*)(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error);

struct hashimyei_tool_descriptor_t {
  std::string_view name;
  std::string_view description;
  std::string_view input_schema_json;
  hashimyei_tool_handler_t handler;
};

[[nodiscard]] bool handle_tool_list(app_context_t* app,
                                    const std::string& arguments_json,
                                    std::string* out_structured,
                                    std::string* out_error);
[[nodiscard]] bool handle_tool_get_founding_dsl_bundle(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error);
[[nodiscard]] bool handle_tool_get_component_manifest(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error);
[[nodiscard]] bool handle_tool_update_rank(app_context_t* app,
                                           const std::string& arguments_json,
                                           std::string* out_structured,
                                           std::string* out_error);
[[nodiscard]] bool handle_tool_reset_catalog(app_context_t* app,
                                             const std::string& arguments_json,
                                             std::string* out_structured,
                                             std::string* out_error);

constexpr hashimyei_tool_descriptor_t kHashimyeiTools[] = {
#define HERO_HASHIMYEI_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER) \
  hashimyei_tool_descriptor_t{NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
#include "hero/hashimyei_hero/hero_hashimyei_tools.def"
#undef HERO_HASHIMYEI_TOOL
};

[[nodiscard]] const hashimyei_tool_descriptor_t* find_hashimyei_tool_descriptor(
    std::string_view name) {
  for (const auto& descriptor : kHashimyeiTools) {
    if (descriptor.name == name) return &descriptor;
  }
  return nullptr;
}

[[nodiscard]] std::string tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < std::size(kHashimyeiTools); ++i) {
    if (i != 0) out << ",";
    out << "{\"name\":" << json_quote(kHashimyeiTools[i].name)
        << ",\"description\":" << json_quote(kHashimyeiTools[i].description)
        << ",\"inputSchema\":" << kHashimyeiTools[i].input_schema_json << "}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string tools_list_human_text() {
  std::ostringstream out;
  for (const auto& descriptor : kHashimyeiTools) {
    out << descriptor.name << "\t" << descriptor.description << "\n";
  }
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

[[nodiscard]] bool handle_tool_list(app_context_t* app,
                                    const std::string& arguments_json,
                                    std::string* out_structured,
                                    std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  bool include_non_hashimyei = false;
  std::string canonical_path_prefix;
  std::size_t limit = 0;
  std::size_t offset = 0;
  (void)extract_json_bool_field(arguments_json, "include_non_hashimyei",
                                &include_non_hashimyei);
  (void)extract_json_string_field(arguments_json, "canonical_path_prefix",
                                  &canonical_path_prefix);
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);

  struct ref_rollup_t {
    std::string canonical_path{};
    std::string hashimyei{};
    std::size_t report_fragment_count{0};
    std::size_t component_count{0};
  };
  std::unordered_map<std::string, ref_rollup_t> by_ref{};

  const auto add_entry = [&](std::string hash, const std::string& canonical_path,
                             const bool from_component) {
    hash = trim_ascii(hash);
    std::string canonical_ref = trim_ascii(canonical_path);
    if (hash.empty()) hash = maybe_hashimyei_from_canonical(canonical_ref);
    if (!canonical_path_prefix.empty() &&
        canonical_ref.rfind(canonical_path_prefix, 0) != 0) {
      return;
    }
    if (hash.empty() && !include_non_hashimyei) return;
    if (!hash.empty() && !canonical_ref.empty()) {
      const std::string suffix = "." + hash;
      if (canonical_ref.size() <= suffix.size() ||
          canonical_ref.compare(canonical_ref.size() - suffix.size(),
                                suffix.size(), suffix) != 0) {
        canonical_ref += suffix;
      }
    }
    if (canonical_ref.empty()) {
      canonical_ref = hash;
    }
    if (canonical_ref.empty()) return;

    auto it = by_ref.find(canonical_ref);
    if (it == by_ref.end()) {
      it = by_ref
               .emplace(canonical_ref,
                        ref_rollup_t{
                            .canonical_path = canonical_ref,
                            .hashimyei = hash.empty()
                                             ? maybe_hashimyei_from_canonical(
                                                   canonical_ref)
                                             : hash,
                            .report_fragment_count = 0,
                            .component_count = 0,
                        })
               .first;
    }
    if (from_component) {
      ++it->second.component_count;
    } else {
      ++it->second.report_fragment_count;
    }
  };

  std::vector<cuwacunu::hero::hashimyei::component_state_t> components{};
  if (!app->catalog.list_components("", "", 0, 0, true, &components, out_error)) {
    return false;
  }
  for (const auto& c : components) {
    add_entry(c.manifest.hashimyei_identity.name, c.manifest.canonical_path,
              true);
  }

  std::vector<cuwacunu::hero::hashimyei::report_fragment_entry_t> report_fragments{};
  if (!app->catalog.list_report_fragments("", "", 0, 0, true, &report_fragments,
                                          out_error)) {
    return false;
  }
  for (const auto& fragment : report_fragments) {
    add_entry(fragment.hashimyei, fragment.canonical_path, false);
  }

  std::vector<ref_rollup_t> rows{};
  rows.reserve(by_ref.size());
  for (const auto& [_, value] : by_ref) {
    rows.push_back(value);
  }

  std::sort(rows.begin(), rows.end(), [](const ref_rollup_t& a,
                                         const ref_rollup_t& b) {
    return a.canonical_path < b.canonical_path;
  });

  const std::size_t total = rows.size();
  const std::size_t off = std::min(offset, rows.size());
  std::size_t count = limit;
  if (count == 0) count = rows.size() - off;
  count = std::min(count, rows.size() - off);
  const auto begin = rows.begin() + static_cast<std::ptrdiff_t>(off);
  const auto end = begin + static_cast<std::ptrdiff_t>(count);
  rows.assign(begin, end);

  std::ostringstream rows_json;
  rows_json << "[";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i != 0) rows_json << ",";
    rows_json << "{\"canonical_path\":"
              << json_quote(rows[i].canonical_path)
              << ",\"hashimyei\":" << json_quote(rows[i].hashimyei)
              << ",\"report_fragment_count\":" << rows[i].report_fragment_count
              << ",\"component_count\":" << rows[i].component_count << "}";
  }
  rows_json << "]";

  std::ostringstream out;
  out << "{\"canonical_path\":\"\""
      << ",\"count\":" << rows.size()
      << ",\"total\":" << total
      << ",\"include_non_hashimyei\":"
      << (include_non_hashimyei ? "true" : "false")
      << ",\"hashimyeis\":" << rows_json.str() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_founding_dsl_bundle(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string canonical_path;
  std::string hashimyei;
  (void)extract_json_string_field(arguments_json, "canonical_path", &canonical_path);
  (void)extract_json_string_field(arguments_json, "hashimyei", &hashimyei);
  if (canonical_path.empty() && hashimyei.empty()) {
    *out_error =
        "get_founding_dsl_bundle requires arguments.canonical_path or "
        "arguments.hashimyei";
    return false;
  }

  bool include_content = false;
  (void)extract_json_bool_field(arguments_json, "include_content", &include_content);

  cuwacunu::hero::hashimyei::component_state_t component{};
  if (!app->catalog.resolve_component(canonical_path, hashimyei, &component,
                                      out_error)) {
    return false;
  }

  const std::string component_id =
      component.component_id.empty()
          ? cuwacunu::hero::hashimyei::compute_component_manifest_id(
                component.manifest)
          : component.component_id;
  const auto bundle_dir =
      cuwacunu::hero::hashimyei::founding_dsl_bundle_directory(
          app->store_root, component_id);
  const auto bundle_manifest_path =
      cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_path(
          app->store_root, component_id);

  cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_t bundle_manifest{};
  if (!cuwacunu::hero::hashimyei::read_founding_dsl_bundle_manifest(
          app->store_root, component_id, &bundle_manifest, out_error)) {
    return false;
  }
  if (!bundle_manifest.component_id.empty() &&
      bundle_manifest.component_id != component_id) {
    *out_error = "founding DSL bundle manifest component_id mismatch: " +
                 bundle_manifest.component_id + " != " + component_id;
    return false;
  }
  if (!bundle_manifest.canonical_path.empty() &&
      bundle_manifest.canonical_path != component.manifest.canonical_path) {
    *out_error = "founding DSL bundle manifest canonical_path mismatch: " +
                 bundle_manifest.canonical_path + " != " +
                 component.manifest.canonical_path;
    return false;
  }
  if (!bundle_manifest.hashimyei_name.empty() &&
      bundle_manifest.hashimyei_name != component.manifest.hashimyei_identity.name) {
    *out_error = "founding DSL bundle manifest hashimyei_name mismatch: " +
                 bundle_manifest.hashimyei_name + " != " +
                 component.manifest.hashimyei_identity.name;
    return false;
  }
  if (bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex.empty()) {
    *out_error =
        "founding DSL bundle manifest missing founding_dsl_bundle_aggregate_sha256_hex";
    return false;
  }

  std::ostringstream files_json;
  files_json << "[";
  bool all_files_match = true;
  std::string bundle_sha_actual{};

  auto verified_manifest = bundle_manifest;
  for (std::size_t i = 0; i < bundle_manifest.files.size(); ++i) {
    if (i != 0) files_json << ",";
    const auto& file = bundle_manifest.files[i];
    const std::filesystem::path stored_path = bundle_dir / file.snapshot_relpath;
    std::string dsl_text{};
    if (!read_text_file(stored_path, &dsl_text, out_error)) {
      *out_error = "cannot read founding DSL bundle snapshot '" +
                   stored_path.string() + "': " + *out_error;
      return false;
    }

    std::string dsl_sha_actual{};
    if (!sha256_hex_bytes(dsl_text, &dsl_sha_actual)) {
      *out_error = "cannot compute sha256 for founding DSL bundle snapshot: " +
                   stored_path.string();
      return false;
    }

    const bool dsl_sha_match =
        lowercase_copy(dsl_sha_actual) == lowercase_copy(file.sha256_hex);
    all_files_match = all_files_match && dsl_sha_match;
    verified_manifest.files[i].sha256_hex = dsl_sha_actual;

    files_json << "{"
               << "\"source_path\":" << json_quote(file.source_path) << ","
               << "\"stored_path\":" << json_quote(stored_path.string()) << ","
               << "\"snapshot_relpath\":"
               << json_quote(file.snapshot_relpath)
               << ",\"sha256_expected\":"
               << json_quote(file.sha256_hex)
               << ",\"sha256_actual\":"
               << json_quote(dsl_sha_actual) << ","
               << "\"sha256_match\":"
               << (dsl_sha_match ? "true" : "false");
    if (include_content) {
      files_json << ",\"text\":" << json_quote(dsl_text);
    }
    files_json << "}";
  }
  bundle_sha_actual =
      cuwacunu::hero::hashimyei::compute_founding_dsl_bundle_aggregate_sha256(
          verified_manifest);
  if (bundle_sha_actual.empty()) {
    *out_error = "cannot compute founding DSL bundle aggregate sha256";
    return false;
  }
  const bool bundle_sha_match =
      lowercase_copy(bundle_sha_actual) ==
      lowercase_copy(bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex);
  all_files_match = all_files_match && bundle_sha_match;
  files_json << "]";

  std::ostringstream out;
  out << "{\"canonical_path\":"
      << json_quote(component.manifest.canonical_path)
      << ",\"hashimyei\":"
      << json_quote(component.manifest.hashimyei_identity.name)
      << ",\"component_id\":" << json_quote(component_id)
      << ",\"founding_dsl_provenance_path\":"
      << json_quote(component.manifest.founding_dsl_provenance_path)
      << ",\"founding_dsl_provenance_sha256_expected\":"
      << json_quote(component.manifest.founding_dsl_provenance_sha256_hex)
      << ",\"founding_dsl_bundle_manifest_path\":"
      << json_quote(bundle_manifest_path.string())
      << ",\"founding_dsl_bundle_directory\":"
      << json_quote(bundle_dir.string())
      << ",\"founding_dsl_bundle_aggregate_sha256_expected\":"
      << json_quote(bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex)
      << ",\"founding_dsl_bundle_aggregate_sha256_actual\":"
      << json_quote(bundle_sha_actual)
      << ",\"founding_dsl_bundle_consistent\":"
      << (all_files_match ? "true" : "false")
      << ",\"founding_dsl_bundle_files\":"
      << files_json.str()
      << ",\"component\":" << component_state_to_json(component);
  out << "}";

  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_component_manifest(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string canonical_path{};
  std::string hashimyei{};
  (void)extract_json_string_field(arguments_json, "canonical_path", &canonical_path);
  (void)extract_json_string_field(arguments_json, "hashimyei", &hashimyei);
  if (canonical_path.empty() && hashimyei.empty()) {
    *out_error =
        "get_component_manifest requires arguments.canonical_path or arguments.hashimyei";
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
      << ",\"hashimyei\":"
      << json_quote(component.manifest.hashimyei_identity.name)
      << ",\"component\":" << component_state_to_json(component) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_update_rank(app_context_t* app,
                                           const std::string& arguments_json,
                                           std::string* out_structured,
                                           std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string family{};
  std::string contract_hash{};
  std::string source_view_kind{};
  std::string source_view_transport_sha256{};
  std::vector<std::string> ordered_hashimyeis{};
  (void)extract_json_string_field(arguments_json, "family", &family);
  (void)extract_json_string_field(arguments_json, "contract_hash", &contract_hash);
  (void)extract_json_string_field(arguments_json, "source_view_kind",
                                  &source_view_kind);
  (void)extract_json_string_field(arguments_json, "source_view_transport_sha256",
                                  &source_view_transport_sha256);
  (void)extract_json_string_array_field(arguments_json, "ordered_hashimyeis",
                                        &ordered_hashimyeis);
  family = trim_ascii(family);
  contract_hash = trim_ascii(contract_hash);
  source_view_kind = trim_ascii(source_view_kind);
  source_view_transport_sha256 = trim_ascii(source_view_transport_sha256);
  if (family.empty()) {
    *out_error = "update_rank requires arguments.family";
    return false;
  }
  if (contract_hash.empty()) {
    *out_error = "update_rank requires arguments.contract_hash";
    return false;
  }
  if (ordered_hashimyeis.empty()) {
    *out_error = "update_rank requires arguments.ordered_hashimyeis";
    return false;
  }
  if (source_view_kind.empty()) source_view_kind = "family_evaluation_report";

  std::unordered_set<std::string> requested_hashimyeis{};
  for (auto& hashimyei : ordered_hashimyeis) {
    hashimyei = trim_ascii(hashimyei);
    if (hashimyei.empty()) {
      *out_error = "ordered_hashimyeis cannot contain empty values";
      return false;
    }
    if (!requested_hashimyeis.insert(hashimyei).second) {
      *out_error = "ordered_hashimyeis contains duplicates";
      return false;
    }
  }

  std::vector<cuwacunu::hero::hashimyei::component_state_t> components{};
  if (!app->catalog.list_components("", "", 0, 0, true, &components, out_error)) {
    return false;
  }

  std::unordered_map<std::string, cuwacunu::hero::hashimyei::component_state_t>
      latest_by_hashimyei{};
  for (const auto& component : components) {
    if (component.manifest.family != family) continue;
    if (cuwacunu::hero::hashimyei::contract_hash_from_identity(
            component.manifest.contract_identity) != contract_hash) {
      continue;
    }
    const std::string hashimyei = component.manifest.hashimyei_identity.name;
    if (hashimyei.empty()) continue;
    const auto it = latest_by_hashimyei.find(hashimyei);
    if (it == latest_by_hashimyei.end() ||
        component.ts_ms > it->second.ts_ms ||
        (component.ts_ms == it->second.ts_ms &&
         component.component_id > it->second.component_id)) {
      latest_by_hashimyei[hashimyei] = component;
    }
  }
  if (latest_by_hashimyei.empty()) {
    *out_error = "family has no components in the catalog for contract_hash=" +
                 contract_hash + ": " + family;
    return false;
  }
  if (ordered_hashimyeis.size() != latest_by_hashimyei.size()) {
    *out_error =
        "ordered_hashimyeis must enumerate every current family member exactly once";
    return false;
  }
  for (const auto& [hashimyei, _] : latest_by_hashimyei) {
    if (requested_hashimyeis.find(hashimyei) == requested_hashimyeis.end()) {
      *out_error =
          "ordered_hashimyeis is missing family member: " + hashimyei;
      return false;
    }
  }

  cuwacunu::hero::family_rank::state_t before_state{};
  const bool has_before_rank = app->catalog.get_explicit_family_rank(
      family, contract_hash, &before_state, nullptr);

  cuwacunu::hero::family_rank::state_t desired_state{};
  desired_state.family = family;
  desired_state.contract_hash = contract_hash;
  desired_state.source_view_kind = source_view_kind;
  desired_state.source_view_transport_sha256 = source_view_transport_sha256;
  desired_state.updated_at_ms = now_ms_utc();
  desired_state.assignments.reserve(ordered_hashimyeis.size());
  for (std::size_t i = 0; i < ordered_hashimyeis.size(); ++i) {
    const auto it = latest_by_hashimyei.find(ordered_hashimyeis[i]);
    if (it == latest_by_hashimyei.end()) {
      *out_error = "ordered_hashimyeis contains unknown family member";
      return false;
    }
    desired_state.assignments.push_back(cuwacunu::hero::family_rank::assignment_t{
        .rank = static_cast<std::uint64_t>(i),
        .hashimyei = ordered_hashimyeis[i],
        .canonical_path = it->second.manifest.canonical_path,
        .component_id = it->second.component_id,
    });
  }

  const auto state_matches = [&](const cuwacunu::hero::family_rank::state_t& lhs,
                                 const cuwacunu::hero::family_rank::state_t& rhs) {
    return cuwacunu::hero::family_rank::ordering_matches(lhs, rhs);
  };

  const std::filesystem::path rank_file =
      family_rank_file_path(app->store_root, family, contract_hash);
  const bool changed = !has_before_rank || !state_matches(before_state, desired_state);
  if (changed) {
    const std::string payload =
        cuwacunu::hero::family_rank::emit_state_payload(desired_state);
    if (!write_text_file_atomic(rank_file, payload, out_error)) return false;
    if (!app->catalog.ingest_filesystem(app->store_root, out_error)) return false;
  }

  bool synchronized_lattice = false;
  std::string lattice_catalog_error{};
  if (app->lattice_catalog_path.empty()) {
    *out_error = "lattice_catalog_path is empty";
    return false;
  }
  const std::filesystem::path lattice_catalog_path = app->lattice_catalog_path;
  cuwacunu::hero::wave::lattice_catalog_store_t lattice_catalog{};
  cuwacunu::hero::wave::lattice_catalog_store_t::options_t lattice_options{};
  lattice_options.catalog_path = lattice_catalog_path;
  lattice_options.encrypted = false;
  lattice_options.projection_version = 2;
  if (!lattice_catalog.open(lattice_options, &lattice_catalog_error)) {
    *out_error = "lattice sync open failed: " + lattice_catalog_error;
    return false;
  }
  if (!lattice_catalog.ingest_runtime_artifact(rank_file,
                                               &lattice_catalog_error)) {
    *out_error = "lattice sync ingest failed: " + lattice_catalog_error;
    return false;
  }
  synchronized_lattice = true;
  (void)lattice_catalog.close(nullptr);

  cuwacunu::hero::family_rank::state_t after_state{};
  if (!app->catalog.get_family_rank(family, contract_hash, &after_state,
                                    out_error)) {
    return false;
  }

  std::ostringstream out;
  out << "{\"family\":" << json_quote(family)
      << ",\"contract_hash\":" << json_quote(contract_hash)
      << ",\"changed\":" << (changed ? "true" : "false")
      << ",\"rank_file\":" << json_quote(rank_file.string())
      << ",\"lattice_catalog_path\":"
      << json_quote(lattice_catalog_path.string())
      << ",\"synchronized_lattice\":"
      << (synchronized_lattice ? "true" : "false")
      << ",\"component_count\":" << latest_by_hashimyei.size()
      << ",\"before\":"
      << (has_before_rank ? family_rank_state_to_json(before_state) : "null")
      << ",\"after\":" << family_rank_state_to_json(after_state) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_reset_catalog(app_context_t* app,
                                             const std::string& arguments_json,
                                             std::string* out_structured,
                                             std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  bool reingest = true;
  (void)extract_json_bool_field(arguments_json, "reingest", &reingest);
  if (!reset_catalog(app, reingest, out_error)) return false;

  std::vector<cuwacunu::hero::hashimyei::component_state_t> components{};
  if (!app->catalog.list_components("", "", 0, 0, true, &components, out_error)) {
    return false;
  }

  std::ostringstream out;
  out << "{\"canonical_path\":\"\""
      << ",\"catalog_path\":"
      << json_quote(app->catalog_options.catalog_path.string())
      << ",\"reingest\":" << (reingest ? "true" : "false")
      << ",\"component_count\":" << components.size() << "}";
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

  const auto* descriptor = find_hashimyei_tool_descriptor(tool_name);
  if (!descriptor) {
    *out_error_json = "unknown tool: " + tool_name;
    return false;
  }

  std::string structured;
  std::string err;
  if (tool_name != "hero.hashimyei.reset_catalog") {
    std::string ingest_error;
    if (!maybe_auto_ingest_catalog(app, false, &ingest_error)) {
      const std::string err_ingest = "auto-ingest failed: " + ingest_error;
      const std::string fallback =
          "{\"canonical_path\":\"\",\"error\":" + json_quote(err_ingest) + "}";
      *out_result_json = make_tool_result_json(err_ingest, fallback, true);
      return true;
    }
  }

  const bool ok =
      descriptor->handler(app, arguments_json, &structured, &err);

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
      << "\"capabilities\":{\"tools\":{\"listChanged\":false}},"
      << "\"instructions\":" << json_quote(kInitializeInstructions)
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

    if (method.rfind("notifications/", 0) == 0) {
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

    if (!app->catalog.opened()) {
      std::string open_error;
      if (!app->catalog.open(app->catalog_options, &open_error)) {
        const std::string err = "catalog open failed: " + open_error;
        const std::string fallback =
            "{\"canonical_path\":\"\",\"error\":" + json_quote(err) + "}";
        write_jsonrpc_result(id_json, make_tool_result_json(err, fallback, true));
        continue;
      }
    }

    std::string tool_result;
    std::string tool_error;
    const bool ok =
        handle_tool_call(app, name, arguments, &tool_result, &tool_error);

    // Release the catalog lock after every request so idle stdio servers do
    // not block other clients from opening the catalog.
    std::string close_error;
    if (!close_catalog_if_open(app, &close_error)) {
      write_jsonrpc_error(id_json, -32603,
                          "catalog close failed: " + close_error);
      continue;
    }

    if (!ok) {
      write_jsonrpc_error(id_json, -32601, tool_error);
      continue;
    }

    write_jsonrpc_result(id_json, tool_result);
  }
}

}  // namespace

namespace cuwacunu {
namespace hero {
namespace hashimyei_mcp {

std::filesystem::path resolve_hashimyei_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  return ::resolve_hashimyei_hero_dsl_path(global_config_path);
}

bool load_hashimyei_runtime_defaults(const std::filesystem::path& hero_dsl_path,
                                     hashimyei_runtime_defaults_t* out,
                                     std::string* error) {
  return ::load_hashimyei_runtime_defaults(hero_dsl_path, out, error);
}

bool execute_tool_json(const std::string& tool_name, std::string arguments_json,
                       app_context_t* app,
                       std::string* out_tool_result_json,
                       std::string* out_error_message) {
  if (out_error_message) out_error_message->clear();
  if (!app || !out_tool_result_json || !out_error_message) return false;
  const bool opened_here = !app->catalog.opened();
  if (opened_here) {
    std::string open_error;
    if (!app->catalog.open(app->catalog_options, &open_error)) {
      *out_error_message = "catalog open failed: " + open_error;
      return false;
    }
  }
  const bool ok = ::handle_tool_call(app, tool_name, arguments_json,
                                     out_tool_result_json, out_error_message);
  if (opened_here) {
    std::string close_error;
    if (!::close_catalog_if_open(app, &close_error)) {
      if (!out_error_message->empty()) out_error_message->append("; ");
      out_error_message->append("catalog close failed: " + close_error);
      return false;
    }
  }
  return ok;
}

bool tool_result_is_error(std::string_view tool_result_json) {
  bool is_error = false;
  return extract_json_bool_field(std::string(tool_result_json), "isError",
                                 &is_error) &&
         is_error;
}

std::string build_tools_list_result_json() {
  return ::tools_list_result_json();
}

std::string build_tools_list_human_text() { return ::tools_list_human_text(); }

void run_jsonrpc_stdio_loop(app_context_t* app) { ::run_jsonrpc_stdio_loop(app); }

}  // namespace hashimyei_mcp
}  // namespace hero
}  // namespace cuwacunu
