// SPDX-License-Identifier: MIT
#include "ujcamei/source/dataloader/graph_anchor_edge_batch.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace dl = cuwacunu::ujcamei::source::dataloader;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "[GraphAnchorEdgeBatch test] " << message << "\n";
    std::exit(1);
  }
}

double scalar(const torch::Tensor &t) {
  return t.detach().to(torch::kCPU).to(torch::kFloat64).item<double>();
}

bool bscalar(const torch::Tensor &t) {
  return t.detach().to(torch::kCPU).to(torch::kBool).item<bool>();
}

void close(double actual, double expected, double tol, const std::string &msg) {
  if (std::fabs(actual - expected) > tol) {
    std::cerr << "[GraphAnchorEdgeBatch test] " << msg << " actual=" << actual
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
  std::cerr << "[GraphAnchorEdgeBatch test] expected throw: " << msg << "\n";
  std::exit(1);
}

torch::Tensor make_keys(int64_t C, int64_t H, int64_t anchor, bool future) {
  auto keys = torch::empty({C, H}, torch::TensorOptions().dtype(torch::kInt64));
  for (int64_t c = 0; c < C; ++c) {
    for (int64_t h = 0; h < H; ++h) {
      const int64_t key = future ? anchor + h + 1 : anchor - (H - 1 - h);
      keys.index_put_({c, h}, key);
    }
  }
  return keys;
}

dl::edge_sample_t
make_sample(double value, int64_t anchor, bool normalized = true,
            bool include_future = true, bool include_keys = true, int64_t C = 2,
            int64_t Hx = 3, int64_t Hf = 2, int64_t D = 9,
            c10::ScalarType dtype = torch::kFloat32, bool mask_valid = true) {
  dl::edge_sample_t sample{};
  auto opts = torch::TensorOptions().dtype(dtype).device(torch::kCPU);
  sample.features = torch::full({C, Hx, D}, value, opts);
  sample.mask = mask_valid ? torch::ones({C, Hx}, torch::kBool)
                           : torch::zeros({C, Hx}, torch::kBool);
  if (include_future) {
    sample.future_features = torch::full({C, Hf, D}, value + 100.0, opts);
    sample.future_mask = torch::ones({C, Hf}, torch::kBool);
  }
  if (include_keys) {
    sample.past_keys = make_keys(C, Hx, anchor, /*future=*/false);
    if (include_future) {
      sample.future_keys = make_keys(C, Hf, anchor, /*future=*/true);
    }
  }
  sample.normalized = normalized;
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

dl::graph_anchor_edge_batch_options_t
options_for(std::vector<std::string> ids) {
  dl::graph_anchor_edge_batch_options_t options{};
  options.edge_ids = std::move(ids);
  return options;
}

void test_single_anchor_full_graph() {
  auto options = options_for({"e0", "e1"});
  std::vector<dl::graph_anchor_edge_sample_t<int64_t>> samples{
      wrap("e1", 100, make_sample(11.0, 100)),
      wrap("e0", 100, make_sample(7.0, 100)),
  };
  auto out = dl::make_graph_anchor_edge_batch(samples, options);
  check(out.edge_features.sizes() == torch::IntArrayRef({1, 2, 2, 3, 9}),
        "edge_features shape");
  check(out.edge_mask.sizes() == torch::IntArrayRef({1, 2, 2, 3}),
        "edge_mask shape");
  close(scalar(out.edge_features.index({0, 0, 0, 0, 0})), 7.0, 1e-6,
        "stable edge order e0");
  close(scalar(out.edge_features.index({0, 1, 0, 0, 0})), 11.0, 1e-6,
        "stable edge order e1");
  close(scalar(out.anchor_keys.index({0})), 100.0, 1e-6, "anchor key");
  check(out.edge_present.all().item<bool>(), "all edges present");
}

void test_missing_and_present_masked_edges() {
  auto options = options_for({"e0", "e1"});
  std::vector<dl::graph_anchor_edge_sample_t<int64_t>> missing{
      wrap("e1", 100, make_sample(11.0, 100)),
  };
  auto out_missing = dl::make_graph_anchor_edge_batch(missing, options);
  check(!bscalar(out_missing.edge_present.index({0, 0})),
        "missing edge_present false");
  check(bscalar(out_missing.edge_present.index({0, 1})),
        "present edge_present true");
  close(scalar(out_missing.edge_features
                   .index({0, 0, torch::indexing::Slice(),
                           torch::indexing::Slice(), torch::indexing::Slice()})
                   .abs()
                   .sum()),
        0.0, 1e-8, "missing edge zero-filled");
  check(!out_missing.edge_mask.index({0, 0}).any().item<bool>(),
        "missing edge mask false");

  std::vector<dl::graph_anchor_edge_sample_t<int64_t>> masked{
      wrap("e0", 100,
           make_sample(5.0, 100, true, true, true, 2, 3, 2, 9, torch::kFloat32,
                       false)),
  };
  auto out_masked = dl::make_graph_anchor_edge_batch(masked, options);
  check(bscalar(out_masked.edge_present.index({0, 0})),
        "all-masked sample is present");
  check(!out_masked.edge_mask.index({0, 0}).any().item<bool>(),
        "all-masked sample mask false");
  close(scalar(out_masked.edge_features.index({0, 0, 0, 0, 0})), 5.0, 1e-6,
        "all-masked sample features copied");
}

void test_anchor_order() {
  auto options = options_for({"e0"});
  std::vector<dl::graph_anchor_edge_sample_t<int64_t>> samples{
      wrap("e0", 200, make_sample(2.0, 200)),
      wrap("e0", 100, make_sample(1.0, 100)),
  };
  auto first_seen = dl::make_graph_anchor_edge_batch(samples, options);
  close(scalar(first_seen.anchor_keys.index({0})), 200.0, 1e-6,
        "first seen anchor 0");
  close(scalar(first_seen.anchor_keys.index({1})), 100.0, 1e-6,
        "first seen anchor 1");

  options.anchor_order = dl::graph_anchor_order_policy_t::sorted;
  auto sorted = dl::make_graph_anchor_edge_batch(samples, options);
  close(scalar(sorted.anchor_keys.index({0})), 100.0, 1e-6, "sorted anchor 0");
  close(scalar(sorted.anchor_keys.index({1})), 200.0, 1e-6, "sorted anchor 1");
}

void test_validation_failures() {
  auto options = options_for({"e0", "e1"});
  expect_throw(
      [&] {
        auto out =
            dl::make_graph_anchor_edge_batch<int64_t>({}, options_for({"e0"}));
        (void)out;
      },
      "empty samples");
  expect_throw(
      [&] {
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, make_sample(1.0, 100))},
            options_for({}));
        (void)out;
      },
      "empty edge ids");
  expect_throw(
      [&] {
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, make_sample(1.0, 100))},
            options_for({"e0", "e0"}));
        (void)out;
      },
      "duplicate edge ids");
  expect_throw(
      [&] {
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("missing", 100, make_sample(1.0, 100))},
            options);
        (void)out;
      },
      "unknown edge id");
  expect_throw(
      [&] {
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, make_sample(1.0, 100)),
                wrap("e0", 100, make_sample(2.0, 100))},
            options);
        (void)out;
      },
      "duplicate anchor edge pair");
  expect_throw(
      [&] {
        auto bad = make_sample(1.0, 100);
        bad.features = bad.features.unsqueeze(0);
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, bad)},
            options_for({"e0"}));
        (void)out;
      },
      "batched rank rejected");
  expect_throw(
      [&] {
        auto bad = make_sample(1.0, 100);
        bad.features = bad.features.index({0});
        bad.mask = bad.mask.index({0});
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, bad)},
            options_for({"e0"}));
        (void)out;
      },
      "rank two rejected");
  expect_throw(
      [&] {
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, make_sample(1.0, 100)),
                wrap("e1", 100, make_sample(2.0, 100, true, true, true, 3))},
            options);
        (void)out;
      },
      "C mismatch");
  expect_throw(
      [&] {
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, make_sample(1.0, 100)),
                wrap("e1", 100, make_sample(2.0, 100, true, true, true, 2, 4))},
            options);
        (void)out;
      },
      "Hx mismatch");
  expect_throw(
      [&] {
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, make_sample(1.0, 100)),
                wrap("e1", 100,
                     make_sample(2.0, 100, true, true, true, 2, 3, 2, 8))},
            options);
        (void)out;
      },
      "D mismatch");
  expect_throw(
      [&] {
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, make_sample(1.0, 100)),
                wrap("e1", 100,
                     make_sample(2.0, 100, true, true, true, 2, 3, 2, 9,
                                 torch::kFloat64))},
            options);
        (void)out;
      },
      "dtype mismatch");
  expect_throw(
      [&] {
        auto bad_options = options_for({"e0"});
        bad_options.feature_width = 8;
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, make_sample(1.0, 100))},
            bad_options);
        (void)out;
      },
      "feature width strict");
  expect_throw(
      [&] {
        auto bad = make_sample(1.0, 100);
        bad.features = torch::ones({2, 3, 9}, torch::kInt32);
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, bad)},
            options_for({"e0"}));
        (void)out;
      },
      "integer features rejected");
  options.require_all_edges = true;
  expect_throw(
      [&] {
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, make_sample(1.0, 100))},
            options);
        (void)out;
      },
      "require all edges");
}

