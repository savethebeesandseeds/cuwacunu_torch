#include "hero/hashimyei_hero/hashimyei_catalog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <openssl/evp.h>
#include <optional>
#include <set>
#include <sstream>
#include <string_view>
#include <thread>
#include <unordered_set>

#include "camahjucunu/db/idydb.h"
#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "hero/hashimyei_hero/hashimyei_report_fragments.h"
#include "hero/hero_catalog_schema.h"
#include "hero/mcp_server_observability.h"
#include "iitepi/contract_space_t.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"

namespace cuwacunu {
namespace hero {
namespace hashimyei {
namespace {

using runtime_lls_document_t =
    cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t;
constexpr std::string_view kObservabilityScope = "hashimyei_catalog";

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

[[nodiscard]] bool is_valid_component_tag_token(std::string_view name) {
  if (name.empty() || name.size() > 128U)
    return false;
  if (name.front() == '.' || name.front() == '-' || name.back() == '.' ||
      name.back() == '-')
    return false;

  bool previous_dot = false;
  for (const unsigned char c : name) {
    const bool ok = std::isalnum(c) != 0 || c == '_' || c == '-' || c == '.';
    if (!ok)
      return false;
    if (c == '.' && previous_dot)
      return false;
    previous_dot = (c == '.');
  }
  return true;
}

[[nodiscard]] bool parse_u64(std::string_view text, std::uint64_t *out) {
  if (!out)
    return false;
  const std::string t = trim_ascii(text);
  if (t.empty())
    return false;
  std::uint64_t v = 0;
  std::istringstream iss(t);
  iss >> v;
  if (!iss || !iss.eof())
    return false;
  *out = v;
  return true;
}

[[nodiscard]] std::uint64_t now_ms_utc() {
  const auto now = std::chrono::system_clock::now();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now.time_since_epoch())
          .count());
}

void log_hashimyei_catalog_event(std::string_view event_name,
                                 std::string_view extra_fields_json = {}) {
  std::string ignored{};
  (void)cuwacunu::hero::mcp_observability::append_event_json(
      kObservabilityScope, event_name, extra_fields_json, &ignored);
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
  *out_hex = hex_lower(digest, digest_len);
  return true;
}

[[nodiscard]] bool sha256_hex_file(const std::filesystem::path &path,
                                   std::string *out_hex, std::string *error) {
  if (error)
    error->clear();
  if (!out_hex) {
    if (error)
      *error = "sha256 output pointer is null";
    return false;
  }
  out_hex->clear();

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error)
      *error = "cannot open file for sha256: " + path.string();
    return false;
  }

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx) {
    if (error)
      *error = "cannot allocate EVP context";
    return false;
  }
  if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
    EVP_MD_CTX_free(ctx);
    if (error)
      *error = "EVP_DigestInit_ex failed";
    return false;
  }

  std::array<char, 1 << 14> buf{};
  while (in.good()) {
    in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
    const std::streamsize got = in.gcount();
    if (got <= 0)
      break;
    if (EVP_DigestUpdate(ctx, buf.data(), static_cast<std::size_t>(got)) != 1) {
      EVP_MD_CTX_free(ctx);
      if (error)
        *error = "EVP_DigestUpdate failed";
      return false;
    }
  }

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
    EVP_MD_CTX_free(ctx);
    if (error)
      *error = "EVP_DigestFinal_ex failed";
    return false;
  }
  EVP_MD_CTX_free(ctx);
  *out_hex = hex_lower(digest, digest_len);
  return true;
}

[[nodiscard]] bool read_text_file(const std::filesystem::path &path,
                                  std::string *out, std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "text output pointer is null";
    return false;
  }
  out->clear();

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error)
      *error = "cannot read file: " + path.string();
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

[[nodiscard]] bool
write_identity_kv(std::ostringstream *out, std::string_view prefix,
                  const cuwacunu::hashimyei::hashimyei_t &id) {
  if (!out)
    return false;
  *out << prefix << ".schema=" << id.schema << "\n";
  *out << prefix
       << ".kind=" << cuwacunu::hashimyei::hashimyei_kind_to_string(id.kind)
       << "\n";
  *out << prefix << ".name=" << id.name << "\n";
  *out << prefix << ".ordinal=" << id.ordinal << "\n";
  *out << prefix << ".hash_sha256_hex=" << id.hash_sha256_hex << "\n";
  return true;
}

[[nodiscard]] std::string
normalize_hashimyei_name_or_same(std::string_view raw_name) {
  std::string normalized{};
  if (cuwacunu::hashimyei::normalize_hex_hash_name(raw_name, &normalized)) {
    return normalized;
  }
  return trim_ascii(raw_name);
}

