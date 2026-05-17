// SPDX-License-Identifier: MIT
#include "ujcamei/source/dataloader/graph_anchor_edge_dataset.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include <unistd.h>

namespace dl = cuwacunu::ujcamei::source::dataloader;
namespace graph = cuwacunu::ujcamei::graph;
namespace mm = cuwacunu::ujcamei::source::storage::memory_mapped;
namespace types = cuwacunu::ujcamei::source::types;
namespace contract = cuwacunu::ujcamei::source::contract;
namespace source = cuwacunu::ujcamei::source;

namespace {

using Kline = types::kline_t;

static_assert(
    !std::is_copy_constructible_v<dl::graph_anchor_edge_dataset_t<Kline>>,
    "graph_anchor_edge_dataset_t owns edge datasets and must stay move-only");
static_assert(
    std::is_move_constructible_v<dl::graph_anchor_edge_dataset_t<Kline>>,
    "graph_anchor_edge_dataset_t should be movable");

void check(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "[GraphAnchorEdgeDataset test] " << message << "\n";
    std::exit(1);
  }
}

double scalar(const torch::Tensor &t) {
  return t.detach().to(torch::kCPU).to(torch::kFloat64).item<double>();
}

void close(double actual, double expected, double tol, const std::string &msg) {
  if (std::fabs(actual - expected) > tol) {
    std::cerr << "[GraphAnchorEdgeDataset test] " << msg << " actual=" << actual
              << " expected=" << expected << "\n";
    std::exit(1);
  }
}

