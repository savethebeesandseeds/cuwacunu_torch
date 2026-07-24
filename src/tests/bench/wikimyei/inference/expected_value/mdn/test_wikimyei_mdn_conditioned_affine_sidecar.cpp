#include "wikimyei/inference/expected_value/mdn/per_edge_affine_return_head.h"
#include "wikimyei/inference/expected_value/mdn/per_edge_conditioned_affine_return_head.h"

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
constexpr std::int64_t kChannels = 2;
constexpr std::int64_t kFeatureDim = 7;
constexpr std::int64_t kSuffixDim = 3;
constexpr std::int64_t kReadoutDim = kFeatureDim + kSuffixDim;

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
  const auto actual_cpu = actual.detach().to(torch::kCPU);
  const auto expected_cpu = expected.detach().to(torch::kCPU);
  check(actual_cpu.defined() && expected_cpu.defined() &&
            actual_cpu.equal(expected_cpu),
        message);
}

void check_close(const torch::Tensor &actual, const torch::Tensor &expected,
                 double tolerance, const std::string &message) {
  check(actual.sizes() == expected.sizes(), message + " shape mismatch");
  const auto max_delta = (actual.detach().to(torch::kCPU).to(torch::kFloat64) -
                          expected.detach().to(torch::kCPU).to(torch::kFloat64))
                             .abs()
                             .max()
                             .item<double>();
  check(std::isfinite(max_delta) && max_delta <= tolerance,
        message + " max_delta=" + std::to_string(max_delta));
}

mdn::PerEdgeConditionedAffineReturnHeadOptions make_options() {
  return {
      .feature_dim = kFeatureDim,
      .edge_count = kEdges,
      .channel_count = kChannels,
  };
}

mdn::PerEdgeConditionedAffineReturnHeadState make_state() {
  const auto f32 = torch::TensorOptions().dtype(torch::kFloat32);
  const auto f64 = torch::TensorOptions().dtype(torch::kFloat64);
  auto mean =
      torch::tensor({0.125f, -1.75f, 2.5f, 0.03125f, -0.5f, 8.0f, -3.25f}, f32);
  auto inv_std =
      torch::tensor({1.25f, 0.5f, 2.0f, 4.0f, 0.75f, 0.125f, 1.5f}, f32);
  auto edge_center = torch::arange(kEdges * kFeatureDim, f64)
                         .view({kEdges, kFeatureDim})
                         .mul(0.003)
                         .sub(0.02);
  auto mapped_weights = torch::arange(kEdges * kFeatureDim, f64)
                            .view({kEdges, kFeatureDim})
                            .mul(-0.005)
                            .add(0.04);
  auto centered_bias = torch::tensor({-0.03, 0.02, 0.07}, f64);
  return {
      .feature_mean = std::move(mean),
      .feature_inv_std = std::move(inv_std),
      .edge_center = std::move(edge_center),
      .mapped_weights = std::move(mapped_weights),
      .centered_bias = std::move(centered_bias),
  };
}

torch::Tensor make_readout() {
  return torch::arange(kBatch * kEdges * kChannels * kReadoutDim,
                       torch::TensorOptions().dtype(torch::kFloat32))
      .view({kBatch, kEdges, kChannels, kReadoutDim})
      .mul(0.013f)
      .sub(0.8f);
}

