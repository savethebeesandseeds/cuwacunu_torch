#include "ujcamei/source/contract/runtime/decode.h"
#include "ujcamei/source/contract/validation/nodelift_compatibility.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace contract = cuwacunu::ujcamei::source::contract;
namespace graph = cuwacunu::kikijyeba::topology::graph;
namespace source = cuwacunu::ujcamei::source;
namespace types = cuwacunu::ujcamei::source::registry::types;
namespace validation = cuwacunu::ujcamei::source::contract::validation;

namespace {

std::string read_text(const std::string &path) {
  std::ifstream in(path);
  assert(in && "expected readable source config file");
  std::ostringstream oss;
  oss << in.rdbuf();
  return oss.str();
}

void assert_no_old_runtime_names(const std::string &text) {
  const std::string old_source_prefix = std::string("t") + "si";
  const std::string old_runtime = std::string("t") + "sie" + "mene";
  const std::string old_contract = std::string("ii") + "tepi";
  const std::string old_hash = std::string("hashi") + "myei";
  const std::string old_dsl_owner = std::string("camah") + "jucunu";
  assert(text.find(old_source_prefix) == std::string::npos);
  assert(text.find(old_runtime) == std::string::npos);
  assert(text.find(old_contract) == std::string::npos);
  assert(text.find(old_hash) == std::string::npos);
  assert(text.find(old_dsl_owner) == std::string::npos);
}

template <typename Fn> void expect_throw(Fn &&fn) {
  bool threw = false;
  try {
    fn();
  } catch (const std::exception &) {
    threw = true;
  }
  assert(threw);
}

std::string replace_once(std::string text, const std::string &from,
                         const std::string &to) {
  const std::size_t pos = text.find(from);
  if (pos == std::string::npos) {
    throw std::runtime_error("test fixture replacement source not found");
  }
  text.replace(pos, from.size(), to);
  return text;
}

} // namespace

