#include <hero/hashimyei_hero/hashimyei_catalog.h>
#include <hero/hero_catalog_schema.h>

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

static void require_impl(bool ok, const char* expr, const char* file, int line) {
  if (!ok) {
    std::cerr << "[TEST FAIL] " << file << ":" << line << "  (" << expr << ")\n";
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
    dir = fs::temp_directory_path() / ("test_hashimyei_catalog_" + random_suffix());
    fs::create_directories(dir);
  }
  ~temp_dir_t() {
    std::error_code ec;
    fs::remove_all(dir, ec);
  }
};

static std::string hex_lower(const unsigned char* bytes, std::size_t n) {
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
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
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

static std::string sixty_four_hex(char c) {
  return std::string(64, c);
}

static hashimyei_t make_test_identity(hashimyei_kind_e kind, std::uint64_t ordinal,
                                 std::string hash = {}) {
  hashimyei_t id = cuwacunu::hashimyei::make_identity(kind, ordinal, std::move(hash));
  id.schema = cuwacunu::hashimyei::kIdentitySchemaV2;
  return id;
}

static wave_contract_binding_t make_binding(std::string alias,
                                            std::uint64_t contract_ordinal = 1,
                                            std::uint64_t wave_ordinal = 2,
                                            std::uint64_t binding_ordinal = 3) {
  wave_contract_binding_t out{};
  out.contract = make_test_identity(hashimyei_kind_e::CONTRACT, contract_ordinal,
                               sixty_four_hex('a'));
  out.wave = make_test_identity(hashimyei_kind_e::WAVE, wave_ordinal,
                           sixty_four_hex('b'));
  out.binding_alias = std::move(alias);
  out.identity = make_test_identity(
      hashimyei_kind_e::WAVE_CONTRACT_BINDING, binding_ordinal,
      sha256_hex(out.contract.hash_sha256_hex + "|" + out.wave.hash_sha256_hex + "|" +
                 out.binding_alias));
  return out;
}

static void write_text_file(const fs::path& path, const std::string& payload) {
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  REQUIRE(static_cast<bool>(out));
  out << payload;
  REQUIRE(static_cast<bool>(out));
}

static std::string component_payload(const component_manifest_t& m) {
  std::ostringstream out;
  out << "schema=" << m.schema << "\n";
  out << "canonical_path=" << m.canonical_path << "\n";
  out << "family=" << m.family << "\n";
  out << "tsi_type=" << m.tsi_type << "\n";
  out << "component_identity.schema=" << m.component_identity.schema << "\n";
  out << "component_identity.kind="
      << cuwacunu::hashimyei::hashimyei_kind_to_string(m.component_identity.kind) << "\n";
  out << "component_identity.name=" << m.component_identity.name << "\n";
  out << "component_identity.ordinal=" << m.component_identity.ordinal << "\n";
  out << "component_identity.hash_sha256_hex=" << m.component_identity.hash_sha256_hex
      << "\n";
  out << "parent_identity.present=" << (m.parent_identity.has_value() ? "1" : "0")
      << "\n";
  if (m.parent_identity.has_value()) {
    out << "parent_identity.schema=" << m.parent_identity->schema << "\n";
    out << "parent_identity.kind="
        << cuwacunu::hashimyei::hashimyei_kind_to_string(m.parent_identity->kind)
        << "\n";
    out << "parent_identity.name=" << m.parent_identity->name << "\n";
    out << "parent_identity.ordinal=" << m.parent_identity->ordinal << "\n";
    out << "parent_identity.hash_sha256_hex="
        << m.parent_identity->hash_sha256_hex << "\n";
  }
  out << "revision_reason=" << m.revision_reason << "\n";
  out << "config_revision_id=" << m.config_revision_id << "\n";
  out << "wave_contract_binding.identity.schema="
      << m.wave_contract_binding.identity.schema << "\n";
  out << "wave_contract_binding.identity.kind="
      << cuwacunu::hashimyei::hashimyei_kind_to_string(
             m.wave_contract_binding.identity.kind)
      << "\n";
  out << "wave_contract_binding.identity.name="
      << m.wave_contract_binding.identity.name << "\n";
  out << "wave_contract_binding.identity.ordinal="
      << m.wave_contract_binding.identity.ordinal << "\n";
  out << "wave_contract_binding.identity.hash_sha256_hex="
      << m.wave_contract_binding.identity.hash_sha256_hex << "\n";
  out << "wave_contract_binding.contract.schema="
      << m.wave_contract_binding.contract.schema << "\n";
  out << "wave_contract_binding.contract.kind="
      << cuwacunu::hashimyei::hashimyei_kind_to_string(
             m.wave_contract_binding.contract.kind)
      << "\n";
  out << "wave_contract_binding.contract.name="
      << m.wave_contract_binding.contract.name << "\n";
  out << "wave_contract_binding.contract.ordinal="
      << m.wave_contract_binding.contract.ordinal << "\n";
  out << "wave_contract_binding.contract.hash_sha256_hex="
      << m.wave_contract_binding.contract.hash_sha256_hex << "\n";
  out << "wave_contract_binding.wave.schema=" << m.wave_contract_binding.wave.schema
      << "\n";
  out << "wave_contract_binding.wave.kind="
      << cuwacunu::hashimyei::hashimyei_kind_to_string(
             m.wave_contract_binding.wave.kind)
      << "\n";
  out << "wave_contract_binding.wave.name=" << m.wave_contract_binding.wave.name
      << "\n";
  out << "wave_contract_binding.wave.ordinal="
      << m.wave_contract_binding.wave.ordinal << "\n";
  out << "wave_contract_binding.wave.hash_sha256_hex="
      << m.wave_contract_binding.wave.hash_sha256_hex << "\n";
  out << "wave_contract_binding.binding_alias="
      << m.wave_contract_binding.binding_alias << "\n";
  out << "dsl_canonical_path=" << m.dsl_canonical_path << "\n";
  out << "dsl_sha256_hex=" << m.dsl_sha256_hex << "\n";
  out << "status=" << m.status << "\n";
  out << "replaced_by=" << m.replaced_by << "\n";
  out << "created_at_ms=" << m.created_at_ms << "\n";
  out << "updated_at_ms=" << m.updated_at_ms << "\n";
  return out.str();
}

static run_manifest_t make_run_manifest() {
  run_manifest_t m{};
  m.schema = cuwacunu::hashimyei::kRunManifestSchemaV2;
  m.started_at_ms = 1711111111000ULL;
  m.board_identity = make_test_identity(hashimyei_kind_e::BOARD, 4);
  m.wave_contract_binding = make_binding("bind_train_vicreg");
  m.sampler = "uniform";
  m.record_type = "train";
  m.seed = "42";
  m.device = "cpu";
  m.dtype = "float32";
  m.run_id = compute_run_id(m.board_identity, m.wave_contract_binding, m.started_at_ms);
  m.dependency_files.push_back({"iitepi.board.bind_train_vicreg", "sha.board"});
  m.dependency_files.push_back({"iitepi.contract.bind_train_vicreg", "sha.contract"});
  m.components.push_back({"tsi.source.dataloader", "tsi.source.dataloader", ""});
  m.components.push_back({"tsi.wikimyei.representation.vicreg.0x0010",
                          "tsi.wikimyei.representation.vicreg", "0x0010"});
  return m;
}

static component_manifest_t make_component_manifest() {
  component_manifest_t m{};
  m.schema = cuwacunu::hashimyei::kComponentManifestSchemaV2;
  m.canonical_path = "tsi.wikimyei.representation.vicreg.0x0010";
  m.family = "tsi.wikimyei.representation.vicreg";
  m.tsi_type = "tsi.wikimyei.representation.vicreg";
  m.component_identity = make_test_identity(hashimyei_kind_e::TSIEMENE, 0x10);
  m.revision_reason = "initial";
  m.config_revision_id = "cfgrev.initial";
  m.wave_contract_binding = make_binding("bind_train_vicreg");
  m.dsl_canonical_path =
      "src/config/instructions/default.tsi.wikimyei.representation.vicreg.network_design.dsl";
  m.dsl_sha256_hex = sixty_four_hex('c');
  m.status = "active";
  m.created_at_ms = 1711111111000ULL;
  m.updated_at_ms = 1711111111001ULL;
  return m;
}

static component_manifest_t make_auto_binding_component_manifest(
    std::uint64_t component_ordinal, std::string_view contract_sha,
    std::string_view wave_sha, std::string_view binding_alias, char dsl_fill,
    std::uint64_t ts_ms) {
  component_manifest_t m = make_component_manifest();
  m.component_identity = make_test_identity(hashimyei_kind_e::TSIEMENE, component_ordinal);
  m.canonical_path = m.family + "." + m.component_identity.name;
  m.config_revision_id = "cfgrev.auto." + m.component_identity.name;
  m.wave_contract_binding.contract =
      make_test_identity(hashimyei_kind_e::CONTRACT, 0, std::string(contract_sha));
  m.wave_contract_binding.contract.name.clear();
  m.wave_contract_binding.contract.ordinal = 0;
  m.wave_contract_binding.wave =
      make_test_identity(hashimyei_kind_e::WAVE, 0, std::string(wave_sha));
  m.wave_contract_binding.wave.name.clear();
  m.wave_contract_binding.wave.ordinal = 0;
  m.wave_contract_binding.binding_alias = std::string(binding_alias);
  const std::string binding_sha =
      sha256_hex(m.wave_contract_binding.contract.hash_sha256_hex + "|" +
                 m.wave_contract_binding.wave.hash_sha256_hex + "|" +
                 m.wave_contract_binding.binding_alias);
  m.wave_contract_binding.identity =
      make_test_identity(hashimyei_kind_e::WAVE_CONTRACT_BINDING, 0, binding_sha);
  m.wave_contract_binding.identity.name.clear();
  m.wave_contract_binding.identity.ordinal = 0;
  m.dsl_sha256_hex = sixty_four_hex(dsl_fill);
  m.created_at_ms = ts_ms;
  m.updated_at_ms = ts_ms;
  return m;
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
  REQUIRE(cuwacunu::hashimyei::parse_hex_hash_name_ordinal("0x001a", &parsed_ordinal));
  REQUIRE(parsed_ordinal == 0x1a);

  hashimyei_t missing_contract_hash = make_test_identity(hashimyei_kind_e::CONTRACT, 7, "");
  std::string identity_error;
  REQUIRE(!cuwacunu::hashimyei::validate_hashimyei(missing_contract_hash, &identity_error));
  REQUIRE(identity_error.find("hash_sha256_hex") != std::string::npos);

  hashimyei_t board_with_hash = make_test_identity(hashimyei_kind_e::BOARD, 5, sixty_four_hex('f'));
  REQUIRE(!cuwacunu::hashimyei::validate_hashimyei(board_with_hash, &identity_error));

  const wave_contract_binding_t binding_a = make_binding("bind_A", 1, 2, 3);
  const wave_contract_binding_t binding_b = make_binding("bind_A", 1, 2, 3);
  REQUIRE(binding_a.identity.hash_sha256_hex == binding_b.identity.hash_sha256_hex);

  temp_dir_t tmp{};
  const fs::path store_root = tmp.dir / ".hashimyei";
  const fs::path catalog_path = store_root / "catalog" / "hashimyei_catalog.idydb";
  constexpr const char* kPassphrase = "catalog-test-pass";

  const run_manifest_t manifest = make_run_manifest();

  fs::path run_manifest_path{};
  std::string error;
  REQUIRE(save_run_manifest(store_root, manifest, &run_manifest_path, &error));
  REQUIRE(run_manifest_path.filename() == cuwacunu::hashimyei::kRunManifestFilenameV2);

  run_manifest_t loaded_run{};
  REQUIRE(load_run_manifest(run_manifest_path, &loaded_run, &error));
  REQUIRE(loaded_run.run_id == manifest.run_id);
  REQUIRE(loaded_run.board_identity.name == manifest.board_identity.name);
  REQUIRE(loaded_run.wave_contract_binding.identity.hash_sha256_hex ==
          manifest.wave_contract_binding.identity.hash_sha256_hex);

  component_manifest_t component_manifest = make_component_manifest();
  const fs::path component_manifest_path =
      store_root / "tsi.wikimyei" / "representation" / "vicreg" /
      component_manifest.component_identity.name /
      cuwacunu::hashimyei::kComponentManifestFilenameV2;
  write_text_file(component_manifest_path, component_payload(component_manifest));

  component_manifest_t loaded_component{};
  REQUIRE(load_component_manifest(component_manifest_path, &loaded_component, &error));
  REQUIRE(loaded_component.component_identity.name ==
          component_manifest.component_identity.name);
  REQUIRE(loaded_component.wave_contract_binding.identity.hash_sha256_hex ==
          component_manifest.wave_contract_binding.identity.hash_sha256_hex);

  const fs::path invalid_v1_filename =
      tmp.dir / "invalid_manifests" / "tsi.wikimyei" / "representation" / "vicreg" /
      "0x9999" /
      "component.manifest.v1.kv";
  write_text_file(invalid_v1_filename, component_payload(component_manifest));
  REQUIRE(!load_component_manifest(invalid_v1_filename, &loaded_component, &error));
  REQUIRE(error.find("v1") != std::string::npos);

  const fs::path invalid_v1_schema =
      tmp.dir / "invalid_manifests" / "tsi.wikimyei" / "representation" / "vicreg" /
      "0x9998" /
      cuwacunu::hashimyei::kComponentManifestFilenameV2;
  component_manifest_t invalid_schema_manifest = component_manifest;
  invalid_schema_manifest.schema = "hashimyei.component.manifest.v1";
  write_text_file(invalid_v1_schema, component_payload(invalid_schema_manifest));
  REQUIRE(!load_component_manifest(invalid_v1_schema, &loaded_component, &error));
  REQUIRE(error.find("schema") != std::string::npos);

  hashimyei_catalog_store_t catalog{};
  hashimyei_catalog_store_t::options_t options{};
  options.catalog_path = catalog_path;
  options.encrypted = true;
  options.passphrase = kPassphrase;
  options.ingest_version = 2;

  REQUIRE(catalog.open(options, &error));
  REQUIRE(catalog.ingest_filesystem(store_root, &error));

  std::vector<run_manifest_t> runs{};
  REQUIRE(catalog.list_runs_by_binding(component_manifest.wave_contract_binding.contract.name,
                                       component_manifest.wave_contract_binding.wave.name,
                                       component_manifest.wave_contract_binding.identity.name,
                                       &runs, &error));
  REQUIRE(runs.size() == 1);
  REQUIRE(runs[0].run_id == manifest.run_id);

  runs.clear();
  REQUIRE(catalog.list_runs_by_binding(
      component_manifest.wave_contract_binding.contract.hash_sha256_hex, "", "", &runs,
      &error));
  REQUIRE(runs.empty());

  component_state_t resolved_component{};
  REQUIRE(catalog.resolve_component("", component_manifest.component_identity.name,
                                    &resolved_component, &error));
  REQUIRE(resolved_component.manifest.component_identity.name ==
          component_manifest.component_identity.name);

  component_manifest_t cutover_manifest = component_manifest;
  cutover_manifest.parent_identity = component_manifest.component_identity;
  cutover_manifest.component_identity = make_test_identity(hashimyei_kind_e::TSIEMENE, 0x21);
  cutover_manifest.canonical_path = cutover_manifest.family + "." +
                                    cutover_manifest.component_identity.name;
  cutover_manifest.revision_reason = "dsl_change";
  cutover_manifest.config_revision_id = "cfgrev.cutover";
  cutover_manifest.dsl_sha256_hex = sixty_four_hex('d');
  cutover_manifest.created_at_ms = 1711111113000ULL;
  cutover_manifest.updated_at_ms = 1711111113000ULL;

  std::string cutover_component_id{};
  bool cutover_inserted = false;
  REQUIRE(catalog.register_component_manifest(cutover_manifest, &cutover_component_id,
                                              &cutover_inserted, &error));
  REQUIRE(cutover_inserted);

  std::string active_hashimyei{};
  REQUIRE(catalog.resolve_active_hashimyei(cutover_manifest.family, "", &active_hashimyei,
                                           &error));
  REQUIRE(active_hashimyei == cutover_manifest.component_identity.name);

  bool cutover_inserted_again = true;
  REQUIRE(catalog.register_component_manifest(cutover_manifest, nullptr,
                                              &cutover_inserted_again, &error));
  REQUIRE(!cutover_inserted_again);

  component_manifest_t auto_component_a =
      make_auto_binding_component_manifest(0x31, sixty_four_hex('1'),
                                           sixty_four_hex('2'), "bind_auto", 'e',
                                           1711111114000ULL);
  std::string auto_component_id_a{};
  bool auto_inserted_a = false;
  REQUIRE(catalog.register_component_manifest(auto_component_a, &auto_component_id_a,
                                              &auto_inserted_a, &error));
  REQUIRE(auto_inserted_a);

  component_state_t resolved_auto_a{};
  REQUIRE(catalog.resolve_component(auto_component_a.canonical_path, "",
                                    &resolved_auto_a, &error));
  REQUIRE(!resolved_auto_a.manifest.wave_contract_binding.contract.name.empty());
  REQUIRE(!resolved_auto_a.manifest.wave_contract_binding.wave.name.empty());
  REQUIRE(!resolved_auto_a.manifest.wave_contract_binding.identity.name.empty());

  component_manifest_t auto_component_b =
      make_auto_binding_component_manifest(0x32, sixty_four_hex('1'),
                                           sixty_four_hex('2'), "bind_auto", 'f',
                                           1711111115000ULL);
  std::string auto_component_id_b{};
  bool auto_inserted_b = false;
  REQUIRE(catalog.register_component_manifest(auto_component_b, &auto_component_id_b,
                                              &auto_inserted_b, &error));
  REQUIRE(auto_inserted_b);

  component_state_t resolved_auto_b{};
  REQUIRE(catalog.resolve_component(auto_component_b.canonical_path, "",
                                    &resolved_auto_b, &error));
  REQUIRE(resolved_auto_b.manifest.wave_contract_binding.contract.name ==
          resolved_auto_a.manifest.wave_contract_binding.contract.name);
  REQUIRE(resolved_auto_b.manifest.wave_contract_binding.wave.name ==
          resolved_auto_a.manifest.wave_contract_binding.wave.name);
  REQUIRE(resolved_auto_b.manifest.wave_contract_binding.identity.name ==
          resolved_auto_a.manifest.wave_contract_binding.identity.name);

  std::vector<component_state_t> binding_components{};
  REQUIRE(catalog.list_components_by_binding(
      resolved_auto_a.manifest.wave_contract_binding.contract.name,
      resolved_auto_a.manifest.wave_contract_binding.wave.name,
      resolved_auto_a.manifest.wave_contract_binding.identity.name, 0, 0, true,
      &binding_components, &error));
  REQUIRE(binding_components.size() == 2);
  REQUIRE(binding_components[0].manifest.wave_contract_binding.identity.name ==
          resolved_auto_a.manifest.wave_contract_binding.identity.name);
  REQUIRE(binding_components[1].manifest.wave_contract_binding.identity.name ==
          resolved_auto_a.manifest.wave_contract_binding.identity.name);

  // strict v2 ingest: v1 run filename must fail fast
  const fs::path unsupported_v1_run_manifest =
      store_root / "runs" / "unsupported" / "run.manifest.v1.kv";
  write_text_file(unsupported_v1_run_manifest,
                  "schema=hashimyei.run.manifest.v1\nrun_id=v1\n");
  REQUIRE(!catalog.ingest_filesystem(store_root, &error));
  REQUIRE(error.find("v1") != std::string::npos);

  REQUIRE(catalog.close(&error));

  std::cout << "[PASS] test_hashimyei_catalog\n";
  return 0;
}
