#include "hero/lattice_hero/hero_lattice_tools.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdio>
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
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <unistd.h>

#include <openssl/evp.h>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "hero/lattice_hero/lattice_catalog.h"
#include "piaabo/dlogs.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"

namespace {

constexpr const char* kStoreRootEnv = "CUWACUNU_HASHIMYEI_STORE_ROOT";
constexpr const char* kDefaultStoreRoot = "/cuwacunu/.hashimyei";
constexpr const char* kDefaultWaveHeroDslPath =
    "/cuwacunu/src/config/instructions/default.hero.lattice.dsl";
constexpr const char* kServerName = "hero_lattice_mcp";
constexpr const char* kServerVersion = "0.1.0";
constexpr const char* kProtocolVersion = "2025-03-26";
constexpr const char* kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.lattice.*.";
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;  // 8 MiB

bool g_jsonrpc_use_content_length_framing = false;
int g_protocol_stdout_fd = -1;

using app_context_t = cuwacunu::hero::lattice_mcp::app_context_t;
using wave_runtime_defaults_t = cuwacunu::hero::lattice_mcp::wave_runtime_defaults_t;

__attribute__((constructor(101))) void redirect_stdout_logs_to_stderr() {
  if (g_protocol_stdout_fd >= 0) return;
  std::fflush(stdout);
  g_protocol_stdout_fd = ::dup(STDOUT_FILENO);
  if (g_protocol_stdout_fd < 0) return;
  (void)::dup2(STDERR_FILENO, STDOUT_FILENO);
}

__attribute__((constructor(102))) void disable_terminal_logs_pre_main() {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
}

[[nodiscard]] std::uint64_t now_ms_utc() {
  const auto now = std::chrono::system_clock::now();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
          .count());
}

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

[[nodiscard]] bool starts_with(std::string_view text,
                               std::string_view prefix) {
  return text.size() >= prefix.size() &&
         text.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] std::string normalize_source_hashimyei_cursor(
    std::string_view canonical_path) {
  return cuwacunu::hero::wave::lattice_catalog_store_t::
      normalize_runtime_hashimyei_cursor(canonical_path);
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

[[nodiscard]] bool sha256_hex_bytes(std::string_view payload, std::string* out_hex,
                                    std::string* error = nullptr) {
  if (error) error->clear();
  if (!out_hex) {
    if (error) *error = "sha256 output pointer is null";
    return false;
  }
  out_hex->clear();
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) {
    if (error) *error = "cannot allocate EVP context";
    return false;
  }

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  const bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
                  EVP_DigestUpdate(ctx, payload.data(), payload.size()) == 1 &&
                  EVP_DigestFinal_ex(ctx, digest, &digest_len) == 1;
  EVP_MD_CTX_free(ctx);
  if (!ok) {
    if (error) *error = "EVP sha256 failed";
    return false;
  }
  *out_hex = hex_lower(digest, digest_len);
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

[[nodiscard]] bool extract_json_number_field(const std::string& json,
                                             const std::string& key,
                                             double* out) {
  if (!out) return false;
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  raw = trim_ascii(raw);
  if (raw.empty()) return false;
  std::size_t pos = 0;
  double value = 0.0;
  try {
    value = std::stod(raw, &pos);
  } catch (...) {
    return false;
  }
  if (pos != raw.size()) return false;
  *out = value;
  return true;
}

[[nodiscard]] bool parse_u64_token(std::string_view text, std::uint64_t* out) {
  if (!out) return false;
  std::string token = trim_ascii(text);
  if (token.empty()) return false;
  const int base = (token.size() > 2 && token[0] == '0' &&
                    (token[1] == 'x' || token[1] == 'X'))
                       ? 16
                       : 10;
  errno = 0;
  char* end = nullptr;
  const unsigned long long parsed = std::strtoull(token.c_str(), &end, base);
  if (errno != 0 || end == nullptr || *end != '\0') return false;
  *out = static_cast<std::uint64_t>(parsed);
  return true;
}

[[nodiscard]] bool extract_json_u64_field(const std::string& json,
                                          const std::string& key,
                                          std::uint64_t* out) {
  if (!out) return false;
  std::string as_text;
  if (extract_json_string_field(json, key, &as_text)) {
    return parse_u64_token(as_text, out);
  }
  double as_number = 0.0;
  if (!extract_json_number_field(json, key, &as_number)) return false;
  if (!std::isfinite(as_number) || as_number < 0.0) return false;
  const double rounded = std::floor(as_number);
  if (rounded != as_number) return false;
  if (rounded > static_cast<double>(std::numeric_limits<std::uint64_t>::max())) {
    return false;
  }
  *out = static_cast<std::uint64_t>(rounded);
  return true;
}

[[nodiscard]] bool extract_json_wave_cursor_field(const std::string& json,
                                                  const std::string& key,
                                                  std::uint64_t* out) {
  if (!out) return false;
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  raw = trim_ascii(raw);
  if (raw.empty()) return false;

  std::string as_text;
  std::size_t end_pos = 0;
  if (parse_json_string_at(raw, 0, &as_text, &end_pos)) {
    end_pos = skip_json_whitespace(raw, end_pos);
    if (end_pos != raw.size()) return false;
    as_text = trim_ascii(as_text);
    if (as_text.find('.') == std::string::npos || as_text.find(',') != std::string::npos) {
      return false;
    }
    return cuwacunu::hero::wave::lattice_catalog_store_t::
        parse_runtime_wave_cursor_token(as_text, out);
  }
  return false;
}

enum class wave_cursor_resolution_e : std::uint8_t {
  Run = 0,
  Episode = 1,
  Batch = 2,
};

[[nodiscard]] std::uint64_t wave_cursor_mask_for_resolution(
    wave_cursor_resolution_e resolution) {
  const auto layout =
      cuwacunu::hero::wave::lattice_catalog_store_t::runtime_wave_cursor_layout();
  db::wave_cursor::masked_query_t q{};
  const db::wave_cursor::parts_t zero{};
  std::uint8_t field_mask = db::wave_cursor::field_run;
  if (resolution == wave_cursor_resolution_e::Episode ||
      resolution == wave_cursor_resolution_e::Batch) {
    field_mask = static_cast<std::uint8_t>(field_mask |
                                           db::wave_cursor::field_episode);
  }
  if (resolution == wave_cursor_resolution_e::Batch) {
    field_mask =
        static_cast<std::uint8_t>(field_mask | db::wave_cursor::field_batch);
  }
  (void)db::wave_cursor::build_masked_query(layout, zero, field_mask, &q);
  return q.mask;
}

[[nodiscard]] bool parse_wave_cursor_resolution(
    std::string_view token,
    wave_cursor_resolution_e* out_resolution) {
  if (!out_resolution) return false;
  const std::string lowered = lowercase_copy(trim_ascii(token));
  if (lowered == "run") {
    *out_resolution = wave_cursor_resolution_e::Run;
    return true;
  }
  if (lowered == "episode") {
    *out_resolution = wave_cursor_resolution_e::Episode;
    return true;
  }
  if (lowered == "batch") {
    *out_resolution = wave_cursor_resolution_e::Batch;
    return true;
  }
  return false;
}

[[nodiscard]] const char* wave_cursor_resolution_to_text(
    wave_cursor_resolution_e resolution) {
  switch (resolution) {
    case wave_cursor_resolution_e::Run:
      return "run";
    case wave_cursor_resolution_e::Episode:
      return "episode";
    case wave_cursor_resolution_e::Batch:
      return "batch";
    default:
      return "run";
  }
}

[[nodiscard]] bool extract_payload_kv(std::string_view payload,
                                      std::string_view key,
                                      std::string* out) {
  if (!out) return false;
  out->clear();
  cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t document{};
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          payload, &document, nullptr)) {
    return false;
  }
  std::unordered_map<std::string, std::string> kv{};
  if (!cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          document, &kv, nullptr)) {
    return false;
  }
  const auto it = kv.find(std::string(key));
  if (it == kv.end()) return false;
  *out = it->second;
  return true;
}

