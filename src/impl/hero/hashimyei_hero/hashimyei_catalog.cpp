#include "hero/hashimyei_hero/hashimyei_catalog.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <csignal>
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
#include <thread>
#include <unordered_set>
#include <fcntl.h>
#include <unistd.h>

#include <openssl/evp.h>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "camahjucunu/db/idydb.h"
#include "hero/hero_catalog_schema.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"

namespace cuwacunu {
namespace hero {
namespace hashimyei {
namespace {

using runtime_lls_document_t =
    cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t;

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

[[nodiscard]] bool write_identity_kv(std::ostringstream* out, std::string_view prefix,
                                     const cuwacunu::hashimyei::hashimyei_t& id) {
  if (!out) return false;
  *out << prefix << ".schema=" << id.schema << "\n";
  *out << prefix << ".kind="
       << cuwacunu::hashimyei::hashimyei_kind_to_string(id.kind) << "\n";
  *out << prefix << ".name=" << id.name << "\n";
  *out << prefix << ".ordinal=" << id.ordinal << "\n";
  *out << prefix << ".hash_sha256_hex=" << id.hash_sha256_hex << "\n";
  return true;
}

[[nodiscard]] bool parse_identity_kv(
    const std::unordered_map<std::string, std::string>& kv, std::string_view prefix,
    bool required, cuwacunu::hashimyei::hashimyei_t* out, std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "identity output pointer is null";
    return false;
  }
  *out = cuwacunu::hashimyei::hashimyei_t{};

  const std::string prefix_s(prefix);
  const auto key = [&](std::string_view suffix) {
    return prefix_s + "." + std::string(suffix);
  };

  const auto it_name = kv.find(key("name"));
  if (it_name == kv.end() || trim_ascii(it_name->second).empty()) {
    if (!required) return true;
    if (error) *error = "missing identity field: " + key("name");
    return false;
  }

  const auto it_schema = kv.find(key("schema"));
  if (it_schema != kv.end() && !trim_ascii(it_schema->second).empty()) {
    out->schema = trim_ascii(it_schema->second);
  }
  out->name = trim_ascii(it_name->second);

  const auto it_kind = kv.find(key("kind"));
  if (it_kind == kv.end()) {
    if (error) *error = "missing identity field: " + key("kind");
    return false;
  }
  cuwacunu::hashimyei::hashimyei_kind_e parsed_kind{};
  if (!cuwacunu::hashimyei::parse_hashimyei_kind(trim_ascii(it_kind->second),
                                                  &parsed_kind)) {
    if (error) *error = "invalid identity kind for key: " + key("kind");
    return false;
  }
  out->kind = parsed_kind;

  const auto it_ordinal = kv.find(key("ordinal"));
  if (it_ordinal == kv.end()) {
    if (error) *error = "missing identity field: " + key("ordinal");
    return false;
  }
  if (!parse_u64(it_ordinal->second, &out->ordinal)) {
    if (error) *error = "invalid identity ordinal for key: " + key("ordinal");
    return false;
  }

  const auto it_hash = kv.find(key("hash_sha256_hex"));
  if (it_hash != kv.end()) out->hash_sha256_hex = trim_ascii(it_hash->second);

  std::string validate_error;
  if (!cuwacunu::hashimyei::validate_hashimyei(*out, &validate_error)) {
    if (error) *error = "identity validation failed for prefix " + prefix_s + ": " +
                        validate_error;
    return false;
  }
  return true;
}

[[nodiscard]] std::string compute_wave_contract_binding_hash(
    const cuwacunu::hero::hashimyei::wave_contract_binding_t& binding) {
  std::ostringstream seed;
  seed << binding.contract.hash_sha256_hex << "|"
       << binding.wave.hash_sha256_hex << "|" << binding.binding_alias;
  std::string digest;
  if (!sha256_hex_bytes(seed.str(), &digest)) return {};
  return digest;
}

[[nodiscard]] bool validate_wave_contract_binding(
    const cuwacunu::hero::hashimyei::wave_contract_binding_t& binding,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (binding.binding_alias.empty()) {
    if (error) *error = "binding_alias is required";
    return false;
  }
  if (binding.contract.kind != cuwacunu::hashimyei::hashimyei_kind_e::CONTRACT) {
    if (error) *error = "binding.contract kind must be CONTRACT";
    return false;
  }
  if (binding.wave.kind != cuwacunu::hashimyei::hashimyei_kind_e::WAVE) {
    if (error) *error = "binding.wave kind must be WAVE";
    return false;
  }
  if (binding.identity.kind !=
      cuwacunu::hashimyei::hashimyei_kind_e::WAVE_CONTRACT_BINDING) {
    if (error) *error = "binding.identity kind must be WAVE_CONTRACT_BINDING";
    return false;
  }
  std::string validate_error;
  if (!cuwacunu::hashimyei::validate_hashimyei(binding.contract, &validate_error)) {
    if (error) *error = "binding.contract invalid: " + validate_error;
    return false;
  }
  if (!cuwacunu::hashimyei::validate_hashimyei(binding.wave, &validate_error)) {
    if (error) *error = "binding.wave invalid: " + validate_error;
    return false;
  }
  if (!cuwacunu::hashimyei::validate_hashimyei(binding.identity, &validate_error)) {
    if (error) *error = "binding.identity invalid: " + validate_error;
    return false;
  }
  const std::string expected = compute_wave_contract_binding_hash(binding);
  if (expected.empty()) {
    if (error) *error = "cannot derive wave_contract_binding hash";
    return false;
  }
  if (binding.identity.hash_sha256_hex != expected) {
    if (error) *error = "binding.identity.hash_sha256_hex does not match canonical tuple";
    return false;
  }
  return true;
}

[[nodiscard]] bool write_binding_kv(
    std::ostringstream* out, std::string_view prefix,
    const cuwacunu::hero::hashimyei::wave_contract_binding_t& binding) {
  if (!out) return false;
  if (!write_identity_kv(out, std::string(prefix) + ".identity", binding.identity)) {
    return false;
  }
  if (!write_identity_kv(out, std::string(prefix) + ".contract", binding.contract)) {
    return false;
  }
  if (!write_identity_kv(out, std::string(prefix) + ".wave", binding.wave)) {
    return false;
  }
  *out << prefix << ".binding_alias=" << binding.binding_alias << "\n";
  return true;
}

[[nodiscard]] bool parse_binding_kv(
    const std::unordered_map<std::string, std::string>& kv, std::string_view prefix,
    cuwacunu::hero::hashimyei::wave_contract_binding_t* out, std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "binding output pointer is null";
    return false;
  }
  *out = cuwacunu::hero::hashimyei::wave_contract_binding_t{};

  const std::string prefix_s(prefix);
  const auto alias_it = kv.find(prefix_s + ".binding_alias");
  if (alias_it == kv.end()) {
    if (error) *error = "missing binding field: " + prefix_s + ".binding_alias";
    return false;
  }
  out->binding_alias = trim_ascii(alias_it->second);

