#include "kikijyeba/composition/pipeline_builder.h"

#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

namespace builder = cuwacunu::kikijyeba::composition;
namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
namespace mdnstream =
    cuwacunu::wikimyei::inference::expected_value::mdn::stream;
namespace types = cuwacunu::ujcamei::source::types;

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
             ("cuwacunu_composition_pipeline_builder_" + label + "_" +
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
                                    bool discover_edges = false) {
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

  const auto sources_dsl = out.dir / "ujcamei.sources.dsl";
  const auto channels_dsl =
      out.dir / "kikijyeba.protocol.cwu_01v.settings.dock.channels.dsl";
  const auto graph_dsl =
      out.dir / "kikijyeba.protocol.cwu_01v.topology.graph.dsl";
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

  write_text(
      out.config,
      std::string("[UJCAMEI]\n"
                  "ujcamei_sources_bnf_path = "
                  "/cuwacunu/src/config/grammar/ujcamei.sources.bnf\n"
                  "ujcamei_sources_dsl_path = ") +
          sources_dsl.string() +
          "\n"
          "kikijyeba_protocol_cwu_01v_settings_dock_channels_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "kikijyeba.protocol.cwu_01v.settings.dock.channels.bnf\n"
          "kikijyeba_protocol_cwu_01v_settings_dock_channels_dsl_path = " +
          channels_dsl.string() +
          "\n"
          "kikijyeba_protocol_cwu_01v_topology_graph_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "kikijyeba.protocol.cwu_01v.topology.graph.bnf\n"
          "kikijyeba_protocol_cwu_01v_topology_graph_dsl_path = " +
          graph_dsl.string() +
          "\n\n"
          "[WIKIMYEI]\n"
          "wikimyei_representation_vicreg_bnf_path = "
          "/cuwacunu/src/config/grammar/wikimyei.representation.vicreg.bnf\n"
          "wikimyei_representation_vicreg_dsl_path = "
          "/cuwacunu/src/config/wikimyei.representation.vicreg.dsl\n"
          "wikimyei_inference_expected_value_mdn_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.bnf\n"
          "wikimyei_inference_expected_value_mdn_dsl_path = "
          "/cuwacunu/src/config/"
          "wikimyei.inference.expected_value.mdn.dsl\n\n"
          "[JKIMYEI]\n"
          "jkimyei_representation_vicreg_bnf_path = "
          "/cuwacunu/src/config/grammar/jkimyei.representation.vicreg.bnf\n"
          "jkimyei_representation_vicreg_dsl_path = "
          "/cuwacunu/src/config/jkimyei.representation.vicreg.dsl\n"
          "jkimyei_inference_expected_value_mdn_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "jkimyei.inference.expected_value.mdn.bnf\n"
          "jkimyei_inference_expected_value_mdn_dsl_path = "
          "/cuwacunu/src/config/"
          "jkimyei.inference.expected_value.mdn.dsl\n");

  return out;
}

std::vector<torch::Tensor>
collect_head_parameters(std::vector<mdn::MdnModel> &heads) {
  std::vector<torch::Tensor> params;
  for (auto &head : heads) {
    for (auto &param : head->parameters()) {
      params.push_back(param);
    }
  }
  return params;
}

