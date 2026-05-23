// SPDX-License-Identifier: MIT
#include "wikimyei/expression/nodelift/srl/stream/node_lifted_stream.h"
#include "wikimyei/inference/expected_value/mdn/stream/legacy_node_mdn_adapter.h"
#include "wikimyei/representation/encoding/vicreg/stream/node_representation_stream.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include <unistd.h>

namespace dl = cuwacunu::ujcamei::source::retrieval::dataloader;
namespace graph = cuwacunu::kikijyeba::topology::graph;
namespace contract = cuwacunu::ujcamei::source::contract;
namespace mm = cuwacunu::ujcamei::source::retrieval::storage::memory_mapped;
namespace srl = cuwacunu::wikimyei::expression::nodelift::srl;
namespace stream = cuwacunu::wikimyei::expression::nodelift::srl::stream;
namespace mdnstream =
    cuwacunu::wikimyei::inference::expected_value::mdn::stream;
namespace repstream =
    cuwacunu::wikimyei::representation::encoding::vicreg::stream;
namespace runtime_lls = cuwacunu::kikijyeba::lattice::runtime_report;
namespace source = cuwacunu::ujcamei::source;
namespace types = cuwacunu::ujcamei::source::registry::types;

namespace {

using Kline = types::kline_t;

static_assert(
    !std::is_copy_constructible_v<dl::graph_anchor_edge_dataset_t<Kline>>,
    "graph_anchor_edge_dataset_t should be move-only");
static_assert(
    std::is_constructible_v<stream::node_lifted_stream_t<Kline>,
                            dl::graph_anchor_edge_dataset_t<Kline> &&,
                            srl::graph_t,
                            stream::node_lifted_stream_options_t<Kline>>,
    "node_lifted_stream_t should take graph_anchor_edge_dataset_t by move");
static_assert(
    !std::is_constructible_v<stream::node_lifted_stream_t<Kline>,
                             dl::graph_anchor_edge_dataset_t<Kline> &,
                             srl::graph_t,
                             stream::node_lifted_stream_options_t<Kline>>,
    "node_lifted_stream_t should not copy graph_anchor_edge_dataset_t");

void check(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "[NodeLiftedStream test] " << message << "\n";
    std::exit(1);
  }
}

double scalar(const torch::Tensor &t) {
  return t.detach().to(torch::kCPU).to(torch::kFloat64).item<double>();
}

double max_abs(const torch::Tensor &t) {
  if (!t.defined() || t.numel() == 0) {
    return 0.0;
  }
  return scalar(t.detach().abs().max());
}

void close(double actual, double expected, double tol, const std::string &msg) {
  if (std::fabs(actual - expected) > tol) {
    std::cerr << "[NodeLiftedStream test] " << msg << " actual=" << actual
              << " expected=" << expected << "\n";
    std::exit(1);
  }
}

struct MeanNodeEncoder {
  torch::Tensor forward(const torch::Tensor &data, const torch::Tensor &mask) {
    auto mask_f = mask.to(data.scalar_type()).unsqueeze(-1);
    auto denom = mask_f.sum(std::vector<int64_t>{1, 2}).clamp_min(1.0); // [M,1]
    return (data * mask_f).sum(std::vector<int64_t>{1, 2}) / denom;
  }
};

template <typename Fn> void expect_throw(Fn &&fn, const std::string &msg) {
  try {
    fn();
  } catch (const c10::Error &) {
    return;
  } catch (const std::exception &) {
    return;
  }
  std::cerr << "[NodeLiftedStream test] expected throw: " << msg << "\n";
  std::exit(1);
}

torch::Tensor make_keys(int64_t C, int64_t H, int64_t anchor, bool future) {
  auto keys = torch::empty({C, H}, torch::TensorOptions().dtype(torch::kInt64));
  for (int64_t c = 0; c < C; ++c) {
    for (int64_t h = 0; h < H; ++h) {
      keys.index_put_({c, h}, future ? anchor + h + 1 : anchor - (H - 1 - h));
    }
  }
  return keys;
}

