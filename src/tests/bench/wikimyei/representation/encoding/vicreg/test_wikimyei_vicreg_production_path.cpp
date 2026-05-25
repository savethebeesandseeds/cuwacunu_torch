#include "wikimyei/inference/expected_value/mdn/assembly.h"
#include "wikimyei/inference/expected_value/mdn/channel_context_mdn.h"
#include "wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h"
#include "wikimyei/inference/expected_value/mdn/stream/mdn_adapter.h"
#include "wikimyei/representation/encoding/vicreg/assembly.h"
#include "wikimyei/representation/encoding/vicreg/channel_global_fusion.h"
#include "wikimyei/representation/encoding/vicreg/channel_node_stream_adapter.h"
#include "wikimyei/representation/encoding/vicreg/channel_preserving_encoder.h"
#include "wikimyei/representation/encoding/vicreg/channel_representation_adapter.h"
#include "wikimyei/representation/encoding/vicreg/stream/channel_representation_stream.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_projector.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_train_model.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <torch/torch.h>

namespace assembly = cuwacunu::wikimyei::assembly;
namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
namespace mdn_stream =
    cuwacunu::wikimyei::inference::expected_value::mdn::stream;
namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
namespace vicreg_stream =
    cuwacunu::wikimyei::representation::encoding::vicreg::stream;
namespace runtime_lls = cuwacunu::kikijyeba::lattice::runtime_report;
namespace nodelift = cuwacunu::wikimyei::expression::nodelift::srl::stream;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void close(double actual, double expected, double tolerance,
           const std::string &message) {
  if (std::abs(actual - expected) > tolerance) {
    throw std::runtime_error(message + " expected=" + std::to_string(expected) +
                             " actual=" + std::to_string(actual));
  }
}

nodelift::node_lifted_batch_t<int64_t> make_lifted_batch() {
  constexpr int64_t B = 2;
  constexpr int64_t C = 2;
  constexpr int64_t Hx = 4;
  constexpr int64_t N = 2;
  constexpr int64_t Dx = 3;
  constexpr int64_t Hf = 1;

  nodelift::node_lifted_batch_t<int64_t> out{};
  out.node_features = torch::empty({B, C, Hx, N, Dx}, torch::kFloat32);
  auto acc = out.node_features.accessor<float, 5>();
  for (int64_t b = 0; b < B; ++b) {
    for (int64_t c = 0; c < C; ++c) {
      for (int64_t h = 0; h < Hx; ++h) {
        for (int64_t n = 0; n < N; ++n) {
          for (int64_t d = 0; d < Dx; ++d) {
            acc[b][c][h][n][d] =
                static_cast<float>(1000 * b + 100 * n + 10 * c + h + 0.1 * d);
          }
        }
      }
    }
  }
  out.node_mask = torch::ones({B, C, Hx, N, Dx}, torch::kBool);
  out.future_node_features = torch::randn({B, C, Hf, N, Dx}, torch::kFloat32);
  out.future_node_mask = torch::ones({B, C, Hf, N, Dx}, torch::kBool);
  out.future_node_mask.index_put_({0, 1, 0, 0, 2}, false);
  out.anchor_keys = torch::tensor({100, 200}, torch::kInt64);
  out.node_ids = {"n0", "n1"};
  out.edge_ids = {"e0", "e1"};
  out.graph_order_fingerprint = "vicreg-production-test";
  out.cursor.graph_order_fingerprint = out.graph_order_fingerprint;
  out.cursor.begin_anchor_index = 0;
  out.cursor.end_anchor_index = B;
  out.cursor.requested_batch_size = B;
  out.cursor.anchor_keys = {100, 200};
  return out;
}

vicreg::channel_preserving_encoder_options_t make_encoder_options() {
  vicreg::channel_preserving_encoder_options_t opts{};
  opts.channel_count = 2;
  opts.history_length = 4;
  opts.input_width = 3;
  opts.encoding_dim = 5;
  opts.feature_hidden_dim = 7;
  opts.temporal_depth = 1;
  opts.recency_decay = 0.5;
  opts.cell_valid_policy = vicreg::cell_valid_policy_t::all_features;
  return opts;
}

