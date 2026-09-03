#include "structured_readout_repair_gate.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace gate = cuwacunu::tests::structured_readout_repair_gate;
namespace psm = cuwacunu::tests::pooling_structure_mechanism_map_gate;

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

psm::ContinuousInput material_gain() {
  const double positive = above(0.0);
  return {.point = psm::kMaterialityThreshold,
          .low = positive,
          .high = 0.05,
          .seed_deltas = {positive, positive, 0.0},
          .family_deltas = {0.01, 0.01, 0.01, 0.01}};
}

psm::ContinuousInput noninferior() {
  const double seed = above(psm::kNoninferiorityMargin);
  const double family = above(psm::kFamilyNoninferiorityMargin);
  return {.point = 0.0,
          .low = seed,
          .high = 0.03,
          .seed_deltas = {seed, seed, psm::kNoninferiorityMargin},
          .family_deltas = {family, family, family, family}};
}

psm::ContinuousInput unresolved_continuous() {
  return {.point = 0.0,
          .low = below(psm::kNoninferiorityMargin),
          .high = 0.03,
          .seed_deltas = {psm::kNoninferiorityMargin,
                          psm::kNoninferiorityMargin,
                          psm::kNoninferiorityMargin},
          .family_deltas = {psm::kFamilyNoninferiorityMargin,
                            psm::kFamilyNoninferiorityMargin,
                            psm::kFamilyNoninferiorityMargin,
                            psm::kFamilyNoninferiorityMargin}};
}

psm::OrderInput order_decodable() {
  const double seed = above(psm::kOrderChance);
  return {.point = psm::kOrderDecodablePoint,
          .low = seed,
          .high = 0.75,
          .seed_points = {seed, seed, psm::kOrderChance}};
}

psm::OrderInput order_unresolved() {
  return {.point = 0.57,
          .low = 0.54,
          .high = 0.60,
          .seed_points = {0.56, 0.57, 0.58}};
}

psm::OrderInput chance_consistent() {
  return {.point = 0.50,
          .low = 0.45,
          .high = 0.55,
          .seed_points = {0.49, 0.50, 0.51}};
}

gate::GateInput passing() {
  gate::GateInput input{};
  input.offline_minus_channel = material_gain();
  input.offline_minus_encoder = noninferior();
  input.shadow_minus_channel = material_gain();
  input.shadow_minus_encoder = noninferior();
  input.channel_order = order_unresolved();
  input.offline_order = order_decodable();
  input.shadow_order = order_decodable();
  input.encoder_order = order_decodable();
  input.offline_equivalence_max_abs = 0.0;
  input.device_translation_max_abs = 0.0;
  input.validity = {
      .no_training_or_end_to_end = true,
      .local_contracts_exact = true,
      .parameters_and_rng_unchanged = true,
      .partition_and_projection_valid = true,
      .deterministic_tables_valid = true,
      .manifest_exact = true,
      .parent_artifacts_exact = true,
      .parent_classification_exact = true,
      .parent_attempt_count_exact = true,
      .parent_audit_pass = true,
      .parent_authorizations_false = true,
      .all_reference_keys_exact = true,
      .offline_feature_hashes_exact = true,
      .offline_bytes_exact = true,
      .device_contracts_exact = true,
      .continuous_shuffle_pass = {true, true, true, true},
      .order_shuffle_pass = {true, true, true, true}};
  return input;
}

void expect_gate(const gate::GateInput &input,
                 gate::TerminalClassification classification,
                 gate::FailureReason reason, const std::string &message) {
  const auto result = gate::evaluate(input);
  expect(result.classification == classification,
         message + ": wrong terminal classification");
  expect(result.failure_reason == reason, message + ": wrong failure reason");
}

