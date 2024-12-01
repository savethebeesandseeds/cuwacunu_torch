#pragma once
#include <torch/torch.h>
#include "piaabo/dutils.h"
#include "piaabo/darchitecture.h"
#include "piaabo/torch_compat/torch_utils.h"

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {
namespace distributions {
class Gamma {
public:
  /* Constructor */
  Gamma(torch::Device device, torch::Dtype type, torch::Tensor concentration, torch::Tensor rate, bool validate_args = true);

  /* Sample from the Gamma distribution */
  torch::Tensor sample(const torch::IntArrayRef& sample_shape) const;

  /* Method to generate reparameterized samples suitable for gradient descent */
  torch::Tensor rsample(const torch::IntArrayRef& sample_shape) const;

  /* Calculate the log probability of a given value */
  torch::Tensor log_prob(torch::Tensor value) const;

  /* Compute the entropy of the distribution */
  torch::Tensor entropy() const;

  /* Compute the cumulative distribution function */
  torch::Tensor cdf(torch::Tensor value) const;

  /* Accessors for mean, mode, and variance */
  torch::Tensor mean() const;
  torch::Tensor mode() const;
  torch::Tensor variance() const;

private:
  torch::Tensor concentration;
  torch::Tensor rate;
  torch::Device kDevice;
  torch::Dtype kType;
  bool validate_args;

  /* Utility to check if tensor values are within the expected range */
  void _validate_sample(const torch::Tensor& value) const;

public:
  /* Static utility to standard gamma sampling function */
  static torch::Tensor _standard_gamma(const torch::Tensor& concentration, const torch::TensorOptions& options);
};
ENFORCE_ARCHITECTURE_DESIGN(Gamma);
} /* namespace distributions */
} /* namespace torch_compat */
} /* namespace piaabo */
} /* namespace cuwacunu */