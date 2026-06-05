#include "kikijyeba/protocol/pipeline_builder.h"

#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

namespace builder = cuwacunu::kikijyeba::protocol;
namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
namespace mdnstream =
    cuwacunu::wikimyei::inference::expected_value::mdn::stream;
namespace types = cuwacunu::ujcamei::source::registry::types;

namespace {

using Kline = types::kline_t;

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  static int counter = 0;
  auto dir = std::filesystem::temp_directory_path() /
             ("cuwacunu_protocol_pipeline_builder_" + label + "_" +
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
};

fixture_paths_t make_config_fixture(const std::string &label,
                                    bool discover_edges = false,
                                    const std::string &wave_mode = "run") {
  fixture_paths_t out{};
  out.dir = make_tmp_dir(label);
  const auto btc_csv = out.dir / "BTCUSDT.csv";
  const auto usdt_btc_csv = out.dir / "USDTBTC.csv";
  const auto eth_csv = out.dir / "ETHUSDT.csv";
  const auto usdt_eth_csv = out.dir / "USDTETH.csv";
  write_kline_csv(btc_csv, {1000, 1001, 1002, 1003, 1004, 1005}, 100.0);
  write_kline_csv(usdt_btc_csv, {1000, 1001, 1002, 1003, 1004, 1005}, 0.01);
  write_kline_csv(eth_csv, {1000, 1001, 1002, 1003, 1004, 1005}, 200.0);
  write_kline_csv(usdt_eth_csv, {1000, 1001, 1002, 1003, 1004, 1005}, 0.005);

  const auto sources_dsl = out.dir / "ujcamei.source.registry.dsl";
  const auto channels_dsl = out.dir / "ujcamei.source.retrieval.channels.dsl";
  const auto graph_dsl = out.dir / "kikijyeba.topology.graph.dsl";
  const auto wave_dsl = out.dir / "hero.runtime.wave.dsl";
  const auto vicreg_dsl = out.dir / "wikimyei.representation.vicreg.dsl";
  const auto vicreg_net = out.dir / "wikimyei.representation.vicreg.net";
  const auto channel_mdn_dsl =
      out.dir / "wikimyei.inference.expected_value.mdn.dsl";
  const auto channel_mdn_net =
      out.dir / "wikimyei.inference.expected_value.mdn.net";
  const auto vicreg_jkimyei =
      out.dir / "wikimyei.representation.vicreg.jkimyei";
  const auto channel_mdn_jkimyei =
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

  std::string graph_text =
      "GRAPH_POLICY {\n"
      "  EDGE_RESOLUTION_POLICY = " +
      std::string(discover_edges ? "edge_discovery" : "explicit_only") +
      ";\n"
      "  EDGE_SOURCE_KIND = real;\n"
      "  FETCH_MODE = parallel_by_edge;\n"
      "  MAX_FETCH_WORKERS = 0;\n"
      "  PARALLEL_MIN_WORK_ITEMS = 1;\n"
      "};\n"
      "/------------------------------------\\\n"
      "|  node_id  |  node_kind  |  active  |\n"
      "|------------------------------------|\n"
      "|   BTC     |    asset    |   true   |\n"
      "|   USDT    |    asset    |   true   |\n"
      "|   ETH     |    asset    |   true   |\n"
      "\\------------------------------------/\n";
  if (!discover_edges) {
    graph_text += "/-----------------------------------------------------"
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
                  "-------------------/\n";
  }
  write_text(graph_dsl, graph_text);
  const std::string wave_text =
      "WAVE_SETTINGS {\n"
      "  WAVE_ID = fixture_graph_first_pipeline;\n"
      "  TARGET = wikimyei.inference.expected_value.mdn;\n"
      "  MODE = " +
      wave_mode +
      ";\n"
      "  SOURCE_CURSOR_KIND = graph_anchor;\n"
      "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
      "  SOURCE_RANGE = all;\n"
      "};\n";
  write_text(wave_dsl, wave_text);

  write_text(vicreg_dsl, "VICREG {\n"
                         "  VERSION = wikimyei.representation.vicreg.v1;\n"
                         "  COMPONENT_ASSEMBLY_ID = vicreg_v1;\n"
                         "  INPUT_ROUTE = channel_node_stream;\n"
                         "  CHANNEL_COUNT = 1;\n"
                         "  HISTORY_LENGTH = 2;\n"
                         "  INPUT_WIDTH = 9;\n"
                         "  CELL_VALID_POLICY = required_features;\n"
                         "  REQUIRED_FEATURE_COORDS = 0,1,2,3;\n"
                         "  MIN_VALID_FRACTION = 1.0;\n"
                         "  USE_MISSINGNESS_INDICATORS = true;\n"
                         "  DTYPE = float32;\n"
                         "  DEVICE = cpu;\n"
                         "};\n");
  write_text(vicreg_net, "VICREG_NET {\n"
                         "  ENCODING_DIM = 4;\n"
                         "  FEATURE_HIDDEN_DIM = 8;\n"
                         "  TEMPORAL_DEPTH = 1;\n"
                         "  RECENCY_DECAY = 0.75;\n"
                         "  VICREG_PROJECTOR_DIM = 6;\n"
                         "  VICREG_PROJECTOR_HIDDEN_DIM = 8;\n"
                         "  VICREG_PROJECTOR_DEPTH = 1;\n"
                         "  VICREG_INVARIANCE_WEIGHT = 25.0;\n"
                         "  VICREG_VARIANCE_WEIGHT = 25.0;\n"
                         "  VICREG_COVARIANCE_WEIGHT = 1.0;\n"
                         "  VICREG_VARIANCE_FLOOR = 1.0;\n"
                         "  VICREG_EPS = 0.0001;\n"
                         "  GLOBAL_AUX_WEIGHT = 0.0;\n"
                         "  MIN_VALID_ROWS = 2;\n"
                         "  SKIP_NON_FINITE_LOSS = true;\n"
                         "  JITTER_STD = 0.01;\n"
                         "  FEATURE_DROPOUT_PROB = 0.0;\n"
                         "  HISTORY_DROPOUT_PROB = 0.0;\n"
                         "};\n");
  write_text(channel_mdn_dsl,
             "MDN {\n"
             "  VERSION = wikimyei.inference.expected_value.mdn.v1;\n"
             "  COMPONENT_ASSEMBLY_ID = mdn_v1;\n"
             "  INPUT_REPRESENTATION_ASSEMBLY_ID = vicreg_v1;\n"
             "  CONTEXT_MODE = channel_context_strict;\n"
             "  TARGET_DOMAIN = channel_node_future;\n"
             "  TARGET_COORDS = 0,1,2,3;\n"
             "  TARGET_MASK_POLICY = per_target_feature_valid;\n"
             "  ACTIVITY_TARGET = node_feature_support_mean;\n"
             "  SIGMA_MIN = 0.001;\n"
             "  SIGMA_MAX = 0.0;\n"
             "  EPS = 0.000001;\n"
             "};\n");
  write_text(channel_mdn_net, "MDN_NET {\n"
                              "  CHANNEL_COUNT = 1;\n"
                              "  FUTURE_HORIZON = 1;\n"
                              "  MIXTURE_COUNT = 2;\n"
                              "  HIDDEN_WIDTH = 8;\n"
                              "  RESIDUAL_DEPTH = 1;\n"
                              "  FEATURE_EMBEDDING_DIM = 2;\n"
                              "  CHANNEL_ADAPTER_RANK = 2;\n"
                              "  GLOBAL_CONTEXT_DIM = 0;\n"
                              "};\n");
  write_text(vicreg_jkimyei,
             "TRAINING {\n"
             "  VERSION = wikimyei.representation.vicreg.jkimyei.v1;\n"
             "  TRAINING_ID = fixture_vicreg;\n"
             "  TASK = vicreg_representation;\n"
             "  COMPONENT_ASSEMBLY_ID = vicreg_v1;\n"
             "  OPTIMIZER = adam;\n"
             "  LEARNING_RATE = 0.001;\n"
             "  MAX_STEPS = 0;\n"
             "  BATCH_SIZE = 2;\n"
             "  GRAD_CLIP_NORM = 0.0;\n"
             "  CHECKPOINT_EVERY = 1;\n"
             "  REPORT_EVERY = 1;\n"
             "  VALIDATION_EVERY = 0;\n"
             "  SEED = 17;\n"
             "  FREEZE_REPRESENTATION = false;\n"
             "};\n");
  write_text(channel_mdn_jkimyei,
             "TRAINING {\n"
             "  VERSION = wikimyei.inference.expected_value.mdn."
             "jkimyei.v1;\n"
             "  TRAINING_ID = fixture_mdn;\n"
             "  TASK = mdn_expected_value_inference;\n"
             "  COMPONENT_ASSEMBLY_ID = mdn_v1;\n"
             "  OPTIMIZER = adam;\n"
             "  LEARNING_RATE = 0.01;\n"
             "  MAX_STEPS = 0;\n"
             "  BATCH_SIZE = 2;\n"
             "  GRAD_CLIP_NORM = 0.0;\n"
             "  CHECKPOINT_EVERY = 1;\n"
             "  REPORT_EVERY = 1;\n"
             "  VALIDATION_EVERY = 0;\n"
             "  SEED = 31;\n"
             "  FREEZE_REPRESENTATION = true;\n"
             "  INPUT_REPRESENTATION_CHECKPOINT = ;\n"
             "  INPUT_MDN_CHECKPOINT = ;\n"
             "  ALLOW_UNTRAINED_REPRESENTATION = true;\n"
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
          "[HERO]\n"
          "runtime_wave_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/hero.runtime.wave.dsl.bnf\n"
          "runtime_wave_dsl_path = " +
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
          "wikimyei.inference.expected_value.mdn.net\n"
          "wikimyei_observer_belief_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.observer.belief.dsl.bnf\n"
          "wikimyei_observer_belief_dsl_path = "
          "/cuwacunu/src/config/wikimyei.observer.belief.dsl\n"
          "wikimyei_policy_portfolio_spot_distributional_utility_dsl_bnf_path"
          " = /cuwacunu/src/config/grammar/"
          "wikimyei.policy.portfolio.spot_distributional_utility.dsl.bnf\n"
          "wikimyei_policy_portfolio_spot_distributional_utility_dsl_path = "
          "/cuwacunu/src/config/"
          "wikimyei.policy.portfolio.spot_distributional_utility.dsl\n\n"
          "wikimyei_representation_vicreg_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.representation.vicreg.dsl.bnf\n"
          "wikimyei_representation_vicreg_dsl_path = " +
          vicreg_dsl.string() +
          "\n"
          "wikimyei_representation_vicreg_net_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.representation.vicreg.net.bnf\n"
          "wikimyei_representation_vicreg_net_path = " +
          vicreg_net.string() +
          "\n"
          "wikimyei_inference_expected_value_mdn_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.dsl.bnf\n"
          "wikimyei_inference_expected_value_mdn_dsl_path = " +
          channel_mdn_dsl.string() +
          "\n"
          "wikimyei_inference_expected_value_mdn_net_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.net.bnf\n"
          "wikimyei_inference_expected_value_mdn_net_path = " +
          channel_mdn_net.string() +
          "\n\n"
          "[JKIMYEI]\n"
          "wikimyei_representation_vicreg_jkimyei_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.representation.vicreg.jkimyei.bnf\n"
          "wikimyei_representation_vicreg_jkimyei_path = "
          "/cuwacunu/src/config/wikimyei.representation.vicreg.jkimyei\n"
          "wikimyei_inference_expected_value_mdn_jkimyei_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.jkimyei.bnf\n"
          "wikimyei_inference_expected_value_mdn_jkimyei_path = "
          "/cuwacunu/src/config/"
          "wikimyei.inference.expected_value.mdn.jkimyei\n"
          "wikimyei_representation_vicreg_jkimyei_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.representation.vicreg.jkimyei.bnf\n"
          "wikimyei_representation_vicreg_jkimyei_path = " +
          vicreg_jkimyei.string() +
          "\n"
          "wikimyei_inference_expected_value_mdn_jkimyei_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.jkimyei.bnf\n"
          "wikimyei_inference_expected_value_mdn_jkimyei_path = " +
          channel_mdn_jkimyei.string() + "\n");

  return out;
}

void test_default_channel_config_dry_run_report() {
  const auto bundle =
      builder::load_channel_graph_first_config_bundle_from_config(
          "/cuwacunu/src/config/.config");
  const auto contract =
      builder::load_channel_graph_first_protocol_contract_from_config(
          "/cuwacunu/src/config/.config");
  check(!builder::channel_graph_first_protocol_contract_fingerprint(contract)
             .empty(),
        "default channel protocol contract has fingerprint");
  check(builder::channel_graph_first_protocol_contract_token(contract).find(
            "kikijyeba.protocol.channel_graph_first.contract.v1/") == 0,
        "default channel protocol contract has token");
  check(bundle.vicreg.component_assembly_id == "vicreg_v1",
        "default channel VICReg component id");
  check(bundle.channel_mdn.input_representation_assembly_id ==
            bundle.vicreg.component_assembly_id,
        "default MDN points to channel VICReg");
  check(bundle.vicreg.channel_count ==
            builder::active_channel_count(bundle.source_dock),
        "default channel VICReg count matches source dock");
  check(bundle.vicreg.history_length ==
            builder::max_input_length(bundle.source_dock),
        "default channel VICReg Hx matches source dock");
  check(bundle.channel_mdn.future_horizon ==
            builder::max_future_length(bundle.source_dock),
        "default MDN Hf matches source dock");
  check(bundle.channel_mdn.feature_embedding_dim == 8 &&
            bundle.channel_mdn.channel_adapter_rank == 16,
        "default MDN v2 adapter/head dimensions decode");

  builder::graph_first_pipeline_builder_options_t options{};
  options.dry_run = true;
  builder::channel_graph_first_pipeline_builder_t<Kline> pipe(bundle, options);
  const auto report = pipe.dry_run_report();
  check(pipe.options().batch_size ==
            static_cast<std::size_t>(bundle.channel_mdn_training.batch_size),
        "default channel builder batch size derives from config");
  check(pipe.batch_size_source() == "derived",
        "default channel builder reports derived batch size");
  check(report.protocol_contract_token.find(
            "kikijyeba.protocol.channel_graph_first.contract.v1/") == 0,
        "channel dry-run reports channel protocol token");
  check(report.stream_plan.steps.size() == 4,
        "channel dry-run reports four stream plan steps");
  const bool default_uses_mtf_representation =
      builder::active_protocol_uses_mtf_jepa_mae_vicreg(bundle);
  const std::string expected_representation_family =
      default_uses_mtf_representation
          ? "wikimyei.representation.encoding.mtf_jepa_mae_vicreg"
          : "wikimyei.representation.encoding.vicreg";
  const std::string expected_representation_batch =
      default_uses_mtf_representation
          ? "mtf_jepa_mae_vicreg_representation_batch_t"
          : "channel_representation_batch_t";
  check(report.stream_plan.steps.at(2).component_family_id ==
            expected_representation_family,
        "channel stream plan includes active representation");
  check(report.stream_plan.steps.at(2).output_batch == expected_representation_batch,
        "channel stream plan exports active representation batch");
  check(report.stream_plan.steps.at(3).component_family_id ==
            "wikimyei.inference.expected_value.mdn",
        "channel stream plan includes MDN");
  check(report.stream_plan.steps.at(3).output_batch ==
            "channel_mdn_input_batch_t",
        "channel stream plan exports MDN input batch");
  check(report.head_policy == "per_channel_strict",
        "channel dry-run reports strict channel head policy");
  check(report.target_domain == "channel_node_future",
        "channel dry-run reports channel target domain");
  check(report.mask_profile == "required_features",
        "channel dry-run reports cell-valid policy");
  check(report.context_dim == bundle.vicreg.encoding_dim,
        "channel dry-run reports encoding dim");
  check(report.mixture_count == bundle.channel_mdn.mixture_count,
        "channel dry-run reports mixture count");
  check(report.dock_binding_warning_count == 0,
        "channel dry-run reports no dock binding warnings");
  std::cout << "[protocol_pipeline_builder channel dry-run] "
            << report.summary() << "\n";
}

void test_train_wave_defaults_to_random_source_order_in_pipeline() {
  const auto fixture =
      make_config_fixture("train_random_source_order", false, "train");
  const auto bundle =
      builder::load_channel_graph_first_config_bundle_from_config(
          fixture.config);
  check(bundle.wave_settings.source_order_policy ==
            cuwacunu::hero::runtime::settings::wave_source_order_policy_t::
                random_per_epoch,
        "train fixture defaults to random_per_epoch source order");
  check(!bundle.wave_settings.source_order_policy_explicit,
        "train fixture source order is implicit");

  builder::graph_first_pipeline_builder_options_t options{};
  options.dry_run = true;
  options.batch_size = 2;
  builder::channel_graph_first_pipeline_builder_t<Kline> pipe(bundle, options);
  const auto report = pipe.dry_run_report();
  check(report.wave_source_order_policy == "random_per_epoch",
        "pipeline dry-run reports train default random source order");
  check(!report.wave_source_order_policy_explicit,
        "pipeline dry-run reports train default source order as implicit");
  check(
      report.wave_source_order_warning_level == "none",
      "pipeline dry-run does not warn for implicit train random source order");
  check(report.wave_source_order_random_seed == 31,
        "pipeline dry-run seeds train random source order from MDN seed");
  check(report.wave_source_order_random_seed_source ==
            "channel_mdn_training.seed",
        "pipeline dry-run reports source-order seed source");
  check(report.summary().find("source_order=random_per_epoch") !=
            std::string::npos,
        "pipeline summary includes train random source order");
  check(report.summary().find("source_order_random_seed=31") !=
            std::string::npos,
        "pipeline summary includes source-order random seed");
}

void test_channel_contract_ignores_runtime_model_state_inputs() {
  const auto fixture = make_config_fixture("channel_contract_model_state");
  const auto base_contract =
      builder::load_channel_graph_first_protocol_contract_from_config(
          fixture.config);
  const auto base_fingerprint =
      builder::channel_graph_first_protocol_contract_fingerprint(base_contract);

  const auto channel_mdn_jkimyei =
      fixture.dir / "wikimyei.inference.expected_value.mdn.jkimyei";
  write_text(channel_mdn_jkimyei,
             "TRAINING {\n"
             "  VERSION = wikimyei.inference.expected_value.mdn."
             "jkimyei.v1;\n"
             "  TRAINING_ID = fixture_mdn;\n"
             "  TASK = mdn_expected_value_inference;\n"
             "  COMPONENT_ASSEMBLY_ID = mdn_v1;\n"
             "  OPTIMIZER = adam;\n"
             "  LEARNING_RATE = 0.01;\n"
             "  MAX_STEPS = 0;\n"
             "  BATCH_SIZE = 2;\n"
             "  GRAD_CLIP_NORM = 0.0;\n"
             "  CHECKPOINT_EVERY = 1;\n"
             "  REPORT_EVERY = 1;\n"
             "  VALIDATION_EVERY = 0;\n"
             "  SEED = 31;\n"
             "  FREEZE_REPRESENTATION = true;\n"
             "  INPUT_REPRESENTATION_CHECKPOINT = "
             "/tmp/runtime-evidence/representation.pt;\n"
             "  INPUT_MDN_CHECKPOINT = /tmp/runtime-evidence/mdn.pt;\n"
             "  ALLOW_UNTRAINED_REPRESENTATION = false;\n"
             "};\n");

  const auto model_state_contract =
      builder::load_channel_graph_first_protocol_contract_from_config(
          fixture.config);
  const auto model_state_fingerprint =
      builder::channel_graph_first_protocol_contract_fingerprint(
          model_state_contract);
  check(model_state_contract.channel_mdn_training
                .input_representation_checkpoint_path ==
            "/tmp/runtime-evidence/representation.pt",
        "channel contract still records representation checkpoint input");
  check(model_state_contract.channel_mdn_training.input_mdn_checkpoint_path ==
            "/tmp/runtime-evidence/mdn.pt",
        "channel contract still records MDN checkpoint input");
  check(
      !model_state_contract.channel_mdn_training.allow_untrained_representation,
      "channel contract still records model-state admission policy");
  check(base_fingerprint == model_state_fingerprint,
        "model-state inputs do not alter channel protocol contract identity");
}

void test_channel_config_backed_forward_nll_smoke() {
  torch::manual_seed(43);
  const auto fixture = make_config_fixture("channel_forward_nll");
  const auto bundle =
      builder::load_channel_graph_first_config_bundle_from_config(
          fixture.config);
  check(bundle.vicreg.channel_count == 1, "fixture channel VICReg count");
  check(bundle.vicreg.history_length == 2, "fixture channel VICReg Hx");
  check(bundle.channel_mdn.future_horizon == 1, "fixture MDN Hf");
  check(bundle.channel_mdn.feature_embedding_dim == 2 &&
            bundle.channel_mdn.channel_adapter_rank == 2,
        "fixture MDN v2 adapter/head dimensions");

  builder::graph_first_pipeline_builder_options_t options{};
  options.force_rebuild_cache = true;
  options.batch_size = 2;
  builder::channel_graph_first_pipeline_builder_t<Kline> pipe(bundle, options);
  const auto report = pipe.dry_run_report();
  check(report.stream_plan.steps.at(2).output_batch ==
            "channel_representation_batch_t",
        "fixture channel stream plan includes channel representation");
  check(report.stream_plan.steps.at(3).output_batch ==
            "channel_mdn_input_batch_t",
        "fixture channel stream plan includes MDN input");

  auto graph_source = pipe.make_graph_source();
  check(graph_source.size() >= 2, "fixture channel graph source has anchors");
  auto lifted_stream = pipe.make_node_lifted_stream(std::move(graph_source));
  auto encoder = pipe.make_vicreg_encoder();
  auto representation_stream = pipe.make_channel_representation_stream(
      std::move(lifted_stream), encoder);
  check(representation_stream.has_next(),
        "fixture channel representation stream has batch");
  auto channel_batch = representation_stream.next();
  check(channel_batch.node_encoding.sizes() ==
            torch::IntArrayRef({2, 3, 1, pipe.context_dim()}),
        "fixture channel representation shape");
  check(channel_batch.node_encoding_mask.sizes() ==
            torch::IntArrayRef({2, 3, 1}),
        "fixture channel representation mask shape");
  check(channel_batch.stream_report.identity.component_family_id ==
            "wikimyei.representation.encoding.vicreg",
        "fixture channel stream report uses channel VICReg family");
  check(torch::isfinite(channel_batch.node_encoding).all().item<bool>(),
        "fixture channel encoding finite");

  auto mdn_batch = mdnstream::make_channel_mdn_input_batch(
      channel_batch, pipe.channel_mdn_adapter_options());
  check(mdn_batch.context.sizes() ==
            torch::IntArrayRef({2, 3, 1, pipe.context_dim()}),
        "fixture MDN context shape");
  check(mdn_batch.future.sizes() == torch::IntArrayRef({2, 3, 1, 4}),
        "fixture MDN future shape");
  check(mdn_batch.representation_stream_report.cursor.batch_cursor_token ==
            channel_batch.stream_report.cursor.batch_cursor_token,
        "MDN adapter preserves representation stream cursor");

  auto mdn_model = pipe.make_channel_context_mdn(
      /*context_dim=*/mdn_batch.context.size(3),
      /*channel_count=*/mdn_batch.context.size(2),
      /*horizon_count=*/1);
  auto params = builder::channel_graph_first_pipeline_builder_t<
      Kline>::collect_channel_mdn_parameters(mdn_model);
  check(!params.empty(), "MDN exposes trainable parameters");

  const auto mask = mdn::combine_channel_context_and_future_mask(
      mdn_batch.context_mask, mdn_batch.future_mask);
  check(mask.sum().item<int64_t>() > 0, "fixture MDN has valid targets");
  const auto nll_options =
      mdn::channel_mdn_nll_options_from_spec(pipe.bundle().channel_mdn);
  auto out = mdn_model->forward(mdn_batch.context);
  check(torch::isfinite(out.log_pi).all().item<bool>(), "MDN log_pi finite");
  check(torch::isfinite(out.mu).all().item<bool>(), "MDN mu finite");
  check(torch::isfinite(out.sigma).all().item<bool>(), "MDN sigma finite");
  auto nll = cuwacunu::wikimyei::inference::expected_value::mdn::mdn_nll_map(
      out, mdn_batch.future, nll_options);
  check(torch::isfinite(nll).all().item<bool>(), "MDN NLL finite");
  auto loss = (nll * mask.to(nll.scalar_type())).sum() /
              mask.to(nll.scalar_type()).sum().clamp_min(1.0);
  check(std::isfinite(loss.item<double>()), "fixture channel total NLL finite");
}

void test_edge_discovery_policy() {
  const auto fixture = make_config_fixture("discover_edges",
                                           /*discover_edges=*/true);
  const auto bundle =
      builder::load_channel_graph_first_config_bundle_from_config(
          fixture.config);
  check(bundle.source_dock.edge_resolution_policy ==
            builder::graph_first_edge_resolution_policy_t::edge_discovery,
        "fixture uses edge_discovery policy");
  check(bundle.source_dock.edge_source_kind == "real",
        "fixture uses real edge source kind");
  check(bundle.source_dock.graph_edge_forms.size() == 4,
        "edge_discovery resolves four real edges");
  check(bundle.source_plan.market_graph.edge_ids ==
            std::vector<std::string>(
                {"BTCUSDT", "ETHUSDT", "USDTBTC", "USDTETH"}),
        "edge_discovery edge order is deterministic");
  check(bundle.source_plan.edge_instruments.size() == 4,
        "edge_discovery resolves edge instruments");
  check(bundle.source_resolution_report.edge_resolution_policy ==
            "edge_discovery",
        "resolver report records discovery policy");
  check(bundle.source_resolution_report.edge_source_kind == "real",
        "resolver report records edge source kind");
  check(bundle.source_resolution_report.fetch_mode == "parallel_by_edge",
        "resolver report records fetch mode");
  check(bundle.source_resolution_report.parallel_min_work_items == 1,
        "resolver report records fixture parallel threshold");
  check(bundle.source_resolution_report.possible_directed_edge_count == 6,
        "edge_discovery possible directed edge count");
  check(bundle.source_resolution_report.available_source_directed_edge_count ==
            4,
        "edge_discovery available source edge count");
  check(bundle.source_resolution_report.selected_edge_count == 4,
        "edge_discovery selected all discovered edges");
  check(bundle.source_resolution_report.available_unselected_edge_count == 0,
        "edge_discovery has no unselected available edges");
  check(bundle.source_resolution_report.selected_missing_source_edge_count == 0,
        "edge_discovery selected edges have source coverage");
  check(bundle.source_resolution_report.missing_directed_pair_count == 2,
        "edge_discovery records missing directed pairs");
  check(bundle.source_resolution_report.reverse_pair_count == 2,
        "edge_discovery reverse pair count");
  check(bundle.source_resolution_report.connected_component_count == 1,
        "edge_discovery connected component count");
  check(bundle.source_resolution_report.selected_cycle_dimension == 2,
        "edge_discovery selected cycle dimension");

  builder::graph_first_pipeline_builder_options_t options{};
  options.dry_run = true;
  options.batch_size = 2;
  builder::channel_graph_first_pipeline_builder_t<Kline> pipe(bundle, options);
  const auto report = pipe.dry_run_report();
  check(report.edge_resolution_policy == "edge_discovery",
        "dry-run reports discovery policy");
  check(report.edge_source_kind == "real", "dry-run reports edge source kind");
  check(report.fetch_mode == "parallel_by_edge", "dry-run reports fetch mode");
  check(report.edge_count == 4, "dry-run reports discovered edge count");
  check(report.edge_ids == std::vector<std::string>(
                               {"BTCUSDT", "ETHUSDT", "USDTBTC", "USDTETH"}),
        "dry-run reports discovered edge order");
}

} // namespace

int main() {
  try {
    test_default_channel_config_dry_run_report();
    test_train_wave_defaults_to_random_source_order_in_pipeline();
    test_channel_contract_ignores_runtime_model_state_inputs();
    test_edge_discovery_policy();
    test_channel_config_backed_forward_nll_smoke();
    std::cout << "[Jkimyei GraphFirstPipelineBuilder test] all checks passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "[Jkimyei GraphFirstPipelineBuilder test] failure: "
              << ex.what() << "\n";
    return 1;
  }
}