  if (!parse_identity_kv(kv, prefix_s + ".identity", true, &out->identity, error)) {
    return false;
  }
  if (!parse_identity_kv(kv, prefix_s + ".contract", true, &out->contract, error)) {
    return false;
  }
  if (!parse_identity_kv(kv, prefix_s + ".wave", true, &out->wave, error)) {
    return false;
  }
  return validate_wave_contract_binding(*out, error);
}

[[nodiscard]] std::string run_manifest_payload(const run_manifest_t& m) {
  std::ostringstream out;
  out << "schema=" << m.schema << "\n";
  out << "run_id=" << m.run_id << "\n";
  out << "started_at_ms=" << m.started_at_ms << "\n";
  (void)write_identity_kv(&out, "campaign_identity", m.campaign_identity);
  (void)write_binding_kv(&out, "wave_contract_binding", m.wave_contract_binding);
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

[[nodiscard]] std::string component_manifest_payload(
    const component_manifest_t& manifest) {
  std::ostringstream out;
  out << "schema=" << manifest.schema << "\n";
  out << "canonical_path=" << manifest.canonical_path << "\n";
  out << "family=" << manifest.family << "\n";
  out << "tsi_type=" << manifest.tsi_type << "\n";
  (void)write_identity_kv(&out, "component_identity", manifest.component_identity);
  out << "parent_identity.present="
      << (manifest.parent_identity.has_value() ? "1" : "0") << "\n";
  if (manifest.parent_identity.has_value()) {
    (void)write_identity_kv(&out, "parent_identity", *manifest.parent_identity);
  }
  out << "revision_reason=" << manifest.revision_reason << "\n";
  out << "config_revision_id=" << manifest.config_revision_id << "\n";
  (void)write_binding_kv(&out, "wave_contract_binding",
                         manifest.wave_contract_binding);
  out << "dsl_canonical_path=" << manifest.dsl_canonical_path << "\n";
  out << "dsl_sha256_hex=" << manifest.dsl_sha256_hex << "\n";
  out << "status=" << manifest.status << "\n";
  out << "replaced_by=" << manifest.replaced_by << "\n";
  out << "created_at_ms=" << manifest.created_at_ms << "\n";
  out << "updated_at_ms=" << manifest.updated_at_ms << "\n";
  return out.str();
}

[[nodiscard]] std::string kind_sha_key(
    cuwacunu::hashimyei::hashimyei_kind_e kind, std::string_view sha256_hex) {
  return cuwacunu::hashimyei::hashimyei_kind_to_string(kind) + "|" +
         std::string(sha256_hex);
}

[[nodiscard]] std::string component_binding_index_payload(
    const component_manifest_t& manifest) {
  std::ostringstream out;
  out << "binding_hashimyei=" << manifest.wave_contract_binding.identity.name << "\n";
  out << "contract_hashimyei=" << manifest.wave_contract_binding.contract.name << "\n";
  out << "wave_hashimyei=" << manifest.wave_contract_binding.wave.name << "\n";
  out << "binding_alias=" << manifest.wave_contract_binding.binding_alias << "\n";
  out << "binding_hash_sha256="
      << manifest.wave_contract_binding.identity.hash_sha256_hex << "\n";
  out << "contract_hash_sha256="
      << manifest.wave_contract_binding.contract.hash_sha256_hex << "\n";
  out << "wave_hash_sha256=" << manifest.wave_contract_binding.wave.hash_sha256_hex
      << "\n";
  return out.str();
}

[[nodiscard]] bool is_known_schema(std::string_view schema) {
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
  if (schema == "piaabo.torch_compat.network_analytics.v1" ||
      schema == "piaabo.torch_compat.network_analytics.v2" ||
      schema == "piaabo.torch_compat.network_analytics.v3" ||
      schema == "piaabo.torch_compat.network_analytics.v4") {
    return true;
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

[[nodiscard]] std::string component_family_from_parts(std::string_view canonical_path,
                                                      std::string_view hashimyei,
                                                      std::string_view tsi_type) {
  if (!hashimyei.empty() && !canonical_path.empty()) {
    std::string family = std::string(canonical_path);
    const std::string suffix = "." + std::string(hashimyei);
    if (family.size() > suffix.size() &&
        family.compare(family.size() - suffix.size(), suffix.size(), suffix) ==
            0) {
      family.resize(family.size() - suffix.size());
    }
    if (!family.empty()) return family;
  }
  if (!tsi_type.empty()) return std::string(tsi_type);
  if (!canonical_path.empty()) {
    const std::string family = std::string(canonical_path);
    if (!family.empty()) {
      return family;
    }
  }
  return {};
}

[[nodiscard]] std::string component_active_pointer_key(std::string_view canonical_path,
                                                       std::string_view family,
                                                       std::string_view hashimyei) {
  std::string canonical_key_path = std::string(canonical_path);
  if (!hashimyei.empty()) {
    canonical_key_path =
        component_family_from_parts(canonical_path, hashimyei, "");
  }
  const std::string family_key =
      family.empty() ? component_family_from_parts(canonical_path, hashimyei, "")
                     : std::string(family);
  return canonical_key_path + "|" + family_key;
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

[[nodiscard]] std::optional<pid_t> parse_lock_pid(const std::filesystem::path& lock_path) {
  std::ifstream in(lock_path, std::ios::binary);
  if (!in) return std::nullopt;
  std::string line;
  if (!std::getline(in, line)) return std::nullopt;
  line = trim_ascii(line);
  constexpr std::string_view kPrefix = "pid=";
  if (line.rfind(kPrefix, 0) != 0) return std::nullopt;
  std::uint64_t parsed = 0;
  if (!parse_u64(line.substr(kPrefix.size()), &parsed)) return std::nullopt;
  if (parsed == 0) return std::nullopt;
  if (parsed > static_cast<std::uint64_t>(std::numeric_limits<pid_t>::max())) {
    return std::nullopt;
  }
  return static_cast<pid_t>(parsed);
}

[[nodiscard]] bool process_is_alive(pid_t pid) {
  if (pid <= 0) return false;
  if (::kill(pid, 0) == 0) return true;
  return errno == EPERM;
}

[[nodiscard]] bool clear_stale_ingest_lock_if_needed(
    const std::filesystem::path& lock_path) {
  const auto lock_pid = parse_lock_pid(lock_path);
  if (!lock_pid.has_value()) return false;
  if (process_is_alive(*lock_pid)) return false;
  std::error_code ec;
  const bool removed = std::filesystem::remove(lock_path, ec);
  if (ec) return false;
  if (removed) return true;
  return !std::filesystem::exists(lock_path, ec) && !ec;
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

[[nodiscard]] std::string join_key(std::string_view canonical_path,
                                   std::string_view schema) {
  return std::string(canonical_path) + "|" + std::string(schema);
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

}  // namespace

bool parse_latent_lineage_state_payload(
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

std::string compute_run_id(
    const cuwacunu::hashimyei::hashimyei_t& campaign_identity,
    const wave_contract_binding_t& wave_contract_binding,
    std::uint64_t started_at_ms) {
  std::ostringstream seed;
  seed << campaign_identity.schema << "|"
       << cuwacunu::hashimyei::hashimyei_kind_to_string(campaign_identity.kind) << "|"
       << campaign_identity.name << "|" << campaign_identity.ordinal << "|"
       << campaign_identity.hash_sha256_hex << "|"
       << wave_contract_binding.identity.schema << "|"
       << cuwacunu::hashimyei::hashimyei_kind_to_string(
              wave_contract_binding.identity.kind)
       << "|" << wave_contract_binding.identity.name << "|"
       << wave_contract_binding.identity.ordinal << "|"
       << wave_contract_binding.identity.hash_sha256_hex << "|"
       << wave_contract_binding.contract.hash_sha256_hex << "|"
       << wave_contract_binding.wave.hash_sha256_hex << "|"
       << wave_contract_binding.binding_alias << "|" << started_at_ms;
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

  if (path.filename() == "run.manifest.v1.kv") {
    set_error(error, std::string("v1 run manifest filename is not supported; use ") +
                         cuwacunu::hashimyei::kRunManifestFilenameV2);
    return false;
  }
  if (path.filename() != cuwacunu::hashimyei::kRunManifestFilenameV2) {
    set_error(error, std::string("invalid run manifest filename, expected ") +
                         cuwacunu::hashimyei::kRunManifestFilenameV2);
    return false;
  }

  std::string payload;
  std::string read_error;
  if (!read_text_file(path, &payload, &read_error)) {
    set_error(error, "cannot read run manifest: " + read_error);
    return false;
  }

  std::unordered_map<std::string, std::string> kv;
  (void)parse_latent_lineage_state_payload(payload, &kv);
  out->schema = kv.count("schema") != 0 ? trim_ascii(kv.at("schema"))
                                        : cuwacunu::hashimyei::kRunManifestSchemaV2;
  if (out->schema != cuwacunu::hashimyei::kRunManifestSchemaV2) {
    set_error(error, std::string("invalid run manifest schema; expected ") +
                         cuwacunu::hashimyei::kRunManifestSchemaV2);
    return false;
  }
  out->run_id = kv["run_id"];
  (void)parse_u64(kv["started_at_ms"], &out->started_at_ms);
  if (!parse_identity_kv(kv, "campaign_identity", true, &out->campaign_identity,
                         error)) {
    return false;
  }
  if (!parse_binding_kv(kv, "wave_contract_binding", &out->wave_contract_binding,
                        error)) {
    return false;
  }
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
  out->schema = kv.count("schema") != 0
                    ? trim_ascii(kv.at("schema"))
                    : cuwacunu::hashimyei::kComponentManifestSchemaV2;
  out->canonical_path = kv.count("canonical_path") != 0
                            ? kv.at("canonical_path")
                            : kv.count("canonical_base") != 0 ? kv.at("canonical_base")
                                                               : std::string{};
  out->family = kv.count("family") != 0 ? kv.at("family") : std::string{};
  out->tsi_type = kv.count("tsi_type") != 0
                      ? kv.at("tsi_type")
                      : kv.count("canonical_type") != 0 ? kv.at("canonical_type")
                                                        : std::string{};
  if (!parse_identity_kv(kv, "component_identity", true, &out->component_identity,
                         error)) {
    return false;
  }
  bool parent_present = false;
  if (const auto it = kv.find("parent_identity.present"); it != kv.end()) {
    const std::string token = lowercase_copy(trim_ascii(it->second));
    parent_present = token == "1" || token == "true" || token == "yes";
  } else {
    parent_present = kv.count("parent_identity.name") != 0;
  }
  if (parent_present) {
    cuwacunu::hashimyei::hashimyei_t parent{};
    if (!parse_identity_kv(kv, "parent_identity", true, &parent, error)) return false;
    out->parent_identity = parent;
  }
  out->revision_reason = kv.count("revision_reason") != 0
                             ? kv.at("revision_reason")
                             : (out->parent_identity.has_value() ? "dsl_change"
                                                                 : "initial");
  out->config_revision_id = kv.count("config_revision_id") != 0
                                ? kv.at("config_revision_id")
                                : std::string{};
  if (!parse_binding_kv(kv, "wave_contract_binding", &out->wave_contract_binding,
                        error)) {
    return false;
  }
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
  if (out->family.empty()) {
    out->family = component_family_from_parts(out->canonical_path,
                                              out->component_identity.name,
                                              out->tsi_type);
  }

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

  if (manifest.schema != cuwacunu::hashimyei::kComponentManifestSchemaV2) {
    set_error(error, "invalid component manifest schema");
    return false;
  }
  if (manifest.canonical_path.empty() ||
      !starts_with(manifest.canonical_path, "tsi.")) {
    set_error(error, "component manifest missing canonical_path");
    return false;
  }
  if (manifest.family.empty() || !starts_with(manifest.family, "tsi.")) {
    set_error(error, "component manifest missing family");
    return false;
  }
  if (manifest.tsi_type.empty() || !starts_with(manifest.tsi_type, "tsi.")) {
    set_error(error, "component manifest missing tsi_type");
    return false;
  }
  std::string identity_error;
  if (!cuwacunu::hashimyei::validate_hashimyei(manifest.component_identity,
                                                &identity_error)) {
    set_error(error, "component manifest component_identity invalid: " + identity_error);
    return false;
  }
  if (manifest.component_identity.kind !=
      cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE) {
    set_error(error, "component manifest component_identity.kind must be TSIEMENE");
    return false;
  }
  if (manifest.parent_identity.has_value()) {
    if (!cuwacunu::hashimyei::validate_hashimyei(*manifest.parent_identity,
                                                  &identity_error)) {
      set_error(error, "component manifest parent_identity invalid: " + identity_error);
      return false;
    }
    if (manifest.parent_identity->kind !=
        cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE) {
      set_error(error, "component manifest parent_identity.kind must be TSIEMENE");
      return false;
    }
  }
  if (trim_ascii(manifest.revision_reason).empty()) {
    set_error(error, "component manifest missing revision_reason");
    return false;
  }
  if (!validate_wave_contract_binding(manifest.wave_contract_binding, &identity_error)) {
    set_error(error,
              "component manifest wave_contract_binding invalid: " + identity_error);
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

  if (path.filename() == "component.manifest.v1.kv") {
    set_error(error, std::string("v1 component manifest filename is not supported; use ") +
                         cuwacunu::hashimyei::kComponentManifestFilenameV2);
    return false;
  }
  if (path.filename() != cuwacunu::hashimyei::kComponentManifestFilenameV2) {
    set_error(error, std::string("invalid component manifest filename, expected ") +
                         cuwacunu::hashimyei::kComponentManifestFilenameV2);
    return false;
  }

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
  (void)parse_latent_lineage_state_payload(payload, &kv);
  if (!parse_component_manifest_kv(kv, out, error)) return false;
  return true;
}

std::string compute_component_manifest_id(const component_manifest_t& manifest) {
  std::ostringstream seed;
  seed << manifest.canonical_path << "|" << manifest.family << "|"
       << manifest.tsi_type << "|"
       << manifest.component_identity.schema << "|"
       << cuwacunu::hashimyei::hashimyei_kind_to_string(
              manifest.component_identity.kind)
       << "|" << manifest.component_identity.name << "|"
       << manifest.component_identity.ordinal << "|"
       << manifest.component_identity.hash_sha256_hex << "|";
  if (manifest.parent_identity.has_value()) {
    seed << manifest.parent_identity->schema << "|"
         << cuwacunu::hashimyei::hashimyei_kind_to_string(
                manifest.parent_identity->kind)
         << "|" << manifest.parent_identity->name << "|"
         << manifest.parent_identity->ordinal << "|"
         << manifest.parent_identity->hash_sha256_hex << "|";
  }
  seed << manifest.revision_reason << "|" << manifest.config_revision_id << "|"
       << manifest.wave_contract_binding.identity.hash_sha256_hex << "|"
       << manifest.wave_contract_binding.contract.hash_sha256_hex << "|"
       << manifest.wave_contract_binding.wave.hash_sha256_hex << "|"
       << manifest.wave_contract_binding.binding_alias << "|"
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
  if (manifest.schema != cuwacunu::hashimyei::kRunManifestSchemaV2) {
    set_error(error, std::string("run manifest schema must be ") +
                         cuwacunu::hashimyei::kRunManifestSchemaV2);
    return false;
  }
  if (manifest.run_id.empty()) {
    set_error(error, "run manifest requires run_id");
    return false;
  }
  std::string identity_error;
  if (!cuwacunu::hashimyei::validate_hashimyei(manifest.campaign_identity,
                                                &identity_error)) {
    set_error(error, "run manifest campaign_identity invalid: " + identity_error);
    return false;
  }
  if (!validate_wave_contract_binding(manifest.wave_contract_binding, &identity_error)) {
    set_error(error, "run manifest wave_contract_binding invalid: " + identity_error);
    return false;
  }

  std::error_code ec;
  const auto dir = store_root / "runs" / manifest.run_id;
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    set_error(error, "cannot create run manifest directory: " + dir.string());
    return false;
  }

  const auto target = dir / cuwacunu::hashimyei::kRunManifestFilenameV2;
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
  for (int attempt = 0; attempt <= kCatalogOpenBusyRetryCount; ++attempt) {
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
    set_error(error, "cannot open catalog: " + detail);
    if (db_) {
      (void)idydb_close(&db_);
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
  report_fragments_by_id_.clear();
  latest_report_fragment_by_key_.clear();
  metrics_num_by_report_fragment_.clear();
  metrics_txt_by_report_fragment_.clear();
  components_by_id_.clear();
  latest_component_by_canonical_.clear();
  latest_component_by_hashimyei_.clear();
  active_hashimyei_by_key_.clear();
  active_component_by_key_.clear();
  active_component_by_canonical_.clear();
  dependency_files_by_run_id_.clear();
  kind_counters_.clear();
  hash_identity_by_kind_sha_.clear();
  component_ids_by_binding_hashimyei_.clear();
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
      const std::string version = trim_ascii(as_text_or_empty(&db_, kColMetricTxt, row));
      if (version != "2") {
        set_error(error, "unsupported catalog schema version '" + version +
                             "'; expected strict v2");
        return false;
      }
      return true;
    }
  }

  return append_row_(cuwacunu::hero::schema::kRecordKindHEADER,
                     "catalog_schema_version", "", "", "",
                     cuwacunu::hashimyei::kCatalogSchemaV2, "catalog_schema_version",
                     std::numeric_limits<double>::quiet_NaN(), "2", "",
                     options_.catalog_path.string(),
                     std::to_string(now_ms_utc()), "{}", error);
}

bool hashimyei_catalog_store_t::append_row_(
    std::string_view record_kind, std::string_view record_id, std::string_view run_id,
    std::string_view canonical_path, std::string_view hashimyei, std::string_view schema,
    std::string_view metric_key, double metric_num, std::string_view metric_txt,
    std::string_view report_fragment_sha256, std::string_view path, std::string_view ts_ms,
    std::string_view payload_json, std::string* error) {
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
  if (!insert_text(&db_, kColRunId, row, run_id, error)) return false;
  if (!insert_text(&db_, kColCanonicalPath, row, canonical_path, error)) return false;
  if (!insert_text(&db_, kColHashimyei, row, hashimyei, error)) return false;
  if (!insert_text(&db_, kColSchema, row, schema, error)) return false;
  if (!insert_text(&db_, kColMetricKey, row, metric_key, error)) return false;
  if (!insert_num(&db_, kColMetricNum, row, metric_num, error)) return false;
  if (!insert_text(&db_, kColMetricTxt, row, metric_txt, error)) return false;
  if (!insert_text(&db_, kColReportFragmentSha256, row, report_fragment_sha256, error)) return false;
  if (!insert_text(&db_, kColPath, row, path, error)) return false;
  if (!insert_text(&db_, kColTsMs, row, ts_ms, error)) return false;
  if (!insert_text(&db_, kColPayload, row, payload_json, error)) return false;
  return true;
}

bool hashimyei_catalog_store_t::ledger_contains_(std::string_view report_fragment_sha256,
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
  if (report_fragment_sha256.empty()) return true;

  idydb_filter_term t_kind{};
  t_kind.column = kColRecordKind;
  t_kind.type = IDYDB_CHAR;
  t_kind.op = IDYDB_FILTER_OP_EQ;
  t_kind.value.s = cuwacunu::hero::schema::kRecordKindLEDGER;
  idydb_filter_term t_sha{};
  t_sha.column = kColReportFragmentSha256;
  t_sha.type = IDYDB_CHAR;
  t_sha.op = IDYDB_FILTER_OP_EQ;
  const std::string sha(report_fragment_sha256);
  t_sha.value.s = sha.c_str();
  return db::query::exists_row(&db_, {t_kind, t_sha}, out_exists, error);
}

bool hashimyei_catalog_store_t::append_ledger_(std::string_view report_fragment_sha256,
                                               std::string_view path,
                                               std::string* error) {
  return append_row_(cuwacunu::hero::schema::kRecordKindLEDGER,
                     std::string(report_fragment_sha256), "", "", "", "",
                     "ingest_version", static_cast<double>(options_.ingest_version),
                     "", report_fragment_sha256, path, std::to_string(now_ms_utc()), "{}",
                     error);
}

bool hashimyei_catalog_store_t::append_kind_counter_(
    cuwacunu::hashimyei::hashimyei_kind_e kind, std::uint64_t next_value,
    std::string* error) {
  return append_row_(
      cuwacunu::hero::schema::kRecordKindCOUNTER,
      cuwacunu::hashimyei::hashimyei_kind_to_string(kind), "", "",
      "", cuwacunu::hashimyei::kCounterSchemaV2, "next_ordinal",
      static_cast<double>(next_value), std::to_string(next_value), "", "",
      std::to_string(now_ms_utc()), "{}", error);
}

bool hashimyei_catalog_store_t::reserve_next_ordinal_(
    cuwacunu::hashimyei::hashimyei_kind_e kind, std::uint64_t* out_ordinal,
    std::string* error) {
  clear_error(error);
  if (!out_ordinal) {
    set_error(error, "out_ordinal is null");
    return false;
  }
  const int key = static_cast<int>(kind);
  const auto it = kind_counters_.find(key);
  const std::uint64_t next = (it == kind_counters_.end()) ? 0 : it->second;
  const std::uint64_t next_value = next + 1;
  *out_ordinal = next;
  kind_counters_[key] = next_value;
  if (!append_kind_counter_(kind, next_value, error)) return false;
  return true;
}

bool hashimyei_catalog_store_t::ensure_identity_allocated_(
    cuwacunu::hashimyei::hashimyei_t* identity, std::string* error) {
  clear_error(error);
  if (!identity) {
    set_error(error, "identity pointer is null");
    return false;
  }
  if (identity->schema.empty()) identity->schema = cuwacunu::hashimyei::kIdentitySchemaV2;
  const bool requires_sha = cuwacunu::hashimyei::hashimyei_kind_requires_sha256(identity->kind);

  if (requires_sha && identity->hash_sha256_hex.empty()) {
    set_error(error, "identity.hash_sha256_hex is required for this identity kind");
    return false;
  }

  const std::string identity_key =
      requires_sha ? kind_sha_key(identity->kind, identity->hash_sha256_hex)
                   : std::string{};
  if (!identity_key.empty()) {
    const auto it_known = hash_identity_by_kind_sha_.find(identity_key);
    if (it_known != hash_identity_by_kind_sha_.end()) {
      const auto& known = it_known->second;
      if (identity->name.empty()) identity->name = known.name;
      if (identity->ordinal == 0) identity->ordinal = known.ordinal;
      if (!identity->name.empty() && identity->name != known.name) {
        set_error(error, "identity.name conflicts with existing hashimyei alias for kind/hash");
        return false;
      }
      if (identity->ordinal != 0 && identity->ordinal != known.ordinal) {
        set_error(error, "identity.ordinal conflicts with existing hashimyei alias for kind/hash");
        return false;
      }
    }
  }

  if (identity->name.empty()) {
    std::uint64_t ordinal = identity->ordinal;
    if (ordinal == 0) {
      if (!reserve_next_ordinal_(identity->kind, &ordinal, error)) return false;
    }
    identity->ordinal = ordinal;
    identity->name = cuwacunu::hashimyei::make_hex_hash_name(ordinal);
  } else {
    std::uint64_t parsed = 0;
    if (!cuwacunu::hashimyei::parse_hex_hash_name_ordinal(identity->name, &parsed)) {
      set_error(error, "identity.name is not a valid lowercase 0x... hex ordinal");
      return false;
    }
    if (identity->ordinal == 0) identity->ordinal = parsed;
    if (identity->ordinal != parsed) {
      set_error(error, "identity.ordinal does not match identity.name");
      return false;
    }
  }

  std::string validate_error;
  if (!cuwacunu::hashimyei::validate_hashimyei(*identity, &validate_error)) {
    set_error(error, "identity validation failed: " + validate_error);
    return false;
  }

  if (!identity_key.empty()) {
    const auto it_known = hash_identity_by_kind_sha_.find(identity_key);
    if (it_known == hash_identity_by_kind_sha_.end()) {
      hash_identity_by_kind_sha_.emplace(identity_key, *identity);
    } else if (it_known->second.name != identity->name ||
               it_known->second.ordinal != identity->ordinal) {
      set_error(error, "identity alias collision for kind/hash");
      return false;
    }
  }

  const int key = static_cast<int>(identity->kind);
  const std::uint64_t observed_next = identity->ordinal + 1;
  const auto it = kind_counters_.find(key);
  if (it == kind_counters_.end() || observed_next > it->second) {
    kind_counters_[key] = observed_next;
    if (!append_kind_counter_(identity->kind, observed_next, error)) return false;
  }
  return true;
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
  t_kind.value.s = cuwacunu::hero::schema::kRecordKindRUN;
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
    if (!append_row_(cuwacunu::hero::schema::kRecordKindRUN, m.run_id, m.run_id, "", "", m.schema, "",
                     std::numeric_limits<double>::quiet_NaN(),
                     m.wave_contract_binding.identity.name,
                     manifest_sha, cp.string(), std::to_string(m.started_at_ms),
                     run_manifest_payload(m), error)) {
      return false;
    }
    for (const auto& d : m.dependency_files) {
      const std::string rec_id = m.run_id + "|" + d.canonical_path;
      if (!append_row_(cuwacunu::hero::schema::kRecordKindRUN_DEPENDENCY, rec_id, m.run_id, "", "", m.schema,
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

  std::string report_fragment_sha;
  if (!sha256_hex_file(path, &report_fragment_sha, error)) return false;
  bool already = false;
  if (!ledger_contains_(report_fragment_sha, &already, error)) return false;
  if (already) return true;

  std::filesystem::path cp = path;
  if (const auto can = canonicalized(path); can.has_value()) cp = *can;

  std::uint64_t ts_ms = manifest.updated_at_ms != 0 ? manifest.updated_at_ms
                                                     : manifest.created_at_ms;
  if (ts_ms == 0) ts_ms = now_ms_utc();

  const std::string component_id = compute_component_manifest_id(manifest);
  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT, component_id, "", manifest.canonical_path,
                   manifest.component_identity.name, manifest.schema,
                   manifest.tsi_type,
                   std::numeric_limits<double>::quiet_NaN(), manifest.status,
                   report_fragment_sha, cp.string(), std::to_string(ts_ms), payload,
                   error)) {
    return false;
  }

  const std::string prov_id = component_id + "|dsl";
  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_DEPENDENCY, prov_id, "", manifest.canonical_path,
                   manifest.component_identity.name, manifest.schema,
                   manifest.dsl_canonical_path,
                   std::numeric_limits<double>::quiet_NaN(),
                   manifest.dsl_sha256_hex, "", cp.string(),
                   std::to_string(ts_ms), "{}", error)) {
    return false;
  }

  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_REVISION, component_id + "|revision", "",
                   manifest.canonical_path, manifest.component_identity.name,
                   manifest.schema,
                   manifest.family, std::numeric_limits<double>::quiet_NaN(),
                   manifest.parent_identity.has_value()
                       ? std::string_view(manifest.parent_identity->name)
                       : std::string_view{},
                   "", cp.string(),
                   std::to_string(ts_ms), manifest.revision_reason, error)) {
    return false;
  }

  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_BINDING, component_id + "|binding", "",
                   manifest.canonical_path, manifest.component_identity.name,
                   manifest.schema, manifest.wave_contract_binding.identity.name,
                   std::numeric_limits<double>::quiet_NaN(),
                   manifest.wave_contract_binding.contract.name + "|" +
                       manifest.wave_contract_binding.wave.name,
                   "", cp.string(), std::to_string(ts_ms),
                   component_binding_index_payload(manifest), error)) {
    return false;
  }

  if (trim_ascii(manifest.status) == "active") {
    const std::string pointer_key = component_active_pointer_key(
        manifest.canonical_path, manifest.family, manifest.component_identity.name);
    std::ostringstream pointer_payload;
    pointer_payload << "canonical_path=" << manifest.canonical_path << "\n";
    pointer_payload << "family=" << manifest.family << "\n";
    pointer_payload << "active_hashimyei=" << manifest.component_identity.name
                    << "\n";
    pointer_payload << "component_id=" << component_id << "\n";
    pointer_payload << "parent_hashimyei="
                    << (manifest.parent_identity.has_value()
                            ? std::string_view(manifest.parent_identity->name)
                            : std::string_view{})
                    << "\n";
    pointer_payload << "revision_reason=" << manifest.revision_reason << "\n";
    pointer_payload << "config_revision_id=" << manifest.config_revision_id << "\n";
    if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_ACTIVE, pointer_key, "", manifest.canonical_path,
                     manifest.component_identity.name,
                     cuwacunu::hashimyei::kComponentActivePointerSchemaV2,
                     manifest.family, std::numeric_limits<double>::quiet_NaN(),
                     manifest.config_revision_id, "", cp.string(),
                     std::to_string(ts_ms), pointer_payload.str(), error)) {
      return false;
    }
  }

