#include "jkimyei/training/representation/graph_first_representation_launcher.h"

#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

namespace builder = cuwacunu::kikijyeba::protocol;
namespace launcher = cuwacunu::jkimyei::training::representation;
namespace types = cuwacunu::ujcamei::source::registry::types;

namespace {

using Kline = types::kline_t;

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::string read_text(const std::filesystem::path &path) {
  std::ifstream in(path);
  check(in.is_open(), "failed to open text input");
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  static int counter = 0;
  auto dir = std::filesystem::temp_directory_path() /
             ("cuwacunu_graph_first_representation_launcher_" + label + "_" +
              std::to_string(static_cast<long long>(::getpid())) + "_" +
              std::to_string(counter++));
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir;
}

Kline make_kline(types::ms_t close_time, double price) {
  Kline out{};
  out.open_time = close_time - 1;
  out.open_price = price;
  out.high_price = price + 1.0;
  out.low_price = price - 1.0;
  out.close_price = price;
  out.volume = 1.0;
  out.close_time = close_time;
  out.quote_asset_volume = price;
  out.number_of_trades = 1;
  out.taker_buy_base_volume = 0.5;
  out.taker_buy_quote_volume = 0.5 * price;
  return out;
}

void write_kline_csv(const std::filesystem::path &path,
                     const std::vector<types::ms_t> &keys,
                     double price_offset) {
  std::ofstream out(path, std::ios::trunc);
  check(out.is_open(), "failed to open csv output");
  for (std::size_t i = 0; i < keys.size(); ++i) {
    make_kline(keys[i], price_offset + static_cast<double>(i)).to_csv(out, ',');
    out << '\n';
  }
}

void write_text(const std::filesystem::path &path, const std::string &text) {
  std::ofstream out(path, std::ios::trunc);
  check(out.is_open(), "failed to open text output");
  out << text;
}

struct fixture_paths_t {
  std::filesystem::path dir{};
  std::filesystem::path config{};
  std::filesystem::path report{};
};

fixture_paths_t
make_config_fixture(const std::string &label, int64_t max_steps = 2,
                    int64_t checkpoint_every = 1, int64_t batch_size = 2,
                    int64_t report_every = 1,
                    const std::string &wave_mode = "run",
                    const std::string &wave_range = "  SOURCE_RANGE = all;\n") {
  fixture_paths_t out{};
  out.dir = make_tmp_dir(label);
  out.report = out.dir / "representation.report";
  const auto btc_csv = out.dir / "BTCUSDT.csv";
  const auto usdt_btc_csv = out.dir / "USDTBTC.csv";
  const auto eth_csv = out.dir / "ETHUSDT.csv";
  const auto usdt_eth_csv = out.dir / "USDTETH.csv";
  write_kline_csv(btc_csv, {1000, 1001, 1002, 1003, 1004, 1005, 1006}, 100.0);
  write_kline_csv(usdt_btc_csv, {1000, 1001, 1002, 1003, 1004, 1005, 1006},
                  0.01);
  write_kline_csv(eth_csv, {1000, 1001, 1002, 1003, 1004, 1005, 1006}, 200.0);
  write_kline_csv(usdt_eth_csv, {1000, 1001, 1002, 1003, 1004, 1005, 1006},
                  0.005);

  const auto sources_dsl = out.dir / "ujcamei.source.registry.dsl";
  const auto channels_dsl = out.dir / "ujcamei.source.retrieval.channels.dsl";
  const auto graph_dsl = out.dir / "kikijyeba.topology.graph.dsl";
  const auto wave_dsl = out.dir / "kikijyeba.settings.wave.dsl";
  const auto vicreg_training_dsl =
      out.dir / "wikimyei.representation.vicreg.jkimyei";
  out.config = out.dir / ".config";

  write_text(
      sources_dsl,
      std::string(
          "CSV_POLICY {\n"
          "  CSV_BOOTSTRAP_DELTAS = 128;\n"
          "  CSV_STEP_ABS_TOL = 1e-7;\n"
          "  CSV_STEP_REL_TOL = 1e-9;\n"
          "};\n"
          "DATA_ANALYTICS_POLICY {\n"
          "  MAX_SAMPLES = 64;\n"
          "  MAX_FEATURES = 32;\n"
          "  MASK_EPSILON = 1e-12;\n"
          "  STANDARDIZE_EPSILON = 1e-8;\n"
          "};\n"
          "/-------------------------------------------------------------------"
          "--------------------------------------------------------------------"
          "-----\\\n"
          "|  instrument  |  interval  |  record_type  |  market_type  |  "
          "venue  |  base_asset  |  quote_asset  |  source_kind  |  source     "
          "         |\n"
          "|-------------------------------------------------------------------"
          "--------------------------------------------------------------------"
          "-----|\n"
          "|  BTCUSDT     |    1m      |     kline     | spot          | "
          "binance | BTC          | USDT          |  real         | ") +
          btc_csv.string() +
          " |\n"
          "|  USDTBTC     |    1m      |     kline     | spot          | "
          "binance | USDT         | BTC           |  real         | " +
          usdt_btc_csv.string() +
          " |\n"
          "|  ETHUSDT     |    1m      |     kline     | spot          | "
          "binance | ETH          | USDT          |  real         | " +
          eth_csv.string() +
          " |\n"
          "|  USDTETH     |    1m      |     kline     | spot          | "
          "binance | USDT         | ETH           |  real         | " +
          usdt_eth_csv.string() +
          " |\n"
          "\\------------------------------------------------------------------"
          "--------------------------------------------------------------------"
          "------/\n");

  write_text(channels_dsl,
             "/----------------------------------------------------------------"
             "-----------------------------------------------\\\n"
             "|  interval   |  active   |  record_type | input_length | "
             "future_length | channel_weight | normalization_policy |\n"
             "|----------------------------------------------------------------"
             "-----------------------------------------------|\n"
             "|    1m       |   true    |    kline     |     2        |       "
             "1       |      1.0       |  log_returns         |\n"
             "\\---------------------------------------------------------------"
             "------------------------------------------------/\n");

  write_text(graph_dsl, "GRAPH_POLICY {\n"
                        "  EDGE_RESOLUTION_POLICY = explicit_only;\n"
                        "  EDGE_SOURCE_KIND = real;\n"
                        "  FETCH_MODE = serial;\n"
                        "  MAX_FETCH_WORKERS = 0;\n"
                        "  PARALLEL_MIN_WORK_ITEMS = 16;\n"
                        "};\n"
                        "/------------------------------------\\\n"
                        "|  node_id  |  node_kind  |  active  |\n"
                        "|------------------------------------|\n"
                        "|   BTC     |    asset    |   true   |\n"
                        "|   USDT    |    asset    |   true   |\n"
                        "|   ETH     |    asset    |   true   |\n"
                        "\\------------------------------------/\n"
                        "/-----------------------------------------------------"
                        "------------------\\\n"
                        "|  edge_id  |  base_node  |  quote_node  |  "
                        "source_instrument  | active |\n"
                        "|-----------------------------------------------------"
                        "------------------|\n"
                        "| BTCUSDT   |    BTC      |    USDT      |       "
                        "BTCUSDT       |  true  |\n"
                        "| USDTBTC   |    USDT     |    BTC       |       "
                        "USDTBTC       |  true  |\n"
                        "| ETHUSDT   |    ETH      |    USDT      |       "
                        "ETHUSDT       |  true  |\n"
                        "| USDTETH   |    USDT     |    ETH       |       "
                        "USDTETH       |  true  |\n"
                        "\\----------------------------------------------------"
                        "-------------------/\n");

  write_text(wave_dsl,
             std::string("WAVE_SETTINGS {\n"
                         "  WAVE_ID = cwu_01v_smoke;\n"
                         "  TARGET = wikimyei.representation.encoding.vicreg;\n"
                         "  MODE = ") +
                 wave_mode +
                 ";\n"
                 "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                 "  SOURCE_CURSOR_SCOPE = wave_batch;\n" +
                 wave_range + "};\n");

  write_text(
      vicreg_training_dsl,
      std::string("TRAINING {\n"
                  "  VERSION = wikimyei.representation.vicreg.jkimyei.v1;\n"
                  "  TRAINING_ID = smoke_vicreg_node_representation;\n"
                  "  TASK = vicreg_node_representation;\n"
                  "  COMPONENT_ID = node_vicreg_v1;\n"
                  "  OPTIMIZER = adam;\n"
                  "  LEARNING_RATE = 0.001;\n"
                  "  MAX_STEPS = ") +
          std::to_string(max_steps) +
          ";\n"
          "  BATCH_SIZE = " +
          std::to_string(batch_size) +
          ";\n"
          "  GRAD_CLIP_NORM = 1.0;\n"
          "  CHECKPOINT_EVERY = " +
          std::to_string(checkpoint_every) +
          ";\n"
          "  REPORT_EVERY = " +
          std::to_string(report_every) +
          ";\n"
          "  VALIDATION_EVERY = 0;\n"
          "  SEED = 17;\n"
          "  FREEZE_REPRESENTATION = false;\n"
          "};\n");

  write_text(
      out.config,
      std::string(
          "[UJCAMEI]\n"
          "ujcamei_source_registry_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/ujcamei.source.registry.dsl.bnf\n"
          "ujcamei_source_registry_dsl_path = ") +
          sources_dsl.string() +
          "\n"
          "ujcamei_source_retrieval_channels_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "ujcamei.source.retrieval.channels.dsl.bnf\n"
          "ujcamei_source_retrieval_channels_dsl_path = " +
          channels_dsl.string() +
          "\n"
          "kikijyeba_topology_graph_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "kikijyeba.topology.graph.dsl.bnf\n"
          "kikijyeba_topology_graph_dsl_path = " +
          graph_dsl.string() +
          "\n"
          "kikijyeba_settings_wave_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/kikijyeba.settings.wave.dsl.bnf\n"
          "kikijyeba_settings_wave_dsl_path = " +
          wave_dsl.string() +
          "\n\n"
          "[WIKIMYEI]\n"
          "wikimyei_expression_nodelift_srl_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.expression.nodelift.srl.dsl.bnf\n"
          "wikimyei_expression_nodelift_srl_dsl_path = "
          "/cuwacunu/src/config/wikimyei.expression.nodelift.srl.dsl\n"
          "wikimyei_representation_vicreg_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.representation.vicreg.dsl.bnf\n"
          "wikimyei_representation_vicreg_dsl_path = "
          "/cuwacunu/src/config/wikimyei.representation.vicreg.dsl\n"
          "wikimyei_representation_vicreg_net_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.representation.vicreg.net.bnf\n"
          "wikimyei_representation_vicreg_net_path = "
          "/cuwacunu/src/config/wikimyei.representation.vicreg.net\n"
          "wikimyei_inference_expected_value_mdn_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.dsl.bnf\n"
          "wikimyei_inference_expected_value_mdn_dsl_path = "
          "/cuwacunu/src/config/wikimyei.inference.expected_value.mdn.dsl\n"
          "wikimyei_inference_expected_value_mdn_net_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.net.bnf\n"
          "wikimyei_inference_expected_value_mdn_net_path = "
          "/cuwacunu/src/config/wikimyei.inference.expected_value.mdn.net\n\n"
          "[JKIMYEI]\n"
          "wikimyei_representation_vicreg_jkimyei_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.representation.vicreg.jkimyei.bnf\n"
          "wikimyei_representation_vicreg_jkimyei_path = " +
          vicreg_training_dsl.string() +
          "\n"
          "wikimyei_inference_expected_value_mdn_jkimyei_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.jkimyei.bnf\n"
          "wikimyei_inference_expected_value_mdn_jkimyei_path = "
          "/cuwacunu/src/config/"
          "wikimyei.inference.expected_value.mdn.jkimyei\n");

  return out;
}

launcher::graph_first_representation_launcher_t<Kline> make_launcher(
    const fixture_paths_t &fixture, bool force_rebuild_cache = true,
    bool write_report = false,
    cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t
        runtime_report_mode = cuwacunu::kikijyeba::lattice::runtime_report::
            runtime_report_mode_t::normal,
    std::size_t batch_size = 2) {
  auto bundle =
      builder::load_graph_first_config_bundle_from_config(fixture.config);
  builder::graph_first_pipeline_builder_options_t builder_options{};
  builder_options.force_rebuild_cache = force_rebuild_cache;
  builder_options.batch_size = batch_size;
  builder::graph_first_pipeline_builder_t<Kline> pipe(bundle, builder_options);

  launcher::graph_first_representation_launcher_options_t launcher_options{};
  launcher_options.write_report = write_report;
  launcher_options.report_path = fixture.report;
  launcher_options.runtime_report_mode = runtime_report_mode;
  return launcher::graph_first_representation_launcher_t<Kline>(
      std::move(pipe), launcher_options);
}

void test_dry_run_launcher_report() {
  const auto fixture = make_config_fixture("dry_run", /*max_steps=*/2,
                                           /*checkpoint_every=*/0);
  auto graph_launcher = make_launcher(fixture);
  const auto report = graph_launcher.dry_run_report();
  check(report.training_id == "smoke_vicreg_node_representation",
        "dry-run training id");
  check(!report.graph_order_fingerprint.empty(), "dry-run graph fingerprint");
  check(report.node_ids.size() == 3, "dry-run node count");
  check(report.edge_ids.size() == 4, "dry-run edge count");
  check(report.encoding_dim == 8, "dry-run encoding dim");
  check(report.mask_profile == "price_only", "dry-run mask profile");
  check(report.required_feature_coords == std::vector<int64_t>({0, 1, 2, 3}),
        "dry-run required feature coords");
  check(report.effective_batch_size == 2, "dry-run effective batch size");
  check(report.dtype == "float32", "dry-run dtype");
  check(report.device == "cpu", "dry-run device");
  check(report.seed == 17, "dry-run seed");
  check(report.checkpoint_every == 0, "dry-run checkpoint cadence");
  check(report.report_every == 1, "dry-run report cadence");
  check(report.validation_every == 0, "dry-run validation cadence");
  check(report.wave_source_range_policy == "all", "dry-run wave range policy");
  check(report.runtime_report_mode == "normal", "dry-run runtime mode");
  check(report.stream_plan.find("representation") != std::string::npos,
        "dry-run stream plan includes representation");
}

void test_config_backed_training_run_and_report_checkpoint() {
  torch::manual_seed(71);
  const auto fixture = make_config_fixture("training", /*max_steps=*/2,
                                           /*checkpoint_every=*/1,
                                           /*batch_size=*/2,
                                           /*report_every=*/1, "train");
  auto graph_launcher = make_launcher(fixture, /*force_rebuild_cache=*/true,
                                      /*write_report=*/true);
  const auto report = graph_launcher.run();
  check(report.steps_attempted > 0, "training attempted steps");
  check(report.steps_completed > 0, "training completed steps");
  check(report.optimizer_steps == report.steps_completed,
        "optimizer steps match completed steps");
  check(report.wave_pulses_attempted == report.steps_attempted,
        "training wave pulse attempts track steps");
  check(report.wave_pulses_completed == report.steps_completed,
        "training wave pulse completions track completed steps");
  check(report.total_valid_projection_rows > 0, "valid projection rows");
  check(report.adapter_kept_rows > 0, "adapter kept rows");
  check(std::isfinite(report.last_loss), "last loss finite");
  check(std::isfinite(report.mean_loss), "mean loss finite");
  check(std::isfinite(report.mean_invariance_loss), "mean invariance finite");
  check(std::isfinite(report.mean_variance_loss), "mean variance finite");
  check(std::isfinite(report.mean_covariance_loss), "mean covariance finite");
  check(report.finite_parameter_check == 1.0, "finite parameter check");
  check(report.node_ids.size() == 3, "training node ids");
  check(report.edge_ids.size() == 4, "training edge ids");
  check(report.effective_batch_size == 2, "training effective batch size");
  check(report.seed == 17, "training seed");
  check(report.wave_source_range_policy == "all", "training wave range policy");
  check(!report.source_cursor_token.empty(), "training source cursor token");
  check(report.source_anchor_count > 0, "training source anchor count");
  check(report.wave_streamed_anchor_count > 0,
        "training streamed wave anchors");
  check(report.report_written, "report written");
  check(std::filesystem::exists(fixture.report), "report path exists");
  check(report.checkpoint_written, "checkpoint metadata written");
  check(report.checkpoint_write_count == report.optimizer_steps,
        "checkpoint every optimizer step");
  check(std::filesystem::exists(report.checkpoint_path), "checkpoint exists");
  check(report.checkpoint_format == "torch_archive_vicreg_encoder_v1",
        "checkpoint format");
  const auto report_text = read_text(fixture.report);
  check(report_text.find("training_id=smoke_vicreg_node_representation") !=
            std::string::npos,
        "report contains training id");
  check(report_text.find("wave_source_range_policy=all") != std::string::npos,
        "report contains wave range policy");
  check(report_text.find("wave_pulses_attempted=") != std::string::npos,
        "report contains wave pulse attempts");
  check(report_text.find("mean_invariance_loss=") != std::string::npos,
        "report contains invariance loss");
  check(report_text.find("torch_archive_vicreg_encoder_v1") !=
            std::string::npos,
        "report contains checkpoint format");
}

void test_wave_range_and_debug_lls() {
  torch::manual_seed(73);
  const auto range_fixture = make_config_fixture(
      "training_wave_range", /*max_steps=*/0, /*checkpoint_every=*/0,
      /*batch_size=*/1, /*report_every=*/1, "train | debug",
      "  SOURCE_RANGE = anchor_index;\n"
      "  ANCHOR_INDEX_BEGIN = 1;\n"
      "  ANCHOR_INDEX_END = 3;\n");
  auto graph_launcher =
      make_launcher(range_fixture, /*force_rebuild_cache=*/true,
                    /*write_report=*/true,
                    cuwacunu::kikijyeba::lattice::runtime_report::
                        runtime_report_mode_t::normal,
                    /*batch_size=*/1);
  const auto report = graph_launcher.run();
  check(report.wave_source_range_policy == "anchor_index", "range wave policy");
  check(report.requested_anchor_index_begin == 1, "range wave begin index");
  check(report.requested_anchor_index_end == 3, "range wave end index");
  check(report.wave_streamed_anchor_count == 2,
        "range wave streams requested anchors");
  check(report.steps_attempted == 2,
        "range wave pulses until interval exhaustion");
  check(report.wave_pulses_attempted == 2, "range wave pulse counter");
  check(report.source_anchor_count > report.wave_streamed_anchor_count,
        "range wave narrower than full source domain");
  check(report.runtime_lls_emitted, "debug runtime LLS emitted");
  check(report.runtime_report_mode == "debug",
        "debug wave sets runtime report mode");
  check(
      report.representation_training_runtime_lls.find(
          "schema:str = wikimyei.representation.vicreg.training.runtime.v1") !=
          std::string::npos,
      "debug representation training LLS schema");
  check(report.representation_training_runtime_lls.find(
            "dock_binding_token:str = "
            "kikijyeba.topology.graph.dock_binding.v1/") != std::string::npos,
        "debug representation training LLS dock binding token");
  check(report.representation_training_runtime_lls.find(
            "loss_preference:str = lower_is_better") != std::string::npos,
        "debug representation training LLS loss preference");
  check(report.representation_training_runtime_lls.find("wave_pulse_index") !=
            std::string::npos,
        "debug representation training LLS pulse index");
  check(std::filesystem::exists(range_fixture.report.string() +
                                ".representation_training.lls"),
        "debug representation training LLS sidecar exists");
}

} // namespace

int main() {
  try {
    test_dry_run_launcher_report();
    test_config_backed_training_run_and_report_checkpoint();
    test_wave_range_and_debug_lls();
    std::cout << "[Jkimyei GraphFirstRepresentationLauncher test] all checks "
                 "passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "[Jkimyei GraphFirstRepresentationLauncher test] failure: "
              << ex.what() << "\n";
    return 1;
  }
}
