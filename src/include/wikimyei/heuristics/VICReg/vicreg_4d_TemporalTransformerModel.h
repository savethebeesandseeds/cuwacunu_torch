/* vicreg_4d_TemporalTransformerModel.h */
#pragma once

#include <torch/torch.h>

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {
    
/* ------------------------------
 * Tiny Temporal‑Transformer‑1D  
 * ------------------------------*/
 struct TemporalTransformer1DImpl : public torch::nn::Cloneable<TemporalTransformer1DImpl> {
    // channels = (C * D_expansion) once flattened
    int channels;
    int T;           // input sequence length (assumed constant for simplicity)

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
     *  x : [B, channels, T]                     (float)
     *  returns warped x of identical shape.
     *
     * 1) predict offset field Δ ∈ (‑1,1)^{B×1×T}
     * 2) build a sampling grid  g(t) = 2·t/(T−1) − 1 + Δ_t      (‑1…1)
     * 3) torch::grid_sampler  (N,C,H=1,W=T)
     * ----------------------------------------------------------------*/
    torch::Tensor forward(const torch::Tensor &x) {
        const auto B = x.size(0);
        const auto T_len = x.size(2);

        // 1) Offset prediction (shared over channels)
        auto offsets = torch::tanh(conv2(torch::relu(conv1(x)))); // [B, C, T]
        // Average across channels → [B,1,T]
        offsets = offsets.mean(1, /*keepdim=*/true);

        // 2) Build grid   shape [B, 1, 1, T]
        auto lin = torch::linspace(-1.0, 1.0, T_len, x.options());
        lin = lin.repeat({B,1}).unsqueeze(1).unsqueeze(1); // [B,1,1,T]
        auto grid = lin + offsets.unsqueeze(2);            // [B,1,1,T]

        // Convert 1‑D grid to 2‑D expected by grid_sampler:  last dim is x‑coord, y‑coord is 0
        auto grid2d = torch::stack({grid, torch::zeros_like(grid)}, /*dim=*/4); // [B,1,1,T,2]

        // 3) Need x as 4‑D : [B, C, 1, T]
        auto x4d = x.unsqueeze(2); // [B, channels, 1, T]
        auto warped = torch::grid_sampler(x4d, grid2d, /*mode=*/0, /*padding_mode=*/0, /*align_corners=*/true);
        warped = warped.squeeze(2); // back to [B, channels, T]
        return warped;
    }
};
TORCH_MODULE(TemporalTransformer1D);

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
