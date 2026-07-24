#include "wikimyei/inference/expected_value/mdn/mixture_density_network_head.h"
#include "wikimyei/inference/expected_value/mdn/per_edge_affine_return_head.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include <torch/torch.h>

namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;

namespace {

constexpr std::int64_t kBatch = 2;
constexpr std::int64_t kEdges = 3;
constexpr std::int64_t kNodes = kEdges + 1;
constexpr std::int64_t kChannels = 2;
constexpr std::int64_t kFeatureDim = 5;
constexpr std::int64_t kDynamicFeatureDim = 3 * kFeatureDim;
constexpr std::int64_t kIdentityDim = 2;
constexpr std::int64_t kReadoutFeatureDim = kDynamicFeatureDim + kIdentityDim;

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <typename Fn> void expect_throw(Fn &&fn, const std::string &message) {
  bool threw = false;
  try {
    fn();
  } catch (const std::exception &) {
    threw = true;
  }
  check(threw, message);
}

void check_equal(const torch::Tensor &actual, const torch::Tensor &expected,
                 const std::string &message) {
  check(actual.defined() && expected.defined() && actual.equal(expected),
        message);
}

void check_close(const torch::Tensor &actual, const torch::Tensor &expected,
                 double tolerance, const std::string &message) {
  check(actual.sizes() == expected.sizes(), message + " shape mismatch");
  const auto max_delta =
      (actual.to(torch::kFloat64) - expected.to(torch::kFloat64))
          .abs()
          .max()
          .item<double>();
  check(std::isfinite(max_delta) && max_delta <= tolerance,
        message + " max_delta=" + std::to_string(max_delta));
}

mdn::PerEdgeAffineReturnHeadOptions make_options() {
  return {
      .feature_dim = kFeatureDim,
      .edge_count = kEdges,
      .readout_feature_dim = kReadoutFeatureDim,
      .channel_count = kChannels,
      .quote_node_index = 0,
  };
}

mdn::PerEdgeAffineReturnHeadState make_state() {
  const auto f64 = torch::TensorOptions().dtype(torch::kFloat64);
  auto mean = torch::linspace(-0.25, 0.35, kDynamicFeatureDim, f64);
  auto inv_std = torch::linspace(0.75, 1.25, kDynamicFeatureDim, f64);
  auto weights = torch::arange(kEdges * kDynamicFeatureDim, f64)
                     .view({kEdges, kDynamicFeatureDim})
                     .mul(0.002)
                     .sub(0.04);
  auto bias = torch::tensor({-0.03, 0.02, 0.07}, f64);
  return {
      .feature_mean = std::move(mean),
      .feature_inv_std = std::move(inv_std),
      .weights = std::move(weights),
      .bias = std::move(bias),
  };
}

torch::Tensor make_context() {
  return torch::arange(kBatch * kNodes * kChannels * kFeatureDim,
                       torch::TensorOptions().dtype(torch::kFloat32))
      .view({kBatch, kNodes, kChannels, kFeatureDim})
      .mul(0.013f)
      .sub(0.8f);
}

mdn::PerEdgeAffineReturnHeadArtifactMetadata make_metadata() {
  mdn::PerEdgeAffineReturnHeadArtifactMetadata metadata;
  metadata.feature_dim = kFeatureDim;
  metadata.edge_count = kEdges;
  metadata.readout_feature_dim = kReadoutFeatureDim;
  metadata.channel_count = kChannels;
  metadata.quote_node_index = 0;
  metadata.selected_alpha = 1.0e-10;
  metadata.selection_train_begin = 0;
  metadata.selection_train_end = 554;
  metadata.validation_purge_begin = 554;
  metadata.validation_purge_end = 584;
  metadata.validation_begin = 584;
  metadata.validation_end = 730;
  metadata.refit_begin = 0;
  metadata.refit_end = 730;
  metadata.valid_from_anchor = 730;
  metadata.graph_order_fingerprint = "synthetic_graph_order.v1.sha256:feed";
  metadata.edge_ids = {"BTC/USDT", "ETH/USDT", "SOL/USDT"};
  metadata.node_ids = {"USDT", "BTC", "ETH", "SOL"};
  metadata.fit_probe_schema_id = "synthetic_mdn_frozen_feature_capture.v1";
  metadata.fit_probe_sha256 = std::string(64, 'a');
  metadata.representation_checkpoint_sha256 = std::string(64, 'b');
  metadata.mdn_checkpoint_sha256 = std::string(64, 'c');
  metadata.legacy_capture_authority =
      "legacy_capture_exact_post_adapter_readout.v1";
  return metadata;
}

void check_state_equal(const mdn::PerEdgeAffineReturnHeadState &actual,
                       const mdn::PerEdgeAffineReturnHeadState &expected,
                       const std::string &message) {
  check_equal(actual.feature_mean, expected.feature_mean,
              message + " feature_mean");
  check_equal(actual.feature_inv_std, expected.feature_inv_std,
              message + " feature_inv_std");
  check_equal(actual.weights, expected.weights, message + " weights");
  check_equal(actual.bias, expected.bias, message + " bias");
}

torch::Tensor manual_dynamic_features(const torch::Tensor &context) {
  auto quote = context.select(1, 0).unsqueeze(1).expand(
      {kBatch, kEdges, kChannels, kFeatureDim});
  auto base = context.narrow(1, 1, kEdges);
  return torch::cat({base, quote, base - quote}, -1).contiguous();
}

torch::Tensor manual_affine(const torch::Tensor &readout,
                            const mdn::PerEdgeAffineReturnHeadState &state) {
  auto dynamic = readout.narrow(3, 0, kDynamicFeatureDim).to(torch::kFloat64);
  auto standardized =
      (dynamic - state.feature_mean.view({1, 1, 1, kDynamicFeatureDim})) *
      state.feature_inv_std.view({1, 1, 1, kDynamicFeatureDim});
  std::vector<torch::Tensor> outputs;
  for (std::int64_t edge = 0; edge < kEdges; ++edge) {
    outputs.push_back(
        (standardized.select(1, edge) * state.weights.select(0, edge))
            .sum(-1)
            .add(state.bias[edge])
            .unsqueeze(1));
  }
  return torch::cat(outputs, 1);
}

void test_constructor_state_and_input_rejections() {
  auto options = make_options();
  auto state = make_state();

  auto bad_options = options;
  bad_options.feature_dim = 0;
  expect_throw([&] { (void)mdn::PerEdgeAffineReturnHead(bad_options); },
               "zero H rejected");
  bad_options = options;
  bad_options.quote_node_index = 1;
  expect_throw([&] { (void)mdn::PerEdgeAffineReturnHead(bad_options); },
               "nonzero quote index rejected");
  bad_options = options;
  bad_options.readout_feature_dim = kDynamicFeatureDim - 1;
  expect_throw([&] { (void)mdn::PerEdgeAffineReturnHead(bad_options); },
               "F below 3H rejected");

  auto bad_state = make_state();
  bad_state.feature_mean = torch::zeros(
      {kDynamicFeatureDim + 1}, torch::TensorOptions().dtype(torch::kFloat64));
  expect_throw([&] { (void)mdn::PerEdgeAffineReturnHead(options, bad_state); },
               "mean shape rejected");
  bad_state = make_state();
  bad_state.feature_inv_std[0] = 0.0;
  expect_throw([&] { (void)mdn::PerEdgeAffineReturnHead(options, bad_state); },
               "nonpositive inverse standard deviation rejected");
  bad_state = make_state();
  bad_state.weights[0][0] = std::numeric_limits<double>::quiet_NaN();
  expect_throw([&] { (void)mdn::PerEdgeAffineReturnHead(options, bad_state); },
               "nonfinite coefficient rejected");
  bad_state = make_state();
  bad_state.bias = bad_state.bias.to(torch::kFloat32);
  expect_throw([&] { (void)mdn::PerEdgeAffineReturnHead(options, bad_state); },
               "non-float64 state rejected");

  auto head = mdn::PerEdgeAffineReturnHead(options, state);
  auto context = make_context();
  expect_throw([&] { (void)head->forward(context.narrow(1, 0, kNodes - 1)); },
               "wrong context N rejected");
  expect_throw([&] { (void)head->forward(context.narrow(2, 0, 1)); },
               "wrong context C rejected");
  expect_throw([&] { (void)head->forward(context.narrow(3, 0, 4)); },
               "wrong context H rejected");
  expect_throw([&] { (void)head->forward(context.to(torch::kInt64)); },
               "integer context rejected");
  auto nonfinite = context.clone();
  nonfinite[0][0][0][0] = std::numeric_limits<float>::infinity();
  expect_throw([&] { (void)head->forward(nonfinite); },
               "nonfinite context rejected");

  auto features = torch::zeros({kBatch, kEdges, kChannels, kReadoutFeatureDim},
                               torch::TensorOptions().dtype(torch::kFloat32));
  expect_throw(
      [&] { (void)head->forward_readout_features(features.narrow(1, 0, 2)); },
      "wrong readout E rejected");
  expect_throw(
      [&] { (void)head->forward_readout_features(features.narrow(2, 0, 1)); },
      "wrong readout C rejected");
  expect_throw(
      [&] {
        (void)head->forward_readout_features(
            features.narrow(3, 0, kDynamicFeatureDim));
      },
      "wrong exact readout F rejected");

  auto cast_head = mdn::PerEdgeAffineReturnHead(options, state);
  cast_head->to(torch::kFloat32);
  expect_throw([&] { cast_head->validate_registered_state(); },
               "float32 module state rejected");
}

void test_direct_feature_and_affine_parity() {
  torch::manual_seed(20260714);
  auto options = make_options();
  auto state = make_state();
  auto head = mdn::PerEdgeAffineReturnHead(options, state);
  auto context = make_context();

  mdn::DirectEdgeReturnHeadOptions direct_options{};
  direct_options.feature_dim = kFeatureDim;
  direct_options.quote_node_index = 0;
  direct_options.identity_mode = "edge_embedding_per_edge";
  direct_options.base_edge_count = kEdges;
  direct_options.identity_embedding_dim = kIdentityDim;
  direct_options.adapter_hidden_dim = 0;
  auto direct = mdn::DirectEdgeReturnHead(direct_options);
  direct->eval();
  const auto exact_readout = direct->readout_input_features(context);
  check(exact_readout.sizes() ==
            torch::IntArrayRef({kBatch, kEdges, kChannels, kReadoutFeatureDim}),
        "DirectEdgeReturnHead readout shape");
  check_equal(exact_readout.narrow(3, 0, kDynamicFeatureDim),
              manual_dynamic_features(context),
              "DirectEdgeReturnHead exact dynamic prefix");

  const auto context_prediction = head->forward(context);
  const auto readout_prediction = head->forward_readout_features(exact_readout);
  check(context_prediction.sizes() ==
            torch::IntArrayRef({kBatch, kEdges, kChannels}),
        "sidecar prediction shape");
  check(context_prediction.scalar_type() == torch::kFloat32,
        "sidecar matches float32 input dtype");
  check_equal(context_prediction, readout_prediction,
              "context and exact readout parity");

  auto positive_tail = exact_readout.clone();
  auto negative_tail = exact_readout.clone();
  positive_tail.narrow(3, kDynamicFeatureDim, kIdentityDim).fill_(1.0e6);
  negative_tail.narrow(3, kDynamicFeatureDim, kIdentityDim).fill_(-1.0e6);
  check_equal(head->forward_readout_features(positive_tail),
              head->forward_readout_features(negative_tail),
              "identity suffix is explicitly ignored");
  auto changed_active = exact_readout.clone();
  changed_active.select(3, 0).add_(10.0);
  check(
      !head->forward_readout_features(changed_active).equal(readout_prediction),
      "active dynamic prefix affects prediction");

  const auto double_readout = exact_readout.to(torch::kFloat64);
  const auto registered = head->forward_readout_features(double_readout);
  const auto analytic = manual_affine(double_readout, state);
  check(registered.scalar_type() == torch::kFloat64,
        "sidecar matches float64 input dtype");
  check_close(registered, analytic, 1.0e-12,
              "registered linear matches analytic affine formula");
}

void test_registration_archive_digest_and_tamper_rejection() {
  auto options = make_options();
  auto state = make_state();
  auto metadata = make_metadata();
  auto head = mdn::PerEdgeAffineReturnHead(options, state);

  const auto buffers = head->named_buffers(/*recurse=*/true);
  check(buffers.size() == 2 && buffers.contains("feature_mean") &&
            buffers.contains("feature_inv_std"),
        "mean and inverse standard deviation are registered buffers");
  check(buffers["feature_mean"].scalar_type() == torch::kFloat64 &&
            buffers["feature_mean"].device().is_cpu(),
        "registered buffers are float64 CPU");
  const auto parameters = head->named_parameters(/*recurse=*/true);
  check(parameters.size() == static_cast<std::size_t>(2 * kEdges),
        "per-edge weight and bias parameter count");
  for (std::int64_t edge = 0; edge < kEdges; ++edge) {
    const auto prefix = "projection_edge_" + std::to_string(edge);
    check(parameters.contains(prefix + ".weight") &&
              parameters.contains(prefix + ".bias"),
          "deterministic per-edge parameter names");
  }
  for (const auto &parameter : parameters) {
    check(parameter.value().scalar_type() == torch::kFloat64 &&
              parameter.value().device().is_cpu() &&
              !parameter.value().requires_grad(),
          "registered per-edge parameters are frozen float64 CPU");
  }

  const auto digest = mdn::semantic_tensor_digest(head, metadata);
  check(cuwacunu::piaabo::digest::is_sha256_hex(digest),
        "semantic tensor digest is lowercase SHA-256");
  check(digest == mdn::semantic_tensor_digest(head, metadata),
        "semantic tensor digest is deterministic");
  auto changed_state = make_state();
  changed_state.weights[0][0] += 0.5;
  auto changed_head = mdn::PerEdgeAffineReturnHead(options, changed_state);
  check(digest != mdn::semantic_tensor_digest(changed_head, metadata),
        "semantic digest binds coefficient bytes");

  const auto temp_dir = std::filesystem::temp_directory_path() /
                        "cuwacunu_test_wikimyei_mdn_affine_sidecar";
  std::filesystem::remove_all(temp_dir);
  std::filesystem::create_directories(temp_dir);
  const auto artifact = temp_dir / "affine_sidecar.v1.pt";
  mdn::save_per_edge_affine_return_head(artifact, head, metadata);
  auto loaded = mdn::load_per_edge_affine_return_head(artifact);
  check(loaded.metadata.semantic_tensor_digest == digest,
        "loaded metadata carries computed semantic digest");
  check_state_equal(loaded.head->affine_state(), state,
                    "save/load exact registered state");
  auto expected_metadata = metadata;
  expected_metadata.semantic_tensor_digest = digest;
  check(mdn::canonical_metadata_text(loaded.metadata) ==
            mdn::canonical_metadata_text(expected_metadata),
        "canonical metadata text is deterministic");
  const auto canonical = mdn::canonical_metadata_text(loaded.metadata);
  check(canonical.find("semantic_tensor_digest=64:" + digest) !=
            std::string::npos,
        "canonical metadata text includes digest");
  check(canonical.find("evaluation") == std::string::npos &&
            canonical.find("holdout") == std::string::npos,
        "artifact metadata has no evaluation or holdout fields");
  check(loaded.metadata.edge_ids == metadata.edge_ids &&
            loaded.metadata.node_ids == metadata.node_ids &&
            loaded.metadata.graph_order_fingerprint ==
                metadata.graph_order_fingerprint &&
            loaded.metadata.valid_from_anchor == metadata.valid_from_anchor,
        "identity, split, and valid-from metadata round-trip exactly");

  torch::manual_seed(20260714);
  auto context = make_context();
  mdn::DirectEdgeReturnHeadOptions direct_options{};
  direct_options.feature_dim = kFeatureDim;
  direct_options.quote_node_index = 0;
  direct_options.identity_mode = "edge_embedding_per_edge";
  direct_options.base_edge_count = kEdges;
  direct_options.identity_embedding_dim = kIdentityDim;
  auto direct = mdn::DirectEdgeReturnHead(direct_options);
  const auto readout = direct->readout_input_features(context);
  check_equal(loaded.head->forward(context), head->forward(context),
              "save/load exact context prediction");
  check_equal(loaded.head->forward_readout_features(readout),
              head->forward_readout_features(readout),
              "save/load exact readout prediction");

  auto bad_family = loaded.metadata;
  bad_family.artifact_family = "unsupported.affine.sidecar.v2";
  const auto bad_family_path = temp_dir / "bad_family.pt";
  mdn::per_edge_affine_detail::write_artifact_archive_unchecked(
      bad_family_path, head, bad_family,
      mdn::canonical_metadata_text(bad_family));
  expect_throw(
      [&] { (void)mdn::load_per_edge_affine_return_head(bad_family_path); },
      "incompatible artifact family rejected");

  const auto tampered_state_path = temp_dir / "tampered_state.pt";
  mdn::per_edge_affine_detail::write_artifact_archive_unchecked(
      tampered_state_path, changed_head, loaded.metadata, canonical);
  expect_throw(
      [&] { (void)mdn::load_per_edge_affine_return_head(tampered_state_path); },
      "tampered tensor state rejected by semantic digest");

  auto bad_dimensions = loaded.metadata;
  bad_dimensions.readout_feature_dim = kDynamicFeatureDim - 1;
  const auto bad_dimensions_path = temp_dir / "bad_dimensions.pt";
  mdn::per_edge_affine_detail::write_artifact_archive_unchecked(
      bad_dimensions_path, head, bad_dimensions,
      mdn::canonical_metadata_text(bad_dimensions));
  expect_throw(
      [&] { (void)mdn::load_per_edge_affine_return_head(bad_dimensions_path); },
      "incompatible artifact dimensions rejected");

  auto bad_digest_metadata = metadata;
  bad_digest_metadata.semantic_tensor_digest = std::string(64, 'd');
  expect_throw(
      [&] {
        mdn::save_per_edge_affine_return_head(
            temp_dir / "bad_supplied_digest.pt", head, bad_digest_metadata);
      },
      "save rejects incompatible caller-supplied digest");
  std::filesystem::remove_all(temp_dir);
}

} // namespace

int main() {
  try {
    test_constructor_state_and_input_rejections();
    test_direct_feature_and_affine_parity();
    test_registration_archive_digest_and_tamper_rejection();
    std::cout << "[PASS] wikimyei MDN affine sidecar\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] wikimyei MDN affine sidecar: " << error.what() << '\n';
    return 1;
  }
}
