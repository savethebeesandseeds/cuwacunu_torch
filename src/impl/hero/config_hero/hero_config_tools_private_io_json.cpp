#include "hero_config_tools_internal.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <poll.h>
#include <sstream>
#include <string_view>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <openssl/evp.h>

#include "hero/config_hero/hero.config.h"
#include "hero/hashimyei_hero/hashimyei_catalog.h"
#include "iitepi/contract_space_t.h"

namespace cuwacunu::hero::mcp::detail {
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20; // 8 MiB
constexpr const char *kStdioIdleTimeoutEnv = "HERO_MCP_IDLE_TIMEOUT_SEC";
constexpr int kDefaultStdioIdleTimeoutSec = 600;
bool g_jsonrpc_use_content_length_framing = false;

[[nodiscard]] bool write_all_fd(int fd, const void *bytes, std::size_t size) {
  const char *data = reinterpret_cast<const char *>(bytes);
  std::size_t remaining = size;
  while (remaining > 0) {
    const ssize_t wrote = ::write(fd, data, remaining);
    if (wrote < 0) {
      if (errno == EINTR)
        continue;
      return false;
    }
    if (wrote == 0)
      return false;
    const auto wrote_size = static_cast<std::size_t>(wrote);
    data += wrote_size;
    remaining -= wrote_size;
  }
  return true;
}

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

[[nodiscard]] bool parse_bool_ascii(std::string_view raw, bool *out) {
  if (!out)
    return false;
  const std::string lowered = lowercase_copy(trim_ascii(raw));
  if (lowered == "true" || lowered == "1" || lowered == "yes" ||
      lowered == "on") {
    *out = true;
    return true;
  }
  if (lowered == "false" || lowered == "0" || lowered == "no" ||
      lowered == "off") {
    *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] std::filesystem::path
resolve_path_near_config(std::string_view raw_path,
                         std::string_view config_path);
[[nodiscard]] bool read_ini_general_key(std::string_view ini_path,
                                        std::string_view key,
                                        std::string *out_value);
[[nodiscard]] std::string
path_vector_json(const std::vector<std::filesystem::path> &paths);
[[nodiscard]] std::string json_quote(std::string_view in);
[[nodiscard]] std::string bool_json(bool v);
[[nodiscard]] bool parse_int64_ascii(std::string_view raw, std::int64_t *out);

[[nodiscard]] std::optional<int> resolve_stdio_idle_timeout_ms() {
  static const std::optional<int> cached_timeout_ms =
      []() -> std::optional<int> {
    const char *raw = std::getenv(kStdioIdleTimeoutEnv);
    if (raw == nullptr || *raw == '\0') {
      return kDefaultStdioIdleTimeoutSec * 1000;
    }

    errno = 0;
    char *end = nullptr;
    const long long parsed = std::strtoll(raw, &end, 10);
    while (end != nullptr &&
           std::isspace(static_cast<unsigned char>(*end)) != 0) {
      ++end;
    }
    if (errno != 0 || end == raw || (end != nullptr && *end != '\0')) {
      return kDefaultStdioIdleTimeoutSec * 1000;
    }
    if (parsed <= 0)
      return std::nullopt;

    constexpr long long kMaxTimeoutSec =
        static_cast<long long>(std::numeric_limits<int>::max()) / 1000;
    if (parsed >= kMaxTimeoutSec)
      return std::numeric_limits<int>::max();
    return static_cast<int>(parsed * 1000);
  }();
  return cached_timeout_ms;
}

[[nodiscard]] bool wait_for_stdio_activity() {
  const std::optional<int> timeout_ms = resolve_stdio_idle_timeout_ms();
  if (!timeout_ms.has_value())
    return true;

  struct pollfd pfd {};
  pfd.fd = STDIN_FILENO;
  pfd.events = POLLIN | POLLHUP | POLLERR;
  while (true) {
    const int rc = ::poll(&pfd, 1, *timeout_ms);
    if (rc > 0)
      return true;
    if (rc == 0)
      return false;
    if (errno == EINTR)
      continue;
    return true;
  }
}

[[nodiscard]] bool read_text_file(std::string_view path, std::string *out,
                                  std::string *err) {
  if (err)
    err->clear();
  if (!out) {
    if (err)
      *err = "output pointer is null";
    return false;
  }
  std::ifstream in(std::string(path), std::ios::binary);
  if (!in) {
    if (err)
      *err = "cannot open file: " + std::string(path);
    return false;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  if (!in.good() && !in.eof()) {
    if (err)
      *err = "cannot read file: " + std::string(path);
    return false;
  }
  *out = ss.str();
  return true;
}

[[nodiscard]] bool write_text_file_atomic(std::string_view path,
                                          std::string_view content,
                                          std::string *err) {
  namespace fs = std::filesystem;
  if (err)
    err->clear();
  const fs::path target{std::string(path)};
  if (target.empty()) {
    if (err)
      *err = "target path is empty";
    return false;
  }
  std::error_code ec{};
  if (target.has_parent_path()) {
    fs::create_directories(target.parent_path(), ec);
    if (ec) {
      if (err)
        *err =
            "cannot create parent directory: " + target.parent_path().string();
      return false;
    }
  }
  const auto stamp =
      std::chrono::steady_clock::now().time_since_epoch().count();
  const fs::path tmp =
      target.parent_path() / (target.filename().string() + ".tmp." +
                              std::to_string(static_cast<long long>(stamp)));
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) {
      if (err)
        *err = "cannot open temp file: " + tmp.string();
      return false;
    }
    out << content;
    out.flush();
    if (!out.good()) {
      if (err)
        *err = "cannot write temp file: " + tmp.string();
      return false;
    }
  }
  fs::rename(tmp, target, ec);
  if (ec) {
    std::error_code rm_ec{};
    fs::remove(tmp, rm_ec);
    if (err)
      *err = "cannot replace target file: " + target.string();
    return false;
  }
  return true;
}

[[nodiscard]] bool read_ini_general_key(std::string_view ini_path,
                                        std::string_view key,
                                        std::string *out_value) {
  if (out_value)
    out_value->clear();
  std::ifstream in(std::string(ini_path), std::ios::binary);
  if (!in)
    return false;

  bool in_general = false;
  std::string line;
  while (std::getline(in, line)) {
    std::string trimmed = trim_ascii(line);
    if (trimmed.empty())
      continue;
    if (trimmed.front() == '#' || trimmed.front() == ';')
      continue;
    if (trimmed.front() == '[' && trimmed.back() == ']') {
      in_general = lowercase_copy(trimmed) == "[general]";
      continue;
    }
    if (!in_general)
      continue;

    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos)
      continue;
    const std::string lhs = trim_ascii(trimmed.substr(0, eq));
    if (lhs != key)
      continue;
    std::string rhs = trim_ascii(trimmed.substr(eq + 1));
    const std::size_t comment = rhs.find('#');
    if (comment != std::string::npos) {
      rhs = trim_ascii(rhs.substr(0, comment));
    }
    if (out_value)
      *out_value = rhs;
    return true;
  }
  return false;
}

