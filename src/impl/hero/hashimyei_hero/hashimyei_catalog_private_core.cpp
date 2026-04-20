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
  if (schema == cuwacunu::hero::family_rank::kFamilyRankSchemaV2) return true;
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
  return schema == cuwacunu::hero::family_rank::kFamilyRankSchemaV2;
}

[[nodiscard]] std::string family_rank_record_id(std::string_view family,
                                                std::string_view dock_hash,
                                                std::string_view artifact_sha256) {
  return cuwacunu::hero::family_rank::scope_key(family, dock_hash) + "|" +
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

