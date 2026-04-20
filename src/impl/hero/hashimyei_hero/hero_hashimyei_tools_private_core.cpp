#include "hero/hashimyei_hero/hero_hashimyei_tools.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <poll.h>
#include <sstream>
#include <string>
#include <string_view>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <openssl/evp.h>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "hero/lattice_hero/lattice_catalog.h"
#include "hero/mcp_server_observability.h"
#include "iitepi/config_space_t.h"
#include "iitepi/contract_space_t.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"

namespace {

constexpr const char *kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char *kServerName = "hero_hashimyei_mcp";
constexpr const char *kServerVersion = "0.1.0";
constexpr const char *kProtocolVersion = "2025-03-26";
constexpr const char *kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.hashimyei.*.";
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

using app_context_t = cuwacunu::hero::hashimyei_mcp::app_context_t;
using hashimyei_runtime_defaults_t =
    cuwacunu::hero::hashimyei_mcp::hashimyei_runtime_defaults_t;

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

[[nodiscard]] std::uint64_t now_ms_utc() {
  const auto now = std::chrono::system_clock::now();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now.time_since_epoch())
          .count());
}

void append_hashimyei_tool_catalog_fields(std::ostringstream &extra,
                                          const app_context_t *app) {
  if (!app)
    return;
  if (!app->catalog_options.catalog_path.empty()) {
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", app->catalog_options.catalog_path.string());
  }
}

void log_hashimyei_tool_begin(const app_context_t *app,
                              std::string_view tool_name,
                              bool catalog_already_open) {
  std::ostringstream extra;
  append_hashimyei_tool_catalog_fields(extra, app);
  cuwacunu::hero::mcp_observability::append_json_string_field(
      extra, "tool_name", tool_name);
  cuwacunu::hero::mcp_observability::append_json_bool_field(
      extra, "catalog_already_open", catalog_already_open);
  cuwacunu::hero::mcp_observability::append_json_bool_field(
      extra, "opened_here", !catalog_already_open);
  if (app) {
    cuwacunu::hero::mcp_observability::append_json_bool_field(
        extra, "close_catalog_after_execute", app->close_catalog_after_execute);
  }
  std::string ignored{};
  (void)cuwacunu::hero::mcp_observability::append_event_json(
      kServerName, "tool_begin", extra.str(), &ignored);
}

void log_hashimyei_tool_end(const app_context_t *app,
                            std::string_view tool_name,
                            bool catalog_already_open, bool ok,
                            bool catalog_closed, std::uint64_t elapsed_ms,
                            std::uint64_t catalog_hold_ms = 0,
                            std::string_view error = {}) {
  std::ostringstream extra;
  append_hashimyei_tool_catalog_fields(extra, app);
  cuwacunu::hero::mcp_observability::append_json_string_field(
      extra, "tool_name", tool_name);
  cuwacunu::hero::mcp_observability::append_json_bool_field(
      extra, "catalog_already_open", catalog_already_open);
  cuwacunu::hero::mcp_observability::append_json_bool_field(
      extra, "opened_here", !catalog_already_open);
  cuwacunu::hero::mcp_observability::append_json_bool_field(extra, "ok", ok);
  cuwacunu::hero::mcp_observability::append_json_bool_field(
      extra, "catalog_closed", catalog_closed);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "elapsed_ms", elapsed_ms);
  if (catalog_hold_ms != 0) {
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "catalog_hold_ms", catalog_hold_ms);
  }
  if (app) {
    cuwacunu::hero::mcp_observability::append_json_bool_field(
        extra, "close_catalog_after_execute", app->close_catalog_after_execute);
  }
  if (!error.empty()) {
    cuwacunu::hero::mcp_observability::append_json_string_field(extra, "error",
                                                                error);
  }
  std::string ignored{};
  (void)cuwacunu::hero::mcp_observability::append_event_json(
      kServerName, "tool_end", extra.str(), &ignored);
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

[[nodiscard]] std::string resolve_path_from_base_folder(std::string base_folder,
                                                        std::string path) {
  base_folder = trim_ascii(std::move(base_folder));
  path = trim_ascii(std::move(path));
  if (path.empty())
    return {};
  const std::filesystem::path p(path);
  if (p.is_absolute())
    return p.string();
  if (base_folder.empty())
    return p.string();
  return (std::filesystem::path(base_folder) / p).string();
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
    const std::string lhs = trim_ascii(trimmed.substr(0, eq));
    if (lhs != key)
      continue;
    return trim_ascii(trimmed.substr(eq + 1));
  }
  return std::nullopt;
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

