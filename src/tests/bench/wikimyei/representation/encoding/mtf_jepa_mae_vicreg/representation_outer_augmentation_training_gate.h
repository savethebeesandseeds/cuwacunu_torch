#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace cuwacunu::tests::mtf_outer_augmentation_training_gate {

constexpr std::size_t kSeedCount = 3;
constexpr std::size_t kFamilyCount = 4;
constexpr double kMaterialityFloor = 0.0024;
constexpr double kNeutralNoninferiorityMargin = -0.005;
constexpr double kFamilyDeltaFloor = -0.02;
constexpr double kBetterReferenceRatioFloor = 0.90;
constexpr double kActiveGapClosureFloor = 0.50;
constexpr double kCandidateActiveDimensionFloor = 0.75;

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

struct OuterAugmentationTrainingGateInput {
  PairedContrast qualified_minus_full_active{};
  PairedContrast qualified_minus_neutral{};
  PairedContrast full_active_minus_neutral{};
  GeometryBySeed neutral_geometry{};
  GeometryBySeed full_active_geometry{};
  GeometryBySeed qualified_geometry{};
  FamilyDeltas qualified_minus_full_active_family{};
  FamilyDeltas qualified_minus_neutral_family{};
  bool mechanics{false};
};

enum class Classification {
  invalid_numeric_or_mechanics,
  qualified_candidate_representation_improvement_supported,
  qualified_candidate_harm_mitigation_only,
  qualified_candidate_neutral_aulc_improvement_only_replacement_not_supported,
  qualified_candidate_not_supported,
};

[[nodiscard]] inline const char *
classification_name(Classification classification) {
  switch (classification) {
  case Classification::invalid_numeric_or_mechanics:
    return "invalid_numeric_or_mechanics";
  case Classification::qualified_candidate_representation_improvement_supported:
    return "qualified_candidate_representation_improvement_supported";
  case Classification::qualified_candidate_harm_mitigation_only:
    return "qualified_candidate_harm_mitigation_only";
  case Classification::
      qualified_candidate_neutral_aulc_improvement_only_replacement_not_supported:
    return "qualified_candidate_neutral_aulc_improvement_only_replacement_not_"
           "supported";
  case Classification::qualified_candidate_not_supported:
    return "qualified_candidate_not_supported";
  }
  return "invalid_numeric_or_mechanics";
}

struct ActiveGapRepairResult {
  bool applicable{false};
  bool closure_defined{false};
  double closure{std::numeric_limits<double>::quiet_NaN()};
  bool closure_finite{false};
  bool closure_pass{false};
  int64_t repair_direction_count{0};
  bool repair_direction_pass{false};
  bool pass{false};
};

struct GeometryGateResult {
  PerSeedGeometry neutral_mean{};
  PerSeedGeometry full_active_mean{};
  PerSeedGeometry qualified_mean{};

  double better_effective_reference{
      std::numeric_limits<double>::quiet_NaN()};
  double better_participation_reference{
      std::numeric_limits<double>::quiet_NaN()};
  double better_top_reference{std::numeric_limits<double>::quiet_NaN()};
  double effective_denominator{std::numeric_limits<double>::quiet_NaN()};
  double participation_denominator{
      std::numeric_limits<double>::quiet_NaN()};
  double top_denominator{std::numeric_limits<double>::quiet_NaN()};
  bool effective_denominator_positive{false};
  bool participation_denominator_positive{false};
  bool top_denominator_positive{false};
  bool all_better_reference_denominators_positive{false};

  double effective_ratio{std::numeric_limits<double>::quiet_NaN()};
  double participation_ratio{std::numeric_limits<double>::quiet_NaN()};
  double top_ratio{std::numeric_limits<double>::quiet_NaN()};
  bool effective_ratio_defined{false};
  bool participation_ratio_defined{false};
  bool top_ratio_defined{false};
  bool all_better_reference_ratios_defined{false};
  bool required_ratios_finite{false};
  bool effective_ratio_pass{false};
  bool participation_ratio_pass{false};
  bool top_ratio_pass{false};
  bool all_better_reference_ratios_pass{false};

  ActiveGapRepairResult effective_gap{};
  ActiveGapRepairResult participation_gap{};
  ActiveGapRepairResult top_gap{};
  ActiveGapRepairResult active_gap{};
  bool required_gap_closures_finite{false};
  bool all_active_gap_repairs_pass{false};

