#include "HERO/hashimyei/hashimyei_catalog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <string_view>
#include <unordered_set>
#include <unistd.h>

#include <openssl/evp.h>

#include "camahjucunu/db/idydb.h"

namespace cuwacunu {
namespace hero {
namespace hashimyei {
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

[[nodiscard]] bool parse_u64(std::string_view text, std::uint64_t* out) {
  if (!out) return false;
  const std::string t = trim_ascii(text);
  if (t.empty()) return false;
  std::uint64_t v = 0;
  std::istringstream iss(t);
  iss >> v;
  if (!iss || !iss.eof()) return false;
  *out = v;
  return true;
}

[[nodiscard]] std::uint64_t now_ms_utc() {
  const auto now = std::chrono::system_clock::now();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
          .count());
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
  *out_hex = hex_lower(digest, digest_len);
  return true;
}

[[nodiscard]] bool sha256_hex_file(const std::filesystem::path& path,
                                   std::string* out_hex, std::string* error) {
  if (error) error->clear();
  if (!out_hex) {
    if (error) *error = "sha256 output pointer is null";
    return false;
  }
  out_hex->clear();

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error) *error = "cannot open file for sha256: " + path.string();
    return false;
  }

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) {
    if (error) *error = "cannot allocate EVP context";
    return false;
  }
  if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
    EVP_MD_CTX_free(ctx);
    if (error) *error = "EVP_DigestInit_ex failed";
    return false;
  }

  std::array<char, 1 << 14> buf{};
  while (in.good()) {
    in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
    const std::streamsize got = in.gcount();
    if (got <= 0) break;
    if (EVP_DigestUpdate(ctx, buf.data(), static_cast<std::size_t>(got)) != 1) {
      EVP_MD_CTX_free(ctx);
      if (error) *error = "EVP_DigestUpdate failed";
      return false;
    }
  }

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
    EVP_MD_CTX_free(ctx);
    if (error) *error = "EVP_DigestFinal_ex failed";
    return false;
  }
  EVP_MD_CTX_free(ctx);
  *out_hex = hex_lower(digest, digest_len);
  return true;
}

[[nodiscard]] bool read_text_file(const std::filesystem::path& path,
                                  std::string* out,
                                  std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "text output pointer is null";
    return false;
  }
  out->clear();

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error) *error = "cannot read file: " + path.string();
    return false;
  }
  out->assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  if (!in.eof() && in.fail()) {
    if (error) *error = "cannot read file: " + path.string();
    return false;
  }
  return true;
}

[[nodiscard]] std::string to_json_string(std::string_view in) {
  std::ostringstream out;
  out << '"';
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

[[nodiscard]] std::string run_manifest_payload(const run_manifest_t& m) {
  std::ostringstream out;
  out << "schema=" << m.schema << "\n";
  out << "run_id=" << m.run_id << "\n";
  out << "started_at_ms=" << m.started_at_ms << "\n";
  out << "board_hash=" << m.board_hash << "\n";
  out << "contract_hash=" << m.contract_hash << "\n";
  out << "wave_hash=" << m.wave_hash << "\n";
  out << "binding_id=" << m.binding_id << "\n";
  out << "sampler=" << m.sampler << "\n";
  out << "record_type=" << m.record_type << "\n";
  out << "seed=" << m.seed << "\n";
  out << "device=" << m.device << "\n";
  out << "dtype=" << m.dtype << "\n";
  out << "dep_count=" << m.dependency_files.size() << "\n";
  for (std::size_t i = 0; i < m.dependency_files.size(); ++i) {
    out << "dep_" << std::setw(4) << std::setfill('0') << i << "_path="
        << m.dependency_files[i].canonical_path << "\n";
    out << "dep_" << std::setw(4) << std::setfill('0') << i << "_sha256="
        << m.dependency_files[i].sha256_hex << "\n";
  }
  out << "component_count=" << m.components.size() << "\n";
  for (std::size_t i = 0; i < m.components.size(); ++i) {
    out << "component_" << std::setw(4) << std::setfill('0') << i
        << "_canonical_path=" << m.components[i].canonical_path << "\n";
    out << "component_" << std::setw(4) << std::setfill('0') << i
        << "_tsi_type=" << m.components[i].tsi_type << "\n";
    out << "component_" << std::setw(4) << std::setfill('0') << i
        << "_hashimyei=" << m.components[i].hashimyei << "\n";
  }
  return out.str();
}

[[nodiscard]] bool is_known_schema(std::string_view schema) {
  if (schema == "piaabo.torch_compat.data_analytics.v1") return true;
  if (schema == "piaabo.torch_compat.entropic_capacity_comparison.v1")
    return true;
  if (schema == "piaabo.torch_compat.network_analytics.v1" ||
      schema == "piaabo.torch_compat.network_analytics.v2" ||
      schema == "piaabo.torch_compat.network_analytics.v3" ||
      schema == "piaabo.torch_compat.network_analytics.v4") {
    return true;
  }
  if (schema.rfind(
          "tsi.probe.representation.transfer_matrix_evaluation.", 0) == 0) {
    return schema ==
               "tsi.probe.representation.transfer_matrix_evaluation.epoch.v1" ||
           schema ==
               "tsi.probe.representation.transfer_matrix_evaluation.history.v2" ||
           schema ==
               "tsi.probe.representation.transfer_matrix_evaluation.activation.v1" ||
           schema ==
               "tsi.probe.representation.transfer_matrix_evaluation.prequential_raw.v1";
  }
  return false;
}

[[nodiscard]] bool parse_double_token(std::string_view in, double* out) {
  if (!out) return false;
  const std::string t = trim_ascii(in);
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

[[nodiscard]] std::optional<std::filesystem::path> canonicalized(
    const std::filesystem::path& p) {
  std::error_code ec;
  const auto c = std::filesystem::weakly_canonical(p, ec);
  if (ec) return std::nullopt;
  return c;
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

  const std::string probe_hash = find_after(
      "/tsi.probe/representation/transfer_matrix_evaluation/");
  if (!probe_hash.empty()) {
    return "tsi.probe.representation.transfer_matrix_evaluation." + probe_hash;
  }

  const std::string vicreg_hash =
      find_after("/tsi.wikimyei/representation/vicreg/");
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

[[nodiscard]] std::string as_text_or_empty(idydb** db,
                                           idydb_column_row_sizing column,
                                           idydb_column_row_sizing row) {
  if (idydb_extract(db, column, row) != IDYDB_DONE) return {};
  if (idydb_retrieved_type(db) != IDYDB_CHAR) return {};
  const char* s = idydb_retrieve_char(db);
  return s ? std::string(s) : std::string{};
}

[[nodiscard]] std::optional<double> as_numeric_or_null(idydb** db,
                                                       idydb_column_row_sizing column,
                                                       idydb_column_row_sizing row) {
  if (idydb_extract(db, column, row) != IDYDB_DONE) return std::nullopt;
  if (idydb_retrieved_type(db) != IDYDB_FLOAT) return std::nullopt;
  return static_cast<double>(idydb_retrieve_float(db));
}

void clear_error(std::string* error) {
  if (error) error->clear();
}

void set_error(std::string* error, std::string_view msg) {
  if (error) *error = std::string(msg);
}

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

[[nodiscard]] std::string join_key(std::string_view canonical_path,
                                   std::string_view schema) {
  return std::string(canonical_path) + "|" + std::string(schema);
}

}  // namespace

bool parse_key_value_payload(
    std::string_view payload, std::unordered_map<std::string, std::string>* out) {
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
      if (eq != std::string_view::npos && eq > 0) {
        const std::string key = trim_ascii(trimmed.substr(0, eq));
        const std::string val = trim_ascii(trimmed.substr(eq + 1));
        if (!key.empty()) (*out)[key] = val;
      }
    }
    if (line_end == payload.size()) break;
    cursor = line_end + 1;
  }
  return true;
}

