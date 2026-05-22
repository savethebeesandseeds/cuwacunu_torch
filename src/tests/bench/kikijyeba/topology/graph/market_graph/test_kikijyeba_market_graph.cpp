// SPDX-License-Identifier: MIT
#include "kikijyeba/topology/graph/graph.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace graph = cuwacunu::kikijyeba::topology::graph;
namespace source = cuwacunu::ujcamei::source;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "[MarketGraph test] " << message << "\n";
    std::exit(1);
  }
}

template <typename Fn> void expect_throw(Fn &&fn, const std::string &message) {
  try {
    fn();
  } catch (const std::exception &) {
    return;
  }
  std::cerr << "[MarketGraph test] expected throw: " << message << "\n";
  std::exit(1);
}

source::registry::instrument_signature_t
signature(std::string symbol, std::string base, std::string quote) {
  return source::registry::instrument_signature_t{
      .symbol = std::move(symbol),
      .record_type = "KLINE",
      .market_type = "SPOT",
      .venue = "BINANCE",
      .base_asset = std::move(base),
      .quote_asset = std::move(quote),
  };
}

void test_directed_instrument_edge() {
  const auto sig = signature("BTCUSDT", "BTC", "USDT");
  const auto edge = graph::make_directed_instrument_edge(sig);
  check(edge.edge_id == "BTCUSDT", "edge id from instrument symbol");
  check(edge.base_node_id == "BTC", "base node id");
  check(edge.quote_node_id == "USDT", "quote node id");
  check(edge.instrument.symbol == "BTCUSDT", "instrument retained");

  const auto generic = graph::make_directed_edge("ETHBTC", "ETH", "BTC");
  check(generic.edge_id == "ETHBTC", "generic edge id");
  check(generic.base_node_id == "ETH", "generic base node id");
  check(generic.quote_node_id == "BTC", "generic quote node id");
}

void test_market_graph_order() {
  const std::vector<graph::directed_instrument_edge_t> edges{
      graph::make_directed_instrument_edge(signature("BTCUSDT", "BTC", "USDT")),
      graph::make_directed_instrument_edge(signature("ETHBTC", "ETH", "BTC")),
  };
  const auto market = graph::make_market_graph(edges);
  check(market.num_nodes() == 3, "node count");
  check(market.num_edges() == 2, "edge count");
  check(market.node_ids ==
            std::vector<graph::asset_node_id_t>{"BTC", "USDT", "ETH"},
        "stable node order");
  check(market.edge_ids ==
            std::vector<graph::instrument_edge_id_t>{"BTCUSDT", "ETHBTC"},
        "stable edge order");
  check(market.base_index == std::vector<std::int64_t>{0, 2}, "base indices");
  check(market.quote_index == std::vector<std::int64_t>{1, 0}, "quote indices");
  check(market.node_index_or_throw("ETH") == 2, "node lookup");
  check(market.edge_index_or_throw("ETHBTC") == 1, "edge lookup");
  check(market.base_node_id(1) == "ETH", "base node lookup");
  check(market.quote_node_id(1) == "BTC", "quote node lookup");
  const auto edge = market.directed_edge("BTCUSDT");
  check(edge.edge_id == "BTCUSDT", "directed edge view id");
  check(edge.base_node_id == "BTC", "directed edge view base");
  check(edge.quote_node_id == "USDT", "directed edge view quote");
  check(!market.has_reverse_edge(0), "no synthetic reverse edge");
  market.validate();
}

void test_real_reverse_edge() {
  const std::vector<graph::directed_instrument_edge_t> edges{
      graph::make_directed_instrument_edge(signature("BTCUSDT", "BTC", "USDT")),
      graph::make_directed_instrument_edge(signature("USDTBTC", "USDT", "BTC")),
  };
  const auto market = graph::make_market_graph(edges);
  check(market.has_reverse_edge(0), "real reverse edge detected");
  check(market.has_reverse_edge(1), "reverse relation symmetric");
}

void test_validation_failures() {
  expect_throw(
      [] {
        graph::market_graph_t{
            .node_ids = {"BTC", "BTC"},
            .edge_ids = {"BTCUSDT"},
            .base_index = {0},
            .quote_index = {1},
        }
            .validate();
      },
      "duplicate node ids");
  expect_throw(
      [] {
        graph::market_graph_t{
            .node_ids = {"BTC", "USDT"},
            .edge_ids = {"BTCUSDT", "BTCUSDT"},
            .base_index = {0, 0},
            .quote_index = {1, 1},
        }
            .validate();
      },
      "duplicate edge ids");
  expect_throw(
      [] {
        graph::market_graph_t{
            .node_ids = {"BTC"},
            .edge_ids = {"BTCBTC"},
            .base_index = {0},
            .quote_index = {0},
        }
            .validate();
      },
      "self edge");
  expect_throw(
      [] {
        graph::market_graph_t{
            .node_ids = {"BTC", "USDT"},
            .edge_ids = {"BTCUSDT"},
            .base_index = {0},
            .quote_index = {2},
        }
            .validate();
      },
      "endpoint out of range");
  expect_throw(
      [] {
        const auto market = graph::make_market_graph({
            graph::make_directed_instrument_edge(
                signature("BTCUSDT", "BTC", "USDT")),
        });
        (void)market.node_index_or_throw("ETH");
      },
      "unknown node lookup");
  expect_throw(
      [] {
        const auto market = graph::make_market_graph({
            graph::make_directed_instrument_edge(
                signature("BTCUSDT", "BTC", "USDT")),
        });
        (void)market.edge_index_or_throw("ETHBTC");
      },
      "unknown edge lookup");
}

} // namespace

int main() {
  test_directed_instrument_edge();
  test_market_graph_order();
  test_real_reverse_edge();
  test_validation_failures();
  std::cout << "[MarketGraph test] ok\n";
  return 0;
}