void test_vicreg_production_components() {
  torch::manual_seed(17);
  auto lifted = make_lifted_batch();
  lifted.node_mask.index_put_(
      {0, 1, torch::indexing::Slice(), 0, torch::indexing::Slice()}, false);

  auto input = vicreg::make_channel_node_encoder_input(lifted);
  check(input.data.sizes() == torch::IntArrayRef({4, 2, 4, 3}),
        "production adapter preserves [M,C,Hx,Dx]");
  check(input.feature_mask.sizes() == torch::IntArrayRef({4, 2, 4, 3}),
        "production adapter preserves feature masks");

  auto encoder = vicreg::ChannelPreservingEncoder(make_encoder_options());
  encoder->eval();
  torch::NoGradGuard no_grad;
  const auto encoded = encoder->forward(input.data, input.feature_mask);
  check(encoded.sequence.sizes() == torch::IntArrayRef({4, 2, 4, 5}),
        "production encoder emits [M,C,Hx,De]");
  check(encoded.reduced.sizes() == torch::IntArrayRef({4, 2, 5}),
        "production encoder emits reduced [M,C,De]");

  vicreg::vicreg_projector_options_t popts{};
  popts.input_dim = 5;
  popts.projector_dim = 6;
  popts.hidden_dim = 8;
  auto projector = vicreg::VicregProjector(popts);
  const auto projected = projector->forward(encoded.reduced);
  check(projected.sizes() == torch::IntArrayRef({4, 2, 6}),
        "production VICReg projector preserves channel rank");

  auto batch = vicreg::make_channel_representation_batch(
      encoded.reduced, encoded.reduced_mask,
      vicreg::make_graph_row_index(input), input.anchor_keys, input.node_ids,
      encoded.reducer_weights);
  check(batch.node_encoding.sizes() == torch::IntArrayRef({2, 2, 2, 5}),
        "production adapter exports [B,N,C,De]");
  check(batch.node_encoding_mask.sizes() == torch::IntArrayRef({2, 2, 2}),
        "production adapter exports [B,N,C] mask");
}