std::string compute_run_id(std::string_view board_hash,
                           std::string_view contract_hash,
                           std::string_view wave_hash, std::string_view binding_id,
                           std::uint64_t started_at_ms) {
  std::ostringstream seed;
  seed << board_hash << "|" << contract_hash << "|" << wave_hash << "|"
       << binding_id << "|" << started_at_ms;
  std::string hash_hex;
  if (!sha256_hex_bytes(seed.str(), &hash_hex) || hash_hex.empty()) {
    return "run.invalid";
  }
  return "run." + hash_hex.substr(0, 24);
}

bool load_run_manifest(const std::filesystem::path& path, run_manifest_t* out,
                       std::string* error) {
  clear_error(error);
  if (!out) {
    set_error(error, "run manifest output pointer is null");
    return false;
  }
  *out = run_manifest_t{};

  std::string payload;
  std::string read_error;
  if (!read_text_file(path, &payload, &read_error)) {
    set_error(error, "cannot read run manifest: " + read_error);
    return false;
  }

  std::unordered_map<std::string, std::string> kv;
  (void)parse_key_value_payload(payload, &kv);
  out->schema = kv["schema"];
  out->run_id = kv["run_id"];
  (void)parse_u64(kv["started_at_ms"], &out->started_at_ms);
  out->board_hash = kv["board_hash"];
  out->contract_hash = kv["contract_hash"];
  out->wave_hash = kv["wave_hash"];
  out->binding_id = kv["binding_id"];
  out->sampler = kv["sampler"];
  out->record_type = kv["record_type"];
  out->seed = kv["seed"];
  out->device = kv["device"];
  out->dtype = kv["dtype"];

  std::uint64_t dep_count = 0;
  if (parse_u64(kv["dep_count"], &dep_count)) {
    out->dependency_files.reserve(static_cast<std::size_t>(dep_count));
    for (std::uint64_t i = 0; i < dep_count; ++i) {
      std::ostringstream pfx;
      pfx << "dep_" << std::setw(4) << std::setfill('0') << i << "_";
      dependency_file_t d{};
      d.canonical_path = kv[pfx.str() + "path"];
      d.sha256_hex = kv[pfx.str() + "sha256"];
      if (!d.canonical_path.empty() || !d.sha256_hex.empty()) {
        out->dependency_files.push_back(std::move(d));
      }
    }
  }

  std::uint64_t component_count = 0;
  if (parse_u64(kv["component_count"], &component_count)) {
    out->components.reserve(static_cast<std::size_t>(component_count));
    for (std::uint64_t i = 0; i < component_count; ++i) {
      std::ostringstream pfx;
      pfx << "component_" << std::setw(4) << std::setfill('0') << i << "_";
      component_instance_t c{};
      c.canonical_path = kv[pfx.str() + "canonical_path"];
      c.tsi_type = kv[pfx.str() + "tsi_type"];
      c.hashimyei = kv[pfx.str() + "hashimyei"];
      if (!c.canonical_path.empty() || !c.tsi_type.empty() || !c.hashimyei.empty()) {
        out->components.push_back(std::move(c));
      }
    }
  }

  if (out->schema.empty() || out->run_id.empty()) {
    set_error(error, "invalid run manifest: missing schema or run_id");
    return false;
  }
  return true;
}

bool parse_component_manifest_kv(
    const std::unordered_map<std::string, std::string>& kv,
    component_manifest_t* out,
    std::string* error) {
  clear_error(error);
  if (!out) {
    set_error(error, "component manifest output pointer is null");
    return false;
  }

  *out = component_manifest_t{};
  out->schema = kv.count("schema") != 0 ? kv.at("schema")
                                         : "hashimyei.component.manifest.v1";
  out->canonical_path = kv.count("canonical_path") != 0
                            ? kv.at("canonical_path")
                            : kv.count("canonical_base") != 0 ? kv.at("canonical_base")
                                                               : std::string{};
  out->tsi_type = kv.count("tsi_type") != 0
                      ? kv.at("tsi_type")
                      : kv.count("canonical_type") != 0 ? kv.at("canonical_type")
                                                        : std::string{};
  out->hashimyei = kv.count("hashimyei") != 0 ? kv.at("hashimyei") : std::string{};
  out->contract_hash = kv.count("contract_hash") != 0
                           ? kv.at("contract_hash")
                           : std::string{};
  out->wave_hash = kv.count("wave_hash") != 0 ? kv.at("wave_hash") : std::string{};
  out->binding_id = kv.count("binding_id") != 0 ? kv.at("binding_id") : std::string{};
  out->dsl_canonical_path = kv.count("dsl_canonical_path") != 0
                                ? kv.at("dsl_canonical_path")
                                : kv.count("dsl_path") != 0 ? kv.at("dsl_path")
                                                             : std::string{};
  out->dsl_sha256_hex = kv.count("dsl_sha256_hex") != 0
                            ? kv.at("dsl_sha256_hex")
                            : kv.count("dsl_sha256") != 0 ? kv.at("dsl_sha256")
                                                           : std::string{};
  out->status = kv.count("status") != 0 ? kv.at("status") : std::string{"active"};
  out->replaced_by = kv.count("replaced_by") != 0 ? kv.at("replaced_by") : std::string{};
  (void)parse_u64(kv.count("created_at_ms") != 0 ? kv.at("created_at_ms") : "0",
                  &out->created_at_ms);
  (void)parse_u64(kv.count("updated_at_ms") != 0 ? kv.at("updated_at_ms") : "0",
                  &out->updated_at_ms);

  return validate_component_manifest(*out, error);
}