void test_normalization_and_numeric_masks() {
  auto options = options_for({"e0"});
  expect_throw(
      [&] {
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, make_sample(1.0, 100, false))},
            options);
        (void)out;
      },
      "normalized required");

  options.require_normalized = false;
  auto accepted = dl::make_graph_anchor_edge_batch(
      std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
          wrap("e0", 100, make_sample(1.0, 100, false))},
      options);
  check(bscalar(accepted.edge_present.index({0, 0})),
        "unnormalized accepted when allowed");

  auto numeric_mask = make_sample(1.0, 100);
  numeric_mask.mask = torch::ones({2, 3}, torch::kInt32);
  auto converted = dl::make_graph_anchor_edge_batch(
      std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
          wrap("e0", 100, numeric_mask)},
      options_for({"e0"}));
  check(converted.edge_mask.scalar_type() == torch::kBool,
        "numeric mask converted to bool");
  check(converted.edge_mask.all().item<bool>(), "numeric mask truth");
}

void test_keys_and_future_modes() {
  auto options = options_for({"e0"});
  auto with_keys = dl::make_graph_anchor_edge_batch(
      std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
          wrap("e0", 100, make_sample(1.0, 100))},
      options);
  check(with_keys.past_keys.sizes() == torch::IntArrayRef({1, 1, 2, 3}),
        "past keys shape");
  check(with_keys.future_keys.sizes() == torch::IntArrayRef({1, 1, 2, 2}),
        "future keys shape");
  close(scalar(with_keys.past_keys.index({0, 0, 0, 2})), 100.0, 1e-6,
        "terminal past key");
  close(scalar(with_keys.future_keys.index({0, 0, 0, 0})), 101.0, 1e-6,
        "first future key");

  expect_throw(
      [&] {
        auto missing = make_sample(1.0, 100);
        missing.past_keys = torch::Tensor();
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, missing)},
            options);
        (void)out;
      },
      "missing past keys");

  expect_throw(
      [&] {
        auto missing = make_sample(1.0, 100);
        missing.future_keys = torch::Tensor();
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, missing)},
            options);
        (void)out;
      },
      "missing future keys");

  auto no_future = make_sample(1.0, 100, true, false, true);
  expect_throw(
      [&] {
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, no_future)},
            options);
        (void)out;
      },
      "future required");

  options.require_future = false;
  options.future_length = 4;
  auto filled = dl::make_graph_anchor_edge_batch(
      std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
          wrap("e0", 100, no_future)},
      options);
  check(filled.future_features.sizes() == torch::IntArrayRef({1, 1, 2, 4, 9}),
        "future_length fills shape");
  check(!filled.future_mask.any().item<bool>(), "missing future mask false");

  options.future_length = std::nullopt;
  expect_throw(
      [&] {
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, no_future)},
            options);
        (void)out;
      },
      "future_length required when all futures missing");

  std::vector<dl::graph_anchor_edge_sample_t<int64_t>> mixed{
      wrap("e0", 100, make_sample(1.0, 100)),
      wrap("e1", 100, make_sample(2.0, 100, true, false, true)),
  };
  auto mixed_options = options_for({"e0", "e1"});
  mixed_options.require_future = false;
  auto inferred = dl::make_graph_anchor_edge_batch(mixed, mixed_options);
  check(inferred.future_features.sizes() == torch::IntArrayRef({1, 2, 2, 2, 9}),
        "future inferred from present sample");
  check(!inferred.future_mask.index({0, 1}).any().item<bool>(),
        "missing per-sample future mask false");

  auto no_future_options = options_for({"e0"});
  no_future_options.include_future = false;
  auto omitted = dl::make_graph_anchor_edge_batch(
      std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
          wrap("e0", 100, make_sample(1.0, 100))},
      no_future_options);
  check(!omitted.future_features.defined(), "future_features undefined");
  check(!omitted.future_mask.defined(), "future_mask undefined");

  auto no_keys_options = options_for({"e0"});
  no_keys_options.include_keys = false;
  auto no_keys_sample = make_sample(1.0, 100);
  no_keys_sample.past_keys = torch::Tensor();
  no_keys_sample.future_keys = torch::Tensor();
  auto no_keys = dl::make_graph_anchor_edge_batch(
      std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
          wrap("e0", 100, no_keys_sample)},
      no_keys_options);
  check(!no_keys.past_keys.defined(), "past_keys undefined when omitted");
  check(!no_keys.future_keys.defined(), "future_keys undefined when omitted");
}