[[nodiscard]] std::string normalize_hashimyei_canonical_path_or_same(
    std::string_view raw_canonical_path) {
  return cuwacunu::hashimyei::normalize_hashimyei_canonical_path(
      trim_ascii(raw_canonical_path));
}

inline void normalize_identity_hex_name_inplace(
    cuwacunu::hashimyei::hashimyei_t *identity) {
  if (!identity)
    return;
  cuwacunu::hashimyei::normalize_hex_hash_name_inplace(&identity->name);
}

inline void normalize_component_instance_inplace(
    cuwacunu::hero::hashimyei::component_instance_t *component) {
  if (!component)
    return;
  component->canonical_path =
      normalize_hashimyei_canonical_path_or_same(component->canonical_path);
  component->hashimyei = normalize_hashimyei_name_or_same(component->hashimyei);
}

inline void normalize_run_manifest_inplace(
    cuwacunu::hero::hashimyei::run_manifest_t *manifest) {
  if (!manifest)
    return;
  normalize_identity_hex_name_inplace(&manifest->campaign_identity);
  normalize_identity_hex_name_inplace(
      &manifest->wave_contract_binding.identity);
  normalize_identity_hex_name_inplace(
      &manifest->wave_contract_binding.contract);
  normalize_identity_hex_name_inplace(&manifest->wave_contract_binding.wave);
  for (auto &component : manifest->components) {
    normalize_component_instance_inplace(&component);
  }
}

inline void normalize_component_manifest_inplace(
    cuwacunu::hero::hashimyei::component_manifest_t *manifest) {
  if (!manifest)
    return;
  manifest->canonical_path =
      normalize_hashimyei_canonical_path_or_same(manifest->canonical_path);
  manifest->component_tag = trim_ascii(manifest->component_tag);
  normalize_identity_hex_name_inplace(&manifest->hashimyei_identity);
  normalize_identity_hex_name_inplace(&manifest->contract_identity);
  if (manifest->parent_identity.has_value()) {
    normalize_identity_hex_name_inplace(&*manifest->parent_identity);
  }
  cuwacunu::hashimyei::normalize_hex_hash_name_inplace(&manifest->replaced_by);
  manifest->docking_signature_sha256_hex =
      lowercase_copy(trim_ascii(manifest->docking_signature_sha256_hex));
  manifest->component_compatibility_sha256_hex =
      lowercase_copy(trim_ascii(manifest->component_compatibility_sha256_hex));
  for (const auto &[field, value] :
       cuwacunu::camahjucunu::instrument_signature_fields(
           manifest->instrument_signature)) {
    std::string ignored_error{};
    (void)cuwacunu::camahjucunu::instrument_signature_set_field(
        &manifest->instrument_signature, field, value, &ignored_error);
  }
}

[[nodiscard]] bool
parse_identity_kv(const std::unordered_map<std::string, std::string> &kv,
                  std::string_view prefix, bool required,
                  cuwacunu::hashimyei::hashimyei_t *out, std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "identity output pointer is null";
    return false;
  }
  *out = cuwacunu::hashimyei::hashimyei_t{};

  const std::string prefix_s(prefix);
  const auto key = [&](std::string_view suffix) {
    return prefix_s + "." + std::string(suffix);
  };

  const auto it_name = kv.find(key("name"));
  if (it_name == kv.end() || trim_ascii(it_name->second).empty()) {
    if (!required)
      return true;
    if (error)
      *error = "missing identity field: " + key("name");
    return false;
  }

  const auto it_schema = kv.find(key("schema"));
  if (it_schema != kv.end() && !trim_ascii(it_schema->second).empty()) {
    out->schema = trim_ascii(it_schema->second);
  }
  out->name = normalize_hashimyei_name_or_same(it_name->second);

  const auto it_kind = kv.find(key("kind"));
  if (it_kind == kv.end()) {
    if (error)
      *error = "missing identity field: " + key("kind");
    return false;
  }
  cuwacunu::hashimyei::hashimyei_kind_e parsed_kind{};
  if (!cuwacunu::hashimyei::parse_hashimyei_kind(trim_ascii(it_kind->second),
                                                 &parsed_kind)) {
    if (error)
      *error = "invalid identity kind for key: " + key("kind");
    return false;
  }
  out->kind = parsed_kind;

  const auto it_ordinal = kv.find(key("ordinal"));
  if (it_ordinal == kv.end()) {
    if (error)
      *error = "missing identity field: " + key("ordinal");
    return false;
  }
  if (!parse_u64(it_ordinal->second, &out->ordinal)) {
    if (error)
      *error = "invalid identity ordinal for key: " + key("ordinal");
    return false;
  }

  const auto it_hash = kv.find(key("hash_sha256_hex"));
  if (it_hash != kv.end())
    out->hash_sha256_hex = trim_ascii(it_hash->second);

  std::string validate_error;
  if (!cuwacunu::hashimyei::validate_hashimyei(*out, &validate_error)) {
    if (error)
      *error = "identity validation failed for prefix " + prefix_s + ": " +
               validate_error;
    return false;
  }
  return true;
}

