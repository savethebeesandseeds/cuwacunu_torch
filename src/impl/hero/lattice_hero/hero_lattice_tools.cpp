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
[[nodiscard]] bool handle_tool_browser_tree(app_context_t* app,
                                            const std::string& arguments_json,
                                            std::string* out_structured,
                                            std::string* out_error);
[[nodiscard]] bool handle_tool_browser_detail(app_context_t* app,
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
  std::size_t fragment_count{0};
  std::set<std::string> binding_ids{};
  std::set<std::string> semantic_taxa{};
  std::set<std::string> source_runtime_cursors{};
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
    if (!fragment_wave_cursor(row, &row_wave_cursor)) {
      continue;
    }
    auto& summary = by_wave[row_wave_cursor];
    summary.canonical_path = normalize_source_hashimyei_cursor(canonical_path);
    summary.wave_cursor = row_wave_cursor;
    ++summary.fragment_count;
    if (!row.binding_id.empty()) {
      summary.binding_ids.insert(trim_ascii(row.binding_id));
    }
    if (!row.semantic_taxon.empty()) {
      summary.semantic_taxa.insert(trim_ascii(row.semantic_taxon));
    }
    if (!row.source_runtime_cursor.empty()) {
      summary.source_runtime_cursors.insert(trim_ascii(row.source_runtime_cursor));
    }
    if (row.ts_ms > summary.latest_ts_ms) {
      summary.latest_ts_ms = row.ts_ms;
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

[[nodiscard]] bool tool_requires_catalog_sync(std::string_view tool_name) {
  return tool_name == "hero.lattice.list_facts" ||
         tool_name == "hero.lattice.get_fact" ||
         tool_name == "hero.lattice.get_view";
}

[[nodiscard]] bool tool_requires_catalog_preopen(std::string_view tool_name) {
  return tool_name != "hero.lattice.refresh";
}

[[nodiscard]] std::vector<std::string> split_browser_segments(
    std::string_view text) {
  std::vector<std::string> out{};
  std::size_t start = 0;
  while (start <= text.size()) {
    const std::size_t dot = text.find('.', start);
    const std::string_view segment =
        dot == std::string_view::npos ? text.substr(start)
                                      : text.substr(start, dot - start);
    if (!segment.empty()) out.emplace_back(segment);
    if (dot == std::string_view::npos) break;
    start = dot + 1;
  }
  return out;
}

[[nodiscard]] bool browser_has_hashimyei_suffix(std::string_view canonical_path,
                                                std::string* out_hashimyei =
                                                    nullptr) {
  const std::size_t dot = canonical_path.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= canonical_path.size()) {
    return false;
  }
  std::string normalized{};
  if (!cuwacunu::hashimyei::normalize_hex_hash_name(
          std::string_view(canonical_path).substr(dot + 1), &normalized)) {
    return false;
  }
  if (out_hashimyei) *out_hashimyei = std::move(normalized);
  return true;
}

[[nodiscard]] std::string browser_project_path_for_canonical(
    std::string_view canonical_path) {
  const std::string normalized = normalize_source_hashimyei_cursor(canonical_path);
  if (!browser_has_hashimyei_suffix(normalized)) return normalized;
  const std::size_t dot = normalized.rfind('.');
  return dot == std::string::npos ? normalized : normalized.substr(0, dot);
}

[[nodiscard]] std::string browser_family_from_canonical(
    std::string_view canonical_path) {
  const auto segments = split_browser_segments(canonical_path);
  if (segments.size() >= 3 && segments[0] == "tsi" && segments[1] == "wikimyei") {
    return segments[2];
  }
  if (segments.size() >= 2 && segments[0] == "tsi") return segments[1];
  if (!segments.empty()) return segments.front();
  return "unknown";
}

[[nodiscard]] std::string browser_display_name_for_member(
    std::string_view canonical_path, std::string_view hashimyei) {
  const std::string normalized_hash = trim_ascii(hashimyei);
  if (!normalized_hash.empty()) return normalized_hash;
  const auto segments =
      split_browser_segments(browser_project_path_for_canonical(canonical_path));
  if (!segments.empty()) return segments.back();
  return std::string(canonical_path);
}

[[nodiscard]] std::uint64_t browser_member_latest_ts_ms(
    const cuwacunu::hero::hashimyei::component_state_t* component,
    const cuwacunu::hero::wave::runtime_fact_summary_t* fact) {
  std::uint64_t ts_ms = 0;
  if (component) ts_ms = std::max(ts_ms, component->ts_ms);
  if (fact) ts_ms = std::max(ts_ms, fact->latest_ts_ms);
  return ts_ms;
}

[[nodiscard]] std::string browser_contract_hash_from_component(
    const cuwacunu::hero::hashimyei::component_state_t& component) {
  return cuwacunu::hero::hashimyei::contract_hash_from_identity(
      component.manifest.contract_identity);
}

[[nodiscard]] std::string browser_dock_hash_from_component(
    const cuwacunu::hero::hashimyei::component_state_t& component) {
  return trim_ascii(component.manifest.docking_signature_sha256_hex);
}

void append_unique_browser_string(std::string_view value,
                                  std::vector<std::string>* values) {
  if (!values) return;
  const std::string token = trim_ascii(value);
  if (token.empty()) return;
  if (std::find(values->begin(), values->end(), token) != values->end()) return;
  values->push_back(token);
}

struct browser_member_row_t {
  std::string canonical_path{};
  std::string project_path{};
  std::string family{};
  std::string display_name{};
  std::string hashimyei{};
  std::string contract_hash{};
  std::string dock_hash{};
  std::string lineage_state{"unknown"};
  bool has_component{false};
  cuwacunu::hero::hashimyei::component_state_t component{};
  bool has_fact{false};
  cuwacunu::hero::wave::runtime_fact_summary_t fact{};
  std::optional<std::uint64_t> family_rank{};
};

struct browser_project_row_t {
  std::string project_path{};
  std::string family{};
  std::vector<browser_member_row_t> members{};
  std::size_t fragment_count{0};
  std::size_t available_context_count{0};
  std::uint64_t latest_ts_ms{0};
  std::vector<std::string> wave_cursors{};
  std::vector<std::string> binding_ids{};
  std::vector<std::string> semantic_taxa{};
  std::vector<std::string> source_runtime_cursors{};
  std::vector<std::string> report_schemas{};
  std::vector<std::string> contract_hashes{};
};

struct browser_family_row_t {
  std::string family{};
  std::vector<browser_project_row_t> projects{};
  std::size_t member_count{0};
  std::size_t active_member_count{0};
  std::size_t fragment_count{0};
  std::size_t available_context_count{0};
  std::uint64_t latest_ts_ms{0};
  std::vector<std::string> wave_cursors{};
  std::vector<std::string> binding_ids{};
  std::vector<std::string> semantic_taxa{};
  std::vector<std::string> contract_hashes{};
};

[[nodiscard]] bool open_hashimyei_catalog_if_needed(app_context_t* app,
                                                    std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app) {
    if (out_error) *out_error = "app context is null";
    return false;
  }
  if (app->hashimyei_catalog.opened()) return true;
  if (app->hashimyei_catalog_path.empty()) {
    if (out_error) *out_error = "hashimyei catalog path is empty";
    return false;
  }
  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t::options_t opts{};
  opts.catalog_path = app->hashimyei_catalog_path;
  opts.encrypted = false;
  return app->hashimyei_catalog.open(opts, out_error);
}

