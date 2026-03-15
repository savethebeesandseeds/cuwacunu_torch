#include "hero/lattice_hero/lattice_catalog.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>
#include <thread>

#include <openssl/evp.h>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "hero/hero_catalog_schema.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"

namespace cuwacunu {
namespace hero {
namespace wave {
namespace {

using runtime_lls_document_t =
    cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t;

constexpr std::string_view kEntropicCapacityComparisonViewKind =
    "entropic_capacity_comparison";
constexpr std::string_view kEntropicCapacityComparisonViewCanonicalPath =
    "tsi.analysis.entropic_capacity_comparison";
constexpr std::string_view kEntropicCapacityComparisonViewTransportSchema =
    "lattice.derived_view.entropic_capacity_comparison.v1";
constexpr std::string_view kSourceDataAnalyticsSchema =
    "piaabo.torch_compat.data_analytics.v1";
constexpr std::string_view kNetworkAnalyticsSchema =
    "piaabo.torch_compat.network_analytics.v4";
constexpr double kEntropicCapacityComparisonNumericEpsilon = 1e-18;

[[nodiscard]] bool parse_kv_payload(
    std::string_view payload,
    std::unordered_map<std::string, std::string>* out);

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

[[nodiscard]] bool parse_u64(std::string_view text, std::uint64_t* out) {
  if (!out) return false;
  const std::string t = trim_ascii(text);
  if (t.empty()) return false;
  const int base =
      (t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) ? 16 : 10;
  errno = 0;
  char* end = nullptr;
  const unsigned long long parsed = std::strtoull(t.c_str(), &end, base);
  if (errno != 0 || end == nullptr || *end != '\0') return false;
  *out = static_cast<std::uint64_t>(parsed);
  return true;
}

[[nodiscard]] bool parse_double(std::string_view text, double* out) {
  if (!out) return false;
  const std::string t = trim_ascii(text);
  if (t.empty()) return false;
  std::size_t pos = 0;
  double v = 0.0;
  try {
    v = std::stod(t, &pos);
  } catch (...) {
    return false;
  }
  if (pos != t.size()) return false;
  if (!std::isfinite(v)) return false;
  *out = v;
  return true;
}

void append_runtime_string_entry_(runtime_lls_document_t* document,
                                  std::string_view key,
                                  std::string_view value) {
  if (!document) return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          std::string(key), std::string(value)));
}

void append_runtime_string_entry_if_nonempty_(runtime_lls_document_t* document,
                                              std::string_view key,
                                              std::string_view value) {
  if (!document || value.empty()) return;
  append_runtime_string_entry_(document, key, value);
}

void append_runtime_int_entry_(runtime_lls_document_t* document,
                               std::string_view key,
                               std::int64_t value) {
  if (!document) return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_int_entry(
          std::string(key), value));
}

void append_runtime_u64_entry_(runtime_lls_document_t* document,
                               std::string_view key,
                               std::uint64_t value) {
  if (!document) return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
          std::string(key), value));
}

void append_runtime_double_entry_(runtime_lls_document_t* document,
                                  std::string_view key,
                                  double value) {
  if (!document) return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          std::string(key), value));
}

[[nodiscard]] cuwacunu::piaabo::latent_lineage_state::runtime_lls_entry_t*
find_runtime_entry_(runtime_lls_document_t* document, std::string_view key) {
  if (!document) return nullptr;
  for (auto& entry : document->entries) {
    if (entry.key == key) return &entry;
  }
  return nullptr;
}

void upsert_runtime_string_entry_(runtime_lls_document_t* document,
                                  std::string_view key,
                                  std::string_view value) {
  if (!document) return;
  auto replacement =
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          std::string(key), std::string(value));
  if (auto* existing = find_runtime_entry_(document, key)) {
    *existing = std::move(replacement);
    return;
  }
  document->entries.push_back(std::move(replacement));
}

void upsert_runtime_u64_entry_(runtime_lls_document_t* document,
                               std::string_view key,
                               std::uint64_t value) {
  if (!document) return;
  auto replacement =
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
          std::string(key), value);
  if (auto* existing = find_runtime_entry_(document, key)) {
    *existing = std::move(replacement);
    return;
  }
  document->entries.push_back(std::move(replacement));
}

[[nodiscard]] bool parse_runtime_lls_payload(
    std::string_view payload,
    std::unordered_map<std::string, std::string>* out,
    runtime_lls_document_t* out_document,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "runtime .lls kv output pointer is null";
    return false;
  }
  out->clear();
  if (out_document) out_document->entries.clear();

  runtime_lls_document_t parsed{};
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          payload, &parsed, error)) {
    return false;
  }
  if (!cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          parsed, out, error)) {
    return false;
  }
  if (const auto it = out->find("schema"); it == out->end() || it->second.empty()) {
    if (error) *error = "persisted runtime .lls requires top-level schema key";
    return false;
  }
  if (out_document) *out_document = std::move(parsed);
  return true;
}

[[nodiscard]] bool extract_joined_source_runtime_projection_document(
    std::string_view joined_report_lls,
    std::string_view fallback_run_id,
    runtime_lls_document_t* out_document,
    std::string* out_schema,
    std::string* out_run_id,
    std::string* error) {
  // DEV_WARNING: this is the remaining bridge from the synthetic joined
  // report_lls transport back into strict runtime .lls. The goal is to delete
  // this path once source-runtime projection is preserved as a first-class
  // runtime fragment instead of being reconstructed from joined text.
  if (error) error->clear();
  if (!out_document) {
    if (error) *error = "source runtime projection document output pointer is null";
    return false;
  }
  out_document->entries.clear();
  if (out_schema) out_schema->clear();
  if (out_run_id) out_run_id->clear();

  std::string schema_line{};
  std::vector<std::string> normalized_lines{};
  std::string explicit_run_id{};
  bool saw_embedded_marker = false;
  bool in_embedded_section = false;
  std::size_t cursor = 0;
  while (cursor < joined_report_lls.size()) {
    std::size_t line_end = joined_report_lls.find('\n', cursor);
    if (line_end == std::string_view::npos) line_end = joined_report_lls.size();
    std::string_view line = joined_report_lls.substr(cursor, line_end - cursor);
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    const std::string trimmed = trim_ascii(line);
    if (trimmed == "/* embedded source.runtime.projection.v2 */") {
      saw_embedded_marker = true;
      in_embedded_section = true;
      if (line_end == joined_report_lls.size()) break;
      cursor = line_end + 1;
      continue;
    }
    if (in_embedded_section && trimmed.rfind("/*", 0) == 0) {
      in_embedded_section = false;
      if (line_end == joined_report_lls.size()) break;
      cursor = line_end + 1;
      continue;
    }
    if (!trimmed.empty() && trimmed.front() != '#') {
      const std::size_t eq = trimmed.find('=');
      if (eq != std::string::npos && eq > 0) {
        const std::string lhs = trim_ascii(trimmed.substr(0, eq));
        const std::string rhs = trim_ascii(trimmed.substr(eq + 1));
        const std::string key =
            cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(lhs);
        if (in_embedded_section &&
            !(key.size() >= 15 &&
              key.compare(0, 15, "source.runtime.") == 0)) {
          in_embedded_section = false;
          if (line_end == joined_report_lls.size()) break;
          cursor = line_end + 1;
          continue;
        }
        if (key.size() >= 15 && key.compare(0, 15, "source.runtime.") == 0) {
          if (key == "source.runtime.projection.run_id") {
            if (explicit_run_id.empty()) explicit_run_id = rhs;
          } else {
            if (key == "source.runtime.projection.schema") {
              schema_line = trimmed;
            } else {
              normalized_lines.push_back(trimmed);
            }
          }
        }
      }
    }
    if (line_end == joined_report_lls.size()) break;
    cursor = line_end + 1;
  }

  if (schema_line.empty()) {
    if (error) {
      *error = saw_embedded_marker
                   ? "joined report_lls embedded source.runtime.projection.v2 "
                     "section is missing source.runtime.projection.schema"
                   : "joined report_lls missing source.runtime.projection.schema";
    }
    return false;
  }

  std::ostringstream payload;
  payload << schema_line << "\n";
  for (const auto& line : normalized_lines) payload << line << "\n";

  runtime_lls_document_t parsed{};
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          payload.str(), &parsed, error)) {
    return false;
  }
  std::unordered_map<std::string, std::string> kv{};
  if (!cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          parsed, &kv, error)) {
    return false;
  }

  const auto it_schema = kv.find("source.runtime.projection.schema");
  if (it_schema == kv.end() || trim_ascii(it_schema->second).empty()) {
    if (error) *error = "source runtime projection document is missing schema";
    return false;
  }

  *out_document = std::move(parsed);
  if (out_schema) *out_schema = trim_ascii(it_schema->second);
  if (out_run_id) {
    *out_run_id = trim_ascii(explicit_run_id);
    if (out_run_id->empty()) *out_run_id = trim_ascii(fallback_run_id);
  }
  return true;
}

