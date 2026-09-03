#include "representation_surface_sufficiency_map_gate.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace gate = cuwacunu::tests::mtf_surface_sufficiency_map_gate;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

double above(double value) {
  return std::nextafter(value, std::numeric_limits<double>::infinity());
}

double below(double value) {
  return std::nextafter(value, -std::numeric_limits<double>::infinity());
}

gate::TransitionInput unresolved() {
  return {.point = 0.0,
          .low = -0.03,
          .high = 0.03,
          .seed_deltas = {0.0, 0.0, 0.0},
          .family_deltas = {0.0, 0.0, 0.0, 0.0}};
}

gate::TransitionInput material_loss() {
  const double negative = -std::numeric_limits<double>::denorm_min();
  return {.point = -gate::kMaterialityThreshold,
          .low = -0.03,
          .high = negative,
          .seed_deltas = {negative, negative, 0.0},
          .family_deltas = {negative, negative, 0.0, 0.0}};
}

gate::TransitionInput family_specific_loss() {
  auto input = material_loss();
  input.family_deltas = {-std::numeric_limits<double>::denorm_min(), 0.0, 0.0,
                         0.0};
  return input;
}

gate::TransitionInput noninferior() {
  const double seed_pass = above(gate::kNoninferiorityMargin);
  const double family_pass = above(gate::kFamilyNoninferiorityMargin);
  return {
      .point = 0.0,
      .low = seed_pass,
      .high = 0.01,
      .seed_deltas = {seed_pass, seed_pass, gate::kNoninferiorityMargin},
      .family_deltas = {family_pass, family_pass, family_pass, family_pass}};
}

gate::TransitionInput material_gain() {
  const double positive = std::numeric_limits<double>::denorm_min();
  return {.point = gate::kMaterialityThreshold,
          .low = positive,
          .high = 0.03,
          .seed_deltas = {positive, positive, 0.0},
          .family_deltas = {0.0, 0.0, 0.0, 0.0}};
}

gate::ValidityInput passing_validity() {
  return {.no_optimizer_or_backward = true,
          .parameters_and_rng_unchanged = true,
          .repeated_capture_identical = true,
          .surface_identity_hashes_match = true,
          .projections_valid = true,
          .accepted_reference_reproduced = true,
          .legacy_raw_reference_reproduced = true,
          .tokenizer_plan_reproduced = true,
          .normalized_fixed96_informative = true,
          .shuffled_controls_pass = true};
}

gate::TrackInput neutral_track() {
  return {.tokenizer_minus_raw = unresolved(),
          .encoder_minus_tokenizer = unresolved(),
          .served_minus_encoder = unresolved(),
          .served_minus_raw = unresolved()};
}

gate::GateInput baseline() {
  return {.native = neutral_track(),
          .fixed96 = neutral_track(),
          .legacy_served_minus_raw = unresolved(),
          .validity = passing_validity()};
}

void test_material_loss_boundaries() {
  auto input = material_loss();
  auto result = gate::evaluate_transition(input);
  expect(result.numeric_valid, "material-loss boundary is valid");
  expect(result.macro_loss_point_pass, "loss point equality passes");
  expect(result.macro_loss_interval_pass,
         "strictly negative upper bound passes");
  expect(result.negative_seed_count == 2,
         "exactly two negative seed directions pass");
  expect(result.negative_family_count == 2,
         "exactly two negative family points pass");
  expect(result.classification == gate::TransitionClassification::material_loss,
         "all material-loss boundaries classify as material loss");

  input = material_loss();
  input.point = above(-gate::kMaterialityThreshold);
  expect(!gate::evaluate_transition(input).macro_loss,
         "loss point above -0.02 fails");
  input = material_loss();
  input.high = 0.0;
  expect(!gate::evaluate_transition(input).macro_loss,
         "zero interval upper bound fails strict loss interval");
  input = material_loss();
  input.seed_deltas[1] = 0.0;
  expect(!gate::evaluate_transition(input).macro_loss,
         "one negative seed direction fails");

  input = family_specific_loss();
  result = gate::evaluate_transition(input);
  expect(result.macro_loss, "family-specific case retains macro loss");
  expect(result.negative_family_count == 1,
         "one negative family is counted exactly");
  expect(result.classification ==
             gate::TransitionClassification::family_specific_loss,
         "exactly one negative family classifies family-specific loss");
  input.family_deltas[0] = 0.0;
  result = gate::evaluate_transition(input);
  expect(!result.family_specific_loss,
         "zero negative families fails the family-specific rule");
  expect(result.classification == gate::TransitionClassification::unresolved,
         "zero negative families is inconsistent macro evidence");
}

