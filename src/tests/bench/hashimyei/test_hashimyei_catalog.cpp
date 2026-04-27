#include <hero/hashimyei_hero/hashimyei_catalog.h>
#include <hero/hashimyei_hero/hero_hashimyei_tools.h>
#include <hero/hero_catalog_schema.h>
#include <hero/lattice_hero/lattice_catalog.h>
#include <iitepi/config_space_t.h>
#include <iitepi/contract_space_t.h>

#include <openssl/evp.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

namespace fs = std::filesystem;

using cuwacunu::hashimyei::hashimyei_kind_e;
using cuwacunu::hashimyei::hashimyei_t;
using cuwacunu::hero::hashimyei::component_manifest_t;
using cuwacunu::hero::hashimyei::component_state_t;
using cuwacunu::hero::hashimyei::compute_run_id;
using cuwacunu::hero::hashimyei::hashimyei_catalog_store_t;
using cuwacunu::hero::hashimyei::load_component_manifest;
using cuwacunu::hero::hashimyei::load_run_manifest;
using cuwacunu::hero::hashimyei::run_manifest_t;
using cuwacunu::hero::hashimyei::save_run_manifest;
using cuwacunu::hero::hashimyei::wave_contract_binding_t;

static constexpr const char *kGlobalConfigPath = "/cuwacunu/src/config/.config";
static constexpr const char *kDefaultContractPath =
    "/cuwacunu/src/config/instructions/defaults/default.iitepi.contract.dsl";

static void require_impl(bool ok, const char *expr, const char *file,
                         int line) {
  if (!ok) {
    std::cerr << "[TEST FAIL] " << file << ":" << line << "  (" << expr
              << ")\n";
    std::exit(1);
  }
}
#define REQUIRE(x) require_impl((x), #x, __FILE__, __LINE__)

static std::string random_suffix() {
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<std::uint64_t> dist;
  const std::uint64_t v = dist(gen);
  return std::to_string(static_cast<unsigned long long>(getpid())) + "_" +
         std::to_string(static_cast<unsigned long long>(v));
}

struct temp_dir_t {
  fs::path dir{};
  temp_dir_t() {
    dir = fs::temp_directory_path() /
          ("test_hashimyei_catalog_" + random_suffix());
    fs::create_directories(dir);
  }
  ~temp_dir_t() {
    std::error_code ec;
    fs::remove_all(dir, ec);
  }
};

static std::string hex_lower(const unsigned char *bytes, std::size_t n) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string out;
  out.resize(n * 2);
  for (std::size_t i = 0; i < n; ++i) {
    const unsigned char b = bytes[i];
    out[2 * i + 0] = kHex[(b >> 4) & 0x0F];
    out[2 * i + 1] = kHex[b & 0x0F];
  }
  return out;
}

static std::string sha256_hex(std::string_view payload) {
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  REQUIRE(ctx != nullptr);
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  const bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
                  EVP_DigestUpdate(ctx, payload.data(), payload.size()) == 1 &&
                  EVP_DigestFinal_ex(ctx, digest, &digest_len) == 1;
  EVP_MD_CTX_free(ctx);
  REQUIRE(ok);
  return hex_lower(digest, digest_len);
}

static std::string sixty_four_hex(char c) { return std::string(64, c); }

static hashimyei_t make_test_identity(hashimyei_kind_e kind,
                                      std::uint64_t ordinal,
                                      std::string hash = {}) {
  hashimyei_t id =
      cuwacunu::hashimyei::make_identity(kind, ordinal, std::move(hash));
  id.schema = cuwacunu::hashimyei::kIdentitySchemaV2;
  return id;
}

static wave_contract_binding_t make_binding(std::string alias,
                                            std::uint64_t contract_ordinal = 1,
                                            std::uint64_t wave_ordinal = 2,
                                            std::uint64_t binding_ordinal = 3) {
  wave_contract_binding_t out{};
  out.contract = make_test_identity(hashimyei_kind_e::CONTRACT,
                                    contract_ordinal, sixty_four_hex('a'));
  out.wave = make_test_identity(hashimyei_kind_e::WAVE, wave_ordinal,
                                sixty_four_hex('b'));
  out.binding_id = std::move(alias);
  out.identity = make_test_identity(
      hashimyei_kind_e::WAVE_CONTRACT_BINDING, binding_ordinal,
      sha256_hex(out.contract.hash_sha256_hex + "|" + out.wave.hash_sha256_hex +
                 "|" + out.binding_id));
  return out;
}

static void write_text_file(const fs::path &path, const std::string &payload) {
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  REQUIRE(static_cast<bool>(out));
  out << payload;
  REQUIRE(static_cast<bool>(out));
}

