#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace cuwacunu::tests::representation_module_certification_gate {

constexpr std::size_t kSeedCount = 3;
constexpr std::size_t kChannelCount = 3;
constexpr std::size_t kFamilyCount = 4;
constexpr double kLearnedGainFloor = 0.005;
constexpr double kRawNoninferiorityMargin = -0.01;
constexpr double kFamilyFloor = -0.02;
constexpr double kFinalOrderPointFloor = 0.90;
constexpr double kFinalOrderLowerFloor = 0.85;
constexpr double kOrderRetentionMargin = -0.02;
constexpr double kContinuousShuffleUpper = 0.05;
constexpr double kOrderShuffleLower = 0.40;
constexpr double kOrderShuffleUpper = 0.60;
constexpr double kEffectiveRankFloor = 0.25;
constexpr double kParticipationRankFloor = 0.25;
constexpr double kTopEigenvalueCeiling = 0.80;
constexpr double kActiveDimensionFloor = 0.75;
constexpr double kAugmentationAdvantageFloor = 0.0024;

struct Contrast {
  double point{0.0};
  double low{0.0};
  double high{0.0};
  int64_t positive_seed_count{0};
};

struct Geometry {
  double effective{0.0};
  double participation{0.0};
  double top{1.0};
  double active{0.0};
};

using GeometryBySeed =
    std::array<std::array<Geometry, kChannelCount>, kSeedCount>;

struct CandidateInput {
  Contrast trained_minus_initialization{};
  Contrast final_minus_raw{};
  std::array<double, kFamilyCount> learned_family_deltas{};
  double final_order_point{0.0};
  double final_order_low{0.0};
  Contrast order_trained_minus_initialization{};
  double continuous_shuffle_high{1.0};
  double order_shuffle_low{0.0};
  double order_shuffle_high{1.0};
  GeometryBySeed geometry{};
  bool mechanics{false};
};

struct CandidateResult {
  bool numeric_valid{false};
  bool mechanics_pass{false};
  bool learned_point_pass{false};
  bool learned_lower_pass{false};
  bool learned_seed_pass{false};
  bool family_positive_count_pass{false};
  bool family_floor_pass{false};
  bool raw_noninferiority_pass{false};
  bool order_point_pass{false};
  bool order_lower_pass{false};
  bool order_retention_pass{false};
  bool continuous_shuffle_pass{false};
  bool order_shuffle_pass{false};
  bool geometry_pass{false};
  bool pass{false};
};

enum class Classification {
  invalid_mechanics_or_numeric,
  encoder_training_not_working,
  neutral_candidate,
  qualified_candidate,
};

[[nodiscard]] inline const char *
classification_name(Classification classification) {
  switch (classification) {
  case Classification::invalid_mechanics_or_numeric:
    return "invalid_mechanics_or_numeric";
  case Classification::encoder_training_not_working:
    return "encoder_training_not_working";
  case Classification::neutral_candidate:
    return "neutral_candidate";
  case Classification::qualified_candidate:
    return "qualified_candidate";
  }
  return "invalid_mechanics_or_numeric";
}

struct GateInput {
  CandidateInput neutral{};
  CandidateInput qualified{};
  Contrast qualified_minus_neutral{};
  bool global_mechanics{false};
};

struct GateResult {
  CandidateResult neutral{};
  CandidateResult qualified{};
  bool augmentation_contrast_valid{false};
  bool augmentation_point_pass{false};
  bool augmentation_lower_pass{false};
  bool augmentation_seed_pass{false};
  bool augmentation_advantage_pass{false};
  Classification classification{Classification::invalid_mechanics_or_numeric};
};

