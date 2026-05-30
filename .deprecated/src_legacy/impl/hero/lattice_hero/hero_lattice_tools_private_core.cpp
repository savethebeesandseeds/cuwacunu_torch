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
#include <poll.h>
#include <set>
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
#include "hero/mcp_server_observability.h"
#include "piaabo/dlogs.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"

namespace {

constexpr const char* kServerName = "hero_lattice_mcp";
constexpr const char* kServerVersion = "0.1.0";
constexpr const char* kProtocolVersion = "2025-03-26";
constexpr const char* kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.lattice.*.";
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;  // 8 MiB
constexpr const char* kStdioIdleTimeoutEnv = "HERO_MCP_IDLE_TIMEOUT_SEC";
constexpr int kDefaultStdioIdleTimeoutSec = 600;

bool g_jsonrpc_use_content_length_framing = false;
int g_protocol_stdout_fd = -1;

using app_context_t = cuwacunu::hero::lattice_mcp::app_context_t;
using wave_runtime_defaults_t = cuwacunu::hero::lattice_mcp::wave_runtime_defaults_t;

[[nodiscard]] bool sha256_hex_bytes(std::string_view payload, std::string* out_hex,
                                    std::string* error);
[[nodiscard]] bool read_text_file(const std::filesystem::path& path,
                                  std::string* out,
                                  std::string* error);

__attribute__((constructor(101))) void disable_terminal_logs_pre_main() {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
}

[[nodiscard]] std::uint64_t now_ms_utc() {
  const auto now = std::chrono::system_clock::now();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
          .count());
}

void append_lattice_tool_catalog_fields(std::ostringstream& extra,
                                        const app_context_t* app) {
  if (!app) return;
  if (!app->lattice_catalog_path.empty()) {
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", app->lattice_catalog_path.string());
  }
}