[[nodiscard]] std::string build_projection_lls_payload(
    const wave_projection_t& projection) {
  std::vector<std::pair<std::string, double>> projection_num =
      projection.projection_num;
  std::vector<std::pair<std::string, std::string>> projection_txt =
      projection.projection_txt;

  std::sort(projection_num.begin(), projection_num.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
  std::sort(projection_txt.begin(), projection_txt.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  runtime_lls_document_t payload{};
  append_runtime_string_entry_(&payload, "schema", "wave.projection.lls.v2");
  append_runtime_int_entry_(
      &payload, "projection_version", static_cast<std::int64_t>(projection.projection_version));
  append_runtime_string_entry_(
      &payload, "projector_build_id", projection.projector_build_id);
  for (const auto& [k, v] : projection_num) {
    append_runtime_double_entry_(&payload, k, v);
  }
  for (const auto& [k, v] : projection_txt) {
    append_runtime_string_entry_(&payload, k, v);
  }
  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      payload);
}

[[nodiscard]] std::uint64_t fnv1a64(std::string_view text) {
  std::uint64_t h = 1469598103934665603ULL;
  for (const unsigned char c : text) {
    h ^= static_cast<std::uint64_t>(c);
    h *= 1099511628211ULL;
  }
  return h;
}

[[nodiscard]] bool parse_trailing_u64_suffix(std::string_view text,
                                             std::uint64_t* out) {
  if (!out) return false;
  const std::string t = trim_ascii(text);
  if (t.empty()) return false;
  std::size_t begin = t.size();
  while (begin > 0 &&
         std::isdigit(static_cast<unsigned char>(t[begin - 1])) != 0) {
    --begin;
  }
  if (begin == t.size()) return false;
  return parse_u64(std::string_view(t).substr(begin), out);
}

[[nodiscard]] std::uint64_t normalize_to_bits(std::uint64_t v,
                                              std::uint8_t bits) {
  const std::uint64_t m = db::wave_cursor::bitmask(bits);
  return v & m;
}

[[nodiscard]] std::uint64_t runtime_run_numeric_id(
    std::string_view run_id,
    const std::unordered_map<std::string, cuwacunu::hero::hashimyei::run_manifest_t>&
        runtime_runs_by_id) {
  if (const auto it = runtime_runs_by_id.find(std::string(run_id));
      it != runtime_runs_by_id.end() && it->second.started_at_ms > 0) {
    return it->second.started_at_ms;
  }
  std::uint64_t suffix = 0;
  if (parse_trailing_u64_suffix(run_id, &suffix)) return suffix;
  return fnv1a64(run_id);
}

[[nodiscard]] std::uint64_t runtime_kv_u64_or_default(
    const std::unordered_map<std::string, std::string>& kv,
    std::initializer_list<std::string_view> keys, std::uint64_t fallback) {
  for (const auto key : keys) {
    const auto it = kv.find(std::string(key));
    if (it == kv.end()) continue;
    std::uint64_t parsed = 0;
    if (parse_u64(it->second, &parsed)) return parsed;
  }
  return fallback;
}

enum class runtime_wave_cursor_resolution_e : std::uint8_t {
  Run = 0,
  Episode = 1,
  Batch = 2,
};

[[nodiscard]] std::string lower_ascii_copy(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] std::string runtime_wave_cursor_resolution_token(
    runtime_wave_cursor_resolution_e resolution) {
  switch (resolution) {
    case runtime_wave_cursor_resolution_e::Run:
      return "run";
    case runtime_wave_cursor_resolution_e::Episode:
      return "episode";
    case runtime_wave_cursor_resolution_e::Batch:
      return "batch";
  }
  return "run";
}

[[nodiscard]] bool parse_runtime_wave_cursor_resolution(
    std::string_view text, runtime_wave_cursor_resolution_e* out) {
  if (!out) return false;
  const std::string token = lower_ascii_copy(trim_ascii(text));
  if (token == "run") {
    *out = runtime_wave_cursor_resolution_e::Run;
    return true;
  }
  if (token == "episode") {
    *out = runtime_wave_cursor_resolution_e::Episode;
    return true;
  }
  if (token == "batch") {
    *out = runtime_wave_cursor_resolution_e::Batch;
    return true;
  }
  return false;
}

[[nodiscard]] runtime_wave_cursor_resolution_e infer_runtime_wave_cursor_resolution(
    const std::unordered_map<std::string, std::string>& kv) {
  if (const auto it = kv.find("wave_cursor_resolution"); it != kv.end()) {
    runtime_wave_cursor_resolution_e parsed{};
    if (parse_runtime_wave_cursor_resolution(it->second, &parsed)) return parsed;
  }
  bool has_episode = false;
  bool has_batch = false;
  for (const auto& [k, _] : kv) {
    if (k == "wave.cursor.episode" || k == "episode_k" || k == "episode" ||
        k == "wave_episode" || k == "epoch_k") {
      has_episode = true;
      continue;
    }
    if (k == "wave.cursor.batch" || k == "batch_j" || k == "batch" ||
        k == "wave_batch") {
      has_batch = true;
    }
  }
  if (has_batch) return runtime_wave_cursor_resolution_e::Batch;
  if (has_episode) return runtime_wave_cursor_resolution_e::Episode;
  return runtime_wave_cursor_resolution_e::Run;
}

[[nodiscard]] std::uint64_t runtime_wave_cursor_mask_for_resolution(
    runtime_wave_cursor_resolution_e resolution) {
  const auto layout = lattice_catalog_store_t::runtime_wave_cursor_layout();
  db::wave_cursor::masked_query_t q{};
  const db::wave_cursor::parts_t zero{};
  std::uint8_t field_mask = db::wave_cursor::field_run;
  if (resolution == runtime_wave_cursor_resolution_e::Episode ||
      resolution == runtime_wave_cursor_resolution_e::Batch) {
    field_mask = static_cast<std::uint8_t>(field_mask |
                                           db::wave_cursor::field_episode);
  }
  if (resolution == runtime_wave_cursor_resolution_e::Batch) {
    field_mask =
        static_cast<std::uint8_t>(field_mask | db::wave_cursor::field_batch);
  }
  (void)db::wave_cursor::build_masked_query(layout, zero, field_mask, &q);
  return q.mask;
}

[[nodiscard]] bool build_runtime_wave_cursor(
    std::string_view run_id,
    const std::unordered_map<std::string, std::string>& kv,
    const std::unordered_map<std::string, cuwacunu::hero::hashimyei::run_manifest_t>&
        runtime_runs_by_id,
    std::uint64_t* out_wave_cursor) {
  if (!out_wave_cursor) return false;
  const auto layout = lattice_catalog_store_t::runtime_wave_cursor_layout();
  if (!db::wave_cursor::valid_layout(layout)) return false;

  const std::uint64_t run_id_numeric =
      runtime_run_numeric_id(run_id, runtime_runs_by_id);
  const std::uint64_t episode_k = runtime_kv_u64_or_default(
      kv, {"wave.cursor.episode", "episode_k", "episode", "wave_episode"}, 0ULL);
  const std::uint64_t batch_j = runtime_kv_u64_or_default(
      kv, {"wave.cursor.batch", "batch_j", "batch", "wave_batch"}, 0ULL);

  db::wave_cursor::parts_t parts{};
  parts.run_id = normalize_to_bits(run_id_numeric, layout.run_bits);
  parts.episode_k = normalize_to_bits(episode_k, layout.episode_bits);
  parts.batch_j = normalize_to_bits(batch_j, layout.batch_bits);
  return db::wave_cursor::pack(layout, parts, out_wave_cursor);
}

[[nodiscard]] bool wave_cursor_matches(std::uint64_t value,
                                       std::uint64_t expected,
                                       std::uint64_t mask,
                                       runtime_wave_cursor_resolution_e
                                           value_resolution) {
  const std::uint64_t comparable_mask =
      mask & runtime_wave_cursor_mask_for_resolution(value_resolution);
  if (mask != 0 && comparable_mask == 0) return false;
  db::wave_cursor::masked_query_t q{};
  q.value = expected & comparable_mask;
  q.mask = comparable_mask;
  return db::wave_cursor::match_masked(value, q);
}

[[nodiscard]] std::optional<std::filesystem::path> canonicalized(
    const std::filesystem::path& p) {
  std::error_code ec;
  const auto c = std::filesystem::weakly_canonical(p, ec);
  if (ec) return std::nullopt;
  return c;
}

[[nodiscard]] bool path_is_within(std::filesystem::path base,
                                  std::filesystem::path candidate) {
  if (const auto c = canonicalized(base); c.has_value()) {
    base = *c;
  } else {
    base = base.lexically_normal();
  }
  if (const auto c = canonicalized(candidate); c.has_value()) {
    candidate = *c;
  } else {
    candidate = candidate.lexically_normal();
  }
  auto b = base.begin();
  auto c = candidate.begin();
  for (; b != base.end() && c != candidate.end(); ++b, ++c) {
    if (*b != *c) return false;
  }
  return b == base.end();
}

[[nodiscard]] bool read_text_file(const std::filesystem::path& path, std::string* out,
                                  std::string* error = nullptr) {
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

[[nodiscard]] bool sha256_hex_file(const std::filesystem::path& path, std::string* out,
                                   std::string* error = nullptr) {
  if (error) error->clear();
  std::string payload{};
  if (!read_text_file(path, &payload, error)) return false;
  return sha256_hex_bytes(payload, out, error);
}

[[nodiscard]] std::string join_key(std::string_view canonical_path,
                                   std::string_view schema) {
  return std::string(canonical_path) + "|" + std::string(schema);
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

[[nodiscard]] std::string canonical_path_from_report_fragment_path(
    const std::filesystem::path& p) {
  const std::string s = p.generic_string();
  const auto find_after = [&](std::string_view needle) -> std::string {
    const std::size_t pos = s.find(needle);
    if (pos == std::string::npos) return {};
    const std::size_t begin = pos + needle.size();
    if (begin >= s.size()) return {};
    std::size_t end = s.find('/', begin);
    if (end == std::string::npos) end = s.size();
    return s.substr(begin, end - begin);
  };

  const std::string vicreg_hash = find_after("/tsi.wikimyei/representation/vicreg/");
  if (!vicreg_hash.empty()) {
    return "tsi.wikimyei.representation.vicreg." + vicreg_hash;
  }

  const std::string source_head = "/tsi.source/data_analytics/";
  const std::size_t p0 = s.find(source_head);
  if (p0 != std::string::npos) {
    const std::size_t b0 = p0 + source_head.size();
    const std::size_t slash1 = s.find('/', b0);
    if (slash1 != std::string::npos) {
      const std::size_t b1 = slash1 + 1;
      const std::size_t slash2 = s.find('/', b1);
      if (slash2 != std::string::npos && slash2 > b1) {
        return s.substr(b1, slash2 - b1);
      }
    }
  }
  return {};
}

[[nodiscard]] std::string contract_hash_from_report_fragment_path(
    const std::filesystem::path& p) {
  const std::string s = p.generic_string();
  const std::string source_head = "/tsi.source/data_analytics/";
  const std::size_t p0 = s.find(source_head);
  if (p0 == std::string::npos) return {};
  const std::size_t b0 = p0 + source_head.size();
  const std::size_t slash1 = s.find('/', b0);
  if (slash1 == std::string::npos || slash1 <= b0) return {};
  return s.substr(b0, slash1 - b0);
}

[[nodiscard]] bool starts_with(std::string_view text,
                               std::string_view prefix) {
  return text.size() >= prefix.size() &&
         text.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] bool is_hashimyei_hex_token(std::string_view token) {
  if (token.size() < 3) return false;
  if (token[0] != '0' || token[1] != 'x') return false;
  for (std::size_t i = 2; i < token.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(token[i]);
    const bool hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    if (!hex) return false;
  }
  return true;
}

[[nodiscard]] std::string normalize_source_hashimyei_cursor(
    std::string_view canonical_path) {
  return lattice_catalog_store_t::normalize_runtime_hashimyei_cursor(
      canonical_path);
}

[[nodiscard]] bool runtime_hashimyei_cursor_matches(
    std::string_view query_canonical_path,
    std::string_view fragment_canonical_path) {
  return lattice_catalog_store_t::runtime_hashimyei_cursor_matches(
      query_canonical_path, fragment_canonical_path);
}

[[nodiscard]] std::string build_intersection_cursor(
    std::string_view hashimyei_cursor,
    std::uint64_t wave_cursor) {
  std::ostringstream out;
  out << trim_ascii(hashimyei_cursor) << "|" << wave_cursor;
  return out.str();
}

void append_synthetic_source_runtime_report_fragments(
    const std::unordered_map<std::string, wave_cell_t>& cells_by_id,
    const std::unordered_map<std::string, cuwacunu::hero::hashimyei::run_manifest_t>&
        runtime_runs_by_id,
    std::string_view canonical_path, std::string_view schema,
    std::vector<runtime_report_fragment_t>* out) {
  if (!out) return;
  const std::string cp = normalize_source_hashimyei_cursor(canonical_path);
  const std::string sc(schema);
  if (cp.empty() || !starts_with(cp, "tsi.source.")) return;
  if (!sc.empty() && sc != "wave.source.runtime.projection.v2") return;

  for (const auto& [_, cell] : cells_by_id) {
    // DEV_WARNING: synthetic source-runtime fragments are still reconstructed
    // from joined transport text, not from a persisted original fragment body.
    // This is the last major Lattice/runtime-LLS compatibility gap.
    std::unordered_map<std::string, std::string> joined_kv{};
    (void)parse_kv_payload(cell.report.report_lls, &joined_kv);
    std::string fallback_run_id{};
    if (const auto it = joined_kv.find("run_id"); it != joined_kv.end()) {
      fallback_run_id = trim_ascii(it->second);
    }

    runtime_lls_document_t source_payload{};
    std::string synthetic_schema{};
    std::string synthetic_run_id{};
    std::string projection_error{};
    if (!extract_joined_source_runtime_projection_document(
            cell.report.report_lls, fallback_run_id, &source_payload,
            &synthetic_schema, &synthetic_run_id, &projection_error)) {
      continue;
    }
    if (synthetic_schema.empty()) continue;
    if (!sc.empty() && synthetic_schema != sc) continue;
    if (synthetic_run_id.empty()) continue;

    std::unordered_map<std::string, std::string> runtime_kv{};
    runtime_kv["run_id"] = synthetic_run_id;
    std::uint64_t synthetic_wave_cursor = 0;
    if (!build_runtime_wave_cursor(synthetic_run_id, runtime_kv,
                                   runtime_runs_by_id,
                                   &synthetic_wave_cursor)) {
      continue;
    }

    runtime_lls_document_t payload = std::move(source_payload);
    upsert_runtime_string_entry_(
        &payload, "source.runtime.projection.run_id", synthetic_run_id);
    upsert_runtime_string_entry_(&payload, "run_id", synthetic_run_id);
    upsert_runtime_string_entry_(&payload, "canonical_path", cp);
    upsert_runtime_string_entry_(&payload, "source_label", cp);
    upsert_runtime_u64_entry_(&payload, "wave_cursor", synthetic_wave_cursor);
    upsert_runtime_string_entry_(&payload, "wave_cursor_resolution", "run");
    upsert_runtime_string_entry_(&payload, "hashimyei_cursor", cp);
    upsert_runtime_string_entry_(
        &payload, "intersection_cursor",
        build_intersection_cursor(cp, synthetic_wave_cursor));
    const std::string synthetic_payload =
        cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
            payload);

    std::string synthetic_report_fragment_id{};
    const std::string seed = "source-runtime-synth|" + cell.cell_id + "|" +
                             synthetic_run_id + "|" + synthetic_payload;
    if (!sha256_hex_bytes(seed, &synthetic_report_fragment_id, nullptr)) {
      continue;
    }

    runtime_report_fragment_t synthetic{};
    synthetic.report_fragment_id = synthetic_report_fragment_id;
    synthetic.run_id = synthetic_run_id;
    synthetic.canonical_path = cp;
    synthetic.hashimyei = maybe_hashimyei_from_canonical(cp);
    synthetic.schema = synthetic_schema;
    synthetic.report_fragment_sha256 = synthetic_report_fragment_id;
    synthetic.path =
        "[lattice.synthetic.source_runtime cell=" + cell.cell_id + "]";
    synthetic.ts_ms = cell.updated_at_ms;
    synthetic.payload_json = synthetic_payload;
    out->push_back(std::move(synthetic));
  }
}

[[nodiscard]] bool is_known_runtime_schema(std::string_view schema) {
  if (schema == "wave.source.runtime.projection.v2") return true;
  if (schema == "piaabo.torch_compat.data_analytics.v1") return true;
  if (schema == "piaabo.torch_compat.data_analytics_symbolic.v1") return true;
  if (schema == "piaabo.torch_compat.embedding_sequence_analytics.v1")
    return true;
  if (schema ==
      "piaabo.torch_compat.embedding_sequence_analytics_symbolic.v1") {
    return true;
  }
  if (schema == "tsi.wikimyei.representation.vicreg.status.v1") return true;
  if (schema ==
      "tsi.wikimyei.representation.vicreg.transfer_matrix_evaluation.run.v1") {
    return true;
  }
  if (schema == "piaabo.torch_compat.network_analytics.v4") {
    return true;
  }
  return false;
}

struct entropic_capacity_view_candidate_t {
  const runtime_report_fragment_t* fragment{nullptr};
  std::uint64_t wave_cursor{0};
  std::string contract_hash{};
};

struct entropic_capacity_view_summary_t {
  double source_entropic_load{0.0};
  double network_entropic_capacity{0.0};
  double capacity_margin{0.0};
  double capacity_ratio{0.0};
  std::string capacity_regime{"matched"};
};

[[nodiscard]] std::string classify_entropic_capacity_regime(double ratio) {
  if (!std::isfinite(ratio)) return "matched";
  if (ratio < 0.8) return "under_capacity";
  if (ratio > 1.5) return "surplus_capacity";
  return "matched";
}

[[nodiscard]] bool summarize_entropic_capacity_view_from_payloads(
    std::string_view source_payload, std::string_view network_payload,
    entropic_capacity_view_summary_t* out, std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "entropic comparison output pointer is null";
    return false;
  }
  *out = entropic_capacity_view_summary_t{};

  std::unordered_map<std::string, std::string> source_kv{};
  std::unordered_map<std::string, std::string> network_kv{};
  if (!parse_kv_payload(source_payload, &source_kv)) {
    if (error) *error = "invalid source analytics payload";
    return false;
  }
  if (!parse_kv_payload(network_payload, &network_kv)) {
    if (error) *error = "invalid network analytics payload";
    return false;
  }

  const auto it_source = source_kv.find("source_entropic_load");
  if (it_source == source_kv.end() ||
      !parse_double(it_source->second, &out->source_entropic_load)) {
    if (error) {
      *error = "source analytics key missing or invalid: source_entropic_load";
    }
    return false;
  }
  const auto it_network = network_kv.find("network_global_entropic_capacity");
  if (it_network == network_kv.end() ||
      !parse_double(it_network->second, &out->network_entropic_capacity)) {
    if (error) {
      *error =
          "network analytics key missing or invalid: network_global_entropic_capacity";
    }
    return false;
  }

  out->capacity_margin =
      out->network_entropic_capacity - out->source_entropic_load;
  out->capacity_ratio = out->network_entropic_capacity /
                        (out->source_entropic_load +
                         kEntropicCapacityComparisonNumericEpsilon);
  out->capacity_regime = classify_entropic_capacity_regime(out->capacity_ratio);
  return true;
}

[[nodiscard]] bool build_entropic_capacity_view_candidate(
    const runtime_report_fragment_t& fragment,
    entropic_capacity_view_candidate_t* out) {
  if (!out) return false;
  std::unordered_map<std::string, std::string> kv{};
  if (!parse_kv_payload(fragment.payload_json, &kv)) return false;

  std::uint64_t wave_cursor = 0;
  const auto it_wave = kv.find("wave_cursor");
  if (it_wave == kv.end() || !parse_u64(it_wave->second, &wave_cursor)) {
    return false;
  }

  std::string contract_hash{};
  if (const auto it = kv.find("contract_hash");
      it != kv.end() && !trim_ascii(it->second).empty()) {
    contract_hash = trim_ascii(it->second);
  } else {
    contract_hash = contract_hash_from_report_fragment_path(fragment.path);
  }

  out->fragment = &fragment;
  out->wave_cursor = wave_cursor;
  out->contract_hash = std::move(contract_hash);
  return true;
}

void append_view_line(std::ostringstream* out, std::string_view key,
                      std::string_view value) {
  if (!out) return;
  *out << key << "=" << value << "\n";
}

void append_view_u64_line(std::ostringstream* out, std::string_view key,
                          std::uint64_t value) {
  if (!out) return;
  *out << key << "=" << value << "\n";
}

void append_view_size_line(std::ostringstream* out, std::string_view key,
                           std::size_t value) {
  if (!out) return;
  *out << key << "=" << value << "\n";
}

void append_view_double_line(std::ostringstream* out, std::string_view key,
                             double value) {
  if (!out) return;
  *out << key << "=" << std::fixed << std::setprecision(12) << value << "\n";
}

[[nodiscard]] std::uint64_t now_ms_utc() {
  const auto now = std::chrono::system_clock::now();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
          .count());
}