mdn::PerEdgeConditionedAffineReturnHeadArtifactMetadata make_metadata() {
  mdn::PerEdgeConditionedAffineReturnHeadArtifactMetadata metadata;
  metadata.feature_dim = kFeatureDim;
  metadata.edge_count = kEdges;
  metadata.channel_count = kChannels;
  metadata.conditioning_alpha = 1.0e-10;
  metadata.fit_begin = 0;
  metadata.fit_end = 554;
  metadata.purge_begin = 554;
  metadata.purge_end = 584;
  metadata.validation_begin = 584;
  metadata.validation_end = 730;
  metadata.max_anchor = 729;
  metadata.graph_order_fingerprint = "synthetic_graph_order.v1.sha256:feed";
  metadata.edge_ids = {"SYNALPHASYNUSD", "SYNBETASYNUSD", "SYNGAMMASYNUSD"};
  metadata.node_ids = {"SYNUSD", "SYNALPHA", "SYNBETA", "SYNGAMMA"};
  metadata.source_probe_schema_id =
      "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1";
  metadata.source_probe_sha256 = std::string(64, 'a');
  metadata.conditioning_probe_schema_id =
      "synthetic_mdn_frozen_affine_conditioning_parity_v1";
  metadata.conditioning_probe_sha256 = std::string(64, 'b');
  metadata.representation_checkpoint_sha256 = std::string(64, 'c');
  metadata.mdn_checkpoint_sha256 = std::string(64, 'd');
  metadata.conditioning_transform_schema_id =
      "full_rank_damped_conditioning_transform.v1";
  metadata.conditioning_transform_sha256 = std::string(64, 'e');
  return metadata;
}

torch::Tensor
manual_formula(const torch::Tensor &readout,
               const mdn::PerEdgeConditionedAffineReturnHeadState &state) {
  const auto F = state.feature_mean.numel();
  const auto E = state.edge_center.size(0);
  auto prefix_f32 = readout.narrow(3, 0, F);
  auto normalized_f32 = (prefix_f32 - state.feature_mean.view({1, 1, 1, F})) *
                        state.feature_inv_std.view({1, 1, 1, F});
  auto centered_f64 =
      normalized_f32.to(torch::kFloat64) - state.edge_center.view({1, E, 1, F});
  return ((centered_f64 * state.mapped_weights.view({1, E, 1, F})).sum(3) +
          state.centered_bias.view({1, E, 1}))
      .to(torch::kFloat32);
}

void check_state_equal(
    const mdn::PerEdgeConditionedAffineReturnHeadState &actual,
    const mdn::PerEdgeConditionedAffineReturnHeadState &expected,
    const std::string &message) {
  check_equal(actual.feature_mean, expected.feature_mean,
              message + " feature_mean");
  check_equal(actual.feature_inv_std, expected.feature_inv_std,
              message + " feature_inv_std");
  check_equal(actual.edge_center, expected.edge_center,
              message + " edge_center");
  check_equal(actual.mapped_weights, expected.mapped_weights,
              message + " mapped_weights");
  check_equal(actual.centered_bias, expected.centered_bias,
              message + " centered_bias");
}

void check_mixed_buffer_contract(
    const mdn::PerEdgeConditionedAffineReturnHead &head,
    const torch::Device &device, const std::string &message) {
  const auto buffers = head->named_buffers(/*recurse=*/true);
  check(buffers.size() == 5 && buffers.contains("feature_mean") &&
            buffers.contains("feature_inv_std") &&
            buffers.contains("edge_center") &&
            buffers.contains("mapped_weights") &&
            buffers.contains("centered_bias"),
        message + " exact buffer registration");
  check(buffers["feature_mean"].scalar_type() == torch::kFloat32 &&
            buffers["feature_inv_std"].scalar_type() == torch::kFloat32,
        message + " float32 normalization state");
  check(buffers["edge_center"].scalar_type() == torch::kFloat64 &&
            buffers["mapped_weights"].scalar_type() == torch::kFloat64 &&
            buffers["centered_bias"].scalar_type() == torch::kFloat64,
        message + " float64 conditioned affine state");
  for (const auto &buffer : buffers) {
    const auto actual_device = buffer.value().device();
    check(actual_device.type() == device.type() &&
              (!device.has_index() || actual_device.index() == device.index()),
          message + " buffer device preservation");
  }
  check(head->named_parameters(/*recurse=*/true).size() == 0,
        message + " parameter-free invariant");
}