[[nodiscard]] bool matches_wave_cursor_filter(
    std::uint64_t wave_cursor,
    std::uint64_t expected,
    std::uint64_t mask,
    wave_cursor_resolution_e cursor_resolution) {
  const std::uint64_t comparable_mask =
      mask & wave_cursor_mask_for_resolution(cursor_resolution);
  if (mask != 0 && comparable_mask == 0) return false;
  db::wave_cursor::masked_query_t q{};
  q.value = expected & comparable_mask;
  q.mask = comparable_mask;
  return db::wave_cursor::match_masked(wave_cursor, q);
}

[[nodiscard]] bool extract_wave_cursor_from_payload(
    std::string_view payload,
    std::uint64_t* out_wave_cursor) {
  if (!out_wave_cursor) return false;
  *out_wave_cursor = 0;
  std::string wave_cursor_text{};
  return extract_payload_kv(payload, "wave_cursor", &wave_cursor_text) &&
         parse_u64_token(wave_cursor_text, out_wave_cursor);
}

[[nodiscard]] bool extract_wave_cursor_resolution_from_payload(
    std::string_view payload,
    wave_cursor_resolution_e* out_resolution) {
  if (!out_resolution) return false;
  *out_resolution = wave_cursor_resolution_e::Run;
  cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t document{};
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          payload, &document, nullptr)) {
    return false;
  }
  std::unordered_map<std::string, std::string> kv{};
  if (!cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          document, &kv, nullptr)) {
    return false;
  }
  bool has_episode_keys = false;
  bool has_batch_keys = false;
  for (const auto& [key, value] : kv) {
    if (key == "wave_cursor_resolution") {
      return parse_wave_cursor_resolution(value, out_resolution);
    }
    if (key == "wave.cursor.episode" || key == "episode_k" || key == "episode" ||
        key == "wave_episode" || key == "epoch_k") {
      has_episode_keys = true;
    }
    if (key == "wave.cursor.batch" || key == "batch_j" || key == "batch" ||
        key == "wave_batch") {
      has_batch_keys = true;
    }
  }
  if (has_batch_keys) {
    *out_resolution = wave_cursor_resolution_e::Batch;
  } else if (has_episode_keys) {
    *out_resolution = wave_cursor_resolution_e::Episode;
  } else {
    *out_resolution = wave_cursor_resolution_e::Run;
  }
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

