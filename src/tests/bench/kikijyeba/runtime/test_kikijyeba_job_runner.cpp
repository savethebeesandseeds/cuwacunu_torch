#include "kikijyeba/runtime/job_runner.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>

namespace runtime = cuwacunu::kikijyeba::runtime;
namespace runtime_report = cuwacunu::kikijyeba::lattice::runtime_report;
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
             ("cuwacunu_job_runner_" + label + "_" +
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

std::string read_text(const std::filesystem::path &path) {
  std::ifstream in(path);
  check(in.is_open(), "failed to open text input");
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

struct fixture_paths_t {
  std::filesystem::path dir{};
  std::filesystem::path config{};
  std::filesystem::path wave_dsl{};
  std::filesystem::path vicreg_jkimyei{};
  std::filesystem::path channel_mdn_jkimyei{};
};

fixture_paths_t make_config_fixture(
    const std::string &label, const std::string &wave_range,
    const std::string &target = "wikimyei.inference.expected_value.mdn",
    const std::string &wave_mode = "run | debug") {
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
  out.wave_dsl = out.dir / "kikijyeba.settings.wave.dsl";
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
  out.vicreg_jkimyei = vicreg_jkimyei;
  out.channel_mdn_jkimyei = channel_mdn_jkimyei;
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

  write_text(out.wave_dsl, std::string("WAVE_SETTINGS {\n"
                                       "  WAVE_ID = runtime_test_wave;\n"
                                       "  TARGET = ") +
                               target +
                               ";\n"
                               "  MODE = " +
                               wave_mode +
                               ";\n"
                               "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                               "  SOURCE_CURSOR_SCOPE = wave_batch;\n" +
                               wave_range + "};\n");

  write_text(vicreg_dsl, "VICREG {\n"
                         "  VERSION = wikimyei.representation.vicreg.v1;\n"
                         "  COMPONENT_ID = vicreg_v1;\n"
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
                         "  GLOBAL_AUX_WEIGHT = 0.0;\n"
                         "};\n");
  write_text(channel_mdn_dsl,
             "MDN {\n"
             "  VERSION = wikimyei.inference.expected_value.mdn.v1;\n"
             "  COMPONENT_ID = channel_context_mdn_v1;\n"
             "  INPUT_REPRESENTATION_ID = vicreg_v1;\n"
             "  CONTEXT_MODE = channel_context_strict;\n"
             "  TARGET_DOMAIN = channel_node_future;\n"
             "  TARGET_COORDS = 0,1,2,3;\n"
             "  TARGET_MASK_POLICY = all_target_features_valid;\n"
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
                              "};\n");
  write_text(vicreg_jkimyei,
             "TRAINING {\n"
             "  VERSION = wikimyei.representation.vicreg.jkimyei.v1;\n"
             "  TRAINING_ID = runtime_vicreg;\n"
             "  TASK = vicreg_representation;\n"
             "  COMPONENT_ID = vicreg_v1;\n"
             "  OPTIMIZER = adam;\n"
             "  LEARNING_RATE = 0.001;\n"
             "  MAX_STEPS = 0;\n"
             "  BATCH_SIZE = 2;\n"
             "  GRAD_CLIP_NORM = 0.0;\n"
             "  CHECKPOINT_EVERY = 0;\n"
             "  REPORT_EVERY = 1;\n"
             "  VALIDATION_EVERY = 0;\n"
             "  SEED = 17;\n"
             "  FREEZE_REPRESENTATION = false;\n"
             "};\n");
  write_text(channel_mdn_jkimyei,
             "TRAINING {\n"
             "  VERSION = "
             "wikimyei.inference.expected_value.mdn.jkimyei.v1;\n"
             "  TRAINING_ID = runtime_channel_mdn;\n"
             "  TASK = mdn_expected_value_inference;\n"
             "  COMPONENT_ID = channel_context_mdn_v1;\n"
             "  OPTIMIZER = adam;\n"
             "  LEARNING_RATE = 0.01;\n"
             "  MAX_STEPS = 0;\n"
             "  BATCH_SIZE = 2;\n"
             "  GRAD_CLIP_NORM = 0.0;\n"
             "  CHECKPOINT_EVERY = 0;\n"
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
          "/cuwacunu/src/config/grammar/kikijyeba.topology.graph.dsl.bnf\n"
          "kikijyeba_topology_graph_dsl_path = " +
          graph_dsl.string() +
          "\n"
          "kikijyeba_settings_wave_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/kikijyeba.settings.wave.dsl.bnf\n"
          "kikijyeba_settings_wave_dsl_path = " +
          out.wave_dsl.string() +
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

void replace_once(std::string &text, const std::string &needle,
                  const std::string &replacement) {
  const auto pos = text.find(needle);
  check(pos != std::string::npos, "expected text replacement needle missing");
  text.replace(pos, needle.size(), replacement);
}

void configure_vicreg_train_smoke(const fixture_paths_t &fixture) {
  auto text = read_text(fixture.vicreg_jkimyei);
  replace_once(text, "  MAX_STEPS = 0;\n", "  MAX_STEPS = 1;\n");
  replace_once(text, "  CHECKPOINT_EVERY = 0;\n", "  CHECKPOINT_EVERY = 1;\n");
  write_text(fixture.vicreg_jkimyei, text);
}

void configure_channel_mdn_strict_smoke(
    const fixture_paths_t &fixture,
    const std::filesystem::path &representation_checkpoint) {
  auto text = read_text(fixture.channel_mdn_jkimyei);
  replace_once(text, "  MAX_STEPS = 0;\n", "  MAX_STEPS = 1;\n");
  replace_once(text, "  CHECKPOINT_EVERY = 0;\n", "  CHECKPOINT_EVERY = 1;\n");
  replace_once(text, "  INPUT_REPRESENTATION_CHECKPOINT = ;\n",
               "  INPUT_REPRESENTATION_CHECKPOINT = " +
                   representation_checkpoint.string() + ";\n");
  replace_once(text, "  ALLOW_UNTRAINED_REPRESENTATION = true;\n",
               "  ALLOW_UNTRAINED_REPRESENTATION = false;\n");
  write_text(fixture.channel_mdn_jkimyei, text);
}

void configure_channel_mdn_strict_eval(
    const fixture_paths_t &fixture,
    const std::filesystem::path &representation_checkpoint,
    const std::filesystem::path &mdn_checkpoint) {
  auto text = read_text(fixture.channel_mdn_jkimyei);
  replace_once(text, "  MAX_STEPS = 0;\n", "  MAX_STEPS = 1;\n");
  replace_once(text, "  INPUT_REPRESENTATION_CHECKPOINT = ;\n",
               "  INPUT_REPRESENTATION_CHECKPOINT = " +
                   representation_checkpoint.string() + ";\n");
  replace_once(text, "  INPUT_MDN_CHECKPOINT = ;\n",
               "  INPUT_MDN_CHECKPOINT = " + mdn_checkpoint.string() + ";\n");
  replace_once(text, "  ALLOW_UNTRAINED_REPRESENTATION = true;\n",
               "  ALLOW_UNTRAINED_REPRESENTATION = false;\n");
  write_text(fixture.channel_mdn_jkimyei, text);
}

void test_inference_dry_run_writes_manifest_and_state() {
  const auto fixture =
      make_config_fixture("inference_dry_run", "  SOURCE_RANGE = all;\n");
  const auto job_dir = fixture.dir / "job";
  runtime::job_runner_options_t options{};
  options.job_kind = runtime::runtime_job_kind_t::inference_mdn;
  options.dry_run = true;
  options.batch_size = 2;
  options.job_dir = job_dir;
  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), options);

  check(result.manifest.job_kind == "channel_inference_mdn",
        "manifest job kind");
  check(result.manifest.node_ids.size() == 3, "manifest node ids");
  check(result.manifest.edge_ids.size() == 4, "manifest edge ids");
  check(!result.manifest.graph_order_fingerprint.empty(),
        "manifest graph fingerprint");
  check(result.wave_plan.accepted_anchor_count > 0,
        "wave plan accepted anchors");
  check(result.wave_plan.candidate_anchor_count ==
            result.wave_plan.accepted_anchor_count,
        "healthy fixture has no rejected graph anchors");
  check(result.wave_plan.accepted_anchor_fraction == 1.0,
        "healthy fixture records full accepted-anchor fraction");
  check(result.wave_plan.anchor_domain_warning_level == "none",
        "healthy fixture has no anchor-domain warning");
  check(result.wave_plan.target_component ==
            "wikimyei.inference.expected_value.mdn",
        "wave target component is inference");
  check(result.wave_plan.action == "run", "wave action is run");
  check(result.wave_plan.resolved_anchor_index_begin == 0,
        "all range begins at zero");
  check(result.wave_plan.resolved_anchor_index_end ==
            result.wave_plan.accepted_anchor_count,
        "all range ends at accepted count");
  check(result.wave_plan.source_input_length == 2,
        "wave plan records source input length");
  check(result.wave_plan.source_future_length == 1,
        "wave plan records source future length");
  check(result.wave_plan.first_anchor_key == "1001",
        "all range records first accepted anchor key");
  check(result.wave_plan.last_anchor_key == "1004",
        "all range records last accepted anchor key");
  check(result.wave_plan.observed_source_row_begin == -1,
        "all range observed source-row footprint begins before first anchor");
  check(result.wave_plan.observed_source_row_end ==
            static_cast<int64_t>(result.wave_plan.resolved_anchor_index_end),
        "all range observed source-row footprint ends at range end");
  check(result.wave_plan.target_source_row_begin == 1,
        "all range target source-row footprint begins after first anchor");
  check(
      result.wave_plan.target_source_row_end ==
          static_cast<int64_t>(result.wave_plan.resolved_anchor_index_end + 1),
      "all range target source-row footprint includes future horizon");
  check(result.wave_plan.observed_source_key_begin == "1000",
        "all range observed source-key footprint begins at raw input key");
  check(result.wave_plan.observed_source_key_end == "1005",
        "all range observed source-key footprint is half-open");
  check(result.wave_plan.target_source_key_begin == "1002",
        "all range target source-key footprint begins after first anchor");
  check(result.wave_plan.target_source_key_end == "1006",
        "all range target source-key footprint is half-open");
  check(result.wave_plan.source_key_footprint_precision ==
            "graph_anchor_key_window_v1",
        "all range records source-key footprint precision");
  check(result.wave_plan.common_left_key == "1001" &&
            result.wave_plan.common_right_key == "1004",
        "all range records common graph-anchor domain keys");
  check(result.wave_plan.reference_left_key == "1001" &&
            result.wave_plan.reference_right_key == "1004",
        "all range records reference graph-anchor domain keys");
  check(result.state.status == "dry_run", "state dry-run status");
  check(std::filesystem::exists(job_dir / "job.manifest"),
        "manifest file exists");
  check(std::filesystem::exists(job_dir / "job.state"), "state file exists");
  const auto manifest_text = read_text(job_dir / "job.manifest");
  check(manifest_text.find("dock_binding_fingerprint=") != std::string::npos,
        "manifest records dock fingerprint");
  check(manifest_text.find("protocol_contract_fingerprint=") !=
            std::string::npos,
        "manifest records protocol contract fingerprint");
  check(manifest_text.find("protocol_contract_token=") != std::string::npos,
        "manifest records protocol contract token");
  check(manifest_text.find("job_kind=channel_inference_mdn") !=
            std::string::npos,
        "manifest records inference kind");
  check(manifest_text.find("target_component=wikimyei.inference.expected_"
                           "value.mdn") != std::string::npos,
        "manifest records inference target component");
  check(manifest_text.find("wave_action=run") != std::string::npos,
        "manifest records run action");
  check(manifest_text.find("source_input_length=2") != std::string::npos,
        "manifest records source input length");
  check(manifest_text.find("source_future_length=1") != std::string::npos,
        "manifest records source future length");
  check(manifest_text.find("candidate_anchor_count=4") != std::string::npos,
        "manifest records graph-anchor candidate count");
  check(manifest_text.find("accepted_anchor_fraction=1") != std::string::npos,
        "manifest records accepted-anchor fraction");
  check(manifest_text.find("anchor_domain_warning_level=none") !=
            std::string::npos,
        "manifest records anchor-domain warning level");
  check(manifest_text.find("observed_source_row_begin=-1") != std::string::npos,
        "manifest records observed source-row footprint");
  check(manifest_text.find("target_source_row_begin=1") != std::string::npos,
        "manifest records target source-row footprint");
  check(manifest_text.find("source_footprint_precision=graph_anchor_row_"
                           "index_v1") != std::string::npos,
        "manifest records source footprint precision");
  check(manifest_text.find("first_anchor_key=1001") != std::string::npos,
        "manifest records first resolved anchor key");
  check(manifest_text.find("last_anchor_key=1004") != std::string::npos,
        "manifest records last resolved anchor key");
  check(manifest_text.find("observed_source_key_begin=1000") !=
            std::string::npos,
        "manifest records observed source-key footprint");
  check(manifest_text.find("target_source_key_end=1006") != std::string::npos,
        "manifest records target source-key footprint");
  check(manifest_text.find("source_key_footprint_precision=graph_anchor_key_"
                           "window_v1") != std::string::npos,
        "manifest records source-key footprint precision");
  check(manifest_text.find("source_file_receipts=edge=BTCUSDT") !=
            std::string::npos,
        "manifest records active source-file receipts");
  check(manifest_text.find("|source=") != std::string::npos,
        "source-file receipts include source paths");
  check(manifest_text.find("common_left_key=1001") != std::string::npos,
        "manifest records common graph-anchor domain key");
  check(manifest_text.find("reference_right_key=1004") != std::string::npos,
        "manifest records reference graph-anchor domain key");
  check(manifest_text.find("mutated_components=\n") != std::string::npos,
        "run action does not mutate components");
  check(manifest_text.find("frozen_components=wikimyei.representation."
                           "encoding.vicreg") != std::string::npos,
        "manifest records frozen representation dependency");
  const auto state_text = read_text(job_dir / "job.state");
  check(state_text.find("status=dry_run") != std::string::npos,
        "state records dry-run status");
  check(state_text.find("target_component=wikimyei.inference.expected_value."
                        "mdn") != std::string::npos,
        "state records inference target component");
  check(state_text.find("wave_action=run") != std::string::npos,
        "state records run action");
  check(state_text.find("candidate_anchor_count=4") != std::string::npos,
        "state records graph-anchor candidate count");
  check(state_text.find("anchor_domain_warning_level=none") !=
            std::string::npos,
        "state records anchor-domain warning level");
  check(state_text.find("observed_source_row_begin=-1") != std::string::npos,
        "state records observed source-row footprint");
  check(state_text.find("wave_first_anchor_key=1001") != std::string::npos,
        "state records first resolved anchor key");
  check(state_text.find("observed_source_key_begin=1000") != std::string::npos,
        "state records observed source-key footprint");
  check(result.state.lattice_exposure_fact_written,
        "dry-run writes a lattice exposure sidecar for audit");
  check(!result.state.lattice_checkpoint_fact_written,
        "dry-run does not write a lattice checkpoint fact");
  check(std::filesystem::exists(result.state.lattice_exposure_fact_path),
        "dry-run exposure sidecar path exists");
  check(!std::filesystem::exists(job_dir / "lattice.checkpoint.fact"),
        "dry-run checkpoint sidecar is absent");
  const auto dry_run_exposure_text =
      read_text(result.state.lattice_exposure_fact_path);
  check(dry_run_exposure_text.find("job_status=dry_run") != std::string::npos,
        "dry-run exposure fact records dry-run status");
  check(dry_run_exposure_text.find(
            "coverage_precision=requested_range_untrusted_v0") !=
            std::string::npos,
        "dry-run exposure fact keeps requested range untrusted");
  check(dry_run_exposure_text.find("completed_anchor_begin=0") !=
                std::string::npos &&
            dry_run_exposure_text.find("completed_anchor_end=0") !=
                std::string::npos,
        "dry-run exposure fact has no completed anchor range");
}

void test_representation_dry_run_anchor_range() {
  const auto fixture = make_config_fixture("representation_range",
                                           "  SOURCE_RANGE = anchor_index;\n"
                                           "  ANCHOR_INDEX_BEGIN = 1;\n"
                                           "  ANCHOR_INDEX_END = 3;\n",
                                           "wikimyei.representation.encoding."
                                           "vicreg");
  runtime::job_runner_options_t options{};
  options.dry_run = true;
  options.batch_size = 2;
  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  check(result.manifest.job_kind == "channel_representation_vicreg",
        "representation manifest kind");
  check(result.wave_plan.target_component ==
            "wikimyei.representation.encoding.vicreg",
        "representation target component");
  check(result.wave_plan.resolved_anchor_index_begin == 1,
        "range begin resolved");
  check(result.wave_plan.resolved_anchor_index_end == 3, "range end resolved");
  check(result.wave_plan.resolved_anchor_count() == 2, "range anchor count");
  check(result.wave_plan.first_anchor_key == "1002",
        "ranged first anchor key is resolved from selected range");
  check(result.wave_plan.last_anchor_key == "1003",
        "ranged last anchor key is resolved from selected range");
  check(result.wave_plan.observed_source_row_begin == 0,
        "ranged observed source-row footprint begin");
  check(result.wave_plan.observed_source_row_end == 3,
        "ranged observed source-row footprint end");
  check(result.wave_plan.target_source_row_begin == 2,
        "ranged target source-row footprint begin");
  check(result.wave_plan.target_source_row_end == 4,
        "ranged target source-row footprint end");
  check(result.wave_plan.observed_source_key_begin == "1001",
        "ranged observed source-key footprint begin");
  check(result.wave_plan.observed_source_key_end == "1004",
        "ranged observed source-key footprint end");
  check(result.wave_plan.target_source_key_begin == "1003",
        "ranged target source-key footprint begin");
  check(result.wave_plan.target_source_key_end == "1005",
        "ranged target source-key footprint end");
  check(result.state.status == "dry_run", "representation dry-run status");
}

void test_train_mode_manifest_mutation_policy() {
  const auto fixture =
      make_config_fixture("train_mode_manifest", "  SOURCE_RANGE = all;\n",
                          "wikimyei.inference.expected_value.mdn", "train");
  runtime::job_runner_options_t options{};
  options.dry_run = true;
  options.batch_size = 2;
  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  check(result.manifest.wave_action == "train",
        "manifest records train action");
  check(result.manifest.mutated_components ==
            "wikimyei.inference.expected_value.mdn",
        "train mode mutates only MDN target");
  check(result.manifest.frozen_components ==
            "wikimyei.representation.encoding.vicreg",
        "MDN train mode freezes representation dependency");
  check(result.manifest.execution_chain.find(
            "wikimyei.representation.encoding.vicreg:run_frozen") !=
            std::string::npos,
        "execution chain records frozen upstream representation");
  check(result.state.wave_action == "train", "state records train action");
}

void test_channel_targets_dispatch_to_channel_runner() {
  const auto representation_fixture = make_config_fixture(
      "channel_representation_target", "  SOURCE_RANGE = all;\n",
      "wikimyei.representation.encoding.vicreg", "run");
  const auto representation_job_dir = representation_fixture.dir / "job";
  runtime::job_runner_options_t representation_options{};
  representation_options.dry_run = true;
  representation_options.batch_size = 2;
  representation_options.job_dir = representation_job_dir;
  const auto representation_result = runtime::run_graph_first_job<Kline>(
      representation_fixture.config.string(), representation_options);
  check(representation_result.manifest.job_kind ==
            "channel_representation_vicreg",
        "channel representation manifest kind");
  check(representation_result.wave_plan.target_component ==
            "wikimyei.representation.encoding.vicreg",
        "channel representation target component");
  check(representation_result.manifest.execution_chain.find(
            "wikimyei.representation.encoding.vicreg:run") != std::string::npos,
        "channel representation execution chain");
  check(representation_result.delegated_report_path.filename() ==
            "channel_representation.report",
        "channel representation delegated report path");
  check(std::filesystem::exists(representation_job_dir / "job.manifest"),
        "channel representation manifest exists");

  const auto inference_fixture =
      make_config_fixture("channel_inference_target", "  SOURCE_RANGE = all;\n",
                          "wikimyei.inference.expected_value.mdn", "train");
  const auto inference_job_dir = inference_fixture.dir / "job";
  runtime::job_runner_options_t inference_options{};
  inference_options.dry_run = true;
  inference_options.batch_size = 2;
  inference_options.job_dir = inference_job_dir;
  const auto inference_result = runtime::run_graph_first_job<Kline>(
      inference_fixture.config.string(), inference_options);
  check(inference_result.manifest.job_kind == "channel_inference_mdn",
        "channel inference manifest kind");
  check(inference_result.wave_plan.target_component ==
            "wikimyei.inference.expected_value.mdn",
        "channel inference target component");
  check(inference_result.manifest.mutated_components ==
            "wikimyei.inference.expected_value.mdn",
        "channel inference mutates only channel MDN");
  check(inference_result.manifest.frozen_components ==
            "wikimyei.representation.encoding.vicreg",
        "channel inference freezes channel representation");
  check(inference_result.manifest.execution_chain.find(
            "wikimyei.representation.encoding.vicreg:run_frozen") !=
            std::string::npos,
        "channel inference execution chain records frozen representation");
  check(inference_result.delegated_report_path.filename() ==
            "channel_inference.report",
        "channel inference delegated report path");
  check(std::filesystem::exists(inference_job_dir / "job.state"),
        "channel inference state exists");
}

void test_strict_channel_baseline_runs_through_runtime() {
  const auto representation_fixture = make_config_fixture(
      "strict_channel_baseline_representation", "  SOURCE_RANGE = all;\n",
      "wikimyei.representation.encoding.vicreg", "train");
  configure_vicreg_train_smoke(representation_fixture);
  const auto representation_job_dir = representation_fixture.dir / "job";
  runtime::job_runner_options_t representation_options{};
  representation_options.batch_size = 2;
  representation_options.job_dir = representation_job_dir;
  representation_options.force_rebuild_cache = true;
  representation_options.runtime_report_mode =
      runtime_report::runtime_report_mode_t::debug;
  const auto representation_result = runtime::run_graph_first_job<Kline>(
      representation_fixture.config.string(), representation_options);

  check(representation_result.manifest.job_kind ==
            "channel_representation_vicreg",
        "strict baseline representation job kind");
  check(representation_result.state.status == "completed",
        "strict baseline representation completed");
  check(representation_result.state.steps_completed > 0,
        "strict baseline representation completed steps");
  check(representation_result.state.optimizer_steps > 0,
        "strict baseline representation optimizer steps");
  check(representation_result.state.checkpoint_written,
        "strict baseline representation checkpoint written");
  check(std::filesystem::exists(representation_result.state.checkpoint_path),
        "strict baseline representation checkpoint exists");
  check(representation_result.state.lattice_exposure_fact_written,
        "strict baseline representation exposure fact written");
  check(representation_result.state.lattice_checkpoint_fact_written,
        "strict baseline representation checkpoint fact written");
  const auto representation_report =
      read_text(representation_result.delegated_report_path);
  check(representation_report.find("checkpoint_format="
                                   "torch_archive_vicreg_v1") !=
            std::string::npos,
        "strict baseline representation checkpoint format");
  check(representation_report.find("representation_contract="
                                   "graph_order.channel_node_representation."
                                   "v1") != std::string::npos,
        "strict baseline representation report records channel contract");
  check(representation_report.find("primary_value_shape=[B,N,C,De]") !=
            std::string::npos,
        "strict baseline representation report records channel shape");
  check(representation_report.find("representation_effective_rank=") !=
            std::string::npos,
        "strict baseline representation report records effective rank");
  check(representation_report.find("representation_condition_number=") !=
            std::string::npos,
        "strict baseline representation report records condition number");
  check(representation_report.find(
            "representation_effective_rank_per_channel=") != std::string::npos,
        "strict baseline representation report records per-channel rank");
  check(representation_report.find(
            "representation_isotropy_score_per_channel=") != std::string::npos,
        "strict baseline representation report records per-channel isotropy");
  check(representation_report.find("runtime_report_mode=debug") !=
            std::string::npos,
        "strict baseline representation report records debug runtime mode");
  check(std::filesystem::exists(
            representation_result.delegated_report_path.string() +
            ".nodelift.lls"),
        "strict baseline representation writes NodeLift LLS sidecar");
  check(std::filesystem::exists(
            representation_result.delegated_report_path.string() +
            ".representation_training.lls"),
        "strict baseline representation writes representation training LLS "
        "sidecar");
  const auto representation_training_lls =
      read_text(representation_result.delegated_report_path.string() +
                ".representation_training.lls");
  check(representation_training_lls.find(
            "wikimyei.representation.vicreg.training.runtime.v1") !=
            std::string::npos,
        "strict baseline representation LLS carries channel VICReg training "
        "schema");
  check(representation_training_lls.find("view1_valid_feature_fraction") !=
            std::string::npos,
        "strict baseline representation LLS carries augmented view support");
  const auto representation_exposure_fact =
      read_text(representation_result.state.lattice_exposure_fact_path);
  check(representation_exposure_fact.find(
            "representation_contract=graph_order.channel_node_"
            "representation.v1") != std::string::npos,
        "strict baseline representation exposure fact carries channel "
        "contract");
  check(representation_exposure_fact.find(
            "representation_value_shape=[B,N,C,De]") != std::string::npos,
        "strict baseline representation exposure fact carries channel shape");
  check(representation_exposure_fact.find(
            "temporal_reduction=mask_aware_anchor_weighted") !=
            std::string::npos,
        "strict baseline representation exposure fact carries temporal "
        "reduction policy");
  check(representation_exposure_fact.find("representation_effective_rank=") !=
            std::string::npos,
        "strict baseline representation exposure fact carries effective "
        "rank");
  check(representation_exposure_fact.find("representation_condition_number=") !=
            std::string::npos,
        "strict baseline representation exposure fact carries condition "
        "number");

  const auto inference_fixture = make_config_fixture(
      "strict_channel_baseline_inference", "  SOURCE_RANGE = all;\n",
      "wikimyei.inference.expected_value.mdn", "train");
  configure_channel_mdn_strict_smoke(
      inference_fixture, representation_result.state.checkpoint_path);
  const auto inference_job_dir = inference_fixture.dir / "job";
  runtime::job_runner_options_t inference_options{};
  inference_options.batch_size = 2;
  inference_options.job_dir = inference_job_dir;
  inference_options.force_rebuild_cache = true;
  inference_options.runtime_report_mode =
      runtime_report::runtime_report_mode_t::debug;
  const auto inference_result = runtime::run_graph_first_job<Kline>(
      inference_fixture.config.string(), inference_options);

  check(inference_result.manifest.job_kind == "channel_inference_mdn",
        "strict baseline inference job kind");
  check(inference_result.manifest.input_representation_checkpoint_path ==
            representation_result.state.checkpoint_path,
        "strict baseline manifest records representation checkpoint input");
  check(inference_result.state.status == "completed",
        "strict baseline inference completed");
  check(inference_result.state.steps_completed > 0,
        "strict baseline inference completed steps");
  check(inference_result.state.optimizer_steps > 0,
        "strict baseline inference optimizer steps");
  check(inference_result.state.checkpoint_written,
        "strict baseline inference checkpoint written");
  check(std::filesystem::exists(inference_result.state.checkpoint_path),
        "strict baseline inference checkpoint exists");
  check(inference_result.state.lattice_exposure_fact_written,
        "strict baseline inference exposure fact written");
  check(inference_result.state.lattice_checkpoint_fact_written,
        "strict baseline inference checkpoint fact written");
  const auto inference_report =
      read_text(inference_result.delegated_report_path);
  check(inference_report.find("context_mode=channel_context_strict") !=
            std::string::npos,
        "strict baseline inference uses strict channel context");
  check(inference_report.find("context_contract="
                              "graph_order.channel_node_representation.v1") !=
            std::string::npos,
        "strict baseline inference records channel context contract");
  check(inference_report.find("context_value_shape=[B_node,C,De]") !=
            std::string::npos,
        "strict baseline inference records channel context shape");
  check(inference_report.find("output_contract="
                              "graph_order.channel_node_future_distribution."
                              "v1") != std::string::npos,
        "strict baseline inference records channel output contract");
  check(inference_report.find("representation_checkpoint_loaded=true") !=
            std::string::npos,
        "strict baseline inference loaded representation checkpoint");
  check(inference_report.find("allow_untrained_representation=false") !=
            std::string::npos,
        "strict baseline inference forbids untrained representation");
  check(inference_report.find("checkpoint_format="
                              "torch_archive_channel_mdn_v1") !=
            std::string::npos,
        "strict baseline inference checkpoint format");
  check(inference_report.find("mean_nll_per_channel=") != std::string::npos,
        "strict baseline inference emits per-channel NLL");
  check(inference_report.find("mean_nll_per_horizon=") != std::string::npos,
        "strict baseline inference emits per-horizon NLL");
  check(inference_report.find("mean_mixture_usage=") != std::string::npos,
        "strict baseline inference emits mixture usage");
  check(inference_report.find("mean_sigma_mean_valid=") != std::string::npos,
        "strict baseline inference emits masked sigma mean");
  check(inference_report.find("min_sigma_min_valid=") != std::string::npos,
        "strict baseline inference emits masked sigma min");
  check(inference_report.find("max_sigma_max_valid=") != std::string::npos,
        "strict baseline inference emits masked sigma max");
  check(inference_report.find("max_grad_norm=") != std::string::npos,
        "strict baseline inference emits gradient norm");
  check(inference_report.find("runtime_report_mode=debug") != std::string::npos,
        "strict baseline inference report records debug runtime mode");
  check(std::filesystem::exists(
            inference_result.delegated_report_path.string() + ".nodelift.lls"),
        "strict baseline inference writes NodeLift LLS sidecar");
  check(
      std::filesystem::exists(inference_result.delegated_report_path.string() +
                              ".representation.lls"),
      "strict baseline inference writes representation LLS sidecar");
  check(std::filesystem::exists(
            inference_result.delegated_report_path.string() + ".mdn.lls"),
        "strict baseline inference writes channel MDN LLS sidecar");
  const auto channel_mdn_lls =
      read_text(inference_result.delegated_report_path.string() + ".mdn.lls");
  check(channel_mdn_lls.find(
            "wikimyei.inference.expected_value.mdn.runtime.v1") !=
            std::string::npos,
        "strict baseline inference LLS carries channel MDN schema");
  check(channel_mdn_lls.find("context_mask_fraction") != std::string::npos &&
            channel_mdn_lls.find("future_mask_fraction") != std::string::npos,
        "strict baseline inference LLS carries context/future mask fractions");
  check(channel_mdn_lls.find("sigma_mean_valid") != std::string::npos,
        "strict baseline inference LLS carries masked sigma mean");
  const auto inference_exposure_fact =
      read_text(inference_result.state.lattice_exposure_fact_path);
  check(inference_exposure_fact.find("target_component=wikimyei.inference."
                                     "expected_value.mdn") != std::string::npos,
        "strict baseline exposure fact keeps MDN component");
  check(inference_exposure_fact.find("input_representation_id="
                                     "vicreg_v1") != std::string::npos,
        "strict baseline exposure fact carries input representation id");
  check(inference_exposure_fact.find("context_mode=channel_context_strict") !=
            std::string::npos,
        "strict baseline exposure fact carries strict context mode");
  check(inference_exposure_fact.find("context_contract=graph_order."
                                     "channel_node_representation.v1") !=
            std::string::npos,
        "strict baseline exposure fact carries channel context contract");
  check(inference_exposure_fact.find("context_value_shape=[B_node,C,De]") !=
            std::string::npos,
        "strict baseline exposure fact carries channel context shape");
  check(inference_exposure_fact.find("output_contract=graph_order."
                                     "channel_node_future_distribution.v1") !=
            std::string::npos,
        "strict baseline exposure fact carries channel output contract");
  const std::string assembly_key = "component_assembly_fingerprint=";
  const auto assembly_pos = inference_exposure_fact.find(assembly_key);
  check(assembly_pos != std::string::npos,
        "strict baseline exposure fact emits assembly fingerprint field");
  const auto assembly_end = inference_exposure_fact.find('\n', assembly_pos);
  check(assembly_end != std::string::npos &&
            assembly_end > assembly_pos + assembly_key.size(),
        "strict baseline exposure fact carries non-empty assembly fingerprint");
  check(inference_exposure_fact.find("inference_health_available=true") !=
            std::string::npos,
        "strict baseline exposure fact marks inference health available");
  check(inference_exposure_fact.find("mean_nll_per_channel=") !=
            std::string::npos,
        "strict baseline exposure fact carries per-channel NLL");
  check(inference_exposure_fact.find("mean_nll_per_horizon=") !=
            std::string::npos,
        "strict baseline exposure fact carries per-horizon NLL");
  check(inference_exposure_fact.find("mean_mixture_usage=") !=
            std::string::npos,
        "strict baseline exposure fact carries mixture usage");

  const auto missing_mdn_eval_fixture =
      make_config_fixture("strict_channel_eval_missing_mdn_checkpoint",
                          "  SOURCE_RANGE = anchor_index;\n"
                          "  ANCHOR_INDEX_BEGIN = 1;\n"
                          "  ANCHOR_INDEX_END = 3;\n",
                          "wikimyei.inference.expected_value.mdn", "run");
  configure_channel_mdn_strict_smoke(
      missing_mdn_eval_fixture, representation_result.state.checkpoint_path);
  const auto missing_mdn_eval_job_dir = missing_mdn_eval_fixture.dir / "job";
  runtime::job_runner_options_t missing_mdn_eval_options{};
  missing_mdn_eval_options.batch_size = 2;
  missing_mdn_eval_options.job_dir = missing_mdn_eval_job_dir;
  missing_mdn_eval_options.force_rebuild_cache = true;
  bool missing_mdn_eval_threw = false;
  try {
    (void)runtime::run_graph_first_job<Kline>(
        missing_mdn_eval_fixture.config.string(), missing_mdn_eval_options);
  } catch (const std::exception &ex) {
    missing_mdn_eval_threw = std::string(ex.what()).find(
                                 "INPUT_MDN_CHECKPOINT") != std::string::npos;
  }
  check(missing_mdn_eval_threw, "strict baseline eval requires MDN checkpoint");
  const auto missing_mdn_eval_state =
      read_text(missing_mdn_eval_job_dir / "job.state");
  check(missing_mdn_eval_state.find("status=failed") != std::string::npos,
        "strict baseline missing-MDN eval writes failed state");
  check(missing_mdn_eval_state.find("INPUT_MDN_CHECKPOINT") !=
            std::string::npos,
        "strict baseline missing-MDN eval state records checkpoint error");
  check(!std::filesystem::exists(missing_mdn_eval_job_dir /
                                 "channel_inference.report"),
        "strict baseline missing-MDN eval does not delegate report");
  check(!std::filesystem::exists(missing_mdn_eval_job_dir /
                                 "lattice.exposure.fact"),
        "strict baseline missing-MDN eval does not write exposure fact");

  const auto eval_fixture =
      make_config_fixture("strict_channel_baseline_eval",
                          "  SOURCE_RANGE = anchor_index;\n"
                          "  ANCHOR_INDEX_BEGIN = 1;\n"
                          "  ANCHOR_INDEX_END = 3;\n",
                          "wikimyei.inference.expected_value.mdn", "run");
  configure_channel_mdn_strict_eval(eval_fixture,
                                    representation_result.state.checkpoint_path,
                                    inference_result.state.checkpoint_path);
  const auto eval_job_dir = eval_fixture.dir / "job";
  runtime::job_runner_options_t eval_options{};
  eval_options.batch_size = 2;
  eval_options.job_dir = eval_job_dir;
  eval_options.force_rebuild_cache = true;
  const auto eval_result = runtime::run_graph_first_job<Kline>(
      eval_fixture.config.string(), eval_options);

  check(eval_result.manifest.job_kind == "channel_inference_mdn",
        "strict baseline eval job kind");
  check(eval_result.manifest.wave_action == "run",
        "strict baseline eval uses run mode");
  check(eval_result.manifest.mutated_components.empty(),
        "strict baseline eval does not mutate model components");
  check(eval_result.manifest.frozen_components ==
            "wikimyei.representation.encoding.vicreg",
        "strict baseline eval freezes channel representation");
  check(eval_result.manifest.input_representation_checkpoint_path ==
            representation_result.state.checkpoint_path,
        "strict baseline eval manifest records representation checkpoint");
  check(eval_result.manifest.input_mdn_checkpoint_path ==
            inference_result.state.checkpoint_path,
        "strict baseline eval manifest records MDN checkpoint");
  check(eval_result.wave_plan.resolved_anchor_index_begin == 1 &&
            eval_result.wave_plan.resolved_anchor_index_end == 3,
        "strict baseline eval uses configured anchor range");
  check(eval_result.state.status == "completed",
        "strict baseline eval completed");
  check(eval_result.state.steps_completed > 0,
        "strict baseline eval completed steps");
  check(eval_result.state.optimizer_steps == 0,
        "strict baseline eval has no optimizer steps");
  check(!eval_result.state.checkpoint_written,
        "strict baseline eval does not write a new checkpoint");
  check(eval_result.state.lattice_exposure_fact_written,
        "strict baseline eval exposure fact written");
  check(!eval_result.state.lattice_checkpoint_fact_written,
        "strict baseline eval does not write a checkpoint fact");
  const auto eval_report = read_text(eval_result.delegated_report_path);
  check(eval_report.find("representation_checkpoint_loaded=true") !=
            std::string::npos,
        "strict baseline eval loaded representation checkpoint");
  check(eval_report.find("mdn_checkpoint_loaded=true") != std::string::npos,
        "strict baseline eval loaded MDN checkpoint");
  check(eval_report.find("context_mode=channel_context_strict") !=
            std::string::npos,
        "strict baseline eval uses strict channel context");
  check(eval_report.find("output_contract="
                         "graph_order.channel_node_future_distribution.v1") !=
            std::string::npos,
        "strict baseline eval records channel output contract");
  check(eval_report.find("mean_nll_per_channel=") != std::string::npos,
        "strict baseline eval emits per-channel NLL");
  check(eval_report.find("mean_sigma_mean_valid=") != std::string::npos,
        "strict baseline eval emits masked sigma mean");
  const auto eval_exposure_fact =
      read_text(eval_result.state.lattice_exposure_fact_path);
  check(eval_exposure_fact.find("wave_action=run") != std::string::npos,
        "strict baseline eval exposure fact records run action");
  check(eval_exposure_fact.find("mutated_component=false") != std::string::npos,
        "strict baseline eval exposure fact is non-mutating");
  check(eval_exposure_fact.find("use_evaluation_metric=true") !=
            std::string::npos,
        "strict baseline eval exposure fact carries evaluation metric use");
  check(eval_exposure_fact.find("context_contract=graph_order."
                                "channel_node_representation.v1") !=
            std::string::npos,
        "strict baseline eval exposure fact carries channel context contract");
  check(eval_exposure_fact.find("output_contract=graph_order."
                                "channel_node_future_distribution.v1") !=
            std::string::npos,
        "strict baseline eval exposure fact carries channel output contract");
  check(eval_exposure_fact.find(representation_result.state.checkpoint_path) !=
            std::string::npos,
        "strict baseline eval exposure fact carries representation checkpoint "
        "input");
  check(eval_exposure_fact.find(inference_result.state.checkpoint_path) !=
            std::string::npos,
        "strict baseline eval exposure fact carries MDN checkpoint input");
}

void test_invalid_wave_range_fails_before_launch() {
  const auto fixture =
      make_config_fixture("invalid_range", "  SOURCE_RANGE = anchor_index;\n"
                                           "  ANCHOR_INDEX_BEGIN = 0;\n"
                                           "  ANCHOR_INDEX_END = 200;\n");
  runtime::job_runner_options_t options{};
  options.job_kind = runtime::runtime_job_kind_t::inference_mdn;
  options.dry_run = true;
  options.batch_size = 2;
  bool threw = false;
  try {
    (void)runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  } catch (const std::exception &ex) {
    threw = std::string(ex.what()).find(
                "exceeds accepted graph anchor domain") != std::string::npos;
  }
  check(threw, "invalid wave range fails with domain error");
}

void test_invalid_wave_mode_fails() {
  const auto fixture = make_config_fixture(
      "invalid_mode", "  SOURCE_RANGE = all;\n",
      "wikimyei.inference.expected_value.mdn", "run | train");
  runtime::job_runner_options_t options{};
  options.dry_run = true;
  bool threw = false;
  try {
    (void)runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  } catch (const std::exception &ex) {
    threw = std::string(ex.what()).find("exactly one primary action") !=
            std::string::npos;
  }
  check(threw, "run and train are mutually exclusive wave actions");
}

void test_resume_and_default_job_dir() {
  const auto fixture =
      make_config_fixture("fail_fast", "  SOURCE_RANGE = all;\n");
  runtime::job_runner_options_t resume_options{};
  resume_options.resume = true;
  bool resume_threw = false;
  try {
    (void)runtime::run_graph_first_job<Kline>(fixture.config.string(),
                                              resume_options);
  } catch (const std::exception &ex) {
    resume_threw = std::string(ex.what()).find("resume is not implemented") !=
                   std::string::npos;
  }
  check(resume_threw, "resume fails fast");

  runtime::job_runner_options_t run_options{};
  run_options.dry_run = true;
  const auto previous_cwd = std::filesystem::current_path();
  const auto isolated_cwd = fixture.dir / "isolated_cwd";
  std::filesystem::create_directories(isolated_cwd);
  std::filesystem::current_path(isolated_cwd);
  runtime::job_run_result_t result{};
  try {
    result = runtime::run_graph_first_job<Kline>(fixture.config.string(),
                                                 run_options);
    std::filesystem::current_path(previous_cwd);
  } catch (...) {
    std::filesystem::current_path(previous_cwd);
    throw;
  }
  check(!result.manifest_path.empty(), "default job dir writes manifest path");
  check(result.manifest_path.string().find(".runtime/cuwacunu_exec/") !=
            std::string::npos,
        "default job dir is under .runtime/cuwacunu_exec");
  check(std::filesystem::exists(result.manifest_path),
        "default job dir manifest exists");
}

} // namespace

int main() {
  try {
    test_inference_dry_run_writes_manifest_and_state();
    test_representation_dry_run_anchor_range();
    test_train_mode_manifest_mutation_policy();
    test_channel_targets_dispatch_to_channel_runner();
    test_strict_channel_baseline_runs_through_runtime();
    test_invalid_wave_range_fails_before_launch();
    test_invalid_wave_mode_fails();
    test_resume_and_default_job_dir();
    std::cout << "[Kikijyeba JobRunner test] all checks passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "[Kikijyeba JobRunner test] failure: " << ex.what() << "\n";
    return 1;
  }
}
