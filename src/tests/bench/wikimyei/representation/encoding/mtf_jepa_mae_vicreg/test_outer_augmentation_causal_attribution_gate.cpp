#include "outer_augmentation_causal_attribution_gate.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

namespace gate = cuwacunu::tests::oaa1_gate;

void require(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

gate::CandidateInput
rescue_input(gate::Objective objective = gate::Objective::jepa,
             gate::Profile profile = gate::Profile::gaussian_only) {
  gate::CandidateInput input{};
  input.objective = objective;
  input.profile = profile;
  input.candidate_minus_anchor = {0.010, 0.004, 0.016, 3};
  input.candidate_minus_identity_objective = {0.020, 0.010, 0.030, 3};
  input.family_deltas_vs_anchor = {0.010, 0.010, 0.010, -0.010};
  input.safeguards = {true, true, true, true, true};
  input.semantic = true;
  input.mechanics = true;
  input.reproducible_new_safeguard_failure = false;
  return input;
}

gate::CandidateInput unsupported_input(gate::Objective objective,
                                       gate::Profile profile) {
  auto input = rescue_input(objective, profile);
  input.candidate_minus_identity_objective = {0.0, -0.001, 0.001, 1};
  return input;
}

gate::CandidateInventory canonical_inventory(bool rescues = false) {
  gate::CandidateInventory inventory{};
  std::size_t index = 0;
  for (const auto objective :
       {gate::Objective::jepa, gate::Objective::vicreg}) {
    for (const auto profile :
         {gate::Profile::gaussian_only, gate::Profile::amplitude_only,
          gate::Profile::frequency_gain_only,
          gate::Profile::candidate_safe_stack}) {
      inventory[index++] = rescues ? rescue_input(objective, profile)
                                   : unsupported_input(objective, profile);
    }
  }
  return inventory;
}

void require_classification(const gate::CandidateInput &input,
                            gate::Classification expected,
                            const std::string &message) {
  const auto result = gate::evaluate_candidate(input);
  require(result.classification == expected, message);
}

void require_invalid(gate::CandidateInput input, const std::string &message) {
  const auto result = gate::evaluate_candidate(input);
  require(!result.numeric_inputs_valid || !result.mechanics_pass,
          message + ": validity flag");
  require(!result.rescue_pass && !result.mitigation_pass &&
              result.classification ==
                  gate::Classification::invalid_numeric_or_mechanics,
          message + ": invalidity precedence");
}

void test_names_and_protocol_pin() {
  require(
      std::string(gate::kProtocolSha256) ==
          "3370817d8e81686961ce87ab8cd99157616e4bc2cee3a9d262f674b1f2f3b4a2",
      "gate must identify the corrected frozen OAA-1 protocol");
  require(std::string(gate::objective_name(gate::Objective::jepa)) == "jepa" &&
              std::string(gate::objective_name(gate::Objective::vicreg)) ==
                  "vicreg",
          "objective names must be frozen");
  require(
      std::string(gate::profile_name(gate::Profile::gaussian_only)) ==
              "gaussian_only" &&
          std::string(gate::profile_name(gate::Profile::amplitude_only)) ==
              "amplitude_only" &&
          std::string(gate::profile_name(gate::Profile::frequency_gain_only)) ==
              "frequency_gain_only" &&
          std::string(gate::profile_name(
              gate::Profile::candidate_safe_stack)) == "candidate_safe_stack",
      "profile names and tie order must be frozen");
  using C = gate::Classification;
  require(
      std::string(gate::classification_name(C::invalid_numeric_or_mechanics)) ==
              "invalid_numeric_or_mechanics" &&
          std::string(gate::classification_name(
              C::outer_augmentation_rescues_objective)) ==
              "outer_augmentation_rescues_objective" &&
          std::string(gate::classification_name(
              C::outer_augmentation_mitigates_harm_only)) ==
              "outer_augmentation_mitigates_harm_only" &&
          std::string(gate::classification_name(
              C::outer_augmentation_worsens_objective)) ==
              "outer_augmentation_worsens_objective" &&
          std::string(
              gate::classification_name(C::outer_augmentation_not_supported)) ==
              "outer_augmentation_not_supported",
      "classification names must be frozen");
}

void test_complete_rescue() {
  const auto result = gate::evaluate_candidate(rescue_input());
  require(
      result.numeric_inputs_valid && result.mechanics_pass &&
          result.semantic_pass && result.anchor_point_pass &&
          result.anchor_lower_bound_pass && result.all_anchor_seeds_improve &&
          result.identity_lower_bound_pass &&
          result.positive_family_count == 3 &&
          result.positive_family_count_pass && result.all_family_floors_pass &&
          result.all_frozen_safeguards_pass && result.rescue_pass &&
          !result.worsening_pass && !result.mitigation_pass &&
          result.classification ==
              gate::Classification::outer_augmentation_rescues_objective,
      "complete rescue fixture must pass every conjunctive clause");
}

void test_rescue_threshold_boundaries() {
  {
    auto input = rescue_input();
    input.candidate_minus_anchor.point = gate::kRescueMaterialityFloor;
    auto result = gate::evaluate_candidate(input);
    require(result.anchor_point_pass && result.rescue_pass,
            "+0.005 equality must pass");
    input.candidate_minus_anchor.point =
        std::nextafter(gate::kRescueMaterialityFloor,
                       -std::numeric_limits<double>::infinity());
    result = gate::evaluate_candidate(input);
    require(
        !result.anchor_point_pass && !result.rescue_pass &&
            result.classification ==
                gate::Classification::outer_augmentation_mitigates_harm_only,
        "one representable value below +0.005 must fail rescue");
  }
  {
    auto input = rescue_input();
    input.candidate_minus_anchor.low = 0.0;
    auto result = gate::evaluate_candidate(input);
    require(!result.anchor_lower_bound_pass && !result.rescue_pass,
            "anchor lower-bound equality at zero must fail");
    input.candidate_minus_anchor.low =
        std::nextafter(0.0, std::numeric_limits<double>::infinity());
    result = gate::evaluate_candidate(input);
    require(result.anchor_lower_bound_pass && result.rescue_pass,
            "anchor lower bound immediately above zero must pass");
  }
  {
    auto input = rescue_input();
    input.candidate_minus_identity_objective.low = 0.0;
    auto result = gate::evaluate_candidate(input);
    require(!result.identity_lower_bound_pass && !result.rescue_pass &&
                !result.mitigation_pass &&
                result.classification ==
                    gate::Classification::outer_augmentation_not_supported,
            "identity lower-bound equality at zero must fail rescue and "
            "mitigation");
    input.candidate_minus_identity_objective.low =
        std::nextafter(0.0, std::numeric_limits<double>::infinity());
    result = gate::evaluate_candidate(input);
    require(result.identity_lower_bound_pass && result.rescue_pass,
            "identity lower bound immediately above zero must pass");
  }
  {
    auto input = rescue_input();
    input.candidate_minus_anchor.positive_seed_count = 2;
    auto result = gate::evaluate_candidate(input);
    require(
        !result.all_anchor_seeds_improve && !result.rescue_pass &&
            result.classification ==
                gate::Classification::outer_augmentation_mitigates_harm_only,
        "two of three anchor seeds must fail the all-three clause");
    input.candidate_minus_anchor.positive_seed_count = 3;
    result = gate::evaluate_candidate(input);
    require(result.all_anchor_seeds_improve && result.rescue_pass,
            "three of three anchor seeds must pass");
  }
}

void test_family_clauses() {
  for (std::size_t family = 0; family < gate::kFamilyCount; ++family) {
    auto input = rescue_input();
    input.family_deltas_vs_anchor[family] = gate::kFamilyDeltaFloor;
    if (family < 3) {
      input.family_deltas_vs_anchor[3] = 0.01;
    }
    auto result = gate::evaluate_candidate(input);
    require(result.family_floor_pass[family] && result.all_family_floors_pass &&
                result.rescue_pass,
            "family floor equality at -0.02 must pass for every family");

    input.family_deltas_vs_anchor[family] = std::nextafter(
        gate::kFamilyDeltaFloor, -std::numeric_limits<double>::infinity());
    result = gate::evaluate_candidate(input);
    require(
        !result.family_floor_pass[family] && !result.all_family_floors_pass &&
            !result.rescue_pass &&
            result.classification ==
                gate::Classification::outer_augmentation_mitigates_harm_only,
        "family immediately below -0.02 must fail rescue");
  }

  {
    auto input = rescue_input();
    input.family_deltas_vs_anchor = {0.01, 0.01, 0.0, 0.0};
    const auto result = gate::evaluate_candidate(input);
    require(result.positive_family_count == 2 &&
                !result.positive_family_count_pass && !result.rescue_pass,
            "zero is not a positive family and two positives must fail");
  }
  {
    auto input = rescue_input();
    input.family_deltas_vs_anchor = {
        std::nextafter(0.0, std::numeric_limits<double>::infinity()),
        std::nextafter(0.0, std::numeric_limits<double>::infinity()),
        std::nextafter(0.0, std::numeric_limits<double>::infinity()), 0.0};
    const auto result = gate::evaluate_candidate(input);
    require(result.positive_family_count == 3 &&
                result.positive_family_count_pass && result.rescue_pass,
            "three strictly positive families must pass");
  }
}

void test_safeguard_and_semantic_clauses() {
  using Member = bool gate::FrozenSafeguards::*;
  constexpr std::array<Member, 5> members{
      &gate::FrozenSafeguards::raw_control,
      &gate::FrozenSafeguards::reversal_order,
      &gate::FrozenSafeguards::continuous_shuffle,
      &gate::FrozenSafeguards::order_shuffle,
      &gate::FrozenSafeguards::geometry,
  };
  for (const auto member : members) {
    auto input = rescue_input();
    input.safeguards.*member = false;
    auto result = gate::evaluate_candidate(input);
    require(
        !result.all_frozen_safeguards_pass && !result.rescue_pass &&
            result.classification ==
                gate::Classification::outer_augmentation_mitigates_harm_only,
        "a failed but not-new safeguard must block rescue only");

    input.reproducible_new_safeguard_failure = true;
    result = gate::evaluate_candidate(input);
    require(result.worsening_pass && !result.mitigation_pass &&
                result.classification ==
                    gate::Classification::outer_augmentation_worsens_objective,
            "a reproducible new safeguard failure must classify worsening");
  }

  auto input = rescue_input();
  input.semantic = false;
  const auto result = gate::evaluate_candidate(input);
  require(result.numeric_inputs_valid && result.mechanics_pass &&
              !result.semantic_pass && !result.rescue_pass &&
              !result.mitigation_pass &&
              result.classification ==
                  gate::Classification::outer_augmentation_not_supported,
          "semantic failure is a scientific stop, not numeric invalidity");
}

void test_worsens_and_not_supported_boundaries() {
  {
    auto input = rescue_input();
    input.candidate_minus_identity_objective = {
        -0.010, -0.020,
        std::nextafter(0.0, -std::numeric_limits<double>::infinity()), 0};
    const auto result = gate::evaluate_candidate(input);
    require(result.identity_upper_bound_strictly_negative &&
                result.worsening_pass &&
                result.classification ==
                    gate::Classification::outer_augmentation_worsens_objective,
            "identity upper bound immediately below zero must worsen");
  }
  {
    auto input = rescue_input();
    input.candidate_minus_identity_objective = {-0.005, -0.010, 0.0, 0};
    const auto result = gate::evaluate_candidate(input);
    require(!result.identity_upper_bound_strictly_negative &&
                !result.worsening_pass && !result.mitigation_pass &&
                result.classification ==
                    gate::Classification::outer_augmentation_not_supported,
            "identity upper-bound equality at zero must not worsen");
  }
  {
    auto input =
        unsupported_input(gate::Objective::jepa, gate::Profile::gaussian_only);
    require_classification(
        input, gate::Classification::outer_augmentation_not_supported,
        "an interval crossing zero without other evidence is unsupported");
  }
}

void test_numeric_and_mechanics_invalidity() {
  using ContrastMember = gate::PairedContrast gate::CandidateInput::*;
  constexpr std::array<ContrastMember, 2> contrasts{
      &gate::CandidateInput::candidate_minus_anchor,
      &gate::CandidateInput::candidate_minus_identity_objective,
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
        auto input = rescue_input();
        (input.*contrast_member).*scalar_member = value;
        require_invalid(input,
                        "every non-finite contrast value must invalidate");
      }
    }
    {
      auto input = rescue_input();
      auto &contrast = input.*contrast_member;
      contrast.low = 0.02;
      contrast.high = 0.01;
      require_invalid(input, "reversed interval must invalidate");
    }
    for (const std::int64_t count : {-1, 4}) {
      auto input = rescue_input();
      (input.*contrast_member).positive_seed_count = count;
      require_invalid(input, "seed count outside [0,3] must invalidate");
    }
  }

  {
    auto input = rescue_input();
    input.candidate_minus_anchor = {0.005, 0.006, 0.010, 3};
    const auto result = gate::evaluate_candidate(input);
    require(result.numeric_inputs_valid && result.rescue_pass,
            "a percentile interval need not contain its plug-in point");
  }

  for (std::size_t family = 0; family < gate::kFamilyCount; ++family) {
    for (const double value : nonfinite) {
      auto input = rescue_input();
      input.family_deltas_vs_anchor[family] = value;
      require_invalid(input, "every non-finite family must invalidate");
    }
  }

  {
    auto input = rescue_input();
    input.objective = static_cast<gate::Objective>(99);
    require_invalid(input, "unknown objective must invalidate");
  }
  {
    auto input = rescue_input();
    input.profile = static_cast<gate::Profile>(99);
    require_invalid(input, "unknown profile must invalidate");
  }
  {
    auto input = rescue_input();
    input.mechanics = false;
    input.reproducible_new_safeguard_failure = true;
    const auto result = gate::evaluate_candidate(input);
    require(result.numeric_inputs_valid && !result.mechanics_pass &&
                result.worsening_pass == false &&
                result.classification ==
                    gate::Classification::invalid_numeric_or_mechanics,
            "mechanical invalidity must precede rescue and worsening");
  }
}