void clear_error(std::string* error) {
  if (error) error->clear();
}

void set_error(std::string* error, std::string_view msg) {
  if (error) *error = std::string(msg);
}

[[nodiscard]] const char* idydb_rc_label(int rc) {
  switch (rc) {
    case IDYDB_SUCCESS:
      return "success";
    case IDYDB_ERROR:
      return "error";
    case IDYDB_PERM:
      return "permission_denied";
    case IDYDB_BUSY:
      return "busy";
    case IDYDB_NOT_FOUND:
      return "not_found";
    case IDYDB_CORRUPT:
      return "corrupt";
    case IDYDB_RANGE:
      return "range";
    case IDYDB_DONE:
      return "done";
    default:
      return "unknown";
  }
}

constexpr int kCatalogOpenBusyRetryCount = 40;
constexpr auto kCatalogOpenBusyRetryDelay = std::chrono::milliseconds(25);

[[nodiscard]] bool insert_text(idydb** db, idydb_column_row_sizing col,
                               idydb_column_row_sizing row, std::string_view value,
                               std::string* error) {
  if (value.empty()) return true;
  const std::string v(value);
  const int rc = idydb_insert_const_char(db, col, row, v.c_str());
  if (rc == IDYDB_DONE) return true;
  if (error) {
    const char* msg = idydb_errmsg(db);
    *error = "idydb_insert_const_char failed: " +
             std::string(msg ? msg : "<no error message>");
  }
  return false;
}

[[nodiscard]] bool insert_num(idydb** db, idydb_column_row_sizing col,
                              idydb_column_row_sizing row, double value,
                              std::string* error) {
  if (!std::isfinite(value)) return true;
  const int rc = idydb_insert_float(db, col, row, static_cast<float>(value));
  if (rc == IDYDB_DONE) return true;
  if (error) {
    const char* msg = idydb_errmsg(db);
    *error = "idydb_insert_float failed: " +
             std::string(msg ? msg : "<no error message>");
  }
  return false;
}

[[nodiscard]] std::string as_text_or_empty(idydb** db,
                                           idydb_column_row_sizing col,
                                           idydb_column_row_sizing row) {
  if (idydb_extract(db, col, row) != IDYDB_DONE) return {};
  if (idydb_retrieved_type(db) != IDYDB_CHAR) return {};
  const char* s = idydb_retrieve_char(db);
  return s ? std::string(s) : std::string{};
}

[[nodiscard]] std::optional<double> as_num_or_empty(idydb** db,
                                                     idydb_column_row_sizing col,
                                                     idydb_column_row_sizing row) {
  if (idydb_extract(db, col, row) != IDYDB_DONE) return std::nullopt;
  if (idydb_retrieved_type(db) != IDYDB_FLOAT) return std::nullopt;
  return static_cast<double>(idydb_retrieve_float(db));
}

[[nodiscard]] std::string to_hex(std::string_view bytes) {
  static constexpr std::array<char, 16> kHex{
      '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  std::string out;
  out.resize(bytes.size() * 2);
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    const unsigned char b = static_cast<unsigned char>(bytes[i]);
    out[2 * i + 0] = kHex[(b >> 4) & 0x0F];
    out[2 * i + 1] = kHex[b & 0x0F];
  }
  return out;
}

[[nodiscard]] bool from_hex(std::string_view hex, std::string* out,
                            std::string* error) {
  if (!out) {
    set_error(error, "hex decode output pointer is null");
    return false;
  }
  out->clear();
  if ((hex.size() % 2) != 0) {
    set_error(error, "hex payload has odd length");
    return false;
  }
  out->reserve(hex.size() / 2);
  auto nibble = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
  };
  for (std::size_t i = 0; i < hex.size(); i += 2) {
    const int hi = nibble(hex[i]);
    const int lo = nibble(hex[i + 1]);
    if (hi < 0 || lo < 0) {
      set_error(error, "hex payload has invalid digit");
      return false;
    }
    out->push_back(static_cast<char>((hi << 4) | lo));
  }
  return true;
}

[[nodiscard]] bool parse_kv_payload(
    std::string_view payload,
    std::unordered_map<std::string, std::string>* out) {
  if (!out) return false;
  out->clear();
  std::size_t cursor = 0;
  while (cursor < payload.size()) {
    std::size_t line_end = payload.find('\n', cursor);
    if (line_end == std::string_view::npos) line_end = payload.size();
    std::string_view line = payload.substr(cursor, line_end - cursor);
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    const std::string trimmed = trim_ascii(line);
    if (!trimmed.empty() && trimmed.front() != '#') {
      const std::size_t eq = trimmed.find('=');
      if (eq != std::string::npos && eq > 0) {
        const std::string key =
            cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(
                trimmed.substr(0, eq));
        const std::string val = trim_ascii(trimmed.substr(eq + 1));
        if (!key.empty()) (*out)[key] = val;
      }
    }
    if (line_end == payload.size()) break;
    cursor = line_end + 1;
  }
  return true;
}

[[nodiscard]] bool parse_json_string_field(std::string_view json,
                                           std::string_view key,
                                           std::string* out) {
  if (!out) return false;
  out->clear();
  const std::string token = "\"" + std::string(key) + "\":";
  const std::size_t pos = json.find(token);
  if (pos == std::string_view::npos) return false;
  std::size_t i = pos + token.size();
  if (i >= json.size() || json[i] != '"') return false;
  ++i;
  std::string value;
  bool esc = false;
  for (; i < json.size(); ++i) {
    const char c = json[i];
    if (esc) {
      switch (c) {
        case 'n':
          value.push_back('\n');
          break;
        case 'r':
          value.push_back('\r');
          break;
        case 't':
          value.push_back('\t');
          break;
        case '\\':
        case '"':
        case '/':
          value.push_back(c);
          break;
        default:
          value.push_back(c);
          break;
      }
      esc = false;
      continue;
    }
    if (c == '\\') {
      esc = true;
      continue;
    }
    if (c == '"') {
      *out = value;
      return true;
    }
    value.push_back(c);
  }
  return false;
}

[[nodiscard]] wave_execution_profile_t profile_from_json(
    std::string_view profile_json) {
  wave_execution_profile_t profile{};
  (void)parse_json_string_field(profile_json, "binding_id", &profile.binding_id);
  (void)parse_json_string_field(profile_json, "wave_id", &profile.wave_id);
  (void)parse_json_string_field(profile_json, "device", &profile.device);
  (void)parse_json_string_field(profile_json, "sampler", &profile.sampler);
  (void)parse_json_string_field(profile_json, "record_type", &profile.record_type);
  (void)parse_json_string_field(profile_json, "dtype", &profile.dtype);
  (void)parse_json_string_field(profile_json, "seed", &profile.seed);
  (void)parse_json_string_field(profile_json, "determinism_policy",
                                &profile.determinism_policy);
  return profile;
}

[[nodiscard]] std::string to_lower(std::string in) {
  std::transform(in.begin(), in.end(), in.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return in;
}

[[nodiscard]] bool is_numeric_close(double lhs, double rhs) {
  constexpr double kAbsEps = 1e-9;
  return std::fabs(lhs - rhs) <= kAbsEps;
}

}  // namespace

std::string compute_coord_hash(std::string_view contract_hash,
                               std::string_view wave_hash) {
  std::string payload;
  payload.reserve(contract_hash.size() + wave_hash.size() + 1);
  payload.append(contract_hash);
  payload.push_back('|');
  payload.append(wave_hash);
  std::string out;
  (void)sha256_hex_bytes(payload, &out);
  return out;
}

std::string canonical_execution_profile_json(
    const wave_execution_profile_t& profile) {
  std::ostringstream out;
  out << "{"
      << "\"binding_id\":" << json_quote(profile.binding_id) << ","
      << "\"wave_id\":" << json_quote(profile.wave_id) << ","
      << "\"device\":" << json_quote(profile.device) << ","
      << "\"sampler\":" << json_quote(profile.sampler) << ","
      << "\"record_type\":" << json_quote(profile.record_type) << ","
      << "\"dtype\":" << json_quote(profile.dtype) << ","
      << "\"seed\":" << json_quote(profile.seed) << ","
      << "\"determinism_policy\":" << json_quote(profile.determinism_policy)
      << "}";
  return out.str();
}

std::string compute_profile_id(const wave_execution_profile_t& profile) {
  const std::string canonical = canonical_execution_profile_json(profile);
  std::string out;
  (void)sha256_hex_bytes(canonical, &out);
  return out;
}

std::string compute_cell_id(std::string_view contract_hash,
                            std::string_view wave_hash,
                            const wave_execution_profile_t& profile) {
  const std::string coord_hash = compute_coord_hash(contract_hash, wave_hash);
  const std::string profile_id = compute_profile_id(profile);
  std::string payload;
  payload.reserve(coord_hash.size() + profile_id.size() + 1);
  payload.append(coord_hash);
  payload.push_back('|');
  payload.append(profile_id);
  std::string out;
  (void)sha256_hex_bytes(payload, &out);
  return out;
}

