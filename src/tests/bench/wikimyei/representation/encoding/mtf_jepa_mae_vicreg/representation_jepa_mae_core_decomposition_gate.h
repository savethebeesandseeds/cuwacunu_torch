#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace cuwacunu::tests::mtf_jepa_mae_core_decomposition_gate {

constexpr std::size_t kSeedCount = 3;
constexpr std::size_t kFamilyCount = 4;
constexpr double kMaterialityFloor = 0.0024;
constexpr double kStandaloneNonharmMargin = -0.0024;
constexpr double kFamilyDeltaFloor = -0.02;
constexpr double kBetterReferenceRatioFloor = 0.90;
constexpr double kGapClosureFloor = 0.50;
constexpr double kActiveDimensionFloor = 0.75;

struct PairedContrast {
  double point{0.0};
  double low{0.0};
  double high{0.0};
  int64_t positive_seed_count{0};
};

struct PerSeedGeometry {
  double effective{0.0};
  double participation{0.0};
  double top{0.0};
  double active{0.0};
};

using GeometryBySeed = std::array<PerSeedGeometry, kSeedCount>;
using FamilyDeltas = std::array<double, kFamilyCount>;

struct SingletonInput {
  PairedContrast minus_null{};
  PairedContrast minus_combined{};
  GeometryBySeed geometry{};
  FamilyDeltas minus_null_family{};
  FamilyDeltas minus_combined_family{};
};

struct GateInput {
  PairedContrast combined_minus_null{};
  PairedContrast harmful_interaction_residual{};
  GeometryBySeed null_geometry{};
  GeometryBySeed combined_geometry{};
  SingletonInput jepa{};
  SingletonInput mae{};
  bool null_identity{false};
  bool mechanics{false};
  bool accepted_reference_reproduced{false};
};

struct MaterialContrastResult {
  bool numeric_valid{false};
  bool point_pass{false};
  bool lower_bound_pass{false};
  bool positive_seed_count_pass{false};
  bool pass{false};
};

struct GapResult {
  bool applicable{false};
  bool closure_defined{false};
  double closure{std::numeric_limits<double>::quiet_NaN()};
  bool closure_pass{false};
  int64_t direction_count{0};
  bool direction_pass{false};
  bool pass{false};
};

struct GeometryResult {
  PerSeedGeometry null_mean{};
  PerSeedGeometry combined_mean{};
  PerSeedGeometry candidate_mean{};
  double effective_ratio{std::numeric_limits<double>::quiet_NaN()};
  double participation_ratio{std::numeric_limits<double>::quiet_NaN()};
  double top_ratio{std::numeric_limits<double>::quiet_NaN()};
  bool ratios_defined{false};
  bool effective_ratio_pass{false};
  bool participation_ratio_pass{false};
  bool top_ratio_pass{false};
  GapResult effective_gap{};
  GapResult participation_gap{};
  GapResult top_gap{};
  GapResult active_gap{};
  std::array<bool, kSeedCount> active_seed_pass{};
  double minimum_active{std::numeric_limits<double>::quiet_NaN()};
  bool all_active_seeds_pass{false};
  bool numeric_valid{false};
  bool pass{false};
};

struct SafetyResult {
  std::array<bool, kFamilyCount> minus_null_family_pass{};
  std::array<bool, kFamilyCount> minus_combined_family_pass{};
  bool all_family_deltas_pass{false};
  GeometryResult geometry{};
  bool pass{false};
};

struct SingletonResult {
  MaterialContrastResult standalone_improvement{};
  MaterialContrastResult removal_rescue{};
  bool standalone_nonharm{false};
  SafetyResult safety{};
  bool less_harmful{false};
  bool replacement_supported{false};
};

enum class Classification {
  invalid_numeric_or_mechanics,
  accepted_jepa_mae_reference_not_reproduced,
  harmful_jepa_mae_interaction_supported,
  both_core_branches_conditionally_harmful,
  jepa_conditional_contributor_supported,
  mae_conditional_contributor_supported,
  core_component_marginal_harm_not_localized,
};

[[nodiscard]] inline const char *
classification_name(Classification classification) {
  switch (classification) {
  case Classification::invalid_numeric_or_mechanics:
    return "invalid_numeric_or_mechanics";
  case Classification::accepted_jepa_mae_reference_not_reproduced:
    return "accepted_jepa_mae_reference_not_reproduced";
  case Classification::harmful_jepa_mae_interaction_supported:
    return "harmful_jepa_mae_interaction_supported";
  case Classification::both_core_branches_conditionally_harmful:
    return "both_core_branches_conditionally_harmful";
  case Classification::jepa_conditional_contributor_supported:
    return "jepa_conditional_contributor_supported";
  case Classification::mae_conditional_contributor_supported:
    return "mae_conditional_contributor_supported";
  case Classification::core_component_marginal_harm_not_localized:
    return "core_component_marginal_harm_not_localized";
  }
  return "invalid_numeric_or_mechanics";
}

