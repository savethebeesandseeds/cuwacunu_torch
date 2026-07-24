#include "jkimyei/training/inference/channel_graph_first_inference_launcher.h"
#include "jkimyei/training/representation/channel_graph_first_representation_launcher.h"

#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

namespace protocol = cuwacunu::kikijyeba::protocol;
namespace inference_launcher = cuwacunu::jkimyei::training::inference;
namespace representation_launcher = cuwacunu::jkimyei::training::representation;
namespace runtime_report = cuwacunu::hero::lattice::runtime_report;
namespace types = cuwacunu::ujcamei::source::registry::types;
namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
namespace mdn_stream =
    cuwacunu::wikimyei::inference::expected_value::mdn::stream;
namespace vicreg_stream =
    cuwacunu::wikimyei::representation::encoding::vicreg::stream;

namespace {

using Kline = types::kline_t;

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void check_close(double actual, double expected, double tolerance,
                 const std::string &message) {
  if (!std::isfinite(actual) || !std::isfinite(expected) ||
      std::fabs(actual - expected) > tolerance) {
    throw std::runtime_error(message + ": actual=" + std::to_string(actual) +
                             " expected=" + std::to_string(expected));
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
             ("cuwacunu_channel_graph_first_launchers_" + label + "_" +
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
  std::filesystem::path vicreg_dsl{};
  std::filesystem::path vicreg_net{};
  std::filesystem::path channel_mdn_dsl{};
  std::filesystem::path channel_mdn_net{};
  std::filesystem::path vicreg_jkimyei{};
  std::filesystem::path channel_mdn_jkimyei{};
};

fixture_paths_t make_config_fixture(const std::string &label,
                                    int64_t max_steps = 2,
                                    int64_t checkpoint_every = 1,
                                    int64_t batch_size = 2,
                                    int64_t report_every = 1,
                                    const std::string &wave_mode = "train") {
  fixture_paths_t out{};
  out.dir = make_tmp_dir(label);
  out.report = out.dir / "channel_graph_first.report";
  const auto btc_csv = out.dir / "BTCUSDT.csv";
  const auto usdt_btc_csv = out.dir / "USDTBTC.csv";
  const auto eth_csv = out.dir / "ETHUSDT.csv";
  const auto usdt_eth_csv = out.dir / "USDTETH.csv";
  const std::vector<types::ms_t> keys{1000, 1001, 1002, 1003,
                                      1004, 1005, 1006, 1007};
  write_kline_csv(btc_csv, keys, 100.0);
  write_kline_csv(usdt_btc_csv, keys, 0.01);
  write_kline_csv(eth_csv, keys, 200.0);
  write_kline_csv(usdt_eth_csv, keys, 0.005);

  const auto sources_dsl = out.dir / "ujcamei.source.registry.dsl";
  const auto channels_dsl = out.dir / "ujcamei.source.retrieval.channels.dsl";
  const auto graph_dsl = out.dir / "kikijyeba.topology.graph.dsl";
  const auto protocol_dsl = out.dir / "kikijyeba.protocol.dsl";
  const auto cursor_dsl = out.dir / "ujcamei.source.cursor.dsl";
  const auto source_splits_dsl = out.dir / "ujcamei.source.splits.dsl";
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
  out.vicreg_dsl = vicreg_dsl;
  out.vicreg_net = vicreg_net;
  out.channel_mdn_dsl = channel_mdn_dsl;
  out.channel_mdn_net = channel_mdn_net;
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
                         "  WAVE_ID = cwu_01v_channel_smoke;\n"
                         "  PROTOCOL = cwu_01v;\n"
                         "  TARGET = wikimyei.inference.expected_value;\n"
                         "  MODE = ") +
                 wave_mode +
                 ";\n"
                 "  SOURCE_CURSOR_ID = smoke_cursor;\n"
                 "};\n");
  write_text(cursor_dsl, "UJCAMEI_SOURCE_CURSOR {\n"
                         "  CURSOR_ID = smoke_cursor;\n"
                         "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                         "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
                         "  SOURCE_RANGE = all;\n"
                         "};\n");
  write_text(source_splits_dsl, "UJCAMEI_SOURCE_SPLIT_CATALOG {\n"
                                "  CATALOG_ID = graph_anchor_splits_v1;\n"
                                "  CURSOR_DOMAIN = ujcamei.graph_anchor;\n"
                                "};\n"
                                "UJCAMEI_SOURCE_SPLIT {\n"
                                "  SPLIT_ID = train_core;\n"
                                "  ROLE = train;\n"
                                "  SELECTOR = fraction_range;\n"
                                "  BEGIN_FRACTION = 0/1;\n"
                                "  END_FRACTION = 1/1;\n"
                                "  MIN_COUNT = 1;\n"
                                "};\n");
  write_text(
      protocol_dsl,
      "PROTOCOL {\n"
      "  PROTOCOL_ID = cwu_01v;\n"
      "  PROTOCOL_KIND = channel_graph_first;\n"
      "  GRAPH_TOPOLOGY = kikijyeba.topology.graph;\n"
      "  NODELIFT = wikimyei.expression.nodelift.srl;\n"
      "  REPRESENTATION = wikimyei.representation.encoding.vicreg;\n"
      "  INFERENCE = wikimyei.inference.expected_value.mdn;\n"
      "  OBSERVER = wikimyei.observer.belief;\n"
      "  ALLOCATION_POLICY = "
      "wikimyei.policy.portfolio.spot_distributional_utility;\n"
      "  POLICY_COMPONENT = "
      "wikimyei.policy.portfolio.graph_node_allocation;\n"
      "  REPRESENTATION_CONTRACT = "
      "graph_order.channel_node_representation.v1;\n"
      "};\n"
      "NO_LOOKAHEAD_CONTRACT {\n"
      "  CONTRACT_ID = "
      "cwu_01v_no_lookahead_artifact_provenance.anchor_v1;\n"
      "  CERTIFICATE_SCHEMA = no_lookahead_artifact_provenance.v1;\n"
      "  INFLUENCE_SCHEMA = no_lookahead_artifact_provenance.anchor_v1;\n"
      "  FRONTIER_UNIT = accepted_anchor_index;\n"
      "  SERVING_ORDER = representation,mdn,policy;\n"
      "  VISIBILITY_POLICY = prior_generation_per_slice;\n"
      "  DERIVED_ARTIFACT_RULE = inherit_parent_influence;\n"
      "  CHECKPOINT_RULE = generation_manifest_required;\n"
      "  PUBLISH_RULE = valid_from_anchor_gte_fit_end;\n"
      "  BOOTSTRAP_POLICY = explicit_lane_only;\n"
      "  RESEARCH_POLICY = smoke_or_research_not_promotable;\n"
      "};\n");

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
             std::string("TRAINING {\n"
                         "  VERSION = "
                         "wikimyei.representation.vicreg.jkimyei.v1;\n"
                         "  TRAINING_ID = fixture_vicreg;\n"
                         "  TASK = vicreg_representation;\n"
                         "  COMPONENT_ASSEMBLY_ID = vicreg_v1;\n"
                         "  OPTIMIZER = adam;\n"
                         "  LEARNING_RATE = 0.001;\n"
                         "  MAX_STEPS = ") +
                 std::to_string(max_steps) +
                 ";\n"
                 "  BATCH_SIZE = " +
                 std::to_string(batch_size) +
                 ";\n"
                 "  GRAD_CLIP_NORM = 0.0;\n"
                 "  CHECKPOINT_EVERY = " +
                 std::to_string(checkpoint_every) +
                 ";\n"
                 "  REPORT_EVERY = " +
                 std::to_string(report_every) +
                 ";\n"
                 "  VALIDATION_EVERY = 0;\n"
                 "  SEED = 17;\n"
                 "  FREEZE_REPRESENTATION = false;\n"
                 "  TRAINING_VISIBILITY_POLICY = prior_generation_per_slice;\n"
                 "  GENERATION_LANE_POLICY = "
                 "readiness_grade_bootstrap_frozen_init_only;\n"
                 "  VALID_FROM_POLICY = valid_from_anchor_gte_fit_end;\n"
                 "  ARTIFACT_PROVENANCE_POLICY = "
                 "transitive_influence_required;\n"
                 "};\n");
  write_text(channel_mdn_jkimyei,
             std::string("TRAINING {\n"
                         "  VERSION = "
                         "wikimyei.inference.expected_value.mdn."
                         "jkimyei.v1;\n"
                         "  TRAINING_ID = fixture_mdn;\n"
                         "  TASK = mdn_expected_value_inference;\n"
                         "  COMPONENT_ASSEMBLY_ID = mdn_v1;\n"
                         "  OPTIMIZER = adam;\n"
                         "  LEARNING_RATE = 0.01;\n"
                         "  MAX_STEPS = ") +
                 std::to_string(max_steps) +
                 ";\n"
                 "  BATCH_SIZE = " +
                 std::to_string(batch_size) +
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
                 "  MDN_EDGE_RETURN_AUXILIARY_LOSS_WEIGHT = 0.0;\n"
                 "  MDN_EDGE_RETURN_AUXILIARY_DIRECTION_WEIGHT = 0.0;\n"
                 "  MDN_EDGE_RETURN_AUXILIARY_RANK_WEIGHT = 0.0;\n"
                 "  MDN_EDGE_RETURN_AUXILIARY_HUBER_BETA = 0.01;\n"
                 "  MDN_EDGE_RETURN_AUXILIARY_LOGIT_SCALE = 50.0;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_ENABLED = false;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_LOSS_WEIGHT = 0.0;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_DIRECTION_WEIGHT = 0.0;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_RANK_WEIGHT = 0.0;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_HUBER_BETA = 0.01;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_LOGIT_SCALE = 50.0;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_TARGET_SCALE = 1.0;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_WARMUP_STEPS = 0;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_WARMUP_NLL_WEIGHT = 1.0;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_POST_WARMUP_NLL_WEIGHT = "
                 "1.0;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_WARMUP_DIRECT_HEAD_ONLY = "
                 "false;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_IDENTITY_MODE = shared;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_BASE_EDGE_COUNT = 0;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_IDENTITY_EMBEDDING_DIM = "
                 "0;\n"
                 "  MDN_DIRECT_EDGE_RETURN_READOUT_ADAPTER_HIDDEN_DIM = 0;\n"
                 "  FREEZE_REPRESENTATION = true;\n"
                 "  INPUT_REPRESENTATION_CHECKPOINT = ;\n"
                 "  INPUT_MDN_CHECKPOINT = ;\n"
                 "  ALLOW_UNTRAINED_REPRESENTATION = true;\n"
                 "  TRAINING_VISIBILITY_POLICY = prior_generation_per_slice;\n"
                 "  GENERATION_LANE_POLICY = "
                 "readiness_grade_bootstrap_frozen_init_only;\n"
                 "  VALID_FROM_POLICY = valid_from_anchor_gte_fit_end;\n"
                 "  ARTIFACT_PROVENANCE_POLICY = "
                 "transitive_influence_required;\n"
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
          "ujcamei_source_cursor_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/ujcamei.source.cursor.dsl.bnf\n"
          "ujcamei_source_cursor_dsl_path = " +
          cursor_dsl.string() +
          "\n"
          "ujcamei_source_splits_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/ujcamei.source.splits.dsl.bnf\n"
          "ujcamei_source_splits_dsl_path = " +
          source_splits_dsl.string() +
          "\n\n"
          "[KIKIJYEBA]\n"
          "kikijyeba_protocol_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/kikijyeba.protocol.dsl.bnf\n"
          "kikijyeba_protocol_dsl_path = " +
          protocol_dsl.string() +
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
          "\n"
          "lattice_split_policy_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/hero.lattice.split_policy.dsl.bnf\n"
          "lattice_split_policy_dsl_path = "
          "/cuwacunu/src/config/hero.lattice.split_policy.dsl"
          "\n\n"
          "[WIKIMYEI]\n"
          "wikimyei_expression_nodelift_srl_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.expression.nodelift.srl.dsl.bnf\n"
          "wikimyei_expression_nodelift_srl_dsl_path = "
          "/cuwacunu/src/config/wikimyei.expression.nodelift.srl.dsl\n"
          "wikimyei_observer_belief_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/wikimyei.observer.belief.dsl.bnf\n"
          "wikimyei_observer_belief_dsl_path = "
          "/cuwacunu/src/config/wikimyei.observer.belief.dsl\n"
          "wikimyei_policy_portfolio_spot_distributional_utility_dsl_bnf_path "
          "= /cuwacunu/src/config/grammar/"
          "wikimyei.policy.portfolio.spot_distributional_utility.dsl.bnf\n"
          "wikimyei_policy_portfolio_spot_distributional_utility_dsl_path = "
          "/cuwacunu/src/config/"
          "wikimyei.policy.portfolio.spot_distributional_utility.dsl\n"
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
          "wikimyei.inference.expected_value.mdn.dsl\n"
          "wikimyei_inference_expected_value_mdn_net_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.net.bnf\n"
          "wikimyei_inference_expected_value_mdn_net_path = "
          "/cuwacunu/src/config/"
          "wikimyei.inference.expected_value.mdn.net\n"
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

void replace_once(std::string &text, const std::string &needle,
                  const std::string &replacement) {
  const auto pos = text.find(needle);
  check(pos != std::string::npos, "expected text replacement needle missing");
  text.replace(pos, needle.size(), replacement);
}

void configure_channel_mdn_checkpoints(
    const fixture_paths_t &fixture,
    const std::filesystem::path &representation_checkpoint,
    const std::filesystem::path &mdn_checkpoint = {}) {
  auto text = read_text(fixture.channel_mdn_jkimyei);
  replace_once(text, "  INPUT_REPRESENTATION_CHECKPOINT = ;\n",
               "  INPUT_REPRESENTATION_CHECKPOINT = " +
                   representation_checkpoint.string() + ";\n");
  if (!mdn_checkpoint.empty()) {
    replace_once(text, "  INPUT_MDN_CHECKPOINT = ;\n",
                 "  INPUT_MDN_CHECKPOINT = " + mdn_checkpoint.string() + ";\n");
  }
  replace_once(text, "  ALLOW_UNTRAINED_REPRESENTATION = true;\n",
               "  ALLOW_UNTRAINED_REPRESENTATION = false;\n");
  write_text(fixture.channel_mdn_jkimyei, text);
}

void configure_vicreg_cell_policy(const fixture_paths_t &fixture,
                                  const std::string &policy) {
  auto text = read_text(fixture.vicreg_dsl);
  replace_once(text, "  CELL_VALID_POLICY = required_features;\n",
               "  CELL_VALID_POLICY = " + policy + ";\n");
  write_text(fixture.vicreg_dsl, text);
}

void configure_vicreg_recency_decay(const fixture_paths_t &fixture,
                                    const std::string &value) {
  auto text = read_text(fixture.vicreg_net);
  replace_once(text, "  RECENCY_DECAY = 0.75;\n",
               "  RECENCY_DECAY = " + value + ";\n");
  write_text(fixture.vicreg_net, text);
}

void configure_channel_mdn_sigma_min(const fixture_paths_t &fixture,
                                     const std::string &value) {
  auto text = read_text(fixture.channel_mdn_dsl);
  replace_once(text, "  SIGMA_MIN = 0.001;\n",
               "  SIGMA_MIN = " + value + ";\n");
  write_text(fixture.channel_mdn_dsl, text);
}

void configure_channel_mdn_identity_conditioned_direct_readout(
    const fixture_paths_t &fixture) {
  auto text = read_text(fixture.channel_mdn_jkimyei);
  replace_once(text, "  MDN_DIRECT_EDGE_RETURN_READOUT_ENABLED = false;\n",
               "  MDN_DIRECT_EDGE_RETURN_READOUT_ENABLED = true;\n");
  replace_once(text, "  MDN_DIRECT_EDGE_RETURN_READOUT_LOSS_WEIGHT = 0.0;\n",
               "  MDN_DIRECT_EDGE_RETURN_READOUT_LOSS_WEIGHT = 2.0;\n");
  replace_once(text,
               "  MDN_DIRECT_EDGE_RETURN_READOUT_DIRECTION_WEIGHT = 0.0;\n",
               "  MDN_DIRECT_EDGE_RETURN_READOUT_DIRECTION_WEIGHT = 0.25;\n");
  replace_once(text, "  MDN_DIRECT_EDGE_RETURN_READOUT_RANK_WEIGHT = 0.0;\n",
               "  MDN_DIRECT_EDGE_RETURN_READOUT_RANK_WEIGHT = 0.25;\n");
  replace_once(text, "  MDN_DIRECT_EDGE_RETURN_READOUT_TARGET_SCALE = 1.0;\n",
               "  MDN_DIRECT_EDGE_RETURN_READOUT_TARGET_SCALE = 4.0;\n");
  replace_once(
      text, "  MDN_DIRECT_EDGE_RETURN_READOUT_POST_WARMUP_NLL_WEIGHT = 1.0;\n",
      "  MDN_DIRECT_EDGE_RETURN_READOUT_POST_WARMUP_NLL_WEIGHT = 0.25;\n");
  replace_once(text,
               "  MDN_DIRECT_EDGE_RETURN_READOUT_IDENTITY_MODE = shared;\n",
               "  MDN_DIRECT_EDGE_RETURN_READOUT_IDENTITY_MODE = "
               "edge_embedding_per_edge;\n");
  replace_once(text, "  MDN_DIRECT_EDGE_RETURN_READOUT_BASE_EDGE_COUNT = 0;\n",
               "  MDN_DIRECT_EDGE_RETURN_READOUT_BASE_EDGE_COUNT = 3;\n");
  replace_once(
      text, "  MDN_DIRECT_EDGE_RETURN_READOUT_IDENTITY_EMBEDDING_DIM = 0;\n",
      "  MDN_DIRECT_EDGE_RETURN_READOUT_IDENTITY_EMBEDDING_DIM = 4;\n");
  replace_once(text,
               "  MDN_DIRECT_EDGE_RETURN_READOUT_ADAPTER_HIDDEN_DIM = 0;\n",
               "  MDN_DIRECT_EDGE_RETURN_READOUT_ADAPTER_HIDDEN_DIM = 8;\n");
  write_text(fixture.channel_mdn_jkimyei, text);
}

void configure_channel_mdn_one_step_direct_readout_warmup(
    const fixture_paths_t &fixture) {
  auto text = read_text(fixture.channel_mdn_jkimyei);
  replace_once(text, "  MDN_DIRECT_EDGE_RETURN_READOUT_WARMUP_STEPS = 0;\n",
               "  MDN_DIRECT_EDGE_RETURN_READOUT_WARMUP_STEPS = 1;\n");
  replace_once(
      text, "  MDN_DIRECT_EDGE_RETURN_READOUT_WARMUP_NLL_WEIGHT = 1.0;\n",
      "  MDN_DIRECT_EDGE_RETURN_READOUT_WARMUP_NLL_WEIGHT = 0.0;\n");
  replace_once(
      text,
      "  MDN_DIRECT_EDGE_RETURN_READOUT_WARMUP_DIRECT_HEAD_ONLY = false;\n",
      "  MDN_DIRECT_EDGE_RETURN_READOUT_WARMUP_DIRECT_HEAD_ONLY = true;\n");
  write_text(fixture.channel_mdn_jkimyei, text);
}

protocol::channel_graph_first_pipeline_builder_t<Kline>
make_pipeline(const fixture_paths_t &fixture, std::size_t batch_size = 2) {
  auto bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      fixture.config);
  protocol::graph_first_pipeline_builder_options_t builder_options{};
  builder_options.force_rebuild_cache = true;
  builder_options.batch_size = batch_size;
  return protocol::channel_graph_first_pipeline_builder_t<Kline>(
      std::move(bundle), builder_options);
}

void test_channel_representation_dry_run() {
  const auto fixture =
      make_config_fixture("representation_dry_run", /*max_steps=*/2,
                          /*checkpoint_every=*/0, /*batch_size=*/2,
                          /*report_every=*/1, "run");
  auto pipe = make_pipeline(fixture);
  representation_launcher::channel_graph_first_representation_launcher_t<Kline>
      launcher(std::move(pipe));
  const auto report = launcher.dry_run_report();
  check(report.training_id == "fixture_vicreg",
        "representation dry-run training id");
  check(report.component_assembly_id == "vicreg_v1",
        "representation dry-run component id");
  check(report.representation_architecture ==
            "channel_preserving_local_node_encoder.v1",
        "representation dry-run architecture");
  check(report.representation_contract ==
            "graph_order.channel_node_representation.v1",
        "representation dry-run contract");
  check(report.primary_value_shape == "[B,N,C,De]",
        "representation dry-run primary shape");
  check(report.channel_axis_policy == "preserved_primary_output",
        "representation dry-run channel axis policy");
  check(report.channel_count == 1, "representation dry-run channel count");
  check(report.history_length == 2, "representation dry-run Hx");
  check(report.input_width == 9, "representation dry-run input width");
  check(report.encoding_dim == 4, "representation dry-run encoding dim");
  check(report.feature_hidden_dim == 8,
        "representation dry-run feature hidden dim");
  check(report.temporal_depth == 1, "representation dry-run temporal depth");
  check(report.recency_decay == 0.75, "representation dry-run recency decay");
  check(report.min_valid_fraction == 1.0,
        "representation dry-run min valid fraction");
  check(report.use_missingness_indicators,
        "representation dry-run missingness indicators");
  check(report.projector_dim == 6, "representation dry-run projector dim");
  check(report.projector_hidden_dim == 8,
        "representation dry-run projector hidden dim");
  check(report.projector_depth == 1, "representation dry-run projector depth");
  check(report.global_aux_weight == 0.0,
        "representation dry-run global aux weight");
  check(report.jitter_std == 0.01, "representation dry-run jitter");
  check(report.feature_dropout_prob == 0.0,
        "representation dry-run feature dropout");
  check(report.history_dropout_prob == 0.0,
        "representation dry-run history dropout");
  check(report.cell_valid_policy == "required_features",
        "representation dry-run cell policy");
  check(report.target_action == "run", "representation dry-run action");
  check(report.runtime_report_mode == "normal",
        "representation dry-run runtime mode");
  check(report.stream_plan.find("channel_representation") != std::string::npos,
        "representation dry-run stream plan");
}

void test_channel_representation_training_run() {
  torch::manual_seed(101);
  const auto fixture =
      make_config_fixture("representation_training", /*max_steps=*/2,
                          /*checkpoint_every=*/1, /*batch_size=*/2,
                          /*report_every=*/1, "train");
  auto pipe = make_pipeline(fixture);
  representation_launcher::channel_graph_first_representation_launcher_options_t
      options{};
  options.write_report = true;
  options.report_path = fixture.report;
  representation_launcher::channel_graph_first_representation_launcher_t<Kline>
      launcher(std::move(pipe), options);
  const auto report = launcher.run();
  check(report.steps_attempted > 0, "representation attempted steps");
  check(report.steps_completed > 0, "representation completed steps");
  check(report.optimizer_steps == report.steps_completed,
        "representation optimizer steps match completed");
  check(report.total_valid_projection_rows > 0,
        "representation valid projection rows");
  check(report.adapter_valid_feature_count > 0,
        "representation adapter valid features");
  check(report.adapter_valid_cell_all_count > 0,
        "representation adapter valid all-feature cells");
  check(report.augmented_valid_feature_count > 0,
        "representation augmented valid features");
  check(std::isfinite(report.mean_augmented_valid_feature_fraction) &&
            report.mean_augmented_valid_feature_fraction > 0.0 &&
            report.mean_augmented_valid_feature_fraction <= 1.0,
        "representation augmented feature fraction finite");
  check(std::isfinite(report.mean_augmented_feature_retention_fraction) &&
            std::abs(report.mean_augmented_feature_retention_fraction - 1.0) <
                1e-8,
        "representation default augmentation retains valid features");
  check(std::isfinite(report.mean_loss), "representation mean loss finite");
  check(std::isfinite(report.mean_invariance_loss),
        "representation invariance finite");
  check(std::isfinite(report.mean_variance_loss),
        "representation variance finite");
  check(std::isfinite(report.mean_covariance_loss),
        "representation covariance finite");
  check(report.finite_parameter_check == 1.0,
        "representation finite parameter check");
  check(report.representation_embedding_dim == report.encoding_dim,
        "representation embedding dim diagnostic");
  check(std::isfinite(report.representation_effective_rank) &&
            report.representation_effective_rank > 0.0,
        "representation effective rank diagnostic");
  check(std::isfinite(report.representation_effective_rank_fraction) &&
            report.representation_effective_rank_fraction > 0.0 &&
            report.representation_effective_rank_fraction <= 1.0,
        "representation effective rank fraction diagnostic");
  check(std::isfinite(report.representation_min_dimension_variance),
        "representation min variance diagnostic");
  check(std::isfinite(report.representation_max_dimension_variance),
        "representation max variance diagnostic");
  check(std::isfinite(report.representation_condition_number) &&
            report.representation_condition_number >= 1.0,
        "representation condition number diagnostic");
  check(std::isfinite(report.representation_isotropy_score) &&
            report.representation_isotropy_score >= 0.0 &&
            report.representation_isotropy_score <= 1.0,
        "representation isotropy diagnostic");
  check(report.representation_effective_rank_per_channel.size() == 1 &&
            std::isfinite(report.representation_effective_rank_per_channel[0]),
        "representation per-channel effective rank diagnostic");
  check(report.representation_min_dimension_variance_per_channel.size() == 1 &&
            std::isfinite(
                report.representation_min_dimension_variance_per_channel[0]),
        "representation per-channel min variance diagnostic");
  check(
      report.representation_condition_number_per_channel.size() == 1 &&
          std::isfinite(report.representation_condition_number_per_channel[0]),
      "representation per-channel condition number diagnostic");
  check(report.representation_isotropy_score_per_channel.size() == 1 &&
            std::isfinite(report.representation_isotropy_score_per_channel[0]),
        "representation per-channel isotropy diagnostic");
  check(report.report_written, "representation report written");
  check(std::filesystem::exists(fixture.report),
        "representation report path exists");
  check(report.checkpoint_written, "representation checkpoint metadata");
  check(report.checkpoint_write_count == report.optimizer_steps,
        "representation checkpoint every optimizer step");
  check(report.checkpoint_format == "torch_archive_vicreg_v1",
        "representation checkpoint format");
  check(std::filesystem::exists(report.checkpoint_path),
        "representation checkpoint file exists");
  const auto report_text = read_text(fixture.report);
  check(report_text.find("component_assembly_id=vicreg_v1") !=
            std::string::npos,
        "representation report component id");
  check(report_text.find("representation_architecture="
                         "channel_preserving_local_node_encoder.v1") !=
            std::string::npos,
        "representation report architecture");
  check(report_text.find("representation_contract="
                         "graph_order.channel_node_representation.v1") !=
            std::string::npos,
        "representation report contract");
  check(report_text.find("primary_value_shape=[B,N,C,De]") != std::string::npos,
        "representation report primary shape");
  check(report_text.find("channel_axis_policy=preserved_primary_output") !=
            std::string::npos,
        "representation report channel axis policy");
  check(report_text.find("cell_valid_policy=required_features") !=
            std::string::npos,
        "representation report cell policy");
  check(report_text.find("recency_decay=0.75") != std::string::npos,
        "representation report recency decay");
  check(report_text.find("use_missingness_indicators=true") !=
            std::string::npos,
        "representation report missingness indicators");
  check(report_text.find("jitter_std=0.01") != std::string::npos,
        "representation report jitter policy");
  check(report_text.find("feature_dropout_prob=0") != std::string::npos,
        "representation report feature dropout policy");
  check(report_text.find("history_dropout_prob=0") != std::string::npos,
        "representation report history dropout policy");
  check(report_text.find("augmented_valid_feature_count=") != std::string::npos,
        "representation report augmented feature count");
  check(report_text.find("mean_augmented_valid_feature_fraction=") !=
            std::string::npos,
        "representation report augmented feature fraction");
  check(report_text.find("mean_augmented_feature_retention_fraction=") !=
            std::string::npos,
        "representation report augmented retention fraction");
  check(report_text.find("representation_effective_rank=") != std::string::npos,
        "representation report effective rank");
  check(report_text.find("representation_condition_number=") !=
            std::string::npos,
        "representation report condition number");
  check(report_text.find("representation_effective_rank_per_channel=") !=
            std::string::npos,
        "representation report per-channel effective rank");
  check(report_text.find("representation_isotropy_score_per_channel=") !=
            std::string::npos,
        "representation report per-channel isotropy");
  check(report_text.find("channel_count=1") != std::string::npos,
        "representation report channel count");
  check(report_text.find("runtime_report_mode=normal") != std::string::npos,
        "representation report runtime mode");
}

void test_channel_representation_debug_lls() {
  torch::manual_seed(107);
  const auto fixture =
      make_config_fixture("representation_debug_lls", /*max_steps=*/1,
                          /*checkpoint_every=*/0, /*batch_size=*/2,
                          /*report_every=*/1, "train");
  auto pipe = make_pipeline(fixture);
  representation_launcher::channel_graph_first_representation_launcher_options_t
      options{};
  options.write_report = true;
  options.report_path = fixture.report;
  options.runtime_report_mode = runtime_report::runtime_report_mode_t::debug;
  representation_launcher::channel_graph_first_representation_launcher_t<Kline>
      launcher(std::move(pipe), options);
  const auto report = launcher.run();
  check(report.runtime_lls_emitted, "channel representation debug LLS emitted");
  check(report.runtime_report_mode == "debug",
        "channel representation debug runtime mode");
  check(report.nodelift_runtime_lls.find(
            "component_assembly_id:str = nodelift_srl_v1") != std::string::npos,
        "channel representation debug NodeLift LLS component");
  check(report.representation_training_runtime_lls.find(
            "wikimyei.representation.vicreg.training.runtime.v1") !=
            std::string::npos,
        "channel representation training LLS schema");
  check(report.representation_training_runtime_lls.find(
            "component_assembly_id:str = vicreg_v1") != std::string::npos,
        "channel representation training LLS component");
  check(report.representation_training_runtime_lls.find(
            "valid_feature_fraction") != std::string::npos,
        "channel representation training LLS valid feature fraction");
  check(report.representation_training_runtime_lls.find(
            "view1_valid_feature_fraction") != std::string::npos,
        "channel representation training LLS view fraction");
  check(report.representation_training_runtime_lls.find(
            "view1_feature_retention_fraction") != std::string::npos,
        "channel representation training LLS retention fraction");
  check(report.representation_training_runtime_lls.find("wave_pulse_index") !=
            std::string::npos,
        "channel representation training LLS pulse index");
  check(std::filesystem::exists(fixture.report.string() + ".nodelift.lls"),
        "channel representation debug NodeLift LLS sidecar exists");
  check(std::filesystem::exists(fixture.report.string() +
                                ".representation_training.lls"),
        "channel representation training LLS sidecar exists");
}

void test_channel_representation_run_mode_no_mutation() {
  torch::manual_seed(102);
  const auto fixture =
      make_config_fixture("representation_run_mode", /*max_steps=*/2,
                          /*checkpoint_every=*/1, /*batch_size=*/2,
                          /*report_every=*/1, "run");
  auto pipe = make_pipeline(fixture);
  representation_launcher::channel_graph_first_representation_launcher_options_t
      options{};
  options.write_report = true;
  options.report_path = fixture.report;
  representation_launcher::channel_graph_first_representation_launcher_t<Kline>
      launcher(std::move(pipe), options);
  const auto report = launcher.run();
  check(report.target_action == "run", "representation run action");
  check(report.steps_attempted > 0, "representation run attempted steps");
  check(report.steps_completed > 0, "representation run completed steps");
  check(report.optimizer_steps == 0, "representation run no optimizer steps");
  check(!report.checkpoint_written, "representation run no checkpoint");
  check(report.total_valid_projection_rows > 0,
        "representation run valid projection rows");
  check(report.finite_parameter_check == 1.0,
        "representation run finite parameter check");
  check(report.representation_embedding_dim == report.encoding_dim,
        "representation run embedding dim diagnostic");
  check(std::isfinite(report.representation_effective_rank),
        "representation run effective rank diagnostic");
  check(report.report_written, "representation run report written");
  const auto report_text = read_text(fixture.report);
  check(report_text.find("target_action=run") != std::string::npos,
        "representation run report action");
  check(report_text.find("optimizer_steps=0") != std::string::npos,
        "representation run report optimizer steps");
  check(report_text.find("checkpoint_written=false") != std::string::npos,
        "representation run report checkpoint");
}

void test_channel_mdn_training_run() {
  torch::manual_seed(103);
  const auto fixture = make_config_fixture("mdn_training", /*max_steps=*/2,
                                           /*checkpoint_every=*/1,
                                           /*batch_size=*/2,
                                           /*report_every=*/1, "train");
  auto pipe = make_pipeline(fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      options{};
  options.write_report = true;
  options.report_path = fixture.report;
  inference_launcher::channel_graph_first_inference_launcher_t<Kline> launcher(
      std::move(pipe), options);
  const auto report = launcher.run();
  check(report.steps_attempted > 0, "MDN attempted steps");
  check(report.steps_completed > 0, "MDN completed steps");
  check(report.optimizer_steps == report.steps_completed,
        "MDN optimizer steps match completed");
  check(report.total_valid_target_count > 0, "MDN valid targets");
  check(report.nonfinite_output_count == 0, "MDN finite outputs");
  check(std::isfinite(report.mean_loss), "MDN mean loss finite");
  check(std::isfinite(report.mean_sigma_mean), "MDN sigma mean finite");
  check(std::isfinite(report.min_sigma_min), "MDN sigma min finite");
  check(std::isfinite(report.max_sigma_max), "MDN sigma max finite");
  check(std::isfinite(report.mean_sigma_mean_valid),
        "MDN masked sigma mean finite");
  check(std::isfinite(report.min_sigma_min_valid),
        "MDN masked sigma min finite");
  check(std::isfinite(report.max_sigma_max_valid),
        "MDN masked sigma max finite");
  check(std::isfinite(report.mean_mixture_entropy),
        "MDN mixture entropy finite");
  check(report.mean_nll_per_channel.size() == 1, "MDN per-channel NLL count");
  check(std::isfinite(report.mean_nll_per_channel[0]),
        "MDN per-channel NLL finite");
  check(report.mean_nll_per_target_feature.size() == 4,
        "MDN per-target-feature NLL count");
  check(std::isfinite(report.mean_nll_per_target_feature[0]),
        "MDN per-target-feature NLL finite");
  check(report.mean_nll_per_channel_target_feature.size() == 4,
        "MDN per-channel/target-feature NLL count");
  check(std::isfinite(report.mean_nll_per_channel_target_feature[0]),
        "MDN per-channel/target-feature NLL finite");
  check(report.valid_target_count_per_channel.size() == 1 &&
            report.valid_target_count_per_channel[0] > 0,
        "MDN per-channel support count");
  check(report.valid_target_count_per_target_feature.size() == 4 &&
            report.valid_target_count_per_target_feature[0] > 0,
        "MDN per-feature support count");
  check(report.valid_target_count_per_channel_target_feature.size() == 4 &&
            report.valid_target_count_per_channel_target_feature[0] > 0,
        "MDN per-channel/feature support count");
  check(report.mdn_architecture ==
                "shared_slot_trunk.channel_adapter.shared_feature_head."
                "direct_edge_readout.shared.v4" &&
            report.loss_reduction == "balanced_channel_feature_mean" &&
            report.feature_embedding_dim == 2 &&
            report.channel_adapter_rank == 2 && report.shared_trunk &&
            report.channel_adapters_enabled && report.shared_feature_head &&
            report.feature_embedding_enabled && !report.node_id_embedding &&
            !report.cross_node_attention && !report.cross_channel_attention,
        "MDN architecture and loss contract recorded");
  check(report.mean_mixture_usage.size() == 2, "MDN mixture usage count");
  check(std::isfinite(report.mean_mixture_usage[0]) &&
            std::isfinite(report.mean_mixture_usage[1]),
        "MDN mixture usage finite");
  check(std::isfinite(report.last_grad_norm), "MDN last grad norm finite");
  check(std::isfinite(report.max_grad_norm), "MDN max grad norm finite");
  check(report.finite_parameter_check == 1.0, "MDN finite parameter check");
  check(report.context_mode == "channel_context_strict", "MDN context mode");
  check(report.target_domain == "channel_node_future", "MDN target domain");
  check(report.target_mask_policy == "per_target_feature_valid",
        "MDN target mask policy");
  check(report.activity_target == "node_feature_support_mean",
        "MDN activity target");
  check(report.hidden_width == 8, "MDN hidden width");
  check(report.residual_depth == 1, "MDN residual depth");
  check(report.sigma_min == 0.001, "MDN sigma min");
  check(report.sigma_max == 0.0, "MDN sigma max");
  check(report.eps == 0.000001, "MDN eps");
  check(report.component_assembly_id == "mdn_v1", "MDN component id");
  check(report.input_representation_assembly_id == "vicreg_v1",
        "MDN input representation id");
  check(report.context_contract == "graph_order.channel_node_representation.v1",
        "MDN context contract");
  check(report.context_value_shape == "[B,N,C,De]", "MDN context value shape");
  check(report.output_contract ==
            "graph_order.channel_node_future_distribution.v1",
        "MDN output contract");
  check(report.runtime_report_mode == "normal", "MDN runtime mode");
  check(report.report_written, "MDN report written");
  check(std::filesystem::exists(fixture.report), "MDN report path exists");
  check(report.checkpoint_written, "MDN checkpoint metadata");
  check(report.checkpoint_format == "torch_archive_channel_mdn_v2",
        "MDN checkpoint format");
  check(std::filesystem::exists(report.checkpoint_path),
        "MDN checkpoint file exists");
  const auto report_text = read_text(fixture.report);
  check(report_text.find("context_mode=channel_context_strict") !=
            std::string::npos,
        "MDN report context mode");
  check(report_text.find("component_assembly_id=mdn_v1") != std::string::npos,
        "MDN report component id");
  check(report_text.find("input_representation_assembly_id=vicreg_v1") !=
            std::string::npos,
        "MDN report input representation id");
  check(report_text.find("context_contract="
                         "graph_order.channel_node_representation.v1") !=
            std::string::npos,
        "MDN report context contract");
  check(report_text.find("context_value_shape=[B,N,C,De]") != std::string::npos,
        "MDN report context value shape");
  check(report_text.find("output_contract="
                         "graph_order.channel_node_future_distribution.v1") !=
            std::string::npos,
        "MDN report output contract");
  check(report_text.find("target_domain=channel_node_future") !=
            std::string::npos,
        "MDN report target domain");
  check(report_text.find("target_mask_policy=per_target_feature_valid") !=
            std::string::npos,
        "MDN report target mask policy");
  check(report_text.find("sigma_min=0.001") != std::string::npos,
        "MDN report sigma min");
  check(report_text.find("mean_nll_per_channel=") != std::string::npos,
        "MDN report per-channel NLL");
  check(report_text.find("mean_sigma_mean_valid=") != std::string::npos,
        "MDN report masked sigma mean");
  check(report_text.find("min_sigma_min_valid=") != std::string::npos,
        "MDN report masked sigma min");
  check(report_text.find("max_sigma_max_valid=") != std::string::npos,
        "MDN report masked sigma max");
  check(report_text.find("mean_nll_per_target_feature=") != std::string::npos,
        "MDN report per-target-feature NLL");
  check(report_text.find("mean_nll_per_channel_target_feature=") !=
            std::string::npos,
        "MDN report per-channel/target-feature NLL");
  check(report_text.find("valid_target_count_per_channel=") !=
            std::string::npos,
        "MDN report per-channel support");
  check(report_text.find("mdn_architecture="
                         "shared_slot_trunk.channel_adapter."
                         "shared_feature_head.direct_edge_readout.shared.v4") !=
            std::string::npos,
        "MDN report architecture");
  check(report_text.find("loss_reduction=balanced_channel_feature_mean") !=
            std::string::npos,
        "MDN report balanced loss reduction");
  check(report_text.find("mean_mixture_usage=") != std::string::npos,
        "MDN report mixture usage");
  check(report_text.find("max_grad_norm=") != std::string::npos,
        "MDN report max grad norm");
  check(report_text.find("runtime_report_mode=normal") != std::string::npos,
        "MDN report runtime mode");
}

void test_channel_mdn_identity_conditioned_direct_readout_training_run() {
  torch::manual_seed(104);
  const auto fixture =
      make_config_fixture("mdn_identity_direct_readout", /*max_steps=*/1,
                          /*checkpoint_every=*/1, /*batch_size=*/2,
                          /*report_every=*/1, "train");
  configure_channel_mdn_identity_conditioned_direct_readout(fixture);
  auto pipe = make_pipeline(fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      options{};
  options.write_report = true;
  options.report_path = fixture.report;
  inference_launcher::channel_graph_first_inference_launcher_t<Kline> launcher(
      std::move(pipe), options);
  const auto report = launcher.run();

  check(report.direct_edge_return_readout_enabled,
        "identity direct readout enabled");
  check(report.direct_edge_return_readout_identity_mode ==
            "edge_embedding_per_edge",
        "identity direct readout mode recorded");
  check(report.direct_edge_return_readout_base_edge_count == 3,
        "identity direct readout base edge count recorded");
  check(report.direct_edge_return_readout_identity_embedding_dim == 4,
        "identity direct readout embedding dim recorded");
  check(report.direct_edge_return_readout_adapter_hidden_dim == 8,
        "identity direct readout adapter dim recorded");
  check(report.direct_edge_return_readout_post_warmup_nll_weight == 0.25,
        "identity direct readout post-warmup NLL weight recorded");
  check(report.mdn_architecture ==
            "shared_slot_trunk.channel_adapter.shared_feature_head."
            "direct_edge_readout.edge_embedding_per_edge.v4",
        "identity direct readout architecture recorded");
  check(report.direct_edge_return_readout_loss_valid_count > 0,
        "identity direct readout valid loss count");
  check(report.direct_edge_return_readout_loss_pairwise_valid_count > 0,
        "identity direct readout valid pairwise loss count");
  check(std::isfinite(report.last_direct_edge_return_readout_loss),
        "identity direct readout loss finite");
  check(report.checkpoint_written,
        "identity direct readout checkpoint written");
  check(std::filesystem::exists(report.checkpoint_path),
        "identity direct readout checkpoint exists");

  const auto report_text = read_text(fixture.report);
  check(report_text.find("direct_edge_return_readout_identity_mode="
                         "edge_embedding_per_edge") != std::string::npos,
        "identity direct readout report mode");
  check(report_text.find("direct_edge_return_readout_base_edge_count=3") !=
            std::string::npos,
        "identity direct readout report base edge count");
  check(
      report_text.find("direct_edge_return_readout_identity_embedding_dim=4") !=
          std::string::npos,
      "identity direct readout report embedding dim");
  check(report_text.find("direct_edge_return_readout_adapter_hidden_dim=8") !=
            std::string::npos,
        "identity direct readout report adapter dim");
  check(report_text.find("direct_edge_return_readout_post_warmup_nll_weight="
                         "0.25") != std::string::npos,
        "identity direct readout report post-warmup NLL weight");
  check(report_text.find("mdn_architecture="
                         "shared_slot_trunk.channel_adapter."
                         "shared_feature_head.direct_edge_readout."
                         "edge_embedding_per_edge.v4") != std::string::npos,
        "identity direct readout report architecture");
}

void test_channel_mdn_debug_lls() {
  torch::manual_seed(108);
  const auto fixture = make_config_fixture("mdn_debug_lls", /*max_steps=*/1,
                                           /*checkpoint_every=*/0,
                                           /*batch_size=*/2,
                                           /*report_every=*/1, "train");
  auto pipe = make_pipeline(fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      options{};
  options.write_report = true;
  options.report_path = fixture.report;
  options.runtime_report_mode = runtime_report::runtime_report_mode_t::debug;
  inference_launcher::channel_graph_first_inference_launcher_t<Kline> launcher(
      std::move(pipe), options);
  const auto report = launcher.run();
  check(report.runtime_lls_emitted, "MDN debug LLS emitted");
  check(report.runtime_report_mode == "debug", "MDN debug runtime mode");
  check(report.nodelift_runtime_lls.find(
            "component_assembly_id:str = nodelift_srl_v1") != std::string::npos,
        "MDN debug NodeLift LLS component");
  check(report.representation_runtime_lls.find(
            "wikimyei.representation.vicreg.runtime.v1") != std::string::npos,
        "MDN debug representation LLS schema");
  check(report.representation_runtime_lls.find("node_encoding_mask_fraction") !=
            std::string::npos,
        "MDN debug representation mask fraction");
  check(report.representation_runtime_lls.find("reducer_weight_entropy") !=
            std::string::npos,
        "MDN debug representation reducer entropy");
  check(report.mdn_runtime_lls.find(
            "wikimyei.inference.expected_value.mdn.runtime.v1") !=
            std::string::npos,
        "MDN debug runtime LLS schema");
  check(report.mdn_runtime_lls.find("component_assembly_id:str = mdn_v1") !=
            std::string::npos,
        "MDN debug runtime LLS component");
  check(report.mdn_runtime_lls.find("valid_target_fraction") !=
            std::string::npos,
        "MDN debug runtime LLS valid target fraction");
  check(report.mdn_runtime_lls.find("context_mask_fraction") !=
            std::string::npos,
        "MDN debug runtime LLS context mask fraction");
  check(report.mdn_runtime_lls.find("future_mask_fraction") !=
            std::string::npos,
        "MDN debug runtime LLS future mask fraction");
  check(report.mdn_runtime_lls.find("sigma_mean_valid") != std::string::npos,
        "MDN debug runtime LLS masked sigma mean");
  check(report.mdn_runtime_lls.find(
            "loss_reduction:str = balanced_channel_feature_mean") !=
            std::string::npos,
        "MDN debug runtime LLS balanced loss reduction");
  check(report.mdn_runtime_lls.find(
            "mdn_architecture:str = "
            "shared_slot_trunk.channel_adapter.shared_feature_head."
            "direct_edge_readout.v4") != std::string::npos,
        "MDN debug runtime LLS architecture");
  check(std::filesystem::exists(fixture.report.string() + ".nodelift.lls"),
        "MDN debug NodeLift LLS sidecar exists");
  check(
      std::filesystem::exists(fixture.report.string() + ".representation.lls"),
      "MDN debug representation LLS sidecar exists");
  check(std::filesystem::exists(fixture.report.string() + ".mdn.lls"),
        "MDN debug MDN LLS sidecar exists");
}

void test_channel_mdn_run_requires_mdn_checkpoint() {
  torch::manual_seed(105);
  const auto fixture =
      make_config_fixture("mdn_run_requires_checkpoint", /*max_steps=*/1,
                          /*checkpoint_every=*/0, /*batch_size=*/2,
                          /*report_every=*/1, "run");
  auto pipe = make_pipeline(fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      options{};
  options.write_report = true;
  options.report_path = fixture.report;
  inference_launcher::channel_graph_first_inference_launcher_t<Kline> launcher(
      std::move(pipe), options);
  bool threw = false;
  try {
    (void)launcher.run();
  } catch (const std::exception &ex) {
    threw = std::string(ex.what()).find("INPUT_MDN_CHECKPOINT") !=
            std::string::npos;
  }
  check(threw, "MDN run requires checkpoint");
}

void test_channel_mdn_conditioned_affine_shadow_defaults_absent_and_rejects_train() {
  torch::manual_seed(117);
  const auto fixture =
      make_config_fixture("mdn_conditioned_affine_shadow_train_rejection",
                          /*max_steps=*/1, /*checkpoint_every=*/0,
                          /*batch_size=*/2, /*report_every=*/1, "train");
  auto pipe = make_pipeline(fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      options{};
  check(!options.conditioned_affine_shadow.has_value(),
        "conditioned affine shadow defaults absent");
  options.conditioned_affine_shadow =
      inference_launcher::channel_mdn_conditioned_affine_shadow_options_t{};
  inference_launcher::channel_graph_first_inference_launcher_t<Kline> launcher(
      std::move(pipe), options);
  bool threw = false;
  try {
    (void)launcher.run();
  } catch (const std::exception &ex) {
    threw =
        std::string(ex.what()).find("train_target=true") != std::string::npos;
  }
  check(threw, "conditioned affine shadow rejects train target");
}

void test_channel_mdn_loads_representation_checkpoint() {
  torch::manual_seed(109);
  const auto rep_fixture =
      make_config_fixture("mdn_representation_restore_source",
                          /*max_steps=*/1, /*checkpoint_every=*/1,
                          /*batch_size=*/2, /*report_every=*/1, "train");
  auto rep_pipe = make_pipeline(rep_fixture);
  representation_launcher::channel_graph_first_representation_launcher_options_t
      rep_options{};
  rep_options.write_report = true;
  rep_options.report_path = rep_fixture.report;
  representation_launcher::channel_graph_first_representation_launcher_t<Kline>
      rep_launcher(std::move(rep_pipe), rep_options);
  const auto rep_report = rep_launcher.run();
  check(rep_report.checkpoint_written,
        "restore source representation checkpoint written");
  check(std::filesystem::exists(rep_report.checkpoint_path),
        "restore source representation checkpoint exists");

  const auto mdn_fixture =
      make_config_fixture("mdn_representation_restore_consumer",
                          /*max_steps=*/1, /*checkpoint_every=*/0,
                          /*batch_size=*/2, /*report_every=*/1, "train");
  configure_channel_mdn_checkpoints(mdn_fixture, rep_report.checkpoint_path);
  auto mdn_pipe = make_pipeline(mdn_fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      mdn_options{};
  mdn_options.write_report = true;
  mdn_options.report_path = mdn_fixture.report;
  inference_launcher::channel_graph_first_inference_launcher_t<Kline>
      mdn_launcher(std::move(mdn_pipe), mdn_options);
  const auto mdn_report = mdn_launcher.run();
  check(mdn_report.representation_checkpoint_loaded,
        "MDN loaded channel representation checkpoint");
  check(!mdn_report.mdn_checkpoint_loaded,
        "MDN did not load absent MDN checkpoint");
  check(mdn_report.steps_completed > 0,
        "MDN completed with restored representation");
  const auto report_text = read_text(mdn_fixture.report);
  check(report_text.find("representation_checkpoint_loaded=true") !=
            std::string::npos,
        "MDN report records representation checkpoint load");
}

void test_channel_mdn_rejects_representation_checkpoint_policy_mismatch() {
  torch::manual_seed(111);
  const auto rep_fixture = make_config_fixture(
      "mdn_representation_policy_mismatch_source", /*max_steps=*/1,
      /*checkpoint_every=*/1, /*batch_size=*/2, /*report_every=*/1, "train");
  auto rep_pipe = make_pipeline(rep_fixture);
  representation_launcher::channel_graph_first_representation_launcher_options_t
      rep_options{};
  rep_options.write_report = true;
  rep_options.report_path = rep_fixture.report;
  representation_launcher::channel_graph_first_representation_launcher_t<Kline>
      rep_launcher(std::move(rep_pipe), rep_options);
  const auto rep_report = rep_launcher.run();
  check(rep_report.checkpoint_written,
        "policy mismatch source representation checkpoint written");

  const auto mdn_fixture = make_config_fixture(
      "mdn_representation_policy_mismatch_consumer", /*max_steps=*/1,
      /*checkpoint_every=*/0, /*batch_size=*/2, /*report_every=*/1, "train");
  configure_channel_mdn_checkpoints(mdn_fixture, rep_report.checkpoint_path);
  configure_vicreg_cell_policy(mdn_fixture, "any_feature");
  auto mdn_pipe = make_pipeline(mdn_fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      mdn_options{};
  mdn_options.write_report = true;
  mdn_options.report_path = mdn_fixture.report;
  inference_launcher::channel_graph_first_inference_launcher_t<Kline>
      mdn_launcher(std::move(mdn_pipe), mdn_options);
  bool threw = false;
  try {
    (void)mdn_launcher.run();
  } catch (const std::exception &ex) {
    threw =
        std::string(ex.what()).find("cell_valid_policy") != std::string::npos;
  }
  check(threw,
        "MDN rejects representation checkpoint with mismatched cell policy");
}

void test_channel_mdn_rejects_representation_checkpoint_recency_mismatch() {
  torch::manual_seed(112);
  const auto rep_fixture = make_config_fixture(
      "mdn_representation_recency_mismatch_source", /*max_steps=*/1,
      /*checkpoint_every=*/1, /*batch_size=*/2, /*report_every=*/1, "train");
  auto rep_pipe = make_pipeline(rep_fixture);
  representation_launcher::channel_graph_first_representation_launcher_options_t
      rep_options{};
  rep_options.write_report = true;
  rep_options.report_path = rep_fixture.report;
  representation_launcher::channel_graph_first_representation_launcher_t<Kline>
      rep_launcher(std::move(rep_pipe), rep_options);
  const auto rep_report = rep_launcher.run();
  check(rep_report.checkpoint_written,
        "recency mismatch source representation checkpoint written");

  const auto mdn_fixture = make_config_fixture(
      "mdn_representation_recency_mismatch_consumer", /*max_steps=*/1,
      /*checkpoint_every=*/0, /*batch_size=*/2, /*report_every=*/1, "train");
  configure_channel_mdn_checkpoints(mdn_fixture, rep_report.checkpoint_path);
  configure_vicreg_recency_decay(mdn_fixture, "0.5");
  auto mdn_pipe = make_pipeline(mdn_fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      mdn_options{};
  mdn_options.write_report = true;
  mdn_options.report_path = mdn_fixture.report;
  inference_launcher::channel_graph_first_inference_launcher_t<Kline>
      mdn_launcher(std::move(mdn_pipe), mdn_options);
  bool threw = false;
  try {
    (void)mdn_launcher.run();
  } catch (const std::exception &ex) {
    threw = std::string(ex.what()).find("recency_decay") != std::string::npos;
  }
  check(threw,
        "MDN rejects representation checkpoint with mismatched recency decay");
}

void test_channel_mdn_checkpoint_identity_rejects_representation_id_mismatch() {
  torch::manual_seed(113);
  const auto dir = make_tmp_dir("mdn_checkpoint_identity_mismatch");
  const auto checkpoint_path = dir / "channel_mdn.pt";

  inference_launcher::channel_graph_first_inference_training_report_t report{};
  report.component_assembly_id = "mdn_v1";
  report.input_representation_assembly_id = "vicreg_v1";
  report.context_contract = "graph_order.channel_node_representation.v1";
  report.output_contract = "graph_order.channel_node_future_distribution.v1";
  report.context_mode = "channel_context_strict";
  report.target_domain = "channel_node_future";
  report.graph_order_fingerprint = "checkpoint-identity-test";
  report.node_ids = {"BTC"};
  report.target_coords = {0, 1};
  report.channel_count = 1;
  report.context_dim = 4;
  report.future_horizon = 1;
  report.mixture_count = 2;
  report.hidden_width = 8;
  report.residual_depth = 1;
  report.feature_embedding_dim = 2;
  report.channel_adapter_rank = 2;
  report.sigma_min = 0.001;
  report.sigma_max = 0.0;
  report.eps = 0.000001;
  report.optimizer_steps = 2;

  auto mdn = mdn::ChannelContextMdn(/*context_dim=*/4, /*target_dim=*/2,
                                    /*channel_count=*/1,
                                    /*horizon_count=*/1,
                                    /*mixture_count=*/2,
                                    /*hidden_width=*/8,
                                    /*residual_depth=*/1);
  mdn::channel_context_mdn_train_model_t model(std::move(mdn), 0.001);
  model.set_optimizer_step_index(7);
  inference_launcher::channel_graph_first_inference_launcher_detail::
      save_channel_mdn_checkpoint_file(checkpoint_path, report, model);

  auto restore = mdn::ChannelContextMdn(
      /*context_dim=*/4, /*target_dim=*/2, /*channel_count=*/1,
      /*horizon_count=*/1, /*mixture_count=*/2, /*hidden_width=*/8,
      /*residual_depth=*/1);
  auto expected =
      inference_launcher::channel_graph_first_inference_launcher_detail::
          checkpoint_identity_from_report(report);
  int64_t restored_optimizer_step_index = -1;
  inference_launcher::channel_graph_first_inference_launcher_detail::
      load_channel_mdn_checkpoint_file(
          checkpoint_path, restore, nullptr, &expected,
          &restored_optimizer_step_index);
  check(restored_optimizer_step_index == 7,
        "MDN checkpoint restores the train wrapper optimizer step index");

  const auto legacy_counter_path = dir / "legacy_optimizer_counter.pt";
  torch::serialize::OutputArchive legacy_counter_archive;
  legacy_counter_archive.write(
      "meta/optimizer_steps",
      torch::tensor({5}, torch::TensorOptions().dtype(torch::kInt64)));
  legacy_counter_archive.save_to(legacy_counter_path.string());
  torch::serialize::InputArchive legacy_counter_input;
  legacy_counter_input.load_from(legacy_counter_path.string(),
                                 torch::Device(torch::kCPU));
  check(inference_launcher::channel_graph_first_inference_launcher_detail::
            read_channel_mdn_optimizer_step_index(legacy_counter_input) == 5,
        "MDN checkpoint loader falls back to the legacy optimizer step total");

  expected.input_representation_assembly_id = "other_channel_representation";
  bool threw = false;
  try {
    inference_launcher::channel_graph_first_inference_launcher_detail::
        load_channel_mdn_checkpoint_file(checkpoint_path, restore, nullptr,
                                         &expected);
  } catch (const std::exception &ex) {
    threw = std::string(ex.what()).find("input_representation_assembly_id") !=
            std::string::npos;
  }
  check(threw, "MDN checkpoint rejects mismatched input representation id");
}

void test_channel_mdn_loads_mdn_checkpoint() {
  torch::manual_seed(114);
  const auto rep_fixture =
      make_config_fixture("mdn_checkpoint_restore_representation",
                          /*max_steps=*/1, /*checkpoint_every=*/1,
                          /*batch_size=*/2, /*report_every=*/1, "train");
  auto rep_pipe = make_pipeline(rep_fixture);
  representation_launcher::channel_graph_first_representation_launcher_options_t
      rep_options{};
  rep_options.write_report = true;
  rep_options.report_path = rep_fixture.report;
  representation_launcher::channel_graph_first_representation_launcher_t<Kline>
      rep_launcher(std::move(rep_pipe), rep_options);
  const auto rep_report = rep_launcher.run();
  check(rep_report.checkpoint_written,
        "MDN checkpoint restore representation checkpoint written");

  const auto source_mdn_fixture =
      make_config_fixture("mdn_checkpoint_restore_source", /*max_steps=*/1,
                          /*checkpoint_every=*/1, /*batch_size=*/2,
                          /*report_every=*/1, "train");
  configure_channel_mdn_checkpoints(source_mdn_fixture,
                                    rep_report.checkpoint_path);
  auto source_mdn_pipe = make_pipeline(source_mdn_fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      source_mdn_options{};
  source_mdn_options.write_report = true;
  source_mdn_options.report_path = source_mdn_fixture.report;
  inference_launcher::channel_graph_first_inference_launcher_t<Kline>
      source_mdn_launcher(std::move(source_mdn_pipe), source_mdn_options);
  const auto source_mdn_report = source_mdn_launcher.run();
  check(source_mdn_report.checkpoint_written, "source MDN checkpoint written");
  check(std::filesystem::exists(source_mdn_report.checkpoint_path),
        "source MDN checkpoint exists");

  const auto restore_mdn_fixture =
      make_config_fixture("mdn_checkpoint_restore_consumer", /*max_steps=*/1,
                          /*checkpoint_every=*/0, /*batch_size=*/2,
                          /*report_every=*/1, "train");
  configure_channel_mdn_checkpoints(restore_mdn_fixture,
                                    rep_report.checkpoint_path,
                                    source_mdn_report.checkpoint_path);
  auto restore_mdn_pipe = make_pipeline(restore_mdn_fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      restore_mdn_options{};
  restore_mdn_options.write_report = true;
  restore_mdn_options.report_path = restore_mdn_fixture.report;
  inference_launcher::channel_graph_first_inference_launcher_t<Kline>
      restore_mdn_launcher(std::move(restore_mdn_pipe), restore_mdn_options);
  const auto restore_mdn_report = restore_mdn_launcher.run();
  check(restore_mdn_report.representation_checkpoint_loaded,
        "restore MDN loaded representation checkpoint");
  check(restore_mdn_report.mdn_checkpoint_loaded,
        "restore MDN loaded MDN checkpoint");
  check(restore_mdn_report.steps_completed > 0,
        "restore MDN completed after checkpoint load");
  const auto report_text = read_text(restore_mdn_fixture.report);
  check(report_text.find("mdn_checkpoint_loaded=true") != std::string::npos,
        "MDN report records MDN checkpoint load");
}

void test_channel_mdn_checkpoint_identity_rejects_nll_mismatch() {
  torch::manual_seed(115);
  const auto rep_fixture =
      make_config_fixture("mdn_nll_mismatch_representation",
                          /*max_steps=*/1, /*checkpoint_every=*/1,
                          /*batch_size=*/2, /*report_every=*/1, "train");
  auto rep_pipe = make_pipeline(rep_fixture);
  representation_launcher::channel_graph_first_representation_launcher_options_t
      rep_options{};
  rep_options.write_report = true;
  rep_options.report_path = rep_fixture.report;
  representation_launcher::channel_graph_first_representation_launcher_t<Kline>
      rep_launcher(std::move(rep_pipe), rep_options);
  const auto rep_report = rep_launcher.run();
  check(rep_report.checkpoint_written,
        "NLL mismatch source representation checkpoint written");

  const auto source_mdn_fixture =
      make_config_fixture("mdn_nll_mismatch_source", /*max_steps=*/1,
                          /*checkpoint_every=*/1, /*batch_size=*/2,
                          /*report_every=*/1, "train");
  configure_channel_mdn_checkpoints(source_mdn_fixture,
                                    rep_report.checkpoint_path);
  auto source_mdn_pipe = make_pipeline(source_mdn_fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      source_mdn_options{};
  source_mdn_options.write_report = true;
  source_mdn_options.report_path = source_mdn_fixture.report;
  inference_launcher::channel_graph_first_inference_launcher_t<Kline>
      source_mdn_launcher(std::move(source_mdn_pipe), source_mdn_options);
  const auto source_mdn_report = source_mdn_launcher.run();
  check(source_mdn_report.checkpoint_written,
        "NLL mismatch source MDN checkpoint written");

  const auto restore_mdn_fixture =
      make_config_fixture("mdn_nll_mismatch_consumer", /*max_steps=*/1,
                          /*checkpoint_every=*/0, /*batch_size=*/2,
                          /*report_every=*/1, "train");
  configure_channel_mdn_checkpoints(restore_mdn_fixture,
                                    rep_report.checkpoint_path,
                                    source_mdn_report.checkpoint_path);
  configure_channel_mdn_sigma_min(restore_mdn_fixture, "0.002");
  auto restore_mdn_pipe = make_pipeline(restore_mdn_fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      restore_mdn_options{};
  restore_mdn_options.write_report = true;
  restore_mdn_options.report_path = restore_mdn_fixture.report;
  inference_launcher::channel_graph_first_inference_launcher_t<Kline>
      restore_mdn_launcher(std::move(restore_mdn_pipe), restore_mdn_options);
  bool threw = false;
  try {
    (void)restore_mdn_launcher.run();
  } catch (const std::exception &ex) {
    threw = std::string(ex.what()).find("sigma_min") != std::string::npos;
  }
  check(threw, "MDN checkpoint rejects mismatched NLL sigma_min");
}

void test_channel_mdn_run_mode_loads_checkpoints_without_training() {
  torch::manual_seed(116);
  const auto rep_fixture =
      make_config_fixture("mdn_run_mode_representation",
                          /*max_steps=*/1, /*checkpoint_every=*/1,
                          /*batch_size=*/2, /*report_every=*/1, "train");
  auto rep_pipe = make_pipeline(rep_fixture);
  representation_launcher::channel_graph_first_representation_launcher_options_t
      rep_options{};
  rep_options.write_report = true;
  rep_options.report_path = rep_fixture.report;
  representation_launcher::channel_graph_first_representation_launcher_t<Kline>
      rep_launcher(std::move(rep_pipe), rep_options);
  const auto rep_report = rep_launcher.run();
  check(rep_report.checkpoint_written,
        "MDN run source representation checkpoint written");

  const auto source_mdn_fixture =
      make_config_fixture("mdn_run_mode_source_mdn", /*max_steps=*/1,
                          /*checkpoint_every=*/1, /*batch_size=*/2,
                          /*report_every=*/1, "train");
  configure_channel_mdn_checkpoints(source_mdn_fixture,
                                    rep_report.checkpoint_path);
  configure_channel_mdn_identity_conditioned_direct_readout(source_mdn_fixture);
  configure_channel_mdn_one_step_direct_readout_warmup(source_mdn_fixture);
  auto source_mdn_pipe = make_pipeline(source_mdn_fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      source_mdn_options{};
  source_mdn_options.write_report = true;
  source_mdn_options.report_path = source_mdn_fixture.report;
  inference_launcher::channel_graph_first_inference_launcher_t<Kline>
      source_mdn_launcher(std::move(source_mdn_pipe), source_mdn_options);
  const auto source_mdn_report = source_mdn_launcher.run();
  check(source_mdn_report.checkpoint_written,
        "MDN run source MDN checkpoint written");
  check(source_mdn_report.last_direct_edge_return_readout_warmup_active &&
            source_mdn_report
                    .last_direct_edge_return_readout_scheduled_nll_weight ==
                0.0,
        "MDN run source checkpoint is written after its one warmup step");

  const auto eval_fixture =
      make_config_fixture("mdn_run_mode_eval", /*max_steps=*/1,
                          /*checkpoint_every=*/1, /*batch_size=*/2,
                          /*report_every=*/1, "run");
  configure_channel_mdn_checkpoints(eval_fixture, rep_report.checkpoint_path,
                                    source_mdn_report.checkpoint_path);
  configure_channel_mdn_identity_conditioned_direct_readout(eval_fixture);
  configure_channel_mdn_one_step_direct_readout_warmup(eval_fixture);
  auto eval_pipe = make_pipeline(eval_fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      eval_options{};
  eval_options.write_report = true;
  eval_options.report_path = eval_fixture.report;
  using key_t = Kline::key_type_t;
  int64_t observed_inference_batches = 0;
  bool observed_future_keys = false;
  inference_launcher::channel_graph_first_inference_launcher_t<Kline>
      eval_launcher(
          std::move(eval_pipe), eval_options,
          [&](const mdn::MdnOut &out,
              const mdn_stream::channel_mdn_input_batch_t<key_t> &batch,
              const vicreg_stream::channel_representation_batch_t<key_t>
                  &representation_batch,
              int64_t wave_pulse_index) {
            ++observed_inference_batches;
            check(out.log_pi.defined() && out.log_pi.dim() == 5,
                  "MDN run batch observer receives distribution output");
            check(batch.anchor_keys.defined() && batch.anchor_keys.numel() > 0,
                  "MDN run batch observer receives channel batch identity");
            check(representation_batch.future_keys.defined(),
                  "MDN run batch observer receives representation future keys");
            check(wave_pulse_index > 0,
                  "MDN run batch observer receives wave pulse index");
            observed_future_keys = true;
          });
  const auto eval_report = eval_launcher.run();
  check(eval_report.target_action == "run", "MDN run action");
  check(eval_report.representation_checkpoint_loaded,
        "MDN run loaded representation checkpoint");
  check(eval_report.mdn_checkpoint_loaded, "MDN run loaded MDN checkpoint");
  check(eval_report.steps_attempted > 0, "MDN run attempted steps");
  check(eval_report.steps_completed > 0, "MDN run completed steps");
  check(eval_report.optimizer_steps == 0, "MDN run no optimizer steps");
  check(!eval_report.checkpoint_written, "MDN run no checkpoint write");
  check(eval_report.total_valid_target_count > 0, "MDN run valid targets");
  check(std::isfinite(eval_report.mean_loss), "MDN run mean loss finite");
  check(std::isfinite(eval_report.last_nll_loss),
        "MDN run records the raw evaluation NLL");
  check(!eval_report.last_direct_edge_return_readout_warmup_active,
        "MDN run restores the completed warmup step from checkpoint");
  check_close(
      eval_report.last_direct_edge_return_readout_scheduled_nll_weight, 0.25,
      1e-12, "MDN run uses the restored post-warmup NLL weight");
  check(std::isfinite(eval_report.last_edge_return_auxiliary_loss) &&
            std::isfinite(eval_report.last_direct_edge_return_readout_loss),
        "MDN run records both auxiliary objective terms");
  check_close(
      eval_report.last_loss,
      eval_report.last_nll_loss *
              eval_report
                  .last_direct_edge_return_readout_scheduled_nll_weight +
          eval_report.last_edge_return_auxiliary_loss +
          eval_report.last_direct_edge_return_readout_loss,
      1e-5,
      "MDN run loss uses training-equivalent scheduled objective accounting");
  check(eval_report.forecast_ev_valid_count ==
            eval_report.total_valid_target_count,
        "MDN run EV metric count matches valid targets");
  check(std::isfinite(eval_report.ev_mae), "MDN run EV MAE finite");
  check(std::isfinite(eval_report.ev_rmse), "MDN run EV RMSE finite");
  check(std::isfinite(eval_report.signed_error), "MDN run signed error finite");
  check(std::isfinite(eval_report.directional_accuracy) &&
            eval_report.directional_accuracy >= 0.0 &&
            eval_report.directional_accuracy <= 1.0,
        "MDN run directional accuracy finite bounded");
  check(eval_report.nonfinite_output_count == 0, "MDN run finite outputs");
  check(observed_inference_batches > 0,
        "MDN run invoked inference batch observer");
  check(observed_future_keys,
        "MDN run observer can see future realization keys for replay");
  const auto report_text = read_text(eval_fixture.report);
  check(report_text.find("target_action=run") != std::string::npos,
        "MDN run report action");
  check(report_text.find("representation_checkpoint_loaded=true") !=
            std::string::npos,
        "MDN run report representation load");
  check(report_text.find("mdn_checkpoint_loaded=true") != std::string::npos,
        "MDN run report MDN load");
  check(report_text.find("optimizer_steps=0") != std::string::npos,
        "MDN run report optimizer steps");
  check(report_text.find("checkpoint_written=false") != std::string::npos,
        "MDN run report checkpoint");
  check(report_text.find("forecast_ev_valid_count=") != std::string::npos,
        "MDN run report EV metric count");
  check(report_text.find("ev_mae=") != std::string::npos,
        "MDN run report EV MAE");
  check(report_text.find("ev_rmse=") != std::string::npos,
        "MDN run report EV RMSE");
  check(report_text.find("signed_error=") != std::string::npos,
        "MDN run report signed error");
  check(report_text.find("directional_accuracy=") != std::string::npos,
        "MDN run report directional accuracy");
}

void test_channel_mdn_zero_valid_targets_skip() {
  torch::manual_seed(107);
  const auto fixture = make_config_fixture("mdn_zero_targets", /*max_steps=*/2,
                                           /*checkpoint_every=*/0,
                                           /*batch_size=*/2,
                                           /*report_every=*/1, "train");
  auto pipe = make_pipeline(fixture);
  inference_launcher::channel_graph_first_inference_launcher_options_t
      options{};
  options.force_empty_targets_for_test = true;
  options.write_report = true;
  options.report_path = fixture.report;
  inference_launcher::channel_graph_first_inference_launcher_t<Kline> launcher(
      std::move(pipe), options);
  const auto report = launcher.run();
  check(report.all_target_masks_forced_empty,
        "zero-target fixture marked empty");
  check(report.steps_attempted > 0, "zero-target attempted steps");
  check(report.steps_completed == 0, "zero-target no completed steps");
  check(report.skipped_batches > 0, "zero-target skipped batches");
  check(report.optimizer_steps == 0, "zero-target no optimizer steps");
  check(report.total_valid_target_count == 0, "zero-target no valid targets");
  check(!report.checkpoint_written, "zero-target no checkpoint");
  check(report.report_written, "zero-target report written");
}

} // namespace

int main() {
  try {
    test_channel_representation_dry_run();
    test_channel_representation_training_run();
    test_channel_representation_debug_lls();
    test_channel_representation_run_mode_no_mutation();
    test_channel_mdn_training_run();
    test_channel_mdn_identity_conditioned_direct_readout_training_run();
    test_channel_mdn_debug_lls();
    test_channel_mdn_run_requires_mdn_checkpoint();
    test_channel_mdn_conditioned_affine_shadow_defaults_absent_and_rejects_train();
    test_channel_mdn_loads_representation_checkpoint();
    test_channel_mdn_rejects_representation_checkpoint_policy_mismatch();
    test_channel_mdn_rejects_representation_checkpoint_recency_mismatch();
    test_channel_mdn_checkpoint_identity_rejects_representation_id_mismatch();
    test_channel_mdn_loads_mdn_checkpoint();
    test_channel_mdn_checkpoint_identity_rejects_nll_mismatch();
    test_channel_mdn_run_mode_loads_checkpoints_without_training();
    test_channel_mdn_zero_valid_targets_skip();
    std::cout
        << "[Jkimyei ChannelGraphFirstLaunchers test] all checks passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "[Jkimyei ChannelGraphFirstLaunchers test] failure: "
              << ex.what() << "\n";
    return 1;
  }
}