[[nodiscard]] bool close_hashimyei_catalog_if_open(app_context_t* app,
                                                   std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !app->hashimyei_catalog.opened()) return true;
  return app->hashimyei_catalog.close(out_error);
}

void finalize_browser_member_row(browser_member_row_t* member) {
  if (!member) return;
  if (member->project_path.empty()) {
    member->project_path = browser_project_path_for_canonical(member->canonical_path);
  }
  if (member->family.empty()) {
    member->family = browser_family_from_canonical(member->canonical_path);
  }
  if (member->hashimyei.empty()) {
    std::string hashimyei{};
    if (browser_has_hashimyei_suffix(member->canonical_path, &hashimyei)) {
      member->hashimyei = std::move(hashimyei);
    }
  }
  if (member->display_name.empty()) {
    member->display_name =
        browser_display_name_for_member(member->canonical_path, member->hashimyei);
  }
  if (member->has_component && member->lineage_state == "unknown" &&
      !member->component.manifest.lineage_state.empty()) {
    member->lineage_state = member->component.manifest.lineage_state;
  }
  if (!member->has_component && member->has_fact) {
    member->lineage_state = "fact-only";
  }
}

[[nodiscard]] bool build_browser_tree_rows(
    app_context_t* app, std::vector<browser_family_row_t>* out,
    std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !out) return false;
  out->clear();

  if (!open_hashimyei_catalog_if_needed(app, out_error)) return false;

  std::vector<cuwacunu::hero::hashimyei::component_state_t> components{};
  if (!app->hashimyei_catalog.list_component_heads(0, 0, true, &components,
                                                   out_error)) {
    return false;
  }

  std::vector<cuwacunu::hero::wave::runtime_fact_summary_t> facts{};
  if (!app->catalog.list_runtime_fact_summaries(0, 0, true, &facts, out_error)) {
    return false;
  }

  std::unordered_map<std::string, browser_member_row_t> members_by_canonical{};
  members_by_canonical.reserve(components.size() + facts.size());

  for (const auto& component : components) {
    const std::string canonical_path =
        normalize_source_hashimyei_cursor(component.manifest.canonical_path);
    if (canonical_path.empty()) continue;
    auto& member = members_by_canonical[canonical_path];
    member.canonical_path = canonical_path;
    member.has_component = true;
    member.component = component;
    member.family = component.manifest.family.empty()
                        ? browser_family_from_canonical(canonical_path)
                        : component.manifest.family;
    member.project_path = browser_project_path_for_canonical(canonical_path);
    member.hashimyei = component.manifest.hashimyei_identity.name;
    member.contract_hash = browser_contract_hash_from_component(component);
    member.dock_hash = browser_dock_hash_from_component(component);
    member.family_rank = component.family_rank;
    member.lineage_state = component.manifest.lineage_state.empty()
                               ? std::string("unknown")
                               : component.manifest.lineage_state;
    finalize_browser_member_row(&member);
  }

  for (const auto& fact : facts) {
    if (fact.canonical_path.empty()) continue;
    auto& member = members_by_canonical[fact.canonical_path];
    member.canonical_path = fact.canonical_path;
    member.has_fact = true;
    member.fact = fact;
    finalize_browser_member_row(&member);
  }

  std::map<std::string, std::map<std::string, std::vector<browser_member_row_t>>>
      grouped{};
  for (auto& [canonical_path, member] : members_by_canonical) {
    (void)canonical_path;
    finalize_browser_member_row(&member);
    grouped[member.family][member.project_path].push_back(std::move(member));
  }

  out->reserve(grouped.size());
  for (auto& [family_name, projects_map] : grouped) {
    browser_family_row_t family{};
    family.family = family_name;
    for (auto& [project_path, members] : projects_map) {
      std::sort(members.begin(), members.end(),
                [](const browser_member_row_t& a, const browser_member_row_t& b) {
                  if (a.family_rank.has_value() && b.family_rank.has_value() &&
                      a.family_rank != b.family_rank) {
                    return *a.family_rank < *b.family_rank;
                  }
                  if (a.family_rank.has_value() != b.family_rank.has_value()) {
                    return a.family_rank.has_value();
                  }
                  const std::uint64_t a_ts = browser_member_latest_ts_ms(
                      a.has_component ? &a.component : nullptr,
                      a.has_fact ? &a.fact : nullptr);
                  const std::uint64_t b_ts = browser_member_latest_ts_ms(
                      b.has_component ? &b.component : nullptr,
                      b.has_fact ? &b.fact : nullptr);
                  if (a_ts != b_ts) return a_ts > b_ts;
                  return a.display_name < b.display_name;
                });

      browser_project_row_t project{};
      project.project_path = project_path;
      project.family = family_name;
      project.members = std::move(members);
      for (const auto& member : project.members) {
        const std::uint64_t member_ts = browser_member_latest_ts_ms(
            member.has_component ? &member.component : nullptr,
            member.has_fact ? &member.fact : nullptr);
        project.latest_ts_ms = std::max(project.latest_ts_ms, member_ts);
        family.latest_ts_ms = std::max(family.latest_ts_ms, member_ts);
        ++family.member_count;
        if (member.lineage_state == "active") ++family.active_member_count;
        if (member.has_fact) {
          project.fragment_count += member.fact.fragment_count;
          family.fragment_count += member.fact.fragment_count;
          for (const auto wave_cursor : member.fact.wave_cursors) {
            append_unique_browser_string(
                cuwacunu::hero::wave::lattice_catalog_store_t::
                    format_runtime_wave_cursor(wave_cursor),
                &project.wave_cursors);
            append_unique_browser_string(
                cuwacunu::hero::wave::lattice_catalog_store_t::
                    format_runtime_wave_cursor(wave_cursor),
                &family.wave_cursors);
          }
          for (const auto& value : member.fact.binding_ids) {
            append_unique_browser_string(value, &project.binding_ids);
            append_unique_browser_string(value, &family.binding_ids);
          }
          for (const auto& value : member.fact.semantic_taxa) {
            append_unique_browser_string(value, &project.semantic_taxa);
            append_unique_browser_string(value, &family.semantic_taxa);
          }
          for (const auto& value : member.fact.source_runtime_cursors) {
            append_unique_browser_string(value, &project.source_runtime_cursors);
          }
          for (const auto& value : member.fact.report_schemas) {
            append_unique_browser_string(value, &project.report_schemas);
          }
        }
        if (!member.contract_hash.empty()) {
          append_unique_browser_string(member.contract_hash, &project.contract_hashes);
          append_unique_browser_string(member.contract_hash, &family.contract_hashes);
        }
      }
      project.available_context_count = project.wave_cursors.size();
      family.available_context_count = family.wave_cursors.size();
      std::sort(project.wave_cursors.begin(), project.wave_cursors.end());
      std::sort(project.binding_ids.begin(), project.binding_ids.end());
      std::sort(project.semantic_taxa.begin(), project.semantic_taxa.end());
      std::sort(project.source_runtime_cursors.begin(),
                project.source_runtime_cursors.end());
      std::sort(project.report_schemas.begin(), project.report_schemas.end());
      std::sort(project.contract_hashes.begin(), project.contract_hashes.end());
      family.projects.push_back(std::move(project));
    }

    std::sort(family.projects.begin(), family.projects.end(),
              [](const browser_project_row_t& a, const browser_project_row_t& b) {
                if (a.latest_ts_ms != b.latest_ts_ms) {
                  return a.latest_ts_ms > b.latest_ts_ms;
                }
                return a.project_path < b.project_path;
              });
    std::sort(family.wave_cursors.begin(), family.wave_cursors.end());
    std::sort(family.binding_ids.begin(), family.binding_ids.end());
    std::sort(family.semantic_taxa.begin(), family.semantic_taxa.end());
    std::sort(family.contract_hashes.begin(), family.contract_hashes.end());
    family.available_context_count = family.wave_cursors.size();
    out->push_back(std::move(family));
  }

  std::sort(out->begin(), out->end(),
            [](const browser_family_row_t& a, const browser_family_row_t& b) {
              if (a.latest_ts_ms != b.latest_ts_ms) return a.latest_ts_ms > b.latest_ts_ms;
              return a.family < b.family;
            });
  return true;
}