[[nodiscard]] std::filesystem::path
configured_hashimyei_store_root(std::string_view global_config_path) {
  const char *env = std::getenv("CUWACUNU_HASHIMYEI_STORE_ROOT");
  if (env != nullptr && env[0] != '\0') {
    const std::string value = trim_ascii(env);
    if (!value.empty())
      return std::filesystem::path(value);
  }

  const std::string cfg_path = trim_ascii(global_config_path);
  const std::string ini_path =
      cfg_path.empty() ? std::string(kDefaultGlobalConfigPath) : cfg_path;
  std::string configured{};
  if (read_ini_general_key(ini_path, "runtime_root", &configured)) {
    configured = trim_ascii(configured);
    if (!configured.empty()) {
      return (std::filesystem::path(configured) / ".hashimyei")
          .lexically_normal();
    }
  }
  return {};
}

[[nodiscard]] std::filesystem::path
configured_hashimyei_catalog_path(std::string_view global_config_path) {
  return cuwacunu::hashimyei::catalog_db_path(
      configured_hashimyei_store_root(global_config_path));
}

[[nodiscard]] std::string
normalized_path_key(const std::filesystem::path &path);
[[nodiscard]] bool sha256_hex_file(const std::filesystem::path &path,
                                   std::string *out_hex, std::string *err);

[[nodiscard]] std::string
sanitize_bundle_snapshot_filename(std::string_view filename) {
  std::string out{};
  out.reserve(filename.size());
  for (const char ch : filename) {
    const bool ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' ||
                    ch == '-';
    out.push_back(ok ? ch : '_');
  }
  if (out.empty())
    out = "dsl";
  return out;
}