void test_default_config_dry_run_report() {
  const auto bundle = builder::load_graph_first_config_bundle_from_config(
      "/cuwacunu/src/config/.config");
  check(!bundle.source_universe.source_forms.empty(),
        "default bundle has Ujcamei source universe");
  check(bundle.source_universe.data_analytics_policy.declared,
        "default bundle keeps source analytics policy in Ujcamei universe");
  check(bundle.source_dock.channel_forms.size() == 14,
        "default bundle keeps channels in Kikijyeba source dock");
  check(bundle.source_plan.market_graph.node_ids.size() == 3,
        "default resolved source plan has graph nodes");
  check(bundle.source_plan.market_graph.edge_ids.size() == 3,
        "default resolved source plan has graph edges");
  check(bundle.source_plan.edge_instruments.size() == 3,
        "default resolved source plan has edge instruments");
  check(bundle.source_resolution_report.active_node_count == 3,
        "default resolver sees active nodes");
  check(bundle.source_resolution_report.possible_directed_edge_count == 6,
        "default resolver sees complete directed pair count");
  check(bundle.source_resolution_report.available_source_directed_edge_count ==
            3,
        "default resolver sees available source source edges");
  check(bundle.source_resolution_report.fetch_mode == "parallel_by_edge",
        "default resolver carries fetch mode");
  check(bundle.source_dock.fetch_mode ==
            builder::graph_first_fetch_mode_t::parallel_by_edge,
        "default source dock uses parallel edge fetch");
  check(bundle.source_resolution_report.selected_edge_count == 3,
        "default resolver sees selected graph edges");
  check(bundle.source_resolution_report.missing_directed_pair_count == 3,
        "default resolver records missing directed source pairs");
  check(bundle.source_resolution_report.available_unselected_edge_count == 0,
        "default resolver has no unused available source edges");
  check(bundle.source_resolution_report.selected_missing_source_edge_count == 0,
        "default resolver selected edges all have source coverage");
  check(bundle.source_resolution_report.connected_component_count == 1,
        "default resolver graph is connected");
  check(bundle.source_resolution_report.selected_cycle_dimension == 1,
        "default resolver reports one selected graph cycle");
  builder::graph_first_pipeline_builder_options_t options{};
  options.dry_run = true;
  builder::graph_first_pipeline_builder_t<Kline> pipe(bundle, options);
  const auto report = pipe.dry_run_report();

  check(pipe.options().batch_size ==
            static_cast<std::size_t>(bundle.mdn_training.batch_size),
        "default builder batch size derives from config");
  check(pipe.batch_size_source() == "derived",
        "default builder reports derived batch size");
  check(pipe.options().dtype == torch::kFloat32,
        "default builder dtype derives from config");
  check(pipe.options().device.is_cpu(),
        "default builder device derives from config");
  check(report.effective_batch_size ==
            static_cast<std::size_t>(bundle.mdn_training.batch_size),
        "dry-run reports effective batch size");
  check(report.batch_size_source == "derived",
        "dry-run reports batch size source");
  check(report.fetch_mode == "parallel_by_edge", "dry-run reports fetch mode");
  check(report.parallel_min_work_items == 16,
        "dry-run reports parallel threshold");
  check(report.dtype == "float32", "dry-run reports dtype");
  check(report.device == "cpu", "dry-run reports device");
  check(report.mdn_seed == bundle.mdn_training.seed, "dry-run reports seed");
  check(report.mdn_checkpoint_every == bundle.mdn_training.checkpoint_every,
        "dry-run reports checkpoint cadence");
  check(report.mdn_report_every == bundle.mdn_training.report_every,
        "dry-run reports report cadence");
  check(report.mdn_validation_every == 0, "dry-run reports validation cadence");
  check(report.analytics_status == "decoded_validated_not_emitted",
        "dry-run reports analytics status");
  check(report.node_count == 3, "default dry-run node count");
  check(report.edge_count == 3, "default dry-run edge count");
  check(report.active_channel_count == 3, "default dry-run active channels");
  check(report.possible_directed_edge_count == 6,
        "default dry-run possible directed edges");
  check(report.available_source_directed_edge_count == 3,
        "default dry-run available source edges");
  check(report.missing_directed_pair_count == 3,
        "default dry-run missing directed pairs");
  check(report.selected_missing_source_edge_count == 0,
        "default dry-run selected edges have source coverage");
  check(report.connected_component_count == 1,
        "default dry-run connected component count");
  check(report.selected_cycle_dimension == 1,
        "default dry-run selected cycle dimension");
  check(report.input_length == 30, "default dry-run Hx");
  check(report.future_length == 1, "default dry-run Hf");
  check(report.target_coords ==
            std::vector<int64_t>({0, 1, 2, 3, 4, 5, 6, 7, 8}),
        "default target coords");
  check(report.required_feature_coords == std::vector<int64_t>({0, 1, 2, 3}),
        "default mask profile coords");
  check(report.mask_profile == "price_only", "default mask profile");
  check(report.head_policy == "per_node", "default head policy");
  check(report.target_domain == "node_future", "default target domain");
  check(report.context_dim == 8, "default context dim");
  check(!report.graph_order_fingerprint.empty(),
        "default graph fingerprint available");
  std::cout << "[composition_pipeline_builder dry-run] " << report.summary()
            << "\n";
}