void test_noninferiority_boundaries() {
  auto input = noninferior();
  auto result = gate::evaluate_transition(input);
  expect(result.noninferiority_interval_pass,
         "interval just above -0.02 passes");
  expect(result.noninferior_seed_count == 2,
         "exactly two seed deltas above -0.02 pass");
  expect(result.all_families_noninferior,
         "all family deltas just above -0.05 pass");
  expect(result.classification == gate::TransitionClassification::noninferior,
         "noninferiority boundaries classify as noninferior");

  input = noninferior();
  input.low = gate::kNoninferiorityMargin;
  expect(!gate::evaluate_transition(input).noninferior,
         "interval equality at -0.02 fails");
  input = noninferior();
  input.seed_deltas[1] = gate::kNoninferiorityMargin;
  expect(!gate::evaluate_transition(input).noninferior,
         "only one seed above -0.02 fails");
  input = noninferior();
  input.family_deltas[3] = gate::kFamilyNoninferiorityMargin;
  expect(!gate::evaluate_transition(input).noninferior,
         "family equality at -0.05 fails");
}

void test_material_gain_boundaries_and_precedence() {
  auto input = material_gain();
  auto result = gate::evaluate_transition(input);
  expect(result.material_gain_point_pass, "gain point equality passes");
  expect(result.material_gain_interval_pass,
         "strictly positive lower bound passes");
  expect(result.positive_seed_count == 2,
         "exactly two positive seed directions pass");
  expect(result.noninferior, "material gain also satisfies noninferiority");
  expect(result.classification == gate::TransitionClassification::material_gain,
         "material gain takes precedence over overlapping noninferiority");

  input = material_gain();
  input.point = below(gate::kMaterialityThreshold);
  result = gate::evaluate_transition(input);
  expect(!result.material_gain, "gain point below +0.02 fails");
  expect(result.classification == gate::TransitionClassification::noninferior,
         "failed gain can remain noninferior");
  input = material_gain();
  input.low = 0.0;
  expect(!gate::evaluate_transition(input).material_gain,
         "zero lower bound fails strict gain interval");
  input = material_gain();
  input.seed_deltas[1] = 0.0;
  expect(!gate::evaluate_transition(input).material_gain,
         "one positive seed direction fails");
}

void test_invalid_numeric_inputs() {
  expect(gate::evaluate_transition(unresolved()).classification ==
             gate::TransitionClassification::unresolved,
         "valid evidence outside every frozen gate is unresolved");

  auto input = unresolved();
  input.point = std::numeric_limits<double>::quiet_NaN();
  expect(gate::evaluate_transition(input).classification ==
             gate::TransitionClassification::invalid_numeric,
         "NaN point is rejected");
  input = unresolved();
  input.low = 0.01;
  expect(gate::evaluate_transition(input).classification ==
             gate::TransitionClassification::invalid_numeric,
         "unordered interval is rejected");
  input = unresolved();
  input.seed_deltas[0] = std::numeric_limits<double>::infinity();
  expect(gate::evaluate_transition(input).classification ==
             gate::TransitionClassification::invalid_numeric,
         "infinite seed delta is rejected");
  input = unresolved();
  input.family_deltas[0] = -std::numeric_limits<double>::infinity();
  expect(gate::evaluate_transition(input).classification ==
             gate::TransitionClassification::invalid_numeric,
         "infinite family delta is rejected");
}

void test_aligned_stage_terminal_classifications() {
  auto input = baseline();
  input.fixed96.served_minus_raw = material_loss();
  input.native.tokenizer_minus_raw = material_loss();
  input.fixed96.tokenizer_minus_raw = material_loss();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::token_construction_loss,
         "aligned material tokenizer losses localize token construction");

  input.fixed96.tokenizer_minus_raw = family_specific_loss();
  expect(
      gate::evaluate(input).classification ==
          gate::TerminalClassification::token_construction_family_specific_loss,
      "mixed tokenizer losses preserve family-specific evidence");

  input = baseline();
  input.fixed96.served_minus_raw = material_loss();
  input.native.encoder_minus_tokenizer = family_specific_loss();
  input.fixed96.encoder_minus_tokenizer = family_specific_loss();
  expect(
      gate::evaluate(input).classification ==
          gate::TerminalClassification::encoder_processing_family_specific_loss,
      "aligned family-specific encoder losses localize encoder processing");
  input.native.encoder_minus_tokenizer = material_loss();
  input.fixed96.encoder_minus_tokenizer = material_loss();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::encoder_processing_loss,
         "aligned material encoder losses localize encoder processing");

  input = baseline();
  input.fixed96.served_minus_raw = material_loss();
  input.native.served_minus_encoder = material_loss();
  input.fixed96.served_minus_encoder = family_specific_loss();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::serving_pooling_family_specific_loss,
         "mixed serving losses preserve family-specific evidence");
  input.fixed96.served_minus_encoder = material_loss();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::serving_pooling_loss,
         "aligned material serving losses localize serving pooling");

  input.native.tokenizer_minus_raw = material_loss();
  input.fixed96.tokenizer_minus_raw = material_loss();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::token_construction_loss,
         "earliest aligned loss takes precedence over later aligned loss");
}

