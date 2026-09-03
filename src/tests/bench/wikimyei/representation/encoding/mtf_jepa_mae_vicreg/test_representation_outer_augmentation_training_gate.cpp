#include "representation_outer_augmentation_training_gate.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

namespace gate =
    cuwacunu::tests::mtf_outer_augmentation_training_gate;

void require(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool near(double lhs, double rhs, double tolerance = 1e-15) {
  return std::abs(lhs - rhs) <= tolerance;
}

gate::GeometryBySeed repeated_geometry(double effective, double participation,
                                       double top, double active) {
  gate::GeometryBySeed geometry{};
  for (auto &seed : geometry) {
    seed = {.effective = effective,
            .participation = participation,
            .top = top,
            .active = active};
  }
  return geometry;
}

gate::OuterAugmentationTrainingGateInput passing_input() {
  return {
      .qualified_minus_full_active = {0.010, 0.005, 0.015, 3},
      .qualified_minus_neutral = {0.010, 0.005, 0.015, 3},
      .full_active_minus_neutral = {0.0, -0.005, 0.005, 1},
      .neutral_geometry = repeated_geometry(0.80, 0.70, 0.20, 0.90),
      .full_active_geometry = repeated_geometry(0.60, 0.50, 0.40, 0.80),
      .qualified_geometry = repeated_geometry(0.75, 0.65, 0.25, 0.86),
      .qualified_minus_full_active_family = {0.0, 0.0, 0.0, 0.0},
      .qualified_minus_neutral_family = {0.0, 0.0, 0.0, 0.0},
      .mechanics = true,
  };
}

using GeometryMember = double gate::PerSeedGeometry::*;

constexpr std::array<GeometryMember, 4> kGeometryMembers{
    &gate::PerSeedGeometry::effective,
    &gate::PerSeedGeometry::participation,
    &gate::PerSeedGeometry::top,
    &gate::PerSeedGeometry::active,
};

void set_repeated_metric(gate::GeometryBySeed &geometry,
                         GeometryMember member, double value) {
  for (auto &seed : geometry) {
    seed.*member = value;
  }
}

const gate::ActiveGapRepairResult &
gap_result(const gate::GeometryGateResult &geometry, std::size_t metric) {
  switch (metric) {
  case 0:
    return geometry.effective_gap;
  case 1:
    return geometry.participation_gap;
  case 2:
    return geometry.top_gap;
  default:
    return geometry.active_gap;
  }
}

gate::OuterAugmentationTrainingGateInput gap_boundary_input(
    std::size_t metric) {
  auto input = passing_input();
  const auto member = kGeometryMembers[metric];
  if (metric == 2) {
    set_repeated_metric(input.neutral_geometry, member, 0.25);
    set_repeated_metric(input.full_active_geometry, member, 0.375);
    set_repeated_metric(input.qualified_geometry, member, 0.3125);
  } else if (metric == 3) {
    set_repeated_metric(input.neutral_geometry, member, 0.875);
    set_repeated_metric(input.full_active_geometry, member, 0.75);
    set_repeated_metric(input.qualified_geometry, member, 0.8125);
  } else {
    set_repeated_metric(input.neutral_geometry, member, 0.50);
    set_repeated_metric(input.full_active_geometry, member, 0.4375);
    set_repeated_metric(input.qualified_geometry, member, 0.46875);
  }
  return input;
}

void require_invalid(
    const gate::OuterAugmentationTrainingGateResult &result,
    const std::string &message) {
  require(!result.numeric_inputs_valid && !result.replacement_pass &&
              !result.representation_improvement_pass &&
              result.classification ==
                  gate::Classification::invalid_numeric_or_mechanics,
          message);
}

void test_exact_passing_fixture() {
  const auto result =
      gate::evaluate_outer_augmentation_training_gate(passing_input());
  require(result.basic_numeric_inputs_valid && result.numeric_inputs_valid &&
              result.mechanics_pass && result.replacement_contrast_pass &&
              result.neutral_noninferiority_pass &&
              result.all_eight_family_deltas_pass && result.geometry.pass &&
              result.replacement_pass &&
              result.neutral_improvement_contrast_pass &&
              result.representation_improvement_pass &&
              result.classification == gate::Classification::
                  qualified_candidate_representation_improvement_supported,
          "fully passing fixture must support representation improvement");
  require(result.geometry.all_better_reference_ratios_pass &&
              result.geometry.all_active_gap_repairs_pass &&
              result.geometry.all_candidate_active_pass &&
              result.geometry.effective_gap.applicable &&
              result.geometry.participation_gap.applicable &&
              result.geometry.top_gap.applicable &&
              result.geometry.active_gap.applicable,
          "passing fixture must exercise every geometry clause");
}

void test_classification_names() {
  using Classification = gate::Classification;
  require(std::string(gate::classification_name(
              Classification::invalid_numeric_or_mechanics)) ==
              "invalid_numeric_or_mechanics",
          "invalid classification name must be frozen");
  require(std::string(gate::classification_name(
              Classification::
                  qualified_candidate_representation_improvement_supported)) ==
              "qualified_candidate_representation_improvement_supported",
          "joint-support classification name must be frozen");
  require(std::string(gate::classification_name(
              Classification::qualified_candidate_harm_mitigation_only)) ==
              "qualified_candidate_harm_mitigation_only",
          "harm-mitigation classification name must be frozen");
  require(std::string(gate::classification_name(
              Classification::
                  qualified_candidate_neutral_aulc_improvement_only_replacement_not_supported)) ==
              "qualified_candidate_neutral_aulc_improvement_only_replacement_"
              "not_supported",
          "neutral-only classification name must be frozen");
  require(std::string(gate::classification_name(
              Classification::qualified_candidate_not_supported)) ==
              "qualified_candidate_not_supported",
          "unsupported classification name must be frozen");
}

void test_replacement_contrast_boundaries() {
  {
    auto input = passing_input();
    input.qualified_minus_full_active.low = 0.001;
    input.qualified_minus_full_active.point = gate::kMaterialityFloor;
    const auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(result.replacement_point_pass && result.replacement_pass,
            "replacement point equality at +0.0024 must pass");
  }
  {
    auto input = passing_input();
    input.qualified_minus_full_active.low = 0.001;
    input.qualified_minus_full_active.point = std::nextafter(
        gate::kMaterialityFloor, -std::numeric_limits<double>::infinity());
    const auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(!result.replacement_point_pass && !result.replacement_pass,
            "replacement point immediately below +0.0024 must fail");
  }
  {
    auto input = passing_input();
    input.qualified_minus_full_active.low = 0.0;
    const auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(!result.replacement_lower_bound_pass && !result.replacement_pass,
            "replacement lower-bound equality at zero must fail");
  }
  {
    auto input = passing_input();
    input.qualified_minus_full_active.low =
        std::nextafter(0.0, std::numeric_limits<double>::infinity());
    const auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(result.replacement_lower_bound_pass && result.replacement_pass,
            "replacement lower bound immediately above zero must pass");
  }
  {
    auto input = passing_input();
    input.qualified_minus_full_active.positive_seed_count = 2;
    auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(result.replacement_positive_seed_count_pass &&
                result.replacement_pass,
            "exactly two positive replacement seeds must pass");
    input.qualified_minus_full_active.positive_seed_count = 1;
    result = gate::evaluate_outer_augmentation_training_gate(input);
    require(!result.replacement_positive_seed_count_pass &&
                !result.replacement_pass,
            "one positive replacement seed must fail");
  }
}

void test_neutral_noninferiority_boundary() {
  {
    auto input = passing_input();
    input.qualified_minus_neutral.low = gate::kNeutralNoninferiorityMargin;
    const auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(!result.neutral_noninferiority_pass && !result.replacement_pass,
            "neutral noninferiority equality at -0.005 must fail");
  }
  {
    auto input = passing_input();
    input.qualified_minus_neutral.low = std::nextafter(
        gate::kNeutralNoninferiorityMargin,
        std::numeric_limits<double>::infinity());
    const auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(result.neutral_noninferiority_pass && result.replacement_pass,
            "neutral noninferiority immediately above -0.005 must pass");
  }
}

void test_standalone_neutral_improvement_boundaries() {
  {
    auto input = passing_input();
    input.qualified_minus_neutral.low = 0.001;
    input.qualified_minus_neutral.point = gate::kMaterialityFloor;
    auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(result.neutral_improvement_point_pass &&
                result.neutral_improvement_contrast_pass,
            "neutral point equality at +0.0024 must pass");
    input.qualified_minus_neutral.point = std::nextafter(
        gate::kMaterialityFloor, -std::numeric_limits<double>::infinity());
    result = gate::evaluate_outer_augmentation_training_gate(input);
    require(!result.neutral_improvement_point_pass &&
                !result.neutral_improvement_contrast_pass &&
                result.replacement_pass,
            "neutral point below +0.0024 must fail only the independent subgate");
  }
  {
    auto input = passing_input();
    input.qualified_minus_neutral.low = 0.0;
    auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(!result.neutral_improvement_lower_bound_pass &&
                !result.neutral_improvement_contrast_pass &&
                result.replacement_pass,
            "neutral lower-bound equality at zero must fail the strict subgate");
    input.qualified_minus_neutral.low =
        std::nextafter(0.0, std::numeric_limits<double>::infinity());
    result = gate::evaluate_outer_augmentation_training_gate(input);
    require(result.neutral_improvement_lower_bound_pass &&
                result.neutral_improvement_contrast_pass,
            "neutral lower bound immediately above zero must pass");
  }
  {
    auto input = passing_input();
    input.qualified_minus_neutral.positive_seed_count = 2;
    auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(result.neutral_improvement_positive_seed_count_pass &&
                result.neutral_improvement_contrast_pass,
            "exactly two positive neutral seeds must pass");
    input.qualified_minus_neutral.positive_seed_count = 1;
    result = gate::evaluate_outer_augmentation_training_gate(input);
    require(!result.neutral_improvement_positive_seed_count_pass &&
                !result.neutral_improvement_contrast_pass &&
                result.replacement_pass,
            "one positive neutral seed must fail only the independent subgate");
  }
}

void test_all_eight_family_boundaries() {
  using Input = gate::OuterAugmentationTrainingGateInput;
  using FamilyMember = gate::FamilyDeltas Input::*;
  constexpr std::array<FamilyMember, 2> family_sets{
      &Input::qualified_minus_full_active_family,
      &Input::qualified_minus_neutral_family,
  };
  for (std::size_t reference = 0; reference < family_sets.size(); ++reference) {
    for (std::size_t family = 0; family < gate::kFamilyCount; ++family) {
      auto input = passing_input();
      (input.*family_sets[reference])[family] = gate::kFamilyDeltaFloor;
      auto result = gate::evaluate_outer_augmentation_training_gate(input);
      require(result.all_eight_family_deltas_pass && result.replacement_pass,
              "each family equality at -0.02 must pass");

      (input.*family_sets[reference])[family] = std::nextafter(
          gate::kFamilyDeltaFloor, -std::numeric_limits<double>::infinity());
      result = gate::evaluate_outer_augmentation_training_gate(input);
      const bool clause = reference == 0
                              ? result
                                    .qualified_minus_full_active_family_pass
                                        [family]
                              : result.qualified_minus_neutral_family_pass
                                    [family];
      require(!clause && !result.all_eight_family_deltas_pass &&
                  !result.replacement_pass,
              "each family value immediately below -0.02 must fail");
    }
  }
}

void test_better_reference_selection() {
  auto input = passing_input();
  input.neutral_geometry = repeated_geometry(0.40, 0.40, 0.30, 0.90);
  input.full_active_geometry = repeated_geometry(0.50, 0.50, 0.20, 0.80);
  input.qualified_geometry = repeated_geometry(0.45, 0.45, 0.27, 0.86);
  const auto result = gate::evaluate_outer_augmentation_training_gate(input);
  require(near(result.geometry.better_effective_reference, 0.50) &&
              near(result.geometry.better_participation_reference, 0.50) &&
              near(result.geometry.better_top_reference, 0.20),
          "better reference must use max for ranks and min for top share");
  require(result.geometry.effective_ratio_pass &&
              result.geometry.participation_ratio_pass &&
              result.geometry.top_ratio_pass,
          "candidate must be compared with the selected better reference");
}

void test_seed_means_precede_better_reference_selection() {
  auto input = passing_input();
  input.neutral_geometry[0].effective = 1.0;
  input.neutral_geometry[1].effective = 0.0;
  input.neutral_geometry[2].effective = 0.0;
  input.full_active_geometry[0].effective = 0.0;
  input.full_active_geometry[1].effective = 1.0;
  input.full_active_geometry[2].effective = 0.0;
  set_repeated_metric(input.qualified_geometry,
                      &gate::PerSeedGeometry::effective, 0.31);

  input.neutral_geometry[0].participation = 1.0;
  input.neutral_geometry[1].participation = 0.0;
  input.neutral_geometry[2].participation = 0.0;
  input.full_active_geometry[0].participation = 0.0;
  input.full_active_geometry[1].participation = 1.0;
  input.full_active_geometry[2].participation = 0.0;
  set_repeated_metric(input.qualified_geometry,
                      &gate::PerSeedGeometry::participation, 0.31);

  input.neutral_geometry[0].top = 0.0;
  input.neutral_geometry[1].top = 1.0;
  input.neutral_geometry[2].top = 1.0;
  input.full_active_geometry[0].top = 1.0;
  input.full_active_geometry[1].top = 0.0;
  input.full_active_geometry[2].top = 1.0;
  set_repeated_metric(input.qualified_geometry, &gate::PerSeedGeometry::top,
                      0.69);

  const auto result = gate::evaluate_outer_augmentation_training_gate(input);
  require(near(result.geometry.neutral_mean.effective, 1.0 / 3.0) &&
              near(result.geometry.full_active_mean.effective, 1.0 / 3.0) &&
              near(result.geometry.better_effective_reference, 1.0 / 3.0) &&
              near(result.geometry.neutral_mean.participation, 1.0 / 3.0) &&
              near(result.geometry.full_active_mean.participation,
                   1.0 / 3.0) &&
              near(result.geometry.better_participation_reference,
                   1.0 / 3.0) &&
              near(result.geometry.neutral_mean.top, 2.0 / 3.0) &&
              near(result.geometry.full_active_mean.top, 2.0 / 3.0) &&
              near(result.geometry.better_top_reference, 2.0 / 3.0),
          "geometry must average each arm over seeds before selecting a reference");
  require(result.geometry.all_better_reference_ratios_pass,
          "mean-first better-reference ratios must be evaluated from those means");
}

void test_better_reference_ratio_boundaries() {
  for (std::size_t metric = 0; metric < 3; ++metric) {
    auto input = passing_input();
    if (metric == 2) {
      set_repeated_metric(input.neutral_geometry, kGeometryMembers[metric],
                          0.20);
      set_repeated_metric(input.full_active_geometry,
                          kGeometryMembers[metric], 0.20);
      const double boundary =
          1.0 - gate::kBetterReferenceRatioFloor * (1.0 - 0.20);
      set_repeated_metric(input.qualified_geometry, kGeometryMembers[metric],
                          boundary);
    } else {
      set_repeated_metric(input.neutral_geometry, kGeometryMembers[metric],
                          0.50);
      set_repeated_metric(input.full_active_geometry,
                          kGeometryMembers[metric], 0.50);
      const double boundary = gate::kBetterReferenceRatioFloor * 0.50;
      set_repeated_metric(input.qualified_geometry, kGeometryMembers[metric],
                          boundary);
    }
    auto result = gate::evaluate_outer_augmentation_training_gate(input);
    const bool ratio_pass = metric == 0   ? result.geometry.effective_ratio_pass
                            : metric == 1
                                ? result.geometry.participation_ratio_pass
                                : result.geometry.top_ratio_pass;
    require(ratio_pass && result.geometry.pass,
            "each better-reference ratio equality at 0.90 must pass");

    double boundary = input.qualified_geometry.front().*kGeometryMembers[metric];
    double failing = boundary;
    const double direction =
        metric == 2 ? std::numeric_limits<double>::infinity()
                    : -std::numeric_limits<double>::infinity();
    // A single ULP can be rounded back to the inclusive boundary when three
    // identical seed values are summed and divided by three. Move only enough
    // representable values to survive that prescribed mean-first reduction.
    for (int ulp = 0; ulp < 8; ++ulp) {
      failing = std::nextafter(failing, direction);
    }
    set_repeated_metric(input.qualified_geometry, kGeometryMembers[metric],
                        failing);
    result = gate::evaluate_outer_augmentation_training_gate(input);
    const bool failed_ratio =
        metric == 0   ? !result.geometry.effective_ratio_pass
        : metric == 1 ? !result.geometry.participation_ratio_pass
                      : !result.geometry.top_ratio_pass;
    require(failed_ratio && !result.geometry.pass && !result.replacement_pass,
            "each better-reference ratio immediately below 0.90 must fail");
  }
}

void test_better_reference_zero_denominators() {
  for (std::size_t metric = 0; metric < 3; ++metric) {
    auto input = passing_input();
    const double degenerate = metric == 2 ? 1.0 : 0.0;
    set_repeated_metric(input.neutral_geometry, kGeometryMembers[metric],
                        degenerate);
    set_repeated_metric(input.full_active_geometry, kGeometryMembers[metric],
                        degenerate);
    set_repeated_metric(input.qualified_geometry, kGeometryMembers[metric],
                        degenerate);
    const auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(result.basic_numeric_inputs_valid &&
                !result.geometry.all_better_reference_denominators_positive &&
                !result.geometry.all_better_reference_ratios_defined,
            "zero required geometry denominator must remain explicit");
    require_invalid(result,
                    "zero required geometry denominator must be numeric invalid");
  }
}

void test_nonfinite_required_ratios() {
  for (std::size_t metric = 0; metric < 2; ++metric) {
    auto input = passing_input();
    const double tiny = std::numeric_limits<double>::denorm_min();
    set_repeated_metric(input.neutral_geometry, kGeometryMembers[metric], tiny);
    set_repeated_metric(input.full_active_geometry, kGeometryMembers[metric],
                        tiny);
    set_repeated_metric(input.qualified_geometry, kGeometryMembers[metric],
                        1.0);
    const auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(result.basic_numeric_inputs_valid &&
                result.geometry.all_better_reference_denominators_positive &&
                !result.geometry.required_ratios_finite,
            "overflowed required ratio must be detected after valid fractions");
    require_invalid(result,
                    "non-finite required geometry ratio must be numeric invalid");
  }
}

void test_active_gap_closure_boundaries() {
  for (std::size_t metric = 0; metric < 4; ++metric) {
    auto input = gap_boundary_input(metric);
    auto result = gate::evaluate_outer_augmentation_training_gate(input);
    auto gap = gap_result(result.geometry, metric);
    require(gap.applicable && gap.closure_defined && gap.closure_finite &&
                gap.closure_pass && gap.pass && result.replacement_pass,
            "each applicable active gap must pass closure equality at 0.50");

    const auto member = kGeometryMembers[metric];
    const double boundary = input.qualified_geometry.front().*member;
    const double toward_failure = input.full_active_geometry.front().*member;
    const double failing = std::nextafter(boundary, toward_failure);
    set_repeated_metric(input.qualified_geometry, member, failing);
    result = gate::evaluate_outer_augmentation_training_gate(input);
    gap = gap_result(result.geometry, metric);
    require(gap.applicable && gap.closure_finite && !gap.closure_pass &&
                !gap.pass && !result.replacement_pass,
            "each applicable active gap immediately below 0.50 must fail");
  }
}

void test_active_gap_strict_directions() {
  for (std::size_t metric = 0; metric < 4; ++metric) {
    auto input = gap_boundary_input(metric);
    const auto member = kGeometryMembers[metric];
    const double neutral = input.neutral_geometry.front().*member;
    const double full = input.full_active_geometry.front().*member;

    input.qualified_geometry[0].*member = neutral;
    input.qualified_geometry[1].*member = neutral;
    input.qualified_geometry[2].*member = full;
    auto result = gate::evaluate_outer_augmentation_training_gate(input);
    auto gap = gap_result(result.geometry, metric);
    require(gap.repair_direction_count == 2 &&
                gap.repair_direction_pass && gap.pass,
            "exactly two strict repair directions must pass; equality must not count");

    if (metric == 2) {
      input.qualified_geometry[0].*member = 0.0;
    } else if (metric == 3) {
      input.qualified_geometry[0].*member = 1.0;
    } else {
      input.qualified_geometry[0].*member = 0.625;
    }
    input.qualified_geometry[1].*member = full;
    input.qualified_geometry[2].*member = full;
    result = gate::evaluate_outer_augmentation_training_gate(input);
    gap = gap_result(result.geometry, metric);
    require(gap.closure_pass && gap.repair_direction_count == 1 &&
                !gap.repair_direction_pass && !gap.pass &&
                !result.replacement_pass,
            "one strict repair direction must fail despite sufficient mean closure");
  }
}

void test_active_gap_non_applicable_cases() {
  for (std::size_t metric = 0; metric < 4; ++metric) {
    auto input = passing_input();
    const auto member = kGeometryMembers[metric];
    const double equal_value = metric == 2 ? 0.20 : (metric == 3 ? 0.85 : 0.50);
    set_repeated_metric(input.neutral_geometry, member, equal_value);
    set_repeated_metric(input.full_active_geometry, member, equal_value);
    set_repeated_metric(input.qualified_geometry, member, equal_value);
    auto result = gate::evaluate_outer_augmentation_training_gate(input);
    auto gap = gap_result(result.geometry, metric);
    require(!gap.applicable && !gap.closure_defined && !gap.closure_finite &&
                gap.pass && result.geometry.required_gap_closures_finite &&
                result.replacement_pass,
            "equal active/reference means must be N/A and pass without division");

    const double neutral = metric == 2 ? 0.25 : (metric == 3 ? 0.80 : 0.45);
    const double full = metric == 2 ? 0.20 : (metric == 3 ? 0.90 : 0.50);
    set_repeated_metric(input.neutral_geometry, member, neutral);
    set_repeated_metric(input.full_active_geometry, member, full);
    set_repeated_metric(input.qualified_geometry, member, full);
    result = gate::evaluate_outer_augmentation_training_gate(input);
    gap = gap_result(result.geometry, metric);
    require(!gap.applicable && !gap.closure_defined && gap.pass &&
                result.replacement_pass,
            "non-harmful reversed active/reference gap must be N/A and pass");
  }
}

void test_nonfinite_active_gap_closures() {
  for (std::size_t metric = 0; metric < 4; ++metric) {
    auto input = passing_input();
    const auto member = kGeometryMembers[metric];
    const double tiny = std::numeric_limits<double>::denorm_min();
    if (metric == 2) {
      set_repeated_metric(input.neutral_geometry, member, 0.0);
      set_repeated_metric(input.full_active_geometry, member, tiny);
      set_repeated_metric(input.qualified_geometry, member, 1.0);
    } else {
      set_repeated_metric(input.neutral_geometry, member, tiny);
      set_repeated_metric(input.full_active_geometry, member, 0.0);
      set_repeated_metric(input.qualified_geometry, member, 1.0);
    }
    const auto result = gate::evaluate_outer_augmentation_training_gate(input);
    const auto gap = gap_result(result.geometry, metric);
    require(result.basic_numeric_inputs_valid && gap.applicable &&
                gap.closure_defined && !gap.closure_finite &&
                !result.geometry.required_gap_closures_finite,
            "overflowed applicable closure must be detected");
    require_invalid(result,
                    "non-finite applicable closure must be numeric invalid");
  }
}

void test_per_seed_active_floor() {
  for (std::size_t seed = 0; seed < gate::kSeedCount; ++seed) {
    auto input = passing_input();
    set_repeated_metric(input.neutral_geometry, &gate::PerSeedGeometry::active,
                        gate::kCandidateActiveDimensionFloor);
    set_repeated_metric(input.full_active_geometry,
                        &gate::PerSeedGeometry::active,
                        gate::kCandidateActiveDimensionFloor);
    set_repeated_metric(input.qualified_geometry,
                        &gate::PerSeedGeometry::active,
                        gate::kCandidateActiveDimensionFloor);
    auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(result.geometry.candidate_active_seed_pass[seed] &&
                result.geometry.all_candidate_active_pass &&
                result.replacement_pass,
            "candidate active equality at 0.75 must pass for every seed");

    input.qualified_geometry[seed].active = std::nextafter(
        gate::kCandidateActiveDimensionFloor,
        -std::numeric_limits<double>::infinity());
    result = gate::evaluate_outer_augmentation_training_gate(input);
    require(!result.geometry.candidate_active_seed_pass[seed] &&
                !result.geometry.all_candidate_active_pass &&
                !result.replacement_pass,
            "candidate active immediately below 0.75 must fail per seed");
  }
}

void test_contrast_numeric_domains() {
  using Input = gate::OuterAugmentationTrainingGateInput;
  using ContrastMember = gate::PairedContrast Input::*;
  constexpr std::array<ContrastMember, 3> contrasts{
      &Input::qualified_minus_full_active,
      &Input::qualified_minus_neutral,
      &Input::full_active_minus_neutral,
  };
  using ScalarMember = double gate::PairedContrast::*;
  constexpr std::array<ScalarMember, 3> scalars{
      &gate::PairedContrast::point,
      &gate::PairedContrast::low,
      &gate::PairedContrast::high,
  };
  const std::array<double, 3> nonfinite{
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(),
  };

  for (const auto contrast_member : contrasts) {
    for (const auto scalar_member : scalars) {
      for (const double value : nonfinite) {
        auto input = passing_input();
        (input.*contrast_member).*scalar_member = value;
        require_invalid(gate::evaluate_outer_augmentation_training_gate(input),
                        "every non-finite contrast scalar must be invalid");
      }
    }

    {
      auto input = passing_input();
      auto &contrast = input.*contrast_member;
      contrast.low = 0.01;
      contrast.high = 0.001;
      require_invalid(gate::evaluate_outer_augmentation_training_gate(input),
                      "every reversed contrast interval must be invalid");
    }
    {
      auto input = passing_input();
      auto &contrast = input.*contrast_member;
      contrast.point = std::nextafter(
          contrast.low, -std::numeric_limits<double>::infinity());
      require_invalid(gate::evaluate_outer_augmentation_training_gate(input),
                      "every point below its interval must be invalid");
    }
    {
      auto input = passing_input();
      auto &contrast = input.*contrast_member;
      contrast.point = std::nextafter(
          contrast.high, std::numeric_limits<double>::infinity());
      require_invalid(gate::evaluate_outer_augmentation_training_gate(input),
                      "every point above its interval must be invalid");
    }
    for (const bool use_low : {true, false}) {
      auto input = passing_input();
      auto &contrast = input.*contrast_member;
      contrast.point = use_low ? contrast.low : contrast.high;
      const auto result = gate::evaluate_outer_augmentation_training_gate(input);
      require(result.basic_numeric_inputs_valid,
              "point equality at either interval boundary must be valid");
    }
    {
      auto input = passing_input();
      auto &contrast = input.*contrast_member;
      contrast.point = contrast.low = contrast.high = 0.001;
      const auto result = gate::evaluate_outer_augmentation_training_gate(input);
      require(result.basic_numeric_inputs_valid,
              "a finite degenerate interval containing its point must be valid");
    }
    for (const int64_t invalid_count : {-1, 4}) {
      auto input = passing_input();
      (input.*contrast_member).positive_seed_count = invalid_count;
      require_invalid(gate::evaluate_outer_augmentation_training_gate(input),
                      "every positive-seed count outside [0,3] must be invalid");
    }
    for (const int64_t boundary_count : {0, 3}) {
      auto input = passing_input();
      (input.*contrast_member).positive_seed_count = boundary_count;
      const auto result = gate::evaluate_outer_augmentation_training_gate(input);
      require(result.basic_numeric_inputs_valid,
              "positive-seed counts at 0 and 3 must be valid");
    }
  }
}

void test_geometry_numeric_domains() {
  using Input = gate::OuterAugmentationTrainingGateInput;
  using GeometryInputMember = gate::GeometryBySeed Input::*;
  constexpr std::array<GeometryInputMember, 3> geometries{
      &Input::neutral_geometry,
      &Input::full_active_geometry,
      &Input::qualified_geometry,
  };
  const std::array<double, 3> nonfinite{
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(),
  };

  for (const auto geometry_member : geometries) {
    for (const auto fraction_member : kGeometryMembers) {
      for (const double value : nonfinite) {
        auto input = passing_input();
        (input.*geometry_member)[1].*fraction_member = value;
        require_invalid(gate::evaluate_outer_augmentation_training_gate(input),
                        "every non-finite geometry fraction must be invalid");
      }
      for (const bool below : {true, false}) {
        auto input = passing_input();
        (input.*geometry_member)[1].*fraction_member =
            below
                ? std::nextafter(0.0,
                                 -std::numeric_limits<double>::infinity())
                : std::nextafter(1.0,
                                 std::numeric_limits<double>::infinity());
        require_invalid(gate::evaluate_outer_augmentation_training_gate(input),
                        "every geometry fraction outside [0,1] must be invalid");
      }
      for (const double boundary : {0.0, 1.0}) {
        auto input = passing_input();
        (input.*geometry_member)[1].*fraction_member = boundary;
        const auto result =
            gate::evaluate_outer_augmentation_training_gate(input);
        require(result.basic_numeric_inputs_valid,
                "geometry fractions at 0 and 1 must satisfy the input domain");
      }
    }
  }
}

void test_family_nonfinite_domains() {
  using Input = gate::OuterAugmentationTrainingGateInput;
  using FamilyMember = gate::FamilyDeltas Input::*;
  constexpr std::array<FamilyMember, 2> family_sets{
      &Input::qualified_minus_full_active_family,
      &Input::qualified_minus_neutral_family,
  };
  const std::array<double, 3> nonfinite{
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(),
  };
  for (const auto family_set : family_sets) {
    for (std::size_t family = 0; family < gate::kFamilyCount; ++family) {
      for (const double value : nonfinite) {
        auto input = passing_input();
        (input.*family_set)[family] = value;
        require_invalid(gate::evaluate_outer_augmentation_training_gate(input),
                        "every non-finite family delta must be invalid");
      }
    }
  }
}

void test_descriptive_contrast_is_validated_but_not_gated() {
  auto input = passing_input();
  input.full_active_minus_neutral = {0.75, 0.50, 1.00, 0};
  auto result = gate::evaluate_outer_augmentation_training_gate(input);
  require(result.representation_improvement_pass,
          "finite descriptive full-active-minus-neutral values must not gate support");

  input.full_active_minus_neutral.point =
      std::numeric_limits<double>::quiet_NaN();
  result = gate::evaluate_outer_augmentation_training_gate(input);
  require_invalid(result,
                  "descriptive contrast still belongs to numeric validity");
}

void test_classification_partition_and_precedence() {
  {
    const auto result =
        gate::evaluate_outer_augmentation_training_gate(passing_input());
    require(result.classification == gate::Classification::
                qualified_candidate_representation_improvement_supported,
            "joint replacement and neutral improvement must use joint class");
  }
  {
    auto input = passing_input();
    input.qualified_minus_neutral = {0.001, 0.0005, 0.002, 1};
    const auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(result.replacement_pass &&
                !result.neutral_improvement_contrast_pass &&
                result.classification == gate::Classification::
                    qualified_candidate_harm_mitigation_only,
            "replacement without neutral improvement must be harm mitigation");
  }
  {
    auto input = passing_input();
    input.qualified_minus_neutral_family[0] = std::nextafter(
        gate::kFamilyDeltaFloor, -std::numeric_limits<double>::infinity());
    const auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(!result.replacement_pass &&
                result.neutral_improvement_contrast_pass &&
                result.classification == gate::Classification::
                    qualified_candidate_neutral_aulc_improvement_only_replacement_not_supported,
            "neutral improvement without replacement must use neutral-only class");
  }
  {
    auto input = passing_input();
    input.qualified_minus_full_active = {0.001, 0.0005, 0.002, 1};
    input.qualified_minus_neutral = {0.001, 0.0005, 0.002, 1};
    const auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(!result.replacement_pass &&
                !result.neutral_improvement_contrast_pass &&
                result.classification == gate::Classification::
                    qualified_candidate_not_supported,
            "failure of both scientific gates must be unsupported");
  }
  {
    auto input = passing_input();
    input.mechanics = false;
    auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(result.numeric_inputs_valid && !result.mechanics_pass &&
                !result.replacement_pass &&
                result.neutral_improvement_contrast_pass &&
                result.classification ==
                    gate::Classification::invalid_numeric_or_mechanics,
            "mechanics invalidity must precede an otherwise passing subgate");

    input.qualified_minus_neutral_family[0] = std::nextafter(
        gate::kFamilyDeltaFloor, -std::numeric_limits<double>::infinity());
    result = gate::evaluate_outer_augmentation_training_gate(input);
    require(result.classification ==
                gate::Classification::invalid_numeric_or_mechanics,
            "mechanics invalidity must precede neutral-only classification");
  }
  {
    auto input = passing_input();
    input.full_active_minus_neutral.low =
        std::numeric_limits<double>::quiet_NaN();
    const auto result = gate::evaluate_outer_augmentation_training_gate(input);
    require(result.neutral_improvement_contrast_pass &&
                result.classification ==
                    gate::Classification::invalid_numeric_or_mechanics,
            "numeric invalidity must precede all scientific classifications");
  }
}

} // namespace

int main() {
  try {
    test_exact_passing_fixture();
    test_classification_names();
    test_replacement_contrast_boundaries();
    test_neutral_noninferiority_boundary();
    test_standalone_neutral_improvement_boundaries();
    test_all_eight_family_boundaries();
    test_better_reference_selection();
    test_seed_means_precede_better_reference_selection();
    test_better_reference_ratio_boundaries();
    test_better_reference_zero_denominators();
    test_nonfinite_required_ratios();
    test_active_gap_closure_boundaries();
    test_active_gap_strict_directions();
    test_active_gap_non_applicable_cases();
    test_nonfinite_active_gap_closures();
    test_per_seed_active_floor();
    test_contrast_numeric_domains();
    test_geometry_numeric_domains();
    test_family_nonfinite_domains();
    test_descriptive_contrast_is_validated_but_not_gated();
    test_classification_partition_and_precedence();
    std::cout << "Outer-augmentation training gate tests passed\n";
    return 0;
  } catch (const std::exception &exception) {
    std::cerr << "Outer-augmentation training gate tests failed: "
              << exception.what() << '\n';
    return 1;
  }
}