  return append_ledger_(report_fragment_sha, cp.string(), error);
}

bool hashimyei_catalog_store_t::parse_and_append_metrics_(
    std::string_view report_fragment_id,
    const std::unordered_map<std::string, std::string>& kv, std::string* error) {
  clear_error(error);
  for (const auto& [k, v] : kv) {
    if (k == "schema" || k == "run_id" || k == "canonical_path" ||
        k == "hashimyei" || k == "contract_hash" || k == "wave_hash" ||
        k == "binding_id" || k == "report_kind" || k == "tsi_type" ||
        k == "component_instance_name" || k == "report_event" ||
        k == "source_label") {
      continue;
    }

    double num = 0.0;
    if (parse_double_token(v, &num)) {
      const std::string rec_id = std::string(report_fragment_id) + "|num|" + k;
      if (!append_row_(cuwacunu::hero::schema::kRecordKindMETRIC_NUM, rec_id, "", "", "", "", k, num, "",
                       report_fragment_id, "", std::to_string(now_ms_utc()), "{}", error)) {
        return false;
      }
    } else {
      const std::string rec_id = std::string(report_fragment_id) + "|txt|" + k;
      if (!append_row_(cuwacunu::hero::schema::kRecordKindMETRIC_TXT, rec_id, "", "", "", "", k,
                       std::numeric_limits<double>::quiet_NaN(), v, report_fragment_id, "",
                       std::to_string(now_ms_utc()), "{}", error)) {
        return false;
      }
    }
  }
  return true;
}

bool hashimyei_catalog_store_t::ingest_report_fragment_file_(
    const std::filesystem::path& path, std::string* error) {
  clear_error(error);
  if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
    return true;
  }
  if (path.extension() != ".lls") return true;

