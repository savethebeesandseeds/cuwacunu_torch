/* vicreg_4d_Augmentations.h */
#pragma once
#include <torch/torch.h>
#include <limits>   // std::numeric_limits

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

/* ─────────────────────────────────────────────────────────────
 *  Base‑curve selector
 *  
 *  These define the underlying time-warping shape φ(t), which is
 *  sampled at T points and stretched to [0, T−1] before noise/sort.
 *  
 *  All curves are strictly increasing and preserve causality.
 * ───────────────────────────────────────────────────────────── */
enum class WarpBaseCurve {
    Linear,         // φ(t) = t                             → no warp, baseline
    MarketFade,     // φ(t) = sigmoid(s*(t−0.5))            → early time stretched, tail compressed
    PulseCentered,  // φ(t) = 0.5 − 0.5 * cos(2πt)           → central slow-motion, fast ends
    FrontLoaded,    // φ(t) = pow(t, α), α < 1              → early sharp emphasis
    FadeLate,       // φ(t) = 1 − sigmoid(s*(t−0.5))        → fast start, tail expanded
    ChaoticDrift    // φ(t) = t + noise (smoothed, sorted) → random but smooth variation
};

/* ─────────────────────────────────────────────────────────────
 *  WarpPreset structure
 *  
 *  Used to configure a reusable, meaningful time-warp style.
 *  These presets can be randomly sampled during training.
 *
 *  - `curve`                  : the base time perception mode
 *  - `curve_param`            : parameter for the curve (α or steepness s)
 *  - `noise_scale`            : std-dev of Gaussian noise added to curve
 *  - `smoothing_kernel_size` : size of 1D smoothing filter applied to noise
 * ───────────────────────────────────────────────────────────── */
struct WarpPreset {
    WarpBaseCurve curve;
    double curve_param;
    double noise_scale;
    int64_t smoothing_kernel_size;
    double point_drop_prob;
};

/* ─────────────────────────────────────────────────────────────
 *  Recommended warp map presets (subtle but meaningful)
 *  These can be used to sample randomized warp_maps for data augmentation
 *  or time-invariance training.
 * ───────────────────────────────────────────────────────────── */
inline std::vector<WarpPreset> warp_presets = {
    /* curve                            curve_param     noise_scale     smoothing_kernel_size       point_drop_prob */
    { WarpBaseCurve::Linear,            0.0,            0.0,            1,                          0.03 },                 // Identity: no warp, ideal for control comparisons
    { WarpBaseCurve::Linear,            0.0,            0.0,            5,                          0.03 },                 // Natural Drift: small random drift, gently smoothed
    { WarpBaseCurve::ChaoticDrift,      0.0,            0.0,            7,                          0.03 },                 // Chaotic Drift: noisier structure, strongly smoothed for realism
    { WarpBaseCurve::MarketFade,        3.0,            0.0,            5,                          0.03 },                 // Market Fade (soft): early emphasis, gentle fade out
    { WarpBaseCurve::MarketFade,        5.0,            0.0,            7,                          0.03 },                 // Market Fade (sharp): stronger front focus, softer tail
    { WarpBaseCurve::FadeLate,          3.0,            0.0,            5,                          0.03 },                 // Fade Late: mirror of market fade, tail-focused
    { WarpBaseCurve::PulseCentered,     0.0,            0.0,            5,                          0.03 },                 // Pulse Centered: emphasizes central events in time
    { WarpBaseCurve::FrontLoaded,       0.6,            0.0,            3,                          0.03 },                 // Front-Focus (soft): mild early emphasis, fast decay
    { WarpBaseCurve::FrontLoaded,       0.3,            0.0,            5,                          0.03 },                 // Front-Focus (sharp): stronger focus on initial time
    { WarpBaseCurve::PulseCentered,     0.0,            0.0,            7,                          0.03 }                  // Symmetric Sway: fluid oscillation centered on mid-sequence
};