[[nodiscard]] bool parse_json_object_string_pairs(
    const std::string& object_json,
    std::vector<std::pair<std::string, std::string>>* out) {
  if (!out) return false;
  out->clear();

  std::size_t pos = skip_json_whitespace(object_json, 0);
  if (pos >= object_json.size() || object_json[pos] != '{') return false;
  ++pos;

  while (true) {
    pos = skip_json_whitespace(object_json, pos);
    if (pos >= object_json.size()) return false;
    if (object_json[pos] == '}') return true;

    std::string key;
    std::size_t after_key = pos;
    if (!parse_json_string_at(object_json, pos, &key, &after_key)) return false;
    pos = skip_json_whitespace(object_json, after_key);
    if (pos >= object_json.size() || object_json[pos] != ':') return false;
    ++pos;
    pos = skip_json_whitespace(object_json, pos);
    if (pos >= object_json.size() || object_json[pos] != '"') return false;

    std::string value;
    std::size_t after_value = pos;
    if (!parse_json_string_at(object_json, pos, &value, &after_value)) return false;
    out->emplace_back(std::move(key), std::move(value));

    pos = skip_json_whitespace(object_json, after_value);
    if (pos >= object_json.size()) return false;
    if (object_json[pos] == ',') {
      ++pos;
      continue;
    }
    if (object_json[pos] == '}') return true;
    return false;
  }
}

[[nodiscard]] bool parse_json_object_number_pairs(
    const std::string& object_json,
    std::vector<std::pair<std::string, double>>* out) {
  if (!out) return false;
  out->clear();

  std::size_t pos = skip_json_whitespace(object_json, 0);
  if (pos >= object_json.size() || object_json[pos] != '{') return false;
  ++pos;

  while (true) {
    pos = skip_json_whitespace(object_json, pos);
    if (pos >= object_json.size()) return false;
    if (object_json[pos] == '}') return true;

    std::string key;
    std::size_t after_key = pos;
    if (!parse_json_string_at(object_json, pos, &key, &after_key)) return false;
    pos = skip_json_whitespace(object_json, after_key);
    if (pos >= object_json.size() || object_json[pos] != ':') return false;
    ++pos;

    std::size_t value_end = pos;
    if (!find_json_value_end(object_json, pos, &value_end)) return false;
    std::string raw_value =
        trim_ascii(std::string_view(object_json).substr(pos, value_end - pos));
    std::size_t parse_pos = 0;
    double value = 0.0;
    try {
      value = std::stod(raw_value, &parse_pos);
    } catch (...) {
      return false;
    }
    if (parse_pos != raw_value.size()) return false;
    out->emplace_back(std::move(key), value);

    pos = skip_json_whitespace(object_json, value_end);
    if (pos >= object_json.size()) return false;
    if (object_json[pos] == ',') {
      ++pos;
      continue;
    }
    if (object_json[pos] == '}') return true;
    return false;
  }
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

[[nodiscard]] std::filesystem::path default_store_root() {
  if (const char* env = std::getenv(kStoreRootEnv); env && env[0] != '\0') {
    return std::filesystem::path(env);
  }
  return std::filesystem::path(kDefaultStoreRoot);
}

[[nodiscard]] std::filesystem::path default_catalog_path(
    const std::filesystem::path& store_root) {
  return store_root / "catalog" / "lattice_catalog.idydb";
}

[[nodiscard]] std::filesystem::path resolve_lattice_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "REAL_HERO", "lattice_hero_dsl_filename");
  if (!configured.has_value()) {
    return std::filesystem::path(kDefaultWaveHeroDslPath);
  }
  const std::string resolved = resolve_path_from_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) {
    return std::filesystem::path(kDefaultWaveHeroDslPath);
  }
  return std::filesystem::path(resolved);
}

