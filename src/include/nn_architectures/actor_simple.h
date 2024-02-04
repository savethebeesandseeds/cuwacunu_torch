#pragma once
#include "../dtypes.h"

namespace cuwacunu {
struct ActorModel : torch::nn::Module {
    torch::nn::Linear fc{nullptr}, mean{nullptr}, log_std{nullptr};

    ActorModel(int64_t state_size, int64_t action_dim) {
        fc = register_module("fc", torch::nn::Linear(state_size, 64));
        mean = register_module("mean", torch::nn::Linear(64, action_dim));
        log_std = register_module("log_std", torch::nn::Linear(64, action_dim));

        this->to(cuwacunu::kDevice);
    }

    std::tuple<torch::Tensor, torch::Tensor> forward(torch::Tensor x) {
        x = torch::relu(fc->forward(x));
        auto mean_out = mean->forward(x);
        auto std_out = torch::softplus(log_std->forward(x));
        return std::make_tuple(mean_out, std_out);
    }

    // Method to get the action distribution
    torch::distributions::Normal getActionDistribution(const torch::Tensor& state) {
        auto [mean, log_std] = this->forward(state.unsqueeze(0));
        auto std = log_std.exp();
        return torch::distributions::Normal(mean, std);
    }

    // Method to sample an action directly from the actor model
    torch::Tensor selectAction(const torch::Tensor& state) {
        auto normal_dist = this->getActionDistribution(state);
        auto action = normal_dist.sample();
        return action.squeeze(0); // Adjust for single action dimension, if needed
    }
};
}