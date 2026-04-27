#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include "hero/runtime_hero/hero_runtime_tools.h"
#include "iitepi/config_space_t.h"
#include "iitepi/contract_space_t.h"

namespace {

constexpr const char *kGlobalConfigPath = "/cuwacunu/src/config/.config";

void write_file(const std::filesystem::path &path, const std::string &content) {
  std::error_code ec{};
  std::filesystem::create_directories(path.parent_path(), ec);
  assert(!ec);
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  assert(static_cast<bool>(out));
  out << content;
  out.close();
  assert(static_cast<bool>(out));
}

std::string json_quote(std::string_view value) {
  std::ostringstream out;
  out << '"';
  for (const char c : value) {
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
      out << c;
      break;
    }
  }
  out << '"';
  return out.str();
}

} // namespace

int main() {
  namespace fs = std::filesystem;

  const fs::path root =
      fs::temp_directory_path() / "runtime_binding_selection_explain";
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
      source_dir / "tsi.wikimyei.representation.encoding.vicreg.dsl";
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
  write_file(vicreg_path, "TAG:str = runtime.selection.vicreg\n"
                          "learning_rate:float = % __lr ? 0.001 %\n"
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
      "  __future_target_feature_indices = 0;\n"
      "  INSTRUMENT_SIGNATURE <src> {\n"
      "    SYMBOL: ANY;\n"
      "    RECORD_TYPE: kline;\n"
      "    MARKET_TYPE: spot;\n"
      "    VENUE: binance;\n"
      "    BASE_ASSET: ANY;\n"
      "    QUOTE_ASSET: ANY;\n"
      "  };\n"
      "  INSTRUMENT_SIGNATURE <rep> {\n"
      "    SYMBOL: ANY;\n"
      "    RECORD_TYPE: kline;\n"
      "    MARKET_TYPE: spot;\n"
      "    VENUE: binance;\n"
      "    BASE_ASSET: ANY;\n"
      "    QUOTE_ASSET: ANY;\n"
      "  };\n"
      "  INSTRUMENT_SIGNATURE <sink_null> {\n"
      "    SYMBOL: ANY;\n"
      "    RECORD_TYPE: ANY;\n"
      "    MARKET_TYPE: ANY;\n"
      "    VENUE: ANY;\n"
      "    BASE_ASSET: ANY;\n"
      "    QUOTE_ASSET: ANY;\n"
      "  };\n"
      "}\n"
      "ASSEMBLY {\n"
      "  __lr = 0.123;\n"
      "  __vicreg_config_dsl_file = "
      "tsi.wikimyei.representation.encoding.vicreg.dsl;\n"
      "  __embedding_sequence_analytics_config_dsl_file = "
      "tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl;\n"
      "  __transfer_matrix_evaluation_config_dsl_file = "
      "tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl;\n"
      "  __observation_sources_dsl_file = demo.sources.dsl;\n"
      "  __observation_channels_dsl_file = demo.channels.dsl;\n"
      "  CIRCUIT_FILE: demo.circuit.dsl;\n"
      "  AKNOWLEDGE: src = tsi.source.dataloader;\n"
      "  AKNOWLEDGE: rep = tsi.wikimyei.representation.encoding.vicreg;\n"
      "  AKNOWLEDGE: sink_null = tsi.sink.null;\n"
      "}\n"
      "-----END IITEPI CONTRACT-----\n");

  write_file(wave_path,
             "WAVE wave_alpha {\n"
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
             "      RUNTIME_INSTRUMENT_SIGNATURE: {\n"
             "        SYMBOL: % __symbol %;\n"
             "        RECORD_TYPE: kline;\n"
             "        MARKET_TYPE: spot;\n"
             "        VENUE: binance;\n"
             "        BASE_ASSET: % __base_asset %;\n"
             "        QUOTE_ASSET: % __quote_asset %;\n"
             "      };\n"
             "      FROM: % __from ? 01.01.2020 %;\n"
             "      TO: % __to ? 31.01.2020 %;\n"
             "    };\n"
             "  };\n"
             "  WIKIMYEI: <rep> {\n"
             "    FAMILY: tsi.wikimyei.representation.encoding.vicreg;\n"
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
             "    __base_asset = ADA;\n"
             "    __quote_asset = USDT;\n"
             "    __from = 03.02.2020;\n"
             "    __to = 29.02.2020;\n"
             "    CONTRACT = contract_demo;\n"
             "    WAVE = wave_alpha;\n"
             "  };\n"
             "  RUN bind_alpha;\n"
             "}\n");

  cuwacunu::iitepi::config_space_t::change_config_file(kGlobalConfigPath);

  cuwacunu::hero::runtime_mcp::app_context_t app{};
  app.global_config_path = kGlobalConfigPath;
  app.hero_config_path =
      cuwacunu::hero::runtime_mcp::resolve_runtime_hero_dsl_path(
          app.global_config_path);
  assert(!app.hero_config_path.empty());
  std::string error{};
  const bool loaded = cuwacunu::hero::runtime_mcp::load_runtime_defaults(
      app.hero_config_path, app.global_config_path, &app.defaults, &error);
  assert(loaded);
  assert(error.empty());
  app.self_binary_path = cuwacunu::hero::runtime_mcp::current_executable_path();

  const std::string args =
      "{\"binding_id\":\"bind_alpha\",\"campaign_dsl_path\":" +
      json_quote(campaign_path.string()) + "}";
  std::string tool_result{};
  std::string tool_error{};
  bool ok = cuwacunu::hero::runtime_mcp::execute_tool_json(
      "hero.runtime.explain_binding_selection", args, &app, &tool_result,
      &tool_error);
  assert(ok);
  assert(tool_error.empty());
  assert(tool_result.find("\"isError\":false") != std::string::npos);
  assert(tool_result.find("\"resolved\":true") != std::string::npos);
  assert(tool_result.find("\"binding_id\":\"bind_alpha\"") !=
         std::string::npos);
  assert(tool_result.find("\"component_count\":1") != std::string::npos);
  assert(tool_result.find("\"component_tag\":\"runtime.selection.vicreg\"") !=
         std::string::npos);
  assert(tool_result.find("\"derived_hashimyei\":\"0x") != std::string::npos);
  assert(tool_result.find("\"derived_canonical_path\":"
                          "\"tsi.wikimyei.representation.encoding.vicreg.0x") !=
         std::string::npos);
  assert(
      tool_result.find("\"compatibility_basis\":\"component_compatibility\"") !=
      std::string::npos);

  const std::string missing_bind_args =
      "{\"binding_id\":\"bind_missing\",\"campaign_dsl_path\":" +
      json_quote(campaign_path.string()) + "}";
  tool_result.clear();
  tool_error.clear();
  ok = cuwacunu::hero::runtime_mcp::execute_tool_json(
      "hero.runtime.explain_binding_selection", missing_bind_args, &app,
      &tool_result, &tool_error);
  assert(ok);
  assert(tool_error.empty());
  assert(tool_result.find("\"isError\":false") != std::string::npos);
  assert(tool_result.find("\"resolved\":false") != std::string::npos);
  assert(tool_result.find("unknown campaign binding id: bind_missing") !=
         std::string::npos);

  const std::string tools_json =
      cuwacunu::hero::runtime_mcp::build_tools_list_result_json();
  assert(tools_json.find("\"hero.runtime.explain_binding_selection\"") !=
         std::string::npos);

  fs::remove_all(root, ec);
  return 0;
}
