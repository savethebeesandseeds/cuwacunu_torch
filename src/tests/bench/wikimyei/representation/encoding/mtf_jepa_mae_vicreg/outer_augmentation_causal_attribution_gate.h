#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace cuwacunu::tests::oaa1_gate {

inline constexpr char kProtocolSha256[] =
    "3370817d8e81686961ce87ab8cd99157616e4bc2cee3a9d262f674b1f2f3b4a2";

constexpr std::size_t kSeedCount = 3;
constexpr std::size_t kFamilyCount = 4;
constexpr std::size_t kObjectiveCount = 2;
constexpr std::size_t kEligibleProfileCount = 4;
constexpr std::size_t kCandidateCount = kObjectiveCount * kEligibleProfileCount;
constexpr std::size_t kNoCandidate = std::numeric_limits<std::size_t>::max();

constexpr double kRescueMaterialityFloor = 0.005;
constexpr double kFamilyDeltaFloor = -0.02;
constexpr std::size_t kRequiredPositiveFamilies = 3;

enum class Objective : std::uint8_t {
  jepa = 0,
  vicreg = 1,
};

enum class Profile : std::uint8_t {
  gaussian_only = 0,
  amplitude_only = 1,
  frequency_gain_only = 2,
  candidate_safe_stack = 3,
};

[[nodiscard]] inline const char *objective_name(Objective objective) {
  switch (objective) {
  case Objective::jepa:
    return "jepa";
  case Objective::vicreg:
    return "vicreg";
  }
  return "invalid";
}

[[nodiscard]] inline const char *profile_name(Profile profile) {
  switch (profile) {
  case Profile::gaussian_only:
    return "gaussian_only";
  case Profile::amplitude_only:
    return "amplitude_only";
  case Profile::frequency_gain_only:
    return "frequency_gain_only";
  case Profile::candidate_safe_stack:
    return "candidate_safe_stack";
  }
  return "invalid";
}

struct PairedContrast {
  double point{0.0};
  double low{0.0};
  double high{0.0};
  std::int64_t positive_seed_count{0};
};

struct FrozenSafeguards {
  bool raw_control{false};
  bool reversal_order{false};
  bool continuous_shuffle{false};
  bool order_shuffle{false};
  bool geometry{false};
};

struct CandidateInput {
  Objective objective{Objective::jepa};
  Profile profile{Profile::gaussian_only};
  PairedContrast candidate_minus_anchor{};
  PairedContrast candidate_minus_identity_objective{};
  std::array<double, kFamilyCount> family_deltas_vs_anchor{};
  FrozenSafeguards safeguards{};
  bool semantic{false};
  bool mechanics{false};

  // This is an upstream, paired determination: a failed safeguard is new
  // relative to the cached identity-objective arm and reproducible under the
  // frozen OAA-1 rule. A plain failed safeguard is not automatically "new".
  bool reproducible_new_safeguard_failure{false};
};

enum class Classification {
  invalid_numeric_or_mechanics,
  outer_augmentation_rescues_objective,
  outer_augmentation_mitigates_harm_only,
  outer_augmentation_worsens_objective,
  outer_augmentation_not_supported,
};

[[nodiscard]] inline const char *
classification_name(Classification classification) {
  switch (classification) {
  case Classification::invalid_numeric_or_mechanics:
    return "invalid_numeric_or_mechanics";
  case Classification::outer_augmentation_rescues_objective:
    return "outer_augmentation_rescues_objective";
  case Classification::outer_augmentation_mitigates_harm_only:
    return "outer_augmentation_mitigates_harm_only";
  case Classification::outer_augmentation_worsens_objective:
    return "outer_augmentation_worsens_objective";
  case Classification::outer_augmentation_not_supported:
    return "outer_augmentation_not_supported";
  }
  return "invalid_numeric_or_mechanics";
}

struct CandidateResult {
  bool numeric_inputs_valid{false};
  bool mechanics_pass{false};
  bool semantic_pass{false};

  bool anchor_point_pass{false};
  bool anchor_lower_bound_pass{false};
  bool all_anchor_seeds_improve{false};
  bool identity_lower_bound_pass{false};

  std::array<bool, kFamilyCount> family_positive{};
  std::array<bool, kFamilyCount> family_floor_pass{};
  std::size_t positive_family_count{0};
  bool positive_family_count_pass{false};
  bool all_family_floors_pass{false};

