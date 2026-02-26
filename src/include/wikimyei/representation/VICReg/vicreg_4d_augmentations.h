/* vicreg_4d_augmentations.h */
#pragma once

#include <torch/torch.h>
#include "wikimyei/representation/VICReg/vicreg_4d_types.h"
#include "wikimyei/representation/VICReg/vicreg_4d_augmentations_utils.h"

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

/* ─────────────────────────────────────────────────────────────
 *  Default warp map presets
 *  These can be used to sample randomized warp_maps for data augmentation
 *  or time-invariance training.
 * ───────────────────────────────────────────────────────────── */
inline const std::vector<WarpPreset>& kDefaultWarpPresets() {
  static const std::vector<WarpPreset> kDefaults = {
    // curve,         param, noise, smooth, drop,  jitter,  band,  ch_drop
    { WarpBaseCurve::Linear,        0.0,  0.02,   3,     0.06,  0.015, 0.00, 0.00 },
    { WarpBaseCurve::Linear,        0.0,  0.06,   5,     0.06,  0.015, 0.00, 0.00 },
    { WarpBaseCurve::ChaoticDrift,  0.0,  0.10,   7,     0.08,  0.020, 0.03, 0.05 },
    { WarpBaseCurve::MarketFade,    3.0,  0.02,   5,     0.08,  0.015, 0.00, 0.03 },
    { WarpBaseCurve::MarketFade,    5.0,  0.03,   7,     0.08,  0.015, 0.05, 0.03 },
    { WarpBaseCurve::FadeLate,      3.0,  0.02,   5,     0.08,  0.015, 0.00, 0.03 },
    { WarpBaseCurve::PulseCentered, 0.0,  0.02,   5,     0.06,  0.015, 0.03, 0.00 },
    { WarpBaseCurve::FrontLoaded,   0.6,  0.03,   3,     0.08,  0.020, 0.00, 0.05 },
    { WarpBaseCurve::FrontLoaded,   0.3,  0.03,   5,     0.08,  0.020, 0.00, 0.05 },
    { WarpBaseCurve::PulseCentered, 0.0,  0.04,   7,     0.08,  0.020, 0.05, 0.03 }
  };
  return kDefaults;
}

/*  -----------------------------------------------------------
 *  causal_time_warp.h
 *  Warp a [B,C,T,E] tensor along its temporal axis with a per‑sample strictly
 *  increasing map.  Uses hard‑mask semantics: if either source index is
 *  invalid the interpolated point is marked invalid and set to NaN.
 *  ----------------------------------------------------------- */
 
 /**
  *  @param  x         [B,C,T,E]  – batch of time‑series tensors
  *  @param  m         [B,C,T]    – matching Boolean mask (true = valid)
  *  @param  warp_map  [B,T]      – for every sample b and output step t,
  *                                warp_map[b,t] ∈ [0,T‑1] is the (fractional)
  *                                source index inside x’s original time axis.
  *                                Each row MUST be strictly increasing.
  *  @param dtype       - torch type
  *  @param device      - device, cpu or cuda
  *  @return           [B,C,T,E]  – time‑warped tensor
  */
