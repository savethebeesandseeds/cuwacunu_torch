#pragma once
#include <torch/torch.h>
#pragma once
#include <cstdint>
#include <vector>

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
 *  - `smoothing_kernel_size`  : size of 1D smoothing filter applied to noise
 *  - `point_drop_prob`        : per-point random drop in [0,1]
 *  - `value_jitter_std`       : Gaussian std (fraction of scale) >= 0
 *  - `time_mask_band_frac`    : fraction of T to mask as a contiguous band, 0 ≤ f < 1
 *  - `channel_dropout_prob`   : per-channel dropout prob in [0,1]
 * ───────────────────────────────────────────────────────────── */
struct WarpPreset {
  // Time-warp shape
  WarpBaseCurve curve;            // e.g., Linear, MarketFade …
  double        curve_param;      // e.g., steepness or exponent
  double        noise_scale;      // warp jitter amplitude
  int64_t       smoothing_kernel_size; // >= 1 (recommend odd: 1,3,5,7 …)
  double        point_drop_prob;  // per-point random drop in [0,1]
  double        value_jitter_std;     // Gaussian std (fraction of scale) >= 0
  double        time_mask_band_frac;  // fraction of T to mask as a contiguous band, 0 ≤ f < 1
  double        channel_dropout_prob; // per-channel dropout prob in [0,1]
};

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