/*  -----------------------------------------------------------
 *  causal_time_warp.h
 *  Warp a [B,C,T,E] tensor along its temporal axis with a per‑sample strictly
 *  increasing map.  Uses hard‑mask semantics: if either source index is
 *  invalid the interpolated point is marked invalid and set to NaN.
 *  ----------------------------------------------------------- */
 
 /**
  *  @param  x         [B,C,T,E]  – batch of time‑series tensors
  *  @param  m         [B,C,T]  – matching Boolean mask (true = valid)
  *  @param  warp_map  [B,T]      – for every sample b and output step t,
  *                                warp_map[b,t] ∈ [0,T‑1] is the (fractional)
  *                                source index inside x’s original time axis.
  *                                Each row MUST be strictly increasing.
  *  @return           [B,C,T,E]  – time‑warped tensor
  */
inline std::pair<torch::Tensor, torch::Tensor>
causal_time_warp(const torch::Tensor& x,
                const torch::Tensor& m,
                const torch::Tensor& warp_map)
{
    /* ─── basic checks ─────────────────────────────────────────────── */
    TORCH_CHECK(x.dim()==4,                     "(vicreg_4d_Augmentations.h)[casual_time_wrap] data must be [B,C,T,E]");
    TORCH_CHECK(m.dim()==3,                     "(vicreg_4d_Augmentations.h)[casual_time_wrap] mask must be [B,C,T]");
    TORCH_CHECK(m.sizes().slice(0,3) == x.sizes().slice(0,3).slice(0,3), "(vicreg_4d_Augmentations.h)[casual_time_wrap] mask must match data in B,C,T dims");
    TORCH_CHECK(warp_map.dim()==2,              "(vicreg_4d_Augmentations.h)[casual_time_wrap] warp_map must be [B,T]");
    TORCH_CHECK(x.size(0)==warp_map.size(0),    "(vicreg_4d_Augmentations.h)[casual_time_wrap] batch mismatch");
    TORCH_CHECK(x.device()==warp_map.device() 
                && m.device()==warp_map.device(),  "(vicreg_4d_Augmentations.h)[casual_time_wrap] data, mask and warp_map must be on the same device");
                
    /* ─── monotonicity assertion (debug) ───────────────────────────── */
    TORCH_CHECK((warp_map.diff(1, /*dim=*/1) >= 0).all().item<bool>(), "(vicreg_4d_Augmentations.h)[casual_time_wrap] warp_map must be strictly increasing");
    
    /* ─── 1. indices & weights ─────────────────────────────────────── */
    const int64_t B = x.size(0);
    const int64_t C = x.size(1);
    const int64_t T = x.size(2);
    const int64_t E = x.size(3);

    auto w    = torch::clamp(warp_map, 0, T - 1 - 1e-6);   // avoid ceil overflow
    auto i0   = w.floor().to(torch::kLong);                // ⌊w⌋ int64
    auto i1   = (i0 + 1).to(torch::kLong);                 // ⌈w⌉ int64
    auto a    = (w - i0.to(w.dtype()))
                .unsqueeze(1)            // [B,1,T]
                .unsqueeze(-1);          // [B,1,T,1]

    auto expand4D = [&](const torch::Tensor& t){
        return t.view({B,1,T,1}).expand({B,C,T,E});
    };
    auto expand3D = [&](const torch::Tensor& t){
        return t.view({B,1,T}).expand({B,C,T});
    };

    /* ─── 2. gather data & masks ───────────────────────────────────── */
    auto x0 = x.gather(2, expand4D(i0));
    auto x1 = x.gather(2, expand4D(i1));

    auto m0 = m.gather(2, expand3D(i0));   // [B,C,T] bool
    auto m1 = m.gather(2, expand3D(i1));   // [B,C,T] bool
    auto valid = m0 & m1;                  // [B,C,T] hard AND

    /* ─── 3. linear blend ──────────────────────────────────────────── */
    auto y = x0 + a * (x1 - x0);           // [B,C,T,E]

    /* ─── 4. apply hard mask (NaN fill) ────────────────────────────── */
    const auto nan_val = (x.dtype() == torch::kFloat32)
                        ? std::numeric_limits<float >::quiet_NaN()
                        : std::numeric_limits<double>::quiet_NaN();

    auto valid4D = valid.unsqueeze(-1).expand({B,C,T,E});  // broadcast to E
    y.masked_fill_(~valid4D, nan_val);

    return { y, valid };                                   // [B,C,T,E] , [B,C,T]
}

