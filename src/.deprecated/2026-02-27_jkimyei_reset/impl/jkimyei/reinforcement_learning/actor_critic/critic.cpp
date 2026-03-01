#include "critic.h"

namespace cuwacunu {
CriticModelImpl::CriticModelImpl(int64_t state_size) {
  // Register and initialize the modules
  fc = register_module("fc", torch::nn::Linear(state_size, 64));
  fc_activation = register_module("fc_activation", torch::nn::LeakyReLU(torch::nn::LeakyReLUOptions().negative_slope(0.01)));
  out = register_module("out", torch::nn::Linear(64, 1));

  // Validate all parameters to ensure proper initialization
  validate_module_parameters(*this);
  // Move the entire model to the designated device
  this->to(cuwacunu::kDevice);
  
  // ...reset_memory() { // needed for bidirectional recurrent layers
}

torch::Tensor CriticModelImpl::forward(const torch::Tensor& x) {
  auto x_embedded = fc_activation(fc(x));
  return out(x_embedded);
}
void CriticModelImpl::reset_memory() /* needed for BiLSTM */ {}
} /* namespace cuwacunu */