#include <cassert>
#include <filesystem>
#include <string>

#include "hero/wave_contract_binding_runtime.h"
#include "hero/runtime_hero/runtime_job.h"

namespace {

void write_file(const std::filesystem::path& path, const std::string& content) {
  std::string error{};
  const bool ok =
      cuwacunu::hero::runtime::write_text_file_atomic(path, content, &error);
  assert(ok);
  assert(error.empty());
}

std::string read_file(const std::filesystem::path& path) {
  std::string content{};
  std::string error{};
  const bool ok = cuwacunu::hero::runtime::read_text_file(path, &content, &error);
  assert(ok);
  assert(error.empty());
  return content;
}

void replace_all(std::string* text, std::string_view from, std::string_view to) {
  assert(text != nullptr);
  assert(!from.empty());
  std::size_t pos = 0;
  while ((pos = text->find(from, pos)) != std::string::npos) {
    text->replace(pos, from.size(), to);
    pos += to.size();
  }
}

}  // namespace

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
  const fs::path circuit_path = source_dir / "demo.contract.circuit.dsl";
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
  write_file(vicreg_path,
             "learning_rate:float = % __lr ? 0.001 %\n"
             "network_design_dsl_file:str = demo.network.dsl\n"
             "jkimyei_dsl_file:str = demo.jkimyei.dsl\n");

  write_file(contract_path,
             "-----BEGIN IITEPI CONTRACT-----\n"
             "__lr = 0.123;\n"
             "CIRCUIT_FILE: demo.contract.circuit.dsl;\n"
             "AKNOWLEDGE: src = tsi.source.dataloader;\n"
             "AKNOWLEDGE: rep = tsi.wikimyei.representation.vicreg;\n"
             "AKNOWLEDGE: sink_null = tsi.sink.null;\n"
             "-----END IITEPI CONTRACT-----\n");

  write_file(
      wave_path,
      "WAVE wave_alpha {\n"
      "  MODE: run | train;\n"
      "  EPOCHS: 1;\n"
      "  BATCH_SIZE: 4;\n"
      "  MAX_BATCHES_PER_EPOCH: 8;\n"
      "  SOURCE: <src> {\n"
      "    FAMILY: tsi.source.dataloader;\n"
      "    SETTINGS: {\n"
      "      WORKERS: % __workers ? 0 %;\n"
      "      SAMPLER: % __sampler ? sequential %;\n"
      "      FORCE_REBUILD_CACHE: false;\n"
      "      RANGE_WARN_BATCHES: 1;\n"
      "    };\n"
      "    RUNTIME: {\n"
      "      SYMBOL: % __symbol ? BTCUSDT %;\n"
      "      FROM: % __from ? 01.01.2020 %;\n"
      "      TO: % __to ? 31.01.2020 %;\n"
      "      SOURCES_DSL_FILE: demo.sources.dsl;\n"
      "      CHANNELS_DSL_FILE: demo.channels.dsl;\n"
      "    };\n"
      "  };\n"
      "  WIKIMYEI: <rep> {\n"
      "    FAMILY: tsi.wikimyei.representation.vicreg;\n"
      "    HASHIMYEI: 0x0000;\n"
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
             "  IMPORT_CONTRACT_FILE \"demo.contract.dsl\";\n"
             "  IMPORT_WAVE_FILE \"demo.wave.dsl\";\n"
             "  BIND bind_alpha {\n"
             "    __sampler = random;\n"
             "    __workers = 2;\n"
             "    __symbol = ADAUSDT;\n"
             "    __from = 03.02.2020;\n"
             "    __to = 29.02.2020;\n"
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
      instructions_dir / "tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl";
  const fs::path transfer_eval_snapshot_path =
      instructions_dir / "tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl";
  const std::string vicreg_snapshot_text =
      read_file(vicreg_snapshot_path);
  const std::string embedding_eval_snapshot_text =
      read_file(embedding_eval_snapshot_path);
  const std::string transfer_eval_snapshot_text =
      read_file(transfer_eval_snapshot_path);
  assert(campaign_snapshot_text.find("IMPORT_CONTRACT_FILE \"binding.contract.dsl\";") !=
         std::string::npos);
  assert(campaign_snapshot_text.find("IMPORT_WAVE_FILE \"binding.wave.dsl\";") !=
         std::string::npos);
  assert(campaign_snapshot_text.find("CONTRACT = contract_binding;") !=
         std::string::npos);
  assert(campaign_snapshot_text.find("RUN bind_alpha;") != std::string::npos);
  assert(contract_snapshot_text.find("__lr = 0.123;") != std::string::npos);
  assert(contract_snapshot_text.find("CIRCUIT_FILE: demo.contract.circuit.dsl;") !=
         std::string::npos);
  assert(wave_snapshot_text.find("WORKERS: 2;") != std::string::npos);
  assert(wave_snapshot_text.find("SAMPLER: random;") != std::string::npos);
  assert(wave_snapshot_text.find("SYMBOL: ADAUSDT;") != std::string::npos);
  assert(wave_snapshot_text.find("FROM: 03.02.2020;") != std::string::npos);
  assert(wave_snapshot_text.find("TO: 29.02.2020;") != std::string::npos);
  assert(wave_snapshot_text.find("SOURCES_DSL_FILE: demo.sources.dsl;") !=
         std::string::npos);
  assert(wave_snapshot_text.find("CHANNELS_DSL_FILE: demo.channels.dsl;") !=
         std::string::npos);
  assert(fs::exists(instructions_dir / "demo.contract.circuit.dsl"));
  assert(fs::exists(instructions_dir / "demo.sources.dsl"));
  assert(fs::exists(instructions_dir / "demo.channels.dsl"));
  assert(fs::exists(vicreg_snapshot_path));
  assert(fs::exists(embedding_eval_snapshot_path));
  assert(fs::exists(transfer_eval_snapshot_path));
  assert(fs::exists(instructions_dir / "demo.network.dsl"));
  assert(fs::exists(instructions_dir / "demo.jkimyei.dsl"));
  assert(vicreg_snapshot_text.find("learning_rate:float = 0.123") !=
         std::string::npos);
  assert(vicreg_snapshot_text.find("network_design_dsl_file:str = demo.network.dsl") !=
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

  const fs::path default_campaign_override_path =
      root / "default_campaign_override.dsl";
  std::string default_campaign_override =
      read_file("/cuwacunu/src/config/instructions/defaults/default.iitepi.campaign.dsl");
  replace_all(&default_campaign_override,
              "IMPORT_CONTRACT_FILE \"default.iitepi.contract.dsl\";",
              "IMPORT_CONTRACT_FILE "
              "\"/cuwacunu/src/config/instructions/defaults/default.iitepi.contract.dsl\";");
  replace_all(&default_campaign_override,
              "IMPORT_WAVE_FILE \"default.iitepi.wave.dsl\";",
              "IMPORT_WAVE_FILE "
              "\"/cuwacunu/src/config/instructions/defaults/default.iitepi.wave.dsl\";");
  replace_all(&default_campaign_override, "__sampler = sequential;",
              "__sampler = random;");
  replace_all(&default_campaign_override, "__workers = 0;", "__workers = 2;");
  replace_all(&default_campaign_override, "__symbol = BTCUSDT;",
              "__symbol = ADAUSDT;");
  write_file(default_campaign_override_path, default_campaign_override);

  ok = cuwacunu::hero::wave_contract_binding_runtime::
      prepare_campaign_binding_snapshot(default_campaign_override_path,
                                        "bind_train_vicreg_primary",
                                        root / "default_campaign_job",
                                        &campaign_snapshot, &error);
  assert(ok);
  assert(error.empty());
  const std::string default_wave_snapshot_text =
      read_file(campaign_snapshot.wave_dsl_path);
  assert(default_wave_snapshot_text.find("SAMPLER: random;") !=
         std::string::npos);
  assert(default_wave_snapshot_text.find("WORKERS: 2;") !=
         std::string::npos);
  assert(default_wave_snapshot_text.find("SYMBOL: ADAUSDT;") !=
         std::string::npos);

  fs::remove_all(root, ec);
  return 0;
}
