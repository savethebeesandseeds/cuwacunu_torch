#include "hero/hashimyei_hero/hashimyei_catalog.h"

#include <algorithm>
#include <array>
#include <cerrno>
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
#include <thread>
#include <unordered_set>
#include <openssl/evp.h>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "camahjucunu/db/idydb.h"
#include "hero/hero_catalog_schema.h"
#include "hero/hashimyei_hero/hashimyei_report_fragments.h"
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

void log_hashimyei_catalog_event(std::string_view event_name,
                                 std::string_view extra_fields_json = {}) {
  std::string ignored{};
  (void)cuwacunu::hero::mcp_observability::append_event_json(
      kObservabilityScope, event_name, extra_fields_json, &ignored);
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

[[nodiscard]] std::string normalize_hashimyei_name_or_same(
    std::string_view raw_name) {
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
    cuwacunu::hashimyei::hashimyei_t* identity) {
  if (!identity) return;
  cuwacunu::hashimyei::normalize_hex_hash_name_inplace(&identity->name);
}

inline void normalize_component_instance_inplace(
    cuwacunu::hero::hashimyei::component_instance_t* component) {
  if (!component) return;
  component->canonical_path =
      normalize_hashimyei_canonical_path_or_same(component->canonical_path);
  component->hashimyei = normalize_hashimyei_name_or_same(component->hashimyei);
}

inline void normalize_run_manifest_inplace(
    cuwacunu::hero::hashimyei::run_manifest_t* manifest) {
  if (!manifest) return;
  normalize_identity_hex_name_inplace(&manifest->campaign_identity);
  normalize_identity_hex_name_inplace(&manifest->wave_contract_binding.identity);
  normalize_identity_hex_name_inplace(&manifest->wave_contract_binding.contract);
  normalize_identity_hex_name_inplace(&manifest->wave_contract_binding.wave);
  for (auto& component : manifest->components) {
    normalize_component_instance_inplace(&component);
  }
}

inline void normalize_component_manifest_inplace(
    cuwacunu::hero::hashimyei::component_manifest_t* manifest) {
  if (!manifest) return;
  manifest->canonical_path =
      normalize_hashimyei_canonical_path_or_same(manifest->canonical_path);
  normalize_identity_hex_name_inplace(&manifest->hashimyei_identity);
  normalize_identity_hex_name_inplace(&manifest->contract_identity);
  if (manifest->parent_identity.has_value()) {
    normalize_identity_hex_name_inplace(&*manifest->parent_identity);
  }
  cuwacunu::hashimyei::normalize_hex_hash_name_inplace(&manifest->replaced_by);
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
  out->name = normalize_hashimyei_name_or_same(it_name->second);

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
       << binding.wave.hash_sha256_hex << "|" << binding.binding_id;
  std::string digest;
  if (!sha256_hex_bytes(seed.str(), &digest)) return {};
  return digest;
}

[[nodiscard]] bool validate_wave_contract_binding(
    const cuwacunu::hero::hashimyei::wave_contract_binding_t& binding,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (binding.binding_id.empty()) {
    if (error) *error = "binding_id is required";
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

[[nodiscard]] bool same_contract_identity_(
    const cuwacunu::hashimyei::hashimyei_t& lhs,
    const cuwacunu::hashimyei::hashimyei_t& rhs) {
  const std::string lhs_hash = trim_ascii(lhs.hash_sha256_hex);
  const std::string rhs_hash = trim_ascii(rhs.hash_sha256_hex);
  if (!lhs_hash.empty() && !rhs_hash.empty()) {
    return lowercase_copy(lhs_hash) == lowercase_copy(rhs_hash);
  }
  return trim_ascii(lhs.name) == trim_ascii(rhs.name);
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
  *out << prefix << ".binding_id=" << binding.binding_id << "\n";
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
  const auto binding_id_it = kv.find(prefix_s + ".binding_id");
  if (binding_id_it == kv.end()) {
    if (error) *error = "missing binding field: " + prefix_s + ".binding_id";
    return false;
  }
  out->binding_id = trim_ascii(binding_id_it->second);

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
  run_manifest_t normalized = m;
  normalize_run_manifest_inplace(&normalized);
  std::ostringstream out;
  out << "schema=" << normalized.schema << "\n";
  out << "run_id=" << normalized.run_id << "\n";
  out << "started_at_ms=" << normalized.started_at_ms << "\n";
  (void)write_identity_kv(&out, "campaign_identity", normalized.campaign_identity);
  (void)write_binding_kv(&out, "wave_contract_binding",
                         normalized.wave_contract_binding);
  out << "sampler=" << normalized.sampler << "\n";
  out << "record_type=" << normalized.record_type << "\n";
  out << "seed=" << normalized.seed << "\n";
  out << "device=" << normalized.device << "\n";
  out << "dtype=" << normalized.dtype << "\n";
  out << "dependency_count=" << normalized.dependency_files.size() << "\n";
  for (std::size_t i = 0; i < normalized.dependency_files.size(); ++i) {
    out << "dependency_" << std::setw(4) << std::setfill('0') << i << "_path="
        << normalized.dependency_files[i].canonical_path << "\n";
    out << "dependency_" << std::setw(4) << std::setfill('0') << i << "_sha256="
        << normalized.dependency_files[i].sha256_hex << "\n";
  }
  out << "component_count=" << normalized.components.size() << "\n";
  for (std::size_t i = 0; i < normalized.components.size(); ++i) {
    out << "component_" << std::setw(4) << std::setfill('0') << i
        << "_canonical_path=" << normalized.components[i].canonical_path << "\n";
    out << "component_" << std::setw(4) << std::setfill('0') << i
        << "_family=" << normalized.components[i].family << "\n";
    out << "component_" << std::setw(4) << std::setfill('0') << i
        << "_hashimyei=" << normalized.components[i].hashimyei << "\n";
  }
  return out.str();
}

[[nodiscard]] std::string component_manifest_payload(
    const component_manifest_t& manifest) {
  component_manifest_t normalized = manifest;
  normalize_component_manifest_inplace(&normalized);
  std::ostringstream out;
  out << "schema=" << normalized.schema << "\n";
  out << "canonical_path=" << normalized.canonical_path << "\n";
  out << "family=" << normalized.family << "\n";
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
  out << "founding_dsl_source_path="
      << normalized.founding_dsl_source_path << "\n";
  out << "founding_dsl_source_sha256_hex="
      << normalized.founding_dsl_source_sha256_hex << "\n";
  out << "docking_signature_sha256_hex="
      << normalized.docking_signature_sha256_hex << "\n";
  out << "lineage_state=" << normalized.lineage_state << "\n";
  out << "replaced_by=" << normalized.replaced_by << "\n";
  out << "created_at_ms=" << normalized.created_at_ms << "\n";
  out << "updated_at_ms=" << normalized.updated_at_ms << "\n";
  return out.str();
}

[[nodiscard]] static std::string founding_bundle_file_key(
    std::size_t index, std::string_view suffix) {
  std::ostringstream oss;
  oss << "file_" << std::setfill('0') << std::setw(4) << index << "_"
      << suffix;
  return oss.str();
}

}  // namespace

[[nodiscard]] std::filesystem::path store_root_from_catalog_path(
    const std::filesystem::path& catalog_path) {
  const std::filesystem::path catalog_dir = catalog_path.parent_path();
  if (catalog_dir.filename() == "catalog" &&
      catalog_dir.parent_path().filename() ==
          std::string(cuwacunu::hashimyei::kHashimyeiMetaDirname)) {
    return catalog_dir.parent_path().parent_path();
  }
  return catalog_dir;
}

std::string compute_founding_dsl_bundle_aggregate_sha256(
    const founding_dsl_bundle_manifest_t& manifest) {
  std::ostringstream seed;
  seed << manifest.schema << "|" << manifest.component_id << "|"
       << manifest.canonical_path << "|" << manifest.hashimyei_name << "|"
       << manifest.files.size() << "|";
  for (const auto& file : manifest.files) {
    seed << file.snapshot_relpath << "|" << file.sha256_hex << "|";
  }
  std::string digest{};
  if (!sha256_hex_bytes(seed.str(), &digest)) return {};
  return digest;
}

bool write_founding_dsl_bundle_manifest(
    const std::filesystem::path& store_root,
    const founding_dsl_bundle_manifest_t& manifest, std::string* error) {
  if (error) error->clear();
  if (trim_ascii(manifest.component_id).empty()) {
    if (error) *error = "founding DSL bundle manifest missing component_id";
    return false;
  }

  const auto manifest_path = founding_dsl_bundle_manifest_path(
      store_root, manifest.canonical_path, manifest.component_id);
  std::error_code ec{};
  std::filesystem::create_directories(manifest_path.parent_path(), ec);
  if (ec) {
    if (error) {
      *error = "cannot create founding DSL bundle directory: " +
               manifest_path.parent_path().string();
    }
    return false;
  }

  std::ofstream out(manifest_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error) {
      *error = "cannot open founding DSL bundle manifest for write: " +
               manifest_path.string();
    }
    return false;
  }

  out << "schema=" << cuwacunu::hashimyei::kFoundingDslBundleManifestSchemaV1
      << "\n";
  out << "component_id=" << manifest.component_id << "\n";
  out << "canonical_path=" << manifest.canonical_path << "\n";
  out << "hashimyei_name=" << manifest.hashimyei_name << "\n";
  out << "founding_dsl_bundle_aggregate_sha256_hex="
      << manifest.founding_dsl_bundle_aggregate_sha256_hex << "\n";
  out << "file_count=" << manifest.files.size() << "\n";
  for (std::size_t i = 0; i < manifest.files.size(); ++i) {
    out << founding_bundle_file_key(i, "source_path") << "="
        << manifest.files[i].source_path << "\n";
    out << founding_bundle_file_key(i, "snapshot_relpath") << "="
        << manifest.files[i].snapshot_relpath << "\n";
    out << founding_bundle_file_key(i, "sha256_hex") << "="
        << manifest.files[i].sha256_hex << "\n";
  }
  if (!out) {
    if (error) {
      *error =
          "cannot write founding DSL bundle manifest: " + manifest_path.string();
    }
    return false;
  }
  return true;
}

bool read_founding_dsl_bundle_manifest(
    const std::filesystem::path& store_root, std::string_view canonical_path,
    std::string_view component_id,
    founding_dsl_bundle_manifest_t* out, std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) {
      *error = "founding DSL bundle manifest output pointer is null";
    }
    return false;
  }
  *out = founding_dsl_bundle_manifest_t{};

  const auto manifest_path =
      founding_dsl_bundle_manifest_path(store_root, canonical_path, component_id);
  std::ifstream in(manifest_path, std::ios::binary);
  if (!in) {
    if (error) {
      *error =
          "founding DSL bundle manifest not found: " + manifest_path.string();
    }
    return false;
  }

  std::unordered_map<std::string, std::string> kv{};
  std::string line{};
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    const std::string key = trim_ascii(line.substr(0, eq));
    const std::string value = line.substr(eq + 1);
    if (!key.empty()) kv[key] = value;
  }
  if (!in.eof() && in.fail()) {
    if (error) {
      *error = "cannot parse founding DSL bundle manifest: " +
               manifest_path.string();
    }
    return false;
  }

  if (const auto it = kv.find("schema"); it != kv.end()) out->schema = it->second;
  if (out->schema != cuwacunu::hashimyei::kFoundingDslBundleManifestSchemaV1) {
    if (error) {
      *error = "unsupported founding DSL bundle manifest schema: " + out->schema;
    }
    return false;
  }
  if (const auto it = kv.find("component_id"); it != kv.end()) {
    out->component_id = it->second;
  }
  if (const auto it = kv.find("canonical_path"); it != kv.end()) {
    out->canonical_path = it->second;
  }
  if (const auto it = kv.find("hashimyei_name"); it != kv.end()) {
    out->hashimyei_name = it->second;
  }
  if (const auto it = kv.find("founding_dsl_bundle_aggregate_sha256_hex");
      it != kv.end()) {
    out->founding_dsl_bundle_aggregate_sha256_hex = it->second;
  }
  const auto count_it = kv.find("file_count");
  if (count_it == kv.end()) {
    if (error) *error = "founding DSL bundle manifest missing file_count";
    return false;
  }
  std::size_t file_count = 0;
  try {
    file_count = static_cast<std::size_t>(std::stoull(trim_ascii(count_it->second)));
  } catch (...) {
    if (error) *error = "founding DSL bundle manifest has invalid file_count";
    return false;
  }
  out->files.clear();
  out->files.reserve(file_count);
  for (std::size_t i = 0; i < file_count; ++i) {
    founding_dsl_bundle_manifest_file_t file{};
    if (const auto it = kv.find(founding_bundle_file_key(i, "source_path"));
        it != kv.end()) {
      file.source_path = it->second;
    }
    if (const auto it = kv.find(founding_bundle_file_key(i, "snapshot_relpath"));
        it != kv.end()) {
      file.snapshot_relpath = it->second;
    }
    if (const auto it = kv.find(founding_bundle_file_key(i, "sha256_hex"));
        it != kv.end()) {
      file.sha256_hex = it->second;
    }
    out->files.push_back(std::move(file));
  }
  return true;
}