bool validate_component_manifest(const component_manifest_t& manifest,
                                 std::string* error) {
  clear_error(error);
  const auto starts_with = [](std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() &&
           text.substr(0, prefix.size()) == prefix;
  };
  const auto is_hex_64 = [](std::string_view s) {
    if (s.size() != 64) return false;
    for (char c : s) {
      const bool digit = c >= '0' && c <= '9';
      const bool lower = c >= 'a' && c <= 'f';
      const bool upper = c >= 'A' && c <= 'F';
      if (!digit && !lower && !upper) return false;
    }
    return true;
  };

  if (manifest.schema != "hashimyei.component.manifest.v1") {
    set_error(error, "invalid component manifest schema");
    return false;
  }
  if (manifest.canonical_path.empty() ||
      !starts_with(manifest.canonical_path, "tsi.")) {
    set_error(error, "component manifest missing canonical_path");
    return false;
  }
  if (manifest.tsi_type.empty() || !starts_with(manifest.tsi_type, "tsi.")) {
    set_error(error, "component manifest missing tsi_type");
    return false;
  }
  if (!manifest.hashimyei.empty() && maybe_hashimyei_from_canonical(
                                          "prefix." + manifest.hashimyei)
                                          .empty()) {
    set_error(error, "component manifest hashimyei is not a valid 0x... token");
    return false;
  }
  if (manifest.contract_hash.empty() || manifest.wave_hash.empty() ||
      manifest.binding_id.empty()) {
    set_error(error, "component manifest missing contract_hash/wave_hash/binding_id");
    return false;
  }
  if (manifest.dsl_canonical_path.empty()) {
    set_error(error, "component manifest missing dsl_canonical_path");
    return false;
  }
  if (!is_hex_64(manifest.dsl_sha256_hex)) {
    set_error(error, "component manifest dsl_sha256_hex must be 64 hex chars");
    return false;
  }

  const std::string status = trim_ascii(manifest.status);
  if (status != "active" && status != "deprecated" && status != "replaced" &&
      status != "tombstone") {
    set_error(error, "component manifest status must be active/deprecated/replaced/tombstone");
    return false;
  }
  if (status == "replaced" && manifest.replaced_by.empty()) {
    set_error(error, "component manifest status=replaced requires replaced_by");
    return false;
  }
  if (manifest.created_at_ms == 0) {
    set_error(error, "component manifest missing created_at_ms");
    return false;
  }
  if (manifest.updated_at_ms != 0 && manifest.updated_at_ms < manifest.created_at_ms) {
    set_error(error, "component manifest updated_at_ms is older than created_at_ms");
    return false;
  }
  return true;
}

bool load_component_manifest(const std::filesystem::path& path,
                             component_manifest_t* out,
                             std::string* error) {
  clear_error(error);
  if (!out) {
    set_error(error, "component manifest output pointer is null");
    return false;
  }
  *out = component_manifest_t{};

  std::string payload;
  std::string read_error;
  if (!read_text_file(path, &payload, &read_error)) {
    set_error(error, "cannot read component manifest: " + read_error);
    return false;
  }
  if (payload.empty()) {
    set_error(error, "component manifest payload is empty");
    return false;
  }

  std::unordered_map<std::string, std::string> kv;
  (void)parse_key_value_payload(payload, &kv);
  if (!parse_component_manifest_kv(kv, out, error)) return false;
  return true;
}

std::string compute_component_manifest_id(const component_manifest_t& manifest) {
  std::ostringstream seed;
  seed << manifest.canonical_path << "|" << manifest.tsi_type << "|"
       << manifest.hashimyei << "|" << manifest.contract_hash << "|"
       << manifest.wave_hash << "|" << manifest.binding_id << "|"
       << manifest.dsl_canonical_path << "|" << manifest.dsl_sha256_hex;
  std::string digest;
  if (!sha256_hex_bytes(seed.str(), &digest) || digest.empty()) {
    return "component.invalid";
  }
  return "component." + digest.substr(0, 24);
}

bool save_run_manifest(const std::filesystem::path& store_root,
                       const run_manifest_t& manifest,
                       std::filesystem::path* out_path, std::string* error) {
  clear_error(error);
  if (manifest.run_id.empty()) {
    set_error(error, "run manifest requires run_id");
    return false;
  }

  std::error_code ec;
  const auto dir = store_root / "runs" / manifest.run_id;
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    set_error(error, "cannot create run manifest directory: " + dir.string());
    return false;
  }

  const auto target = dir / "run.manifest.v1.kv";
  const auto tmp = target.string() + ".tmp";
  std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
  if (!out) {
    set_error(error, "cannot write run manifest temporary file: " + tmp);
    return false;
  }
  out << run_manifest_payload(manifest);
  out.flush();
  if (!out) {
    set_error(error, "cannot flush run manifest temporary file: " + tmp);
    return false;
  }
  out.close();

  std::filesystem::rename(tmp, target, ec);
  if (ec) {
    std::filesystem::remove(tmp, ec);
    set_error(error, "cannot rename run manifest into place: " + target.string());
    return false;
  }
  if (out_path) *out_path = target;
  return true;
}

hashimyei_catalog_store_t::~hashimyei_catalog_store_t() {
  std::string ignored;
  (void)close(&ignored);
}

bool hashimyei_catalog_store_t::open(const options_t& options, std::string* error) {
  clear_error(error);
  if (db_ != nullptr) {
    set_error(error, "catalog already open");
    return false;
  }

  std::error_code ec;
  const auto parent = options.catalog_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      set_error(error, "cannot create catalog directory: " + parent.string());
      return false;
    }
  }

  int rc = IDYDB_ERROR;
  if (options.encrypted) {
    if (options.passphrase.empty()) {
      set_error(error, "encrypted catalog requires passphrase");
      return false;
    }
    rc = idydb_open_encrypted(options.catalog_path.string().c_str(), &db_,
                              IDYDB_CREATE, options.passphrase.c_str());
  } else {
    rc = idydb_open(options.catalog_path.string().c_str(), &db_, IDYDB_CREATE);
  }
  if (rc != IDYDB_SUCCESS) {
    const char* msg = db_ ? idydb_errmsg(&db_) : nullptr;
    set_error(error, "cannot open catalog: " +
                         std::string(msg ? msg : "<no error message>"));
    if (db_) {
      idydb_close(&db_);
      db_ = nullptr;
    }
    return false;
  }

  options_ = options;
  if (!ensure_catalog_header_(error)) {
    std::string ignored;
    (void)close(&ignored);
    return false;
  }
  return rebuild_indexes(error);
}