static std::string component_payload(const component_manifest_t &m) {
  std::ostringstream out;
  out << "schema=" << m.schema << "\n";
  out << "canonical_path=" << m.canonical_path << "\n";
  out << "family=" << m.family << "\n";
  out << "component_tag=" << m.component_tag << "\n";
  out << "hashimyei_identity.schema=" << m.hashimyei_identity.schema << "\n";
  out << "hashimyei_identity.kind="
      << cuwacunu::hashimyei::hashimyei_kind_to_string(
             m.hashimyei_identity.kind)
      << "\n";
  out << "hashimyei_identity.name=" << m.hashimyei_identity.name << "\n";
  out << "hashimyei_identity.ordinal=" << m.hashimyei_identity.ordinal << "\n";
  out << "hashimyei_identity.hash_sha256_hex="
      << m.hashimyei_identity.hash_sha256_hex << "\n";
  out << "parent_identity.present="
      << (m.parent_identity.has_value() ? "1" : "0") << "\n";
  if (m.parent_identity.has_value()) {
    out << "parent_identity.schema=" << m.parent_identity->schema << "\n";
    out << "parent_identity.kind="
        << cuwacunu::hashimyei::hashimyei_kind_to_string(
               m.parent_identity->kind)
        << "\n";
    out << "parent_identity.name=" << m.parent_identity->name << "\n";
    out << "parent_identity.ordinal=" << m.parent_identity->ordinal << "\n";
    out << "parent_identity.hash_sha256_hex="
        << m.parent_identity->hash_sha256_hex << "\n";
  }
  out << "revision_reason=" << m.revision_reason << "\n";
  out << "founding_revision_id=" << m.founding_revision_id << "\n";
  out << "contract_identity.schema=" << m.contract_identity.schema << "\n";
  out << "contract_identity.kind="
      << cuwacunu::hashimyei::hashimyei_kind_to_string(m.contract_identity.kind)
      << "\n";
  out << "contract_identity.name=" << m.contract_identity.name << "\n";
  out << "contract_identity.ordinal=" << m.contract_identity.ordinal << "\n";
  out << "contract_identity.hash_sha256_hex="
      << m.contract_identity.hash_sha256_hex << "\n";
  out << "founding_dsl_source_path=" << m.founding_dsl_source_path << "\n";
  out << "founding_dsl_source_sha256_hex=" << m.founding_dsl_source_sha256_hex
      << "\n";
  out << "docking_signature_sha256_hex=" << m.docking_signature_sha256_hex
      << "\n";
  out << "component_compatibility_sha256_hex="
      << m.component_compatibility_sha256_hex << "\n";
  for (const auto &[field, value] :
       cuwacunu::camahjucunu::instrument_signature_fields(
           m.instrument_signature)) {
    out << "instrument_signature." << field << "=" << value << "\n";
  }
  out << "lineage_state=" << m.lineage_state << "\n";
  out << "replaced_by=" << m.replaced_by << "\n";
  out << "created_at_ms=" << m.created_at_ms << "\n";
  out << "updated_at_ms=" << m.updated_at_ms << "\n";
  return out.str();
}

static run_manifest_t make_run_manifest() {
  run_manifest_t m{};
  m.schema = cuwacunu::hashimyei::kRunManifestSchemaV2;
  m.started_at_ms = 1711111111000ULL;
  m.campaign_identity = make_test_identity(hashimyei_kind_e::CAMPAIGN, 4);
  m.wave_contract_binding = make_binding("bind_train_vicreg");
  m.sampler = "uniform";
  m.record_type = "train";
  m.seed = "42";
  m.device = "cpu";
  m.dtype = "float32";
  m.run_id = compute_run_id(m.campaign_identity, m.wave_contract_binding,
                            m.started_at_ms);
  m.dependency_files.push_back(
      {"iitepi.campaign.bind_train_vicreg", "sha.campaign"});
  m.dependency_files.push_back(
      {"iitepi.contract.bind_train_vicreg", "sha.contract"});
  m.components.push_back(
      {"tsi.source.dataloader", "tsi.source.dataloader", ""});
  m.components.push_back({"tsi.wikimyei.representation.encoding.vicreg.0x0010",
                          "tsi.wikimyei.representation.encoding.vicreg",
                          "0x0010"});
  return m;
}

static component_manifest_t make_component_manifest() {
  component_manifest_t m{};
  m.schema = cuwacunu::hashimyei::kComponentManifestSchemaV2;
  m.instrument_signature =
      cuwacunu::camahjucunu::instrument_signature_all_any();
  m.canonical_path = "tsi.wikimyei.representation.encoding.vicreg.0x0010";
  m.family = "tsi.wikimyei.representation.encoding.vicreg";
  m.component_tag = "vicreg.test.default";
  m.hashimyei_identity = make_test_identity(hashimyei_kind_e::TSIEMENE, 0x10);
  m.revision_reason = "initial";
  m.founding_revision_id = "cfgrev.initial";
  m.contract_identity = make_binding("bind_train_vicreg").contract;
  m.founding_dsl_source_path =
      "src/config/instructions/defaults/"
      "default.tsi.wikimyei.representation.encoding.vicreg.network_design.dsl";
  m.founding_dsl_source_sha256_hex = sixty_four_hex('c');
  m.docking_signature_sha256_hex = sixty_four_hex('d');
  m.component_compatibility_sha256_hex = sixty_four_hex('d');
  m.lineage_state = "active";
  m.created_at_ms = 1711111111000ULL;
  m.updated_at_ms = 1711111111001ULL;
  return m;
}

static component_manifest_t
make_auto_contract_component_manifest(std::uint64_t component_ordinal,
                                      std::string_view contract_sha,
                                      char dsl_fill, std::uint64_t ts_ms) {
  component_manifest_t m = make_component_manifest();
  m.hashimyei_identity =
      make_test_identity(hashimyei_kind_e::TSIEMENE, component_ordinal);
  m.canonical_path = m.family + "." + m.hashimyei_identity.name;
  m.founding_revision_id = "cfgrev.auto." + m.hashimyei_identity.name;
  m.contract_identity = make_test_identity(hashimyei_kind_e::CONTRACT, 0,
                                           std::string(contract_sha));
  m.contract_identity.name.clear();
  m.contract_identity.ordinal = 0;
  m.founding_dsl_source_sha256_hex = sixty_four_hex(dsl_fill);
  m.docking_signature_sha256_hex = sixty_four_hex('1');
  m.component_compatibility_sha256_hex = sixty_four_hex('1');
  m.created_at_ms = ts_ms;
  m.updated_at_ms = ts_ms;
  return m;
}