namespace {

[[nodiscard]] std::string kind_sha_key(
    cuwacunu::hashimyei::hashimyei_kind_e kind, std::string_view sha256_hex) {
  return cuwacunu::hashimyei::hashimyei_kind_to_string(kind) + "|" +
         std::string(sha256_hex);
}

[[nodiscard]] bool is_known_schema(std::string_view schema) {
  if (schema == cuwacunu::hero::family_rank::kFamilyRankSchemaV1) return true;
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
  if (schema ==
      "tsi.wikimyei.representation.vicreg.transfer_matrix_evaluation.run.v1") {
    return true;
  }
  if (schema == "piaabo.torch_compat.network_analytics.v5") {
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
  return normalize_hashimyei_name_or_same(canonical.substr(dot + 1));
}

[[nodiscard]] std::string component_family_from_parts(std::string_view canonical_path,
                                                      std::string_view hashimyei) {
  const std::string normalized_canonical =
      normalize_hashimyei_canonical_path_or_same(canonical_path);
  const std::string normalized_hash = normalize_hashimyei_name_or_same(hashimyei);
  if (!hashimyei.empty() && !canonical_path.empty()) {
    std::string family = normalized_canonical;
    const std::string suffix = "." + normalized_hash;
    if (family.size() > suffix.size() &&
        family.compare(family.size() - suffix.size(), suffix.size(), suffix) ==
            0) {
      family.resize(family.size() - suffix.size());
    }
    if (!family.empty()) return family;
  }
  if (!normalized_canonical.empty()) {
    const std::string family = normalized_canonical;
    if (!family.empty()) {
      return family;
    }
  }
  return {};
}

[[nodiscard]] std::string component_active_pointer_key(std::string_view canonical_path,
                                                       std::string_view family,
                                                       std::string_view hashimyei,
                                                       std::string_view contract_hash) {
  const std::string normalized_hash = normalize_hashimyei_name_or_same(hashimyei);
  std::string canonical_key_path =
      normalize_hashimyei_canonical_path_or_same(canonical_path);
  if (!normalized_hash.empty()) {
    canonical_key_path =
        component_family_from_parts(canonical_key_path, normalized_hash);
  }
  const std::string family_key =
      family.empty() ? component_family_from_parts(canonical_key_path, normalized_hash)
                     : normalize_hashimyei_canonical_path_or_same(family);
  return canonical_key_path + "|" + family_key + "|" +
         trim_ascii(contract_hash);
}

[[nodiscard]] bool parse_component_active_pointer_key(
    std::string_view key, std::string* out_canonical_path,
    std::string* out_family, std::string* out_contract_hash) {
  const std::size_t first_sep = key.find('|');
  if (first_sep == std::string::npos) return false;
  const std::size_t second_sep = key.find('|', first_sep + 1);
  if (second_sep == std::string::npos) return false;
  if (out_canonical_path) {
    *out_canonical_path = std::string(key.substr(0, first_sep));
  }
  if (out_family) {
    *out_family =
        std::string(key.substr(first_sep + 1, second_sep - first_sep - 1));
  }
  if (out_contract_hash) {
    *out_contract_hash = std::string(key.substr(second_sep + 1));
  }
  return true;
}

[[nodiscard]] bool is_family_rank_schema(std::string_view schema) {
  return schema == cuwacunu::hero::family_rank::kFamilyRankSchemaV1;
}

[[nodiscard]] std::string family_rank_record_id(std::string_view family,
                                                std::string_view contract_hash,
                                                std::string_view artifact_sha256) {
  return cuwacunu::hero::family_rank::scope_key(family, contract_hash) + "|" +
         std::string(artifact_sha256);
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
    if (parts[i] == "contracts") {
      const std::string token = trim_ascii(parts[i + 1]);
      if (token.rfind("c.", 0) == 0) return {};
      return token;
    }
  }
  return {};
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

[[nodiscard]] std::string compact_lock_owner_description(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  bool pending_separator = false;
  for (char c : text) {
    if (c == '\n' || c == '\r') {
      pending_separator = !out.empty();
      continue;
    }
    if (pending_separator) {
      if (!out.empty() && out.back() != ' ' && out.back() != ';') out += "; ";
      pending_separator = false;
    }
    out.push_back(c);
  }
  return trim_ascii(out);
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

void populate_runtime_report_entry_header_fields_(
    const std::unordered_map<std::string, std::string>& kv,
    report_fragment_entry_t* entry) {
  if (!entry) return;
  if (entry->source_label.empty()) {
    if (const auto it = kv.find("source_label"); it != kv.end()) {
      entry->source_label = trim_ascii(it->second);
    }
  }
  cuwacunu::piaabo::latent_lineage_state::runtime_report_header_t header{};
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_report_header_from_kv(
          kv, &header, nullptr)) {
    return;
  }
  if (entry->semantic_taxon.empty()) {
    entry->semantic_taxon = trim_ascii(header.semantic_taxon);
  }
  if (entry->report_canonical_path.empty()) {
    entry->report_canonical_path = trim_ascii(header.context.canonical_path);
  }
  if (entry->binding_id.empty()) {
    entry->binding_id = trim_ascii(header.context.binding_id);
  }
  if (entry->source_runtime_cursor.empty()) {
    entry->source_runtime_cursor =
        trim_ascii(header.context.source_runtime_cursor);
  }
  if (!entry->has_wave_cursor && header.context.has_wave_cursor) {
    entry->has_wave_cursor = true;
    entry->wave_cursor = header.context.wave_cursor;
  }
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
  cuwacunu::hashimyei::hashimyei_t normalized_campaign = campaign_identity;
  normalize_identity_hex_name_inplace(&normalized_campaign);
  wave_contract_binding_t normalized_binding = wave_contract_binding;
  normalize_identity_hex_name_inplace(&normalized_binding.identity);
  normalize_identity_hex_name_inplace(&normalized_binding.contract);
  normalize_identity_hex_name_inplace(&normalized_binding.wave);
  std::ostringstream seed;
  seed << normalized_campaign.schema << "|"
       << cuwacunu::hashimyei::hashimyei_kind_to_string(normalized_campaign.kind) << "|"
       << normalized_campaign.name << "|" << normalized_campaign.ordinal << "|"
       << normalized_campaign.hash_sha256_hex << "|"
       << normalized_binding.identity.schema << "|"
       << cuwacunu::hashimyei::hashimyei_kind_to_string(
              normalized_binding.identity.kind)
       << "|" << normalized_binding.identity.name << "|"
       << normalized_binding.identity.ordinal << "|"
       << normalized_binding.identity.hash_sha256_hex << "|"
       << normalized_binding.contract.hash_sha256_hex << "|"
       << normalized_binding.wave.hash_sha256_hex << "|"
       << normalized_binding.binding_id << "|" << started_at_ms;
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

  std::uint64_t dependency_count = 0;
  if (parse_u64(kv["dependency_count"], &dependency_count)) {
    out->dependency_files.reserve(static_cast<std::size_t>(dependency_count));
    for (std::uint64_t i = 0; i < dependency_count; ++i) {
      std::ostringstream pfx;
      pfx << "dependency_" << std::setw(4) << std::setfill('0') << i << "_";
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
      c.family = kv[pfx.str() + "family"];
      c.hashimyei = kv[pfx.str() + "hashimyei"];
      if (!c.canonical_path.empty() || !c.family.empty() || !c.hashimyei.empty()) {
        out->components.push_back(std::move(c));
      }
    }
  }

  if (out->schema.empty() || out->run_id.empty()) {
    set_error(error, "invalid run manifest: missing schema or run_id");
    return false;
  }
  normalize_run_manifest_inplace(out);
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
                            ? normalize_hashimyei_canonical_path_or_same(
                                  kv.at("canonical_path"))
                            : std::string{};
  out->family = kv.count("family") != 0 ? kv.at("family") : std::string{};
  if (!parse_identity_kv(kv, "hashimyei_identity", true,
                         &out->hashimyei_identity,
                         error)) {
    return false;
  }
  if (!parse_identity_kv(kv, "contract_identity", true, &out->contract_identity,
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
  out->founding_revision_id = kv.count("founding_revision_id") != 0
                                ? kv.at("founding_revision_id")
                                : std::string{};
  out->founding_dsl_source_path =
      kv.count("founding_dsl_source_path") != 0
          ? kv.at("founding_dsl_source_path")
          : std::string{};
  out->founding_dsl_source_sha256_hex =
      kv.count("founding_dsl_source_sha256_hex") != 0
          ? kv.at("founding_dsl_source_sha256_hex")
          : std::string{};
  out->docking_signature_sha256_hex =
      kv.count("docking_signature_sha256_hex") != 0
          ? kv.at("docking_signature_sha256_hex")
          : std::string{};
  out->lineage_state = kv.count("lineage_state") != 0
                           ? kv.at("lineage_state")
                           : std::string{"active"};
  out->replaced_by = kv.count("replaced_by") != 0 ? kv.at("replaced_by") : std::string{};
  (void)parse_u64(kv.count("created_at_ms") != 0 ? kv.at("created_at_ms") : "0",
                  &out->created_at_ms);
  (void)parse_u64(kv.count("updated_at_ms") != 0 ? kv.at("updated_at_ms") : "0",
                  &out->updated_at_ms);
  if (out->family.empty()) {
    out->family =
        component_family_from_parts(out->canonical_path, out->hashimyei_identity.name);
  }

  normalize_component_manifest_inplace(out);
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
  std::string identity_error;
  if (!cuwacunu::hashimyei::validate_hashimyei(manifest.hashimyei_identity,
                                                &identity_error)) {
    set_error(error, "component manifest hashimyei_identity invalid: " +
                         identity_error);
    return false;
  }
  if (manifest.hashimyei_identity.kind !=
      cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE) {
    set_error(error,
              "component manifest hashimyei_identity.kind must be TSIEMENE");
    return false;
  }
  if (!cuwacunu::hashimyei::validate_hashimyei(manifest.contract_identity,
                                                &identity_error)) {
    set_error(error, "component manifest contract_identity invalid: " +
                          identity_error);
    return false;
  }
  if (manifest.contract_identity.kind !=
      cuwacunu::hashimyei::hashimyei_kind_e::CONTRACT) {
    set_error(error, "component manifest contract_identity.kind must be CONTRACT");
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
  if (manifest.founding_dsl_source_path.empty()) {
    set_error(error,
              "component manifest missing founding_dsl_source_path");
    return false;
  }
  if (!is_hex_64(manifest.founding_dsl_source_sha256_hex)) {
    set_error(error,
              "component manifest founding_dsl_source_sha256_hex must be 64 "
              "hex chars");
    return false;
  }
  if (!trim_ascii(manifest.docking_signature_sha256_hex).empty() &&
      !is_hex_64(manifest.docking_signature_sha256_hex)) {
    set_error(error,
              "component manifest docking_signature_sha256_hex must be 64 "
              "hex chars when present");
    return false;
  }

  const std::string lineage_state = trim_ascii(manifest.lineage_state);
  if (lineage_state != "active" && lineage_state != "deprecated" &&
      lineage_state != "replaced" && lineage_state != "tombstone") {
    set_error(error, "component manifest lineage_state must be active/deprecated/replaced/tombstone");
    return false;
  }
  if (lineage_state == "replaced" && manifest.replaced_by.empty()) {
    set_error(error, "component manifest lineage_state=replaced requires replaced_by");
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

[[nodiscard]] bool is_pre_hard_cut_component_manifest_payload(
    std::string_view payload) {
  return payload.find("schema=hashimyei.component.manifest.v2") !=
             std::string_view::npos &&
         payload.find("canonical_path=") != std::string_view::npos &&
         payload.find("founding_revision_id=") != std::string_view::npos &&
         payload.find("docking_signature_sha256_hex=") !=
             std::string_view::npos &&
         payload.find("founding_dsl_source_path=") == std::string_view::npos;
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
  if (is_pre_hard_cut_component_manifest_payload(payload)) {
    set_error(error,
              "pre-hard-cut component manifest is not supported after the hard cut");
    return false;
  }

  std::unordered_map<std::string, std::string> kv;
  (void)parse_latent_lineage_state_payload(payload, &kv);
  if (!parse_component_manifest_kv(kv, out, error)) return false;
  return true;
}

std::string compute_component_manifest_id(const component_manifest_t& manifest) {
  component_manifest_t normalized = manifest;
  normalize_component_manifest_inplace(&normalized);
  std::ostringstream seed;
  seed << normalized.canonical_path << "|" << normalized.family << "|"
       << normalized.hashimyei_identity.schema << "|"
       << cuwacunu::hashimyei::hashimyei_kind_to_string(
              normalized.hashimyei_identity.kind)
       << "|" << normalized.hashimyei_identity.name << "|"
       << normalized.hashimyei_identity.ordinal << "|"
       << normalized.hashimyei_identity.hash_sha256_hex << "|";
  if (normalized.parent_identity.has_value()) {
    seed << normalized.parent_identity->schema << "|"
         << cuwacunu::hashimyei::hashimyei_kind_to_string(
                normalized.parent_identity->kind)
         << "|" << normalized.parent_identity->name << "|"
         << normalized.parent_identity->ordinal << "|"
         << normalized.parent_identity->hash_sha256_hex << "|";
  }
  seed << normalized.revision_reason << "|" << normalized.founding_revision_id
       << "|"
       << normalized.contract_identity.hash_sha256_hex << "|"
       << normalized.founding_dsl_source_sha256_hex << "|"
       << normalized.docking_signature_sha256_hex;
  std::string digest;
  if (!sha256_hex_bytes(seed.str(), &digest) || digest.empty()) {
    return "component.invalid";
  }
  return "component." + digest.substr(0, 24);
}

bool save_component_manifest(const std::filesystem::path& store_root,
                             const component_manifest_t& manifest,
                             std::filesystem::path* out_path,
                             std::string* error) {
  clear_error(error);
  component_manifest_t normalized = manifest;
  normalize_component_manifest_inplace(&normalized);
  std::string validate_error;
  if (!validate_component_manifest(normalized, &validate_error)) {
    set_error(error, "component manifest invalid: " + validate_error);
    return false;
  }

  const std::string component_id = compute_component_manifest_id(normalized);
  if (trim_ascii(component_id).empty() || component_id == "component.invalid") {
    set_error(error, "component manifest id is invalid");
    return false;
  }

  std::error_code ec;
  const auto dir =
      component_manifest_directory(store_root, normalized.canonical_path,
                                   component_id);
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    set_error(error, "cannot create component manifest directory: " +
                         dir.string());
    return false;
  }

  const auto target =
      component_manifest_path(store_root, normalized.canonical_path,
                              component_id);
  const auto tmp = target.string() + ".tmp";
  std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
  if (!out) {
    set_error(error,
              "cannot write component manifest temporary file: " + tmp);
    return false;
  }
  out << component_manifest_payload(normalized);
  out.flush();
  if (!out) {
    set_error(error,
              "cannot flush component manifest temporary file: " + tmp);
    return false;
  }
  out.close();

  std::filesystem::rename(tmp, target, ec);
  if (ec) {
    std::filesystem::remove(tmp, ec);
    set_error(error,
              "cannot rename component manifest into place: " +
                  target.string());
    return false;
  }
  if (out_path) *out_path = target;
  return true;
}

bool save_run_manifest(const std::filesystem::path& store_root,
                       const run_manifest_t& manifest,
                       std::filesystem::path* out_path, std::string* error) {
  clear_error(error);
  run_manifest_t normalized = manifest;
  normalize_run_manifest_inplace(&normalized);
  if (normalized.schema != cuwacunu::hashimyei::kRunManifestSchemaV2) {
    set_error(error, std::string("run manifest schema must be ") +
                         cuwacunu::hashimyei::kRunManifestSchemaV2);
    return false;
  }
  if (normalized.run_id.empty()) {
    set_error(error, "run manifest requires run_id");
    return false;
  }
  std::string identity_error;
  if (!cuwacunu::hashimyei::validate_hashimyei(normalized.campaign_identity,
                                                &identity_error)) {
    set_error(error, "run manifest campaign_identity invalid: " + identity_error);
    return false;
  }
  if (!validate_wave_contract_binding(normalized.wave_contract_binding,
                                      &identity_error)) {
    set_error(error, "run manifest wave_contract_binding invalid: " + identity_error);
    return false;
  }

  std::error_code ec;
  const auto dir = run_manifest_directory(store_root, normalized.run_id);
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    set_error(error, "cannot create run manifest directory: " + dir.string());
    return false;
  }

  const auto target = run_manifest_path(store_root, normalized.run_id);
  const auto tmp = target.string() + ".tmp";
  std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
  if (!out) {
    set_error(error, "cannot write run manifest temporary file: " + tmp);
    return false;
  }
  out << run_manifest_payload(normalized);
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
  const std::uint64_t open_started_at_ms = now_ms_utc();
  const auto log_open_finish = [&](bool ok,
                                   std::string_view error_message = {}) {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", options.catalog_path.string());
    cuwacunu::hero::mcp_observability::append_json_bool_field(
        extra, "encrypted", options.encrypted);
    cuwacunu::hero::mcp_observability::append_json_bool_field(extra, "ok", ok);
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "duration_ms", now_ms_utc() - open_started_at_ms);
    if (!error_message.empty()) {
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "error", error_message);
    }
    log_hashimyei_catalog_event("catalog_open_finish", extra.str());
  };
  {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", options.catalog_path.string());
    cuwacunu::hero::mcp_observability::append_json_bool_field(
        extra, "encrypted", options.encrypted);
    log_hashimyei_catalog_event("catalog_open_begin", extra.str());
  }
  if (db_ != nullptr) {
    set_error(error, "catalog already open");
    log_open_finish(false, "catalog already open");
    return false;
  }

  std::error_code ec;
  const auto parent = options.catalog_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      set_error(error, "cannot create catalog directory: " + parent.string());
      log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                      : std::string_view{});
      return false;
    }
  }