/**
 *  @brief  Build a causality-preserving warp map with controllable time perception.
 *
 *  This function constructs a tensor warp_map ∈ ℝ^{B×T}, where each row defines
 *  a smooth, strictly increasing temporal reparameterization — a "time warp" — for
 *  one sample in the batch. Each warp map distorts the time axis without folding or
 *  reversing it, enabling models to learn representations that are robust to changes
 *  in temporal pacing while preserving causality.
 *
 *  The warp is generated in several steps:
 *      1. A normalized base curve φ(t) ∈ [0,1] is selected according to the WarpBaseCurve enum.
 *      2. The curve is scaled to span [0, T−1], then repeated across the batch.
 *      3. Gaussian perturbations are added (if applicable).
 *      4. Optional smoothing is applied via 1D convolution.
 *      5. Endpoints are fixed to ensure φ(0)=0 and φ(T−1)=T−1 (full coverage).
 *      6. The curve is sorted to enforce strict monotonicity (no time reversal).
 *      7. A final rescale ensures consistent range even under distortion.
 *
 *  Supported base curve modes (via WarpBaseCurve):
 *      - Linear         : φ(t) = t                             → No warp (default).
 *      - MarketFade     : φ(t) = sigmoid(s*(t−0.5))            → Early attention, fast tail.
 *      - PulseCentered  : φ(t) = 0.5 − 0.5·cos(2πt)            → Slow center, fast ends.
 *      - FrontLoaded    : φ(t) = t^α (α < 1)                   → Sharp early focus.
 *      - FadeLate       : φ(t) = 1 − sigmoid(s*(t−0.5))        → Fast start, slow ending.
 *      - ChaoticDrift   : φ(t) = t + noise                    → Noise-dominated variation.
 *
 *  The final warp_map can be used to perform interpolation-based warping of
 *  time-series tensors (e.g., [B, C, T, E]) while ensuring that all transformations
 *  respect the natural forward progression of time.
 *
 *  @param  B                      Batch size.
 *  @param  T                      Sequence length.
 *  @param  noise_scale            Amplitude of Gaussian perturbations.
 *  @param  smoothing_kernel_size  Width of smoothing kernel (≥1). 1 disables smoothing.
 *  @param  device                 Device on which to construct the tensor.
 *  @param  curve                  Base time perception curve (default: Linear).
 *  @param  curve_param            Curve parameter (steepness or exponent), depending on mode.
 *
 *  @return torch::Tensor          A contiguous warp_map tensor of shape [B, T] (float32).
 */