[[nodiscard]] bool load_wave_runtime_defaults(
    const std::filesystem::path& hero_dsl_path, wave_runtime_defaults_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for wave runtime defaults";
    return false;
  }

  std::error_code ec;
  if (!std::filesystem::exists(hero_dsl_path, ec) ||
      !std::filesystem::is_regular_file(hero_dsl_path, ec)) {
    if (error) *error = "hero lattice DSL file not found: " + hero_dsl_path.string();
    return false;
  }

  std::ifstream in(hero_dsl_path);
  if (!in) {
    if (error) *error = "cannot open hero lattice DSL file: " + hero_dsl_path.string();
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

  const auto it_store_root = values.find("store_root");
  if (it_store_root != values.end()) {
    out->store_root = resolve_path_from_folder(
        hero_dsl_path.parent_path().string(), it_store_root->second);
  }
  const auto it_catalog_path = values.find("catalog_path");
  if (it_catalog_path != values.end()) {
    out->catalog_path = resolve_path_from_folder(
        hero_dsl_path.parent_path().string(), it_catalog_path->second);
  }
  const auto it_config_folder = values.find("config_folder");
  if (it_config_folder != values.end()) {
    out->config_folder = resolve_path_from_folder(
        hero_dsl_path.parent_path().string(), it_config_folder->second);
  }
  const auto it_encrypted = values.find("encrypted");
  if (it_encrypted != values.end()) {
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
  return true;
}

bool write_all_fd(int fd, std::string_view bytes) {
  const char* data = bytes.data();
  std::size_t remaining = bytes.size();
  while (remaining > 0) {
    const ssize_t wrote = ::write(fd, data, remaining);
    if (wrote < 0) return false;
    if (wrote == 0) return false;
    data += static_cast<std::size_t>(wrote);
    remaining -= static_cast<std::size_t>(wrote);
  }
  return true;
}

void write_jsonrpc_payload(std::string_view payload) {
  if (g_protocol_stdout_fd < 0) g_protocol_stdout_fd = STDOUT_FILENO;
  if (g_jsonrpc_use_content_length_framing) {
    std::ostringstream framed;
    framed << "Content-Length: " << payload.size() << "\r\n\r\n" << payload;
    (void)write_all_fd(g_protocol_stdout_fd, framed.str());
  } else {
    (void)write_all_fd(g_protocol_stdout_fd, std::string(payload) + "\n");
  }
}

void write_cli_stdout(std::string_view payload) {
  if (g_protocol_stdout_fd < 0) g_protocol_stdout_fd = STDOUT_FILENO;
  (void)write_all_fd(g_protocol_stdout_fd, std::string(payload));
}

void write_jsonrpc_result(std::string_view id_json, std::string_view result_json) {
  std::ostringstream out;
  out << "{\"jsonrpc\":\"2.0\",\"id\":" << id_json
      << ",\"result\":" << result_json << "}";
  write_jsonrpc_payload(out.str());
}

void write_jsonrpc_error(std::string_view id_json, int code,
                         std::string_view message) {
  std::ostringstream out;
  out << "{\"jsonrpc\":\"2.0\",\"id\":" << id_json << ",\"error\":{"
      << "\"code\":" << code << ",\"message\":" << json_quote(message)
      << "}}";
  write_jsonrpc_payload(out.str());
}

[[nodiscard]] std::string ensure_trailing_newline(std::string value) {
  if (!value.empty() && value.back() != '\n') value.push_back('\n');
  return value;
}

[[nodiscard]] std::string build_joined_report_lls(
    std::string_view canonical_path,
    const std::vector<cuwacunu::hero::wave::runtime_report_fragment_t>&
        report_fragments) {
  // DEV_WARNING: joined report_lls is still a synthetic transport, not a
  // strict runtime .lls document. Embedded fragment payloads are strict .lls,
  // but the wrapper metadata here remains legacy human-oriented text.
  std::ostringstream out;
  out << "/* synthetic report_lls transport: "
      << cuwacunu::hashimyei::kJoinedReportSchemaV1 << " */\n";
  out << "report_transport_schema=" << cuwacunu::hashimyei::kJoinedReportSchemaV1
      << "\n";
  out << "canonical_path=" << canonical_path << "\n";
  out << "source_report_fragment_count=" << report_fragments.size() << "\n";
  for (std::size_t i = 0; i < report_fragments.size(); ++i) {
    const auto& fragment = report_fragments[i];
    std::uint64_t wave_cursor = 0;
    (void)extract_wave_cursor_from_payload(fragment.payload_json, &wave_cursor);
    wave_cursor_resolution_e wave_cursor_resolution = wave_cursor_resolution_e::Run;
    (void)extract_wave_cursor_resolution_from_payload(
        fragment.payload_json, &wave_cursor_resolution);
    const std::string wave_cursor_resolution_text =
        wave_cursor_resolution_to_text(wave_cursor_resolution);
    std::string hashimyei_cursor;
    if (!extract_payload_kv(fragment.payload_json, "hashimyei_cursor",
                            &hashimyei_cursor)) {
      hashimyei_cursor = fragment.canonical_path;
    }
    hashimyei_cursor = normalize_source_hashimyei_cursor(hashimyei_cursor);
    const std::string intersection_cursor =
        hashimyei_cursor + "|" + std::to_string(wave_cursor);
    const std::string wave_cursor_view =
        cuwacunu::hero::wave::lattice_catalog_store_t::format_runtime_wave_cursor(
            wave_cursor);
    const std::string intersection_cursor_view =
        hashimyei_cursor + "|" + wave_cursor_view;
    out << "\n/* source_report_fragment[" << i << "] */\n";
    out << "source_report_fragment_index=" << i << "\n";
    out << "source_report_fragment_id=" << fragment.report_fragment_id << "\n";
    out << "schema=" << fragment.schema << "\n";
    out << "run_id=" << fragment.run_id << "\n";
    out << "wave_cursor=" << wave_cursor << "\n";
    out << "wave_cursor_view=" << wave_cursor_view << "\n";
    out << "wave_cursor_resolution=" << wave_cursor_resolution_text << "\n";
    out << "hashimyei_cursor=" << hashimyei_cursor << "\n";
    out << "intersection_cursor=" << intersection_cursor << "\n";
    out << "intersection_cursor_view=" << intersection_cursor_view << "\n";
    out << "hashimyei=" << fragment.hashimyei << "\n";
    out << "ts_ms=" << fragment.ts_ms << "\n";
    out << "path=" << fragment.path << "\n";
    out << "source_report_fragment_sha256=" << fragment.report_fragment_sha256 << "\n";
    out << "payload_begin=1\n";
    out << ensure_trailing_newline(fragment.payload_json);
    out << "payload_end=1\n";
  }
  return out.str();
}

[[nodiscard]] bool reset_lattice_catalog(app_context_t* app,
                                         bool reingest_report_fragments,
                                         std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app) {
    if (out_error) *out_error = "app context is null";
    return false;
  }
  if (app->lattice_catalog_path.empty()) {
    if (out_error) *out_error = "lattice catalog path is empty";
    return false;
  }

  if (app->catalog.opened()) {
    std::string close_error;
    if (!app->catalog.close(&close_error)) {
      if (out_error) *out_error = "catalog close failed: " + close_error;
      return false;
    }
  }

  std::error_code ec{};
  std::filesystem::remove(app->lattice_catalog_path, ec);
  if (ec && std::filesystem::exists(app->lattice_catalog_path)) {
    if (out_error) {
      *out_error = "cannot remove lattice catalog file: " +
                   app->lattice_catalog_path.string();
    }
    return false;
  }

  cuwacunu::hero::wave::lattice_catalog_store_t::options_t opts{};
  opts.catalog_path = app->lattice_catalog_path;
  opts.encrypted = false;
  opts.projection_version = 2;
  if (!app->catalog.open(opts, out_error)) return false;
  if (!reingest_report_fragments) return true;
  return app->catalog.ingest_runtime_report_fragments(app->store_root, out_error);
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

using lattice_tool_handler_t = bool (*)(app_context_t*, const std::string&,
                                        std::string*, std::string*);

struct lattice_tool_descriptor_t {
  const char* name;
  const char* description;
  const char* input_schema_json;
  lattice_tool_handler_t handler;
};

[[nodiscard]] bool handle_tool_list_facts(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error);
[[nodiscard]] bool handle_tool_get_fact(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error);
[[nodiscard]] bool handle_tool_list_views(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error);
[[nodiscard]] bool handle_tool_get_view(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error);
[[nodiscard]] bool handle_tool_refresh(app_context_t* app,
                                       const std::string& arguments_json,
                                       std::string* out_structured,
                                       std::string* out_error);

constexpr lattice_tool_descriptor_t kLatticeTools[] = {
#define HERO_LATTICE_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER) \
  lattice_tool_descriptor_t{NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
#include "hero/lattice_hero/hero_lattice_tools.def"
#undef HERO_LATTICE_TOOL
};

[[nodiscard]] const lattice_tool_descriptor_t* find_lattice_tool_descriptor(
    std::string_view tool_name) {
  for (const auto& descriptor : kLatticeTools) {
    if (tool_name == descriptor.name) return &descriptor;
  }
  return nullptr;
}

[[nodiscard]] std::string initialize_result_json() {
  std::ostringstream out;
  out << "{"
      << "\"protocolVersion\":" << json_quote(kProtocolVersion) << ","
      << "\"serverInfo\":{"
      << "\"name\":" << json_quote(kServerName) << ","
      << "\"version\":" << json_quote(kServerVersion)
      << "},"
      << "\"capabilities\":{\"tools\":{\"listChanged\":false}},"
      << "\"instructions\":" << json_quote(kInitializeInstructions)
      << "}";
  return out.str();
}

struct fact_bundle_summary_t {
  std::string canonical_path{};
  std::uint64_t wave_cursor{0};
  std::uint64_t latest_ts_ms{0};
  std::string latest_run_id{};
  std::size_t fragment_count{0};
};

struct fact_path_summary_t {
  std::string canonical_path{};
  std::uint64_t latest_wave_cursor{0};
  std::uint64_t latest_ts_ms{0};
  std::string latest_run_id{};
  std::size_t fragment_count{0};
  std::unordered_set<std::uint64_t> wave_cursors{};
};

[[nodiscard]] bool build_fact_bundle_summaries(
    std::string_view canonical_path,
    const std::vector<cuwacunu::hero::wave::runtime_report_fragment_t>& rows,
    std::vector<fact_bundle_summary_t>* out) {
  if (!out) return false;
  out->clear();
  std::map<std::uint64_t, fact_bundle_summary_t> by_wave{};
  for (const auto& row : rows) {
    std::uint64_t row_wave_cursor = 0;
    if (!extract_wave_cursor_from_payload(row.payload_json, &row_wave_cursor)) {
      continue;
    }
    auto& summary = by_wave[row_wave_cursor];
    summary.canonical_path = normalize_source_hashimyei_cursor(canonical_path);
    summary.wave_cursor = row_wave_cursor;
    ++summary.fragment_count;
    if (summary.latest_run_id.empty() || row.ts_ms > summary.latest_ts_ms ||
        (row.ts_ms == summary.latest_ts_ms &&
         row.run_id > summary.latest_run_id)) {
      summary.latest_ts_ms = row.ts_ms;
      summary.latest_run_id = row.run_id;
    }
  }
  out->reserve(by_wave.size());
  for (const auto& [_, summary] : by_wave) {
    out->push_back(summary);
  }
  std::sort(out->begin(), out->end(), [](const auto& a, const auto& b) {
    if (a.latest_ts_ms != b.latest_ts_ms) return a.latest_ts_ms > b.latest_ts_ms;
    return a.wave_cursor > b.wave_cursor;
  });
  return true;
}

[[nodiscard]] bool handle_tool_list_facts(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string canonical_path{};
  (void)extract_json_string_field(arguments_json, "canonical_path", &canonical_path);
  canonical_path = normalize_source_hashimyei_cursor(canonical_path);

  std::size_t limit = 20;
  std::size_t offset = 0;
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);

  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> rows{};
  if (!app->catalog.list_runtime_report_fragments(
          canonical_path, "", 0, 0, true, &rows, out_error)) {
    return false;
  }

  std::ostringstream facts_json;
  facts_json << "[";
  std::size_t emitted = 0;
  std::size_t total = 0;

  if (canonical_path.empty()) {
    std::map<std::string, fact_path_summary_t> by_path{};
    for (const auto& row : rows) {
      const std::string row_path =
          normalize_source_hashimyei_cursor(row.canonical_path);
      if (row_path.empty()) continue;
      std::uint64_t row_wave_cursor = 0;
      if (!extract_wave_cursor_from_payload(row.payload_json, &row_wave_cursor)) {
        continue;
      }
      auto& summary = by_path[row_path];
      summary.canonical_path = row_path;
      ++summary.fragment_count;
      summary.wave_cursors.insert(row_wave_cursor);
      if (summary.latest_run_id.empty() || row.ts_ms > summary.latest_ts_ms ||
          (row.ts_ms == summary.latest_ts_ms &&
           row.run_id > summary.latest_run_id)) {
        summary.latest_ts_ms = row.ts_ms;
        summary.latest_run_id = row.run_id;
        summary.latest_wave_cursor = row_wave_cursor;
      }
    }

    std::vector<fact_path_summary_t> facts{};
    facts.reserve(by_path.size());
    for (auto& [_, summary] : by_path) {
      facts.push_back(std::move(summary));
    }
    std::sort(facts.begin(), facts.end(), [](const auto& a, const auto& b) {
      if (a.latest_ts_ms != b.latest_ts_ms) return a.latest_ts_ms > b.latest_ts_ms;
      return a.canonical_path < b.canonical_path;
    });
    total = facts.size();
    const std::size_t off = std::min(offset, facts.size());
    std::size_t count = limit;
    if (count == 0) count = facts.size() - off;
    count = std::min(count, facts.size() - off);
    for (std::size_t i = 0; i < count; ++i) {
      const auto& fact = facts[off + i];
      if (emitted != 0) facts_json << ",";
      ++emitted;
      facts_json << "{"
                 << "\"canonical_path\":" << json_quote(fact.canonical_path)
                 << ",\"latest_wave_cursor\":"
                 << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                                   format_runtime_wave_cursor(
                                       fact.latest_wave_cursor))
                 << ",\"latest_run_id\":" << json_quote(fact.latest_run_id)
                 << ",\"latest_ts_ms\":" << fact.latest_ts_ms
                 << ",\"available_context_count\":" << fact.wave_cursors.size()
                 << ",\"fragment_count\":" << fact.fragment_count << "}";
    }
  } else {
    std::vector<fact_bundle_summary_t> facts{};
    if (!build_fact_bundle_summaries(canonical_path, rows, &facts)) {
      *out_error = "failed to summarize available fact bundles";
      return false;
    }
    total = facts.size();
    const std::size_t off = std::min(offset, facts.size());
    std::size_t count = limit;
    if (count == 0) count = facts.size() - off;
    count = std::min(count, facts.size() - off);
    for (std::size_t i = 0; i < count; ++i) {
      const auto& fact = facts[off + i];
      if (emitted != 0) facts_json << ",";
      ++emitted;
      facts_json << "{"
                 << "\"canonical_path\":" << json_quote(fact.canonical_path)
                 << ",\"wave_cursor\":"
                 << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                                   format_runtime_wave_cursor(fact.wave_cursor))
                 << ",\"latest_run_id\":" << json_quote(fact.latest_run_id)
                 << ",\"latest_ts_ms\":" << fact.latest_ts_ms
                 << ",\"fragment_count\":" << fact.fragment_count << "}";
    }
  }
  facts_json << "]";

  std::ostringstream out;
  out << "{\"canonical_path\":"
      << (canonical_path.empty() ? "null" : json_quote(canonical_path))
      << ",\"count\":" << emitted
      << ",\"total\":" << total
      << ",\"facts\":" << facts_json.str() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_fact(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string canonical_path{};
  (void)extract_json_string_field(arguments_json, "canonical_path",
                                  &canonical_path);
  canonical_path = normalize_source_hashimyei_cursor(canonical_path);
  if (canonical_path.empty()) {
    *out_error = "get_fact requires arguments.canonical_path";
    return false;
  }

  std::uint64_t selected_wave_cursor = 0;
  const bool use_wave_cursor =
      extract_json_wave_cursor_field(arguments_json, "wave_cursor",
                                    &selected_wave_cursor);

  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> rows{};
  if (!app->catalog.list_runtime_report_fragments(canonical_path, "", 0, 0, true,
                                                  &rows, out_error)) {
    return false;
  }
  if (rows.empty()) {
    *out_error = "fact not found for requested canonical_path";
    return false;
  }

  if (!use_wave_cursor) {
    bool found_latest = false;
    for (const auto& row : rows) {
      if (extract_wave_cursor_from_payload(row.payload_json, &selected_wave_cursor)) {
        found_latest = true;
        break;
      }
    }
    if (!found_latest) {
      *out_error = "fact rows do not carry a valid wave_cursor";
      return false;
    }
  }

  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> selected{};
  for (const auto& row : rows) {
    std::uint64_t row_wave_cursor = 0;
    if (!extract_wave_cursor_from_payload(row.payload_json, &row_wave_cursor)) {
      continue;
    }
    if (row_wave_cursor != selected_wave_cursor) continue;
    selected.push_back(row);
  }
  if (selected.empty()) {
    *out_error = "fact bundle not found for requested selector";
    return false;
  }

  const std::string fact_text = build_joined_report_lls(canonical_path, selected);
  std::string latest_run_id = selected.front().run_id;
  std::uint64_t latest_ts_ms = selected.front().ts_ms;
  for (const auto& row : selected) {
    if (row.ts_ms > latest_ts_ms) {
      latest_ts_ms = row.ts_ms;
      latest_run_id = row.run_id;
    }
  }

  std::ostringstream out;
  out << "{\"canonical_path\":" << json_quote(canonical_path)
      << ",\"wave_cursor\":"
      << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                        format_runtime_wave_cursor(selected_wave_cursor))
      << ",\"latest_run_id\":" << json_quote(latest_run_id)
      << ",\"latest_ts_ms\":" << latest_ts_ms
      << ",\"fragment_count\":" << selected.size()
      << ",\"fact_lls\":" << json_quote(fact_text) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_list_views(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();
  (void)arguments_json;

  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> source_rows{};
  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> network_rows{};
  if (!app->catalog.list_runtime_report_fragments(
          "tsi.source.dataloader", "piaabo.torch_compat.data_analytics.v2", 1,
          0, true, &source_rows, out_error)) {
    return false;
  }
  if (!app->catalog.list_runtime_report_fragments(
          "tsi.wikimyei.representation.vicreg",
          "piaabo.torch_compat.network_analytics.v5", 1, 0, true, &network_rows,
          out_error)) {
    return false;
  }

  std::ostringstream out;
  out << "{\"count\":1,\"views\":[{"
      << "\"view_kind\":\"entropic_capacity_comparison\""
      << ",\"preferred_selector\":\"wave_cursor\""
      << ",\"required_selectors\":[\"wave_cursor\"]"
      << ",\"optional_selectors\":[\"canonical_path\",\"contract_hash\"]"
      << ",\"ready\":"
      << ((!source_rows.empty() && !network_rows.empty()) ? "true" : "false")
      << "}]}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_view(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string view_kind{};
  std::string canonical_path{};
  std::string contract_hash{};
  (void)extract_json_string_field(arguments_json, "view_kind", &view_kind);
  (void)extract_json_string_field(arguments_json, "canonical_path",
                                  &canonical_path);
  (void)extract_json_string_field(arguments_json, "contract_hash",
                                  &contract_hash);
  view_kind = trim_ascii(view_kind);
  canonical_path = normalize_source_hashimyei_cursor(canonical_path);
  contract_hash = trim_ascii(contract_hash);
  if (view_kind.empty()) {
    *out_error = "get_view requires arguments.view_kind";
    return false;
  }

  std::uint64_t wave_cursor = 0;
  const bool use_wave_cursor =
      extract_json_wave_cursor_field(arguments_json, "wave_cursor",
                                    &wave_cursor);
  if (!use_wave_cursor) {
    *out_error = "get_view requires arguments.wave_cursor";
    return false;
  }

  std::string internal_intersection_cursor{};
  if (!canonical_path.empty() && use_wave_cursor) {
    internal_intersection_cursor =
        canonical_path + "|" +
        cuwacunu::hero::wave::lattice_catalog_store_t::format_runtime_wave_cursor(
            wave_cursor);
  }

  cuwacunu::hero::wave::runtime_view_report_t view{};
  if (!app->catalog.get_runtime_view_lls(
          view_kind, internal_intersection_cursor, wave_cursor, use_wave_cursor,
          contract_hash, &view, out_error)) {
    return false;
  }

  std::ostringstream out;
  out << "{\"canonical_path\":" << json_quote(view.canonical_path)
      << ",\"view_kind\":" << json_quote(view.view_kind)
      << ",\"selector_canonical_path\":"
      << (view.selector_hashimyei_cursor.empty()
              ? "null"
              : json_quote(view.selector_hashimyei_cursor))
      << ",\"contract_hash\":"
      << (view.contract_hash.empty() ? "null" : json_quote(view.contract_hash))
      << ",\"wave_cursor\":"
      << (view.has_wave_cursor
              ? json_quote(
                    cuwacunu::hero::wave::lattice_catalog_store_t::
                        format_runtime_wave_cursor(view.wave_cursor))
              : "null")
      << ",\"match_count\":" << view.match_count
      << ",\"ambiguity_count\":" << view.ambiguity_count
      << ",\"view_lls\":" << json_quote(view.view_lls) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_refresh(app_context_t* app,
                                       const std::string& arguments_json,
                                       std::string* out_structured,
                                       std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  bool reingest = true;
  (void)extract_json_bool_field(arguments_json, "reingest", &reingest);
  if (!reset_lattice_catalog(app, reingest, out_error)) return false;

  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> report_fragments{};
  if (!app->catalog.list_runtime_report_fragments(
          "", "", 0, 0, true, &report_fragments, out_error)) {
    return false;
  }

  std::ostringstream out;
  out << "{\"catalog_path\":" << json_quote(app->lattice_catalog_path.string())
      << ",\"reingest\":" << (reingest ? "true" : "false")
      << ",\"runtime_report_fragment_count\":" << report_fragments.size()
      << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] std::string build_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < std::size(kLatticeTools); ++i) {
    if (i != 0) out << ",";
    const auto& descriptor = kLatticeTools[i];
    out << "{\"name\":" << json_quote(descriptor.name)
        << ",\"description\":" << json_quote(descriptor.description)
        << ",\"inputSchema\":" << descriptor.input_schema_json << "}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string build_tools_list_human_text() {
  std::ostringstream out;
  for (const auto& descriptor : kLatticeTools) {
    out << descriptor.name << "\t" << descriptor.description << "\n";
  }
  return out.str();
}

