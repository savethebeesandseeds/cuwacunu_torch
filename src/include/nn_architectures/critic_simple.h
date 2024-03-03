#pragma once
#include "../dtypes.h"

namespace cuwacunu {
struct CriticModel : torch::nn::Module {
    torch::nn::Linear fc{nullptr}, out{nullptr};
    CriticModel(int64_t state_size) {
        fc = register_module("fc", torch::nn::Linear(state_size, 64));
        out = register_module("out", torch::nn::Linear(64, 1));

        this->to(cuwacunu::kDevice);
    }
    void reset_memory() {
        ...
    }
    torch::Tensor forward(torch::Tensor& x) {
        x = torch::relu(fc->forward(x));
        x = out->forward(x);
        return x;
    }
};
}