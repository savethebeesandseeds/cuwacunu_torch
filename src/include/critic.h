#pragma once
#include "dtypes.h"
#include "torch_compat/torch_utils.h"
#include <torch/torch.h>

namespace cuwacunu {
struct CriticModelImpl : torch::nn::Module {
    torch::nn::Linear fc;
    torch::nn::LeakyReLU fc_embedding;
    torch::nn::Linear out;
    CriticModelImpl(int64_t state_size);
    torch::Tensor forward(const torch::Tensor& x);
    void reset_memory();
};
TORCH_MODULE(CriticModel);
} /* namespace cuwacunu */