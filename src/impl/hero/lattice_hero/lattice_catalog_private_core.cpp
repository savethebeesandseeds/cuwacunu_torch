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
#include "hero/hashimyei_hero/hashimyei_report_fragments.h"
#include "hero/mcp_server_observability.h"
#include "piaabo/latent_lineage_state/report_taxonomy.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"

namespace cuwacunu {
namespace hero {
namespace wave {
namespace {

using runtime_lls_document_t =
    cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t;
constexpr std::string_view kObservabilityScope = "lattice_catalog";

constexpr std::string_view kEntropicCapacityComparisonViewKind =
    "entropic_capacity_comparison";
constexpr std::string_view kEntropicCapacityComparisonViewCanonicalPath =
    "tsi.analysis.entropic_capacity_comparison";
constexpr std::string_view kEntropicCapacityComparisonViewTransportSchema =
    "lattice.derived_view.entropic_capacity_comparison.v1";
constexpr std::string_view kFamilyEvaluationReportViewKind =
    "family_evaluation_report";
constexpr std::string_view kFamilyEvaluationReportViewCanonicalPath =
    "tsi.analysis.family_evaluation_report";
constexpr std::string_view kFamilyEvaluationReportViewTransportSchema =
    "lattice.derived_view.family_evaluation_report.v1";
constexpr std::string_view kSourceDataAnalyticsSchema =
    "piaabo.torch_compat.data_analytics.v2";
constexpr std::string_view kNetworkAnalyticsSchema =
    "piaabo.torch_compat.network_analytics.v5";
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
  if (!out_document) {
    return cuwacunu::piaabo::latent_lineage_state::
        parse_runtime_lls_text_fast_to_kv_map(payload, out, error);
  }

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

void populate_runtime_report_fragment_header_fields_(
    const std::unordered_map<std::string, std::string>& kv,
    runtime_report_fragment_t* fragment) {
  if (!fragment) return;
  if (fragment->source_label.empty()) {
    if (const auto it = kv.find("source_label"); it != kv.end()) {
      fragment->source_label = trim_ascii(it->second);
    }
  }
  cuwacunu::piaabo::latent_lineage_state::runtime_report_header_t header{};
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_report_header_from_kv(
          kv, &header, nullptr)) {
    return;
  }
  if (fragment->semantic_taxon.empty()) {
    fragment->semantic_taxon = trim_ascii(header.semantic_taxon);
  }
  if (fragment->report_canonical_path.empty()) {
    fragment->report_canonical_path =
        trim_ascii(header.context.canonical_path);
  }
  if (fragment->binding_id.empty()) {
    fragment->binding_id = trim_ascii(header.context.binding_id);
  }
  if (fragment->source_runtime_cursor.empty()) {
    fragment->source_runtime_cursor =
        trim_ascii(header.context.source_runtime_cursor);
  }
  if (fragment->wave_cursor == 0 && header.context.has_wave_cursor) {
    fragment->wave_cursor = header.context.wave_cursor;
  }
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

[[nodiscard]] std::optional<std::filesystem::path> canonicalized(
    const std::filesystem::path& p) {
  std::error_code ec;
  const auto c = std::filesystem::weakly_canonical(p, ec);
  if (ec) return std::nullopt;
  return c;
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
  std::size_t begin = s.find("/tsi/");
  begin = (begin == std::string::npos) ? s.find("tsi/") : begin + 1;
  if (begin == std::string::npos) return {};

  std::vector<std::string> parts{};
  std::size_t pos = begin;
  while (pos <= s.size()) {
    const std::size_t slash = s.find('/', pos);
    if (slash == std::string::npos) {
      parts.emplace_back(s.substr(pos));
      break;
    }
    parts.emplace_back(s.substr(pos, slash - pos));
    pos = slash + 1;
  }

  if (parts.size() >= 5 && parts[0] == "tsi" && parts[1] == "wikimyei" &&
      parts[2] == "representation" && parts[3] == "vicreg" &&
      !parts[4].empty() && parts[4].rfind("0x", 0) == 0) {
    return "tsi.wikimyei.representation.vicreg." + parts[4];
  }

  if (parts.size() >= 3 && parts[0] == "tsi" && parts[1] == "source" &&
      parts[2] == "dataloader") {
    return "tsi.source.dataloader";
  }
  return {};
}

[[nodiscard]] std::string contract_hash_from_report_fragment_path(
    const std::filesystem::path& p) {
  const std::string s = p.generic_string();
  std::size_t begin = s.find("/tsi/");
  begin = (begin == std::string::npos) ? s.find("tsi/") : begin + 1;
  if (begin == std::string::npos) return {};

  std::vector<std::string> parts{};
  std::size_t pos = begin;
  while (pos <= s.size()) {
    const std::size_t slash = s.find('/', pos);
    if (slash == std::string::npos) {
      parts.emplace_back(s.substr(pos));
      break;
    }
    parts.emplace_back(s.substr(pos, slash - pos));
    pos = slash + 1;
  }
  for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
    if (parts[i] == "contracts") return parts[i + 1];
  }
  return {};
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

[[nodiscard]] bool runtime_hashimyei_cursor_matches_fragment(
    std::string_view query_canonical_path,
    const runtime_report_fragment_t& fragment) {
  const std::string query =
      normalize_source_hashimyei_cursor(query_canonical_path);
  if (query.empty()) return true;
  if (runtime_hashimyei_cursor_matches(
          query, fragment.canonical_path)) {
    return true;
  }
  if (!fragment.report_canonical_path.empty() &&
      runtime_hashimyei_cursor_matches(query, fragment.report_canonical_path)) {
    return true;
  }
  return false;
}

[[nodiscard]] std::string build_intersection_cursor(
    std::string_view hashimyei_cursor,
    std::uint64_t wave_cursor) {
  std::ostringstream out;
  out << trim_ascii(hashimyei_cursor) << "|" << wave_cursor;
  return out.str();
}

[[nodiscard]] std::string runtime_family_from_canonical(
    std::string_view canonical_path) {
  const std::string hashimyei = maybe_hashimyei_from_canonical(canonical_path);
  if (hashimyei.empty()) return trim_ascii(canonical_path);
  const std::string canonical = trim_ascii(canonical_path);
  if (canonical.size() <= hashimyei.size() + 1) return canonical;
  return canonical.substr(0, canonical.size() - hashimyei.size() - 1);
}

[[nodiscard]] bool is_family_rank_schema(std::string_view schema) {
  return schema == cuwacunu::hero::family_rank::kFamilyRankSchemaV2;
}

[[nodiscard]] std::string family_rank_record_id(std::string_view family,
                                                std::string_view dock_hash,
                                                std::string_view artifact_sha256) {
  return cuwacunu::hero::family_rank::scope_key(family, dock_hash) + "|" +
         std::string(artifact_sha256);
}

[[nodiscard]] bool is_known_runtime_schema(std::string_view schema) {
  if (schema == "wave.source.runtime.projection.v2") return true;
  if (schema == "piaabo.torch_compat.data_analytics.v2") return true;
  if (schema == "piaabo.torch_compat.data_analytics_symbolic.v2") return true;
  if (schema == "piaabo.torch_compat.embedding_sequence_analytics.v2")
    return true;
  if (schema ==
      "piaabo.torch_compat.embedding_sequence_analytics_symbolic.v2") {
    return true;
  }
  if (schema == "tsi.wikimyei.representation.vicreg.status.v1") return true;
  if (schema == "tsi.wikimyei.representation.vicreg.train_summary.v1") {
    return true;
  }
  if (schema ==
      "tsi.wikimyei.representation.vicreg.transfer_matrix_evaluation.run.v1") {
    return true;
  }
  if (schema == "piaabo.torch_compat.network_analytics.v5") {
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
  runtime_lls_document_t source_document{};
  runtime_lls_document_t network_document{};
  std::string parse_error{};
  if (!parse_runtime_lls_payload(source_payload, &source_kv, &source_document,
                                 &parse_error)) {
    if (error) *error = "invalid source analytics payload: " + parse_error;
    return false;
  }
  parse_error.clear();
  if (!parse_runtime_lls_payload(network_payload, &network_kv, &network_document,
                                 &parse_error)) {
    if (error) *error = "invalid network analytics payload: " + parse_error;
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
  std::uint64_t wave_cursor = fragment.wave_cursor;
  std::string contract_hash = trim_ascii(fragment.contract_hash);
  if (wave_cursor == 0 || contract_hash.empty()) {
    std::unordered_map<std::string, std::string> kv{};
    runtime_lls_document_t document{};
    std::string parse_error{};
    if (!parse_runtime_lls_payload(fragment.payload_json, &kv, &document,
                                   &parse_error)) {
      return false;
    }
    if (wave_cursor == 0) {
      const auto it_wave = kv.find("wave_cursor");
      if (it_wave == kv.end() ||
          (!parse_u64(it_wave->second, &wave_cursor) &&
           !lattice_catalog_store_t::parse_runtime_wave_cursor_token(
               it_wave->second, &wave_cursor))) {
        return false;
      }
    }
    if (contract_hash.empty()) {
      if (const auto it = kv.find("contract_hash");
          it != kv.end() && !trim_ascii(it->second).empty()) {
        contract_hash = trim_ascii(it->second);
      } else {
        contract_hash = contract_hash_from_report_fragment_path(fragment.path);
      }
    }
  }

  out->fragment = &fragment;
  out->wave_cursor = wave_cursor;
  out->contract_hash = std::move(contract_hash);
  return true;
}

void collect_runtime_report_fragment_candidates_(
    const std::unordered_map<std::string, runtime_report_fragment_t>& fragments_by_id,
    const std::unordered_map<std::uint64_t, std::vector<std::string>>&
        fragments_by_wave_cursor,
    std::string_view canonical_path,
    std::string_view schema,
    std::uint64_t wave_cursor,
    bool use_wave_cursor,
    std::vector<runtime_report_fragment_t>* out) {
  if (!out) return;
  out->clear();

  const std::string cp = normalize_source_hashimyei_cursor(canonical_path);
  const std::string sc = trim_ascii(schema);
  const auto accept_fragment = [&](const runtime_report_fragment_t& fragment) {
    if (!sc.empty() && fragment.schema != sc) return;
    if (use_wave_cursor && fragment.wave_cursor != wave_cursor) return;
    if (!runtime_hashimyei_cursor_matches_fragment(cp, fragment)) return;
    out->push_back(fragment);
  };

  if (use_wave_cursor) {
    if (const auto it = fragments_by_wave_cursor.find(wave_cursor);
        it != fragments_by_wave_cursor.end()) {
      for (const auto& report_fragment_id : it->second) {
        const auto it_fragment = fragments_by_id.find(report_fragment_id);
        if (it_fragment == fragments_by_id.end()) continue;
        accept_fragment(it_fragment->second);
      }
    }
  } else {
    for (const auto& [_, fragment] : fragments_by_id) {
      accept_fragment(fragment);
    }
  }

  std::sort(out->begin(), out->end(),
            [](const runtime_report_fragment_t& a,
               const runtime_report_fragment_t& b) {
              if (a.ts_ms != b.ts_ms) return a.ts_ms > b.ts_ms;
              return a.report_fragment_id > b.report_fragment_id;
            });
}

void update_latest_runtime_report_fragment_key_(
    const runtime_report_fragment_t& fragment,
    const std::unordered_map<std::string, runtime_report_fragment_t>& fragments_by_id,
    std::unordered_map<std::string, std::string>* latest_by_key) {
  if (!latest_by_key) return;
  const std::string latest_key = join_key(fragment.canonical_path, fragment.schema);
  auto it_latest = latest_by_key->find(latest_key);
  if (it_latest == latest_by_key->end()) {
    (*latest_by_key)[latest_key] = fragment.report_fragment_id;
    return;
  }
  const auto it_old = fragments_by_id.find(it_latest->second);
  if (it_old == fragments_by_id.end() ||
      fragment.ts_ms > it_old->second.ts_ms ||
      (fragment.ts_ms == it_old->second.ts_ms &&
       fragment.report_fragment_id > it_old->second.report_fragment_id)) {
    it_latest->second = fragment.report_fragment_id;
  }
}

void append_unique_string_(std::string_view value,
                           std::vector<std::string>* values) {
  if (!values) return;
  const std::string token = trim_ascii(value);
  if (token.empty()) return;
  if (std::find(values->begin(), values->end(), token) != values->end()) return;
  values->push_back(token);
}

void append_unique_wave_cursor_(std::uint64_t wave_cursor,
                                std::vector<std::uint64_t>* values) {
  if (!values || wave_cursor == 0) return;
  if (std::find(values->begin(), values->end(), wave_cursor) != values->end()) {
    return;
  }
  values->push_back(wave_cursor);
}

void refresh_runtime_fact_summary_(
    const runtime_report_fragment_t& fragment,
    std::unordered_map<std::string, runtime_fact_summary_t>* summaries_by_canonical) {
  if (!summaries_by_canonical) return;
  const std::string canonical_path =
      normalize_source_hashimyei_cursor(fragment.canonical_path);
  if (canonical_path.empty()) return;

  auto& summary = (*summaries_by_canonical)[canonical_path];
  summary.canonical_path = canonical_path;
  ++summary.fragment_count;

  const bool is_latest_or_tied = fragment.ts_ms >= summary.latest_ts_ms;
  if (fragment.ts_ms > summary.latest_ts_ms) {
    summary.latest_ts_ms = fragment.ts_ms;
  }
  if (fragment.wave_cursor != 0) {
    append_unique_wave_cursor_(fragment.wave_cursor, &summary.wave_cursors);
    if (is_latest_or_tied) {
      summary.latest_wave_cursor = fragment.wave_cursor;
    }
  }
  summary.available_context_count = summary.wave_cursors.size();
  append_unique_string_(fragment.binding_id, &summary.binding_ids);
  append_unique_string_(fragment.semantic_taxon, &summary.semantic_taxa);
  append_unique_string_(fragment.source_runtime_cursor,
                        &summary.source_runtime_cursors);
  append_unique_string_(fragment.schema, &summary.report_schemas);

  summary.recent_fragments.push_back(fragment);
  std::sort(summary.recent_fragments.begin(), summary.recent_fragments.end(),
            [](const runtime_report_fragment_t& a,
               const runtime_report_fragment_t& b) {
              if (a.ts_ms != b.ts_ms) return a.ts_ms > b.ts_ms;
              return a.report_fragment_id > b.report_fragment_id;
            });
  if (summary.recent_fragments.size() > 6) {
    summary.recent_fragments.resize(6);
  }
}

void finalize_runtime_fact_summary_(runtime_fact_summary_t* summary) {
  if (!summary) return;
  std::sort(summary->wave_cursors.begin(), summary->wave_cursors.end());
  std::sort(summary->binding_ids.begin(), summary->binding_ids.end());
  std::sort(summary->semantic_taxa.begin(), summary->semantic_taxa.end());
  std::sort(summary->source_runtime_cursors.begin(),
            summary->source_runtime_cursors.end());
  std::sort(summary->report_schemas.begin(), summary->report_schemas.end());
  std::sort(summary->recent_fragments.begin(), summary->recent_fragments.end(),
            [](const runtime_report_fragment_t& a,
               const runtime_report_fragment_t& b) {
              if (a.ts_ms != b.ts_ms) return a.ts_ms > b.ts_ms;
              return a.report_fragment_id > b.report_fragment_id;
            });
  if (summary->recent_fragments.size() > 6) {
    summary->recent_fragments.resize(6);
  }
  summary->available_context_count = summary->wave_cursors.size();
}

void index_runtime_report_fragment_(
    const runtime_report_fragment_t& fragment,
    std::unordered_map<std::string, runtime_report_fragment_t>* fragments_by_id,
    std::unordered_map<std::string, std::string>* latest_by_key,
    std::unordered_map<std::string, std::vector<std::string>>*
        fragments_by_canonical,
    std::unordered_map<std::string, runtime_fact_summary_t>*
        summaries_by_canonical,
    std::unordered_map<std::uint64_t, std::vector<std::string>>*
        fragments_by_wave_cursor) {
  if (!fragments_by_id) return;
  const bool already_known =
      fragments_by_id->find(fragment.report_fragment_id) != fragments_by_id->end();
  (*fragments_by_id)[fragment.report_fragment_id] = fragment;
  if (!already_known) {
    if (fragments_by_wave_cursor) {
      (*fragments_by_wave_cursor)[fragment.wave_cursor].push_back(
          fragment.report_fragment_id);
    }
    if (fragments_by_canonical && !fragment.canonical_path.empty()) {
      (*fragments_by_canonical)[fragment.canonical_path].push_back(
          fragment.report_fragment_id);
    }
    refresh_runtime_fact_summary_(fragment, summaries_by_canonical);
  }
  update_latest_runtime_report_fragment_key_(
      fragment, *fragments_by_id, latest_by_key);
}

[[nodiscard]] std::string runtime_fragment_component_key_(
    std::string_view canonical_path, std::string_view hashimyei) {
  const std::string canonical =
      normalize_source_hashimyei_cursor(canonical_path);
  if (canonical.empty()) return {};
  std::string normalized_hash = trim_ascii(hashimyei);
  if (normalized_hash.empty()) {
    normalized_hash = maybe_hashimyei_from_canonical(canonical);
  }
  if (normalized_hash.empty()) return {};
  return canonical + "|" + normalized_hash;
}

void refresh_runtime_report_fragment_family_ranks_(
    std::unordered_map<std::string, runtime_report_fragment_t>* fragments_by_id,
    const std::unordered_map<
        std::string, cuwacunu::hero::hashimyei::component_state_t>&
        runtime_components_by_id,
    const std::unordered_map<std::string, cuwacunu::hero::family_rank::state_t>&
        explicit_family_rank_by_scope,
    std::string_view family_filter = {},
    std::string_view dock_hash_filter = {}) {
  if (!fragments_by_id) return;
  const std::string family_token = trim_ascii(family_filter);
  const std::string dock_token = trim_ascii(dock_hash_filter);

  struct runtime_component_scope_t {
    std::uint64_t ts_ms{0};
    std::string component_id{};
    std::string dock_hash{};
  };
  std::unordered_map<std::string, runtime_component_scope_t>
      latest_component_scope_by_key{};
  latest_component_scope_by_key.reserve(runtime_components_by_id.size());
  for (const auto& [component_id, component] : runtime_components_by_id) {
    const std::string dock_hash =
        trim_ascii(component.manifest.docking_signature_sha256_hex);
    if (dock_hash.empty()) continue;
    const std::string key = runtime_fragment_component_key_(
        component.manifest.canonical_path,
        component.manifest.hashimyei_identity.name);
    if (key.empty()) continue;
    const auto it = latest_component_scope_by_key.find(key);
    if (it == latest_component_scope_by_key.end() ||
        component.ts_ms > it->second.ts_ms ||
        (component.ts_ms == it->second.ts_ms &&
         component_id > it->second.component_id)) {
      latest_component_scope_by_key[key] = runtime_component_scope_t{
          .ts_ms = component.ts_ms,
          .component_id = component_id,
          .dock_hash = dock_hash,
      };
    }
  }
  for (auto& [_, fragment] : *fragments_by_id) {
    fragment.family = runtime_family_from_canonical(fragment.canonical_path);
    if (!family_token.empty() && fragment.family != family_token) continue;
    if (trim_ascii(fragment.dock_hash).empty()) {
      const std::string key = runtime_fragment_component_key_(
          fragment.canonical_path, fragment.hashimyei);
      if (!key.empty()) {
        const auto it_component = latest_component_scope_by_key.find(key);
        if (it_component != latest_component_scope_by_key.end()) {
          fragment.dock_hash = it_component->second.dock_hash;
        }
      }
    }
    fragment.family_rank.reset();
    const std::string effective_dock_hash = trim_ascii(fragment.dock_hash);
    if (!dock_token.empty() && effective_dock_hash != dock_token) continue;
    if (effective_dock_hash.empty()) continue;
    const std::string scope_key = cuwacunu::hero::family_rank::scope_key(
        fragment.family, effective_dock_hash);
    const auto it_rank = explicit_family_rank_by_scope.find(scope_key);
    if (it_rank == explicit_family_rank_by_scope.end()) continue;
    fragment.family_rank = cuwacunu::hero::family_rank::rank_for_hashimyei(
        it_rank->second, fragment.hashimyei);
  }
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

void log_lattice_catalog_event(std::string_view event_name,
                               std::string_view extra_fields_json = {}) {
  std::string ignored{};
  (void)cuwacunu::hero::mcp_observability::append_event_json(
      kObservabilityScope, event_name, extra_fields_json, &ignored);
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