void test_config_backed_forward_nll_smoke() {
  torch::manual_seed(41);
  const auto fixture = make_config_fixture("forward_nll");
  const auto bundle =
      builder::load_graph_first_config_bundle_from_config(fixture.config);
  check(bundle.source_universe.source_forms.size() == 4,
        "fixture source universe contains only source rows");
  check(bundle.source_dock.channel_forms.size() == 1,
        "fixture source dock contains channel activation rows");
  check(bundle.source_plan.edge_instruments.size() == 4,
        "fixture source plan resolves edge instruments");
  check(bundle.source_resolution_report.possible_directed_edge_count == 6,
        "fixture resolver sees complete directed pair count");
  check(bundle.source_resolution_report.available_source_directed_edge_count ==
            4,
        "fixture resolver sees four real source edges");
  check(bundle.source_resolution_report.selected_edge_count == 4,
        "fixture resolver sees selected source edges");
  check(bundle.source_resolution_report.reverse_pair_count == 2,
        "fixture resolver sees two selected reverse pairs");
  check(bundle.source_resolution_report.missing_directed_pair_count == 2,
        "fixture resolver records two missing directed pairs");
  check(bundle.source_resolution_report.connected_component_count == 1,
        "fixture resolver graph is connected");
  check(bundle.source_resolution_report.selected_cycle_dimension == 2,
        "fixture resolver reports reverse-pair cycle dimension");

  builder::graph_first_pipeline_builder_options_t options{};
  options.force_rebuild_cache = true;
  options.batch_size = 2;
  builder::graph_first_pipeline_builder_t<Kline> pipe(bundle, options);
  const auto report = pipe.dry_run_report();
  check(report.node_count == 3, "fixture dry-run node count");
  check(report.edge_count == 4, "fixture dry-run edge count");
  check(report.active_channel_count == 1, "fixture active channels");
  check(report.available_source_directed_edge_count == 4,
        "fixture dry-run available source edges");
  check(report.reverse_pair_count == 2, "fixture dry-run reverse pair count");
  check(report.selected_cycle_dimension == 2,
        "fixture dry-run cycle dimension");
  check(report.input_length == 2, "fixture Hx");
  check(report.future_length == 1, "fixture Hf");

  auto graph_source = pipe.make_graph_source();
  check(graph_source.size() >= 2, "fixture graph source has anchors");
  auto srl_graph = pipe.srl_graph();
  auto lifted_stream = pipe.make_node_lifted_stream(std::move(graph_source));
  auto encoder = pipe.make_vicreg_encoder();
  auto representation_stream =
      pipe.make_node_representation_stream(std::move(lifted_stream), encoder);
  check(representation_stream.has_next(),
        "fixture representation stream has batch");
  auto node_batch = representation_stream.next();
  auto mdn_batch =
      mdnstream::make_mdn_input_batch(node_batch, pipe.mdn_adapter_options());

  check(mdn_batch.context.sizes() ==
            torch::IntArrayRef({6, pipe.context_dim()}),
        "fixture MDN context shape");
  check(mdn_batch.future.sizes() == torch::IntArrayRef({6, 1, 1, 9}),
        "fixture future shape");

  auto heads = pipe.make_mdn_heads(
      /*context_dim=*/mdn_batch.context.size(1),
      /*channel_count=*/mdn_batch.future.size(1),
      /*horizon_count=*/mdn_batch.future.size(2));
  const auto nll_options = mdn::mdn_nll_options_from_spec(pipe.bundle().mdn);
  auto targets = mdn_batch.future;
  auto mask = mdnstream::combine_mdn_context_and_future_mask(
      mdn_batch.context_mask, mdn_batch.future_mask);
  check(mask.sum().item<int64_t>() > 0, "fixture has valid MDN targets");

  std::vector<torch::Tensor> loss_sums;
  std::vector<torch::Tensor> valid_counts;
  for (int64_t node_slot = 0; node_slot < report.node_count; ++node_slot) {
    auto rows = mdnstream::mdn_rows_for_node(mdn_batch, node_slot);
    auto mask_n = mask.index_select(0, rows);
    if (!mask_n.any().item<bool>()) {
      continue;
    }
    auto context_n = mdn_batch.context.index_select(0, rows);
    auto target_n = targets.index_select(0, rows);
    auto out = heads[node_slot]->forward_from_encoding(context_n);
    check(torch::isfinite(out.log_pi).all().item<bool>(), "MDN log_pi finite");
    check(torch::isfinite(out.mu).all().item<bool>(), "MDN mu finite");
    check(torch::isfinite(out.sigma).all().item<bool>(), "MDN sigma finite");
    auto nll = mdn::mdn_nll_map(out, target_n, mask_n, nll_options);
    check(torch::isfinite(nll).all().item<bool>(), "MDN NLL finite");
    loss_sums.push_back(nll.sum());
    valid_counts.push_back(mask_n.to(nll.scalar_type()).sum().clamp_min(1.0));
  }
  check(!loss_sums.empty(), "at least one node contributed NLL");
  auto loss = torch::stack(loss_sums).sum() /
              torch::stack(valid_counts).sum().clamp_min(1.0);
  check(std::isfinite(loss.item<double>()), "fixture total NLL finite");
  check(torch::isfinite(node_batch.node_encoding).all().item<bool>(),
        "node encoding finite");
}

void test_edge_discovery_policy() {
  const auto fixture = make_config_fixture("discover_edges",
                                           /*discover_edges=*/true);
  const auto bundle =
      builder::load_graph_first_config_bundle_from_config(fixture.config);
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
  builder::graph_first_pipeline_builder_t<Kline> pipe(bundle, options);
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
    test_default_config_dry_run_report();
    test_edge_discovery_policy();
    test_config_backed_forward_nll_smoke();
    std::cout << "[Jkimyei GraphFirstPipelineBuilder test] all checks passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "[Jkimyei GraphFirstPipelineBuilder test] failure: "
              << ex.what() << "\n";
    return 1;
  }
}