dl::edge_sample_t make_sample(double price, int64_t anchor) {
  dl::edge_sample_t sample{};
  sample.features =
      torch::zeros({1, 1, types::kKlineFeatureWidth}, torch::kFloat32);
  sample.features.index_put_({0, 0, 0}, price);
  sample.features.index_put_({0, 0, 1}, price + 0.1);
  sample.features.index_put_({0, 0, 2}, price - 0.1);
  sample.features.index_put_({0, 0, 3}, price);
  sample.mask = torch::ones({1, 1}, torch::kBool);
  sample.future_features = torch::full({1, 2, types::kKlineFeatureWidth},
                                       price + 10.0, torch::kFloat32);
  sample.future_mask = torch::ones({1, 2}, torch::kBool);
  sample.past_keys = make_keys(1, 1, anchor, false);
  sample.future_keys = make_keys(1, 2, anchor, true);
  sample.normalized = true;
  return sample;
}

dl::graph_anchor_edge_sample_t<int64_t>
wrap(std::string edge_id, int64_t anchor, dl::edge_sample_t sample) {
  return dl::graph_anchor_edge_sample_t<int64_t>{
      .edge_id = std::move(edge_id),
      .anchor_key = anchor,
      .sample = std::move(sample),
  };
}

srl::graph_t make_srl_graph(std::vector<std::string> edge_ids = {"e0", "e1"}) {
  srl::graph_t out{};
  out.node_ids = {"n0", "n1", "n2"};
  out.edge_ids = std::move(edge_ids);
  out.base_index =
      torch::tensor({0, 1}, torch::TensorOptions().dtype(torch::kInt64));
  out.quote_index =
      torch::tensor({1, 2}, torch::TensorOptions().dtype(torch::kInt64));
  return out;
}

graph::market_graph_t make_market_graph() {
  graph::market_graph_t out{};
  out.node_ids = {"BTC", "USDT", "ETH"};
  out.edge_ids = {"e0", "e1"};
  out.base_index = {0, 1};
  out.quote_index = {1, 2};
  out.validate();
  return out;
}

void test_bridge_batch() {
  std::vector<dl::graph_anchor_edge_sample_t<int64_t>> samples{
      wrap("e0", 100, make_sample(2.0, 100)),
      wrap("e1", 100, make_sample(3.0, 100)),
  };
  dl::graph_anchor_edge_batch_options_t options{};
  options.edge_ids = {"e0", "e1"};
  auto graph_batch = dl::make_graph_anchor_edge_batch(samples, options);
  auto graph = make_srl_graph();
  graph_batch.graph_order_fingerprint =
      graph.computed_graph_order_fingerprint();

  auto lifted =
      stream::node_lift_graph_anchor_edge_batch<int64_t>(graph_batch, graph);
  check(lifted.node_features.sizes() == torch::IntArrayRef({1, 1, 1, 3, 9}),
        "node feature shape");
  check(lifted.node_mask.sizes() == torch::IntArrayRef({1, 1, 1, 3, 9}),
        "node mask shape");
  check(lifted.price_residual.sizes() == torch::IntArrayRef({1, 1, 1, 2, 4}),
        "price residual shape");
  check(lifted.future_node_features.sizes() ==
            torch::IntArrayRef({1, 1, 2, 3, 9}),
        "future node feature shape");
  check(lifted.future_node_mask.sizes() == torch::IntArrayRef({1, 1, 2, 3, 9}),
        "future node mask shape");
  check(lifted.future_price_residual.sizes() ==
            torch::IntArrayRef({1, 1, 2, 2, 4}),
        "future price residual shape");
  close(scalar((lifted.future_edge_features - graph_batch.future_features)
                   .abs()
                   .sum()),
        0.0, 1e-8, "future edge features carried through");
  close(scalar(lifted.anchor_keys.index({0})), 100.0, 1e-6,
        "anchor carried through");
  check(lifted.edge_ids == graph.edge_ids, "edge ids carried through");
  check(lifted.node_ids == graph.node_ids, "node ids carried through");
  check(lifted.graph_order_fingerprint == graph_batch.graph_order_fingerprint,
        "graph fingerprint carried through");
  check(!torch::isnan(lifted.node_features).any().item<bool>(),
        "node features contain no NaNs");
  close(scalar(lifted.alignment_diagnostics.max_past_lag_by_anchor.index({0})),
        0.0, 1e-6, "alignment diagnostics attached");

  auto no_alignment = stream::node_lift_graph_anchor_edge_batch<int64_t>(
      graph_batch, graph, srl::nodelift_options_t{},
      /*compute_alignment_diagnostics=*/false);
  check(!no_alignment.alignment_diagnostics.max_past_lag_by_anchor.defined(),
        "alignment diagnostics can be disabled");

  auto no_future_lift = stream::node_lift_graph_anchor_edge_batch<int64_t>(
      graph_batch, graph, srl::nodelift_options_t{},
      /*compute_alignment_diagnostics=*/true, /*lift_future=*/false);
  check(!no_future_lift.future_node_features.defined(),
        "future NodeLift sidecars can be disabled");

  expect_throw(
      [&] {
        auto bad_graph = make_srl_graph({"e1", "e0"});
        auto bad = stream::node_lift_graph_anchor_edge_batch<int64_t>(
            graph_batch, bad_graph);
        (void)bad;
      },
      "edge order mismatch");

  expect_throw(
      [&] {
        auto bad_batch = graph_batch;
        bad_batch.graph_order_fingerprint = "bad-fingerprint";
        auto bad = stream::node_lift_graph_anchor_edge_batch<int64_t>(bad_batch,
                                                                      graph);
        (void)bad;
      },
      "graph fingerprint mismatch");
}