  const auto remove_catalog_for_recovery =
      [&](std::string* out_error) -> bool {
        if (out_error) out_error->clear();
        std::error_code rm_ec{};
        std::filesystem::remove(options.catalog_path, rm_ec);
        if (rm_ec && std::filesystem::exists(options.catalog_path)) {
          set_error(out_error,
                    "cannot remove damaged catalog: " +
                        options.catalog_path.string());
          return false;
        }
        return true;
      };

  for (int recovery_attempt = 0; recovery_attempt < 2; ++recovery_attempt) {
    int rc = IDYDB_ERROR;
    for (int attempt = 0; attempt <= kCatalogOpenBusyRetryCount; ++attempt) {
      if (options.encrypted) {
        if (options.passphrase.empty()) {
          set_error(error, "encrypted catalog requires passphrase");
          log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                          : std::string_view{});
          return false;
        }
        rc = idydb_open_encrypted(options.catalog_path.string().c_str(), &db_,
                                  IDYDB_CREATE, options.passphrase.c_str());
      } else {
        rc = idydb_open(options.catalog_path.string().c_str(), &db_,
                        IDYDB_CREATE);
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
              : ("idydb rc=" + std::to_string(rc) + " (" +
                 idydb_rc_label(rc) + ")");
      if (rc == IDYDB_BUSY) {
        detail += "; catalog lock is held by another process";
      }
      if (db_) {
        (void)idydb_close(&db_);
        db_ = nullptr;
      }
      if (recovery_attempt == 0 &&
          remove_catalog_for_recovery(nullptr)) {
        continue;
      }
      set_error(error, "cannot open catalog: " + detail);
      log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                      : std::string_view{});
      return false;
    }

    options_ = options;
    if (ensure_catalog_header_(error) && rebuild_indexes(error)) {
      opened_at_ms_ = now_ms_utc();
      log_open_finish(true);
      return true;
    }

    std::string ignored;
    (void)close(&ignored);
    if (recovery_attempt == 0 &&
        remove_catalog_for_recovery(nullptr)) {
      continue;
    }
    log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                    : std::string_view{});
    return false;
  }

  set_error(error, "cannot open catalog after recovery attempts");
  log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                  : std::string_view{});
  return false;
}

bool hashimyei_catalog_store_t::close(std::string* error) {
  clear_error(error);
  if (!db_) return true;
  const std::uint64_t close_started_at_ms = now_ms_utc();
  const std::uint64_t opened_at_ms = opened_at_ms_;
  {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", options_.catalog_path.string());
    log_hashimyei_catalog_event("catalog_close_begin", extra.str());
  }
  const int rc = idydb_close(&db_);
  db_ = nullptr;
  if (rc != IDYDB_DONE) {
    opened_at_ms_ = 0;
    set_error(error, "idydb_close failed");
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", options_.catalog_path.string());
    cuwacunu::hero::mcp_observability::append_json_bool_field(extra, "ok",
                                                              false);
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "duration_ms", now_ms_utc() - close_started_at_ms);
    if (opened_at_ms != 0) {
      cuwacunu::hero::mcp_observability::append_json_uint64_field(
          extra, "catalog_hold_ms", now_ms_utc() - opened_at_ms);
    }
    if (error && !error->empty()) {
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "error", *error);
    }
    log_hashimyei_catalog_event("catalog_close_finish", extra.str());
    return false;
  }
  opened_at_ms_ = 0;
  next_row_hint_ = 0;
  runs_by_id_.clear();
  run_ids_.clear();
  ledger_report_fragment_ids_.clear();
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
  explicit_family_rank_by_scope_.clear();
  {
    const std::uint64_t closed_at_ms = now_ms_utc();
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", options_.catalog_path.string());
    cuwacunu::hero::mcp_observability::append_json_bool_field(extra, "ok", true);
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "duration_ms", closed_at_ms - close_started_at_ms);
    if (opened_at_ms != 0) {
      cuwacunu::hero::mcp_observability::append_json_uint64_field(
          extra, "catalog_hold_ms", closed_at_ms - opened_at_ms);
    }
    log_hashimyei_catalog_event("catalog_close_finish", extra.str());
  }
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
  const idydb_column_row_sizing row =
      buffer_rows_
          ? (buffered_row_start_ != 0
                 ? static_cast<idydb_column_row_sizing>(
                       buffered_row_start_ + buffered_rows_.size())
                 : ((buffered_row_start_ =
                         (next_row_hint_ != 0)
                             ? next_row_hint_
                             : idydb_column_next_row(&db_, kColRecordKind)),
                    buffered_row_start_))
          : ((next_row_hint_ != 0) ? next_row_hint_
                                   : idydb_column_next_row(&db_, kColRecordKind));
  if (row == 0) {
    set_error(error, "failed to resolve next row");
    return false;
  }
  const std::string canonical_key =
      normalize_hashimyei_canonical_path_or_same(canonical_path);
  const std::string hash_key = normalize_hashimyei_name_or_same(hashimyei);
  if (buffer_rows_) {
    buffered_row_t buffered{};
    buffered.record_kind = std::string(record_kind);
    buffered.record_id = std::string(record_id);
    buffered.run_id = std::string(run_id);
    buffered.canonical_path = canonical_key;
    buffered.hashimyei = hash_key;
    buffered.schema = std::string(schema);
    buffered.metric_key = std::string(metric_key);
    buffered.metric_num = metric_num;
    buffered.has_metric_num = std::isfinite(metric_num);
    buffered.metric_txt = std::string(metric_txt);
    buffered.report_fragment_sha256 = std::string(report_fragment_sha256);
    buffered.path = std::string(path);
    buffered.ts_ms = std::string(ts_ms);
    buffered.payload_json = std::string(payload_json);
    buffered_rows_.push_back(std::move(buffered));
    next_row_hint_ = row + 1;
    return true;
  }
  const auto insert_text_if_present =
      [&](idydb_column_row_sizing column, std::string_view value) {
        return value.empty() || insert_text(&db_, column, row, value, error);
      };
  const auto insert_num_if_present =
      [&](idydb_column_row_sizing column, double value) {
        return std::isnan(value) || insert_num(&db_, column, row, value, error);
      };
  if (!insert_text_if_present(kColRecordKind, record_kind)) return false;
  if (!insert_text_if_present(kColRecordId, record_id)) return false;
  if (!insert_text_if_present(kColRunId, run_id)) return false;
  if (!insert_text_if_present(kColCanonicalPath, canonical_key)) return false;
  if (!insert_text_if_present(kColHashimyei, hash_key)) return false;
  if (!insert_text_if_present(kColSchema, schema)) return false;
  if (!insert_text_if_present(kColMetricKey, metric_key)) return false;
  if (!insert_num_if_present(kColMetricNum, metric_num)) return false;
  if (!insert_text_if_present(kColMetricTxt, metric_txt)) return false;
  if (!insert_text_if_present(kColReportFragmentSha256, report_fragment_sha256)) return false;
  if (!insert_text_if_present(kColPath, path)) return false;
  if (!insert_text_if_present(kColTsMs, ts_ms)) return false;
  if (!insert_text_if_present(kColPayload, payload_json)) return false;
  next_row_hint_ = row + 1;
  return true;
}