void test_shapes_state_and_finiteness_rejections() {
  auto options = make_options();
  auto state = make_state();

  auto bad_options = options;
  bad_options.feature_dim = 0;
  expect_throw(
      [&] { (void)mdn::PerEdgeConditionedAffineReturnHead(bad_options); },
      "zero F rejected");
  bad_options = options;
  bad_options.edge_count = 0;
  expect_throw(
      [&] { (void)mdn::PerEdgeConditionedAffineReturnHead(bad_options); },
      "zero E rejected");
  bad_options = options;
  bad_options.channel_count = 0;
  expect_throw(
      [&] { (void)mdn::PerEdgeConditionedAffineReturnHead(bad_options); },
      "zero C rejected");

  auto bad_state = state;
  bad_state.feature_mean = torch::zeros(
      {kFeatureDim + 1}, torch::TensorOptions().dtype(torch::kFloat32));
  expect_throw(
      [&] {
        (void)mdn::PerEdgeConditionedAffineReturnHead(options, bad_state);
      },
      "mean shape rejected");
  bad_state = make_state();
  bad_state.feature_inv_std[0] = 0.0f;
  expect_throw(
      [&] {
        (void)mdn::PerEdgeConditionedAffineReturnHead(options, bad_state);
      },
      "nonpositive inverse standard deviation rejected");
  bad_state = make_state();
  bad_state.edge_center = bad_state.edge_center.to(torch::kFloat32);
  expect_throw(
      [&] {
        (void)mdn::PerEdgeConditionedAffineReturnHead(options, bad_state);
      },
      "edge center dtype rejected");
  bad_state = make_state();
  bad_state.mapped_weights[0][0] = std::numeric_limits<double>::quiet_NaN();
  expect_throw(
      [&] {
        (void)mdn::PerEdgeConditionedAffineReturnHead(options, bad_state);
      },
      "nonfinite mapped weight rejected");

  auto head = mdn::PerEdgeConditionedAffineReturnHead(options, state);
  auto readout = make_readout();
  expect_throw(
      [&] { (void)head->forward_readout_features(readout.narrow(0, 0, 0)); },
      "empty batch rejected");
  expect_throw(
      [&] {
        (void)head->forward_readout_features(readout.narrow(1, 0, kEdges - 1));
      },
      "wrong E rejected");
  expect_throw(
      [&] { (void)head->forward_readout_features(readout.narrow(2, 0, 1)); },
      "wrong C rejected");
  expect_throw(
      [&] {
        (void)head->forward_readout_features(
            readout.narrow(3, 0, kFeatureDim - 1));
      },
      "D below F rejected");
  expect_throw(
      [&] { (void)head->forward_readout_features(readout.select(0, 0)); },
      "wrong rank rejected");
  expect_throw(
      [&] {
        (void)head->forward_readout_features(readout.to(torch::kFloat64));
      },
      "float64 input rejected");
  auto nonfinite = readout.clone();
  nonfinite[0][0][0][0] = std::numeric_limits<float>::infinity();
  expect_throw([&] { (void)head->forward_readout_features(nonfinite); },
               "nonfinite prefix input rejected");

  auto normalization_overflow_state = make_state();
  normalization_overflow_state.feature_mean.zero_();
  normalization_overflow_state.feature_inv_std.fill_(
      std::numeric_limits<float>::max());
  auto normalization_overflow_head = mdn::PerEdgeConditionedAffineReturnHead(
      options, normalization_overflow_state);
  auto normalization_overflow_input = torch::zeros_like(readout);
  normalization_overflow_input.narrow(3, 0, kFeatureDim).fill_(2.0f);
  expect_throw(
      [&] {
        (void)normalization_overflow_head->forward_readout_features(
            normalization_overflow_input);
      },
      "float32 normalization overflow rejected");

  auto accumulation_overflow_state = make_state();
  accumulation_overflow_state.feature_mean.zero_();
  accumulation_overflow_state.feature_inv_std.fill_(1.0f);
  accumulation_overflow_state.edge_center.zero_();
  accumulation_overflow_state.mapped_weights.fill_(
      std::numeric_limits<double>::max());
  accumulation_overflow_state.centered_bias.zero_();
  auto accumulation_overflow_head = mdn::PerEdgeConditionedAffineReturnHead(
      options, accumulation_overflow_state);
  auto unit_prefix_input = torch::zeros_like(readout);
  unit_prefix_input.narrow(3, 0, kFeatureDim).fill_(1.0f);
  expect_throw(
      [&] {
        (void)accumulation_overflow_head->forward_readout_features(
            unit_prefix_input);
      },
      "float64 affine accumulation overflow rejected");

  auto cast_overflow_state = accumulation_overflow_state;
  cast_overflow_state.mapped_weights.fill_(1.0e100);
  auto cast_overflow_head =
      mdn::PerEdgeConditionedAffineReturnHead(options, cast_overflow_state);
  expect_throw(
      [&] {
        (void)cast_overflow_head->forward_readout_features(unit_prefix_input);
      },
      "float32 output cast overflow rejected");
}