bool encode_cell_report_payload(const lattice_cell_report_t& report,
                                std::string* out_payload,
                                std::string* error) {
  clear_error(error);
  if (!out_payload) {
    set_error(error, "cell-report payload output pointer is null");
    return false;
  }

  std::ostringstream out;
  out << "report_schema=" << report.report_schema << "\n";
  out << "report_sha256=" << report.report_sha256 << "\n";

  out << "source_report_fragment_count=" << report.source_report_fragment_ids.size() << "\n";
  for (std::size_t i = 0; i < report.source_report_fragment_ids.size(); ++i) {
    out << "source_report_fragment_" << std::setw(4) << std::setfill('0') << i
        << "=" << to_hex(report.source_report_fragment_ids[i]) << "\n";
  }

  out << "summary_num_count=" << report.summary_num.size() << "\n";
  for (std::size_t i = 0; i < report.summary_num.size(); ++i) {
    out << "summary_num_" << std::setw(4) << std::setfill('0') << i
        << "_key=" << to_hex(report.summary_num[i].first) << "\n";
    out << "summary_num_" << std::setw(4) << std::setfill('0') << i
        << "_value=" << std::setprecision(17)
        << report.summary_num[i].second << "\n";
  }

  out << "summary_txt_count=" << report.summary_txt.size() << "\n";
  for (std::size_t i = 0; i < report.summary_txt.size(); ++i) {
    out << "summary_txt_" << std::setw(4) << std::setfill('0') << i
        << "_key=" << to_hex(report.summary_txt[i].first) << "\n";
    out << "summary_txt_" << std::setw(4) << std::setfill('0') << i
        << "_value=" << to_hex(report.summary_txt[i].second) << "\n";
  }

  out << "report_lls_hex=" << to_hex(report.report_lls) << "\n";
  *out_payload = out.str();
  return true;
}

bool decode_cell_report_payload(std::string_view payload,
                                lattice_cell_report_t* out,
                                std::string* error) {
  clear_error(error);
  if (!out) {
    set_error(error, "cell-report payload output pointer is null");
    return false;
  }
  *out = lattice_cell_report_t{};

  std::unordered_map<std::string, std::string> kv{};
  if (!parse_kv_payload(payload, &kv)) {
    set_error(error, "cannot parse cell-report payload");
    return false;
  }

  auto read_u64 = [&](std::string_view key, std::uint64_t* v) {
    const auto it = kv.find(std::string(key));
    if (it == kv.end()) return false;
    return parse_u64(it->second, v);
  };

  if (const auto it = kv.find("report_schema"); it != kv.end()) {
    out->report_schema = it->second;
  }
  if (const auto it = kv.find("report_sha256"); it != kv.end()) {
    out->report_sha256 = it->second;
  }

  std::uint64_t count = 0;
  if (read_u64("source_report_fragment_count", &count)) {
    out->source_report_fragment_ids.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0; i < count; ++i) {
      std::ostringstream k;
      k << "source_report_fragment_" << std::setw(4) << std::setfill('0') << i;
      const auto it = kv.find(k.str());
      if (it == kv.end()) continue;
      std::string decoded;
      if (!from_hex(it->second, &decoded, error)) return false;
      out->source_report_fragment_ids.push_back(std::move(decoded));
    }
  }

  count = 0;
  if (read_u64("summary_num_count", &count)) {
    out->summary_num.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0; i < count; ++i) {
      std::ostringstream kk;
      kk << "summary_num_" << std::setw(4) << std::setfill('0') << i << "_key";
      std::ostringstream kvv;
      kvv << "summary_num_" << std::setw(4) << std::setfill('0') << i
          << "_value";
      const auto itk = kv.find(kk.str());
      const auto itv = kv.find(kvv.str());
      if (itk == kv.end() || itv == kv.end()) continue;

      std::string key;
      if (!from_hex(itk->second, &key, error)) return false;
      double value = 0.0;
      if (!parse_double(itv->second, &value)) continue;
      out->summary_num.emplace_back(std::move(key), value);
    }
  }

  count = 0;
  if (read_u64("summary_txt_count", &count)) {
    out->summary_txt.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0; i < count; ++i) {
      std::ostringstream kk;
      kk << "summary_txt_" << std::setw(4) << std::setfill('0') << i << "_key";
      std::ostringstream kvv;
      kvv << "summary_txt_" << std::setw(4) << std::setfill('0') << i
          << "_value";
      const auto itk = kv.find(kk.str());
      const auto itv = kv.find(kvv.str());
      if (itk == kv.end() || itv == kv.end()) continue;

      std::string key;
      std::string value;
      if (!from_hex(itk->second, &key, error)) return false;
      if (!from_hex(itv->second, &value, error)) return false;
      out->summary_txt.emplace_back(std::move(key), std::move(value));
    }
  }

  if (const auto it = kv.find("report_lls_hex"); it != kv.end()) {
    if (!from_hex(it->second, &out->report_lls, error)) return false;
  }

  return true;
}

lattice_catalog_store_t::~lattice_catalog_store_t() {
  std::string ignored;
  (void)close(&ignored);
}

bool lattice_catalog_store_t::open(const options_t& options, std::string* error) {
  clear_error(error);
  if (db_ != nullptr) {
    set_error(error, "catalog already open");
    return false;
  }

  options_ = options;
  if (options_.catalog_path.empty()) {
    set_error(error, "catalog_path is required");
    return false;
  }

  const auto parent = options_.catalog_path.parent_path();
  if (!parent.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      set_error(error,
                "cannot create catalog directory: " + parent.string());
      return false;
    }
  }

  int rc = IDYDB_ERROR;
  for (int attempt = 0; attempt <= kCatalogOpenBusyRetryCount; ++attempt) {
    if (options_.encrypted) {
      if (options_.passphrase.empty()) {
        set_error(error, "encrypted catalog requires passphrase");
        return false;
      }
      rc = idydb_open_encrypted(options_.catalog_path.string().c_str(), &db_,
                                IDYDB_CREATE, options_.passphrase.c_str());
    } else {
      rc = idydb_open(options_.catalog_path.string().c_str(), &db_, IDYDB_CREATE);
    }

    if (rc == IDYDB_SUCCESS && db_ != nullptr) break;
    if (db_) {
      (void)idydb_close(&db_);
      db_ = nullptr;
    }

    if (rc != IDYDB_BUSY || attempt >= kCatalogOpenBusyRetryCount) break;
    std::this_thread::sleep_for(kCatalogOpenBusyRetryDelay);
  }

  if (rc != IDYDB_SUCCESS || db_ == nullptr) {
    const char* msg = db_ ? idydb_errmsg(&db_) : nullptr;
    std::string detail =
        (msg && msg[0] != '\0')
            ? std::string(msg)
            : ("idydb rc=" + std::to_string(rc) + " (" + idydb_rc_label(rc) + ")");
    if (rc == IDYDB_BUSY) {
      detail += "; catalog lock is held by another process";
    }
    set_error(error, "cannot open lattice catalog: " + detail);
    if (db_) {
      (void)idydb_close(&db_);
      db_ = nullptr;
    }
    return false;
  }

  if (!ensure_catalog_header_(error) || !rebuild_indexes(error)) {
    std::string close_error;
    (void)close(&close_error);
    return false;
  }
  return true;
}

bool lattice_catalog_store_t::close(std::string* error) {
  clear_error(error);
  if (!db_) return true;
  if (idydb_close(&db_) != IDYDB_DONE) {
    const char* msg = idydb_errmsg(&db_);
    set_error(error, "idydb_close failed: " +
                         std::string(msg ? msg : "<no error message>"));
    return false;
  }
  db_ = nullptr;
  cells_by_id_.clear();
  cell_id_by_coord_profile_.clear();
  trials_by_cell_.clear();
  report_by_trial_id_.clear();
  projection_num_by_cell_.clear();
  projection_txt_by_cell_.clear();
  runtime_runs_by_id_.clear();
  runtime_report_fragments_by_id_.clear();
  runtime_latest_report_fragment_by_key_.clear();
  runtime_dependency_files_by_run_id_.clear();
  runtime_reports_by_intersection_.clear();
  runtime_ledger_.clear();
  return true;
}

bool lattice_catalog_store_t::ensure_catalog_header_(std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  idydb_filter_term t_kind{};
  t_kind.column = kColRecordKind;
  t_kind.type = IDYDB_CHAR;
  t_kind.op = IDYDB_FILTER_OP_EQ;
  t_kind.value.s = cuwacunu::hero::schema::kRecordKindHEADER;

  idydb_filter_term t_id{};
  t_id.column = kColRecordId;
  t_id.type = IDYDB_CHAR;
  t_id.op = IDYDB_FILTER_OP_EQ;
  t_id.value.s = "catalog_schema_version";

  bool exists = false;
  if (!db::query::exists_row(&db_, {t_kind, t_id}, &exists, error)) return false;
  if (exists) {
    const idydb_column_row_sizing next = idydb_column_next_row(&db_, kColRecordKind);
    for (idydb_column_row_sizing row = 1; row < next; ++row) {
      const std::string row_kind = as_text_or_empty(&db_, kColRecordKind, row);
      const std::string row_id = as_text_or_empty(&db_, kColRecordId, row);
      if (row_kind != cuwacunu::hero::schema::kRecordKindHEADER ||
          row_id != "catalog_schema_version") {
        continue;
      }
      const std::string schema =
          trim_ascii(as_text_or_empty(&db_, kColTextA, row));
      const std::string version =
          trim_ascii(as_text_or_empty(&db_, kColProjectionVersion, row));
      if (schema != "lattice.catalog.v2" || version != "2") {
        set_error(error,
                  "unsupported lattice catalog schema '" + schema +
                      "' (version '" + version + "'); expected strict v2");
        return false;
      }
      return true;
    }
    set_error(error, "catalog_schema_version header row exists but cannot be read");
    return false;
  }

  return append_row_(cuwacunu::hero::schema::kRecordKindHEADER,
                     "catalog_schema_version", "", "", "", "", "",
                     "schema", std::numeric_limits<double>::quiet_NaN(),
                     "lattice.catalog.v2", "", "2",
                     std::to_string(now_ms_utc()), "{}", "",
                     std::numeric_limits<double>::quiet_NaN(), "", "", "", "",
                     "", "", "", "", "", error);
}

bool lattice_catalog_store_t::append_row_(
    std::string_view record_kind, std::string_view record_id,
    std::string_view cell_id, std::string_view contract_hash,
    std::string_view wave_hash, std::string_view profile_id,
    std::string_view execution_profile_json, std::string_view state_txt,
    double metric_num, std::string_view text_a, std::string_view text_b,
    std::string_view projection_version, std::string_view ts_ms,
    std::string_view payload_json, std::string_view projection_key,
    double projection_num, std::string_view projection_txt,
    std::string_view projection_key_aux,
    std::string_view projection_txt_aux, std::string_view started_at_ms,
    std::string_view finished_at_ms, std::string_view ok_txt,
    std::string_view total_steps, std::string_view campaign_hash,
    std::string_view run_id, std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }
  if (!cuwacunu::hero::schema::is_known_record_kind(record_kind)) {
    set_error(error, "unknown catalog record_kind: " + std::string(record_kind));
    return false;
  }

  const idydb_column_row_sizing row = idydb_column_next_row(&db_, kColRecordKind);
  if (row == 0) {
    set_error(error, "failed to resolve next row");
    return false;
  }

  if (!insert_text(&db_, kColRecordKind, row, record_kind, error)) return false;
  if (!insert_text(&db_, kColRecordId, row, record_id, error)) return false;
  if (!insert_text(&db_, kColCellId, row, cell_id, error)) return false;
  if (!insert_text(&db_, kColContractHash, row, contract_hash, error)) return false;
  if (!insert_text(&db_, kColWaveHash, row, wave_hash, error)) return false;
  if (!insert_text(&db_, kColProfileId, row, profile_id, error)) return false;
  if (!insert_text(&db_, kColExecutionProfileJson, row, execution_profile_json,
                   error)) {
    return false;
  }
  if (!insert_text(&db_, kColStateTxt, row, state_txt, error)) return false;
  if (!insert_num(&db_, kColMetricNum, row, metric_num, error)) return false;
  if (!insert_text(&db_, kColTextA, row, text_a, error)) return false;
  if (!insert_text(&db_, kColTextB, row, text_b, error)) return false;
  if (!insert_text(&db_, kColProjectionVersion, row, projection_version, error)) {
    return false;
  }
  if (!insert_text(&db_, kColTsMs, row, ts_ms, error)) return false;
  if (!insert_text(&db_, kColPayload, row, payload_json, error)) return false;
  if (!insert_text(&db_, kColProjectionKey, row, projection_key, error)) {
    return false;
  }
  if (!insert_num(&db_, kColProjectionNum, row, projection_num, error)) {
    return false;
  }
  if (!insert_text(&db_, kColProjectionTxt, row, projection_txt, error)) {
    return false;
  }
  if (!insert_text(&db_, kColProjectionKeyAux, row, projection_key_aux, error)) {
    return false;
  }
  if (!insert_text(&db_, kColProjectionTxtAux, row, projection_txt_aux, error)) {
    return false;
  }
  if (!insert_text(&db_, kColStartedAtMs, row, started_at_ms, error)) return false;
  if (!insert_text(&db_, kColFinishedAtMs, row, finished_at_ms, error)) {
    return false;
  }
  if (!insert_text(&db_, kColOkTxt, row, ok_txt, error)) return false;
  if (!insert_text(&db_, kColTotalSteps, row, total_steps, error)) return false;
  if (!insert_text(&db_, kColCampaignHash, row, campaign_hash, error)) return false;
  if (!insert_text(&db_, kColRunId, row, run_id, error)) return false;
  return true;
}