void test_passing_gate() {
  const auto result = gate::evaluate(passing());
  expect(result.numeric_inputs_valid && result.mechanics_valid &&
             result.parent_evidence_valid && result.offline_reference_valid &&
             result.device_translation_valid && result.controls_valid,
         "passing input did not clear every validity stage");
  expect(result.channel_not_order_decodable &&
             result.encoder_order_decodable &&
             result.offline_material_gain_over_channel &&
             result.offline_noninferior_to_encoder &&
             result.offline_order_decodable,
         "passing input did not reproduce all C/D/E boundaries");
  expect(result.material_gain_over_channel && result.noninferior_to_encoder &&
             result.order_decodable && result.representation_gate_pass,
         "passing input did not clear every shadow-quality clause");
  expect_gate(passing(),
              gate::TerminalClassification::structured_readout_reproduced,
              gate::FailureReason::none, "passing repair");
}

void test_continuous_threshold_faces() {
  auto value = material_gain();
  value.point = below(psm::kMaterialityThreshold);
  expect(psm::evaluate_continuous(value).classification !=
             psm::ContinuousClassification::material_gain,
         "material point immediately below threshold passed");
  value.point = psm::kMaterialityThreshold;
  expect(psm::evaluate_continuous(value).classification ==
             psm::ContinuousClassification::material_gain,
         "material point exactly on inclusive threshold failed");
  value.point = above(psm::kMaterialityThreshold);
  expect(psm::evaluate_continuous(value).classification ==
             psm::ContinuousClassification::material_gain,
         "material point immediately above threshold failed");

  value = material_gain();
  value.low = below(0.0);
  expect(psm::evaluate_continuous(value).classification !=
             psm::ContinuousClassification::material_gain,
         "material lower bound below zero passed");
  value.low = 0.0;
  expect(psm::evaluate_continuous(value).classification !=
             psm::ContinuousClassification::material_gain,
         "strict material lower bound accepted zero");
  value.low = above(0.0);
  expect(psm::evaluate_continuous(value).classification ==
             psm::ContinuousClassification::material_gain,
         "material lower bound immediately above zero failed");

  value = material_gain();
  value.seed_deltas = {above(0.0), 0.0, below(0.0)};
  expect(psm::evaluate_continuous(value).classification !=
             psm::ContinuousClassification::material_gain,
         "material gate accepted only one strictly positive seed");
  value.seed_deltas = {above(0.0), above(0.0), 0.0};
  expect(psm::evaluate_continuous(value).classification ==
             psm::ContinuousClassification::material_gain,
         "material gate rejected exactly two strictly positive seeds");

  value = noninferior();
  value.low = below(psm::kNoninferiorityMargin);
  expect(psm::evaluate_continuous(value).classification ==
             psm::ContinuousClassification::unresolved,
         "noninferiority lower bound below margin passed");
  value.low = psm::kNoninferiorityMargin;
  expect(psm::evaluate_continuous(value).classification ==
             psm::ContinuousClassification::unresolved,
         "strict noninferiority lower bound accepted equality");
  value.low = above(psm::kNoninferiorityMargin);
  expect(psm::evaluate_continuous(value).classification ==
             psm::ContinuousClassification::noninferior,
         "noninferiority lower bound immediately above margin failed");

  value = noninferior();
  value.seed_deltas = {above(psm::kNoninferiorityMargin),
                       psm::kNoninferiorityMargin,
                       below(psm::kNoninferiorityMargin)};
  expect(psm::evaluate_continuous(value).classification ==
             psm::ContinuousClassification::unresolved,
         "noninferiority accepted only one seed strictly above margin");
  value.seed_deltas = {above(psm::kNoninferiorityMargin),
                       above(psm::kNoninferiorityMargin),
                       psm::kNoninferiorityMargin};
  expect(psm::evaluate_continuous(value).classification ==
             psm::ContinuousClassification::noninferior,
         "noninferiority rejected exactly two seeds above margin");

  value = noninferior();
  value.family_deltas[0] = below(psm::kFamilyNoninferiorityMargin);
  expect(psm::evaluate_continuous(value).classification ==
             psm::ContinuousClassification::unresolved,
         "family value below margin passed noninferiority");
  value.family_deltas[0] = psm::kFamilyNoninferiorityMargin;
  expect(psm::evaluate_continuous(value).classification ==
             psm::ContinuousClassification::unresolved,
         "strict family margin accepted equality");
  value.family_deltas[0] = above(psm::kFamilyNoninferiorityMargin);
  expect(psm::evaluate_continuous(value).classification ==
             psm::ContinuousClassification::noninferior,
         "family value immediately above margin failed noninferiority");

  value = material_gain();
  const auto overlapping = psm::evaluate_continuous(value);
  expect(overlapping.noninferior &&
             overlapping.classification ==
                 psm::ContinuousClassification::material_gain,
         "material gain did not take precedence over overlapping NI");

  value = material_gain();
  value.low = value.high = above(0.0);
  expect(psm::evaluate_continuous(value).numeric_valid,
         "equal interval endpoints were rejected");
  value.low = 0.1;
  value.high = 0.0;
  expect(!psm::evaluate_continuous(value).numeric_valid,
         "reversed continuous interval was accepted");
}

