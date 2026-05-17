#include "wikimyei/representation/encoding/vicreg/stream/node_representation_stream.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_rank4_encoder.h"

#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

namespace contract = cuwacunu::ujcamei::source::contract;
namespace dl = cuwacunu::ujcamei::source::dataloader;
namespace graph = cuwacunu::ujcamei::graph;
namespace nodelift = cuwacunu::wikimyei::expression::nodelift::srl::stream;
namespace repstream =
    cuwacunu::wikimyei::representation::encoding::vicreg::stream;
namespace source = cuwacunu::ujcamei::source;
namespace srl = cuwacunu::wikimyei::expression::nodelift::srl;
namespace types = cuwacunu::ujcamei::source::types;
namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;

namespace {

using Kline = types::kline_t;

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void close(double actual, double expected, double tol,
           const std::string &message) {
  if (std::abs(actual - expected) > tol) {
    throw std::runtime_error(message + " expected=" + std::to_string(expected) +
                             " actual=" + std::to_string(actual));
  }
}

bool throws(const std::function<void()> &fn) {
  try {
    fn();
  } catch (const std::exception &) {
    return true;
  }
  return false;
}

double feature_value(int64_t b, int64_t c, int64_t h, int64_t n, int64_t d) {
  return static_cast<double>(1000 * b + 100 * n + 10 * c + h) +
         static_cast<double>(d) * 0.01;
}

nodelift::node_lifted_batch_t<int64_t>
make_lifted_batch(int64_t B = 2, int64_t C = 2, int64_t H = 3, int64_t N = 2,
                  int64_t L = 3) {
  nodelift::node_lifted_batch_t<int64_t> out{};
  out.node_features = torch::empty({B, C, H, N, 9}, torch::kFloat32);
  auto acc = out.node_features.accessor<float, 5>();
  for (int64_t b = 0; b < B; ++b) {
    for (int64_t c = 0; c < C; ++c) {
      for (int64_t h = 0; h < H; ++h) {
        for (int64_t n = 0; n < N; ++n) {
          for (int64_t d = 0; d < 9; ++d) {
            acc[b][c][h][n][d] =
                static_cast<float>(feature_value(b, c, h, n, d));
          }
        }
      }
    }
  }
  out.node_mask = torch::ones({B, C, H, N, 9}, torch::kBool);
  out.anchor_keys = torch::tensor({100, 200}, torch::kInt64);
  out.node_ids = {"n0", "n1"};
  out.edge_ids = {"e0", "e1", "e2"};
  out.graph_order_fingerprint = "graph-fingerprint";
  out.price_residual = torch::arange(B * C * H * L * 4, torch::kFloat32)
                           .reshape({B, C, H, L, 4});
  out.price_residual_mask = torch::ones({B, C, H, L, 4}, torch::kBool);
  out.future_edge_features = torch::full({B, L, C, 2, 9}, 7.0, torch::kFloat32);
  out.future_edge_mask = torch::ones({B, L, C, 2}, torch::kBool);
  return out;
}

struct fake_forward_encoder_t {
  torch::Dtype dtype{torch::kFloat32};
  torch::Device device{torch::kCPU};
  torch::Dtype saw_data_dtype{torch::kFloat32};
  torch::Device saw_data_device{torch::kCPU};

  torch::Tensor forward(const torch::Tensor &data, const torch::Tensor &mask) {
    saw_data_dtype = data.scalar_type();
    saw_data_device = data.device();
    auto masked = data.masked_fill(mask.logical_not().unsqueeze(-1), 0.0);
    auto value_sum = masked.sum(std::vector<int64_t>{1, 3}); // [M,H]
    auto valid_count = mask.to(data.scalar_type()).sum(1);   // [M,H]
    return torch::stack({value_sum, valid_count, value_sum * 0.5}, -1);
  }
};

struct fake_encode_model_t {
  bool saw_use_swa{false};
  bool saw_detach_to_cpu{false};

