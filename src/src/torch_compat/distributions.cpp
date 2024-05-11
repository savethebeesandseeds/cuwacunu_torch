#include "torch_compat/distributions.h"
/* --- Categorical --- */
Categorical::Categorical(torch::Device device, torch::Dtype type, torch::Tensor logits) 
  : logits(logits.to(device, type)), kDevice(device), kType(type) {
  if (!logits.defined()) {
    throw std::invalid_argument("Logits must be specified.");
  }
  if (logits.dim() < 1) {
    throw std::invalid_argument("`logits` parameter must be at least one-dimensional.");
  }
  /* Normalize logits to ensure numerical stability */
  this->logits = logits - logits.logsumexp(-1, true).unsqueeze(-1);
}
/* Sample */
torch::Tensor Categorical::sample(const torch::IntArrayRef& sample_shape) const {
  /* Convert logits to probabilities for sampling */
  auto probs = torch::softmax(logits, -1);
  auto samples_flat = torch::multinomial(probs.view({-1, probs.size(-1)}), 1, true);
  auto samples = samples_flat.view(sample_shape);
  return samples.to(kDevice);
}
/* Log Probability */
torch::Tensor Categorical::log_prob(const torch::Tensor& value) const {
  /* Calculate log probabilities */
  auto log_probs = logits - logits.logsumexp(-1, true).unsqueeze(-1);
  auto value_broadcasted = value.unsqueeze(-1).to(kDevice, kType);
  auto log_pmf = torch::gather(log_probs, -1, value_broadcasted);
  return log_pmf.squeeze(-1);
}
/* probability */
torch::Tensor Categorical::probs() const {
    return torch::softmax(logits, -1);
}
/* Entropy */
torch::Tensor Categorical::entropy() const {
  /* Convert logits to probabilities to calculate entropy */
  auto v_probs = probs();
  auto p_log_p = v_probs * torch::log(v_probs);
  return -p_log_p.sum(-1);
}
/* --- BETA --- */
/* Constructor */
Beta::Beta(torch::Device device, torch::Dtype type, torch::Tensor concentration1, torch::Tensor concentration0) 
  : concentration1(concentration1.to(device).to(type)), 
    concentration0(concentration0.to(device).to(type)),
    kDevice(device), kType(type) {}
/* Mean */
torch::Tensor Beta::mean() const {
  return concentration1 / (concentration1 + concentration0);
}
/* Mode */
torch::Tensor Beta::mode() const {
  auto total = concentration1 + concentration0;
  return (concentration1 - 1) / (total - 2);
}
/* Variance */
torch::Tensor Beta::variance() const {
  auto total = concentration1 + concentration0;
  return concentration1 * concentration0 / (total.pow(2) * (total + 1));
}
/* Sample */
torch::Tensor Beta::sample(const torch::IntArrayRef& sample_shape) const {
  /* Adjust for device and type */
  auto options = torch::TensorOptions().dtype(kType).device(kDevice);
  auto dirichlet_concentration = torch::stack({concentration1, concentration0}, -1).to(options);
  auto samples = torch::rand(dirichlet_concentration.sizes(), options);
  samples = samples / samples.sum(-1, true);
  return samples.select(-1, 0);
}
/* Log Probability */
torch::Tensor Beta::log_prob(torch::Tensor value) const {
  /* Ensure value tensor is on the correct device and has the correct dtype */
  value = value.to(kDevice).to(kType);
  auto lgamma = [](const torch::Tensor& x) { return torch::lgamma(x); };
  auto total = concentration1 + concentration0;
  auto log_prob = lgamma(total) - lgamma(concentration1) - lgamma(concentration0) + 
          (concentration1 - 1) * torch::log(value) + 
          (concentration0 - 1) * torch::log(1 - value);
  return log_prob;
}
/* Entropy */
torch::Tensor Beta::entropy() const {
  auto lgamma = [](const torch::Tensor& x) { return torch::lgamma(x); };
  auto total = concentration1 + concentration0;
  auto lgc0 = lgamma(concentration0);
  auto lgc1 = lgamma(concentration1);
  auto lgct = lgamma(total);
  return lgc1 + lgc0 - lgct +
      (total - 2) * (lgct - lgc1 - lgc0) +
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