void test_remaining_terminal_tree() {
  auto input = baseline();
  input.fixed96.served_minus_raw = material_loss();
  input.native.served_minus_encoder = family_specific_loss();
  input.fixed96.served_minus_encoder = noninferior();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::prepool_width_advantage_only,
         "native-only serving loss with fixed-96 noninferiority is width-only");

  input = baseline();
  input.fixed96.served_minus_raw = material_loss();
  input.native.tokenizer_minus_raw = material_loss();
  input.fixed96.encoder_minus_tokenizer = family_specific_loss();
  input.native.served_minus_encoder = material_loss();
  input.fixed96.served_minus_encoder = material_loss();
  auto result = gate::evaluate(input);
  expect(result.native.earliest_adjacent_loss ==
             gate::AdjacentStage::token_construction,
         "native earliest stage is token construction");
  expect(result.fixed96.earliest_adjacent_loss ==
             gate::AdjacentStage::encoder_processing,
         "fixed-96 earliest stage is encoder processing");
  expect(result.classification ==
             gate::TerminalClassification::projection_sensitive_localization,
         "different earliest stages are projection-sensitive despite later "
         "overlap");

  input = baseline();
  input.fixed96.served_minus_raw = material_loss();
  input.native.tokenizer_minus_raw = family_specific_loss();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::projection_sensitive_localization,
         "one-track adjacent localization is projection-sensitive");

  input = baseline();
  input.fixed96.served_minus_raw = family_specific_loss();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::distributed_internal_loss,
         "fixed-96 total loss without adjacent loss is distributed");

  input = baseline();
  input.fixed96.served_minus_raw = noninferior();
  input.native.served_minus_raw = material_loss();
  input.legacy_served_minus_raw = material_loss();
  expect(
      gate::evaluate(input).classification ==
          gate::TerminalClassification::legacy_raw_gap_normalization_confounded,
      "legacy total gap precedes a native-only normalized gap");

  input = baseline();
  input.native.served_minus_raw = material_gain();
  input.fixed96.served_minus_raw = noninferior();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::no_material_surface_gap_reproduced,
         "two non-loss totals do not reproduce a material surface gap");

  input = baseline();
  input.native.served_minus_raw = material_loss();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::projection_sensitive_localization,
         "native-only total loss is projection-sensitive");

  input = baseline();
  input.native.tokenizer_minus_raw = material_loss();
  input.fixed96.tokenizer_minus_raw = material_loss();
  input.native.served_minus_raw = material_gain();
  input.fixed96.served_minus_raw = material_gain();
  result = gate::evaluate(input);
  expect(result.classification ==
             gate::TerminalClassification::no_material_surface_gap_reproduced,
         "adjacent loss followed by total recovery does not localize a stage");
  expect(result.classification !=
             gate::TerminalClassification::token_construction_loss,
         "fixed-96 total recovery blocks the adjacent stage label");
}