bool lattice_catalog_store_t::record_cell_projection_(std::string_view cell_id,
                                                    const wave_projection_t& projection,
                                                    std::uint64_t ts_ms,
                                                    std::string* error) {
  const std::string ts = std::to_string(ts_ms);
  const std::string pv = std::to_string(projection.projection_version);
  std::string projection_lls = trim_ascii(projection.projection_lls);
  if (projection_lls.empty()) {
    projection_lls = build_projection_lls_payload(projection);
  }
  for (const auto& [k, v] : projection.projection_num) {
    const std::string rec_id = std::string(cell_id) + "|projection_num|" + k;
    if (!append_row_(cuwacunu::hero::schema::kRecordKindPROJECTION_NUM, rec_id, cell_id, "", "", "", "", "",
                     std::numeric_limits<double>::quiet_NaN(), "", "", pv, ts,
                     "", k, v, "", "", "", "", "", "", "", "", "",
                     error)) {
      return false;
    }
  }
  for (const auto& [k, v] : projection.projection_txt) {
    const std::string rec_id = std::string(cell_id) + "|projection_txt|" + k;
    if (!append_row_(cuwacunu::hero::schema::kRecordKindPROJECTION_TXT, rec_id,
                     cell_id, "", "", "", "", "",
                     std::numeric_limits<double>::quiet_NaN(), "", "", pv, ts,
                     "", k, std::numeric_limits<double>::quiet_NaN(), v, "", "",
                     "", "", "", "", "", "", error)) {
      return false;
    }
  }

  const std::string rec_id = std::string(cell_id) + "|projection_meta";
  if (!append_row_(cuwacunu::hero::schema::kRecordKindPROJECTION_META, rec_id, cell_id, "", "", "", "", "",
                   std::numeric_limits<double>::quiet_NaN(),
                   projection.projector_build_id, "", pv, ts, projection_lls, "",
                   std::numeric_limits<double>::quiet_NaN(), "", "", "", "",
                   "", "", "", "", "", error)) {
    return false;
  }
  return true;
}

std::string lattice_catalog_store_t::coord_profile_key_(
    std::string_view contract_hash,
    std::string_view wave_hash,
    std::string_view profile_id) {
  std::string out;
  out.reserve(contract_hash.size() + wave_hash.size() + profile_id.size() + 2);
  out.append(contract_hash);
  out.push_back('|');
  out.append(wave_hash);
  out.push_back('|');
  out.append(profile_id);
  return out;
}

bool lattice_catalog_store_t::record_trial(const wave_cell_coord_t& coord,
                                           const wave_execution_profile_t& profile,
                                           const wave_trial_t& trial,
                                           const lattice_cell_report_t& report,
                                           const wave_projection_t& projection,
                                           wave_cell_t* out_cell,
                                           std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  const std::string profile_id = compute_profile_id(profile);
  const std::string cell_id = compute_cell_id(coord.contract_hash, coord.wave_hash,
                                              profile);
  const std::string coord_key = coord_profile_key_(
      coord.contract_hash, coord.wave_hash, profile_id);

  const std::uint64_t ts_now = now_ms_utc();
  wave_cell_t cell{};
  if (const auto it = cells_by_id_.find(cell_id); it != cells_by_id_.end()) {
    cell = it->second;
  } else {
    cell.cell_id = cell_id;
    cell.coord = coord;
    cell.execution_profile = profile;
    cell.created_at_ms = ts_now;
    cell.projection_version = projection.projection_version;
  }

  wave_trial_t stored_trial = trial;
  if (stored_trial.trial_id.empty()) {
    std::string payload = cell_id + "|trial|" + std::to_string(ts_now);
    if (!sha256_hex_bytes(payload, &stored_trial.trial_id, error)) return false;
  }
  stored_trial.cell_id = cell_id;
  if (stored_trial.started_at_ms == 0) stored_trial.started_at_ms = ts_now;
  if (stored_trial.finished_at_ms == 0) {
    stored_trial.finished_at_ms = std::max(stored_trial.started_at_ms, ts_now);
  }

  std::string report_payload;
  if (!encode_cell_report_payload(report, &report_payload, error)) {
    return false;
  }

  cell.execution_profile = profile;
  cell.state = stored_trial.ok ? "ready" : "error";
  cell.trial_count = cell.trial_count + 1;
  cell.last_trial_id = stored_trial.trial_id;
  cell.report = report;
  cell.projection_version = projection.projection_version;
  cell.updated_at_ms = std::max(stored_trial.finished_at_ms, ts_now);

  const std::string execution_profile_json =
      canonical_execution_profile_json(profile);
  const std::string pv = std::to_string(cell.projection_version);

  if (!append_row_(cuwacunu::hero::schema::kRecordKindTRIAL, stored_trial.trial_id, cell_id, coord.contract_hash,
                   coord.wave_hash, profile_id, execution_profile_json,
                   stored_trial.ok ? "ok" : "error",
                   static_cast<double>(stored_trial.total_steps),
                   stored_trial.error, stored_trial.state_snapshot_id, pv,
                   std::to_string(stored_trial.started_at_ms), report_payload, "",
                   std::numeric_limits<double>::quiet_NaN(), "", "", "",
                   std::to_string(stored_trial.started_at_ms),
                   std::to_string(stored_trial.finished_at_ms),
                   stored_trial.ok ? "1" : "0",
                   std::to_string(stored_trial.total_steps),
                   stored_trial.campaign_hash,
                   stored_trial.run_id, error)) {
    return false;
  }

  if (!append_row_(cuwacunu::hero::schema::kRecordKindCELL, cell_id, cell_id, coord.contract_hash, coord.wave_hash,
                   profile_id, execution_profile_json, cell.state,
                   static_cast<double>(cell.trial_count),
                   report.report_schema,
                   report.report_sha256,
                   pv, std::to_string(cell.updated_at_ms), report_payload, "",
                   std::numeric_limits<double>::quiet_NaN(), "", "", "", "",
                   "", "", "", "", "", error)) {
    return false;
  }

  if (!record_cell_projection_(cell_id, projection, cell.updated_at_ms, error)) {
    return false;
  }

  cells_by_id_[cell_id] = cell;
  cell_id_by_coord_profile_[coord_key] = cell_id;
  trials_by_cell_[cell_id].push_back(stored_trial);
  report_by_trial_id_[stored_trial.trial_id] = report;

  auto& projection_num = projection_num_by_cell_[cell_id];
  for (const auto& [k, v] : projection.projection_num) {
    projection_num[k] = v;
  }
  auto& projection_txt = projection_txt_by_cell_[cell_id];
  for (const auto& [k, v] : projection.projection_txt) {
    projection_txt[k] = v;
  }

  if (out_cell) *out_cell = std::move(cell);
  return true;
}

bool lattice_catalog_store_t::rebuild_indexes(std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  cells_by_id_.clear();
  cell_id_by_coord_profile_.clear();
  trials_by_cell_.clear();
  report_by_trial_id_.clear();
  projection_num_by_cell_.clear();
  projection_txt_by_cell_.clear();
  runtime_reports_by_intersection_.clear();

  db::query::query_spec_t q{};
  q.select_columns = {kColRecordKind};
  std::vector<idydb_column_row_sizing> rows{};
  if (!db::query::select_rows(&db_, q, &rows, error)) return false;

  for (const auto row : rows) {
    const std::string kind = as_text_or_empty(&db_, kColRecordKind, row);
    if (kind == cuwacunu::hero::schema::kRecordKindCELL) {
      wave_cell_t cell{};
      cell.cell_id = as_text_or_empty(&db_, kColCellId, row);
      if (cell.cell_id.empty()) {
        cell.cell_id = as_text_or_empty(&db_, kColRecordId, row);
      }
      cell.coord.contract_hash = as_text_or_empty(&db_, kColContractHash, row);
      cell.coord.wave_hash = as_text_or_empty(&db_, kColWaveHash, row);
      const std::string profile_id = as_text_or_empty(&db_, kColProfileId, row);
      const std::string profile_json =
          as_text_or_empty(&db_, kColExecutionProfileJson, row);
      cell.execution_profile = profile_from_json(profile_json);
      cell.state = as_text_or_empty(&db_, kColStateTxt, row);

      if (const auto n = as_num_or_empty(&db_, kColMetricNum, row);
          n.has_value() && n.value() >= 0.0) {
        cell.trial_count = static_cast<std::size_t>(n.value());
      }
      cell.report.report_schema = as_text_or_empty(&db_, kColTextA, row);
      cell.report.report_sha256 = as_text_or_empty(&db_, kColTextB, row);

      if (const std::string pv = as_text_or_empty(&db_, kColProjectionVersion, row);
          !pv.empty()) {
        std::uint64_t parsed = 0;
        if (parse_u64(pv, &parsed)) {
          cell.projection_version = static_cast<std::uint32_t>(
              std::min<std::uint64_t>(parsed,
                                      std::numeric_limits<std::uint32_t>::max()));
        }
      }

      if (const std::string ts = as_text_or_empty(&db_, kColTsMs, row);
          !ts.empty()) {
        std::uint64_t parsed = 0;
        if (parse_u64(ts, &parsed)) {
          cell.updated_at_ms = parsed;
          if (cell.created_at_ms == 0) cell.created_at_ms = parsed;
        }
      }

      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!payload.empty()) {
        lattice_cell_report_t decoded{};
        std::string ignored;
        if (decode_cell_report_payload(payload, &decoded, &ignored)) {
          cell.report = std::move(decoded);
        }
      }

      const std::string map_cell_id = cell.cell_id;
      const std::string map_contract_hash = cell.coord.contract_hash;
      const std::string map_wave_hash = cell.coord.wave_hash;

      if (const auto it = cells_by_id_.find(cell.cell_id); it != cells_by_id_.end()) {
        wave_cell_t merged = cell;
        merged.created_at_ms =
            (it->second.created_at_ms == 0)
                ? cell.created_at_ms
                : std::min(it->second.created_at_ms, cell.created_at_ms);
        if (merged.updated_at_ms < it->second.updated_at_ms) {
          merged.updated_at_ms = it->second.updated_at_ms;
          merged.last_trial_id = it->second.last_trial_id;
        }
        cells_by_id_[cell.cell_id] = std::move(merged);
      } else {
        cells_by_id_[cell.cell_id] = std::move(cell);
      }

      if (!map_contract_hash.empty() && !map_wave_hash.empty() &&
          !profile_id.empty()) {
        const std::string key = coord_profile_key_(
            map_contract_hash, map_wave_hash, profile_id);
        cell_id_by_coord_profile_[key] = map_cell_id;
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindTRIAL) {
      wave_trial_t trial{};
      trial.trial_id = as_text_or_empty(&db_, kColRecordId, row);
      trial.cell_id = as_text_or_empty(&db_, kColCellId, row);
      const std::string started = as_text_or_empty(&db_, kColStartedAtMs, row);
      const std::string finished = as_text_or_empty(&db_, kColFinishedAtMs, row);
      (void)parse_u64(started, &trial.started_at_ms);
      (void)parse_u64(finished, &trial.finished_at_ms);
      trial.ok = as_text_or_empty(&db_, kColOkTxt, row) == "1";
      trial.error = as_text_or_empty(&db_, kColTextA, row);
      trial.state_snapshot_id = as_text_or_empty(&db_, kColTextB, row);
      trial.campaign_hash = as_text_or_empty(&db_, kColCampaignHash, row);
      trial.run_id = as_text_or_empty(&db_, kColRunId, row);
      (void)parse_u64(as_text_or_empty(&db_, kColTotalSteps, row),
                      &trial.total_steps);
      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!trial.trial_id.empty() && !payload.empty()) {
        lattice_cell_report_t decoded{};
        std::string ignored{};
        if (decode_cell_report_payload(payload, &decoded, &ignored)) {
          report_by_trial_id_[trial.trial_id] = std::move(decoded);
        }
      }
      if (!trial.cell_id.empty()) {
        trials_by_cell_[trial.cell_id].push_back(std::move(trial));
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindPROJECTION_NUM) {
      const std::string cell_id = as_text_or_empty(&db_, kColCellId, row);
      const std::string key = as_text_or_empty(&db_, kColProjectionKey, row);
      const auto value = as_num_or_empty(&db_, kColProjectionNum, row);
      if (!cell_id.empty() && !key.empty() && value.has_value()) {
        projection_num_by_cell_[cell_id][key] = value.value();
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindPROJECTION_TXT) {
      const std::string cell_id = as_text_or_empty(&db_, kColCellId, row);
      const std::string key = as_text_or_empty(&db_, kColProjectionKey, row);
      const std::string value = as_text_or_empty(&db_, kColProjectionTxt, row);
      if (!cell_id.empty() && !key.empty()) {
        projection_txt_by_cell_[cell_id][key] = value;
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindRUNTIME_REPORT) {
      const std::string intersection_cursor =
          trim_ascii(as_text_or_empty(&db_, kColRecordId, row));
      if (intersection_cursor.empty()) continue;
      const std::string canonical_path =
          normalize_source_hashimyei_cursor(as_text_or_empty(&db_, kColTextA, row));
      const std::string run_id = trim_ascii(as_text_or_empty(&db_, kColRunId, row));
      const std::string report_lls = as_text_or_empty(&db_, kColPayload, row);
      if (report_lls.empty()) continue;
      std::uint64_t ts_ms = 0;
      (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &ts_ms);

      auto it = runtime_reports_by_intersection_.find(intersection_cursor);
      if (it == runtime_reports_by_intersection_.end() ||
          ts_ms > it->second.ts_ms) {
        runtime_intersection_report_entry_t entry{};
        entry.canonical_path = canonical_path;
        entry.run_id = run_id;
        entry.ts_ms = ts_ms;
        entry.report_lls = report_lls;
        runtime_reports_by_intersection_[intersection_cursor] = std::move(entry);
      }
      continue;
    }
  }

  for (auto& [cell_id, cell] : cells_by_id_) {
    auto it_trials = trials_by_cell_.find(cell_id);
    if (it_trials != trials_by_cell_.end()) {
      auto& trials = it_trials->second;
      std::sort(trials.begin(), trials.end(), [](const wave_trial_t& a,
                                                 const wave_trial_t& b) {
        if (a.started_at_ms != b.started_at_ms) {
          return a.started_at_ms < b.started_at_ms;
        }
        return a.trial_id < b.trial_id;
      });
      cell.trial_count = trials.size();
      if (!trials.empty()) {
        cell.last_trial_id = trials.back().trial_id;
        cell.state = trials.back().ok ? "ready" : "error";
        if (cell.updated_at_ms < trials.back().finished_at_ms) {
          cell.updated_at_ms = trials.back().finished_at_ms;
        }
        if (cell.created_at_ms == 0) {
          cell.created_at_ms = trials.front().started_at_ms;
        }
      }
    }
  }

  return true;
}

bool lattice_catalog_store_t::resolve_cell(const wave_cell_coord_t& coord,
                                        const wave_execution_profile_t& profile,
                                        wave_cell_t* out,
                                        std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "output cell pointer is null");
    return false;
  }
  *out = wave_cell_t{};

  const std::string expected_cell_id =
      compute_cell_id(coord.contract_hash, coord.wave_hash, profile);
  if (const auto it = cells_by_id_.find(expected_cell_id);
      it != cells_by_id_.end()) {
    *out = it->second;
    return true;
  }

  const std::string profile_id = compute_profile_id(profile);
  const std::string key =
      coord_profile_key_(coord.contract_hash, coord.wave_hash, profile_id);
  const auto it_id = cell_id_by_coord_profile_.find(key);
  if (it_id == cell_id_by_coord_profile_.end()) {
    set_error(error, "cell not found for coordinate + execution_profile");
    return false;
  }
  const auto it = cells_by_id_.find(it_id->second);
  if (it == cells_by_id_.end()) {
    set_error(error, "cell id map points to missing cell");
    return false;
  }
  *out = it->second;
  return true;
}