[[nodiscard]] bool open_lattice_catalog_if_needed(app_context_t* app,
                                                  std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app) {
    if (out_error) *out_error = "app context is null";
    return false;
  }
  if (app->catalog.opened()) return true;
  if (app->lattice_catalog_path.empty()) {
    if (out_error) *out_error = "lattice catalog path is empty";
    return false;
  }
  cuwacunu::hero::wave::lattice_catalog_store_t::options_t opts{};
  opts.catalog_path = app->lattice_catalog_path;
  opts.encrypted = false;
  opts.projection_version = 2;
  return app->catalog.open(opts, out_error);
}

[[nodiscard]] bool handle_tool_call(app_context_t* app,
                                    const std::string& tool_name,
                                    const std::string& arguments_json,
                                    std::string* out_result_json,
                                    std::string* out_error_json) {
  if (!app || !out_result_json || !out_error_json) return false;
  out_result_json->clear();
  out_error_json->clear();

  const auto* descriptor = find_lattice_tool_descriptor(tool_name);
  if (!descriptor) {
    *out_error_json = "unknown tool: " + tool_name;
    return false;
  }

  std::string structured;
  std::string err;
  const bool ok = descriptor->handler(app, arguments_json, &structured, &err);
  if (!ok) {
    const std::string fallback =
        "{\"canonical_path\":\"\",\"error\":" + json_quote(err) + "}";
    *out_result_json = make_tool_result_json(err, fallback, true);
    return true;
  }

  *out_result_json = make_tool_result_json("ok", structured, false);
  return true;
}