  torch::Tensor encode(const torch::Tensor &data, const torch::Tensor &mask,
                       bool use_swa, bool detach_to_cpu) {
    saw_use_swa = use_swa;
    saw_detach_to_cpu = detach_to_cpu;
    auto masked = data.masked_fill(mask.logical_not().unsqueeze(-1), 0.0);
    auto value_sum = masked.sum(std::vector<int64_t>{1, 2, 3}); // [M]
    auto out = torch::stack({value_sum, value_sum + 1.0}, -1);
    if (detach_to_cpu) {
      out = out.detach().to(torch::kCPU);
    }
    return out;
  }
};

void test_batch_helper_with_forward_encoder() {
  auto lifted = make_lifted_batch();
  lifted.node_mask.index_put_({0, torch::indexing::Slice(),
                               torch::indexing::Slice(), 1,
                               torch::indexing::Slice()},
                              false);

  fake_forward_encoder_t encoder{};
  auto batch = repstream::make_node_representation_batch(encoder, lifted);

  check(batch.node_encoding.sizes() == torch::IntArrayRef({2, 2, 3, 3}),
        "node representation sequence shape");
  check(batch.node_encoding_mask.sizes() == torch::IntArrayRef({2, 2}),
        "node representation mask shape");
  check(batch.node_encoding_mask.index({0, 0}).item<bool>(),
        "valid node window remains valid");
  check(!batch.node_encoding_mask.index({0, 1}).item<bool>(),
        "all-invalid node window is masked");
  check(batch.node_encoding_mask.index({1, 0}).item<bool>(),
        "later anchor node remains valid");

  close(batch.node_encoding.index({0, 1, 0, 0}).item<double>(), 0.0, 1e-8,
        "invalid node encoding is zero-filled by masked input");
  close(batch.node_encoding.index({0, 0, 1, 1}).item<double>(), 2.0, 1e-8,
        "valid-count feature is preserved");
  check(torch::equal(batch.anchor_keys, lifted.anchor_keys),
        "anchor keys carried");
  check(batch.node_ids == lifted.node_ids, "node ids carried");
  check(batch.edge_ids == lifted.edge_ids, "edge ids carried");
  check(batch.graph_order_fingerprint == lifted.graph_order_fingerprint,
        "graph fingerprint carried");
  check(torch::equal(batch.price_residual, lifted.price_residual),
        "price residual carried");
  check(torch::equal(batch.future_edge_features, lifted.future_edge_features),
        "future edge features carried");
  check(batch.adapter_diagnostics.original_rows == 4,
        "adapter diagnostics carried");
  check(batch.adapter_diagnostics.kept_rows == 4,
        "representation helper does not compact rows");
  check(torch::isfinite(batch.node_encoding).all().item<bool>(),
        "node representation contains no NaNs");
}

void test_encoder_input_dtype_device_mapping() {
  auto lifted = make_lifted_batch();
  fake_forward_encoder_t encoder{};
  encoder.dtype = torch::kFloat64;
  encoder.device = torch::Device(torch::kCPU);
  auto batch = repstream::make_node_representation_batch(encoder, lifted);

  check(encoder.saw_data_dtype == torch::kFloat64,
        "encoder input data moved to encoder dtype");
  check(encoder.saw_data_device.is_cpu(),
        "encoder input data moved to encoder device");
  check(batch.node_encoding.scalar_type() == torch::kFloat64,
        "node encoding preserves encoder dtype");
}

void test_batch_helper_with_encode_model() {
  auto lifted = make_lifted_batch();
  fake_encode_model_t model{};
  auto batch = repstream::make_node_representation_batch(
      model, lifted, vicreg::node_vicreg_encoding_adapter_options(),
      /*use_swa=*/false, /*detach_to_cpu=*/true);

  check(batch.node_encoding.sizes() == torch::IntArrayRef({2, 2, 2}),
        "rank-2 encode model reshapes to [B,N,D]");
  check(!model.saw_use_swa, "use_swa flag forwarded");
  check(model.saw_detach_to_cpu, "detach_to_cpu flag forwarded");
  check(batch.node_encoding.device().is_cpu(), "detached encoding is on CPU");
  check(batch.node_encoding_mask.all().item<bool>(),
        "all node windows valid by default");
}

void test_compacting_options_rejected() {
  auto lifted = make_lifted_batch();
  fake_forward_encoder_t encoder{};
  auto options = vicreg::node_vicreg_training_adapter_options();
  check(throws([&]() {
          auto batch = repstream::make_node_representation_batch(
              encoder, lifted, options);
          (void)batch;
        }),
        "node representation stream rejects compacting adapter options");
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  static int counter = 0;
  auto dir = std::filesystem::temp_directory_path() /
             ("cuwacunu_node_representation_stream_" + label + "_" +
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
make_source_spec(const source::instrument_signature_t &btc_usdt,
                 const source::instrument_signature_t &eth_usdt,
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

graph::market_graph_t
make_contract_market_graph(const source::instrument_signature_t &btc_usdt,
                           const source::instrument_signature_t &eth_usdt) {
  return graph::make_market_graph({
      graph::make_directed_instrument_edge(btc_usdt),
      graph::make_directed_instrument_edge(eth_usdt),
  });
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

void test_real_data_node_representation_stream_smoke() {
  const auto dir = make_tmp_dir("real_data_smoke");
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

  dl::graph_anchor_edge_dataset_t<Kline> graph_source(
      market, std::move(instruments), std::move(spec), source_options);
  check(graph_source.size() >= 2,
        "real-data representation smoke source has anchors");

  nodelift::node_lifted_stream_options_t<Kline> lift_options{};
  lift_options.batch_size = 2;
  nodelift::node_lifted_stream_t<Kline> lifted_stream(
      std::move(graph_source), make_srl_graph_from_market(market),
      lift_options);

  vicreg::VICReg_Rank4_Encoder encoder(
      /*C=*/1, /*T=*/2, /*D=*/types::kKlineFeatureWidth,
      /*encoding_dim=*/5, /*channel_expansion_dim=*/2,
      /*fused_feature_dim=*/2, /*hidden_dims=*/3, /*depth=*/1, torch::kFloat32,
      torch::kCPU);

  auto adapter_options = vicreg::node_vicreg_encoding_adapter_options();
  adapter_options.required_feature_coords =
      vicreg::node_vicreg_price_feature_coords();
  repstream::node_representation_stream_t<Kline, vicreg::VICReg_Rank4_Encoder>
      representation_stream(std::move(lifted_stream), encoder, adapter_options,
                            /*use_swa=*/false, /*detach_to_cpu=*/true);

  check(representation_stream.has_next(),
        "real-data representation stream has a batch");
  auto batch = representation_stream.next();
  check(batch.node_encoding.sizes() == torch::IntArrayRef({2, 3, 2, 5}),
        "real-data node encoding shape");
  check(batch.node_encoding_mask.sizes() == torch::IntArrayRef({2, 3}),
        "real-data node encoding mask shape");
  check(batch.anchor_keys.size(0) == 2, "real-data anchor keys carried");
  check(batch.node_ids == market.node_ids, "real-data node ids carried");
  check(batch.edge_ids == market.edge_ids, "real-data edge ids carried");
  check(batch.graph_order_fingerprint ==
            market.computed_graph_order_fingerprint(),
        "real-data graph fingerprint carried");
  check(batch.price_residual.sizes() == torch::IntArrayRef({2, 1, 2, 2, 4}),
        "real-data residual shape");
  check(batch.future_edge_features.sizes() ==
            torch::IntArrayRef({2, 2, 1, 1, types::kKlineFeatureWidth}),
        "real-data future edge features carried");
  check(batch.adapter_diagnostics.original_rows == 6,
        "real-data adapter original row count");
  check(batch.adapter_diagnostics.kept_rows == 6,
        "real-data representation keeps full node grid");
  check(batch.adapter_diagnostics.kept_valid_channel_time_count > 0,
        "real-data price-profile mask has valid channel-time support");
  check(torch::isfinite(batch.node_encoding).all().item<bool>(),
        "real-data node encoding finite");
  check(batch.node_encoding_mask.any().item<bool>(),
        "real-data node encoding has at least one valid node window");
}

} // namespace

int main() {
  try {
    test_batch_helper_with_forward_encoder();
    test_encoder_input_dtype_device_mapping();
    test_batch_helper_with_encode_model();
    test_compacting_options_rejected();
    test_real_data_node_representation_stream_smoke();
    std::cout << "[VICReg NodeRepresentationStream test] all checks passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "[VICReg NodeRepresentationStream test] failure: " << ex.what()
              << "\n";
    return 1;
  }
}