void test_order_threshold_faces() {
  auto value = order_decodable();
  value.point = below(psm::kOrderDecodablePoint);
  expect(psm::evaluate_order(value).classification !=
             psm::OrderClassification::order_decodable,
         "order point immediately below threshold passed");
  value.point = psm::kOrderDecodablePoint;
  expect(psm::evaluate_order(value).classification ==
             psm::OrderClassification::order_decodable,
         "order point exactly on inclusive threshold failed");
  value.point = above(psm::kOrderDecodablePoint);
  expect(psm::evaluate_order(value).classification ==
             psm::OrderClassification::order_decodable,
         "order point immediately above threshold failed");

  value = order_decodable();
  value.low = below(psm::kOrderChance);
  expect(psm::evaluate_order(value).classification !=
             psm::OrderClassification::order_decodable,
         "order lower bound below chance passed");
  value.low = psm::kOrderChance;
  expect(psm::evaluate_order(value).classification !=
             psm::OrderClassification::order_decodable,
         "strict order lower bound accepted chance equality");
  value.low = above(psm::kOrderChance);
  expect(psm::evaluate_order(value).classification ==
             psm::OrderClassification::order_decodable,
         "order lower bound immediately above chance failed");

  value = order_decodable();
  value.seed_points = {above(psm::kOrderChance), psm::kOrderChance,
                       below(psm::kOrderChance)};
  expect(psm::evaluate_order(value).classification !=
             psm::OrderClassification::order_decodable,
         "order gate accepted only one seed strictly above chance");
  value.seed_points = {above(psm::kOrderChance), above(psm::kOrderChance),
                       psm::kOrderChance};
  expect(psm::evaluate_order(value).classification ==
             psm::OrderClassification::order_decodable,
         "order gate rejected exactly two seeds above chance");

  value = chance_consistent();
  value.high = below(0.55);
  expect(psm::evaluate_order(value).classification ==
             psm::OrderClassification::chance_consistent,
         "order upper bound immediately below chance-control threshold failed");
  value.high = 0.55;
  expect(psm::evaluate_order(value).classification ==
             psm::OrderClassification::chance_consistent,
         "inclusive chance-control upper threshold failed");
  value.high = above(0.55);
  expect(psm::evaluate_order(value).classification ==
             psm::OrderClassification::order_unresolved,
         "order upper bound above chance-control threshold stayed consistent");

  value = order_decodable();
  value.low = value.high = above(psm::kOrderChance);
  expect(psm::evaluate_order(value).numeric_valid,
         "equal order interval endpoints were rejected");
  value.low = 0.8;
  value.high = 0.7;
  expect(!psm::evaluate_order(value).numeric_valid,
         "reversed order interval was accepted");
}

