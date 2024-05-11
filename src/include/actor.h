#pragma once
#include "dtypes.h"
#include "torch_compat/torch_utils.h"
#include <torch/torch.h>

namespace cuwacunu {
struct ActorModelImpl : public torch::nn::Module {
private:
  /* base layer */
  torch::nn::Linear base_embedding;
  torch::nn::LeakyReLU base_activation;
  /* categorical head */
  torch::nn::Linear categorical_head;
  /* continious head */
  torch::nn::Linear continuous_base_embedding;
  torch::nn::LeakyReLU continuous_base_activation;
  torch::nn::Linear continuous_alpha_head;
  torch::nn::Sigmoid continuous_alpha_activation;
  torch::nn::Linear continuous_beta_head;
  torch::nn::Sigmoid continuous_beta_activation;
public:
  /* action dim */
  int64_t _action_dim = 4; /* 4 is: {confidence, urgency, threshold, delta} */
  ActorModelImpl(int64_t state_size);
  action_logits_t forward(const torch::Tensor x);
  cuwacunu::action_space_t selectAction(cuwacunu::state_space_t state, bool explore);
  void reset_memory();
};
TORCH_MODULE(ActorModel);
} /* namespace cuwacunu */
