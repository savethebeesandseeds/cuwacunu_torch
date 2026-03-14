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
  const fs::path board_path = source_dir / "demo.board.dsl";
  const fs::path campaign_path = source_dir / "demo.campaign.dsl";

  write_file(circuit_path, "TSIEMENE_CIRCUIT { }\n");
  write_file(sources_path, "SOURCES { }\n");
  write_file(channels_path, "CHANNELS { }\n");

  write_file(contract_path,
             "-----BEGIN IITEPI CONTRACT-----\n"
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
      "      FROM: 01.01.2020;\n"
      "      TO: 31.01.2020;\n"
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

  write_file(board_path,
             "ACTIVE_BIND bind_alpha;\n\n"
             "BOARD {\n"
             "  IMPORT_CONTRACT_FILE \"demo.contract.dsl\";\n"
             "  IMPORT_WAVE_FILE \"demo.wave.dsl\";\n"
             "  BIND bind_alpha {\n"
             "    __sampler = random;\n"
             "    __workers = 2;\n"
             "    __symbol = ADAUSDT;\n"
             "    CONTRACT = contract_demo;\n"
             "    WAVE = wave_alpha;\n"
             "  };\n"
             "}\n");

  write_file(campaign_path,
             "ACTIVE_CAMPAIGN campaign_alpha;\n\n"
             "JKIMYEI_CAMPAIGN {\n"
             "  IMPORT_CONTRACT_FILE \"demo.contract.dsl\";\n"
             "  IMPORT_WAVE_FILE \"demo.wave.dsl\";\n"
             "  BIND bind_alpha {\n"
             "    __sampler = random;\n"
             "    __workers = 2;\n"
             "    __symbol = ADAUSDT;\n"
             "    CONTRACT = contract_demo;\n"
             "    WAVE = wave_alpha;\n"
             "  };\n"
             "  CAMPAIGN campaign_alpha {\n"
             "    BINDS = bind_alpha;\n"
             "  };\n"
             "}\n");

  cuwacunu::hero::wave_contract_binding_runtime::
      resolved_wave_contract_binding_snapshot_t board_snapshot{};
  std::string error{};
  bool ok = cuwacunu::hero::wave_contract_binding_runtime::
      prepare_board_binding_snapshot(board_path, "bind_alpha", root / "board_job",
                                     &board_snapshot, &error);
  assert(ok);
  assert(error.empty());
  assert(fs::exists(board_snapshot.board_dsl_path));
  assert(fs::exists(board_snapshot.contract_dsl_path));
  assert(fs::exists(board_snapshot.wave_dsl_path));
  assert(board_snapshot.binding_id == "bind_alpha");
  assert(board_snapshot.original_contract_ref == "contract_demo");
  assert(board_snapshot.wave_ref == "wave_alpha");
  assert(board_snapshot.variables.size() == 3);

  const std::string board_snapshot_text = read_file(board_snapshot.board_dsl_path);
  const std::string contract_snapshot_text =
      read_file(board_snapshot.contract_dsl_path);
  const std::string wave_snapshot_text = read_file(board_snapshot.wave_dsl_path);
  assert(board_snapshot_text.find("IMPORT_CONTRACT_FILE \"binding.contract.dsl\";") !=
         std::string::npos);
  assert(board_snapshot_text.find("IMPORT_WAVE_FILE \"binding.wave.dsl\";") !=
         std::string::npos);
  assert(board_snapshot_text.find("CONTRACT = contract_binding;") !=
         std::string::npos);
  assert(contract_snapshot_text.find(circuit_path.string()) != std::string::npos);
  assert(wave_snapshot_text.find("WORKERS: 2;") != std::string::npos);
  assert(wave_snapshot_text.find("SAMPLER: random;") != std::string::npos);
  assert(wave_snapshot_text.find("SYMBOL: ADAUSDT;") != std::string::npos);
  assert(wave_snapshot_text.find(sources_path.string()) != std::string::npos);
  assert(wave_snapshot_text.find(channels_path.string()) != std::string::npos);

  cuwacunu::hero::wave_contract_binding_runtime::
      resolved_wave_contract_binding_snapshot_t campaign_snapshot{};
  ok = cuwacunu::hero::wave_contract_binding_runtime::
      prepare_campaign_binding_snapshot(campaign_path, "bind_alpha",
                                        root / "campaign_job",
                                        &campaign_snapshot, &error);
  assert(ok);
  assert(error.empty());
  assert(fs::exists(campaign_snapshot.board_dsl_path));
  assert(fs::exists(campaign_snapshot.contract_dsl_path));
  assert(fs::exists(campaign_snapshot.wave_dsl_path));
  assert(read_file(campaign_snapshot.wave_dsl_path).find("SYMBOL: ADAUSDT;") !=
         std::string::npos);

  fs::remove_all(root, ec);
  return 0;
}
