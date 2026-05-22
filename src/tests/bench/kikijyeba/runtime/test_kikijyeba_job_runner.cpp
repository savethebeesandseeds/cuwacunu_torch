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
          "wikimyei_representation_vicreg_jkimyei_path = "
          "/cuwacunu/src/config/wikimyei.representation.vicreg.jkimyei\n"
          "wikimyei_inference_expected_value_mdn_jkimyei_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.jkimyei.bnf\n"
          "wikimyei_inference_expected_value_mdn_jkimyei_path = "
          "/cuwacunu/src/config/"
          "wikimyei.inference.expected_value.mdn.jkimyei\n");

  return out;
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

  check(result.manifest.job_kind == "inference_mdn", "manifest job kind");
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
  check(manifest_text.find("job_kind=inference_mdn") != std::string::npos,
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
  check(result.manifest.job_kind == "representation_vicreg",
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
  const auto result =
      runtime::run_graph_first_job<Kline>(fixture.config.string(), run_options);
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
