/* vicreg_4d_augmentations_utils.h */
#pragma once
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include "wikimyei/representation/VICReg/vicreg_4d_types.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs_utils.h"

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
 * Required columns:
 *   - "time_warp_curve"           (string; legacy alias: "name")
 *   - "active"                    (bool)
 *   - "curve_param"               (double)
 *   - "noise_scale"               (double)
 *   - "smoothing_kernel_size"     (long, >=1; recommend odd)
 *   - "point_drop_prob"           (double in [0,1])
 *   - "value_jitter_std"          (double, >=0)
 *   - "time_mask_band_frac"       (double in [0,1))
 *   - "channel_dropout_prob"      (double in [0,1])
 *
 * Optional metadata columns:
 *   - "component_id"
 *   - "component_type"
 *   - "profile_id"
 *   - "profile_row_id"
 *   - "comment"
 *
 * Any missing/malformed value throws with a precise message.
 */
inline std::vector<WarpPreset>
make_warp_presets_from_table(
  const cuwacunu::camahjucunu::jkimyei_specs_t::table_t& table)
{
  using namespace cuwacunu::camahjucunu;

  std::vector<WarpPreset> presets;
  presets.reserve(table.size());

  for (const auto& row : table) {
    // Enforce required keys while allowing stable metadata fields from jkimyei materialization.
    const std::unordered_set<std::string> allowed_columns{
        ROW_ID_COLUMN_HEADER,
        "component_id",
        "component_type",
        "profile_id",
        "profile_row_id",
        "time_warp_curve",
        "name",
        "active",
        "curve_param",
        "noise_scale",
        "smoothing_kernel_size",
        "point_drop_prob",
        "value_jitter_std",
        "time_mask_band_frac",
        "channel_dropout_prob",
        "comment"};
    std::vector<std::string> missing_columns{};
    if (!row.count("time_warp_curve") && !row.count("name")) {
      missing_columns.emplace_back("time_warp_curve");
    }
    for (const char* key : {"active",
                            "curve_param",
                            "noise_scale",
                            "smoothing_kernel_size",
                            "point_drop_prob",
                            "value_jitter_std",
                            "time_mask_band_frac",
                            "channel_dropout_prob"}) {
      if (!row.count(key)) missing_columns.emplace_back(key);
    }
    std::vector<std::string> extra_columns{};
    for (const auto& kv : row) {
      if (!allowed_columns.count(kv.first)) extra_columns.push_back(kv.first);
    }
    if (!missing_columns.empty() || !extra_columns.empty()) {
      std::ostringstream miss;
      std::ostringstream extra;
      for (std::size_t i = 0; i < missing_columns.size(); ++i) {
        if (i != 0) miss << ", ";
        miss << missing_columns[i];
      }
      for (std::size_t i = 0; i < extra_columns.size(); ++i) {
        if (i != 0) extra << ", ";
        extra << extra_columns[i];
      }
      RAISE_FATAL_ROW(row,
                      "Column set mismatch. Missing: [%s]. Unexpected: [%s].",
                      miss.str().c_str(),
                      extra.str().c_str());
    }

    try {
      const bool has_curve_name = row.count("time_warp_curve") != 0;
      const bool has_legacy_name = row.count("name") != 0;
      if (has_curve_name && has_legacy_name &&
          require_column(row, "time_warp_curve") != require_column(row, "name")) {
        throw std::runtime_error(
            "'time_warp_curve' and legacy alias 'name' disagree");
      }
      const auto curve_name = has_curve_name
                                  ? require_column(row, "time_warp_curve")
                                  : require_column(row, "name");
      const auto active = to_bool(require_column(row, "active"));
      if (!active) continue;
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
        /* curve */               parse_curve(curve_name),
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

  if (presets.empty()) {
    throw std::runtime_error(
      "(make_warp_presets_from_table) no active augmentation rows found");
  }

  return presets;
}

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
