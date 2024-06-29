#pragma once
#include <torch/torch.h>
#include "piaabo/dutils.h"

namespace cuwacunu {
/* learn_space_t */
struct learn_space_t {
private: /* to inforce architecture design, only friend classes have access */
  torch::Tensor current_value;
  torch::Tensor next_value;
  torch::Tensor expected_value;
  torch::Tensor critic_losses;
  torch::Tensor actor_categorical_losse;
  torch::Tensor actor_continuous_losse;
};
} /* namespace cuwacunu */