void log_lattice_tool_begin(const app_context_t* app, std::string_view tool_name,
                            bool catalog_already_open) {
  std::ostringstream extra;
  append_lattice_tool_catalog_fields(extra, app);
  cuwacunu::hero::mcp_observability::append_json_string_field(extra, "tool_name",
                                                              tool_name);
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

void log_lattice_tool_end(const app_context_t* app, std::string_view tool_name,
                          bool catalog_already_open, bool ok,
                          bool catalog_closed, std::uint64_t elapsed_ms,
                          std::uint64_t catalog_hold_ms = 0,
                          std::string_view error = {}) {
  std::ostringstream extra;
  append_lattice_tool_catalog_fields(extra, app);
  cuwacunu::hero::mcp_observability::append_json_string_field(extra, "tool_name",
                                                              tool_name);
  cuwacunu::hero::mcp_observability::append_json_bool_field(
      extra, "catalog_already_open", catalog_already_open);
  cuwacunu::hero::mcp_observability::append_json_bool_field(
      extra, "opened_here", !catalog_already_open);
  cuwacunu::hero::mcp_observability::append_json_bool_field(extra, "ok", ok);
  cuwacunu::hero::mcp_observability::append_json_bool_field(
      extra, "catalog_closed", catalog_closed);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(extra, "elapsed_ms",
                                                              elapsed_ms);
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

[[nodiscard]] std::string effective_fragment_canonical_path(
    const cuwacunu::hero::wave::runtime_report_fragment_t& fragment) {
  return normalize_source_hashimyei_cursor(fragment.canonical_path);
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

[[nodiscard]] std::optional<int> resolve_stdio_idle_timeout_ms() {
  static const std::optional<int> cached_timeout_ms = []() -> std::optional<int> {
    const char* raw = std::getenv(kStdioIdleTimeoutEnv);
    if (raw == nullptr || *raw == '\0') {
      return kDefaultStdioIdleTimeoutSec * 1000;
    }

    errno = 0;
    char* end = nullptr;
    const long long parsed = std::strtoll(raw, &end, 10);
    while (end != nullptr &&
           std::isspace(static_cast<unsigned char>(*end)) != 0) {
      ++end;
    }
    if (errno != 0 || end == raw || (end != nullptr && *end != '\0')) {
      return kDefaultStdioIdleTimeoutSec * 1000;
    }
    if (parsed <= 0) return std::nullopt;

    constexpr long long kMaxTimeoutSec =
        static_cast<long long>(std::numeric_limits<int>::max()) / 1000;
    if (parsed >= kMaxTimeoutSec) return std::numeric_limits<int>::max();
    return static_cast<int>(parsed * 1000);
  }();
  return cached_timeout_ms;
}

[[nodiscard]] bool wait_for_stdio_activity() {
  const std::optional<int> timeout_ms = resolve_stdio_idle_timeout_ms();
  if (!timeout_ms.has_value()) return true;

  struct pollfd pfd {};
  pfd.fd = STDIN_FILENO;
  pfd.events = POLLIN | POLLHUP | POLLERR;
  while (true) {
    const int rc = ::poll(&pfd, 1, *timeout_ms);
    if (rc > 0) return true;
    if (rc == 0) return false;
    if (errno == EINTR) continue;
    return true;
  }
}

[[nodiscard]] std::optional<std::filesystem::path> canonicalized(
    const std::filesystem::path& path) {
  std::error_code ec{};
  const auto canonical = std::filesystem::weakly_canonical(path, ec);
  if (ec) return std::nullopt;
  return canonical.lexically_normal();
}

[[nodiscard]] bool path_is_within(std::filesystem::path base,
                                  std::filesystem::path candidate) {
  base = base.lexically_normal();
  candidate = candidate.lexically_normal();
  auto b = base.begin();
  auto c = candidate.begin();
  for (; b != base.end() && c != candidate.end(); ++b, ++c) {
    if (*b != *c) return false;
  }
  return b == base.end();
}

[[nodiscard]] bool write_text_file_atomic(const std::filesystem::path& path,
                                          std::string_view payload,
                                          std::string* error) {
  if (error) error->clear();
  std::error_code ec{};
  std::filesystem::create_directories(path.parent_path(), ec);
  if (ec) {
    if (error) *error = "cannot create parent directory: " + path.parent_path().string();
    return false;
  }
  const auto tmp = path.string() + ".tmp";
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) {
      if (error) *error = "cannot open temp file for write: " + tmp;
      return false;
    }
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    if (!out) {
      if (error) *error = "cannot write temp file: " + tmp;
      return false;
    }
  }
  std::filesystem::rename(tmp, path, ec);
  if (ec) {
    std::filesystem::remove(tmp, ec);
    if (error) *error = "cannot rename temp file into place: " + path.string();
    return false;
  }
  return true;
}

[[nodiscard]] std::filesystem::path catalog_sync_state_path(
    const std::filesystem::path& catalog_path) {
  return catalog_path.parent_path() /
         (catalog_path.filename().string() + ".sync.sha256");
}

[[nodiscard]] std::filesystem::path legacy_lattice_catalog_path(
    const std::filesystem::path& store_root) {
  if (store_root.empty()) return {};
  return (store_root / "catalog" / "lattice_catalog.idydb").lexically_normal();
}

[[nodiscard]] bool read_catalog_sync_state(
    const std::filesystem::path& catalog_path, std::string* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "sync state output pointer is null";
    return false;
  }
  out->clear();
  const auto path = catalog_sync_state_path(catalog_path);
  std::error_code ec{};
  if (!std::filesystem::exists(path, ec)) return true;
  std::string payload{};
  if (!read_text_file(path, &payload, error)) return false;
  *out = trim_ascii(payload);
  return true;
}

[[nodiscard]] bool write_catalog_sync_state(
    const std::filesystem::path& catalog_path, std::string_view token,
    std::string* error) {
  return write_text_file_atomic(catalog_sync_state_path(catalog_path),
                                std::string(token) + "\n", error);
}