void test_translation_tolerance_faces() {
  auto input = passing();
  input.offline_equivalence_max_abs =
      below(gate::kOfflineEquivalenceTolerance);
  expect(gate::evaluate(input).offline_reference_valid,
         "offline error immediately below tolerance failed");
  input.offline_equivalence_max_abs = gate::kOfflineEquivalenceTolerance;
  expect(gate::evaluate(input).offline_reference_valid,
         "offline error exactly on inclusive tolerance failed");
  input.offline_equivalence_max_abs =
      above(gate::kOfflineEquivalenceTolerance);
  expect_gate(input, gate::TerminalClassification::offline_reference_failure,
              gate::FailureReason::offline_tolerance,
              "offline error above tolerance");

  input = passing();
  input.device_translation_max_abs =
      below(gate::kDeviceTranslationTolerance);
  expect(gate::evaluate(input).device_translation_valid,
         "device error immediately below tolerance failed");
  input.device_translation_max_abs = gate::kDeviceTranslationTolerance;
  expect(gate::evaluate(input).device_translation_valid,
         "device error exactly on inclusive tolerance failed");
  input.device_translation_max_abs =
      above(gate::kDeviceTranslationTolerance);
  expect_gate(input, gate::TerminalClassification::device_translation_failure,
              gate::FailureReason::device_tolerance,
              "device error above tolerance");

  input = passing();
  input.offline_equivalence_max_abs = below(0.0);
  expect_gate(input, gate::TerminalClassification::invalid_mechanics,
              gate::FailureReason::invalid_numeric,
              "negative offline error");
  input = passing();
  input.device_translation_max_abs = below(0.0);
  expect_gate(input, gate::TerminalClassification::invalid_mechanics,
              gate::FailureReason::invalid_numeric, "negative device error");
}

