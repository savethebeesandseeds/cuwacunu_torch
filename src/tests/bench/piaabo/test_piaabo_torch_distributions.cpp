#include "piaabo/tensor/torch/distributions/beta.h"
#include "piaabo/tensor/torch/distributions/categorical.h"
#include "piaabo/tensor/torch/distributions/gamma.h"
#include "piaabo/tensor/torch/torch_utils.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace dist = cuwacunu::piaabo::tensor::torch::distributions;
namespace ptorch = cuwacunu::piaabo::tensor::torch;

namespace {

void assert_shape(const ::torch::Tensor &tensor,
                  std::initializer_list<int64_t> expected) {
  assert(tensor.sizes().vec() == std::vector<int64_t>(expected));
}

void assert_close(const ::torch::Tensor &actual,
                  const ::torch::Tensor &expected, double rtol = 1e-10,
                  double atol = 1e-10) {
  assert(::torch::allclose(actual, expected, rtol, atol));
}

template <typename Fn> void expect_throw(Fn &&fn) {
  bool threw = false;
  try {
    fn();
  } catch (const std::exception &) {
    threw = true;
  }
  assert(threw);
}

void test_gamma_contract() {
  const auto device = ::torch::Device(::torch::kCPU);
  const auto dtype = ::torch::kFloat64;
  const auto options = ::torch::TensorOptions().dtype(dtype).device(device);
  const auto concentration = ::torch::tensor({2.0, 3.0}, options);
  const auto rate = ::torch::tensor({4.0, 0.5}, options);
  dist::Gamma gamma(device, dtype, concentration, rate);

  assert_close(gamma.mean(), concentration / rate);
  assert_close(gamma.variance(), concentration / (rate * rate));
  assert_close(gamma.mode(), (concentration - 1.0) / rate);

  const auto x = ::torch::tensor({0.75, 4.0}, options);
  const auto expected_log_prob = concentration * ::torch::log(rate) +
                                 ::torch::xlogy(concentration - 1.0, x) -
                                 rate * x - ::torch::lgamma(concentration);
  assert_close(gamma.log_prob(x), expected_log_prob);

  const auto expected_entropy =
      concentration - ::torch::log(rate) + ::torch::lgamma(concentration) +
      (1.0 - concentration) * ::torch::digamma(concentration);
  assert_close(gamma.entropy(), expected_entropy);

  ::torch::manual_seed(11);
  const auto sample = gamma.sample({5});
  assert_shape(sample, {5, 2});
  assert(!sample.isnan().any().item<bool>());
  assert(!sample.isinf().any().item<bool>());
  assert(sample.gt(0).all().item<bool>());

  const auto matrix_concentration = ::torch::full({2, 2}, 1.5, options);
  const auto matrix_sample =
      dist::Gamma::_standard_gamma(matrix_concentration, options);
  assert_shape(matrix_sample, {2, 2});

  const auto f32_options =
      ::torch::TensorOptions().dtype(::torch::kFloat32).device(device);
  dist::Gamma boundary_gamma(device, ::torch::kFloat32,
                             ::torch::tensor({0.5, 1.0}, f32_options),
                             ::torch::tensor({1.0, 2.0}, f32_options));
  const auto boundary_mode = boundary_gamma.mode();
  assert(boundary_mode.dtype() == ::torch::kFloat32);
  assert(boundary_mode.isnan().all().item<bool>());
}

void test_beta_contract() {
  const auto device = ::torch::Device(::torch::kCPU);
  const auto dtype = ::torch::kFloat64;
  const auto options = ::torch::TensorOptions().dtype(dtype).device(device);
  const auto concentration0 = ::torch::tensor({3.0, 4.0}, options);
  const auto concentration1 = ::torch::tensor({2.0, 5.0}, options);
  dist::Beta beta(device, dtype, concentration0, concentration1);

  const auto total = concentration1 + concentration0;
  assert_close(beta.mean(), concentration1 / total);
  assert_close(beta.mode(), (concentration1 - 1.0) / (total - 2.0));
  assert_close(beta.variance(), concentration1 * concentration0 /
                                    (total.pow(2) * (total + 1.0)));

  const auto x = ::torch::tensor({0.25, 0.75}, options);
  const auto expected_log_prob = ::torch::lgamma(total) -
                                 ::torch::lgamma(concentration1) -
                                 ::torch::lgamma(concentration0) +
                                 (concentration1 - 1.0) * ::torch::log(x) +
                                 (concentration0 - 1.0) * ::torch::log1p(-x);
  assert_close(beta.log_prob(x), expected_log_prob);

  const auto expected_entropy =
      ::torch::lgamma(concentration1) + ::torch::lgamma(concentration0) -
      ::torch::lgamma(total) + (total - 2.0) * ::torch::digamma(total) -
      (concentration1 - 1.0) * ::torch::digamma(concentration1) -
      (concentration0 - 1.0) * ::torch::digamma(concentration0);
  assert_close(beta.entropy(), expected_entropy);

  ::torch::manual_seed(17);
  const auto sample = beta.sample({7});
  assert_shape(sample, {7, 2});
  assert(sample.ge(0).all().item<bool>());
  assert(sample.le(1).all().item<bool>());

  const auto f32_options =
      ::torch::TensorOptions().dtype(::torch::kFloat32).device(device);
  dist::Beta boundary_beta(device, ::torch::kFloat32,
                           ::torch::tensor({0.75, 3.0}, f32_options),
                           ::torch::tensor({2.0, 0.5}, f32_options));
  const auto boundary_mode = boundary_beta.mode();
  assert(boundary_mode.dtype() == ::torch::kFloat32);
  assert(boundary_mode.isnan().all().item<bool>());
}

void test_categorical_contract() {
  const auto device = ::torch::Device(::torch::kCPU);
  const auto dtype = ::torch::kFloat64;
  const auto options = ::torch::TensorOptions().dtype(dtype).device(device);
  const auto logits =
      ::torch::tensor({{0.0, 2.0, -1.0}, {1.0, -1.0, 0.0}}, options);
  dist::Categorical categorical(device, dtype, logits);

  ::torch::manual_seed(23);
  const auto samples = categorical.sample({4});
  assert_shape(samples, {4, 2});
  assert(samples.ge(0).all().item<bool>());
  assert(samples.lt(logits.size(-1)).all().item<bool>());

  const auto values =
      ::torch::tensor({1, 0}, ::torch::TensorOptions().dtype(::torch::kInt64));
  const auto expected_log_prob =
      ::torch::gather(::torch::log_softmax(logits, -1), -1,
                      values.unsqueeze(-1))
          .squeeze(-1);
  assert_close(categorical.log_prob(values), expected_log_prob);

  const auto sample_log_prob = categorical.log_prob(samples);
  assert_shape(sample_log_prob, {4, 2});

  const auto mask = ::torch::tensor(
      {{0, 1, 0}, {1, 0, 0}}, ::torch::TensorOptions().dtype(::torch::kBool));
  const auto masked_samples = categorical.mask_sample(mask, {6});
  assert_shape(masked_samples, {6, 2});
  assert(masked_samples.select(1, 0).eq(1).all().item<bool>());
  assert(masked_samples.select(1, 1).eq(0).all().item<bool>());

  const auto bad_mask = ::torch::tensor(
      {{0, 0, 0}, {1, 0, 0}}, ::torch::TensorOptions().dtype(::torch::kBool));
  expect_throw([&] { categorical.mask_sample(bad_mask, {1}); });
}

void test_torch_runtime_seed_contract() {
  const auto options =
      ::torch::TensorOptions().dtype(::torch::kFloat64).device(::torch::kCPU);

  const auto first_scope = ptorch::seed_torch_runtime(123456);
  const auto first = ::torch::rand({4}, options);
  const auto second_scope = ptorch::seed_torch_runtime(123456);
  const auto second = ::torch::rand({4}, options);

  assert(first_scope == second_scope);
  if (::torch::cuda::is_available()) {
    assert(first_scope == "torch_manual_seed_cuda_manual_seed_all");
  } else {
    assert(first_scope == "torch_manual_seed_cpu");
  }
  assert_close(first, second);
}

} // namespace

int main() {
  test_gamma_contract();
  test_beta_contract();
  test_categorical_contract();
  test_torch_runtime_seed_contract();
  return 0;
}