[[nodiscard]] bool remove_catalog_sync_state(
    const std::filesystem::path& catalog_path, std::string* error) {
  if (error) error->clear();
  std::error_code ec{};
  const auto sync_path = catalog_sync_state_path(catalog_path);
  std::filesystem::remove(sync_path, ec);
  if (ec && std::filesystem::exists(sync_path)) {
    if (error) *error = "cannot remove catalog sync state: " + sync_path.string();
    return false;
  }
  return true;
}

[[nodiscard]] bool remove_catalog_file_and_sync_state(
    const std::filesystem::path& catalog_path, std::string* error) {
  if (error) error->clear();
  if (catalog_path.empty()) return true;

  std::error_code ec{};
  std::filesystem::remove(catalog_path, ec);
  if (ec && std::filesystem::exists(catalog_path)) {
    if (error) *error = "cannot remove lattice catalog file: " + catalog_path.string();
    return false;
  }
  return remove_catalog_sync_state(catalog_path, error);
}

[[nodiscard]] bool remove_legacy_lattice_catalog_if_present(
    const std::filesystem::path& store_root,
    const std::filesystem::path& catalog_path, std::string* error) {
  const std::filesystem::path legacy_path = legacy_lattice_catalog_path(store_root);
  if (legacy_path.empty() || legacy_path == catalog_path) return true;
  return remove_catalog_file_and_sync_state(legacy_path, error);
}

[[nodiscard]] bool should_include_lattice_sync_file(
    const std::filesystem::path& path) {
  if (path.filename() == cuwacunu::hashimyei::kRunManifestFilenameV2) return true;
  if (path.filename() == cuwacunu::hashimyei::kComponentManifestFilenameV2) {
    return true;
  }
  return path.extension() == ".lls";
}

[[nodiscard]] bool compute_lattice_store_sync_token(
    const std::filesystem::path& store_root,
    const std::filesystem::path& catalog_path, std::string* out_token,
    std::string* error) {
  if (error) error->clear();
  if (!out_token) {
    if (error) *error = "sync token output pointer is null";
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
        if (error) *error = "failed scanning store root for lattice sync";
        return false;
      }
      std::error_code entry_ec{};
      const auto symlink_state = it->symlink_status(entry_ec);
      if (entry_ec) {
        if (error) *error = "failed reading store entry status";
        return false;
      }
      if (std::filesystem::is_symlink(symlink_state)) continue;
      if (!it->is_regular_file(entry_ec)) {
        if (entry_ec) {
          if (error) *error = "failed reading store entry type";
          return false;
        }
        continue;
      }

      const auto entry_path = it->path();
      const auto normalized_entry = entry_path.lexically_normal();
      if (!path_is_within(normalized_store_root, normalized_entry)) continue;
      if (!normalized_catalog_root.empty() &&
          path_is_within(normalized_catalog_root, normalized_entry)) {
        continue;
      }
      if (!should_include_lattice_sync_file(entry_path)) continue;

      const auto file_size = it->file_size(entry_ec);
      if (entry_ec) {
        if (error) *error = "failed reading store entry size";
        return false;
      }
      const auto last_write_time = it->last_write_time(entry_ec);
      if (entry_ec) {
        if (error) *error = "failed reading store entry mtime";
        return false;
      }
      const auto mtime_ticks = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   last_write_time.time_since_epoch())
                                   .count();
      std::ostringstream fingerprint;
      fingerprint << file_size << "|" << mtime_ticks;

      std::filesystem::path rel =
          normalized_entry.lexically_relative(normalized_store_root);
      if (rel.empty()) rel = normalized_entry.filename();
      entries.emplace_back(rel.generic_string(), fingerprint.str());
    }
  }

  std::sort(entries.begin(), entries.end());
  std::ostringstream seed;
  seed << "store_root=" << normalized_store_root.generic_string() << "\n";
  for (const auto& [rel, fingerprint] : entries) {
    seed << rel << "|" << fingerprint << "\n";
  }
  return sha256_hex_bytes(seed.str(), out_token, error);
}