void test_stage_precedence_and_reasons() {
  using Terminal = gate::TerminalClassification;
  using Reason = gate::FailureReason;
  const std::array<bool gate::ValidityInput::*, 5> local_members{
      &gate::ValidityInput::no_training_or_end_to_end,
      &gate::ValidityInput::local_contracts_exact,
      &gate::ValidityInput::parameters_and_rng_unchanged,
      &gate::ValidityInput::partition_and_projection_valid,
      &gate::ValidityInput::deterministic_tables_valid};
  for (const auto member : local_members) {
    auto input = passing();
    input.validity.*member = false;
    input.validity.parent_artifacts_exact = false;
    expect_gate(input, Terminal::invalid_mechanics, Reason::local_contract,
                "local validity precedence");
  }

  auto input = passing();
  input.validity.manifest_exact = false;
  input.validity.parent_artifacts_exact = false;
  expect_gate(input, Terminal::invalid_mechanics, Reason::manifest,
              "manifest precedence");

  input = passing();
  input.validity.parent_artifacts_exact = false;
  input.validity.parent_classification_exact = false;
  expect_gate(input, Terminal::parent_evidence_failure,
              Reason::parent_artifact, "parent artifact precedence");
  input = passing();
  input.validity.parent_classification_exact = false;
  input.validity.parent_attempt_count_exact = false;
  expect_gate(input, Terminal::parent_evidence_failure,
              Reason::parent_classification,
              "parent classification precedence");
  input = passing();
  input.validity.parent_attempt_count_exact = false;
  input.validity.parent_audit_pass = false;
  expect_gate(input, Terminal::parent_evidence_failure,
              Reason::parent_attempt_count, "parent attempt precedence");
  input = passing();
  input.validity.parent_audit_pass = false;
  input.validity.parent_authorizations_false = false;
  expect_gate(input, Terminal::parent_evidence_failure, Reason::parent_audit,
              "parent audit precedence");
  input = passing();
  input.validity.parent_authorizations_false = false;
  input.validity.all_reference_keys_exact = false;
  expect_gate(input, Terminal::parent_evidence_failure,
              Reason::parent_authorization,
              "parent authorization precedence");

  input = passing();
  input.validity.all_reference_keys_exact = false;
  input.validity.offline_feature_hashes_exact = false;
  expect_gate(input, Terminal::offline_reference_failure,
              Reason::reference_keys, "reference-key precedence");
  input = passing();
  input.validity.offline_feature_hashes_exact = false;
  input.validity.offline_bytes_exact = false;
  expect_gate(input, Terminal::offline_reference_failure,
              Reason::reference_feature_hash,
              "reference-feature precedence");
  input = passing();
  input.validity.offline_bytes_exact = false;
  input.offline_equivalence_max_abs =
      above(gate::kOfflineEquivalenceTolerance);
  expect_gate(input, Terminal::offline_reference_failure,
              Reason::offline_byte_identity, "offline-byte precedence");
  input = passing();
  input.offline_equivalence_max_abs =
      above(gate::kOfflineEquivalenceTolerance);
  input.validity.device_contracts_exact = false;
  expect_gate(input, Terminal::offline_reference_failure,
              Reason::offline_tolerance, "offline-tolerance precedence");

  input = passing();
  input.channel_order = order_decodable();
  input.encoder_order = chance_consistent();
  expect_gate(input, Terminal::offline_reference_failure,
              Reason::channel_order_boundary, "channel-boundary precedence");
  input = passing();
  input.encoder_order = chance_consistent();
  input.offline_minus_channel = unresolved_continuous();
  expect_gate(input, Terminal::offline_reference_failure,
              Reason::encoder_order_boundary, "encoder-boundary precedence");
  input = passing();
  input.offline_minus_channel = unresolved_continuous();
  input.offline_minus_encoder = unresolved_continuous();
  expect_gate(input, Terminal::offline_reference_failure,
              Reason::offline_material_gain,
              "offline-material precedence");
  input = passing();
  input.offline_minus_encoder = unresolved_continuous();
  input.offline_order = order_unresolved();
  expect_gate(input, Terminal::offline_reference_failure,
              Reason::offline_noninferiority,
              "offline-noninferiority precedence");
  input = passing();
  input.offline_order = order_unresolved();
  input.validity.device_contracts_exact = false;
  expect_gate(input, Terminal::offline_reference_failure,
              Reason::offline_order_boundary,
              "offline-order precedence");

  input = passing();
  input.validity.device_contracts_exact = false;
  input.device_translation_max_abs =
      above(gate::kDeviceTranslationTolerance);
  expect_gate(input, Terminal::device_translation_failure,
              Reason::device_contract, "device-contract precedence");
  input = passing();
  input.device_translation_max_abs =
      above(gate::kDeviceTranslationTolerance);
  input.validity.continuous_shuffle_pass[0] = false;
  expect_gate(input, Terminal::device_translation_failure,
              Reason::device_tolerance, "device-tolerance precedence");

  constexpr std::array<Reason, gate::kArmCount> continuous_reasons{
      Reason::channel_continuous_shuffle,
      Reason::offline_cdsb_continuous_shuffle,
      Reason::shadow_continuous_shuffle,
      Reason::encoder_continuous_shuffle};
  constexpr std::array<Reason, gate::kArmCount> order_reasons{
      Reason::channel_order_shuffle, Reason::offline_cdsb_order_shuffle,
      Reason::shadow_order_shuffle, Reason::encoder_order_shuffle};
  for (std::size_t arm = 0; arm < gate::kArmCount; ++arm) {
    input = passing();
    input.validity.continuous_shuffle_pass[arm] = false;
    input.validity.order_shuffle_pass[arm] = false;
    expect_gate(input, Terminal::invalid_mechanics, continuous_reasons[arm],
                "continuous control precedence");
    input = passing();
    input.validity.order_shuffle_pass[arm] = false;
    expect_gate(input, Terminal::invalid_mechanics, order_reasons[arm],
                "order control reason");
  }
}