[[nodiscard]] std::string browser_member_summary_json(
    const browser_member_row_t& member) {
  std::ostringstream out;
  out << "{"
      << "\"canonical_path\":" << json_quote(member.canonical_path)
      << ",\"project_path\":" << json_quote(member.project_path)
      << ",\"family\":" << json_quote(member.family)
      << ",\"display_name\":" << json_quote(member.display_name)
      << ",\"hashimyei\":"
      << (member.hashimyei.empty() ? "null" : json_quote(member.hashimyei))
      << ",\"contract_hash\":"
      << (member.contract_hash.empty() ? "null" : json_quote(member.contract_hash))
      << ",\"dock_hash\":"
      << (member.dock_hash.empty() ? "null" : json_quote(member.dock_hash))
      << ",\"lineage_state\":" << json_quote(member.lineage_state)
      << ",\"has_component\":" << (member.has_component ? "true" : "false")
      << ",\"has_fact\":" << (member.has_fact ? "true" : "false")
      << ",\"family_rank\":";
  if (member.family_rank.has_value()) {
    out << *member.family_rank;
  } else {
    out << "null";
  }
  out << ",\"latest_ts_ms\":"
      << (member.has_fact ? member.fact.latest_ts_ms : 0)
      << ",\"fragment_count\":"
      << (member.has_fact ? member.fact.fragment_count : 0)
      << ",\"available_context_count\":"
      << (member.has_fact ? member.fact.available_context_count : 0)
      << ",\"latest_wave_cursor\":";
  if (member.has_fact && member.fact.latest_wave_cursor != 0) {
    out << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                          format_runtime_wave_cursor(member.fact.latest_wave_cursor));
  } else {
    out << "null";
  }
  out << "}";
  return out.str();
}

