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

namespace cuwacunu {
namespace hero {
namespace wave {
namespace {

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

[[nodiscard]] std::string format_projection_double(double v) {
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(12) << v;
  return oss.str();
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

  std::ostringstream payload;
  payload << "# wave.projection.lls.v2\n";
  payload << "schema=wave.projection.lls.v2\n";
  payload << "projection_version=" << projection.projection_version << "\n";
  payload << "projector_build_id=" << projection.projector_build_id << "\n";

  payload << "\n# section.projection_num\n";
  for (const auto& [k, v] : projection_num) {
    payload << k << "=" << format_projection_double(v) << "\n";
  }

  payload << "\n# section.projection_txt\n";
  for (const auto& [k, v] : projection_txt) {
    payload << k << "=" << v << "\n";
  }

  return cuwacunu::camahjucunu::dsl::convert_latent_lineage_state_payload_to_lattice_state(
      payload.str());
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

[[nodiscard]] std::string canonical_path_from_artifact_path(
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

[[nodiscard]] std::string contract_hash_from_artifact_path(
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
  const std::string cp = trim_ascii(canonical_path);
  if (!starts_with(cp, "tsi.source.")) return cp;
  std::vector<std::string> parts{};
  std::size_t begin = 0;
  while (begin <= cp.size()) {
    const std::size_t dot = cp.find('.', begin);
    if (dot == std::string::npos) {
      parts.push_back(cp.substr(begin));
      break;
    }
    parts.push_back(cp.substr(begin, dot - begin));
    begin = dot + 1;
  }
  if (parts.size() <= 3) return cp;
  const std::string& tail = parts.back();
  if (is_hashimyei_hex_token(tail)) return cp;
  return parts[0] + "." + parts[1] + "." + parts[2];
}

[[nodiscard]] std::string build_intersection_cursor(
    std::string_view hashimyei_cursor,
    std::uint64_t wave_cursor) {
  std::ostringstream out;
  out << trim_ascii(hashimyei_cursor) << "|" << wave_cursor;
  return out.str();
}

[[nodiscard]] bool is_known_runtime_schema(std::string_view schema) {
  if (schema == "wave.source.runtime.projection.v2") return true;
  if (schema == "piaabo.torch_compat.data_analytics.v1") return true;
  if (schema == "tsi.wikimyei.representation.vicreg.status.v1") return true;
  if (schema ==
      "tsi.wikimyei.representation.vicreg.transfer_matrix_evaluation.run.v1") {
    return true;
  }
  if (schema == "piaabo.torch_compat.entropic_capacity_comparison.v1") return true;
  if (schema == "piaabo.torch_compat.network_analytics.v4") {
    return true;
  }
  return false;
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

bool encode_artifact_link_payload(const wave_artifact_link_t& artifact_link,
                                  std::string* out_payload,
                                  std::string* error) {
  clear_error(error);
  if (!out_payload) {
    set_error(error, "artifact payload output pointer is null");
    return false;
  }

  std::ostringstream out;
  out << "aggregate_schema=" << artifact_link.aggregate_schema << "\n";
  out << "aggregate_sha256=" << artifact_link.aggregate_sha256 << "\n";

  out << "artifact_count=" << artifact_link.artifact_ids.size() << "\n";
  for (std::size_t i = 0; i < artifact_link.artifact_ids.size(); ++i) {
    out << "artifact_" << std::setw(4) << std::setfill('0') << i
        << "=" << to_hex(artifact_link.artifact_ids[i]) << "\n";
  }

  out << "num_count=" << artifact_link.numeric_summary.size() << "\n";
  for (std::size_t i = 0; i < artifact_link.numeric_summary.size(); ++i) {
    out << "num_" << std::setw(4) << std::setfill('0') << i
        << "_key=" << to_hex(artifact_link.numeric_summary[i].first) << "\n";
    out << "num_" << std::setw(4) << std::setfill('0') << i
        << "_value=" << std::setprecision(17)
        << artifact_link.numeric_summary[i].second << "\n";
  }

  out << "txt_count=" << artifact_link.text_summary.size() << "\n";
  for (std::size_t i = 0; i < artifact_link.text_summary.size(); ++i) {
    out << "txt_" << std::setw(4) << std::setfill('0') << i
        << "_key=" << to_hex(artifact_link.text_summary[i].first) << "\n";
    out << "txt_" << std::setw(4) << std::setfill('0') << i
        << "_value=" << to_hex(artifact_link.text_summary[i].second) << "\n";
  }

  out << "joined_kv_report_hex=" << to_hex(artifact_link.joined_kv_report)
      << "\n";
  *out_payload = out.str();
  return true;
}

bool decode_artifact_link_payload(std::string_view payload,
                                  wave_artifact_link_t* out,
                                  std::string* error) {
  clear_error(error);
  if (!out) {
    set_error(error, "artifact payload output pointer is null");
    return false;
  }
  *out = wave_artifact_link_t{};

  std::unordered_map<std::string, std::string> kv{};
  if (!parse_kv_payload(payload, &kv)) {
    set_error(error, "cannot parse artifact payload");
    return false;
  }

  auto read_u64 = [&](std::string_view key, std::uint64_t* v) {
    const auto it = kv.find(std::string(key));
    if (it == kv.end()) return false;
    return parse_u64(it->second, v);
  };

  if (const auto it = kv.find("aggregate_schema"); it != kv.end()) {
    out->aggregate_schema = it->second;
  }
  if (const auto it = kv.find("aggregate_sha256"); it != kv.end()) {
    out->aggregate_sha256 = it->second;
  }

  std::uint64_t count = 0;
  if (read_u64("artifact_count", &count)) {
    out->artifact_ids.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0; i < count; ++i) {
      std::ostringstream k;
      k << "artifact_" << std::setw(4) << std::setfill('0') << i;
      const auto it = kv.find(k.str());
      if (it == kv.end()) continue;
      std::string decoded;
      if (!from_hex(it->second, &decoded, error)) return false;
      out->artifact_ids.push_back(std::move(decoded));
    }
  }

  count = 0;
  if (read_u64("num_count", &count)) {
    out->numeric_summary.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0; i < count; ++i) {
      std::ostringstream kk;
      kk << "num_" << std::setw(4) << std::setfill('0') << i << "_key";
      std::ostringstream kvv;
      kvv << "num_" << std::setw(4) << std::setfill('0') << i << "_value";
      const auto itk = kv.find(kk.str());
      const auto itv = kv.find(kvv.str());
      if (itk == kv.end() || itv == kv.end()) continue;

      std::string key;
      if (!from_hex(itk->second, &key, error)) return false;
      double value = 0.0;
      if (!parse_double(itv->second, &value)) continue;
      out->numeric_summary.emplace_back(std::move(key), value);
    }
  }

  count = 0;
  if (read_u64("txt_count", &count)) {
    out->text_summary.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0; i < count; ++i) {
      std::ostringstream kk;
      kk << "txt_" << std::setw(4) << std::setfill('0') << i << "_key";
      std::ostringstream kvv;
      kvv << "txt_" << std::setw(4) << std::setfill('0') << i << "_value";
      const auto itk = kv.find(kk.str());
      const auto itv = kv.find(kvv.str());
      if (itk == kv.end() || itv == kv.end()) continue;

      std::string key;
      std::string value;
      if (!from_hex(itk->second, &key, error)) return false;
      if (!from_hex(itv->second, &value, error)) return false;
      out->text_summary.emplace_back(std::move(key), std::move(value));
    }
  }

  if (const auto it = kv.find("joined_kv_report_hex"); it != kv.end()) {
    if (!from_hex(it->second, &out->joined_kv_report, error)) return false;
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
  artifact_by_trial_id_.clear();
  projection_num_by_cell_.clear();
  projection_txt_by_cell_.clear();
  runtime_runs_by_id_.clear();
  runtime_artifacts_by_id_.clear();
  runtime_latest_artifact_by_key_.clear();
  runtime_provenance_by_run_id_.clear();
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
    std::string_view total_steps, std::string_view board_hash,
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
  if (!insert_text(&db_, kColBoardHash, row, board_hash, error)) return false;
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
                                        const wave_artifact_link_t& artifact_link,
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

  std::string artifact_payload;
  if (!encode_artifact_link_payload(artifact_link, &artifact_payload, error)) {
    return false;
  }

  cell.execution_profile = profile;
  cell.state = stored_trial.ok ? "ready" : "error";
  cell.trial_count = cell.trial_count + 1;
  cell.last_trial_id = stored_trial.trial_id;
  cell.artifact_link = artifact_link;
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
                   std::to_string(stored_trial.started_at_ms), artifact_payload, "",
                   std::numeric_limits<double>::quiet_NaN(), "", "", "",
                   std::to_string(stored_trial.started_at_ms),
                   std::to_string(stored_trial.finished_at_ms),
                   stored_trial.ok ? "1" : "0",
                   std::to_string(stored_trial.total_steps), stored_trial.board_hash,
                   stored_trial.run_id, error)) {
    return false;
  }