  std::string payload;
  if (!read_text_file(path, &payload, nullptr)) {
    return true;
  }
  if (payload.empty()) return true;

  std::unordered_map<std::string, std::string> kv;
  std::string parse_error;
  if (!parse_runtime_lls_payload(payload, &kv, nullptr, &parse_error)) {
    set_error(error, "runtime .lls parse failure for " + path.string() + ": " +
                         parse_error);
    return false;
  }
  const std::string schema = kv["schema"];
  if (!is_known_schema(schema)) return true;

  std::string report_fragment_sha;
  if (!sha256_hex_file(path, &report_fragment_sha, error)) return false;
  bool already = false;
  if (!ledger_contains_(report_fragment_sha, &already, error)) return false;
  if (already) return true;

  std::string canonical_path = kv["canonical_path"];
  if (canonical_path.empty()) canonical_path = kv["source_label"];
  if (canonical_path.empty()) canonical_path = canonical_path_from_report_fragment_path(path);

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
    set_error(error, "runtime report_fragment missing run_id: " + path.string());
    return false;
  }

  std::filesystem::path cp = path;
  if (const auto can = canonicalized(path); can.has_value()) cp = *can;

  if (!append_row_(cuwacunu::hero::schema::kRecordKindREPORT_FRAGMENT, report_fragment_sha, run_id, canonical_path, hashimyei, schema,
                   "", std::numeric_limits<double>::quiet_NaN(), "", report_fragment_sha,
                   cp.string(), std::to_string(ts_ms), payload, error)) {
    return false;
  }
  if (!parse_and_append_metrics_(report_fragment_sha, kv, error)) return false;
  return append_ledger_(report_fragment_sha, cp.string(), error);
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
  int fd = -1;
  for (int attempt = 0; attempt < 2; ++attempt) {
    fd = ::open(lock_path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
    if (fd >= 0) break;
    if (errno != EEXIST || attempt != 0 ||
        !clear_stale_ingest_lock_if_needed(lock_path)) {
      break;
    }
  }
  if (fd < 0) {
    if (errno == EEXIST) {
      set_error(error, "ingest lock already held: " + lock_path.string());
    } else {
      set_error(error, "cannot create ingest lock file: " + lock_path.string() +
                           " (" + std::strerror(errno) + ")");
    }
    return false;
  }
  const std::string payload = "pid=" + std::to_string(::getpid()) + "\n";
  const ssize_t wrote = ::write(fd, payload.data(), payload.size());
  if (wrote < 0 || static_cast<std::size_t>(wrote) != payload.size()) {
    const int saved_errno = errno;
    (void)::close(fd);
    std::error_code rm_ec;
    std::filesystem::remove(lock_path, rm_ec);
    set_error(error, "cannot initialize ingest lock file: " + lock_path.string() +
                         " (" + std::strerror(saved_errno) + ")");
    return false;
  }
  if (::close(fd) != 0) {
    const int saved_errno = errno;
    std::error_code rm_ec;
    std::filesystem::remove(lock_path, rm_ec);
    set_error(error, "cannot close ingest lock file: " + lock_path.string() +
                         " (" + std::strerror(saved_errno) + ")");
    return false;
  }
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

bool hashimyei_catalog_store_t::ingest_filesystem(
    const std::filesystem::path& store_root, std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  ingest_lock_t lock{};
  if (!acquire_ingest_lock_(store_root, &lock, error)) return false;
  const auto release_lock = [&]() { release_ingest_lock_(&lock); };
  const std::filesystem::path canonical_store_root =
      canonicalized(store_root).value_or(store_root.lexically_normal());

  std::error_code ec;
  for (std::filesystem::recursive_directory_iterator it(store_root, ec), end;
       it != end; it.increment(ec)) {
    if (ec) {
      release_lock();
      set_error(error, "failed scanning store root");
      return false;
    }
    std::error_code entry_ec;
    const auto symlink_state = it->symlink_status(entry_ec);
    if (entry_ec) {
      release_lock();
      set_error(error, "failed reading store entry status");
      return false;
    }
    if (std::filesystem::is_symlink(symlink_state)) continue;
    if (!it->is_regular_file(entry_ec)) {
      if (entry_ec) {
        release_lock();
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
    if (p.filename() == "run.manifest.v1.kv") {
      release_lock();
      set_error(error,
                "unsupported v1 run manifest filename run.manifest.v1.kv");
      return false;
    }
    if (p.filename() == "component.manifest.v1.kv") {
      release_lock();
      set_error(
          error,
          "unsupported v1 component manifest filename component.manifest.v1.kv");
      return false;
    }
    if (p.filename() == cuwacunu::hashimyei::kRunManifestFilenameV2) {
      if (!ingest_run_manifest_file_(p, error)) {
        release_lock();
        return false;
      }
      continue;
    }
    if (p.filename() == cuwacunu::hashimyei::kComponentManifestFilenameV2) {
      if (!ingest_component_manifest_file_(p, error)) {
        release_lock();
        return false;
      }
      continue;
    }
    if (!ingest_report_fragment_file_(p, error)) {
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
  report_fragments_by_id_.clear();
  latest_report_fragment_by_key_.clear();
  metrics_num_by_report_fragment_.clear();
  metrics_txt_by_report_fragment_.clear();
  components_by_id_.clear();
  latest_component_by_canonical_.clear();
  latest_component_by_hashimyei_.clear();
  active_hashimyei_by_key_.clear();
  active_component_by_key_.clear();
  active_component_by_canonical_.clear();
  dependency_files_by_run_id_.clear();
  kind_counters_.clear();
  hash_identity_by_kind_sha_.clear();
  component_ids_by_binding_hashimyei_.clear();

  const auto bump_counter_from_identity =
      [&](const cuwacunu::hashimyei::hashimyei_t& identity) {
        const int key = static_cast<int>(identity.kind);
        const std::uint64_t next = identity.ordinal + 1;
        auto it = kind_counters_.find(key);
        if (it == kind_counters_.end() || next > it->second) {
          kind_counters_[key] = next;
        }
      };
  const auto index_hash_identity =
      [&](const cuwacunu::hashimyei::hashimyei_t& identity) {
        if (!cuwacunu::hashimyei::hashimyei_kind_requires_sha256(identity.kind)) return;
        if (identity.name.empty() || identity.hash_sha256_hex.empty()) return;
        const std::string key = kind_sha_key(identity.kind, identity.hash_sha256_hex);
        const auto it = hash_identity_by_kind_sha_.find(key);
        if (it == hash_identity_by_kind_sha_.end()) {
          hash_identity_by_kind_sha_.emplace(key, identity);
          return;
        }
        // Keep the smallest ordinal when conflicting aliases are observed.
        if (identity.ordinal < it->second.ordinal) {
          it->second = identity;
        }
      };

  const idydb_column_row_sizing next = idydb_column_next_row(&db_, kColRecordKind);
  for (idydb_column_row_sizing row = 1; row < next; ++row) {
    const std::string kind = as_text_or_empty(&db_, kColRecordKind, row);
    if (kind.empty()) continue;

    if (kind == cuwacunu::hero::schema::kRecordKindCOUNTER) {
      const std::string kind_txt = as_text_or_empty(&db_, kColRecordId, row);
      cuwacunu::hashimyei::hashimyei_kind_e parsed_kind{};
      if (!cuwacunu::hashimyei::parse_hashimyei_kind(kind_txt, &parsed_kind)) {
        continue;
      }
      std::uint64_t next_ordinal = 0;
      if (!parse_u64(as_text_or_empty(&db_, kColMetricTxt, row), &next_ordinal)) {
        const auto metric_num = as_numeric_or_null(&db_, kColMetricNum, row);
        if (metric_num.has_value() && *metric_num >= 0.0) {
          next_ordinal = static_cast<std::uint64_t>(*metric_num);
        }
      }
      kind_counters_[static_cast<int>(parsed_kind)] = next_ordinal;
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindRUN) {
      run_manifest_t run{};
      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!payload.empty()) {
        std::unordered_map<std::string, std::string> kv;
        (void)parse_latent_lineage_state_payload(payload, &kv);
        run.schema = kv.count("schema") != 0 ? trim_ascii(kv.at("schema")) : std::string{};
        run.run_id = kv["run_id"];
        (void)parse_u64(kv["started_at_ms"], &run.started_at_ms);
        std::string parse_error;
        if (!parse_identity_kv(kv, "campaign_identity", false,
                               &run.campaign_identity,
                               &parse_error)) {
          run.campaign_identity = cuwacunu::hashimyei::hashimyei_t{};
        }
        if (!parse_binding_kv(kv, "wave_contract_binding", &run.wave_contract_binding,
                              &parse_error)) {
          run.wave_contract_binding = wave_contract_binding_t{};
        }
        run.sampler = kv["sampler"];
        run.record_type = kv["record_type"];
        run.seed = kv["seed"];
        run.device = kv["device"];
        run.dtype = kv["dtype"];

        std::uint64_t dep_count = 0;
        if (parse_u64(kv["dep_count"], &dep_count)) {
          run.dependency_files.reserve(static_cast<std::size_t>(dep_count));
          for (std::uint64_t i = 0; i < dep_count; ++i) {
            std::ostringstream pfx;
            pfx << "dep_" << std::setw(4) << std::setfill('0') << i << "_";
            dependency_file_t d{};
            d.canonical_path = kv[pfx.str() + "path"];
            d.sha256_hex = kv[pfx.str() + "sha256"];
            if (!d.canonical_path.empty() || !d.sha256_hex.empty()) {
              run.dependency_files.push_back(std::move(d));
            }
          }
        }

        std::uint64_t component_count = 0;
        if (parse_u64(kv["component_count"], &component_count)) {
          run.components.reserve(static_cast<std::size_t>(component_count));
          for (std::uint64_t i = 0; i < component_count; ++i) {
            std::ostringstream pfx;
            pfx << "component_" << std::setw(4) << std::setfill('0') << i << "_";
            component_instance_t c{};
            c.canonical_path = kv[pfx.str() + "canonical_path"];
            c.tsi_type = kv[pfx.str() + "tsi_type"];
            c.hashimyei = kv[pfx.str() + "hashimyei"];
            if (!c.canonical_path.empty() || !c.tsi_type.empty() ||
                !c.hashimyei.empty()) {
              run.components.push_back(std::move(c));
            }
          }
        }
      }
      if (run.run_id.empty()) run.run_id = as_text_or_empty(&db_, kColRunId, row);
      if (run.schema.empty()) run.schema = as_text_or_empty(&db_, kColSchema, row);
      if (run.started_at_ms == 0) {
        std::uint64_t parsed = 0;
        if (parse_u64(as_text_or_empty(&db_, kColTsMs, row), &parsed)) run.started_at_ms = parsed;
      }
      if (!run.run_id.empty()) {
        bump_counter_from_identity(run.campaign_identity);
        bump_counter_from_identity(run.wave_contract_binding.identity);
        bump_counter_from_identity(run.wave_contract_binding.contract);
        bump_counter_from_identity(run.wave_contract_binding.wave);
        index_hash_identity(run.wave_contract_binding.identity);
        index_hash_identity(run.wave_contract_binding.contract);
        index_hash_identity(run.wave_contract_binding.wave);
        runs_by_id_[run.run_id] = std::move(run);
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindRUN_DEPENDENCY) {
      const std::string run_id = as_text_or_empty(&db_, kColRunId, row);
      const std::string dep_path = as_text_or_empty(&db_, kColMetricKey, row);
      const std::string dep_sha = as_text_or_empty(&db_, kColMetricTxt, row);
      if (!run_id.empty() && !dep_path.empty()) {
        dependency_files_by_run_id_[run_id].push_back({dep_path, dep_sha});
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindCOMPONENT) {
      component_state_t component{};
      component.component_id = as_text_or_empty(&db_, kColRecordId, row);
      component.manifest_path = as_text_or_empty(&db_, kColPath, row);
      component.report_fragment_sha256 = as_text_or_empty(&db_, kColReportFragmentSha256, row);
      (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &component.ts_ms);

      component.manifest.schema = as_text_or_empty(&db_, kColSchema, row);
      component.manifest.canonical_path =
          as_text_or_empty(&db_, kColCanonicalPath, row);
      const std::string hashimyei_name = as_text_or_empty(&db_, kColHashimyei, row);
      std::uint64_t parsed_ordinal = 0;
      if (cuwacunu::hashimyei::parse_hex_hash_name_ordinal(hashimyei_name,
                                                            &parsed_ordinal)) {
        component.manifest.component_identity.schema = cuwacunu::hashimyei::kIdentitySchemaV2;
        component.manifest.component_identity.kind =
            cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
        component.manifest.component_identity.name = hashimyei_name;
        component.manifest.component_identity.ordinal = parsed_ordinal;
      }
      component.manifest.family = component_family_from_parts(
          component.manifest.canonical_path, component.manifest.component_identity.name,
          as_text_or_empty(&db_, kColMetricKey, row));
      component.manifest.tsi_type = as_text_or_empty(&db_, kColMetricKey, row);
      component.manifest.status = as_text_or_empty(&db_, kColMetricTxt, row);

      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!payload.empty()) {
        std::unordered_map<std::string, std::string> kv;
        (void)parse_latent_lineage_state_payload(payload, &kv);
        component_manifest_t parsed_manifest{};
        std::string ignored_error;
        if (parse_component_manifest_kv(kv, &parsed_manifest, &ignored_error)) {
          component.manifest = std::move(parsed_manifest);
        }
      }

      if (component.manifest.schema.empty()) {
        component.manifest.schema = cuwacunu::hashimyei::kComponentManifestSchemaV2;
      }
      if (component.manifest.canonical_path.empty()) {
        component.manifest.canonical_path =
            as_text_or_empty(&db_, kColCanonicalPath, row);
      }
      if (component.manifest.component_identity.name.empty()) {
        const std::string fallback_name = as_text_or_empty(&db_, kColHashimyei, row);
        if (cuwacunu::hashimyei::parse_hex_hash_name_ordinal(fallback_name,
                                                              &parsed_ordinal)) {
          component.manifest.component_identity.schema = cuwacunu::hashimyei::kIdentitySchemaV2;
          component.manifest.component_identity.kind =
              cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
          component.manifest.component_identity.name = fallback_name;
          component.manifest.component_identity.ordinal = parsed_ordinal;
        }
      }
      if (component.manifest.tsi_type.empty()) {
        component.manifest.tsi_type = as_text_or_empty(&db_, kColMetricKey, row);
      }
      if (component.manifest.family.empty()) {
        component.manifest.family = component_family_from_parts(
            component.manifest.canonical_path,
            component.manifest.component_identity.name,
            component.manifest.tsi_type);
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
        bump_counter_from_identity(component.manifest.component_identity);
        if (component.manifest.parent_identity.has_value()) {
          bump_counter_from_identity(*component.manifest.parent_identity);
        }
        bump_counter_from_identity(component.manifest.wave_contract_binding.identity);
        bump_counter_from_identity(component.manifest.wave_contract_binding.contract);
        bump_counter_from_identity(component.manifest.wave_contract_binding.wave);
        index_hash_identity(component.manifest.wave_contract_binding.identity);
        index_hash_identity(component.manifest.wave_contract_binding.contract);
        index_hash_identity(component.manifest.wave_contract_binding.wave);
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
        if (!component.manifest.component_identity.name.empty()) {
          pick_latest(&latest_component_by_hashimyei_,
                      component.manifest.component_identity.name);
        }
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindCOMPONENT_REVISION) {
      const std::string component_id = as_text_or_empty(&db_, kColRecordId, row);
      const std::string canonical_path =
          as_text_or_empty(&db_, kColCanonicalPath, row);
      const std::string hashimyei = as_text_or_empty(&db_, kColHashimyei, row);
      const std::string family = as_text_or_empty(&db_, kColMetricKey, row);
      const std::string parent_hashimyei = as_text_or_empty(&db_, kColMetricTxt, row);
      const std::string revision_reason = as_text_or_empty(&db_, kColPayload, row);
      if (!component_id.empty()) {
        auto it_component = components_by_id_.find(component_id);
        if (it_component != components_by_id_.end()) {
          if (!parent_hashimyei.empty()) {
            std::uint64_t parent_ordinal = 0;
            if (cuwacunu::hashimyei::parse_hex_hash_name_ordinal(parent_hashimyei,
                                                                  &parent_ordinal)) {
              cuwacunu::hashimyei::hashimyei_t parent{};
              parent.schema = cuwacunu::hashimyei::kIdentitySchemaV2;
              parent.kind = cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
              parent.name = parent_hashimyei;
              parent.ordinal = parent_ordinal;
              it_component->second.manifest.parent_identity = parent;
            }
          }
          if (!revision_reason.empty()) {
            it_component->second.manifest.revision_reason = revision_reason;
          }
          if (!family.empty()) it_component->second.manifest.family = family;
        } else if (!canonical_path.empty()) {
          component_state_t placeholder{};
          placeholder.component_id = component_id;
          placeholder.manifest.canonical_path = canonical_path;
          std::uint64_t ord = 0;
          if (cuwacunu::hashimyei::parse_hex_hash_name_ordinal(hashimyei, &ord)) {
            placeholder.manifest.component_identity.schema = cuwacunu::hashimyei::kIdentitySchemaV2;
            placeholder.manifest.component_identity.kind =
                cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
            placeholder.manifest.component_identity.name = hashimyei;
            placeholder.manifest.component_identity.ordinal = ord;
          }
          if (!parent_hashimyei.empty()) {
            std::uint64_t parent_ord = 0;
            if (cuwacunu::hashimyei::parse_hex_hash_name_ordinal(parent_hashimyei,
                                                                  &parent_ord)) {
              cuwacunu::hashimyei::hashimyei_t parent{};
              parent.schema = cuwacunu::hashimyei::kIdentitySchemaV2;
              parent.kind = cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
              parent.name = parent_hashimyei;
              parent.ordinal = parent_ord;
              placeholder.manifest.parent_identity = parent;
            }
          }
          placeholder.manifest.revision_reason =
              revision_reason.empty() ? "initial" : revision_reason;
          placeholder.manifest.family = family.empty()
                                            ? component_family_from_parts(
                                                  canonical_path, hashimyei, "")
                                            : family;
          components_by_id_[component_id] = std::move(placeholder);
        }
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindCOMPONENT_ACTIVE) {
      const std::string canonical_path =
          as_text_or_empty(&db_, kColCanonicalPath, row);
      std::string family = as_text_or_empty(&db_, kColMetricKey, row);
      const std::string hashimyei = as_text_or_empty(&db_, kColHashimyei, row);
      if (family.empty()) {
        family = component_family_from_parts(canonical_path, hashimyei, "");
      }
      if (!canonical_path.empty() && !family.empty()) {
        const std::string key =
            component_active_pointer_key(canonical_path, family, hashimyei);
        active_hashimyei_by_key_[key] = hashimyei;
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindREPORT_FRAGMENT) {
      report_fragment_entry_t e{};
      e.report_fragment_id = as_text_or_empty(&db_, kColRecordId, row);
      e.run_id = as_text_or_empty(&db_, kColRunId, row);
      e.canonical_path = as_text_or_empty(&db_, kColCanonicalPath, row);
      e.hashimyei = as_text_or_empty(&db_, kColHashimyei, row);
      e.schema = as_text_or_empty(&db_, kColSchema, row);
      e.report_fragment_sha256 = as_text_or_empty(&db_, kColReportFragmentSha256, row);
      e.path = as_text_or_empty(&db_, kColPath, row);
      e.payload_json = as_text_or_empty(&db_, kColPayload, row);
      (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &e.ts_ms);

      if (!e.report_fragment_id.empty()) {
        report_fragments_by_id_[e.report_fragment_id] = e;
        const std::string key = join_key(e.canonical_path, e.schema);
        auto it = latest_report_fragment_by_key_.find(key);
        if (it == latest_report_fragment_by_key_.end()) {
          latest_report_fragment_by_key_[key] = e.report_fragment_id;
        } else {
          const auto old_it = report_fragments_by_id_.find(it->second);
          if (old_it == report_fragments_by_id_.end() ||
              e.ts_ms > old_it->second.ts_ms ||
              (e.ts_ms == old_it->second.ts_ms &&
               e.report_fragment_id > old_it->second.report_fragment_id)) {
            it->second = e.report_fragment_id;
          }
        }
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindMETRIC_NUM) {
      const std::string report_fragment_id = as_text_or_empty(&db_, kColReportFragmentSha256, row);
      const std::string key = as_text_or_empty(&db_, kColMetricKey, row);
      const auto num = as_numeric_or_null(&db_, kColMetricNum, row);
      if (!report_fragment_id.empty() && !key.empty() && num.has_value()) {
        metrics_num_by_report_fragment_[report_fragment_id].push_back({key, *num});
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindMETRIC_TXT) {
      const std::string report_fragment_id = as_text_or_empty(&db_, kColReportFragmentSha256, row);
      const std::string key = as_text_or_empty(&db_, kColMetricKey, row);
      const std::string val = as_text_or_empty(&db_, kColMetricTxt, row);
      if (!report_fragment_id.empty() && !key.empty()) {
        metrics_txt_by_report_fragment_[report_fragment_id].push_back({key, val});
      }
      continue;
    }
  }

  for (const auto& [component_id, component] : components_by_id_) {
    const std::string binding_hashimyei =
        component.manifest.wave_contract_binding.identity.name;
    if (binding_hashimyei.empty()) continue;
    component_ids_by_binding_hashimyei_[binding_hashimyei].push_back(component_id);
  }
  for (auto& [_, ids] : component_ids_by_binding_hashimyei_) {
    std::sort(ids.begin(), ids.end());
  }

  for (const auto& [pointer_key, pointer_hashimyei] : active_hashimyei_by_key_) {
    const std::size_t sep = pointer_key.rfind('|');
    if (sep == std::string::npos) continue;
    const std::string canonical_path = pointer_key.substr(0, sep);
    const std::string family = pointer_key.substr(sep + 1);

    component_state_t best{};
    bool found = false;
    for (const auto& [_, component] : components_by_id_) {
      const std::string component_key = component_active_pointer_key(
          component.manifest.canonical_path, component.manifest.family,
          component.manifest.component_identity.name);
      if (component_key != pointer_key) continue;
      if (!family.empty() && component.manifest.family != family) continue;
      if (component.manifest.component_identity.name != pointer_hashimyei) continue;
      if (!found || component.ts_ms > best.ts_ms ||
          (component.ts_ms == best.ts_ms &&
           component.component_id > best.component_id)) {
        best = component;
        found = true;
      }
    }
    if (!found) continue;
    active_component_by_key_[pointer_key] = best.component_id;
    active_component_by_canonical_[canonical_path] = best.component_id;
    active_component_by_canonical_[best.manifest.canonical_path] = best.component_id;
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
    std::string_view contract_hashimyei, std::string_view wave_hashimyei,
    std::string_view binding_hashimyei, std::vector<run_manifest_t>* out,
    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "run list output pointer is null");
    return false;
  }
  out->clear();
  for (const auto& [_, run] : runs_by_id_) {
    if (!contract_hashimyei.empty() &&
        run.wave_contract_binding.contract.name != contract_hashimyei) {
      continue;
    }
    if (!wave_hashimyei.empty() &&
        run.wave_contract_binding.wave.name != wave_hashimyei) {
      continue;
    }
    if (!binding_hashimyei.empty() &&
        run.wave_contract_binding.identity.name != binding_hashimyei) {
      continue;
    }
    out->push_back(run);
  }
  std::sort(out->begin(), out->end(), [](const run_manifest_t& a, const run_manifest_t& b) {
    if (a.started_at_ms != b.started_at_ms) return a.started_at_ms > b.started_at_ms;
    return a.run_id < b.run_id;
  });
  return true;
}

bool hashimyei_catalog_store_t::latest_report_fragment(std::string_view canonical_path,
                                                std::string_view schema,
                                                report_fragment_entry_t* out,
                                                std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "report_fragment output pointer is null");
    return false;
  }
  const std::string key = join_key(canonical_path, schema);
  const auto it = latest_report_fragment_by_key_.find(key);
  if (it == latest_report_fragment_by_key_.end()) {
    set_error(error, "latest report_fragment not found for key: " + key);
    return false;
  }
  const auto it_art = report_fragments_by_id_.find(it->second);
  if (it_art == report_fragments_by_id_.end()) {
    set_error(error, "catalog inconsistency: latest report_fragment id missing");
    return false;
  }
  *out = it_art->second;
  return true;
}

bool hashimyei_catalog_store_t::get_report_fragment(std::string_view report_fragment_id,
                                             report_fragment_entry_t* out,
                                             std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "report_fragment output pointer is null");
    return false;
  }
  const auto it = report_fragments_by_id_.find(std::string(report_fragment_id));
  if (it == report_fragments_by_id_.end()) {
    set_error(error, "report_fragment not found: " + std::string(report_fragment_id));
    return false;
  }
  *out = it->second;
  return true;
}

bool hashimyei_catalog_store_t::report_fragment_metrics(
    std::string_view report_fragment_id,
    std::vector<std::pair<std::string, double>>* out_numeric,
    std::vector<std::pair<std::string, std::string>>* out_text,
    std::string* error) const {
  clear_error(error);
  if (!out_numeric || !out_text) {
    set_error(error, "report_fragment metrics output pointer is null");
    return false;
  }
  out_numeric->clear();
  out_text->clear();

  const std::string id(report_fragment_id);
  const auto it_num = metrics_num_by_report_fragment_.find(id);
  if (it_num != metrics_num_by_report_fragment_.end()) *out_numeric = it_num->second;
  const auto it_txt = metrics_txt_by_report_fragment_.find(id);
  if (it_txt != metrics_txt_by_report_fragment_.end()) *out_text = it_txt->second;
  return true;
}

bool hashimyei_catalog_store_t::list_report_fragments(
    std::string_view canonical_path, std::string_view schema, std::size_t limit,
    std::size_t offset, bool newest_first, std::vector<report_fragment_entry_t>* out,
    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "report_fragment list output pointer is null");
    return false;
  }
  out->clear();

  const std::string cp(canonical_path);
  const std::string sc(schema);
  for (const auto& [_, art] : report_fragments_by_id_) {
    if (!cp.empty() && art.canonical_path != cp) continue;
    if (!sc.empty() && art.schema != sc) continue;
    out->push_back(art);
  }

  std::sort(out->begin(), out->end(),
            [newest_first](const report_fragment_entry_t& a, const report_fragment_entry_t& b) {
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
      if (component.manifest.component_identity.name != hash) continue;
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
    const auto try_latest_component = [&](const std::string& key) -> bool {
      const auto it = latest_component_by_canonical_.find(key);
      if (it == latest_component_by_canonical_.end()) return false;
      const auto it_comp = components_by_id_.find(it->second);
      if (it_comp == components_by_id_.end()) return false;
      *out = it_comp->second;
      return true;
    };
    const auto try_active_component = [&](const std::string& key) -> bool {
      const auto it_active = active_component_by_canonical_.find(key);
      if (it_active == active_component_by_canonical_.end()) return false;
      const auto it_comp = components_by_id_.find(it_active->second);
      if (it_comp == components_by_id_.end()) return false;
      *out = it_comp->second;
      return true;
    };
    if (try_active_component(canonical)) return true;
    const std::string hash_tail = maybe_hashimyei_from_canonical(canonical);
    if (!hash_tail.empty()) {
      // Explicit hash suffix means exact component identity lookup.
      if (try_latest_component(canonical)) return true;
      set_error(error, "component not found for canonical_path: " + canonical);
      return false;
    }

    if (try_latest_component(canonical)) return true;

    set_error(error, "component not found for canonical_path: " + canonical);
    return false;
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
    if (!hash.empty() && component.manifest.component_identity.name != hash) continue;
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

bool hashimyei_catalog_store_t::list_components_by_binding(
    std::string_view contract_hashimyei, std::string_view wave_hashimyei,
    std::string_view binding_hashimyei, std::size_t limit, std::size_t offset,
    bool newest_first, std::vector<component_state_t>* out, std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "component list output pointer is null");
    return false;
  }
  out->clear();

  const std::string contract(contract_hashimyei);
  const std::string wave(wave_hashimyei);
  const std::string binding(binding_hashimyei);

  const auto passes_binding_filters = [&](const component_state_t& component) {
    if (!contract.empty() &&
        component.manifest.wave_contract_binding.contract.name != contract) {
      return false;
    }
    if (!wave.empty() && component.manifest.wave_contract_binding.wave.name != wave) {
      return false;
    }
    if (!binding.empty() &&
        component.manifest.wave_contract_binding.identity.name != binding) {
      return false;
    }
    return true;
  };

  if (!binding.empty()) {
    const auto it = component_ids_by_binding_hashimyei_.find(binding);
    if (it != component_ids_by_binding_hashimyei_.end()) {
      for (const auto& component_id : it->second) {
        const auto it_component = components_by_id_.find(component_id);
        if (it_component == components_by_id_.end()) continue;
        if (!passes_binding_filters(it_component->second)) continue;
        out->push_back(it_component->second);
      }
    }
  } else {
    for (const auto& [_, component] : components_by_id_) {
      if (!passes_binding_filters(component)) continue;
      out->push_back(component);
    }
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

bool hashimyei_catalog_store_t::resolve_active_hashimyei(
    std::string_view canonical_path, std::string_view family,
    std::string* out_hashimyei, std::string* error) const {
  clear_error(error);
  if (!out_hashimyei) {
    set_error(error, "active hashimyei output pointer is null");
    return false;
  }
  out_hashimyei->clear();

  const std::string canonical(canonical_path);
  const std::string fam(family);
  if (canonical.empty()) {
    set_error(error, "canonical_path is required");
    return false;
  }
  const std::string canonical_hash_tail = maybe_hashimyei_from_canonical(canonical);
  std::string canonical_lookup = canonical;
  if (!canonical_hash_tail.empty()) {
    canonical_lookup = component_family_from_parts(canonical, canonical_hash_tail, "");
  }

  if (!fam.empty()) {
    const std::string key =
        component_active_pointer_key(canonical_lookup, fam, canonical_hash_tail);
    const auto it = active_hashimyei_by_key_.find(key);
    if (it == active_hashimyei_by_key_.end()) {
      set_error(error, "active hashimyei not found for canonical_path/family");
      return false;
    }
    *out_hashimyei = it->second;
    return true;
  }

  std::string best_key{};
  std::uint64_t best_ts = 0;
  bool found = false;
  for (const auto& [key, value] : active_hashimyei_by_key_) {
    const std::size_t sep = key.rfind('|');
    if (sep == std::string::npos) continue;
    if (key.substr(0, sep) != canonical_lookup) continue;
    std::uint64_t ts = 0;
    if (const auto it_comp = active_component_by_key_.find(key);
        it_comp != active_component_by_key_.end()) {
      if (const auto it_state = components_by_id_.find(it_comp->second);
          it_state != components_by_id_.end()) {
        ts = it_state->second.ts_ms;
      }
    }
    if (!found || ts > best_ts || (ts == best_ts && key > best_key)) {
      best_key = key;
      best_ts = ts;
      *out_hashimyei = value;
      found = true;
    }
  }
  if (!found) {
    set_error(error, "active hashimyei not found for canonical_path");
    return false;
  }
  return true;
}

bool hashimyei_catalog_store_t::register_component_manifest(
    const component_manifest_t& manifest_in, std::string* out_component_id,
    bool* out_inserted, std::string* error) {
  clear_error(error);
  if (out_component_id) out_component_id->clear();
  if (out_inserted) *out_inserted = false;
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  component_manifest_t manifest = manifest_in;
  if (trim_ascii(manifest.schema).empty()) {
    manifest.schema = cuwacunu::hashimyei::kComponentManifestSchemaV2;
  }
  if (!ensure_identity_allocated_(&manifest.component_identity, error)) return false;
  if (manifest.parent_identity.has_value() &&
      !manifest.parent_identity->name.empty()) {
    std::string parent_error;
    if (!cuwacunu::hashimyei::validate_hashimyei(*manifest.parent_identity,
                                                  &parent_error)) {
      set_error(error, "parent_identity invalid: " + parent_error);
      return false;
    }
  }
  if (!ensure_identity_allocated_(&manifest.wave_contract_binding.contract, error)) {
    return false;
  }
  if (!ensure_identity_allocated_(&manifest.wave_contract_binding.wave, error)) {
    return false;
  }
  if (manifest.wave_contract_binding.identity.hash_sha256_hex.empty()) {
    manifest.wave_contract_binding.identity.hash_sha256_hex =
        compute_wave_contract_binding_hash(manifest.wave_contract_binding);
  }
  if (!ensure_identity_allocated_(&manifest.wave_contract_binding.identity, error)) {
    return false;
  }
  if (manifest.created_at_ms == 0) manifest.created_at_ms = now_ms_utc();
  if (manifest.updated_at_ms == 0) manifest.updated_at_ms = manifest.created_at_ms;
  if (!validate_component_manifest(manifest, error)) return false;

  const std::string payload = component_manifest_payload(manifest);
  std::string report_fragment_sha;
  if (!sha256_hex_bytes(payload, &report_fragment_sha)) {
    set_error(error, "failed to compute component manifest payload sha256");
    return false;
  }

  bool already = false;
  if (!ledger_contains_(report_fragment_sha, &already, error)) return false;

  const std::string component_id = compute_component_manifest_id(manifest);
  if (out_component_id) *out_component_id = component_id;
  if (already) return true;

  const std::uint64_t ts_ms =
      manifest.updated_at_ms != 0 ? manifest.updated_at_ms
                                  : (manifest.created_at_ms != 0
                                         ? manifest.created_at_ms
                                         : now_ms_utc());

  const std::string virtual_manifest_path =
      "virtual://hero.config/cutover/" + component_id +
      "/" + std::string(cuwacunu::hashimyei::kComponentManifestFilenameV2);

  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT, component_id, "", manifest.canonical_path,
                   manifest.component_identity.name, manifest.schema,
                   manifest.tsi_type,
                   std::numeric_limits<double>::quiet_NaN(), manifest.status,
                   report_fragment_sha, virtual_manifest_path, std::to_string(ts_ms),
                   payload, error)) {
    return false;
  }

  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_DEPENDENCY, component_id + "|dsl", "",
                   manifest.canonical_path, manifest.component_identity.name,
                   manifest.schema,
                   manifest.dsl_canonical_path,
                   std::numeric_limits<double>::quiet_NaN(),
                   manifest.dsl_sha256_hex, "", virtual_manifest_path,
                   std::to_string(ts_ms), "{}", error)) {
    return false;
  }

  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_REVISION, component_id + "|revision", "",
                   manifest.canonical_path, manifest.component_identity.name,
                   manifest.schema,
                   manifest.family, std::numeric_limits<double>::quiet_NaN(),
                   manifest.parent_identity.has_value()
                       ? std::string_view(manifest.parent_identity->name)
                       : std::string_view{},
                   "", virtual_manifest_path,
                   std::to_string(ts_ms), manifest.revision_reason, error)) {
    return false;
  }

  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_BINDING, component_id + "|binding", "",
                   manifest.canonical_path, manifest.component_identity.name,
                   manifest.schema, manifest.wave_contract_binding.identity.name,
                   std::numeric_limits<double>::quiet_NaN(),
                   manifest.wave_contract_binding.contract.name + "|" +
                       manifest.wave_contract_binding.wave.name,
                   "", virtual_manifest_path, std::to_string(ts_ms),
                   component_binding_index_payload(manifest), error)) {
    return false;
  }

  if (trim_ascii(manifest.status) == "active") {
    const std::string pointer_key = component_active_pointer_key(
        manifest.canonical_path, manifest.family, manifest.component_identity.name);
    std::ostringstream pointer_payload;
    pointer_payload << "canonical_path=" << manifest.canonical_path << "\n";
    pointer_payload << "family=" << manifest.family << "\n";
    pointer_payload << "active_hashimyei="
                    << manifest.component_identity.name << "\n";
    pointer_payload << "component_id=" << component_id << "\n";
    pointer_payload << "parent_hashimyei="
                    << (manifest.parent_identity.has_value()
                            ? std::string_view(manifest.parent_identity->name)
                            : std::string_view{})
                    << "\n";
    pointer_payload << "revision_reason=" << manifest.revision_reason << "\n";
    pointer_payload << "config_revision_id=" << manifest.config_revision_id
                    << "\n";
    if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_ACTIVE, pointer_key, "",
                     manifest.canonical_path, manifest.component_identity.name,
                     cuwacunu::hashimyei::kComponentActivePointerSchemaV2, manifest.family,
                     std::numeric_limits<double>::quiet_NaN(),
                     manifest.config_revision_id, "", virtual_manifest_path,
                     std::to_string(ts_ms), pointer_payload.str(), error)) {
      return false;
    }
  }

  if (!append_ledger_(report_fragment_sha, virtual_manifest_path, error)) return false;
  if (!rebuild_indexes(error)) return false;

  if (out_inserted) *out_inserted = true;
  return true;
}