void test_future_nodelift_is_target_side_only() {
  std::vector<dl::graph_anchor_edge_sample_t<int64_t>> samples{
      wrap("e0", 100, make_sample(2.0, 100)),
      wrap("e1", 100, make_sample(3.0, 100)),
  };
  dl::graph_anchor_edge_batch_options_t options{};
  options.edge_ids = {"e0", "e1"};
  auto graph_batch_a = dl::make_graph_anchor_edge_batch(samples, options);
  auto graph_batch_b = graph_batch_a;
  graph_batch_b.future_features = graph_batch_a.future_features.clone() + 7.0;

  auto srl_graph = make_srl_graph();
  graph_batch_a.graph_order_fingerprint =
      srl_graph.computed_graph_order_fingerprint();
  graph_batch_b.graph_order_fingerprint =
      srl_graph.computed_graph_order_fingerprint();

  auto lifted_a = stream::node_lift_graph_anchor_edge_batch<int64_t>(
      graph_batch_a, srl_graph);
  auto lifted_b = stream::node_lift_graph_anchor_edge_batch<int64_t>(
      graph_batch_b, srl_graph);

  close(max_abs(lifted_a.node_features - lifted_b.node_features), 0.0, 1e-8,
        "future changes must not change observed node features");
  close(max_abs(lifted_a.node_mask.to(torch::kInt64) -
                lifted_b.node_mask.to(torch::kInt64)),
        0.0, 1e-8, "future changes must not change observed node masks");
  check(max_abs(lifted_a.future_node_features - lifted_b.future_node_features) >
            1e-4,
        "future changes should affect future NodeLift sidecars");

  MeanNodeEncoder encoder{};
  auto rep_a =
      repstream::make_node_representation_batch<MeanNodeEncoder, int64_t>(
          encoder, lifted_a);
  auto rep_b =
      repstream::make_node_representation_batch<MeanNodeEncoder, int64_t>(
          encoder, lifted_b);
  close(max_abs(rep_a.node_encoding - rep_b.node_encoding), 0.0, 1e-8,
        "future changes must not change node encodings");

  auto mdn_a = mdnstream::make_mdn_input_batch(rep_a);
  auto mdn_b = mdnstream::make_mdn_input_batch(rep_b);
  close(max_abs(mdn_a.context - mdn_b.context), 0.0, 1e-8,
        "future changes must not change MDN conditioning context");
  close(max_abs(mdn_a.context_mask.to(torch::kInt64) -
                mdn_b.context_mask.to(torch::kInt64)),
        0.0, 1e-8, "future changes must not change MDN context mask");
  check(max_abs(mdn_a.future - mdn_b.future) > 1e-4,
        "future changes should affect MDN future node targets");
}