namespace detail {

[[nodiscard]] inline bool finite(double value) { return std::isfinite(value); }

[[nodiscard]] inline bool valid_contrast(const Contrast &value) {
  return finite(value.point) && finite(value.low) && finite(value.high) &&
         value.low <= value.high && value.positive_seed_count >= 0 &&
         value.positive_seed_count <= static_cast<int64_t>(kSeedCount);
}

[[nodiscard]] inline bool unit(double value) {
  return finite(value) && value >= 0.0 && value <= 1.0;
}

[[nodiscard]] inline CandidateResult
evaluate_candidate(const CandidateInput &input) {
  CandidateResult result{};
  bool families_valid = true;
  int64_t positive_families = 0;
  result.family_floor_pass = true;
  for (const double value : input.learned_family_deltas) {
    families_valid = families_valid && finite(value);
    positive_families += value > 0.0 ? 1 : 0;
    result.family_floor_pass =
        result.family_floor_pass && value >= kFamilyFloor;
  }

  bool geometry_valid = true;
  result.geometry_pass = true;
  for (const auto &seed : input.geometry) {
    for (const auto &channel : seed) {
      geometry_valid = geometry_valid && unit(channel.effective) &&
                       unit(channel.participation) && unit(channel.top) &&
                       unit(channel.active);
      result.geometry_pass =
          result.geometry_pass && channel.effective >= kEffectiveRankFloor &&
          channel.participation >= kParticipationRankFloor &&
          channel.top <= kTopEigenvalueCeiling &&
          channel.active >= kActiveDimensionFloor;
    }
  }

  result.numeric_valid =
      valid_contrast(input.trained_minus_initialization) &&
      valid_contrast(input.final_minus_raw) &&
      valid_contrast(input.order_trained_minus_initialization) &&
      families_valid && finite(input.final_order_point) &&
      finite(input.final_order_low) &&
      finite(input.continuous_shuffle_high) &&
      finite(input.order_shuffle_low) && finite(input.order_shuffle_high) &&
      input.final_order_point >= 0.0 && input.final_order_point <= 1.0 &&
      input.final_order_low >= 0.0 && input.final_order_low <= 1.0 &&
      input.order_shuffle_low <= input.order_shuffle_high && geometry_valid;
  result.mechanics_pass = input.mechanics;
  result.learned_point_pass =
      input.trained_minus_initialization.point >= kLearnedGainFloor;
  result.learned_lower_pass = input.trained_minus_initialization.low > 0.0;
  result.learned_seed_pass =
      input.trained_minus_initialization.positive_seed_count ==
      static_cast<int64_t>(kSeedCount);
  result.family_positive_count_pass = positive_families >= 3;
  result.raw_noninferiority_pass =
      input.final_minus_raw.low >= kRawNoninferiorityMargin;
  result.order_point_pass =
      input.final_order_point >= kFinalOrderPointFloor;
  result.order_lower_pass = input.final_order_low >= kFinalOrderLowerFloor;
  result.order_retention_pass =
      input.order_trained_minus_initialization.low >= kOrderRetentionMargin;
  result.continuous_shuffle_pass =
      input.continuous_shuffle_high <= kContinuousShuffleUpper;
  result.order_shuffle_pass =
      input.order_shuffle_low >= kOrderShuffleLower &&
      input.order_shuffle_high <= kOrderShuffleUpper;
  result.pass = result.numeric_valid && result.mechanics_pass &&
                result.learned_point_pass && result.learned_lower_pass &&
                result.learned_seed_pass &&
                result.family_positive_count_pass && result.family_floor_pass &&
                result.raw_noninferiority_pass && result.order_point_pass &&
                result.order_lower_pass && result.order_retention_pass &&
                result.continuous_shuffle_pass && result.order_shuffle_pass &&
                result.geometry_pass;
  return result;
}

} // namespace detail

[[nodiscard]] inline GateResult evaluate(const GateInput &input) {
  GateResult result{};
  result.neutral = detail::evaluate_candidate(input.neutral);
  result.qualified = detail::evaluate_candidate(input.qualified);
  result.augmentation_contrast_valid =
      detail::valid_contrast(input.qualified_minus_neutral);
  result.augmentation_point_pass =
      input.qualified_minus_neutral.point >= kAugmentationAdvantageFloor;
  result.augmentation_lower_pass = input.qualified_minus_neutral.low > 0.0;
  result.augmentation_seed_pass =
      input.qualified_minus_neutral.positive_seed_count >= 2;
  result.augmentation_advantage_pass =
      result.augmentation_contrast_valid && result.augmentation_point_pass &&
      result.augmentation_lower_pass && result.augmentation_seed_pass;

  if (!input.global_mechanics || !result.neutral.numeric_valid ||
      !result.qualified.numeric_valid ||
      !result.augmentation_contrast_valid) {
    result.classification = Classification::invalid_mechanics_or_numeric;
  } else if (result.qualified.pass &&
             (!result.neutral.pass || result.augmentation_advantage_pass)) {
    result.classification = Classification::qualified_candidate;
  } else if (result.neutral.pass) {
    result.classification = Classification::neutral_candidate;
  } else {
    result.classification = Classification::encoder_training_not_working;
  }
  return result;
}

} // namespace cuwacunu::tests::representation_module_certification_gate