static void test_component_manifest_id_stable_under_source_path_churn() {
  component_manifest_t a = make_component_manifest();
  component_manifest_t b = a;
  a.founding_dsl_source_path =
      "/tmp/runtime_a/instructions/"
      "tsi.wikimyei.representation.encoding.vicreg.dsl";
  b.founding_dsl_source_path =
      "/tmp/runtime_b/instructions/"
      "tsi.wikimyei.representation.encoding.vicreg.dsl";

  REQUIRE(cuwacunu::hero::hashimyei::compute_component_manifest_id(a) ==
          cuwacunu::hero::hashimyei::compute_component_manifest_id(b));

  b.founding_dsl_source_sha256_hex = sixty_four_hex('e');
  REQUIRE(cuwacunu::hero::hashimyei::compute_component_manifest_id(a) !=
          cuwacunu::hero::hashimyei::compute_component_manifest_id(b));
}

static void test_founding_bundle_digest_stable_under_source_path_churn() {
  cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_t a{};
  a.component_id = "component.1234";
  a.canonical_path = "tsi.wikimyei.representation.encoding.vicreg.0x0010";
  a.hashimyei_name = "0x0010";
  a.files.push_back(
      {"/tmp/runtime_a/instructions/a.dsl", "0000_a.dsl", sixty_four_hex('a')});
  a.files.push_back(
      {"/tmp/runtime_a/instructions/b.dsl", "0001_b.dsl", sixty_four_hex('b')});

  auto b = a;
  b.files[0].source_path = "/tmp/runtime_b/instructions/a.dsl";
  b.files[1].source_path = "/tmp/runtime_b/instructions/b.dsl";

  REQUIRE(
      cuwacunu::hero::hashimyei::compute_founding_dsl_bundle_aggregate_sha256(
          a) ==
      cuwacunu::hero::hashimyei::compute_founding_dsl_bundle_aggregate_sha256(
          b));

  b.files[1].sha256_hex = sixty_four_hex('c');
  REQUIRE(
      cuwacunu::hero::hashimyei::compute_founding_dsl_bundle_aggregate_sha256(
          a) !=
      cuwacunu::hero::hashimyei::compute_founding_dsl_bundle_aggregate_sha256(
          b));
}

static void test_uppercase_component_manifest_normalization() {
  temp_dir_t tmp{};
  const fs::path store_root = tmp.dir / ".hashimyei";
  const fs::path catalog_path =
      store_root / "_meta" / "catalog" / "hashimyei_catalog.idydb";
  constexpr const char *kPassphrase = "catalog-uppercase-pass";

  component_manifest_t manifest = make_component_manifest();
  manifest.hashimyei_identity =
      make_test_identity(hashimyei_kind_e::TSIEMENE, 0xab);
  manifest.hashimyei_identity.name = "0x00AB";
  manifest.canonical_path = manifest.family + ".0x00AB";
  manifest.founding_revision_id = "cfgrev.uppercase";
  manifest.created_at_ms = 1711111122000ULL;
  manifest.updated_at_ms = 1711111122001ULL;

  fs::path manifest_path{};
  std::string error;
  REQUIRE(cuwacunu::hero::hashimyei::save_component_manifest(
      store_root, manifest, &manifest_path, &error));
  REQUIRE(manifest_path.string().find("0x00ab") != std::string::npos);

  component_manifest_t loaded{};
  REQUIRE(load_component_manifest(manifest_path, &loaded, &error));
  REQUIRE(loaded.hashimyei_identity.name == "0x00ab");
  REQUIRE(loaded.canonical_path == loaded.family + ".0x00ab");

  hashimyei_catalog_store_t catalog{};
  hashimyei_catalog_store_t::options_t options{};
  options.catalog_path = catalog_path;
  options.encrypted = true;
  options.passphrase = kPassphrase;
  options.ingest_version = 2;
  REQUIRE(catalog.open(options, &error));
  REQUIRE(catalog.ingest_filesystem(store_root, &error));

  component_state_t resolved{};
  REQUIRE(catalog.resolve_component("", "0x00AB", &resolved, &error));
  REQUIRE(resolved.manifest.hashimyei_identity.name == "0x00ab");
  REQUIRE(resolved.manifest.canonical_path ==
          "tsi.wikimyei.representation.encoding.vicreg.0x00ab");
}