void test_reference_and_shadow_quality_faces() {
  auto input = passing();
  input.offline_minus_channel.point = below(psm::kMaterialityThreshold);
  expect_gate(input, gate::TerminalClassification::offline_reference_failure,
              gate::FailureReason::offline_material_gain,
              "offline material point below threshold");
  input = passing();
  input.offline_minus_encoder.low = psm::kNoninferiorityMargin;
  expect_gate(input, gate::TerminalClassification::offline_reference_failure,
              gate::FailureReason::offline_noninferiority,
              "offline NI lower bound on strict margin");
  input = passing();
  input.offline_order.low = psm::kOrderChance;
  expect_gate(input, gate::TerminalClassification::offline_reference_failure,
              gate::FailureReason::offline_order_boundary,
              "offline order lower bound on strict chance");

  input = passing();
  input.shadow_minus_channel.point = below(psm::kMaterialityThreshold);
  expect_gate(input, gate::TerminalClassification::readout_gate_failure,
              gate::FailureReason::shadow_quality,
              "shadow material point below threshold");
  input = passing();
  input.shadow_minus_encoder.low = psm::kNoninferiorityMargin;
  expect_gate(input, gate::TerminalClassification::readout_gate_failure,
              gate::FailureReason::shadow_quality,
              "shadow NI lower bound on strict margin");
  input = passing();
  input.shadow_order.low = psm::kOrderChance;
  expect_gate(input, gate::TerminalClassification::readout_gate_failure,
              gate::FailureReason::shadow_quality,
              "shadow order lower bound on strict chance");
}

void test_every_numeric_input_is_gated() {
  using ContinuousMember = psm::ContinuousInput gate::GateInput::*;
  constexpr std::array<ContinuousMember, 4> continuous_members{
      &gate::GateInput::offline_minus_channel,
      &gate::GateInput::offline_minus_encoder,
      &gate::GateInput::shadow_minus_channel,
      &gate::GateInput::shadow_minus_encoder};
  for (const auto member : continuous_members) {
    auto input = passing();
    (input.*member).low = 1.0;
    (input.*member).high = 0.0;
    expect_gate(input, gate::TerminalClassification::invalid_mechanics,
                gate::FailureReason::invalid_numeric,
                "invalid continuous interval");
  }

  using OrderMember = psm::OrderInput gate::GateInput::*;
  constexpr std::array<OrderMember, 4> order_members{
      &gate::GateInput::channel_order, &gate::GateInput::offline_order,
      &gate::GateInput::shadow_order, &gate::GateInput::encoder_order};
  for (const auto member : order_members) {
    auto input = passing();
    (input.*member).low = 1.0;
    (input.*member).high = 0.0;
    expect_gate(input, gate::TerminalClassification::invalid_mechanics,
                gate::FailureReason::invalid_numeric,
                "invalid order interval");
  }

  auto input = passing();
  input.offline_minus_channel.seed_deltas[0] =
      std::numeric_limits<double>::quiet_NaN();
  expect_gate(input, gate::TerminalClassification::invalid_mechanics,
              gate::FailureReason::invalid_numeric,
              "non-finite continuous seed");
  input = passing();
  input.shadow_minus_encoder.family_deltas[0] =
      std::numeric_limits<double>::infinity();
  expect_gate(input, gate::TerminalClassification::invalid_mechanics,
              gate::FailureReason::invalid_numeric,
              "non-finite continuous family");
  input = passing();
  input.encoder_order.seed_points[0] =
      std::numeric_limits<double>::quiet_NaN();
  expect_gate(input, gate::TerminalClassification::invalid_mechanics,
              gate::FailureReason::invalid_numeric, "non-finite order seed");
  input = passing();
  input.device_translation_max_abs =
      std::numeric_limits<double>::quiet_NaN();
  expect_gate(input, gate::TerminalClassification::invalid_mechanics,
              gate::FailureReason::invalid_numeric,
              "non-finite device tolerance input");
}

