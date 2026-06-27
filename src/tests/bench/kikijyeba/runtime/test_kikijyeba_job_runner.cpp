#include "hero/lattice_hero/lattice/target/lattice_target_evaluator.h"
#include "hero/runtime_hero/runtime/job_runner.h"
#include "tests/bench/kikijyeba/test_support/canonical_protocol_fixture.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "kikijyeba/environment/policy/allocation.h"
#include "kikijyeba/environment/policy/baseline.h"
#include "kikijyeba/environment/run/experiment_runner.h"
#include "kikijyeba/environment/runtime/experiment_driver.h"
#include "kikijyeba/environment/runtime/replay_source.h"

#include <unistd.h>

namespace env = cuwacunu::kikijyeba::environment;
namespace exposure = cuwacunu::hero::lattice::exposure;
namespace replay = cuwacunu::kikijyeba::environment::replay;
namespace runtime = cuwacunu::hero::runtime;
namespace runtime_report = cuwacunu::hero::lattice::runtime_report;
namespace target = cuwacunu::hero::lattice::target;
namespace provenance = cuwacunu::kikijyeba::protocol::config_provenance;
namespace protocol_fixture = cuwacunu::tests::kikijyeba::protocol_fixture;
namespace types = cuwacunu::ujcamei::source::registry::types;

namespace {

using Kline = types::kline_t;

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::string describe_lattice_target_evaluation(
    const target::lattice_target_evaluation_t &evaluation) {
  std::string out = std::string("status=") +
                    target::lattice_target_status_name(evaluation.status);
  if (!evaluation.reasons.empty()) {
    out += "; reasons=";
    for (std::size_t i = 0; i < evaluation.reasons.size(); ++i) {
      if (i > 0) {
        out += ",";
      }
      out += evaluation.reasons[i];
    }
  }
  if (!evaluation.deficits.empty()) {
    out += "; deficits=";
    for (std::size_t i = 0; i < evaluation.deficits.size(); ++i) {
      if (i > 0) {
        out += ",";
      }
      out += evaluation.deficits[i].key + ":" + evaluation.deficits[i].message;
    }
  }
  return out;
}

std::string join_issue_tokens(const std::vector<std::string> &issues) {
  std::string out;
  for (std::size_t i = 0; i < issues.size(); ++i) {
    if (i > 0) {
      out += ",";
    }
    out += issues[i];
  }
  return out;
}

std::string join_causal_closure_keys(std::vector<std::string> keys) {
  std::sort(keys.begin(), keys.end());
  std::ostringstream out;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (i != 0) {
      out << ", ";
    }
    out << keys[i];
  }
  return out.str();
}

std::string expected_policy_no_lookahead_closure_digest(
    const std::string &contract_digest,
    std::vector<std::string> artifact_digests,
    std::vector<std::string> checkpoint_digests,
    std::vector<std::string> generation_digests, std::int64_t influence_end,
    bool influence_end_bound, std::int64_t availability_end,
    bool availability_end_bound, const std::string &embargo_policy_fingerprint,
    std::int64_t embargo_purged_window_anchor_begin,
    std::int64_t embargo_purged_window_anchor_end_exclusive,
    bool embargo_purged_window_anchor_range_bound) {
  std::ostringstream closure;
  closure << "policy_no_lookahead_provenance_closure.v1|" << contract_digest
          << "|" << join_causal_closure_keys(std::move(artifact_digests)) << "|"
          << join_causal_closure_keys(std::move(checkpoint_digests)) << "|"
          << join_causal_closure_keys(std::move(generation_digests)) << "|"
          << influence_end << "|" << (influence_end_bound ? "1" : "0") << "|"
          << availability_end << "|" << (availability_end_bound ? "1" : "0")
          << "|" << embargo_policy_fingerprint << "|"
          << embargo_purged_window_anchor_begin << "|"
          << embargo_purged_window_anchor_end_exclusive << "|"
          << (embargo_purged_window_anchor_range_bound ? "1" : "0");
  return exposure::exposure_digest_for_text(closure.str());
}

std::string digest_vector_for_fixture(const std::string &schema,
                                      const std::vector<std::string> &parts) {
  std::ostringstream text;
  text << schema;
  for (const auto &part : parts) {
    text << "|" << part;
  }
  return exposure::exposure_digest_for_text(text.str());
}

bool has_reason_containing(const std::vector<std::string> &reasons,
                           const std::string &needle) {
  for (const auto &reason : reasons) {
    if (reason.find(needle) != std::string::npos) {
      return true;
    }
  }
  return false;
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
  std::filesystem::path protocol_dsl{};
  std::filesystem::path cursor_dsl{};
  std::filesystem::path wave_dsl{};
  std::filesystem::path probes_dsl{};
  std::filesystem::path vicreg_jkimyei{};
  std::filesystem::path mtf_jkimyei{};
  std::filesystem::path channel_mdn_jkimyei{};
};

cuwacunu::kikijyeba::topology::graph::market_graph_t
make_runtime_fixture_market_graph() {
  cuwacunu::kikijyeba::topology::graph::market_graph_t graph{};
  graph.node_ids = {"BTC", "USDT", "ETH"};
  graph.edge_ids = {"BTCUSDT", "USDTBTC", "ETHUSDT", "USDTETH"};
  graph.base_index = {0, 1, 2, 1};
  graph.quote_index = {1, 0, 1, 2};
  graph.validate();
  return graph;
}

env::episode_spec_t make_runtime_replay_base_spec() {
  env::episode_spec_t spec{};
  spec.graph_node_ids = {"BTC", "USDT", "ETH"};
  spec.target_node_ids = {"USDT", "BTC", "ETH"};
  spec.base_policy = {.accounting_numeraire_id = "USDT",
                      .settlement_asset_id = "USDT",
                      .projection_reference_node_id = "USDT"};
  spec.initial_equity_numeraire = 1000.0;
  spec.constraints.max_weight = torch::full({3}, 0.60, torch::kFloat64);
  spec.constraints.max_turnover_l1 = 0.80;
  spec.constraints.lambda_cvar = 0.10;
  spec.constraints.lambda_concentration = 0.01;
  return spec;
}

env::replay_policy_factory_t make_runtime_replay_sdu_policy_factory() {
  return {
      .policy_id = env::kSpotDistributionalUtilityPolicyId,
      .policy_kind = env::policy_kind_t::deterministic_allocator,
      .make_policy =
          [](const replay::replay_episode_bundle_t &) {
            env::spot_distributional_utility_policy_config_t config{};
            config.constraints = make_runtime_replay_base_spec().constraints;
            config.solver_options.iterations = 24;
            config.solver_options.learning_rate = 0.05;
            return std::make_unique<env::spot_distributional_utility_policy_t>(
                config);
          },
  };
}

env::replay_policy_factory_t make_runtime_replay_numeraire_policy_factory() {
  return {
      .policy_id = "numeraire_only.v1",
      .policy_kind = env::policy_kind_t::baseline,
      .make_policy =
          [](const replay::replay_episode_bundle_t &bundle) {
            env::baseline_policy_config_t config{};
            config.policy_id = "numeraire_only.v1";
            config.node_ids = bundle.spec.target_node_ids;
            config.accounting_numeraire_node_id =
                bundle.spec.base_policy.accounting_numeraire_id;
            return std::make_unique<env::numeraire_only_policy_t>(
                std::move(config));
          },
  };
}

std::string canonical_cwu_02v_no_lookahead_contract_text() {
  return protocol_fixture::canonical_cwu_02v_no_lookahead_contract_block_text();
}