void test_channel_train_models_and_streams() {
  torch::manual_seed(23);
  auto lifted = make_lifted_batch();
  auto input = vicreg::make_channel_node_encoder_input(lifted);

  auto encoder = vicreg::ChannelPreservingEncoder(make_encoder_options());
  vicreg::vicreg_projector_options_t popts{};
  popts.input_dim = 5;
  popts.projector_dim = 6;
  popts.hidden_dim = 8;
  auto projector = vicreg::VicregProjector(popts);
  vicreg::vicreg_train_options_t train_opts{};
  train_opts.jitter_std = 0.001;
  vicreg::vicreg_train_model_t vicreg_train(encoder, projector, 0.001,
                                            train_opts);
  auto train_result =
      vicreg_train.train_one_batch(input.data, input.feature_mask);
  check(!train_result.skipped, "VICReg train step is applied");
  check(train_result.optimizer_step_applied,
        "VICReg optimizer step is applied");
  check(train_result.per_channel_valid_rows.sizes() == torch::IntArrayRef({2}),
        "VICReg train result reports per-channel valid rows");
  check(train_result.view1_valid_feature_count > 0 &&
            train_result.view2_valid_feature_count > 0,
        "VICReg train result reports view evidence counts");
  check(std::isfinite(train_result.view1_valid_feature_fraction) &&
            std::isfinite(train_result.view2_valid_feature_fraction),
        "VICReg train result reports finite view evidence fractions");
  check(std::isfinite(train_result.view1_feature_retention_fraction) &&
            std::isfinite(train_result.view2_feature_retention_fraction),
        "VICReg train result reports finite view retention fractions");

  auto stream_batch =
      vicreg_stream::make_channel_representation_stream_batch(encoder, lifted);
  check(stream_batch.node_encoding.sizes() == torch::IntArrayRef({2, 2, 2, 5}),
        "production channel representation stream exports [B,N,C,De]");
  check(stream_batch.reducer_weights.sizes() ==
            torch::IntArrayRef({2, 2, 2, 4}),
        "production channel representation stream carries reducer weights");
  auto debug_stream_batch =
      vicreg_stream::make_channel_representation_stream_batch(
          encoder, lifted,
          /*require_finite_valid_features=*/true, /*detach_to_cpu=*/false,
          runtime_lls::runtime_report_mode_t::debug);
  check(debug_stream_batch.runtime_lls.find(
            "schema:str = wikimyei.representation.vicreg.runtime.v1") !=
            std::string::npos,
        "debug channel representation stream emits runtime LLS schema");
  check(debug_stream_batch.runtime_lls.find("encoded_rank[0,+inf):uint = 4") !=
            std::string::npos,
        "debug channel representation runtime LLS records channel rank");
  check(debug_stream_batch.runtime_lls.find("reducer_weight_entropy") !=
            std::string::npos,
        "debug channel representation runtime LLS records reducer entropy");
  auto reducer_weights =
      torch::tensor({0.1, 0.2, 0.7, 0.3, 0.7, 0.0}, torch::kFloat32)
          .view({1, 1, 2, 3});
  auto reducer_mask =
      torch::tensor({true, true, true, true, true, false}, torch::kBool)
          .view({1, 1, 2, 3});
  close(
      vicreg_stream::channel_representation_stream_detail::
          reducer_last_valid_weight_mean_or_nan(reducer_weights, reducer_mask),
      0.7, 1e-6,
      "reducer diagnostic gathers the anchor-nearest valid history weight");
  check(
      debug_stream_batch.runtime_lls.find("reducer_last_valid_weight_mean") !=
          std::string::npos,
      "debug channel representation runtime LLS records anchor reducer weight");

  mdn_stream::channel_mdn_adapter_options_t mdn_adapter_opts{};
  mdn_adapter_opts.target_coords = {0, 1};
  auto mdn_input_batch =
      mdn_stream::make_channel_mdn_input_batch(stream_batch, mdn_adapter_opts);
  check(mdn_input_batch.context.sizes() == torch::IntArrayRef({2, 2, 2, 5}),
        "production MDN adapter emits [B,N,C,De]");
  check(mdn_input_batch.future.sizes() == torch::IntArrayRef({2, 2, 2, 2}),
        "production MDN adapter emits [B,N,C,Df]");

  auto mdn = mdn::ChannelContextMdn(5, 2, 2, 1, 2, 7, 1);
  mdn::channel_context_mdn_train_model_t mdn_train(mdn, 0.001);
  auto mdn_input = mdn_stream::to_channel_mdn_input(mdn_input_batch);
  auto mdn_result = mdn_train.train_one_batch(mdn_input);
  check(!mdn_result.skipped, "MDN train step is applied");
  check(mdn_result.optimizer_step_applied, "MDN optimizer step is applied");
  check(mdn_result.nll_per_channel.sizes() == torch::IntArrayRef({2}),
        "MDN reports NLL per channel");
  check(mdn_result.nll_per_target_feature.sizes() == torch::IntArrayRef({2}),
        "MDN reports NLL per target feature");
  check(mdn_result.mixture_usage.sizes() == torch::IntArrayRef({2}),
        "MDN reports mixture usage");
  check(std::isfinite(mdn_result.sigma_mean), "MDN sigma mean finite");
  check(std::isfinite(mdn_result.sigma_mean_valid),
        "MDN masked sigma mean finite");
  check(std::isfinite(mdn_result.sigma_min_valid),
        "MDN masked sigma min finite");
  check(std::isfinite(mdn_result.sigma_max_valid),
        "MDN masked sigma max finite");
}

