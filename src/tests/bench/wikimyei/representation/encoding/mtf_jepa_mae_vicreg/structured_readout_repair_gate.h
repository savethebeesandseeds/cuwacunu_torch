#pragma once

#include "pooling_structure_mechanism_map_gate.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace cuwacunu::tests::structured_readout_repair_gate {

namespace psm = pooling_structure_mechanism_map_gate;

constexpr double kOfflineEquivalenceTolerance = 1.0e-12;
constexpr double kDeviceTranslationTolerance = 2.0e-5;
constexpr std::size_t kArmCount = 4;

enum class Arm : std::size_t {
  channel = 0,
  offline_cdsb = 1,
  shadow = 2,
  encoder = 3,
};

[[nodiscard]] constexpr std::size_t arm_index(Arm arm) {
  return static_cast<std::size_t>(arm);
}

struct ValidityInput {
  // Stage 1. The manifest is an external auditor input: the measurement
  // executable must not certify its own files.
  bool no_training_or_end_to_end{false};
  bool local_contracts_exact{false};
  bool parameters_and_rng_unchanged{false};
  bool partition_and_projection_valid{false};
  bool deterministic_tables_valid{false};
  bool manifest_exact{false};

  // Stage 2: sealed parent evidence.
  bool parent_artifacts_exact{false};
  bool parent_classification_exact{false};
  bool parent_attempt_count_exact{false};
  bool parent_audit_pass{false};
  bool parent_authorizations_false{false};

  // Stage 3: complete C/D/E reproduction.
  bool all_reference_keys_exact{false};
  bool offline_feature_hashes_exact{false};
  bool offline_bytes_exact{false};

  // Stages 4 and 5.
  bool device_contracts_exact{false};
  std::array<bool, kArmCount> continuous_shuffle_pass{};
  std::array<bool, kArmCount> order_shuffle_pass{};
};

struct GateInput {
  psm::ContinuousInput offline_minus_channel{};
  psm::ContinuousInput offline_minus_encoder{};
  psm::ContinuousInput shadow_minus_channel{};
  psm::ContinuousInput shadow_minus_encoder{};
  psm::OrderInput channel_order{};
  psm::OrderInput offline_order{};
  psm::OrderInput shadow_order{};
  psm::OrderInput encoder_order{};
  double offline_equivalence_max_abs{0.0};
  double device_translation_max_abs{0.0};
  ValidityInput validity{};
};

enum class TerminalClassification {
  invalid_mechanics,
  parent_evidence_failure,
  offline_reference_failure,
  device_translation_failure,
  readout_gate_failure,
  structured_readout_reproduced,
};

[[nodiscard]] inline const char *
terminal_classification_name(TerminalClassification classification) {
  switch (classification) {
  case TerminalClassification::invalid_mechanics:
    return "invalid_mechanics";
  case TerminalClassification::parent_evidence_failure:
    return "parent_evidence_failure";
  case TerminalClassification::offline_reference_failure:
    return "offline_reference_failure";
  case TerminalClassification::device_translation_failure:
    return "device_translation_failure";
  case TerminalClassification::readout_gate_failure:
    return "readout_gate_failure";
  case TerminalClassification::structured_readout_reproduced:
    return "structured_readout_reproduced";
  }
  return "invalid_mechanics";
}

enum class FailureReason {
  none,
  invalid_numeric,
  local_contract,
  manifest,
  parent_artifact,
  parent_classification,
  parent_attempt_count,
  parent_audit,
  parent_authorization,
  reference_keys,
  reference_feature_hash,
  offline_byte_identity,
  offline_tolerance,
  channel_order_boundary,
  encoder_order_boundary,
  offline_material_gain,
  offline_noninferiority,
  offline_order_boundary,
  device_contract,
  device_tolerance,
  channel_continuous_shuffle,
  offline_cdsb_continuous_shuffle,
  shadow_continuous_shuffle,
  encoder_continuous_shuffle,
  channel_order_shuffle,
  offline_cdsb_order_shuffle,
  shadow_order_shuffle,
  encoder_order_shuffle,
  shadow_quality,
};

