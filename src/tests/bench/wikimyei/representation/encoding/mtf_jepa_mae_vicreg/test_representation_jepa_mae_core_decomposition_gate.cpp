#include "representation_jepa_mae_core_decomposition_gate.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace gate = cuwacunu::tests::mtf_jepa_mae_core_decomposition_gate;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool near(double left, double right) {
  return std::abs(left - right) <= 1.0e-12;
}

gate::PairedContrast material() {
  return {
      .point = 0.003, .low = 0.001, .high = 0.005, .positive_seed_count = 2};
}

gate::PairedContrast no_signal() {
  return {.point = 0.0, .low = -0.001, .high = 0.001, .positive_seed_count = 1};
}

gate::GeometryBySeed geometry(double effective = 0.10,
                              double participation = 0.10, double top = 0.50,
                              double active = 1.0) {
  gate::GeometryBySeed result{};
  result.fill({.effective = effective,
               .participation = participation,
               .top = top,
               .active = active});
  return result;
}

gate::GateInput baseline() {
  gate::GateInput input{};
  input.combined_minus_null = {
      .point = -0.003, .low = -0.005, .high = -0.001, .positive_seed_count = 0};
  input.harmful_interaction_residual = no_signal();
  input.null_geometry = geometry();
  input.combined_geometry = geometry();
  input.jepa.minus_null = no_signal();
  input.jepa.minus_combined = no_signal();
  input.jepa.geometry = geometry();
  input.mae.minus_null = no_signal();
  input.mae.minus_combined = no_signal();
  input.mae.geometry = geometry();
  input.null_identity = true;
  input.mechanics = true;
  input.accepted_reference_reproduced = true;
  return input;
}

void test_default_and_precedence() {
  const auto result = gate::evaluate(baseline());
  expect(result.numeric_inputs_valid, "baseline numeric validity");
  expect(result.combined_declined_from_null, "combined decline diagnostic");
  expect(result.classification ==
             gate::Classification::core_component_marginal_harm_not_localized,
         "baseline classification");

  auto reference_failure = baseline();
  reference_failure.accepted_reference_reproduced = false;
  expect(gate::evaluate(reference_failure).classification ==
             gate::Classification::accepted_jepa_mae_reference_not_reproduced,
         "reference precedence");

  auto invalid = reference_failure;
  invalid.mechanics = false;
  expect(gate::evaluate(invalid).classification ==
             gate::Classification::invalid_numeric_or_mechanics,
         "mechanics precedence");
  invalid = baseline();
  invalid.null_identity = false;
  expect(gate::evaluate(invalid).classification ==
             gate::Classification::invalid_numeric_or_mechanics,
         "null identity precedence");
}

void test_material_boundaries() {
  auto input = baseline();
  input.jepa.minus_combined = {.point = gate::kMaterialityFloor,
                               .low = std::numeric_limits<double>::min(),
                               .high = 0.004,
                               .positive_seed_count = 2};
  auto result = gate::evaluate(input);
  expect(result.jepa.removal_rescue.pass,
         "inclusive point and two-seed boundary");
  expect(result.mae_conditional_harm,
         "JEPA-only rescue maps to MAE conditional harm");

  input.jepa.minus_combined.low = 0.0;
  expect(!gate::evaluate(input).jepa.removal_rescue.pass,
         "zero lower bound is strict failure");
  input.jepa.minus_combined.low = std::numeric_limits<double>::min();
  input.jepa.minus_combined.positive_seed_count = 1;
  expect(!gate::evaluate(input).jepa.removal_rescue.pass,
         "one positive seed fails");
  input.jepa.minus_combined.positive_seed_count = 2;
  input.jepa.minus_combined.point = gate::kMaterialityFloor - 1.0e-12;
  expect(!gate::evaluate(input).jepa.removal_rescue.pass,
         "below materiality fails");

  input = baseline();
  input.jepa.minus_null.low = gate::kStandaloneNonharmMargin;
  expect(!gate::evaluate(input).jepa.standalone_nonharm,
         "nonharm equality is strict failure");
  input.jepa.minus_null.low = gate::kStandaloneNonharmMargin + 1.0e-12;
  expect(gate::evaluate(input).jepa.standalone_nonharm,
         "nonharm above margin passes");
}

void test_conditional_and_interaction_classifications() {
  auto input = baseline();
  input.mae.minus_combined = material();
  expect(gate::evaluate(input).classification ==
             gate::Classification::jepa_conditional_contributor_supported,
         "JEPA conditional contributor");

  input = baseline();
  input.jepa.minus_combined = material();
  expect(gate::evaluate(input).classification ==
             gate::Classification::mae_conditional_contributor_supported,
         "MAE conditional contributor");

  input.mae.minus_combined = material();
  expect(gate::evaluate(input).classification ==
             gate::Classification::both_core_branches_conditionally_harmful,
         "both conditional contributors");

  input.harmful_interaction_residual = material();
  auto result = gate::evaluate(input);
  expect(result.harmful_interaction,
         "factorial interaction requires both rescues and nonharm");
  expect(result.classification ==
             gate::Classification::harmful_jepa_mae_interaction_supported,
         "interaction classification");

  input.jepa.minus_null.low = gate::kStandaloneNonharmMargin;
  result = gate::evaluate(input);
  expect(!result.harmful_interaction,
         "interaction fails without singleton nonharm");
  expect(result.classification ==
             gate::Classification::both_core_branches_conditionally_harmful,
         "conditional classification remains after interaction failure");
}