bool hashimyei_catalog_store_t::flush_buffered_rows_(std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }
  if (buffered_rows_.empty()) {
    buffered_row_start_ = 0;
    return true;
  }
  const idydb_column_row_sizing start_row =
      (buffered_row_start_ != 0) ? buffered_row_start_
                                 : ((next_row_hint_ != 0)
                                        ? static_cast<idydb_column_row_sizing>(
                                              next_row_hint_ - buffered_rows_.size())
                                        : idydb_column_next_row(&db_, kColRecordKind));
  if (start_row == 0) {
    set_error(error, "failed to resolve buffered row start");
    return false;
  }
  const auto row_for = [&](std::size_t index) {
    return static_cast<idydb_column_row_sizing>(start_row + index);
  };
  const idydb_column_row_sizing existing_next_row =
      idydb_column_next_row(&db_, kColRecordKind);
  if (start_row == 2 && existing_next_row == 2) {
    std::vector<idydb_bulk_cell> cells;
    cells.reserve(12 + buffered_rows_.size() * 13);
    std::vector<std::string> owned_header_text;
    owned_header_text.reserve(12);
    const auto append_header_text = [&](idydb_column_row_sizing column) {
      const std::string value = as_text_or_empty(&db_, column, 1);
      if (value.empty()) return;
      owned_header_text.push_back(value);
      idydb_bulk_cell cell{};
      cell.column = column;
      cell.row = 1;
      cell.type = IDYDB_CHAR;
      cell.value.s = owned_header_text.back().c_str();
      cells.push_back(cell);
    };
    const auto append_text_cell = [&](idydb_column_row_sizing column,
                                      idydb_column_row_sizing row,
                                      const std::string& value) {
      if (value.empty()) return;
      idydb_bulk_cell cell{};
      cell.column = column;
      cell.row = row;
      cell.type = IDYDB_CHAR;
      cell.value.s = value.c_str();
      cells.push_back(cell);
    };
    const auto append_num_cell = [&](idydb_column_row_sizing column,
                                     idydb_column_row_sizing row,
                                     double value, bool present) {
      if (!present) return;
      idydb_bulk_cell cell{};
      cell.column = column;
      cell.row = row;
      cell.type = IDYDB_FLOAT;
      cell.value.f = static_cast<float>(value);
      cells.push_back(cell);
    };

    append_header_text(kColRecordKind);
    append_header_text(kColRecordId);
    append_header_text(kColRunId);
    append_header_text(kColCanonicalPath);
    append_header_text(kColHashimyei);
    append_header_text(kColSchema);
    append_header_text(kColMetricKey);
    append_header_text(kColMetricTxt);
    append_header_text(kColReportFragmentSha256);
    append_header_text(kColPath);
    append_header_text(kColTsMs);
    append_header_text(kColPayload);

    for (std::size_t i = 0; i < buffered_rows_.size(); ++i) {
      const auto row = row_for(i);
      const auto& buffered = buffered_rows_[i];
      append_text_cell(kColRecordKind, row, buffered.record_kind);
      append_text_cell(kColRecordId, row, buffered.record_id);
      append_text_cell(kColRunId, row, buffered.run_id);
      append_text_cell(kColCanonicalPath, row, buffered.canonical_path);
      append_text_cell(kColHashimyei, row, buffered.hashimyei);
      append_text_cell(kColSchema, row, buffered.schema);
      append_text_cell(kColMetricKey, row, buffered.metric_key);
      append_num_cell(kColMetricNum, row, buffered.metric_num,
                      buffered.has_metric_num);
      append_text_cell(kColMetricTxt, row, buffered.metric_txt);
      append_text_cell(kColReportFragmentSha256, row,
                       buffered.report_fragment_sha256);
      append_text_cell(kColPath, row, buffered.path);
      append_text_cell(kColTsMs, row, buffered.ts_ms);
      append_text_cell(kColPayload, row, buffered.payload_json);
    }

    const int rc =
        idydb_replace_with_cells(&db_, cells.data(), cells.size());
    if (rc != IDYDB_SUCCESS) {
      const char* msg = idydb_errmsg(&db_);
      set_error(error, "idydb_replace_with_cells failed: " +
                           std::string(msg ? msg : "<no error message>"));
      return false;
    }
    next_row_hint_ = static_cast<idydb_column_row_sizing>(
        start_row + buffered_rows_.size());
    buffered_rows_.clear();
    buffered_row_start_ = 0;
    return true;
  }
  const auto flush_text_column =
      [&](idydb_column_row_sizing column, auto accessor) -> bool {
    for (std::size_t i = 0; i < buffered_rows_.size(); ++i) {
      const auto& value = accessor(buffered_rows_[i]);
      if (!value.empty() && !insert_text(&db_, column, row_for(i), value, error)) {
        return false;
      }
    }
    return true;
  };
  const auto flush_num_column =
      [&](idydb_column_row_sizing column, auto accessor,
          auto present_accessor) -> bool {
    for (std::size_t i = 0; i < buffered_rows_.size(); ++i) {
      if (!present_accessor(buffered_rows_[i])) continue;
      if (!insert_num(&db_, column, row_for(i), accessor(buffered_rows_[i]), error)) {
        return false;
      }
    }
    return true;
  };

  if (!flush_text_column(kColRecordKind,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.record_kind;
                         })) {
    return false;
  }
  if (!flush_text_column(kColRecordId,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.record_id;
                         })) {
    return false;
  }
  if (!flush_text_column(kColRunId,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.run_id;
                         })) {
    return false;
  }
  if (!flush_text_column(kColCanonicalPath,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.canonical_path;
                         })) {
    return false;
  }
  if (!flush_text_column(kColHashimyei,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.hashimyei;
                         })) {
    return false;
  }
  if (!flush_text_column(kColSchema,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.schema;
                         })) {
    return false;
  }
  if (!flush_text_column(kColMetricKey,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.metric_key;
                         })) {
    return false;
  }
  if (!flush_num_column(
          kColMetricNum,
          [](const buffered_row_t& row) { return row.metric_num; },
          [](const buffered_row_t& row) { return row.has_metric_num; })) {
    return false;
  }
  if (!flush_text_column(kColMetricTxt,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.metric_txt;
                         })) {
    return false;
  }
  if (!flush_text_column(
          kColReportFragmentSha256,
          [](const buffered_row_t& row) -> const std::string& {
            return row.report_fragment_sha256;
          })) {
    return false;
  }
  if (!flush_text_column(kColPath,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.path;
                         })) {
    return false;
  }
  if (!flush_text_column(kColTsMs,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.ts_ms;
                         })) {
    return false;
  }
  if (!flush_text_column(kColPayload,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.payload_json;
                         })) {
    return false;
  }

  buffered_rows_.clear();
  buffered_row_start_ = 0;
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
  *out_exists =
      ledger_report_fragment_ids_.count(std::string(report_fragment_sha256)) != 0;
  return true;
}