void test_channel_global_fusion_is_explicit_and_mask_safe() {
  torch::manual_seed(67);
  vicreg::channel_global_fusion_options_t opts{};
  opts.channel_count = 3;
  opts.encoding_dim = 5;
  opts.global_dim = 4;
  opts.hidden_dim = 6;
  auto fusion = vicreg::ChannelGlobalFusion(opts);

  auto context = torch::randn({2, 2, 3, 5}, torch::kFloat32);
  auto mask = torch::ones({2, 2, 3}, torch::kBool);
  mask.index_put_({0, 1, 2}, false);
  mask.index_put_({1, 1, torch::indexing::Slice()}, false);
  auto baseline = context.clone();
  baseline.index_put_({0, 1, 2, torch::indexing::Slice()}, 0.0);
  baseline.index_put_(
      {1, 1, torch::indexing::Slice(), torch::indexing::Slice()}, 0.0);

  auto sentinel = baseline.clone();
  sentinel.index_put_({0, 1, 2, torch::indexing::Slice()},
                      std::numeric_limits<float>::quiet_NaN());
  sentinel.index_put_(
      {1, 1, torch::indexing::Slice(), torch::indexing::Slice()},
      std::numeric_limits<float>::quiet_NaN());

  fusion->eval();
  torch::NoGradGuard no_grad;
  const auto baseline_out = fusion->forward(baseline, mask);
  const auto sentinel_out = fusion->forward(sentinel, mask);

  check(baseline_out.global_context.sizes() == torch::IntArrayRef({2, 2, 4}),
        "global fusion emits [B,N,Dg]");
  check(baseline_out.global_mask.sizes() == torch::IntArrayRef({2, 2}),
        "global fusion emits [B,N] mask");
  check(baseline_out.channel_weights.sizes() == torch::IntArrayRef({2, 2, 3}),
        "global fusion exposes channel weights");
  check(!baseline_out.global_mask.index({1, 1}).item<bool>(),
        "global fusion marks all-invalid nodes invalid");
  close(baseline_out.global_context.index({1, 1}).abs().sum().item<double>(),
        0.0, 1e-8, "global fusion zeros all-invalid global context");
  close(baseline_out.channel_weights.index({0, 1, 2}).item<double>(), 0.0, 1e-8,
        "global fusion gives invalid channels zero weight");
  close(baseline_out.channel_weights.index({0, 0}).sum().item<double>(), 1.0,
        1e-6, "global fusion weights sum to one for valid nodes");
  check(torch::allclose(sentinel_out.global_context,
                        baseline_out.global_context, 1e-6, 1e-6),
        "masked invalid channel sentinels do not affect global context");
  check(torch::allclose(sentinel_out.channel_weights,
                        baseline_out.channel_weights, 1e-6, 1e-6),
        "masked invalid channel sentinels do not affect global weights");
}

