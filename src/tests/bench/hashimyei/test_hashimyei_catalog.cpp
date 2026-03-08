#include <HERO/hashimyei/hashimyei_catalog.h>
#include <camahjucunu/db/idydb.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include <unistd.h>

namespace fs = std::filesystem;
using cuwacunu::hero::hashimyei::artifact_entry_t;
using cuwacunu::hero::hashimyei::component_manifest_t;
using cuwacunu::hero::hashimyei::component_state_t;
using cuwacunu::hero::hashimyei::compute_run_id;
using cuwacunu::hero::hashimyei::dependency_file_t;
using cuwacunu::hero::hashimyei::hashimyei_catalog_store_t;
using cuwacunu::hero::hashimyei::load_component_manifest;
using cuwacunu::hero::hashimyei::load_run_manifest;
using cuwacunu::hero::hashimyei::performance_snapshot_t;
using cuwacunu::hero::hashimyei::run_manifest_t;
using cuwacunu::hero::hashimyei::save_run_manifest;

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
  std::uniform_int_distribution<uint64_t> dist;
  const uint64_t v = dist(gen);
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

static void write_text_file(const fs::path& path, const std::string& payload) {
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  REQUIRE(static_cast<bool>(out));
  out << payload;
  REQUIRE(static_cast<bool>(out));
}

static std::string kv_payload(
    const std::vector<std::pair<std::string, std::string>>& fields) {
  std::string out;
  for (const auto& [k, v] : fields) {
    out += k;
    out += "=";
    out += v;
    out += "\n";
  }
  return out;
}