bool lattice_catalog_store_t::get_cell(std::string_view cell_id,
                                    wave_cell_t* out,
                                    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "output cell pointer is null");
    return false;
  }
  *out = wave_cell_t{};
  const auto it = cells_by_id_.find(std::string(cell_id));
  if (it == cells_by_id_.end()) {
    set_error(error, "cell not found: " + std::string(cell_id));
    return false;
  }
  *out = it->second;
  return true;
}

bool lattice_catalog_store_t::list_trials(std::string_view cell_id,
                                       std::size_t limit,
                                       std::size_t offset,
                                       bool newest_first,
                                       std::vector<wave_trial_t>* out,
                                       std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "trials output pointer is null");
    return false;
  }
  out->clear();

  const auto it = trials_by_cell_.find(std::string(cell_id));
  if (it == trials_by_cell_.end()) return true;

  std::vector<wave_trial_t> trials = it->second;
  if (newest_first) {
    std::reverse(trials.begin(), trials.end());
  }

  const std::size_t begin = std::min(offset, trials.size());
  std::size_t end = trials.size();
  if (limit != 0) end = std::min(end, begin + limit);
  out->assign(trials.begin() + static_cast<std::ptrdiff_t>(begin),
              trials.begin() + static_cast<std::ptrdiff_t>(end));
  return true;
}

bool lattice_catalog_store_t::query_matrix(const matrix_query_t& query,
                                        std::vector<wave_cell_t>* out,
                                        std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "matrix output pointer is null");
    return false;
  }
  out->clear();

  std::vector<wave_cell_t> matches{};
  matches.reserve(cells_by_id_.size());

  for (const auto& [_, cell] : cells_by_id_) {
    if (!query.contract_hash.empty() &&
        cell.coord.contract_hash != query.contract_hash) {
      continue;
    }
    if (!query.wave_hash.empty() && cell.coord.wave_hash != query.wave_hash) {
      continue;
    }

    bool matched = true;
    if (const auto it_projection = projection_num_by_cell_.find(cell.cell_id);
        !query.projection_num_eq.empty()) {
      if (it_projection == projection_num_by_cell_.end()) {
        matched = false;
      } else {
        for (const auto& [k, v] : query.projection_num_eq) {
          const auto it = it_projection->second.find(k);
          if (it == it_projection->second.end() ||
              !is_numeric_close(it->second, v)) {
            matched = false;
            break;
          }
        }
      }
    }
    if (!matched) continue;

    if (const auto it_projection = projection_txt_by_cell_.find(cell.cell_id);
        !query.projection_txt_eq.empty()) {
      if (it_projection == projection_txt_by_cell_.end()) {
        matched = false;
      } else {
        for (const auto& [k, v] : query.projection_txt_eq) {
          const auto it = it_projection->second.find(k);
          if (it == it_projection->second.end() || it->second != v) {
            matched = false;
            break;
          }
        }
      }
    }
    if (!matched) continue;

    const auto it_trials = trials_by_cell_.find(cell.cell_id);
    const std::vector<wave_trial_t>* trials =
        (it_trials == trials_by_cell_.end()) ? nullptr : &it_trials->second;

    const wave_trial_t* chosen_trial = nullptr;
    if (trials && !trials->empty()) {
      if (query.latest_success_only) {
        for (auto it = trials->rbegin(); it != trials->rend(); ++it) {
          if (it->ok) {
            chosen_trial = &(*it);
            break;
          }
        }
      } else {
        chosen_trial = &trials->back();
      }
    } else if (query.latest_success_only) {
      continue;
    }

    if (!query.state_snapshot_id.empty()) {
      if (!chosen_trial ||
          chosen_trial->state_snapshot_id != query.state_snapshot_id) {
        continue;
      }
    }

    wave_cell_t selected = cell;
    if (chosen_trial) {
      selected.last_trial_id = chosen_trial->trial_id;
      selected.state = chosen_trial->ok ? "ready" : "error";
      selected.updated_at_ms = std::max(selected.updated_at_ms,
                                        chosen_trial->finished_at_ms);
      const auto it_report = report_by_trial_id_.find(chosen_trial->trial_id);
      if (it_report != report_by_trial_id_.end()) {
        selected.report = it_report->second;
      }
    }

    matches.push_back(std::move(selected));
  }

  std::sort(matches.begin(), matches.end(), [&](const wave_cell_t& a,
                                                const wave_cell_t& b) {
    if (a.updated_at_ms != b.updated_at_ms) {
      return query.newest_first ? (a.updated_at_ms > b.updated_at_ms)
                                : (a.updated_at_ms < b.updated_at_ms);
    }
    return query.newest_first ? (a.cell_id > b.cell_id) : (a.cell_id < b.cell_id);
  });

  const std::size_t begin = std::min(query.offset, matches.size());
  std::size_t end = matches.size();
  if (query.limit != 0) end = std::min(end, begin + query.limit);

  out->assign(matches.begin() + static_cast<std::ptrdiff_t>(begin),
              matches.begin() + static_cast<std::ptrdiff_t>(end));
  return true;
}

bool lattice_catalog_store_t::get_cell_report(std::string_view cell_id,
                                              lattice_cell_report_t* out,
                                              std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "cell-report output pointer is null");
    return false;
  }
  *out = lattice_cell_report_t{};

  wave_cell_t cell{};
  if (!get_cell(cell_id, &cell, error)) return false;
  *out = cell.report;
  return true;
}

bool lattice_catalog_store_t::runtime_ledger_contains_(
    std::string_view report_fragment_id, bool* out_exists, std::string* error) {
  clear_error(error);
  if (!out_exists) {
    set_error(error, "runtime ledger output pointer is null");
    return false;
  }
  *out_exists = false;
  const std::string id(report_fragment_id);
  if (id.empty()) return true;
  if (runtime_ledger_.count(id) != 0) {
    *out_exists = true;
    return true;
  }

  idydb_filter_term t_kind{};
  t_kind.column = kColRecordKind;
  t_kind.type = IDYDB_CHAR;
  t_kind.op = IDYDB_FILTER_OP_EQ;
  t_kind.value.s = cuwacunu::hero::schema::kRecordKindRUNTIME_LEDGER;
  idydb_filter_term t_id{};
  t_id.column = kColRecordId;
  t_id.type = IDYDB_CHAR;
  t_id.op = IDYDB_FILTER_OP_EQ;
  t_id.value.s = id.c_str();
  if (!db::query::exists_row(&db_, {t_kind, t_id}, out_exists, error)) return false;
  if (*out_exists) runtime_ledger_.insert(id);
  return true;
}

bool lattice_catalog_store_t::append_runtime_ledger_(
    std::string_view report_fragment_id, std::string_view path,
    std::string* error) {
  if (!append_row_(cuwacunu::hero::schema::kRecordKindRUNTIME_LEDGER,
                   report_fragment_id, "", "", "", "", "",
                   "", std::numeric_limits<double>::quiet_NaN(),
                   path, "", "2", std::to_string(now_ms_utc()), "{}",
                   "", std::numeric_limits<double>::quiet_NaN(), "", "", "", "",
                   "", "", "", "", "", error)) {
    return false;
  }
  runtime_ledger_.insert(std::string(report_fragment_id));
  return true;
}

bool lattice_catalog_store_t::ingest_runtime_run_manifest_file_(
    const std::filesystem::path& path, std::string* error) {
  clear_error(error);
  cuwacunu::hero::hashimyei::run_manifest_t m{};
  if (!cuwacunu::hero::hashimyei::load_run_manifest(path, &m, error)) return false;

  runtime_runs_by_id_[m.run_id] = m;
  runtime_dependency_files_by_run_id_[m.run_id] = m.dependency_files;

  idydb_filter_term t_kind{};
  t_kind.column = kColRecordKind;
  t_kind.type = IDYDB_CHAR;
  t_kind.op = IDYDB_FILTER_OP_EQ;
  t_kind.value.s = cuwacunu::hero::schema::kRecordKindRUNTIME_RUN;
  idydb_filter_term t_id{};
  t_id.column = kColRecordId;
  t_id.type = IDYDB_CHAR;
  t_id.op = IDYDB_FILTER_OP_EQ;
  t_id.value.s = m.run_id.c_str();
  bool exists = false;
  if (!db::query::exists_row(&db_, {t_kind, t_id}, &exists, error)) return false;
  if (exists) return true;

  std::filesystem::path cp = path;
  if (const auto can = canonicalized(path); can.has_value()) cp = *can;
  std::string payload{};
  if (!read_text_file(path, &payload, error)) return false;
  std::string manifest_sha{};
  if (!sha256_hex_file(cp, &manifest_sha, error)) return false;

  if (!append_row_(cuwacunu::hero::schema::kRecordKindRUNTIME_RUN, m.run_id, "",
                   m.wave_contract_binding.contract.name,
                   m.wave_contract_binding.wave.name,
                   m.wave_contract_binding.identity.name,
                   "", m.schema, static_cast<double>(m.started_at_ms),
                   m.wave_contract_binding.binding_alias,
                   m.campaign_identity.name, "2", std::to_string(m.started_at_ms),
                   payload, "", std::numeric_limits<double>::quiet_NaN(), "", "", "",
                   std::to_string(m.started_at_ms), "", "", "", manifest_sha,
                   m.run_id, error)) {
    return false;
  }
  for (const auto& d : m.dependency_files) {
    const std::string rec_id = m.run_id + "|" + d.canonical_path;
    if (!append_row_(cuwacunu::hero::schema::kRecordKindRUNTIME_DEPENDENCY, rec_id, "", "", "", "", "",
                     d.canonical_path, std::numeric_limits<double>::quiet_NaN(),
                     d.sha256_hex, "", "2", std::to_string(m.started_at_ms), "{}",
                     "", std::numeric_limits<double>::quiet_NaN(), "", "", "", "", "",
                     "", "", "", m.run_id, error)) {
      return false;
    }
  }
  return true;
}