inline torch::Tensor build_warp_map(int64_t        B,
                                    int64_t        T,
                                    double         noise_scale            = 0.2,
                                    int64_t        smoothing_kernel_size  = 5,
                                    torch::Dtype   dtype                  = torch::kFloat32,
                                    torch::Device  device                 = torch::kCPU,
                                    WarpBaseCurve  curve                  = WarpBaseCurve::Linear,
                                    double         curve_param            = 4.0)    // used as s or α
{
    TORCH_CHECK(B > 0 && T > 1, "(vicreg_4d_Augmentations.h)[build_warp_map] B > 0, T > 1 required");

    auto opts = torch::TensorOptions().dtype(dtype).device(device);

    /* 1. Create base curve φ(t)  ∈ [0,1] ---------------------------------- */
    auto t_norm = torch::linspace(0, 1, T, opts);                // [T]
    torch::Tensor base;

    switch (curve) {
        case WarpBaseCurve::Linear: {
            base = t_norm;
        }   break;

        case WarpBaseCurve::MarketFade: {
            double s = curve_param;  // steepness
            base = torch::sigmoid(s * (t_norm - 0.5));
        }   break;

        case WarpBaseCurve::PulseCentered: {
            base = 0.5 - 0.5 * torch::cos(2 * M_PI * t_norm);                  
        }   break;

        case WarpBaseCurve::FrontLoaded: {
            double a = curve_param;  // alpha exponent (<1)
            base = torch::pow(t_norm, a);
        }   break;

        case WarpBaseCurve::FadeLate: {
            double s = curve_param;
            base = 1 - torch::sigmoid(s * (t_norm - 0.5));
        }   break;

        case WarpBaseCurve::ChaoticDrift: {
            base = t_norm + noise_scale * torch::randn({T}, opts);
        }   break;
    }

    /* 2. Scale base curve to [0, T‑1]  & expand to batch ------------------ */
    base = (base - base.min()) / (base.max() - base.min() + 1e-6) * (T - 1);   // [T]
    auto warp = base.unsqueeze(0).repeat({B, 1});                              // [B,T]

    /* 3. Add Gaussian perturbations (skip for Chaotic which already has) -- */
    if (noise_scale != 0.0 && curve != WarpBaseCurve::ChaoticDrift) {
        warp += noise_scale * torch::randn({B, T}, opts);
    }

    /* 4. Optional temporal smoothing -------------------------------------- */
    if (smoothing_kernel_size > 1) {
        int64_t k = smoothing_kernel_size;
        auto kernel = torch::ones({1, 1, k}, opts) / static_cast<float>(k);
        int64_t pad = k / 2;
        warp = torch::conv1d(warp.unsqueeze(1), kernel, /*bias=*/{}, 1, pad)
                     .squeeze(1);                                             // [B,T]
    }

    /* 5. Fix endpoints ----------------------------------------------------- */
    warp.index_put_({torch::indexing::Slice(), 0},      0.0f);
    warp.index_put_({torch::indexing::Slice(), T - 1}, static_cast<float>(T - 1));

    /* 6. Enforce strict monotonicity via sort ----------------------------- */
    auto sorted = std::get<0>(warp.sort(/*dim=*/1));                          // [B,T]

    /* 7. Rescale (guards against noise pulling extremes inward) ----------- */
    auto min_vals = sorted.index({torch::indexing::Slice(), 0}).unsqueeze(1);
    auto max_vals = sorted.index({torch::indexing::Slice(), T - 1}).unsqueeze(1);
    constexpr float eps = 1e-6f;
    auto warp_map = (sorted - min_vals) / (max_vals - min_vals + eps) * (T - 1);

    return warp_map.contiguous();                                             // [B,T]
}

/**
 * @brief Randomly drops points from a boolean mask tensor with a given probability.
 *        Only points that are initially `true` can be dropped to `false`.
 *        Existing `false` points remain unchanged.
 *
 * @param m     The input mask tensor of shape [B, C, T], must be torch::kBool.
 * @param prob  Probability of dropping each valid (`true`) point (0.0 to 1.0).
 *
 * @return torch::Tensor  A new tensor of shape [B, C, T], with random drops applied.
 */
 inline torch::Tensor random_point_drop(const torch::Tensor& m, double prob) {
    TORCH_CHECK(m.dim() == 3, "(vicreg_4d_Augmentations.h)[random_point_drop] Input mask must be a 3D tensor of shape [B, C, T]");
    TORCH_CHECK(m.dtype() == torch::kBool, "(vicreg_4d_Augmentations.h)[random_point_drop] Input mask must be of type torch::kBool");
    TORCH_CHECK(prob >= 0.0 && prob <= 1.0, "(vicreg_4d_Augmentations.h)[random_point_drop] Probability must be between 0.0 and 1.0");

    // Create random dropout mask ONLY for points that are currently true
    auto random_drop = torch::bernoulli(torch::full_like(m, 1.0 - prob, torch::kFloat32)).to(torch::kBool);

    // Points initially false remain false; points initially true may become false randomly
    return m & random_drop;
}
 