void test_channel_mdn_plus_global_is_separate_and_mask_safe() {
  torch::manual_seed(73);
  constexpr int64_t B = 2;
  constexpr int64_t N = 2;
  constexpr int64_t C = 3;
  constexpr int64_t De = 5;
  constexpr int64_t Dg = 4;
  constexpr int64_t Hf = 1;
  constexpr int64_t Df = 2;

  auto channel_context = torch::randn({B, N, C, De}, torch::kFloat32);
  auto channel_mask = torch::ones({B, N, C}, torch::kBool);
  channel_mask.index_put_({0, 1, 2}, false);
  auto global_context = torch::randn({B, N, Dg}, torch::kFloat32);
  auto global_mask = torch::ones({B, N}, torch::kBool);
  global_mask.index_put_({1, 0}, false);
  auto baseline_channel = channel_context.clone();
  baseline_channel.index_put_({0, 1, 2, torch::indexing::Slice()}, 0.0);
  auto baseline_global = global_context.clone();
  baseline_global.index_put_({1, 0, torch::indexing::Slice()}, 0.0);

  auto sentinel_channel = baseline_channel.clone();
  sentinel_channel.index_put_({0, 1, 2, torch::indexing::Slice()},
                              std::numeric_limits<float>::quiet_NaN());
  auto sentinel_global = baseline_global.clone();
  sentinel_global.index_put_({1, 0, torch::indexing::Slice()},
                             std::numeric_limits<float>::quiet_NaN());

  auto mdn = mdn::ChannelContextPlusGlobalMdn(De, Dg, Df, C, Hf, 2, 7, 1);
  mdn->eval();
  torch::NoGradGuard no_grad;
  const auto baseline_out = mdn->forward(baseline_channel, channel_mask,
                                         baseline_global, global_mask);
  const auto sentinel_out = mdn->forward(sentinel_channel, channel_mask,
                                         sentinel_global, global_mask);

  check(baseline_out.log_pi.sizes() == torch::IntArrayRef({B, N, C, Df, 2}),
        "plus-global MDN emits channel log_pi shape");
  check(baseline_out.mu.sizes() == torch::IntArrayRef({B, N, C, Df, 2}),
        "plus-global MDN emits channel mu shape");
  check(baseline_out.sigma.sizes() == torch::IntArrayRef({B, N, C, Df, 2}),
        "plus-global MDN emits channel sigma shape");
  check(torch::allclose(sentinel_out.log_pi, baseline_out.log_pi, 1e-6, 1e-6),
        "masked invalid plus-global context does not affect log_pi");
  check(torch::allclose(sentinel_out.mu, baseline_out.mu, 1e-6, 1e-6),
        "masked invalid plus-global context does not affect mu");
  check(torch::allclose(sentinel_out.sigma, baseline_out.sigma, 1e-6, 1e-6),
        "masked invalid plus-global context does not affect sigma");

  mdn::channel_mdn_plus_global_input_t input{};
  input.channel.context = baseline_channel;
  input.channel.context_mask = channel_mask;
  input.channel.future = torch::randn({B, N, C, Df}, torch::kFloat32);
  input.channel.future_mask = torch::ones({B, N, C, Df}, torch::kBool);
  input.global_context = baseline_global;
  input.global_mask = global_mask;
  const auto combined =
      mdn::combine_channel_plus_global_context_and_future_mask(
          input.channel.context_mask, input.global_mask,
          input.channel.future_mask);
  check(!combined.index({0, 1, 2, 0}).item<bool>(),
        "plus-global mask rejects invalid channel context rows");
  check(combined.index({1, 0}).sum().item<int64_t>() == 0,
        "plus-global mask rejects every target for invalid global context");
  const auto nll =
      mdn::compute_channel_context_plus_global_mdn_nll(baseline_out, input);
  check(torch::isfinite(nll).item<bool>(),
        "plus-global MDN masked NLL is finite");
}

