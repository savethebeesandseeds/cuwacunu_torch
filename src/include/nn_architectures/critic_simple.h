#pragma once
#include "../dtypes.h"

namespace cuwacunu {
struct CriticModel : torch::nn::Module {
    torch::nn::Linear fc{nullptr}, out{nullptr};
    torch::nn::LeakyReLU fc_embedding{nullptr};
    CriticModel(int64_t state_size) : 
        fc(register_module("fc", torch::nn::Linear(state_size, 64))),
        fc_embedding(register_module("fc_embedding", torch::nn::LeakyReLU(/* negative_slope= */ 0.01))),
        out(register_module("out", torch::nn::Linear(64, 1))) {
        this->to(cuwacunu::kDevice);
        // ...reset_memory() { // needed for bidirectional recurrent layers
    }
    torch::Tensor forward(torch::Tensor& x) {
        x = fc_embedding(fc->forward(x));
        x = out->forward(x);
        return x;
    }
    void reset_memory() {}
};
TORCH_MODULE(CriticModel);
}