void test_validity_precedence() {
  using ValidityMember = bool gate::ValidityInput::*;
  constexpr std::array<ValidityMember, 5> mechanics_controls{
      &gate::ValidityInput::no_optimizer_or_backward,
      &gate::ValidityInput::parameters_and_rng_unchanged,
      &gate::ValidityInput::repeated_capture_identical,
      &gate::ValidityInput::surface_identity_hashes_match,
      &gate::ValidityInput::projections_valid,
  };
  for (ValidityMember control : mechanics_controls) {
    auto input = baseline();
    input.native.tokenizer_minus_raw = material_loss();
    input.fixed96.tokenizer_minus_raw = material_loss();
    input.validity.*control = false;
    input.validity.accepted_reference_reproduced = false;
    input.validity.legacy_raw_reference_reproduced = false;
    input.validity.tokenizer_plan_reproduced = false;
    input.validity.normalized_fixed96_informative = false;
    input.validity.shuffled_controls_pass = false;
    const auto result = gate::evaluate(input);
    expect(result.numeric_inputs_valid,
           "mechanics failure keeps numeric validity");
    expect(!result.validity_controls_pass, "failed control is reported");
    expect(result.classification ==
               gate::TerminalClassification::invalid_numeric_or_mechanics,
           "mechanics failure precedes reference and scientific failures");
  }

  struct FailureCase {
    ValidityMember member;
    gate::TerminalClassification classification;
  };
  constexpr std::array<FailureCase, 5> ordered_failures{
      FailureCase{&gate::ValidityInput::accepted_reference_reproduced,
                  gate::TerminalClassification::
                      accepted_step_zero_reference_not_reproduced},
      FailureCase{
          &gate::ValidityInput::legacy_raw_reference_reproduced,
          gate::TerminalClassification::legacy_raw_reference_not_reproduced},
      FailureCase{&gate::ValidityInput::tokenizer_plan_reproduced,
                  gate::TerminalClassification::tokenizer_plan_not_reproduced},
      FailureCase{
          &gate::ValidityInput::normalized_fixed96_informative,
          gate::TerminalClassification::normalized_raw_control_not_informative},
      FailureCase{&gate::ValidityInput::shuffled_controls_pass,
                  gate::TerminalClassification::shuffled_target_leakage},
  };
  for (std::size_t failure = 0; failure < ordered_failures.size(); ++failure) {
    auto input = baseline();
    input.native.tokenizer_minus_raw = material_loss();
    input.fixed96.tokenizer_minus_raw = material_loss();
    for (std::size_t later = failure; later < ordered_failures.size();
         ++later) {
      input.validity.*ordered_failures[later].member = false;
    }
    const auto result = gate::evaluate(input);
    expect(result.numeric_inputs_valid,
           "validity failure keeps numeric validity");
    expect(!result.validity_controls_pass, "failed control is reported");
    expect(result.classification == ordered_failures[failure].classification,
           "validity failures follow frozen precedence");
  }

  auto input = baseline();
  input.fixed96.served_minus_raw.point =
      std::numeric_limits<double>::quiet_NaN();
  const auto result = gate::evaluate(input);
  expect(!result.numeric_inputs_valid, "gate reports invalid transition input");
  expect(result.classification ==
             gate::TerminalClassification::invalid_numeric_or_mechanics,
         "numeric failure precedes terminal interpretation");
}

void test_classification_names() {
  expect(std::string(gate::transition_classification_name(
             gate::TransitionClassification::family_specific_loss)) ==
             "family_specific_loss",
         "transition classification name is stable");
  expect(std::string(gate::terminal_classification_name(
             gate::TerminalClassification::
                 serving_pooling_family_specific_loss)) ==
             "serving_pooling_family_specific_loss",
         "terminal classification name is stable");
  expect(std::string(gate::terminal_classification_name(
             gate::TerminalClassification::
                 accepted_step_zero_reference_not_reproduced)) ==
             "accepted_step_zero_reference_not_reproduced",
         "accepted-reference failure name is stable");
  expect(
      std::string(gate::terminal_classification_name(
          gate::TerminalClassification::legacy_raw_reference_not_reproduced)) ==
          "legacy_raw_reference_not_reproduced",
      "legacy-reference failure name is stable");
  expect(std::string(gate::terminal_classification_name(
             gate::TerminalClassification::tokenizer_plan_not_reproduced)) ==
             "tokenizer_plan_not_reproduced",
         "tokenizer-plan failure name is stable");
  expect(std::string(gate::terminal_classification_name(
             gate::TerminalClassification::
                 normalized_raw_control_not_informative)) ==
             "normalized_raw_control_not_informative",
         "normalized-control failure name is stable");
  expect(std::string(gate::terminal_classification_name(
             gate::TerminalClassification::shuffled_target_leakage)) ==
             "shuffled_target_leakage",
         "shuffled-target failure name is stable");
  expect(std::string(gate::stage_name(
             gate::AdjacentStage::encoder_processing)) == "encoder_processing",
         "stage name is stable");
}

} // namespace

int main() {
  try {
    test_material_loss_boundaries();
    test_noninferiority_boundaries();
    test_material_gain_boundaries_and_precedence();
    test_invalid_numeric_inputs();
    test_aligned_stage_terminal_classifications();
    test_remaining_terminal_tree();
    test_validity_precedence();
    test_classification_names();
    std::cout << "representation_surface_sufficiency_map_gate=PASS\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "representation_surface_sufficiency_map_gate=FAIL\n";
    std::cerr << "reason=" << error.what() << '\n';
    return 1;
  }
}