  std::array<bool, kSeedCount> candidate_active_seed_pass{};
  double candidate_min_active{std::numeric_limits<double>::quiet_NaN()};
  bool all_candidate_active_pass{false};

  bool numeric_valid{false};
  bool pass{false};
};

struct OuterAugmentationTrainingGateResult {
  bool basic_numeric_inputs_valid{false};
  bool numeric_inputs_valid{false};
  bool mechanics_pass{false};

  bool replacement_point_pass{false};
  bool replacement_lower_bound_pass{false};
  bool replacement_positive_seed_count_pass{false};
  bool replacement_contrast_pass{false};
  bool neutral_noninferiority_pass{false};

  std::array<bool, kFamilyCount>
      qualified_minus_full_active_family_pass{};
  std::array<bool, kFamilyCount> qualified_minus_neutral_family_pass{};
  bool all_eight_family_deltas_pass{false};

  GeometryGateResult geometry{};
  bool replacement_pass{false};

  bool neutral_improvement_point_pass{false};
  bool neutral_improvement_lower_bound_pass{false};
  bool neutral_improvement_positive_seed_count_pass{false};
  bool neutral_improvement_contrast_pass{false};
  bool representation_improvement_pass{false};

  Classification classification{Classification::invalid_numeric_or_mechanics};
};