void test_channel_mdn_plus_global_train_model_sanitizes_masked_sentinels() {
  constexpr int64_t B = 2;
  constexpr int64_t N = 2;
  constexpr int64_t C = 2;
  constexpr int64_t De = 5;
  constexpr int64_t Dg = 3;
  constexpr int64_t Hf = 1;
  constexpr int64_t Df = 2;

  mdn::channel_mdn_plus_global_input_t baseline{};
  baseline.channel.context = torch::randn({B, N, C, De}, torch::kFloat32);
  baseline.channel.context_mask = torch::ones({B, N, C}, torch::kBool);
  baseline.channel.context_mask.index_put_({0, 1, 1}, false);
  baseline.channel.context.index_put_({0, 1, 1, torch::indexing::Slice()}, 0.0);
  baseline.channel.future = torch::randn({B, N, C, Df}, torch::kFloat32);
  baseline.channel.future_mask = torch::ones({B, N, C, Df}, torch::kBool);
  baseline.channel.future_mask.index_put_({0, 1, 1, 0}, false);
  baseline.channel.future.index_put_({0, 1, 1, 0}, 0.0);
  baseline.global_context = torch::randn({B, N, Dg}, torch::kFloat32);
  baseline.global_mask = torch::ones({B, N}, torch::kBool);
  baseline.global_mask.index_put_({1, 0}, false);
  baseline.global_context.index_put_({1, 0, torch::indexing::Slice()}, 0.0);

  auto sentinel = baseline;
  sentinel.channel.context = baseline.channel.context.clone();
  sentinel.channel.future = baseline.channel.future.clone();
  sentinel.global_context = baseline.global_context.clone();
  sentinel.channel.context.index_put_({0, 1, 1, torch::indexing::Slice()},
                                      std::numeric_limits<float>::quiet_NaN());
  sentinel.channel.future.index_put_({0, 1, 1, 0},
                                     std::numeric_limits<float>::quiet_NaN());
  sentinel.global_context.index_put_({1, 0, torch::indexing::Slice()},
                                     std::numeric_limits<float>::quiet_NaN());

  auto train_once = [=](const mdn::channel_mdn_plus_global_input_t &input) {
    torch::manual_seed(89);
    auto mdn = mdn::ChannelContextPlusGlobalMdn(De, Dg, Df, C, Hf, 2, 7, 1);
    mdn::channel_context_plus_global_mdn_train_model_t mdn_train(mdn, 0.001);
    return mdn_train.train_one_batch(input);
  };

  const auto baseline_result = train_once(baseline);
  const auto sentinel_result = train_once(sentinel);
  check(!baseline_result.skipped && !sentinel_result.skipped,
        "masked plus-global sentinels do not skip valid batches");
  check(baseline_result.optimizer_step_applied &&
            sentinel_result.optimizer_step_applied,
        "masked plus-global sentinels still allow optimizer steps");
  check(sentinel_result.nonfinite_output_count == 0,
        "masked plus-global invalid context is sanitized before forward");
  check(std::isfinite(sentinel_result.sigma_mean_valid),
        "plus-global masked sigma mean is finite");
  close(sentinel_result.nll.item<double>(), baseline_result.nll.item<double>(),
        1e-6, "masked plus-global invalid values are sanitized before NLL");

  auto no_global = baseline;
  no_global.global_mask = torch::zeros({B, N}, torch::kBool);
  auto mdn = mdn::ChannelContextPlusGlobalMdn(De, Dg, Df, C, Hf, 2, 7, 1);
  mdn::channel_context_plus_global_mdn_train_model_t mdn_train(mdn, 0.001);
  const auto no_global_result = mdn_train.train_one_batch(no_global);
  check(no_global_result.skipped,
        "plus-global MDN skips batches with no valid global context");
  close(no_global_result.loss.item<double>(), 0.0, 1e-8,
        "plus-global MDN no-valid-global batch has zero loss");
}