  if (!append_row_(cuwacunu::hero::schema::kRecordKindCELL, cell_id, cell_id, coord.contract_hash, coord.wave_hash,
                   profile_id, execution_profile_json, cell.state,
                   static_cast<double>(cell.trial_count),
                   artifact_link.aggregate_schema,
                   artifact_link.aggregate_sha256,
                   pv, std::to_string(cell.updated_at_ms), artifact_payload, "",
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
  artifact_by_trial_id_[stored_trial.trial_id] = artifact_link;

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
  artifact_by_trial_id_.clear();
  projection_num_by_cell_.clear();
  projection_txt_by_cell_.clear();

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
      cell.artifact_link.aggregate_schema = as_text_or_empty(&db_, kColTextA, row);
      cell.artifact_link.aggregate_sha256 = as_text_or_empty(&db_, kColTextB, row);

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
        wave_artifact_link_t decoded{};
        std::string ignored;
        if (decode_artifact_link_payload(payload, &decoded, &ignored)) {
          cell.artifact_link = std::move(decoded);
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
      trial.board_hash = as_text_or_empty(&db_, kColBoardHash, row);
      trial.run_id = as_text_or_empty(&db_, kColRunId, row);
      (void)parse_u64(as_text_or_empty(&db_, kColTotalSteps, row),
                      &trial.total_steps);
      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!trial.trial_id.empty() && !payload.empty()) {
        wave_artifact_link_t decoded{};
        std::string ignored{};
        if (decode_artifact_link_payload(payload, &decoded, &ignored)) {
          artifact_by_trial_id_[trial.trial_id] = std::move(decoded);
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
      const auto it_artifact = artifact_by_trial_id_.find(chosen_trial->trial_id);
      if (it_artifact != artifact_by_trial_id_.end()) {
        selected.artifact_link = it_artifact->second;
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

bool lattice_catalog_store_t::provenance(std::string_view cell_id,
                                      wave_artifact_link_t* out,
                                      std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "provenance output pointer is null");
    return false;
  }
  *out = wave_artifact_link_t{};

  wave_cell_t cell{};
  if (!get_cell(cell_id, &cell, error)) return false;
  *out = cell.artifact_link;
  return true;
}

bool lattice_catalog_store_t::runtime_ledger_contains_(std::string_view artifact_id,
                                                    bool* out_exists,
                                                    std::string* error) {
  clear_error(error);
  if (!out_exists) {
    set_error(error, "runtime ledger output pointer is null");
    return false;
  }
  *out_exists = false;
  const std::string id(artifact_id);
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

bool lattice_catalog_store_t::append_runtime_ledger_(std::string_view artifact_id,
                                                  std::string_view path,
                                                  std::string* error) {
  if (!append_row_(cuwacunu::hero::schema::kRecordKindRUNTIME_LEDGER, artifact_id, "", "", "", "", "",
                   "", std::numeric_limits<double>::quiet_NaN(),
                   path, "", "2", std::to_string(now_ms_utc()), "{}",
                   "", std::numeric_limits<double>::quiet_NaN(), "", "", "", "",
                   "", "", "", "", "", error)) {
    return false;
  }
  runtime_ledger_.insert(std::string(artifact_id));
  return true;
}

bool lattice_catalog_store_t::ingest_runtime_run_manifest_file_(
    const std::filesystem::path& path, std::string* error) {
  clear_error(error);
  cuwacunu::hero::hashimyei::run_manifest_t m{};
  if (!cuwacunu::hero::hashimyei::load_run_manifest(path, &m, error)) return false;

  runtime_runs_by_id_[m.run_id] = m;
  runtime_provenance_by_run_id_[m.run_id] = m.dependency_files;

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
                   m.board_identity.name, "2", std::to_string(m.started_at_ms),
                   payload, "", std::numeric_limits<double>::quiet_NaN(), "", "", "",
                   std::to_string(m.started_at_ms), "", "", "", manifest_sha,
                   m.run_id, error)) {
    return false;
  }
  for (const auto& d : m.dependency_files) {
    const std::string rec_id = m.run_id + "|" + d.canonical_path;
    if (!append_row_(cuwacunu::hero::schema::kRecordKindRUNTIME_PROVENANCE, rec_id, "", "", "", "", "",
                     d.canonical_path, std::numeric_limits<double>::quiet_NaN(),
                     d.sha256_hex, "", "2", std::to_string(m.started_at_ms), "{}",
                     "", std::numeric_limits<double>::quiet_NaN(), "", "", "", "", "",
                     "", "", "", m.run_id, error)) {
      return false;
    }
  }
  return true;
}

bool lattice_catalog_store_t::ingest_runtime_artifact_file_(
    const std::filesystem::path& path, std::string* error) {
  clear_error(error);
  if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
    return true;
  }

  std::string payload;
  if (!read_text_file(path, &payload, nullptr)) return true;
  if (payload.empty()) return true;

  std::unordered_map<std::string, std::string> kv;
  (void)parse_kv_payload(payload, &kv);
  const std::string schema = kv["schema"];
  if (!is_known_runtime_schema(schema)) return true;

  std::string artifact_sha;
  if (!sha256_hex_file(path, &artifact_sha, error)) return false;

  std::string canonical_path = kv["canonical_base"];
  if (canonical_path.empty()) canonical_path = kv["source_label"];
  if (canonical_path.empty()) canonical_path = canonical_path_from_artifact_path(path);
  canonical_path = normalize_source_hashimyei_cursor(canonical_path);

  std::string hashimyei = kv["hashimyei"];
  if (hashimyei.empty()) hashimyei = maybe_hashimyei_from_canonical(canonical_path);

  std::string contract_hash = kv["contract_hash"];
  if (contract_hash.empty()) contract_hash = contract_hash_from_artifact_path(path);

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
              "runtime artifact missing run_id: " + path.string());
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
  std::string payload_with_cursor = payload;
  if (!payload_with_cursor.empty() && payload_with_cursor.back() != '\n') {
    payload_with_cursor.push_back('\n');
  }
  payload_with_cursor += "wave_cursor=" + std::to_string(wave_cursor) + "\n";
  payload_with_cursor +=
      "wave_cursor_resolution=" + wave_cursor_resolution_token + "\n";
  payload_with_cursor += "hashimyei_cursor=" + canonical_path + "\n";
  payload_with_cursor += "intersection_cursor=" +
                         build_intersection_cursor(canonical_path, wave_cursor) +
                         "\n";

  std::filesystem::path cp = path;
  if (const auto can = canonicalized(path); can.has_value()) cp = *can;
  const std::string canonical_file_path = cp.string();

  cuwacunu::hero::hashimyei::artifact_entry_t art{};
  art.artifact_id = artifact_sha;
  art.run_id = run_id;
  art.canonical_path = canonical_path;
  art.hashimyei = hashimyei;
  art.schema = schema;
  art.artifact_sha256 = artifact_sha;
  art.path = canonical_file_path;
  art.ts_ms = ts_ms;
  art.payload_json = payload_with_cursor;
  runtime_artifacts_by_id_[artifact_sha] = art;
  const std::string latest_key = join_key(canonical_path, schema);
  auto it_latest = runtime_latest_artifact_by_key_.find(latest_key);
  if (it_latest == runtime_latest_artifact_by_key_.end()) {
    runtime_latest_artifact_by_key_[latest_key] = artifact_sha;
  } else {
    const auto it_old = runtime_artifacts_by_id_.find(it_latest->second);
    if (it_old == runtime_artifacts_by_id_.end() || ts_ms > it_old->second.ts_ms ||
        (ts_ms == it_old->second.ts_ms && artifact_sha > it_old->second.artifact_id)) {
      it_latest->second = artifact_sha;
    }
  }

  bool already = false;
  if (!runtime_ledger_contains_(artifact_sha, &already, error)) return false;
  if (already) return true;

  if (!append_row_(cuwacunu::hero::schema::kRecordKindRUNTIME_ARTIFACT, artifact_sha, "", contract_hash, "", "", "",
                   schema, std::numeric_limits<double>::quiet_NaN(), canonical_path,
                   hashimyei, "2", std::to_string(ts_ms), payload_with_cursor, "",
                   std::numeric_limits<double>::quiet_NaN(), "", "", "", "", "", "",
                   "", "", run_id, error)) {
    return false;
  }
  return append_runtime_ledger_(artifact_sha, canonical_file_path, error);
}

bool lattice_catalog_store_t::ingest_runtime_reports(
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
    if (!ingest_runtime_artifact_file_(p, error)) return false;
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

bool lattice_catalog_store_t::list_runtime_artifacts(
    std::string_view canonical_path, std::string_view schema, std::size_t limit,
    std::size_t offset, bool newest_first,
    std::vector<cuwacunu::hero::hashimyei::artifact_entry_t>* out,
    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "artifact list output pointer is null");
    return false;
  }
  out->clear();