void test_experiment_selection_and_ties() {
  {
    auto inventory = canonical_inventory(false);
    inventory[7] = rescue_input(gate::Objective::vicreg,
                                gate::Profile::candidate_safe_stack);
    inventory[7].candidate_minus_anchor = {0.020, 0.010, 0.030, 3};
    inventory[1] =
        rescue_input(gate::Objective::jepa, gate::Profile::amplitude_only);
    inventory[1].candidate_minus_anchor = {0.030, 0.020, 0.040, 3};
    const auto result = gate::evaluate_experiment(inventory);
    require(result.inventory_exact && result.experiment_valid &&
                result.rescue_candidate_count == 2 &&
                result.has_selected_rescue &&
                result.selected_candidate_index == 1 &&
                result.selected_objective == gate::Objective::jepa &&
                result.selected_profile == gate::Profile::amplitude_only &&
                result.confirmation_open_authorized,
            "greatest anchor point must select independently of tie priority");
  }

  {
    auto inventory = canonical_inventory(true);
    for (auto &candidate : inventory) {
      candidate.candidate_minus_anchor = {0.010, 0.004, 0.016, 3};
    }
    std::reverse(inventory.begin(), inventory.end());
    const auto result = gate::evaluate_experiment(inventory);
    require(result.experiment_valid && result.rescue_candidate_count == 8 &&
                result.selected_objective == gate::Objective::jepa &&
                result.selected_profile == gate::Profile::gaussian_only,
            "exact ties must use objective-first then profile frozen order");
  }

  {
    auto inventory = canonical_inventory(false);
    inventory[3] = rescue_input(gate::Objective::jepa,
                                gate::Profile::candidate_safe_stack);
    inventory[4] =
        rescue_input(gate::Objective::vicreg, gate::Profile::gaussian_only);
    inventory[3].candidate_minus_anchor = {0.012, 0.005, 0.020, 3};
    inventory[4].candidate_minus_anchor = {0.012, 0.005, 0.020, 3};
    const auto result = gate::evaluate_experiment(inventory);
    require(result.selected_objective == gate::Objective::jepa &&
                result.selected_profile == gate::Profile::candidate_safe_stack,
            "objective tie order must precede profile tie order");
  }

  {
    const auto result = gate::evaluate_experiment(canonical_inventory(false));
    require(result.experiment_valid && result.rescue_candidate_count == 0 &&
                !result.has_selected_rescue &&
                result.selected_candidate_index == gate::kNoCandidate &&
                !result.confirmation_open_authorized,
            "no rescue must leave confirmation closed");
  }
}

