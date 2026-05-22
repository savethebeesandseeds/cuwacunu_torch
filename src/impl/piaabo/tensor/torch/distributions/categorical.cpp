#include "piaabo/tensor/torch/distributions/categorical.h"

#include <limits>
#include <vector>

namespace cuwacunu {
namespace piaabo {
namespace tensor {
namespace torch {
namespace distributions {

namespace {

int64_t sample_count_from_shape(const ::torch::IntArrayRef &sample_shape) {
  int64_t count = 1;
  for (const int64_t dim : sample_shape) {
    TORCH_CHECK(dim >= 0,
                "Categorical sample_shape dimensions must be non-negative.");
    count *= dim;
  }
  return count;
}

std::vector<int64_t>
categorical_result_shape(const ::torch::IntArrayRef &sample_shape,
                         const ::torch::IntArrayRef &logits_shape) {
  TORCH_CHECK(logits_shape.size() >= 1,
              "Categorical logits must have at least one dimension.");
  std::vector<int64_t> shape(sample_shape.begin(), sample_shape.end());
  shape.insert(shape.end(), logits_shape.begin(), logits_shape.end() - 1);
  return shape;
}

::torch::Tensor sample_from_probs(const ::torch::Tensor &probs,
                                  const ::torch::IntArrayRef &sample_shape,
                                  const ::torch::Device &device) {
  const int64_t num_samples = sample_count_from_shape(sample_shape);
  TORCH_CHECK(num_samples > 0,
              "Categorical sample_shape must request at least one sample.");

  const int64_t event_count = probs.size(-1);
  auto flat_probs = probs.reshape({-1, event_count});
  auto samples =
      ::torch::multinomial(flat_probs, num_samples, true).transpose(0, 1);
  auto result_shape = categorical_result_shape(sample_shape, probs.sizes());
  return samples.reshape(result_shape).to(device);
}

} // namespace

/* --- Categorical --- */
/* Constructor */
Categorical::Categorical(::torch::Device device, ::torch::Dtype type,
                         ::torch::Tensor logits)
    : kDevice(device), kType(type) {
  validate_tensor(logits, "Categorical Distribution constructor");
  TORCH_CHECK(logits.dim() >= 1,
              "Categorical logits must have at least one dimension.");
  TORCH_CHECK(logits.size(-1) > 0,
              "Categorical logits must have a non-empty event dimension.");
  /* Move logits to the specified device and type */
  this->logits = logits.to(device, type);
  /* Normalize logits to ensure numerical stability */
  this->logits = this->logits - this->logits.logsumexp(-1, true);
  /* Calculate log probabilities once */
  this->log_probs = ::torch::log_softmax(this->logits, -1);
}

/* Sample */
::torch::Tensor
Categorical::sample(const ::torch::IntArrayRef &sample_shape) const {
  /* Convert logits to probabilities for sampling */
  auto probs = ::torch::softmax(logits, -1);
  return sample_from_probs(probs, sample_shape, kDevice);
}

/* Masked Sample */
::torch::Tensor
Categorical::mask_sample(const ::torch::Tensor &mask,
                         const ::torch::IntArrayRef &sample_shape) const {
  TORCH_CHECK(mask.sizes() == logits.sizes(),
              "Categorical::mask_sample mask shape must match logits shape.");
  /* Apply mask to logits to zero out specified elements */
  auto mask_bool = mask.to(kDevice).to(::torch::kBool);
  TORCH_CHECK(
      mask_bool.any(-1).all().item<bool>(),
      "Categorical::mask_sample requires at least one allowed event per row.");
  auto masked_logits =
      logits.masked_fill(::torch::logical_not(mask_bool),
                         -std::numeric_limits<double>::infinity());
  /* Convert masked logits to probabilities */
  auto masked_probs = ::torch::softmax(masked_logits, -1);
  return sample_from_probs(masked_probs, sample_shape, kDevice);
}

/* Log Probability */
::torch::Tensor Categorical::log_prob(const ::torch::Tensor &value) const {
  /* Calculate log probabilities */
  auto value_broadcasted = value.to(kDevice, ::torch::kInt64);
  validate_tensor(value_broadcasted, "Categorical::log_prob value");
  TORCH_CHECK(value_broadcasted.min().item<int64_t>() >= 0 &&
                  value_broadcasted.max().item<int64_t>() < logits.size(-1),
              "Categorical::log_prob value is outside the event range.");
  auto expanded_log_probs = log_probs;
  while (expanded_log_probs.dim() < value_broadcasted.dim() + 1) {
    expanded_log_probs = expanded_log_probs.unsqueeze(0);
  }
  std::vector<int64_t> gather_shape(value_broadcasted.sizes().begin(),
                                    value_broadcasted.sizes().end());
  gather_shape.push_back(logits.size(-1));
  expanded_log_probs = expanded_log_probs.expand(gather_shape);
  auto log_pmf =
      ::torch::gather(expanded_log_probs, -1, value_broadcasted.unsqueeze(-1));
  return log_pmf.squeeze(-1);
}

/* Probability */
::torch::Tensor Categorical::probs() const {
  return ::torch::softmax(logits, -1);
}

/* Entropy */
::torch::Tensor Categorical::entropy() const {
  /* Convert logits to probabilities to calculate entropy */
  auto v_probs = probs();
  auto p_log_p = v_probs * ::torch::log(v_probs);
  return -p_log_p.sum(-1);
}
} /* namespace distributions */
} /* namespace torch */
} /* namespace tensor */
} /* namespace piaabo */
} /* namespace cuwacunu */
