#include <hero/hashimyei_hero/hashimyei_catalog.h>
#include <hero/hashimyei_hero/hero_hashimyei_tools.h>
#include <hero/hero_catalog_schema.h>
#include <hero/lattice_hero/lattice_catalog.h>

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

static std::string erase_line_with_prefix(std::string input,
                                          std::string_view prefix) {
  std::size_t cursor = 0;
  while (cursor < input.size()) {
    const std::size_t line_end = input.find('\n', cursor);
    const std::size_t line_size =
        (line_end == std::string::npos) ? input.size() - cursor
                                        : line_end - cursor;
    if (std::string_view(input).substr(cursor, prefix.size()) == prefix) {
      const std::size_t erase_size =
          (line_end == std::string::npos) ? line_size : line_size + 1;
      input.erase(cursor, erase_size);
      continue;
    }
    if (line_end == std::string::npos) break;
    cursor = line_end + 1;
  }
  return input;
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
  out.binding_id = std::move(alias);
  out.identity = make_test_identity(
      hashimyei_kind_e::WAVE_CONTRACT_BINDING, binding_ordinal,
      sha256_hex(out.contract.hash_sha256_hex + "|" + out.wave.hash_sha256_hex + "|" +
                 out.binding_id));
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
  out << "hashimyei_identity.schema=" << m.hashimyei_identity.schema << "\n";
  out << "hashimyei_identity.kind="
      << cuwacunu::hashimyei::hashimyei_kind_to_string(m.hashimyei_identity.kind) << "\n";
  out << "hashimyei_identity.name=" << m.hashimyei_identity.name << "\n";
  out << "hashimyei_identity.ordinal=" << m.hashimyei_identity.ordinal << "\n";
  out << "hashimyei_identity.hash_sha256_hex="
      << m.hashimyei_identity.hash_sha256_hex
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
  out << "founding_revision_id=" << m.founding_revision_id << "\n";
  out << "contract_identity.schema=" << m.contract_identity.schema << "\n";
  out << "contract_identity.kind="
      << cuwacunu::hashimyei::hashimyei_kind_to_string(m.contract_identity.kind)
      << "\n";
  out << "contract_identity.name=" << m.contract_identity.name << "\n";
  out << "contract_identity.ordinal=" << m.contract_identity.ordinal << "\n";
  out << "contract_identity.hash_sha256_hex="
      << m.contract_identity.hash_sha256_hex << "\n";
  out << "founding_dsl_source_path="
      << m.founding_dsl_source_path << "\n";
  out << "founding_dsl_source_sha256_hex="
      << m.founding_dsl_source_sha256_hex << "\n";
  out << "docking_signature_sha256_hex="
      << m.docking_signature_sha256_hex << "\n";
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
  m.dependency_files.push_back({"iitepi.campaign.bind_train_vicreg",
                                "sha.campaign"});
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
  m.hashimyei_identity =
      make_test_identity(hashimyei_kind_e::TSIEMENE, 0x10);
  m.revision_reason = "initial";
  m.founding_revision_id = "cfgrev.initial";
  m.contract_identity = make_binding("bind_train_vicreg").contract;
  m.founding_dsl_source_path =
      "src/config/instructions/defaults/default.tsi.wikimyei.representation.vicreg.network_design.dsl";
  m.founding_dsl_source_sha256_hex = sixty_four_hex('c');
  m.docking_signature_sha256_hex = sixty_four_hex('d');
  m.lineage_state = "active";
  m.created_at_ms = 1711111111000ULL;
  m.updated_at_ms = 1711111111001ULL;
  return m;
}

static component_manifest_t make_auto_contract_component_manifest(
    std::uint64_t component_ordinal, std::string_view contract_sha, char dsl_fill,
    std::uint64_t ts_ms) {
  component_manifest_t m = make_component_manifest();
  m.hashimyei_identity =
      make_test_identity(hashimyei_kind_e::TSIEMENE, component_ordinal);
  m.canonical_path = m.family + "." + m.hashimyei_identity.name;
  m.founding_revision_id = "cfgrev.auto." + m.hashimyei_identity.name;
  m.contract_identity =
      make_test_identity(hashimyei_kind_e::CONTRACT, 0, std::string(contract_sha));
  m.contract_identity.name.clear();
  m.contract_identity.ordinal = 0;
  m.founding_dsl_source_sha256_hex = sixty_four_hex(dsl_fill);
  m.created_at_ms = ts_ms;
  m.updated_at_ms = ts_ms;
  return m;
}

