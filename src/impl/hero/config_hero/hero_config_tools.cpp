#include "hero/config_hero/hero_config_tools.h"

#include "hero/config_hero/hero_config.h"
#include "hero/mcp_schema_compat.h"
#include "kikijyeba/protocol/config_provenance.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <openssl/evp.h>
#include <sys/stat.h>
#include <unistd.h>

namespace cuwacunu::hero::config {
namespace {

namespace fs = std::filesystem;

constexpr int kPolicyErrorCode = -32040;

struct tool_descriptor_t {
  const char *name;
  const char *description;
  const char *input_schema_json;
};

constexpr tool_descriptor_t kTools[] = {
    {"hero.config.status",
     "Read-only health: summarize Config Hero policy and global config "
     "state.",
     R"({"type":"object","properties":{},"additionalProperties":false})"},
    {"hero.config.inspect",
     "Read-only inspection: route Config schema, policy values, global config "
     "validation, path provenance, backups, and managed file reads through "
     "subject.",
     R"({"type":"object","properties":{"subject":{"type":"string","enum":["schema","show","value","validate_global_config","map","bundle","resolve_path","diff","backups","file_list","file_read"]},"key":{"type":"string"},"path":{"type":"string"},"config_path":{"type":"string"},"include_content":{"type":"boolean"},"include_machine_payload":{"type":"boolean"},"include_sha256":{"type":"boolean"},"for_write":{"type":"boolean"},"recursive":{"type":"boolean"},"limit":{"type":"integer"}},"required":["subject"],"additionalProperties":false})"},
    {"hero.config.apply",
     "Config mutation: plan or execute policy/file changes through operation. "
     "Only this Config Hero tool mutates.",
     R"({"type":"object","properties":{"operation":{"type":"string","enum":["set","save","reload","rollback","write","delete"]},"requested_mode":{"type":"string","enum":["plan","execute"]},"key":{"type":"string"},"value":{"type":"string"},"path":{"type":"string"},"content":{"type":"string"},"backup_id":{"type":"string"},"expected_sha256":{"type":"string"},"expected_current_sha256":{"type":"string"},"reason":{"type":"string"},"create_backup":{"type":"boolean"},"include_content":{"type":"boolean"}},"required":["operation","requested_mode"],"additionalProperties":false})"},
};

[[nodiscard]] bool tool_is_read_only(std::string_view name) {
  return name == "hero.config.status" || name == "hero.config.inspect";
}

[[nodiscard]] bool tool_is_destructive(std::string_view name) {
  return name == "hero.config.apply";
}

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  std::size_t begin = 0;
  while (begin < in.size() &&
         std::isspace(static_cast<unsigned char>(in[begin])) != 0) {
    ++begin;
  }
  std::size_t end = in.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(in[end - 1])) != 0) {
    --end;
  }
  return std::string(in.substr(begin, end - begin));
}

[[nodiscard]] std::string lowercase_ascii(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] bool parse_bool(std::string_view raw, bool *out) {
  const std::string value = lowercase_ascii(trim_ascii(raw));
  if (value == "true" || value == "1" || value == "yes" || value == "on") {
    if (out) {
      *out = true;
    }
    return true;
  }
  if (value == "false" || value == "0" || value == "no" || value == "off") {
    if (out) {
      *out = false;
    }
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_int(std::string_view raw, int *out) {
  const std::string value = trim_ascii(raw);
  if (value.empty()) {
    return false;
  }
  int parsed = 0;
  const auto result =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) {
    return false;
  }
  if (out) {
    *out = parsed;
  }
  return true;
}

[[nodiscard]] std::vector<std::string> split_csv(std::string_view raw) {
  std::vector<std::string> out;
  std::string item;
  std::istringstream in{std::string(raw)};
  while (std::getline(in, item, ',')) {
    item = trim_ascii(item);
    if (!item.empty()) {
      out.push_back(item);
    }
  }
  return out;
}

[[nodiscard]] std::string json_quote(std::string_view s) {
  std::ostringstream out;
  out << '"';
  for (const unsigned char c : s) {
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
        out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
            << static_cast<int>(c) << std::dec << std::setfill(' ');
      } else {
        out << static_cast<char>(c);
      }
      break;
    }
  }
  out << '"';
  return out.str();
}

[[nodiscard]] const char *bool_json(bool value) {
  return value ? "true" : "false";
}

void skip_ws(const std::string &s, std::size_t *idx) {
  while (*idx < s.size() &&
         std::isspace(static_cast<unsigned char>(s[*idx])) != 0) {
    ++(*idx);
  }
}

[[nodiscard]] bool parse_json_string_token(const std::string &s,
                                           std::size_t *idx, std::string *out) {
  if (*idx >= s.size() || s[*idx] != '"') {
    return false;
  }
  ++(*idx);
  std::string value;
  while (*idx < s.size()) {
    const char c = s[(*idx)++];
    if (c == '"') {
      if (out) {
        *out = std::move(value);
      }
      return true;
    }
    if (c != '\\') {
      if (static_cast<unsigned char>(c) < 0x20) {
        return false;
      }
      value.push_back(c);
      continue;
    }
    if (*idx >= s.size()) {
      return false;
    }
    const char escaped = s[(*idx)++];
    switch (escaped) {
    case '"':
    case '\\':
    case '/':
      value.push_back(escaped);
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
    case 'u':
      if (*idx + 4 > s.size()) {
        return false;
      }
      value.append("\\u");
      value.append(s.substr(*idx, 4));
      *idx += 4;
      break;
    default:
      return false;
    }
  }
  return false;
}

[[nodiscard]] bool skip_json_value(const std::string &s, std::size_t *idx) {
  skip_ws(s, idx);
  if (*idx >= s.size()) {
    return false;
  }
  if (s[*idx] == '"') {
    return parse_json_string_token(s, idx, nullptr);
  }
  if (s[*idx] == '{' || s[*idx] == '[') {
    std::vector<char> stack;
    stack.push_back(s[*idx]);
    ++(*idx);
    bool in_string = false;
    bool escape = false;
    while (*idx < s.size()) {
      const char c = s[(*idx)++];
      if (in_string) {
        if (escape) {
          escape = false;
        } else if (c == '\\') {
          escape = true;
        } else if (c == '"') {
          in_string = false;
        }
        continue;
      }
      if (c == '"') {
        in_string = true;
      } else if (c == '{' || c == '[') {
        stack.push_back(c);
      } else if (c == '}') {
        if (stack.empty() || stack.back() != '{') {
          return false;
        }
        stack.pop_back();
      } else if (c == ']') {
        if (stack.empty() || stack.back() != '[') {
          return false;
        }
        stack.pop_back();
      }
      if (stack.empty()) {
        return true;
      }
    }
    return false;
  }
  const std::size_t begin = *idx;
  while (*idx < s.size()) {
    const char c = s[*idx];
    if (c == ',' || c == '}' || c == ']' ||
        std::isspace(static_cast<unsigned char>(c)) != 0) {
      break;
    }
    ++(*idx);
  }
  return *idx > begin;
}

[[nodiscard]] bool extract_json_raw_field(const std::string &json,
                                          std::string_view key,
                                          std::string *out) {
  std::size_t idx = 0;
  skip_ws(json, &idx);
  if (idx >= json.size() || json[idx] != '{') {
    return false;
  }
  ++idx;
  while (idx < json.size()) {
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == '}') {
      return false;
    }
    std::string current_key;
    if (!parse_json_string_token(json, &idx, &current_key)) {
      return false;
    }
    skip_ws(json, &idx);
    if (idx >= json.size() || json[idx] != ':') {
      return false;
    }
    ++idx;
    skip_ws(json, &idx);
    const std::size_t value_begin = idx;
    if (!skip_json_value(json, &idx)) {
      return false;
    }
    if (current_key == key) {
      if (out) {
        *out = trim_ascii(
            std::string_view(json).substr(value_begin, idx - value_begin));
      }
      return true;
    }
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == ',') {
      ++idx;
      continue;
    }
    if (idx < json.size() && json[idx] == '}') {
      return false;
    }
  }
  return false;
}

[[nodiscard]] bool extract_json_string_field(const std::string &json,
                                             std::string_view key,
                                             std::string *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) {
    return false;
  }
  std::size_t idx = 0;
  skip_ws(raw, &idx);
  std::string value;
  if (!parse_json_string_token(raw, &idx, &value)) {
    return false;
  }
  skip_ws(raw, &idx);
  if (idx != raw.size()) {
    return false;
  }
  if (out) {
    *out = std::move(value);
  }
  return true;
}

[[nodiscard]] bool extract_json_bool_field(const std::string &json,
                                           std::string_view key, bool *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) {
    return false;
  }
  const std::string value = trim_ascii(raw);
  if (value == "true") {
    if (out) {
      *out = true;
    }
    return true;
  }
  if (value == "false") {
    if (out) {
      *out = false;
    }
    return true;
  }
  return false;
}

[[nodiscard]] bool extract_json_int_field(const std::string &json,
                                          std::string_view key, int *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) {
    return false;
  }
  int value = 0;
  if (!parse_int(raw, &value)) {
    return false;
  }
  if (out) {
    *out = value;
  }
  return true;
}

struct json_field_t {
  std::string key;
  std::string raw_value;
};

