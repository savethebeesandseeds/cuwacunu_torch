#include "pooling_structure_mechanism_map_gate.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace gate =
    cuwacunu::tests::pooling_structure_mechanism_map_gate;

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

gate::ContinuousInput material_gain() {
  const double positive = std::numeric_limits<double>::denorm_min();
  return {.point = gate::kMaterialityThreshold,
          .low = positive,
          .high = 0.04,
          .seed_deltas = {positive, positive, 0.0},
          .family_deltas = {0.01, 0.01, 0.01, 0.01}};
}

gate::ContinuousInput noninferior() {
  const double value = above(gate::kNoninferiorityMargin);
  const double family = above(gate::kFamilyNoninferiorityMargin);
  return {.point = 0.0,
          .low = value,
          .high = 0.02,
          .seed_deltas = {value, value, gate::kNoninferiorityMargin},
          .family_deltas = {family, family, family, family}};
}

gate::ContinuousInput unresolved() {
  return {.point = 0.0,
          .low = -0.03,
          .high = 0.03,
          .seed_deltas = {-0.03, 0.0, 0.0},
          .family_deltas = {-0.06, 0.0, 0.0, 0.0}};
}

gate::OrderInput order_decodable() {
  const double positive = above(gate::kOrderChance);
  return {.point = gate::kOrderDecodablePoint,
          .low = positive,
          .high = 0.70,
          .seed_points = {positive, positive, gate::kOrderChance}};
}

gate::OrderInput order_unresolved() {
  return {.point = 0.57,
          .low = 0.55,
          .high = 0.59,
          .seed_points = {0.57, 0.57, 0.57}};
}

gate::CandidateInput restored_candidate() {
  return {.minus_encoder = noninferior(),
          .minus_channel = material_gain(),
          .order = order_decodable()};
}

gate::GateInput baseline() {
  gate::GateInput input{};
  input.encoder_minus_channel = material_gain();
  input.encoder_order = order_decodable();
  input.channel_order = order_unresolved();
  for (auto &candidate : input.candidates) {
    candidate = {.minus_encoder = unresolved(),
                 .minus_channel = unresolved(),
                 .order = order_unresolved()};
  }
  input.validity = {.no_training_or_end_to_end = true,
                    .capture_and_identity_exact = true,
                    .parameters_and_rng_unchanged = true,
                    .partitions_valid = true,
                    .projection_valid = true,
                    .deterministic_tables_valid = true,
                    .references_reproduced = true,
                    .shuffled_controls_pass = true};
  return input;
}

void test_continuous_boundaries() {
  auto input = material_gain();
  auto result = gate::evaluate_continuous(input);
  expect(result.classification ==
             gate::ContinuousClassification::material_gain,
         "inclusive material-gain point and strict supporting clauses pass");
  expect(result.noninferior, "material gain may also satisfy noninferiority");

  input.low = 0.0;
  expect(gate::evaluate_continuous(input).classification !=
             gate::ContinuousClassification::material_gain,
         "material-gain interval is strict");
  input = material_gain();
  input.point = below(gate::kMaterialityThreshold);
  expect(gate::evaluate_continuous(input).classification !=
             gate::ContinuousClassification::material_gain,
         "material-gain point threshold is inclusive only at threshold");
  input = material_gain();
  input.seed_deltas = {std::numeric_limits<double>::denorm_min(), 0.0, 0.0};
  expect(gate::evaluate_continuous(input).classification !=
             gate::ContinuousClassification::material_gain,
         "material gain requires at least two positive seeds");

  input = noninferior();
  result = gate::evaluate_continuous(input);
  expect(result.classification == gate::ContinuousClassification::noninferior,
         "strict noninferiority boundaries pass just above threshold");
  input.low = gate::kNoninferiorityMargin;
  expect(gate::evaluate_continuous(input).classification ==
             gate::ContinuousClassification::unresolved,
         "noninferiority interval boundary is strict");
  input = noninferior();
  input.family_deltas[0] = gate::kFamilyNoninferiorityMargin;
  expect(gate::evaluate_continuous(input).classification ==
             gate::ContinuousClassification::unresolved,
         "family noninferiority boundary is strict");
  input = noninferior();
  input.seed_deltas = {above(gate::kNoninferiorityMargin),
                       gate::kNoninferiorityMargin,
                       gate::kNoninferiorityMargin};
  expect(gate::evaluate_continuous(input).classification ==
             gate::ContinuousClassification::unresolved,
         "noninferiority requires at least two qualifying seeds");
}

