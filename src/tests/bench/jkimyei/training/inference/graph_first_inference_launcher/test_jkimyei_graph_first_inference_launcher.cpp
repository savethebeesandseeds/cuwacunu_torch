#include "jkimyei/training/inference/graph_first_inference_launcher.h"

#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <unistd.h>

namespace builder = cuwacunu::kikijyeba::protocol;
namespace launcher = cuwacunu::jkimyei::training::inference;
namespace types = cuwacunu::ujcamei::source::registry::types;

namespace {

using Kline = types::kline_t;

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <typename Fn> void expect_throws(Fn &&fn, const std::string &message) {
  bool threw = false;
  try {
    fn();
  } catch (const std::exception &) {
    threw = true;
  }
  check(threw, message);
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
             ("cuwacunu_graph_first_inference_launcher_" + label + "_" +
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
make_config_fixture(const std::string &label, int64_t max_steps = 3,
                    int64_t checkpoint_every = 1, int64_t mdn_batch_size = 2,
                    int64_t report_every = 1,
                    const std::string &wave_mode = "run",
                    const std::string &wave_range = "  SOURCE_RANGE = all;\n") {
  fixture_paths_t out{};
  out.dir = make_tmp_dir(label);
  out.report = out.dir / "inference.report";
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
  const auto mdn_training_dsl =
      out.dir / "wikimyei.inference.expected_value.mdn.jkimyei";
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

  write_text(graph_dsl, "/------------------------------------\\\n"
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

  write_text(mdn_training_dsl,
             std::string("TRAINING {\n"
                         "  VERSION = "
                         "wikimyei.inference.expected_value.mdn.jkimyei.v1;\n"
                         "  TRAINING_ID = smoke_mdn_inference;\n"
                         "  TASK = mdn_expected_value_inference;\n"
                         "  COMPONENT_ID = mdn_v1;\n"
                         "  OPTIMIZER = adam;\n"
                         "  LEARNING_RATE = 0.01;\n"
                         "  MAX_STEPS = ") +
                 std::to_string(max_steps) +
                 ";\n"
                 "  BATCH_SIZE = " +
                 std::to_string(mdn_batch_size) +
                 ";\n"
                 "  GRAD_CLIP_NORM = 0.0;\n"
                 "  CHECKPOINT_EVERY = " +
                 std::to_string(checkpoint_every) +
                 ";\n"
                 "  REPORT_EVERY = " +
                 std::to_string(report_every) +
                 ";\n"
                 "  VALIDATION_EVERY = 0;\n"
                 "  SEED = 31;\n"
                 "  FREEZE_REPRESENTATION = true;\n"
                 "  ALLOW_UNTRAINED_REPRESENTATION = true;\n"
                 "};\n");

  write_text(wave_dsl,
             std::string("WAVE_SETTINGS {\n"
                         "  WAVE_ID = cwu_01v_smoke;\n"
                         "  TARGET = wikimyei.inference.expected_value.mdn;\n"
                         "  MODE = ") +
                 wave_mode +
                 ";\n"
                 "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                 "  SOURCE_CURSOR_SCOPE = wave_batch;\n" +
                 wave_range + "};\n");

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
          "/cuwacunu/src/config/"
          "wikimyei.inference.expected_value.mdn.dsl\n\n"
          "wikimyei_inference_expected_value_mdn_net_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.net.bnf\n"
          "wikimyei_inference_expected_value_mdn_net_path = "
          "/cuwacunu/src/config/"
          "wikimyei.inference.expected_value.mdn.net\n\n"
          "[JKIMYEI]\n"
          "wikimyei_representation_vicreg_jkimyei_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.representation.vicreg.jkimyei.bnf\n"
          "wikimyei_representation_vicreg_jkimyei_path = "
          "/cuwacunu/src/config/wikimyei.representation.vicreg.jkimyei\n"
          "wikimyei_inference_expected_value_mdn_jkimyei_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.jkimyei.bnf\n"
          "wikimyei_inference_expected_value_mdn_jkimyei_path = " +
          mdn_training_dsl.string() + "\n");

  return out;
}

launcher::graph_first_inference_launcher_t<Kline> make_launcher(
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

  launcher::graph_first_inference_launcher_options_t launcher_options{};
  launcher_options.write_report = write_report;
  launcher_options.report_path = fixture.report;
  launcher_options.runtime_report_mode = runtime_report_mode;
  return launcher::graph_first_inference_launcher_t<Kline>(std::move(pipe),
                                                           launcher_options);
}

void test_dry_run_launcher_report() {
  const auto fixture = make_config_fixture("dry_run", /*max_steps=*/2,
                                           /*checkpoint_every=*/0);
  auto graph_launcher = make_launcher(fixture, /*force_rebuild_cache=*/true,
                                      /*write_report=*/false);
  const auto report = graph_launcher.dry_run_report();
  check(report.training_id == "smoke_mdn_inference", "dry-run training id");
  check(report.component_id == "mdn_v1", "dry-run fused MDN component id");
  check(report.input_representation_id == "node_vicreg_v1",
        "dry-run fused MDN input representation id");
  check(report.context_mode == "global_node_context",
        "dry-run fused MDN context mode");
  check(report.context_contract == "graph_order.node_representation.v1",
        "dry-run fused MDN context contract");
  check(report.context_value_shape == "[B_node,D_e]",
        "dry-run fused MDN context value shape");
  check(report.output_contract == "graph_order.node_expected_value.v1",
        "dry-run fused MDN output contract");
  check(report.output_value_shape ==
            "log_pi:[B,N,C,Hf,K];mu_sigma:[B,N,C,Hf,K,Df]",
        "dry-run fused MDN output value shape");
  check(!report.graph_order_fingerprint.empty(), "dry-run graph fingerprint");
  check(report.node_ids.size() == 3, "dry-run node count");
  check(report.edge_ids.size() == 4, "dry-run edge count");
  check(report.target_coords ==
            std::vector<int64_t>({0, 1, 2, 3, 4, 5, 6, 7, 8}),
        "dry-run target coords");
  check(report.context_dim == 8, "dry-run context dim");
  check(report.head_policy == "per_node", "dry-run head policy");
  check(report.target_domain == "node_future", "dry-run target domain");
  check(report.activity_target_semantics == "node_feature_support_mean",
        "dry-run activity target semantics");
  check(report.effective_batch_size == 2, "dry-run effective batch size");
  check(report.batch_size_source == "explicit", "dry-run batch size source");
  check(report.dtype == "float32", "dry-run dtype");
  check(report.device == "cpu", "dry-run device");
  check(report.seed == 31, "dry-run seed");
  check(report.seed_scope == "torch_manual_seed_cpu", "dry-run seed scope");
  check(report.representation_checkpoint_path.empty(),
        "dry-run representation checkpoint absent");
  check(!report.representation_checkpoint_loaded,
        "dry-run representation checkpoint not loaded");
  check(report.allow_untrained_representation,
        "dry-run allows untrained representation smoke");
  check(report.checkpoint_every == 0, "dry-run checkpoint cadence");
  check(report.report_every == 1, "dry-run report cadence");
  check(report.validation_every == 0, "dry-run validation cadence");
  check(report.analytics_status == "decoded_validated_not_emitted",
        "dry-run analytics status");
  check(report.wave_id == "cwu_01v_smoke", "dry-run wave id");
  check(report.wave_mode == "run", "dry-run wave mode");
  check(report.wave_source_cursor_kind == "graph_anchor",
        "dry-run wave cursor kind");
  check(report.wave_source_cursor_scope == "wave_batch",
        "dry-run wave cursor scope");
  check(report.wave_source_range_policy == "all", "dry-run wave range policy");
  check(report.requested_anchor_index_begin == -1,
        "dry-run default range begin");
  check(report.requested_anchor_index_end == -1, "dry-run default range end");
  check(report.runtime_report_mode == "normal", "dry-run runtime mode");
  check(report.stream_plan.find("nodelift") != std::string::npos,
        "dry-run report includes stream plan");
  check(report.stream_plan.find("representation") != std::string::npos,
        "dry-run stream plan includes representation");
}

void test_batch_size_config_contract() {
  const auto fixture = make_config_fixture("batch_contract", /*max_steps=*/1,
                                           /*checkpoint_every=*/0,
                                           /*mdn_batch_size=*/3);
  auto bundle =
      builder::load_graph_first_config_bundle_from_config(fixture.config);

  expect_throws(
      [&] {
        builder::graph_first_pipeline_builder_options_t options{};
        options.force_rebuild_cache = true;
        builder::graph_first_pipeline_builder_t<Kline> pipe(bundle, options);
        (void)pipe;
      },
      "builder requires explicit batch size when training specs disagree");

  expect_throws(
      [&] {
        builder::graph_first_pipeline_builder_options_t options{};
        options.force_rebuild_cache = true;
        options.batch_size = 2;
        builder::graph_first_pipeline_builder_t<Kline> pipe(bundle, options);
        launcher::graph_first_inference_launcher_t<Kline> graph_launcher(
            std::move(pipe));
        (void)graph_launcher;
      },
      "MDN launcher rejects builder batch size mismatch");

  builder::graph_first_pipeline_builder_options_t options{};
  options.force_rebuild_cache = true;
  options.batch_size = 3;
  builder::graph_first_pipeline_builder_t<Kline> pipe(bundle, options);
  check(pipe.batch_size_source() == "explicit",
        "builder records explicit batch size override");
  launcher::graph_first_inference_launcher_t<Kline> graph_launcher(
      std::move(pipe));
  (void)graph_launcher;
}

void test_config_backed_training_run_and_report_checkpoint() {
  torch::manual_seed(53);
  const auto fixture = make_config_fixture("training", /*max_steps=*/3,
                                           /*checkpoint_every=*/1,
                                           /*mdn_batch_size=*/2,
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
  check(report.total_valid_target_count > 0, "valid targets observed");
  check(report.total_valid_row_count > 0, "valid rows observed");
  check(report.total_trained_node_head_count > 0,
        "trained node heads observed");
  check(std::isfinite(report.last_loss), "last loss finite");
  check(std::isfinite(report.mean_loss), "mean loss finite");
  check(report.valid_target_count_by_node.size() == 3,
        "valid target counts by node");
  check(report.routed_row_count_by_node.size() == 3,
        "routed row counts by node");
  check(report.active_row_count_by_node.size() == 3,
        "active row counts by node");
  check(report.trained_row_count_by_node.size() == 3,
        "trained row counts by node");
  check(report.evaluated_row_count_by_node.size() == 3,
        "evaluated row counts by node");
  check(report.routed_row_count_by_node[0] > 0, "node routed support observed");
  check(report.trained_row_count_by_node[0] > 0,
        "node trained support observed");
  check(report.mean_nll_by_node.size() == 3, "mean nll by node");
  check(std::isfinite(report.mean_node_context_norm_mean),
        "context norm mean finite");
  check(std::isfinite(report.mean_mixture_entropy_mean),
        "mixture entropy mean finite");
  check(std::isfinite(report.mean_sigma_mean), "sigma mean finite");
  check(report.finite_parameter_check == 1.0, "finite parameter check");
  check(report.edge_ids.size() == 4, "training edge ids");
  check(report.node_ids.size() == 3, "training node ids");
  check(report.effective_batch_size == 2, "training effective batch size");
  check(report.batch_size_source == "explicit", "training batch size source");
  check(report.dtype == "float32", "training dtype");
  check(report.device == "cpu", "training device");
  check(report.seed == 31, "training seed");
  check(report.seed_scope == "torch_manual_seed_cpu", "training seed scope");
  check(report.representation_checkpoint_path.empty(),
        "training representation checkpoint absent");
  check(!report.representation_checkpoint_loaded,
        "training did not load representation checkpoint");
  check(report.allow_untrained_representation,
        "training allows untrained representation smoke");
  check(report.checkpoint_every == 1, "training checkpoint cadence");
  check(report.report_every == 1, "training report cadence");
  check(report.validation_every == 0, "training validation cadence");
  check(report.analytics_status == "decoded_validated_not_emitted",
        "training analytics status");
  check(report.wave_id == "cwu_01v_smoke", "training wave id");
  check(report.wave_mode == "train", "training wave mode");
  check(report.wave_source_range_policy == "all", "training wave range policy");
  check(report.runtime_report_mode == "normal", "training runtime mode");
  check(!report.source_cursor_token.empty(), "training source cursor token");
  check(report.source_anchor_count > 0, "training source anchor count");
  check(report.source_candidate_anchor_count >= report.source_anchor_count,
        "training source candidate anchors");
  check(!report.source_first_anchor_key.empty(),
        "training source first anchor");
  check(!report.source_last_anchor_key.empty(), "training source last anchor");
  check(report.wave_streamed_anchor_count > 0,
        "training streamed wave anchors");
  check(!report.wave_first_anchor_key.empty(), "training wave first anchor");
  check(!report.wave_last_anchor_key.empty(), "training wave last anchor");
  check(report.report_written, "report file written");
  check(std::filesystem::exists(fixture.report), "report path exists");
  check(report.checkpoint_written, "checkpoint metadata written");
  check(report.checkpoint_write_count == report.optimizer_steps,
        "checkpoint every optimizer step");
  check(report.last_checkpoint_optimizer_step == report.optimizer_steps,
        "last checkpoint optimizer step");
  check(std::filesystem::exists(report.checkpoint_path), "checkpoint exists");
  check(report.checkpoint_format == "torch_archive_mdn_v1",
        "checkpoint format");
  const auto report_text = read_text(fixture.report);
  check(report_text.find("training_id=smoke_mdn_inference") !=
            std::string::npos,
        "report contains training id");
  check(report_text.find("component_id=mdn_v1") != std::string::npos,
        "report contains fused MDN component id");
  check(report_text.find("input_representation_id=node_vicreg_v1") !=
            std::string::npos,
        "report contains fused MDN input representation id");
  check(report_text.find("context_mode=global_node_context") !=
            std::string::npos,
        "report contains fused MDN context mode");
  check(
      report_text.find("context_contract=graph_order.node_representation.v1") !=
          std::string::npos,
      "report contains fused MDN context contract");
  check(report_text.find("context_value_shape=[B_node,D_e]") !=
            std::string::npos,
        "report contains fused MDN context shape");
  check(
      report_text.find("output_contract=graph_order.node_expected_value.v1") !=
          std::string::npos,
      "report contains fused MDN output contract");
  check(report_text.find("output_value_shape=log_pi:[B,N,C,Hf,K];"
                         "mu_sigma:[B,N,C,Hf,K,Df]") != std::string::npos,
        "report contains fused MDN output shape");
  check(report_text.find("graph_order_fingerprint=") != std::string::npos,
        "report contains graph fingerprint");
  check(report_text.find("effective_batch_size=2") != std::string::npos,
        "report contains effective batch size");
  check(report_text.find("batch_size_source=explicit") != std::string::npos,
        "report contains batch size source");
  check(report_text.find("dtype=float32") != std::string::npos,
        "report contains dtype");
  check(report_text.find("device=cpu") != std::string::npos,
        "report contains device");
  check(report_text.find("seed=31") != std::string::npos,
        "report contains seed");
  check(report_text.find("checkpoint_every=1") != std::string::npos,
        "report contains checkpoint cadence");
  check(report_text.find("report_every=1") != std::string::npos,
        "report contains report cadence");
  check(report_text.find("checkpoint_write_count=") != std::string::npos,
        "report contains checkpoint write count");
  check(report_text.find("report_write_count=") != std::string::npos,
        "report contains report write count");
  check(report_text.find("analytics_status=decoded_validated_not_emitted") !=
            std::string::npos,
        "report contains analytics status");
  check(report_text.find("wave_id=cwu_01v_smoke") != std::string::npos,
        "report contains wave id");
  check(report_text.find("wave_source_range_policy=all") != std::string::npos,
        "report contains wave range policy");
  check(report_text.find("wave_pulses_attempted=") != std::string::npos,
        "report contains wave pulse attempts");
  check(report_text.find("runtime_report_mode=normal") != std::string::npos,
        "report contains runtime report mode");
  check(report_text.find("edge_ids=BTCUSDT,USDTBTC,ETHUSDT,USDTETH") !=
            std::string::npos,
        "report contains edge ids");
  check(report_text.find("routed_row_count_by_node=") != std::string::npos,
        "report contains per-node routed row counts");
  check(report_text.find("active_row_count_by_node=") != std::string::npos,
        "report contains per-node active row counts");
  check(report_text.find("trained_row_count_by_node=") != std::string::npos,
        "report contains per-node trained row counts");
  check(report_text.find("evaluated_row_count_by_node=") != std::string::npos,
        "report contains per-node evaluated row counts");
  check(report_text.find("mean_nll_by_node=") != std::string::npos,
        "report contains per-node nll");
  check(report_text.find("finite_parameter_check=1") != std::string::npos,
        "report contains parameter check");
  check(report_text.find("torch_archive_mdn_v1") != std::string::npos,
        "report contains MDN checkpoint format");
  check(report_text.find("target_domain=node_future") != std::string::npos,
        "report contains node-future target domain");
  check(report_text.find("mdn_checkpoint_loaded=false") != std::string::npos,
        "initial training report records absent MDN input checkpoint");

  auto restored_bundle =
      builder::load_graph_first_config_bundle_from_config(fixture.config);
  restored_bundle.mdn_training.input_mdn_checkpoint_path =
      report.checkpoint_path;
  restored_bundle.mdn_training.checkpoint_every = 0;
  restored_bundle.mdn_training.max_steps = 1;
  builder::graph_first_pipeline_builder_options_t restored_builder_options{};
  restored_builder_options.force_rebuild_cache = false;
  restored_builder_options.batch_size = 2;
  builder::graph_first_pipeline_builder_t<Kline> restored_pipe(
      restored_bundle, restored_builder_options);
  launcher::graph_first_inference_launcher_options_t restored_options{};
  restored_options.write_report = true;
  restored_options.report_path = fixture.dir / "restored_mdn.report";
  launcher::graph_first_inference_launcher_t<Kline> restored_launcher(
      std::move(restored_pipe), restored_options);
  const auto restored_report = restored_launcher.run();
  check(restored_report.mdn_checkpoint_loaded,
        "MDN input checkpoint should be loaded when configured");
  check(restored_report.mdn_checkpoint_path == report.checkpoint_path,
        "MDN input checkpoint path should be reported");
  const auto restored_report_text = read_text(restored_options.report_path);
  check(restored_report_text.find("mdn_checkpoint_loaded=true") !=
            std::string::npos,
        "restored report records loaded MDN checkpoint");

  const auto range_fixture = make_config_fixture(
      "training_wave_range", /*max_steps=*/0, /*checkpoint_every=*/0,
      /*mdn_batch_size=*/1, /*report_every=*/1, "train",
      "  SOURCE_RANGE = anchor_index;\n"
      "  ANCHOR_INDEX_BEGIN = 1;\n"
      "  ANCHOR_INDEX_END = 3;\n");
  auto range_launcher =
      make_launcher(range_fixture, /*force_rebuild_cache=*/true,
                    /*write_report=*/false,
                    cuwacunu::kikijyeba::lattice::runtime_report::
                        runtime_report_mode_t::normal,
                    /*batch_size=*/1);
  const auto range_report = range_launcher.run();
  check(range_report.wave_source_range_policy == "anchor_index",
        "range wave policy");
  check(range_report.requested_anchor_index_begin == 1,
        "range wave begin index");
  check(range_report.requested_anchor_index_end == 3, "range wave end index");
  check(range_report.wave_streamed_anchor_count == 2,
        "range wave streams requested anchor count");
  check(range_report.steps_attempted == 2,
        "range wave pulses batches until interval exhaustion");
  check(range_report.wave_pulses_attempted == 2, "range wave pulse counter");
  check(range_report.source_anchor_count >
            range_report.wave_streamed_anchor_count,
        "range wave is narrower than full source domain");

  const auto debug_fixture =
      make_config_fixture("training_debug", /*max_steps=*/1,
                          /*checkpoint_every=*/0, /*mdn_batch_size=*/2,
                          /*report_every=*/1, "train | debug");
  auto debug_launcher =
      make_launcher(debug_fixture, /*force_rebuild_cache=*/true,
                    /*write_report=*/true);
  const auto debug_report = debug_launcher.run();
  check(debug_report.runtime_lls_emitted, "debug runtime LLS emitted");
  check(debug_report.runtime_report_mode == "debug",
        "debug wave sets runtime report mode");
  check(debug_report.nodelift_runtime_lls.find(
            "schema:str = wikimyei.expression.nodelift.srl.runtime.v1") !=
            std::string::npos,
        "debug NodeLift runtime LLS schema");
  check(debug_report.nodelift_runtime_lls.find(
            "component_id:str = nodelift_srl_v1") != std::string::npos,
        "debug NodeLift runtime LLS component id");
  check(debug_report.nodelift_runtime_lls.find(
            "assembly_token:str = wikimyei.expression.nodelift.srl/"
            "nodelift_srl_v1/wikimyei.expression.nodelift.srl.v1") !=
            std::string::npos,
        "debug NodeLift runtime LLS assembly token");
  check(debug_report.nodelift_runtime_lls.find(
            "dock_binding_token:str = "
            "kikijyeba.topology.graph.dock_binding.v1/") != std::string::npos,
        "debug NodeLift runtime LLS dock binding token");
  check(debug_report.nodelift_runtime_lls.find("wave_id:str = cwu_01v_smoke") !=
            std::string::npos,
        "debug NodeLift runtime LLS wave id");
  check(debug_report.nodelift_runtime_lls.find(
            "source_range_policy:str = all") != std::string::npos,
        "debug NodeLift runtime LLS source range policy");
  check(debug_report.nodelift_runtime_lls.find("source_cursor_token") ==
            std::string::npos,
        "debug NodeLift runtime LLS does not mislabel batch cursor");
  check(debug_report.representation_runtime_lls.find(
            "schema:str = wikimyei.representation.vicreg.runtime.v1") !=
            std::string::npos,
        "debug representation runtime LLS schema");
  check(debug_report.representation_runtime_lls.find(
            "component_id:str = node_vicreg_v1") != std::string::npos,
        "debug representation runtime LLS component id");
  check(debug_report.representation_runtime_lls.find(
            "assembly_token:str = wikimyei.representation.encoding.vicreg/"
            "node_vicreg_v1/wikimyei.representation.vicreg.v1") !=
            std::string::npos,
        "debug representation runtime LLS assembly token");
  check(debug_report.representation_runtime_lls.find(
            "dock_binding_token:str = "
            "kikijyeba.topology.graph.dock_binding.v1/") != std::string::npos,
        "debug representation runtime LLS dock binding token");
  check(debug_report.mdn_runtime_lls.find(
            "schema:str = wikimyei.inference.expected_value.mdn.runtime.v1") !=
            std::string::npos,
        "debug MDN runtime LLS schema");
  check(debug_report.mdn_runtime_lls.find("component_id:str = mdn_v1") !=
            std::string::npos,
        "debug MDN runtime LLS component id");
  check(debug_report.mdn_runtime_lls.find(
            "assembly_token:str = wikimyei.inference.expected_value.mdn/"
            "mdn_v1/wikimyei.inference.expected_value.mdn.v1") !=
            std::string::npos,
        "debug MDN runtime LLS assembly token");
  check(debug_report.mdn_runtime_lls.find(
            "dock_binding_token:str = "
            "kikijyeba.topology.graph.dock_binding.v1/") != std::string::npos,
        "debug MDN runtime LLS dock binding token");
  check(debug_report.mdn_runtime_lls.find(
            "loss_preference:str = lower_is_better") != std::string::npos,
        "debug MDN runtime LLS loss preference");
  check(debug_report.mdn_runtime_lls.find("wave_pulse_index") !=
            std::string::npos,
        "debug MDN runtime LLS pulse index");
  check(
      std::filesystem::exists(debug_fixture.report.string() + ".nodelift.lls"),
      "debug NodeLift LLS sidecar exists");
  check(std::filesystem::exists(debug_fixture.report.string() +
                                ".representation.lls"),
        "debug representation LLS sidecar exists");
  check(std::filesystem::exists(debug_fixture.report.string() + ".mdn.lls"),
        "debug MDN LLS sidecar exists");

  auto bundle =
      builder::load_graph_first_config_bundle_from_config(fixture.config);
  builder::graph_first_pipeline_builder_options_t builder_options{};
  builder_options.force_rebuild_cache = false;
  builder_options.batch_size = 2;
  builder::graph_first_pipeline_builder_t<Kline> pipe(bundle, builder_options);
  auto source = pipe.make_graph_source();
  auto lifted_stream = pipe.make_node_lifted_stream(std::move(source));
  auto encoder = pipe.make_vicreg_encoder();
  auto representation_stream =
      pipe.make_node_representation_stream(std::move(lifted_stream), encoder);
  check(representation_stream.has_next(), "representation stream has a batch");
  auto node_batch = representation_stream.next();
  auto mdn_batch = cuwacunu::wikimyei::inference::expected_value::mdn::stream::
      make_mdn_input_batch(node_batch, pipe.mdn_adapter_options());
  auto restored_heads = pipe.make_mdn_heads(
      /*context_dim=*/mdn_batch.context.size(1),
      /*channel_count=*/mdn_batch.future.size(1),
      /*horizon_count=*/mdn_batch.future.size(2));
  auto restored_params = builder::graph_first_pipeline_builder_t<
      Kline>::collect_mdn_head_parameters(restored_heads);
  torch::optim::Adam restored_optimizer(
      restored_params,
      torch::optim::AdamOptions(bundle.mdn_training.learning_rate));
  auto identity = launcher::graph_first_inference_launcher_detail::
      checkpoint_identity_from_report(report);
  launcher::graph_first_inference_launcher_detail::load_mdn_checkpoint_file(
      report.checkpoint_path, restored_heads, &restored_optimizer, &identity);
  auto out = restored_heads.front()->forward_from_encoding(torch::zeros(
      {1, mdn_batch.context.size(1)}, mdn_batch.context.options()));
  check(torch::isfinite(out.log_pi).all().item<bool>(),
        "restored head log_pi finite");
  check(torch::isfinite(out.mu).all().item<bool>(), "restored head mu finite");
  check(torch::isfinite(out.sigma).all().item<bool>(),
        "restored head sigma finite");

  auto bad_node_identity = identity;
  std::swap(bad_node_identity.node_ids[0], bad_node_identity.node_ids[1]);
  expect_throws(
      [&] {
        launcher::graph_first_inference_launcher_detail::
            load_mdn_checkpoint_file(report.checkpoint_path, restored_heads,
                                     nullptr, &bad_node_identity);
      },
      "checkpoint load rejects node order mismatch");

  auto bad_graph_identity = identity;
  bad_graph_identity.graph_order_fingerprint += ".mismatch";
  expect_throws(
      [&] {
        launcher::graph_first_inference_launcher_detail::
            load_mdn_checkpoint_file(report.checkpoint_path, restored_heads,
                                     nullptr, &bad_graph_identity);
      },
      "checkpoint load rejects graph fingerprint mismatch");

  auto bad_target_identity = identity;
  bad_target_identity.target_coords.pop_back();
  expect_throws(
      [&] {
        launcher::graph_first_inference_launcher_detail::
            load_mdn_checkpoint_file(report.checkpoint_path, restored_heads,
                                     nullptr, &bad_target_identity);
      },
      "checkpoint load rejects target coordinate mismatch");

  auto bad_context_identity = identity;
  ++bad_context_identity.context_dim;
  expect_throws(
      [&] {
        launcher::graph_first_inference_launcher_detail::
            load_mdn_checkpoint_file(report.checkpoint_path, restored_heads,
                                     nullptr, &bad_context_identity);
      },
      "checkpoint load rejects context dimension mismatch");

  auto bad_mixture_identity = identity;
  ++bad_mixture_identity.mixture_count;
  expect_throws(
      [&] {
        launcher::graph_first_inference_launcher_detail::
            load_mdn_checkpoint_file(report.checkpoint_path, restored_heads,
                                     nullptr, &bad_mixture_identity);
      },
      "checkpoint load rejects mixture count mismatch");
}

void test_run_mode_evaluates_without_training() {
  torch::manual_seed(57);
  const auto fixture = make_config_fixture("run_eval", /*max_steps=*/1,
                                           /*checkpoint_every=*/0,
                                           /*mdn_batch_size=*/2,
                                           /*report_every=*/1, "run");
  auto graph_launcher = make_launcher(fixture, /*force_rebuild_cache=*/true,
                                      /*write_report=*/true);
  const auto report = graph_launcher.run();
  check(report.target_action == "run", "run mode target action");
  check(report.steps_attempted > 0, "run mode attempted steps");
  check(report.steps_completed > 0, "run mode completed evaluation");
  check(report.optimizer_steps == 0, "run mode has no optimizer steps");
  check(report.total_active_node_head_count > 0,
        "run mode active node heads observed");
  check(report.total_evaluated_node_head_count ==
            report.total_active_node_head_count,
        "run mode active heads are evaluated heads");
  check(report.total_trained_node_head_count == 0,
        "run mode does not train node heads");
  check(report.evaluated_row_count_by_node.size() == 3,
        "run mode evaluated row counts by node");
  check(report.evaluated_row_count_by_node[0] > 0,
        "run mode node evaluated support observed");
  check(report.trained_row_count_by_node.size() == 3 &&
            report.trained_row_count_by_node[0] == 0,
        "run mode per-node trained support remains zero");
  check(std::isfinite(report.last_loss), "run mode NLL finite");
  check(!report.checkpoint_written, "run mode writes no checkpoint");

  const auto report_text = read_text(fixture.report);
  check(report_text.find("target_action=run") != std::string::npos,
        "run mode report contains target action");
  check(report_text.find("total_evaluated_node_head_count=") !=
            std::string::npos,
        "run mode report contains evaluated head count");
  check(report_text.find("evaluated_row_count_by_node=") != std::string::npos,
        "run mode report contains per-node evaluated row counts");
}

void test_checkpoint_and_report_cadence() {
  torch::manual_seed(57);
  const auto fixture =
      make_config_fixture("cadence", /*max_steps=*/3,
                          /*checkpoint_every=*/2, /*mdn_batch_size=*/2,
                          /*report_every=*/2, "train");
  auto graph_launcher = make_launcher(fixture, /*force_rebuild_cache=*/true,
                                      /*write_report=*/true);
  const auto report = graph_launcher.run();
  check(report.steps_attempted > 0, "cadence attempted steps");
  check(report.optimizer_steps > 0, "cadence optimizer steps");
  check(report.checkpoint_write_count == report.optimizer_steps / 2,
        "checkpoint cadence count");
  if (report.checkpoint_write_count > 0) {
    check(report.last_checkpoint_optimizer_step ==
              report.checkpoint_write_count * 2,
          "last checkpoint optimizer cadence step");
  }
  const int64_t expected_report_writes =
      report.steps_attempted == 0
          ? 1
          : (report.steps_attempted / 2) +
                ((report.steps_attempted % 2) == 0 ? 0 : 1);
  check(report.report_write_count == expected_report_writes,
        "report cadence count includes final report only when needed");
  check(report.last_report_attempted_step == report.steps_attempted,
        "last report attempted step");

  const auto report_text = read_text(fixture.report);
  check(report_text.find("checkpoint_every=2") != std::string::npos,
        "cadence report contains checkpoint cadence");
  check(report_text.find("report_every=2") != std::string::npos,
        "cadence report contains report cadence");
  check(report_text.find("checkpoint_write_count=" +
                         std::to_string(report.checkpoint_write_count)) !=
            std::string::npos,
        "cadence report contains checkpoint count");
  check(report_text.find("report_write_count=" +
                         std::to_string(report.report_write_count)) !=
            std::string::npos,
        "cadence report contains report count");
}

void test_all_masked_targets_are_skipped() {
  torch::manual_seed(59);
  const auto fixture = make_config_fixture("masked", /*max_steps=*/2,
                                           /*checkpoint_every=*/0,
                                           /*mdn_batch_size=*/2,
                                           /*report_every=*/1, "train");
  auto graph_launcher = make_launcher(fixture, /*force_rebuild_cache=*/true,
                                      /*write_report=*/false);
  graph_launcher.set_force_empty_targets_for_test(true);
  const auto report = graph_launcher.run();
  check(report.all_target_masks_forced_empty, "forced-empty flag");
  check(report.steps_attempted > 0, "masked attempted steps");
  check(report.steps_completed == 0, "masked completed no steps");
  check(report.wave_pulses_skipped == report.steps_attempted,
        "masked skipped every wave pulse");
  check(report.optimizer_steps == 0, "masked no optimizer steps");
  check(report.skipped_batches == report.steps_attempted,
        "masked skipped every batch");
  check(report.total_valid_target_count == 0, "masked no valid targets");
}

void test_low_target_coverage_warning() {
  torch::manual_seed(61);
  const auto fixture = make_config_fixture("low_coverage", /*max_steps=*/1,
                                           /*checkpoint_every=*/0);
  auto bundle =
      builder::load_graph_first_config_bundle_from_config(fixture.config);
  builder::graph_first_pipeline_builder_options_t builder_options{};
  builder_options.force_rebuild_cache = true;
  builder_options.batch_size = 2;
  builder::graph_first_pipeline_builder_t<Kline> pipe(bundle, builder_options);
  auto source = pipe.make_graph_source();
  auto lifted_stream = pipe.make_node_lifted_stream(std::move(source));
  auto encoder = pipe.make_vicreg_encoder();
  auto representation_stream =
      pipe.make_node_representation_stream(std::move(lifted_stream), encoder);
  check(representation_stream.has_next(), "low coverage stream has a batch");
  auto node_batch = representation_stream.next();
  auto mdn_batch = cuwacunu::wikimyei::inference::expected_value::mdn::stream::
      make_mdn_input_batch(node_batch, pipe.mdn_adapter_options());

  mdn_batch.context_mask.fill_(true);
  mdn_batch.future_mask.fill_(false);
  mdn_batch.future_mask.index_put_({0, 0, 0}, true);

  auto heads = pipe.make_mdn_heads(
      /*context_dim=*/mdn_batch.context.size(1),
      /*channel_count=*/mdn_batch.future.size(1),
      /*horizon_count=*/mdn_batch.future.size(2));
  auto params = builder::graph_first_pipeline_builder_t<
      Kline>::collect_mdn_head_parameters(heads);
  torch::optim::Adam optimizer(
      params, torch::optim::AdamOptions(bundle.mdn_training.learning_rate));
  auto options = cuwacunu::jkimyei::training::mdn_training_options_from_specs(
      pipe.bundle().mdn_training, pipe.bundle().mdn);
  options.low_target_coverage_warning_fraction = 0.5;
  const auto report =
      launcher::train_mdn_batch(mdn_batch, heads, optimizer, options);
  check(report.trained, "low coverage still trains with one valid target");
  check(report.valid_target_count == 1, "low coverage valid target count");
  check(report.low_target_coverage_warning, "low coverage warning raised");
  check(report.trained_node_head_count == 1,
        "low coverage trains only one node head");
}

void test_model_state_inputs_do_not_change_contract_identity() {
  const auto fixture =
      make_config_fixture("contract_identity", /*max_steps=*/1,
                          /*checkpoint_every=*/0, /*mdn_batch_size=*/2);
  auto contract =
      builder::load_graph_first_config_bundle_from_config(fixture.config);

  const auto baseline =
      builder::graph_first_protocol_contract_fingerprint(contract);
  contract.mdn_training.input_representation_checkpoint_path = "/tmp/rep_a.pt";
  contract.mdn_training.input_mdn_checkpoint_path = "/tmp/mdn_a.pt";
  const auto with_model_state =
      builder::graph_first_protocol_contract_fingerprint(contract);
  check(baseline == with_model_state,
        "runtime model-state checkpoint inputs must not alter protocol "
        "contract fingerprint");

  contract.mdn_training.batch_size = 3;
  const auto with_contract_change =
      builder::graph_first_protocol_contract_fingerprint(contract);
  check(with_contract_change != baseline,
        "contract identity should still change for canonical training "
        "selectors such as batch size");
}

} // namespace

int main() {
  try {
    test_model_state_inputs_do_not_change_contract_identity();
    test_dry_run_launcher_report();
    test_batch_size_config_contract();
    test_config_backed_training_run_and_report_checkpoint();
    test_run_mode_evaluates_without_training();
    test_checkpoint_and_report_cadence();
    test_all_masked_targets_are_skipped();
    test_low_target_coverage_warning();
    std::cout
        << "[Jkimyei GraphFirstInferenceLauncher test] all checks passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "[Jkimyei GraphFirstInferenceLauncher test] failure: "
              << ex.what() << "\n";
    return 1;
  }
}