[[nodiscard]] bool rebuild_lattice_catalog_for_token(
    app_context_t* app, std::string_view expected_token, std::string* out_error) {
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

  if (!remove_catalog_file_and_sync_state(app->lattice_catalog_path, out_error)) {
    return false;
  }
  if (!remove_legacy_lattice_catalog_if_present(app->store_root,
                                                app->lattice_catalog_path,
                                                out_error)) {
    return false;
  }

  cuwacunu::hero::wave::lattice_catalog_store_t::options_t opts{};
  opts.catalog_path = app->lattice_catalog_path;
  opts.encrypted = false;
  opts.projection_version = 2;
  opts.runtime_only_indexes = true;
  if (!app->catalog.open(opts, out_error)) return false;
  if (!app->catalog.ingest_runtime_report_fragments(app->store_root, out_error)) {
    return false;
  }
  if (!write_catalog_sync_state(app->lattice_catalog_path, expected_token,
                                out_error)) {
    return false;
  }
  app->last_store_sync_token = std::string(expected_token);
  return true;
}

[[nodiscard]] bool ensure_lattice_catalog_synced_to_store(
    app_context_t* app, bool force_rebuild, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app) {
    if (out_error) *out_error = "app context is null";
    return false;
  }
  if (app->store_root.empty()) {
    if (out_error) *out_error = "store_root is empty";
    return false;
  }
  if (app->lattice_catalog_path.empty()) {
    if (out_error) *out_error = "lattice catalog path is empty";
    return false;
  }
  std::string current_token{};
  if (!compute_lattice_store_sync_token(app->store_root, app->lattice_catalog_path,
                                        &current_token, out_error)) {
    return false;
  }

  std::error_code ec{};
  const bool catalog_exists = std::filesystem::exists(app->lattice_catalog_path, ec);
  if (!force_rebuild && catalog_exists &&
      app->last_store_sync_token == current_token) {
    return true;
  }

  std::string recorded_token{};
  if (!read_catalog_sync_state(app->lattice_catalog_path, &recorded_token,
                               out_error)) {
    return false;
  }
  if (!force_rebuild && catalog_exists && recorded_token == current_token) {
    app->last_store_sync_token = current_token;
    return true;
  }

  for (int attempt = 0; attempt < 2; ++attempt) {
    if (!rebuild_lattice_catalog_for_token(app, current_token, out_error)) {
      return false;
    }
    std::string observed_token{};
    if (!compute_lattice_store_sync_token(app->store_root, app->lattice_catalog_path,
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
    *out_error =
        "runtime store changed while synchronizing lattice catalog";
  }
  return false;
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

[[nodiscard]] bool extract_wave_cursor_from_payload(
    std::string_view payload,
    std::uint64_t* out_wave_cursor) {
  if (!out_wave_cursor) return false;
  *out_wave_cursor = 0;
  std::string wave_cursor_text{};
  return extract_payload_kv(payload, "wave_cursor", &wave_cursor_text) &&
         parse_u64_token(wave_cursor_text, out_wave_cursor);
}

[[nodiscard]] bool fragment_wave_cursor(
    const cuwacunu::hero::wave::runtime_report_fragment_t& fragment,
    std::uint64_t* out_wave_cursor) {
  if (!out_wave_cursor) return false;
  *out_wave_cursor = 0;
  if (fragment.wave_cursor != 0) {
    *out_wave_cursor = fragment.wave_cursor;
    return true;
  }
  return extract_wave_cursor_from_payload(fragment.payload_json, out_wave_cursor);
}

template <typename SetLike>
[[nodiscard]] std::string json_string_array_from_set(const SetLike& values) {
  std::ostringstream out;
  out << "[";
  bool first = true;
  for (const auto& value : values) {
    if (!first) out << ",";
    first = false;
    out << json_quote(value);
  }
  out << "]";
  return out.str();
}

template <typename VectorLike>
[[nodiscard]] std::string json_string_array_from_vector(const VectorLike& values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) out << ",";
    out << json_quote(values[i]);
  }
  out << "]";
  return out.str();
}