  bool raw_control_pass{false};
  bool reversal_order_pass{false};
  bool continuous_shuffle_pass{false};
  bool order_shuffle_pass{false};
  bool geometry_pass{false};
  bool all_frozen_safeguards_pass{false};

  bool rescue_pass{false};
  bool identity_upper_bound_strictly_negative{false};
  bool reproducible_new_safeguard_failure{false};
  bool worsening_pass{false};
  bool mitigation_pass{false};

  Classification classification{Classification::invalid_numeric_or_mechanics};
};

namespace detail {

[[nodiscard]] inline bool finite(double value) { return std::isfinite(value); }

[[nodiscard]] inline bool valid_objective(Objective objective) {
  return objective == Objective::jepa || objective == Objective::vicreg;
}

[[nodiscard]] inline bool valid_profile(Profile profile) {
  return profile == Profile::gaussian_only ||
         profile == Profile::amplitude_only ||
         profile == Profile::frequency_gain_only ||
         profile == Profile::candidate_safe_stack;
}

[[nodiscard]] inline std::size_t objective_index(Objective objective) {
  return static_cast<std::size_t>(objective);
}

[[nodiscard]] inline std::size_t profile_index(Profile profile) {
  return static_cast<std::size_t>(profile);
}

[[nodiscard]] inline std::size_t tie_priority(Objective objective,
                                              Profile profile) {
  // The frozen lexicographic tie order is objective first (JEPA, VICReg),
  // then profile (Gaussian, amplitude, frequency gain, safe stack).
  return objective_index(objective) * kEligibleProfileCount +
         profile_index(profile);
}

[[nodiscard]] inline bool valid_contrast(const PairedContrast &contrast) {
  return finite(contrast.point) && finite(contrast.low) &&
         finite(contrast.high) && contrast.low <= contrast.high &&
         contrast.positive_seed_count >= 0 &&
         contrast.positive_seed_count <= static_cast<std::int64_t>(kSeedCount);
}

[[nodiscard]] inline bool
valid_families(const std::array<double, kFamilyCount> &families) {
  for (const double family : families) {
    if (!finite(family)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool safeguards_pass(const FrozenSafeguards &safeguards) {
  return safeguards.raw_control && safeguards.reversal_order &&
         safeguards.continuous_shuffle && safeguards.order_shuffle &&
         safeguards.geometry;
}

} // namespace detail

[[nodiscard]] inline CandidateResult
evaluate_candidate(const CandidateInput &input) {
  CandidateResult result{};
  result.numeric_inputs_valid =
      detail::valid_objective(input.objective) &&
      detail::valid_profile(input.profile) &&
      detail::valid_contrast(input.candidate_minus_anchor) &&
      detail::valid_contrast(input.candidate_minus_identity_objective) &&
      detail::valid_families(input.family_deltas_vs_anchor);
  result.mechanics_pass = input.mechanics;
  result.semantic_pass = input.semantic;

  result.anchor_point_pass =
      input.candidate_minus_anchor.point >= kRescueMaterialityFloor;
  result.anchor_lower_bound_pass = input.candidate_minus_anchor.low > 0.0;
  result.all_anchor_seeds_improve =
      input.candidate_minus_anchor.positive_seed_count ==
      static_cast<std::int64_t>(kSeedCount);
  result.identity_lower_bound_pass =
      input.candidate_minus_identity_objective.low > 0.0;

  result.all_family_floors_pass = true;
  for (std::size_t family = 0; family < kFamilyCount; ++family) {
    const double delta = input.family_deltas_vs_anchor[family];
    result.family_positive[family] = delta > 0.0;
    result.family_floor_pass[family] = delta >= kFamilyDeltaFloor;
    result.positive_family_count += result.family_positive[family] ? 1U : 0U;
    result.all_family_floors_pass =
        result.all_family_floors_pass && result.family_floor_pass[family];
  }
  result.positive_family_count_pass =
      result.positive_family_count >= kRequiredPositiveFamilies;

  result.raw_control_pass = input.safeguards.raw_control;
  result.reversal_order_pass = input.safeguards.reversal_order;
  result.continuous_shuffle_pass = input.safeguards.continuous_shuffle;
  result.order_shuffle_pass = input.safeguards.order_shuffle;
  result.geometry_pass = input.safeguards.geometry;
  result.all_frozen_safeguards_pass = detail::safeguards_pass(input.safeguards);

  result.rescue_pass =
      result.numeric_inputs_valid && result.mechanics_pass &&
      result.semantic_pass && result.anchor_point_pass &&
      result.anchor_lower_bound_pass && result.all_anchor_seeds_improve &&
      result.identity_lower_bound_pass && result.positive_family_count_pass &&
      result.all_family_floors_pass && result.all_frozen_safeguards_pass;

  result.identity_upper_bound_strictly_negative =
      input.candidate_minus_identity_objective.high < 0.0;
  result.reproducible_new_safeguard_failure =
      input.reproducible_new_safeguard_failure;
  result.worsening_pass = result.numeric_inputs_valid &&
                          result.mechanics_pass &&
                          (result.identity_upper_bound_strictly_negative ||
                           result.reproducible_new_safeguard_failure);

  // A reproducible new safeguard failure is worsening, even if the primary
  // AULC contrast is positive; it cannot simultaneously be called mitigation.
  result.mitigation_pass =
      result.numeric_inputs_valid && result.mechanics_pass &&
      result.semantic_pass && result.identity_lower_bound_pass &&
      !result.rescue_pass && !result.reproducible_new_safeguard_failure;

  if (!result.numeric_inputs_valid || !result.mechanics_pass) {
    result.classification = Classification::invalid_numeric_or_mechanics;
  } else if (result.rescue_pass) {
    result.classification =
        Classification::outer_augmentation_rescues_objective;
  } else if (result.worsening_pass) {
    result.classification =
        Classification::outer_augmentation_worsens_objective;
  } else if (result.mitigation_pass) {
    result.classification =
        Classification::outer_augmentation_mitigates_harm_only;
  } else {
    result.classification = Classification::outer_augmentation_not_supported;
  }
  return result;
}

using CandidateInventory = std::array<CandidateInput, kCandidateCount>;

struct ExperimentResult {
  std::array<CandidateResult, kCandidateCount> candidates{};
  bool inventory_exact{false};
  bool all_numeric_and_mechanics_valid{false};
  bool experiment_valid{false};
  std::size_t rescue_candidate_count{0};
  bool has_selected_rescue{false};
  std::size_t selected_candidate_index{kNoCandidate};
  Objective selected_objective{Objective::jepa};
  Profile selected_profile{Profile::gaussian_only};
  double selected_anchor_point{std::numeric_limits<double>::quiet_NaN()};
  bool confirmation_open_authorized{false};
};

[[nodiscard]] inline ExperimentResult
evaluate_experiment(const CandidateInventory &inputs) {
  ExperimentResult result{};
  std::array<std::array<std::size_t, kEligibleProfileCount>, kObjectiveCount>
      inventory_counts{};

  result.all_numeric_and_mechanics_valid = true;
  for (std::size_t index = 0; index < inputs.size(); ++index) {
    result.candidates[index] = evaluate_candidate(inputs[index]);
    result.all_numeric_and_mechanics_valid =
        result.all_numeric_and_mechanics_valid &&
        result.candidates[index].numeric_inputs_valid &&
        result.candidates[index].mechanics_pass;
    if (detail::valid_objective(inputs[index].objective) &&
        detail::valid_profile(inputs[index].profile)) {
      ++inventory_counts[detail::objective_index(inputs[index].objective)]
                        [detail::profile_index(inputs[index].profile)];
    }
  }

  result.inventory_exact = true;
  for (const auto &objective : inventory_counts) {
    for (const std::size_t count : objective) {
      result.inventory_exact = result.inventory_exact && count == 1;
    }
  }
  result.experiment_valid =
      result.inventory_exact && result.all_numeric_and_mechanics_valid;

  if (!result.experiment_valid) {
    return result;
  }

  std::size_t best_priority = kNoCandidate;
  for (std::size_t index = 0; index < inputs.size(); ++index) {
    if (result.candidates[index].classification !=
        Classification::outer_augmentation_rescues_objective) {
      continue;
    }
    ++result.rescue_candidate_count;
    const double point = inputs[index].candidate_minus_anchor.point;
    const std::size_t priority =
        detail::tie_priority(inputs[index].objective, inputs[index].profile);
    if (!result.has_selected_rescue || point > result.selected_anchor_point ||
        (point == result.selected_anchor_point && priority < best_priority)) {
      result.has_selected_rescue = true;
      result.selected_candidate_index = index;
      result.selected_objective = inputs[index].objective;
      result.selected_profile = inputs[index].profile;
      result.selected_anchor_point = point;
      best_priority = priority;
    }
  }
  result.confirmation_open_authorized = result.has_selected_rescue;
  return result;
}

} // namespace cuwacunu::tests::oaa1_gate