int main() {
  const auto config_path = contract::default_source_config_path();
  const auto paths =
      contract::load_source_registry_config_paths_from_config(config_path);

  assert(paths.source_registry_dsl_bnf_path.find(
             "ujcamei.source.registry.dsl.bnf") != std::string::npos);
  assert(paths.source_registry_dsl_path.find("ujcamei.source.registry.dsl") !=
         std::string::npos);

  const auto config_text = read_text(config_path);
  assert_no_old_runtime_names(config_text);
  assert(config_text.find(std::string("ujcamei") + "_cursors") ==
         std::string::npos);
  const auto sources_bnf = read_text(paths.source_registry_dsl_bnf_path);
  assert_no_old_runtime_names(sources_bnf);
  const auto sources_dsl = read_text(paths.source_registry_dsl_path);
  const auto channels_bnf = read_text(
      "/cuwacunu/src/config/grammar/ujcamei.source.retrieval.channels.dsl.bnf");
  const auto channels_dsl =
      read_text("/cuwacunu/src/config/ujcamei.source.retrieval.channels.dsl");
  const auto graph_bnf = read_text("/cuwacunu/src/config/grammar/"
                                   "kikijyeba.topology.graph.dsl.bnf");
  const auto graph_dsl =
      read_text("/cuwacunu/src/config/kikijyeba.topology.graph.dsl");
  assert_no_old_runtime_names(sources_dsl);
  assert(sources_dsl.find("source_kind") != std::string::npos);

  auto universe = contract::decode_source_universe_from_default_config();
  assert(universe.source_forms.size() == 59);
  bool saw_basic_source_row = false;
  for (const auto &source_form : universe.source_forms) {
    saw_basic_source_row =
        saw_basic_source_row || source_form.record_type == "basic";
  }
  assert(saw_basic_source_row);
  assert(universe.csv_bootstrap_deltas == 128);
  assert(universe.data_analytics_policy.declared);
  assert(universe.data_analytics_policy.max_samples == 4096);
  assert(universe.data_analytics_policy.max_features == 2048);

  auto spec = contract::decode_source_spec_from_default_config();

  assert(spec.source_forms.size() == 59);
  assert(spec.channel_forms.size() == 14);
  assert(spec.graph_node_forms.size() == 3);
  assert(spec.graph_edge_forms.empty());
  assert(spec.graph_edge_resolution_policy == "edge_discovery");
  assert(spec.graph_edge_source_kind == "real");
  assert(spec.graph_fetch_mode == "parallel_by_edge");
  assert(spec.graph_max_fetch_workers == "0");
  assert(spec.graph_parallel_min_work_items == "16");
  assert(spec.csv_bootstrap_deltas == 128);
  assert(spec.data_analytics_policy.declared);
  assert(spec.data_analytics_policy.max_samples == 4096);
  assert(spec.data_analytics_policy.max_features == 2048);
  assert(spec.count_channels() == 3);
  assert(spec.max_input_length() == 30);
  assert(spec.max_future_length() == 1);

  const source::registry::instrument_signature_t btc_usdt{
      .symbol = "BTCUSDT",
      .record_type = "kline",
      .market_type = "spot",
      .venue = "binance",
      .base_asset = "BTC",
      .quote_asset = "USDT",
  };
  std::string error;
  assert(spec.active_sources_match_runtime_signature(btc_usdt, &error));

  const auto daily_sources =
      spec.filter_source_forms(btc_usdt, types::interval_type_e::interval_1d);
  assert(daily_sources.size() == 1);
  assert(daily_sources.front().source_kind == "real");
  assert(daily_sources.front().source.find("BTCUSDT/1d") != std::string::npos);

  const source::registry::instrument_signature_t utilities{
      .symbol = "UTILITIES",
      .record_type = "kline",
      .market_type = "synthetic",
      .venue = "local",
      .base_asset = "UTILITIES",
      .quote_asset = "NONE",
  };
  const auto utility_sources =
      spec.filter_source_forms(utilities, types::interval_type_e::interval_1d);
  assert(utility_sources.size() == 1);
  assert(utility_sources.front().source_kind == "synthetic");

  const std::string explicit_graph_dsl = R"(GRAPH_POLICY {
  EDGE_RESOLUTION_POLICY = explicit_only;
  EDGE_SOURCE_KIND = real;
  FETCH_MODE = serial;
  MAX_FETCH_WORKERS = 0;
  PARALLEL_MIN_WORK_ITEMS = 16;
};
/------------------------------------\
|  node_id  |  node_kind  |  active  |
|------------------------------------|
|   BTC     |    asset    |   true   |
|   USDT    |    asset    |   true   |
|   ETH     |    asset    |   true   |
\------------------------------------/
/-----------------------------------------------------------------------\
|  edge_id  |  base_node  |  quote_node  |  source_instrument  | active |
|-----------------------------------------------------------------------|
| BTCUSDT   |    BTC      |    USDT      |       BTCUSDT       |  true  |
| ETHUSDT   |    ETH      |    USDT      |       ETHUSDT       |  true  |
| ETHBTC    |    ETH      |    BTC       |       ETHBTC        |  true  |
\-----------------------------------------------------------------------/
)";
  auto graph_spec = contract::decode_source_spec_from_split_dsl(
      sources_bnf, sources_dsl, channels_bnf, channels_dsl, graph_bnf,
      explicit_graph_dsl);
  assert(graph_spec.graph_edge_forms.size() == 3);
  assert(graph_spec.graph_edge_resolution_policy == "explicit_only");
  assert(graph_spec.graph_edge_source_kind == "real");

  const graph::market_graph_t market_graph = graph_spec.active_market_graph();
  assert(market_graph.node_ids ==
         std::vector<std::string>({"BTC", "USDT", "ETH"}));
  assert(market_graph.edge_ids ==
         std::vector<std::string>({"BTCUSDT", "ETHUSDT", "ETHBTC"}));
  assert(market_graph.base_node_id(0) == "BTC");
  assert(market_graph.quote_node_id(0) == "USDT");
  assert(market_graph.base_node_id(1) == "ETH");
  assert(market_graph.quote_node_id(1) == "USDT");
  assert(market_graph.base_node_id(2) == "ETH");
  assert(market_graph.quote_node_id(2) == "BTC");

  const auto instruments = graph_spec.active_edge_instrument_map();
  assert(instruments.size() == 3);
  assert(instruments.at("BTCUSDT").symbol == "BTCUSDT");
  assert(instruments.at("ETHUSDT").base_asset == "ETH");
  assert(instruments.at("ETHBTC").quote_asset == "BTC");

  const auto base_identity = validation::make_nodelift_compatibility_identity(
      market_graph, &graph_spec);
  auto channel_mutated = graph_spec;
  channel_mutated.channel_forms.front().input_length = "99";
  const auto channel_identity =
      validation::make_nodelift_compatibility_identity(market_graph,
                                                       &channel_mutated);
  assert(channel_identity.ujcamei_source_universe_hash ==
         base_identity.ujcamei_source_universe_hash);
  assert(channel_identity.graph_first_channel_hash !=
         base_identity.graph_first_channel_hash);

  auto graph_mutated = graph_spec;
  graph_mutated.graph_edge_forms.front().active = "false";
  const auto graph_identity = validation::make_nodelift_compatibility_identity(
      market_graph, &graph_mutated);
  assert(graph_identity.ujcamei_source_universe_hash ==
         base_identity.ujcamei_source_universe_hash);
  assert(graph_identity.graph_first_graph_hash !=
         base_identity.graph_first_graph_hash);

  auto source_mutated = graph_spec;
  source_mutated.source_forms.front().source += ".next";
  const auto source_identity = validation::make_nodelift_compatibility_identity(
      market_graph, &source_mutated);
  assert(source_identity.ujcamei_source_universe_hash !=
         base_identity.ujcamei_source_universe_hash);

  const std::string missing_channel_source_dsl =
      replace_once(channels_dsl, "|    12h      |   false   |    kline",
                   "|    1s       |   false   |    kline");
  expect_throw([&] {
    (void)contract::decode_source_spec_from_split_dsl(
        sources_bnf, sources_dsl, channels_bnf, missing_channel_source_dsl,
        graph_bnf, graph_dsl);
  });

  const std::string invalid_channel_interval_dsl =
      replace_once(channels_dsl, "|    1d       |   true    |    kline",
                   "|    bad      |   true    |    kline");
  expect_throw([&] {
    (void)contract::decode_source_spec_from_split_dsl(
        sources_bnf, sources_dsl, channels_bnf, invalid_channel_interval_dsl,
        graph_bnf, graph_dsl);
  });

  const std::string weighted_channel_dsl =
      replace_once(channels_dsl, "|      1.0       |", "|      2.0       |");
  expect_throw([&] {
    (void)contract::decode_source_spec_from_split_dsl(
        sources_bnf, sources_dsl, channels_bnf, weighted_channel_dsl, graph_bnf,
        graph_dsl);
  });

  const std::string active_basic_channel_dsl =
      replace_once(channels_dsl, "|    1d       |   true    |    kline",
                   "|    1d       |   true    |    basic");
  expect_throw([&] {
    (void)contract::decode_source_spec_from_split_dsl(
        sources_bnf, sources_dsl, channels_bnf, active_basic_channel_dsl,
        graph_bnf, graph_dsl);
  });

  const std::string missing_graph_source_dsl =
      replace_once(explicit_graph_dsl,
                   "| BTCUSDT   |    BTC      |    USDT      |       BTCUSDT   "
                   "    |  true  |",
                   "| BTCUSDTX  |    BTC      |    USDT      |       BTCUSDTX  "
                   "    |  true  |");
  expect_throw([&] {
    (void)contract::decode_source_spec_from_split_dsl(
        sources_bnf, sources_dsl, channels_bnf, channels_dsl, graph_bnf,
        missing_graph_source_dsl);
  });

  const std::string synthetic_graph_dsl = R"(/*
  synthetic active graph should be rejected for NodeLift v1
*/
/--------------------------------------\
|  node_id    |  node_kind  |  active  |
|--------------------------------------|
|  UTILITIES  |    asset    |   true   |
|  NONE       |    asset    |   true   |
\--------------------------------------/
/-------------------------------------------------------------------------\
|  edge_id    |  base_node  |  quote_node  |  source_instrument  | active |
|-------------------------------------------------------------------------|
| UTILITIES   |  UTILITIES  |    NONE      |      UTILITIES      |  true  |
\-------------------------------------------------------------------------/
)";
  expect_throw([&] {
    (void)contract::decode_source_spec_from_split_dsl(
        sources_bnf, sources_dsl, channels_bnf, channels_dsl, graph_bnf,
        synthetic_graph_dsl);
  });

  std::cout << "[test_ujcamei_source_contract] ok\n";
  return 0;
}
