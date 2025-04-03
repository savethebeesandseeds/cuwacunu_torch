/* encoder.h */
#pragma once
#include <torch/torch.h>
#include "dilated_conv.h"

namespace cuwacunu {
namespace wikimyei {
namespace ts2vec {

// Utility functions
inline torch::Tensor generate_continuous_mask(int64_t B, int64_t T, int n=5, double l=0.1) {
    auto res = torch::full({B, T}, true, torch::kBool);
    if (l < 1.0) l = int(l * T);
    l = std::max<int>(l, 1);
    
    if (n < 1.0) n = int(n * T);
    n = std::max<int>(std::min<int>(n, T / 2), 1);

    for (int64_t i = 0; i < B; ++i) {
        for (int j = 0; j < n; ++j) {
            int64_t t = torch::randint(0, T - l + 1, {1}).item<int64_t>();
            res.index({i, torch::indexing::Slice(t, t + l)}) = false;
        }
    }
    return res;
}

inline torch::Tensor generate_binomial_mask(int64_t B, int64_t T, double p=0.5) {
    auto mask = torch::bernoulli(torch::full({B, T}, p)).to(torch::kBool);
    return mask;
}

struct TSEncoderImpl : public torch::nn::Cloneable<TSEncoderImpl> {
    int input_dims;
    int output_dims;
    int hidden_dims;
    int depth;
    std::string mask_mode;
    torch::nn::Linear input_fc{nullptr};
    DilatedConvEncoder feature_extractor{nullptr};
    torch::nn::Dropout repr_dropout{nullptr};

    TSEncoderImpl(
        int input_dims_, 
        int output_dims_, 
        int hidden_dims_=64, 
        int depth_=10, 
        std::string mask_mode_="binomial"
    ) : input_dims(input_dims_), 
        output_dims(output_dims_), 
        hidden_dims(hidden_dims_), 
        depth(depth_),
        mask_mode(mask_mode_) 
    {
        // According to Cloneable pattern, initialization should happen in reset()
        reset();
    }

    void reset() override {
        input_fc = register_module("input_fc", torch::nn::Linear(input_dims, hidden_dims));

        std::vector<int> channels(depth, hidden_dims);
        channels.push_back(output_dims);
        feature_extractor = register_module("feature_extractor", DilatedConvEncoder(hidden_dims, channels, 3));
        repr_dropout = register_module("repr_dropout", torch::nn::Dropout(0.1));
    }

    torch::Tensor forward(const torch::Tensor& x_input, c10::optional<std::string> mask_opt=c10::nullopt) {
        auto x = x_input;

        auto nan_mask = ~torch::isnan(x).any(-1);
        x = torch::nan_to_num(x, /*nan=*/0.0);
        x = input_fc(x);

        std::string mask_type = mask_opt.has_value() ? mask_opt.value() : (is_training() ? mask_mode : "all_true");

        torch::Tensor mask;
        if (mask_type == "binomial") {
            mask = generate_binomial_mask(x.size(0), x.size(1)).to(x.device());
        } else if (mask_type == "continuous") {
            mask = generate_continuous_mask(x.size(0), x.size(1)).to(x.device());
        } else if (mask_type == "all_true") {
            mask = torch::full({x.size(0), x.size(1)}, true, torch::kBool).to(x.device());
        } else if (mask_type == "all_false") {
            mask = torch::full({x.size(0), x.size(1)}, false, torch::kBool).to(x.device());
        } else if (mask_type == "mask_last") {
            mask = torch::full({x.size(0), x.size(1)}, true, torch::kBool).to(x.device());
            mask.index({torch::indexing::Slice(), -1}) = false;
        }

        mask = mask & nan_mask;
        x = torch::where(mask.unsqueeze(-1), x, torch::zeros_like(x));

        x = x.transpose(1, 2); // B x Ch x T
        x = repr_dropout(feature_extractor->forward(x));
        x = x.transpose(1, 2); // B x T x Co

        return x;
    }
};
TORCH_MODULE(TSEncoder);

} // namespace ts2vec
} // namespace wikimyei
} // namespace cuwacunu