void test_standalone_and_less_harmful() {
  auto input = baseline();
  input.jepa.minus_combined = material();
  auto result = gate::evaluate(input);
  expect(result.jepa.less_harmful, "removal rescue alone is less harmful");
  expect(!result.jepa.replacement_supported,
         "less harmful is not replacement support");

  input.jepa.minus_null = material();
  result = gate::evaluate(input);
  expect(result.jepa.standalone_improvement.pass,
         "standalone AULC improvement");
  expect(result.jepa.safety.pass, "baseline safety");
  expect(result.jepa.replacement_supported,
         "standalone plus rescue plus safety supports replacement");
  expect(!result.jepa.less_harmful,
         "supported improvement is not less-harmful-only");

  input.jepa.minus_null_family[2] = gate::kFamilyDeltaFloor - 1.0e-12;
  result = gate::evaluate(input);
  expect(!result.jepa.safety.pass, "family harm blocks safety");
  expect(!result.jepa.replacement_supported,
         "family harm blocks replacement support");
  input.jepa.minus_null_family[2] = gate::kFamilyDeltaFloor;
  expect(gate::evaluate(input).jepa.safety.pass,
         "family floor equality passes");
}

void test_geometry_boundaries_and_gaps() {
  auto input = baseline();
  input.null_geometry = geometry(1.0, 1.0, 0.0, 0.75);
  input.combined_geometry = geometry(1.0, 1.0, 0.0, 0.75);
  input.jepa.geometry = geometry(0.90, 0.90, 0.10, 0.75);
  auto result = gate::evaluate(input);
  expect(near(result.jepa.safety.geometry.effective_ratio, 0.90),
         "effective ratio equality");
  expect(near(result.jepa.safety.geometry.participation_ratio, 0.90),
         "participation ratio equality");
  expect(near(result.jepa.safety.geometry.top_ratio, 0.90),
         "top ratio equality");
  expect(result.jepa.safety.geometry.pass,
         "inclusive ratio and active boundaries pass");

  input = baseline();
  input.null_geometry = geometry(0.50, 0.50, 0.50, 1.0);
  input.combined_geometry = geometry(0.4375, 0.4375, 0.5625, 0.875);
  input.jepa.geometry = geometry(0.46875, 0.46875, 0.53125, 0.9375);
  result = gate::evaluate(input);
  expect(near(result.jepa.safety.geometry.effective_gap.closure, 0.50),
         "effective gap equality");
  expect(near(result.jepa.safety.geometry.participation_gap.closure, 0.50),
         "participation gap equality");
  expect(near(result.jepa.safety.geometry.top_gap.closure, 0.50),
         "top gap equality");
  expect(near(result.jepa.safety.geometry.active_gap.closure, 0.50),
         "active gap equality");
  expect(result.jepa.safety.geometry.pass,
         "inclusive geometry boundaries pass");

  input.jepa.geometry[0].effective = 0.4375;
  input.jepa.geometry[1].effective = 0.4375;
  result = gate::evaluate(input);
  expect(!result.jepa.safety.geometry.effective_gap.direction_pass,
         "strict two-seed gap direction");
  expect(!result.jepa.safety.geometry.pass, "gap direction blocks geometry");

  input = baseline();
  input.jepa.geometry = geometry(0.10, 0.10, 0.50, gate::kActiveDimensionFloor);
  expect(gate::evaluate(input).jepa.safety.geometry.pass,
         "active floor equality passes");
  input.jepa.geometry[2].active = gate::kActiveDimensionFloor - 1.0e-12;
  expect(!gate::evaluate(input).jepa.safety.geometry.pass,
         "active below floor fails");
}

void test_invalid_numeric_inputs() {
  auto input = baseline();
  input.jepa.minus_null.point = std::numeric_limits<double>::quiet_NaN();
  expect(gate::evaluate(input).classification ==
             gate::Classification::invalid_numeric_or_mechanics,
         "NaN contrast rejected");

  input = baseline();
  input.mae.minus_combined.low = 0.1;
  expect(gate::evaluate(input).classification ==
             gate::Classification::invalid_numeric_or_mechanics,
         "unordered interval rejected");

  input = baseline();
  input.harmful_interaction_residual.positive_seed_count = 4;
  expect(gate::evaluate(input).classification ==
             gate::Classification::invalid_numeric_or_mechanics,
         "seed count rejected");

  input = baseline();
  input.null_geometry[0].effective = 1.1;
  expect(gate::evaluate(input).classification ==
             gate::Classification::invalid_numeric_or_mechanics,
         "geometry fraction rejected");

  input = baseline();
  input.jepa.minus_combined_family[0] = std::numeric_limits<double>::infinity();
  expect(gate::evaluate(input).classification ==
             gate::Classification::invalid_numeric_or_mechanics,
         "family infinity rejected");
}

} // namespace

int main() {
  try {
    test_default_and_precedence();
    test_material_boundaries();
    test_conditional_and_interaction_classifications();
    test_standalone_and_less_harmful();
    test_geometry_boundaries_and_gaps();
    test_invalid_numeric_inputs();
    std::cout << "representation_jepa_mae_core_decomposition_gate=PASS\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "representation_jepa_mae_core_decomposition_gate=FAIL\n";
    std::cerr << "reason=" << error.what() << '\n';
    return 1;
  }
}
