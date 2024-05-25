#include "torch_compat/distributions/categorical.h"

RUNTIME_WARNING("(categorical.cpp)[] #FIXME: Categorical distribution needs testing.\n");

namespace torch_compat {
namespace distributions {
/* --- Categorical --- */
/* Constructor */
Categorical::Categorical(torch::Device device, torch::Dtype type, torch::Tensor logits) 
  : kDevice(device), kType(type) {
    cuwacunu::validate_tensor(logits, "Categorical Distribution constructor");
    /* Move logits to the specified device and type */
    this->logits = logits.to(device, type);
    /* Normalize logits to ensure numerical stability */
    this->logits = this->logits - this->logits.logsumexp(-1, true);
    /* Calculate log probabilities once */
    this->log_probs = torch::log_softmax(this->logits, -1);
}

/* Sample */
torch::Tensor Categorical::sample(const torch::IntArrayRef& sample_shape) const {
  /* Convert logits to probabilities for sampling */
  auto probs = torch::softmax(logits, -1);
  /* Handle the case where sample_shape is empty */
  int64_t num_samples = sample_shape.empty() ? 1 : sample_shape[0];
  /* Generate flat samples */
  auto samples_flat = torch::multinomial(probs.view({-1, probs.size(-1)}), num_samples, true);
  /* Reshape the samples to the desired shape */
  if (!sample_shape.empty()) {
    std::vector<int64_t> result_shape(sample_shape.begin(), sample_shape.end());
    samples_flat = samples_flat.view(result_shape);
  }
  
  return samples_flat.to(kDevice);
}

/* Masked Sample */
torch::Tensor Categorical::mask_sample(const torch::Tensor& mask, const torch::IntArrayRef& sample_shape) const {
  cuwacunu::assert_tensor_shape(mask, logits.size(0), "Categorical::mask_sample");
  /* Apply mask to logits to zero out specified elements */
  auto masked_logits = logits + (mask.to(kDevice, kType) - 1) * 1e10;  /* Large negative number to zero out */
  /* Convert masked logits to probabilities */
  auto masked_probs = torch::softmax(masked_logits, -1);
  /* Handle the case where sample_shape is empty */
  int64_t num_samples = sample_shape.empty() ? 1 : sample_shape[0];
  /* Generate flat samples */
  auto samples_flat = torch::multinomial(masked_probs.view({-1, masked_probs.size(-1)}), num_samples, true);
  /* Reshape the samples to the desired shape */
  if (!sample_shape.empty()) {
    std::vector<int64_t> result_shape(sample_shape.begin(), sample_shape.end());
    samples_flat = samples_flat.view(result_shape);
  }
  
  return samples_flat.to(kDevice);
}

/* Log Probability */
torch::Tensor Categorical::log_prob(const torch::Tensor& value) const {
  /* Calculate log probabilities */
  auto value_broadcasted = value.unsqueeze(-1).to(kDevice, torch::kInt64);
  auto log_pmf = torch::gather(log_probs, -1, value_broadcasted);
  return log_pmf.squeeze(-1);
}

/* Probability */
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
} /* namespace distributions */
} /* namespace torch_compat */