/* ========================================================================
 *  VICReg_4D_Augmentation
 *
 *  Purpose:
 *    Applies causal temporal augmentations to 4D time-series tensors for
 *    self-supervised learning, particularly suited to models like VICReg
 *    operating on structured sequences with shape [B, C, T, D].
 *
 *  Overview:
 *    - Applies smooth, causality-preserving time warps using a parameterized
 *      base curve and stochastic perturbations.
 *    - Applies random masking over space-time points to enforce robustness
 *      in the learned representation.
 *
 *  Assumptions:
 *    - Input tensor `x` contains the data with shape [Batch, Channels, Time, Depth].
 *    - Input tensor `m` contains the corresponding mask with the same shape.
 *    - A global table of `warp_presets` provides named temporal warp profiles.
 *
 *  Key Components:
 *    - `operator()`     : applies a user-defined `WarpPreset` to generate warped data and mask.
 *    - `augment()`      : randomly samples one preset from `warp_presets` for stochastic augmentation.
 *
 * ======================================================================== */
 struct VICReg_4D_Augmentation {

    /* -----------------------------------------------------------------------
     * operator()
     *
     * Applies a warp and masking transformation to the input tensors using a
     * specified `WarpPreset`. This allows explicit control over the temporal
     * augmentation curve used (e.g., MarketFade, PulseCentered, etc).
     *
     * @param x       Input data tensor [B, C, T, D]
     * @param m       Input mask tensor [B, C, T]
     * @param preset  WarpPreset defining base curve, noise, and smoothing
     *
     * @return        Pair of (warped data, warped mask)
     * ----------------------------------------------------------------------- */
    std::pair<torch::Tensor, torch::Tensor>
    operator()(const torch::Tensor& x, 
               const torch::Tensor& m,
               const WarpPreset& preset) const {

        // Extract dimensions
        const int64_t B = x.size(0);  // Batch
        const int64_t T = x.size(2);  // Time

        // Generate a per-sample warp map with the selected preset
        auto warp_map = build_warp_map(
            B, T,
            preset.noise_scale,
            preset.smoothing_kernel_size,
            x.scalar_type(), 
            x.device(),
            preset.curve,
            preset.curve_param
        );
        
        // Apply interpolation (e.g., causal_time_warp) to x and m using warp_map
        auto [data_timewrap, mask_timewrap] = causal_time_warp(x, m, warp_map);
        
        // Apply random masking
        auto mask_drop = random_point_drop(mask_timewrap, preset.point_drop_prob);

        return {data_timewrap, mask_drop};
    }

    /* ---------------------------------------------------------------------
     * augment()
     *
     * Performs randomized augmentation by sampling a warp style from the
     * global `warp_presets` table and applying it to the input.
     *
     * @param x   Input data tensor [B, C, T, D]
     * @param m   Input mask tensor [B, C, T]
     *
     * @return    Pair of (augmented data, augmented mask)
     * --------------------------------------------------------------------- */
    inline std::pair<torch::Tensor, torch::Tensor>
    augment(torch::Tensor x, torch::Tensor m, const WarpPreset& preset) const {
        return (*this)(x, m, preset);
    }
    inline std::pair<torch::Tensor, torch::Tensor>
    augment(torch::Tensor x, torch::Tensor m) const {
        auto preset = warp_presets[rand() % warp_presets.size()];
        return augment(x, m, preset);
    }
};


} /* namespace vicreg_4d */
} /* namespace wikimyei */
} /* namespace cuwacunu */