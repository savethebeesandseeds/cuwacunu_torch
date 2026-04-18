#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "hero/hashimyei_hero/hashimyei_catalog.h"
#include "hero/runtime_hero/runtime_job.h"
#include "hero/wave_contract_binding_runtime.h"
#include "iitepi/runtime_binding/runtime_binding.h"
#include "iitepi/runtime_binding/runtime_binding.validation.h"

namespace {

void write_file(const std::filesystem::path &path, const std::string &content) {
  std::string error{};
  const bool ok =
      cuwacunu::hero::runtime::write_text_file_atomic(path, content, &error);
  assert(ok);
  assert(error.empty());
}

std::string read_file(const std::filesystem::path &path) {
  std::string content{};
  std::string error{};
  const bool ok =
      cuwacunu::hero::runtime::read_text_file(path, &content, &error);
  assert(ok);
  assert(error.empty());
  return content;
}

std::string sixty_four_hex(char c) { return std::string(64, c); }

struct env_guard_t {
  std::string key{};
  std::optional<std::string> previous{};

  explicit env_guard_t(std::string key_in) : key(std::move(key_in)) {
    const char *current = std::getenv(key.c_str());
    if (current != nullptr)
      previous = std::string(current);
  }

  ~env_guard_t() {
    if (previous.has_value()) {
      (void)setenv(key.c_str(), previous->c_str(), 1);
    } else {
      (void)unsetenv(key.c_str());
    }
  }

  void set(std::string_view value) {
    assert(setenv(key.c_str(), std::string(value).c_str(), 1) == 0);
  }
};

[[nodiscard]] cuwacunu::hashimyei::hashimyei_t make_test_identity(
    cuwacunu::hashimyei::hashimyei_kind_e kind, std::uint64_t ordinal,
    std::string hash = {}) {
  auto id = cuwacunu::hashimyei::make_identity(kind, ordinal, std::move(hash));
  id.schema = cuwacunu::hashimyei::kIdentitySchemaV2;
  return id;
}

[[nodiscard]] cuwacunu::hero::hashimyei::component_manifest_t
make_component_manifest(std::string family, std::uint64_t ordinal,
                        std::string contract_hash,
                        std::string docking_signature) {
  cuwacunu::hero::hashimyei::component_manifest_t manifest{};
  manifest.schema = cuwacunu::hashimyei::kComponentManifestSchemaV2;
  manifest.family = std::move(family);
  manifest.hashimyei_identity = make_test_identity(
      cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE, ordinal);
  manifest.canonical_path =
      manifest.family + "." + manifest.hashimyei_identity.name;
  manifest.contract_identity = make_test_identity(
      cuwacunu::hashimyei::hashimyei_kind_e::CONTRACT, 1,
      std::move(contract_hash));
  manifest.revision_reason = "initial";
  manifest.founding_revision_id = "cfgrev." + manifest.hashimyei_identity.name;
  manifest.founding_dsl_source_path = "demo.founding.dsl";
  manifest.founding_dsl_source_sha256_hex = sixty_four_hex('a');
  manifest.docking_signature_sha256_hex = std::move(docking_signature);
  manifest.lineage_state = "active";
  manifest.created_at_ms = 1711111111000ULL;
  manifest.updated_at_ms = 1711111111000ULL;
  return manifest;
}

void replace_all(std::string *text, std::string_view from,
                 std::string_view to) {
  assert(text != nullptr);
  assert(!from.empty());
  std::size_t pos = 0;
  while ((pos = text->find(from, pos)) != std::string::npos) {
    text->replace(pos, from.size(), to);
    pos += to.size();
  }
}

} // namespace

