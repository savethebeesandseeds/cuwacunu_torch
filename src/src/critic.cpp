#include "critic.h"

namespace cuwacunu {
CriticModelImpl::CriticModelImpl(int64_t state_size)
  : fc(register_module("fc", torch::nn::Linear(state_size, 64))),
    fc_embedding(register_module("fc_embedding", torch::nn::LeakyReLU(torch::nn::LeakyReLUOptions().negative_slope(0.01)))),
    out(register_module("out", torch::nn::Linear(64, 1))) {
      // ...reset_memory() { // needed for bidirectional recurrent layers
      validate_module_parameters(*this);
      this->to(cuwacunu::kDevice);
}
torch::Tensor CriticModelImpl::forward(const torch::Tensor& x) {
  auto x_embedded = fc_embedding(fc(x));
  return out(x_embedded);
}
void CriticModelImpl::reset_memory() /* needed for BiLSTM */ {}
} /* namespace cuwacunu */