void run_jsonrpc_stdio_loop_impl(app_context_t* app) {
  if (!app) return;

  for (;;) {
    std::string message;
    bool used_content_length = false;
    if (!read_next_jsonrpc_message(&std::cin, &message, &used_content_length)) {
      return;
    }
    if (message.empty()) continue;
    if (used_content_length) g_jsonrpc_use_content_length_framing = true;

    std::string id_json;
    if (!extract_json_id_field(message, &id_json)) {
      write_jsonrpc_error("null", -32700, "invalid JSON-RPC id");
      continue;
    }

    std::string method;
    if (!extract_json_string_field(message, "method", &method)) {
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
      write_jsonrpc_result(id_json, "{}");
      continue;
    }
    if (method == "tools/list") {
      write_jsonrpc_result(id_json, build_tools_list_result_json());
      continue;
    }
    if (method != "tools/call") {
      write_jsonrpc_error(id_json, -32601, "method not found");
      continue;
    }

    std::string params;
    if (!extract_json_object_field(message, "params", &params)) {
      write_jsonrpc_error(id_json, -32602, "tools/call requires params object");
      continue;
    }

    std::string name;
    if (!extract_json_string_field(params, "name", &name) || name.empty()) {
      write_jsonrpc_error(id_json, -32602, "tools/call requires params.name");
      continue;
    }

    std::string arguments = "{}";
    (void)extract_json_object_field(params, "arguments", &arguments);

    std::string open_error;
    if (!open_lattice_catalog_if_needed(app, &open_error)) {
      const std::string err = "catalog open failed: " + open_error;
      const std::string fallback =
          "{\"canonical_path\":\"\",\"error\":" + json_quote(err) + "}";
      write_jsonrpc_result(id_json, make_tool_result_json(err, fallback, true));
      continue;
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

}  // namespace

namespace cuwacunu {
namespace hero {
namespace lattice_mcp {

std::filesystem::path default_store_root() { return ::default_store_root(); }

std::filesystem::path default_catalog_path(
    const std::filesystem::path& store_root) {
  return ::default_catalog_path(store_root);
}

std::filesystem::path resolve_lattice_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  return ::resolve_lattice_hero_dsl_path(global_config_path);
}

bool load_wave_runtime_defaults(const std::filesystem::path& hero_dsl_path,
                                wave_runtime_defaults_t* out,
                                std::string* error) {
  return ::load_wave_runtime_defaults(hero_dsl_path, out, error);
}

bool execute_tool_json(const std::string& tool_name, std::string arguments_json,
                       app_context_t* app,
                       std::string* out_tool_result_json,
                       std::string* out_error_message) {
  if (out_error_message) out_error_message->clear();
  if (!app || !out_tool_result_json || !out_error_message) return false;
  if (!::open_lattice_catalog_if_needed(app, out_error_message)) {
    *out_error_message = "catalog open failed: " + *out_error_message;
    return false;
  }
  return ::handle_tool_call(app, tool_name, arguments_json, out_tool_result_json,
                            out_error_message);
}

bool tool_result_is_error(std::string_view tool_result_json) {
  const std::string json(tool_result_json);
  bool is_error = false;
  return ::extract_json_bool_field(json, "isError", &is_error) && is_error;
}

std::string build_tools_list_result_json() {
  return ::build_tools_list_result_json();
}

std::string build_tools_list_human_text() {
  return ::build_tools_list_human_text();
}

void write_cli_stdout(std::string_view payload) { ::write_cli_stdout(payload); }

void run_jsonrpc_stdio_loop(app_context_t* app) {
  ::run_jsonrpc_stdio_loop_impl(app);
}

}  // namespace lattice_mcp
}  // namespace hero
}  // namespace cuwacunu