void test_order_boundaries() {
  auto input = order_decodable();
  expect(gate::evaluate_order(input).classification ==
             gate::OrderClassification::order_decodable,
         "order point is inclusive and lower/seed boundaries are strict");
  input.low = gate::kOrderChance;
  expect(gate::evaluate_order(input).classification !=
             gate::OrderClassification::order_decodable,
         "order interval lower boundary is strict");
  input = order_decodable();
  input.point = below(gate::kOrderDecodablePoint);
  expect(gate::evaluate_order(input).classification !=
             gate::OrderClassification::order_decodable,
         "order point below threshold is not decodable");
  input = order_decodable();
  input.seed_points = {above(gate::kOrderChance), gate::kOrderChance,
                       gate::kOrderChance};
  expect(gate::evaluate_order(input).classification !=
             gate::OrderClassification::order_decodable,
         "order decodability requires at least two above-chance seeds");
  input = {.point = 0.50,
           .low = 0.45,
           .high = 0.55,
           .seed_points = {0.50, 0.50, 0.50}};
  expect(gate::evaluate_order(input).classification ==
             gate::OrderClassification::chance_consistent,
         "chance upper boundary is inclusive");
}

void test_terminal_tree() {
  auto input = baseline();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::fixed_summaries_not_sufficient,
         "valid boundary with no restored candidate stops at none sufficient");

  input.candidates[gate::candidate_index(gate::Candidate::channel_domain)] =
      restored_candidate();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::domain_separation_sufficient,
         "domain is the earliest restored arm");

  input = baseline();
  input.candidates[gate::candidate_index(
      gate::Candidate::channel_domain_scale)] = restored_candidate();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::domain_scale_separation_sufficient,
         "domain-scale is selected when domain does not restore");

  input = baseline();
  input.candidates[gate::candidate_index(
      gate::Candidate::channel_domain_scale_bin)] = restored_candidate();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::
                 coarse_position_separation_sufficient,
         "coarse position is selected when coarser arms do not restore");

  input = baseline();
  input.candidates[0].minus_encoder = noninferior();
  input.candidates[0].minus_channel = material_gain();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::
                 factors_restored_order_not_restored,
         "continuous-only recovery is reported explicitly");

  input = baseline();
  input.candidates[0].order = order_decodable();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::
                 order_restored_factors_not_restored,
         "order-only recovery is reported explicitly");
}

void test_validity_and_boundary_precedence() {
  auto input = baseline();
  input.validity.partitions_valid = false;
  input.validity.references_reproduced = false;
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::invalid_mechanics,
         "mechanics failure precedes reference failure");

  input = baseline();
  input.validity.references_reproduced = false;
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::reference_reproduction_failure,
         "reference failure precedes scientific tree");

  input = baseline();
  input.validity.shuffled_controls_pass = false;
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::invalid_mechanics,
         "shuffle failure invalidates the map");

  input = baseline();
  input.validity.references_reproduced = false;
  input.validity.shuffled_controls_pass = false;
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::reference_reproduction_failure,
         "reference failure precedes a simultaneous shuffle failure");

  input = baseline();
  input.encoder_minus_channel = noninferior();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::encoder_boundary_not_reproduced,
         "continuous boundary must be a material gain");

  input = baseline();
  input.encoder_order = order_unresolved();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::encoder_boundary_not_reproduced,
         "encoder order must be decodable");

  input = baseline();
  input.channel_order = order_decodable();
  expect(gate::evaluate(input).classification ==
             gate::TerminalClassification::encoder_boundary_not_reproduced,
         "already-decodable channel baseline does not reproduce boundary");
}

void test_invalid_numeric_and_names() {
  auto input = baseline();
  input.candidates[1].minus_encoder.point =
      std::numeric_limits<double>::quiet_NaN();
  const auto result = gate::evaluate(input);
  expect(!result.numeric_inputs_valid, "NaN is reported as invalid input");
  expect(result.classification ==
             gate::TerminalClassification::invalid_mechanics,
         "numeric failure precedes terminal interpretation");
  expect(std::string(gate::terminal_classification_name(
             gate::TerminalClassification::domain_scale_separation_sufficient)) ==
             "domain_scale_separation_sufficient",
         "terminal classification name is stable");
  expect(std::string(gate::continuous_classification_name(
             gate::ContinuousClassification::noninferior)) == "noninferior",
         "continuous classification name is stable");
  expect(std::string(gate::order_classification_name(
             gate::OrderClassification::order_decodable)) ==
             "order_decodable",
         "order classification name is stable");
}

} // namespace

int main() {
  try {
    test_continuous_boundaries();
    test_order_boundaries();
    test_terminal_tree();
    test_validity_and_boundary_precedence();
    test_invalid_numeric_and_names();
    std::cout << "pooling_structure_mechanism_map_gate=PASS\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "pooling_structure_mechanism_map_gate=FAIL: " << error.what()
              << '\n';
    return 1;
  }
}