  const std::string cp = normalize_source_hashimyei_cursor(canonical_path);
  const std::string sc(schema);
  for (const auto& [_, art] : runtime_artifacts_by_id_) {
    const std::string art_cp =
        normalize_source_hashimyei_cursor(art.canonical_path);
    if (!cp.empty() && art_cp != cp) continue;
    if (!sc.empty() && art.schema != sc) continue;
    cuwacunu::hero::hashimyei::artifact_entry_t normalized_art = art;
    normalized_art.canonical_path = art_cp;
    out->push_back(std::move(normalized_art));
  }

  if (!cp.empty() && starts_with(cp, "tsi.source.") &&
      (sc.empty() || sc == "wave.source.runtime.projection.v2")) {
    for (const auto& [_, cell] : cells_by_id_) {
      std::unordered_map<std::string, std::string> joined_kv{};
      (void)parse_kv_payload(cell.artifact_link.joined_kv_report, &joined_kv);
      const auto it_schema = joined_kv.find("source.runtime.projection.schema");
      if (it_schema == joined_kv.end()) continue;
      const std::string synthetic_schema = trim_ascii(it_schema->second);
      if (synthetic_schema.empty()) continue;
      if (!sc.empty() && synthetic_schema != sc) continue;

      const auto it_symbol = joined_kv.find("source.runtime.symbol");
      const std::string source_symbol =
          (it_symbol == joined_kv.end()) ? "" : trim_ascii(it_symbol->second);
      (void)source_symbol;

      std::string synthetic_run_id{};
      if (const auto it = joined_kv.find("source.runtime.projection.run_id");
          it != joined_kv.end()) {
        synthetic_run_id = trim_ascii(it->second);
      }
      if (synthetic_run_id.empty()) {
        if (const auto it = joined_kv.find("run_id"); it != joined_kv.end()) {
          synthetic_run_id = trim_ascii(it->second);
        }
      }
      if (synthetic_run_id.empty()) continue;

      std::vector<std::pair<std::string, std::string>> source_pairs{};
      source_pairs.reserve(joined_kv.size());
      for (const auto& [k, v] : joined_kv) {
        if (starts_with(k, "source.runtime.")) {
          source_pairs.push_back({k, v});
        }
      }
      std::sort(source_pairs.begin(), source_pairs.end(),
                [](const auto& a, const auto& b) {
                  if (a.first != b.first) return a.first < b.first;
                  return a.second < b.second;
                });

      std::unordered_map<std::string, std::string> runtime_kv{};
      runtime_kv["run_id"] = synthetic_run_id;
      std::uint64_t synthetic_wave_cursor = 0;
      if (!build_runtime_wave_cursor(synthetic_run_id, runtime_kv,
                                     runtime_runs_by_id_,
                                     &synthetic_wave_cursor)) {
        continue;
      }

      std::ostringstream payload;
      payload << "schema=" << synthetic_schema << "\n";
      payload << "run_id=" << synthetic_run_id << "\n";
      payload << "canonical_base=" << cp << "\n";
      payload << "source_label=" << cp << "\n";
      payload << "wave_cursor=" << synthetic_wave_cursor << "\n";
      payload << "wave_cursor_resolution=run\n";
      payload << "hashimyei_cursor=" << cp << "\n";
      payload << "intersection_cursor="
              << build_intersection_cursor(cp, synthetic_wave_cursor)
              << "\n";
      for (const auto& [k, v] : source_pairs) {
        payload << k << "=" << v << "\n";
      }
      const std::string synthetic_payload = payload.str();

      std::string synthetic_artifact_id{};
      const std::string seed = "source-runtime-synth|" + cell.cell_id + "|" +
                               synthetic_run_id + "|" + synthetic_payload;
      if (!sha256_hex_bytes(seed, &synthetic_artifact_id, nullptr)) continue;

      cuwacunu::hero::hashimyei::artifact_entry_t synthetic{};
      synthetic.artifact_id = synthetic_artifact_id;
      synthetic.run_id = synthetic_run_id;
      synthetic.canonical_path = cp;
      synthetic.hashimyei = maybe_hashimyei_from_canonical(cp);
      synthetic.schema = synthetic_schema;
      synthetic.artifact_sha256 = synthetic_artifact_id;
      synthetic.path = "[lattice.synthetic.source_runtime cell=" + cell.cell_id +
                       "]";
      synthetic.ts_ms = cell.updated_at_ms;
      synthetic.payload_json = synthetic_payload;
      out->push_back(std::move(synthetic));
    }
  }

  std::sort(out->begin(), out->end(),
            [newest_first](const auto& a, const auto& b) {
              if (a.ts_ms != b.ts_ms) {
                return newest_first ? (a.ts_ms > b.ts_ms) : (a.ts_ms < b.ts_ms);
              }
              return newest_first ? (a.artifact_id > b.artifact_id)
                                  : (a.artifact_id < b.artifact_id);
            });

  const std::size_t off = std::min(offset, out->size());
  std::size_t count = limit;
  if (count == 0) count = out->size() - off;
  count = std::min(count, out->size() - off);
  out->assign(out->begin() + static_cast<std::ptrdiff_t>(off),
              out->begin() + static_cast<std::ptrdiff_t>(off + count));
  return true;
}

}  // namespace wave
}  // namespace hero
}  // namespace cuwacunu
