#include "piaabo/tensor/torch/distributions/gamma.h"

#include <ATen/ops/_standard_gamma.h>
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

/* Constructor */
Gamma::Gamma(::torch::Device device, ::torch::Dtype type,
             ::torch::Tensor concentration, ::torch::Tensor rate,
             bool validate_args)
    : concentration(concentration.to(device).to(type)),
      rate(rate.to(device).to(type)), kDevice(device), kType(type),
      validate_args(validate_args) {
  if (validate_args) {
    /* Validation logic */
    validate_tensor(this->concentration,
                    "Gamma Distribution constructor [concentration]");
    validate_tensor(this->rate, "Gamma Distribution constructor [rate]");
    /* Ensure concentration and rate are positive */
    TORCH_CHECK(this->concentration.is_floating_point(),
                "Concentration must be floating point.");
    TORCH_CHECK(this->rate.is_floating_point(), "Rate must be floating point.");
    TORCH_CHECK(this->concentration.min().item<double>() > 0,
                "Concentration elements must be positive.");
    TORCH_CHECK(this->rate.min().item<double>() > 0,
                "Rate elements must be positive.");
  }
}

/* Reparameterized sampling suitable for gradient descent */
::torch::Tensor Gamma::rsample(const ::torch::IntArrayRef &sample_shape) const {
  auto options = ::torch::TensorOptions().device(kDevice).dtype(kType);
  auto params = ::torch::broadcast_tensors({concentration, rate});
  const auto extended_shape =
      extended_sample_shape(sample_shape, params[0].sizes());
  auto expanded_concentration = params[0].expand(extended_shape).to(options);
  auto gamma_samples = Gamma::_standard_gamma(expanded_concentration, options);
  auto expanded_rate = params[1].expand(extended_shape).to(options);

  return gamma_samples / expanded_rate;
}

/* Sampling method for generating Gamma distributed samples */
::torch::Tensor Gamma::sample(const ::torch::IntArrayRef &sample_shape) const {
  return rsample(sample_shape); /* Forward to reparameterized sample method */
}

/* Log probability of a value */
::torch::Tensor Gamma::log_prob(::torch::Tensor value) const {
  value = value.to(kDevice).to(kType);
  if (validate_args) {
    _validate_sample(value);
  }
  auto tensors = ::torch::broadcast_tensors({concentration, rate, value});
  const auto &c = tensors[0];
  const auto &r = tensors[1];
  const auto &x = tensors[2];
  return c * ::torch::log(r) + ::torch::xlogy(c - 1, x) - r * x -
         ::torch::lgamma(c);
}

/* Entropy of the distribution */
::torch::Tensor Gamma::entropy() const {
  return concentration - ::torch::log(rate) + ::torch::lgamma(concentration) +
         (1 - concentration) * ::torch::digamma(concentration);
}

/* Cumulative distribution function */
::torch::Tensor Gamma::cdf(::torch::Tensor value) const {
  value = value.to(kDevice).to(kType);
  if (validate_args) {
    _validate_sample(value);
  }
  auto tensors = ::torch::broadcast_tensors({concentration, rate, value});
  return ::torch::special::gammainc(tensors[0], tensors[2] * tensors[1]);
}

/* Mean of the distribution */
::torch::Tensor Gamma::mean() const { return concentration / rate; }

/* Mode of the distribution */
::torch::Tensor Gamma::mode() const {
  if (concentration.gt(1).all().item<bool>()) {
    return (concentration - 1).clamp(0) / rate;
  } else {
    return ::torch::full_like(concentration,
                              std::numeric_limits<double>::quiet_NaN(),
                              concentration.options());
  }
}

/* Variance of the distribution */
::torch::Tensor Gamma::variance() const {
  return concentration / (rate * rate);
}

/* Validation of sample values */
void Gamma::_validate_sample(const ::torch::Tensor &value) const {
  TORCH_CHECK(value.ge(0).all().item<bool>(), "Values must be non-negative.");
  TORCH_CHECK(value.dtype() == concentration.dtype(),
              "Values must have the same dtype as concentration.");
  TORCH_CHECK(value.device() == concentration.device(),
              "Values must be on the same device as concentration.");
}

/* Marsaglia and Tsang Method */
::torch::Tensor Gamma::_standard_gamma(const ::torch::Tensor &concentration,
                                       const ::torch::TensorOptions &options) {
  auto alpha = concentration.to(options);
  TORCH_CHECK(alpha.is_floating_point(),
              "Gamma concentration must be floating point.");
  TORCH_CHECK(alpha.min().item<double>() > 0,
              "Gamma concentration elements must be positive.");
  return at::_standard_gamma(alpha);
}
} /* namespace distributions */
} /* namespace torch */
} /* namespace tensor */
} /* namespace piaabo */
} /* namespace cuwacunu */
