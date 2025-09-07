/* vicreg_4d_augmentations_utils.h */
#pragma once
#include <stdexcept>
#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d_types.h"
#include "camahjucunu/BNF/implementations/training_components/training_components.h"
#include "camahjucunu/BNF/implementations/training_components/training_components_utils.h"

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

/* Map string → enum */
inline WarpBaseCurve parse_curve(const std::string& s) {
  if (s == "Linear")        return WarpBaseCurve::Linear;
  if (s == "MarketFade")    return WarpBaseCurve::MarketFade;
  if (s == "PulseCentered") return WarpBaseCurve::PulseCentered;
  if (s == "FrontLoaded")   return WarpBaseCurve::FrontLoaded;
  if (s == "FadeLate")      return WarpBaseCurve::FadeLate;
  if (s == "ChaoticDrift")  return WarpBaseCurve::ChaoticDrift;
  throw std::runtime_error("Unknown WarpBaseCurve: " + s);
}

/**
 * Convert a configuration table into a vector<WarpPreset>.
 *
 * Required columns (exact):
 *   - "curve"                     (string)
 *   - "curve_param"               (double)
 *   - "noise_scale"               (double)
 *   - "smoothing_kernel_size"     (long, >=1; recommend odd)
 *   - "point_drop_prob"           (double in [0,1])
 *   - "value_jitter_std"          (double, >=0)
 *   - "time_mask_band_frac"       (double in [0,1))
 *   - "channel_dropout_prob"      (double in [0,1])
 *   - "comment"                   (string; kept for documentation)
 *
 * Any missing/malformed value throws with a precise message.
 */
inline std::vector<WarpPreset>
make_warp_presets_from_table(
  const cuwacunu::camahjucunu::training_instruction_t::table_t& table)
{
  using namespace cuwacunu::camahjucunu;

  std::vector<WarpPreset> presets;
  presets.reserve(table.size());

  for (const auto& row : table) {
    // Enforce schema exactly (no silent defaults)
    cuwacunu::camahjucunu::require_columns_exact(row, {
      ROW_ID_COLUMN_HEADER,
      "curve", "curve_param", "noise_scale", "smoothing_kernel_size",
      "point_drop_prob",
      "value_jitter_std", "time_mask_band_frac", "channel_dropout_prob",
      "comment"
    });

    try {
      const auto curve_str   = require_column(row, "curve");
      const auto curve_param = to_double(require_column(row, "curve_param"));
      const auto noise_scale = to_double(require_column(row, "noise_scale"));
      const auto smoothing   = to_long  (require_column(row, "smoothing_kernel_size"));
      const auto drop_prob   = to_double(require_column(row, "point_drop_prob"));

      const auto jitter_std  = to_double(require_column(row, "value_jitter_std"));
      const auto band_frac   = to_double(require_column(row, "time_mask_band_frac"));
      const auto ch_drop     = to_double(require_column(row, "channel_dropout_prob"));

      // ---- Validation (fail fast with clear error messages) ----
      if (smoothing < 1) {
        throw std::runtime_error("'smoothing_kernel_size' must be >= 1");
      }
      if (!(drop_prob >= 0.0 && drop_prob <= 1.0)) {
        throw std::runtime_error("'point_drop_prob' must be in [0,1]");
      }
      if (jitter_std < 0.0) {
        throw std::runtime_error("'value_jitter_std' must be >= 0");
      }
      if (!(band_frac >= 0.0 && band_frac < 1.0)) {
        throw std::runtime_error("'time_mask_band_frac' must be in [0,1)");
      }
      if (!(ch_drop >= 0.0 && ch_drop <= 1.0)) {
        throw std::runtime_error("'channel_dropout_prob' must be in [0,1]");
      }

      // NOTE: we do not silently "round to odd" for smoothing; if you require odd-only,
      //       validate here and ask the user to fix the table.

      WarpPreset p{
        /* curve */               parse_curve(curve_str),
        /* curve_param */         curve_param,
        /* noise_scale */         noise_scale,
        /* smoothing_kernel_size*/smoothing,
        /* point_drop_prob */     drop_prob,
        /* value_jitter_std */    jitter_std,
        /* time_mask_band_frac */ band_frac,
        /* channel_dropout_prob */ch_drop
      };
      presets.push_back(std::move(p));
    } catch (const std::exception& e) {
      throw std::runtime_error(
        std::string("(make_warp_presets_from_table) Failed to parse row: ") + e.what()
      );
    }
  }

  return presets;
}

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