bool hashimyei_catalog_store_t::append_ledger_(std::string_view report_fragment_sha256,
                                               std::string_view path,
                                               std::string* error) {
  if (!append_row_(cuwacunu::hero::schema::kRecordKindLEDGER,
                   std::string(report_fragment_sha256), "", "", "", "",
                   "ingest_version", static_cast<double>(options_.ingest_version),
                   "", report_fragment_sha256, path, std::to_string(now_ms_utc()), "{}",
                   error)) {
    return false;
  }
  ledger_report_fragment_ids_.insert(std::string(report_fragment_sha256));
  return true;
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
      set_error(error, "identity.name is not a valid 0x... hex ordinal");
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

void hashimyei_catalog_store_t::observe_identity_(
    const cuwacunu::hashimyei::hashimyei_t& identity) {
  if (identity.name.empty()) return;

  const int key = static_cast<int>(identity.kind);
  const std::uint64_t observed_next = identity.ordinal + 1;
  const auto it_counter = kind_counters_.find(key);
  if (it_counter == kind_counters_.end() || observed_next > it_counter->second) {
    kind_counters_[key] = observed_next;
  }

  if (!cuwacunu::hashimyei::hashimyei_kind_requires_sha256(identity.kind) ||
      identity.hash_sha256_hex.empty()) {
    return;
  }

  const std::string identity_key =
      kind_sha_key(identity.kind, identity.hash_sha256_hex);
  const auto it_known = hash_identity_by_kind_sha_.find(identity_key);
  if (it_known == hash_identity_by_kind_sha_.end() ||
      identity.ordinal < it_known->second.ordinal) {
    hash_identity_by_kind_sha_[identity_key] = identity;
  }
}

void hashimyei_catalog_store_t::observe_component_(
    const component_state_t& component) {
  if (component.component_id.empty()) return;

  observe_identity_(component.manifest.hashimyei_identity);
  if (component.manifest.parent_identity.has_value()) {
    observe_identity_(*component.manifest.parent_identity);
  }
  observe_identity_(component.manifest.contract_identity);

  components_by_id_[component.component_id] = component;

  const auto pick_latest =
      [&](std::unordered_map<std::string, std::string>* map,
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
  if (!component.manifest.hashimyei_identity.name.empty()) {
    pick_latest(&latest_component_by_hashimyei_,
                component.manifest.hashimyei_identity.name);
  }
}

void hashimyei_catalog_store_t::refresh_active_component_views_() {
  active_component_by_key_.clear();
  active_component_by_canonical_.clear();

  for (const auto& [pointer_key, pointer_hashimyei] : active_hashimyei_by_key_) {
    std::string canonical_path{};
    std::string family{};
    std::string contract_hash{};
    if (!parse_component_active_pointer_key(pointer_key, &canonical_path, &family,
                                            &contract_hash)) {
      continue;
    }

    component_state_t best{};
    bool found = false;
    for (const auto& [_, component] : components_by_id_) {
      const std::string component_contract_hash =
          cuwacunu::hero::hashimyei::contract_hash_from_identity(
              component.manifest.contract_identity);
      const std::string component_key = component_active_pointer_key(
          component.manifest.canonical_path, component.manifest.family,
          component.manifest.hashimyei_identity.name, component_contract_hash);
      if (component_key != pointer_key) continue;
      if (!family.empty() && component.manifest.family != family) continue;
      if (!contract_hash.empty() && component_contract_hash != contract_hash) {
        continue;
      }
      if (component.manifest.hashimyei_identity.name != pointer_hashimyei) {
        continue;
      }
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
    active_component_by_canonical_[best.manifest.canonical_path] =
        best.component_id;
  }
}

void hashimyei_catalog_store_t::refresh_component_family_ranks_() {
  for (auto& [_, component] : components_by_id_) {
    component.family_rank.reset();
    const std::string scope_key = cuwacunu::hero::family_rank::scope_key(
        component.manifest.family,
        cuwacunu::hero::hashimyei::contract_hash_from_identity(
            component.manifest.contract_identity));
    const auto it_rank = explicit_family_rank_by_scope_.find(scope_key);
    if (it_rank == explicit_family_rank_by_scope_.end()) continue;
    component.family_rank = cuwacunu::hero::family_rank::rank_for_hashimyei(
        it_rank->second, component.manifest.hashimyei_identity.name);
  }
}

bool hashimyei_catalog_store_t::ingest_run_manifest_file_(const std::filesystem::path& path,
                                                          std::string* error) {
  clear_error(error);
  run_manifest_t m{};
  if (!load_run_manifest(path, &m, error)) return false;

  if (run_ids_.count(m.run_id) == 0) {
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
    run_ids_.insert(m.run_id);
  }
  dependency_files_by_run_id_[m.run_id] = m.dependency_files;
  observe_identity_(m.campaign_identity);
  observe_identity_(m.wave_contract_binding.identity);
  observe_identity_(m.wave_contract_binding.contract);
  observe_identity_(m.wave_contract_binding.wave);
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

  std::string payload;
  std::string read_error;
  if (!read_text_file(path, &payload, &read_error)) {
    set_error(error, "cannot read component manifest payload: " + read_error);
    return false;
  }
  if (is_pre_hard_cut_component_manifest_payload(payload)) {
    return true;
  }

  component_manifest_t manifest{};
  if (!load_component_manifest(path, &manifest, error)) {
    if (error != nullptr &&
        *error ==
            "pre-hard-cut component manifest is not supported after the hard cut") {
      error->clear();
      return true;
    }
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
                   manifest.hashimyei_identity.name, manifest.schema,
                   manifest.family,
                   std::numeric_limits<double>::quiet_NaN(), manifest.lineage_state,
                   report_fragment_sha, cp.string(), std::to_string(ts_ms), payload,
                   error)) {
    return false;
  }

  const std::string source_payload =
      "{\"component_edge_kind\":\"founding_dsl_source\"}";
  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_LINEAGE,
                   component_id + "|founding_dsl_source", "",
                   manifest.canonical_path, manifest.hashimyei_identity.name,
                   manifest.schema, manifest.founding_dsl_source_path,
                   std::numeric_limits<double>::quiet_NaN(),
                   manifest.founding_dsl_source_sha256_hex, "",
                   cp.string(), std::to_string(ts_ms), source_payload,
                   error)) {
    return false;
  }

  const std::filesystem::path store_root =
      store_root_from_catalog_path(options_.catalog_path);
  founding_dsl_bundle_manifest_t bundle_manifest{};
  std::string bundle_error{};
  if (read_founding_dsl_bundle_manifest(store_root, manifest.canonical_path,
                                        component_id, &bundle_manifest,
                                        &bundle_error)) {
    const std::string bundle_payload =
        std::string("{\"component_edge_kind\":\"founding_dsl_bundle\","
                    "\"founding_dsl_bundle_aggregate_sha256_hex\":\"") +
        bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex + "\"}";
    if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_LINEAGE,
                     component_id + "|founding_dsl_bundle", "",
                     manifest.canonical_path, manifest.hashimyei_identity.name,
                     manifest.schema,
                     founding_dsl_bundle_manifest_path(
                         store_root, manifest.canonical_path, component_id)
                         .string(),
                     std::numeric_limits<double>::quiet_NaN(),
                     bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex, "",
                     cp.string(),
                     std::to_string(ts_ms), bundle_payload, error)) {
      return false;
    }
  }

  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_REVISION, component_id + "|revision", "",
                   manifest.canonical_path, manifest.hashimyei_identity.name,
                   manifest.schema,
                   manifest.family, std::numeric_limits<double>::quiet_NaN(),
                   manifest.parent_identity.has_value()
                       ? std::string_view(manifest.parent_identity->name)
                       : std::string_view{},
                   "", cp.string(),
                   std::to_string(ts_ms), manifest.revision_reason, error)) {
    return false;
  }

  if (trim_ascii(manifest.lineage_state) == "active") {
    const std::string contract_hash =
        cuwacunu::hero::hashimyei::contract_hash_from_identity(
            manifest.contract_identity);
    const std::string pointer_key = component_active_pointer_key(
        manifest.canonical_path, manifest.family,
        manifest.hashimyei_identity.name, contract_hash);
    std::ostringstream pointer_payload;
    pointer_payload << "canonical_path=" << manifest.canonical_path << "\n";
    pointer_payload << "family=" << manifest.family << "\n";
    pointer_payload << "contract_hash=" << contract_hash << "\n";
    pointer_payload << "active_hashimyei=" << manifest.hashimyei_identity.name
                    << "\n";
    pointer_payload << "component_id=" << component_id << "\n";
    pointer_payload << "docking_signature_sha256_hex="
                    << manifest.docking_signature_sha256_hex << "\n";
    pointer_payload << "parent_hashimyei="
                    << (manifest.parent_identity.has_value()
                            ? std::string_view(manifest.parent_identity->name)
                            : std::string_view{})
                    << "\n";
    pointer_payload << "revision_reason=" << manifest.revision_reason << "\n";
    pointer_payload << "founding_revision_id=" << manifest.founding_revision_id
                    << "\n";
    if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_ACTIVE, pointer_key, "", manifest.canonical_path,
                     manifest.hashimyei_identity.name,
                     cuwacunu::hashimyei::kComponentActivePointerSchemaV2,
                     manifest.family, std::numeric_limits<double>::quiet_NaN(),
                     manifest.founding_revision_id, "", cp.string(),
                     std::to_string(ts_ms), pointer_payload.str(), error)) {
      return false;
    }
  }

  if (!append_ledger_(report_fragment_sha, cp.string(), error)) return false;

  component_state_t component{};
  component.component_id = component_id;
  component.ts_ms = ts_ms;
  component.manifest_path = cp.string();
  component.report_fragment_sha256 = report_fragment_sha;
  component.manifest = manifest;
  observe_component_(component);

  if (trim_ascii(manifest.lineage_state) == "active") {
    const std::string contract_hash =
        cuwacunu::hero::hashimyei::contract_hash_from_identity(
            manifest.contract_identity);
    const std::string pointer_key = component_active_pointer_key(
        manifest.canonical_path, manifest.family,
        manifest.hashimyei_identity.name, contract_hash);
    active_hashimyei_by_key_[pointer_key] = manifest.hashimyei_identity.name;
  }
  return true;
}

void hashimyei_catalog_store_t::populate_metrics_from_kv_(
    std::string_view report_fragment_id,
    const std::unordered_map<std::string, std::string>& kv) {
  if (report_fragment_id.empty()) return;
  auto& numeric = metrics_num_by_report_fragment_[std::string(report_fragment_id)];
  auto& text = metrics_txt_by_report_fragment_[std::string(report_fragment_id)];
  numeric.clear();
  text.clear();
  for (const auto& [k, v] : kv) {
    if (k == "schema" || k == "canonical_path" || k == "binding_id" ||
        k == "source_runtime_cursor" || k == "wave_cursor" ||
        k == "semantic_taxon" ||
        k == "component_instance_name" || k == "report_event" ||
        k == "source_label") {
      continue;
    }

    double num = 0.0;
    if (parse_double_token(v, &num)) {
      numeric.push_back({k, num});
    } else {
      text.push_back({k, v});
    }
  }
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

  std::filesystem::path cp = path;
  if (const auto can = canonicalized(path); can.has_value()) cp = *can;

  if (is_family_rank_schema(schema)) {
    cuwacunu::hero::family_rank::state_t rank_state{};
    if (!cuwacunu::hero::family_rank::parse_state_from_kv(kv, &rank_state, error)) {
      set_error(error, "family rank payload parse failure for " + path.string() +
                           ": " + (error ? *error : std::string{}));
      return false;
    }
    std::uint64_t ts_ms = rank_state.updated_at_ms;
    if (ts_ms == 0) ts_ms = now_ms_utc();
    const std::string record_id =
        family_rank_record_id(rank_state.family, rank_state.contract_hash,
                              report_fragment_sha);
    if (!append_row_(cuwacunu::hero::schema::kRecordKindFAMILY_RANK, record_id, "",
                     rank_state.family, "", rank_state.schema,
                     rank_state.contract_hash,
                     static_cast<double>(rank_state.assignments.size()),
                     rank_state.source_view_kind, report_fragment_sha, cp.string(),
                     std::to_string(ts_ms), payload, error)) {
      return false;
    }
    if (!append_ledger_(report_fragment_sha, cp.string(), error)) return false;
    explicit_family_rank_by_scope_[cuwacunu::hero::family_rank::scope_key(
        rank_state.family, rank_state.contract_hash)] = std::move(rank_state);
    return true;
  }

  cuwacunu::piaabo::latent_lineage_state::runtime_report_header_t header{};
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_report_header_from_kv(
          kv, &header, error)) {
    set_error(error, "runtime report_fragment header parse failure: " + path.string());
    return false;
  }
  const std::string report_canonical_path =
      trim_ascii(header.context.canonical_path);
  if (report_canonical_path.empty()) {
    set_error(error,
              "runtime report_fragment missing canonical_path: " +
                  path.string());
    return false;
  }
  std::string canonical_path = trim_ascii(kv["canonical_path"]);
  if (canonical_path.empty()) canonical_path = report_canonical_path;

  std::string hashimyei = kv["hashimyei"];
  if (hashimyei.empty()) hashimyei = maybe_hashimyei_from_canonical(canonical_path);

  std::string contract_hash = kv["contract_hash"];
  if (contract_hash.empty()) contract_hash = kv["runtime.contract_hash"];
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

  if (!append_row_(cuwacunu::hero::schema::kRecordKindREPORT_FRAGMENT, report_fragment_sha, "", canonical_path, hashimyei, schema,
                   "", std::numeric_limits<double>::quiet_NaN(), "", report_fragment_sha,
                   cp.string(), std::to_string(ts_ms), payload, error)) {
    return false;
  }
  report_fragment_entry_t entry{};
  entry.report_fragment_id = report_fragment_sha;
  entry.canonical_path = canonical_path;
  entry.hashimyei = hashimyei;
  entry.schema = schema;
  entry.report_fragment_sha256 = report_fragment_sha;
  entry.path = cp.string();
  entry.ts_ms = ts_ms;
  entry.payload_json = payload;
  populate_runtime_report_entry_header_fields_(kv, &entry);
  report_fragments_by_id_[entry.report_fragment_id] = entry;
  {
    const std::string key = join_key(entry.canonical_path, entry.schema);
    auto it = latest_report_fragment_by_key_.find(key);
    if (it == latest_report_fragment_by_key_.end()) {
      latest_report_fragment_by_key_[key] = entry.report_fragment_id;
    } else {
      const auto old_it = report_fragments_by_id_.find(it->second);
      if (old_it == report_fragments_by_id_.end() ||
          entry.ts_ms > old_it->second.ts_ms ||
          (entry.ts_ms == old_it->second.ts_ms &&
           entry.report_fragment_id > old_it->second.report_fragment_id)) {
        it->second = entry.report_fragment_id;
      }
    }
  }
  populate_metrics_from_kv_(report_fragment_sha, kv);
  return append_ledger_(report_fragment_sha, cp.string(), error);
}