bool hashimyei_catalog_store_t::close(std::string* error) {
  clear_error(error);
  if (!db_) return true;
  const int rc = idydb_close(&db_);
  db_ = nullptr;
  if (rc != IDYDB_DONE) {
    set_error(error, "idydb_close failed");
    return false;
  }
  runs_by_id_.clear();
  artifacts_by_id_.clear();
  latest_artifact_by_key_.clear();
  metrics_num_by_artifact_.clear();
  metrics_txt_by_artifact_.clear();
  components_by_id_.clear();
  latest_component_by_canonical_.clear();
  latest_component_by_hashimyei_.clear();
  provenance_by_run_id_.clear();
  return true;
}

bool hashimyei_catalog_store_t::ensure_catalog_header_(std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  idydb_filter_term t_kind{};
  t_kind.column = kColRecordKind;
  t_kind.type = IDYDB_CHAR;
  t_kind.op = IDYDB_FILTER_OP_EQ;
  t_kind.value.s = "header";
  idydb_filter_term t_id{};
  t_id.column = kColRecordId;
  t_id.type = IDYDB_CHAR;
  t_id.op = IDYDB_FILTER_OP_EQ;
  t_id.value.s = "catalog_schema_version";

  bool exists = false;
  if (!db::query::exists_row(&db_, {t_kind, t_id}, &exists, error)) return false;
  if (exists) return true;

  return append_row_("header", "catalog_schema_version", "", "", "", "hashimyei.catalog.v1",
                     "catalog_schema_version", std::numeric_limits<double>::quiet_NaN(),
                     "1", "", options_.catalog_path.string(),
                     std::to_string(now_ms_utc()), "{}", error);
}

bool hashimyei_catalog_store_t::append_row_(
    std::string_view record_kind, std::string_view record_id, std::string_view run_id,
    std::string_view canonical_path, std::string_view hashimyei, std::string_view schema,
    std::string_view metric_key, double metric_num, std::string_view metric_txt,
    std::string_view artifact_sha256, std::string_view path, std::string_view ts_ms,
    std::string_view payload_json, std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }
  const idydb_column_row_sizing row = idydb_column_next_row(&db_, kColRecordKind);
  if (row == 0) {
    set_error(error, "failed to resolve next row");
    return false;
  }
  if (!insert_text(&db_, kColRecordKind, row, record_kind, error)) return false;
  if (!insert_text(&db_, kColRecordId, row, record_id, error)) return false;
  if (!insert_text(&db_, kColRunId, row, run_id, error)) return false;
  if (!insert_text(&db_, kColCanonicalPath, row, canonical_path, error)) return false;
  if (!insert_text(&db_, kColHashimyei, row, hashimyei, error)) return false;
  if (!insert_text(&db_, kColSchema, row, schema, error)) return false;
  if (!insert_text(&db_, kColMetricKey, row, metric_key, error)) return false;
  if (!insert_num(&db_, kColMetricNum, row, metric_num, error)) return false;
  if (!insert_text(&db_, kColMetricTxt, row, metric_txt, error)) return false;
  if (!insert_text(&db_, kColArtifactSha256, row, artifact_sha256, error)) return false;
  if (!insert_text(&db_, kColPath, row, path, error)) return false;
  if (!insert_text(&db_, kColTsMs, row, ts_ms, error)) return false;
  if (!insert_text(&db_, kColPayload, row, payload_json, error)) return false;
  return true;
}

bool hashimyei_catalog_store_t::ledger_contains_(std::string_view artifact_sha256,
                                                 bool* out_exists,
                                                 std::string* error) {
  clear_error(error);
  if (!out_exists) {
    set_error(error, "out_exists is null");
    return false;
  }
  *out_exists = false;
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }
  if (artifact_sha256.empty()) return true;

  idydb_filter_term t_kind{};
  t_kind.column = kColRecordKind;
  t_kind.type = IDYDB_CHAR;
  t_kind.op = IDYDB_FILTER_OP_EQ;
  t_kind.value.s = "ledger";
  idydb_filter_term t_sha{};
  t_sha.column = kColArtifactSha256;
  t_sha.type = IDYDB_CHAR;
  t_sha.op = IDYDB_FILTER_OP_EQ;
  const std::string sha(artifact_sha256);
  t_sha.value.s = sha.c_str();
  return db::query::exists_row(&db_, {t_kind, t_sha}, out_exists, error);
}

bool hashimyei_catalog_store_t::append_ledger_(std::string_view artifact_sha256,
                                               std::string_view path,
                                               std::string* error) {
  return append_row_("ledger", std::string(artifact_sha256), "", "", "", "",
                     "ingest_version", static_cast<double>(options_.ingest_version),
                     "", artifact_sha256, path, std::to_string(now_ms_utc()), "{}",
                     error);
}

bool hashimyei_catalog_store_t::ingest_run_manifest_file_(const std::filesystem::path& path,
                                                          std::string* error) {
  clear_error(error);
  run_manifest_t m{};
  if (!load_run_manifest(path, &m, error)) return false;

  idydb_filter_term t_kind{};
  t_kind.column = kColRecordKind;
  t_kind.type = IDYDB_CHAR;
  t_kind.op = IDYDB_FILTER_OP_EQ;
  t_kind.value.s = "run";
  idydb_filter_term t_id{};
  t_id.column = kColRecordId;
  t_id.type = IDYDB_CHAR;
  t_id.op = IDYDB_FILTER_OP_EQ;
  t_id.value.s = m.run_id.c_str();
  bool exists = false;
  if (!db::query::exists_row(&db_, {t_kind, t_id}, &exists, error)) return false;
  if (!exists) {
    std::filesystem::path cp = path;
    if (const auto can = canonicalized(path); can.has_value()) cp = *can;
    std::string manifest_sha;
    if (!sha256_hex_file(cp, &manifest_sha, error)) return false;
    if (!append_row_("run", m.run_id, m.run_id, "", "", m.schema, "",
                     std::numeric_limits<double>::quiet_NaN(), m.binding_id,
                     manifest_sha, cp.string(), std::to_string(m.started_at_ms),
                     run_manifest_payload(m), error)) {
      return false;
    }
    for (const auto& d : m.dependency_files) {
      const std::string rec_id = m.run_id + "|" + d.canonical_path;
      if (!append_row_("provenance", rec_id, m.run_id, "", "", m.schema,
                       d.canonical_path,
                       std::numeric_limits<double>::quiet_NaN(), d.sha256_hex, "",
                       cp.string(), std::to_string(m.started_at_ms), "{}", error)) {
        return false;
      }
    }
  }
  runs_by_id_[m.run_id] = std::move(m);
  return true;
}