void test_exact_operation_order_manual_formula_and_suffix() {
  auto options = make_options();
  auto state = make_state();
  auto head = mdn::PerEdgeConditionedAffineReturnHead(options, state);
  auto readout = make_readout();

  const auto actual = head->forward_readout_features(readout);
  const auto expected = manual_formula(readout, state);
  check(actual.sizes() == torch::IntArrayRef({kBatch, kEdges, kChannels}),
        "output shape is [B,E,C]");
  check(actual.scalar_type() == torch::kFloat32,
        "output dtype is always float32");
  check(torch::isfinite(actual).all().item<bool>(), "output is finite");
  check_equal(actual, expected, "exact manual mixed-dtype formula");

  auto positive_suffix = readout.clone();
  auto negative_suffix = readout.clone();
  positive_suffix.narrow(3, kFeatureDim, kSuffixDim).fill_(1.0e20f);
  negative_suffix.narrow(3, kFeatureDim, kSuffixDim).fill_(-1.0e20f);
  check_equal(head->forward_readout_features(positive_suffix),
              head->forward_readout_features(negative_suffix),
              "suffix after F is ignored exactly");
  auto nonfinite_suffix = readout.clone();
  nonfinite_suffix[0][0][0][kFeatureDim] =
      std::numeric_limits<float>::infinity();
  nonfinite_suffix[0][0][0][kFeatureDim + 1] =
      std::numeric_limits<float>::quiet_NaN();
  check_equal(head->forward_readout_features(nonfinite_suffix), actual,
              "nonfinite suffix after F is ignored exactly");
  check_equal(head->forward_readout_features(readout.narrow(3, 0, kFeatureDim)),
              actual, "D equal to F and D greater than F agree");

  const auto f32 = torch::TensorOptions().dtype(torch::kFloat32);
  const auto f64 = torch::TensorOptions().dtype(torch::kFloat64);
  mdn::PerEdgeConditionedAffineReturnHeadOptions order_options{
      .feature_dim = 1,
      .edge_count = 1,
      .channel_count = 1,
  };
  mdn::PerEdgeConditionedAffineReturnHeadState order_state{
      .feature_mean = torch::tensor({1.0f}, f32),
      .feature_inv_std = torch::tensor({1.0f}, f32),
      .edge_center = torch::tensor({{100000000.0}}, f64),
      .mapped_weights = torch::tensor({{1.0}}, f64),
      .centered_bias = torch::tensor({0.0}, f64),
  };
  auto order_head =
      mdn::PerEdgeConditionedAffineReturnHead(order_options, order_state);
  auto order_input = torch::tensor({{{{100000000.0f, 17.0f}}}}, f32);
  const auto ordered = order_head->forward_readout_features(order_input);
  check_equal(ordered, torch::zeros({1, 1, 1}, f32),
              "normalization executes before float64 promotion");
  auto promote_first =
      ((order_input.narrow(3, 0, 1).to(torch::kFloat64) -
        order_state.feature_mean.to(torch::kFloat64).view({1, 1, 1, 1})) *
           order_state.feature_inv_std.to(torch::kFloat64).view({1, 1, 1, 1}) -
       order_state.edge_center.view({1, 1, 1, 1}))
          .sum(3)
          .to(torch::kFloat32);
  check(!ordered.equal(promote_first),
        "operation-order fixture distinguishes promote-before-normalize");
}