void test_vicreg_safe_augmentations() {
  torch::manual_seed(29);
  auto data = torch::ones({2, 2, 3, 4}, torch::kFloat32);
  auto mask = torch::ones({2, 2, 3, 4}, torch::kBool);
  mask.index_put_({0, 1, 2, 3}, false);
  data.index_put_({0, 1, 2, 3}, 1.0e20);

  vicreg::vicreg_train_options_t jitter_opts{};
  jitter_opts.jitter_std = 0.05;
  const auto jittered = vicreg::vicreg_train_detail::augment_channel_view(
      data, mask, jitter_opts);
  check(torch::equal(jittered.feature_mask, mask),
        "valid-only jitter preserves the feature mask");
  close(jittered.data.index({0, 1, 2, 3}).item<double>(), 0.0, 1e-8,
        "valid-only jitter zeros invalid sentinel values");

  vicreg::vicreg_train_options_t feature_drop_opts{};
  feature_drop_opts.jitter_std = 0.0;
  feature_drop_opts.feature_dropout_prob = 1.0;
  const auto feature_dropped =
      vicreg::vicreg_train_detail::augment_channel_view(data, mask,
                                                        feature_drop_opts);
  check(feature_dropped.feature_mask.sum().item<int64_t>() == 0,
        "feature dropout updates every dropped feature mask");
  close(feature_dropped.data.abs().sum().item<double>(), 0.0, 1e-8,
        "feature dropout zeros every dropped feature value");

  vicreg::vicreg_train_options_t history_drop_opts{};
  history_drop_opts.jitter_std = 0.0;
  history_drop_opts.history_dropout_prob = 1.0;
  const auto history_dropped =
      vicreg::vicreg_train_detail::augment_channel_view(data, mask,
                                                        history_drop_opts);
  check(history_dropped.feature_mask.sum().item<int64_t>() == 0,
        "history dropout updates feature masks for dropped history cells");
  close(history_dropped.data.abs().sum().item<double>(), 0.0, 1e-8,
        "history dropout zeros values for dropped history cells");

  auto encoder = vicreg::ChannelPreservingEncoder(make_encoder_options());
  vicreg::vicreg_projector_options_t popts{};
  popts.input_dim = 5;
  popts.projector_dim = 6;
  popts.hidden_dim = 8;
  auto projector = vicreg::VicregProjector(popts);
  vicreg::vicreg_train_options_t full_drop_opts{};
  full_drop_opts.jitter_std = 0.0;
  full_drop_opts.feature_dropout_prob = 1.0;
  full_drop_opts.min_valid_rows = 1;
  vicreg::vicreg_train_model_t model(encoder, projector, 0.001, full_drop_opts);
  auto full_drop_result =
      model.train_one_batch(torch::ones({2, 2, 4, 3}, torch::kFloat32),
                            torch::ones({2, 2, 4, 3}, torch::kBool));
  check(full_drop_result.skipped,
        "full feature dropout train step is skipped safely");
  check(full_drop_result.view1_valid_feature_count == 0 &&
            full_drop_result.view2_valid_feature_count == 0,
        "full feature dropout train step reports zero retained features");
  close(full_drop_result.view1_valid_feature_fraction, 0.0, 1e-8,
        "full feature dropout view1 fraction is zero");
  close(full_drop_result.view2_valid_feature_fraction, 0.0, 1e-8,
        "full feature dropout view2 fraction is zero");
}

void test_production_assemblies() {
  const auto representation = vicreg::make_vicreg_assembly();
  const auto global_fusion = vicreg::make_channel_global_fusion_assembly();
  const auto mdn = mdn::make_channel_context_mdn_assembly();
  const auto mdn_plus_global =
      mdn::make_channel_context_plus_global_mdn_assembly();
  check(assembly::dock_domain_compatible(representation.docks.at(1),
                                         mdn.docks.at(0)),
        "production channel representation binds MDN");
  check(assembly::dock_domain_compatible(representation.docks.at(1),
                                         global_fusion.docks.at(0)),
        "production channel representation binds explicit global fusion");
  check(!assembly::dock_domain_compatible(global_fusion.docks.at(1),
                                          mdn.docks.at(0)),
        "auxiliary global context cannot bind strict MDN");
  check(assembly::dock_domain_compatible(representation.docks.at(1),
                                         mdn_plus_global.docks.at(0)),
        "production channel representation binds plus-global MDN");
  check(assembly::dock_domain_compatible(global_fusion.docks.at(1),
                                         mdn_plus_global.docks.at(1)),
        "explicit global fusion binds plus-global MDN");
  check(mdn.docks.at(2).coordinate_space ==
            "graph_order.channel_node_future_distribution.v1",
        "production MDN output coordinate space is channel-explicit");

  const auto old_fused_producer = assembly::make_dock(
      "old_fused_node_representation", assembly::dock_direction_t::produces,
      assembly::dock_role_t::output,
      assembly::dock_domain_t::node_representation,
      "graph_order.node_representation.v1", "[B,N,De]", "[B,N]",
      /*required=*/true, /*target_side_only=*/false, {"B", "N", "De"});
  check(!assembly::dock_domain_compatible(old_fused_producer, mdn.docks.at(0)),
        "old fused representation cannot bind production MDN");
}