bool hashimyei_catalog_store_t::ingest_component_manifest_file_(
    const std::filesystem::path& path, std::string* error) {
  clear_error(error);
  if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
    set_error(error, "component manifest path is not a regular file");
    return false;
  }

  component_manifest_t manifest{};
  if (!load_component_manifest(path, &manifest, error)) return false;

  std::string payload;
  std::string read_error;
  if (!read_text_file(path, &payload, &read_error)) {
    set_error(error, "cannot read component manifest payload: " + read_error);
    return false;
  }

  std::string artifact_sha;
  if (!sha256_hex_file(path, &artifact_sha, error)) return false;
  bool already = false;
  if (!ledger_contains_(artifact_sha, &already, error)) return false;
  if (already) return true;

  std::filesystem::path cp = path;
  if (const auto can = canonicalized(path); can.has_value()) cp = *can;

  std::uint64_t ts_ms = manifest.updated_at_ms != 0 ? manifest.updated_at_ms
                                                     : manifest.created_at_ms;
  if (ts_ms == 0) ts_ms = now_ms_utc();

  const std::string component_id = compute_component_manifest_id(manifest);
  if (!append_row_("component", component_id, "", manifest.canonical_path,
                   manifest.hashimyei, manifest.schema, manifest.tsi_type,
                   std::numeric_limits<double>::quiet_NaN(), manifest.status,
                   artifact_sha, cp.string(), std::to_string(ts_ms), payload,
                   error)) {
    return false;
  }

  const std::string prov_id = component_id + "|dsl";
  if (!append_row_("component_provenance", prov_id, "", manifest.canonical_path,
                   manifest.hashimyei, manifest.schema,
                   manifest.dsl_canonical_path,
                   std::numeric_limits<double>::quiet_NaN(),
                   manifest.dsl_sha256_hex, "", cp.string(),
                   std::to_string(ts_ms), "{}", error)) {
    return false;
  }

  return append_ledger_(artifact_sha, cp.string(), error);
}

bool hashimyei_catalog_store_t::append_legacy_run_if_needed_(
    std::string_view contract_hash, std::uint64_t ts_ms, std::string* out_run_id,
    std::string* error) {
  clear_error(error);
  if (!out_run_id) {
    set_error(error, "out_run_id is null");
    return false;
  }
  std::string contract = std::string(contract_hash);
  if (contract.empty()) contract = "unknown_contract";
  const std::string run_id =
      "legacy." + compute_run_id("legacy_board", contract, "legacy_wave",
                                 "legacy_binding", ts_ms).substr(4);

  auto it = runs_by_id_.find(run_id);
  if (it != runs_by_id_.end()) {
    *out_run_id = run_id;
    return true;
  }

  run_manifest_t legacy{};
  legacy.run_id = run_id;
  legacy.started_at_ms = ts_ms;
  legacy.board_hash = "legacy_board";
  legacy.contract_hash = contract;
  legacy.wave_hash = "legacy_wave";
  legacy.binding_id = "legacy_binding";
  legacy.sampler = "legacy_sampler";
  legacy.record_type = "legacy_record_type";
  legacy.seed = "unknown";
  legacy.device = "unknown";
  legacy.dtype = "unknown";

  if (!append_row_("run", legacy.run_id, legacy.run_id, "", "", legacy.schema, "",
                   std::numeric_limits<double>::quiet_NaN(), legacy.binding_id, "",
                   "legacy", std::to_string(ts_ms), run_manifest_payload(legacy),
                   error)) {
    return false;
  }
  runs_by_id_[legacy.run_id] = legacy;
  *out_run_id = legacy.run_id;
  return true;
}

bool hashimyei_catalog_store_t::parse_and_append_metrics_(
    std::string_view artifact_id,
    const std::unordered_map<std::string, std::string>& kv, std::string* error) {
  clear_error(error);
  for (const auto& [k, v] : kv) {
    if (k == "schema" || k == "run_id" || k == "canonical_base" || k == "hashimyei" ||
        k == "contract_hash") {
      continue;
    }

    double num = 0.0;
    if (parse_double_token(v, &num)) {
      const std::string rec_id = std::string(artifact_id) + "|num|" + k;
      if (!append_row_("metric_num", rec_id, "", "", "", "", k, num, "",
                       artifact_id, "", std::to_string(now_ms_utc()), "{}", error)) {
        return false;
      }
    } else {
      const std::string rec_id = std::string(artifact_id) + "|txt|" + k;
      if (!append_row_("metric_txt", rec_id, "", "", "", "", k,
                       std::numeric_limits<double>::quiet_NaN(), v, artifact_id, "",
                       std::to_string(now_ms_utc()), "{}", error)) {
        return false;
      }
    }
  }
  return true;
}

bool hashimyei_catalog_store_t::ingest_artifact_file_(const std::filesystem::path& path,
                                                      bool backfill_legacy,
                                                      std::string* error) {
  clear_error(error);
  if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
    return true;
  }

  std::string payload;
  if (!read_text_file(path, &payload, nullptr)) {
    return true;
  }
  if (payload.empty()) return true;

  std::unordered_map<std::string, std::string> kv;
  (void)parse_key_value_payload(payload, &kv);
  const std::string schema = kv["schema"];
  if (!is_known_schema(schema)) return true;

  std::string artifact_sha;
  if (!sha256_hex_file(path, &artifact_sha, error)) return false;
  bool already = false;
  if (!ledger_contains_(artifact_sha, &already, error)) return false;
  if (already) return true;

  std::string canonical_path = kv["canonical_base"];
  if (canonical_path.empty()) canonical_path = kv["source_label"];
  if (canonical_path.empty()) canonical_path = canonical_path_from_artifact_path(path);

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
    for (const auto& [id, run] : runs_by_id_) {
      if (!contract_hash.empty() && run.contract_hash == contract_hash) {
        run_id = id;
        break;
      }
    }
  }
  if (run_id.empty() && backfill_legacy) {
    if (!append_legacy_run_if_needed_(contract_hash, ts_ms, &run_id, error)) {
      return false;
    }
  }

  std::filesystem::path cp = path;
  if (const auto can = canonicalized(path); can.has_value()) cp = *can;

  if (!append_row_("artifact", artifact_sha, run_id, canonical_path, hashimyei, schema,
                   "", std::numeric_limits<double>::quiet_NaN(), "", artifact_sha,
                   cp.string(), std::to_string(ts_ms), payload, error)) {
    return false;
  }
  if (!parse_and_append_metrics_(artifact_sha, kv, error)) return false;
  return append_ledger_(artifact_sha, cp.string(), error);
}