void test_key_alignment_validation() {
  auto options = options_for({"e0"});
  options.validate_keys_against_anchor = true;
  auto valid = dl::make_graph_anchor_edge_batch(
      std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
          wrap("e0", 100, make_sample(1.0, 100))},
      options);
  check(valid.edge_present.all().item<bool>(), "valid key alignment");

  expect_throw(
      [&] {
        auto bad = make_sample(1.0, 100);
        bad.past_keys.index_put_({0, 2}, 101);
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, bad)},
            options);
        (void)out;
      },
      "past key after anchor");

  expect_throw(
      [&] {
        auto bad = make_sample(1.0, 100);
        bad.future_keys.index_put_({0, 0}, 100);
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, bad)},
            options);
        (void)out;
      },
      "future key not after anchor");

  expect_throw(
      [&] {
        auto bad_options = options_for({"e0"});
        bad_options.include_keys = false;
        bad_options.validate_keys_against_anchor = true;
        auto out = dl::make_graph_anchor_edge_batch(
            std::vector<dl::graph_anchor_edge_sample_t<int64_t>>{
                wrap("e0", 100, make_sample(1.0, 100))},
            bad_options);
        (void)out;
      },
      "key validation requires keys");
}

} // namespace

int main() {
  test_single_anchor_full_graph();
  test_missing_and_present_masked_edges();
  test_anchor_order();
  test_validation_failures();
  test_normalization_and_numeric_masks();
  test_keys_and_future_modes();
  test_key_alignment_validation();
  std::cout << "[GraphAnchorEdgeBatch test] ok\n";
  return 0;
}
