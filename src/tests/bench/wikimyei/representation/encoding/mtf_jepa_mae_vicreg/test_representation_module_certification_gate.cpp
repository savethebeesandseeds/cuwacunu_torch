#include "representation_module_certification_gate.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace gate =
    cuwacunu::tests::representation_module_certification_gate;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

gate::CandidateInput passing_candidate() {
  gate::CandidateInput input{};
  input.trained_minus_initialization = {0.012, 0.004, 0.020, 3};
  input.final_minus_raw = {0.008, -0.004, 0.018, 2};
  input.learned_family_deltas = {0.010, 0.008, 0.004, -0.005};
  input.final_order_point = 0.93;
  input.final_order_low = 0.90;
  input.order_trained_minus_initialization = {0.001, -0.010, 0.012, 2};
  input.continuous_shuffle_high = 0.01;
  input.order_shuffle_low = 0.47;
  input.order_shuffle_high = 0.53;
  input.mechanics = true;
  for (auto &seed : input.geometry) {
    for (auto &channel : seed) {
      channel = {.effective = 0.40,
                 .participation = 0.45,
                 .top = 0.50,
                 .active = 0.90};
    }
  }
  return input;
}

gate::GateInput passing_input() {
  gate::GateInput input{};
  input.neutral = passing_candidate();
  input.qualified = passing_candidate();
  input.qualified_minus_neutral = {0.0001, -0.001, 0.0012, 1};
  input.global_mechanics = true;
  return input;
}

} // namespace

int main() {
  try {
    {
      const auto result = gate::evaluate(passing_input());
      expect(result.classification == gate::Classification::neutral_candidate,
             "passing equivalent arms must prefer neutral");
      expect(result.neutral.pass && result.qualified.pass,
             "passing candidate fixtures failed");
    }
    {
      auto input = passing_input();
      input.qualified_minus_neutral = {0.004, 0.001, 0.007, 2};
      const auto result = gate::evaluate(input);
      expect(result.classification ==
                 gate::Classification::qualified_candidate,
             "material qualified advantage was not selected");
    }
    {
      auto input = passing_input();
      input.neutral.trained_minus_initialization.low = -0.001;
      const auto result = gate::evaluate(input);
      expect(result.classification ==
                 gate::Classification::qualified_candidate,
             "sole passing qualified candidate was not selected");
    }
    {
      auto input = passing_input();
      input.neutral.trained_minus_initialization.point = 0.001;
      input.qualified.trained_minus_initialization.point = 0.001;
      const auto result = gate::evaluate(input);
      expect(result.classification ==
                 gate::Classification::encoder_training_not_working,
             "failed learned-gain arms were not rejected");
    }
    {
      auto input = passing_input();
      input.global_mechanics = false;
      const auto result = gate::evaluate(input);
      expect(result.classification ==
                 gate::Classification::invalid_mechanics_or_numeric,
             "mechanics failure did not invalidate the gate");
    }
    {
      auto input = passing_input();
      input.neutral.geometry[1][2].top = 0.81;
      input.qualified.geometry[0][0].effective = 0.24;
      const auto result = gate::evaluate(input);
      expect(!result.neutral.geometry_pass && !result.qualified.geometry_pass,
             "geometry boundaries were not enforced");
      expect(result.classification ==
                 gate::Classification::encoder_training_not_working,
             "geometry failures did not reject both candidates");
    }
    {
      auto input = passing_input();
      input.neutral.order_shuffle_high = 0.61;
      const auto result = gate::evaluate(input);
      expect(!result.neutral.order_shuffle_pass,
             "order-shuffle ceiling was not enforced");
    }
    std::cout << "representation_module_certification_gate_tests=passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "representation_module_certification_gate_test_error="
              << error.what() << '\n';
    return 1;
  }
}