template <typename VectorLike>
[[nodiscard]] std::string json_wave_cursor_array_from_vector(
    const VectorLike& values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) out << ",";
    out << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                          format_runtime_wave_cursor(values[i]));
  }
  out << "]";
  return out.str();
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

[[nodiscard]] std::filesystem::path resolve_lattice_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "REAL_HERO", "lattice_hero_dsl_filename");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved);
}

[[nodiscard]] bool load_wave_runtime_defaults(
    const std::filesystem::path& hero_dsl_path,
    const std::filesystem::path& global_config_path,
    wave_runtime_defaults_t* out,
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
  *out = wave_runtime_defaults_t{};
  const std::optional<std::string> runtime_root_value =
      read_ini_value(global_config_path, "GENERAL", "runtime_root");
  if (runtime_root_value.has_value()) {
    const std::string resolved_runtime_root = resolve_path_from_folder(
        global_config_path.parent_path().string(), *runtime_root_value);
    if (!resolved_runtime_root.empty()) {
      out->store_root =
          (std::filesystem::path(resolved_runtime_root) / ".hashimyei")
              .lexically_normal();
      out->catalog_path =
          (out->store_root / "_meta" / "catalog" / "lattice_catalog.idydb")
              .lexically_normal();
    }
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
  if (out->store_root.empty()) {
    if (error) *error = "missing GENERAL.runtime_root in " + global_config_path.string();
    return false;
  }
  if (out->catalog_path.empty()) {
    if (error) {
      *error = "cannot derive lattice catalog path from GENERAL.runtime_root in " +
               global_config_path.string();
    }
    return false;
  }
  if (!saw_encrypted) {
    if (error) *error = "missing encrypted in " + hero_dsl_path.string();
    return false;
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
    std::string hashimyei_cursor;
    if (!extract_payload_kv(fragment.payload_json, "hashimyei_cursor",
                            &hashimyei_cursor)) {
      hashimyei_cursor = effective_fragment_canonical_path(fragment);
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
    if (!fragment.semantic_taxon.empty()) {
      out << "semantic_taxon=" << fragment.semantic_taxon << "\n";
    }
    if (!fragment.report_canonical_path.empty()) {
      out << "report_canonical_path=" << fragment.report_canonical_path << "\n";
    }
    if (!fragment.binding_id.empty()) {
      out << "binding_id=" << fragment.binding_id << "\n";
    }
    if (!fragment.source_runtime_cursor.empty()) {
      out << "source_runtime_cursor=" << fragment.source_runtime_cursor << "\n";
    }
    out << "wave_cursor=" << wave_cursor << "\n";
    out << "wave_cursor_view=" << wave_cursor_view << "\n";
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

  if (!remove_catalog_file_and_sync_state(app->lattice_catalog_path, out_error)) {
    return false;
  }
  if (!remove_legacy_lattice_catalog_if_present(app->store_root,
                                                app->lattice_catalog_path,
                                                out_error)) {
    return false;
  }

  app->last_store_sync_token.clear();
  if (reingest_report_fragments) {
    return ensure_lattice_catalog_synced_to_store(app, true, out_error);
  }

  cuwacunu::hero::wave::lattice_catalog_store_t::options_t opts{};
  opts.catalog_path = app->lattice_catalog_path;
  opts.encrypted = false;
  opts.projection_version = 2;
  opts.runtime_only_indexes = true;
  return app->catalog.open(opts, out_error);
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