inline std::pair<torch::Tensor, torch::Tensor>
causal_time_warp(const torch::Tensor& x,
                const torch::Tensor& m,
                const torch::Tensor& warp_map, 
                torch::Dtype   dtype                  = torch::kFloat32,
                torch::Device  device                 = torch::kCPU)
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
    TORCH_CHECK((warp_map.diff(1, /*dim=*/1) > 0).all().item<bool>(),"(vicreg_4d_Augmentations.h)[causal_time_warp] warp_map must be strictly increasing");
    
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
    const auto zero_val = (x.dtype() == torch::kFloat32) ? 0.0f : 0.0;

    auto valid4D = valid.unsqueeze(-1).expand({B,C,T,E});  // broadcast to E
    y.masked_fill_(~valid4D, zero_val);

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

    /* 5) Ensure strictly positive steps then integrate (monotonicity without sorting) */
    constexpr float kMinStep = 1e-3f;
    auto diffs     = torch::diff(warp, /*n=*/1, /*dim=*/1);           // [B,T-1]
    auto pos_diffs = torch::relu(diffs) + kMinStep;                   // enforce >0
    auto first     = warp.index({torch::indexing::Slice(), 0}).unsqueeze(1); // [B,1]
    auto warp_mono = torch::cat({ first, first + pos_diffs.cumsum(/*dim=*/1) }, /*dim=*/1); // [B,T]

    /* 6) Rescale to [0, T-1] and lock the endpoints */
    constexpr float eps = 1e-6f;
    auto min_vals = warp_mono.index({torch::indexing::Slice(), 0}).unsqueeze(1);
    auto max_vals = warp_mono.index({torch::indexing::Slice(), T - 1}).unsqueeze(1);
    auto warp_map = (warp_mono - min_vals) / (max_vals - min_vals + eps) * (T - 1);

    warp_map.index_put_({torch::indexing::Slice(), 0},      0.0f);
    warp_map.index_put_({torch::indexing::Slice(), T - 1}, static_cast<float>(T - 1));
    return warp_map.contiguous();                                          // [B,T]
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
  std::vector<WarpPreset> warp_presets;

  VICReg_4D_Augmentation()
  : warp_presets(kDefaultWarpPresets()) {}

  explicit VICReg_4D_Augmentation(
      const cuwacunu::camahjucunu::jkimyei_specs_t::table_t& table)
  : warp_presets(cuwacunu::wikimyei::vicreg_4d::make_warp_presets_from_table(table)) {}

  std::pair<torch::Tensor, torch::Tensor>
  operator()(const torch::Tensor& x,
             const torch::Tensor& m,
             const WarpPreset& preset) const
  {
    // Basic dims
    const int64_t B = x.size(0);
    const int64_t C = x.size(1);
    const int64_t T = x.size(2);
    const int64_t E = x.size(3);

    // 1) Build monotone warp map (you already implemented the cumsum monotone fix)
    auto warp_map = build_warp_map(
      B, T,
      preset.noise_scale,
      preset.smoothing_kernel_size,
      x.scalar_type(),
      x.device(),
      preset.curve,
      preset.curve_param
    );

    // 2) Time-warp (hard-mask semantics inside)
    auto [data_timewrap, mask_timewrap] = causal_time_warp(
      x, m, warp_map, x.scalar_type(), x.device()
    );

    // 3) Value jitter on valid points only
    if (preset.value_jitter_std > 0.0) {
      auto noise = torch::empty_like(data_timewrap).normal_(0.0, preset.value_jitter_std);
      auto valid4D_f = mask_timewrap.to(noise.dtype()).unsqueeze(-1).expand({B,C,T,E});
      data_timewrap.add_(noise.mul_(valid4D_f));
    }

    // 4) Optional temporal band mask (SpecAugment-style)
    if (preset.time_mask_band_frac > 0.0) {
      const int64_t band = std::max<int64_t>(1, static_cast<int64_t>(std::llround(T * preset.time_mask_band_frac)));
      if (band < T) {
        auto starts = torch::randint(/*high=*/T - band + 1, {B},
                        mask_timewrap.options().dtype(torch::kLong).device(torch::kCPU));
        for (int64_t b = 0; b < B; ++b) {
          const int64_t s = starts[b].item<int64_t>();
          mask_timewrap.index_put_({b, torch::indexing::Slice(), torch::indexing::Slice(s, s + band)}, false);
          data_timewrap.index_put_({b, torch::indexing::Slice(), torch::indexing::Slice(s, s + band), torch::indexing::Slice()}, 0.0);
        }
      } else {
        // If band == T, masking all time would leave no valid samples; we avoid that by requiring band < T in the parser.
        TORCH_CHECK(false, "[VICReg_4D_Augmentation] time_mask_band_frac leads to band==T; adjust config.");
      }
    }

    // 5) Optional channel dropout (per-sample)
    if (preset.channel_dropout_prob > 0.0) {
      // keep ∈ {0,1}; broadcast across T,E for data; across T for mask
      auto keep = torch::bernoulli(
                    torch::full({B, C, 1, 1},
                                1.0 - preset.channel_dropout_prob,
                                data_timewrap.options().dtype(torch::kFloat)))
                      .to(torch::kBool);               // [B, C, 1, 1]

      // Zero out dropped channels in data
      data_timewrap = data_timewrap.masked_fill(~keep, 0.0);  // [B, C, T, E] & [B, C, 1, 1]

      // Propagate channel drop into the time mask (broadcast across T)
      auto keep_bc1 = keep.squeeze(-1);                       // [B, C, 1]
      mask_timewrap = mask_timewrap & keep_bc1;               // [B, C, T] & [B, C, 1] → [B, C, T]
    }

    // 6) Random point drop (existing op)
    auto mask_drop = random_point_drop(mask_timewrap, preset.point_drop_prob);

    return { data_timewrap, mask_drop };
  }

  // Sample a preset using the torch RNG so torch::manual_seed controls it.
  inline std::pair<torch::Tensor, torch::Tensor>
  augment(torch::Tensor x, torch::Tensor m, const std::vector<WarpPreset>& conf_presets) const {
    TORCH_CHECK(!conf_presets.empty(), "[VICReg_4D_Augmentation] no presets configured");
    auto idx_t = torch::randint(static_cast<long>(conf_presets.size()), {1},
                    torch::TensorOptions().dtype(torch::kLong).device(torch::kCPU));
    size_t idx = static_cast<size_t>(idx_t.item<int64_t>());
    return (*this)(x, m, conf_presets[idx]);
  }

  inline std::pair<torch::Tensor, torch::Tensor>
  augment(torch::Tensor x, torch::Tensor m) const {
    return augment(x, m, warp_presets);
  }
};

} /* namespace vicreg_4d */
} /* namespace wikimyei */
} /* namespace cuwacunu */