bool hashimyei_catalog_store_t::acquire_ingest_lock_(
    const std::filesystem::path& store_root, ingest_lock_t* lock, std::string* error) {
  clear_error(error);
  const std::uint64_t acquire_started_at_ms = now_ms_utc();
  if (!lock) {
    set_error(error, "lock pointer is null");
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "wait_ms", now_ms_utc() - acquire_started_at_ms);
    if (error && !error->empty()) {
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "error", *error);
    }
    log_hashimyei_catalog_event("ingest_lock_failed", extra.str());
    return false;
  }
  lock->path.clear();
  lock->handle = nullptr;
  lock->acquired_at_ms = 0;
  std::error_code ec;
  const auto lock_dir = cuwacunu::hashimyei::catalog_directory(store_root);
  std::filesystem::create_directories(lock_dir, ec);
  if (ec) {
    set_error(error, "cannot create lock directory: " + lock_dir.string());
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "lock_dir", lock_dir.string());
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "wait_ms", now_ms_utc() - acquire_started_at_ms);
    if (error && !error->empty()) {
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "error", *error);
    }
    log_hashimyei_catalog_event("ingest_lock_failed", extra.str());
    return false;
  }
  const auto lock_path = lock_dir / ".ingest.lock";
  idydb_named_lock_options options{};
  options.shared = false;
  options.create_parent_directories = true;
  options.retry_count = 0;
  options.retry_delay_ms = 0;
  options.owner_label = "hashimyei_catalog_ingest";
  const std::string lock_path_string = lock_path.string();
  const int rc =
      idydb_named_lock_acquire(lock_path_string.c_str(), &lock->handle, &options);
  if (rc != IDYDB_SUCCESS) {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "lock_path", lock_path_string);
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "wait_ms", now_ms_utc() - acquire_started_at_ms);
    cuwacunu::hero::mcp_observability::append_json_int_field(extra, "idydb_rc",
                                                             rc);
    if (rc == IDYDB_BUSY) {
      char owner[1024] = {0};
      std::string owner_suffix{};
      if (idydb_named_lock_describe_owner(lock_path_string.c_str(), owner,
                                          sizeof(owner)) == IDYDB_SUCCESS) {
        const std::string compact_owner = compact_lock_owner_description(owner);
        if (!compact_owner.empty()) {
          owner_suffix = " owner={" + compact_owner + "}";
          cuwacunu::hero::mcp_observability::append_json_string_field(
              extra, "owner", compact_owner);
        }
      }
      set_error(error, "ingest lock already held: " + lock_path.string() +
                           owner_suffix);
      if (error && !error->empty()) {
        cuwacunu::hero::mcp_observability::append_json_string_field(
            extra, "error", *error);
      }
      log_hashimyei_catalog_event("ingest_lock_busy", extra.str());
    } else {
      set_error(error, "cannot acquire ingest lock: " + lock_path.string() +
                           " (idydb rc=" + std::to_string(rc) + " " +
                           idydb_rc_label(rc) + ")");
      if (error && !error->empty()) {
        cuwacunu::hero::mcp_observability::append_json_string_field(
            extra, "error", *error);
      }
      log_hashimyei_catalog_event("ingest_lock_failed", extra.str());
    }
    return false;
  }
  lock->path = lock_path;
  lock->acquired_at_ms = now_ms_utc();
  {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "lock_path", lock_path.string());
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "wait_ms", lock->acquired_at_ms - acquire_started_at_ms);
    log_hashimyei_catalog_event("ingest_lock_acquired", extra.str());
  }
  return true;
}

void hashimyei_catalog_store_t::release_ingest_lock_(ingest_lock_t* lock) {
  if (!lock) return;
  const std::filesystem::path lock_path = lock->path;
  const std::uint64_t acquired_at_ms = lock->acquired_at_ms;
  (void)idydb_named_lock_release(&lock->handle);
  if (!lock_path.empty()) {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "lock_path", lock_path.string());
    if (acquired_at_ms != 0) {
      cuwacunu::hero::mcp_observability::append_json_uint64_field(
          extra, "hold_ms", now_ms_utc() - acquired_at_ms);
    }
    log_hashimyei_catalog_event("ingest_lock_released", extra.str());
  }
  lock->path.clear();
  lock->acquired_at_ms = 0;
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
  struct ingest_lock_guard_t {
    hashimyei_catalog_store_t* self{nullptr};
    ingest_lock_t* lock{nullptr};
    ~ingest_lock_guard_t() {
      if (self && lock) self->release_ingest_lock_(lock);
    }
  } lock_guard{this, &lock};
  const std::filesystem::path normalized_store_root =
      store_root.lexically_normal();
  const std::filesystem::path normalized_catalog_root =
      lock.path.parent_path().lexically_normal();
  struct buffered_rows_guard_t {
    hashimyei_catalog_store_t* self{nullptr};
    ~buffered_rows_guard_t() {
      if (!self) return;
      self->buffer_rows_ = false;
      self->buffered_row_start_ = 0;
      self->buffered_rows_.clear();
    }
  } buffered_rows_guard{this};
  buffer_rows_ = true;
  buffered_row_start_ =
      (next_row_hint_ != 0) ? next_row_hint_ : idydb_column_next_row(&db_, kColRecordKind);
  buffered_rows_.clear();
  const std::uint64_t ingest_started_at_ms = now_ms_utc();
  std::uint64_t regular_files_scanned = 0;
  std::uint64_t run_manifest_files = 0;
  std::uint64_t component_manifest_files = 0;
  std::uint64_t report_fragment_files = 0;
  std::uint64_t run_manifest_ms = 0;
  std::uint64_t component_manifest_ms = 0;
  std::uint64_t report_fragment_ms = 0;
  std::uint64_t flush_ms = 0;
  std::uint64_t refresh_active_ms = 0;
  std::uint64_t refresh_family_rank_ms = 0;

  std::error_code ec;
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
    ++regular_files_scanned;
    const auto normalized_entry = it->path().lexically_normal();
    if (!path_is_within(normalized_store_root, normalized_entry)) continue;
    if (path_is_within(normalized_catalog_root, normalized_entry)) {
      continue;
    }

    const auto p = it->path();
    if (p.filename() == "run.manifest.v1.kv") {
      set_error(error,
                "unsupported v1 run manifest filename run.manifest.v1.kv");
      return false;
    }
    if (p.filename() == "component.manifest.v1.kv") {
      set_error(
          error,
          "unsupported v1 component manifest filename component.manifest.v1.kv");
      return false;
    }
    if (p.filename() == cuwacunu::hashimyei::kRunManifestFilenameV2) {
      const std::uint64_t phase_started_at_ms = now_ms_utc();
      if (!ingest_run_manifest_file_(p, error)) {
        return false;
      }
      ++run_manifest_files;
      run_manifest_ms += now_ms_utc() - phase_started_at_ms;
      continue;
    }
    if (p.filename() == cuwacunu::hashimyei::kComponentManifestFilenameV2) {
      const std::uint64_t phase_started_at_ms = now_ms_utc();
      if (!ingest_component_manifest_file_(p, error)) {
        return false;
      }
      ++component_manifest_files;
      component_manifest_ms += now_ms_utc() - phase_started_at_ms;
      continue;
    }
    if (p.extension() != ".lls") continue;
    const std::uint64_t phase_started_at_ms = now_ms_utc();
    if (!ingest_report_fragment_file_(p, error)) {
      return false;
    }
    ++report_fragment_files;
    report_fragment_ms += now_ms_utc() - phase_started_at_ms;
  }

  buffer_rows_ = false;
  {
    const std::uint64_t phase_started_at_ms = now_ms_utc();
    if (!flush_buffered_rows_(error)) return false;
    flush_ms = now_ms_utc() - phase_started_at_ms;
  }
  {
    const std::uint64_t phase_started_at_ms = now_ms_utc();
    refresh_active_component_views_();
    refresh_active_ms = now_ms_utc() - phase_started_at_ms;
  }
  {
    const std::uint64_t phase_started_at_ms = now_ms_utc();
    refresh_component_family_ranks_();
    refresh_family_rank_ms = now_ms_utc() - phase_started_at_ms;
  }
  std::ostringstream extra;
  cuwacunu::hero::mcp_observability::append_json_string_field(
      extra, "store_root", store_root.string());
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "regular_files_scanned", regular_files_scanned);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "run_manifest_files", run_manifest_files);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "component_manifest_files", component_manifest_files);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "report_fragment_files", report_fragment_files);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "run_manifest_ms", run_manifest_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "component_manifest_ms", component_manifest_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "report_fragment_ms", report_fragment_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(extra, "flush_ms",
                                                              flush_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "refresh_active_ms", refresh_active_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "refresh_family_rank_ms", refresh_family_rank_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "duration_ms", now_ms_utc() - ingest_started_at_ms);
  log_hashimyei_catalog_event("ingest_summary", extra.str());
  return true;
}

