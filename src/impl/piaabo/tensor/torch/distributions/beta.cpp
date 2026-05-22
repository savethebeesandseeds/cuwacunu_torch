#include "piaabo/tensor/torch/distributions/beta.h"

#include <limits>
#include <vector>

namespace cuwacunu {
namespace piaabo {
namespace tensor {
namespace torch {
namespace distributions {

namespace {

std::vector<int64_t>
extended_sample_shape(const ::torch::IntArrayRef &sample_shape,
                      const ::torch::IntArrayRef &batch_shape) {
  std::vector<int64_t> shape(sample_shape.begin(), sample_shape.end());
  shape.insert(shape.end(), batch_shape.begin(), batch_shape.end());
  return shape;
}

} // namespace

/* --- BETA --- */
/* Constructor */
Beta::Beta(::torch::Device device, ::torch::Dtype type,
           ::torch::Tensor concentration0, ::torch::Tensor concentration1)
    : concentration0(concentration0.to(device).to(type)),
      concentration1(concentration1.to(device).to(type)), kDevice(device),
      kType(type) {
  validate_tensor(this->concentration1,
                  "Beta Distribution constructor [concentration1]");
  validate_tensor(this->concentration0,
                  "Beta Distribution constructor [concentration0]");
  TORCH_CHECK(this->concentration1.is_floating_point(),
              "Beta concentration1 must be floating point.");
  TORCH_CHECK(this->concentration0.is_floating_point(),
              "Beta concentration0 must be floating point.");
  TORCH_CHECK(this->concentration1.min().item<double>() > 0,
              "Beta concentration1 elements must be positive.");
  TORCH_CHECK(this->concentration0.min().item<double>() > 0,
              "Beta concentration0 elements must be positive.");
}

/* Mean */
::torch::Tensor Beta::mean() const {
  return concentration1 / (concentration1 + concentration0);
}

/* Mode */
::torch::Tensor Beta::mode() const {
  auto total = concentration1 + concentration0;
  auto valid = ::torch::logical_and(concentration1 > 1, concentration0 > 1);
  auto mode = (concentration1 - 1) / (total - 2);

  /* Handling NaN assignment with ::torch::where or manual tensor creation */
  auto nan_tensor = ::torch::full_like(
      mode, std::numeric_limits<double>::quiet_NaN(), mode.options());
  return ::torch::where(valid, mode, nan_tensor);
}
/* Variance */
::torch::Tensor Beta::variance() const {
  auto total = concentration1 + concentration0;
  return concentration1 * concentration0 / (total.pow(2) * (total + 1));
}

/* Sample */
::torch::Tensor Beta::sample(const ::torch::IntArrayRef &sample_shape) const {
  auto options = ::torch::TensorOptions().dtype(kType).device(kDevice);
  auto params = ::torch::broadcast_tensors({concentration1, concentration0});
  auto shape = extended_sample_shape(sample_shape, params[0].sizes());

  auto gamma1 = Gamma::_standard_gamma(params[0].expand(shape), options);
  auto gamma2 = Gamma::_standard_gamma(params[1].expand(shape), options);

  auto samples = gamma1 / (gamma1 + gamma2);
  return samples;
}

/* Log Probability */
::torch::Tensor Beta::log_prob(::torch::Tensor value) const {
  value = value.to(kDevice).to(kType);
  TORCH_CHECK(value.ge(0).all().item<bool>() && value.le(1).all().item<bool>(),
              "Beta log_prob value must be in [0, 1].");
  auto tensors =
      ::torch::broadcast_tensors({concentration1, concentration0, value});
  const auto &c1 = tensors[0];
  const auto &c0 = tensors[1];
  const auto &x = tensors[2];
  auto total = c1 + c0;
  auto log_prob = ::torch::lgamma(total) - ::torch::lgamma(c1) -
                  ::torch::lgamma(c0) + (c1 - 1) * ::torch::log(x) +
                  (c0 - 1) * ::torch::log1p(-x);
  return log_prob;
}

/* Entropy */
::torch::Tensor Beta::entropy() const {
  auto total = concentration1 + concentration0;
  return ::torch::lgamma(concentration1) + ::torch::lgamma(concentration0) -
         ::torch::lgamma(total) + (total - 2) * ::torch::digamma(total) -
         (concentration1 - 1) * ::torch::digamma(concentration1) -
         (concentration0 - 1) * ::torch::digamma(concentration0);
}

/* Getters for concentration parameters */
::torch::Tensor Beta::get_concentration1() const { return concentration1; }
::torch::Tensor Beta::get_concentration0() const { return concentration0; }
} /* namespace distributions */
} /* namespace torch */
} /* namespace tensor */
} /* namespace piaabo */
} /* namespace cuwacunu */
