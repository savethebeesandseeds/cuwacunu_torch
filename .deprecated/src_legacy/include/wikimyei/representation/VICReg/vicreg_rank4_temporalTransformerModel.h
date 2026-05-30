/* vicreg_rank4_TemporalTransformerModel.h */
#pragma once

#include <torch/torch.h>

#include <utility>

#include "piaabo/torch_compat/torch_utils.h"

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_rank4 {
    
/* ------------------------------
 * Tiny Temporal‑Transformer‑1D  
 * ------------------------------*/
 struct TemporalTransformer1DImpl : public torch::nn::Cloneable<TemporalTransformer1DImpl> {
    // channels = (C * D_expansion) once flattened
    int channels;
    int T;           // history horizon length Hx (assumed constant for simplicity)

    torch::nn::Conv1d conv1{nullptr}, conv2{nullptr};

    TemporalTransformer1DImpl(int channels_, int T_)
        : channels(channels_), T(T_)
    {
        reset();
    }

    void reset() override {
        conv1 = register_module("conv1", torch::nn::Conv1d(
            torch::nn::Conv1dOptions(channels, channels, /*kernel_size=*/3).padding(1)));
        conv2 = register_module("conv2", torch::nn::Conv1d(
            torch::nn::Conv1dOptions(channels, channels, /*kernel_size=*/3).padding(1)));
    }

    /* ------------------------------------------------------------------
     * Forward
     *  x : [B, channels, Hx]                    (float)
     *  time_mask : optional [B, 1, Hx] bool mask for fused-valid steps
     *  returns warped x of identical shape.
     *
     * 1) predict offset field over [B,1,Hx]
     * 2) build a sampling grid over the Hx axis
     * 3) torch::grid_sampler  (N,C,H=1,W=Hx)
     * ----------------------------------------------------------------*/
    torch::Tensor forward(const torch::Tensor &x,
                          c10::optional<torch::Tensor> time_mask = c10::nullopt) {
        const auto B = x.size(0);
        const auto T_len = x.size(2);

        const auto mask = time_mask.has_value()
            ? time_mask.value().to(torch::kBool)
            : torch::Tensor{};
        auto remask = [&](torch::Tensor t) {
            if (!mask.defined()) return t;
            return t.masked_fill(~mask.expand_as(t), 0.0);
        };

        // 1) Offset prediction (shared over channels)
        auto hidden = torch::relu(conv1(x));   // [B, C, Hx]
        hidden = remask(std::move(hidden));

        auto offset_logits = conv2(hidden);    // [B, C, Hx]
        offset_logits = remask(std::move(offset_logits));

        auto offsets = torch::tanh(offset_logits);
        offsets = offsets.mean(1, /*keepdim=*/true);               // [B,1,Hx]
    
        // 2) Build grid with shape [B,1,Hx]
        auto lin = torch::linspace(-1.0, 1.0, T_len, x.options());
        lin = lin.repeat({B,1}).unsqueeze(1);  // [B,1,Hx]
        auto grid = lin + offsets;             // [B,1,Hx]
    
        // Convert to 2-D expected by grid_sampler: last dim is (x, y)
        auto zeros = torch::zeros_like(grid);      
        auto grid2d = torch::stack({grid, zeros}, /*dim=*/3); // [B,1,Hx,2]
    
        // 3) Reshape x for sampling: [B,C,1,Hx]
        auto x4d = x.unsqueeze(2);
    
        // 4) Sample
        auto warped = torch::grid_sampler(
            x4d, grid2d, /*mode=*/0, /*padding_mode=*/0, /*align_corners=*/true
        );
    
        // 5) Remove the extra spatial dim
        warped = warped.squeeze(2); // [B, C, Hx]
        return remask(std::move(warped));
    }
};
TORCH_MODULE(TemporalTransformer1D);

} // namespace vicreg_rank4
} // namespace wikimyei
} // namespace cuwacunu