void test_future_mask_and_residual_reconstruction() {
  std::vector<dl::graph_anchor_edge_sample_t<int64_t>> samples{
      wrap("e0", 100, make_sample(2.0, 100)),
      wrap("e1", 100, make_sample(3.0, 100)),
  };
  dl::graph_anchor_edge_batch_options_t options{};
  options.edge_ids = {"e0", "e1"};
  auto graph_batch = dl::make_graph_anchor_edge_batch(samples, options);
  auto graph = make_srl_graph();
  graph_batch.graph_order_fingerprint =
      graph.computed_graph_order_fingerprint();

  auto lifted =
      stream::node_lift_graph_anchor_edge_batch<int64_t>(graph_batch, graph);
  for (int64_t h = 0; h < graph_batch.future_features.size(3); ++h) {
    for (int64_t e = 0; e < static_cast<int64_t>(graph.edge_ids.size()); ++e) {
      const int64_t base = graph.base_index.index({e}).item<int64_t>();
      const int64_t quote = graph.quote_index.index({e}).item<int64_t>();
      for (int64_t p = 0; p < srl::kPriceWidth; ++p) {
        if (!lifted.future_price_residual_mask.index({0, 0, h, e, p})
                 .item<bool>()) {
          continue;
        }
        const double original =
            scalar(graph_batch.future_features.index({0, e, 0, h, p}));
        const double reconstructed =
            scalar(lifted.future_node_features.index({0, 0, h, base, p})) -
            scalar(lifted.future_node_features.index({0, 0, h, quote, p})) +
            scalar(lifted.future_price_residual.index({0, 0, h, e, p}));
        close(reconstructed, original, 1e-4,
              "future price residual reconstructs original edge coordinate");
      }
    }
  }

  auto masked_future = graph_batch;
  masked_future.future_features =
      torch::full_like(graph_batch.future_features, 123.0);
  masked_future.future_mask = torch::zeros_like(graph_batch.future_mask);
  auto masked_lifted =
      stream::node_lift_graph_anchor_edge_batch<int64_t>(masked_future, graph);
  check(!masked_lifted.future_node_mask.any().item<bool>(),
        "masked future edges must produce false future node masks");
  check(!masked_lifted.future_price_residual_mask.any().item<bool>(),
        "masked future edges must produce false future residual masks");
  close(scalar(masked_lifted.future_diagnostics.valid_edge_count.sum()), 0.0,
        1e-8, "masked future edges have zero valid future edge count");

  auto missing_future_mask = graph_batch;
  missing_future_mask.future_mask = torch::Tensor{};
  expect_throw(
      [&] {
        auto bad = stream::node_lift_graph_anchor_edge_batch<int64_t>(
            missing_future_mask, graph);
        (void)bad;
      },
      "future shape validation requires future mask");
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  static int counter = 0;
  auto dir = std::filesystem::temp_directory_path() /
             ("cuwacunu_node_lifted_stream_" + label + "_" +
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

mm::MemoryMappedEdgeDataset<Kline>
make_edge_dataset(const std::filesystem::path &csv) {
  mm::MemoryMappedEdgeDataset<Kline> dataset;
  dataset.add_dataset(csv.string(), 2, 1, "none", true);
  return dataset;
}

srl::graph_t make_srl_graph_from_market(const graph::market_graph_t &market) {
  srl::graph_t out{};
  out.node_ids = market.node_ids;
  out.edge_ids = market.edge_ids;
  out.base_index = torch::tensor(market.base_index,
                                 torch::TensorOptions().dtype(torch::kInt64));
  out.quote_index = torch::tensor(market.quote_index,
                                  torch::TensorOptions().dtype(torch::kInt64));
  out.graph_order_fingerprint = market.computed_graph_order_fingerprint();
  return out;
}

source::registry::instrument_signature_t
make_signature(std::string symbol, std::string base, std::string quote) {
  return source::registry::instrument_signature_t{
      .symbol = std::move(symbol),
      .record_type = "kline",
      .market_type = "spot",
      .venue = "binance",
      .base_asset = std::move(base),
      .quote_asset = std::move(quote),
  };
}

contract::source_spec_t
make_source_spec(const source::registry::instrument_signature_t &btc_usdt,
                 const source::registry::instrument_signature_t &eth_usdt,
                 const std::filesystem::path &btc_csv,
                 const std::filesystem::path &eth_csv) {
  contract::source_spec_t spec{};
  spec.csv_bootstrap_deltas = 2;
  spec.source_forms = {
      contract::source_form_t{
          .instrument = btc_usdt.symbol,
          .interval = types::interval_type_e::interval_1m,
          .record_type = btc_usdt.record_type,
          .market_type = btc_usdt.market_type,
          .venue = btc_usdt.venue,
          .base_asset = btc_usdt.base_asset,
          .quote_asset = btc_usdt.quote_asset,
          .source = btc_csv.string(),
      },
      contract::source_form_t{
          .instrument = eth_usdt.symbol,
          .interval = types::interval_type_e::interval_1m,
          .record_type = eth_usdt.record_type,
          .market_type = eth_usdt.market_type,
          .venue = eth_usdt.venue,
          .base_asset = eth_usdt.base_asset,
          .quote_asset = eth_usdt.quote_asset,
          .source = eth_csv.string(),
      },
  };
  spec.channel_forms = {
      contract::channel_form_t{
          .interval = types::interval_type_e::interval_1m,
          .active = "true",
          .record_type = "kline",
          .input_length = "2",
          .future_length = "1",
          .channel_weight = "1",
          .normalization_policy = "log_returns",
      },
  };
  return spec;
}

graph::market_graph_t make_contract_market_graph(
    const source::registry::instrument_signature_t &btc_usdt,
    const source::registry::instrument_signature_t &eth_usdt) {
  return graph::make_market_graph({
      graph::make_directed_instrument_edge(btc_usdt),
      graph::make_directed_instrument_edge(eth_usdt),
  });
}

void test_node_lifted_stream() {
  const auto dir = make_tmp_dir("node_lifted_stream");
  const auto csv0 = dir / "e0.csv";
  const auto csv1 = dir / "e1.csv";
  write_kline_csv(csv0, {1000, 1001, 1002, 1003, 1004, 1005}, 10.0);
  write_kline_csv(csv1, {998, 1000, 1002, 1004, 1006, 1008}, 20.0);

  dl::graph_anchor_edge_dataset_t<Kline>::edge_dataset_map_t datasets;
  datasets.emplace("e0", make_edge_dataset(csv0));
  datasets.emplace("e1", make_edge_dataset(csv1));

  dl::graph_anchor_edge_dataset_options_t<Kline> source_options{};
  source_options.require_normalized = false;
  source_options.validate_anchor_fetch = true;
  auto market = make_market_graph();
  dl::graph_anchor_edge_dataset_t<Kline> source(market, std::move(datasets),
                                                source_options);
  check(source.size() == 4, "node-lifted stream source anchor count");

  stream::node_lifted_stream_options_t<Kline> options{};
  options.batch_size = 3;
  options.runtime_report_mode = runtime_lls::runtime_report_mode_t::debug;
  stream::node_lifted_stream_t<Kline> node_lifted_stream(
      std::move(source), make_srl_graph_from_market(market), options);

  check(node_lifted_stream.has_next(), "node-lifted stream has first batch");
  auto first = node_lifted_stream.next();
  check(first.node_features.size(0) == 3, "first batch size");
  check(first.future_edge_features.defined(), "future edge tensors carried");
  check(first.future_node_features.defined(), "future NodeLift sidecars built");
  check(first.graph_order_fingerprint ==
            market.computed_graph_order_fingerprint(),
        "node-lifted stream graph fingerprint carried");
  check(!torch::isnan(first.node_features).any().item<bool>(),
        "first node-lifted batch no NaNs");
  check(first.cursor.anchor_count() == 3, "first node-lifted cursor carried");
  check(first.runtime_lls.find(
            "schema:str = wikimyei.expression.nodelift.srl.runtime.v1") !=
            std::string::npos,
        "NodeLift runtime LLS emitted in debug mode");
  check(first.runtime_lls.find("batch_cursor_token") != std::string::npos,
        "NodeLift runtime LLS carries batch cursor token");
  check(first.runtime_lls.find("source_cursor_token") == std::string::npos,
        "NodeLift runtime LLS does not mislabel batch cursor as source cursor");
  check(first.runtime_lls.find("component_id:str = nodelift_srl_v1") !=
            std::string::npos,
        "NodeLift runtime LLS carries component id");
  MeanNodeEncoder encoder{};
  auto rep_debug =
      repstream::make_node_representation_batch<MeanNodeEncoder,
                                                typename Kline::key_type_t>(
          encoder, first,
          cuwacunu::wikimyei::representation::encoding::vicreg::
              node_vicreg_encoding_adapter_options(),
          /*use_swa=*/true, /*detach_to_cpu=*/false,
          runtime_lls::runtime_report_mode_t::debug);
  check(rep_debug.runtime_lls.find(
            "schema:str = wikimyei.representation.vicreg.runtime.v1") !=
            std::string::npos,
        "representation runtime LLS emitted in debug mode");
  check(rep_debug.runtime_lls.find("component_id:str = node_vicreg_v1") !=
            std::string::npos,
        "representation runtime LLS carries component id");
  check(rep_debug.nodelift_runtime_lls == first.runtime_lls,
        "representation batch carries NodeLift runtime LLS");

  check(node_lifted_stream.has_next(),
        "node-lifted stream has final short batch");
  auto second = node_lifted_stream.next();
  check(second.node_features.size(0) == 1, "final short batch size");
  check(!node_lifted_stream.has_next(), "node-lifted stream exhausted");

  dl::graph_anchor_edge_dataset_t<Kline>::edge_dataset_map_t range_datasets;
  range_datasets.emplace("e0", make_edge_dataset(csv0));
  range_datasets.emplace("e1", make_edge_dataset(csv1));
  dl::graph_anchor_edge_dataset_t<Kline> range_source(
      market, std::move(range_datasets), source_options);
  stream::node_lifted_stream_options_t<Kline> range_options{};
  range_options.batch_size = 1;
  range_options.begin_anchor_index = 1;
  range_options.end_anchor_index = 3;
  stream::node_lifted_stream_t<Kline> range_stream(
      std::move(range_source), make_srl_graph_from_market(market),
      range_options);
  check(range_stream.begin_anchor_index() == 1,
        "node-lifted stream range begin");
  check(range_stream.end_anchor_index() == 3, "node-lifted stream range end");
  auto range_first = range_stream.next();
  check(range_first.cursor.begin_anchor_index == 1,
        "range stream first batch cursor begin");
  check(range_first.cursor.end_anchor_index == 2,
        "range stream first batch cursor end");
  auto range_second = range_stream.next();
  check(range_second.cursor.begin_anchor_index == 2,
        "range stream second batch cursor begin");
  check(range_second.cursor.end_anchor_index == 3,
        "range stream second batch cursor end");
  check(!range_stream.has_next(), "range stream exhausted at requested end");

  dl::graph_anchor_edge_dataset_t<Kline>::edge_dataset_map_t datasets_no_diag;
  datasets_no_diag.emplace("e0", make_edge_dataset(csv0));
  datasets_no_diag.emplace("e1", make_edge_dataset(csv1));
  dl::graph_anchor_edge_dataset_t<Kline> source_no_diag(
      market, std::move(datasets_no_diag), source_options);
  stream::node_lifted_stream_options_t<Kline> no_diag_options{};
  no_diag_options.batch_size = 2;
  no_diag_options.compute_alignment_diagnostics = false;
  stream::node_lifted_stream_t<Kline> no_diag_loader(
      std::move(source_no_diag), make_srl_graph_from_market(market),
      no_diag_options);
  auto no_diag_batch = no_diag_loader.next();
  check(!no_diag_batch.alignment_diagnostics.max_past_lag_by_anchor.defined(),
        "node-lifted stream alignment diagnostics can be disabled");
}

void test_source_spec_to_nodelift_contract_smoke() {
  const auto dir = make_tmp_dir("contract_smoke");
  const auto btc_csv = dir / "BTCUSDT.csv";
  const auto eth_csv = dir / "ETHUSDT.csv";
  write_kline_csv(btc_csv, {1000, 1001, 1002, 1003, 1004, 1005}, 100.0);
  write_kline_csv(eth_csv, {1000, 1001, 1002, 1003, 1004, 1005}, 200.0);

  const auto btc_usdt = make_signature("BTCUSDT", "BTC", "USDT");
  const auto eth_usdt = make_signature("ETHUSDT", "ETH", "USDT");
  auto market = make_contract_market_graph(btc_usdt, eth_usdt);
  auto spec = make_source_spec(btc_usdt, eth_usdt, btc_csv, eth_csv);

  dl::graph_anchor_edge_dataset_t<Kline>::instrument_map_t instruments;
  instruments.emplace("BTCUSDT", btc_usdt);
  instruments.emplace("ETHUSDT", eth_usdt);

  dl::graph_anchor_edge_dataset_options_t<Kline> source_options{};
  source_options.force_rebuild_cache = true;
  source_options.require_normalized = true;

  dl::graph_anchor_edge_dataset_t<Kline> source_graph(
      market, std::move(instruments), std::move(spec), source_options);
  check(source_graph.size() >= 2, "source-spec graph source has anchors");

  auto graph_batch = source_graph.get_graph_batch(0, 2);
  check(graph_batch.graph_order_fingerprint ==
            market.computed_graph_order_fingerprint(),
        "source-spec graph batch fingerprint");
  check(graph_batch.edge_features.sizes() ==
            torch::IntArrayRef({2, 2, 1, 2, types::kKlineFeatureWidth}),
        "source-spec graph batch feature shape");
  check(graph_batch.future_features.sizes() ==
            torch::IntArrayRef({2, 2, 1, 1, types::kKlineFeatureWidth}),
        "source-spec graph batch future shape");

  auto lifted =
      stream::node_lift_graph_anchor_edge_batch<typename Kline::key_type_t>(
          graph_batch, make_srl_graph_from_market(market));
  check(lifted.node_features.sizes() ==
            torch::IntArrayRef({2, 1, 2, 3, types::kKlineFeatureWidth}),
        "source-spec lifted node feature shape");
  check(lifted.price_residual.sizes() == torch::IntArrayRef({2, 1, 2, 2, 4}),
        "source-spec lifted residual shape");
  check(lifted.future_node_features.sizes() ==
            torch::IntArrayRef({2, 1, 1, 3, types::kKlineFeatureWidth}),
        "source-spec future node feature shape");
  check(lifted.future_price_residual.sizes() ==
            torch::IntArrayRef({2, 1, 1, 2, 4}),
        "source-spec future residual shape");
  check(lifted.graph_order_fingerprint ==
            market.computed_graph_order_fingerprint(),
        "source-spec lifted fingerprint");
  check(lifted.anchor_keys.size(0) == 2, "source-spec anchors carried");
  check(!torch::isnan(lifted.node_features).any().item<bool>(),
        "source-spec lifted node features no NaNs");
  check(lifted.alignment_diagnostics.max_past_lag_by_anchor.defined(),
        "source-spec alignment diagnostics attached");
}

} // namespace

int main() {
  test_bridge_batch();
  test_future_nodelift_is_target_side_only();
  test_future_mask_and_residual_reconstruction();
  test_node_lifted_stream();
  test_source_spec_to_nodelift_contract_smoke();
  std::cout << "[NodeLiftedStream test] all checks passed\n";
  return 0;
}