[[nodiscard]] inline const char *failure_reason_name(FailureReason reason) {
  switch (reason) {
  case FailureReason::none:
    return "none";
  case FailureReason::invalid_numeric:
    return "invalid_numeric";
  case FailureReason::local_contract:
    return "local_contract";
  case FailureReason::manifest:
    return "manifest";
  case FailureReason::parent_artifact:
    return "parent_artifact";
  case FailureReason::parent_classification:
    return "parent_classification";
  case FailureReason::parent_attempt_count:
    return "parent_attempt_count";
  case FailureReason::parent_audit:
    return "parent_audit";
  case FailureReason::parent_authorization:
    return "parent_authorization";
  case FailureReason::reference_keys:
    return "reference_keys";
  case FailureReason::reference_feature_hash:
    return "reference_feature_hash";
  case FailureReason::offline_byte_identity:
    return "offline_byte_identity";
  case FailureReason::offline_tolerance:
    return "offline_tolerance";
  case FailureReason::channel_order_boundary:
    return "channel_order_boundary";
  case FailureReason::encoder_order_boundary:
    return "encoder_order_boundary";
  case FailureReason::offline_material_gain:
    return "offline_material_gain";
  case FailureReason::offline_noninferiority:
    return "offline_noninferiority";
  case FailureReason::offline_order_boundary:
    return "offline_order_boundary";
  case FailureReason::device_contract:
    return "device_contract";
  case FailureReason::device_tolerance:
    return "device_tolerance";
  case FailureReason::channel_continuous_shuffle:
    return "channel_continuous_shuffle";
  case FailureReason::offline_cdsb_continuous_shuffle:
    return "offline_cdsb_continuous_shuffle";
  case FailureReason::shadow_continuous_shuffle:
    return "shadow_continuous_shuffle";
  case FailureReason::encoder_continuous_shuffle:
    return "encoder_continuous_shuffle";
  case FailureReason::channel_order_shuffle:
    return "channel_order_shuffle";
  case FailureReason::offline_cdsb_order_shuffle:
    return "offline_cdsb_order_shuffle";
  case FailureReason::shadow_order_shuffle:
    return "shadow_order_shuffle";
  case FailureReason::encoder_order_shuffle:
    return "encoder_order_shuffle";
  case FailureReason::shadow_quality:
    return "shadow_quality";
  }
  return "invalid_numeric";
}

struct GateResult {
  psm::ContinuousResult offline_minus_channel{};
  psm::ContinuousResult offline_minus_encoder{};
  psm::ContinuousResult shadow_minus_channel{};
  psm::ContinuousResult shadow_minus_encoder{};
  psm::OrderResult channel_order{};
  psm::OrderResult offline_order{};
  psm::OrderResult shadow_order{};
  psm::OrderResult encoder_order{};
  bool numeric_inputs_valid{false};
  bool mechanics_valid{false};
  bool parent_evidence_valid{false};
  bool channel_not_order_decodable{false};
  bool encoder_order_decodable{false};
  bool offline_material_gain_over_channel{false};
  bool offline_noninferior_to_encoder{false};
  bool offline_order_decodable{false};
  bool offline_reference_valid{false};
  bool device_translation_valid{false};
  bool controls_valid{false};
  bool material_gain_over_channel{false};
  bool noninferior_to_encoder{false};
  bool order_decodable{false};
  bool representation_gate_pass{false};
  FailureReason failure_reason{FailureReason::invalid_numeric};
  TerminalClassification classification{
      TerminalClassification::invalid_mechanics};
};

