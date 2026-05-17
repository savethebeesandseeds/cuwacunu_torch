#include "wikimyei/representation/encoding/vicreg/node_stream_adapter.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_rank4_encoder.h"

#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <string>

namespace stream = cuwacunu::wikimyei::expression::nodelift::srl::stream;
namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;

namespace {

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

stream::node_lifted_batch_t<int64_t> make_batch(int64_t B = 2, int64_t C = 2,
                                                int64_t H = 3, int64_t N = 2) {
  stream::node_lifted_batch_t<int64_t> out{};
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
  out.node_ids = {"n0", "n1"};
  out.anchor_keys = torch::tensor({100, 200}, torch::kInt64);
  return out;
}

void test_flatten_order() {
  auto batch = make_batch();
  const auto input = vicreg::make_node_encoder_input(batch);

  check(input.data.sizes() == torch::IntArrayRef({4, 2, 3, 9}),
        "flattened node data shape");
  check(input.mask.sizes() == torch::IntArrayRef({4, 2, 3}),
        "flattened node mask shape");
  check(input.B_anchor == 2, "B_anchor preserved");
  check(input.N == 2, "N preserved");
  check(input.diagnostics.original_rows == 4, "diagnostics original rows");
  check(input.diagnostics.kept_rows == 4, "diagnostics kept rows");
  check(input.diagnostics.dropped_rows == 0, "diagnostics dropped rows");
  check(input.diagnostics.valid_channel_time_count == 24,
        "diagnostics valid channel-time count");
  close(input.diagnostics.valid_channel_time_fraction, 1.0, 1e-12,
        "diagnostics valid channel-time fraction");
  check(input.diagnostics.kept_valid_channel_time_count == 24,
        "diagnostics kept valid channel-time count");
  close(input.diagnostics.kept_valid_channel_time_fraction, 1.0, 1e-12,
        "diagnostics kept valid channel-time fraction");
  check(input.node_ids == batch.node_ids, "node ids carried");
  check(torch::equal(input.anchor_keys, batch.anchor_keys),
        "anchor keys carried");

  close(input.data.index({1, 1, 2, 3}).item<double>(),
        feature_value(0, 1, 2, 1, 3), 1e-4, "node-major row order");
  close(input.data.index({2, 0, 1, 4}).item<double>(),
        feature_value(1, 0, 1, 0, 4), 1e-4, "anchor-major row order");

  check(input.anchor_index.index({0}).item<int64_t>() == 0,
        "anchor index row 0");
  check(input.anchor_index.index({1}).item<int64_t>() == 0,
        "anchor index row 1");
  check(input.anchor_index.index({2}).item<int64_t>() == 1,
        "anchor index row 2");
  check(input.node_index.index({0}).item<int64_t>() == 0, "node index row 0");
  check(input.node_index.index({1}).item<int64_t>() == 1, "node index row 1");
  check(input.node_index.index({2}).item<int64_t>() == 0, "node index row 2");
}

void test_mask_policies() {
  using torch::indexing::Slice;
  auto batch = make_batch();
  batch.node_mask.index_put_({0, 0, 1, 0, 3}, false);

  vicreg::node_vicreg_adapter_options_t required_all{};
  auto required_input = vicreg::make_node_encoder_input(batch, required_all);
  check(!required_input.mask.index({0, 0, 1}).item<bool>(),
        "required all features marks slot invalid");

  vicreg::node_vicreg_adapter_options_t any_feature{};
  any_feature.mask_policy = vicreg::node_mask_reduce_policy_t::any_feature;
  auto any_input = vicreg::make_node_encoder_input(batch, any_feature);
  check(any_input.mask.index({0, 0, 1}).item<bool>(),
        "any feature keeps slot valid");

  vicreg::node_vicreg_adapter_options_t only_price_open_high{};
  only_price_open_high.required_feature_coords = {0, 1};
  auto price_input =
      vicreg::make_node_encoder_input(batch, only_price_open_high);
  check(price_input.mask.index({0, 0, 1}).item<bool>(),
        "required feature subset ignores other missing coords");

  vicreg::node_vicreg_adapter_options_t coord_three{};
  coord_three.required_feature_coords = {3};
  auto coord_three_input = vicreg::make_node_encoder_input(batch, coord_three);
  check(!coord_three_input.mask.index({0, 0, 1}).item<bool>(),
        "required feature subset catches selected missing coord");

  batch.node_mask.index_put_({0, Slice(), Slice(), 1, Slice()}, false);
  vicreg::node_vicreg_adapter_options_t compact{};
  compact.compact_valid_nodes_for_training = true;
  auto compacted = vicreg::make_node_encoder_input(batch, compact);
  check(compacted.data.size(0) == 3, "compaction removes all-invalid node");
  check(compacted.anchor_index.index({0}).item<int64_t>() == 0,
        "compacted anchor row 0");
  check(compacted.node_index.index({0}).item<int64_t>() == 0,
        "compacted node row 0");
  check(compacted.anchor_index.index({1}).item<int64_t>() == 1,
        "compacted anchor row 1");
  check(compacted.node_index.index({1}).item<int64_t>() == 0,
        "compacted node row 1");
  check(compacted.diagnostics.original_rows == 4,
        "compacted diagnostics original rows");
  check(compacted.diagnostics.kept_rows == 3,
        "compacted diagnostics kept rows");
  check(compacted.diagnostics.dropped_rows == 1,
        "compacted diagnostics dropped rows");
  check(compacted.diagnostics.valid_channel_time_count == 17,
        "compacted diagnostics original valid count");
  check(compacted.diagnostics.kept_valid_channel_time_count == 17,
        "compacted diagnostics kept valid count");
  close(compacted.diagnostics.kept_valid_channel_time_fraction, 17.0 / 18.0,
        1e-12, "compacted diagnostics kept valid fraction");
}

void test_default_options_and_profiles() {
  auto encoding = vicreg::node_vicreg_encoding_adapter_options();
  auto training = vicreg::node_vicreg_training_adapter_options();
  check(!encoding.compact_valid_nodes_for_training,
        "encoding defaults preserve full node grid");
  check(training.compact_valid_nodes_for_training,
        "training defaults compact valid nodes");
  check(vicreg::node_vicreg_all_9_feature_coords().size() == 9,
        "all-9 feature profile");
  check(vicreg::node_vicreg_price_feature_coords() ==
            std::vector<int64_t>({0, 1, 2, 3}),
        "price feature profile");
  check(vicreg::node_vicreg_close_feature_coords() == std::vector<int64_t>({3}),
        "close feature profile");
  check(vicreg::node_vicreg_activity_feature_coords() ==
            std::vector<int64_t>({4, 5, 6, 7, 8}),
        "activity feature profile");
}

void test_nonfinite_handling() {
  auto batch = make_batch();
  batch.node_features.index_put_({0, 0, 0, 0, 0},
                                 std::numeric_limits<float>::quiet_NaN());

  check(throws([&]() { (void)vicreg::make_node_encoder_input(batch); }),
        "valid NaN should throw by default");

  batch.node_mask.index_put_({0, 0, 0, 0, 0}, false);
  auto input = vicreg::make_node_encoder_input(batch);
  check(!input.mask.index({0, 0, 0}).item<bool>(),
        "masked NaN reduces slot mask to false");
  close(input.data.index({0, 0, 0, 0}).item<double>(), 0.0, 1e-8,
        "masked invalid slot is zero-filled");

  auto non_strict_batch = make_batch();
  non_strict_batch.node_features.index_put_(
      {0, 0, 0, 0, 0}, std::numeric_limits<float>::infinity());
  vicreg::node_vicreg_adapter_options_t non_strict{};
  non_strict.require_finite_features = false;
  auto non_strict_input =
      vicreg::make_node_encoder_input(non_strict_batch, non_strict);
  check(!non_strict_input.mask.index({0, 0, 0}).item<bool>(),
        "non-strict non-finite value is still masked out");
  close(non_strict_input.data.index({0, 0, 0, 0}).item<double>(), 0.0, 1e-8,
        "non-strict non-finite value is zero-filled");
}

void test_reshape_node_encoding() {
  auto input = vicreg::make_node_encoder_input(make_batch());
  auto node_window_mask = vicreg::reshape_node_window_mask(input);
  check(node_window_mask.sizes() == torch::IntArrayRef({2, 2}),
        "node window mask shape");
  check(node_window_mask.all().item<bool>(), "node window mask all valid");

  auto encoded = torch::arange(20, torch::kFloat32).reshape({4, 5});
  auto reshaped = vicreg::reshape_node_encoding(encoded, input);
  check(reshaped.sizes() == torch::IntArrayRef({2, 2, 5}),
        "rank-2 encoding reshape shape");
  close(reshaped.index({1, 0, 2}).item<double>(),
        encoded.index({2, 2}).item<double>(), 1e-8,
        "rank-2 encoding reshape value");

  auto encoded_seq =
      torch::arange(4 * 3 * 5, torch::kFloat32).reshape({4, 3, 5});
  auto reshaped_seq = vicreg::reshape_node_encoding(encoded_seq, input);
  check(reshaped_seq.sizes() == torch::IntArrayRef({2, 2, 3, 5}),
        "rank-3 encoding reshape shape");
  close(reshaped_seq.index({1, 1, 2, 4}).item<double>(),
        encoded_seq.index({3, 2, 4}).item<double>(), 1e-8,
        "rank-3 encoding reshape value");
}

struct fake_train_result_t {
  int64_t rows{0};
  int64_t valid_channel_time_count{0};
  int swa_start_iter{0};
  bool verbose{false};
};

struct fake_train_model_t {
  fake_train_result_t train_one_batch(const torch::Tensor &data,
                                      const torch::Tensor &mask,
                                      int swa_start_iter, bool verbose) {
    return fake_train_result_t{
        .rows = data.size(0),
        .valid_channel_time_count = mask.sum().item<int64_t>(),
        .swa_start_iter = swa_start_iter,
        .verbose = verbose,
    };
  }
};

void test_training_helper_defaults() {
  using torch::indexing::Slice;
  auto batch = make_batch();
  batch.node_mask.index_put_({0, Slice(), Slice(), 1, Slice()}, false);

  fake_train_model_t model{};
  auto default_result =
      vicreg::train_vicreg_on_node_lifted_batch(model, batch, 11, true);
  check(default_result.rows == 3,
        "training helper defaults to compact valid node windows");
  check(default_result.swa_start_iter == 11,
        "training helper forwards swa_start_iter");
  check(default_result.verbose, "training helper forwards verbose");

  auto encoding_options = vicreg::node_vicreg_encoding_adapter_options();
  auto explicit_result = vicreg::train_vicreg_on_node_lifted_batch(
      model, batch, encoding_options, 12, false);
  check(explicit_result.rows == 4,
        "explicit adapter options still control training helper");
  check(explicit_result.swa_start_iter == 12,
        "explicit helper forwards swa_start_iter");
  check(!explicit_result.verbose, "explicit helper forwards verbose");
}

void test_encoder_bridge() {
  torch::manual_seed(7);
  auto batch = make_batch(/*B=*/2, /*C=*/2, /*H=*/4, /*N=*/2);
  auto input = vicreg::make_node_encoder_input(batch);

  vicreg::VICReg_Rank4_Encoder encoder(
      /*C=*/2, /*T=*/4, /*D=*/9, /*encoding_dim=*/6,
      /*channel_expansion_dim=*/2, /*fused_feature_dim=*/2,
      /*hidden_dims=*/3, /*depth=*/1, torch::kFloat32, torch::kCPU);
  auto encoded = encoder->forward(input.data, input.mask);
  check(encoded.sizes() == torch::IntArrayRef({4, 4, 6}),
        "encoder bridge output shape");
  check(torch::isfinite(encoded).all().item<bool>(),
        "encoder bridge output finite");

  auto reshaped = vicreg::reshape_node_encoding(encoded, input);
  check(reshaped.sizes() == torch::IntArrayRef({2, 2, 4, 6}),
        "encoder bridge reshaped output");
}

} // namespace

int main() {
  try {
    test_flatten_order();
    test_mask_policies();
    test_default_options_and_profiles();
    test_nonfinite_handling();
    test_reshape_node_encoding();
    test_training_helper_defaults();
    test_encoder_bridge();
    std::cout << "[VICReg NodeStreamAdapter test] all checks passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "[VICReg NodeStreamAdapter test] failure: " << ex.what()
              << "\n";
    return 1;
  }
}