[[nodiscard]] std::filesystem::path resolve_hashimyei_hero_dsl_path(
    const std::filesystem::path &global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "REAL_HERO", "hashimyei_hero_dsl_filename");
  if (!configured.has_value())
    return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty())
    return {};
  return std::filesystem::path(resolved);
}

[[nodiscard]] std::filesystem::path resolve_runtime_root_from_global_config(
    const std::filesystem::path &global_config_path) {
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "GENERAL", "runtime_root");
  if (!configured.has_value())
    return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty())
    return {};
  return std::filesystem::path(resolved);
}

[[nodiscard]] std::filesystem::path
derive_hashimyei_store_root(const std::filesystem::path &runtime_root) {
  if (runtime_root.empty())
    return {};
  return (runtime_root / ".hashimyei").lexically_normal();
}

[[nodiscard]] std::filesystem::path
derive_hashimyei_catalog_path(const std::filesystem::path &store_root) {
  if (store_root.empty())
    return {};
  return (store_root / "_meta" / "catalog" / "hashimyei_catalog.idydb")
      .lexically_normal();
}

[[nodiscard]] std::filesystem::path
derive_legacy_hashimyei_catalog_path(const std::filesystem::path &store_root) {
  if (store_root.empty())
    return {};
  return (store_root / "catalog" / "hashimyei_catalog.idydb")
      .lexically_normal();
}

[[nodiscard]] bool
load_hashimyei_runtime_defaults(const std::filesystem::path &hero_dsl_path,
                                const std::filesystem::path &global_config_path,
                                hashimyei_runtime_defaults_t *out,
                                std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "missing destination for hashimyei runtime defaults";
    return false;
  }

  std::error_code ec;
  if (!std::filesystem::exists(hero_dsl_path, ec) ||
      !std::filesystem::is_regular_file(hero_dsl_path, ec)) {
    if (error)
      *error = "hero hashimyei DSL file not found: " + hero_dsl_path.string();
    return false;
  }
  *out = hashimyei_runtime_defaults_t{};
  out->store_root = derive_hashimyei_store_root(
      resolve_runtime_root_from_global_config(global_config_path));
  out->catalog_path = derive_hashimyei_catalog_path(out->store_root);

  std::ifstream in(hero_dsl_path);
  if (!in) {
    if (error)
      *error = "cannot open hero hashimyei DSL file: " + hero_dsl_path.string();
    return false;
  }

  std::unordered_map<std::string, std::string> values{};
  std::string line;
  bool in_block_comment = false;
  while (std::getline(in, line)) {
    std::string candidate = line;
    if (in_block_comment) {
      const std::size_t end_block = candidate.find("*/");
      if (end_block == std::string::npos)
        continue;
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
    if (candidate.empty())
      continue;
    const std::size_t eq = candidate.find('=');
    if (eq == std::string::npos)
      continue;
    std::string lhs = trim_ascii(candidate.substr(0, eq));
    const std::string value =
        unquote_if_wrapped(trim_ascii(candidate.substr(eq + 1)));
    lhs = cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(lhs);
    if (lhs.empty())
      continue;
    values[lhs] = value;
  }

  bool saw_encrypted = false;
  const auto it_encrypted = values.find("encrypted");
  if (it_encrypted != values.end()) {
    saw_encrypted = true;
    bool parsed = false;
    if (!parse_bool_token(it_encrypted->second, &parsed)) {
      if (error)
        *error = "invalid bool for key encrypted in " + hero_dsl_path.string();
      return false;
    }
    if (parsed) {
      if (error) {
        *error = "encrypted=true is not supported in " +
                 hero_dsl_path.string() + "; set encrypted=false";
      }
      return false;
    }
  }
  if (out->store_root.empty()) {
    if (error)
      *error = "missing GENERAL.runtime_root in " + global_config_path.string();
    return false;
  }
  if (out->catalog_path.empty()) {
    if (error) {
      *error =
          "cannot derive hashimyei catalog path from GENERAL.runtime_root in " +
          global_config_path.string();
    }
    return false;
  }
  if (!saw_encrypted) {
    if (error)
      *error = "missing encrypted in " + hero_dsl_path.string();
    return false;
  }
  return true;
}