[[nodiscard]] bool parse_json_object_fields(const std::string &json,
                                            std::vector<json_field_t> *out,
                                            std::string *err) {
  if (out) {
    out->clear();
  }
  std::size_t idx = 0;
  skip_ws(json, &idx);
  if (idx >= json.size() || json[idx] != '{') {
    if (err) {
      *err = "arguments must be a JSON object";
    }
    return false;
  }
  ++idx;
  skip_ws(json, &idx);
  if (idx < json.size() && json[idx] == '}') {
    ++idx;
    skip_ws(json, &idx);
    if (idx != json.size()) {
      if (err) {
        *err = "unexpected trailing content after JSON object";
      }
      return false;
    }
    return true;
  }
  while (idx < json.size()) {
    std::string key;
    if (!parse_json_string_token(json, &idx, &key)) {
      if (err) {
        *err = "expected JSON object key";
      }
      return false;
    }
    if (out) {
      for (const auto &field : *out) {
        if (field.key == key) {
          if (err) {
            *err = "duplicate field: " + key;
          }
          return false;
        }
      }
    }
    skip_ws(json, &idx);
    if (idx >= json.size() || json[idx] != ':') {
      if (err) {
        *err = "expected ':' after JSON object key";
      }
      return false;
    }
    ++idx;
    skip_ws(json, &idx);
    const std::size_t value_begin = idx;
    if (!skip_json_value(json, &idx)) {
      if (err) {
        *err = "invalid JSON value for field: " + key;
      }
      return false;
    }
    if (out) {
      out->push_back({key, trim_ascii(std::string_view(json).substr(
                               value_begin, idx - value_begin))});
    }
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == ',') {
      ++idx;
      skip_ws(json, &idx);
      continue;
    }
    if (idx < json.size() && json[idx] == '}') {
      ++idx;
      skip_ws(json, &idx);
      if (idx != json.size()) {
        if (err) {
          *err = "unexpected trailing content after JSON object";
        }
        return false;
      }
      return true;
    }
    if (err) {
      *err = "expected ',' or '}' in JSON object";
    }
    return false;
  }
  if (err) {
    *err = "unterminated JSON object";
  }
  return false;
}