bool hashimyei_catalog_store_t::acquire_ingest_lock_(
    const std::filesystem::path& store_root, ingest_lock_t* lock, std::string* error) {
  clear_error(error);
  if (!lock) {
    set_error(error, "lock pointer is null");
    return false;
  }
  lock->path.clear();
  lock->held = false;
  std::error_code ec;
  const auto lock_dir = store_root / "catalog";
  std::filesystem::create_directories(lock_dir, ec);
  if (ec) {
    set_error(error, "cannot create lock directory: " + lock_dir.string());
    return false;
  }
  const auto lock_path = lock_dir / ".ingest.lock";
  if (std::filesystem::exists(lock_path, ec)) {
    set_error(error, "ingest lock already held: " + lock_path.string());
    return false;
  }
  std::ofstream out(lock_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    set_error(error, "cannot create ingest lock file: " + lock_path.string());
    return false;
  }
  out << "pid=" << ::getpid() << "\n";
  out.close();
  lock->path = lock_path;
  lock->held = true;
  return true;
}

void hashimyei_catalog_store_t::release_ingest_lock_(ingest_lock_t* lock) {
  if (!lock || !lock->held) return;
  std::error_code ec;
  std::filesystem::remove(lock->path, ec);
  lock->held = false;
}

bool hashimyei_catalog_store_t::ingest_filesystem(const std::filesystem::path& store_root,
                                                  bool backfill_legacy,
                                                  std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  ingest_lock_t lock{};
  if (!acquire_ingest_lock_(store_root, &lock, error)) return false;
  const auto release_lock = [&]() { release_ingest_lock_(&lock); };

  std::error_code ec;
  const auto runs_root = store_root / "runs";
  if (std::filesystem::exists(runs_root, ec) && std::filesystem::is_directory(runs_root, ec)) {
    for (std::filesystem::recursive_directory_iterator it(runs_root, ec), end;
         it != end; it.increment(ec)) {
      if (ec) {
        release_lock();
        set_error(error, "failed scanning runs root");
        return false;
      }
      if (!it->is_regular_file()) continue;
      if (it->path().filename() != "run.manifest.v1.kv") continue;
      if (!ingest_run_manifest_file_(it->path(), error)) {
        release_lock();
        return false;
      }
    }
  }

  for (std::filesystem::recursive_directory_iterator it(store_root, ec), end;
       it != end; it.increment(ec)) {
    if (ec) {
      release_lock();
      set_error(error, "failed scanning store root");
      return false;
    }
    if (!it->is_regular_file()) continue;

    const auto p = it->path();
    if (p == options_.catalog_path) continue;
    if (p.filename() == "run.manifest.v1.kv") continue;
    if (p.filename() == "component.manifest.v1.kv") {
      if (!ingest_component_manifest_file_(p, error)) {
        release_lock();
        return false;
      }
      continue;
    }

    const auto ext = p.extension().string();
    if (ext != ".kv") continue;
    if (!ingest_artifact_file_(p, backfill_legacy, error)) {
      release_lock();
      return false;
    }
  }

  release_lock();
  return rebuild_indexes(error);
}