int main() {
  // unified catalog schema vocabulary checks
  REQUIRE(cuwacunu::hero::schema::is_known_record_kind(
      cuwacunu::hero::schema::kRecordKindRUN));
  REQUIRE(cuwacunu::hero::schema::logical_table_for_record_kind(
              cuwacunu::hero::schema::kRecordKindRUN) ==
          cuwacunu::hero::schema::logical_table_e::ENTITY);
  REQUIRE(cuwacunu::hero::schema::logical_table_for_record_kind(
              cuwacunu::hero::schema::kRecordKindRUNTIME_REPORT_FRAGMENT) ==
          cuwacunu::hero::schema::logical_table_e::BLOB);
  REQUIRE(!cuwacunu::hero::schema::is_known_record_kind("unknown_kind"));

  // identity-level checks
  REQUIRE(cuwacunu::hashimyei::make_hex_hash_name(0x1a) == "0x001a");
  std::uint64_t parsed_ordinal = 0;
  REQUIRE(cuwacunu::hashimyei::parse_hex_hash_name_ordinal("0x001a",
                                                           &parsed_ordinal));
  REQUIRE(parsed_ordinal == 0x1a);

  hashimyei_t missing_contract_hash =
      make_test_identity(hashimyei_kind_e::CONTRACT, 7, "");
  std::string identity_error;
  REQUIRE(!cuwacunu::hashimyei::validate_hashimyei(missing_contract_hash,
                                                   &identity_error));
  REQUIRE(identity_error.find("hash_sha256_hex") != std::string::npos);

  hashimyei_t campaign_with_hash =
      make_test_identity(hashimyei_kind_e::CAMPAIGN, 5, sixty_four_hex('f'));
  REQUIRE(!cuwacunu::hashimyei::validate_hashimyei(campaign_with_hash,
                                                   &identity_error));

  const wave_contract_binding_t binding_a = make_binding("bind_A", 1, 2, 3);
  const wave_contract_binding_t binding_b = make_binding("bind_A", 1, 2, 3);
  REQUIRE(binding_a.identity.hash_sha256_hex ==
          binding_b.identity.hash_sha256_hex);

  temp_dir_t tmp{};
  const fs::path store_root = tmp.dir / ".hashimyei";
  const fs::path catalog_path =
      store_root / "_meta" / "catalog" / "hashimyei_catalog.idydb";
  constexpr const char *kPassphrase = "catalog-test-pass";

  const run_manifest_t manifest = make_run_manifest();
  test_component_manifest_id_stable_under_source_path_churn();
  test_founding_bundle_digest_stable_under_source_path_churn();
  test_uppercase_component_manifest_normalization();

  fs::path run_manifest_path{};
  std::string error;
  REQUIRE(save_run_manifest(store_root, manifest, &run_manifest_path, &error));
  REQUIRE(run_manifest_path.filename() ==
          cuwacunu::hashimyei::kRunManifestFilenameV2);

  run_manifest_t loaded_run{};
  REQUIRE(load_run_manifest(run_manifest_path, &loaded_run, &error));
  REQUIRE(loaded_run.run_id == manifest.run_id);
  REQUIRE(loaded_run.campaign_identity.name == manifest.campaign_identity.name);
  REQUIRE(loaded_run.wave_contract_binding.identity.hash_sha256_hex ==
          manifest.wave_contract_binding.identity.hash_sha256_hex);
  REQUIRE(loaded_run.components.size() == manifest.components.size());
  REQUIRE(loaded_run.components[0].canonical_path ==
          manifest.components[0].canonical_path);
  REQUIRE(loaded_run.components[1].hashimyei ==
          manifest.components[1].hashimyei);

  component_manifest_t component_manifest = make_component_manifest();
  fs::path component_manifest_file{};
  REQUIRE(cuwacunu::hero::hashimyei::save_component_manifest(
      store_root, component_manifest, &component_manifest_file, &error));

  component_manifest_t loaded_component{};
  REQUIRE(load_component_manifest(component_manifest_file, &loaded_component,
                                  &error));
  REQUIRE(loaded_component.hashimyei_identity.name ==
          component_manifest.hashimyei_identity.name);
  REQUIRE(loaded_component.contract_identity.hash_sha256_hex ==
          component_manifest.contract_identity.hash_sha256_hex);

  const fs::path invalid_v1_filename =
      tmp.dir / "invalid_manifests" / "tsi.wikimyei" / "representation" /
      "vicreg" / "0x9999" / "component.manifest.v1.kv";
  write_text_file(invalid_v1_filename, component_payload(component_manifest));
  REQUIRE(
      !load_component_manifest(invalid_v1_filename, &loaded_component, &error));
  REQUIRE(error.find("v1") != std::string::npos);

  const fs::path invalid_v1_schema =
      tmp.dir / "invalid_manifests" / "tsi.wikimyei" / "representation" /
      "vicreg" / "0x9998" / cuwacunu::hashimyei::kComponentManifestFilenameV2;
  component_manifest_t invalid_schema_manifest = component_manifest;
  invalid_schema_manifest.schema = "hashimyei.component.manifest.v1";
  write_text_file(invalid_v1_schema,
                  component_payload(invalid_schema_manifest));
  REQUIRE(
      !load_component_manifest(invalid_v1_schema, &loaded_component, &error));
  REQUIRE(error.find("schema") != std::string::npos);

  hashimyei_catalog_store_t catalog{};
  hashimyei_catalog_store_t::options_t options{};
  options.catalog_path = catalog_path;
  options.encrypted = true;
  options.passphrase = kPassphrase;
  options.ingest_version = 2;

  REQUIRE(catalog.open(options, &error));
  const fs::path ingest_lock_path =
      store_root / "_meta" / "catalog" / ".ingest.lock";
  idydb_named_lock *busy_lock = nullptr;
  idydb_named_lock_options lock_options{};
  lock_options.shared = false;
  lock_options.create_parent_directories = true;
  lock_options.owner_label = "test_hashimyei_catalog_busy_holder";
  REQUIRE(idydb_named_lock_acquire(ingest_lock_path.c_str(), &busy_lock,
                                   &lock_options) == IDYDB_SUCCESS);
  REQUIRE(!catalog.ingest_filesystem(store_root, &error));
  REQUIRE(error.find("ingest lock already held") != std::string::npos);
  REQUIRE(error.find("test_hashimyei_catalog_busy_holder") !=
          std::string::npos);
  REQUIRE(idydb_named_lock_release(&busy_lock) == IDYDB_DONE);
  REQUIRE(catalog.ingest_filesystem(store_root, &error));

  std::vector<run_manifest_t> runs{};
  REQUIRE(catalog.list_runs_by_binding(
      manifest.wave_contract_binding.contract.name,
      manifest.wave_contract_binding.wave.name,
      manifest.wave_contract_binding.identity.name, &runs, &error));
  REQUIRE(runs.size() == 1);
  REQUIRE(runs[0].run_id == manifest.run_id);

  runs.clear();
  REQUIRE(catalog.list_runs_by_binding(
      manifest.wave_contract_binding.contract.hash_sha256_hex, "", "", &runs,
      &error));
  REQUIRE(runs.empty());

  component_state_t resolved_component{};
  REQUIRE(catalog.resolve_component("",
                                    component_manifest.hashimyei_identity.name,
                                    &resolved_component, &error));
  REQUIRE(resolved_component.manifest.hashimyei_identity.name ==
          component_manifest.hashimyei_identity.name);

  component_manifest_t conflicting_contract_manifest = component_manifest;
  conflicting_contract_manifest.contract_identity =
      make_binding("bind_train_vicreg_other_contract", 9, 2, 3).contract;
  conflicting_contract_manifest.updated_at_ms = 1711111112000ULL;
  conflicting_contract_manifest.created_at_ms = 1711111112000ULL;
  REQUIRE(!catalog.register_component_manifest(conflicting_contract_manifest,
                                               nullptr, nullptr, &error));
  REQUIRE(!error.empty());

  component_manifest_t replacement_manifest = component_manifest;
  replacement_manifest.parent_identity = component_manifest.hashimyei_identity;
  replacement_manifest.hashimyei_identity =
      make_test_identity(hashimyei_kind_e::TSIEMENE, 0x21);
  replacement_manifest.canonical_path =
      replacement_manifest.family + "." +
      replacement_manifest.hashimyei_identity.name;
  replacement_manifest.revision_reason = "dsl_change";
  replacement_manifest.founding_revision_id = "cfgrev.replacement";
  replacement_manifest.founding_dsl_source_sha256_hex = sixty_four_hex('d');
  replacement_manifest.created_at_ms = 1711111113000ULL;
  replacement_manifest.updated_at_ms = 1711111113000ULL;

  std::string replacement_component_id{};
  bool replacement_inserted = false;
  REQUIRE(catalog.register_component_manifest(replacement_manifest,
                                              &replacement_component_id,
                                              &replacement_inserted, &error));
  REQUIRE(replacement_inserted);

  std::string active_hashimyei{};
  REQUIRE(catalog.resolve_active_hashimyei(
      replacement_manifest.canonical_path, replacement_manifest.family,
      replacement_manifest.contract_identity.hash_sha256_hex, &active_hashimyei,
      &error));
  REQUIRE(active_hashimyei == replacement_manifest.hashimyei_identity.name);

  bool replacement_inserted_again = true;
  REQUIRE(catalog.register_component_manifest(
      replacement_manifest, nullptr, &replacement_inserted_again, &error));
  REQUIRE(!replacement_inserted_again);

  component_manifest_t auto_component_a = make_auto_contract_component_manifest(
      0x31, sixty_four_hex('1'), 'e', 1711111114000ULL);
  std::string auto_component_id_a{};
  bool auto_inserted_a = false;
  REQUIRE(catalog.register_component_manifest(
      auto_component_a, &auto_component_id_a, &auto_inserted_a, &error));
  REQUIRE(auto_inserted_a);

  component_state_t resolved_auto_a{};
  REQUIRE(catalog.resolve_component(auto_component_a.canonical_path, "",
                                    &resolved_auto_a, &error));
  REQUIRE(!resolved_auto_a.manifest.contract_identity.name.empty());

  component_manifest_t auto_component_b = make_auto_contract_component_manifest(
      0x32, sixty_four_hex('1'), 'f', 1711111115000ULL);
  std::string auto_component_id_b{};
  bool auto_inserted_b = false;
  REQUIRE(catalog.register_component_manifest(
      auto_component_b, &auto_component_id_b, &auto_inserted_b, &error));
  REQUIRE(auto_inserted_b);

  component_state_t resolved_auto_b{};
  REQUIRE(catalog.resolve_component(auto_component_b.canonical_path, "",
                                    &resolved_auto_b, &error));
  REQUIRE(resolved_auto_b.manifest.contract_identity.hash_sha256_hex ==
          resolved_auto_a.manifest.contract_identity.hash_sha256_hex);

  const std::string auto_contract_hash =
      resolved_auto_a.manifest.contract_identity.hash_sha256_hex;
  const std::string auto_component_compatibility_sha256_hex =
      resolved_auto_a.manifest.component_compatibility_sha256_hex;
  REQUIRE(!auto_component_compatibility_sha256_hex.empty());
  cuwacunu::iitepi::config_space_t::change_config_file(kGlobalConfigPath);
  cuwacunu::iitepi::config_space_t::update_config();
  const std::string requested_contract_hash =
      cuwacunu::iitepi::contract_space_t::register_contract_file(
          kDefaultContractPath);
  const auto requested_contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(
          requested_contract_hash);
  REQUIRE(static_cast<bool>(requested_contract_snapshot));
  const std::string requested_contract_docking =
      requested_contract_snapshot->signature.docking_signature_sha256_hex;
  REQUIRE(!requested_contract_docking.empty());
  std::string requested_contract_component_compatibility{};
  std::string requested_contract_component_tag{};
  for (const auto &component_signature :
       requested_contract_snapshot->component_compatibility_signatures) {
    if (component_signature.family ==
        "tsi.wikimyei.representation.encoding.vicreg") {
      requested_contract_component_compatibility =
          component_signature.sha256_hex;
      requested_contract_component_tag = component_signature.component_tag;
      break;
    }
  }
  REQUIRE(!requested_contract_component_compatibility.empty());
  REQUIRE(!requested_contract_component_tag.empty());

  cuwacunu::hero::family_rank::state_t bootstrap_rank{};
  REQUIRE(!catalog.get_family_rank(
      "tsi.wikimyei.representation.encoding.vicreg",
      component_manifest.component_compatibility_sha256_hex, &bootstrap_rank,
      &error));
  REQUIRE(error.find("family rank not found") != std::string::npos);

  cuwacunu::hero::hashimyei_mcp::app_context_t rank_app{};
  rank_app.store_root = store_root;
  rank_app.lattice_catalog_path =
      store_root / "_meta" / "catalog" / "lattice_catalog.idydb";
  rank_app.global_config_path = kGlobalConfigPath;
  rank_app.catalog_options = options;
  std::string tool_result{};
  std::string tool_error{};
  REQUIRE(catalog.close(&error));
  if (!cuwacunu::hero::hashimyei_mcp::execute_tool_json(
          "hero.hashimyei.get_component_manifest",
          "{\"hashimyei\":\"" + component_manifest.hashimyei_identity.name +
              "\"}",
          &rank_app, &tool_result, &tool_error)) {
    std::cerr << "[test_hashimyei_catalog] get_component_manifest error: "
              << tool_error << "\n";
    return 1;
  }
  REQUIRE(tool_error.empty());
  REQUIRE(tool_result.find("\"isError\":false") != std::string::npos);
  REQUIRE(fs::exists(rank_app.catalog_options.catalog_path));

  fs::remove(component_manifest_file);
  tool_result.clear();
  tool_error.clear();
  REQUIRE(cuwacunu::hero::hashimyei_mcp::execute_tool_json(
      "hero.hashimyei.get_component_manifest",
      "{\"hashimyei\":\"" + component_manifest.hashimyei_identity.name + "\"}",
      &rank_app, &tool_result, &tool_error));
  REQUIRE(tool_error.empty());
  REQUIRE(tool_result.find("\"isError\":true") != std::string::npos);
  REQUIRE(tool_result.find("component not found") != std::string::npos);

  REQUIRE(
      save_component_manifest(store_root, component_manifest, nullptr, &error));
  component_manifest_t component_compatible_manifest =
      make_component_manifest();
  component_compatible_manifest.hashimyei_identity =
      make_test_identity(hashimyei_kind_e::TSIEMENE, 0x41);
  component_compatible_manifest.canonical_path =
      component_compatible_manifest.family + "." +
      component_compatible_manifest.hashimyei_identity.name;
  component_compatible_manifest.founding_revision_id =
      "cfgrev.component.compatible";
  component_compatible_manifest.component_tag =
      requested_contract_component_tag;
  component_compatible_manifest.docking_signature_sha256_hex =
      requested_contract_docking;
  component_compatible_manifest.component_compatibility_sha256_hex =
      requested_contract_component_compatibility;
  component_compatible_manifest.created_at_ms = 1711111116100ULL;
  component_compatible_manifest.updated_at_ms = 1711111116100ULL;
  REQUIRE(save_component_manifest(store_root, component_compatible_manifest,
                                  nullptr, &error));

  component_manifest_t component_incompatible_manifest =
      component_compatible_manifest;
  component_incompatible_manifest.hashimyei_identity =
      make_test_identity(hashimyei_kind_e::TSIEMENE, 0x42);
  component_incompatible_manifest.canonical_path =
      component_incompatible_manifest.family + "." +
      component_incompatible_manifest.hashimyei_identity.name;
  component_incompatible_manifest.founding_revision_id =
      "cfgrev.component.incompatible";
  component_incompatible_manifest.component_compatibility_sha256_hex =
      sixty_four_hex('f');
  if (component_incompatible_manifest.component_compatibility_sha256_hex ==
      requested_contract_component_compatibility) {
    component_incompatible_manifest.component_compatibility_sha256_hex =
        sixty_four_hex('e');
  }
  component_incompatible_manifest.created_at_ms = 1711111116200ULL;
  component_incompatible_manifest.updated_at_ms = 1711111116200ULL;
  REQUIRE(save_component_manifest(store_root, component_incompatible_manifest,
                                  nullptr, &error));

  tool_result.clear();
  tool_error.clear();
  REQUIRE(cuwacunu::hero::hashimyei_mcp::execute_tool_json(
      "hero.hashimyei.evaluate_contract_compatibility",
      "{\"hashimyei\":\"" +
          component_compatible_manifest.hashimyei_identity.name +
          "\",\"contract_dsl_path\":\"" + std::string(kDefaultContractPath) +
          "\"}",
      &rank_app, &tool_result, &tool_error));
  REQUIRE(tool_error.empty());
  REQUIRE(tool_result.find("\"isError\":false") != std::string::npos);
  REQUIRE(tool_result.find("\"compatible\":true") != std::string::npos);
  REQUIRE(
      tool_result.find("\"compatibility_basis\":\"component_compatibility\"") !=
      std::string::npos);
  REQUIRE(tool_result.find("component-compatible") != std::string::npos);
  REQUIRE(tool_result.find("\"founding_contract_match\":false") !=
          std::string::npos);

  tool_result.clear();
  tool_error.clear();
  REQUIRE(cuwacunu::hero::hashimyei_mcp::execute_tool_json(
      "hero.hashimyei.evaluate_contract_compatibility",
      "{\"hashimyei\":\"" +
          component_incompatible_manifest.hashimyei_identity.name +
          "\",\"contract_hash\":\"" + requested_contract_hash + "\"}",
      &rank_app, &tool_result, &tool_error));
  REQUIRE(tool_error.empty());
  REQUIRE(tool_result.find("\"isError\":false") != std::string::npos);
  REQUIRE(tool_result.find("\"compatible\":false") != std::string::npos);
  REQUIRE(tool_result.find("not component-compatible") != std::string::npos);
  REQUIRE(tool_result.find("compatibility signature mismatch") !=
          std::string::npos);

  tool_result.clear();
  tool_error.clear();
  REQUIRE(cuwacunu::hero::hashimyei_mcp::execute_tool_json(
      "hero.hashimyei.evaluate_contract_compatibility",
      "{\"hashimyei\":\"" +
          component_compatible_manifest.hashimyei_identity.name +
          "\",\"contract_hash\":\"" + sixty_four_hex('9') + "\"}",
      &rank_app, &tool_result, &tool_error));
  REQUIRE(tool_error.empty());
  REQUIRE(tool_result.find("\"isError\":true") != std::string::npos);
  REQUIRE(tool_result.find("provide arguments.contract_dsl_path") !=
          std::string::npos);

  if (!cuwacunu::hero::hashimyei_mcp::execute_tool_json(
          "hero.hashimyei.update_rank",
          "{\"family\":\"tsi.wikimyei.representation.encoding.vicreg\","
          "\"component_compatibility_sha256_hex\":\"" +
              auto_component_compatibility_sha256_hex +
              "\","
              "\"ordered_hashimyeis\":[\"0x0032\",\"0x0031\"],"
              "\"source_view_kind\":\"family_evaluation_report\","
              "\"source_view_transport_sha256\":"
              "\"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcd"
              "ef\"}",
          &rank_app, &tool_result, &tool_error)) {
    std::cerr << "[test_hashimyei_catalog] update_rank error: " << tool_error
              << "\n";
    return 1;
  }
  REQUIRE(tool_error.empty());
  if (tool_result.find("\"isError\":false") == std::string::npos) {
    std::cerr << "[test_hashimyei_catalog] unexpected tool_result: "
              << tool_result << "\n";
    return 1;
  }
  REQUIRE(tool_result.find("\"changed\":true") != std::string::npos);
  REQUIRE(tool_result.find("\"synchronized_lattice\":true") !=
          std::string::npos);

  REQUIRE(catalog.open(options, &error));
  REQUIRE(catalog.ingest_filesystem(store_root, &error));

  cuwacunu::hero::family_rank::state_t family_rank{};
  REQUIRE(catalog.get_family_rank("tsi.wikimyei.representation.encoding.vicreg",
                                  auto_component_compatibility_sha256_hex,
                                  &family_rank, &error));
  REQUIRE(family_rank.assignments.size() == 2);
  REQUIRE(family_rank.assignments[0].hashimyei == "0x0032");
  REQUIRE(family_rank.assignments[1].hashimyei == "0x0031");
  REQUIRE(family_rank.component_compatibility_sha256_hex ==
          auto_component_compatibility_sha256_hex);

  std::string ranked_zero_hashimyei{};
  REQUIRE(catalog.resolve_ranked_hashimyei(
      "tsi.wikimyei.representation.encoding.vicreg",
      auto_component_compatibility_sha256_hex, 0, &ranked_zero_hashimyei,
      &error));
  REQUIRE(ranked_zero_hashimyei == "0x0032");

  std::vector<component_state_t> ranked_components{};
  REQUIRE(
      catalog.list_components("", "", 0, 0, true, &ranked_components, &error));
  std::unordered_map<std::string, std::optional<std::uint64_t>>
      observed_ranks{};
  for (const auto &component : ranked_components) {
    if (component.manifest.family !=
        "tsi.wikimyei.representation.encoding.vicreg")
      continue;
    if (component.manifest.contract_identity.hash_sha256_hex !=
        auto_contract_hash) {
      continue;
    }
    observed_ranks[component.manifest.hashimyei_identity.name] =
        component.family_rank;
  }
  REQUIRE(observed_ranks["0x0032"].has_value());
  REQUIRE(*observed_ranks["0x0032"] == 0);
  REQUIRE(observed_ranks["0x0031"].has_value());
  REQUIRE(*observed_ranks["0x0031"] == 1);

  tool_result.clear();
  tool_error.clear();
  REQUIRE(catalog.close(&error));
  if (!cuwacunu::hero::hashimyei_mcp::execute_tool_json(
          "hero.hashimyei.update_rank",
          "{\"family\":\"tsi.wikimyei.representation.encoding.vicreg\","
          "\"component_compatibility_sha256_hex\":\"" +
              auto_component_compatibility_sha256_hex +
              "\","
              "\"ordered_hashimyeis\":[\"0x0032\",\"0x0031\"]}",
          &rank_app, &tool_result, &tool_error)) {
    std::cerr << "[test_hashimyei_catalog] update_rank error: " << tool_error
              << "\n";
    return 1;
  }
  REQUIRE(tool_error.empty());
  REQUIRE(tool_result.find("\"isError\":false") != std::string::npos);
  REQUIRE(tool_result.find("\"changed\":false") != std::string::npos);
  REQUIRE(catalog.open(options, &error));
  REQUIRE(catalog.ingest_filesystem(store_root, &error));

  component_manifest_t auto_component_c = make_auto_contract_component_manifest(
      0x43, sixty_four_hex('1'), 'a', 1711111116000ULL);
  std::string auto_component_id_c{};
  bool auto_inserted_c = false;
  if (!catalog.register_component_manifest(
          auto_component_c, &auto_component_id_c, &auto_inserted_c, &error)) {
    std::cerr << "[test_hashimyei_catalog] register_component_manifest error: "
              << error << "\n";
    return 1;
  }
  REQUIRE(auto_inserted_c);

  cuwacunu::hero::family_rank::state_t normalized_rank{};
  REQUIRE(catalog.get_family_rank("tsi.wikimyei.representation.encoding.vicreg",
                                  auto_component_compatibility_sha256_hex,
                                  &normalized_rank, &error));
  REQUIRE(normalized_rank.assignments.size() == 2);
  REQUIRE(normalized_rank.assignments[0].hashimyei == "0x0032");
  REQUIRE(normalized_rank.assignments[1].hashimyei == "0x0031");

  ranked_components.clear();
  REQUIRE(
      catalog.list_components("", "", 0, 0, true, &ranked_components, &error));
  observed_ranks.clear();
  for (const auto &component : ranked_components) {
    if (component.manifest.family !=
        "tsi.wikimyei.representation.encoding.vicreg")
      continue;
    if (component.manifest.contract_identity.hash_sha256_hex !=
        auto_contract_hash) {
      continue;
    }
    observed_ranks[component.manifest.hashimyei_identity.name] =
        component.family_rank;
  }
  REQUIRE(observed_ranks["0x0043"] == std::nullopt);

  tool_result.clear();
  tool_error.clear();
  REQUIRE(catalog.close(&error));
  if (!cuwacunu::hero::hashimyei_mcp::execute_tool_json(
          "hero.hashimyei.update_rank",
          "{\"family\":\"tsi.wikimyei.representation.encoding.vicreg\","
          "\"component_compatibility_sha256_hex\":\"" +
              auto_component_compatibility_sha256_hex +
              "\","
              "\"ordered_hashimyeis\":[\"0x0032\",\"0x0031\"]}",
          &rank_app, &tool_result, &tool_error)) {
    std::cerr << "[test_hashimyei_catalog] update_rank error: " << tool_error
              << "\n";
    return 1;
  }
  REQUIRE(tool_error.empty());
  REQUIRE(tool_result.find("\"isError\":true") != std::string::npos);
  REQUIRE(tool_result.find(
              "must enumerate every current family member exactly once") !=
          std::string::npos);

  tool_result.clear();
  tool_error.clear();
  REQUIRE(cuwacunu::hero::hashimyei_mcp::execute_tool_json(
      "hero.hashimyei.update_rank",
      "{\"family\":\"tsi.wikimyei.representation.encoding.vicreg\","
      "\"component_compatibility_sha256_hex\":\"" +
          auto_component_compatibility_sha256_hex +
          "\","
          "\"ordered_hashimyeis\":[\"0x0032\",\"0x0032\"]}",
      &rank_app, &tool_result, &tool_error));
  REQUIRE(tool_error.empty());
  REQUIRE(tool_result.find("\"isError\":true") != std::string::npos);
  REQUIRE(tool_result.find("duplicates") != std::string::npos);

  cuwacunu::hero::wave::lattice_catalog_store_t lattice_catalog{};
  cuwacunu::hero::wave::lattice_catalog_store_t::options_t lattice_options{};
  lattice_options.catalog_path = rank_app.lattice_catalog_path;
  lattice_options.encrypted = false;
  lattice_options.projection_version = 2;
  REQUIRE(lattice_catalog.open(lattice_options, &error));
  cuwacunu::hero::family_rank::state_t lattice_family_rank{};
  REQUIRE(lattice_catalog.get_family_rank(
      "tsi.wikimyei.representation.encoding.vicreg",
      auto_component_compatibility_sha256_hex, &lattice_family_rank, &error));
  REQUIRE(lattice_family_rank.assignments.size() ==
          family_rank.assignments.size());
  for (std::size_t i = 0; i < family_rank.assignments.size(); ++i) {
    REQUIRE(lattice_family_rank.assignments[i].hashimyei ==
            family_rank.assignments[i].hashimyei);
    REQUIRE(lattice_family_rank.assignments[i].rank ==
            family_rank.assignments[i].rank);
  }
  REQUIRE(lattice_catalog.close(&error));

  // strict v2 ingest: v1 run filename must fail fast
  const fs::path unsupported_v1_run_manifest =
      store_root / "_meta" / "runs" / "unsupported" / "run.manifest.v1.kv";
  write_text_file(unsupported_v1_run_manifest,
                  "schema=hashimyei.run.manifest.v1\nrun_id=v1\n");
  REQUIRE(!catalog.ingest_filesystem(store_root, &error));
  REQUIRE(!error.empty());

  REQUIRE(catalog.close(&error));

  std::cout << "[PASS] test_hashimyei_catalog\n";
  return 0;
}
