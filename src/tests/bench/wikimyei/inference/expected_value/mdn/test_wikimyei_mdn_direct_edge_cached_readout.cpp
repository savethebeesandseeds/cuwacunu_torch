#include "wikimyei/inference/expected_value/mdn/mixture_density_network_head.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;

namespace {

constexpr std::int64_t kBatch = 2;
constexpr std::int64_t kEdges = 3;
constexpr std::int64_t kNodes = kEdges + 1;
constexpr std::int64_t kChannels = 2;
constexpr std::int64_t kFeatureDim = 5;
constexpr std::int64_t kDynamicReadoutDim = 3 * kFeatureDim;
constexpr std::int64_t kIdentityDim = 4;
constexpr std::int64_t kAdapterHiddenDim = 7;

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
  check(actual.defined() && expected.defined(), message + " undefined tensor");
  const auto actual_cpu = actual.detach().to(torch::kCPU);
  const auto expected_cpu = expected.detach().to(torch::kCPU);
  check(actual_cpu.equal(expected_cpu), message);
}

void check_not_equal(const torch::Tensor &actual, const torch::Tensor &expected,
                     const std::string &message) {
  const auto actual_cpu = actual.detach().to(torch::kCPU);
  const auto expected_cpu = expected.detach().to(torch::kCPU);
  check(!actual_cpu.equal(expected_cpu), message);
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

mdn::DirectEdgeReturnHeadOptions
make_options(const std::string &identity_mode) {
  const bool embedded = identity_mode == "edge_embedding_per_edge";
  const bool per_edge =
      identity_mode == "per_edge" || identity_mode == "edge_embedding_per_edge";
  return {
      .feature_dim = kFeatureDim,
      .quote_node_index = 0,
      .identity_mode = identity_mode,
      .base_edge_count = per_edge ? kEdges : 0,
      .identity_embedding_dim = embedded ? kIdentityDim : 0,
      .adapter_hidden_dim = kAdapterHiddenDim,
  };
}

void initialize_deterministic_state(const mdn::DirectEdgeReturnHead &head) {
  torch::NoGradGuard no_grad;
  std::int64_t ordinal = 1;
  for (auto &item : head->named_parameters(/*recurse=*/true)) {
    auto &parameter = item.value();
    auto values = torch::arange(parameter.numel(), parameter.options())
                      .reshape(parameter.sizes())
                      .mul(0.001)
                      .add(static_cast<double>(ordinal) * 0.01);
    parameter.copy_(values);
    ++ordinal;
  }
}

mdn::DirectEdgeReturnHead make_head(const std::string &identity_mode) {
  torch::manual_seed(907);
  auto head = mdn::DirectEdgeReturnHead(make_options(identity_mode));
  initialize_deterministic_state(head);
  head->eval();
  return head;
}

torch::Tensor make_context() {
  return torch::arange(kBatch * kNodes * kChannels * kFeatureDim,
                       torch::TensorOptions().dtype(torch::kFloat32))
      .view({kBatch, kNodes, kChannels, kFeatureDim})
      .mul(0.017f)
      .sub(0.9f);
}

using module_state_t = std::map<std::string, torch::Tensor>;

module_state_t snapshot_module_state(const mdn::DirectEdgeReturnHead &head) {
  module_state_t state;
  for (const auto &item : head->named_parameters(/*recurse=*/true)) {
    state.emplace("parameter/" + item.key(),
                  item.value().detach().to(torch::kCPU).clone());
  }
  for (const auto &item : head->named_buffers(/*recurse=*/true)) {
    state.emplace("buffer/" + item.key(),
                  item.value().detach().to(torch::kCPU).clone());
  }
  return state;
}

void check_module_state_equal(const module_state_t &actual,
                              const module_state_t &expected,
                              const std::string &message) {
  check(actual.size() == expected.size(), message + " state size mismatch");
  for (const auto &[name, expected_tensor] : expected) {
    const auto found = actual.find(name);
    check(found != actual.end(), message + " missing " + name);
    check_equal(found->second, expected_tensor, message + " changed " + name);
  }
}

torch::Tensor legacy_readout_forward(mdn::DirectEdgeReturnHead &head,
                                     const torch::Tensor &readout_features) {
  const auto B = readout_features.size(0);
  const auto E = readout_features.size(1);
  const auto C = readout_features.size(2);
  auto x = readout_features.contiguous().view({B * E * C, -1});
  auto z = torch::silu(head->hidden->forward(head->input_norm->forward(x)))
               .view({B, E, C, head->H});
  if (!head->uses_per_edge_projection) {
    return head->projection->forward(z.view({B * E * C, head->H}))
        .view({B, E, C});
  }
  using torch::indexing::Slice;
  std::vector<torch::Tensor> outputs;
  outputs.reserve(static_cast<std::size_t>(E));
  for (std::int64_t edge = 0; edge < E; ++edge) {
    auto edge_hidden = z.index({Slice(), edge, Slice(), Slice()})
                           .contiguous()
                           .view({B * C, head->H});
    outputs.push_back(head->edge_projections[static_cast<std::size_t>(edge)]
                          ->forward(edge_hidden)
                          .view({B, 1, C}));
  }
  return torch::cat(outputs, /*dim=*/1);
}

torch::Tensor exercise_exact_cached_boundary(mdn::DirectEdgeReturnHead &head,
                                             const torch::Tensor &context,
                                             const std::string &identity_mode,
                                             const std::string &device_label) {
  const auto state_before = snapshot_module_state(head);
  torch::NoGradGuard no_grad;
  const auto readout = head->readout_input_features(context);
  const auto cached_prediction = head->forward_readout_features(readout);
  const auto context_prediction = head->forward(context);
  const auto legacy_prediction = legacy_readout_forward(head, readout);

  const auto expected_width =
      kDynamicReadoutDim +
      (identity_mode == "edge_embedding_per_edge" ? kIdentityDim : 0);
  check(readout.sizes() ==
            torch::IntArrayRef({kBatch, kEdges, kChannels, expected_width}),
        device_label + " " + identity_mode + " cached readout shape");
  check(cached_prediction.sizes() ==
            torch::IntArrayRef({kBatch, kEdges, kChannels}),
        device_label + " " + identity_mode + " prediction shape");
  check(cached_prediction.device() == context.device(),
        device_label + " " + identity_mode + " output device");
  check(cached_prediction.scalar_type() == context.scalar_type(),
        device_label + " " + identity_mode + " output dtype");
  check(torch::isfinite(cached_prediction).all().item<bool>(),
        device_label + " " + identity_mode + " finite output");
  check_equal(cached_prediction, context_prediction,
              device_label + " " + identity_mode +
                  " context and cached paths are torch-exact");
  check_equal(cached_prediction, legacy_prediction,
              device_label + " " + identity_mode +
                  " cached path preserves legacy arithmetic exactly");

  const auto state_after = snapshot_module_state(head);
  check_module_state_equal(state_after, state_before,
                           device_label + " " + identity_mode);
  return cached_prediction.detach().to(torch::kCPU);
}

module_state_t
snapshot_parameter_gradients(const mdn::DirectEdgeReturnHead &head) {
  module_state_t gradients;
  for (const auto &item : head->named_parameters(/*recurse=*/true)) {
    check(item.value().grad().defined(),
          "missing parameter gradient for " + item.key());
    gradients.emplace(item.key(),
                      item.value().grad().detach().to(torch::kCPU).clone());
  }
  return gradients;
}

void test_training_gradient_preservation() {
  for (const auto &mode :
       {std::string("shared"), std::string("edge_embedding_per_edge")}) {
    auto legacy_head = make_head(mode);
    auto delegated_head = make_head(mode);
    legacy_head->train();
    delegated_head->train();
    auto legacy_context = make_context().clone().detach();
    auto delegated_context = make_context().clone().detach();
    legacy_context.set_requires_grad(true);
    delegated_context.set_requires_grad(true);

    const auto legacy_readout =
        legacy_head->readout_input_features(legacy_context);
    const auto legacy_prediction =
        legacy_readout_forward(legacy_head, legacy_readout);
    const auto delegated_prediction =
        delegated_head->forward(delegated_context);
    check_equal(delegated_prediction, legacy_prediction,
                mode + " training forward preserves legacy arithmetic");

    auto loss_weights =
        torch::arange(legacy_prediction.numel(), legacy_prediction.options())
            .view(legacy_prediction.sizes())
            .mul(0.013)
            .add(0.25);
    (legacy_prediction * loss_weights).sum().backward();
    (delegated_prediction * loss_weights).sum().backward();
    check_equal(delegated_context.grad(), legacy_context.grad(),
                mode + " context gradients are torch-exact");
    check_module_state_equal(snapshot_parameter_gradients(delegated_head),
                             snapshot_parameter_gradients(legacy_head),
                             mode + " parameter gradients");
  }
}

void test_width_semantics_and_malformed_inputs() {
  auto shared = make_head("shared");
  auto per_edge = make_head("per_edge");
  auto embedded = make_head("edge_embedding_per_edge");
  auto context = make_context();
  torch::NoGradGuard no_grad;
  auto shared_readout = shared->readout_input_features(context);
  auto embedded_readout = embedded->readout_input_features(context);

  check(shared_readout.size(3) == kDynamicReadoutDim,
        "shared readout has exactly 3H features");
  check(embedded_readout.size(3) == kDynamicReadoutDim + kIdentityDim,
        "embedded readout has exactly 3H plus identity features");

  auto expected_identity =
      embedded->edge_embedding
          ->forward(torch::arange(kEdges,
                                  torch::TensorOptions().dtype(torch::kInt64)))
          .view({1, kEdges, 1, kIdentityDim})
          .expand({kBatch, kEdges, kChannels, kIdentityDim});
  check_equal(embedded_readout.narrow(3, kDynamicReadoutDim, kIdentityDim),
              expected_identity, "cached identity suffix is exact");

  const auto shared_prediction =
      shared->forward_readout_features(shared_readout);
  auto changed_dynamic = shared_readout.clone();
  changed_dynamic[0][0][0][0].add_(0.75f);
  check_not_equal(shared->forward_readout_features(changed_dynamic),
                  shared_prediction, "shared dynamic dimensions are used");

  const auto embedded_prediction =
      embedded->forward_readout_features(embedded_readout);
  auto changed_identity = embedded_readout.clone();
  changed_identity[0][0][0][kDynamicReadoutDim].add_(0.75f);
  check_not_equal(embedded->forward_readout_features(changed_identity),
                  embedded_prediction,
                  "production identity suffix participates in readout");

  expect_throw([&] { (void)shared->forward_readout_features(torch::Tensor{}); },
               "undefined readout rejected");
  expect_throw(
      [&] {
        (void)shared->forward_readout_features(shared_readout.select(0, 0));
      },
      "rank-three readout rejected");
  expect_throw(
      [&] {
        (void)shared->forward_readout_features(shared_readout.narrow(0, 0, 0));
      },
      "empty batch rejected");
  expect_throw(
      [&] {
        (void)shared->forward_readout_features(shared_readout.narrow(1, 0, 0));
      },
      "empty edge axis rejected");
  expect_throw(
      [&] {
        (void)shared->forward_readout_features(shared_readout.narrow(2, 0, 0));
      },
      "empty channel axis rejected");
  expect_throw(
      [&] {
        (void)shared->forward_readout_features(
            shared_readout.narrow(3, 0, kDynamicReadoutDim - 1));
      },
      "short readout width rejected");
  auto shared_with_suffix =
      torch::cat({shared_readout, torch::zeros({kBatch, kEdges, kChannels, 1},
                                               shared_readout.options())},
                 /*dim=*/3);
  expect_throw(
      [&] { (void)shared->forward_readout_features(shared_with_suffix); },
      "unexpected shared suffix rejected");
  expect_throw(
      [&] {
        (void)embedded->forward_readout_features(
            embedded_readout.narrow(3, 0, embedded_readout.size(3) - 1));
      },
      "missing identity feature rejected");
  expect_throw(
      [&] {
        (void)shared->forward_readout_features(
            shared_readout.to(torch::kInt64));
      },
      "integer readout rejected");
  expect_throw(
      [&] {
        (void)shared->forward_readout_features(
            shared_readout.to(torch::kFloat64));
      },
      "mismatched floating dtype rejected");

  auto nonfinite_dynamic = shared_readout.clone();
  nonfinite_dynamic[0][0][0][0] = std::numeric_limits<float>::quiet_NaN();
  expect_throw(
      [&] { (void)shared->forward_readout_features(nonfinite_dynamic); },
      "nonfinite dynamic feature rejected");
  auto nonfinite_identity = embedded_readout.clone();
  nonfinite_identity[0][0][0][kDynamicReadoutDim] =
      std::numeric_limits<float>::infinity();
  expect_throw(
      [&] { (void)embedded->forward_readout_features(nonfinite_identity); },
      "nonfinite identity feature rejected");

  auto too_many_edges = torch::cat(
      {embedded_readout, embedded_readout.narrow(1, 0, 1)}, /*dim=*/1);
  expect_throw(
      [&] { (void)embedded->forward_readout_features(too_many_edges); },
      "edge count beyond registered per-edge projections rejected");
  auto too_many_nodes =
      torch::cat({context, context.narrow(1, 0, 1)}, /*dim=*/1);
  expect_throw([&] { (void)per_edge->forward(too_many_nodes); },
               "normal per-edge context path preserves edge overflow check");
  auto edge_prefix = embedded_readout.narrow(1, 0, kEdges - 1);
  auto prefix_prediction = embedded->forward_readout_features(edge_prefix);
  check(prefix_prediction.sizes() ==
            torch::IntArrayRef({kBatch, kEdges - 1, kChannels}),
        "registered per-edge prefix is supported");
  check_equal(prefix_prediction, legacy_readout_forward(embedded, edge_prefix),
              "registered per-edge prefix preserves legacy arithmetic");

  auto padded =
      torch::cat({shared_readout, torch::zeros({kBatch, kEdges, kChannels, 1},
                                               shared_readout.options())},
                 /*dim=*/3);
  auto noncontiguous = padded.narrow(3, 0, kDynamicReadoutDim);
  check(!noncontiguous.is_contiguous(),
        "test fixture is a noncontiguous exact-width view");
  check_equal(shared->forward_readout_features(noncontiguous),
              shared_prediction, "noncontiguous cached readout is supported");

  auto overflow = make_head("shared");
  auto overflow_readout = overflow->readout_input_features(context);
  overflow->hidden->weight.zero_();
  overflow->hidden->bias.fill_(1.0f);
  overflow->projection->weight.fill_(std::numeric_limits<float>::max());
  overflow->projection->bias.zero_();
  expect_throw(
      [&] { (void)overflow->forward_readout_features(overflow_readout); },
      "finite inputs and parameters that overflow output are rejected");
  const auto unchecked_hot_path_output = overflow->forward(context);
  check(!torch::isfinite(unchecked_hot_path_output).all().item<bool>(),
        "normal context forward preserves legacy nonfinite propagation");
}

void test_matching_float64_dtype_is_supported() {
  auto head = make_head("shared");
  head->to(torch::kFloat64);
  auto context = make_context().to(torch::kFloat64);
  const auto prediction =
      exercise_exact_cached_boundary(head, context, "shared", "CPU float64");
  check(prediction.scalar_type() == torch::kFloat64,
        "float64 head returns float64 output");
}

void test_cpu_and_cuda_paths() {
  auto context = make_context();
  for (const auto &mode :
       {std::string("shared"), std::string("edge_embedding_per_edge")}) {
    auto cpu_head = make_head(mode);
    const auto cpu_prediction =
        exercise_exact_cached_boundary(cpu_head, context, mode, "CPU");

    if (!torch::cuda::is_available()) {
      std::cout << "CUDA unavailable; skipped " << mode << " CUDA checks\n";
      continue;
    }

    auto cuda_head = make_head(mode);
    cuda_head->to(torch::Device(torch::kCUDA));
    auto cuda_context = context.to(torch::Device(torch::kCUDA));
    const auto cuda_prediction =
        exercise_exact_cached_boundary(cuda_head, cuda_context, mode, "CUDA");
    check_close(cuda_prediction, cpu_prediction, 2.0e-5,
                mode + " CPU/CUDA prediction parity");

    auto cpu_readout = cpu_head->readout_input_features(context);
    auto cuda_readout = cuda_head->readout_input_features(cuda_context);
    expect_throw(
        [&] { (void)cuda_head->forward_readout_features(cpu_readout); },
        mode + " CPU readout rejected by CUDA head");
    expect_throw(
        [&] { (void)cpu_head->forward_readout_features(cuda_readout); },
        mode + " CUDA readout rejected by CPU head");
  }
}

} // namespace

int main() {
  try {
    test_width_semantics_and_malformed_inputs();
    test_matching_float64_dtype_is_supported();
    test_training_gradient_preservation();
    test_cpu_and_cuda_paths();
    std::cout << "test_wikimyei_mdn_direct_edge_cached_readout: PASS\n";
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "torch error: " << error.what() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "error: " << error.what() << '\n';
  }
  return 1;
}