namespace detail {

[[nodiscard]] inline bool finite(double value) { return std::isfinite(value); }

[[nodiscard]] inline bool unit_fraction(double value) {
  return finite(value) && value >= 0.0 && value <= 1.0;
}

[[nodiscard]] inline bool valid_contrast(const PairedContrast &contrast) {
  return finite(contrast.point) && finite(contrast.low) &&
         finite(contrast.high) && contrast.low <= contrast.high &&
         contrast.point >= contrast.low && contrast.point <= contrast.high &&
         contrast.positive_seed_count >= 0 &&
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
  for (const double delta : families) {
    if (!finite(delta)) {
      return false;
    }
  }
  return true;
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

using GeometryMember = double PerSeedGeometry::*;

[[nodiscard]] inline ActiveGapRepairResult evaluate_high_is_good_gap(
    const GeometryBySeed &neutral, const GeometryBySeed &full_active,
    const GeometryBySeed &qualified, const PerSeedGeometry &neutral_mean,
    const PerSeedGeometry &full_active_mean,
    const PerSeedGeometry &qualified_mean, GeometryMember member) {
  ActiveGapRepairResult result{};
  result.applicable = full_active_mean.*member < neutral_mean.*member;
  for (std::size_t seed = 0; seed < kSeedCount; ++seed) {
    result.repair_direction_count +=
        qualified[seed].*member > full_active[seed].*member ? 1 : 0;
  }
  result.repair_direction_pass = result.repair_direction_count >= 2;
  if (!result.applicable) {
    result.pass = true;
    return result;
  }

  const double denominator =
      neutral_mean.*member - full_active_mean.*member;
  result.closure =
      (qualified_mean.*member - full_active_mean.*member) / denominator;
  result.closure_defined = true;
  result.closure_finite = finite(result.closure);
  result.closure_pass =
      result.closure_finite && result.closure >= kActiveGapClosureFloor;
  result.pass = result.closure_pass && result.repair_direction_pass;
  return result;
}

[[nodiscard]] inline ActiveGapRepairResult evaluate_top_gap(
    const GeometryBySeed &full_active, const GeometryBySeed &qualified,
    const PerSeedGeometry &neutral_mean,
    const PerSeedGeometry &full_active_mean,
    const PerSeedGeometry &qualified_mean) {
  ActiveGapRepairResult result{};
  result.applicable = full_active_mean.top > neutral_mean.top;
  for (std::size_t seed = 0; seed < kSeedCount; ++seed) {
    result.repair_direction_count +=
        qualified[seed].top < full_active[seed].top ? 1 : 0;
  }
  result.repair_direction_pass = result.repair_direction_count >= 2;
  if (!result.applicable) {
    result.pass = true;
    return result;
  }

  const double denominator = full_active_mean.top - neutral_mean.top;
  result.closure =
      (full_active_mean.top - qualified_mean.top) / denominator;
  result.closure_defined = true;
  result.closure_finite = finite(result.closure);
  result.closure_pass =
      result.closure_finite && result.closure >= kActiveGapClosureFloor;
  result.pass = result.closure_pass && result.repair_direction_pass;
  return result;
}

[[nodiscard]] inline bool required_gap_closure_finite(
    const ActiveGapRepairResult &gap) {
  return !gap.applicable || (gap.closure_defined && gap.closure_finite);
}

[[nodiscard]] inline GeometryGateResult
evaluate_geometry(const GeometryBySeed &neutral,
                  const GeometryBySeed &full_active,
                  const GeometryBySeed &qualified) {
  GeometryGateResult result{};
  result.neutral_mean = mean_geometry(neutral);
  result.full_active_mean = mean_geometry(full_active);
  result.qualified_mean = mean_geometry(qualified);

  result.better_effective_reference =
      std::max(result.neutral_mean.effective,
               result.full_active_mean.effective);
  result.better_participation_reference =
      std::max(result.neutral_mean.participation,
               result.full_active_mean.participation);
  result.better_top_reference =
      std::min(result.neutral_mean.top, result.full_active_mean.top);
  result.effective_denominator = result.better_effective_reference;
  result.participation_denominator = result.better_participation_reference;
  result.top_denominator = 1.0 - result.better_top_reference;
  result.effective_denominator_positive = result.effective_denominator > 0.0;
  result.participation_denominator_positive =
      result.participation_denominator > 0.0;
  result.top_denominator_positive = result.top_denominator > 0.0;
  result.all_better_reference_denominators_positive =
      result.effective_denominator_positive &&
      result.participation_denominator_positive &&
      result.top_denominator_positive;

  if (result.effective_denominator_positive) {
    result.effective_ratio =
        result.qualified_mean.effective / result.effective_denominator;
    result.effective_ratio_defined = true;
  }
  if (result.participation_denominator_positive) {
    result.participation_ratio = result.qualified_mean.participation /
                                 result.participation_denominator;
    result.participation_ratio_defined = true;
  }
  if (result.top_denominator_positive) {
    result.top_ratio = (1.0 - result.qualified_mean.top) /
                       result.top_denominator;
    result.top_ratio_defined = true;
  }
  result.all_better_reference_ratios_defined =
      result.effective_ratio_defined && result.participation_ratio_defined &&
      result.top_ratio_defined;
  result.required_ratios_finite =
      result.all_better_reference_ratios_defined &&
      finite(result.effective_ratio) && finite(result.participation_ratio) &&
      finite(result.top_ratio);
  result.effective_ratio_pass =
      finite(result.effective_ratio) &&
      result.effective_ratio >= kBetterReferenceRatioFloor;
  result.participation_ratio_pass =
      finite(result.participation_ratio) &&
      result.participation_ratio >= kBetterReferenceRatioFloor;
  result.top_ratio_pass = finite(result.top_ratio) &&
                          result.top_ratio >= kBetterReferenceRatioFloor;
  result.all_better_reference_ratios_pass =
      result.required_ratios_finite && result.effective_ratio_pass &&
      result.participation_ratio_pass && result.top_ratio_pass;

  result.effective_gap = evaluate_high_is_good_gap(
      neutral, full_active, qualified, result.neutral_mean,
      result.full_active_mean, result.qualified_mean,
      &PerSeedGeometry::effective);
  result.participation_gap = evaluate_high_is_good_gap(
      neutral, full_active, qualified, result.neutral_mean,
      result.full_active_mean, result.qualified_mean,
      &PerSeedGeometry::participation);
  result.top_gap =
      evaluate_top_gap(full_active, qualified, result.neutral_mean,
                       result.full_active_mean, result.qualified_mean);
  result.active_gap = evaluate_high_is_good_gap(
      neutral, full_active, qualified, result.neutral_mean,
      result.full_active_mean, result.qualified_mean, &PerSeedGeometry::active);
  result.required_gap_closures_finite =
      required_gap_closure_finite(result.effective_gap) &&
      required_gap_closure_finite(result.participation_gap) &&
      required_gap_closure_finite(result.top_gap) &&
      required_gap_closure_finite(result.active_gap);
  result.all_active_gap_repairs_pass =
      result.effective_gap.pass && result.participation_gap.pass &&
      result.top_gap.pass && result.active_gap.pass;

  result.all_candidate_active_pass = true;
  result.candidate_min_active = qualified.front().active;
  for (std::size_t seed = 0; seed < kSeedCount; ++seed) {
    result.candidate_active_seed_pass[seed] =
        qualified[seed].active >= kCandidateActiveDimensionFloor;
    result.all_candidate_active_pass = result.all_candidate_active_pass &&
                                       result.candidate_active_seed_pass[seed];
    result.candidate_min_active =
        std::min(result.candidate_min_active, qualified[seed].active);
  }

  result.numeric_valid =
      result.all_better_reference_denominators_positive &&
      result.required_ratios_finite && result.required_gap_closures_finite;
  result.pass = result.numeric_valid &&
                result.all_better_reference_ratios_pass &&
                result.all_active_gap_repairs_pass &&
                result.all_candidate_active_pass;
  return result;
}

} // namespace detail

[[nodiscard]] inline OuterAugmentationTrainingGateResult
evaluate_outer_augmentation_training_gate(
    const OuterAugmentationTrainingGateInput &input) {
  OuterAugmentationTrainingGateResult result{};
  result.basic_numeric_inputs_valid =
      detail::valid_contrast(input.qualified_minus_full_active) &&
      detail::valid_contrast(input.qualified_minus_neutral) &&
      detail::valid_contrast(input.full_active_minus_neutral) &&
      detail::valid_geometry(input.neutral_geometry) &&
      detail::valid_geometry(input.full_active_geometry) &&
      detail::valid_geometry(input.qualified_geometry) &&
      detail::valid_families(input.qualified_minus_full_active_family) &&
      detail::valid_families(input.qualified_minus_neutral_family);
  result.mechanics_pass = input.mechanics;

  result.replacement_point_pass =
      input.qualified_minus_full_active.point >= kMaterialityFloor;
  result.replacement_lower_bound_pass =
      input.qualified_minus_full_active.low > 0.0;
  result.replacement_positive_seed_count_pass =
      input.qualified_minus_full_active.positive_seed_count >= 2;
  result.replacement_contrast_pass =
      result.replacement_point_pass && result.replacement_lower_bound_pass &&
      result.replacement_positive_seed_count_pass;
  result.neutral_noninferiority_pass =
      input.qualified_minus_neutral.low > kNeutralNoninferiorityMargin;

  result.all_eight_family_deltas_pass = true;
  for (std::size_t family = 0; family < kFamilyCount; ++family) {
    result.qualified_minus_full_active_family_pass[family] =
        input.qualified_minus_full_active_family[family] >= kFamilyDeltaFloor;
    result.qualified_minus_neutral_family_pass[family] =
        input.qualified_minus_neutral_family[family] >= kFamilyDeltaFloor;
    result.all_eight_family_deltas_pass =
        result.all_eight_family_deltas_pass &&
        result.qualified_minus_full_active_family_pass[family] &&
        result.qualified_minus_neutral_family_pass[family];
  }

  if (result.basic_numeric_inputs_valid) {
    result.geometry = detail::evaluate_geometry(
        input.neutral_geometry, input.full_active_geometry,
        input.qualified_geometry);
  }
  result.numeric_inputs_valid =
      result.basic_numeric_inputs_valid && result.geometry.numeric_valid;
  result.replacement_pass =
      result.numeric_inputs_valid && result.mechanics_pass &&
      result.replacement_contrast_pass &&
      result.neutral_noninferiority_pass &&
      result.all_eight_family_deltas_pass && result.geometry.pass;

  result.neutral_improvement_point_pass =
      input.qualified_minus_neutral.point >= kMaterialityFloor;
  result.neutral_improvement_lower_bound_pass =
      input.qualified_minus_neutral.low > 0.0;
  result.neutral_improvement_positive_seed_count_pass =
      input.qualified_minus_neutral.positive_seed_count >= 2;
  result.neutral_improvement_contrast_pass =
      result.neutral_improvement_point_pass &&
      result.neutral_improvement_lower_bound_pass &&
      result.neutral_improvement_positive_seed_count_pass;
  result.representation_improvement_pass =
      result.replacement_pass && result.neutral_improvement_contrast_pass;

  if (!result.numeric_inputs_valid || !result.mechanics_pass) {
    result.classification = Classification::invalid_numeric_or_mechanics;
  } else if (result.representation_improvement_pass) {
    result.classification =
        Classification::qualified_candidate_representation_improvement_supported;
  } else if (result.replacement_pass) {
    result.classification =
        Classification::qualified_candidate_harm_mitigation_only;
  } else if (result.neutral_improvement_contrast_pass) {
    result.classification = Classification::
        qualified_candidate_neutral_aulc_improvement_only_replacement_not_supported;
  } else {
    result.classification =
        Classification::qualified_candidate_not_supported;
  }
  return result;
}

} // namespace cuwacunu::tests::mtf_outer_augmentation_training_gate