static void test_component_manifest_id_stable_under_source_path_churn() {
  component_manifest_t a = make_component_manifest();
  component_manifest_t b = a;
  a.founding_dsl_source_path =
      "/tmp/runtime_a/instructions/tsi.wikimyei.representation.vicreg.dsl";
  b.founding_dsl_source_path =
      "/tmp/runtime_b/instructions/tsi.wikimyei.representation.vicreg.dsl";

  REQUIRE(cuwacunu::hero::hashimyei::compute_component_manifest_id(a) ==
          cuwacunu::hero::hashimyei::compute_component_manifest_id(b));

  b.founding_dsl_source_sha256_hex = sixty_four_hex('e');
  REQUIRE(cuwacunu::hero::hashimyei::compute_component_manifest_id(a) !=
          cuwacunu::hero::hashimyei::compute_component_manifest_id(b));
}

static void test_founding_bundle_digest_stable_under_source_path_churn() {
  cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_t a{};
  a.component_id = "component.1234";
  a.canonical_path = "tsi.wikimyei.representation.vicreg.0x0010";
  a.hashimyei_name = "0x0010";
  a.files.push_back({"/tmp/runtime_a/instructions/a.dsl", "0000_a.dsl",
                     sixty_four_hex('a')});
  a.files.push_back({"/tmp/runtime_a/instructions/b.dsl", "0001_b.dsl",
                     sixty_four_hex('b')});

  auto b = a;
  b.files[0].source_path = "/tmp/runtime_b/instructions/a.dsl";
  b.files[1].source_path = "/tmp/runtime_b/instructions/b.dsl";

  REQUIRE(cuwacunu::hero::hashimyei::compute_founding_dsl_bundle_aggregate_sha256(a) ==
          cuwacunu::hero::hashimyei::compute_founding_dsl_bundle_aggregate_sha256(b));

  b.files[1].sha256_hex = sixty_four_hex('c');
  REQUIRE(cuwacunu::hero::hashimyei::compute_founding_dsl_bundle_aggregate_sha256(a) !=
          cuwacunu::hero::hashimyei::compute_founding_dsl_bundle_aggregate_sha256(b));
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

  hashimyei_t campaign_with_hash =
      make_test_identity(hashimyei_kind_e::CAMPAIGN, 5, sixty_four_hex('f'));
  REQUIRE(!cuwacunu::hashimyei::validate_hashimyei(campaign_with_hash,
                                                   &identity_error));

  const wave_contract_binding_t binding_a = make_binding("bind_A", 1, 2, 3);
  const wave_contract_binding_t binding_b = make_binding("bind_A", 1, 2, 3);
  REQUIRE(binding_a.identity.hash_sha256_hex == binding_b.identity.hash_sha256_hex);

  temp_dir_t tmp{};
  const fs::path store_root = tmp.dir / ".hashimyei";
  const fs::path catalog_path =
      store_root / "_meta" / "catalog" / "hashimyei_catalog.idydb";
  constexpr const char* kPassphrase = "catalog-test-pass";

  const run_manifest_t manifest = make_run_manifest();
  test_component_manifest_id_stable_under_source_path_churn();
  test_founding_bundle_digest_stable_under_source_path_churn();

  fs::path run_manifest_path{};
  std::string error;
  REQUIRE(save_run_manifest(store_root, manifest, &run_manifest_path, &error));
  REQUIRE(run_manifest_path.filename() == cuwacunu::hashimyei::kRunManifestFilenameV2);

  run_manifest_t loaded_run{};
  REQUIRE(load_run_manifest(run_manifest_path, &loaded_run, &error));
  REQUIRE(loaded_run.run_id == manifest.run_id);
  REQUIRE(loaded_run.campaign_identity.name ==
          manifest.campaign_identity.name);
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
  REQUIRE(load_component_manifest(component_manifest_file, &loaded_component, &error));
  REQUIRE(loaded_component.hashimyei_identity.name ==
          component_manifest.hashimyei_identity.name);
  REQUIRE(loaded_component.contract_identity.hash_sha256_hex ==
          component_manifest.contract_identity.hash_sha256_hex);

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

  component_manifest_t pre_hard_cut_component_manifest = make_component_manifest();
  pre_hard_cut_component_manifest.hashimyei_identity =
      make_test_identity(hashimyei_kind_e::TSIEMENE, 0x99);
  pre_hard_cut_component_manifest.canonical_path =
      pre_hard_cut_component_manifest.family + "." +
      pre_hard_cut_component_manifest.hashimyei_identity.name;
  pre_hard_cut_component_manifest.founding_revision_id = "cfgrev.pre_hard_cut";
  const fs::path pre_hard_cut_component_manifest_path =
      cuwacunu::hero::hashimyei::component_manifest_path(
          store_root, pre_hard_cut_component_manifest.canonical_path,
          cuwacunu::hero::hashimyei::compute_component_manifest_id(
              pre_hard_cut_component_manifest));
  std::string pre_hard_cut_component_payload =
      component_payload(pre_hard_cut_component_manifest);
  pre_hard_cut_component_payload = erase_line_with_prefix(
      std::move(pre_hard_cut_component_payload), "founding_dsl_source_path=");
  pre_hard_cut_component_payload = erase_line_with_prefix(
      std::move(pre_hard_cut_component_payload),
      "founding_dsl_source_sha256_hex=");
  write_text_file(pre_hard_cut_component_manifest_path,
                  pre_hard_cut_component_payload);
  if (!catalog.ingest_filesystem(store_root, &error)) {
    std::cerr << "[test_hashimyei_catalog] pre-hard-cut manifest ingest error: "
              << error << "\n";
    return 1;
  }
  component_state_t skipped_legacy_component{};
  REQUIRE(!catalog.resolve_component("",
                                     pre_hard_cut_component_manifest.hashimyei_identity.name,
                                     &skipped_legacy_component, &error));
  REQUIRE(error.find("component not found") != std::string::npos);

  std::vector<run_manifest_t> runs{};
  REQUIRE(catalog.list_runs_by_binding(manifest.wave_contract_binding.contract.name,
                                       manifest.wave_contract_binding.wave.name,
                                       manifest.wave_contract_binding.identity.name,
                                       &runs, &error));
  REQUIRE(runs.size() == 1);
  REQUIRE(runs[0].run_id == manifest.run_id);

  runs.clear();
  REQUIRE(catalog.list_runs_by_binding(
      manifest.wave_contract_binding.contract.hash_sha256_hex, "", "", &runs,
      &error));
  REQUIRE(runs.empty());

  component_state_t resolved_component{};
  REQUIRE(catalog.resolve_component("", component_manifest.hashimyei_identity.name,
                                    &resolved_component, &error));
  REQUIRE(resolved_component.manifest.hashimyei_identity.name ==
          component_manifest.hashimyei_identity.name);

  component_manifest_t conflicting_contract_manifest = component_manifest;
  conflicting_contract_manifest.contract_identity =
      make_binding("bind_train_vicreg_other_contract", 9, 2, 3).contract;
  conflicting_contract_manifest.updated_at_ms = 1711111112000ULL;
  conflicting_contract_manifest.created_at_ms = 1711111112000ULL;
  REQUIRE(!catalog.register_component_manifest(conflicting_contract_manifest, nullptr,
                                               nullptr, &error));
  REQUIRE(!error.empty());

  component_manifest_t cutover_manifest = component_manifest;
  cutover_manifest.parent_identity = component_manifest.hashimyei_identity;
  cutover_manifest.hashimyei_identity =
      make_test_identity(hashimyei_kind_e::TSIEMENE, 0x21);
  cutover_manifest.canonical_path = cutover_manifest.family + "." +
                                    cutover_manifest.hashimyei_identity.name;
  cutover_manifest.revision_reason = "dsl_change";
  cutover_manifest.founding_revision_id = "cfgrev.cutover";
  cutover_manifest.founding_dsl_source_sha256_hex = sixty_four_hex('d');
  cutover_manifest.created_at_ms = 1711111113000ULL;
  cutover_manifest.updated_at_ms = 1711111113000ULL;

  std::string cutover_component_id{};
  bool cutover_inserted = false;
  REQUIRE(catalog.register_component_manifest(cutover_manifest, &cutover_component_id,
                                              &cutover_inserted, &error));
  REQUIRE(cutover_inserted);

  std::string active_hashimyei{};
  REQUIRE(catalog.resolve_active_hashimyei(
      cutover_manifest.family, "",
      cutover_manifest.contract_identity.hash_sha256_hex, &active_hashimyei,
      &error));
  REQUIRE(active_hashimyei == cutover_manifest.hashimyei_identity.name);

  bool cutover_inserted_again = true;
  REQUIRE(catalog.register_component_manifest(cutover_manifest, nullptr,
                                              &cutover_inserted_again, &error));
  REQUIRE(!cutover_inserted_again);

  component_manifest_t auto_component_a =
      make_auto_contract_component_manifest(0x31, sixty_four_hex('1'), 'e',
                                            1711111114000ULL);
  std::string auto_component_id_a{};
  bool auto_inserted_a = false;
  REQUIRE(catalog.register_component_manifest(auto_component_a, &auto_component_id_a,
                                              &auto_inserted_a, &error));
  REQUIRE(auto_inserted_a);

  component_state_t resolved_auto_a{};
  REQUIRE(catalog.resolve_component(auto_component_a.canonical_path, "",
                                    &resolved_auto_a, &error));
  REQUIRE(!resolved_auto_a.manifest.contract_identity.name.empty());

  component_manifest_t auto_component_b =
      make_auto_contract_component_manifest(0x32, sixty_four_hex('1'), 'f',
                                            1711111115000ULL);
  std::string auto_component_id_b{};
  bool auto_inserted_b = false;
  REQUIRE(catalog.register_component_manifest(auto_component_b, &auto_component_id_b,
                                              &auto_inserted_b, &error));
  REQUIRE(auto_inserted_b);

  component_state_t resolved_auto_b{};
  REQUIRE(catalog.resolve_component(auto_component_b.canonical_path, "",
                                    &resolved_auto_b, &error));
  REQUIRE(resolved_auto_b.manifest.contract_identity.hash_sha256_hex ==
          resolved_auto_a.manifest.contract_identity.hash_sha256_hex);

  const std::string base_contract_hash =
      component_manifest.contract_identity.hash_sha256_hex;
  const std::string auto_contract_hash =
      resolved_auto_a.manifest.contract_identity.hash_sha256_hex;

  cuwacunu::hero::family_rank::state_t bootstrap_rank{};
  REQUIRE(!catalog.get_family_rank("tsi.wikimyei.representation.vicreg",
                                   base_contract_hash, &bootstrap_rank, &error));
  REQUIRE(error.find("family rank not found") != std::string::npos);

  cuwacunu::hero::hashimyei_mcp::app_context_t rank_app{};
  rank_app.store_root = store_root;
  rank_app.lattice_catalog_path =
      store_root / "_meta" / "catalog" / "lattice_catalog.idydb";
  rank_app.catalog_options = options;
  const fs::path legacy_hashimyei_catalog_path =
      store_root / "catalog" / "hashimyei_catalog.idydb";
  write_text_file(legacy_hashimyei_catalog_path,
                  "legacy hashimyei catalog placeholder\n");
  REQUIRE(fs::exists(legacy_hashimyei_catalog_path));

  std::string tool_result{};
  std::string tool_error{};
  REQUIRE(catalog.close(&error));
  if (!cuwacunu::hero::hashimyei_mcp::execute_tool_json(
          "hero.hashimyei.get_component_manifest",
          "{\"hashimyei\":\"" + component_manifest.hashimyei_identity.name + "\"}",
          &rank_app, &tool_result, &tool_error)) {
    std::cerr << "[test_hashimyei_catalog] get_component_manifest error: "
              << tool_error << "\n";
    return 1;
  }
  REQUIRE(tool_error.empty());
  REQUIRE(tool_result.find("\"isError\":false") != std::string::npos);
  REQUIRE(fs::exists(rank_app.catalog_options.catalog_path));
  REQUIRE(fs::exists(rank_app.catalog_options.catalog_path.parent_path() /
                     "hashimyei_catalog.idydb.sync.sha256"));
  REQUIRE(!fs::exists(legacy_hashimyei_catalog_path));

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

  REQUIRE(save_component_manifest(store_root, component_manifest, nullptr, &error));
  if (!cuwacunu::hero::hashimyei_mcp::execute_tool_json(
          "hero.hashimyei.update_rank",
          "{\"family\":\"tsi.wikimyei.representation.vicreg\","
          "\"contract_hash\":\"" + auto_contract_hash + "\","
          "\"ordered_hashimyeis\":[\"0x0032\",\"0x0031\"],"
          "\"source_view_kind\":\"family_evaluation_report\","
          "\"source_view_transport_sha256\":\"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\"}",
          &rank_app, &tool_result, &tool_error)) {
    std::cerr << "[test_hashimyei_catalog] update_rank error: " << tool_error
              << "\n";
    return 1;
  }
  REQUIRE(tool_error.empty());
  if (tool_result.find("\"isError\":false") == std::string::npos) {
    std::cerr << "[test_hashimyei_catalog] unexpected tool_result: " << tool_result
              << "\n";
    return 1;
  }
  REQUIRE(tool_result.find("\"changed\":true") != std::string::npos);
  REQUIRE(tool_result.find("\"synchronized_lattice\":true") != std::string::npos);

  REQUIRE(catalog.open(options, &error));
  REQUIRE(catalog.ingest_filesystem(store_root, &error));

  cuwacunu::hero::family_rank::state_t family_rank{};
  REQUIRE(catalog.get_family_rank("tsi.wikimyei.representation.vicreg",
                                  auto_contract_hash, &family_rank, &error));
  REQUIRE(family_rank.assignments.size() == 2);
  REQUIRE(family_rank.assignments[0].hashimyei == "0x0032");
  REQUIRE(family_rank.assignments[1].hashimyei == "0x0031");
  REQUIRE(family_rank.contract_hash == auto_contract_hash);

  std::string ranked_zero_hashimyei{};
  REQUIRE(catalog.resolve_ranked_hashimyei("tsi.wikimyei.representation.vicreg",
                                           auto_contract_hash, 0,
                                           &ranked_zero_hashimyei, &error));
  REQUIRE(ranked_zero_hashimyei == "0x0032");

  std::vector<component_state_t> ranked_components{};
  REQUIRE(catalog.list_components("", "", 0, 0, true, &ranked_components, &error));
  std::unordered_map<std::string, std::optional<std::uint64_t>> observed_ranks{};
  for (const auto& component : ranked_components) {
    if (component.manifest.family != "tsi.wikimyei.representation.vicreg") continue;
    if (component.manifest.contract_identity.hash_sha256_hex != auto_contract_hash) {
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
          "{\"family\":\"tsi.wikimyei.representation.vicreg\","
          "\"contract_hash\":\"" + auto_contract_hash + "\","
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

  component_manifest_t auto_component_c =
      make_auto_contract_component_manifest(0x43, sixty_four_hex('1'), 'a',
                                            1711111116000ULL);
  std::string auto_component_id_c{};
  bool auto_inserted_c = false;
  if (!catalog.register_component_manifest(auto_component_c, &auto_component_id_c,
                                           &auto_inserted_c, &error)) {
    std::cerr << "[test_hashimyei_catalog] register_component_manifest error: "
              << error << "\n";
    return 1;
  }
  REQUIRE(auto_inserted_c);

  cuwacunu::hero::family_rank::state_t normalized_rank{};
  REQUIRE(catalog.get_family_rank("tsi.wikimyei.representation.vicreg",
                                  auto_contract_hash, &normalized_rank, &error));
  REQUIRE(normalized_rank.assignments.size() == 2);
  REQUIRE(normalized_rank.assignments[0].hashimyei == "0x0032");
  REQUIRE(normalized_rank.assignments[1].hashimyei == "0x0031");

  ranked_components.clear();
  REQUIRE(catalog.list_components("", "", 0, 0, true, &ranked_components, &error));
  observed_ranks.clear();
  for (const auto& component : ranked_components) {
    if (component.manifest.family != "tsi.wikimyei.representation.vicreg") continue;
    if (component.manifest.contract_identity.hash_sha256_hex != auto_contract_hash) {
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
          "{\"family\":\"tsi.wikimyei.representation.vicreg\","
          "\"contract_hash\":\"" + auto_contract_hash + "\","
          "\"ordered_hashimyeis\":[\"0x0032\",\"0x0031\"]}",
          &rank_app, &tool_result, &tool_error)) {
    std::cerr << "[test_hashimyei_catalog] update_rank error: " << tool_error
              << "\n";
    return 1;
  }
  REQUIRE(tool_error.empty());
  REQUIRE(tool_result.find("\"isError\":true") != std::string::npos);
  REQUIRE(tool_result.find("must enumerate every current family member exactly once") !=
          std::string::npos);

  tool_result.clear();
  tool_error.clear();
  REQUIRE(cuwacunu::hero::hashimyei_mcp::execute_tool_json(
      "hero.hashimyei.update_rank",
      "{\"family\":\"tsi.wikimyei.representation.vicreg\","
      "\"contract_hash\":\"" + auto_contract_hash + "\","
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
  REQUIRE(lattice_catalog.get_family_rank("tsi.wikimyei.representation.vicreg",
                                          auto_contract_hash,
                                          &lattice_family_rank, &error));
  REQUIRE(lattice_family_rank.assignments.size() == family_rank.assignments.size());
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