fixture_paths_t make_config_fixture(
    const std::string &label, const std::string &wave_range,
    const std::string &target = "wikimyei.inference.expected_value.mdn",
    const std::string &wave_mode = "run | debug",
    const std::string &protocol_id_override = {},
    const std::string &wave_protocol_override = {},
    bool job_events_enabled = false, bool include_probe_catalog = true) {
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
  out.protocol_dsl = out.dir / "kikijyeba.protocol.dsl";
  out.cursor_dsl = out.dir / "ujcamei.source.cursor.dsl";
  const auto source_splits_dsl = out.dir / "ujcamei.source.splits.dsl";
  const auto lattice_split_policy_dsl =
      out.dir / "hero.lattice.split_policy.dsl";
  out.wave_dsl = out.dir / "hero.runtime.wave.dsl";
  out.probes_dsl = out.dir / "hero.runtime.probes.dsl";
  const auto vicreg_dsl = out.dir / "wikimyei.representation.vicreg.dsl";
  const auto vicreg_net = out.dir / "wikimyei.representation.vicreg.net";
  const auto mtf_dsl =
      out.dir / "wikimyei.representation.mtf_jepa_mae_vicreg.dsl";
  const auto mtf_net =
      out.dir / "wikimyei.representation.mtf_jepa_mae_vicreg.net";
  const auto channel_mdn_dsl =
      out.dir / "wikimyei.inference.expected_value.mdn.dsl";
  const auto channel_mdn_net =
      out.dir / "wikimyei.inference.expected_value.mdn.net";
  const auto vicreg_jkimyei =
      out.dir / "wikimyei.representation.vicreg.jkimyei";
  const auto mtf_jkimyei =
      out.dir / "wikimyei.representation.mtf_jepa_mae_vicreg.jkimyei";
  const auto channel_mdn_jkimyei =
      out.dir / "wikimyei.inference.expected_value.mdn.jkimyei";
  out.vicreg_jkimyei = vicreg_jkimyei;
  out.mtf_jkimyei = mtf_jkimyei;
  out.channel_mdn_jkimyei = channel_mdn_jkimyei;
  out.config = out.dir / ".config";

  const bool target_is_vicreg =
      target == "wikimyei.representation.encoding.vicreg" ||
      target == "vicreg_representation";
  const std::string protocol_id =
      protocol_id_override.empty()
          ? (target_is_vicreg ? std::string{"cwu_01v"} : std::string{"cwu_02v"})
          : protocol_id_override;
  const bool protocol_uses_mtf = protocol_id == "cwu_02v";
  const std::string protocol_representation =
      protocol_uses_mtf ? "wikimyei.representation.encoding.mtf_jepa_mae_vicreg"
                        : "wikimyei.representation.encoding.vicreg";
  const std::string no_lookahead_contract =
      canonical_cwu_02v_no_lookahead_contract_text();

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

  write_text(out.wave_dsl,
             std::string("WAVE_SETTINGS {\n"
                         "  WAVE_ID = runtime_test_wave;\n"
                         "  PROTOCOL = ") +
                 (wave_protocol_override.empty() ? protocol_id
                                                 : wave_protocol_override) +
                 ";\n"
                 "  TARGET = " +
                 target +
                 ";\n"
                 "  MODE = " +
                 wave_mode +
                 ";\n"
                 "  SOURCE_CURSOR_ID = runtime_test_cursor;\n"
                 "};\n");
  write_text(
      out.probes_dsl,
      std::string("RUNTIME_PROBE {\n"
                  "  PROBE_ID = job_events;\n"
                  "  PROBE_KIND = job_events;\n"
                  "  ENABLED = ") +
          (job_events_enabled ? "true" : "false") +
          ";\n"
          "  RECORD_SCHEMA = kikijyeba.runtime.job_events.probe_record.v1;\n"
          "  STREAM_LEAF = runtime.job_events.probe;\n"
          "  EMIT_LIFECYCLE = true;\n"
          "  EMIT_SCALAR_METRICS = true;\n"
          "  EMIT_REPORT_METRICS = true;\n"
          "  EMIT_ARTIFACTS = true;\n"
          "  EMIT_WARNINGS = true;\n"
          "};\n");
  write_text(out.cursor_dsl,
             std::string("UJCAMEI_SOURCE_CURSOR {\n"
                         "  CURSOR_ID = runtime_test_cursor;\n"
                         "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                         "  SOURCE_CURSOR_SCOPE = wave_batch;\n") +
                 wave_range + "};\n");
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
  write_text(lattice_split_policy_dsl,
             "LATTICE_SPLIT_POLICY {\n"
             "  POLICY_ID = graph_anchor_holdout_v1;\n"
             "  SOURCE_SPLIT_CATALOG_ID = graph_anchor_splits_v1;\n"
             "  PURGE_LEFT_CONTEXT = auto_from_Hx;\n"
             "  PURGE_RIGHT_FUTURE = auto_from_Hf;\n"
             "};\n");

  write_text(out.protocol_dsl,
             "PROTOCOL {\n"
             "  PROTOCOL_ID = " +
                 protocol_id +
                 ";\n"
                 "  PROTOCOL_KIND = channel_graph_first;\n"
                 "  GRAPH_TOPOLOGY = kikijyeba.topology.graph;\n"
                 "  NODELIFT = wikimyei.expression.nodelift.srl;\n"
                 "  REPRESENTATION = " +
                 protocol_representation +
                 ";\n"
                 "  INFERENCE = wikimyei.inference.expected_value.mdn;\n"
                 "  OBSERVER = wikimyei.observer.belief;\n"
                 "  ALLOCATION_POLICY = "
                 "wikimyei.policy.portfolio.spot_distributional_utility;\n"
                 "  POLICY_COMPONENT = "
                 "wikimyei.policy.portfolio.graph_node_allocation;\n"
                 "  REPRESENTATION_CONTRACT = "
                 "graph_order.channel_node_representation.v1;\n"
                 "};\n" +
                 no_lookahead_contract);

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
  write_text(mtf_dsl,
             "MTF_JEPA_MAE_VICREG {\n"
             "  VERSION = wikimyei.representation.mtf_jepa_mae_vicreg.v1;\n"
             "  COMPONENT_ASSEMBLY_ID = mtf_jepa_mae_vicreg_v1;\n"
             "  INPUT_ROUTE = channel_node_stream;\n"
             "  CHANNEL_COUNT = 1;\n"
             "  HISTORY_LENGTH = 2;\n"
             "  INPUT_WIDTH = 9;\n"
             "  DTYPE = float32;\n"
             "  DEVICE = cpu;\n"
             "};\n");
  write_text(mtf_net, "MTF_JEPA_MAE_VICREG_NET {\n"
                      "  D_MODEL = 8;\n"
                      "  LATENT_DIM = 8;\n"
                      "  PROJECTOR_DIM = 8;\n"
                      "  PREDICTOR_HIDDEN_DIM = 8;\n"
                      "  NUM_ENCODER_LAYERS = 1;\n"
                      "  NUM_PREDICTOR_LAYERS = 1;\n"
                      "  NUM_DECODER_LAYERS = 1;\n"
                      "  NUM_HEADS = 2;\n"
                      "  TIME_SCALES = 2;\n"
                      "  SCALE_STRIDES = 1;\n"
                      "  USE_FREQUENCY_TOKENS = true;\n"
                      "  FREQUENCY_NUM_BINS = 4;\n"
                      "  FREQUENCY_LOG_MAGNITUDE = true;\n"
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
             "  TRAINING_ID = runtime_vicreg;\n"
             "  TASK = vicreg_representation;\n"
             "  COMPONENT_ASSEMBLY_ID = vicreg_v1;\n"
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
             "  TRAINING_VISIBILITY_POLICY = prior_generation_per_slice;\n"
             "  GENERATION_LANE_POLICY = "
             "readiness_grade_bootstrap_frozen_init_only;\n"
             "  VALID_FROM_POLICY = valid_from_anchor_gte_fit_end;\n"
             "  ARTIFACT_PROVENANCE_POLICY = "
             "transitive_influence_required;\n"
             "};\n");
  write_text(mtf_jkimyei,
             "TRAINING {\n"
             "  VERSION = wikimyei.representation.mtf_jepa_mae_vicreg."
             "jkimyei.v1;\n"
             "  TRAINING_ID = runtime_mtf_jepa_mae_vicreg;\n"
             "  TASK = mtf_jepa_mae_vicreg_representation;\n"
             "  COMPONENT_ASSEMBLY_ID = mtf_jepa_mae_vicreg_v1;\n"
             "  OPTIMIZER = adam;\n"
             "  LEARNING_RATE = 0.001;\n"
             "  MAX_STEPS = 0;\n"
             "  BATCH_SIZE = 2;\n"
             "  GRAD_CLIP_NORM = 5.0;\n"
             "  CHECKPOINT_EVERY = 0;\n"
             "  REPORT_EVERY = 1;\n"
             "  VALIDATION_EVERY = 0;\n"
             "  SEED = 17;\n"
             "  FREEZE_REPRESENTATION = false;\n"
             "  TRAINING_VISIBILITY_POLICY = prior_generation_per_slice;\n"
             "  GENERATION_LANE_POLICY = "
             "readiness_grade_bootstrap_frozen_init_only;\n"
             "  VALID_FROM_POLICY = valid_from_anchor_gte_fit_end;\n"
             "  ARTIFACT_PROVENANCE_POLICY = "
             "transitive_influence_required;\n"
             "  AUGMENTATION_PROFILE = runtime_fixture_light_v1;\n"
             "  DROPOUT = 0.0;\n"
             "  MASK_RATIO_TIME = 0.10;\n"
             "  MASK_RATIO_FREQUENCY = 0.05;\n"
             "  MASK_RATIO_CHANNEL = 0.0;\n"
             "  MIN_CONTEXT_RATIO = 0.75;\n"
             "  LAMBDA_JEPA = 1.0;\n"
             "  LAMBDA_MAE = 0.25;\n"
             "  LAMBDA_TF_ALIGN = 0.10;\n"
             "  LAMBDA_VICREG = 0.05;\n"
             "  LAMBDA_GLOBAL_VICREG = 0.25;\n"
             "  LAMBDA_CHANNEL_VICREG = 1.0;\n"
             "  VICREG_SIM_WEIGHT = 25.0;\n"
             "  VICREG_VAR_WEIGHT = 25.0;\n"
             "  VICREG_COV_WEIGHT = 1.0;\n"
             "  VICREG_VARIANCE_FLOOR = 1.0;\n"
             "  VICREG_VARIANCE_EPSILON = 0.0001;\n"
             "  TARGET_EMA_TAU = 0.990;\n"
             "  USE_TARGET_EMA = true;\n"
             "  STOP_GRADIENT_TARGET = true;\n"
             "  RETURN_DIAGNOSTICS = true;\n"
             "  USE_MAE_DECODER = true;\n"
             "  USE_JEPA_LOSS = true;\n"
             "  USE_TF_ALIGN_LOSS = true;\n"
             "  USE_VICREG_LOSS = true;\n"
             "  USE_GLOBAL_VICREG = true;\n"
             "  USE_CHANNEL_VICREG = false;\n"
             "  USE_RAW_RECONSTRUCTION_TARGETS = true;\n"
             "  STRICT_FINITE_LOSS = true;\n"
             "  COUPLE_TIME_FREQUENCY_MASKS = false;\n"
             "  MASK_SAME_WINDOW_ACROSS_DOMAINS = false;\n"
             "  MASK_SAME_CHANNEL_BLOCK = false;\n"
             "  MAX_CONTEXT_TARGET_TIME_OVERLAP = 0.50;\n"
             "  GAUSSIAN_JITTER_STD = 0.0;\n"
             "  FEATURE_DROPOUT_PROB = 0.0;\n"
             "  HISTORY_DROPOUT_PROB = 0.0;\n"
             "  TIME_CROP_JITTER_MAX = 0;\n"
             "  TIME_DILATION_MIN = 1.0;\n"
             "  TIME_DILATION_MAX = 1.0;\n"
             "  TIME_WARP_MAX = 0.0;\n"
             "  AMPLITUDE_SCALE_MIN = 1.0;\n"
             "  AMPLITUDE_SCALE_MAX = 1.0;\n"
             "  AMPLITUDE_SHIFT_STD = 0.0;\n"
             "  FREQUENCY_MASK_RATIO = 0.0;\n"
             "  FREQUENCY_JITTER_STD = 0.0;\n"
             "  PHASE_JITTER_MAX = 0.0;\n"
             "  CHANNEL_DROPOUT_PROB = 0.0;\n"
             "  CROSS_CHANNEL_DROPOUT_PROB = 0.0;\n"
             "  NODE_DROPOUT_PROB = 0.0;\n"
             "  EDGE_DROPOUT_PROB = 0.0;\n"
             "  MAGNITUDE_NORMALIZATION_NOISE_STD = 0.0;\n"
             "};\n");
  write_text(channel_mdn_jkimyei,
             "TRAINING {\n"
             "  VERSION = "
             "wikimyei.inference.expected_value.mdn.jkimyei.v1;\n"
             "  TRAINING_ID = runtime_channel_mdn;\n"
             "  TASK = mdn_expected_value_inference;\n"
             "  COMPONENT_ASSEMBLY_ID = mdn_v1;\n"
             "  OPTIMIZER = adam;\n"
             "  LEARNING_RATE = 0.01;\n"
             "  MAX_STEPS = 0;\n"
             "  BATCH_SIZE = 2;\n"
             "  GRAD_CLIP_NORM = 0.0;\n"
             "  CHECKPOINT_EVERY = 0;\n"
             "  REPORT_EVERY = 1;\n"
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

  const std::string probe_config_entries =
      include_probe_catalog ? std::string("runtime_probes_dsl_bnf_path = "
                                          "/cuwacunu/src/config/grammar/"
                                          "hero.runtime.probes.dsl.bnf\n"
                                          "runtime_probes_dsl_path = ") +
                                  out.probes_dsl.string() + "\n"
                            : std::string{};

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
          out.cursor_dsl.string() +
          "\n"
          "ujcamei_source_splits_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/ujcamei.source.splits.dsl.bnf\n"
          "ujcamei_source_splits_dsl_path = " +
          source_splits_dsl.string() +
          "\n"
          "\n"
          "[KIKIJYEBA]\n"
          "kikijyeba_protocol_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/kikijyeba.protocol.dsl.bnf\n"
          "kikijyeba_protocol_dsl_path = " +
          out.protocol_dsl.string() +
          "\n"
          "kikijyeba_topology_graph_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/kikijyeba.topology.graph.dsl.bnf\n"
          "kikijyeba_topology_graph_dsl_path = " +
          graph_dsl.string() +
          "\n"
          "[HERO]\n"
          "runtime_wave_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/hero.runtime.wave.dsl.bnf\n"
          "runtime_wave_dsl_path = " +
          out.wave_dsl.string() + "\n" + probe_config_entries +
          "lattice_split_policy_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/hero.lattice.split_policy.dsl.bnf\n"
          "lattice_split_policy_dsl_path = " +
          lattice_split_policy_dsl.string() +
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
          "wikimyei_representation_mtf_jepa_mae_vicreg_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.representation.mtf_jepa_mae_vicreg.dsl.bnf\n"
          "wikimyei_representation_mtf_jepa_mae_vicreg_dsl_path = " +
          mtf_dsl.string() +
          "\n"
          "wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.representation.mtf_jepa_mae_vicreg.net.bnf\n"
          "wikimyei_representation_mtf_jepa_mae_vicreg_net_path = " +
          mtf_net.string() +
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
          "\n"
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
          "\n\n"
          "[JKIMYEI]\n"
          "wikimyei_representation_vicreg_jkimyei_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.representation.vicreg.jkimyei.bnf\n"
          "wikimyei_representation_vicreg_jkimyei_path = " +
          vicreg_jkimyei.string() +
          "\n"
          "wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.representation.mtf_jepa_mae_vicreg.jkimyei.bnf\n"
          "wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path = " +
          mtf_jkimyei.string() +
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

std::size_t count_occurrences(std::string_view text, std::string_view needle) {
  if (needle.empty()) {
    return 0;
  }
  std::size_t count = 0;
  std::size_t pos = 0;
  while ((pos = text.find(needle, pos)) != std::string_view::npos) {
    ++count;
    pos += needle.size();
  }
  return count;
}

void configure_vicreg_train_smoke(const fixture_paths_t &fixture) {
  auto text = read_text(fixture.vicreg_jkimyei);
  replace_once(text, "  MAX_STEPS = 0;\n", "  MAX_STEPS = 1;\n");
  replace_once(text, "  CHECKPOINT_EVERY = 0;\n", "  CHECKPOINT_EVERY = 1;\n");
  write_text(fixture.vicreg_jkimyei, text);
}

void configure_mtf_train_smoke(const fixture_paths_t &fixture) {
  auto text = read_text(fixture.mtf_jkimyei);
  replace_once(text, "  MAX_STEPS = 0;\n", "  MAX_STEPS = 1;\n");
  replace_once(text, "  CHECKPOINT_EVERY = 0;\n", "  CHECKPOINT_EVERY = 1;\n");
  write_text(fixture.mtf_jkimyei, text);
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

void write_fixture_wave(const fixture_paths_t &fixture,
                        const std::string &wave_id,
                        const std::string &protocol_id,
                        const std::string &target, const std::string &mode,
                        const std::string &wave_range) {
  write_text(fixture.wave_dsl, std::string("WAVE_SETTINGS {\n"
                                           "  WAVE_ID = ") +
                                   wave_id +
                                   ";\n"
                                   "  PROTOCOL = " +
                                   protocol_id +
                                   ";\n"
                                   "  TARGET = " +
                                   target +
                                   ";\n"
                                   "  MODE = " +
                                   mode +
                                   ";\n"
                                   "  SOURCE_CURSOR_ID = runtime_test_cursor;\n"
                                   "};\n");
  write_text(fixture.cursor_dsl,
             std::string("UJCAMEI_SOURCE_CURSOR {\n"
                         "  CURSOR_ID = runtime_test_cursor;\n"
                         "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                         "  SOURCE_CURSOR_SCOPE = wave_batch;\n") +
                 wave_range + "};\n");
}

void rewrite_fixture_protocol(const fixture_paths_t &fixture,
                              const std::string &protocol_id,
                              const std::string &representation_family) {
  const std::string no_lookahead_contract =
      canonical_cwu_02v_no_lookahead_contract_text();
  write_text(fixture.protocol_dsl,
             std::string("PROTOCOL {\n"
                         "  PROTOCOL_ID = ") +
                 protocol_id +
                 ";\n"
                 "  PROTOCOL_KIND = channel_graph_first;\n"
                 "  GRAPH_TOPOLOGY = kikijyeba.topology.graph;\n"
                 "  NODELIFT = wikimyei.expression.nodelift.srl;\n"
                 "  REPRESENTATION = " +
                 representation_family +
                 ";\n"
                 "  INFERENCE = wikimyei.inference.expected_value.mdn;\n"
                 "  OBSERVER = wikimyei.observer.belief;\n"
                 "  ALLOCATION_POLICY = "
                 "wikimyei.policy.portfolio.spot_distributional_utility;\n"
                 "  POLICY_COMPONENT = "
                 "wikimyei.policy.portfolio.graph_node_allocation;\n"
                 "  REPRESENTATION_CONTRACT = "
                 "graph_order.channel_node_representation.v1;\n"
                 "};\n" +
                 no_lookahead_contract);
}

runtime::job_run_result_t
run_dry_job_in_fixture_runtime(const fixture_paths_t &fixture,
                               const std::string &job_leaf) {
  runtime::job_runner_options_t options{};
  options.job_kind = runtime::runtime_job_kind_t::channel_inference_mdn;
  options.dry_run = true;
  options.batch_size = 2;
  options.job_dir = fixture.dir / "runtime" / "components" / "test" / "spawns" /
                    "manual" / "jobs" / "run" / job_leaf;
  return runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
}

void test_component_spawn_identity_is_protocol_not_wave_scoped() {
  const auto fixture = make_config_fixture("spawn_identity_wave_boundary",
                                           "  SOURCE_RANGE = anchor_index;\n"
                                           "  ANCHOR_INDEX_BEGIN = 0;\n"
                                           "  ANCHOR_INDEX_END = 2;\n",
                                           "wikimyei.inference.expected_value."
                                           "mdn",
                                           "run | debug", "cwu_01v");

  provenance::component_spawn_tuple_t tuple{};
  tuple.component_family_id = "wikimyei.inference.expected_value.mdn";
  tuple.protocol_contract_fingerprint = "contract";
  tuple.graph_order_fingerprint = "graph";
  tuple.component_assembly_fingerprint = "mdn";
  const auto tuple_text = provenance::canonical_component_spawn_text(tuple);
  check(tuple.schema == "kikijyeba.component_spawn.v2",
        "component spawn tuple uses v2 schema");
  check(tuple_text.find("source_cursor_token") == std::string::npos,
        "component spawn v2 excludes source cursor token");

  const auto run_a = run_dry_job_in_fixture_runtime(fixture, "wave_a");
  check(run_a.manifest.component_spawn_schema == "kikijyeba.component_spawn.v2",
        "manifest records component spawn v2 schema");
  check(!run_a.manifest.component_operator_surface_digest.empty(),
        "component wave records operator surface digest");

  write_fixture_wave(fixture, "runtime_test_wave_eval_later", "cwu_01v",
                     "wikimyei.inference.expected_value.mdn", "run | debug",
                     "  SOURCE_RANGE = anchor_index;\n"
                     "  ANCHOR_INDEX_BEGIN = 1;\n"
                     "  ANCHOR_INDEX_END = 4;\n");
  const auto run_b = run_dry_job_in_fixture_runtime(fixture, "wave_b");
  check(run_a.manifest.source_cursor_token ==
            run_b.manifest.source_cursor_token,
        "graph-source cursor token is stable across wave slices");
  check(run_a.wave_plan.resolved_anchor_index_begin !=
            run_b.wave_plan.resolved_anchor_index_begin,
        "test waves use different resolved source slices");
  check(run_a.manifest.component_spawn_fingerprint ==
            run_b.manifest.component_spawn_fingerprint,
        "different wave ranges keep the same component spawn fingerprint");
  check(run_a.manifest.component_spawn_id == run_b.manifest.component_spawn_id,
        "different wave ranges keep the same scoped component spawn id");
  check(run_a.manifest.component_operator_surface_digest !=
            run_b.manifest.component_operator_surface_digest,
        "different wave ranges produce different component operator surfaces");

  write_fixture_wave(fixture, "runtime_test_wave_train_same_component",
                     "cwu_01v", "wikimyei.inference.expected_value.mdn",
                     "train | debug",
                     "  SOURCE_RANGE = anchor_index;\n"
                     "  ANCHOR_INDEX_BEGIN = 0;\n"
                     "  ANCHOR_INDEX_END = 3;\n");
  const auto train_c = run_dry_job_in_fixture_runtime(fixture, "wave_c");
  check(train_c.wave_plan.action == "train", "third wave is a train wave");
  check(run_a.manifest.component_spawn_fingerprint ==
            train_c.manifest.component_spawn_fingerprint,
        "train/run mode keeps the same component spawn fingerprint");
  check(run_a.manifest.component_operator_surface_digest !=
            train_c.manifest.component_operator_surface_digest,
        "train/run mode changes the component operator surface");

  rewrite_fixture_protocol(
      fixture, "cwu_02v",
      "wikimyei.representation.encoding.mtf_jepa_mae_vicreg");
  write_fixture_wave(fixture, "runtime_test_wave_mtf_protocol", "cwu_02v",
                     "wikimyei.inference.expected_value.mdn", "run | debug",
                     "  SOURCE_RANGE = anchor_index;\n"
                     "  ANCHOR_INDEX_BEGIN = 1;\n"
                     "  ANCHOR_INDEX_END = 4;\n");
  const auto mtf_protocol =
      run_dry_job_in_fixture_runtime(fixture, "wave_mtf_protocol");
  check(mtf_protocol.manifest.protocol_id == "cwu_02v",
        "protocol rewrite activates cwu_02v");
  check(run_a.manifest.component_spawn_fingerprint !=
            mtf_protocol.manifest.component_spawn_fingerprint,
        "different protocol versions produce different component spawns");
  check(run_a.manifest.component_operator_surface_digest !=
            mtf_protocol.manifest.component_operator_surface_digest,
        "different protocol versions produce different component operator "
        "surfaces");
}

void test_inference_dry_run_writes_manifest_and_state() {
  const auto fixture =
      make_config_fixture("inference_dry_run", "  SOURCE_RANGE = all;\n");
  const auto job_dir = fixture.dir / "job";
  runtime::job_runner_options_t options{};
  options.job_kind = runtime::runtime_job_kind_t::channel_inference_mdn;
  options.dry_run = true;
  options.batch_size = 2;
  options.job_dir = job_dir;
  options.runtime_handoff_id = "runtime_handoff_test_digest";
  options.runtime_handoff_digest = "test_digest";
  options.marshal_target_driver_run_id = "target_driver_test_run";
  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), options);

  check(result.manifest.job_kind == "channel_inference_mdn",
        "manifest job kind");
  check(result.manifest.protocol_id == "cwu_02v",
        "default inference fixture records cwu_02v protocol id");
  check(!result.manifest.config_bundle_id.empty(),
        "manifest records config bundle id");
  check(!result.manifest.config_receipt_id.empty(),
        "manifest records config receipt id");
  check(!result.manifest.component_spawn_registry_id.empty(),
        "manifest records component spawn registry id");
  check(result.manifest.component_family_id ==
            "wikimyei.inference.expected_value.mdn",
        "manifest records component family id");
  check(!result.manifest.component_spawn_fingerprint.empty(),
        "manifest records component spawn fingerprint");
  check(!result.manifest.component_operator_surface_digest.empty(),
        "manifest records component operator surface digest");
  check(!result.manifest.component_spawn_id.empty(),
        "manifest records scoped component spawn id");
  check(result.manifest.component_spawn_label ==
            result.manifest.component_family_id + "#" +
                result.manifest.component_spawn_id,
        "manifest records component spawn label");
  check(result.manifest.runtime_handoff_id == options.runtime_handoff_id,
        "manifest records runtime handoff id");
  check(result.manifest.runtime_handoff_digest ==
            options.runtime_handoff_digest,
        "manifest records runtime handoff digest");
  check(result.manifest.marshal_target_driver_run_id ==
            options.marshal_target_driver_run_id,
        "manifest records Marshal target-driver run id");
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
  check(result.wave_plan.target_component_family_id ==
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
  check(manifest_text.find("config_bundle_id=" +
                           result.manifest.config_bundle_id) !=
            std::string::npos,
        "manifest text records config bundle id");
  check(manifest_text.find("config_receipt_id=" +
                           result.manifest.config_receipt_id) !=
            std::string::npos,
        "manifest text records config receipt id");
  check(manifest_text.find("component_spawn_registry_id=" +
                           result.manifest.component_spawn_registry_id) !=
            std::string::npos,
        "manifest text records component spawn registry id");
  check(manifest_text.find("component_spawn_fingerprint=" +
                           result.manifest.component_spawn_fingerprint) !=
            std::string::npos,
        "manifest text records component spawn fingerprint");
  check(manifest_text.find("runtime_handoff_id=" +
                           options.runtime_handoff_id) != std::string::npos,
        "manifest text records Runtime handoff id");
  check(manifest_text.find("runtime_handoff_digest=" +
                           options.runtime_handoff_digest) != std::string::npos,
        "manifest text records Runtime handoff digest");
  check(manifest_text.find("marshal_target_driver_run_id=" +
                           options.marshal_target_driver_run_id) !=
            std::string::npos,
        "manifest text records Marshal target-driver run id");
  const auto runtime_root =
      runtime::job_runner_detail::runtime_root_for_job_dir(job_dir);
  const auto registry_path =
      provenance::component_spawn_registry_path(runtime_root);
  check(std::filesystem::exists(registry_path),
        "component spawn registry exists");
  const auto spawn_ref_path = runtime::job_layout::component_spawn_ref_path(
      runtime_root, result.manifest.component_family_id,
      result.manifest.component_spawn_id,
      result.manifest.component_spawn_fingerprint);
  check(std::filesystem::exists(spawn_ref_path), "component spawn ref exists");
  const auto registry_text = read_text(registry_path);
  check(registry_text.find(result.manifest.component_spawn_fingerprint) !=
            std::string::npos,
        "registry records component spawn fingerprint");
  check(registry_text.find(result.manifest.config_bundle_id) !=
            std::string::npos,
        "registry records config bundle link");
  const auto resolved_spawn = provenance::resolve_component_spawn_id(
      runtime_root, result.manifest.component_spawn_registry_id,
      result.manifest.component_family_id, result.manifest.component_spawn_id);
  check(resolved_spawn.resolved,
        "component spawn id resolves with full fingerprint");
  check(resolved_spawn.entry.component_spawn_fingerprint ==
            result.manifest.component_spawn_fingerprint,
        "resolved component spawn id preserves full fingerprint");
  const auto missing_spawn = provenance::resolve_component_spawn_id(
      runtime_root, result.manifest.component_spawn_registry_id,
      result.manifest.component_family_id, "notaspawn");
  check(!missing_spawn.resolved,
        "component spawn resolver fails closed on missing id");
  const auto receipt_a =
      provenance::capture_config_bundle_receipt(fixture.config, "a");
  const auto receipt_b =
      provenance::capture_config_bundle_receipt(fixture.config, "b");
  check(receipt_a.config_bundle_id == receipt_b.config_bundle_id,
        "identical config bundle captures have stable bundle id");
  check(receipt_a.config_receipt_id != receipt_b.config_receipt_id,
        "distinct config bundle captures have distinct receipt ids");
  check(manifest_text.find("dock_binding_fingerprint=") != std::string::npos,
        "manifest records dock fingerprint");
  check(manifest_text.find("protocol_contract_fingerprint=") !=
            std::string::npos,
        "manifest records protocol contract fingerprint");
  check(manifest_text.find("protocol_contract_token=") != std::string::npos,
        "manifest records protocol contract token");
  check(manifest_text.find("protocol_id=cwu_02v") != std::string::npos,
        "manifest records protocol id");
  check(manifest_text.find("job_kind=channel_inference_mdn") !=
            std::string::npos,
        "manifest records inference kind");
  check(manifest_text.find(
            "target_component_family_id=wikimyei.inference.expected_"
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
                           "encoding.mtf_jepa_mae_vicreg") != std::string::npos,
        "manifest records frozen representation dependency");
  const auto state_text = read_text(job_dir / "job.state");
  check(state_text.find("status=dry_run") != std::string::npos,
        "state records dry-run status");
  check(state_text.find(
            "target_component_family_id=wikimyei.inference.expected_value."
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
  check(result.state.runtime_result_fact_written,
        "dry-run writes a Runtime terminal result fact");
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
  check(dry_run_exposure_text.find("config_bundle_id=" +
                                   result.manifest.config_bundle_id) !=
            std::string::npos,
        "dry-run exposure fact records config bundle id");
  check(dry_run_exposure_text.find(
            "component_spawn_fingerprint=" +
            result.manifest.component_spawn_fingerprint) != std::string::npos,
        "dry-run exposure fact records component spawn fingerprint");
  check(dry_run_exposure_text.find(
            "component_operator_surface_digest=" +
            result.manifest.component_operator_surface_digest) !=
            std::string::npos,
        "dry-run exposure fact records component operator surface digest");
  check(dry_run_exposure_text.find("runtime_handoff_digest=" +
                                   options.runtime_handoff_digest) !=
            std::string::npos,
        "dry-run exposure fact records Runtime handoff digest");
  const auto dry_run_result_fact = read_text(job_dir / "runtime.result.fact");
  check(dry_run_result_fact.find("runtime_handoff_id=" +
                                 options.runtime_handoff_id) !=
            std::string::npos,
        "Runtime terminal fact records Runtime handoff id");
  check(dry_run_result_fact.find(
            "component_operator_surface_digest=" +
            result.manifest.component_operator_surface_digest) !=
            std::string::npos,
        "Runtime terminal fact records component operator surface digest");
  check(dry_run_result_fact.find("runtime_handoff_digest=" +
                                 options.runtime_handoff_digest) !=
            std::string::npos,
        "Runtime terminal fact records Runtime handoff digest");
  check(dry_run_result_fact.find("marshal_target_driver_run_id=" +
                                 options.marshal_target_driver_run_id) !=
            std::string::npos,
        "Runtime terminal fact records Marshal target-driver run id");
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

void test_probe_catalog_sidecar_writes_visibility_records() {
  const auto fixture = make_config_fixture(
      "probe_sidecar", "  SOURCE_RANGE = all;\n",
      "wikimyei.inference.expected_value.mdn", "run | debug", {}, {}, true);
  const auto job_dir = fixture.dir / "job";
  runtime::job_runner_options_t options{};
  options.job_kind = runtime::runtime_job_kind_t::channel_inference_mdn;
  options.dry_run = true;
  options.batch_size = 2;
  options.job_dir = job_dir;

  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  const auto stream_path =
      job_dir / runtime::job_events_probe::k_job_events_probe_stream_leaf;
  check(result.manifest.probe_sidecar_enabled,
        "probe catalog enables sidecar in manifest");
  check(result.state.probe_sidecar_enabled,
        "probe catalog enables sidecar in state");
  check(result.state.probe_records_written,
        "probe sidecar writes at least one record");
  check(result.state.probe_record_error.empty(),
        "probe sidecar reports no write error");
  check(result.state.probe_record_count >= 8,
        "probe sidecar writes lifecycle, metric, and artifact records");
  check(result.state.probe_stream_path == stream_path.string(),
        "state records probe stream path");
  check(std::filesystem::exists(stream_path), "probe stream file exists");

  const auto manifest_text = read_text(result.manifest_path);
  check(manifest_text.find("probe_sidecar_enabled=true") != std::string::npos,
        "manifest text records probe sidecar enabled");
  check(manifest_text.find("probe_record_schema="
                           "kikijyeba.runtime.job_events.probe_record.v1") !=
            std::string::npos,
        "manifest text records probe record schema");

  const auto state_text = read_text(result.state_path);
  check(state_text.find("probe_sidecar_enabled=true") != std::string::npos,
        "state text records probe sidecar enabled");
  check(state_text.find("probe_records_written=true") != std::string::npos,
        "state text records probe records written");
  check(state_text.find("probe_record_count=") != std::string::npos,
        "state text records probe record count");

  const auto stream_text = read_text(stream_path);
  check(stream_text.find("record_schema="
                         "kikijyeba.runtime.job_events.probe_record.v1") !=
            std::string::npos,
        "probe stream records schema");
  check(stream_text.find("authority=visibility_only") != std::string::npos,
        "probe records are visibility-only");
  check(stream_text.find("proof_authority=false") != std::string::npos,
        "probe records are not proof authority");
  check(stream_text.find("dispatch_authority=false") != std::string::npos,
        "probe records are not dispatch authority");
  check(stream_text.find("lattice_fact=false") != std::string::npos,
        "probe records are not lattice facts");
  check(stream_text.find("event_kind=job.lifecycle.started") !=
            std::string::npos,
        "probe stream records job start");
  check(stream_text.find("event_kind=job.progress.phase") != std::string::npos,
        "probe stream records progress phase events");
  check(stream_text.find("phase=delegate_start") != std::string::npos,
        "probe stream records delegate start progress");
  check(stream_text.find("phase=delegate_complete") != std::string::npos,
        "probe stream records delegate complete progress");
  check(stream_text.find("event_kind=job.lifecycle.dry_run") !=
            std::string::npos,
        "probe stream records dry-run completion");
  check(stream_text.find("event_kind=job.metric.scalar") != std::string::npos,
        "probe stream records scalar metrics");
  check(stream_text.find("event_kind=forecast.training.metric") !=
            std::string::npos,
        "probe stream records classified forecast training diagnostics");
  check(stream_text.find("phase=learning_diagnostics") != std::string::npos,
        "classified learning diagnostics use a stable probe phase");
  check(stream_text.find("metric_source=learning_diagnostics") !=
            std::string::npos,
        "classified learning diagnostics use a stable metric source");
  check(stream_text.find("series_id=forecast.training.metric."
                         "steps_completed") != std::string::npos,
        "classified learning diagnostics carry plot-ready series ids");
  check(stream_text.find("component_id=") != std::string::npos,
        "classified learning diagnostics carry component identity");
  check(stream_text.find("metric_name=steps_completed") != std::string::npos,
        "probe stream records step metrics");
  check(stream_text.find("series_id=job_state.steps_completed") !=
            std::string::npos,
        "probe metric events carry stable series ids");
  check(stream_text.find("step=0") != std::string::npos,
        "probe metric events carry chart step axis");
  check(stream_text.find("anchor_begin=0") != std::string::npos &&
            stream_text.find("anchor_end=4") != std::string::npos,
        "probe metric events carry accepted-anchor chart axes");
  check(stream_text.find("event_kind=job.artifact.published") !=
            std::string::npos,
        "probe stream records artifact publication");
  check(stream_text.find("artifact_kind=runtime_result_fact") !=
            std::string::npos,
        "probe stream records Runtime fact publication");
  check(stream_text.find("artifact_kind=lattice_exposure_fact") !=
            std::string::npos,
        "probe stream records Lattice sidecar publication");
  check(stream_text.find("protocol_contract_fingerprint=" +
                         result.manifest.protocol_contract_fingerprint) !=
            std::string::npos,
        "probe records bind protocol contract fingerprint");
  check(stream_text.find("dock_binding_fingerprint=" +
                         result.manifest.dock_binding_fingerprint) !=
            std::string::npos,
        "probe records bind dock fingerprint");
}

void test_probe_catalog_sidecar_writes_learning_timeseries_records() {
  const auto fixture = make_config_fixture(
      "probe_sidecar_learning_timeseries", "  SOURCE_RANGE = all;\n",
      "wikimyei.inference.expected_value.mdn", "train | debug", {}, {}, true);
  auto mdn_training = read_text(fixture.channel_mdn_jkimyei);
  replace_once(mdn_training, "  MAX_STEPS = 0;\n", "  MAX_STEPS = 2;\n");
  write_text(fixture.channel_mdn_jkimyei, mdn_training);

  const auto job_dir = fixture.dir / "job";
  runtime::job_runner_options_t options{};
  options.job_kind = runtime::runtime_job_kind_t::channel_inference_mdn;
  options.dry_run = false;
  options.batch_size = 2;
  options.job_dir = job_dir;

  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  const auto stream_path =
      job_dir / runtime::job_events_probe::k_job_events_probe_stream_leaf;
  check(result.state.status == "completed",
        "learning time-series probe fixture completes");
  check(std::filesystem::exists(stream_path),
        "learning time-series probe stream file exists");

  const auto stream_text = read_text(stream_path);
  check(count_occurrences(stream_text,
                          "series_id=forecast.training.metric.last_loss") >= 2,
        "learning diagnostics record repeated forecast loss samples");
  check(stream_text.find("step=1") != std::string::npos &&
            stream_text.find("step=2") != std::string::npos,
        "learning diagnostics record evolving chart steps");
  check(stream_text.find("event_kind=forecast.oracle.metric") !=
            std::string::npos,
        "learning diagnostics include forecast oracle metrics when available");
  check(stream_text.find("metric_source=learning_diagnostics") !=
            std::string::npos,
        "learning time-series records keep diagnostics metric source");
  check(stream_text.find("anchor_begin=0") != std::string::npos &&
            stream_text.find("anchor_end=4") != std::string::npos,
        "learning time-series records carry resolved accepted-anchor axes");
}

void test_learning_probe_fraction_range_uses_resolved_anchor_axes() {
  runtime::job_state_t state{};
  state.resolved_anchor_index_begin = 0;
  state.resolved_anchor_index_end = 730;

  const std::unordered_map<std::string, std::string> report{
      {"source_anchor_count", "1170"},
      {"source_range_start", "0"},
      {"source_range_end", "1170"},
      {"completed_anchor_start", "0"},
      {"completed_anchor_end", "1170"},
      {"requested_anchor_index_begin", "-1"},
      {"requested_anchor_index_end", "-1"},
  };

  check(runtime::job_events_probe::learning_anchor_begin(state, report) == "0",
        "fraction-range learning probes keep resolved anchor begin");
  check(runtime::job_events_probe::learning_anchor_end(state, report) == "730",
        "fraction-range learning probes keep resolved anchor end");

  state.resolved_anchor_index_begin = 760;
  state.resolved_anchor_index_end = 1088;
  check(runtime::job_events_probe::learning_anchor_begin(state, report) ==
                "760" &&
            runtime::job_events_probe::learning_anchor_end(state, report) ==
                "1088",
        "fraction-range eval probes do not fall back to full accepted range");
}

void test_missing_probe_config_does_not_attach_sidecar() {
  const auto fixture = make_config_fixture(
      "probe_config_missing", "  SOURCE_RANGE = all;\n",
      "wikimyei.inference.expected_value.mdn", "run", {}, {}, true, false);
  const auto job_dir = fixture.dir / "job";
  runtime::job_runner_options_t options{};
  options.job_kind = runtime::runtime_job_kind_t::channel_inference_mdn;
  options.dry_run = true;
  options.batch_size = 2;
  options.job_dir = job_dir;

  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  const auto stream_path =
      job_dir / runtime::job_events_probe::k_job_events_probe_stream_leaf;
  check(!result.manifest.probe_sidecar_enabled,
        "missing probe catalog path does not enable sidecar");
  check(!result.state.probe_sidecar_enabled,
        "missing probe catalog path leaves state sidecar disabled");
  check(!result.state.probe_records_written,
        "missing probe catalog path writes no probe records");
  check(!std::filesystem::exists(stream_path),
        "missing probe catalog path creates no probe stream");
}

void test_disabled_probe_catalog_writes_no_sidecar() {
  const auto fixture = make_config_fixture(
      "probe_catalog_disabled", "  SOURCE_RANGE = all;\n",
      "wikimyei.inference.expected_value.mdn", "run | debug", {}, {}, false);
  const auto job_dir = fixture.dir / "job";
  runtime::job_runner_options_t options{};
  options.job_kind = runtime::runtime_job_kind_t::channel_inference_mdn;
  options.dry_run = true;
  options.batch_size = 2;
  options.job_dir = job_dir;

  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  const auto stream_path =
      job_dir / runtime::job_events_probe::k_job_events_probe_stream_leaf;
  check(!result.manifest.probe_sidecar_enabled,
        "disabled probe catalog leaves manifest sidecar disabled");
  check(!result.state.probe_sidecar_enabled,
        "disabled probe catalog leaves state sidecar disabled");
  check(!std::filesystem::exists(stream_path),
        "disabled probe catalog creates no probe stream");
}

void test_probe_sidecar_requires_debug_wave() {
  const auto fixture = make_config_fixture(
      "probe_sidecar_requires_debug", "  SOURCE_RANGE = all;\n",
      "wikimyei.inference.expected_value.mdn", "run", {}, {}, true);
  runtime::job_runner_options_t options{};
  options.job_kind = runtime::runtime_job_kind_t::channel_inference_mdn;
  options.dry_run = true;
  options.batch_size = 2;
  options.job_dir = fixture.dir / "job";

  bool threw = false;
  try {
    (void)runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  } catch (const std::exception &ex) {
    threw = std::string(ex.what()).find(
                "Runtime probes require active WAVE_SETTINGS.MODE to "
                "include debug") != std::string::npos;
  }
  check(threw, "enabled probe catalog requires debug wave mode");
  check(!std::filesystem::exists(options.job_dir),
        "debug-gate failure does not create job directory");
}

void expect_probe_catalog_rejection(const std::string &dsl,
                                    const std::string &needle,
                                    const std::string &message) {
  bool threw = false;
  try {
    (void)runtime::probe_settings::decode_runtime_probe_catalog_from_dsl(dsl);
  } catch (const std::exception &ex) {
    threw = std::string(ex.what()).find(needle) != std::string::npos;
  }
  check(threw, message);
}

void test_probe_catalog_validation_rejects_bad_inputs() {
  expect_probe_catalog_rejection(
      "RUNTIME_PROBE {\n"
      "  PROBE_ID = job_events;\n"
      "  PROBE_KIND = job_events;\n"
      "  ENABLED = maybe;\n"
      "};\n",
      "ENABLED must be true or false",
      "probe catalog rejects invalid boolean values");

  expect_probe_catalog_rejection(
      "RUNTIME_PROBE {\n"
      "  PROBE_ID = job_events;\n"
      "  PROBE_KIND = job_events;\n"
      "  ENABLED = true;\n"
      "  STREAM_LEAF = ..;\n"
      "};\n",
      "STREAM_LEAF must not contain parent traversal",
      "probe catalog rejects parent traversal stream leaves");

  expect_probe_catalog_rejection(
      "RUNTIME_PROBE {\n"
      "  PROBE_ID = job_events_a;\n"
      "  PROBE_KIND = job_events;\n"
      "  ENABLED = true;\n"
      "};\n"
      "RUNTIME_PROBE {\n"
      "  PROBE_ID = job_events_b;\n"
      "  PROBE_KIND = job_events;\n"
      "  ENABLED = true;\n"
      "};\n",
      "multiple enabled job_events probes",
      "probe catalog rejects duplicate enabled sidecars");
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
  options.job_dir = fixture.dir / "job";
  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  check(result.manifest.job_kind == "channel_representation_vicreg",
        "representation manifest kind");
  check(result.wave_plan.target_component_family_id ==
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

void test_representation_dry_run_source_key_range() {
  const auto fixture = make_config_fixture("representation_source_key_range",
                                           "  SOURCE_RANGE = source_key;\n"
                                           "  SOURCE_KEY_BEGIN = 1002;\n"
                                           "  SOURCE_KEY_END = 1004;\n",
                                           "wikimyei.representation.encoding."
                                           "vicreg");
  runtime::job_runner_options_t options{};
  options.dry_run = true;
  options.batch_size = 2;
  options.job_dir = fixture.dir / "job";
  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  check(result.wave_plan.source_range_policy == "source_key",
        "source-key range keeps canonical policy");
  check(result.wave_plan.requested_source_key_begin.has_value() &&
            *result.wave_plan.requested_source_key_begin == 1002,
        "source-key range records requested begin");
  check(result.wave_plan.requested_source_key_end.has_value() &&
            *result.wave_plan.requested_source_key_end == 1004,
        "source-key range records requested end");
  check(result.wave_plan.resolved_anchor_index_begin == 1,
        "source-key range resolves begin to accepted anchor index");
  check(result.wave_plan.resolved_anchor_index_end == 3,
        "source-key range resolves end to accepted anchor index");
  check(result.wave_plan.first_anchor_key == "1002",
        "source-key range records first selected anchor key");
  check(result.wave_plan.last_anchor_key == "1003",
        "source-key range records last selected anchor key");
  check(result.wave_plan.observed_source_row_begin == 0,
        "source-key range preserves row-space proof footprint");
  check(result.wave_plan.observed_source_key_begin == "1001",
        "source-key range records observed source-key footprint");
  check(result.wave_plan.target_source_key_end == "1005",
        "source-key range records target source-key footprint");
  const auto manifest_text = read_text(result.manifest_path);
  check(manifest_text.find("source_range_policy=source_key") !=
            std::string::npos,
        "manifest records source-key range policy");
  check(manifest_text.find("requested_source_key_begin=1002") !=
            std::string::npos,
        "manifest records requested source-key begin");
  check(manifest_text.find("resolved_anchor_index_begin=1") !=
            std::string::npos,
        "manifest records resolved anchor-index begin");
  const auto state_text = read_text(result.state_path);
  check(state_text.find("requested_source_key_end=1004") != std::string::npos,
        "state records requested source-key end");
}

void test_source_range_launch_overlay() {
  const auto fixture = make_config_fixture("representation_range_overlay",
                                           "  SOURCE_RANGE = all;\n",
                                           "wikimyei.representation.encoding."
                                           "vicreg");
  runtime::job_runner_options_t options{};
  options.dry_run = true;
  options.batch_size = 2;
  options.job_dir = fixture.dir / "job";
  options.source_range_override_enabled = true;
  options.source_range_override = "anchor_index";
  options.anchor_index_begin_override = 1;
  options.anchor_index_end_override = 3;
  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  check(result.wave_plan.source_range_policy == "anchor_index",
        "launch overlay changes effective source range policy");
  check(result.wave_plan.requested_anchor_index_begin.has_value() &&
            *result.wave_plan.requested_anchor_index_begin == 1,
        "launch overlay records requested anchor begin");
  check(result.wave_plan.requested_anchor_index_end.has_value() &&
            *result.wave_plan.requested_anchor_index_end == 3,
        "launch overlay records requested anchor end");
  check(result.wave_plan.resolved_anchor_index_begin == 1 &&
            result.wave_plan.resolved_anchor_index_end == 3,
        "launch overlay resolves selected anchor range");
  const auto manifest_text = read_text(result.manifest_path);
  check(manifest_text.find("source_range_policy=anchor_index") !=
            std::string::npos,
        "manifest records launch overlay source range policy");
  check(manifest_text.find("resolved_anchor_index_begin=1") !=
            std::string::npos,
        "manifest records launch overlay resolved begin");
}

void test_train_mode_manifest_mutation_policy() {
  const auto fixture =
      make_config_fixture("train_mode_manifest", "  SOURCE_RANGE = all;\n",
                          "wikimyei.inference.expected_value.mdn", "train");
  runtime::job_runner_options_t options{};
  options.dry_run = true;
  options.batch_size = 2;
  options.job_dir = fixture.dir / "job";
  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  check(result.manifest.wave_action == "train",
        "manifest records train action");
  check(result.manifest.mutated_components ==
            "wikimyei.inference.expected_value.mdn",
        "train mode mutates only MDN target");
  check(result.manifest.frozen_components ==
            "wikimyei.representation.encoding.mtf_jepa_mae_vicreg",
        "MDN train mode freezes representation dependency");
  check(result.manifest.execution_chain.find(
            "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:"
            "run_frozen") != std::string::npos,
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
  check(representation_result.wave_plan.target_component_family_id ==
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

  const auto mtf_fixture = make_config_fixture(
      "mtf_channel_representation_target", "  SOURCE_RANGE = all;\n",
      "wikimyei.representation.encoding.mtf_jepa_mae_vicreg", "train");
  const auto mtf_job_dir = mtf_fixture.dir / "job";
  runtime::job_runner_options_t mtf_options{};
  mtf_options.dry_run = true;
  mtf_options.job_dir = mtf_job_dir;
  const auto mtf_result = runtime::run_graph_first_job<Kline>(
      mtf_fixture.config.string(), mtf_options);
  check(mtf_result.manifest.job_kind ==
            "channel_representation_mtf_jepa_mae_vicreg",
        "MTF representation manifest kind");
  check(mtf_result.manifest.protocol_id == "cwu_02v",
        "MTF representation fixture records cwu_02v protocol id");
  check(mtf_result.wave_plan.target_component_family_id ==
            "wikimyei.representation.encoding.mtf_jepa_mae_vicreg",
        "MTF representation target component");
  check(mtf_result.manifest.mutated_components ==
            "wikimyei.representation.encoding.mtf_jepa_mae_vicreg",
        "MTF train mode mutates only the MTF representation");
  check(mtf_result.manifest.execution_chain.find(
            "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:train") !=
            std::string::npos,
        "MTF representation execution chain");
  check(mtf_result.delegated_report_path.filename() ==
            "channel_representation.report",
        "MTF representation delegated report path");
  check(std::filesystem::exists(mtf_job_dir / "job.state"),
        "MTF representation dry-run state exists");

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
  check(inference_result.wave_plan.target_component_family_id ==
            "wikimyei.inference.expected_value.mdn",
        "channel inference target component");
  check(inference_result.manifest.mutated_components ==
            "wikimyei.inference.expected_value.mdn",
        "channel inference mutates only MDN");
  check(inference_result.manifest.frozen_components ==
            "wikimyei.representation.encoding.mtf_jepa_mae_vicreg",
        "channel inference freezes channel representation");
  check(inference_result.manifest.execution_chain.find(
            "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:"
            "run_frozen") != std::string::npos,
        "channel inference execution chain records frozen representation");
  check(inference_result.delegated_report_path.filename() ==
            "channel_inference.report",
        "channel inference delegated report path");
  check(std::filesystem::exists(inference_job_dir / "job.state"),
        "channel inference state exists");
}

void test_mtf_representation_runs_through_runtime() {
  const auto fixture = make_config_fixture(
      "mtf_representation_smoke", "  SOURCE_RANGE = all;\n",
      "wikimyei.representation.encoding.mtf_jepa_mae_vicreg", "train");
  configure_mtf_train_smoke(fixture);
  const auto job_dir = fixture.dir / "job";
  runtime::job_runner_options_t options{};
  options.job_dir = job_dir;
  options.force_rebuild_cache = true;
  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  check(result.manifest.job_kind ==
            "channel_representation_mtf_jepa_mae_vicreg",
        "MTF smoke job kind");
  check(result.manifest.protocol_id == "cwu_02v",
        "MTF smoke manifest records cwu_02v protocol id");
  check(result.state.status == "completed", "MTF smoke completed");
  check(result.state.optimizer_steps > 0, "MTF smoke optimizer step");
  check(result.state.checkpoint_written, "MTF smoke checkpoint written");
  check(std::filesystem::exists(result.state.checkpoint_path),
        "MTF smoke checkpoint exists");
  const auto report = read_text(result.delegated_report_path);
  check(report.find("report_schema_id="
                    "wikimyei.representation.mtf_jepa_mae_vicreg.report.v1") !=
            std::string::npos,
        "MTF report records schema identity");
  check(report.find("representation_architecture=mtf_jepa_mae_vicreg.v1") !=
            std::string::npos,
        "MTF report records architecture");
  check(report.find("primary_value_shape=[B_flat,De]") != std::string::npos &&
            report.find("channel_value_shape=[B_flat,C,De]") !=
                std::string::npos &&
            report.find("recommended_graph_restore_shape=[B,N,C,De]") !=
                std::string::npos,
        "MTF report records flattened graph shape contract");
  check(report.find("flattening_contract=anchor_node_flatten.v1") !=
            std::string::npos,
        "MTF report records flattening contract");
  check(report.find("checkpoint_digest_verified=true") != std::string::npos &&
            report.find("checkpoint_file_exists=true") != std::string::npos &&
            report.find("checkpoint_digest_reported=") != std::string::npos,
        "MTF report records verified checkpoint digest facts");
  check(report.find("market_readiness_claimed=false") != std::string::npos,
        "MTF report does not claim market readiness");
  check(report.find("finite_parameter_check=true") != std::string::npos,
        "MTF report records finite parameter check");
  check(report.find("loss_jepa_mean=") != std::string::npos,
        "MTF report records JEPA loss summary");
  check(report.find("valid_latent_rows=") != std::string::npos,
        "MTF report records valid latent rows");
  check(report.find("raw_embedding") == std::string::npos &&
            report.find("token_tensor") == std::string::npos &&
            report.find("per_token_mask") == std::string::npos &&
            report.find("forecast_claim") == std::string::npos,
        "MTF report avoids raw internals and forecast claims");
}

void test_protocol_wave_representation_mismatch_fails() {
  const auto fixture = make_config_fixture(
      "protocol_wave_mismatch", "  SOURCE_RANGE = all;\n",
      "wikimyei.representation.encoding.mtf_jepa_mae_vicreg", "train",
      "cwu_01v");
  runtime::job_runner_options_t options{};
  options.dry_run = true;
  options.job_dir = fixture.dir / "job";
  bool threw = false;
  try {
    (void)runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  } catch (const std::exception &ex) {
    threw = std::string(ex.what()).find(
                "active protocol docks VICReg but active wave targets "
                "MTF-JEPA-MAE-VICReg") != std::string::npos;
  }
  check(threw, "protocol/wave representation mismatch fails closed");
}

void test_wave_profile_protocol_mismatch_fails() {
  const auto fixture = make_config_fixture(
      "wave_profile_protocol_mismatch", "  SOURCE_RANGE = all;\n",
      "wikimyei.inference.expected_value.mdn", "run | debug", "cwu_01v",
      "cwu_02v");
  runtime::job_runner_options_t options{};
  options.dry_run = true;
  options.job_dir = fixture.dir / "job";
  bool threw = false;
  try {
    (void)runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  } catch (const std::exception &ex) {
    threw = std::string(ex.what()).find(
                "WAVE_SETTINGS.PROTOCOL cwu_02v does not match active "
                "protocol cwu_01v") != std::string::npos;
  }
  check(threw, "wave profile incompatible protocol fails closed");
}

void test_strict_channel_baseline_runs_through_runtime() {
  const auto representation_fixture =
      make_config_fixture("strict_channel_baseline_representation",
                          "  SOURCE_RANGE = anchor_index;\n"
                          "  ANCHOR_INDEX_BEGIN = 0;\n"
                          "  ANCHOR_INDEX_END = 1;\n",
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
  check(representation_result.state.runtime_result_fact_written,
        "strict baseline representation runtime result fact written");
  check(representation_result.state.runtime_checkpoint_io_fact_written,
        "strict baseline representation runtime checkpoint I/O fact written");
  check(representation_result.state.runtime_health_measurement_fact_written,
        "strict baseline representation runtime health fact written");
  const auto representation_result_fact =
      read_text(representation_job_dir / "runtime.result.fact");
  check(representation_result_fact.find("fact_type=runtime.result.fact") !=
            std::string::npos,
        "strict baseline representation result fact type");
  check(representation_result_fact.find("optimizer_steps=") !=
            std::string::npos,
        "strict baseline representation result fact carries optimizer steps");
  check(representation_result_fact.find("checkpoint_written=true") !=
            std::string::npos,
        "strict baseline representation result fact carries checkpoint write");
  check(representation_result_fact.find(
            "representation_effective_rank_fraction=") != std::string::npos,
        "strict baseline representation result fact carries geometry summary");
  check(representation_result_fact.find("source_report_digest=") !=
            std::string::npos,
        "strict baseline representation result fact carries source report "
        "digest");
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
      "strict_channel_baseline_inference",
      "  SOURCE_RANGE = anchor_index;\n"
      "  ANCHOR_INDEX_BEGIN = 0;\n"
      "  ANCHOR_INDEX_END = 1;\n",
      "wikimyei.inference.expected_value.mdn", "train", "cwu_01v");
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
  check(inference_result.state.runtime_result_fact_written,
        "strict baseline inference runtime result fact written");
  check(inference_result.state.runtime_checkpoint_io_fact_written,
        "strict baseline inference runtime checkpoint I/O fact written");
  check(inference_result.state.runtime_health_measurement_fact_written,
        "strict baseline inference runtime health fact written");
  const auto inference_result_fact =
      read_text(inference_job_dir / "runtime.result.fact");
  check(inference_result_fact.find("valid_target_fraction=") !=
            std::string::npos,
        "strict baseline inference result fact carries target support");
  check(inference_result_fact.find("grad_norm_max_pre_clip=") !=
            std::string::npos,
        "strict baseline inference result fact carries pre-clip grad norm");
  check(inference_result_fact.find("mixture_usage=") != std::string::npos,
        "strict baseline inference result fact carries mixture usage");
  check(inference_result_fact.find("checkpoint_written=true") !=
            std::string::npos,
        "strict baseline inference result fact carries checkpoint write");
  const auto inference_checkpoint_io_fact =
      read_text(inference_job_dir / "runtime.checkpoint_io.fact");
  check(inference_checkpoint_io_fact.find(
            "representation_checkpoint_loaded=true") != std::string::npos,
        "strict baseline inference checkpoint I/O fact records loaded "
        "representation");
  const auto inference_report =
      read_text(inference_result.delegated_report_path);
  check(inference_report.find("context_mode=channel_context_strict") !=
            std::string::npos,
        "strict baseline inference uses strict channel context");
  check(inference_report.find("context_contract="
                              "graph_order.channel_node_representation.v1") !=
            std::string::npos,
        "strict baseline inference records channel context contract");
  check(inference_report.find("context_value_shape=[B,N,C,De]") !=
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
                              "torch_archive_channel_mdn_v2") !=
            std::string::npos,
        "strict baseline inference checkpoint format");
  check(inference_report.find("mean_nll_per_channel=") != std::string::npos,
        "strict baseline inference emits per-channel NLL");
  check(inference_report.find("mean_nll_per_target_feature=") !=
            std::string::npos,
        "strict baseline inference emits per-target-feature NLL");
  check(inference_report.find("mean_nll_per_channel_target_feature=") !=
            std::string::npos,
        "strict baseline inference emits per-channel/target-feature NLL");
  check(inference_report.find("valid_target_count_per_channel=") !=
            std::string::npos,
        "strict baseline inference emits per-channel support");
  check(inference_report.find("valid_target_count_per_target_feature=") !=
            std::string::npos,
        "strict baseline inference emits per-feature support");
  check(inference_report.find("mdn_architecture=shared_slot_trunk."
                              "channel_adapter.shared_feature_head.v2") !=
            std::string::npos,
        "strict baseline inference emits MDN architecture");
  check(inference_report.find("loss_reduction=balanced_channel_feature_mean") !=
            std::string::npos,
        "strict baseline inference emits balanced loss reduction");
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
        "strict baseline inference writes MDN LLS sidecar");
  const auto channel_mdn_lls =
      read_text(inference_result.delegated_report_path.string() + ".mdn.lls");
  check(channel_mdn_lls.find(
            "wikimyei.inference.expected_value.mdn.runtime.v1") !=
            std::string::npos,
        "strict baseline inference LLS carries MDN schema");
  check(channel_mdn_lls.find("context_mask_fraction") != std::string::npos &&
            channel_mdn_lls.find("future_mask_fraction") != std::string::npos,
        "strict baseline inference LLS carries context/future mask fractions");
  check(channel_mdn_lls.find("sigma_mean_valid") != std::string::npos,
        "strict baseline inference LLS carries masked sigma mean");
  check(channel_mdn_lls.find(
            "loss_reduction:str = balanced_channel_feature_mean") !=
            std::string::npos,
        "strict baseline inference LLS carries balanced loss reduction");
  check(channel_mdn_lls.find("mdn_architecture:str = shared_slot_trunk."
                             "channel_adapter.shared_feature_head.v2") !=
            std::string::npos,
        "strict baseline inference LLS carries MDN architecture");
  const auto inference_exposure_fact =
      read_text(inference_result.state.lattice_exposure_fact_path);
  check(inference_exposure_fact.find(
            "target_component_family_id=wikimyei.inference."
            "expected_value.mdn") != std::string::npos,
        "strict baseline exposure fact keeps MDN component");
  check(inference_exposure_fact.find("input_representation_assembly_id="
                                     "vicreg_v1") != std::string::npos,
        "strict baseline exposure fact carries input representation id");
  check(inference_exposure_fact.find("context_mode=channel_context_strict") !=
            std::string::npos,
        "strict baseline exposure fact carries strict context mode");
  check(inference_exposure_fact.find("context_contract=graph_order."
                                     "channel_node_representation.v1") !=
            std::string::npos,
        "strict baseline exposure fact carries channel context contract");
  check(inference_exposure_fact.find("context_value_shape=[B,N,C,De]") !=
            std::string::npos,
        "strict baseline exposure fact carries channel context shape");
  check(inference_exposure_fact.find("output_contract=graph_order."
                                     "channel_node_future_distribution.v1") !=
            std::string::npos,
        "strict baseline exposure fact carries channel output contract");
  check(inference_exposure_fact.find(
            "mdn_architecture=shared_slot_trunk."
            "channel_adapter.shared_feature_head.v2") != std::string::npos,
        "strict baseline exposure fact carries MDN architecture");
  check(inference_exposure_fact.find(
            "loss_reduction=balanced_channel_feature_mean") !=
            std::string::npos,
        "strict baseline exposure fact carries balanced loss reduction");
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
  check(inference_exposure_fact.find("mean_nll_per_target_feature=") !=
            std::string::npos,
        "strict baseline exposure fact carries per-target-feature NLL");
  check(inference_exposure_fact.find("mean_mixture_usage=") !=
            std::string::npos,
        "strict baseline exposure fact carries mixture usage");

  const auto missing_mdn_eval_fixture = make_config_fixture(
      "strict_channel_eval_missing_mdn_checkpoint",
      "  SOURCE_RANGE = anchor_index;\n"
      "  ANCHOR_INDEX_BEGIN = 1;\n"
      "  ANCHOR_INDEX_END = 3;\n",
      "wikimyei.inference.expected_value.mdn", "run", "cwu_01v");
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
  check(missing_mdn_eval_state.find("runtime_result_fact_written=true") !=
            std::string::npos,
        "strict baseline missing-MDN eval writes runtime result fact");
  const auto missing_mdn_eval_result_fact =
      read_text(missing_mdn_eval_job_dir / "runtime.result.fact");
  check(missing_mdn_eval_result_fact.find("status=failed") != std::string::npos,
        "strict baseline missing-MDN eval result fact records failure");
  check(missing_mdn_eval_result_fact.find("INPUT_MDN_CHECKPOINT") !=
            std::string::npos,
        "strict baseline missing-MDN eval result fact records failure reason");
  check(!std::filesystem::exists(missing_mdn_eval_job_dir /
                                 "channel_inference.report"),
        "strict baseline missing-MDN eval does not delegate report");
  check(!std::filesystem::exists(missing_mdn_eval_job_dir /
                                 "lattice.exposure.fact"),
        "strict baseline missing-MDN eval does not write exposure fact");

  const auto eval_fixture = make_config_fixture(
      "strict_channel_baseline_eval",
      "  SOURCE_RANGE = source_key;\n"
      "  SOURCE_KEY_BEGIN = 1002;\n"
      "  SOURCE_KEY_END = 1004;\n",
      "wikimyei.inference.expected_value.mdn", "run", "cwu_01v");
  configure_channel_mdn_strict_eval(eval_fixture,
                                    representation_result.state.checkpoint_path,
                                    inference_result.state.checkpoint_path);
  const auto eval_job_dir = eval_fixture.dir / "job";
  runtime::job_runner_options_t eval_options{};
  eval_options.batch_size = 2;
  eval_options.job_dir = eval_job_dir;
  eval_options.force_rebuild_cache = true;
  eval_options.replay_accounting_numeraire_node_id = "USDT";
  eval_options.replay_target_node_ids = {"USDT", "BTC", "ETH"};
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
        "strict baseline eval resolves configured source-key range");
  check(eval_result.wave_plan.source_range_policy == "source_key" &&
            !eval_result.wave_plan.requested_anchor_index_begin.has_value() &&
            !eval_result.wave_plan.requested_anchor_index_end.has_value() &&
            eval_result.wave_plan.requested_source_key_begin.has_value() &&
            *eval_result.wave_plan.requested_source_key_begin == 1002 &&
            eval_result.wave_plan.requested_source_key_end.has_value() &&
            *eval_result.wave_plan.requested_source_key_end == 1004,
        "strict baseline eval preserves source-key request separately from "
        "resolved anchors");
  check(eval_result.manifest.source_range_policy == "source_key" &&
            eval_result.manifest.requested_source_key_begin == "1002" &&
            eval_result.manifest.requested_source_key_end == "1004",
        "strict baseline eval manifest records source-key request");
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
  check(eval_result.state.runtime_result_fact_written,
        "strict baseline eval runtime result fact written");
  check(eval_result.state.runtime_checkpoint_io_fact_written,
        "strict baseline eval runtime checkpoint I/O fact written");
  check(eval_result.state.runtime_health_measurement_fact_written,
        "strict baseline eval runtime health fact written");
  check(eval_result.state.replay_artifacts_written,
        "strict baseline eval writes replay artifacts");
  check(std::filesystem::exists(eval_result.state.replay_batch_index_path),
        "strict baseline eval replay batch index exists");
  auto runtime_replay_source =
      replay::make_runtime_graph_anchor_replay_bundle_source_from_job_dir<
          std::int64_t>("strict_channel_eval_runtime_replay_source",
                        eval_job_dir, make_runtime_fixture_market_graph(),
                        make_runtime_replay_base_spec());
  auto runtime_replay_bundle = runtime_replay_source.next_bundle();
  check(runtime_replay_bundle.has_value(),
        "strict baseline eval replay source emits bundle from job dir");
  check(
      !runtime_replay_bundle->spec.requested_range.anchor_index_begin
              .has_value() &&
          !runtime_replay_bundle->spec.requested_range.anchor_index_end
               .has_value() &&
          runtime_replay_bundle->spec.requested_range.source_key_begin ==
              1002 &&
          runtime_replay_bundle->spec.requested_range.source_key_end == 1004 &&
          runtime_replay_bundle->spec.accepted_range.anchor_index_begin == 1 &&
          runtime_replay_bundle->spec.accepted_range.anchor_index_end == 3 &&
          runtime_replay_bundle->spec.accepted_range.cursor
                  .source_range_policy == "source_key",
      "strict baseline eval replay bundle separates source-key request from "
      "accepted anchor cursor");
  check(runtime_replay_bundle->frames.size() == 2,
        "strict baseline eval replay bundle covers streamed MDN pulse");
  check(runtime_replay_bundle->frames[0]
            .observation.allocation_belief.has_value(),
        "strict baseline eval replay bundle attaches AllocationBelief");
  check(
      runtime_replay_bundle->frames[0].projected_log_return_scenarios.defined(),
      "strict baseline eval replay bundle attaches projected scenarios");
  runtime_replay_source.reset();
  env::replay_experiment_options_t runtime_replay_options{};
  runtime_replay_options.max_parallel_jobs = 1;
  runtime_replay_options.episode_options.max_steps = 8;
  auto runtime_replay_report = env::run_replay_experiment(
      "strict_channel_eval_runtime_replay_experiment", runtime_replay_source,
      std::vector{make_runtime_replay_numeraire_policy_factory(),
                  make_runtime_replay_sdu_policy_factory()},
      runtime_replay_options);
  check(runtime_replay_report.completed_count == 2,
        "strict baseline eval Runtime replay artifacts run numeraire and SDU "
        "policies");
  env::runtime_job_replay_driver_options_t replay_driver_options{};
  replay_driver_options.job_dir = eval_job_dir;
  replay_driver_options.config_path = eval_fixture.config.string();
  replay_driver_options.experiment_id =
      "strict_channel_eval_runtime_replay_driver";
  replay_driver_options.accounting_numeraire_node_id = "USDT";
  replay_driver_options.target_node_ids = {"USDT", "BTC", "ETH"};
  replay_driver_options.initial_equity_numeraire = 1000.0;
  replay_driver_options.max_node_weight = 0.60;
  replay_driver_options.max_turnover_l1 = 0.80;
  replay_driver_options.execution_profile_digest =
      "cajtucu.paper.validation.profile.digest.fixture";
  replay_driver_options.policy_set_digest =
      "kikijyeba.replay.validation.policy_set.digest.fixture";
  replay_driver_options.world_options.linear_transaction_cost_rate = 0.001;
  replay_driver_options.include_equal_weight_policy = true;
  replay_driver_options.experiment_options.max_parallel_jobs = 1;
  replay_driver_options.experiment_options.episode_options.max_steps = 8;
  auto replay_driver_result =
      env::run_runtime_job_replay_experiment(replay_driver_options);
  check(replay_driver_result.replay_bundle_count == 1,
        "strict baseline replay driver sees one Runtime pulse bundle");
  check(replay_driver_result.report.completed_count == 3,
        "strict baseline replay driver runs numeraire, equal-weight, and SDU "
        "policies");
  check(
      std::isfinite(replay_driver_result.report
                        .mean_total_transaction_cost_numeraire()) &&
          replay_driver_result.report.mean_total_transaction_cost_numeraire() >
              0.0,
      "strict baseline replay driver applies nonzero Cajtucu cost profile");
  check(replay_driver_result.report.cajtucu_invalid_trace_count() == 0,
        "strict baseline replay driver produces only valid Cajtucu traces");
  check(replay_driver_result.report.cajtucu_numeraire_fallback_pair_count() ==
            0,
        "strict baseline replay driver does not route through accounting "
        "numeraire fallback");
  check(std::filesystem::exists(replay_driver_result.report_path),
        "strict baseline replay driver writes experiment report");
  check(std::filesystem::exists(replay_driver_result.experience_trace_path),
        "strict baseline replay driver writes experience trace sidecar");
  check(std::filesystem::exists(replay_driver_result.experiment_index_path),
        "strict baseline replay driver writes experiment index");
  const auto replay_driver_report_text =
      read_text(replay_driver_result.report_path);
  check(replay_driver_report_text.find(
            "schema=kikijyeba.environment.replay.cajtucu_ready_experiment_"
            "artifact.v1") != std::string::npos,
        "strict baseline replay driver report carries Cajtucu-ready "
        "experiment schema");
  check(replay_driver_report_text.find("policy_summary_count=3") !=
            std::string::npos,
        "strict baseline replay driver report carries all validation policy "
        "summaries");
  check(replay_driver_report_text.find(
            "execution_profile_digest=cajtucu.paper.validation.profile.digest."
            "fixture") != std::string::npos &&
            replay_driver_report_text.find(
                "policy_set_digest=kikijyeba.replay.validation.policy_set."
                "digest.fixture") != std::string::npos,
        "strict baseline replay driver report binds validation profile and "
        "policy-set digests");
  check(replay_driver_report_text.find(
            "top_level_metric_scope=mean_over_completed_episode_reports_"
            "across_policies") != std::string::npos &&
            replay_driver_report_text.find(
                "policy_metric_scope=mean_over_completed_episode_reports_for_"
                "policy") != std::string::npos,
        "strict baseline replay driver report declares aggregate metric "
        "scopes");
  check(
      replay_driver_report_text.find("cajtucu_valid_trace_count=") !=
              std::string::npos &&
          replay_driver_report_text.find("cajtucu_invalid_trace_count=0") !=
              std::string::npos &&
          replay_driver_report_text.find(
              "cajtucu_numeraire_fallback_pair_count=0") != std::string::npos &&
          replay_driver_report_text.find(
              "cajtucu_missing_direct_pair_count=") != std::string::npos &&
          replay_driver_report_text.find("requested_notional_numeraire=") !=
              std::string::npos &&
          replay_driver_report_text.find("executed_notional_numeraire=") !=
              std::string::npos &&
          replay_driver_report_text.find("fee_cost_numeraire=") !=
              std::string::npos &&
          replay_driver_report_text.find("spread_cost_numeraire=") !=
              std::string::npos &&
          replay_driver_report_text.find("slippage_cost_numeraire=") !=
              std::string::npos &&
          replay_driver_report_text.find("total_transaction_cost_numeraire=") !=
              std::string::npos,
      "strict baseline replay driver report carries validation-grade "
      "Cajtucu cost and feasibility evidence");
  const auto replay_driver_trace_text =
      read_text(replay_driver_result.experience_trace_path);
  check(replay_driver_trace_text.find(
            "policy_comparison_0_cajtucu_valid_trace_count=") !=
                std::string::npos &&
            replay_driver_trace_text.find(
                "episode_0_transition_0_cajtucu_execution_trace_available="
                "true") != std::string::npos &&
            replay_driver_trace_text.find(
                "episode_0_transition_0_cajtucu_total_transaction_cost_"
                "numeraire=") != std::string::npos &&
            replay_driver_trace_text.find(
                "episode_0_transition_0_cajtucu_numeraire_fallback_pair_count="
                "0") != std::string::npos,
        "strict baseline replay driver experience trace carries "
        "per-transition Cajtucu validation evidence");
  check(replay_driver_report_text.find("time_law_expected_step_count=6") !=
                std::string::npos &&
            replay_driver_report_text.find(
                "time_law_observation_step_count=6") != std::string::npos &&
            replay_driver_report_text.find("time_law_action_step_count=6") !=
                std::string::npos &&
            replay_driver_report_text.find("time_law_execution_step_count=6") !=
                std::string::npos &&
            replay_driver_report_text.find(
                "time_law_realization_after_action_count=6") !=
                std::string::npos &&
            replay_driver_report_text.find(
                "time_law_future_observation_violation_count=0") !=
                std::string::npos &&
            replay_driver_report_text.find(
                "mixed_future_realization_key_count=0") != std::string::npos &&
            replay_driver_report_text.find(
                "projection_validation_step_count=6") != std::string::npos,
        "strict baseline replay driver report carries aggregate time-law and "
        "projection step evidence for parked replay environment audit");
  check(
      replay_driver_report_text.find("episode_0_accepted_cursor_kind="
                                     "graph_anchor") != std::string::npos &&
          replay_driver_report_text.find(
              "episode_0_requested_anchor_index_begin=-1") !=
              std::string::npos &&
          replay_driver_report_text.find(
              "episode_0_requested_source_key_begin=1002") !=
              std::string::npos &&
          replay_driver_report_text.find(
              "episode_0_accepted_batch_cursor_token=") != std::string::npos &&
          replay_driver_report_text.find(
              "episode_0_accepted_source_range_policy=source_key") !=
              std::string::npos &&
          replay_driver_report_text.find(
              "episode_0_accepted_anchor_index_begin=1") != std::string::npos &&
          replay_driver_report_text.find("episode_0_accepted_anchor_keys=") !=
              std::string::npos,
      "strict baseline replay driver report carries source-key request and "
      "accepted cursor evidence");
  check(replay_driver_report_text.find(
            "episode_1_allocation_target_node_weights=") != std::string::npos,
        "strict baseline replay driver report carries allocation target "
        "weights");
  check(replay_driver_report_text.find("episode_1_allocation_cvar_loss=") !=
            std::string::npos,
        "strict baseline replay driver report carries allocation CVaR");
  const auto replay_driver_index =
      env::read_runtime_replay_experiment_index(eval_job_dir);
  check(replay_driver_index.size() == 1,
        "strict baseline replay driver index records one experiment");
  check(replay_driver_index[0].experiment_id ==
            "strict_channel_eval_runtime_replay_driver",
        "strict baseline replay driver index records experiment id");
  check(replay_driver_index[0].policy_count == 3,
        "strict baseline replay driver index records policy count");
  check(replay_driver_index[0].completed_count == 3,
        "strict baseline replay driver index records completed count");
  check(replay_driver_index[0].report_path == replay_driver_result.report_path,
        "strict baseline replay driver index records report path");
  const auto eval_result_fact = read_text(eval_job_dir / "runtime.result.fact");
  check(eval_result_fact.find("optimizer_steps=0") != std::string::npos,
        "strict baseline eval result fact records zero optimizer steps");
  check(eval_result_fact.find("checkpoint_written=false") != std::string::npos,
        "strict baseline eval result fact records no checkpoint write");
  const auto eval_checkpoint_io_fact =
      read_text(eval_job_dir / "runtime.checkpoint_io.fact");
  check(eval_checkpoint_io_fact.find("representation_checkpoint_loaded=true") !=
            std::string::npos,
        "strict baseline eval checkpoint I/O fact records representation load");
  check(eval_checkpoint_io_fact.find("mdn_checkpoint_loaded=true") !=
            std::string::npos,
        "strict baseline eval checkpoint I/O fact records MDN load");
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
  check(eval_result.state.lattice_source_analytics_fact_written,
        "strict baseline eval writes source-analytics fact state");
  check(std::filesystem::exists(
            eval_result.state.lattice_source_analytics_fact_path),
        "strict baseline eval writes source-analytics fact sidecar");
  check(std::filesystem::exists(eval_job_dir / "lattice.target_transform.fact"),
        "strict baseline eval writes target-transform fact sidecar");
  check(
      std::filesystem::exists(eval_job_dir / "lattice.forecast_baseline.fact"),
      "strict baseline eval writes forecast-baseline fact sidecar");
  check(
      std::filesystem::exists(eval_job_dir /
                              "lattice.forecast_baseline.zero_return.fact") &&
          std::filesystem::exists(
              eval_job_dir / "lattice.forecast_baseline.moving_average.fact") &&
          std::filesystem::exists(
              eval_job_dir /
              "lattice.forecast_baseline.last_valid_channel.fact"),
      "strict baseline eval writes deterministic baseline family sidecars");
  check(std::filesystem::exists(eval_job_dir / "lattice.forecast_eval.fact"),
        "strict baseline eval writes forecast-eval fact sidecar");
  check(std::filesystem::exists(eval_job_dir / "lattice.observer_belief.fact"),
        "strict baseline eval writes observer-belief fact sidecar");
  check(
      !std::filesystem::exists(eval_job_dir / "lattice.allocation_engine.fact"),
      "strict baseline replay driver leaves allocation-policy Lattice sidecar "
      "to the Lattice workstream");

  exposure::exposure_scan_options_t eval_scan_options{};
  eval_scan_options.derive_replay_environment_facts = true;
  const auto eval_scan = exposure::scan_exposure_ledger_from_runtime_root(
      eval_job_dir, {}, eval_scan_options);
  check(eval_scan.ledger.source_analytics_facts().size() == 1,
        "strict baseline eval scan sees Runtime-emitted source-analytics fact");
  check(eval_scan.ledger.target_transform_facts().size() == 1,
        "strict baseline eval scan sees target-transform fact");
  check(eval_scan.ledger.forecast_baseline_facts().size() == 4,
        "strict baseline eval scan sees deterministic forecast-baseline "
        "family facts");
  check(eval_scan.ledger.forecast_eval_facts().size() == 1,
        "strict baseline eval scan sees forecast-eval fact");
  check(eval_scan.ledger.observer_belief_facts().size() == 1,
        "strict baseline eval scan sees observer-belief fact");
  check(eval_scan.ledger.replay_environment_facts().size() == 1,
        "strict baseline eval scan derives replay-environment fact from "
        "Runtime replay artifacts");
  check(eval_scan.ledger.allocation_engine_facts().empty(),
        "strict baseline eval scan does not derive allocation-policy fact from "
        "replay output");
  const auto &source_analytics_fact =
      eval_scan.ledger.source_analytics_facts().front();
  check(exposure::source_analytics_fact_issues(source_analytics_fact).empty(),
        "strict baseline emitted source-analytics fact remains visibility-only "
        "with source identity");
  check(std::abs(source_analytics_fact.sample_validity_fraction -
                 eval_result.wave_plan.accepted_anchor_fraction) < 1e-12 &&
            std::abs(source_analytics_fact.missingness_fraction -
                     (1.0 - eval_result.wave_plan.accepted_anchor_fraction)) <
                1e-12 &&
            source_analytics_fact.visibility_only &&
            !source_analytics_fact.readiness_authority &&
            !source_analytics_fact.coverage_authority &&
            !source_analytics_fact.leakage_authority &&
            !source_analytics_fact.contract_identity_authority,
        "strict baseline source-analytics fact binds source health metrics "
        "without readiness, coverage, leakage, or contract authority");
  check(exposure::target_transform_fact_issues(
            eval_scan.ledger.target_transform_facts().front())
            .empty(),
        "strict baseline emitted target-transform fact has complete contract");
  std::set<std::string> emitted_baseline_kinds;
  for (const auto &baseline_fact : eval_scan.ledger.forecast_baseline_facts()) {
    check(exposure::forecast_baseline_fact_issues(baseline_fact).empty(),
          "strict baseline emitted forecast-baseline fact has closed "
          "transform lineage");
    check(baseline_fact.metric_status == "deferred_v1" &&
              exposure::forecast_baseline_finite_metric_count(baseline_fact) ==
                  0,
          "strict baseline emits baseline identities as metric-deferred "
          "evidence until per-kind scoring is declared");
    emitted_baseline_kinds.insert(exposure::normalized_forecast_baseline_kind(
        baseline_fact.baseline_kind));
  }
  check(emitted_baseline_kinds.count("previous_value") == 1 &&
            emitted_baseline_kinds.count("zero_return") == 1 &&
            emitted_baseline_kinds.count("moving_average") == 1 &&
            emitted_baseline_kinds.count("last_valid_channel") == 1,
        "strict baseline emitted roadmap baseline kinds as catalog evidence");
  const auto &emitted_forecast_eval_fact =
      eval_scan.ledger.forecast_eval_facts().front();
  check(exposure::forecast_eval_fact_issues(emitted_forecast_eval_fact).empty(),
        "strict baseline emitted forecast-eval fact has closed checkpoint, "
        "transform, baseline, and selection audit lineage");
  check(exposure::artifact_influence_summary_declared(
            emitted_forecast_eval_fact.influence_summary) &&
            emitted_forecast_eval_fact.influence_summary.complete &&
            emitted_forecast_eval_fact.influence_summary
                .influence_anchor_end_exclusive_max_bound &&
            emitted_forecast_eval_fact.influence_summary
                    .influence_anchor_end_exclusive_max <=
                emitted_forecast_eval_fact.anchor_range.begin,
        "strict baseline emitted forecast-eval fact carries complete prior "
        "checkpoint no-lookahead provenance");
  check(
      !emitted_forecast_eval_fact.mean_nll_per_channel.empty() &&
          !emitted_forecast_eval_fact.mean_nll_per_target_feature.empty() &&
          !emitted_forecast_eval_fact.mean_nll_per_channel_target_feature
               .empty() &&
          !emitted_forecast_eval_fact.valid_target_count_per_channel.empty() &&
          !emitted_forecast_eval_fact.valid_target_count_per_target_feature
               .empty() &&
          !emitted_forecast_eval_fact
               .valid_target_count_per_channel_target_feature.empty() &&
          !emitted_forecast_eval_fact.mean_nll_per_horizon.empty() &&
          !emitted_forecast_eval_fact.valid_target_count_per_horizon.empty() &&
          emitted_forecast_eval_fact.baseline_fact_digests.size() == 4 &&
          emitted_forecast_eval_fact.visibility_only &&
          !emitted_forecast_eval_fact.quality_authority &&
          !emitted_forecast_eval_fact.performance_authority,
      "strict baseline emitted forecast-eval fact carries stratified NLL and "
      "support surfaces as visibility evidence only");
  check(exposure::observer_belief_fact_issues(
            eval_scan.ledger.observer_belief_facts().front())
            .empty(),
        "strict baseline emitted observer-belief fact has forecast and "
        "scenario lineage");
  const auto &emitted_replay_environment_fact =
      eval_scan.ledger.replay_environment_facts().front();
  const auto replay_environment_issues =
      exposure::replay_environment_fact_issues(emitted_replay_environment_fact);
  check(replay_environment_issues.empty(),
        "strict baseline emitted replay-environment fact has cost-aware "
        "Cajtucu, time-law, projection, and source cursor evidence: " +
            join_issue_tokens(replay_environment_issues));
  check(exposure::artifact_influence_summary_declared(
            emitted_replay_environment_fact.influence_summary) &&
            emitted_replay_environment_fact.influence_summary.complete &&
            emitted_replay_environment_fact.influence_summary
                    .influence_anchor_end_exclusive_max ==
                emitted_forecast_eval_fact.influence_summary
                    .influence_anchor_end_exclusive_max,
        "strict baseline emitted replay-environment fact inherits forecast "
        "no-lookahead provenance");

  const auto &eval_parent_exposure = eval_scan.ledger.facts().front();
  const auto forecast_eval_digest =
      exposure::forecast_eval_fact_digest(emitted_forecast_eval_fact);
  const auto observer_belief_digest = exposure::observer_belief_fact_digest(
      eval_scan.ledger.observer_belief_facts().front());
  const auto replay_environment_digest =
      exposure::replay_environment_fact_digest(emitted_replay_environment_fact);

  exposure::lattice_allocation_engine_fact_t allocation_fact{};
  exposure::populate_artifact_fact_identity(allocation_fact,
                                            eval_parent_exposure);
  allocation_fact.target_node_weights = "BTC:0.25,ETH:0.25,USDT:0.50";
  allocation_fact.accounting_numeraire_node_id = "USDT";
  allocation_fact.accounting_numeraire_node_source = "base_policy";
  allocation_fact.base_policy_accounting_numeraire_node_id = "USDT";
  allocation_fact.accounting_numeraire_node_graph_bound = true;
  allocation_fact.numeraire_weight = 0.50;
  allocation_fact.turnover = 0.10;
  allocation_fact.objective_terms =
      "post_execution_log_growth:0.70,cost:0.20,drawdown:0.10";
  allocation_fact.cvar_loss = 0.02;
  allocation_fact.transaction_cost_estimate = 0.001;
  allocation_fact.constraint_diagnostics = "contract_fixture_all_clear";
  allocation_fact.cap_diagnostics = "contract_fixture_caps_clear";
  allocation_fact.scenario_growth_floor_status = "not_applicable";
  allocation_fact.fallback_reasons = "none";
  allocation_fact.derisk_reasons = "none";
  allocation_fact.observer_belief_fact_digest = observer_belief_digest;
  allocation_fact.forecast_artifact_digest =
      emitted_forecast_eval_fact.forecast_artifact_digest;
  allocation_fact.base_policy_digest = "graph_node_allocation_base_policy";
  exposure::write_allocation_engine_fact_sidecar(
      eval_job_dir / "lattice.allocation_engine.fact", allocation_fact);

  const auto allocation_scan = exposure::scan_exposure_ledger_from_runtime_root(
      eval_job_dir, {}, eval_scan_options);
  check(allocation_scan.ledger.allocation_engine_facts().size() == 1,
        "strict baseline eval scan sees durable allocation-engine sidecar for "
        "PPO policy-training parent evidence");
  const auto &emitted_allocation_fact =
      allocation_scan.ledger.allocation_engine_facts().front();
  const auto allocation_issues =
      exposure::allocation_engine_fact_issues(emitted_allocation_fact);
  check(allocation_issues.empty(),
        "strict baseline allocation-engine sidecar has closed observer and "
        "numeraire lineage: " +
            join_issue_tokens(allocation_issues));
  const auto allocation_engine_digest =
      exposure::allocation_engine_fact_digest(emitted_allocation_fact);
  const auto representation_checkpoint_fact =
      exposure::find_checkpoint_fact_for_checkpoint_path(
          representation_result.state.checkpoint_path,
          emitted_forecast_eval_fact.evaluated_representation_checkpoint_digest,
          eval_parent_exposure.split_policy_fingerprint);
  const auto mdn_checkpoint_fact =
      exposure::find_checkpoint_fact_for_checkpoint_path(
          inference_result.state.checkpoint_path,
          emitted_forecast_eval_fact.evaluated_mdn_checkpoint_digest,
          eval_parent_exposure.split_policy_fingerprint);
  check(representation_checkpoint_fact.has_value() &&
            mdn_checkpoint_fact.has_value(),
        "strict baseline resolves parent representation and MDN checkpoint "
        "identity facts for PPO bundle provenance");

  exposure::lattice_policy_training_fact_t policy_training_fact{};
  exposure::populate_artifact_fact_identity(policy_training_fact,
                                            eval_parent_exposure);
  policy_training_fact.target_component_family_id =
      "wikimyei.policy.portfolio.graph_node_allocation";
  policy_training_fact.policy_id =
      "wikimyei.policy.portfolio.graph_node_allocation.v1";
  policy_training_fact.policy_kind = "ppo_policy_adapter.v1";
  policy_training_fact.policy_architecture_digest =
      "graph_node_allocation_contract_fixture_arch";
  policy_training_fact.training_config_digest =
      "graph_node_allocation_contract_fixture_training_config";
  policy_training_fact.training_range_digest = "range_train_digest";
  policy_training_fact.validation_range_digest = "range_validation_digest";
  policy_training_fact.test_range_digest = "range_test_digest";
  policy_training_fact.environment_contract_id =
      "kikijyeba.environment.replay.v1";
  policy_training_fact.observation_schema_digest =
      "kikijyeba.environment.policy_input.v1";
  policy_training_fact.action_schema_digest =
      "kikijyeba.environment.action.target_node_weights.v1";
  policy_training_fact.reward_contract_digest =
      "kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_"
      "drawdown.v1";
  policy_training_fact.policy_input_schema_id =
      "kikijyeba.environment.policy_input.v1";
  policy_training_fact.action_adapter_id = "target_node_weights_simplex.v1";
  policy_training_fact.action_distribution_id = "masked_dirichlet_simplex.v1";
  policy_training_fact.reward_contract_id =
      "kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_"
      "drawdown.v1";
  policy_training_fact.execution_profile_digest =
      eval_scan.ledger.replay_environment_facts()
          .front()
          .execution_profile_digest;
  policy_training_fact.ppo_policy_artifact_contract_id =
      "kikijyeba.runtime.ppo_policy_artifact_contract.v1";
  policy_training_fact.policy_family_id =
      "wikimyei.policy.portfolio.graph_node_allocation";
  policy_training_fact.policy_checkpoint_schema_id =
      "wikimyei.policy.portfolio.graph_node_allocation.ppo_checkpoint.v1";
  policy_training_fact.policy_dsl_digest = "policy_dsl_digest_fixture";
  policy_training_fact.policy_net_digest = "policy_net_digest_fixture";
  policy_training_fact.policy_input_feature_manifest_digest =
      "policy_input_feature_manifest_digest_fixture";
  policy_training_fact.policy_jkimyei_digest = "policy_jkimyei_digest_fixture";
  const auto canonical_no_lookahead =
      protocol_fixture::canonical_cwu_02v_no_lookahead_contract();
  policy_training_fact.policy_jkimyei_no_lookahead_contract_id =
      canonical_no_lookahead.contract_id;
  policy_training_fact.policy_jkimyei_no_lookahead_contract_digest =
      canonical_no_lookahead.contract_digest;
  policy_training_fact.policy_jkimyei_component_order_contract_id =
      canonical_no_lookahead.contract_id;
  policy_training_fact.policy_jkimyei_component_order_contract_digest =
      canonical_no_lookahead.contract_digest;
  policy_training_fact.policy_jkimyei_component_role = "policy";
  policy_training_fact.policy_jkimyei_serving_order_index = 2;
  policy_training_fact.policy_jkimyei_serving_order_index_bound = true;
  policy_training_fact.target_node_universe_digest =
      "target_node_universe_digest_fixture";
  policy_training_fact.action_distribution_config_digest =
      "action_distribution_config_digest_fixture";
  policy_training_fact.snapshot_family_digest =
      "snapshot_family_digest_fixture";
  policy_training_fact.training_schedule_mode =
      "causal_walk_forward_training.v1";
  policy_training_fact.causal_schedule_schema_id =
      "kikijyeba.runtime.policy_training_causal_schedule.v1";
  policy_training_fact.causal_schedule_digest = "causal_schedule_ppo_v0_digest";
  policy_training_fact.causal_schedule_cursor_key_kind = "numeric_anchor_index";
  policy_training_fact.causal_schedule_no_future_snapshot_use_source =
      "derived_from_artifact_fit_use_ledgers";
  policy_training_fact.embargo_policy_fingerprint = "embargo_policy_anchor_v1";
  policy_training_fact.embargo_purged_window_anchor_range.begin =
      policy_training_fact.anchor_range.begin;
  policy_training_fact.embargo_purged_window_anchor_range.end =
      policy_training_fact.anchor_range.begin;
  policy_training_fact.embargo_purged_window_anchor_range_bound = true;
  policy_training_fact.normalization_fit_range_digest =
      policy_training_fact.training_range_digest;
  policy_training_fact.replay_buffer_source_range_digest =
      policy_training_fact.training_range_digest;
  policy_training_fact.early_stopping_policy_digest =
      "early_stopping_validation_only_digest";
  policy_training_fact.hyperparameter_selection_policy_digest =
      "hyperparameter_selection_validation_only_digest";
  policy_training_fact.selector_split = "validation";
  policy_training_fact.selector_policy_digest =
      "selector_policy_validation_only_digest";
  policy_training_fact.parent_checkpoint_digest =
      "ppo_policy_training_parent_checkpoint_digest";
  policy_training_fact.checkpoint_digest =
      "ppo_policy_training_checkpoint_digest";
  policy_training_fact.snapshot_bundle_id =
      "snapshot_bundle_policy_ready_fixture";
  policy_training_fact.snapshot_bundle_selection_basis = "bundle_manifest";
  policy_training_fact.snapshot_bundle_no_lookahead_contract_id =
      canonical_no_lookahead.contract_id;
  policy_training_fact.snapshot_bundle_no_lookahead_contract_digest =
      canonical_no_lookahead.contract_digest;
  policy_training_fact.snapshot_bundle_component_order_contract_id =
      canonical_no_lookahead.contract_id;
  policy_training_fact.snapshot_bundle_component_order_contract_digest =
      canonical_no_lookahead.contract_digest;
  policy_training_fact.snapshot_bundle_protocol_contract_fingerprint =
      policy_training_fact.contract_fingerprint;
  policy_training_fact.snapshot_bundle_generation_vector_digest =
      "snapshot_bundle_generation_vector_digest_fixture";
  policy_training_fact.snapshot_bundle_valid_from_anchor =
      policy_training_fact.anchor_range.end;
  policy_training_fact.snapshot_bundle_valid_from_anchor_bound = true;
  policy_training_fact.snapshot_bundle_compatibility_closure_digest =
      "snapshot_bundle_compatibility_closure_digest_fixture";
  policy_training_fact.bundle_representation_component_id =
      representation_checkpoint_fact->component;
  policy_training_fact.bundle_representation_checkpoint_digest =
      representation_checkpoint_fact->checkpoint_file_digest;
  policy_training_fact.bundle_representation_generation_id =
      representation_checkpoint_fact->generation_id;
  policy_training_fact.bundle_representation_generation_vector_digest =
      representation_checkpoint_fact->generation_vector_member_digest;
  policy_training_fact.bundle_mdn_component_id = mdn_checkpoint_fact->component;
  policy_training_fact.bundle_mdn_checkpoint_digest =
      mdn_checkpoint_fact->checkpoint_file_digest;
  policy_training_fact.bundle_mdn_generation_id =
      mdn_checkpoint_fact->generation_id;
  policy_training_fact.bundle_mdn_generation_vector_digest =
      mdn_checkpoint_fact->generation_vector_member_digest;
  policy_training_fact.bundle_policy_component_id =
      "wikimyei.policy.portfolio.graph_node_allocation";
  policy_training_fact.bundle_policy_checkpoint_digest =
      policy_training_fact.checkpoint_digest;
  policy_training_fact.bundle_policy_generation_id =
      "policy_generation_fixture";
  policy_training_fact.bundle_policy_generation_vector_digest =
      "policy_generation_vector_digest_fixture";
  policy_training_fact
      .mdn_trained_against_representation_generation_vector_digest =
      representation_checkpoint_fact->generation_vector_member_digest;
  policy_training_fact
      .policy_trained_against_representation_generation_vector_digest =
      representation_checkpoint_fact->generation_vector_member_digest;
  policy_training_fact.policy_trained_against_mdn_generation_vector_digest =
      mdn_checkpoint_fact->generation_vector_member_digest;
  policy_training_fact
      .policy_trained_against_policy_parent_generation_vector_digest =
      "generation_vector_seed_policy_v1";
  policy_training_fact.snapshot_bundle_policy_execution_certificate_digest =
      "snapshot_bundle_policy_execution_certificate_digest_fixture";
  policy_training_fact.actor_architecture_digest =
      "actor_architecture_digest_fixture";
  policy_training_fact.critic_architecture_digest =
      "critic_architecture_digest_fixture";
  policy_training_fact.input_policy_checkpoint_digest =
      "seed_policy_checkpoint_digest_fixture";
  policy_training_fact.actor_checkpoint_digest =
      policy_training_fact.checkpoint_digest;
  policy_training_fact.critic_checkpoint_digest =
      "critic_checkpoint_digest_fixture";
  policy_training_fact.optimizer_state_digest =
      "optimizer_state_digest_fixture";
  policy_training_fact.optimizer_state_schema_id =
      "kikijyeba.runtime.ppo_optimizer_state.v1";
  policy_training_fact.optimizer_torch_state_path = "ppo_optimizer_state.pt";
  policy_training_fact.optimizer_torch_state_digest =
      "optimizer_torch_state_digest_fixture";
  policy_training_fact.ppo_config_digest = "ppo_config_digest_fixture";
  policy_training_fact.device_policy = "require_cuda";
  policy_training_fact.runtime_device_kind = "cuda";
  policy_training_fact.cuda_device_index = 0;
  policy_training_fact.cuda_device_index_bound = true;
  policy_training_fact.cuda_available = true;
  policy_training_fact.module_parameters_on_cuda = true;
  policy_training_fact.forward_input_on_cuda = true;
  policy_training_fact.loss_on_cuda = true;
  policy_training_fact.optimizer_state_on_cuda = true;
  policy_training_fact.cuda_verification_passed = true;
  policy_training_fact.resume_mode = "fresh_spawn";
  policy_training_fact.advantage_estimator_id = "gae.v1";
  policy_training_fact.advantage_normalization_policy =
      "per_rollout_standardize_v1";
  policy_training_fact.rollout_collection_schema_id =
      "kikijyeba.runtime.ppo_rollout_collection.v1";
  policy_training_fact.rollout_collection_digest =
      "rollout_collection_digest_fixture";
  policy_training_fact.ppo_update_report_schema_id =
      "kikijyeba.runtime.ppo_update_report.v1";
  policy_training_fact.ppo_update_report_digest =
      "ppo_update_report_digest_fixture";
  policy_training_fact.validation_rollout_report_digest =
      "validation_rollout_report_digest_fixture";
  policy_training_fact.ppo_gamma = 0.99;
  policy_training_fact.ppo_gamma_bound = true;
  policy_training_fact.ppo_gae_lambda = 0.95;
  policy_training_fact.ppo_gae_lambda_bound = true;
  policy_training_fact.ppo_clip_epsilon = 0.2;
  policy_training_fact.ppo_clip_epsilon_bound = true;
  policy_training_fact.ppo_target_kl = 0.02;
  policy_training_fact.ppo_target_kl_bound = true;
  policy_training_fact.ppo_entropy_coeff = 0.01;
  policy_training_fact.ppo_entropy_coeff_bound = true;
  policy_training_fact.ppo_value_loss_coeff = 0.5;
  policy_training_fact.ppo_value_loss_coeff_bound = true;
  policy_training_fact.ppo_max_grad_norm = 0.5;
  policy_training_fact.ppo_max_grad_norm_bound = true;
  policy_training_fact.ppo_minibatch_size = 32;
  policy_training_fact.ppo_minibatch_size_bound = true;
  policy_training_fact.ppo_epochs_per_rollout = 4;
  policy_training_fact.ppo_epochs_per_rollout_bound = true;
  policy_training_fact.parent_forecast_eval_fact_digest = forecast_eval_digest;
  policy_training_fact.parent_observer_belief_fact_digest =
      observer_belief_digest;
  policy_training_fact.parent_allocation_engine_fact_digest =
      allocation_engine_digest;
  policy_training_fact.parent_replay_environment_fact_digest =
      replay_environment_digest;
  policy_training_fact.random_seed = 0;
  policy_training_fact.random_seed_bound = true;
  policy_training_fact.training_range_disjoint_validation = true;
  policy_training_fact.training_range_disjoint_test = true;
  policy_training_fact.validation_range_disjoint_test = true;
  policy_training_fact.normalization_fit_training_only = true;
  policy_training_fact.replay_buffer_training_only = true;
  policy_training_fact.reward_baseline_training_only = true;
  policy_training_fact.early_stopping_uses_validation_only = true;
  policy_training_fact.hyperparameter_selection_uses_validation_only = true;
  policy_training_fact.test_sealed_until_final_report = true;
  policy_training_fact.test_first_access_after_selection = true;
  policy_training_fact.runtime_job_kind_bound = true;
  policy_training_fact.policy_checkpoint_written = true;
  policy_training_fact.causal_schedule_readiness_eligible = true;
  policy_training_fact.causal_schedule_no_future_snapshot_use = true;
  policy_training_fact.offline_full_window_research = false;
  std::vector<std::string> policy_closure_artifacts{
      forecast_eval_digest,
      emitted_forecast_eval_fact.forecast_artifact_digest};
  policy_closure_artifacts.insert(policy_closure_artifacts.end(),
                                  emitted_forecast_eval_fact.influence_summary
                                      .parent_artifact_digests.begin(),
                                  emitted_forecast_eval_fact.influence_summary
                                      .parent_artifact_digests.end());
  policy_closure_artifacts.insert(
      policy_closure_artifacts.end(),
      emitted_replay_environment_fact.influence_summary.parent_artifact_digests
          .begin(),
      emitted_replay_environment_fact.influence_summary.parent_artifact_digests
          .end());
  policy_closure_artifacts.push_back(
      emitted_replay_environment_fact.parent_forecast_eval_fact_digest);
  policy_closure_artifacts.push_back(
      emitted_replay_environment_fact.parent_forecast_artifact_digest);
  policy_closure_artifacts.insert(policy_closure_artifacts.end(),
                                  emitted_forecast_eval_fact.influence_summary
                                      .parent_artifact_digests.begin(),
                                  emitted_forecast_eval_fact.influence_summary
                                      .parent_artifact_digests.end());
  auto policy_execution_artifacts = policy_closure_artifacts;
  policy_execution_artifacts.push_back(replay_environment_digest);
  std::vector<std::string> policy_closure_checkpoints =
      emitted_forecast_eval_fact.influence_summary.parent_checkpoint_digests;
  policy_closure_checkpoints.insert(
      policy_closure_checkpoints.end(),
      emitted_replay_environment_fact.influence_summary
          .parent_checkpoint_digests.begin(),
      emitted_replay_environment_fact.influence_summary
          .parent_checkpoint_digests.end());
  policy_closure_checkpoints.insert(policy_closure_checkpoints.end(),
                                    emitted_forecast_eval_fact.influence_summary
                                        .parent_checkpoint_digests.begin(),
                                    emitted_forecast_eval_fact.influence_summary
                                        .parent_checkpoint_digests.end());
  std::vector<std::string> policy_closure_generations =
      emitted_forecast_eval_fact.influence_summary
          .parent_generation_vector_digests;
  policy_closure_generations.push_back(
      emitted_forecast_eval_fact.influence_summary
          .producer_generation_vector_digest);
  policy_closure_generations.insert(
      policy_closure_generations.end(),
      emitted_replay_environment_fact.influence_summary
          .parent_generation_vector_digests.begin(),
      emitted_replay_environment_fact.influence_summary
          .parent_generation_vector_digests.end());
  policy_closure_generations.insert(
      policy_closure_generations.end(),
      emitted_forecast_eval_fact.influence_summary
          .parent_generation_vector_digests.begin(),
      emitted_forecast_eval_fact.influence_summary
          .parent_generation_vector_digests.end());
  policy_closure_generations.push_back(
      emitted_forecast_eval_fact.influence_summary
          .producer_generation_vector_digest);
  const auto policy_no_lookahead_closure =
      expected_policy_no_lookahead_closure_digest(
          emitted_forecast_eval_fact.influence_summary
              .no_lookahead_contract_digest,
          policy_closure_artifacts, policy_closure_checkpoints,
          policy_closure_generations,
          std::max(emitted_forecast_eval_fact.influence_summary
                       .influence_anchor_end_exclusive_max,
                   emitted_replay_environment_fact.influence_summary
                       .influence_anchor_end_exclusive_max),
          emitted_forecast_eval_fact.influence_summary
                  .influence_anchor_end_exclusive_max_bound ||
              emitted_replay_environment_fact.influence_summary
                  .influence_anchor_end_exclusive_max_bound,
          std::max(emitted_forecast_eval_fact.influence_summary
                       .label_or_reward_availability_end_exclusive_max,
                   emitted_replay_environment_fact.influence_summary
                       .label_or_reward_availability_end_exclusive_max),
          emitted_forecast_eval_fact.influence_summary
                  .label_or_reward_availability_end_exclusive_max_bound ||
              emitted_replay_environment_fact.influence_summary
                  .label_or_reward_availability_end_exclusive_max_bound,
          policy_training_fact.embargo_policy_fingerprint,
          policy_training_fact.embargo_purged_window_anchor_range.begin,
          policy_training_fact.embargo_purged_window_anchor_range.end,
          policy_training_fact.embargo_purged_window_anchor_range_bound);
  const auto policy_output_provenance_closure = digest_vector_for_fixture(
      "policy_output_provenance_closure.v1",
      {policy_no_lookahead_closure,
       policy_training_fact
           .policy_trained_against_policy_parent_generation_vector_digest,
       policy_training_fact.bundle_policy_generation_vector_digest});
  policy_training_fact.parent_replay_environment_report_digest =
      emitted_replay_environment_fact.experiment_report_digest;
  policy_training_fact.policy_trained_against_forecast_eval_fact_digest =
      forecast_eval_digest;
  policy_training_fact.policy_trained_against_replay_environment_fact_digest =
      replay_environment_digest;
  policy_training_fact.mdn_trained_against_representation_generation_id =
      representation_checkpoint_fact->generation_id;
  policy_training_fact.mdn_trained_against_representation_checkpoint_digest =
      representation_checkpoint_fact->checkpoint_file_digest;
  policy_training_fact.policy_execution_input_lock_digest =
      digest_vector_for_fixture("policy_execution_input_lock.v1",
                                {policy_no_lookahead_closure});
  policy_training_fact.policy_execution_no_lookahead_certificate_schema =
      "no_lookahead_artifact_provenance.v1";
  policy_training_fact.policy_execution_no_lookahead_certificate_digest =
      policy_no_lookahead_closure;
  policy_training_fact.policy_execution_evidence_snapshot_digest =
      policy_no_lookahead_closure;
  policy_training_fact.policy_execution_provenance_closure_digest =
      policy_no_lookahead_closure;
  policy_training_fact.policy_execution_target_id =
      "emitted_policy_training_artifact_ready";
  policy_training_fact.policy_execution_target_anchor_range =
      policy_training_fact.anchor_range;
  policy_training_fact.policy_execution_target_anchor_range_bound = true;
  policy_training_fact.policy_execution_no_lookahead_contract_digest =
      emitted_forecast_eval_fact.influence_summary.no_lookahead_contract_digest;
  policy_training_fact.policy_execution_input_parent_forecast_eval_fact_digest =
      forecast_eval_digest;
  policy_training_fact.policy_execution_input_parent_forecast_artifact_digest =
      emitted_forecast_eval_fact.forecast_artifact_digest;
  policy_training_fact
      .policy_execution_input_parent_replay_environment_fact_digest =
      replay_environment_digest;
  policy_training_fact
      .policy_execution_input_parent_replay_environment_report_digest =
      emitted_replay_environment_fact.experiment_report_digest;
  policy_training_fact.policy_execution_replay_job_dir_digest =
      exposure::exposure_digest_for_text(eval_job_dir.string());
  policy_training_fact.policy_execution_consumed_artifact_digests =
      policy_execution_artifacts;
  policy_training_fact.policy_execution_consumed_checkpoint_digests =
      policy_closure_checkpoints;
  policy_training_fact.policy_execution_consumed_generation_vector_digests =
      policy_closure_generations;
  policy_training_fact.parent_policy_generation_id =
      "policy_generation_seed_v1";
  policy_training_fact.parent_policy_generation_vector_digest =
      "generation_vector_seed_policy_v1";
  policy_training_fact.parent_policy_influence_summary.complete = true;
  policy_training_fact.parent_policy_influence_summary.graph_order_fingerprint =
      policy_training_fact.graph_order_fingerprint;
  policy_training_fact.parent_policy_influence_summary.source_cursor_token =
      policy_training_fact.source_cursor_token;
  policy_training_fact.parent_policy_influence_summary
      .split_policy_fingerprint = policy_training_fact.split_policy_fingerprint;
  policy_training_fact.parent_policy_influence_summary.coverage_anchor_range = {
      .begin = 0, .end = policy_training_fact.anchor_range.begin};
  policy_training_fact.parent_policy_influence_summary
      .coverage_anchor_range_bound = true;
  policy_training_fact.parent_policy_influence_summary
      .influence_anchor_begin_min = 0;
  policy_training_fact.parent_policy_influence_summary
      .influence_anchor_begin_min_bound = true;
  policy_training_fact.parent_policy_influence_summary
      .influence_anchor_end_exclusive_max = 0;
  policy_training_fact.parent_policy_influence_summary
      .influence_anchor_end_exclusive_max_bound = true;
  policy_training_fact.parent_policy_influence_summary
      .label_or_reward_availability_end_exclusive_max = 0;
  policy_training_fact.parent_policy_influence_summary
      .label_or_reward_availability_end_exclusive_max_bound = true;
  policy_training_fact.parent_policy_influence_summary
      .producer_generation_vector_digest =
      policy_training_fact.parent_policy_generation_vector_digest;
  policy_training_fact.parent_policy_influence_summary
      .provenance_closure_digest = digest_vector_for_fixture(
      "policy_seed_parent_closure.v1",
      {policy_training_fact.parent_policy_generation_vector_digest});
  policy_training_fact.parent_policy_influence_summary
      .no_lookahead_contract_digest =
      policy_training_fact.policy_execution_no_lookahead_contract_digest;
  policy_training_fact.parent_policy_valid_from_anchor = 0;
  policy_training_fact.parent_policy_valid_from_anchor_bound = true;
  policy_training_fact.policy_output_generation_id =
      policy_training_fact.bundle_policy_generation_id;
  policy_training_fact.policy_output_generation_vector_digest =
      policy_training_fact.bundle_policy_generation_vector_digest;
  policy_training_fact.policy_output_fit_anchor_range =
      policy_training_fact.anchor_range;
  policy_training_fact.policy_output_fit_anchor_range_bound = true;
  policy_training_fact.policy_output_valid_from_anchor =
      policy_training_fact.anchor_range.end;
  policy_training_fact.policy_output_valid_from_anchor_bound = true;
  policy_training_fact.policy_output_provenance_closure_digest =
      policy_output_provenance_closure;
  policy_training_fact.influence_summary.complete = true;
  policy_training_fact.influence_summary.graph_order_fingerprint =
      policy_training_fact.graph_order_fingerprint;
  policy_training_fact.influence_summary.source_cursor_token =
      policy_training_fact.source_cursor_token;
  policy_training_fact.influence_summary.split_policy_fingerprint =
      policy_training_fact.split_policy_fingerprint;
  policy_training_fact.influence_summary.coverage_anchor_range =
      policy_training_fact.anchor_range;
  policy_training_fact.influence_summary.coverage_anchor_range_bound = true;
  policy_training_fact.influence_summary.influence_anchor_begin_min =
      policy_training_fact.anchor_range.begin;
  policy_training_fact.influence_summary.influence_anchor_begin_min_bound =
      true;
  policy_training_fact.influence_summary.influence_anchor_end_exclusive_max =
      policy_training_fact.anchor_range.end;
  policy_training_fact.influence_summary
      .influence_anchor_end_exclusive_max_bound = true;
  policy_training_fact.influence_summary
      .label_or_reward_availability_end_exclusive_max =
      policy_training_fact.anchor_range.end;
  policy_training_fact.influence_summary
      .label_or_reward_availability_end_exclusive_max_bound = true;
  policy_training_fact.influence_summary.producer_generation_vector_digest =
      policy_training_fact.policy_output_generation_vector_digest;
  policy_training_fact.influence_summary.parent_artifact_digests = {
      policy_no_lookahead_closure};
  policy_training_fact.influence_summary.parent_checkpoint_digests = {
      policy_training_fact.input_policy_checkpoint_digest};
  policy_training_fact.influence_summary.parent_generation_vector_digests =
      policy_closure_generations;
  policy_training_fact.influence_summary.parent_generation_vector_digests
      .push_back(policy_training_fact.parent_policy_generation_vector_digest);
  policy_training_fact.influence_summary.provenance_closure_digest =
      policy_output_provenance_closure;
  policy_training_fact.influence_summary.no_lookahead_contract_digest =
      policy_training_fact.policy_execution_no_lookahead_contract_digest;
  policy_training_fact.snapshot_bundle_no_lookahead_certificate_digests = {
      policy_no_lookahead_closure};
  policy_training_fact.snapshot_bundle_policy_execution_certificate_digest =
      policy_no_lookahead_closure;
  policy_training_fact.snapshot_bundle_generation_vector_digest =
      digest_vector_for_fixture(
          "snapshot_bundle_generation_vector.v1",
          {policy_training_fact.bundle_representation_generation_vector_digest,
           policy_training_fact.bundle_mdn_generation_vector_digest,
           policy_training_fact.bundle_policy_generation_vector_digest});
  policy_training_fact.snapshot_bundle_influence_anchor_end_exclusive_max =
      policy_training_fact.anchor_range.end;
  policy_training_fact
      .snapshot_bundle_influence_anchor_end_exclusive_max_bound = true;
  policy_training_fact
      .snapshot_bundle_compatibility_closure_digest = digest_vector_for_fixture(
      "snapshot_bundle_compatibility_closure.v1",
      {policy_training_fact.snapshot_bundle_id,
       policy_training_fact.bundle_representation_generation_vector_digest,
       policy_training_fact.bundle_mdn_generation_vector_digest,
       policy_training_fact.bundle_policy_generation_vector_digest,
       policy_training_fact
           .mdn_trained_against_representation_generation_vector_digest,
       policy_training_fact
           .policy_trained_against_representation_generation_vector_digest,
       policy_training_fact.policy_trained_against_mdn_generation_vector_digest,
       policy_training_fact
           .policy_trained_against_policy_parent_generation_vector_digest,
       policy_training_fact.policy_execution_no_lookahead_certificate_digest,
       policy_training_fact.policy_execution_no_lookahead_contract_digest});
  exposure::lattice_checkpoint_fact_t policy_checkpoint_fact{};
  policy_checkpoint_fact.checkpoint_path =
      eval_job_dir / "ppo_policy_actor.checkpoint";
  policy_checkpoint_fact.checkpoint_file_digest =
      policy_training_fact.actor_checkpoint_digest;
  policy_checkpoint_fact.component =
      policy_training_fact.bundle_policy_component_id;
  policy_checkpoint_fact.component_role =
      exposure::component_role_from_component_id(
          policy_checkpoint_fact.component);
  policy_checkpoint_fact.created_by_job_id = policy_training_fact.job_id;
  policy_checkpoint_fact.created_by_wave_id = policy_training_fact.wave_id;
  policy_checkpoint_fact.direct_exposure_digest =
      exposure::policy_training_fact_digest(policy_training_fact);
  policy_checkpoint_fact.checkpoint_id = exposure::exposure_digest_for_text(
      "checkpoint|" + policy_checkpoint_fact.checkpoint_path.string() + "|" +
      policy_checkpoint_fact.checkpoint_file_digest + "|" +
      policy_checkpoint_fact.direct_exposure_digest);
  policy_checkpoint_fact.generation_id =
      policy_training_fact.policy_output_generation_id;
  policy_checkpoint_fact.generation_vector_member_digest =
      policy_training_fact.policy_output_generation_vector_digest;
  policy_checkpoint_fact.parent_generation_ids = {
      policy_training_fact.parent_policy_generation_id};
  policy_checkpoint_fact.parent_generation_vector_digests = {
      policy_training_fact.parent_policy_generation_vector_digest};
  policy_checkpoint_fact.read_checkpoint_digests = {
      policy_training_fact.input_policy_checkpoint_digest};
  policy_checkpoint_fact.read_generation_ids = {
      policy_training_fact.parent_policy_generation_id};
  policy_checkpoint_fact.generation_lane = "bootstrap";
  policy_checkpoint_fact.fit_anchor_range = policy_training_fact.anchor_range;
  policy_checkpoint_fact.fit_anchor_range_bound = true;
  policy_checkpoint_fact.valid_from_anchor =
      policy_training_fact.policy_output_valid_from_anchor;
  policy_checkpoint_fact.valid_from_anchor_bound = true;
  policy_checkpoint_fact.no_lookahead_contract_digest =
      policy_training_fact.policy_execution_no_lookahead_contract_digest;
  policy_checkpoint_fact.contract_fingerprint =
      policy_training_fact.contract_fingerprint;
  policy_checkpoint_fact.protocol_id = policy_training_fact.protocol_id;
  policy_checkpoint_fact.graph_order_fingerprint =
      policy_training_fact.graph_order_fingerprint;
  policy_checkpoint_fact.source_cursor_token =
      policy_training_fact.source_cursor_token;
  policy_checkpoint_fact.component_assembly_fingerprint =
      policy_training_fact.component_assembly_fingerprint;
  policy_checkpoint_fact.influence_summary =
      policy_training_fact.influence_summary;
  policy_checkpoint_fact.closure_digest = policy_output_provenance_closure;
  policy_training_fact.bundle_policy_generation_manifest_digest =
      exposure::exposure_digest_for_text(
          exposure::canonical_checkpoint_fact_text(policy_checkpoint_fact));
  policy_training_fact.causal_provenance_schema =
      exposure::k_causal_provenance_generalization_schema_v1;
  policy_training_fact.causal_atom_schema =
      exposure::k_causal_atom_schema_anchor_interval_v1;
  policy_training_fact.causal_interval_set_schema =
      exposure::k_causal_interval_set_schema_anchor_half_open_v1;
  policy_training_fact.causal_label_reward_horizon_policy_fingerprint =
      "label_reward_horizon.anchor_scalar_v1";
  policy_training_fact.causal_fold_policy_fingerprint =
      "fold_policy.single_time_split_v1";
  policy_training_fact.causal_purged_embargo_policy_fingerprint =
      policy_training_fact.embargo_policy_fingerprint;
  policy_training_fact.causal_artifact_production_schema =
      exposure::k_runtime_artifact_production_inline_schema_v1;
  policy_training_fact.causal_artifact_production_closure_digest =
      policy_no_lookahead_closure;
  policy_training_fact.policy_trained_against_no_lookahead_certificate_digest =
      policy_no_lookahead_closure;
  policy_training_fact.causal_interface_stability_contract_digest =
      exposure::k_interface_stability_trained_against_generation_vector_v1;
  policy_training_fact.causal_provenance_closure_digest =
      policy_no_lookahead_closure;
  auto bundled_representation_checkpoint_fact = *representation_checkpoint_fact;
  bundled_representation_checkpoint_fact.generation_lane = "bootstrap";
  bundled_representation_checkpoint_fact.producer_component_update_fact_digest
      .clear();
  auto bundled_mdn_checkpoint_fact = *mdn_checkpoint_fact;
  bundled_mdn_checkpoint_fact.generation_lane = "bootstrap";
  bundled_mdn_checkpoint_fact.producer_component_update_fact_digest.clear();
  write_text(eval_job_dir / "representation.checkpoint.fact",
             exposure::canonical_checkpoint_fact_text(
                 bundled_representation_checkpoint_fact) +
                 "fact_digest=" +
                 exposure::exposure_digest_for_text(
                     exposure::canonical_checkpoint_fact_text(
                         bundled_representation_checkpoint_fact)) +
                 "\n");
  write_text(
      eval_job_dir / "mdn.checkpoint.fact",
      exposure::canonical_checkpoint_fact_text(bundled_mdn_checkpoint_fact) +
          "fact_digest=" +
          exposure::exposure_digest_for_text(
              exposure::canonical_checkpoint_fact_text(
                  bundled_mdn_checkpoint_fact)) +
          "\n");
  write_text(eval_job_dir / "lattice.checkpoint.fact",
             exposure::canonical_checkpoint_fact_text(policy_checkpoint_fact) +
                 "fact_digest=" +
                 exposure::exposure_digest_for_text(
                     exposure::canonical_checkpoint_fact_text(
                         policy_checkpoint_fact)) +
                 "\n");
  const auto policy_training_canonical =
      exposure::canonical_policy_training_fact_text(policy_training_fact);
  write_text(eval_job_dir / "runtime.policy_training.fact",
             policy_training_canonical + "fact_digest=" +
                 exposure::policy_training_fact_digest(policy_training_fact) +
                 "\n");

  const auto ppo_scan = exposure::scan_exposure_ledger_from_runtime_root(
      eval_job_dir, {}, eval_scan_options);
  check(ppo_scan.ledger.policy_training_facts().size() == 1,
        "strict baseline eval scan sees durable PPO policy-training fact");
  check(ppo_scan.ledger.policy_training_facts()
                .front()
                .target_component_family_id ==
            "wikimyei.policy.portfolio.graph_node_allocation",
        "strict baseline policy-training fact preserves policy component "
        "identity");
  const auto policy_training_issues = exposure::policy_training_fact_issues(
      ppo_scan.ledger.policy_training_facts().front());
  check(policy_training_issues.empty(),
        "strict baseline policy-training fact has causal schedule, unified "
        "action, post-execution reward, and parent evidence: " +
            join_issue_tokens(policy_training_issues));

  const auto artifact_specs =
      target::decode_lattice_targets_from_dsl(std::string(R"DSL(
LATTICE_TARGET {
  TARGET_ID = emitted_target_transform_contract_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = target_transform;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = )DSL") + eval_result.manifest.protocol_id +
                                              R"DSL(;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 1;
  ANCHOR_INDEX_END = 3;
  REQUIRE_CONTRACT_MATCH = true;
};

LATTICE_TARGET {
  TARGET_ID = emitted_forecast_baseline_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = forecast_baseline;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = )DSL" + eval_result.manifest.protocol_id +
                                              R"DSL(;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 1;
  ANCHOR_INDEX_END = 3;
  REQUIRE_CONTRACT_MATCH = true;
};

LATTICE_TARGET {
  TARGET_ID = emitted_forecast_eval_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = forecast_eval;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = )DSL" + eval_result.manifest.protocol_id +
                                              R"DSL(;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 1;
  ANCHOR_INDEX_END = 3;
  REQUIRE_CONTRACT_MATCH = true;
};

LATTICE_TARGET {
  TARGET_ID = emitted_observer_belief_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = observer_belief;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = )DSL" + eval_result.manifest.protocol_id +
                                              R"DSL(;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 1;
  ANCHOR_INDEX_END = 3;
  REQUIRE_CONTRACT_MATCH = true;
};

LATTICE_TARGET {
  TARGET_ID = emitted_replay_environment_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = replay_environment;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = )DSL" + eval_result.manifest.protocol_id +
                                              R"DSL(;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 1;
  ANCHOR_INDEX_END = 3;
  REQUIRE_CONTRACT_MATCH = true;
};

LATTICE_TARGET {
  TARGET_ID = emitted_policy_training_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = policy_training;
  SUBJECT_COMPONENT = wikimyei.policy.portfolio.graph_node_allocation;
  PROTOCOL_ID = )DSL" + eval_result.manifest.protocol_id +
                                              R"DSL(;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 1;
  ANCHOR_INDEX_END = 3;
  REQUIRE_CONTRACT_MATCH = true;
};

)DSL");
  target::lattice_target_evaluator_options_t artifact_options{};
  artifact_options.exposure_ledger = &ppo_scan.ledger;
  artifact_options.auto_build_exposure_ledger = false;
  artifact_options.active_identity.protocol_id =
      eval_result.manifest.protocol_id;
  artifact_options.active_identity.protocol_contract_fingerprint =
      eval_result.manifest.protocol_contract_fingerprint;
  artifact_options.active_identity.graph_order_fingerprint =
      eval_result.manifest.graph_order_fingerprint;
  artifact_options.active_identity.source_cursor_token =
      eval_result.manifest.source_cursor_token;
  artifact_options.active_identity.mdn_assembly_fingerprint =
      eval_result.manifest.mdn_assembly_fingerprint;
  target::lattice_target_evaluator_t artifact_evaluator(artifact_specs,
                                                        artifact_options);
  const auto emitted_transform_eval =
      artifact_evaluator.evaluate("emitted_target_transform_contract_ready");
  const auto emitted_baseline_eval =
      artifact_evaluator.evaluate("emitted_forecast_baseline_artifact_ready");
  const auto emitted_forecast_eval =
      artifact_evaluator.evaluate("emitted_forecast_eval_artifact_ready");
  const auto emitted_observer_eval =
      artifact_evaluator.evaluate("emitted_observer_belief_artifact_ready");
  const auto emitted_replay_eval =
      artifact_evaluator.evaluate("emitted_replay_environment_artifact_ready");
  const auto emitted_policy_training_eval =
      artifact_evaluator.evaluate("emitted_policy_training_artifact_ready");
  check(emitted_transform_eval.status ==
            target::lattice_target_status_t::satisfied,
        "strict baseline emitted target-transform fact satisfies artifact "
        "readiness: " +
            describe_lattice_target_evaluation(emitted_transform_eval));
  check(emitted_baseline_eval.status ==
            target::lattice_target_status_t::satisfied,
        "strict baseline emitted forecast-baseline fact satisfies artifact "
        "readiness: " +
            describe_lattice_target_evaluation(emitted_baseline_eval));
  check(emitted_forecast_eval.status ==
            target::lattice_target_status_t::satisfied,
        "strict baseline emitted forecast-eval fact satisfies artifact "
        "readiness: " +
            describe_lattice_target_evaluation(emitted_forecast_eval));
  check(emitted_observer_eval.status ==
            target::lattice_target_status_t::satisfied,
        "strict baseline emitted observer-belief fact satisfies artifact "
        "readiness: " +
            describe_lattice_target_evaluation(emitted_observer_eval));
  check(emitted_replay_eval.status ==
            target::lattice_target_status_t::satisfied,
        "strict baseline emitted cost-aware replay report satisfies "
        "replay-environment artifact readiness: " +
            describe_lattice_target_evaluation(emitted_replay_eval));
  check(emitted_policy_training_eval.status ==
                target::lattice_target_status_t::satisfied &&
            emitted_policy_training_eval.proof_certificate_check.passed &&
            emitted_policy_training_eval.proof_certificate.artifacts.size() ==
                1 &&
            emitted_policy_training_eval.proof_certificate.artifacts.front()
                .no_lookahead_provenance_checked &&
            emitted_policy_training_eval.proof_certificate.artifacts.front()
                .no_lookahead_provenance_complete &&
            emitted_policy_training_eval.proof_certificate.artifacts.front()
                .no_lookahead_provenance_admissible,
        "strict baseline emitted PPO policy-training fact passes with "
        "Runtime-emitted forecast/replay no-lookahead provenance: " +
            describe_lattice_target_evaluation(emitted_policy_training_eval));
  check(emitted_policy_training_eval.proof_certificate.artifacts.size() == 1 &&
            emitted_policy_training_eval.proof_certificate.artifacts.front()
                .causal_provenance_checked &&
            emitted_policy_training_eval.proof_certificate.artifacts.front()
                .causal_provenance_complete &&
            emitted_policy_training_eval.proof_certificate.artifacts.front()
                .causal_provenance_admissible &&
            emitted_policy_training_eval.proof_certificate.artifacts.front()
                    .causal_artifact_production_closure_digest ==
                emitted_policy_training_eval.proof_certificate.artifacts.front()
                    .provenance_closure_digest,
        "strict baseline emitted PPO policy-training fact carries passing "
        "causal_provenance_generalization.v1 aggregate state");

  auto old_style_forecast_eval_fact = emitted_forecast_eval_fact;
  old_style_forecast_eval_fact.influence_summary = {};
  const auto old_style_forecast_eval_digest =
      exposure::forecast_eval_fact_digest(old_style_forecast_eval_fact);
  auto old_style_replay_environment_fact =
      ppo_scan.ledger.replay_environment_facts().front();
  old_style_replay_environment_fact.influence_summary = {};
  old_style_replay_environment_fact.parent_forecast_eval_fact_digest =
      old_style_forecast_eval_digest;
  const auto old_style_replay_environment_digest =
      exposure::replay_environment_fact_digest(
          old_style_replay_environment_fact);
  auto old_style_policy_training_fact =
      ppo_scan.ledger.policy_training_facts().front();
  old_style_policy_training_fact.parent_forecast_eval_fact_digest =
      old_style_forecast_eval_digest;
  old_style_policy_training_fact
      .policy_execution_input_parent_forecast_eval_fact_digest =
      old_style_forecast_eval_digest;
  old_style_policy_training_fact.parent_replay_environment_fact_digest =
      old_style_replay_environment_digest;
  old_style_policy_training_fact
      .policy_execution_input_parent_replay_environment_fact_digest =
      old_style_replay_environment_digest;
  exposure::lattice_exposure_ledger_t old_style_ledger{};
  for (const auto &fact : ppo_scan.ledger.target_transform_facts()) {
    old_style_ledger.add_target_transform(fact);
  }
  for (const auto &fact : ppo_scan.ledger.forecast_baseline_facts()) {
    old_style_ledger.add_forecast_baseline(fact);
  }
  old_style_ledger.add_forecast_eval(old_style_forecast_eval_fact);
  for (const auto &fact : ppo_scan.ledger.observer_belief_facts()) {
    old_style_ledger.add_observer_belief(fact);
  }
  for (const auto &fact : ppo_scan.ledger.allocation_engine_facts()) {
    old_style_ledger.add_allocation_engine(fact);
  }
  old_style_ledger.add_replay_environment(old_style_replay_environment_fact);
  old_style_ledger.add_policy_training(old_style_policy_training_fact);
  target::lattice_target_evaluator_options_t old_style_options =
      artifact_options;
  old_style_options.exposure_ledger = &old_style_ledger;
  target::lattice_target_evaluator_t old_style_evaluator(artifact_specs,
                                                         old_style_options);
  const auto old_style_policy_training_eval =
      old_style_evaluator.evaluate("emitted_policy_training_artifact_ready");
  check(
      old_style_policy_training_eval.status !=
              target::lattice_target_status_t::satisfied &&
          !old_style_policy_training_eval.proof_certificate_check.passed &&
          has_reason_containing(old_style_policy_training_eval.reasons,
                                "no_lookahead_forecast_influence_incomplete") &&
          has_reason_containing(old_style_policy_training_eval.reasons,
                                "no_lookahead_replay_influence_incomplete"),
      "old-style Runtime evidence without forecast/replay influence still "
      "fails closed: " +
          describe_lattice_target_evaluation(old_style_policy_training_eval));
}

void test_invalid_wave_range_fails_before_launch() {
  const auto fixture =
      make_config_fixture("invalid_range", "  SOURCE_RANGE = anchor_index;\n"
                                           "  ANCHOR_INDEX_BEGIN = 0;\n"
                                           "  ANCHOR_INDEX_END = 200;\n");
  runtime::job_runner_options_t options{};
  options.job_kind = runtime::runtime_job_kind_t::channel_inference_mdn;
  options.dry_run = true;
  options.batch_size = 2;
  options.job_dir = fixture.dir / "job";
  bool threw = false;
  try {
    (void)runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  } catch (const std::exception &ex) {
    threw = std::string(ex.what()).find(
                "exceeds accepted graph anchor domain") != std::string::npos;
  }
  check(threw, "invalid wave range fails with domain error");
}

void test_invalid_source_key_wave_range_fails_before_launch() {
  const auto fixture = make_config_fixture("invalid_source_key_range",
                                           "  SOURCE_RANGE = source_key;\n"
                                           "  SOURCE_KEY_BEGIN = 1000;\n"
                                           "  SOURCE_KEY_END = 1002;\n");
  runtime::job_runner_options_t options{};
  options.job_kind = runtime::runtime_job_kind_t::channel_inference_mdn;
  options.dry_run = true;
  options.batch_size = 2;
  options.job_dir = fixture.dir / "job";
  bool threw = false;
  try {
    (void)runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  } catch (const std::exception &ex) {
    threw =
        std::string(ex.what()).find(
            "begins before accepted graph anchor domain") != std::string::npos;
  }
  check(threw, "invalid source-key wave range fails with domain error");
}

void test_invalid_wave_mode_fails() {
  const auto fixture = make_config_fixture(
      "invalid_mode", "  SOURCE_RANGE = all;\n",
      "wikimyei.inference.expected_value.mdn", "run | train");
  runtime::job_runner_options_t options{};
  options.dry_run = true;
  options.job_dir = fixture.dir / "job";
  bool threw = false;
  try {
    (void)runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  } catch (const std::exception &ex) {
    threw = std::string(ex.what()).find("exactly one primary action") !=
            std::string::npos;
  }
  check(threw, "run and train are mutually exclusive wave actions");
}

struct fake_training_report_t {
  int64_t steps_attempted{1};
  int64_t steps_completed{1};
  int64_t skipped_batches{0};
  int64_t optimizer_steps{1};
  int64_t wave_pulses_attempted{1};
  int64_t wave_pulses_completed{1};
  int64_t wave_pulses_skipped{0};
  int64_t wave_streamed_anchor_count{2};
  std::string wave_first_anchor_key{"2000"};
  std::string wave_last_anchor_key{"1000"};
  double last_loss{1.0};
  bool checkpoint_written{false};
  int64_t checkpoint_write_count{0};
  std::string checkpoint_path{};
  bool report_written{false};
  int64_t report_write_count{0};
  std::string report_path{};
};

void test_completed_job_state_keeps_plan_wave_keys() {
  runtime::job_manifest_t manifest{};
  manifest.job_id = "wave_key_identity.train.channel_representation_vicreg";
  manifest.job_kind = "channel_representation_vicreg";

  runtime::wave_plan_t wave_plan{};
  wave_plan.wave_id = "wave_key_identity";
  wave_plan.target_component_family_id =
      "wikimyei.representation.encoding.vicreg";
  wave_plan.action = "train";
  wave_plan.resolved_anchor_index_begin = 29;
  wave_plan.resolved_anchor_index_end = 31;
  wave_plan.first_anchor_key = "1000";
  wave_plan.last_anchor_key = "2000";

  const fake_training_report_t report{};
  const auto state =
      runtime::make_completed_job_state(manifest, wave_plan, report, false);

  check(state.wave_first_anchor_key == "1000" &&
            state.wave_last_anchor_key == "2000",
        "completed job state keeps canonical wave-plan keys");
}

void test_resume_and_default_job_dir() {
  const auto fixture =
      make_config_fixture("fail_fast", "  SOURCE_RANGE = all;\n");
  runtime::job_runner_options_t resume_options{};
  resume_options.resume = true;
  resume_options.job_dir = fixture.dir / "resume_job";
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
  run_options.job_id = "runtime_default_job_dir_smoke_" +
                       std::to_string(static_cast<long long>(::getpid()));
  const auto default_runtime_root =
      std::filesystem::path("/cuwacunu/.runtime/cuwacunu_exec");
  const auto default_registry_path =
      provenance::component_spawn_registry_path(default_runtime_root);
  const bool had_default_registry =
      std::filesystem::exists(default_registry_path);
  const auto default_registry_before =
      had_default_registry ? read_text(default_registry_path) : std::string{};
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
  check(result.manifest_path.string().find(
            "/cuwacunu/.runtime/cuwacunu_exec/") != std::string::npos,
        "default job dir is under /cuwacunu/.runtime/cuwacunu_exec");
  check(result.manifest_path.string().find("/components/") != std::string::npos,
        "default job dir uses component/spawn layout");
  check(result.manifest.job_stable_id == run_options.job_id,
        "default job keeps caller-supplied stable job id");
  check(result.manifest.job_id != result.manifest.job_stable_id,
        "default job id is an immutable attempt id");
  check(result.manifest.job_attempt_policy == "immutable_attempt_v1",
        "default job uses immutable attempt policy");
  check(result.manifest.job_attempt_id == "attempt_000000",
        "first default job uses first attempt id");
  check(std::filesystem::exists(result.manifest_path),
        "default job dir manifest exists");
  const auto second_result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), run_options);
  check(second_result.manifest.job_stable_id == result.manifest.job_stable_id,
        "rerun keeps the same stable job id");
  check(second_result.manifest.component_spawn_fingerprint ==
            result.manifest.component_spawn_fingerprint,
        "rerun keeps the same component spawn fingerprint");
  check(second_result.manifest.component_spawn_id ==
            result.manifest.component_spawn_id,
        "rerun keeps the same component spawn id");
  check(second_result.manifest.job_id != result.manifest.job_id,
        "rerun gets a distinct immutable job id");
  check(second_result.manifest_path != result.manifest_path,
        "rerun writes a distinct manifest path");
  check(second_result.manifest.job_attempt_id == "attempt_000001",
        "rerun uses the next attempt id");
  const auto latest_by_stable_id =
      runtime::job_layout::find_runtime_job_dir_by_id(
          default_runtime_root, result.manifest.job_stable_id);
  check(latest_by_stable_id.has_value(),
        "stable job id resolves to a runtime attempt");
  std::error_code cleanup_ec;
  std::filesystem::remove_all(result.manifest_path.parent_path(), cleanup_ec);
  std::filesystem::remove_all(second_result.manifest_path.parent_path(),
                              cleanup_ec);
  const auto runtime_root =
      runtime::job_runner_detail::runtime_root_for_job_dir(
          result.manifest_path.parent_path());
  const auto spawn_dir = runtime::job_layout::component_spawn_dir(
      runtime_root, result.manifest.component_family_id,
      result.manifest.component_spawn_id,
      result.manifest.component_spawn_fingerprint);
  std::filesystem::remove_all(spawn_dir, cleanup_ec);
  if (had_default_registry) {
    write_text(default_registry_path, default_registry_before);
  } else {
    std::filesystem::remove(default_registry_path, cleanup_ec);
  }
}

void test_explicit_job_dir_refuses_overwrite() {
  const auto fixture = make_config_fixture("explicit_job_dir_refuses_overwrite",
                                           "  SOURCE_RANGE = all;\n");
  runtime::job_runner_options_t options{};
  options.dry_run = true;
  options.batch_size = 2;
  options.job_dir = fixture.dir / "job";
  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  check(result.manifest.job_attempt_policy ==
            "explicit_job_dir_no_overwrite_v1",
        "explicit job dir records no-overwrite policy");
  bool overwrite_threw = false;
  try {
    (void)runtime::run_graph_first_job<Kline>(fixture.config.string(), options);
  } catch (const std::exception &ex) {
    overwrite_threw =
        std::string(ex.what()).find("refusing to overwrite existing job_dir") !=
        std::string::npos;
  }
  check(overwrite_threw, "explicit job dir refuses overwrite");
}

} // namespace