[[nodiscard]] inline GateResult evaluate(const GateInput &input) {
  GateResult result{};
  result.offline_minus_channel =
      psm::evaluate_continuous(input.offline_minus_channel);
  result.offline_minus_encoder =
      psm::evaluate_continuous(input.offline_minus_encoder);
  result.shadow_minus_channel =
      psm::evaluate_continuous(input.shadow_minus_channel);
  result.shadow_minus_encoder =
      psm::evaluate_continuous(input.shadow_minus_encoder);
  result.channel_order = psm::evaluate_order(input.channel_order);
  result.offline_order = psm::evaluate_order(input.offline_order);
  result.shadow_order = psm::evaluate_order(input.shadow_order);
  result.encoder_order = psm::evaluate_order(input.encoder_order);
  result.numeric_inputs_valid =
      result.offline_minus_channel.numeric_valid &&
      result.offline_minus_encoder.numeric_valid &&
      result.shadow_minus_channel.numeric_valid &&
      result.shadow_minus_encoder.numeric_valid &&
      result.channel_order.numeric_valid && result.offline_order.numeric_valid &&
      result.shadow_order.numeric_valid && result.encoder_order.numeric_valid &&
      std::isfinite(input.offline_equivalence_max_abs) &&
      input.offline_equivalence_max_abs >= 0.0 &&
      std::isfinite(input.device_translation_max_abs) &&
      input.device_translation_max_abs >= 0.0;
  if (!result.numeric_inputs_valid) {
    result.failure_reason = FailureReason::invalid_numeric;
    return result;
  }

  const bool local_contract =
      input.validity.no_training_or_end_to_end &&
      input.validity.local_contracts_exact &&
      input.validity.parameters_and_rng_unchanged &&
      input.validity.partition_and_projection_valid &&
      input.validity.deterministic_tables_valid;
  if (!local_contract) {
    result.failure_reason = FailureReason::local_contract;
    return result;
  }
  if (!input.validity.manifest_exact) {
    result.failure_reason = FailureReason::manifest;
    return result;
  }
  result.mechanics_valid = true;

  if (!input.validity.parent_artifacts_exact) {
    result.failure_reason = FailureReason::parent_artifact;
    result.classification = TerminalClassification::parent_evidence_failure;
    return result;
  }
  if (!input.validity.parent_classification_exact) {
    result.failure_reason = FailureReason::parent_classification;
    result.classification = TerminalClassification::parent_evidence_failure;
    return result;
  }
  if (!input.validity.parent_attempt_count_exact) {
    result.failure_reason = FailureReason::parent_attempt_count;
    result.classification = TerminalClassification::parent_evidence_failure;
    return result;
  }
  if (!input.validity.parent_audit_pass) {
    result.failure_reason = FailureReason::parent_audit;
    result.classification = TerminalClassification::parent_evidence_failure;
    return result;
  }
  if (!input.validity.parent_authorizations_false) {
    result.failure_reason = FailureReason::parent_authorization;
    result.classification = TerminalClassification::parent_evidence_failure;
    return result;
  }
  result.parent_evidence_valid = true;

  result.channel_not_order_decodable =
      result.channel_order.classification !=
      psm::OrderClassification::order_decodable;
  result.encoder_order_decodable =
      result.encoder_order.classification ==
      psm::OrderClassification::order_decodable;
  result.offline_material_gain_over_channel =
      result.offline_minus_channel.classification ==
      psm::ContinuousClassification::material_gain;
  result.offline_noninferior_to_encoder =
      result.offline_minus_encoder.classification ==
          psm::ContinuousClassification::noninferior ||
      result.offline_minus_encoder.classification ==
          psm::ContinuousClassification::material_gain;
  result.offline_order_decodable =
      result.offline_order.classification ==
      psm::OrderClassification::order_decodable;
  if (!input.validity.all_reference_keys_exact) {
    result.failure_reason = FailureReason::reference_keys;
    result.classification = TerminalClassification::offline_reference_failure;
    return result;
  }
  if (!input.validity.offline_feature_hashes_exact) {
    result.failure_reason = FailureReason::reference_feature_hash;
    result.classification = TerminalClassification::offline_reference_failure;
    return result;
  }
  if (!input.validity.offline_bytes_exact) {
    result.failure_reason = FailureReason::offline_byte_identity;
    result.classification = TerminalClassification::offline_reference_failure;
    return result;
  }
  if (input.offline_equivalence_max_abs > kOfflineEquivalenceTolerance) {
    result.failure_reason = FailureReason::offline_tolerance;
    result.classification = TerminalClassification::offline_reference_failure;
    return result;
  }
  if (!result.channel_not_order_decodable) {
    result.failure_reason = FailureReason::channel_order_boundary;
    result.classification = TerminalClassification::offline_reference_failure;
    return result;
  }
  if (!result.encoder_order_decodable) {
    result.failure_reason = FailureReason::encoder_order_boundary;
    result.classification = TerminalClassification::offline_reference_failure;
    return result;
  }
  if (!result.offline_material_gain_over_channel) {
    result.failure_reason = FailureReason::offline_material_gain;
    result.classification = TerminalClassification::offline_reference_failure;
    return result;
  }
  if (!result.offline_noninferior_to_encoder) {
    result.failure_reason = FailureReason::offline_noninferiority;
    result.classification = TerminalClassification::offline_reference_failure;
    return result;
  }
  if (!result.offline_order_decodable) {
    result.failure_reason = FailureReason::offline_order_boundary;
    result.classification = TerminalClassification::offline_reference_failure;
    return result;
  }
  result.offline_reference_valid = true;

  if (!input.validity.device_contracts_exact) {
    result.failure_reason = FailureReason::device_contract;
    result.classification = TerminalClassification::device_translation_failure;
    return result;
  }
  if (input.device_translation_max_abs > kDeviceTranslationTolerance) {
    result.failure_reason = FailureReason::device_tolerance;
    result.classification = TerminalClassification::device_translation_failure;
    return result;
  }
  result.device_translation_valid = true;

  constexpr std::array<FailureReason, kArmCount> continuous_reasons{
      FailureReason::channel_continuous_shuffle,
      FailureReason::offline_cdsb_continuous_shuffle,
      FailureReason::shadow_continuous_shuffle,
      FailureReason::encoder_continuous_shuffle};
  constexpr std::array<FailureReason, kArmCount> order_reasons{
      FailureReason::channel_order_shuffle,
      FailureReason::offline_cdsb_order_shuffle,
      FailureReason::shadow_order_shuffle,
      FailureReason::encoder_order_shuffle};
  for (std::size_t index = 0; index < kArmCount; ++index) {
    if (!input.validity.continuous_shuffle_pass[index]) {
      result.failure_reason = continuous_reasons[index];
      return result;
    }
    if (!input.validity.order_shuffle_pass[index]) {
      result.failure_reason = order_reasons[index];
      return result;
    }
  }
  result.controls_valid = true;

  result.material_gain_over_channel =
      result.shadow_minus_channel.classification ==
      psm::ContinuousClassification::material_gain;
  result.noninferior_to_encoder =
      result.shadow_minus_encoder.classification ==
          psm::ContinuousClassification::noninferior ||
      result.shadow_minus_encoder.classification ==
          psm::ContinuousClassification::material_gain;
  result.order_decodable =
      result.shadow_order.classification ==
      psm::OrderClassification::order_decodable;
  result.representation_gate_pass = result.material_gain_over_channel &&
                                    result.noninferior_to_encoder &&
                                    result.order_decodable;
  if (!result.representation_gate_pass) {
    result.failure_reason = FailureReason::shadow_quality;
    result.classification = TerminalClassification::readout_gate_failure;
    return result;
  }
  result.failure_reason = FailureReason::none;
  result.classification =
      TerminalClassification::structured_readout_reproduced;
  return result;
}

} // namespace cuwacunu::tests::structured_readout_repair_gate