bool lattice_catalog_store_t::ingest_runtime_report_fragment_file_(
    const std::filesystem::path& path, std::string* error) {
  clear_error(error);
  if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
    return true;
  }

  std::string payload;
  if (!read_text_file(path, &payload, nullptr)) return true;
  if (payload.empty()) return true;

  std::unordered_map<std::string, std::string> kv;
  runtime_lls_document_t document{};
  if (!parse_runtime_lls_payload(payload, &kv, &document, error)) {
    set_error(error, "runtime .lls parse failure for " + path.string() + ": " +
                         (error ? *error : std::string{}));
    return false;
  }
  const std::string schema = kv["schema"];
  if (!is_known_runtime_schema(schema)) return true;

  std::string report_fragment_sha;
  if (!sha256_hex_file(path, &report_fragment_sha, error)) return false;

  std::string canonical_path = kv["canonical_path"];
  if (canonical_path.empty()) canonical_path = kv["source_label"];
  if (canonical_path.empty()) canonical_path = canonical_path_from_report_fragment_path(path);
  canonical_path = normalize_source_hashimyei_cursor(canonical_path);

  std::string hashimyei = kv["hashimyei"];
  if (hashimyei.empty()) hashimyei = maybe_hashimyei_from_canonical(canonical_path);

  std::string contract_hash = kv["contract_hash"];
  if (contract_hash.empty()) contract_hash = contract_hash_from_report_fragment_path(path);

  std::uint64_t ts_ms = now_ms_utc();
  {
    std::error_code ec;
    const auto fts = std::filesystem::last_write_time(path, ec);
    if (!ec) {
      const auto d = fts.time_since_epoch();
      const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
      if (ms > 0) ts_ms = static_cast<std::uint64_t>(ms);
    }
  }

  std::string run_id = kv["run_id"];
  if (run_id.empty()) {
    set_error(error,
              "runtime report_fragment missing run_id: " + path.string());
    return false;
  }
  std::uint64_t wave_cursor = 0;
  if (!build_runtime_wave_cursor(
          run_id, kv, runtime_runs_by_id_, &wave_cursor)) {
    set_error(error, "failed to compute runtime wave_cursor for: " + path.string());
    return false;
  }
  const runtime_wave_cursor_resolution_e wave_cursor_resolution =
      infer_runtime_wave_cursor_resolution(kv);
  const std::string wave_cursor_resolution_token =
      runtime_wave_cursor_resolution_token(wave_cursor_resolution);
  upsert_runtime_u64_entry_(&document, "wave_cursor", wave_cursor);
  upsert_runtime_string_entry_(
      &document, "wave_cursor_resolution", wave_cursor_resolution_token);
  upsert_runtime_string_entry_(&document, "hashimyei_cursor", canonical_path);
  upsert_runtime_string_entry_(
      &document,
      "intersection_cursor",
      build_intersection_cursor(canonical_path, wave_cursor));
  const std::string payload_with_cursor =
      cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(document);

  std::filesystem::path cp = path;
  if (const auto can = canonicalized(path); can.has_value()) cp = *can;
  const std::string canonical_file_path = cp.string();

  runtime_report_fragment_t fragment{};
  fragment.report_fragment_id = report_fragment_sha;
  fragment.run_id = run_id;
  fragment.canonical_path = canonical_path;
  fragment.hashimyei = hashimyei;
  fragment.schema = schema;
  fragment.report_fragment_sha256 = report_fragment_sha;
  fragment.path = canonical_file_path;
  fragment.ts_ms = ts_ms;
  fragment.payload_json = payload_with_cursor;
  runtime_report_fragments_by_id_[report_fragment_sha] = fragment;
  const std::string latest_key = join_key(canonical_path, schema);
  auto it_latest = runtime_latest_report_fragment_by_key_.find(latest_key);
  if (it_latest == runtime_latest_report_fragment_by_key_.end()) {
    runtime_latest_report_fragment_by_key_[latest_key] = report_fragment_sha;
  } else {
    const auto it_old = runtime_report_fragments_by_id_.find(it_latest->second);
    if (it_old == runtime_report_fragments_by_id_.end() || ts_ms > it_old->second.ts_ms ||
        (ts_ms == it_old->second.ts_ms && report_fragment_sha > it_old->second.report_fragment_id)) {
      it_latest->second = report_fragment_sha;
    }
  }

  bool already = false;
  if (!runtime_ledger_contains_(report_fragment_sha, &already, error)) return false;
  if (already) return true;

  if (!append_row_(cuwacunu::hero::schema::kRecordKindRUNTIME_REPORT_FRAGMENT, report_fragment_sha, "", contract_hash, "", "", "",
                   schema, std::numeric_limits<double>::quiet_NaN(), canonical_path,
                   hashimyei, "2", std::to_string(ts_ms), payload_with_cursor, "",
                   std::numeric_limits<double>::quiet_NaN(), "", "", "", "", "", "",
                   "", "", run_id, error)) {
    return false;
  }
  return append_runtime_ledger_(report_fragment_sha, canonical_file_path, error);
}

bool lattice_catalog_store_t::ingest_runtime_report_fragments(
    const std::filesystem::path& store_root, std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  const std::filesystem::path canonical_store_root =
      canonicalized(store_root).value_or(store_root.lexically_normal());
  std::error_code ec;

  const auto runs_root = store_root / "runs";
  if (std::filesystem::exists(runs_root, ec) && std::filesystem::is_directory(runs_root, ec)) {
    for (std::filesystem::recursive_directory_iterator it(runs_root, ec), end;
         it != end; it.increment(ec)) {
      if (ec) {
        set_error(error, "failed scanning runs root");
        return false;
      }
      std::error_code entry_ec;
      const auto symlink_state = it->symlink_status(entry_ec);
      if (entry_ec) {
        set_error(error, "failed reading runs entry status");
        return false;
      }
      if (std::filesystem::is_symlink(symlink_state)) continue;
      if (!it->is_regular_file(entry_ec)) {
        if (entry_ec) {
          set_error(error, "failed reading runs entry type");
          return false;
        }
        continue;
      }
      const auto canonical_entry = canonicalized(it->path());
      if (!canonical_entry.has_value() ||
          !path_is_within(canonical_store_root, *canonical_entry)) {
        continue;
      }
      if (it->path().filename() != cuwacunu::hashimyei::kRunManifestFilenameV2) continue;
      if (!ingest_runtime_run_manifest_file_(it->path(), error)) return false;
    }
  }

  for (std::filesystem::recursive_directory_iterator it(store_root, ec), end;
       it != end; it.increment(ec)) {
    if (ec) {
      set_error(error, "failed scanning store root");
      return false;
    }
    std::error_code entry_ec;
    const auto symlink_state = it->symlink_status(entry_ec);
    if (entry_ec) {
      set_error(error, "failed reading store entry status");
      return false;
    }
    if (std::filesystem::is_symlink(symlink_state)) continue;
    if (!it->is_regular_file(entry_ec)) {
      if (entry_ec) {
        set_error(error, "failed reading store entry type");
        return false;
      }
      continue;
    }
    const auto canonical_entry = canonicalized(it->path());
    if (!canonical_entry.has_value() ||
        !path_is_within(canonical_store_root, *canonical_entry)) {
      continue;
    }

    const auto p = it->path();
    if (p == options_.catalog_path) continue;
    if (p.filename() == cuwacunu::hashimyei::kRunManifestFilenameV2) continue;
    if (p.filename() == cuwacunu::hashimyei::kComponentManifestFilenameV2) continue;

    const auto ext = p.extension().string();
    if (ext != ".lls") continue;
    if (!ingest_runtime_report_fragment_file_(p, error)) return false;
  }

  return true;
}

bool lattice_catalog_store_t::list_runtime_runs_by_binding(
    std::string_view contract_hashimyei, std::string_view wave_hashimyei,
    std::string_view binding_hashimyei,
    std::vector<cuwacunu::hero::hashimyei::run_manifest_t>* out,
    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "run list output pointer is null");
    return false;
  }
  out->clear();
  for (const auto& [_, run] : runtime_runs_by_id_) {
    if (!contract_hashimyei.empty() &&
        run.wave_contract_binding.contract.name != contract_hashimyei) {
      continue;
    }
    if (!wave_hashimyei.empty() && run.wave_contract_binding.wave.name != wave_hashimyei) {
      continue;
    }
    if (!binding_hashimyei.empty() &&
        run.wave_contract_binding.identity.name != binding_hashimyei) {
      continue;
    }
    out->push_back(run);
  }
  std::sort(out->begin(), out->end(), [](const auto& a, const auto& b) {
    if (a.started_at_ms != b.started_at_ms) return a.started_at_ms > b.started_at_ms;
    return a.run_id < b.run_id;
  });
  return true;
}

bool lattice_catalog_store_t::list_runtime_report_fragments(
    std::string_view canonical_path, std::string_view schema, std::size_t limit,
    std::size_t offset, bool newest_first,
    std::vector<runtime_report_fragment_t>* out,
    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "report_fragment list output pointer is null");
    return false;
  }
  out->clear();

  const std::string cp = normalize_source_hashimyei_cursor(canonical_path);
  const std::string sc(schema);
  for (const auto& [_, fragment] : runtime_report_fragments_by_id_) {
    const std::string fragment_cp =
        normalize_source_hashimyei_cursor(fragment.canonical_path);
    if (!runtime_hashimyei_cursor_matches(cp, fragment_cp)) continue;
    if (!sc.empty() && fragment.schema != sc) continue;
    runtime_report_fragment_t normalized_fragment = fragment;
    normalized_fragment.canonical_path = fragment_cp;
    out->push_back(std::move(normalized_fragment));
  }
  append_synthetic_source_runtime_report_fragments(
      cells_by_id_, runtime_runs_by_id_, cp, sc, out);

  std::sort(out->begin(), out->end(),
            [newest_first](const auto& a, const auto& b) {
              if (a.ts_ms != b.ts_ms) {
                return newest_first ? (a.ts_ms > b.ts_ms) : (a.ts_ms < b.ts_ms);
              }
              return newest_first ? (a.report_fragment_id > b.report_fragment_id)
                                  : (a.report_fragment_id < b.report_fragment_id);
            });

  const std::size_t off = std::min(offset, out->size());
  std::size_t count = limit;
  if (count == 0) count = out->size() - off;
  count = std::min(count, out->size() - off);
  out->assign(out->begin() + static_cast<std::ptrdiff_t>(off),
              out->begin() + static_cast<std::ptrdiff_t>(off + count));
  return true;
}

bool lattice_catalog_store_t::latest_runtime_report_fragment(
    std::string_view canonical_path, std::string_view schema,
    runtime_report_fragment_t* out, std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "report_fragment output pointer is null");
    return false;
  }
  *out = runtime_report_fragment_t{};

  std::vector<runtime_report_fragment_t> rows{};
  if (!list_runtime_report_fragments(canonical_path, schema, 1, 0, true, &rows,
                                     error)) {
    return false;
  }
  if (rows.empty()) {
    set_error(error, "report_fragment not found for canonical_path/schema");
    return false;
  }
  *out = std::move(rows.front());
  return true;
}

bool lattice_catalog_store_t::get_runtime_report_fragment(
    std::string_view report_fragment_id, runtime_report_fragment_t* out,
    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "report_fragment output pointer is null");
    return false;
  }
  *out = runtime_report_fragment_t{};

  const auto it = runtime_report_fragments_by_id_.find(std::string(report_fragment_id));
  if (it != runtime_report_fragments_by_id_.end()) {
    *out = it->second;
    out->canonical_path = normalize_source_hashimyei_cursor(out->canonical_path);
    return true;
  }

  std::unordered_set<std::string> source_paths{};
  for (const auto& [_, fragment] : runtime_report_fragments_by_id_) {
    const std::string cp =
        normalize_source_hashimyei_cursor(fragment.canonical_path);
    if (starts_with(cp, "tsi.source.")) {
      source_paths.insert(cp);
    }
  }

  for (const auto& cp : source_paths) {
    std::vector<runtime_report_fragment_t> synthetic_rows{};
    append_synthetic_source_runtime_report_fragments(
        cells_by_id_, runtime_runs_by_id_, cp,
        "wave.source.runtime.projection.v2", &synthetic_rows);
    for (const auto& fragment : synthetic_rows) {
      if (fragment.report_fragment_id == report_fragment_id) {
        *out = fragment;
        return true;
      }
    }
  }

  set_error(error, "report_fragment not found: " + std::string(report_fragment_id));
  return false;
}