void test_buffer_registration_and_device_dtype_preservation() {
  auto options = make_options();
  auto state = make_state();
  auto metadata = make_metadata();
  auto head = mdn::PerEdgeConditionedAffineReturnHead(options, state);
  check_mixed_buffer_contract(head, torch::Device(torch::kCPU), "CPU");
  const auto cpu_prediction = head->forward_readout_features(make_readout());
  const auto cpu_digest =
      mdn::conditioned_affine_semantic_tensor_digest(head, metadata);

  head->to(torch::Device(torch::kCPU));
  head->validate_registered_state();
  check_mixed_buffer_contract(head, torch::Device(torch::kCPU),
                              "CPU device-only move");
  check(cpu_digest ==
            mdn::conditioned_affine_semantic_tensor_digest(head, metadata),
        "CPU device-only move preserves exact tensor bytes");

  if (torch::cuda::is_available()) {
    head->to(torch::Device(torch::kCUDA));
    head->validate_registered_state();
    check_mixed_buffer_contract(head, torch::Device(torch::kCUDA),
                                "CUDA device-only move");
    expect_throw([&] { (void)head->forward_readout_features(make_readout()); },
                 "CPU input rejected for CUDA buffers");
    const auto cuda_prediction =
        head->forward_readout_features(make_readout().to(torch::kCUDA));
    check(cuda_prediction.device().is_cuda(), "CUDA output remains on CUDA");
    check(cuda_prediction.scalar_type() == torch::kFloat32,
          "CUDA output remains float32");
    check_close(cuda_prediction, cpu_prediction, 2.0e-5,
                "CPU/CUDA mixed-dtype parity");
    check(cpu_digest ==
              mdn::conditioned_affine_semantic_tensor_digest(head, metadata),
          "CUDA device-only move preserves exact tensor bytes");
    head->to(torch::Device(torch::kCPU));
    head->validate_registered_state();
    check_mixed_buffer_contract(head, torch::Device(torch::kCPU),
                                "CUDA-to-CPU device-only move");
    check_equal(head->forward_readout_features(make_readout()), cpu_prediction,
                "CUDA round trip preserves CPU prediction exactly");
  }
}

mdn::PerEdgeAffineReturnHeadArtifactMetadata make_v1_metadata() {
  mdn::PerEdgeAffineReturnHeadArtifactMetadata metadata;
  metadata.feature_dim = 2;
  metadata.edge_count = kEdges;
  metadata.readout_feature_dim = 6;
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
  metadata.edge_ids = {"SYNALPHASYNUSD", "SYNBETASYNUSD", "SYNGAMMASYNUSD"};
  metadata.node_ids = {"SYNUSD", "SYNALPHA", "SYNBETA", "SYNGAMMA"};
  metadata.fit_probe_schema_id = "v1_probe";
  metadata.fit_probe_sha256 = std::string(64, 'a');
  metadata.representation_checkpoint_sha256 = std::string(64, 'b');
  metadata.mdn_checkpoint_sha256 = std::string(64, 'c');
  metadata.legacy_capture_authority = "legacy_capture.v1";
  return metadata;
}