void test_channel_mdn_zero_valid_targets() {
  auto input = mdn::channel_mdn_input_t{};
  input.context = torch::randn({2, 2, 2, 5}, torch::kFloat32);
  input.context_mask = torch::zeros({2, 2, 2}, torch::kBool);
  input.future = torch::randn({2, 2, 2, 2}, torch::kFloat32);
  input.future_mask = torch::zeros({2, 2, 2, 2}, torch::kBool);
  auto mdn = mdn::ChannelContextMdn(5, 2, 2, 1, 2, 7, 1);
  mdn::channel_context_mdn_train_model_t mdn_train(mdn, 0.001);
  auto result = mdn_train.train_one_batch(input);
  check(result.skipped, "zero-valid MDN batch is skipped");
  close(result.loss.item<double>(), 0.0, 1e-8,
        "zero-valid MDN batch has zero loss");
}

void test_channel_mdn_train_model_sanitizes_masked_sentinels() {
  auto baseline = mdn::channel_mdn_input_t{};
  baseline.context = torch::randn({3, 2, 2, 5}, torch::kFloat32);
  baseline.context_mask = torch::ones({3, 2, 2}, torch::kBool);
  baseline.context_mask.index_put_({1, 1, 1}, false);
  baseline.context.index_put_({1, 1, 1, torch::indexing::Slice()}, 0.0);
  baseline.future = torch::randn({3, 2, 2, 2}, torch::kFloat32);
  baseline.future_mask = torch::ones({3, 2, 2, 2}, torch::kBool);
  baseline.future_mask.index_put_({0, 1, 1, 1}, false);
  baseline.future.index_put_({0, 1, 1, 1}, 0.0);

  auto sentinel = baseline;
  sentinel.context = baseline.context.clone();
  sentinel.future = baseline.future.clone();
  sentinel.context.index_put_({1, 1, 1, torch::indexing::Slice()},
                              std::numeric_limits<float>::quiet_NaN());
  sentinel.future.index_put_({0, 1, 1, 1},
                             std::numeric_limits<float>::quiet_NaN());

  auto train_once = [](const mdn::channel_mdn_input_t &input) {
    torch::manual_seed(71);
    auto mdn = mdn::ChannelContextMdn(5, 2, 2, 1, 2, 7, 1);
    mdn::channel_context_mdn_train_model_t mdn_train(mdn, 0.001);
    return mdn_train.train_one_batch(input);
  };

  const auto baseline_result = train_once(baseline);
  const auto sentinel_result = train_once(sentinel);
  check(!baseline_result.skipped && !sentinel_result.skipped,
        "masked MDN sentinels do not skip valid batches");
  check(baseline_result.optimizer_step_applied &&
            sentinel_result.optimizer_step_applied,
        "masked MDN sentinels still allow optimizer steps");
  check(sentinel_result.nonfinite_output_count == 0,
        "masked invalid context is sanitized before MDN forward");
  check(std::isfinite(sentinel_result.sigma_mean_valid),
        "masked MDN sigma mean is finite");
  close(sentinel_result.nll.item<double>(), baseline_result.nll.item<double>(),
        1e-6, "masked invalid future values are sanitized before MDN NLL");
}

} // namespace

int main() {
  try {
    test_vicreg_production_components();
    test_channel_train_models_and_streams();
    test_channel_global_fusion_is_explicit_and_mask_safe();
    test_channel_mdn_plus_global_is_separate_and_mask_safe();
    test_channel_mdn_plus_global_train_model_sanitizes_masked_sentinels();
    test_vicreg_safe_augmentations();
    test_production_assemblies();
    test_channel_mdn_zero_valid_targets();
    test_channel_mdn_train_model_sanitizes_masked_sentinels();
    std::cout << "[test_wikimyei_vicreg_production_path] all checks "
                 "passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "[test_wikimyei_vicreg_production_path] failure: " << ex.what()
              << "\n";
    return 1;
  }
}