bool lattice_catalog_store_t::list_runtime_report_schemas(
    std::string_view canonical_path, std::vector<std::string>* out,
    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "schema list output pointer is null");
    return false;
  }
  out->clear();

  std::vector<runtime_report_fragment_t> rows{};
  if (!list_runtime_report_fragments(canonical_path, "", 0, 0, true, &rows,
                                     error)) {
    return false;
  }

  std::unordered_set<std::string> seen{};
  for (const auto& row : rows) {
    if (row.schema.empty()) continue;
    if (!seen.insert(row.schema).second) continue;
    out->push_back(row.schema);
  }
  std::sort(out->begin(), out->end());
  return true;
}

bool lattice_catalog_store_t::upsert_runtime_intersection_report(
    std::string_view intersection_cursor, std::string_view canonical_path,
    std::string_view run_id, std::uint64_t ts_ms, std::string_view report_lls,
    std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }
  const std::string key = trim_ascii(intersection_cursor);
  if (key.empty()) {
    set_error(error, "intersection_cursor is empty");
    return false;
  }
  const std::string payload(report_lls);
  if (payload.empty()) {
    set_error(error, "report_lls is empty");
    return false;
  }
  const std::string cp = normalize_source_hashimyei_cursor(canonical_path);
  const std::string rid = trim_ascii(run_id);
  const std::uint64_t effective_ts = (ts_ms == 0) ? now_ms_utc() : ts_ms;
  const std::string ts = std::to_string(effective_ts);

  auto it_existing = runtime_reports_by_intersection_.find(key);
  if (it_existing != runtime_reports_by_intersection_.end()) {
    const bool same_payload = (it_existing->second.report_lls == payload);
    const bool ts_not_newer = (effective_ts <= it_existing->second.ts_ms);
    if (same_payload && ts_not_newer) {
      return true;
    }
  }

  if (!append_row_(cuwacunu::hero::schema::kRecordKindRUNTIME_REPORT, key, "", "",
                   "", "", "", "wave.intersection.report.v1",
                   std::numeric_limits<double>::quiet_NaN(), cp,
                   "wave.intersection.report.v1", "2", ts, payload, "",
                   std::numeric_limits<double>::quiet_NaN(), "", "", "", "", "",
                   "", "", "", rid, error)) {
    return false;
  }

  runtime_intersection_report_entry_t entry{};
  entry.canonical_path = cp;
  entry.run_id = rid;
  entry.ts_ms = effective_ts;
  entry.report_lls = payload;
  runtime_reports_by_intersection_[key] = std::move(entry);
  return true;
}

bool lattice_catalog_store_t::get_runtime_intersection_report(
    std::string_view intersection_cursor, std::string* out_report_lls,
    std::string* out_canonical_path, std::string* out_run_id,
    std::string* error) const {
  clear_error(error);
  if (!out_report_lls) {
    set_error(error, "intersection report output pointer is null");
    return false;
  }
  out_report_lls->clear();
  if (out_canonical_path) out_canonical_path->clear();
  if (out_run_id) out_run_id->clear();

  const std::string key = trim_ascii(intersection_cursor);
  if (key.empty()) return true;

  const auto it = runtime_reports_by_intersection_.find(key);
  if (it == runtime_reports_by_intersection_.end()) return true;

  *out_report_lls = it->second.report_lls;
  if (out_canonical_path) *out_canonical_path = it->second.canonical_path;
  if (out_run_id) *out_run_id = it->second.run_id;
  return true;
}

bool lattice_catalog_store_t::get_runtime_view_lls(
    std::string_view view_kind, std::string_view run_id,
    std::uint64_t wave_cursor, bool use_wave_cursor,
    std::string_view contract_hash, runtime_view_report_t* out,
    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "runtime view output pointer is null");
    return false;
  }
  *out = runtime_view_report_t{};

  const std::string view_kind_token = trim_ascii(view_kind);
  const std::string run_id_token = trim_ascii(run_id);
  const std::string contract_hash_token = trim_ascii(contract_hash);
  if (view_kind_token.empty()) {
    set_error(error, "view_kind is empty");
    return false;
  }
  if (run_id_token.empty()) {
    set_error(error, "run_id is empty");
    return false;
  }
  if (view_kind_token != kEntropicCapacityComparisonViewKind) {
    set_error(error, "unsupported runtime view_kind: " + view_kind_token);
    return false;
  }

  out->view_kind = view_kind_token;
  out->canonical_path = std::string(kEntropicCapacityComparisonViewCanonicalPath);
  out->run_id = run_id_token;
  out->contract_hash = contract_hash_token;
  out->wave_cursor = wave_cursor;
  out->has_wave_cursor = use_wave_cursor;

  std::vector<runtime_report_fragment_t> source_rows{};
  std::vector<runtime_report_fragment_t> network_rows{};
  if (!list_runtime_report_fragments(
          "tsi.source.dataloader", kSourceDataAnalyticsSchema, 0, 0, true,
          &source_rows, error)) {
    return false;
  }
  if (!list_runtime_report_fragments(
          "tsi.wikimyei.representation.vicreg", kNetworkAnalyticsSchema, 0, 0,
          true, &network_rows, error)) {
    return false;
  }

  std::unordered_map<std::uint64_t, std::vector<entropic_capacity_view_candidate_t>>
      sources_by_wave{};
  std::unordered_map<std::uint64_t, std::vector<entropic_capacity_view_candidate_t>>
      networks_by_wave{};
  auto append_candidate =
      [&](const runtime_report_fragment_t& row,
          std::unordered_map<std::uint64_t,
                             std::vector<entropic_capacity_view_candidate_t>>*
              out_groups) {
        if (row.run_id != run_id_token || !out_groups) return;
        entropic_capacity_view_candidate_t candidate{};
        if (!build_entropic_capacity_view_candidate(row, &candidate)) return;
        if (use_wave_cursor && candidate.wave_cursor != wave_cursor) return;
        if (!contract_hash_token.empty() &&
            candidate.contract_hash != contract_hash_token) {
          return;
        }
        (*out_groups)[candidate.wave_cursor].push_back(std::move(candidate));
      };
  for (const auto& row : source_rows) append_candidate(row, &sources_by_wave);
  for (const auto& row : network_rows) append_candidate(row, &networks_by_wave);

  std::vector<std::uint64_t> group_wave_cursors{};
  group_wave_cursors.reserve(sources_by_wave.size() + networks_by_wave.size());
  for (const auto& [group_wave_cursor, _] : sources_by_wave) {
    if (const auto it = networks_by_wave.find(group_wave_cursor);
        it != networks_by_wave.end() && !it->second.empty()) {
      group_wave_cursors.push_back(group_wave_cursor);
    }
  }
  std::sort(group_wave_cursors.begin(), group_wave_cursors.end());
  group_wave_cursors.erase(
      std::unique(group_wave_cursors.begin(), group_wave_cursors.end()),
      group_wave_cursors.end());

  std::ostringstream view{};
  view << "/* synthetic view transport: "
       << kEntropicCapacityComparisonViewTransportSchema << " */\n";
  append_view_line(&view, "report_transport_schema",
                   kEntropicCapacityComparisonViewTransportSchema);
  append_view_line(&view, "view_kind", out->view_kind);
  append_view_line(&view, "canonical_path", out->canonical_path);
  append_view_line(&view, "run_id", out->run_id);
  if (!out->contract_hash.empty()) {
    append_view_line(&view, "contract_hash", out->contract_hash);
  }
  if (out->has_wave_cursor) {
    append_view_u64_line(&view, "wave_cursor", out->wave_cursor);
    append_view_line(&view, "wave_cursor_view",
                     format_runtime_wave_cursor(out->wave_cursor));
  }

  std::size_t block_index = 0;
  for (const std::uint64_t group_wave_cursor : group_wave_cursors) {
    const auto& sources = sources_by_wave[group_wave_cursor];
    const auto& networks = networks_by_wave[group_wave_cursor];
    if (sources.empty() || networks.empty()) continue;

    ++block_index;
    view << "/* view block " << block_index << " */\n";
    append_view_u64_line(
        &view, "view_block." + std::to_string(block_index) + ".wave_cursor",
        group_wave_cursor);
    append_view_line(
        &view,
        "view_block." + std::to_string(block_index) + ".wave_cursor_view",
        format_runtime_wave_cursor(group_wave_cursor));

    if (sources.size() == 1 && networks.size() == 1) {
      entropic_capacity_view_summary_t comparison{};
      std::string summarize_error{};
      if (!summarize_entropic_capacity_view_from_payloads(
              sources.front().fragment->payload_json,
              networks.front().fragment->payload_json, &comparison,
              &summarize_error)) {
        set_error(
            error,
            "failed to resolve entropic_capacity_comparison view for run_id=" +
                run_id_token + " wave_cursor=" +
                std::to_string(group_wave_cursor) + ": " + summarize_error);
        return false;
      }

      ++out->match_count;
      append_view_line(&view,
                       "view_block." + std::to_string(block_index) + ".kind",
                       "comparison");
      append_view_line(
          &view,
          "view_block." + std::to_string(block_index) +
              ".source.report_fragment_id",
          sources.front().fragment->report_fragment_id);
      append_view_line(
          &view,
          "view_block." + std::to_string(block_index) +
              ".source.canonical_path",
          sources.front().fragment->canonical_path);
      append_view_line(&view,
                       "view_block." + std::to_string(block_index) +
                           ".source.schema",
                       sources.front().fragment->schema);
      if (!sources.front().contract_hash.empty()) {
        append_view_line(
            &view,
            "view_block." + std::to_string(block_index) +
                ".source.contract_hash",
            sources.front().contract_hash);
      }
      append_view_line(
          &view,
          "view_block." + std::to_string(block_index) +
              ".network.report_fragment_id",
          networks.front().fragment->report_fragment_id);
      append_view_line(
          &view,
          "view_block." + std::to_string(block_index) +
              ".network.canonical_path",
          networks.front().fragment->canonical_path);
      append_view_line(&view,
                       "view_block." + std::to_string(block_index) +
                           ".network.schema",
                       networks.front().fragment->schema);
      if (!networks.front().contract_hash.empty()) {
        append_view_line(
            &view,
            "view_block." + std::to_string(block_index) +
                ".network.contract_hash",
            networks.front().contract_hash);
      }
      append_view_double_line(
          &view,
          "view_block." + std::to_string(block_index) + ".source_entropic_load",
          comparison.source_entropic_load);
      append_view_double_line(
          &view,
          "view_block." + std::to_string(block_index) +
              ".network_entropic_capacity",
          comparison.network_entropic_capacity);
      append_view_double_line(
          &view,
          "view_block." + std::to_string(block_index) + ".capacity_margin",
          comparison.capacity_margin);
      append_view_double_line(
          &view,
          "view_block." + std::to_string(block_index) + ".capacity_ratio",
          comparison.capacity_ratio);
      append_view_line(
          &view,
          "view_block." + std::to_string(block_index) + ".capacity_regime",
          comparison.capacity_regime);
      continue;
    }

    ++out->ambiguity_count;
    append_view_line(&view,
                     "view_block." + std::to_string(block_index) + ".kind",
                     "ambiguity");
    append_view_size_line(
        &view,
        "view_block." + std::to_string(block_index) + ".source_count",
        sources.size());
    append_view_size_line(
        &view,
        "view_block." + std::to_string(block_index) + ".network_count",
        networks.size());
    for (std::size_t i = 0; i < sources.size(); ++i) {
      const auto& source = sources[i];
      const std::string prefix =
          "view_block." + std::to_string(block_index) + ".source." +
          std::to_string(i + 1);
      append_view_line(&view, prefix + ".report_fragment_id",
                       source.fragment->report_fragment_id);
      append_view_line(&view, prefix + ".canonical_path",
                       source.fragment->canonical_path);
      append_view_line(&view, prefix + ".schema", source.fragment->schema);
      if (!source.contract_hash.empty()) {
        append_view_line(&view, prefix + ".contract_hash", source.contract_hash);
      }
    }
    for (std::size_t i = 0; i < networks.size(); ++i) {
      const auto& network = networks[i];
      const std::string prefix =
          "view_block." + std::to_string(block_index) + ".network." +
          std::to_string(i + 1);
      append_view_line(&view, prefix + ".report_fragment_id",
                       network.fragment->report_fragment_id);
      append_view_line(&view, prefix + ".canonical_path",
                       network.fragment->canonical_path);
      append_view_line(&view, prefix + ".schema", network.fragment->schema);
      if (!network.contract_hash.empty()) {
        append_view_line(&view, prefix + ".contract_hash", network.contract_hash);
      }
    }
  }

  append_view_size_line(&view, "match_count", out->match_count);
  append_view_size_line(&view, "ambiguity_count", out->ambiguity_count);
  out->view_lls = view.str();
  return true;
}

}  // namespace wave
}  // namespace hero
}  // namespace cuwacunu