void test_archive_digest_tamper_split_and_cross_version_rejection() {
  auto options = make_options();
  auto state = make_state();
  auto metadata = make_metadata();
  auto head = mdn::PerEdgeConditionedAffineReturnHead(options, state);

  const auto digest =
      mdn::conditioned_affine_semantic_tensor_digest(head, metadata);
  check(cuwacunu::piaabo::digest::is_sha256_hex(digest),
        "semantic tensor digest is lowercase SHA-256");
  check(digest ==
            mdn::conditioned_affine_semantic_tensor_digest(head, metadata),
        "semantic tensor digest is deterministic");
  auto changed_metadata_binding = metadata;
  changed_metadata_binding.graph_order_fingerprint =
      "synthetic_graph_order.v1.sha256:changed";
  check(digest != mdn::conditioned_affine_semantic_tensor_digest(
                      head, changed_metadata_binding),
        "semantic digest binds canonical metadata bytes");
  auto changed_state = make_state();
  changed_state.feature_mean[0] =
      std::nextafter(changed_state.feature_mean[0].item<float>(), 1.0f);
  auto changed_normalization_head =
      mdn::PerEdgeConditionedAffineReturnHead(options, changed_state);
  check(digest != mdn::conditioned_affine_semantic_tensor_digest(
                      changed_normalization_head, metadata),
        "semantic digest binds exact float32 normalization bytes");
  changed_state = make_state();
  changed_state.mapped_weights[0][0] += 0.5;
  auto changed_affine_head =
      mdn::PerEdgeConditionedAffineReturnHead(options, changed_state);
  check(digest != mdn::conditioned_affine_semantic_tensor_digest(
                      changed_affine_head, metadata),
        "semantic digest binds exact float64 affine bytes");

  const auto canonical_without_digest =
      mdn::canonical_conditioned_affine_metadata_text(metadata);
  check(canonical_without_digest.find("refit") == std::string::npos &&
            canonical_without_digest.find("holdout") == std::string::npos &&
            canonical_without_digest.find("evaluation") == std::string::npos,
        "v2 metadata has no refit, holdout, or evaluation fields");

  const auto temp_dir = std::filesystem::temp_directory_path() /
                        "cuwacunu_test_wikimyei_mdn_conditioned_affine";
  std::filesystem::remove_all(temp_dir);
  std::filesystem::create_directories(temp_dir);
  const auto artifact = temp_dir / "conditioned_affine_sidecar.v2.pt";
  mdn::save_per_edge_conditioned_affine_return_head(artifact, head, metadata);
  auto loaded = mdn::load_per_edge_conditioned_affine_return_head(artifact);
  check(loaded.metadata.semantic_tensor_digest == digest,
        "load returns stored semantic digest");
  check(loaded.metadata.run_only && !loaded.metadata.policy_eligible,
        "run-only and policy-ineligible metadata round-trip");
  check(loaded.metadata.max_anchor == 729 && loaded.metadata.fit_end == 554 &&
            loaded.metadata.purge_end == 584 &&
            loaded.metadata.validation_end == 730,
        "development split metadata round-trips");
  check(loaded.metadata.representation_checkpoint_sha256 ==
                metadata.representation_checkpoint_sha256 &&
            loaded.metadata.mdn_checkpoint_sha256 ==
                metadata.mdn_checkpoint_sha256 &&
            loaded.metadata.conditioning_probe_sha256 ==
                metadata.conditioning_probe_sha256 &&
            loaded.metadata.conditioning_transform_sha256 ==
                metadata.conditioning_transform_sha256,
        "checkpoint, probe, and transform bindings round-trip");
  check_state_equal(loaded.head->conditioned_affine_state(), state,
                    "archive exact mixed-dtype state");
  check_equal(loaded.head->forward_readout_features(make_readout()),
              head->forward_readout_features(make_readout()),
              "archive exact prediction");
  check_mixed_buffer_contract(loaded.head, torch::Device(torch::kCPU),
                              "archive reload");

  auto supplied_bad_digest = metadata;
  supplied_bad_digest.semantic_tensor_digest = std::string(64, 'f');
  expect_throw(
      [&] {
        mdn::save_per_edge_conditioned_affine_return_head(
            temp_dir / "bad_supplied_digest.pt", head, supplied_bad_digest);
      },
      "save rejects caller-supplied digest mismatch");

  const auto stored_canonical =
      mdn::canonical_conditioned_affine_metadata_text(loaded.metadata);
  const auto tampered_state_path = temp_dir / "tampered_state.pt";
  mdn::per_edge_conditioned_affine_detail::write_artifact_archive_unchecked(
      tampered_state_path, changed_affine_head, loaded.metadata,
      stored_canonical);
  expect_throw(
      [&] {
        (void)mdn::load_per_edge_conditioned_affine_return_head(
            tampered_state_path);
      },
      "load rejects tampered state bytes");

  auto tampered_metadata = loaded.metadata;
  tampered_metadata.graph_order_fingerprint = "tampered_graph_order";
  const auto tampered_metadata_path = temp_dir / "tampered_metadata.pt";
  mdn::per_edge_conditioned_affine_detail::write_artifact_archive_unchecked(
      tampered_metadata_path, head, tampered_metadata, stored_canonical);
  expect_throw(
      [&] {
        (void)mdn::load_per_edge_conditioned_affine_return_head(
            tampered_metadata_path);
      },
      "load rejects tampered canonical metadata");

  auto bad_split = metadata;
  bad_split.max_anchor = 730;
  expect_throw(
      [&] {
        mdn::save_per_edge_conditioned_affine_return_head(
            temp_dir / "bad_max_anchor.pt", head, bad_split);
      },
      "max anchor must equal validation_end minus one");
  bad_split = metadata;
  bad_split.purge_begin = 555;
  expect_throw(
      [&] {
        mdn::save_per_edge_conditioned_affine_return_head(
            temp_dir / "bad_fit_purge_boundary.pt", head, bad_split);
      },
      "fit and purge boundary mismatch rejected");
  bad_split = metadata;
  bad_split.validation_begin = 585;
  expect_throw(
      [&] {
        mdn::save_per_edge_conditioned_affine_return_head(
            temp_dir / "bad_purge_validation_boundary.pt", head, bad_split);
      },
      "purge and validation boundary mismatch rejected");
  auto bad_policy = metadata;
  bad_policy.policy_eligible = true;
  expect_throw(
      [&] {
        mdn::save_per_edge_conditioned_affine_return_head(
            temp_dir / "bad_policy.pt", head, bad_policy);
      },
      "policy-eligible artifact rejected");

  mdn::PerEdgeAffineReturnHeadOptions v1_options{
      .feature_dim = 2,
      .edge_count = kEdges,
      .readout_feature_dim = 6,
      .channel_count = kChannels,
      .quote_node_index = 0,
  };
  const auto f64 = torch::TensorOptions().dtype(torch::kFloat64);
  mdn::PerEdgeAffineReturnHeadState v1_state{
      .feature_mean = torch::zeros({6}, f64),
      .feature_inv_std = torch::ones({6}, f64),
      .weights = torch::zeros({kEdges, 6}, f64),
      .bias = torch::zeros({kEdges}, f64),
  };
  auto v1_head = mdn::PerEdgeAffineReturnHead(v1_options, v1_state);
  const auto v1_artifact = temp_dir / "affine_sidecar.v1.pt";
  mdn::save_per_edge_affine_return_head(v1_artifact, v1_head,
                                        make_v1_metadata());
  expect_throw(
      [&] {
        (void)mdn::load_per_edge_conditioned_affine_return_head(v1_artifact);
      },
      "v2 loader rejects v1 artifact");
  expect_throw([&] { (void)mdn::load_per_edge_affine_return_head(artifact); },
               "v1 loader rejects v2 artifact");

  if (torch::cuda::is_available()) {
    auto cuda_head = mdn::PerEdgeConditionedAffineReturnHead(options, state);
    cuda_head->to(torch::Device(torch::kCUDA));
    const auto cuda_artifact = temp_dir / "conditioned_affine_cuda.v2.pt";
    mdn::save_per_edge_conditioned_affine_return_head(cuda_artifact, cuda_head,
                                                      metadata);
    auto cuda_loaded =
        mdn::load_per_edge_conditioned_affine_return_head(cuda_artifact);
    check_mixed_buffer_contract(cuda_loaded.head, torch::Device(torch::kCPU),
                                "CUDA archive CPU reload");
    check_state_equal(cuda_loaded.head->conditioned_affine_state(), state,
                      "CUDA archive reload exact state");
    check_equal(cuda_loaded.head->forward_readout_features(make_readout()),
                head->forward_readout_features(make_readout()),
                "CUDA archive reload exact CPU prediction");
  }

  std::filesystem::remove_all(temp_dir);
}

} // namespace

int main() {
  try {
    test_shapes_state_and_finiteness_rejections();
    test_exact_operation_order_manual_formula_and_suffix();
    test_buffer_registration_and_device_dtype_preservation();
    test_archive_digest_tamper_split_and_cross_version_rejection();
    std::cout << "[PASS] wikimyei MDN conditioned affine sidecar\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] wikimyei MDN conditioned affine sidecar: "
              << error.what() << '\n';
    return 1;
  }
}