struct GateResult {
  bool numeric_inputs_valid{false};
  bool mechanics_pass{false};
  bool null_identity_pass{false};
  bool accepted_reference_reproduced{false};
  bool combined_declined_from_null{false};
  SingletonResult jepa{};
  SingletonResult mae{};
  MaterialContrastResult harmful_interaction_residual{};
  bool jepa_conditional_harm{false};
  bool mae_conditional_harm{false};
  bool harmful_interaction{false};
  Classification classification{Classification::invalid_numeric_or_mechanics};
};

namespace detail {

[[nodiscard]] inline bool finite(double value) { return std::isfinite(value); }

[[nodiscard]] inline bool unit_fraction(double value) {
  return finite(value) && value >= 0.0 && value <= 1.0;
}

[[nodiscard]] inline bool valid_contrast(const PairedContrast &contrast) {
  return finite(contrast.point) && finite(contrast.low) &&
         finite(contrast.high) && contrast.low <= contrast.point &&
         contrast.point <= contrast.high && contrast.positive_seed_count >= 0 &&
         contrast.positive_seed_count <= static_cast<int64_t>(kSeedCount);
}

[[nodiscard]] inline bool valid_geometry(const GeometryBySeed &geometry) {
  for (const auto &seed : geometry) {
    if (!unit_fraction(seed.effective) || !unit_fraction(seed.participation) ||
        !unit_fraction(seed.top) || !unit_fraction(seed.active)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool valid_families(const FamilyDeltas &families) {
  return std::all_of(families.begin(), families.end(),
                     [](double value) { return finite(value); });
}

[[nodiscard]] inline PerSeedGeometry
mean_geometry(const GeometryBySeed &geometry) {
  PerSeedGeometry mean{};
  for (const auto &seed : geometry) {
    mean.effective += seed.effective;
    mean.participation += seed.participation;
    mean.top += seed.top;
    mean.active += seed.active;
  }
  constexpr double denominator = static_cast<double>(kSeedCount);
  mean.effective /= denominator;
  mean.participation /= denominator;
  mean.top /= denominator;
  mean.active /= denominator;
  return mean;
}

[[nodiscard]] inline MaterialContrastResult
evaluate_material(const PairedContrast &contrast) {
  MaterialContrastResult result{};
  result.numeric_valid = valid_contrast(contrast);
  result.point_pass =
      result.numeric_valid && contrast.point >= kMaterialityFloor;
  result.lower_bound_pass = result.numeric_valid && contrast.low > 0.0;
  result.positive_seed_count_pass =
      result.numeric_valid && contrast.positive_seed_count >= 2;
  result.pass = result.point_pass && result.lower_bound_pass &&
                result.positive_seed_count_pass;
  return result;
}

using GeometryMember = double PerSeedGeometry::*;

[[nodiscard]] inline GapResult evaluate_high_good_gap(
    const GeometryBySeed &combined, const GeometryBySeed &candidate,
    const PerSeedGeometry &null_mean, const PerSeedGeometry &combined_mean,
    const PerSeedGeometry &candidate_mean, GeometryMember member) {
  GapResult result{};
  result.applicable = combined_mean.*member < null_mean.*member;
  for (std::size_t seed = 0; seed < kSeedCount; ++seed) {
    result.direction_count +=
        candidate[seed].*member > combined[seed].*member ? 1 : 0;
  }
  result.direction_pass = result.direction_count >= 2;
  if (!result.applicable) {
    result.pass = true;
    return result;
  }
  const double denominator = null_mean.*member - combined_mean.*member;
  result.closure =
      (candidate_mean.*member - combined_mean.*member) / denominator;
  result.closure_defined = finite(result.closure);
  result.closure_pass =
      result.closure_defined && result.closure >= kGapClosureFloor;
  result.pass = result.closure_pass && result.direction_pass;
  return result;
}

[[nodiscard]] inline GapResult evaluate_top_gap(
    const GeometryBySeed &combined, const GeometryBySeed &candidate,
    const PerSeedGeometry &null_mean, const PerSeedGeometry &combined_mean,
    const PerSeedGeometry &candidate_mean) {
  GapResult result{};
  result.applicable = combined_mean.top > null_mean.top;
  for (std::size_t seed = 0; seed < kSeedCount; ++seed) {
    result.direction_count += candidate[seed].top < combined[seed].top ? 1 : 0;
  }
  result.direction_pass = result.direction_count >= 2;
  if (!result.applicable) {
    result.pass = true;
    return result;
  }
  const double denominator = combined_mean.top - null_mean.top;
  result.closure = (combined_mean.top - candidate_mean.top) / denominator;
  result.closure_defined = finite(result.closure);
  result.closure_pass =
      result.closure_defined && result.closure >= kGapClosureFloor;
  result.pass = result.closure_pass && result.direction_pass;
  return result;
}

[[nodiscard]] inline GeometryResult
evaluate_geometry(const GeometryBySeed &null_geometry,
                  const GeometryBySeed &combined_geometry,
                  const GeometryBySeed &candidate_geometry) {
  GeometryResult result{};
  result.numeric_valid = valid_geometry(null_geometry) &&
                         valid_geometry(combined_geometry) &&
                         valid_geometry(candidate_geometry);
  if (!result.numeric_valid) {
    return result;
  }
  result.null_mean = mean_geometry(null_geometry);
  result.combined_mean = mean_geometry(combined_geometry);
  result.candidate_mean = mean_geometry(candidate_geometry);
  const double better_effective =
      std::max(result.null_mean.effective, result.combined_mean.effective);
  const double better_participation = std::max(
      result.null_mean.participation, result.combined_mean.participation);
  const double better_top =
      std::min(result.null_mean.top, result.combined_mean.top);
  const double top_denominator = 1.0 - better_top;
  result.ratios_defined = better_effective > 0.0 &&
                          better_participation > 0.0 && top_denominator > 0.0;
  if (result.ratios_defined) {
    result.effective_ratio = result.candidate_mean.effective / better_effective;
    result.participation_ratio =
        result.candidate_mean.participation / better_participation;
    result.top_ratio = (1.0 - result.candidate_mean.top) / top_denominator;
  }
  result.effective_ratio_pass =
      result.ratios_defined && finite(result.effective_ratio) &&
      result.effective_ratio >= kBetterReferenceRatioFloor;
  result.participation_ratio_pass =
      result.ratios_defined && finite(result.participation_ratio) &&
      result.participation_ratio >= kBetterReferenceRatioFloor;
  result.top_ratio_pass = result.ratios_defined && finite(result.top_ratio) &&
                          result.top_ratio >= kBetterReferenceRatioFloor;
  result.effective_gap = evaluate_high_good_gap(
      combined_geometry, candidate_geometry, result.null_mean,
      result.combined_mean, result.candidate_mean, &PerSeedGeometry::effective);
  result.participation_gap = evaluate_high_good_gap(
      combined_geometry, candidate_geometry, result.null_mean,
      result.combined_mean, result.candidate_mean,
      &PerSeedGeometry::participation);
  result.top_gap =
      evaluate_top_gap(combined_geometry, candidate_geometry, result.null_mean,
                       result.combined_mean, result.candidate_mean);
  result.active_gap = evaluate_high_good_gap(
      combined_geometry, candidate_geometry, result.null_mean,
      result.combined_mean, result.candidate_mean, &PerSeedGeometry::active);
  result.minimum_active = 1.0;
  result.all_active_seeds_pass = true;
  for (std::size_t seed = 0; seed < kSeedCount; ++seed) {
    result.active_seed_pass[seed] =
        candidate_geometry[seed].active >= kActiveDimensionFloor;
    result.all_active_seeds_pass =
        result.all_active_seeds_pass && result.active_seed_pass[seed];
    result.minimum_active =
        std::min(result.minimum_active, candidate_geometry[seed].active);
  }
  result.pass = result.effective_ratio_pass &&
                result.participation_ratio_pass && result.top_ratio_pass &&
                result.effective_gap.pass && result.participation_gap.pass &&
                result.top_gap.pass && result.active_gap.pass &&
                result.all_active_seeds_pass;
  return result;
}

[[nodiscard]] inline SafetyResult
evaluate_safety(const FamilyDeltas &minus_null_family,
                const FamilyDeltas &minus_combined_family,
                const GeometryBySeed &null_geometry,
                const GeometryBySeed &combined_geometry,
                const GeometryBySeed &candidate_geometry) {
  SafetyResult result{};
  const bool families_valid = valid_families(minus_null_family) &&
                              valid_families(minus_combined_family);
  result.all_family_deltas_pass = families_valid;
  for (std::size_t family = 0; family < kFamilyCount; ++family) {
    result.minus_null_family_pass[family] =
        families_valid && minus_null_family[family] >= kFamilyDeltaFloor;
    result.minus_combined_family_pass[family] =
        families_valid && minus_combined_family[family] >= kFamilyDeltaFloor;
    result.all_family_deltas_pass = result.all_family_deltas_pass &&
                                    result.minus_null_family_pass[family] &&
                                    result.minus_combined_family_pass[family];
  }
  result.geometry =
      evaluate_geometry(null_geometry, combined_geometry, candidate_geometry);
  result.pass = result.all_family_deltas_pass && result.geometry.pass;
  return result;
}

[[nodiscard]] inline SingletonResult
evaluate_singleton(const SingletonInput &input,
                   const GeometryBySeed &null_geometry,
                   const GeometryBySeed &combined_geometry) {
  SingletonResult result{};
  result.standalone_improvement = evaluate_material(input.minus_null);
  result.removal_rescue = evaluate_material(input.minus_combined);
  result.standalone_nonharm = valid_contrast(input.minus_null) &&
                              input.minus_null.low > kStandaloneNonharmMargin;
  result.safety =
      evaluate_safety(input.minus_null_family, input.minus_combined_family,
                      null_geometry, combined_geometry, input.geometry);
  result.less_harmful =
      result.removal_rescue.pass && !result.standalone_improvement.pass;
  result.replacement_supported = result.standalone_improvement.pass &&
                                 result.removal_rescue.pass &&
                                 result.safety.pass;
  return result;
}

} // namespace detail

[[nodiscard]] inline GateResult evaluate(const GateInput &input) {
  GateResult result{};
  result.jepa = detail::evaluate_singleton(input.jepa, input.null_geometry,
                                           input.combined_geometry);
  result.mae = detail::evaluate_singleton(input.mae, input.null_geometry,
                                          input.combined_geometry);
  result.harmful_interaction_residual =
      detail::evaluate_material(input.harmful_interaction_residual);
  result.numeric_inputs_valid =
      detail::valid_contrast(input.combined_minus_null) &&
      result.jepa.standalone_improvement.numeric_valid &&
      result.jepa.removal_rescue.numeric_valid &&
      result.mae.standalone_improvement.numeric_valid &&
      result.mae.removal_rescue.numeric_valid &&
      result.harmful_interaction_residual.numeric_valid &&
      result.jepa.safety.geometry.numeric_valid &&
      result.mae.safety.geometry.numeric_valid &&
      detail::valid_families(input.jepa.minus_null_family) &&
      detail::valid_families(input.jepa.minus_combined_family) &&
      detail::valid_families(input.mae.minus_null_family) &&
      detail::valid_families(input.mae.minus_combined_family);
  result.mechanics_pass = input.mechanics;
  result.null_identity_pass = input.null_identity;
  result.accepted_reference_reproduced = input.accepted_reference_reproduced;
  result.combined_declined_from_null =
      result.numeric_inputs_valid && input.combined_minus_null.point < 0.0;
  result.jepa_conditional_harm = result.mae.removal_rescue.pass;
  result.mae_conditional_harm = result.jepa.removal_rescue.pass;
  result.harmful_interaction =
      result.harmful_interaction_residual.pass &&
      result.jepa_conditional_harm && result.mae_conditional_harm &&
      result.jepa.standalone_nonharm && result.mae.standalone_nonharm;

  if (!result.numeric_inputs_valid || !result.mechanics_pass ||
      !result.null_identity_pass) {
    result.classification = Classification::invalid_numeric_or_mechanics;
  } else if (!result.accepted_reference_reproduced) {
    result.classification =
        Classification::accepted_jepa_mae_reference_not_reproduced;
  } else if (result.harmful_interaction) {
    result.classification =
        Classification::harmful_jepa_mae_interaction_supported;
  } else if (result.jepa_conditional_harm && result.mae_conditional_harm) {
    result.classification =
        Classification::both_core_branches_conditionally_harmful;
  } else if (result.jepa_conditional_harm) {
    result.classification =
        Classification::jepa_conditional_contributor_supported;
  } else if (result.mae_conditional_harm) {
    result.classification =
        Classification::mae_conditional_contributor_supported;
  } else {
    result.classification =
        Classification::core_component_marginal_harm_not_localized;
  }
  return result;
}

} // namespace cuwacunu::tests::mtf_jepa_mae_core_decomposition_gate