int main() {
  namespace fs = std::filesystem;
  const fs::path root =
      fs::temp_directory_path() / "wave_contract_binding_runtime_snapshot";
  std::error_code ec{};
  fs::remove_all(root, ec);
  ec.clear();
  fs::create_directories(root / "source", ec);
  assert(!ec);

  const fs::path source_dir = root / "source";
  const fs::path contract_path = source_dir / "demo.contract.dsl";
  const fs::path circuit_path = source_dir / "demo.circuit.dsl";
  const fs::path wave_path = source_dir / "demo.wave.dsl";
  const fs::path sources_path = source_dir / "demo.sources.dsl";
  const fs::path channels_path = source_dir / "demo.channels.dsl";
  const fs::path vicreg_path =
      source_dir / "tsi.wikimyei.representation.vicreg.dsl";
  const fs::path embedding_eval_path =
      source_dir / "tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl";
  const fs::path transfer_eval_path =
      source_dir / "tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl";
  const fs::path network_design_path = source_dir / "demo.network.dsl";
  const fs::path jkimyei_path = source_dir / "demo.jkimyei.dsl";
  const fs::path campaign_path = source_dir / "demo.campaign.dsl";

  write_file(circuit_path, "TSIEMENE_CIRCUIT { }\n");
  write_file(sources_path, "SOURCES { }\n");
  write_file(channels_path, "CHANNELS { }\n");
  write_file(network_design_path, "NETWORK_DESIGN { }\n");
  write_file(jkimyei_path, "JKSPEC 2.0\n");
  write_file(embedding_eval_path, "max_samples:int = 1024\n");
  write_file(transfer_eval_path, "summary_every_steps:int = 64\n");
  write_file(vicreg_path, "learning_rate:float = % __lr ? 0.001 %\n"
                          "network_design_dsl_file:str = demo.network.dsl\n"
                          "jkimyei_dsl_file:str = demo.jkimyei.dsl\n");

  write_file(
      contract_path,
      "-----BEGIN IITEPI CONTRACT-----\n"
      "DOCK {\n"
      "  __obs_channels = 1;\n"
      "  __obs_seq_length = 1;\n"
      "  __obs_feature_dim = 1;\n"
      "  __embedding_dims = 8;\n"
      "  __future_target_dims = 1;\n"
      "}\n"
      "ASSEMBLY {\n"
      "  __lr = 0.123;\n"
      "  __vicreg_config_dsl_file = tsi.wikimyei.representation.vicreg.dsl;\n"
      "  __embedding_sequence_analytics_config_dsl_file = "
      "tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl;\n"
      "  __transfer_matrix_evaluation_config_dsl_file = "
      "tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl;\n"
      "  __observation_sources_dsl_file = demo.sources.dsl;\n"
      "  __observation_channels_dsl_file = demo.channels.dsl;\n"
      "  CIRCUIT_FILE: demo.circuit.dsl;\n"
      "  AKNOWLEDGE: src = tsi.source.dataloader;\n"
      "  AKNOWLEDGE: rep = tsi.wikimyei.representation.vicreg;\n"
      "  AKNOWLEDGE: sink_null = tsi.sink.null;\n"
      "}\n"
      "-----END IITEPI CONTRACT-----\n");

  write_file(wave_path, "WAVE wave_alpha {\n"
                        "  MODE: run | train;\n"
                        "  SAMPLER: % __sampler ? sequential %;\n"
                        "  EPOCHS: 1;\n"
                        "  BATCH_SIZE: 4;\n"
                        "  MAX_BATCHES_PER_EPOCH: 8;\n"
                        "  SOURCE: <src> {\n"
                        "    FAMILY: tsi.source.dataloader;\n"
                        "    SETTINGS: {\n"
                        "      WORKERS: % __workers ? 0 %;\n"
                        "      FORCE_REBUILD_CACHE: false;\n"
                        "      RANGE_WARN_BATCHES: 1;\n"
                        "    };\n"
                        "    RUNTIME: {\n"
                        "      SYMBOL: % __symbol ? BTCUSDT %;\n"
                        "      FROM: % __from ? 01.01.2020 %;\n"
                        "      TO: % __to ? 31.01.2020 %;\n"
                        "    };\n"
                        "  };\n"
                        "  WIKIMYEI: <rep> {\n"
                        "    FAMILY: tsi.wikimyei.representation.vicreg;\n"
                        "    JKIMYEI: {\n"
                        "      HALT_TRAIN: false;\n"
                        "      PROFILE_ID: stable_pretrain;\n"
                        "    };\n"
                        "  };\n"
                        "  SINK: <sink_null> {\n"
                        "    FAMILY: tsi.sink.null;\n"
                        "  };\n"
                        "}\n");

  write_file(campaign_path,
             "CAMPAIGN {\n"
             "  IMPORT_CONTRACT \"demo.contract.dsl\" AS contract_demo;\n"
             "  FROM \"demo.wave.dsl\" IMPORT_WAVE wave_alpha;\n"
             "  BIND bind_alpha {\n"
             "    __sampler = random;\n"
             "    __workers = 2;\n"
             "    __symbol = ADAUSDT;\n"
             "    __from = 03.02.2020;\n"
             "    __to = 29.02.2020;\n"
             "    MOUNT {\n"
             "      rep = EXACT 0x0000;\n"
             "    };\n"
             "    CONTRACT = contract_demo;\n"
             "    WAVE = wave_alpha;\n"
             "  };\n"
             "  RUN bind_alpha;\n"
             "}\n");

  cuwacunu::hero::wave_contract_binding_runtime::
      resolved_wave_contract_binding_snapshot_t campaign_snapshot{};
  std::string error{};
  bool ok = cuwacunu::hero::wave_contract_binding_runtime::
      prepare_campaign_binding_snapshot(campaign_path, "bind_alpha",
                                        root / "campaign_job",
                                        &campaign_snapshot, &error);
  assert(ok);
  assert(error.empty());
  assert(fs::exists(campaign_snapshot.campaign_dsl_path));
  assert(fs::exists(campaign_snapshot.contract_dsl_path));
  assert(fs::exists(campaign_snapshot.wave_dsl_path));
  assert(campaign_snapshot.binding_id == "bind_alpha");
  assert(campaign_snapshot.original_contract_ref == "contract_demo");
  assert(campaign_snapshot.wave_ref == "wave_alpha");
  assert(campaign_snapshot.variables.size() == 5);

  const std::string campaign_snapshot_text =
      read_file(campaign_snapshot.campaign_dsl_path);
  const std::string contract_snapshot_text =
      read_file(campaign_snapshot.contract_dsl_path);
  const std::string wave_snapshot_text =
      read_file(campaign_snapshot.wave_dsl_path);
  const fs::path instructions_dir =
      fs::path(campaign_snapshot.campaign_dsl_path).parent_path();
  const fs::path vicreg_snapshot_path =
      instructions_dir / "tsi.wikimyei.representation.vicreg.dsl";
  const fs::path embedding_eval_snapshot_path =
      instructions_dir /
      "tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl";
  const fs::path transfer_eval_snapshot_path =
      instructions_dir /
      "tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl";
  const std::string vicreg_snapshot_text = read_file(vicreg_snapshot_path);
  const std::string embedding_eval_snapshot_text =
      read_file(embedding_eval_snapshot_path);
  const std::string transfer_eval_snapshot_text =
      read_file(transfer_eval_snapshot_path);
  assert(campaign_snapshot_text.find(
             "IMPORT_CONTRACT \"binding.contract.dsl\" AS contract_binding;") !=
         std::string::npos);
  assert(campaign_snapshot_text.find(
             "FROM \"binding.wave.dsl\" IMPORT_WAVE wave_alpha;") !=
         std::string::npos);
  assert(campaign_snapshot_text.find("MOUNT {") != std::string::npos);
  assert(campaign_snapshot_text.find("rep = EXACT 0x0000;") !=
         std::string::npos);
  assert(campaign_snapshot_text.find("CONTRACT = contract_binding;") !=
         std::string::npos);
  assert(campaign_snapshot_text.find("RUN bind_alpha;") != std::string::npos);
  assert(contract_snapshot_text.find("DOCK {") != std::string::npos);
  assert(contract_snapshot_text.find("ASSEMBLY {") != std::string::npos);
  assert(contract_snapshot_text.find("__lr = 0.123;") != std::string::npos);
  assert(contract_snapshot_text.find("CIRCUIT_FILE: demo.circuit.dsl;") !=
         std::string::npos);
  assert(wave_snapshot_text.find("WORKERS: 2;") != std::string::npos);
  assert(wave_snapshot_text.find("SAMPLER: random;") != std::string::npos);
  assert(wave_snapshot_text.find("SYMBOL: ADAUSDT;") != std::string::npos);
  assert(wave_snapshot_text.find("FROM: 03.02.2020;") != std::string::npos);
  assert(wave_snapshot_text.find("TO: 29.02.2020;") != std::string::npos);
  assert(wave_snapshot_text.find(
             "PATH: tsi.wikimyei.representation.vicreg.0x0000;") !=
         std::string::npos);
  {
    const std::string wave_grammar = read_file(
        "/cuwacunu/src/config/bnf/iitepi.wave.bnf");
    const auto decoded_wave_set =
        cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(
            wave_grammar, wave_snapshot_text);
    assert(decoded_wave_set.waves.size() == 1);
    assert(decoded_wave_set.waves.front().wikimyeis.size() == 1);
    assert(decoded_wave_set.waves.front().wikimyeis.front().wikimyei_path ==
           "tsi.wikimyei.representation.vicreg.0x0000");
    const auto staged_contract_hash =
        cuwacunu::iitepi::contract_space_t::register_contract_file(
            campaign_snapshot.contract_dsl_path);
    const auto wave_report = tsiemene::validate_wave_definition(
        decoded_wave_set.waves.front(), staged_contract_hash);
    assert(wave_report.ok);
  }
  assert(fs::exists(instructions_dir / "demo.circuit.dsl"));
  assert(fs::exists(instructions_dir / "demo.sources.dsl"));
  assert(fs::exists(instructions_dir / "demo.channels.dsl"));
  assert(fs::exists(vicreg_snapshot_path));
  assert(fs::exists(embedding_eval_snapshot_path));
  assert(fs::exists(transfer_eval_snapshot_path));
  assert(fs::exists(instructions_dir / "demo.network.dsl"));
  assert(fs::exists(instructions_dir / "demo.jkimyei.dsl"));
  assert(vicreg_snapshot_text.find("learning_rate:float = 0.123") !=
         std::string::npos);
  assert(vicreg_snapshot_text.find(
             "network_design_dsl_file:str = demo.network.dsl") !=
         std::string::npos);
  assert(vicreg_snapshot_text.find("jkimyei_dsl_file:str = demo.jkimyei.dsl") !=
         std::string::npos);
  assert(embedding_eval_snapshot_text.find("max_samples:int = 1024") !=
         std::string::npos);
  assert(transfer_eval_snapshot_text.find("summary_every_steps:int = 64") !=
         std::string::npos);

  ok = cuwacunu::hero::wave_contract_binding_runtime::
      prepare_campaign_binding_snapshot(campaign_path, "bind_alpha",
                                        root / "campaign_job_second",
                                        &campaign_snapshot, &error);
  assert(ok);
  assert(error.empty());
  assert(fs::exists(campaign_snapshot.campaign_dsl_path));
  assert(fs::exists(campaign_snapshot.contract_dsl_path));
  assert(fs::exists(campaign_snapshot.wave_dsl_path));
  assert(read_file(campaign_snapshot.wave_dsl_path).find("SYMBOL: ADAUSDT;") !=
         std::string::npos);

  const fs::path invalid_contract_path =
      source_dir / "demo.invalid.contract.dsl";
  const fs::path invalid_campaign_path =
      source_dir / "demo.invalid.campaign.dsl";
  write_file(invalid_contract_path, "-----BEGIN IITEPI CONTRACT-----\n"
                                    "__lr = 0.123;\n"
                                    "CIRCUIT_FILE: demo.circuit.dsl;\n"
                                    "AKNOWLEDGE: src = tsi.source.dataloader;\n"
                                    "-----END IITEPI CONTRACT-----\n");
  write_file(
      invalid_campaign_path,
      "CAMPAIGN {\n"
      "  IMPORT_CONTRACT \"demo.invalid.contract.dsl\" AS contract_invalid;\n"
      "  FROM \"demo.wave.dsl\" IMPORT_WAVE wave_alpha;\n"
      "  BIND bind_alpha {\n"
      "    __sampler = random;\n"
      "    __workers = 2;\n"
      "    __symbol = ADAUSDT;\n"
      "    __from = 03.02.2020;\n"
      "    __to = 29.02.2020;\n"
      "    MOUNT {\n"
      "      rep = EXACT 0x0000;\n"
      "    };\n"
      "    CONTRACT = contract_invalid;\n"
      "    WAVE = wave_alpha;\n"
      "  };\n"
      "  RUN bind_alpha;\n"
      "}\n");
  error.clear();
  ok = cuwacunu::hero::wave_contract_binding_runtime::
      prepare_campaign_binding_snapshot(invalid_campaign_path, "bind_alpha",
                                        root / "invalid_campaign_job",
                                        &campaign_snapshot, &error);
  assert(!ok);
  assert(!error.empty());
  assert(error.find("DOCK") != std::string::npos);

  const fs::path default_campaign_override_path =
      root / "default_campaign_override.dsl";
  std::string default_campaign_override = read_file(
      "/cuwacunu/src/config/instructions/defaults/default.iitepi.campaign.dsl");
  replace_all(&default_campaign_override,
              "IMPORT_CONTRACT \"default.iitepi.contract.dsl\" AS "
              "contract_default_iitepi;",
              "IMPORT_CONTRACT "
              "\"/cuwacunu/src/config/instructions/defaults/"
              "default.iitepi.contract.dsl\" "
              "AS contract_default_iitepi;");
  replace_all(
      &default_campaign_override,
      "FROM \"default.iitepi.wave.dsl\" IMPORT_WAVE train_vicreg_primary;",
      "FROM "
      "\"/cuwacunu/src/config/instructions/defaults/default.iitepi.wave.dsl\" "
      "IMPORT_WAVE train_vicreg_primary;");
  replace_all(&default_campaign_override, "__sampler = sequential;",
              "__sampler = random;");
  replace_all(&default_campaign_override, "__workers = 0;", "__workers = 2;");
  replace_all(&default_campaign_override, "__symbol = BTCUSDT;",
              "__symbol = ADAUSDT;");
  write_file(default_campaign_override_path, default_campaign_override);

  ok = cuwacunu::hero::wave_contract_binding_runtime::
      prepare_campaign_binding_snapshot(
          default_campaign_override_path, "bind_train_vicreg_primary",
          root / "default_campaign_job", &campaign_snapshot, &error);
  assert(ok);
  assert(error.empty());
  const std::string default_wave_snapshot_text =
      read_file(campaign_snapshot.wave_dsl_path);
  assert(default_wave_snapshot_text.find("SAMPLER: random;") !=
         std::string::npos);
  assert(default_wave_snapshot_text.find("WORKERS: 2;") != std::string::npos);
  assert(default_wave_snapshot_text.find("SYMBOL: ADAUSDT;") !=
         std::string::npos);

  const auto contract_hash =
      cuwacunu::iitepi::contract_space_t::register_contract_file(
          contract_path.string());
  const auto contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
  assert(contract_snapshot != nullptr);

  {
    env_guard_t store_root_env("CUWACUNU_HASHIMYEI_STORE_ROOT");
    store_root_env.set((root / "hashimyei_store_incompatible").string());

    auto incompatible_manifest = make_component_manifest(
        "tsi.wikimyei.representation.vicreg", 0x0, contract_hash,
        sixty_four_hex('f'));
    if (incompatible_manifest.docking_signature_sha256_hex ==
        contract_snapshot->signature.docking_signature_sha256_hex) {
      incompatible_manifest.docking_signature_sha256_hex = sixty_four_hex('e');
    }
    std::string save_error{};
    ok = cuwacunu::hero::hashimyei::save_component_manifest(
        cuwacunu::hashimyei::store_root(), incompatible_manifest, nullptr,
        &save_error);
    assert(ok);
    assert(save_error.empty());

    error.clear();
    ok = cuwacunu::hero::wave_contract_binding_runtime::
        prepare_campaign_binding_snapshot(campaign_path, "bind_alpha",
                                          root / "campaign_job_incompatible",
                                          &campaign_snapshot, &error);
    assert(!ok);
    assert(error.find("not dock-compatible") != std::string::npos);
  }

  {
    env_guard_t store_root_env("CUWACUNU_HASHIMYEI_STORE_ROOT");
    store_root_env.set((root / "hashimyei_store_wrong_family").string());

    auto wrong_family_manifest =
        make_component_manifest("tsi.probe.log", 0x0, contract_hash,
                                contract_snapshot->signature
                                    .docking_signature_sha256_hex);
    std::string save_error{};
    ok = cuwacunu::hero::hashimyei::save_component_manifest(
        cuwacunu::hashimyei::store_root(), wrong_family_manifest, nullptr,
        &save_error);
    assert(ok);
    assert(save_error.empty());

    std::string docking_error{};
    ok = tsiemene::validate_runtime_hashimyei_contract_docking(
        "tsi.wikimyei.representation.vicreg", "0x0000", contract_hash,
        /*require_registered_manifest=*/true, &docking_error);
    assert(!ok);
    assert(docking_error.find("configured hashimyei manifest lookup failed") !=
           std::string::npos);
  }

  fs::remove_all(root, ec);
  return 0;
}