bool hashimyei_catalog_store_t::rebuild_indexes(std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  runs_by_id_.clear();
  run_ids_.clear();
  ledger_report_fragment_ids_.clear();
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
  explicit_family_rank_by_scope_.clear();

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

  std::unordered_map<std::string, std::uint64_t> family_rank_row_ts_by_scope{};
  std::unordered_map<std::string, std::string> family_rank_row_id_by_scope{};

  const idydb_column_row_sizing next = idydb_column_next_row(&db_, kColRecordKind);
  next_row_hint_ = next;
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

        std::uint64_t dependency_count = 0;
        if (parse_u64(kv["dependency_count"], &dependency_count)) {
          run.dependency_files.reserve(static_cast<std::size_t>(dependency_count));
          for (std::uint64_t i = 0; i < dependency_count; ++i) {
            std::ostringstream pfx;
            pfx << "dependency_" << std::setw(4) << std::setfill('0') << i << "_";
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
            c.family = kv[pfx.str() + "family"];
            c.hashimyei = kv[pfx.str() + "hashimyei"];
            if (!c.canonical_path.empty() || !c.family.empty() ||
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
      normalize_run_manifest_inplace(&run);
      if (!run.run_id.empty()) {
        run_ids_.insert(run.run_id);
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

    if (kind == cuwacunu::hero::schema::kRecordKindLEDGER) {
      const std::string report_fragment_id =
          as_text_or_empty(&db_, kColReportFragmentSha256, row).empty()
              ? as_text_or_empty(&db_, kColRecordId, row)
              : as_text_or_empty(&db_, kColReportFragmentSha256, row);
      if (!report_fragment_id.empty()) {
        ledger_report_fragment_ids_.insert(report_fragment_id);
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
          normalize_hashimyei_canonical_path_or_same(
              as_text_or_empty(&db_, kColCanonicalPath, row));
      const std::string hashimyei_name = normalize_hashimyei_name_or_same(
          as_text_or_empty(&db_, kColHashimyei, row));
      std::uint64_t parsed_ordinal = 0;
      if (cuwacunu::hashimyei::parse_hex_hash_name_ordinal(hashimyei_name,
                                                            &parsed_ordinal)) {
        component.manifest.hashimyei_identity.schema = cuwacunu::hashimyei::kIdentitySchemaV2;
        component.manifest.hashimyei_identity.kind =
            cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
        component.manifest.hashimyei_identity.name = hashimyei_name;
        component.manifest.hashimyei_identity.ordinal = parsed_ordinal;
      }
      component.manifest.family = component_family_from_parts(
          component.manifest.canonical_path,
          component.manifest.hashimyei_identity.name);
      const std::string stored_family = as_text_or_empty(&db_, kColMetricKey, row);
      if (!stored_family.empty()) {
        component.manifest.family = stored_family;
      }
      component.manifest.lineage_state = as_text_or_empty(&db_, kColMetricTxt, row);

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
            normalize_hashimyei_canonical_path_or_same(
                as_text_or_empty(&db_, kColCanonicalPath, row));
      }
      if (component.manifest.hashimyei_identity.name.empty()) {
        const std::string fallback_name = normalize_hashimyei_name_or_same(
            as_text_or_empty(&db_, kColHashimyei, row));
        if (cuwacunu::hashimyei::parse_hex_hash_name_ordinal(fallback_name,
                                                              &parsed_ordinal)) {
          component.manifest.hashimyei_identity.schema = cuwacunu::hashimyei::kIdentitySchemaV2;
          component.manifest.hashimyei_identity.kind =
              cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
          component.manifest.hashimyei_identity.name = fallback_name;
          component.manifest.hashimyei_identity.ordinal = parsed_ordinal;
        }
      }
      if (component.manifest.family.empty()) {
        component.manifest.family = component_family_from_parts(
            component.manifest.canonical_path,
            component.manifest.hashimyei_identity.name);
      }
      if (component.manifest.lineage_state.empty()) {
        component.manifest.lineage_state = as_text_or_empty(&db_, kColMetricTxt, row);
      }
      if (component.manifest.updated_at_ms == 0) {
        component.manifest.updated_at_ms = component.ts_ms;
      }
      if (component.manifest.created_at_ms == 0) {
        component.manifest.created_at_ms = component.ts_ms;
      }
      normalize_component_manifest_inplace(&component.manifest);

      if (!component.component_id.empty()) {
        bump_counter_from_identity(component.manifest.hashimyei_identity);
        if (component.manifest.parent_identity.has_value()) {
          bump_counter_from_identity(*component.manifest.parent_identity);
        }
        bump_counter_from_identity(component.manifest.contract_identity);
        index_hash_identity(component.manifest.contract_identity);
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
        if (!component.manifest.hashimyei_identity.name.empty()) {
          pick_latest(&latest_component_by_hashimyei_,
                      component.manifest.hashimyei_identity.name);
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
            placeholder.manifest.hashimyei_identity.schema = cuwacunu::hashimyei::kIdentitySchemaV2;
            placeholder.manifest.hashimyei_identity.kind =
                cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
            placeholder.manifest.hashimyei_identity.name =
                normalize_hashimyei_name_or_same(hashimyei);
            placeholder.manifest.hashimyei_identity.ordinal = ord;
          }
          if (!parent_hashimyei.empty()) {
            std::uint64_t parent_ord = 0;
            if (cuwacunu::hashimyei::parse_hex_hash_name_ordinal(parent_hashimyei,
                                                                  &parent_ord)) {
              cuwacunu::hashimyei::hashimyei_t parent{};
              parent.schema = cuwacunu::hashimyei::kIdentitySchemaV2;
              parent.kind = cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
              parent.name = normalize_hashimyei_name_or_same(parent_hashimyei);
              parent.ordinal = parent_ord;
              placeholder.manifest.parent_identity = parent;
            }
          }
          placeholder.manifest.revision_reason =
              revision_reason.empty() ? "initial" : revision_reason;
          placeholder.manifest.family = family.empty()
                                            ? component_family_from_parts(
                                                  canonical_path, hashimyei)
                                            : family;
          normalize_component_manifest_inplace(&placeholder.manifest);
          components_by_id_[component_id] = std::move(placeholder);
        }
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindCOMPONENT_ACTIVE) {
      const std::string canonical_path =
          normalize_hashimyei_canonical_path_or_same(
              as_text_or_empty(&db_, kColCanonicalPath, row));
      std::string family = normalize_hashimyei_canonical_path_or_same(
          as_text_or_empty(&db_, kColMetricKey, row));
      const std::string hashimyei = normalize_hashimyei_name_or_same(
          as_text_or_empty(&db_, kColHashimyei, row));
      if (family.empty()) {
        family = component_family_from_parts(canonical_path, hashimyei);
      }
      std::string contract_hash{};
      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!payload.empty()) {
        std::unordered_map<std::string, std::string> kv{};
        (void)parse_latent_lineage_state_payload(payload, &kv);
        contract_hash = trim_ascii(kv["contract_hash"]);
      }
      if (!canonical_path.empty() && !family.empty()) {
        const std::string key = component_active_pointer_key(
            canonical_path, family, hashimyei, contract_hash);
        active_hashimyei_by_key_[key] = hashimyei;
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindFAMILY_RANK) {
      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!payload.empty()) {
        std::unordered_map<std::string, std::string> kv;
        std::string parse_error{};
        if (parse_runtime_lls_payload(payload, &kv, nullptr, &parse_error)) {
          cuwacunu::hero::family_rank::state_t rank_state{};
          if (cuwacunu::hero::family_rank::parse_state_from_kv(
                  kv, &rank_state, &parse_error)) {
            const std::string scope_key = cuwacunu::hero::family_rank::scope_key(
                rank_state.family, rank_state.contract_hash);
            std::uint64_t row_ts = 0;
            if (rank_state.updated_at_ms != 0) {
              row_ts = rank_state.updated_at_ms;
            } else {
              (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &row_ts);
            }
            const std::string row_id = as_text_or_empty(&db_, kColRecordId, row);
            const auto current_ts_it = family_rank_row_ts_by_scope.find(scope_key);
            const bool should_replace =
                current_ts_it == family_rank_row_ts_by_scope.end() ||
                row_ts > current_ts_it->second ||
                (row_ts == current_ts_it->second &&
                 row_id > family_rank_row_id_by_scope[scope_key]);
            if (should_replace) {
              family_rank_row_ts_by_scope[scope_key] = row_ts;
              family_rank_row_id_by_scope[scope_key] = row_id;
              explicit_family_rank_by_scope_[scope_key] = std::move(rank_state);
            }
          }
        }
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindREPORT_FRAGMENT) {
      report_fragment_entry_t e{};
      e.report_fragment_id = as_text_or_empty(&db_, kColRecordId, row);
      e.canonical_path = normalize_hashimyei_canonical_path_or_same(
          as_text_or_empty(&db_, kColCanonicalPath, row));
      e.hashimyei = normalize_hashimyei_name_or_same(
          as_text_or_empty(&db_, kColHashimyei, row));
      e.schema = as_text_or_empty(&db_, kColSchema, row);
      e.report_fragment_sha256 = as_text_or_empty(&db_, kColReportFragmentSha256, row);
      e.path = as_text_or_empty(&db_, kColPath, row);
      e.payload_json = as_text_or_empty(&db_, kColPayload, row);
      (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &e.ts_ms);
      if (!e.payload_json.empty()) {
        std::unordered_map<std::string, std::string> kv{};
        std::string parse_error{};
        if (parse_runtime_lls_payload(e.payload_json, &kv, nullptr, &parse_error)) {
          populate_runtime_report_entry_header_fields_(kv, &e);
          populate_metrics_from_kv_(e.report_fragment_id, kv);
          if (e.canonical_path.empty()) {
            e.canonical_path =
                normalize_hashimyei_canonical_path_or_same(kv["canonical_path"]);
          }
          if (e.hashimyei.empty()) {
            e.hashimyei = normalize_hashimyei_name_or_same(kv["hashimyei"]);
          }
          if (e.schema.empty()) e.schema = kv["schema"];
        }
      }

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

  }

  for (const auto& [pointer_key, pointer_hashimyei] : active_hashimyei_by_key_) {
    std::string canonical_path{};
    std::string family{};
    std::string contract_hash{};
    if (!parse_component_active_pointer_key(pointer_key, &canonical_path, &family,
                                            &contract_hash)) {
      continue;
    }

    component_state_t best{};
    bool found = false;
    for (const auto& [_, component] : components_by_id_) {
      const std::string component_contract_hash =
          cuwacunu::hero::hashimyei::contract_hash_from_identity(
              component.manifest.contract_identity);
      const std::string component_key = component_active_pointer_key(
          component.manifest.canonical_path, component.manifest.family,
          component.manifest.hashimyei_identity.name, component_contract_hash);
      if (component_key != pointer_key) continue;
      if (!family.empty() && component.manifest.family != family) continue;
      if (!contract_hash.empty() && component_contract_hash != contract_hash) {
        continue;
      }
      if (component.manifest.hashimyei_identity.name != pointer_hashimyei) continue;
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

  for (auto& [_, component] : components_by_id_) {
    component.family_rank.reset();
    const std::string scope_key = cuwacunu::hero::family_rank::scope_key(
        component.manifest.family,
        cuwacunu::hero::hashimyei::contract_hash_from_identity(
            component.manifest.contract_identity));
    const auto it_rank = explicit_family_rank_by_scope_.find(scope_key);
    if (it_rank == explicit_family_rank_by_scope_.end()) continue;
    component.family_rank = cuwacunu::hero::family_rank::rank_for_hashimyei(
        it_rank->second, component.manifest.hashimyei_identity.name);
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

  const std::string canonical =
      normalize_hashimyei_canonical_path_or_same(canonical_path);
  const std::string hash = normalize_hashimyei_name_or_same(hashimyei);

  if (!canonical.empty() && !hash.empty()) {
    component_state_t best{};
    bool found = false;
    for (const auto& [_, component] : components_by_id_) {
      if (component.manifest.canonical_path != canonical) continue;
      if (component.manifest.hashimyei_identity.name != hash) continue;
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
  const std::string canonical =
      normalize_hashimyei_canonical_path_or_same(canonical_path);
  const std::string hash = normalize_hashimyei_name_or_same(hashimyei);

  for (const auto& [_, component] : components_by_id_) {
    if (!canonical.empty() && component.manifest.canonical_path != canonical) continue;
    if (!hash.empty() &&
        component.manifest.hashimyei_identity.name != hash) {
      continue;
    }
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

bool hashimyei_catalog_store_t::list_component_heads(
    std::size_t limit, std::size_t offset, bool newest_first,
    std::vector<component_state_t>* out, std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "component head list output pointer is null");
    return false;
  }
  out->clear();

  std::unordered_map<std::string, std::string> selected_by_canonical{};
  selected_by_canonical.reserve(latest_component_by_canonical_.size() +
                                active_component_by_canonical_.size());
  for (const auto& [canonical_path, component_id] : latest_component_by_canonical_) {
    selected_by_canonical[canonical_path] = component_id;
  }
  for (const auto& [canonical_path, component_id] : active_component_by_canonical_) {
    selected_by_canonical[canonical_path] = component_id;
  }

  std::unordered_set<std::string> emitted_component_ids{};
  out->reserve(selected_by_canonical.size());
  for (const auto& [_, component_id] : selected_by_canonical) {
    if (!emitted_component_ids.insert(component_id).second) continue;
    const auto it = components_by_id_.find(component_id);
    if (it == components_by_id_.end()) continue;
    out->push_back(it->second);
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
    std::string_view contract_hash,
    std::string* out_hashimyei, std::string* error) const {
  clear_error(error);
  if (!out_hashimyei) {
    set_error(error, "active hashimyei output pointer is null");
    return false;
  }
  out_hashimyei->clear();

  const std::string canonical =
      normalize_hashimyei_canonical_path_or_same(canonical_path);
  const std::string fam =
      normalize_hashimyei_canonical_path_or_same(family);
  const std::string contract = trim_ascii(contract_hash);
  if (canonical.empty()) {
    set_error(error, "canonical_path is required");
    return false;
  }
  if (contract.empty()) {
    set_error(error, "contract_hash is required");
    return false;
  }
  const std::string canonical_hash_tail = maybe_hashimyei_from_canonical(canonical);
  std::string canonical_lookup = canonical;
  if (!canonical_hash_tail.empty()) {
    canonical_lookup = component_family_from_parts(canonical, canonical_hash_tail);
  }

  if (!fam.empty()) {
    const std::string key = component_active_pointer_key(
        canonical_lookup, fam, canonical_hash_tail, contract);
    const auto it = active_hashimyei_by_key_.find(key);
    if (it == active_hashimyei_by_key_.end()) {
      set_error(error,
                "active hashimyei not found for canonical_path/family/contract");
      return false;
    }
    *out_hashimyei = it->second;
    return true;
  }

  std::string best_key{};
  std::uint64_t best_ts = 0;
  bool found = false;
  for (const auto& [key, value] : active_hashimyei_by_key_) {
    std::string key_canonical{};
    std::string key_family{};
    std::string key_contract{};
    if (!parse_component_active_pointer_key(key, &key_canonical, &key_family,
                                            &key_contract)) {
      continue;
    }
    if (key_canonical != canonical_lookup) continue;
    if (key_contract != contract) continue;
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

bool hashimyei_catalog_store_t::get_explicit_family_rank(
    std::string_view family, std::string_view contract_hash,
    cuwacunu::hero::family_rank::state_t* out, std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "family rank output pointer is null");
    return false;
  }
  *out = cuwacunu::hero::family_rank::state_t{};

  const std::string family_key = trim_ascii(family);
  const std::string contract_key = trim_ascii(contract_hash);
  if (family_key.empty() || contract_key.empty()) {
    set_error(error, "family and contract_hash are required");
    return false;
  }
  const std::string scope_key =
      cuwacunu::hero::family_rank::scope_key(family_key, contract_key);
  const auto it = explicit_family_rank_by_scope_.find(scope_key);
  if (it == explicit_family_rank_by_scope_.end()) {
    set_error(error, "explicit family rank not found: " + scope_key);
    return false;
  }
  *out = it->second;
  return true;
}

bool hashimyei_catalog_store_t::get_family_rank(
    std::string_view family, std::string_view contract_hash,
    cuwacunu::hero::family_rank::state_t* out, std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "family rank output pointer is null");
    return false;
  }
  *out = cuwacunu::hero::family_rank::state_t{};

  const std::string family_key = trim_ascii(family);
  const std::string contract_key = trim_ascii(contract_hash);
  if (family_key.empty() || contract_key.empty()) {
    set_error(error, "family and contract_hash are required");
    return false;
  }
  const std::string scope_key =
      cuwacunu::hero::family_rank::scope_key(family_key, contract_key);
  const auto it = explicit_family_rank_by_scope_.find(scope_key);
  if (it == explicit_family_rank_by_scope_.end()) {
    set_error(error, "family rank not found: " + scope_key);
    return false;
  }
  *out = it->second;
  return true;
}

bool hashimyei_catalog_store_t::resolve_ranked_hashimyei(
    std::string_view family, std::string_view contract_hash, std::uint64_t rank,
    std::string* out_hashimyei, std::string* error) const {
  clear_error(error);
  if (!out_hashimyei) {
    set_error(error, "ranked hashimyei output pointer is null");
    return false;
  }
  out_hashimyei->clear();

  cuwacunu::hero::family_rank::state_t rank_state{};
  if (!get_family_rank(family, contract_hash, &rank_state, error)) return false;
  for (const auto& assignment : rank_state.assignments) {
    if (assignment.rank != rank) continue;
    *out_hashimyei = assignment.hashimyei;
    return true;
  }

  set_error(error, "family rank not found for requested rank");
  return false;
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
  if (!ensure_identity_allocated_(&manifest.hashimyei_identity, error)) return false;
  if (!ensure_identity_allocated_(&manifest.contract_identity, error)) return false;
  const std::string contract_hash =
      cuwacunu::hero::hashimyei::contract_hash_from_identity(
          manifest.contract_identity);
  if (trim_ascii(manifest.docking_signature_sha256_hex).empty() &&
      !trim_ascii(contract_hash).empty()) {
    const auto contract_snapshot =
        cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
    manifest.docking_signature_sha256_hex =
        contract_snapshot->signature.docking_signature_sha256_hex;
  }
  if (manifest.parent_identity.has_value() &&
      !manifest.parent_identity->name.empty()) {
    std::string parent_error;
    if (!cuwacunu::hashimyei::validate_hashimyei(*manifest.parent_identity,
                                                  &parent_error)) {
      set_error(error, "parent_identity invalid: " + parent_error);
      return false;
    }
  }
  if (manifest.created_at_ms == 0) manifest.created_at_ms = now_ms_utc();
  if (manifest.updated_at_ms == 0) manifest.updated_at_ms = manifest.created_at_ms;
  if (!validate_component_manifest(manifest, error)) return false;

  if (!manifest.hashimyei_identity.name.empty()) {
    const auto existing_it =
        latest_component_by_hashimyei_.find(manifest.hashimyei_identity.name);
    if (existing_it != latest_component_by_hashimyei_.end()) {
      const auto component_it = components_by_id_.find(existing_it->second);
      if (component_it != components_by_id_.end() &&
          !same_contract_identity_(
              component_it->second.manifest.contract_identity,
              manifest.contract_identity)) {
        set_error(
            error,
            "component hashimyei is contract-scoped and cannot be reused across "
            "contracts: " +
                manifest.hashimyei_identity.name);
        return false;
      }
    }
  }

  const std::string payload = component_manifest_payload(manifest);
  std::string report_fragment_sha;
  if (!sha256_hex_bytes(payload, &report_fragment_sha)) {
    set_error(error, "failed to compute component manifest payload sha256");
    return false;
  }

  const std::string component_id = compute_component_manifest_id(manifest);
  if (out_component_id) *out_component_id = component_id;
  const std::filesystem::path store_root =
      store_root_from_catalog_path(options_.catalog_path);
  std::filesystem::path manifest_path =
      component_manifest_path(store_root, manifest.canonical_path, component_id);
  if (!std::filesystem::exists(manifest_path)) {
    if (!save_component_manifest(store_root, manifest, &manifest_path, error)) {
      return false;
    }
  }

  bool already = false;
  if (!ledger_contains_(report_fragment_sha, &already, error)) return false;
  if (already) return true;

  const std::uint64_t ts_ms =
      manifest.updated_at_ms != 0 ? manifest.updated_at_ms
                                  : (manifest.created_at_ms != 0
                                         ? manifest.created_at_ms
                                         : now_ms_utc());

  const std::string manifest_path_s = manifest_path.string();
  const bool is_active = trim_ascii(manifest.lineage_state) == "active";
  std::string active_pointer_key{};

  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT, component_id, "", manifest.canonical_path,
                   manifest.hashimyei_identity.name, manifest.schema,
                   manifest.family,
                   std::numeric_limits<double>::quiet_NaN(), manifest.lineage_state,
                   report_fragment_sha, manifest_path_s, std::to_string(ts_ms),
                   payload, error)) {
    return false;
  }

  const std::string source_payload =
      "{\"component_edge_kind\":\"founding_dsl_source\"}";
  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_LINEAGE,
                   component_id + "|founding_dsl_source", "",
                   manifest.canonical_path, manifest.hashimyei_identity.name,
                   manifest.schema, manifest.founding_dsl_source_path,
                   std::numeric_limits<double>::quiet_NaN(),
                   manifest.founding_dsl_source_sha256_hex, "",
                   manifest_path_s, std::to_string(ts_ms),
                   source_payload, error)) {
    return false;
  }
  founding_dsl_bundle_manifest_t bundle_manifest{};
  std::string bundle_error{};
  if (read_founding_dsl_bundle_manifest(store_root, manifest.canonical_path,
                                        component_id, &bundle_manifest,
                                        &bundle_error)) {
    const std::string bundle_payload =
        std::string("{\"component_edge_kind\":\"founding_dsl_bundle\","
                    "\"founding_dsl_bundle_aggregate_sha256_hex\":\"") +
        bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex + "\"}";
    if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_LINEAGE,
                     component_id + "|founding_dsl_bundle", "",
                     manifest.canonical_path, manifest.hashimyei_identity.name,
                     manifest.schema,
                     founding_dsl_bundle_manifest_path(
                         store_root, manifest.canonical_path, component_id)
                         .string(),
                     std::numeric_limits<double>::quiet_NaN(),
                     bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex, "",
                     manifest_path_s, std::to_string(ts_ms),
                     bundle_payload, error)) {
      return false;
    }
  }

  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_REVISION, component_id + "|revision", "",
                   manifest.canonical_path, manifest.hashimyei_identity.name,
                   manifest.schema,
                   manifest.family, std::numeric_limits<double>::quiet_NaN(),
                   manifest.parent_identity.has_value()
                       ? std::string_view(manifest.parent_identity->name)
                       : std::string_view{},
                   "", manifest_path_s,
                   std::to_string(ts_ms), manifest.revision_reason, error)) {
    return false;
  }

  if (is_active) {
    const std::string contract_hash =
        cuwacunu::hero::hashimyei::contract_hash_from_identity(
            manifest.contract_identity);
    active_pointer_key = component_active_pointer_key(
        manifest.canonical_path, manifest.family,
        manifest.hashimyei_identity.name, contract_hash);
    std::ostringstream pointer_payload;
    pointer_payload << "canonical_path=" << manifest.canonical_path << "\n";
    pointer_payload << "family=" << manifest.family << "\n";
    pointer_payload << "contract_hash=" << contract_hash << "\n";
    pointer_payload << "active_hashimyei="
                    << manifest.hashimyei_identity.name << "\n";
    pointer_payload << "component_id=" << component_id << "\n";
    pointer_payload << "docking_signature_sha256_hex="
                    << manifest.docking_signature_sha256_hex << "\n";
    pointer_payload << "parent_hashimyei="
                    << (manifest.parent_identity.has_value()
                            ? std::string_view(manifest.parent_identity->name)
                            : std::string_view{})
                    << "\n";
    pointer_payload << "revision_reason=" << manifest.revision_reason << "\n";
    pointer_payload << "founding_revision_id=" << manifest.founding_revision_id
                    << "\n";
    if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_ACTIVE, active_pointer_key, "",
                     manifest.canonical_path, manifest.hashimyei_identity.name,
                     cuwacunu::hashimyei::kComponentActivePointerSchemaV2, manifest.family,
                     std::numeric_limits<double>::quiet_NaN(),
                     manifest.founding_revision_id, "", manifest_path_s,
                     std::to_string(ts_ms), pointer_payload.str(), error)) {
      return false;
    }
  }

  if (!append_ledger_(report_fragment_sha, manifest_path_s, error)) return false;

  component_state_t component{};
  component.component_id = component_id;
  component.ts_ms = ts_ms;
  component.manifest_path = manifest_path_s;
  component.report_fragment_sha256 = report_fragment_sha;
  component.manifest = manifest;
  observe_component_(component);
  if (is_active && !active_pointer_key.empty()) {
    active_hashimyei_by_key_[active_pointer_key] =
        manifest.hashimyei_identity.name;
    refresh_active_component_views_();
  }
  refresh_component_family_ranks_();

  if (out_inserted) *out_inserted = true;
  return true;
}

bool hashimyei_catalog_store_t::latest_report_fragment_snapshot(
    std::string_view canonical_path, report_fragment_snapshot_t* out,
    std::string* error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "latest report_fragment snapshot output pointer is null");
    return false;
  }
  *out = report_fragment_snapshot_t{};

  const std::string cp(canonical_path);
  report_fragment_entry_t best{};
  bool found = false;
  for (const auto& [_, art] : report_fragments_by_id_) {
    if (art.canonical_path != cp) continue;
    if (!found || art.ts_ms > best.ts_ms ||
        (art.ts_ms == best.ts_ms && art.report_fragment_id > best.report_fragment_id)) {
      best = art;
      found = true;
    }
  }
  if (!found) {
    set_error(error, "latest report_fragment snapshot not found");
    return false;
  }

  out->report_fragment = best;
  (void)report_fragment_metrics(best.report_fragment_id, &out->numeric_metrics, &out->text_metrics,
                         nullptr);
  return true;
}

}  // namespace hashimyei
}  // namespace hero
}  // namespace cuwacunu