[[nodiscard]] std::string browser_project_tree_json(
    const browser_project_row_t& project) {
  std::ostringstream out;
  out << "{"
      << "\"project_path\":" << json_quote(project.project_path)
      << ",\"family\":" << json_quote(project.family)
      << ",\"member_count\":" << project.members.size()
      << ",\"active_member_count\":"
      << std::count_if(project.members.begin(), project.members.end(),
                       [](const browser_member_row_t& member) {
                         return member.lineage_state == "active";
                       })
      << ",\"fragment_count\":" << project.fragment_count
      << ",\"available_context_count\":" << project.available_context_count
      << ",\"latest_ts_ms\":" << project.latest_ts_ms
      << ",\"wave_cursors\":"
      << json_string_array_from_vector(project.wave_cursors)
      << ",\"binding_ids\":"
      << json_string_array_from_vector(project.binding_ids)
      << ",\"semantic_taxa\":"
      << json_string_array_from_vector(project.semantic_taxa)
      << ",\"source_runtime_cursors\":"
      << json_string_array_from_vector(project.source_runtime_cursors)
      << ",\"report_schemas\":"
      << json_string_array_from_vector(project.report_schemas)
      << ",\"contract_hashes\":"
      << json_string_array_from_vector(project.contract_hashes)
      << ",\"members\":[";
  for (std::size_t i = 0; i < project.members.size(); ++i) {
    if (i != 0) out << ",";
    out << browser_member_summary_json(project.members[i]);
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string browser_family_tree_json(
    const browser_family_row_t& family) {
  std::ostringstream out;
  out << "{"
      << "\"family\":" << json_quote(family.family)
      << ",\"member_count\":" << family.member_count
      << ",\"active_member_count\":" << family.active_member_count
      << ",\"fragment_count\":" << family.fragment_count
      << ",\"available_context_count\":" << family.available_context_count
      << ",\"latest_ts_ms\":" << family.latest_ts_ms
      << ",\"wave_cursors\":"
      << json_string_array_from_vector(family.wave_cursors)
      << ",\"binding_ids\":"
      << json_string_array_from_vector(family.binding_ids)
      << ",\"semantic_taxa\":"
      << json_string_array_from_vector(family.semantic_taxa)
      << ",\"contract_hashes\":"
      << json_string_array_from_vector(family.contract_hashes)
      << ",\"projects\":[";
  for (std::size_t i = 0; i < family.projects.size(); ++i) {
    if (i != 0) out << ",";
    out << browser_project_tree_json(family.projects[i]);
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string browser_recent_fragments_json(
    const std::vector<cuwacunu::hero::wave::runtime_report_fragment_t>& fragments) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < fragments.size(); ++i) {
    if (i != 0) out << ",";
    const auto& fragment = fragments[i];
    out << "{"
        << "\"report_fragment_id\":" << json_quote(fragment.report_fragment_id)
        << ",\"schema\":" << json_quote(fragment.schema)
        << ",\"semantic_taxon\":"
        << (fragment.semantic_taxon.empty() ? "null"
                                            : json_quote(fragment.semantic_taxon))
        << ",\"wave_cursor\":";
    if (fragment.wave_cursor != 0) {
      out << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                            format_runtime_wave_cursor(fragment.wave_cursor));
    } else {
      out << "null";
    }
    out << ",\"ts_ms\":" << fragment.ts_ms << "}";
  }
  out << "]";
  return out.str();
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

  std::ostringstream facts_json;
  facts_json << "[";
  std::size_t emitted = 0;
  std::size_t total = 0;

  if (canonical_path.empty()) {
    std::vector<cuwacunu::hero::wave::runtime_fact_summary_t> facts{};
    if (!app->catalog.list_runtime_fact_summaries(0, 0, true, &facts, out_error)) {
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
                 << ",\"latest_wave_cursor\":"
                 << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                                   format_runtime_wave_cursor(
                                       fact.latest_wave_cursor))
                 << ",\"latest_ts_ms\":" << fact.latest_ts_ms
                 << ",\"binding_ids\":"
                 << json_string_array_from_vector(fact.binding_ids)
                 << ",\"semantic_taxa\":"
                 << json_string_array_from_vector(fact.semantic_taxa)
                 << ",\"source_runtime_cursors\":"
                 << json_string_array_from_vector(fact.source_runtime_cursors)
                 << ",\"available_context_count\":" << fact.available_context_count
                 << ",\"fragment_count\":" << fact.fragment_count << "}";
    }
  } else {
    std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> rows{};
    if (!app->catalog.list_runtime_report_fragments(canonical_path, "", 0, 0,
                                                    true, &rows, out_error)) {
      return false;
    }
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
                 << ",\"latest_ts_ms\":" << fact.latest_ts_ms
                 << ",\"binding_ids\":"
                 << json_string_array_from_set(fact.binding_ids)
                 << ",\"semantic_taxa\":"
                 << json_string_array_from_set(fact.semantic_taxa)
                 << ",\"source_runtime_cursors\":"
                 << json_string_array_from_set(fact.source_runtime_cursors)
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
      if (fragment_wave_cursor(row, &selected_wave_cursor)) {
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
    if (!fragment_wave_cursor(row, &row_wave_cursor)) {
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
  std::uint64_t latest_ts_ms = selected.front().ts_ms;
  std::set<std::string> binding_ids{};
  std::set<std::string> semantic_taxa{};
  std::set<std::string> source_runtime_cursors{};
  for (const auto& row : selected) {
    if (row.ts_ms > latest_ts_ms) {
      latest_ts_ms = row.ts_ms;
    }
    if (!row.binding_id.empty()) {
      binding_ids.insert(trim_ascii(row.binding_id));
    }
    if (!row.semantic_taxon.empty()) {
      semantic_taxa.insert(trim_ascii(row.semantic_taxon));
    }
    if (!row.source_runtime_cursor.empty()) {
      source_runtime_cursors.insert(trim_ascii(row.source_runtime_cursor));
    }
  }

  std::ostringstream out;
  out << "{\"canonical_path\":" << json_quote(canonical_path)
      << ",\"selection_mode\":" << json_quote(use_wave_cursor ? "historical" : "latest")
      << ",\"wave_cursor\":"
      << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                        format_runtime_wave_cursor(selected_wave_cursor))
      << ",\"latest_ts_ms\":" << latest_ts_ms
      << ",\"binding_ids\":" << json_string_array_from_set(binding_ids)
      << ",\"semantic_taxa\":" << json_string_array_from_set(semantic_taxa)
      << ",\"source_runtime_cursors\":"
      << json_string_array_from_set(source_runtime_cursors)
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
  const bool has_family_reports = !network_rows.empty();
  out << "{\"count\":2,\"views\":[{"
      << "\"view_kind\":\"entropic_capacity_comparison\""
      << ",\"preferred_selector\":\"wave_cursor\""
      << ",\"required_selectors\":[\"wave_cursor\"]"
      << ",\"optional_selectors\":[\"canonical_path\",\"contract_hash\"]"
      << ",\"ready\":"
      << ((!source_rows.empty() && !network_rows.empty()) ? "true" : "false")
      << "},{"
      << "\"view_kind\":\"family_evaluation_report\""
      << ",\"preferred_selector\":\"canonical_path\""
      << ",\"required_selectors\":[\"canonical_path\",\"dock_hash\"]"
      << ",\"optional_selectors\":[\"wave_cursor\"]"
      << ",\"ready\":" << (has_family_reports ? "true" : "false")
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
  std::string dock_hash{};
  (void)extract_json_string_field(arguments_json, "view_kind", &view_kind);
  (void)extract_json_string_field(arguments_json, "canonical_path",
                                  &canonical_path);
  (void)extract_json_string_field(arguments_json, "contract_hash",
                                  &contract_hash);
  (void)extract_json_string_field(arguments_json, "dock_hash", &dock_hash);
  view_kind = trim_ascii(view_kind);
  canonical_path = normalize_source_hashimyei_cursor(canonical_path);
  contract_hash = trim_ascii(contract_hash);
  dock_hash = trim_ascii(dock_hash);
  if (view_kind.empty()) {
    *out_error = "get_view requires arguments.view_kind";
    return false;
  }

  std::uint64_t wave_cursor = 0;
  const bool use_wave_cursor =
      extract_json_wave_cursor_field(arguments_json, "wave_cursor",
                                    &wave_cursor);
  std::string internal_intersection_cursor{};
  if (!canonical_path.empty() && use_wave_cursor) {
    internal_intersection_cursor =
        canonical_path + "|" +
        cuwacunu::hero::wave::lattice_catalog_store_t::format_runtime_wave_cursor(
            wave_cursor);
  } else if (!canonical_path.empty() &&
             view_kind == "family_evaluation_report") {
    internal_intersection_cursor = canonical_path;
  }

  cuwacunu::hero::wave::runtime_view_report_t view{};
  if (!app->catalog.get_runtime_view_lls(
          view_kind, internal_intersection_cursor, wave_cursor, use_wave_cursor,
          contract_hash, dock_hash, &view, out_error)) {
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
      << ",\"dock_hash\":"
      << (view.dock_hash.empty() ? "null" : json_quote(view.dock_hash))
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

[[nodiscard]] bool handle_tool_browser_tree(app_context_t* app,
                                            const std::string& arguments_json,
                                            std::string* out_structured,
                                            std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();
  (void)arguments_json;

  std::vector<browser_family_row_t> families{};
  if (!build_browser_tree_rows(app, &families, out_error)) return false;

  std::size_t project_count = 0;
  std::size_t member_count = 0;
  for (const auto& family : families) {
    project_count += family.projects.size();
    member_count += family.member_count;
  }

  std::ostringstream out;
  out << "{\"family_count\":" << families.size()
      << ",\"project_count\":" << project_count
      << ",\"member_count\":" << member_count
      << ",\"families\":[";
  for (std::size_t i = 0; i < families.size(); ++i) {
    if (i != 0) out << ",";
    out << browser_family_tree_json(families[i]);
  }
  out << "]}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_browser_detail(app_context_t* app,
                                              const std::string& arguments_json,
                                              std::string* out_structured,
                                              std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string kind{};
  std::string family_name{};
  std::string project_path{};
  std::string canonical_path{};
  (void)extract_json_string_field(arguments_json, "kind", &kind);
  (void)extract_json_string_field(arguments_json, "family", &family_name);
  (void)extract_json_string_field(arguments_json, "project_path", &project_path);
  (void)extract_json_string_field(arguments_json, "canonical_path",
                                  &canonical_path);
  kind = trim_ascii(kind);
  family_name = trim_ascii(family_name);
  project_path = normalize_source_hashimyei_cursor(project_path);
  canonical_path = normalize_source_hashimyei_cursor(canonical_path);
  if (kind.empty()) {
    *out_error = "browser_detail requires arguments.kind";
    return false;
  }

  std::vector<browser_family_row_t> families{};
  if (!build_browser_tree_rows(app, &families, out_error)) return false;

  if (kind == "family") {
    for (const auto& family : families) {
      if (family.family != family_name) continue;
      std::vector<std::string> project_paths{};
      project_paths.reserve(family.projects.size());
      for (const auto& project : family.projects) {
        project_paths.push_back(project.project_path);
      }
      std::ostringstream out;
      out << "{\"kind\":\"family\""
          << ",\"family\":" << json_quote(family.family)
          << ",\"project_count\":" << family.projects.size()
          << ",\"member_count\":" << family.member_count
          << ",\"active_member_count\":" << family.active_member_count
          << ",\"fragment_count\":" << family.fragment_count
          << ",\"available_context_count\":" << family.available_context_count
          << ",\"latest_ts_ms\":" << family.latest_ts_ms
          << ",\"wave_cursors\":"
          << json_string_array_from_vector(family.wave_cursors)
          << ",\"binding_ids\":"
          << json_string_array_from_vector(family.binding_ids)
          << ",\"semantic_taxa\":"
          << json_string_array_from_vector(family.semantic_taxa)
          << ",\"contract_hashes\":"
          << json_string_array_from_vector(family.contract_hashes)
          << ",\"project_paths\":"
          << json_string_array_from_vector(project_paths) << "}";
      *out_structured = out.str();
      return true;
    }
    *out_error = "browser_detail family not found";
    return false;
  }

  if (kind == "project") {
    for (const auto& family : families) {
      for (const auto& project : family.projects) {
        if (project.project_path != project_path) continue;
        std::vector<std::string> member_paths{};
        member_paths.reserve(project.members.size());
        for (const auto& member : project.members) {
          member_paths.push_back(member.canonical_path);
        }
        std::ostringstream out;
        out << "{\"kind\":\"project\""
            << ",\"family\":" << json_quote(project.family)
            << ",\"project_path\":" << json_quote(project.project_path)
            << ",\"member_count\":" << project.members.size()
            << ",\"active_member_count\":"
            << std::count_if(project.members.begin(), project.members.end(),
                             [](const browser_member_row_t& member) {
                               return member.lineage_state == "active";
                             })
            << ",\"fragment_count\":" << project.fragment_count
            << ",\"available_context_count\":" << project.available_context_count
            << ",\"latest_ts_ms\":" << project.latest_ts_ms
            << ",\"wave_cursors\":"
            << json_string_array_from_vector(project.wave_cursors)
            << ",\"binding_ids\":"
            << json_string_array_from_vector(project.binding_ids)
            << ",\"semantic_taxa\":"
            << json_string_array_from_vector(project.semantic_taxa)
            << ",\"source_runtime_cursors\":"
            << json_string_array_from_vector(project.source_runtime_cursors)
            << ",\"report_schemas\":"
            << json_string_array_from_vector(project.report_schemas)
            << ",\"contract_hashes\":"
            << json_string_array_from_vector(project.contract_hashes)
            << ",\"member_paths\":"
            << json_string_array_from_vector(member_paths) << "}";
        *out_structured = out.str();
        return true;
      }
    }
    *out_error = "browser_detail project not found";
    return false;
  }

  if (kind == "member") {
    for (const auto& family : families) {
      for (const auto& project : family.projects) {
        for (const auto& member : project.members) {
          if (member.canonical_path != canonical_path) continue;
          std::ostringstream out;
          out << "{"
              << "\"kind\":\"member\""
              << ",\"family\":" << json_quote(member.family)
              << ",\"project_path\":" << json_quote(member.project_path)
              << ",\"canonical_path\":" << json_quote(member.canonical_path)
              << ",\"display_name\":" << json_quote(member.display_name)
              << ",\"hashimyei\":"
              << (member.hashimyei.empty() ? "null" : json_quote(member.hashimyei))
              << ",\"contract_hash\":"
              << (member.contract_hash.empty() ? "null"
                                              : json_quote(member.contract_hash))
              << ",\"lineage_state\":" << json_quote(member.lineage_state)
              << ",\"family_rank\":";
          if (member.family_rank.has_value()) {
            out << *member.family_rank;
          } else {
            out << "null";
          }
          out << ",\"has_component\":" << (member.has_component ? "true" : "false")
              << ",\"has_fact\":" << (member.has_fact ? "true" : "false")
              << ",\"parent_hashimyei\":";
          if (member.has_component &&
              member.component.manifest.parent_identity.has_value() &&
              !member.component.manifest.parent_identity->name.empty()) {
            out << json_quote(member.component.manifest.parent_identity->name);
          } else {
            out << "null";
          }
          out << ",\"replaced_by\":";
          if (member.has_component &&
              !trim_ascii(member.component.manifest.replaced_by).empty()) {
            out << json_quote(member.component.manifest.replaced_by);
          } else {
            out << "null";
          }
          out << ",\"fragment_count\":"
              << (member.has_fact ? member.fact.fragment_count : 0)
              << ",\"available_context_count\":"
              << (member.has_fact ? member.fact.available_context_count : 0)
              << ",\"latest_ts_ms\":"
              << (member.has_fact ? member.fact.latest_ts_ms : 0)
              << ",\"latest_wave_cursor\":";
          if (member.has_fact && member.fact.latest_wave_cursor != 0) {
            out << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                                  format_runtime_wave_cursor(
                                      member.fact.latest_wave_cursor));
          } else {
            out << "null";
          }
          out << ",\"wave_cursors\":";
          if (member.has_fact) {
            out << json_wave_cursor_array_from_vector(member.fact.wave_cursors);
          } else {
            out << "[]";
          }
          out << ",\"binding_ids\":";
          if (member.has_fact) {
            out << json_string_array_from_vector(member.fact.binding_ids);
          } else {
            out << "[]";
          }
          out << ",\"semantic_taxa\":";
          if (member.has_fact) {
            out << json_string_array_from_vector(member.fact.semantic_taxa);
          } else {
            out << "[]";
          }
          out << ",\"source_runtime_cursors\":";
          if (member.has_fact) {
            out << json_string_array_from_vector(member.fact.source_runtime_cursors);
          } else {
            out << "[]";
          }
          out << ",\"report_schemas\":";
          if (member.has_fact) {
            out << json_string_array_from_vector(member.fact.report_schemas);
          } else {
            out << "[]";
          }
          out << ",\"recent_fragments\":";
          if (member.has_fact) {
            out << browser_recent_fragments_json(member.fact.recent_fragments);
          } else {
            out << "[]";
          }
          out << "}";
          *out_structured = out.str();
          return true;
        }
      }
    }
    *out_error = "browser_detail member not found";
    return false;
  }

  *out_error = "browser_detail kind must be family, project, or member";
  return false;
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
    const bool read_only =
        std::string_view(descriptor.name) != "hero.lattice.refresh";
    out << "{\"name\":" << json_quote(descriptor.name)
        << ",\"description\":" << json_quote(descriptor.description)
        << ",\"inputSchema\":" << descriptor.input_schema_json
        << ",\"annotations\":{\"readOnlyHint\":"
        << (read_only ? "true" : "false")
        << ",\"destructiveHint\":false,\"openWorldHint\":false}}";
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
  std::error_code ec{};
  opts.read_only = std::filesystem::exists(app->lattice_catalog_path, ec) && !ec;
  opts.projection_version = 2;
  opts.runtime_only_indexes = true;
  return app->catalog.open(opts, out_error);
}

[[nodiscard]] bool close_lattice_catalog_if_open(app_context_t* app,
                                                 std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app) return true;
  if (app->hashimyei_catalog.opened()) {
    std::string hashimyei_error{};
    if (!close_hashimyei_catalog_if_open(app, &hashimyei_error)) {
      if (out_error) *out_error = "hashimyei catalog close failed: " + hashimyei_error;
      return false;
    }
  }
  if (!app->catalog.opened()) return true;
  return app->catalog.close(out_error);
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
  if (tool_requires_catalog_sync(tool_name)) {
    std::string sync_error;
    if (!ensure_lattice_catalog_synced_to_store(app, false, &sync_error)) {
      *out_error_json = "catalog sync failed: " + sync_error;
      return false;
    }
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

  bool shutdown_requested = false;
  for (;;) {
    if (!wait_for_stdio_activity()) return;

    std::string message;
    bool used_content_length = false;
    if (!read_next_jsonrpc_message(&std::cin, &message, &used_content_length)) {
      return;
    }
    if (message.empty()) continue;
    if (used_content_length) g_jsonrpc_use_content_length_framing = true;

    std::string method;
    if (!extract_json_string_field(message, "method", &method)) {
      write_jsonrpc_error("null", -32600, "missing method");
      continue;
    }

    if (method == "exit") return;

    std::string id_json = "null";
    const bool has_id = extract_json_id_field(message, &id_json);
    if (method.rfind("notifications/", 0) == 0) {
      continue;
    }
    if (!has_id) {
      write_jsonrpc_error("null", -32700, "invalid JSON-RPC id");
      continue;
    }
    if (method == "shutdown") {
      write_jsonrpc_result(id_json, "{}");
      shutdown_requested = true;
      continue;
    }
    if (shutdown_requested) {
      write_jsonrpc_error(id_json, -32000, "server is shutting down");
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
    if (method == "resources/list") {
      write_jsonrpc_result(id_json, "{\"resources\":[]}");
      continue;
    }
    if (method == "resources/templates/list") {
      write_jsonrpc_result(id_json, "{\"resourceTemplates\":[]}");
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

    const bool catalog_already_open = app->catalog.opened();
    const std::uint64_t tool_started_at_ms = now_ms_utc();
    std::uint64_t catalog_hold_started_at_ms = 0;
    log_lattice_tool_begin(app, name, catalog_already_open);

    const bool needs_preopen = tool_requires_catalog_preopen(name);
    std::string open_error;
    if (needs_preopen && !open_lattice_catalog_if_needed(app, &open_error)) {
      const std::string err = "catalog open failed: " + open_error;
      log_lattice_tool_end(app, name, catalog_already_open, false, false,
                           now_ms_utc() - tool_started_at_ms, 0, err);
      const std::string fallback =
          "{\"canonical_path\":\"\",\"error\":" + json_quote(err) + "}";
      write_jsonrpc_result(id_json, make_tool_result_json(err, fallback, true));
      continue;
    }
    if (!catalog_already_open && needs_preopen) {
      catalog_hold_started_at_ms = now_ms_utc();
    }

    std::string tool_result;
    std::string tool_error;
    const bool ok =
        handle_tool_call(app, name, arguments, &tool_result, &tool_error);
    std::string close_error;
    if (!close_lattice_catalog_if_open(app, &close_error)) {
      const std::string err = "catalog close failed: " + close_error;
      log_lattice_tool_end(
          app, name, catalog_already_open, false, false,
          now_ms_utc() - tool_started_at_ms,
          catalog_hold_started_at_ms == 0 ? 0
                                          : now_ms_utc() -
                                                catalog_hold_started_at_ms,
          err);
      write_jsonrpc_error(id_json, -32603,
                          "catalog close failed: " + close_error);
      continue;
    }
    const std::uint64_t finished_at_ms = now_ms_utc();
    log_lattice_tool_end(
        app, name, catalog_already_open, ok, true,
        finished_at_ms - tool_started_at_ms,
        catalog_hold_started_at_ms == 0 ? 0
                                        : finished_at_ms -
                                              catalog_hold_started_at_ms,
        ok ? std::string_view{} : std::string_view(tool_error));
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
namespace lattice_mcp {

std::filesystem::path resolve_lattice_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  return ::resolve_lattice_hero_dsl_path(global_config_path);
}

bool load_wave_runtime_defaults(const std::filesystem::path& hero_dsl_path,
                                const std::filesystem::path& global_config_path,
                                wave_runtime_defaults_t* out,
                                std::string* error) {
  return ::load_wave_runtime_defaults(hero_dsl_path, global_config_path, out,
                                      error);
}

bool execute_tool_json(const std::string& tool_name, std::string arguments_json,
                       app_context_t* app,
                       std::string* out_tool_result_json,
                       std::string* out_error_message) {
  if (out_error_message) out_error_message->clear();
  if (!app || !out_tool_result_json || !out_error_message) return false;
  const bool catalog_already_open = app->catalog.opened();
  const bool needs_preopen = tool_requires_catalog_preopen(tool_name);
  const bool opened_here = !catalog_already_open && needs_preopen;
  const std::uint64_t tool_started_at_ms = now_ms_utc();
  std::uint64_t catalog_hold_started_at_ms = 0;
  log_lattice_tool_begin(app, tool_name, catalog_already_open);
  if (needs_preopen && !::open_lattice_catalog_if_needed(app, out_error_message)) {
    *out_error_message = "catalog open failed: " + *out_error_message;
    log_lattice_tool_end(app, tool_name, catalog_already_open, false, false,
                         now_ms_utc() - tool_started_at_ms, 0,
                         *out_error_message);
    return false;
  }
  if (opened_here) {
    catalog_hold_started_at_ms = now_ms_utc();
  }
  const bool ok = ::handle_tool_call(app, tool_name, arguments_json,
                                     out_tool_result_json, out_error_message);
  bool catalog_closed = false;
  if (opened_here && app->close_catalog_after_execute) {
    std::string close_error;
    if (!::close_lattice_catalog_if_open(app, &close_error)) {
      if (!out_error_message->empty()) out_error_message->append("; ");
      out_error_message->append("catalog close failed: " + close_error);
      log_lattice_tool_end(
          app, tool_name, catalog_already_open, false, false,
          now_ms_utc() - tool_started_at_ms,
          catalog_hold_started_at_ms == 0 ? 0
                                          : now_ms_utc() -
                                                catalog_hold_started_at_ms,
          *out_error_message);
      return false;
    }
    catalog_closed = true;
  }
  const std::uint64_t finished_at_ms = now_ms_utc();
  log_lattice_tool_end(
      app, tool_name, catalog_already_open, ok, catalog_closed,
      finished_at_ms - tool_started_at_ms,
      catalog_hold_started_at_ms == 0 ? 0
                                      : finished_at_ms -
                                            catalog_hold_started_at_ms,
      out_error_message->empty() ? std::string_view{}
                                 : std::string_view(*out_error_message));
  return ok;
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

void prepare_jsonrpc_stdio_server_output() {
  if (g_protocol_stdout_fd >= 0) return;
  std::fflush(stdout);
  g_protocol_stdout_fd = ::dup(STDOUT_FILENO);
  if (g_protocol_stdout_fd < 0) return;
  (void)::dup2(STDERR_FILENO, STDOUT_FILENO);
}

void write_cli_stdout(std::string_view payload) { ::write_cli_stdout(payload); }

void run_jsonrpc_stdio_loop(app_context_t* app) {
  ::run_jsonrpc_stdio_loop_impl(app);
}

}  // namespace lattice_mcp
}  // namespace hero
}  // namespace cuwacunu
