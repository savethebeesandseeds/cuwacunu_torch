#pragma once
#include "dtypes.h"
#include "torch_compat/torch_utils.h"
#include <torch/torch.h>

...make it inherit from abstract
namespace cuwacunu {
namespace learning_schemas {
namespace networks {
struct ActorModelImpl : public torch::nn::Module {
private:
  /* base layer */
  torch::nn::Linear base_embedding{nullptr};
  torch::nn::LeakyReLU base_activation{nullptr};
  /* categorical head */
  torch::nn::Linear categorical_head{nullptr};
  /* continious head */
  torch::nn::Linear continious_base_embedding{nullptr};
  torch::nn::LeakyReLU continious_base_activation{nullptr};
  torch::nn::Linear continious_alpha_head{nullptr};
  torch::nn::Sigmoid continious_alpha_activation{nullptr};
  torch::nn::Linear continious_beta_head{nullptr};
  torch::nn::Sigmoid continious_beta_activation{nullptr};
public:
  /* action dim */
  int64_t _action_dim = 4; /* 4 is: {confidence, urgency, threshold, delta} */
  ActorModelImpl(int64_t state_size);
  action_logits_t forward(const torch::Tensor& x);
  cuwacunu::action_space_t selectAction(cuwacunu::state_space_t& state, bool explore);
  void reset_memory();
};
TORCH_MODULE(ActorModel);
} /* namespace networks */
} /* namespace learning_schemas */
} /* namespace cuwacunu */