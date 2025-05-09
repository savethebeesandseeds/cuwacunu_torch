/* encoder.h */
#pragma once
#include <torch/torch.h>
#include "dilated_conv.h"

namespace cuwacunu {
namespace wikimyei {
namespace ts2vec {

enum class TSEncoder_MaskMode_e {
    Binomial,
    Continuous,
    AllTrue,
    AllFalse,
    MaskLast
};

/* Utility functions */
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
    TSEncoder_MaskMode_e default_mask_mode;
    torch::Tensor pad_mask;

    torch::nn::Linear input_fc{nullptr};
    DilatedConvEncoder feature_extractor{nullptr};
    torch::nn::Dropout repr_dropout{nullptr};

    /**
     * @brief Constructs a temporal sequence encoder with configurable architecture and optional padding mask.
     *
     * This encoder processes time-series data with multiple channels, using a deep stack of layers.
     * It supports optional masking to ignore padded or invalid regions in the input.
     *
     * @param input_dims            Dimensionality of the input features per time step (i.e., number of channels).
     * @param output_dims           Desired dimensionality of the output representation.
     * @param hidden_dims           Number of hidden units per layer (default: 64).
     * @param depth                 Number of layers (default: 10).
     * @param default_mask_mode     Strategy used for masking when no explicit mask is provided
     *                              (e.g., binomial dropout, uniform, or other custom logic).
     * @param pad_mask              Optional binary mask of shape [T, C] indicating padding positions.
     *                              Each element should be `true` or `1` where the input is padding (to be ignored),
     *                              and `false` or `0` where the input is valid. Used to guide attention or loss
     *                              computation to avoid learning from padded data.
     */
    TSEncoderImpl(
        int input_dims_, 
        int output_dims_, 
        int hidden_dims_=64, 
        int depth_=10, 
        TSEncoder_MaskMode_e default_mask_mode_ = TSEncoder_MaskMode_e::Binomial, 
        c10::optional<torch::Tensor> pad_mask_ = c10::nullopt   /* [T, C] */
    ) : input_dims(input_dims_), 
        output_dims(output_dims_), 
        hidden_dims(hidden_dims_), 
        depth(depth_),
        default_mask_mode(default_mask_mode_),
        pad_mask(pad_mask_.has_value() ? pad_mask_.value().to(torch::kFloat32).unsqueeze(0) : torch::tensor(NAN, torch::kFloat32))
    {
        /* According to Cloneable pattern, initialization should happen in reset() */
        reset();
    }

    void reset() override {
        input_fc = register_module("input_fc", torch::nn::Linear(input_dims, hidden_dims));

        std::vector<int> channels(depth, hidden_dims);
        channels.push_back(output_dims);
        feature_extractor = register_module("feature_extractor", DilatedConvEncoder(hidden_dims, channels, 3));
        repr_dropout = register_module("repr_dropout", torch::nn::Dropout(0.1));
    }
    /**
     * @brief Encodes a batch of time series with structural and self-supervised masking.
     *
     * Applies two kinds of masking:
     * 
     * 1. Structural `pad_mask`: A static binary mask [T, C] that marks which input features are valid (non-padding) 
     *    across all batches. If provided, this mask is broadcasted to [B, T, C] and used to zero out padding regions.
     *
     * 2. Dynamic `mask_mode`: A per-example masking strategy applied at runtime to simulate missing data
     *    (e.g., for self-supervised training). This mask operates over the temporal axis [B, T].
     *
     * The final mask used in training is the intersection of both the runtime mask and a NaN-derived mask,
     * ensuring only valid and non-corrupted data contributes to learning.
     *
     * @param x_input               Input tensor of shape [B, T, C].
     * @param mask_mode_overwrite  Optional override for the masking strategy (e.g., Binomial, AllTrue).
     * @return                      Encoded tensor of shape [B, T, output_dims].
     */
    torch::Tensor forward(
        const torch::Tensor& x_input, 
        c10::optional<TSEncoder_MaskMode_e> mask_mode_overwrite = c10::nullopt
    ) {
        auto x = x_input;

        /* 1. Apply static structural mask if provided: zero out padding regions. */
        if (!torch::isnan(pad_mask).any().item<bool>()) {
            /* pad_mask: [1, T, C], broadcasted to [B, T, C] */
            x = x * pad_mask;
        }

        /* 2. Compute dynamic runtime mask from missing values (NaNs in input) */
        /*    → nan_mask: [B, T], true where all channels at timestep t are valid (not NaN) */
        auto nan_mask = ~torch::isnan(x).any(-1); 

        /* Replace NaNs with zero before projecting input */
        x = torch::nan_to_num(x, /*nan=*/0.0);

        /* Linear projection of input to hidden dimension */
        x = input_fc(x);

        /* 3. Determine runtime masking mode for self-supervised masking */
        TSEncoder_MaskMode_e mask_type = mask_mode_overwrite.has_value() 
            ? mask_mode_overwrite.value() 
            : (is_training() ? default_mask_mode : TSEncoder_MaskMode_e::AllTrue);

        torch::Tensor mask;

        /* Generate a time mask [B, T] based on selected strategy */
        switch (mask_type) {
            case TSEncoder_MaskMode_e::Binomial:
                mask = generate_binomial_mask(x.size(0), x.size(1)).to(x.device());
                break;
            case TSEncoder_MaskMode_e::Continuous:
                mask = generate_continuous_mask(x.size(0), x.size(1)).to(x.device());
                break;
            case TSEncoder_MaskMode_e::AllTrue:
                mask = torch::full({x.size(0), x.size(1)}, true, torch::kBool).to(x.device());
                break;
            case TSEncoder_MaskMode_e::AllFalse:
                mask = torch::full({x.size(0), x.size(1)}, false, torch::kBool).to(x.device());
                break;
            case TSEncoder_MaskMode_e::MaskLast:
                mask = torch::full({x.size(0), x.size(1)}, true, torch::kBool).to(x.device());
                mask.index({torch::indexing::Slice(), -1}) = false;
                break;
        }

        /* 4. Combine runtime mask with nan-derived mask: only valid & unmasked entries are used */
        mask = mask & nan_mask;

        /* Apply mask to encoded features: zero out masked timesteps */
        x = torch::where(mask.unsqueeze(-1), x, torch::zeros_like(x)); /* shape: [B, T, C] */

        /* 5. Pass through dilated conv stack, transpose for [B, C, T] and back */
        x = x.transpose(1, 2); /* [B, C, T] */
        x = repr_dropout(feature_extractor->forward(x));
        x = x.transpose(1, 2); /* [B, T, C] */

        return x;
    }

};
TORCH_MODULE(TSEncoder);

} /* namespace ts2vec */
} /* namespace wikimyei */
} /* namespace cuwacunu */