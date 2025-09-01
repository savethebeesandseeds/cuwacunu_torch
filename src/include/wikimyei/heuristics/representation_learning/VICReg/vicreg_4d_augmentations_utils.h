/* vicreg_4d_augmentations_utils.h */
#pragma once
#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d_types.h"
#include "camahjucunu/BNF/implementations/training_components/training_components.h"
#include "camahjucunu/BNF/implementations/training_components/training_components_utils.h"

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

/* ─────────────────────────────────────────────────────────────
 *  Map string → WarpBaseCurve enum
 * ───────────────────────────────────────────────────────────── */
inline WarpBaseCurve parse_curve(const std::string& s) {
  if (s == "Linear")        return WarpBaseCurve::Linear;
  if (s == "MarketFade")    return WarpBaseCurve::MarketFade;
  if (s == "PulseCentered") return WarpBaseCurve::PulseCentered;
  if (s == "FrontLoaded")   return WarpBaseCurve::FrontLoaded;
  if (s == "FadeLate")      return WarpBaseCurve::FadeLate;
  if (s == "ChaoticDrift")  return WarpBaseCurve::ChaoticDrift;
  throw std::runtime_error("Unknown WarpBaseCurve: " + s);
}

/* ─────────────────────────────────────────────────────────────
 *  Convert a configuration table into WarpPreset vector
 * ─────────────────────────────────────────────────────────────
 * Expected columns per row:
 *   - "curve"                  (string: Linear, MarketFade, etc.)
 *   - "curve_param"            (double)
 *   - "noise_scale"            (double)
 *   - "smoothing_kernel_size"  (long)
 *   - "point_drop_prob"        (double)
 * ───────────────────────────────────────────────────────────── */
inline std::vector<WarpPreset>
make_warp_presets_from_table(const cuwacunu::camahjucunu::BNF::training_instruction_t::table_t& table)
{
  using namespace cuwacunu::camahjucunu;
  
  std::vector<WarpPreset> presets;
  presets.reserve(table.size());
  
  for (const auto& row : table) {
    cuwacunu::camahjucunu::require_columns_exact(row, 
      {ROW_ID_COLUMN_HEADER, "curve", "curve_param", "noise_scale", "smoothing_kernel_size", "point_drop_prob", "comment"});
    try {
      auto curve_str   = require_column(row, "curve");
      auto curve_param = to_double(require_column(row, "curve_param"));
      auto noise_scale = to_double(require_column(row, "noise_scale"));
      auto smoothing   = to_long(require_column(row, "smoothing_kernel_size"));
      auto drop_prob   = to_double(require_column(row, "point_drop_prob"));

      WarpPreset p {
        parse_curve(curve_str),
        curve_param,
        noise_scale,
        smoothing,
        drop_prob
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