[[nodiscard]] bool raw_field(const std::vector<json_field_t> &fields,
                             std::string_view key, std::string *out) {
  for (const auto &field : fields) {
    if (field.key == key) {
      if (out) {
        *out = field.raw_value;
      }
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool has_field(const std::vector<json_field_t> &fields,
                             std::string_view key) {
  return raw_field(fields, key, nullptr);
}

[[nodiscard]] bool
field_allowed(std::string_view key,
              std::initializer_list<std::string_view> allowed) {
  return std::find(allowed.begin(), allowed.end(), key) != allowed.end();
}

[[nodiscard]] bool
validate_allowed_fields(const std::vector<json_field_t> &fields,
                        std::initializer_list<std::string_view> allowed,
                        std::string *err) {
  for (const auto &field : fields) {
    if (!field_allowed(field.key, allowed)) {
      if (err) {
        *err = "unknown field: " + field.key;
      }
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool validate_optional_bool_field(const std::string &args,
                                                std::string_view key,
                                                std::string *err) {
  if (!extract_json_raw_field(args, key, nullptr)) {
    return true;
  }
  bool ignored = false;
  if (!extract_json_bool_field(args, key, &ignored)) {
    if (err) {
      *err = std::string(key) + " must be boolean";
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool optional_bool_value(const std::string &args,
                                       std::string_view key, bool fallback,
                                       std::string *err) {
  if (!extract_json_raw_field(args, key, nullptr)) {
    return fallback;
  }
  bool value = fallback;
  if (!extract_json_bool_field(args, key, &value)) {
    if (err) {
      *err = std::string(key) + " must be boolean";
    }
    return fallback;
  }
  return value;
}

void append_raw_member(std::ostringstream *out, bool *has_any,
                       const std::vector<json_field_t> &fields,
                       std::string_view source_key,
                       std::string_view output_key = {}) {
  std::string raw;
  if (!raw_field(fields, source_key, &raw)) {
    return;
  }
  if (*has_any) {
    *out << ",";
  }
  *out << json_quote(output_key.empty() ? source_key : output_key) << ":"
       << raw;
  *has_any = true;
}

[[nodiscard]] std::string object_with_fields(
    const std::vector<json_field_t> &fields,
    std::initializer_list<std::pair<std::string_view, std::string_view>>
        field_map) {
  std::ostringstream out;
  bool has_any = false;
  out << "{";
  for (const auto &[source_key, output_key] : field_map) {
    append_raw_member(&out, &has_any, fields, source_key, output_key);
  }
  out << "}";
  return out.str();
}

void append_literal_member(std::ostringstream *out, bool *has_any,
                           std::string_view key, std::string_view raw_value) {
  if (*has_any) {
    *out << ",";
  }
  *out << json_quote(key) << ":" << raw_value;
  *has_any = true;
}

[[nodiscard]] fs::path normalize_path(const fs::path &path) {
  std::error_code ec;
  const fs::path canonical = fs::weakly_canonical(path, ec);
  return ec ? path.lexically_normal() : canonical;
}

[[nodiscard]] fs::path resolve_against_root(std::string_view raw,
                                            const fs::path &root) {
  fs::path path(trim_ascii(raw));
  if (path.empty()) {
    return path;
  }
  if (path.is_absolute()) {
    return normalize_path(path);
  }
  return normalize_path(root / path);
}

[[nodiscard]] bool path_within(const fs::path &root, const fs::path &path) {
  const fs::path norm_root = normalize_path(root);
  const fs::path norm_path = normalize_path(path);
  auto root_it = norm_root.begin();
  auto path_it = norm_path.begin();
  for (; root_it != norm_root.end(); ++root_it, ++path_it) {
    if (path_it == norm_path.end() || *root_it != *path_it) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::uint64_t now_ms_utc() {
  const auto now = std::chrono::system_clock::now();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now.time_since_epoch())
          .count());
}

[[nodiscard]] std::string sanitize_filename_component(std::string in) {
  if (in.empty()) {
    return "config";
  }
  for (char &c : in) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
    if (!ok) {
      c = '_';
    }
  }
  return in;
}

[[nodiscard]] bool sha256_hex_text(std::string_view text, std::string *out,
                                   std::string *err) {
  if (!out) {
    if (err) {
      *err = "sha256 output pointer is null";
    }
    return false;
  }
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx) {
    if (err) {
      *err = "cannot allocate EVP context";
    }
    return false;
  }
  if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
      (!text.empty() && EVP_DigestUpdate(ctx, text.data(), text.size()) != 1)) {
    EVP_MD_CTX_free(ctx);
    if (err) {
      *err = "EVP sha256 update failed";
    }
    return false;
  }
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
    EVP_MD_CTX_free(ctx);
    if (err) {
      *err = "EVP sha256 final failed";
    }
    return false;
  }
  EVP_MD_CTX_free(ctx);
  static constexpr std::array<char, 16> kHex{'0', '1', '2', '3', '4', '5',
                                             '6', '7', '8', '9', 'a', 'b',
                                             'c', 'd', 'e', 'f'};
  out->resize(static_cast<std::size_t>(digest_len) * 2);
  for (std::size_t i = 0; i < static_cast<std::size_t>(digest_len); ++i) {
    (*out)[i * 2] = kHex[(digest[i] >> 4) & 0x0F];
    (*out)[i * 2 + 1] = kHex[digest[i] & 0x0F];
  }
  return true;
}

[[nodiscard]] bool read_text_file(const fs::path &path, std::string *out,
                                  std::string *err) {
  if (!out) {
    if (err) {
      *err = "read output pointer is null";
    }
    return false;
  }
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (err) {
      *err = "cannot open file: " + path.string();
    }
    return false;
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  if (in.bad()) {
    if (err) {
      *err = "cannot read file: " + path.string();
    }
    return false;
  }
  *out = buffer.str();
  return true;
}

[[nodiscard]] bool sha256_hex_file(const fs::path &path, std::string *out,
                                   std::string *err) {
  std::string text;
  if (!read_text_file(path, &text, err)) {
    return false;
  }
  return sha256_hex_text(text, out, err);
}

[[nodiscard]] bool write_text_file_atomic(const fs::path &path,
                                          std::string_view text,
                                          std::string *err) {
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  if (ec) {
    if (err) {
      *err = "cannot create parent directory for " + path.string() + ": " +
             ec.message();
    }
    return false;
  }
  const fs::path tmp = path.string() + ".tmp." + std::to_string(::getpid());
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) {
      if (err) {
        *err = "cannot open temp file for write: " + tmp.string();
      }
      return false;
    }
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    out.flush();
    if (!out) {
      std::error_code ignored;
      fs::remove(tmp, ignored);
      if (err) {
        *err = "cannot write temp file: " + tmp.string();
      }
      return false;
    }
  }
  (void)::chmod(tmp.c_str(), S_IRUSR | S_IWUSR);
  fs::rename(tmp, path, ec);
  if (ec) {
    std::error_code ignored;
    fs::remove(tmp, ignored);
    if (err) {
      *err =
          "cannot publish temp file to " + path.string() + ": " + ec.message();
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool backup_file(const hero_config_store_t &store,
                               const fs::path &source, std::string *err) {
  bool enabled = true;
  (void)parse_bool(store.get_or_default("backup_enabled"), &enabled);
  if (!enabled) {
    return true;
  }
  std::error_code ec;
  if (!fs::exists(source, ec)) {
    return true;
  }
  if (ec) {
    if (err) {
      *err = "cannot inspect file for backup: " + source.string() + ": " +
             ec.message();
    }
    return false;
  }
  int max_entries = 20;
  (void)parse_int(store.get_or_default("backup_max_entries"), &max_entries);
  const fs::path backup_root =
      resolve_against_root(store.get_or_default("backup_dir"),
                           fs::path(store.config_path()).parent_path()) /
      "files";
  fs::create_directories(backup_root, ec);
  if (ec) {
    if (err) {
      *err = "cannot create backup directory " + backup_root.string() + ": " +
             ec.message();
    }
    return false;
  }
  const std::string leaf = std::to_string(now_ms_utc()) + "--" +
                           sanitize_filename_component(source.string()) +
                           ".bak";
  const fs::path backup_path = backup_root / leaf;
  fs::copy_file(source, backup_path, fs::copy_options::overwrite_existing, ec);
  if (ec) {
    if (err) {
      *err =
          "cannot write backup " + backup_path.string() + ": " + ec.message();
    }
    return false;
  }
  (void)::chmod(backup_path.c_str(), S_IRUSR | S_IWUSR);

  if (max_entries < 1) {
    return true;
  }
  std::vector<fs::directory_entry> backups;
  for (fs::directory_iterator it(backup_root, ec), end; !ec && it != end;
       it.increment(ec)) {
    if (it->is_regular_file(ec) && it->path().extension() == ".bak") {
      backups.push_back(*it);
    }
  }
  if (!ec) {
    std::sort(backups.begin(), backups.end(),
              [](const fs::directory_entry &a, const fs::directory_entry &b) {
                return a.path().filename().string() >
                       b.path().filename().string();
              });
    for (std::size_t i = static_cast<std::size_t>(max_entries);
         i < backups.size(); ++i) {
      std::error_code ignored;
      fs::remove(backups[i].path(), ignored);
    }
  }
  return true;
}

[[nodiscard]] fs::path config_root(const hero_config_store_t &store) {
  return resolve_against_root(store.get_or_default("config_root"),
                              fs::path(store.config_path()).parent_path());
}

[[nodiscard]] bool policy_bool_or(const hero_config_store_t &store,
                                  std::string_view key, bool fallback) {
  bool parsed = fallback;
  (void)parse_bool(store.get_or_default(key), &parsed);
  return parsed;
}

[[nodiscard]] std::optional<std::string>
relative_to_config_root(const hero_config_store_t &store,
                        const fs::path &path) {
  const fs::path root = config_root(store);
  const fs::path normalized = normalize_path(path);
  if (!path_within(root, normalized)) {
    return std::nullopt;
  }
  std::error_code ec;
  fs::path relative = fs::relative(normalized, root, ec);
  if (ec) {
    relative = normalized.lexically_relative(root);
  }
  if (relative.empty()) {
    return std::string(".");
  }
  return relative.string();
}

[[nodiscard]] std::string
json_optional_string(const std::optional<std::string> &value) {
  if (!value.has_value()) {
    return "null";
  }
  return json_quote(*value);
}

[[nodiscard]] bool extension_allowed(const hero_config_store_t &store,
                                     const fs::path &path) {
  std::string ext = path.extension().string();
  if (ext.empty()) {
    const std::string filename = path.filename().string();
    if (filename.size() > 1 && filename.front() == '.') {
      ext = filename;
    }
  }
  if (ext.empty()) {
    return false;
  }
  for (const std::string &allowed :
       split_csv(store.get_or_default("allowed_extensions"))) {
    if (ext == allowed) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool under_any_root(const fs::path &path,
                                  const std::vector<std::string> &roots,
                                  const fs::path &base) {
  for (const std::string &raw : roots) {
    const fs::path root = resolve_against_root(raw, base);
    if (path_within(root, path)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool resolve_managed_path(const hero_config_store_t &store,
                                        std::string_view raw, bool for_write,
                                        fs::path *out, std::string *err,
                                        bool require_allowed_extension = true) {
  const fs::path base = config_root(store);
  const fs::path path = resolve_against_root(raw, base);
  if (path.empty()) {
    if (err) {
      *err = "path is required";
    }
    return false;
  }
  if (require_allowed_extension && !extension_allowed(store, path)) {
    if (err) {
      *err = "path extension is not allowed: " + path.string();
    }
    return false;
  }
  if (!under_any_root(path, split_csv(store.get_or_default("managed_roots")),
                      fs::path(store.config_path()).parent_path())) {
    if (err) {
      *err = "path is outside managed_roots: " + path.string();
    }
    return false;
  }
  bool allowed = true;
  if (for_write) {
    if (!parse_bool(store.get_or_default("allow_write"), &allowed) ||
        !allowed) {
      if (err) {
        *err = "E_CONFIG_WRITE_DENIED: allow_write is false";
      }
      return false;
    }
    if (!under_any_root(path, split_csv(store.get_or_default("write_roots")),
                        fs::path(store.config_path()).parent_path())) {
      if (err) {
        *err = "E_CONFIG_WRITE_DENIED: path is outside write_roots: " +
               path.string();
      }
      return false;
    }
  } else {
    if (!parse_bool(store.get_or_default("allow_read"), &allowed) || !allowed) {
      if (err) {
        *err = "E_CONFIG_READ_DENIED: allow_read is false";
      }
      return false;
    }
  }
  if (out) {
    *out = path;
  }
  return true;
}

[[nodiscard]] std::string
build_resolve_path_json(const hero_config_store_t &store, std::string_view raw,
                        bool for_write, bool include_sha256) {
  const std::string input = trim_ascii(raw);
  const fs::path base = config_root(store);
  const fs::path path =
      input.empty() ? fs::path() : resolve_against_root(input, base);
  const fs::path policy_base = fs::path(store.config_path()).parent_path();
  const bool within_managed =
      !path.empty() &&
      under_any_root(path, split_csv(store.get_or_default("managed_roots")),
                     policy_base);
  const bool within_write_roots =
      !path.empty() &&
      under_any_root(path, split_csv(store.get_or_default("write_roots")),
                     policy_base);
  const bool allow_read = policy_bool_or(store, "allow_read", true);
  const bool allow_write = policy_bool_or(store, "allow_write", false);
  const bool extension_ok = !path.empty() && extension_allowed(store, path);

  std::error_code ec;
  const bool exists = !path.empty() && fs::exists(path, ec) && !ec;
  std::error_code type_ec;
  const bool is_regular_file =
      exists && fs::is_regular_file(path, type_ec) && !type_ec;
  type_ec.clear();
  const bool is_directory =
      exists && fs::is_directory(path, type_ec) && !type_ec;
  const bool extension_gate_ok = extension_ok || is_directory;
  const bool read_allowed = allow_read && within_managed;
  const bool write_allowed =
      allow_write && within_managed && within_write_roots;

  std::string reason = "ok";
  bool decision_ok = true;
  if (input.empty()) {
    decision_ok = false;
    reason = "path is required";
  } else if (!within_managed) {
    decision_ok = false;
    reason = "path is outside managed_roots: " + path.string();
  } else if (!extension_gate_ok) {
    decision_ok = false;
    reason = "path extension is not allowed: " + path.string();
  } else if (for_write && !allow_write) {
    decision_ok = false;
    reason = "E_CONFIG_WRITE_DENIED: allow_write is false";
  } else if (for_write && !within_write_roots) {
    decision_ok = false;
    reason =
        "E_CONFIG_WRITE_DENIED: path is outside write_roots: " + path.string();
  } else if (!for_write && !allow_read) {
    decision_ok = false;
    reason = "E_CONFIG_READ_DENIED: allow_read is false";
  }

  std::uintmax_t size = 0;
  bool size_known = false;
  if (is_regular_file) {
    std::error_code size_ec;
    size = fs::file_size(path, size_ec);
    size_known = !size_ec;
  }
  std::string sha;
  std::string sha_error;
  if (include_sha256 && is_regular_file) {
    (void)sha256_hex_file(path, &sha, &sha_error);
  }

  const std::optional<std::string> relative_path =
      path.empty() ? std::nullopt : relative_to_config_root(store, path);
  std::ostringstream out;
  out << "{\"input_path\":" << json_quote(input)
      << ",\"path\":" << json_quote(path.string())
      << ",\"relative_path\":" << json_optional_string(relative_path)
      << ",\"for_write\":" << bool_json(for_write)
      << ",\"exists\":" << bool_json(exists)
      << ",\"is_regular_file\":" << bool_json(is_regular_file)
      << ",\"is_directory\":" << bool_json(is_directory) << ",\"size\":";
  if (size_known) {
    out << size;
  } else {
    out << "null";
  }
  out << ",\"extension_allowed\":" << bool_json(extension_ok)
      << ",\"within_managed_roots\":" << bool_json(within_managed)
      << ",\"within_write_roots\":" << bool_json(within_write_roots)
      << ",\"allow_read\":" << bool_json(allow_read)
      << ",\"allow_write\":" << bool_json(allow_write)
      << ",\"read_allowed\":" << bool_json(read_allowed)
      << ",\"write_allowed\":" << bool_json(write_allowed);
  if (!sha.empty()) {
    out << ",\"sha256\":" << json_quote(sha);
  }
  if (!sha_error.empty()) {
    out << ",\"sha256_error\":" << json_quote(sha_error);
  }
  out << ",\"decision\":{\"ok\":" << bool_json(decision_ok)
      << ",\"reason\":" << json_quote(reason) << "}}";
  return out.str();
}

[[nodiscard]] std::string make_tool_result(std::string_view tool_name,
                                           std::string_view structured_json) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":"
      << json_quote(std::string(tool_name) + " executed")
      << "}],\"structuredContent\":" << structured_json
      << ",\"isError\":false}";
  return out.str();
}

[[nodiscard]] std::string make_error_result(std::string_view message,
                                            int code) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":" << json_quote(message)
      << "}],\"structuredContent\":{\"ok\":false,\"error_code\":" << code
      << ",\"message\":" << json_quote(message) << "},\"isError\":true}";
  return out.str();
}

[[nodiscard]] std::string build_status_json(const hero_config_store_t &store) {
  const auto errors = store.validate();
  std::ostringstream out;
  out << "{\"valid\":" << bool_json(errors.empty())
      << ",\"config_path\":" << json_quote(store.config_path())
      << ",\"global_config_path\":" << json_quote(store.global_config_path())
      << ",\"dirty\":" << bool_json(store.dirty())
      << ",\"from_template\":" << bool_json(store.from_template())
      << ",\"protocol_layer\":"
      << json_quote(store.get_or_default("protocol_layer"))
      << ",\"config_root\":" << json_quote(config_root(store).string())
      << ",\"errors\":[";
  for (std::size_t i = 0; i < errors.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(errors[i]);
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string
build_preview_json(const hero_config_store_t::save_preview_t &preview,
                   bool include_text) {
  std::ostringstream out;
  out << "{\"file_exists\":" << bool_json(preview.file_exists)
      << ",\"text_changed\":" << bool_json(preview.text_changed)
      << ",\"has_changes\":" << bool_json(preview.has_changes)
      << ",\"diff_count\":" << preview.diff_count << ",\"diffs\":[";
  for (std::size_t i = 0; i < preview.diffs.size(); ++i) {
    const auto &d = preview.diffs[i];
    if (i != 0) {
      out << ",";
    }
    out << "{\"key\":" << json_quote(d.key)
        << ",\"action\":" << json_quote(d.action)
        << ",\"before_value\":" << json_quote(d.before_value)
        << ",\"after_value\":" << json_quote(d.after_value) << "}";
  }
  out << "]";
  if (include_text) {
    out << ",\"current_text\":" << json_quote(preview.current_text)
        << ",\"proposed_text\":" << json_quote(preview.proposed_text);
  }
  out << "}";
  return out.str();
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
    if (!in_single && !in_double && (c == '#' || c == ';')) {
      break;
    }
    out.push_back(c);
  }
  return out;
}

[[nodiscard]] bool key_has_suffix(std::string_view key,
                                  std::string_view suffix) {
  return key.size() >= suffix.size() &&
         key.substr(key.size() - suffix.size()) == suffix;
}

struct global_config_reference_t {
  std::string key;
  fs::path path;
  bool exists = false;
  bool is_regular_file = false;
  bool is_directory = false;
  bool size_known = false;
  std::uintmax_t size = 0;
  std::string sha256;
  std::string sha256_error;
};

struct global_config_map_t {
  bool readable = false;
  std::string error;
  std::vector<global_config_reference_t> entries;
  std::vector<std::string> missing;
};

[[nodiscard]] global_config_map_t
collect_global_config_map(const hero_config_store_t &store,
                          bool include_sha256) {
  global_config_map_t map;
  std::ifstream in(store.global_config_path());
  if (!in) {
    map.error = "cannot open " + store.global_config_path();
    map.missing.push_back(map.error);
    return map;
  }
  map.readable = true;
  std::string section;
  std::string line;
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(strip_ini_comment(line));
    if (trimmed.empty()) {
      continue;
    }
    if (trimmed.size() >= 2 && trimmed.front() == '[' &&
        trimmed.back() == ']') {
      section = trim_ascii(trimmed.substr(1, trimmed.size() - 2));
      continue;
    }
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const std::string key = trim_ascii(trimmed.substr(0, eq));
    const std::string value = trim_ascii(trimmed.substr(eq + 1));
    if (!key_has_suffix(key, "_path") && !key_has_suffix(key, "_filename")) {
      continue;
    }

    fs::path path(value);
    if (!path.is_absolute()) {
      path = fs::path(store.global_config_path()).parent_path() / path;
    }
    path = normalize_path(path);

    global_config_reference_t entry;
    entry.key = section.empty() ? key : section + "." + key;
    entry.path = path;
    std::error_code ec;
    entry.exists = fs::exists(path, ec) && !ec;
    if (entry.exists) {
      std::error_code type_ec;
      entry.is_regular_file = fs::is_regular_file(path, type_ec) && !type_ec;
      entry.is_directory = fs::is_directory(path, type_ec) && !type_ec;
      if (entry.is_regular_file) {
        std::error_code size_ec;
        entry.size = fs::file_size(path, size_ec);
        entry.size_known = !size_ec;
        if (include_sha256) {
          std::string sha_error;
          if (!sha256_hex_file(path, &entry.sha256, &sha_error)) {
            entry.sha256_error = sha_error;
          }
        }
      }
    } else {
      map.missing.push_back(entry.key + "=" + path.string());
    }
    map.entries.push_back(std::move(entry));
  }
  return map;
}

[[nodiscard]] std::string
build_global_config_map_json(const hero_config_store_t &store,
                             bool include_sha256, bool *valid) {
  const global_config_map_t map =
      collect_global_config_map(store, include_sha256);
  if (valid) {
    *valid = map.readable && map.missing.empty();
  }
  std::ostringstream out;
  out << "{\"readable\":" << bool_json(map.readable)
      << ",\"global_config_path\":" << json_quote(store.global_config_path());
  if (!map.error.empty()) {
    out << ",\"error\":" << json_quote(map.error);
  }
  out << ",\"count\":" << map.entries.size() << ",\"entries\":[";
  for (std::size_t i = 0; i < map.entries.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    const auto &entry = map.entries[i];
    out << "{\"key\":" << json_quote(entry.key)
        << ",\"path\":" << json_quote(entry.path.string())
        << ",\"relative_path\":"
        << json_optional_string(relative_to_config_root(store, entry.path))
        << ",\"exists\":" << bool_json(entry.exists)
        << ",\"is_regular_file\":" << bool_json(entry.is_regular_file)
        << ",\"is_directory\":" << bool_json(entry.is_directory)
        << ",\"size\":";
    if (entry.size_known) {
      out << entry.size;
    } else {
      out << "null";
    }
    if (include_sha256 && !entry.sha256.empty()) {
      out << ",\"sha256\":" << json_quote(entry.sha256);
    }
    if (include_sha256 && !entry.sha256_error.empty()) {
      out << ",\"sha256_error\":" << json_quote(entry.sha256_error);
    }
    out << "}";
  }
  out << "],\"missing\":[";
  for (std::size_t i = 0; i < map.missing.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(map.missing[i]);
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string config_bundle_receipt_json(
    const cuwacunu::kikijyeba::protocol::config_provenance::
        config_bundle_receipt_t &receipt,
    bool include_text) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(receipt.schema)
      << ",\"global_config_path\":"
      << json_quote(receipt.global_config_path.string())
      << ",\"config_bundle_id\":" << json_quote(receipt.config_bundle_id)
      << ",\"config_receipt_id\":" << json_quote(receipt.config_receipt_id)
      << ",\"captured_at_utc\":" << json_quote(receipt.captured_at_utc)
      << ",\"complete\":" << bool_json(receipt.complete)
      << ",\"file_count\":" << receipt.files.size() << ",\"files\":[";
  for (std::size_t i = 0; i < receipt.files.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    const auto &file = receipt.files[i];
    out << "{\"config_key\":" << json_quote(file.config_key)
        << ",\"path\":" << json_quote(file.path.string())
        << ",\"exists\":" << bool_json(file.exists)
        << ",\"is_regular_file\":" << bool_json(file.is_regular_file)
        << ",\"size_known\":" << bool_json(file.size_known) << ",\"size\":";
    if (file.size_known) {
      out << file.size;
    } else {
      out << "null";
    }
    out << ",\"content_digest\":" << json_quote(file.content_digest)
        << ",\"error\":" << json_quote(file.error) << "}";
  }
  out << "],\"issues\":[";
  for (std::size_t i = 0; i < receipt.issues.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(receipt.issues[i]);
  }
  out << "]";
  if (include_text) {
    out << ",\"receipt_text\":"
        << json_quote(cuwacunu::kikijyeba::protocol::config_provenance::
                          canonical_config_bundle_receipt_text(receipt));
  }
  out << "}";
  return out.str();
}

[[nodiscard]] bool handle_status(const std::string &,
                                 hero_config_store_t *store, std::string *out,
                                 std::string *) {
  *out = build_status_json(*store);
  return true;
}

[[nodiscard]] bool handle_schema(const std::string &, hero_config_store_t *,
                                 std::string *out, std::string *) {
  std::ostringstream json;
  json << "{\"keys\":[";
  for (std::size_t i = 0; i < kRuntimeKeyDescriptors.size(); ++i) {
    const auto &d = kRuntimeKeyDescriptors[i];
    if (i != 0) {
      json << ",";
    }
    json << "{\"key\":" << json_quote(d.key)
         << ",\"type\":" << json_quote(d.type)
         << ",\"required\":" << bool_json(d.required)
         << ",\"default\":" << json_quote(d.default_value)
         << ",\"range\":" << json_quote(d.range)
         << ",\"allowed\":" << json_quote(d.allowed)
         << ",\"summary\":" << json_quote(d.summary) << "}";
  }
  json << "]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_show(const std::string &, hero_config_store_t *store,
                               std::string *out, std::string *) {
  const auto entries = store->entries_snapshot();
  std::ostringstream json;
  json << "{\"entries\":[";
  for (std::size_t i = 0; i < entries.size(); ++i) {
    if (i != 0) {
      json << ",";
    }
    json << "{\"key\":" << json_quote(entries[i].key)
         << ",\"domain\":" << json_quote(entries[i].declared_domain)
         << ",\"type\":" << json_quote(entries[i].declared_type)
         << ",\"value\":" << json_quote(entries[i].value) << "}";
  }
  json << "]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_get(const std::string &args,
                              hero_config_store_t *store, std::string *out,
                              std::string *err) {
  std::string key;
  if (!extract_json_string_field(args, "key", &key) || key.empty()) {
    *err = "hero.config.inspect subject=value requires string argument key";
    return false;
  }
  const auto value = store->get_value(key);
  if (!value.has_value()) {
    *err = "key not found: " + key;
    return false;
  }
  *out =
      "{\"key\":" + json_quote(key) + ",\"value\":" + json_quote(*value) + "}";
  return true;
}

[[nodiscard]] bool handle_set(const std::string &args,
                              hero_config_store_t *store, std::string *out,
                              std::string *err) {
  std::string key;
  std::string value;
  if (!extract_json_string_field(args, "key", &key) || key.empty()) {
    *err = "hero.config.apply operation=set requires string argument key";
    return false;
  }
  if (!extract_json_string_field(args, "value", &value)) {
    *err = "hero.config.apply operation=set requires string argument value";
    return false;
  }
  if (!store->set_value(key, value, err)) {
    return false;
  }
  *out = "{\"updated\":true,\"key\":" + json_quote(key) +
         ",\"value\":" + json_quote(value) + "}";
  return true;
}

[[nodiscard]] bool handle_validate(const std::string &,
                                   hero_config_store_t *store, std::string *out,
                                   std::string *) {
  const auto errors = store->validate();
  bool global_config_valid = false;
  const std::string global_config =
      build_global_config_map_json(*store, false, &global_config_valid);
  std::ostringstream json;
  json << "{\"valid\":" << bool_json(errors.empty() && global_config_valid)
       << ",\"errors\":[";
  for (std::size_t i = 0; i < errors.size(); ++i) {
    if (i != 0) {
      json << ",";
    }
    json << json_quote(errors[i]);
  }
  json << "],\"global_config\":" << global_config << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_map(const std::string &args,
                              hero_config_store_t *store, std::string *out,
                              std::string *err) {
  bool include_sha256 = false;
  if (extract_json_raw_field(args, "include_sha256", nullptr) &&
      !extract_json_bool_field(args, "include_sha256", &include_sha256)) {
    *err = "include_sha256 must be boolean";
    return false;
  }
  bool valid = false;
  *out = build_global_config_map_json(*store, include_sha256, &valid);
  return true;
}

[[nodiscard]] bool handle_capture_bundle(const std::string &args,
                                         hero_config_store_t *store,
                                         std::string *out, std::string *err) {
  bool include_text = false;
  if (extract_json_raw_field(args, "include_text", nullptr) &&
      !extract_json_bool_field(args, "include_text", &include_text)) {
    *err = "include_text must be boolean";
    return false;
  }
  std::string config_arg;
  fs::path config_path;
  if (extract_json_string_field(args, "config_path", &config_arg) &&
      !trim_ascii(config_arg).empty()) {
    if (!resolve_managed_path(*store, config_arg, false, &config_path, err)) {
      return false;
    }
  } else {
    config_path = fs::path(store->global_config_path());
    if (!resolve_managed_path(*store, config_path.string(), false, &config_path,
                              err)) {
      return false;
    }
  }
  try {
    const auto receipt = cuwacunu::kikijyeba::protocol::config_provenance::
        capture_config_bundle_receipt(config_path);
    *out = config_bundle_receipt_json(receipt, include_text);
  } catch (const std::exception &ex) {
    *err = ex.what();
    return false;
  }
  return true;
}

[[nodiscard]] bool handle_resolve(const std::string &args,
                                  hero_config_store_t *store, std::string *out,
                                  std::string *err) {
  std::string path_arg;
  if (!extract_json_string_field(args, "path", &path_arg) ||
      trim_ascii(path_arg).empty()) {
    *err = "hero.config.inspect subject=resolve_path requires string argument "
           "path";
    return false;
  }
  bool for_write = false;
  if (extract_json_raw_field(args, "for_write", nullptr) &&
      !extract_json_bool_field(args, "for_write", &for_write)) {
    *err = "for_write must be boolean";
    return false;
  }
  bool include_sha256 = true;
  if (extract_json_raw_field(args, "include_sha256", nullptr) &&
      !extract_json_bool_field(args, "include_sha256", &include_sha256)) {
    *err = "include_sha256 must be boolean";
    return false;
  }
  *out = build_resolve_path_json(*store, path_arg, for_write, include_sha256);
  return true;
}

[[nodiscard]] bool handle_diff(const std::string &args,
                               hero_config_store_t *store, std::string *out,
                               std::string *err) {
  bool include_text = false;
  if (extract_json_raw_field(args, "include_text", nullptr) &&
      !extract_json_bool_field(args, "include_text", &include_text)) {
    *err = "include_text must be boolean";
    return false;
  }
  hero_config_store_t::save_preview_t preview;
  if (!store->preview_save(&preview, err)) {
    return false;
  }
  *out = build_preview_json(preview, include_text);
  return true;
}

[[nodiscard]] bool enforce_policy_write(const hero_config_store_t &store,
                                        std::string *err) {
  fs::path config_path = normalize_path(store.config_path());
  return resolve_managed_path(store, config_path.string(), true, nullptr, err);
}

[[nodiscard]] bool handle_save(const std::string &, hero_config_store_t *store,
                               std::string *out, std::string *err) {
  if (!enforce_policy_write(*store, err)) {
    return false;
  }
  if (!store->save(err)) {
    return false;
  }
  *out = "{\"saved\":true,\"path\":" + json_quote(store->config_path()) + "}";
  return true;
}

[[nodiscard]] bool handle_reload(const std::string &,
                                 hero_config_store_t *store, std::string *out,
                                 std::string *err) {
  if (!store->load(err)) {
    return false;
  }
  *out =
      "{\"reloaded\":true,\"path\":" + json_quote(store->config_path()) + "}";
  return true;
}

[[nodiscard]] bool handle_backups(const std::string &,
                                  hero_config_store_t *store, std::string *out,
                                  std::string *err) {
  std::vector<hero_config_store_t::backup_entry_t> backups;
  if (!store->list_backups(&backups, err)) {
    return false;
  }
  std::ostringstream json;
  json << "{\"count\":" << backups.size() << ",\"backups\":[";
  for (std::size_t i = 0; i < backups.size(); ++i) {
    if (i != 0) {
      json << ",";
    }
    json << "{\"index\":" << i
         << ",\"filename\":" << json_quote(backups[i].filename)
         << ",\"path\":" << json_quote(backups[i].path) << "}";
  }
  json << "]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_rollback(const std::string &args,
                                   hero_config_store_t *store, std::string *out,
                                   std::string *err) {
  if (!enforce_policy_write(*store, err)) {
    return false;
  }
  std::string selector;
  (void)extract_json_string_field(args, "backup", &selector);
  std::string selected;
  if (!store->rollback_to_backup(selector, &selected, err)) {
    return false;
  }
  *out =
      "{\"rolled_back\":true,\"selected_backup\":" + json_quote(selected) + "}";
  return true;
}

[[nodiscard]] bool handle_list(const std::string &args,
                               hero_config_store_t *store, std::string *out,
                               std::string *err) {
  std::string root_arg;
  (void)extract_json_string_field(args, "root", &root_arg);
  bool recursive = true;
  if (extract_json_raw_field(args, "recursive", nullptr) &&
      !extract_json_bool_field(args, "recursive", &recursive)) {
    *err = "recursive must be boolean";
    return false;
  }
  bool include_sha256 = false;
  if (extract_json_raw_field(args, "include_sha256", nullptr) &&
      !extract_json_bool_field(args, "include_sha256", &include_sha256)) {
    *err = "include_sha256 must be boolean";
    return false;
  }
  int limit = 0;
  const bool has_limit = extract_json_raw_field(args, "limit", nullptr);
  if (has_limit && !extract_json_int_field(args, "limit", &limit)) {
    *err = "limit must be integer";
    return false;
  }
  if (has_limit && limit < 0) {
    *err = "limit must be non-negative";
    return false;
  }
  fs::path root;
  if (!resolve_managed_path(*store, root_arg.empty() ? "." : root_arg, false,
                            &root, err, false)) {
    return false;
  }
  std::error_code ec;
  if (!fs::exists(root, ec)) {
    *err = "path does not exist: " + root.string();
    return false;
  }

  std::vector<fs::path> files;
  if (fs::is_regular_file(root, ec)) {
    files.push_back(root);
  } else if (fs::is_directory(root, ec)) {
    if (recursive) {
      for (fs::recursive_directory_iterator it(root, ec), end; !ec && it != end;
           it.increment(ec)) {
        if (it->is_regular_file(ec) && extension_allowed(*store, it->path())) {
          files.push_back(normalize_path(it->path()));
        }
      }
    } else {
      for (fs::directory_iterator it(root, ec), end; !ec && it != end;
           it.increment(ec)) {
        if (it->is_regular_file(ec) && extension_allowed(*store, it->path())) {
          files.push_back(normalize_path(it->path()));
        }
      }
    }
  }
  if (ec) {
    *err = "cannot list path " + root.string() + ": " + ec.message();
    return false;
  }
  std::sort(files.begin(), files.end());
  if (has_limit && static_cast<std::size_t>(limit) < files.size()) {
    files.resize(static_cast<std::size_t>(limit));
  }

  std::ostringstream json;
  json << "{\"root\":" << json_quote(root.string())
       << ",\"root_relative_path\":"
       << json_optional_string(relative_to_config_root(*store, root))
       << ",\"count\":" << files.size() << ",\"files\":[";
  for (std::size_t i = 0; i < files.size(); ++i) {
    if (i != 0) {
      json << ",";
    }
    std::error_code size_ec;
    const auto size = fs::file_size(files[i], size_ec);
    json << "{\"path\":" << json_quote(files[i].string())
         << ",\"relative_path\":"
         << json_optional_string(relative_to_config_root(*store, files[i]))
         << ",\"size\":" << (size_ec ? 0 : size);
    if (include_sha256) {
      std::string sha;
      std::string sha_error;
      if (sha256_hex_file(files[i], &sha, &sha_error)) {
        json << ",\"sha256\":" << json_quote(sha);
      }
    }
    json << "}";
  }
  json << "]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_read(const std::string &args,
                               hero_config_store_t *store, std::string *out,
                               std::string *err) {
  std::string path_arg;
  if (!extract_json_string_field(args, "path", &path_arg) ||
      trim_ascii(path_arg).empty()) {
    *err =
        "hero.config.inspect subject=file_read requires string argument path";
    return false;
  }
  fs::path path;
  if (!resolve_managed_path(*store, path_arg, false, &path, err)) {
    return false;
  }
  std::string content;
  if (!read_text_file(path, &content, err)) {
    return false;
  }
  std::string sha;
  if (!sha256_hex_text(content, &sha, err)) {
    return false;
  }
  *out = "{\"path\":" + json_quote(path.string()) + ",\"relative_path\":" +
         json_optional_string(relative_to_config_root(*store, path)) +
         ",\"sha256\":" + json_quote(sha) +
         ",\"content\":" + json_quote(content) + "}";
  return true;
}

[[nodiscard]] bool handle_write(const std::string &args,
                                hero_config_store_t *store, std::string *out,
                                std::string *err) {
  std::string path_arg;
  std::string content;
  if (!extract_json_string_field(args, "path", &path_arg) ||
      trim_ascii(path_arg).empty()) {
    *err = "hero.config.apply operation=write requires string argument path";
    return false;
  }
  if (!extract_json_string_field(args, "content", &content)) {
    *err = "hero.config.apply operation=write requires string argument content";
    return false;
  }
  bool dry_run = false;
  if (extract_json_raw_field(args, "dry_run", nullptr) &&
      !extract_json_bool_field(args, "dry_run", &dry_run)) {
    *err = "dry_run must be boolean";
    return false;
  }
  fs::path path;
  if (!resolve_managed_path(*store, path_arg, true, &path, err)) {
    return false;
  }
  std::string expected_sha;
  const bool has_expected =
      extract_json_string_field(args, "expected_sha256", &expected_sha) &&
      !expected_sha.empty();
  std::error_code ec;
  const bool existed = fs::exists(path, ec);
  if (ec) {
    *err = "cannot inspect target " + path.string() + ": " + ec.message();
    return false;
  }
  if (existed) {
    std::error_code type_ec;
    if (!fs::is_regular_file(path, type_ec) || type_ec) {
      *err = "target exists but is not a regular file: " + path.string();
      return false;
    }
  }
  const bool require_expected =
      policy_bool_or(*store, "require_expected_sha256_for_replace", true);
  if (existed && require_expected && !has_expected) {
    *err = "E_CONFIG_EXPECTED_SHA256_REQUIRED: expected_sha256 is required "
           "when replacing " +
           path.string();
    return false;
  }
  std::string current_sha;
  if (existed && (has_expected || dry_run)) {
    if (!sha256_hex_file(path, &current_sha, err)) {
      return false;
    }
  }
  if (has_expected) {
    if (!existed) {
      *err = "expected_sha256 was provided but target does not exist";
      return false;
    }
    if (current_sha.empty() && !sha256_hex_file(path, &current_sha, err)) {
      return false;
    }
    if (current_sha != expected_sha) {
      *err = "expected_sha256 mismatch for " + path.string();
      return false;
    }
  }
  std::string new_sha;
  if (!sha256_hex_text(content, &new_sha, err)) {
    return false;
  }
  const bool backup_enabled = policy_bool_or(*store, "backup_enabled", true);
  if (dry_run) {
    std::ostringstream json;
    json << "{\"written\":false,\"dry_run\":true,\"created\":"
         << bool_json(!existed) << ",\"path\":" << json_quote(path.string())
         << ",\"relative_path\":"
         << json_optional_string(relative_to_config_root(*store, path))
         << ",\"sha256\":" << json_quote(new_sha)
         << ",\"backup_would_run\":" << bool_json(existed && backup_enabled);
    if (!current_sha.empty()) {
      json << ",\"current_sha256\":" << json_quote(current_sha);
    }
    json << "}";
    *out = json.str();
    return true;
  }
  if (!backup_file(*store, path, err)) {
    return false;
  }
  if (!write_text_file_atomic(path, content, err)) {
    return false;
  }
  *out = "{\"written\":true,\"dry_run\":false,\"created\":" +
         std::string(bool_json(!existed)) +
         ",\"path\":" + json_quote(path.string()) + ",\"relative_path\":" +
         json_optional_string(relative_to_config_root(*store, path)) +
         ",\"sha256\":" + json_quote(new_sha) + ",\"backup_attempted\":" +
         std::string(bool_json(existed && backup_enabled)) + "}";
  return true;
}

[[nodiscard]] bool handle_delete(const std::string &args,
                                 hero_config_store_t *store, std::string *out,
                                 std::string *err) {
  std::string path_arg;
  if (!extract_json_string_field(args, "path", &path_arg) ||
      trim_ascii(path_arg).empty()) {
    *err = "hero.config.apply operation=delete requires string argument path";
    return false;
  }
  bool dry_run = false;
  if (extract_json_raw_field(args, "dry_run", nullptr) &&
      !extract_json_bool_field(args, "dry_run", &dry_run)) {
    *err = "dry_run must be boolean";
    return false;
  }
  fs::path path;
  if (!resolve_managed_path(*store, path_arg, true, &path, err)) {
    return false;
  }
  std::error_code ec;
  if (!fs::is_regular_file(path, ec)) {
    *err = "target is not a regular file: " + path.string();
    return false;
  }
  std::string expected_sha;
  const bool has_expected =
      extract_json_string_field(args, "expected_sha256", &expected_sha) &&
      !expected_sha.empty();
  const bool require_expected =
      policy_bool_or(*store, "require_expected_sha256_for_delete", true);
  if (require_expected && !has_expected) {
    *err = "E_CONFIG_EXPECTED_SHA256_REQUIRED: expected_sha256 is required "
           "when deleting " +
           path.string();
    return false;
  }
  std::string current_sha;
  if (!sha256_hex_file(path, &current_sha, err)) {
    return false;
  }
  if (has_expected) {
    if (current_sha != expected_sha) {
      *err = "expected_sha256 mismatch for " + path.string();
      return false;
    }
  }
  const bool backup_enabled = policy_bool_or(*store, "backup_enabled", true);
  if (dry_run) {
    std::ostringstream json;
    json << "{\"deleted\":false,\"dry_run\":true,\"path\":"
         << json_quote(path.string()) << ",\"relative_path\":"
         << json_optional_string(relative_to_config_root(*store, path))
         << ",\"sha256\":" << json_quote(current_sha)
         << ",\"backup_would_run\":" << bool_json(backup_enabled) << "}";
    *out = json.str();
    return true;
  }
  if (!backup_file(*store, path, err)) {
    return false;
  }
  fs::remove(path, ec);
  if (ec) {
    *err = "cannot delete file " + path.string() + ": " + ec.message();
    return false;
  }
  *out = "{\"deleted\":true,\"dry_run\":false,\"path\":" +
         json_quote(path.string()) + ",\"relative_path\":" +
         json_optional_string(relative_to_config_root(*store, path)) +
         ",\"sha256\":" + json_quote(current_sha) +
         ",\"backup_attempted\":" + std::string(bool_json(backup_enabled)) +
         "}";
  return true;
}

[[nodiscard]] bool
validate_inspect_fields(const std::string &subject,
                        const std::vector<json_field_t> &fields,
                        std::string *err) {
  if (subject == "schema" || subject == "show" ||
      subject == "validate_global_config" || subject == "backups") {
    return validate_allowed_fields(fields,
                                   {"subject", "include_machine_payload"}, err);
  }
  if (subject == "value") {
    return validate_allowed_fields(
        fields, {"subject", "key", "include_machine_payload"}, err);
  }
  if (subject == "map") {
    return validate_allowed_fields(
        fields, {"subject", "include_sha256", "include_machine_payload"}, err);
  }
  if (subject == "bundle") {
    return validate_allowed_fields(fields,
                                   {"subject", "config_path", "include_content",
                                    "include_machine_payload"},
                                   err);
  }
  if (subject == "resolve_path") {
    return validate_allowed_fields(fields,
                                   {"subject", "path", "for_write",
                                    "include_sha256",
                                    "include_machine_payload"},
                                   err);
  }
  if (subject == "diff") {
    return validate_allowed_fields(
        fields, {"subject", "include_content", "include_machine_payload"}, err);
  }
  if (subject == "file_list") {
    return validate_allowed_fields(fields,
                                   {"subject", "path", "recursive",
                                    "include_sha256", "limit",
                                    "include_machine_payload"},
                                   err);
  }
  if (subject == "file_read") {
    return validate_allowed_fields(
        fields, {"subject", "path", "include_machine_payload"}, err);
  }
  if (err) {
    *err = "hero.config.inspect subject must be one of schema, show, value, "
           "validate_global_config, map, bundle, resolve_path, diff, backups, "
           "file_list, file_read";
  }
  return false;
}

[[nodiscard]] bool handle_inspect(const std::string &args,
                                  hero_config_store_t *store, std::string *out,
                                  std::string *err) {
  std::vector<json_field_t> fields;
  if (!parse_json_object_fields(args, &fields, err)) {
    return false;
  }
  std::string subject;
  if (!extract_json_string_field(args, "subject", &subject) ||
      trim_ascii(subject).empty()) {
    *err = "hero.config.inspect requires string argument subject";
    return false;
  }
  subject = trim_ascii(subject);
  if (!validate_inspect_fields(subject, fields, err)) {
    return false;
  }
  for (const auto &bool_key : {"include_content", "include_machine_payload",
                               "include_sha256", "for_write", "recursive"}) {
    if (!validate_optional_bool_field(args, bool_key, err)) {
      return false;
    }
  }

  if (subject == "schema") {
    return handle_schema("{}", store, out, err);
  }
  if (subject == "show") {
    return handle_show("{}", store, out, err);
  }
  if (subject == "value") {
    const std::string subargs = object_with_fields(fields, {{"key", "key"}});
    return handle_get(subargs, store, out, err);
  }
  if (subject == "validate_global_config") {
    return handle_validate("{}", store, out, err);
  }
  if (subject == "map") {
    const std::string subargs =
        object_with_fields(fields, {{"include_sha256", "include_sha256"}});
    return handle_map(subargs, store, out, err);
  }
  if (subject == "bundle") {
    std::ostringstream subargs;
    bool has_any = false;
    subargs << "{";
    append_raw_member(&subargs, &has_any, fields, "config_path", "config_path");
    append_raw_member(&subargs, &has_any, fields, "include_content",
                      "include_text");
    subargs << "}";
    return handle_capture_bundle(subargs.str(), store, out, err);
  }
  if (subject == "resolve_path") {
    const std::string subargs =
        object_with_fields(fields, {{"path", "path"},
                                    {"for_write", "for_write"},
                                    {"include_sha256", "include_sha256"}});
    return handle_resolve(subargs, store, out, err);
  }
  if (subject == "diff") {
    std::ostringstream subargs;
    bool has_any = false;
    subargs << "{";
    append_raw_member(&subargs, &has_any, fields, "include_content",
                      "include_text");
    subargs << "}";
    return handle_diff(subargs.str(), store, out, err);
  }
  if (subject == "backups") {
    return handle_backups("{}", store, out, err);
  }
  if (subject == "file_list") {
    std::ostringstream subargs;
    bool has_any = false;
    subargs << "{";
    append_raw_member(&subargs, &has_any, fields, "path", "root");
    append_raw_member(&subargs, &has_any, fields, "recursive", "recursive");
    append_raw_member(&subargs, &has_any, fields, "include_sha256",
                      "include_sha256");
    append_raw_member(&subargs, &has_any, fields, "limit", "limit");
    subargs << "}";
    return handle_list(subargs.str(), store, out, err);
  }
  if (subject == "file_read") {
    const std::string subargs = object_with_fields(fields, {{"path", "path"}});
    return handle_read(subargs, store, out, err);
  }
  *err = "unsupported hero.config.inspect subject: " + subject;
  return false;
}

[[nodiscard]] bool select_backup_for_plan(const hero_config_store_t &store,
                                          const std::string &selector,
                                          std::string *selected,
                                          std::string *err) {
  std::vector<hero_config_store_t::backup_entry_t> backups;
  if (!store.list_backups(&backups, err)) {
    return false;
  }
  if (backups.empty()) {
    if (err) {
      *err = "no Config Hero policy backups found";
    }
    return false;
  }
  if (trim_ascii(selector).empty()) {
    if (selected) {
      *selected = backups.front().path;
    }
    return true;
  }
  int index = -1;
  if (parse_int(selector, &index) && index >= 0 &&
      static_cast<std::size_t>(index) < backups.size()) {
    if (selected) {
      *selected = backups[static_cast<std::size_t>(index)].path;
    }
    return true;
  }
  for (const auto &backup : backups) {
    if (backup.filename == selector || backup.path == selector) {
      if (selected) {
        *selected = backup.path;
      }
      return true;
    }
  }
  if (err) {
    *err = "backup not found: " + selector;
  }
  return false;
}

[[nodiscard]] bool
build_expected_sha256_args(const std::vector<json_field_t> &fields,
                           std::ostringstream *subargs, bool *has_any,
                           std::string *err) {
  std::string expected_sha;
  std::string expected_current_sha;
  const bool has_expected = extract_json_string_field(
      object_with_fields(fields, {{"expected_sha256", "expected_sha256"}}),
      "expected_sha256", &expected_sha);
  const bool has_expected_current = extract_json_string_field(
      object_with_fields(
          fields, {{"expected_current_sha256", "expected_current_sha256"}}),
      "expected_current_sha256", &expected_current_sha);
  if (has_expected && has_expected_current &&
      expected_sha != expected_current_sha) {
    if (err) {
      *err = "expected_sha256 and expected_current_sha256 must match when "
             "both are provided";
    }
    return false;
  }
  if (has_expected || has_expected_current) {
    append_literal_member(
        subargs, has_any, "expected_sha256",
        json_quote(has_expected ? expected_sha : expected_current_sha));
  }
  return true;
}

[[nodiscard]] bool
validate_apply_fields(const std::string &operation,
                      const std::vector<json_field_t> &fields,
                      std::string *err) {
  if (operation == "set") {
    return validate_allowed_fields(fields,
                                   {"operation", "requested_mode", "key",
                                    "value", "reason", "include_content"},
                                   err);
  }
  if (operation == "save" || operation == "reload") {
    return validate_allowed_fields(
        fields, {"operation", "requested_mode", "reason", "include_content"},
        err);
  }
  if (operation == "rollback") {
    return validate_allowed_fields(fields,
                                   {"operation", "requested_mode", "backup_id",
                                    "reason", "include_content"},
                                   err);
  }
  if (operation == "write") {
    return validate_allowed_fields(fields,
                                   {"operation", "requested_mode", "path",
                                    "content", "expected_sha256",
                                    "expected_current_sha256", "reason",
                                    "create_backup", "include_content"},
                                   err);
  }
  if (operation == "delete") {
    return validate_allowed_fields(fields,
                                   {"operation", "requested_mode", "path",
                                    "expected_sha256",
                                    "expected_current_sha256", "reason",
                                    "create_backup", "include_content"},
                                   err);
  }
  if (err) {
    *err = "hero.config.apply operation must be one of set, save, reload, "
           "rollback, write, delete";
  }
  return false;
}

[[nodiscard]] bool handle_apply_plan(const std::string &operation,
                                     const std::vector<json_field_t> &fields,
                                     hero_config_store_t *store,
                                     std::string *out, std::string *err) {
  if (operation == "set") {
    std::string key;
    std::string value;
    if (!extract_json_string_field(object_with_fields(fields, {{"key", "key"}}),
                                   "key", &key) ||
        key.empty()) {
      *err = "hero.config.apply operation=set requires string argument key";
      return false;
    }
    if (!extract_json_string_field(
            object_with_fields(fields, {{"value", "value"}}), "value",
            &value)) {
      *err = "hero.config.apply operation=set requires string argument value";
      return false;
    }
    hero_config_store_t copy = *store;
    std::string validation_error;
    if (!copy.set_value(key, value, &validation_error)) {
      *err = validation_error;
      return false;
    }
    *out = "{\"planned\":true,\"operation\":\"set\",\"would_mutate\":true,"
           "\"key\":" +
           json_quote(key) + ",\"value\":" + json_quote(value) + "}";
    return true;
  }
  if (operation == "save") {
    hero_config_store_t::save_preview_t preview;
    if (!store->preview_save(&preview, err)) {
      return false;
    }
    const bool include_content = optional_bool_value(
        object_with_fields(fields, {{"include_content", "include_content"}}),
        "include_content", false, err);
    if (!err->empty()) {
      return false;
    }
    *out = "{\"planned\":true,\"operation\":\"save\",\"would_mutate\":true,"
           "\"preview\":" +
           build_preview_json(preview, include_content) + "}";
    return true;
  }
  if (operation == "reload") {
    *out = "{\"planned\":true,\"operation\":\"reload\",\"would_mutate\":true,"
           "\"path\":" +
           json_quote(store->config_path()) + "}";
    return true;
  }
  if (operation == "rollback") {
    std::string selector;
    (void)extract_json_string_field(
        object_with_fields(fields, {{"backup_id", "backup"}}), "backup",
        &selector);
    std::string selected;
    if (!select_backup_for_plan(*store, selector, &selected, err)) {
      return false;
    }
    *out = "{\"planned\":true,\"operation\":\"rollback\",\"would_mutate\":true,"
           "\"selected_backup\":" +
           json_quote(selected) + "}";
    return true;
  }
  *err = "unsupported hero.config.apply plan operation: " + operation;
  return false;
}

[[nodiscard]] bool handle_apply(const std::string &args,
                                hero_config_store_t *store, std::string *out,
                                std::string *err) {
  std::vector<json_field_t> fields;
  if (!parse_json_object_fields(args, &fields, err)) {
    return false;
  }
  std::string operation;
  if (!extract_json_string_field(args, "operation", &operation) ||
      trim_ascii(operation).empty()) {
    *err = "hero.config.apply requires string argument operation";
    return false;
  }
  operation = trim_ascii(operation);
  std::string requested_mode;
  if (!extract_json_string_field(args, "requested_mode", &requested_mode) ||
      trim_ascii(requested_mode).empty()) {
    *err = "hero.config.apply requires string argument requested_mode";
    return false;
  }
  requested_mode = trim_ascii(requested_mode);
  if (requested_mode != "plan" && requested_mode != "execute") {
    *err = "hero.config.apply requested_mode must be plan or execute";
    return false;
  }
  if (!validate_apply_fields(operation, fields, err)) {
    return false;
  }
  for (const auto &bool_key : {"create_backup", "include_content"}) {
    if (!validate_optional_bool_field(args, bool_key, err)) {
      return false;
    }
  }

  if (requested_mode == "plan" &&
      (operation == "set" || operation == "save" || operation == "reload" ||
       operation == "rollback")) {
    return handle_apply_plan(operation, fields, store, out, err);
  }

  if (operation == "set") {
    const std::string subargs =
        object_with_fields(fields, {{"key", "key"}, {"value", "value"}});
    return handle_set(subargs, store, out, err);
  }
  if (operation == "save") {
    return handle_save("{}", store, out, err);
  }
  if (operation == "reload") {
    return handle_reload("{}", store, out, err);
  }
  if (operation == "rollback") {
    const std::string subargs =
        object_with_fields(fields, {{"backup_id", "backup"}});
    return handle_rollback(subargs, store, out, err);
  }
  if (operation == "write" || operation == "delete") {
    std::ostringstream subargs;
    bool has_any = false;
    subargs << "{";
    append_raw_member(&subargs, &has_any, fields, "path", "path");
    append_raw_member(&subargs, &has_any, fields, "content", "content");
    if (!build_expected_sha256_args(fields, &subargs, &has_any, err)) {
      return false;
    }
    append_literal_member(&subargs, &has_any, "dry_run",
                          requested_mode == "plan" ? "true" : "false");
    subargs << "}";
    if (operation == "write") {
      return handle_write(subargs.str(), store, out, err);
    }
    return handle_delete(subargs.str(), store, out, err);
  }

  *err = "unsupported hero.config.apply operation: " + operation;
  return false;
}

using handler_fn = bool (*)(const std::string &, hero_config_store_t *,
                            std::string *, std::string *);

[[nodiscard]] std::optional<handler_fn> find_handler(std::string_view name) {
  if (name == "hero.config.status") {
    return handle_status;
  }
  if (name == "hero.config.inspect") {
    return handle_inspect;
  }
  if (name == "hero.config.apply") {
    return handle_apply;
  }
  return std::nullopt;
}

} // namespace

bool execute_tool_json(const std::string &tool_name, std::string arguments_json,
                       hero_config_store_t *store,
                       std::string *out_tool_result_json,
                       std::string *out_error_message) {
  if (out_tool_result_json) {
    out_tool_result_json->clear();
  }
  if (out_error_message) {
    out_error_message->clear();
  }
  if (!store) {
    if (out_error_message) {
      *out_error_message = "Config Hero store pointer is null";
    }
    if (out_tool_result_json) {
      *out_tool_result_json =
          make_error_result("Config Hero store pointer is null", -32603);
    }
    return false;
  }
  arguments_json = trim_ascii(arguments_json);
  if (arguments_json.empty()) {
    arguments_json = "{}";
  }
  if (arguments_json.front() != '{') {
    if (out_error_message) {
      *out_error_message = "--args-json must be a JSON object";
    }
    if (out_tool_result_json) {
      *out_tool_result_json =
          make_error_result("--args-json must be a JSON object", -32602);
    }
    return false;
  }

  const auto handler = find_handler(tool_name);
  if (!handler.has_value()) {
    const std::string err = "unknown tool: " + tool_name;
    if (out_error_message) {
      *out_error_message = err;
    }
    if (out_tool_result_json) {
      *out_tool_result_json = make_error_result(err, -32601);
    }
    return false;
  }

  std::string structured;
  std::string err;
  const bool ok = (*handler)(arguments_json, store, &structured, &err);
  if (!ok) {
    if (out_error_message) {
      *out_error_message = err;
    }
    if (out_tool_result_json) {
      const int code = err.find("E_CONFIG_") == 0 ? kPolicyErrorCode : -32602;
      *out_tool_result_json = make_error_result(err, code);
    }
    return false;
  }
  if (out_tool_result_json) {
    *out_tool_result_json = make_tool_result(tool_name, structured);
  }
  return true;
}

std::string build_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < std::size(kTools); ++i) {
    mcp_schema_compat::assert_tool_input_schema_compatible(
        kTools[i].name, kTools[i].input_schema_json);
    if (i != 0) {
      out << ",";
    }
    out << "{\"name\":" << json_quote(kTools[i].name)
        << ",\"description\":" << json_quote(kTools[i].description)
        << ",\"inputSchema\":" << kTools[i].input_schema_json
        << ",\"annotations\":{\"readOnlyHint\":"
        << bool_json(tool_is_read_only(kTools[i].name))
        << ",\"destructiveHint\":"
        << bool_json(tool_is_destructive(kTools[i].name))
        << ",\"openWorldHint\":false}}";
  }
  out << "]}";
  return out.str();
}

std::string build_tools_list_human_text() {
  std::ostringstream out;
  for (const auto &tool : kTools) {
    out << tool.name << "\t" << tool.description << "\n";
  }
  return out.str();
}

void run_jsonrpc_stdio_loop(hero_config_store_t *store) {
  std::string line;
  bool shutdown_seen = false;
  while (std::getline(std::cin, line)) {
    line = trim_ascii(line);
    if (line.empty()) {
      continue;
    }
    std::string id_raw = "null";
    (void)extract_json_raw_field(line, "id", &id_raw);
    std::string method;
    if (!extract_json_string_field(line, "method", &method)) {
      continue;
    }
    if (method == "notifications/initialized") {
      continue;
    }
    if (method == "exit") {
      break;
    }

    if (method == "initialize") {
      std::string protocol = "2024-11-05";
      (void)extract_json_string_field(line, "protocolVersion", &protocol);
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":{\"protocolVersion\":" << json_quote(protocol)
                << ",\"capabilities\":{\"tools\":{}},\"serverInfo\":{"
                   "\"name\":\"hero_config\",\"version\":\"2\"}}}"
                << std::endl;
      continue;
    }
    if (method == "tools/list") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":" << build_tools_list_result_json() << "}"
                << std::endl;
      continue;
    }
    if (method == "resources/list") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":{\"resources\":[]}}" << std::endl;
      continue;
    }
    if (method == "resources/templates/list") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":{\"resourceTemplates\":[]}}" << std::endl;
      continue;
    }
    if (method == "shutdown") {
      shutdown_seen = true;
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":null}" << std::endl;
      continue;
    }
    if (method == "tools/call") {
      std::string params;
      if (!extract_json_raw_field(line, "params", &params)) {
        std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                  << ",\"error\":{\"code\":-32602,\"message\":\"missing "
                     "params\"}}"
                  << std::endl;
        continue;
      }
      std::string name;
      if (!extract_json_string_field(params, "name", &name)) {
        std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                  << ",\"error\":{\"code\":-32602,\"message\":\"missing tool "
                     "name\"}}"
                  << std::endl;
        continue;
      }
      std::string arguments = "{}";
      (void)extract_json_raw_field(params, "arguments", &arguments);
      std::string result;
      std::string error;
      (void)execute_tool_json(name, arguments, store, &result, &error);
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":" << result << "}" << std::endl;
      continue;
    }

    std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
              << ",\"error\":{\"code\":-32601,\"message\":\"unknown method\"}}"
              << std::endl;
    if (shutdown_seen) {
      break;
    }
  }
}

} // namespace cuwacunu::hero::config