[[nodiscard]] std::string compute_wave_contract_binding_hash(
    const cuwacunu::hero::hashimyei::wave_contract_binding_t &binding) {
  std::ostringstream seed;
  seed << binding.contract.hash_sha256_hex << "|"
       << binding.wave.hash_sha256_hex << "|" << binding.binding_id;
  std::string digest;
  if (!sha256_hex_bytes(seed.str(), &digest))
    return {};
  return digest;
}

[[nodiscard]] bool validate_wave_contract_binding(
    const cuwacunu::hero::hashimyei::wave_contract_binding_t &binding,
    std::string *error = nullptr) {
  if (error)
    error->clear();
  if (binding.binding_id.empty()) {
    if (error)
      *error = "binding_id is required";
    return false;
  }
  if (binding.contract.kind !=
      cuwacunu::hashimyei::hashimyei_kind_e::CONTRACT) {
    if (error)
      *error = "binding.contract kind must be CONTRACT";
    return false;
  }
  if (binding.wave.kind != cuwacunu::hashimyei::hashimyei_kind_e::WAVE) {
    if (error)
      *error = "binding.wave kind must be WAVE";
    return false;
  }
  if (binding.identity.kind !=
      cuwacunu::hashimyei::hashimyei_kind_e::WAVE_CONTRACT_BINDING) {
    if (error)
      *error = "binding.identity kind must be WAVE_CONTRACT_BINDING";
    return false;
  }
  std::string validate_error;
  if (!cuwacunu::hashimyei::validate_hashimyei(binding.contract,
                                               &validate_error)) {
    if (error)
      *error = "binding.contract invalid: " + validate_error;
    return false;
  }
  if (!cuwacunu::hashimyei::validate_hashimyei(binding.wave, &validate_error)) {
    if (error)
      *error = "binding.wave invalid: " + validate_error;
    return false;
  }
  if (!cuwacunu::hashimyei::validate_hashimyei(binding.identity,
                                               &validate_error)) {
    if (error)
      *error = "binding.identity invalid: " + validate_error;
    return false;
  }
  const std::string expected = compute_wave_contract_binding_hash(binding);
  if (expected.empty()) {
    if (error)
      *error = "cannot derive wave_contract_binding hash";
    return false;
  }
  if (binding.identity.hash_sha256_hex != expected) {
    if (error)
      *error =
          "binding.identity.hash_sha256_hex does not match canonical tuple";
    return false;
  }
  return true;
}

[[nodiscard]] bool
same_contract_identity_(const cuwacunu::hashimyei::hashimyei_t &lhs,
                        const cuwacunu::hashimyei::hashimyei_t &rhs) {
  const std::string lhs_hash = trim_ascii(lhs.hash_sha256_hex);
  const std::string rhs_hash = trim_ascii(rhs.hash_sha256_hex);
  if (!lhs_hash.empty() && !rhs_hash.empty()) {
    return lowercase_copy(lhs_hash) == lowercase_copy(rhs_hash);
  }
  return trim_ascii(lhs.name) == trim_ascii(rhs.name);
}

[[nodiscard]] bool write_binding_kv(
    std::ostringstream *out, std::string_view prefix,
    const cuwacunu::hero::hashimyei::wave_contract_binding_t &binding) {
  if (!out)
    return false;
  if (!write_identity_kv(out, std::string(prefix) + ".identity",
                         binding.identity)) {
    return false;
  }
  if (!write_identity_kv(out, std::string(prefix) + ".contract",
                         binding.contract)) {
    return false;
  }
  if (!write_identity_kv(out, std::string(prefix) + ".wave", binding.wave)) {
    return false;
  }
  *out << prefix << ".binding_id=" << binding.binding_id << "\n";
  return true;
}

[[nodiscard]] bool
parse_binding_kv(const std::unordered_map<std::string, std::string> &kv,
                 std::string_view prefix,
                 cuwacunu::hero::hashimyei::wave_contract_binding_t *out,
                 std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "binding output pointer is null";
    return false;
  }
  *out = cuwacunu::hero::hashimyei::wave_contract_binding_t{};

  const std::string prefix_s(prefix);
  const auto binding_id_it = kv.find(prefix_s + ".binding_id");
  if (binding_id_it == kv.end()) {
    if (error)
      *error = "missing binding field: " + prefix_s + ".binding_id";
    return false;
  }
  out->binding_id = trim_ascii(binding_id_it->second);

  if (!parse_identity_kv(kv, prefix_s + ".identity", true, &out->identity,
                         error)) {
    return false;
  }
  if (!parse_identity_kv(kv, prefix_s + ".contract", true, &out->contract,
                         error)) {
    return false;
  }
  if (!parse_identity_kv(kv, prefix_s + ".wave", true, &out->wave, error)) {
    return false;
  }
  return validate_wave_contract_binding(*out, error);
}