bool hashimyei_catalog_store_t::rebuild_indexes(std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  runs_by_id_.clear();
  artifacts_by_id_.clear();
  latest_artifact_by_key_.clear();
  metrics_num_by_artifact_.clear();
  metrics_txt_by_artifact_.clear();
  components_by_id_.clear();
  latest_component_by_canonical_.clear();
  latest_component_by_hashimyei_.clear();
  provenance_by_run_id_.clear();

  const idydb_column_row_sizing next = idydb_column_next_row(&db_, kColRecordKind);
  for (idydb_column_row_sizing row = 1; row < next; ++row) {
    const std::string kind = as_text_or_empty(&db_, kColRecordKind, row);
    if (kind.empty()) continue;

    if (kind == "run") {
      run_manifest_t run{};
      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!payload.empty()) {
        std::unordered_map<std::string, std::string> kv;
        (void)parse_key_value_payload(payload, &kv);
        run.schema = kv["schema"];
        run.run_id = kv["run_id"];
        (void)parse_u64(kv["started_at_ms"], &run.started_at_ms);
        run.board_hash = kv["board_hash"];
        run.contract_hash = kv["contract_hash"];
        run.wave_hash = kv["wave_hash"];
        run.binding_id = kv["binding_id"];
        run.sampler = kv["sampler"];
        run.record_type = kv["record_type"];
        run.seed = kv["seed"];
        run.device = kv["device"];
        run.dtype = kv["dtype"];
      }
      if (run.run_id.empty()) run.run_id = as_text_or_empty(&db_, kColRunId, row);
      if (run.schema.empty()) run.schema = as_text_or_empty(&db_, kColSchema, row);
      if (run.started_at_ms == 0) {
        std::uint64_t parsed = 0;
        if (parse_u64(as_text_or_empty(&db_, kColTsMs, row), &parsed)) run.started_at_ms = parsed;
      }
      if (!run.run_id.empty()) runs_by_id_[run.run_id] = std::move(run);
      continue;
    }

    if (kind == "provenance") {
      const std::string run_id = as_text_or_empty(&db_, kColRunId, row);
      const std::string dep_path = as_text_or_empty(&db_, kColMetricKey, row);
      const std::string dep_sha = as_text_or_empty(&db_, kColMetricTxt, row);
      if (!run_id.empty() && !dep_path.empty()) {
        provenance_by_run_id_[run_id].push_back({dep_path, dep_sha});
      }
      continue;
    }

    if (kind == "component") {
      component_state_t component{};
      component.component_id = as_text_or_empty(&db_, kColRecordId, row);
      component.manifest_path = as_text_or_empty(&db_, kColPath, row);
      component.artifact_sha256 = as_text_or_empty(&db_, kColArtifactSha256, row);
      (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &component.ts_ms);

      component.manifest.schema = as_text_or_empty(&db_, kColSchema, row);
      component.manifest.canonical_path =
          as_text_or_empty(&db_, kColCanonicalPath, row);
      component.manifest.hashimyei = as_text_or_empty(&db_, kColHashimyei, row);
      component.manifest.tsi_type = as_text_or_empty(&db_, kColMetricKey, row);
      component.manifest.status = as_text_or_empty(&db_, kColMetricTxt, row);

      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!payload.empty()) {
        std::unordered_map<std::string, std::string> kv;
        (void)parse_key_value_payload(payload, &kv);
        component_manifest_t parsed_manifest{};
        std::string ignored_error;
        if (parse_component_manifest_kv(kv, &parsed_manifest, &ignored_error)) {
          component.manifest = std::move(parsed_manifest);
        }
      }

      if (component.manifest.schema.empty()) {
        component.manifest.schema = "hashimyei.component.manifest.v1";
      }
      if (component.manifest.canonical_path.empty()) {
        component.manifest.canonical_path =
            as_text_or_empty(&db_, kColCanonicalPath, row);
      }
      if (component.manifest.hashimyei.empty()) {
        component.manifest.hashimyei = as_text_or_empty(&db_, kColHashimyei, row);
      }
      if (component.manifest.tsi_type.empty()) {
        component.manifest.tsi_type = as_text_or_empty(&db_, kColMetricKey, row);
      }
      if (component.manifest.status.empty()) {
        component.manifest.status = as_text_or_empty(&db_, kColMetricTxt, row);
      }
      if (component.manifest.updated_at_ms == 0) {
        component.manifest.updated_at_ms = component.ts_ms;
      }
      if (component.manifest.created_at_ms == 0) {
        component.manifest.created_at_ms = component.ts_ms;
      }

      if (!component.component_id.empty()) {
        components_by_id_[component.component_id] = component;

        const auto pick_latest = [&](std::unordered_map<std::string, std::string>* map,
                                     const std::string& key) {
          if (!map || key.empty()) return;
          auto it = map->find(key);
          if (it == map->end()) {
            (*map)[key] = component.component_id;
            return;
          }
          const auto old_it = components_by_id_.find(it->second);
          if (old_it == components_by_id_.end()) {
            it->second = component.component_id;
            return;
          }
          if (component.ts_ms > old_it->second.ts_ms ||
              (component.ts_ms == old_it->second.ts_ms &&
               component.component_id > old_it->second.component_id)) {
            it->second = component.component_id;
          }
        };
        pick_latest(&latest_component_by_canonical_,
                    component.manifest.canonical_path);
        if (!component.manifest.hashimyei.empty()) {
          pick_latest(&latest_component_by_hashimyei_,
                      component.manifest.hashimyei);
        }
      }
      continue;
    }

    if (kind == "artifact") {
      artifact_entry_t e{};
      e.artifact_id = as_text_or_empty(&db_, kColRecordId, row);
      e.run_id = as_text_or_empty(&db_, kColRunId, row);
      e.canonical_path = as_text_or_empty(&db_, kColCanonicalPath, row);
      e.hashimyei = as_text_or_empty(&db_, kColHashimyei, row);
      e.schema = as_text_or_empty(&db_, kColSchema, row);
      e.artifact_sha256 = as_text_or_empty(&db_, kColArtifactSha256, row);
      e.path = as_text_or_empty(&db_, kColPath, row);
      e.payload_json = as_text_or_empty(&db_, kColPayload, row);
      (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &e.ts_ms);

      if (!e.artifact_id.empty()) {
        artifacts_by_id_[e.artifact_id] = e;
        const std::string key = join_key(e.canonical_path, e.schema);
        auto it = latest_artifact_by_key_.find(key);
        if (it == latest_artifact_by_key_.end()) {
          latest_artifact_by_key_[key] = e.artifact_id;
        } else {
          const auto old_it = artifacts_by_id_.find(it->second);
          if (old_it == artifacts_by_id_.end() || e.ts_ms >= old_it->second.ts_ms) {
            it->second = e.artifact_id;
          }
        }
      }
      continue;
    }

    if (kind == "metric_num") {
      const std::string artifact_id = as_text_or_empty(&db_, kColArtifactSha256, row);
      const std::string key = as_text_or_empty(&db_, kColMetricKey, row);
      const auto num = as_numeric_or_null(&db_, kColMetricNum, row);
      if (!artifact_id.empty() && !key.empty() && num.has_value()) {
        metrics_num_by_artifact_[artifact_id].push_back({key, *num});
      }
      continue;
    }

    if (kind == "metric_txt") {
      const std::string artifact_id = as_text_or_empty(&db_, kColArtifactSha256, row);
      const std::string key = as_text_or_empty(&db_, kColMetricKey, row);
      const std::string val = as_text_or_empty(&db_, kColMetricTxt, row);
      if (!artifact_id.empty() && !key.empty()) {
        metrics_txt_by_artifact_[artifact_id].push_back({key, val});
      }
      continue;
    }
  }
  return true;
}

bool hashimyei_catalog_store_t::get_run(std::string_view run_id, run_manifest_t* out,
                                        std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "run output pointer is null");
    return false;
  }
  const auto it = runs_by_id_.find(std::string(run_id));
  if (it == runs_by_id_.end()) {
    set_error(error, "run not found: " + std::string(run_id));
    return false;
  }
  *out = it->second;
  return true;
}

bool hashimyei_catalog_store_t::list_runs_by_binding(
    std::string_view contract_hash, std::string_view wave_hash,
    std::string_view binding_id, std::vector<run_manifest_t>* out,
    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "run list output pointer is null");
    return false;
  }
  out->clear();
  for (const auto& [_, run] : runs_by_id_) {
    if (!contract_hash.empty() && run.contract_hash != contract_hash) continue;
    if (!wave_hash.empty() && run.wave_hash != wave_hash) continue;
    if (!binding_id.empty() && run.binding_id != binding_id) continue;
    out->push_back(run);
  }
  std::sort(out->begin(), out->end(), [](const run_manifest_t& a, const run_manifest_t& b) {
    if (a.started_at_ms != b.started_at_ms) return a.started_at_ms > b.started_at_ms;
    return a.run_id < b.run_id;
  });
  return true;
}

bool hashimyei_catalog_store_t::latest_artifact(std::string_view canonical_path,
                                                std::string_view schema,
                                                artifact_entry_t* out,
                                                std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "artifact output pointer is null");
    return false;
  }
  const std::string key = join_key(canonical_path, schema);
  const auto it = latest_artifact_by_key_.find(key);
  if (it == latest_artifact_by_key_.end()) {
    set_error(error, "latest artifact not found for key: " + key);
    return false;
  }
  const auto it_art = artifacts_by_id_.find(it->second);
  if (it_art == artifacts_by_id_.end()) {
    set_error(error, "catalog inconsistency: latest artifact id missing");
    return false;
  }
  *out = it_art->second;
  return true;
}