void test_stable_names() {
  using Terminal = gate::TerminalClassification;
  const std::array<std::pair<Terminal, const char *>, 6> terminal_names{{
      {Terminal::invalid_mechanics, "invalid_mechanics"},
      {Terminal::parent_evidence_failure, "parent_evidence_failure"},
      {Terminal::offline_reference_failure, "offline_reference_failure"},
      {Terminal::device_translation_failure, "device_translation_failure"},
      {Terminal::readout_gate_failure, "readout_gate_failure"},
      {Terminal::structured_readout_reproduced,
       "structured_readout_reproduced"}}};
  for (const auto &[classification, name] : terminal_names) {
    expect(std::string(gate::terminal_classification_name(classification)) ==
               name,
           "terminal classification name changed");
  }

  using Reason = gate::FailureReason;
  const std::array<std::pair<Reason, const char *>, 30> reason_names{{
      {Reason::none, "none"},
      {Reason::invalid_numeric, "invalid_numeric"},
      {Reason::local_contract, "local_contract"},
      {Reason::manifest, "manifest"},
      {Reason::parent_artifact, "parent_artifact"},
      {Reason::parent_classification, "parent_classification"},
      {Reason::parent_attempt_count, "parent_attempt_count"},
      {Reason::parent_audit, "parent_audit"},
      {Reason::parent_authorization, "parent_authorization"},
      {Reason::reference_keys, "reference_keys"},
      {Reason::reference_feature_hash, "reference_feature_hash"},
      {Reason::offline_byte_identity, "offline_byte_identity"},
      {Reason::offline_tolerance, "offline_tolerance"},
      {Reason::channel_order_boundary, "channel_order_boundary"},
      {Reason::encoder_order_boundary, "encoder_order_boundary"},
      {Reason::offline_material_gain, "offline_material_gain"},
      {Reason::offline_noninferiority, "offline_noninferiority"},
      {Reason::offline_order_boundary, "offline_order_boundary"},
      {Reason::device_contract, "device_contract"},
      {Reason::device_tolerance, "device_tolerance"},
      {Reason::channel_continuous_shuffle,
       "channel_continuous_shuffle"},
      {Reason::offline_cdsb_continuous_shuffle,
       "offline_cdsb_continuous_shuffle"},
      {Reason::shadow_continuous_shuffle, "shadow_continuous_shuffle"},
      {Reason::encoder_continuous_shuffle, "encoder_continuous_shuffle"},
      {Reason::channel_order_shuffle, "channel_order_shuffle"},
      {Reason::offline_cdsb_order_shuffle,
       "offline_cdsb_order_shuffle"},
      {Reason::shadow_order_shuffle, "shadow_order_shuffle"},
      {Reason::encoder_order_shuffle, "encoder_order_shuffle"},
      {Reason::shadow_quality, "shadow_quality"},
      {static_cast<Reason>(999), "invalid_numeric"}}};
  for (const auto &[reason, name] : reason_names) {
    expect(std::string(gate::failure_reason_name(reason)) == name,
           "failure-reason name changed");
  }
}

} // namespace

int main() {
  try {
    test_passing_gate();
    test_continuous_threshold_faces();
    test_order_threshold_faces();
    test_translation_tolerance_faces();
    test_stage_precedence_and_reasons();
    test_reference_and_shadow_quality_faces();
    test_every_numeric_input_is_gated();
    test_stable_names();
    std::cout << "structured_readout_repair_gate=PASS\n";
    std::cout << "threshold_faces=material,noninferiority,order,tolerances\n";
    std::cout << "precedence_stages=local,manifest,parent,offline,device,controls,quality\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "structured_readout_repair_gate=FAIL: " << error.what()
              << '\n';
    return 1;
  }
}