int main() {
  try {
    test_inference_dry_run_writes_manifest_and_state();
    test_probe_catalog_sidecar_writes_visibility_records();
    test_probe_catalog_sidecar_writes_learning_timeseries_records();
    test_learning_probe_fraction_range_uses_resolved_anchor_axes();
    test_missing_probe_config_does_not_attach_sidecar();
    test_disabled_probe_catalog_writes_no_sidecar();
    test_probe_sidecar_requires_debug_wave();
    test_probe_catalog_validation_rejects_bad_inputs();
    test_representation_dry_run_anchor_range();
    test_representation_dry_run_source_key_range();
    test_source_range_launch_overlay();
    test_train_mode_manifest_mutation_policy();
    test_channel_targets_dispatch_to_channel_runner();
    test_mtf_representation_runs_through_runtime();
    test_protocol_wave_representation_mismatch_fails();
    test_wave_profile_protocol_mismatch_fails();
    test_component_spawn_identity_is_protocol_not_wave_scoped();
    test_strict_channel_baseline_runs_through_runtime();
    test_invalid_wave_range_fails_before_launch();
    test_invalid_source_key_wave_range_fails_before_launch();
    test_invalid_wave_mode_fails();
    test_completed_job_state_keeps_plan_wave_keys();
    test_resume_and_default_job_dir();
    test_explicit_job_dir_refuses_overwrite();
    std::cout << "[Kikijyeba JobRunner test] all checks passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "[Kikijyeba JobRunner test] failure: " << ex.what() << "\n";
    return 1;
  }
}
