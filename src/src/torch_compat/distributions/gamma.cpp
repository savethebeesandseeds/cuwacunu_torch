#include "torch_compat/distributions/gamma.h"

RUNTIME_WARNING("(gamma.cpp)[] #FIXME change floats to double. \n");

namespace torch_compat {
namespace distributions {
/* Constructor */
Gamma::Gamma(torch::Device device, torch::Dtype type, torch::Tensor concentration, torch::Tensor rate, bool validate_args)
  : concentration(concentration), rate(rate), kDevice(device), kType(type), validate_args(validate_args) {
  if (validate_args) {
    /* Validation logic */
    cuwacunu::validate_tensor(concentration, "Gamma Distribution constructor [concentration]");
    cuwacunu::validate_tensor(rate, "Gamma Distribution constructor [rate]");
    /* Ensure concentration and rate are positive */
    TORCH_CHECK(concentration.min().item<double>() > 0, "Concentration elements must be positive.");
    TORCH_CHECK(rate.min().item<double>() > 0, "Rate elements must be positive.");
  }
}

/* Reparameterized sampling suitable for gradient descent */
torch::Tensor Gamma::rsample(const torch::IntArrayRef& sample_shape) const {
  auto options = torch::TensorOptions().device(kDevice).dtype(kType);
  auto extended_shape = concentration.sizes().vec();
  extended_shape.insert(extended_shape.begin(), sample_shape.begin(), sample_shape.end());
  auto expanded_concentration = concentration.expand(extended_shape).to(options);
  auto gamma_samples = Gamma::_standard_gamma(expanded_concentration, options);
  auto expanded_rate = rate.expand(extended_shape).to(options);

  return gamma_samples / expanded_rate;
}

/* Sampling method for generating Gamma distributed samples */
torch::Tensor Gamma::sample(const torch::IntArrayRef& sample_shape) const {
  return rsample(sample_shape); /* Forward to reparameterized sample method */
}

/* Log probability of a value */
torch::Tensor Gamma::log_prob(torch::Tensor value) const {
  if (validate_args) {
    _validate_sample(value);
  }
  return torch::xlogy(concentration - 1, value) - (value / rate) - torch::lgamma(concentration) - concentration * torch::log(rate);
}

/* Entropy of the distribution */
torch::Tensor Gamma::entropy() const {
  return concentration + torch::log(rate) + torch::lgamma(concentration) + (1 - concentration) * torch::digamma(concentration);
}

/* Cumulative distribution function */
torch::Tensor Gamma::cdf(torch::Tensor value) const {
  if (validate_args) {
    _validate_sample(value);
  }
  return torch::special::gammainc(concentration, value * rate);
}

/* Mean of the distribution */
torch::Tensor Gamma::mean() const {
  return concentration / rate;
}

/* Mode of the distribution */
torch::Tensor Gamma::mode() const {
  if (concentration.gt(1).all().item<bool>()) {
    return (concentration - 1).clamp(0) / rate;
  } else {
    return torch::full_like(concentration, std::numeric_limits<float>::quiet_NaN(), concentration.options());
  }
}

/* Variance of the distribution */
torch::Tensor Gamma::variance() const {
  return concentration / (rate * rate);
}

/* Validation of sample values */
void Gamma::_validate_sample(const torch::Tensor& value) const {
  TORCH_CHECK(value.ge(0).all().item<bool>(), "Values must be non-negative.");
  TORCH_CHECK(value.dtype() == concentration.dtype(), "Values must have the same dtype as concentration.");
  TORCH_CHECK(value.device() == concentration.device(), "Values must be on the same device as concentration.");
}

/* Marsaglia and Tsang Method */
torch::Tensor Gamma::_standard_gamma(const torch::Tensor& concentration, const torch::TensorOptions& options) {
  auto alpha = concentration;
  auto uniform = torch::rand_like(alpha, options);  /* Using options for uniform distribution */
  auto normal = torch::randn_like(alpha, options);  /* Using options for normal distribution */

  auto d = alpha - 1.0 / 3.0;
  auto c = 1.0 / torch::sqrt(9 * d);

  torch::Tensor x, v, z;
  torch::Tensor condition = torch::ones_like(alpha, options.dtype(torch::kBool)); /* Initialize condition to true */

  /* For alpha >= 1 */
  if (alpha.item<double>() >= 1.0) {
    int max_iter = 1000;  /* Maximum iterations to prevent infinite loop */
    int iter = 0;

    do {
      z = torch::randn_like(alpha, options);
      v = torch::pow(1 + c * z, 3);
      x = d * v;
      condition = torch::logical_and(z > -1.0/c, torch::log(v) > (0.5 * z * z + d - d * v + d * torch::log(v)));
      iter++;
      if(iter < max_iter) {
        log_warn("GAMMA::_standard_gamma exceeded the maximun iteration limit.\n");
        break;
      }
    } while (!condition.all().item<bool>());
    
    return x;
  } else {
    /* For alpha < 1, use alpha+1 and scale by U^(1/alpha) */
    auto gamma_plus_one = _standard_gamma(alpha + 1, options);
    return gamma_plus_one * torch::pow(uniform, 1.0 / alpha);
  }
}
} /* namespace distributions */
} /* namespace torch_compat */