[[nodiscard]] bool write_component_founding_dsl_bundle_snapshot(
    const std::filesystem::path &store_root,
    const cuwacunu::hero::hashimyei::component_manifest_t &manifest,
    std::string_view primary_source_path,
    const std::shared_ptr<const cuwacunu::iitepi::contract_record_t>
        &contract_record,
    std::string *error) {
  if (error)
    error->clear();
  if (!contract_record) {
    if (error)
      *error = "missing contract snapshot for founding DSL bundle";
    return false;
  }

  std::vector<std::filesystem::path> source_paths{};
  std::unordered_set<std::string> seen{};
  const auto add_source_path = [&](std::string_view raw_path) {
    const std::string trimmed = trim_ascii(raw_path);
    if (trimmed.empty())
      return;
    const std::filesystem::path source_path(trimmed);
    const std::string key = normalized_path_key(source_path);
    if (!seen.insert(key).second)
      return;
    source_paths.push_back(source_path);
  };

  add_source_path(contract_record->config_file_path_canonical);
  add_source_path(primary_source_path);
  for (const auto &surface : contract_record->docking_signature.surfaces) {
    add_source_path(surface.canonical_path);
  }
  for (const auto &module : contract_record->signature.module_dsl_entries) {
    add_source_path(module.module_dsl_path);
  }

  std::sort(source_paths.begin(), source_paths.end(),
            [](const std::filesystem::path &a, const std::filesystem::path &b) {
              return normalized_path_key(a) < normalized_path_key(b);
            });

  const std::string component_id =
      cuwacunu::hero::hashimyei::compute_component_manifest_id(manifest);
  const auto bundle_dir =
      cuwacunu::hero::hashimyei::founding_dsl_bundle_directory(
          store_root, manifest.canonical_path, component_id);
  std::error_code ec{};
  std::filesystem::create_directories(bundle_dir, ec);
  if (ec) {
    if (error) {
      *error =
          "cannot create founding DSL bundle directory: " + bundle_dir.string();
    }
    return false;
  }

  cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_t bundle_manifest{};
  bundle_manifest.component_id = component_id;
  bundle_manifest.canonical_path = manifest.canonical_path;
  bundle_manifest.hashimyei_name = manifest.hashimyei_identity.name;
  bundle_manifest.files.reserve(source_paths.size());

  for (std::size_t i = 0; i < source_paths.size(); ++i) {
    const auto &source_path = source_paths[i];
    std::string text{};
    if (!read_text_file(source_path.string(), &text, error)) {
      if (error) {
        *error = "cannot read founding DSL source '" + source_path.string() +
                 "': " + *error;
      }
      return false;
    }
    std::ostringstream filename;
    filename << std::setfill('0') << std::setw(4) << i << "_"
             << sanitize_bundle_snapshot_filename(
                    source_path.filename().string());
    const auto snapshot_path = bundle_dir / filename.str();
    if (!write_text_file_atomic(snapshot_path.string(), text, error)) {
      if (error) {
        *error = "cannot write founding DSL snapshot '" +
                 snapshot_path.string() + "': " + *error;
      }
      return false;
    }
    std::string sha256_hex{};
    if (!sha256_hex_file(snapshot_path, &sha256_hex, error)) {
      if (error) {
        *error = "cannot fingerprint founding DSL snapshot '" +
                 snapshot_path.string() + "': " + *error;
      }
      return false;
    }
    bundle_manifest.files.push_back(
        cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_file_t{
            .source_path = source_path.string(),
            .snapshot_relpath = filename.str(),
            .sha256_hex = sha256_hex,
        });
  }
  bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex =
      cuwacunu::hero::hashimyei::compute_founding_dsl_bundle_aggregate_sha256(
          bundle_manifest);
  if (bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex.empty()) {
    if (error) {
      *error = "cannot compute founding DSL bundle aggregate digest";
    }
    return false;
  }

  if (!cuwacunu::hero::hashimyei::write_founding_dsl_bundle_manifest(
          store_root, bundle_manifest, error)) {
    return false;
  }
  return true;
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
        const char *hex = "0123456789abcdef";
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

[[nodiscard]] bool parse_json_string_at(const std::string &json,
                                        std::size_t pos, std::string *out,
                                        std::size_t *end_pos) {
  if (pos >= json.size() || json[pos] != '"')
    return false;
  ++pos;
  std::string value;
  while (pos < json.size()) {
    const char c = json[pos++];
    if (c == '"') {
      if (out)
        *out = value;
      if (end_pos)
        *end_pos = pos;
      return true;
    }
    if (c != '\\') {
      value.push_back(c);
      continue;
    }
    if (pos >= json.size())
      return false;
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

[[nodiscard]] std::size_t skip_json_whitespace(const std::string &json,
                                               std::size_t pos) {
  while (pos < json.size() &&
         std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
    ++pos;
  }
  return pos;
}

[[nodiscard]] bool find_json_value_end(const std::string &json, std::size_t pos,
                                       std::size_t *out_end_pos) {
  pos = skip_json_whitespace(json, pos);
  if (pos >= json.size())
    return false;

  if (json[pos] == '"') {
    std::size_t end_pos = pos;
    if (!parse_json_string_at(json, pos, nullptr, &end_pos))
      return false;
    if (out_end_pos)
      *out_end_pos = end_pos;
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
        if (stack.empty() || stack.back() != c)
          return false;
        stack.pop_back();
        if (stack.empty()) {
          if (out_end_pos)
            *out_end_pos = i + 1;
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
  if (end <= pos)
    return false;
  if (out_end_pos)
    *out_end_pos = end;
  return true;
}

[[nodiscard]] bool extract_json_raw_field(const std::string &json,
                                          const std::string &key,
                                          std::string *out) {
  std::size_t pos = skip_json_whitespace(json, 0);
  if (pos >= json.size() || json[pos] != '{')
    return false;
  ++pos;

  while (true) {
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
  const std::string trimmed = trim_ascii(raw);
  if (trimmed.empty() || trimmed.front() != '{')
    return false;
  if (out)
    *out = trimmed;
  return true;
}

[[nodiscard]] bool extract_json_string_field(const std::string &json,
                                             const std::string &key,
                                             std::string *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw))
    return false;
  std::size_t end_pos = 0;
  if (!parse_json_string_at(raw, 0, out, &end_pos))
    return false;
  end_pos = skip_json_whitespace(raw, end_pos);
  return end_pos == raw.size();
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
    std::size_t end_pos = 0;
    if (!parse_json_string_at(raw_id, 0, &parsed, &end_pos))
      return false;
    end_pos = skip_json_whitespace(raw_id, end_pos);
    if (end_pos != raw_id.size())
      return false;
    if (out_id_json)
      *out_id_json = json_quote(parsed);
    return true;
  }

  if (out_id_json)
    *out_id_json = raw_id;
  return true;
}

[[nodiscard]] bool has_json_field(const std::string &json,
                                  const std::string &key) {
  return extract_json_raw_field(json, key, nullptr);
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
  if (number.empty())
    return false;

  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(number.data(), number.data() + number.size(), parsed);
  if (ec != std::errc() || ptr != number.data() + number.size())
    return false;
  if (parsed >
      static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return false;
  }
  if (out_length)
    *out_length = static_cast<std::size_t>(parsed);
  return true;
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
      if (content_length == 0) {
        continue;
      }
      if (content_length > kMaxJsonRpcPayloadBytes)
        return false;
      std::string payload(content_length, '\0');
      in->read(payload.data(), static_cast<std::streamsize>(content_length));
      if (in->gcount() != static_cast<std::streamsize>(content_length)) {
        return false;
      }
      if (out_payload)
        *out_payload = payload;
      if (out_used_content_length)
        *out_used_content_length = true;
      return true;
    }

    if (trimmed.front() == '{' || trimmed.front() == '[') {
      if (out_payload)
        *out_payload = trimmed;
      return true;
    }
  }

  return false;
}

void emit_jsonrpc_payload(std::string_view payload_json) {
  std::string framed;
  if (g_jsonrpc_use_content_length_framing) {
    framed = std::string("Content-Length: ") +
             std::to_string(payload_json.size()) + "\r\n\r\n" +
             std::string(payload_json);
  } else {
    framed = std::string(payload_json) + "\n";
  }
  (void)write_all_fd(STDOUT_FILENO, framed.data(), framed.size());
}

void emit_jsonrpc_result(std::string_view id_json,
                         std::string_view result_object_json) {
  const std::string payload =
      std::string("{\"jsonrpc\":\"2.0\",\"id\":") + std::string(id_json) +
      ",\"result\":" + std::string(result_object_json) + "}";
  emit_jsonrpc_payload(payload);
}

void emit_jsonrpc_error(std::string_view id_json, int code,
                        std::string_view message) {
  const std::string payload = std::string("{\"jsonrpc\":\"2.0\",\"id\":") +
                              std::string(id_json) +
                              ",\"error\":{\"code\":" + std::to_string(code) +
                              ",\"message\":" + json_quote(message) + "}}";
  emit_jsonrpc_payload(payload);
}