[[nodiscard]] std::string
maybe_hashimyei_from_canonical(std::string_view canonical) {
  const std::size_t dot = canonical.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= canonical.size())
    return {};
  std::string normalized{};
  if (!cuwacunu::hashimyei::normalize_hex_hash_name(canonical.substr(dot + 1),
                                                    &normalized)) {
    return {};
  }
  return normalized;
}

[[nodiscard]] std::string hex_lower(const unsigned char *bytes, std::size_t n) {
  static constexpr std::array<char, 16> kHex{'0', '1', '2', '3', '4', '5',
                                             '6', '7', '8', '9', 'a', 'b',
                                             'c', 'd', 'e', 'f'};
  std::string out;
  out.resize(n * 2);
  for (std::size_t i = 0; i < n; ++i) {
    const unsigned char b = bytes[i];
    out[2 * i + 0] = kHex[(b >> 4) & 0x0F];
    out[2 * i + 1] = kHex[b & 0x0F];
  }
  return out;
}

[[nodiscard]] bool sha256_hex_bytes(std::string_view payload,
                                    std::string *out_hex) {
  if (!out_hex)
    return false;
  out_hex->clear();
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx)
    return false;

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  const bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
                  EVP_DigestUpdate(ctx, payload.data(), payload.size()) == 1 &&
                  EVP_DigestFinal_ex(ctx, digest, &digest_len) == 1;
  EVP_MD_CTX_free(ctx);
  if (!ok)
    return false;
  *out_hex = hex_lower(digest, static_cast<std::size_t>(digest_len));
  return true;
}

[[nodiscard]] bool read_text_file(const std::filesystem::path &path,
                                  std::string *out, std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "output pointer is null";
    return false;
  }
  out->clear();
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error)
      *error = "cannot open file: " + path.string();
    return false;
  }
  out->assign(std::istreambuf_iterator<char>(in),
              std::istreambuf_iterator<char>());
  if (!in.eof() && in.fail()) {
    if (error)
      *error = "cannot read file: " + path.string();
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
        const char *hex = "0123456789abcdef";
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

[[nodiscard]] bool
remove_catalog_sync_state(const std::filesystem::path &catalog_path,
                          std::string *error);
[[nodiscard]] bool
remove_catalog_file_and_sync_state(const std::filesystem::path &catalog_path,
                                   std::string *error);
[[nodiscard]] bool remove_legacy_hashimyei_catalog_if_present(
    const std::filesystem::path &store_root,
    const std::filesystem::path &catalog_path, std::string *error);
[[nodiscard]] bool
ensure_hashimyei_catalog_synced_to_store(app_context_t *app, bool force_rebuild,
                                         std::string *out_error);

[[nodiscard]] bool close_catalog_if_open(app_context_t *app,
                                         std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!app || !app->catalog.opened())
    return true;
  return app->catalog.close(out_error);
}

[[nodiscard]] bool reset_catalog(app_context_t *app, bool reingest,
                                 std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!app) {
    if (out_error)
      *out_error = "missing app context";
    return false;
  }
  if (app->catalog.opened()) {
    std::string close_error;
    if (!app->catalog.close(&close_error)) {
      if (out_error)
        *out_error = "catalog close failed: " + close_error;
      return false;
    }
  }

  const std::filesystem::path catalog_path = app->catalog_options.catalog_path;
  if (catalog_path.empty()) {
    if (out_error)
      *out_error = "catalog_path is empty";
    return false;
  }

  if (!remove_catalog_file_and_sync_state(catalog_path, out_error))
    return false;
  if (!remove_legacy_hashimyei_catalog_if_present(app->store_root, catalog_path,
                                                  out_error)) {
    return false;
  }

  app->last_store_sync_token.clear();
  if (reingest) {
    return ensure_hashimyei_catalog_synced_to_store(app, true, out_error);
  }

  return app->catalog.open(app->catalog_options, out_error);
}