[[nodiscard]] std::string run_manifest_payload(const run_manifest_t &m) {
  run_manifest_t normalized = m;
  normalize_run_manifest_inplace(&normalized);
  std::ostringstream out;
  out << "schema=" << normalized.schema << "\n";
  out << "run_id=" << normalized.run_id << "\n";
  out << "started_at_ms=" << normalized.started_at_ms << "\n";
  (void)write_identity_kv(&out, "campaign_identity",
                          normalized.campaign_identity);
  (void)write_binding_kv(&out, "wave_contract_binding",
                         normalized.wave_contract_binding);
  out << "sampler=" << normalized.sampler << "\n";
  out << "record_type=" << normalized.record_type << "\n";
  out << "seed=" << normalized.seed << "\n";
  out << "device=" << normalized.device << "\n";
  out << "dtype=" << normalized.dtype << "\n";
  out << "dependency_count=" << normalized.dependency_files.size() << "\n";
  for (std::size_t i = 0; i < normalized.dependency_files.size(); ++i) {
    out << "dependency_" << std::setw(4) << std::setfill('0') << i
        << "_path=" << normalized.dependency_files[i].canonical_path << "\n";
    out << "dependency_" << std::setw(4) << std::setfill('0') << i
        << "_sha256=" << normalized.dependency_files[i].sha256_hex << "\n";
  }
  out << "component_count=" << normalized.components.size() << "\n";
  for (std::size_t i = 0; i < normalized.components.size(); ++i) {
    out << "component_" << std::setw(4) << std::setfill('0') << i
        << "_canonical_path=" << normalized.components[i].canonical_path
        << "\n";
    out << "component_" << std::setw(4) << std::setfill('0') << i
        << "_family=" << normalized.components[i].family << "\n";
    out << "component_" << std::setw(4) << std::setfill('0') << i
        << "_hashimyei=" << normalized.components[i].hashimyei << "\n";
  }
  return out.str();
}

[[nodiscard]] std::string
component_manifest_payload(const component_manifest_t &manifest) {
  component_manifest_t normalized = manifest;
  normalize_component_manifest_inplace(&normalized);
  std::ostringstream out;
  out << "schema=" << normalized.schema << "\n";
  out << "canonical_path=" << normalized.canonical_path << "\n";
  out << "family=" << normalized.family << "\n";
  out << "component_tag=" << normalized.component_tag << "\n";
  (void)write_identity_kv(&out, "hashimyei_identity",
                          normalized.hashimyei_identity);
  (void)write_identity_kv(&out, "contract_identity",
                          normalized.contract_identity);
  out << "parent_identity.present="
      << (normalized.parent_identity.has_value() ? "1" : "0") << "\n";
  if (normalized.parent_identity.has_value()) {
    (void)write_identity_kv(&out, "parent_identity",
                            *normalized.parent_identity);
  }
  out << "revision_reason=" << normalized.revision_reason << "\n";
  out << "founding_revision_id=" << normalized.founding_revision_id << "\n";
  out << "founding_dsl_source_path=" << normalized.founding_dsl_source_path
      << "\n";
  out << "founding_dsl_source_sha256_hex="
      << normalized.founding_dsl_source_sha256_hex << "\n";
  out << "docking_signature_sha256_hex="
      << normalized.docking_signature_sha256_hex << "\n";
  out << "component_compatibility_sha256_hex="
      << normalized.component_compatibility_sha256_hex << "\n";
  for (const auto &[field, value] :
       cuwacunu::camahjucunu::instrument_signature_fields(
           normalized.instrument_signature)) {
    out << "instrument_signature." << field << "=" << value << "\n";
  }
  out << "lineage_state=" << normalized.lineage_state << "\n";
  out << "replaced_by=" << normalized.replaced_by << "\n";
  out << "created_at_ms=" << normalized.created_at_ms << "\n";
  out << "updated_at_ms=" << normalized.updated_at_ms << "\n";
  return out.str();
}

[[nodiscard]] static std::string
founding_bundle_file_key(std::size_t index, std::string_view suffix) {
  std::ostringstream oss;
  oss << "file_" << std::setfill('0') << std::setw(4) << index << "_" << suffix;
  return oss.str();
}

} // namespace