void test_experiment_invalidity_and_inventory() {
  {
    auto inventory = canonical_inventory(false);
    inventory[0] =
        rescue_input(gate::Objective::jepa, gate::Profile::gaussian_only);
    inventory[7].mechanics = false;
    const auto result = gate::evaluate_experiment(inventory);
    require(result.inventory_exact && !result.all_numeric_and_mechanics_valid &&
                !result.experiment_valid && !result.has_selected_rescue &&
                !result.confirmation_open_authorized,
            "one invalid arm must invalidate selection despite another rescue");
  }
  {
    auto inventory = canonical_inventory(false);
    inventory[0] = inventory[1];
    const auto result = gate::evaluate_experiment(inventory);
    require(!result.inventory_exact && !result.experiment_valid &&
                !result.has_selected_rescue &&
                !result.confirmation_open_authorized,
            "duplicate/missing treatment inventory must fail closed");
  }
}

} // namespace

int main() {
  try {
    test_names_and_protocol_pin();
    test_complete_rescue();
    test_rescue_threshold_boundaries();
    test_family_clauses();
    test_safeguard_and_semantic_clauses();
    test_worsens_and_not_supported_boundaries();
    test_numeric_and_mechanics_invalidity();
    test_experiment_selection_and_ties();
    test_experiment_invalidity_and_inventory();
    std::cout << "OAA-1 causal-attribution gate tests passed\n";
    return 0;
  } catch (const std::exception &exception) {
    std::cerr << "OAA-1 causal-attribution gate tests failed: "
              << exception.what() << '\n';
    return 1;
  }
}