bool hashimyei_catalog_store_t::report_fragment_snapshot(
    std::string_view canonical_path, std::string_view run_id,
    report_fragment_snapshot_t* out, std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "report_fragment snapshot output pointer is null");
    return false;
  }
  *out = report_fragment_snapshot_t{};

  const std::string cp(canonical_path);
  report_fragment_entry_t best{};
  bool found = false;
  for (const auto& [_, art] : report_fragments_by_id_) {
    if (art.canonical_path != cp) continue;
    if (!run_id.empty() && art.run_id != run_id) continue;
    if (!found || art.ts_ms > best.ts_ms ||
        (art.ts_ms == best.ts_ms && art.report_fragment_id > best.report_fragment_id)) {
      best = art;
      found = true;
    }
  }
  if (!found) {
    set_error(error, "report_fragment snapshot not found");
    return false;
  }

  out->report_fragment = best;
  (void)report_fragment_metrics(best.report_fragment_id, &out->numeric_metrics, &out->text_metrics,
                         nullptr);
  return true;
}

bool hashimyei_catalog_store_t::dependency_trace(
    std::string_view report_fragment_id, std::vector<dependency_file_t>* out,
    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "dependency output pointer is null");
    return false;
  }
  out->clear();
  const auto it_art = report_fragments_by_id_.find(std::string(report_fragment_id));
  if (it_art == report_fragments_by_id_.end()) {
    set_error(error, "report_fragment not found: " + std::string(report_fragment_id));
    return false;
  }
  const auto it = dependency_files_by_run_id_.find(it_art->second.run_id);
  if (it == dependency_files_by_run_id_.end()) return true;
  *out = it->second;
  return true;
}

}  // namespace hashimyei
}  // namespace hero
}  // namespace cuwacunu