void write_jsonrpc_payload(std::string_view payload) {
  std::string framed;
  if (g_jsonrpc_use_content_length_framing) {
    framed = std::string("Content-Length: ") + std::to_string(payload.size()) +
             "\r\n\r\n" + std::string(payload);
  } else {
    framed = std::string(payload) + "\n";
  }
  (void)write_all_fd(STDOUT_FILENO, framed.data(), framed.size());
}

void write_jsonrpc_result(std::string_view id_json,
                          std::string_view result_json) {
  std::ostringstream out;
  out << "{\"jsonrpc\":\"2.0\",\"id\":" << id_json
      << ",\"result\":" << result_json << "}";
  write_jsonrpc_payload(out.str());
}

void write_jsonrpc_error(std::string_view id_json, int code,
                         std::string_view message) {
  std::ostringstream out;
  out << "{\"jsonrpc\":\"2.0\",\"id\":" << id_json << ",\"error\":{"
      << "\"code\":" << code << ",\"message\":" << json_quote(message) << "}}";
  write_jsonrpc_payload(out.str());
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
  raw = trim_ascii(raw);
  if (raw.empty() || raw.front() != '{')
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

[[nodiscard]] bool extract_json_size_field(const std::string &json,
                                           const std::string &key,
                                           std::size_t *out) {
  if (!out)
    return false;
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw))
    return false;
  raw = trim_ascii(raw);
  if (raw.empty())
    return false;
  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(raw.data(), raw.data() + raw.size(), parsed);
  if (ec != std::errc() || ptr != raw.data() + raw.size())
    return false;
  *out = static_cast<std::size_t>(parsed);
  return true;
}

