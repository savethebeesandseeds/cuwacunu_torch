... add windowed time wrap
... mask puts what values, nan, probably, but mask must be handled as well. 
... beware, i dont think this code is doing time wrapping, instead it is adding a random portion to prolong or crop

/* vicreg_4d_Augmentations.h */
#include <torch/torch.h>

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

/* ========================================================================
 *  VICReg_4D_Augmentation Struct
 *
 *  Purpose:
 *    Performs data augmentation on 4D tensors (Batch, Channels, Time, Depth)
 *    by applying temporal warping and random masking to help self-supervised
 *    learning methods (like VICReg) learn robust representations.
 *
 *  Usage:
 *    - Input tensors `x` (data) and `m` (mask) must have dimensions [B, C, T, D].
 *    - Augmentation parameters control the degree of warping and masking.
 * ======================================================================== */
struct VICReg_4D_Augmentation {
    int T;            // Nominal length of the time dimension
    int T_buf;        // Extra temporal buffer to safely apply warp slicing

    /* -----------------------------------------------------------------------
     * operator(): Applies elastic warp and random point masking augmentations
     * ----------------------------------------------------------------------- */
    std::pair<torch::Tensor, torch::Tensor>
    operator()(torch::Tensor x, torch::Tensor m,
               float a_t_wrap, float p_t_mask, float p_d_mask) const
    {
        // Step 0: Validate data -------------------------------------
        const auto B = x.size(0);  // Batch size
        const auto C = x.size(1);  // Number of channels
        TORCH_CHECK(data.size(2) == (T + T_buf), "(vicreg_4d_Augmentations.h)[VICReg_4D_Augmentation::operator()] Time dimension (T) in data mismatch");
        TORCH_CHECK(mask.size(2) == (T + T_buf), "(vicreg_4d_Augmentations.h)[VICReg_4D_Augmentation::operator()] Time dimension (T) in mask mismatch");
        const auto D = x.size(3);  // Depth dimension size

        // Step 1: Elastic Temporal Warp -------------------------------------
        // Calculate min and max warp lengths (Lmin, Lmax) around nominal length T
        int Lmin = static_cast<int>(T * (1.0f - a_t_wrap));  // Shorter warp limit (e.g., 0.95 * T)
        int Lmax = static_cast<int>(T * (1.0f + a_t_wrap));  // Longer warp limit (e.g., 1.05 * T)

        // Randomly select warp length L within [Lmin, Lmax]
        int L = torch::randint(Lmin, Lmax + 1, {1}, x.options()).item<int>();

        // Random starting position for the temporal slice within buffer limits
        int start_max = T + T_buf - L;
        int s = torch::randint(0, start_max + 1, {1}, x.options()).item<int>();

        // Extract random slice of length L from original tensors (data and mask)
        auto slice  = x.index({torch::indexing::Slice(), torch::indexing::Slice(),
                               torch::indexing::Slice(s, s + L), torch::indexing::Slice()});
        auto sliceM = m.index({torch::indexing::Slice(), torch::indexing::Slice(),
                               torch::indexing::Slice(s, s + L), torch::indexing::Slice()});

        // Prepare interpolation grid to resize the slice back to length T
        auto grid = torch::linspace(-1.0, 1.0, T, x.options()).view({1, 1, 1, T});
        grid = grid.expand({B, 1, 1, T});
        auto zeros = torch::zeros_like(grid);

        // Grid for sampling (no spatial shifts, only temporal interpolation)
        auto grid4 = torch::stack({grid, zeros}, 4);  // [B, 1, 1, T, 2]

        // Apply grid sampling to interpolate slice to original temporal length T
        auto slice4 = slice.view({B * C * D, 1, 1, L});
        slice4 = torch::grid_sampler(slice4, grid4, 0, 0, true);
        auto out = slice4.view({B, C, T, D});  // Final warped tensor

        // Similarly interpolate the mask tensor
        auto sliceM4 = sliceM.view({B * C * D, 1, 1, L});
        sliceM4 = torch::grid_sampler(sliceM4, grid4, 0, 0, true);
        auto outM = sliceM4.view({B, C, T, D}).round();  // Round to maintain mask binary nature

        // Step 2: Random Point Masking --------------------------------------

        // Apply temporal masking: randomly masks points along the time axis
        if (p_t_mask > 0.0f) {
            auto tmask = torch::bernoulli(torch::full({B, 1, T, 1}, 1.0f - p_t_mask, x.options()));
            out  = out  * tmask;  // Apply temporal mask to data
            outM = outM * tmask;  // Apply temporal mask to mask tensor
        }

        // Apply depth masking: randomly masks points along the depth axis
        if (p_d_mask > 0.0f) {
            auto dmask = torch::bernoulli(torch::full({B, 1, 1, D}, 1.0f - p_d_mask, x.options()));
            out  = out  * dmask;  // Apply depth mask to data
            outM = outM * dmask;  // Apply depth mask to mask tensor
        }

        // Return augmented data and corresponding mask as a pair
        return {out, outM};
    }

    /* ---------------------------------------------------------------------
     * augment1 & augment2: Convenience wrappers
     *
     * Provide interface for two independent augmentations with identical policies
     * --------------------------------------------------------------------- */
    inline std::pair<torch::Tensor, torch::Tensor>
    augment1(torch::Tensor x, torch::Tensor m, float a, float pT, float pD) const {
        return (*this)(x, m, a, pT, pD);
    }

    inline std::pair<torch::Tensor, torch::Tensor>
    augment2(torch::Tensor x, torch::Tensor m, float a, float pT, float pD) const {
        return (*this)(x, m, a, pT, pD);
    }
};

} /* namespace vicreg_4d */
} /* namespace wikimyei */
} /* namespace cuwacunu */