static void write_known_schema_fixtures(const fs::path& store_root,
                                        const std::string& run_id,
                                        const std::string& contract_hash,
                                        const std::string& wave_hash,
                                        const std::string& binding_id) {
  write_text_file(
      store_root / "tsi.source" / "data_analytics" / contract_hash /
          "tsi.source.dataloader.BTCUSDT" / "latest.kv",
      kv_payload({{"schema", "piaabo.torch_compat.data_analytics.v1"},
                  {"run_id", run_id},
                  {"canonical_base", "tsi.source.dataloader.BTCUSDT"},
                  {"contract_hash", contract_hash},
                  {"load_ms", "12.5"},
                  {"status", "ok"}}));

  write_text_file(
      store_root / "tsi.wikimyei" / "representation" / "vicreg" / "0x0001" /
          "weights.init.network_analytics.kv",
      kv_payload({{"schema", "piaabo.torch_compat.network_analytics.v2"},
                  {"run_id", run_id},
                  {"canonical_base", "tsi.wikimyei.representation.vicreg.0x0001"},
                  {"hashimyei", "0x0001"},
                  {"accuracy", "0.875"},
                  {"notes", "stable"}}));

  write_text_file(
      store_root / "tsi.wikimyei" / "representation" / "vicreg" / "0x0001" /
          "weights.init.pt.entropic_capacity.kv",
      kv_payload({{"schema", "piaabo.torch_compat.entropic_capacity_comparison.v1"},
                  {"run_id", run_id},
                  {"canonical_base", "tsi.wikimyei.representation.vicreg.0x0001"},
                  {"hashimyei", "0x0001"},
                  {"delta", "0.12"}}));

  // Known-schema .txt sidecars are ignored by v1 ingest to keep .kv as
  // canonical report format.
  write_text_file(
      store_root / "tsi.wikimyei" / "representation" / "vicreg" / "0x0001" /
          "weights.init.network_analytics.txt",
      kv_payload({{"schema", "piaabo.torch_compat.network_analytics.v2"},
                  {"run_id", run_id},
                  {"canonical_base", "tsi.wikimyei.representation.vicreg.0x0001"},
                  {"hashimyei", "0x0001"},
                  {"accuracy", "0.222"}}));

  write_text_file(
      store_root / "tsi.wikimyei" / "representation" / "vicreg" / "0x0001" /
          "component.manifest.v1.kv",
      kv_payload({{"schema", "hashimyei.component.manifest.v1"},
                  {"canonical_path", "tsi.wikimyei.representation.vicreg.0x0001"},
                  {"tsi_type", "tsi.wikimyei.representation.vicreg"},
                  {"hashimyei", "0x0001"},
                  {"contract_hash", contract_hash},
                  {"wave_hash", wave_hash},
                  {"binding_id", binding_id},
                  {"dsl_canonical_path", "src/config/instructions/tsi.wikimyei.representation.vicreg.network_design.dsl"},
                  {"dsl_sha256_hex", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},
                  {"status", "active"},
                  {"created_at_ms", "1711111111000"},
                  {"updated_at_ms", "1711111111001"}}));

  write_text_file(
      store_root / "tsi.source" / "data_analytics" / contract_hash /
          "tsi.source.dataloader.BTCUSDT" / "component.manifest.v1.kv",
      kv_payload({{"schema", "hashimyei.component.manifest.v1"},
                  {"canonical_path", "tsi.source.dataloader.BTCUSDT"},
                  {"tsi_type", "tsi.source.dataloader"},
                  {"hashimyei", ""},
                  {"contract_hash", contract_hash},
                  {"wave_hash", wave_hash},
                  {"binding_id", binding_id},
                  {"dsl_canonical_path", "src/config/instructions/iitepi.wave.example.dsl"},
                  {"dsl_sha256_hex", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"},
                  {"status", "active"},
                  {"created_at_ms", "1711111111000"},
                  {"updated_at_ms", "1711111111001"}}));

  write_text_file(
      store_root / "tsi.probe" / "representation" /
          "transfer_matrix_evaluation" / "0x00aa" /
          "metrics.epoch.latest.kv",
      kv_payload({{"schema", "tsi.probe.representation.transfer_matrix_evaluation.epoch.v1"},
                  {"canonical_base", "tsi.probe.representation.transfer_matrix_evaluation.0x00aa"},
                  {"loss", "1.25"}}));

  write_text_file(
      store_root / "tsi.probe" / "representation" /
          "transfer_matrix_evaluation" / "0x00aa" /
          "metrics.history.latest.kv",
      kv_payload({{"schema", "tsi.probe.representation.transfer_matrix_evaluation.history.v2"},
                  {"run_id", run_id},
                  {"canonical_base", "tsi.probe.representation.transfer_matrix_evaluation.0x00aa"},
                  {"window", "64"}}));

  write_text_file(
      store_root / "tsi.probe" / "representation" /
          "transfer_matrix_evaluation" / "0x00aa" /
          "metrics.activation.latest.kv",
      kv_payload({{"schema", "tsi.probe.representation.transfer_matrix_evaluation.activation.v1"},
                  {"run_id", run_id},
                  {"canonical_base", "tsi.probe.representation.transfer_matrix_evaluation.0x00aa"},
                  {"act_mean", "0.44"}}));

  write_text_file(
      store_root / "tsi.probe" / "representation" /
          "transfer_matrix_evaluation" / "0x00aa" /
          "metrics.prequential.latest.kv",
      kv_payload({{"schema", "tsi.probe.representation.transfer_matrix_evaluation.prequential_raw.v1"},
                  {"run_id", run_id},
                  {"canonical_base", "tsi.probe.representation.transfer_matrix_evaluation.0x00aa"},
                  {"ahead", "4"}}));

  // Unknown schema must be ignored.
  write_text_file(store_root / "misc" / "ignored.kv",
                  kv_payload({{"schema", "unknown.schema.v1"},
                              {"value", "1"}}));
}

static std::size_t count_record_kind(const fs::path& db_path,
                                     const char* passphrase,
                                     const char* record_kind) {
  idydb* db = nullptr;
  REQUIRE(idydb_open_encrypted(db_path.c_str(), &db, 0, passphrase) ==
          IDYDB_SUCCESS);

  idydb_filter_term f{};
  f.column = 1;
  f.type = IDYDB_CHAR;
  f.op = IDYDB_FILTER_OP_EQ;
  f.value.s = record_kind;

  std::size_t count = 0;
  std::string error;
  REQUIRE(db::query::count_rows(&db, {f}, &count, &error));
  REQUIRE(idydb_close(&db) == IDYDB_DONE);
  return count;
}

static run_manifest_t make_run_manifest() {
  run_manifest_t m{};
  m.started_at_ms = 1711111111000ULL;
  m.board_hash = "board_abc";
  m.contract_hash = "contract_xyz";
  m.wave_hash = "wave_123";
  m.binding_id = "bind_train_vicreg";
  m.sampler = "uniform";
  m.record_type = "train";
  m.seed = "42";
  m.device = "cpu";
  m.dtype = "float32";
  m.run_id =
      compute_run_id(m.board_hash, m.contract_hash, m.wave_hash, m.binding_id,
                     m.started_at_ms);
  m.dependency_files.push_back(
      dependency_file_t{"iitepi.board.bind_train_vicreg", "sha.board"});
  m.dependency_files.push_back(
      dependency_file_t{"iitepi.contract.bind_train_vicreg", "sha.contract"});
  m.components.push_back(
      {"tsi.source.dataloader.BTCUSDT", "tsi.source.dataloader", ""});
  m.components.push_back(
      {"tsi.wikimyei.representation.vicreg.0x0001", "tsi.wikimyei.representation.vicreg", "0x0001"});
  return m;
}

int main() {
  temp_dir_t tmp{};
  const fs::path store_root = tmp.dir / ".hashimyei";
  const fs::path catalog_path = store_root / "catalog" / "hashimyei_catalog.idydb";
  constexpr const char* kPassphrase = "catalog-test-pass";

  const run_manifest_t manifest = make_run_manifest();

  fs::path manifest_path{};
  std::string error;
  REQUIRE(save_run_manifest(store_root, manifest, &manifest_path, &error));
  REQUIRE(manifest_path.filename() == "run.manifest.v1.kv");

  run_manifest_t loaded{};
  if (!load_run_manifest(manifest_path, &loaded, &error)) {
    std::cerr << "load_run_manifest error: " << error << "\\n";
    REQUIRE(false);
  }
  REQUIRE(loaded.run_id == manifest.run_id);
  REQUIRE(loaded.contract_hash == manifest.contract_hash);
  REQUIRE(loaded.wave_hash == manifest.wave_hash);
  REQUIRE(loaded.binding_id == manifest.binding_id);
  REQUIRE(loaded.dependency_files.size() == 2);

  write_known_schema_fixtures(store_root, manifest.run_id, manifest.contract_hash,
                              manifest.wave_hash, manifest.binding_id);

  component_manifest_t source_component{};
  const fs::path source_component_path =
      store_root / "tsi.source" / "data_analytics" / manifest.contract_hash /
      "tsi.source.dataloader.BTCUSDT" / "component.manifest.v1.kv";
  REQUIRE(load_component_manifest(source_component_path, &source_component, &error));
  REQUIRE(source_component.canonical_path == "tsi.source.dataloader.BTCUSDT");
  REQUIRE(source_component.hashimyei.empty());
  REQUIRE(source_component.binding_id == manifest.binding_id);

  const fs::path invalid_component_path =
      tmp.dir / "invalid.component.manifest.v1.kv";
  write_text_file(
      invalid_component_path,
      kv_payload({{"schema", "hashimyei.component.manifest.v1"},
                  {"canonical_path", "tsi.source.dataloader.BTCUSDT"},
                  {"tsi_type", "tsi.source.dataloader"},
                  {"contract_hash", manifest.contract_hash},
                  {"wave_hash", manifest.wave_hash},
                  {"binding_id", manifest.binding_id},
                  {"dsl_canonical_path", "src/config/instructions/iitepi.wave.example.dsl"},
                  {"dsl_sha256_hex", "not-a-hex"},
                  {"status", "active"},
                  {"created_at_ms", "1711111111000"}}));
  component_manifest_t invalid_component{};
  REQUIRE(!load_component_manifest(invalid_component_path, &invalid_component, &error));
  REQUIRE(error.find("dsl_sha256_hex") != std::string::npos);

  hashimyei_catalog_store_t catalog{};
  hashimyei_catalog_store_t::options_t options{};
  options.catalog_path = catalog_path;
  options.encrypted = true;
  options.passphrase = kPassphrase;
  options.ingest_version = 1;

  if (!catalog.open(options, &error)) {
    std::cerr << "catalog.open error: " << error << "\\n";
    REQUIRE(false);
  }
  REQUIRE(catalog.ingest_filesystem(store_root, true, &error));

  std::vector<artifact_entry_t> artifacts{};
  REQUIRE(catalog.list_artifacts("", "", 0, 0, true, &artifacts, &error));
  const std::size_t initial_artifact_count = artifacts.size();
  REQUIRE(initial_artifact_count == 7);

  run_manifest_t got_run{};
  REQUIRE(catalog.get_run(manifest.run_id, &got_run, &error));
  REQUIRE(got_run.binding_id == manifest.binding_id);

  artifact_entry_t source_latest{};
  REQUIRE(catalog.latest_artifact("tsi.source.dataloader.BTCUSDT",
                                  "piaabo.torch_compat.data_analytics.v1",
                                  &source_latest, &error));
  REQUIRE(source_latest.hashimyei.empty());

  component_state_t source_component_state{};
  REQUIRE(catalog.resolve_component("tsi.source.dataloader.BTCUSDT", "",
                                    &source_component_state, &error));
  REQUIRE(source_component_state.manifest.hashimyei.empty());
  REQUIRE(source_component_state.manifest.dsl_canonical_path ==
          "src/config/instructions/iitepi.wave.example.dsl");

  artifact_entry_t vicreg_latest{};
  REQUIRE(catalog.latest_artifact("tsi.wikimyei.representation.vicreg.0x0001",
                                  "piaabo.torch_compat.network_analytics.v2",
                                  &vicreg_latest, &error));
  REQUIRE(vicreg_latest.hashimyei == "0x0001");

  component_state_t vicreg_component_state{};
  REQUIRE(catalog.resolve_component("", "0x0001", &vicreg_component_state,
                                    &error));
  REQUIRE(vicreg_component_state.manifest.canonical_path ==
          "tsi.wikimyei.representation.vicreg.0x0001");
  REQUIRE(vicreg_component_state.manifest.dsl_canonical_path ==
          "src/config/instructions/tsi.wikimyei.representation.vicreg.network_design.dsl");

  std::vector<component_state_t> all_components{};
  REQUIRE(catalog.list_components("", "", 0, 0, true, &all_components, &error));
  REQUIRE(all_components.size() == 2);

  performance_snapshot_t snapshot{};
  REQUIRE(catalog.performance_snapshot("tsi.source.dataloader.BTCUSDT",
                                       manifest.run_id, &snapshot, &error));
  REQUIRE(snapshot.artifact.canonical_path == "tsi.source.dataloader.BTCUSDT");
  REQUIRE(!snapshot.numeric_metrics.empty());
  REQUIRE(!snapshot.text_metrics.empty());

  std::vector<dependency_file_t> provenance{};
  REQUIRE(catalog.provenance_trace(source_latest.artifact_id, &provenance, &error));
  REQUIRE(provenance.size() == 2);

  // Idempotency: ingesting same tree again should not duplicate artifacts.
  REQUIRE(catalog.ingest_filesystem(store_root, true, &error));
  artifacts.clear();
  REQUIRE(catalog.list_artifacts("", "", 0, 0, true, &artifacts, &error));
  REQUIRE(artifacts.size() == initial_artifact_count);

  // Stable history ordering (newest-first deterministic by ts/artifact_id).
  std::vector<artifact_entry_t> history{};
  REQUIRE(catalog.list_artifacts("tsi.probe.representation.transfer_matrix_evaluation.0x00aa",
                                 "", 10, 0, true, &history, &error));
  REQUIRE(history.size() == 4);
  for (const auto& row : history) {
    REQUIRE(row.canonical_path == "tsi.probe.representation.transfer_matrix_evaluation.0x00aa");
  }

  REQUIRE(catalog.close(&error));
  REQUIRE(count_record_kind(catalog_path, kPassphrase, "component") == 2);
  REQUIRE(count_record_kind(catalog_path, kPassphrase, "component_provenance") ==
          2);

  // Encrypted reopen with wrong passphrase must fail.
  hashimyei_catalog_store_t wrong{};
  options.passphrase = "wrong-pass";
  REQUIRE(!wrong.open(options, &error));

  // Correct passphrase reopen + DB-first query path.
  hashimyei_catalog_store_t reopened{};
  options.passphrase = kPassphrase;
  REQUIRE(reopened.open(options, &error));
  artifacts.clear();
  REQUIRE(reopened.list_artifacts("", "", 0, 0, true, &artifacts, &error));
  REQUIRE(artifacts.size() == initial_artifact_count);
  REQUIRE(reopened.close(&error));

  std::cout << "[PASS] test_hashimyei_catalog\n";
  return 0;
}