[[nodiscard]] bool
extract_json_string_array_field(const std::string &json, const std::string &key,
                                std::vector<std::string> *out) {
  if (!out)
    return false;
  out->clear();

  std::string raw;
  if (!extract_json_raw_field(json, key, &raw))
    return false;
  std::size_t pos = skip_json_whitespace(raw, 0);
  if (pos >= raw.size() || raw[pos] != '[')
    return false;
  ++pos;
  while (true) {
    pos = skip_json_whitespace(raw, pos);
    if (pos >= raw.size())
      return false;
    if (raw[pos] == ']') {
      ++pos;
      pos = skip_json_whitespace(raw, pos);
      return pos == raw.size();
    }
    std::string value{};
    std::size_t end_pos = pos;
    if (!parse_json_string_at(raw, pos, &value, &end_pos))
      return false;
    out->push_back(value);
    pos = skip_json_whitespace(raw, end_pos);
    if (pos >= raw.size())
      return false;
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

[[nodiscard]] std::string
hashimyei_identity_to_json(const cuwacunu::hashimyei::hashimyei_t &id) {
  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(id.schema) << ","
      << "\"kind\":"
      << json_quote(cuwacunu::hashimyei::hashimyei_kind_to_string(id.kind))
      << ","
      << "\"name\":" << json_quote(id.name) << ","
      << "\"ordinal\":" << id.ordinal << ","
      << "\"hash_sha256_hex\":" << json_quote(id.hash_sha256_hex) << "}";
  return out.str();
}

[[nodiscard]] std::string component_state_to_json(
    const cuwacunu::hero::hashimyei::component_state_t &component) {
  const auto &m = component.manifest;
  std::ostringstream out;
  out << "{"
      << "\"component_id\":" << json_quote(component.component_id) << ","
      << "\"ts_ms\":" << component.ts_ms << ","
      << "\"manifest_path\":" << json_quote(component.manifest_path) << ","
      << "\"report_fragment_sha256\":"
      << json_quote(component.report_fragment_sha256) << ","
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
      << "\"founding_revision_id\":" << json_quote(m.founding_revision_id)
      << ","
      << "\"founding_dsl_source_path\":"
      << json_quote(m.founding_dsl_source_path) << ","
      << "\"founding_dsl_source_sha256_hex\":"
      << json_quote(m.founding_dsl_source_sha256_hex) << ","
      << "\"docking_signature_sha256_hex\":"
      << json_quote(m.docking_signature_sha256_hex) << ","
      << "\"lineage_state\":" << json_quote(m.lineage_state) << ","
      << "\"replaced_by\":" << json_quote(m.replaced_by) << ","
      << "\"created_at_ms\":" << m.created_at_ms << ","
      << "\"updated_at_ms\":" << m.updated_at_ms << "}"
      << "}";
  return out.str();
}

[[nodiscard]] std::string
family_rank_state_to_json(const cuwacunu::hero::family_rank::state_t &state) {
  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(state.schema) << ","
      << "\"family\":" << json_quote(state.family) << ","
      << "\"dock_hash\":"
      << (state.dock_hash.empty() ? "null" : json_quote(state.dock_hash))
      << ",\"source_view_kind\":"
      << (state.source_view_kind.empty() ? "null"
                                         : json_quote(state.source_view_kind))
      << ",\"source_view_transport_sha256\":"
      << (state.source_view_transport_sha256.empty()
              ? "null"
              : json_quote(state.source_view_transport_sha256))
      << ",\"updated_at_ms\":" << state.updated_at_ms << ",\"assignments\":[";
  for (std::size_t i = 0; i < state.assignments.size(); ++i) {
    const auto &assignment = state.assignments[i];
    if (i != 0)
      out << ",";
    out << "{"
        << "\"rank\":" << assignment.rank << ","
        << "\"hashimyei\":" << json_quote(assignment.hashimyei) << ","
        << "\"canonical_path\":" << json_quote(assignment.canonical_path) << ","
        << "\"component_id\":" << json_quote(assignment.component_id) << "}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string
sanitize_family_rank_path_token(std::string_view family) {
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
  if (out.empty())
    out = "family";
  return out;
}

[[nodiscard]] std::filesystem::path
family_rank_file_path(const std::filesystem::path &store_root,
                      std::string_view family, std::string_view dock_hash) {
  std::string family_sha{};
  if (!sha256_hex_bytes(std::string(family), &family_sha))
    family_sha.clear();
  std::string dock_sha{};
  if (!sha256_hex_bytes(std::string(dock_hash), &dock_sha)) {
    dock_sha.clear();
  }
  const std::string folder =
      sanitize_family_rank_path_token(family) +
      (family_sha.empty() ? std::string{} : ("_" + family_sha.substr(0, 12)));
  const std::string dock_folder =
      sanitize_family_rank_path_token(dock_hash) +
      (dock_sha.empty() ? std::string{} : ("_" + dock_sha.substr(0, 12)));
  return store_root / "hero" / "family_rank" / folder / dock_folder /
         "family.rank.latest.lls";
}

[[nodiscard]] bool write_text_file_atomic(const std::filesystem::path &path,
                                          std::string_view payload,
                                          std::string *error) {
  if (error)
    error->clear();
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  if (ec) {
    if (error)
      *error = "cannot create rank directory: " + path.parent_path().string();
    return false;
  }
  const std::filesystem::path tmp_path = path.string() + ".tmp";
  {
    std::ofstream out(tmp_path, std::ios::binary | std::ios::trunc);
    if (!out) {
      if (error)
        *error = "cannot open temp rank file: " + tmp_path.string();
      return false;
    }
    out << payload;
    if (!out) {
      if (error)
        *error = "cannot write temp rank file: " + tmp_path.string();
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
    if (error)
      *error = "cannot finalize rank file: " + path.string();
    return false;
  }
  return true;
}

[[nodiscard]] std::optional<std::filesystem::path>
canonicalized(const std::filesystem::path &path) {
  std::error_code ec{};
  const auto canonical = std::filesystem::weakly_canonical(path, ec);
  if (ec)
    return std::nullopt;
  return canonical.lexically_normal();
}

[[nodiscard]] bool path_is_within(std::filesystem::path base,
                                  std::filesystem::path candidate) {
  base = base.lexically_normal();
  candidate = candidate.lexically_normal();
  auto b = base.begin();
  auto c = candidate.begin();
  for (; b != base.end() && c != candidate.end(); ++b, ++c) {
    if (*b != *c)
      return false;
  }
  return b == base.end();
}

[[nodiscard]] std::filesystem::path
catalog_sync_state_path(const std::filesystem::path &catalog_path) {
  return catalog_path.parent_path() /
         (catalog_path.filename().string() + ".sync.sha256");
}

[[nodiscard]] bool
read_catalog_sync_state(const std::filesystem::path &catalog_path,
                        std::string *out, std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "sync state output pointer is null";
    return false;
  }
  out->clear();
  const auto path = catalog_sync_state_path(catalog_path);
  std::error_code ec{};
  if (!std::filesystem::exists(path, ec))
    return true;
  if (!read_text_file(path, out, error))
    return false;
  *out = trim_ascii(*out);
  return true;
}

[[nodiscard]] bool
write_catalog_sync_state(const std::filesystem::path &catalog_path,
                         std::string_view token, std::string *error) {
  return write_text_file_atomic(catalog_sync_state_path(catalog_path),
                                std::string(token) + "\n", error);
}

[[nodiscard]] bool
remove_catalog_sync_state(const std::filesystem::path &catalog_path,
                          std::string *error) {
  if (error)
    error->clear();
  std::error_code ec{};
  const auto sync_path = catalog_sync_state_path(catalog_path);
  std::filesystem::remove(sync_path, ec);
  if (ec && std::filesystem::exists(sync_path)) {
    if (error)
      *error = "cannot remove catalog sync state: " + sync_path.string();
    return false;
  }
  return true;
}

[[nodiscard]] bool
remove_catalog_file_and_sync_state(const std::filesystem::path &catalog_path,
                                   std::string *error) {
  if (error)
    error->clear();
  if (catalog_path.empty())
    return true;

  std::error_code ec{};
  std::filesystem::remove(catalog_path, ec);
  if (ec && std::filesystem::exists(catalog_path)) {
    if (error)
      *error = "cannot remove catalog file: " + catalog_path.string();
    return false;
  }
  return remove_catalog_sync_state(catalog_path, error);
}

[[nodiscard]] bool remove_legacy_hashimyei_catalog_if_present(
    const std::filesystem::path &store_root,
    const std::filesystem::path &catalog_path, std::string *error) {
  const std::filesystem::path legacy_path =
      derive_legacy_hashimyei_catalog_path(store_root);
  if (legacy_path.empty() || legacy_path == catalog_path)
    return true;
  return remove_catalog_file_and_sync_state(legacy_path, error);
}

[[nodiscard]] bool
should_include_hashimyei_sync_file(const std::filesystem::path &path) {
  if (path.filename() == cuwacunu::hashimyei::kRunManifestFilenameV2)
    return true;
  if (path.filename() == cuwacunu::hashimyei::kComponentManifestFilenameV2) {
    return true;
  }
  return path.extension() == ".lls";
}

[[nodiscard]] bool
compute_hashimyei_store_sync_token(const std::filesystem::path &store_root,
                                   const std::filesystem::path &catalog_path,
                                   std::string *out_token, std::string *error) {
  if (error)
    error->clear();
  if (!out_token) {
    if (error)
      *error = "sync token output pointer is null";
    return false;
  }
  out_token->clear();

  const std::filesystem::path normalized_store_root =
      store_root.lexically_normal();
  const std::filesystem::path normalized_catalog_root =
      catalog_path.parent_path().empty()
          ? std::filesystem::path{}
          : catalog_path.parent_path().lexically_normal();

  std::vector<std::pair<std::string, std::string>> entries{};
  std::error_code ec{};
  if (std::filesystem::exists(store_root, ec) &&
      std::filesystem::is_directory(store_root, ec)) {
    for (std::filesystem::recursive_directory_iterator it(store_root, ec), end;
         it != end; it.increment(ec)) {
      if (ec) {
        if (error)
          *error = "failed scanning store root for hashimyei sync";
        return false;
      }
      std::error_code entry_ec{};
      const auto symlink_state = it->symlink_status(entry_ec);
      if (entry_ec) {
        if (error)
          *error = "failed reading store entry status";
        return false;
      }
      if (std::filesystem::is_symlink(symlink_state))
        continue;
      if (!it->is_regular_file(entry_ec)) {
        if (entry_ec) {
          if (error)
            *error = "failed reading store entry type";
          return false;
        }
        continue;
      }

      const auto entry_path = it->path();
      const auto normalized_entry = entry_path.lexically_normal();
      if (!path_is_within(normalized_store_root, normalized_entry))
        continue;
      if (!normalized_catalog_root.empty() &&
          path_is_within(normalized_catalog_root, normalized_entry)) {
        continue;
      }
      if (!should_include_hashimyei_sync_file(entry_path))
        continue;

      const auto file_size = it->file_size(entry_ec);
      if (entry_ec) {
        if (error)
          *error = "failed reading store entry size";
        return false;
      }
      const auto last_write_time = it->last_write_time(entry_ec);
      if (entry_ec) {
        if (error)
          *error = "failed reading store entry mtime";
        return false;
      }
      const auto mtime_ticks =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              last_write_time.time_since_epoch())
              .count();
      std::ostringstream fingerprint;
      fingerprint << file_size << "|" << mtime_ticks;

      std::filesystem::path rel =
          normalized_entry.lexically_relative(normalized_store_root);
      if (rel.empty())
        rel = normalized_entry.filename();
      entries.emplace_back(rel.generic_string(), fingerprint.str());
    }
  }

  std::sort(entries.begin(), entries.end());
  std::ostringstream seed;
  seed << "store_root=" << normalized_store_root.generic_string() << "\n";
  for (const auto &[rel, fingerprint] : entries) {
    seed << rel << "|" << fingerprint << "\n";
  }
  if (!sha256_hex_bytes(seed.str(), out_token)) {
    if (error)
      *error = "cannot compute hashimyei store sync token";
    return false;
  }
  return true;
}

[[nodiscard]] bool
rebuild_hashimyei_catalog_for_token(app_context_t *app,
                                    std::string_view expected_token,
                                    std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!app) {
    if (out_error)
      *out_error = "missing app context";
    return false;
  }
  const std::filesystem::path catalog_path = app->catalog_options.catalog_path;
  if (catalog_path.empty()) {
    if (out_error)
      *out_error = "catalog_path is empty";
    return false;
  }

  if (app->catalog.opened()) {
    std::string close_error;
    if (!app->catalog.close(&close_error)) {
      if (out_error)
        *out_error = "catalog close failed: " + close_error;
      return false;
    }
  }

  if (!remove_catalog_file_and_sync_state(catalog_path, out_error))
    return false;
  if (!remove_legacy_hashimyei_catalog_if_present(app->store_root, catalog_path,
                                                  out_error)) {
    return false;
  }

  if (!app->catalog.open(app->catalog_options, out_error))
    return false;
  if (!app->catalog.ingest_filesystem(app->store_root, out_error))
    return false;
  if (!write_catalog_sync_state(catalog_path, expected_token, out_error)) {
    return false;
  }
  app->last_store_sync_token = std::string(expected_token);
  return true;
}

[[nodiscard]] bool
ensure_hashimyei_catalog_synced_to_store(app_context_t *app, bool force_rebuild,
                                         std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!app) {
    if (out_error)
      *out_error = "missing app context";
    return false;
  }
  if (app->store_root.empty()) {
    if (out_error)
      *out_error = "store_root is empty";
    return false;
  }
  const std::filesystem::path catalog_path = app->catalog_options.catalog_path;
  if (catalog_path.empty()) {
    if (out_error)
      *out_error = "catalog_path is empty";
    return false;
  }
  std::string current_token{};
  if (!compute_hashimyei_store_sync_token(app->store_root, catalog_path,
                                          &current_token, out_error)) {
    return false;
  }

  std::error_code ec{};
  const bool catalog_exists = std::filesystem::exists(catalog_path, ec);
  if (!force_rebuild && catalog_exists &&
      app->last_store_sync_token == current_token) {
    return true;
  }

  std::string recorded_token{};
  if (!read_catalog_sync_state(catalog_path, &recorded_token, out_error)) {
    return false;
  }
  if (!force_rebuild && catalog_exists && recorded_token == current_token) {
    app->last_store_sync_token = current_token;
    return true;
  }

  for (int attempt = 0; attempt < 2; ++attempt) {
    if (!rebuild_hashimyei_catalog_for_token(app, current_token, out_error)) {
      return false;
    }
    std::string observed_token{};
    if (!compute_hashimyei_store_sync_token(app->store_root, catalog_path,
                                            &observed_token, out_error)) {
      return false;
    }
    if (observed_token == current_token) {
      app->last_store_sync_token = observed_token;
      return true;
    }
    current_token = observed_token;
  }

  if (out_error) {
    *out_error = "runtime store changed while synchronizing hashimyei catalog";
  }
  return false;
}