template <typename Fn> void expect_throw(Fn &&fn, const std::string &msg) {
  try {
    fn();
  } catch (const c10::Error &) {
    return;
  } catch (const std::exception &) {
    return;
  }
  std::cerr << "[GraphAnchorEdgeDataset test] expected throw: " << msg << "\n";
  std::exit(1);
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  static int counter = 0;
  auto dir = std::filesystem::temp_directory_path() /
             ("cuwacunu_graph_anchor_edge_dataset_" + label + "_" +
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
  out.volume = 10.0;
  out.close_time = close_time;
  out.quote_asset_volume = 10.0 * price;
  out.number_of_trades = 3;
  out.taker_buy_base_volume = 4.0;
  out.taker_buy_quote_volume = 4.0 * price;
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

graph::market_graph_t make_graph() {
  graph::market_graph_t out{};
  out.node_ids = {"BTC", "USDT", "ETH"};
  out.edge_ids = {"e0", "e1"};
  out.base_index = {0, 2};
  out.quote_index = {1, 1};
  out.validate();
  return out;
}

source::instrument_signature_t
make_signature(std::string symbol, std::string base, std::string quote) {
  return source::instrument_signature_t{
      .symbol = std::move(symbol),
      .record_type = "kline",
      .market_type = "spot",
      .venue = "binance",
      .base_asset = std::move(base),
      .quote_asset = std::move(quote),
  };
}

contract::source_spec_t
make_source_spec(const std::vector<std::pair<source::instrument_signature_t,
                                             std::filesystem::path>> &sources,
                 std::string normalization_policy = "log_returns") {
  contract::source_spec_t spec{};
  spec.csv_bootstrap_deltas = 2;
  for (const auto &[signature, csv] : sources) {
    spec.source_forms.push_back(contract::source_form_t{
        .instrument = signature.symbol,
        .interval = types::interval_type_e::interval_1m,
        .record_type = signature.record_type,
        .market_type = signature.market_type,
        .venue = signature.venue,
        .base_asset = signature.base_asset,
        .quote_asset = signature.quote_asset,
        .source = csv.string(),
    });
  }
  spec.channel_forms = {
      contract::channel_form_t{
          .interval = types::interval_type_e::interval_1m,
          .active = "true",
          .record_type = "kline",
          .input_length = "2",
          .future_length = "1",
          .channel_weight = "1",
          .normalization_policy = std::move(normalization_policy),
      },
  };
  return spec;
}

graph::market_graph_t make_contract_graph() {
  const auto btc_usdt = make_signature("BTCUSDT", "BTC", "USDT");
  const auto eth_usdt = make_signature("ETHUSDT", "ETH", "USDT");
  return graph::make_market_graph({
      graph::make_directed_instrument_edge(btc_usdt),
      graph::make_directed_instrument_edge(eth_usdt),
  });
}

graph::market_graph_t make_reverse_graph() {
  const auto btc_usdt = make_signature("BTCUSDT", "BTC", "USDT");
  const auto usdt_btc = make_signature("USDTBTC", "USDT", "BTC");
  return graph::make_market_graph({
      graph::make_directed_instrument_edge(btc_usdt),
      graph::make_directed_instrument_edge(usdt_btc),
  });
}

mm::MemoryMappedEdgeDataset<Kline>
make_edge_dataset(const std::filesystem::path &csv) {
  mm::MemoryMappedEdgeDataset<Kline> dataset;
  dataset.add_dataset(csv.string(), 2, 1, "none", true);
  return dataset;
}

dl::graph_anchor_edge_dataset_options_t<Kline> options() {
  dl::graph_anchor_edge_dataset_options_t<Kline> opts{};
  opts.require_normalized = false;
  opts.validate_anchor_fetch = true;
  return opts;
}

dl::graph_anchor_edge_dataset_t<Kline>
make_source(const std::filesystem::path &csv0,
            const std::filesystem::path &csv1,
            dl::graph_anchor_edge_dataset_options_t<Kline> opts = options()) {
  dl::graph_anchor_edge_dataset_t<Kline>::edge_dataset_map_t datasets;
  datasets.emplace("e0", make_edge_dataset(csv0));
  datasets.emplace("e1", make_edge_dataset(csv1));
  return dl::graph_anchor_edge_dataset_t<Kline>(make_graph(),
                                                std::move(datasets), opts);
}

void test_reference_grid_common_coverage_and_fetch() {
  const auto dir = make_tmp_dir("coverage");
  const auto csv0 = dir / "e0.csv";
  const auto csv1 = dir / "e1.csv";
  write_kline_csv(csv0, {1000, 1001, 1002, 1003, 1004, 1005}, 10.0);
  write_kline_csv(csv1, {998, 1000, 1002, 1004, 1006, 1008}, 20.0);

  auto source = make_source(csv0, csv1);
  check(source.size() == 4, "anchor count from reference grid");
  close(static_cast<double>(source.anchor_keys().front()), 1001.0, 1e-6,
        "first common anchor");
  close(static_cast<double>(source.anchor_keys().back()), 1004.0, 1e-6,
        "last common anchor");
  const auto &cursor_report = source.cursor_report();
  check(cursor_report.anchor_count() == source.size(),
        "cursor report accepted anchor count");
  check(cursor_report.candidate_anchor_count >= cursor_report.anchor_count(),
        "cursor report candidate count covers accepted anchors");
  check(cursor_report.edge_ids.size() == 2, "cursor report edge count");
  check(cursor_report.first_anchor_key().has_value() &&
            *cursor_report.first_anchor_key() == 1001,
        "cursor report first anchor");
  check(cursor_report.last_anchor_key().has_value() &&
            *cursor_report.last_anchor_key() == 1004,
        "cursor report last anchor");
  check(cursor_report.cursor_token().find("accepted=4") != std::string::npos,
        "cursor report token includes accepted anchors");

  auto samples = source.get_edge_samples(0);
  check(samples.size() == 2, "one sample per staged edge");
  check(samples[0].edge_id == "e0", "stable edge order e0");
  check(samples[1].edge_id == "e1", "stable edge order e1");
  close(scalar(samples[0].sample.past_keys.index({0, 1})), 1001.0, 1e-6,
        "reference edge terminal key equals anchor");
  close(scalar(samples[1].sample.past_keys.index({0, 1})), 1000.0, 1e-6,
        "non-reference edge can use terminal key <= anchor");

  auto batch = source.get_graph_batch(0, 2);
  check(batch.edge_features.sizes() == torch::IntArrayRef({2, 2, 1, 2, 9}),
        "graph-anchor batch feature shape");
  check(batch.edge_mask.sizes() == torch::IntArrayRef({2, 2, 1, 2}),
        "graph-anchor batch mask shape");
  check(batch.edge_present.all().item<bool>(), "all staged edges present");
  close(scalar(batch.anchor_keys.index({0})), 1001.0, 1e-6, "batch anchor 0");
  close(scalar(batch.anchor_keys.index({1})), 1002.0, 1e-6, "batch anchor 1");
  check(batch.cursor.begin_anchor_index == 0, "batch cursor begin index");
  check(batch.cursor.end_anchor_index == 2, "batch cursor end index");
  check(batch.cursor.requested_batch_size == 2, "batch cursor requested size");
  check(batch.cursor.anchor_count() == 2, "batch cursor anchor count");
  check(batch.cursor.first_anchor_key().has_value() &&
            *batch.cursor.first_anchor_key() == 1001,
        "batch cursor first anchor");
  check(batch.cursor.last_anchor_key().has_value() &&
            *batch.cursor.last_anchor_key() == 1002,
        "batch cursor last anchor");
  check(batch.cursor.cursor_token().find("anchors=2") != std::string::npos,
        "batch cursor token includes anchor count");

  auto diagnostics = dl::compute_graph_anchor_edge_alignment_diagnostics(batch);
  close(scalar(diagnostics.max_past_lag_by_edge.index({0, 0})), 0.0, 1e-6,
        "reference edge lag");
  close(scalar(diagnostics.max_past_lag_by_edge.index({0, 1})), 1.0, 1e-6,
        "non-reference edge lag");
  close(scalar(diagnostics.max_future_gap_by_anchor.index({0})), 1.0, 1e-6,
        "future gap diagnostic");

  dl::graph_anchor_edge_dataset_options_t<Kline> parallel_opts = options();
  parallel_opts.fetch_mode = dl::fetch_mode_t::parallel_by_edge;
  parallel_opts.parallel_min_work_items = 1;
  auto parallel_source = make_source(csv0, csv1, parallel_opts);
  auto parallel_batch = parallel_source.get_graph_batch(0, 2);
  close(
      scalar((parallel_batch.edge_features - batch.edge_features).abs().sum()),
      0.0, 1e-8, "parallel features equal serial");
  close(scalar((parallel_batch.edge_mask.to(torch::kInt64) -
                batch.edge_mask.to(torch::kInt64))
                   .abs()
                   .sum()),
        0.0, 1e-8, "parallel masks equal serial");
  close(
      scalar(
          (parallel_batch.future_features - batch.future_features).abs().sum()),
      0.0, 1e-8, "parallel futures equal serial");
  check(parallel_batch.edge_ids == batch.edge_ids,
        "parallel preserves edge order");
}

void test_required_future_window_excludes_right_raw_boundary() {
  const auto dir = make_tmp_dir("future_boundary");
  const auto csv0 = dir / "e0.csv";
  const auto csv1 = dir / "e1.csv";
  write_kline_csv(csv0, {2000, 2001, 2002, 2003, 2004}, 10.0);
  write_kline_csv(csv1, {2000, 2001, 2002, 2003, 2004}, 20.0);

  auto source = make_source(csv0, csv1);
  check(source.size() == 3, "future boundary anchor count");
  close(static_cast<double>(source.anchor_keys().front()), 2001.0, 1e-6,
        "future boundary first anchor");
  close(static_cast<double>(source.anchor_keys().back()), 2003.0, 1e-6,
        "future boundary excludes final raw key");
}

void test_explicit_reference_edge() {
  const auto dir = make_tmp_dir("reference");
  const auto csv0 = dir / "e0.csv";
  const auto csv1 = dir / "e1.csv";
  write_kline_csv(csv0, {1000, 1001, 1002, 1003, 1004, 1005}, 10.0);
  write_kline_csv(csv1, {998, 1000, 1002, 1004, 1006, 1008}, 20.0);

  auto opts = options();
  opts.reference_edge_id = "e1";
  auto source = make_source(csv0, csv1, opts);
  check(source.size() == 2, "explicit reference grid anchor count");
  close(static_cast<double>(source.anchor_keys()[0]), 1002.0, 1e-6,
        "explicit reference anchor 0");
  close(static_cast<double>(source.anchor_keys()[1]), 1004.0, 1e-6,
        "explicit reference anchor 1");
}

void test_validation_failures() {
  const auto dir = make_tmp_dir("validation");
  const auto csv0 = dir / "e0.csv";
  write_kline_csv(csv0, {1000, 1001, 1002, 1003}, 10.0);

  expect_throw(
      [&] {
        dl::graph_anchor_edge_dataset_t<Kline>::edge_dataset_map_t datasets;
        datasets.emplace("e0", make_edge_dataset(csv0));
        auto source = dl::graph_anchor_edge_dataset_t<Kline>(
            make_graph(), std::move(datasets), options());
        (void)source;
      },
      "missing staged edge dataset");

  expect_throw(
      [&] {
        auto opts = options();
        opts.reference_edge_id = "missing";
        dl::graph_anchor_edge_dataset_t<Kline>::edge_dataset_map_t datasets;
        datasets.emplace("e0", make_edge_dataset(csv0));
        datasets.emplace("e1", make_edge_dataset(csv0));
        auto source = dl::graph_anchor_edge_dataset_t<Kline>(
            make_graph(), std::move(datasets), opts);
        (void)source;
      },
      "unknown reference edge");

  expect_throw(
      [&] {
        auto opts = options();
        opts.reference_edge_id = "e1";
        dl::graph_anchor_edge_dataset_t<Kline>::edge_dataset_map_t datasets;
        datasets.emplace("e0", make_edge_dataset(csv0));
        auto source = dl::graph_anchor_edge_dataset_t<Kline>(
            make_graph(), std::move(datasets), opts);
        (void)source;
      },
      "reference edge dataset missing");
}

void test_source_contract_validation() {
  const auto dir = make_tmp_dir("source_contract");
  const auto btc_csv = dir / "BTCUSDT.csv";
  const auto eth_csv = dir / "ETHUSDT.csv";
  write_kline_csv(btc_csv, {3000, 3001, 3002, 3003, 3004, 3005}, 100.0);
  write_kline_csv(eth_csv, {3000, 3001, 3002, 3003, 3004, 3005}, 200.0);

  const auto btc_usdt = make_signature("BTCUSDT", "BTC", "USDT");
  const auto eth_usdt = make_signature("ETHUSDT", "ETH", "USDT");

  dl::graph_anchor_edge_dataset_t<Kline>::instrument_map_t instruments;
  instruments.emplace("BTCUSDT", btc_usdt);
  instruments.emplace("ETHUSDT", eth_usdt);

  dl::graph_anchor_edge_dataset_options_t<Kline> opts{};
  opts.force_rebuild_cache = true;
  auto source = dl::graph_anchor_edge_dataset_t<Kline>(
      make_contract_graph(), instruments,
      make_source_spec({{btc_usdt, btc_csv}, {eth_usdt, eth_csv}}), opts);
  check(source.validation_report().ok(), "strict source validation passes");
  check(source.validation_report().node_count == 3, "validation node count");
  check(source.validation_report().edge_count == 2, "validation edge count");
  check(!source.validation_identity().ujcamei_source_universe_hash.empty(),
        "Ujcamei source universe hash stored");
  check(!source.validation_identity().graph_first_channel_hash.empty(),
        "graph-first channel hash stored");
  check(!source.validation_identity().graph_first_graph_hash.empty(),
        "graph-first graph hash stored");

  expect_throw(
      [&] {
        auto bad_opts = opts;
        auto bad = dl::graph_anchor_edge_dataset_t<Kline>(
            make_contract_graph(), instruments,
            make_source_spec({{btc_usdt, btc_csv}, {eth_usdt, eth_csv}},
                             "none"),
            bad_opts);
        (void)bad;
      },
      "normalization policy mismatch");

  {
    auto test_opts = opts;
    test_opts.require_normalized = false;
    test_opts.validation_options.source_validation_mode =
        dl::source_validation_mode_t::none_for_tests;
    auto relaxed = dl::graph_anchor_edge_dataset_t<Kline>(
        make_contract_graph(), instruments,
        make_source_spec({{btc_usdt, btc_csv}, {eth_usdt, eth_csv}}, "none"),
        test_opts);
    check(relaxed.size() >= 2, "test validation mode allows raw sources");
  }

  expect_throw(
      [&] {
        graph::market_graph_t mismatched{};
        mismatched.node_ids = {"BTC", "USDT", "ETH"};
        mismatched.edge_ids = {"BTCUSDT", "ETHUSDT"};
        mismatched.base_index = {0, 1};
        mismatched.quote_index = {1, 2};
        mismatched.validate();
        auto bad = dl::graph_anchor_edge_dataset_t<Kline>(
            mismatched, instruments,
            make_source_spec({{btc_usdt, btc_csv}, {eth_usdt, eth_csv}}), opts);
        (void)bad;
      },
      "graph source base quote mismatch");
}

void test_source_runtime_cursor_token() {
  const auto signature = make_signature("BTCUSDT", "BTC", "USDT");
  const auto cursor = dl::make_source_runtime_cursor(
      signature, "2020-01-01T00:00:00Z", "2020-02-01T00:00:00Z");
  check(!cursor.empty(), "source runtime cursor token is populated");
  check(cursor.cursor_token.find("BTCUSDT|kline|spot|binance|BTC|USDT|") == 0,
        "source runtime cursor token carries source signature");
}

void test_reverse_edge_policy() {
  const auto dir = make_tmp_dir("reverse");
  const auto btc_csv = dir / "BTCUSDT.csv";
  const auto usdt_csv = dir / "USDTBTC.csv";
  write_kline_csv(btc_csv, {4000, 4001, 4002, 4003, 4004, 4005}, 100.0);
  write_kline_csv(usdt_csv, {4000, 4001, 4002, 4003, 4004, 4005}, 0.01);

  const auto btc_usdt = make_signature("BTCUSDT", "BTC", "USDT");
  const auto usdt_btc = make_signature("USDTBTC", "USDT", "BTC");

  dl::graph_anchor_edge_dataset_t<Kline>::instrument_map_t instruments;
  instruments.emplace("BTCUSDT", btc_usdt);
  instruments.emplace("USDTBTC", usdt_btc);

  dl::graph_anchor_edge_dataset_options_t<Kline> opts{};
  opts.force_rebuild_cache = true;
  auto proven = dl::graph_anchor_edge_dataset_t<Kline>(
      make_reverse_graph(), instruments,
      make_source_spec({{btc_usdt, btc_csv}, {usdt_btc, usdt_csv}}), opts);
  check(proven.validation_report().ok(), "proven reverse pair allowed");
  check(proven.validation_report().has_code(
            dl::validation_code_t::reverse_pair_real),
        "proven reverse pair warning reported");

  expect_throw(
      [&] {
        dl::graph_anchor_edge_dataset_t<Kline>::edge_dataset_map_t datasets;
        datasets.emplace("BTCUSDT", make_edge_dataset(btc_csv));
        datasets.emplace("USDTBTC", make_edge_dataset(usdt_csv));
        auto bad = dl::graph_anchor_edge_dataset_t<Kline>(
            make_reverse_graph(), std::move(datasets), options());
        (void)bad;
      },
      "unproven reverse pair fails by default");

  {
    dl::graph_anchor_edge_dataset_t<Kline>::edge_dataset_map_t datasets;
    datasets.emplace("BTCUSDT", make_edge_dataset(btc_csv));
    datasets.emplace("USDTBTC", make_edge_dataset(usdt_csv));
    auto relaxed_opts = options();
    relaxed_opts.validation_options.reverse_edge_policy =
        dl::reverse_edge_policy_t::allow_without_proof_for_tests;
    auto relaxed = dl::graph_anchor_edge_dataset_t<Kline>(
        make_reverse_graph(), std::move(datasets), relaxed_opts);
    check(relaxed.validation_report().has_code(
              dl::validation_code_t::reverse_pair_unproven),
          "test policy records unproven reverse warning");
  }
}

} // namespace

int main() {
  test_reference_grid_common_coverage_and_fetch();
  test_required_future_window_excludes_right_raw_boundary();
  test_explicit_reference_edge();
  test_validation_failures();
  test_source_runtime_cursor_token();
  test_source_contract_validation();
  test_reverse_edge_policy();
  std::cout << "[GraphAnchorEdgeDataset test] all checks passed\n";
  return 0;
}