bool hashimyei_catalog_store_t::get_artifact(std::string_view artifact_id,
                                             artifact_entry_t* out,
                                             std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "artifact output pointer is null");
    return false;
  }
  const auto it = artifacts_by_id_.find(std::string(artifact_id));
  if (it == artifacts_by_id_.end()) {
    set_error(error, "artifact not found: " + std::string(artifact_id));
    return false;
  }
  *out = it->second;
  return true;
}

bool hashimyei_catalog_store_t::artifact_metrics(
    std::string_view artifact_id,
    std::vector<std::pair<std::string, double>>* out_numeric,
    std::vector<std::pair<std::string, std::string>>* out_text,
    std::string* error) const {
  clear_error(error);
  if (!out_numeric || !out_text) {
    set_error(error, "artifact metrics output pointer is null");
    return false;
  }
  out_numeric->clear();
  out_text->clear();

  const std::string id(artifact_id);
  const auto it_num = metrics_num_by_artifact_.find(id);
  if (it_num != metrics_num_by_artifact_.end()) *out_numeric = it_num->second;
  const auto it_txt = metrics_txt_by_artifact_.find(id);
  if (it_txt != metrics_txt_by_artifact_.end()) *out_text = it_txt->second;
  return true;
}

bool hashimyei_catalog_store_t::list_artifacts(
    std::string_view canonical_path, std::string_view schema, std::size_t limit,
    std::size_t offset, bool newest_first, std::vector<artifact_entry_t>* out,
    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "artifact list output pointer is null");
    return false;
  }
  out->clear();

  const std::string cp(canonical_path);
  const std::string sc(schema);
  for (const auto& [_, art] : artifacts_by_id_) {
    if (!cp.empty() && art.canonical_path != cp) continue;
    if (!sc.empty() && art.schema != sc) continue;
    out->push_back(art);
  }

  std::sort(out->begin(), out->end(),
            [newest_first](const artifact_entry_t& a, const artifact_entry_t& b) {
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

bool hashimyei_catalog_store_t::resolve_component(
    std::string_view canonical_path, std::string_view hashimyei,
    component_state_t* out, std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "component output pointer is null");
    return false;
  }
  *out = component_state_t{};

  const std::string canonical(canonical_path);
  const std::string hash(hashimyei);

  if (!canonical.empty() && !hash.empty()) {
    component_state_t best{};
    bool found = false;
    for (const auto& [_, component] : components_by_id_) {
      if (component.manifest.canonical_path != canonical) continue;
      if (component.manifest.hashimyei != hash) continue;
      if (!found || component.ts_ms > best.ts_ms ||
          (component.ts_ms == best.ts_ms &&
           component.component_id > best.component_id)) {
        best = component;
        found = true;
      }
    }
    if (!found) {
      set_error(error, "component not found for canonical_path/hashimyei");
      return false;
    }
    *out = std::move(best);
    return true;
  }

  if (!canonical.empty()) {
    const auto it = latest_component_by_canonical_.find(canonical);
    if (it == latest_component_by_canonical_.end()) {
      set_error(error, "component not found for canonical_path: " + canonical);
      return false;
    }
    const auto it_comp = components_by_id_.find(it->second);
    if (it_comp == components_by_id_.end()) {
      set_error(error, "catalog inconsistency: latest component id missing");
      return false;
    }
    *out = it_comp->second;
    return true;
  }

  if (!hash.empty()) {
    const auto it = latest_component_by_hashimyei_.find(hash);
    if (it == latest_component_by_hashimyei_.end()) {
      set_error(error, "component not found for hashimyei: " + hash);
      return false;
    }
    const auto it_comp = components_by_id_.find(it->second);
    if (it_comp == components_by_id_.end()) {
      set_error(error, "catalog inconsistency: latest component id missing");
      return false;
    }
    *out = it_comp->second;
    return true;
  }

  set_error(error, "resolve_component requires canonical_path or hashimyei");
  return false;
}

bool hashimyei_catalog_store_t::list_components(
    std::string_view canonical_path, std::string_view hashimyei, std::size_t limit,
    std::size_t offset, bool newest_first, std::vector<component_state_t>* out,
    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "component list output pointer is null");
    return false;
  }
  out->clear();
  const std::string canonical(canonical_path);
  const std::string hash(hashimyei);

  for (const auto& [_, component] : components_by_id_) {
    if (!canonical.empty() && component.manifest.canonical_path != canonical) continue;
    if (!hash.empty() && component.manifest.hashimyei != hash) continue;
    out->push_back(component);
  }

  std::sort(out->begin(), out->end(),
            [newest_first](const component_state_t& a, const component_state_t& b) {
              if (a.ts_ms != b.ts_ms) {
                return newest_first ? (a.ts_ms > b.ts_ms) : (a.ts_ms < b.ts_ms);
              }
              return newest_first ? (a.component_id > b.component_id)
                                  : (a.component_id < b.component_id);
            });

  const std::size_t off = std::min(offset, out->size());
  std::size_t count = limit;
  if (count == 0) count = out->size() - off;
  count = std::min(count, out->size() - off);
  out->assign(out->begin() + static_cast<std::ptrdiff_t>(off),
              out->begin() + static_cast<std::ptrdiff_t>(off + count));
  return true;
}

bool hashimyei_catalog_store_t::performance_snapshot(
    std::string_view canonical_path, std::string_view run_id,
    performance_snapshot_t* out, std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "performance snapshot output pointer is null");
    return false;
  }
  *out = performance_snapshot_t{};

  const std::string cp(canonical_path);
  artifact_entry_t best{};
  bool found = false;
  for (const auto& [_, art] : artifacts_by_id_) {
    if (art.canonical_path != cp) continue;
    if (!run_id.empty() && art.run_id != run_id) continue;
    if (!found || art.ts_ms > best.ts_ms) {
      best = art;
      found = true;
    }
  }
  if (!found) {
    set_error(error, "performance snapshot artifact not found");
    return false;
  }

  out->artifact = best;
  (void)artifact_metrics(best.artifact_id, &out->numeric_metrics, &out->text_metrics,
                         nullptr);
  return true;
}

bool hashimyei_catalog_store_t::provenance_trace(
    std::string_view artifact_id, std::vector<dependency_file_t>* out,
    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "provenance output pointer is null");
    return false;
  }
  out->clear();
  const auto it_art = artifacts_by_id_.find(std::string(artifact_id));
  if (it_art == artifacts_by_id_.end()) {
    set_error(error, "artifact not found: " + std::string(artifact_id));
    return false;
  }
  const auto it = provenance_by_run_id_.find(it_art->second.run_id);
  if (it == provenance_by_run_id_.end()) return true;
  *out = it->second;
  return true;
}

}  // namespace hashimyei
}  // namespace hero
}  // namespace cuwacunu
