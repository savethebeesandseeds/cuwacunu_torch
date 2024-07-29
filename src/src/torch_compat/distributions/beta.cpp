#include "torch_compat/distributions/beta.h"

RUNTIME_WARNING("(beta.cpp)[] #FIXME: Beta distribution needs testing.\n");
RUNTIME_WARNING("(beta.cpp)[] #FIXME change floats to double. \n");

namespace torch_compat {
namespace distributions {
/* --- BETA --- */
/* Constructor */
Beta::Beta(torch::Device device, torch::Dtype type, torch::Tensor concentration0, torch::Tensor concentration1) 
  : concentration0(concentration0.to(device).to(type)),
    concentration1(concentration1.to(device).to(type)), 
    kDevice(device),
    kType(type) {
      cuwacunu::validate_tensor(concentration1, "Beta Distribution constructor [concentration1]");
      cuwacunu::validate_tensor(concentration0, "Beta Distribution constructor [concentration0]");
}

/* Mean */
torch::Tensor Beta::mean() const {
  return concentration1 / (concentration1 + concentration0);
}

/* Mode */
torch::Tensor Beta::mode() const {
  auto total = concentration1 + concentration0;
  auto valid = (concentration1 > 1) * (concentration0 > 1);
  auto mode = (concentration1 - 1) / (total - 2);

  /* Handling NaN assignment with torch::where or manual tensor creation */
  auto nan_tensor = torch::full_like(mode, std::numeric_limits<float>::quiet_NaN(), mode.options());
  return torch::where(valid, mode, nan_tensor);
}
/* Variance */
torch::Tensor Beta::variance() const {
  auto total = concentration1 + concentration0;
  return concentration1 * concentration0 / (total.pow(2) * (total + 1));
}

/* Sample */
torch::Tensor Beta::sample(const torch::IntArrayRef& sample_shape) const {
  auto options = torch::TensorOptions().dtype(kType).device(kDevice);
  
  /* Create expanded shapes */
  std::vector<int64_t> shape(sample_shape.begin(), sample_shape.end());
  if (shape.empty()) {
    shape.push_back(1); /* Default to a single sample if shape is empty */
  }
  shape.insert(shape.end(), concentration1.sizes().begin(), concentration1.sizes().end());

  auto gamma1 = Gamma::_standard_gamma(concentration1.expand(shape), options);
  auto gamma2 = Gamma::_standard_gamma(concentration0.expand(shape), options);

  auto samples = gamma1 / (gamma1 + gamma2);
  return samples;
}

/* Log Probability */
torch::Tensor Beta::log_prob(torch::Tensor value) const {
  value = value.to(kDevice).to(kType);
  auto total = concentration1 + concentration0;
  auto log_prob = torch::lgamma(total) - torch::lgamma(concentration1) - torch::lgamma(concentration0) + 
                  (concentration1 - 1) * torch::log(value) + 
                  (concentration0 - 1) * torch::log(1 - value);
  return log_prob;
}

/* Entropy */
torch::Tensor Beta::entropy() const {
  auto total = concentration1 + concentration0;
  return torch::lgamma(concentration1) + torch::lgamma(concentration0) - torch::lgamma(total) +
         (concentration1 - 1) * torch::digamma(concentration1) +
         (concentration0 - 1) * torch::digamma(concentration0) -
         (total - 2) * torch::digamma(total);
}

/* Getters for concentration parameters */
torch::Tensor Beta::get_concentration1() const {
  return concentration1;
}
torch::Tensor Beta::get_concentration0() const {
  return concentration0;
}
} /* namespace distributions */
} /* namespace torch_compat */