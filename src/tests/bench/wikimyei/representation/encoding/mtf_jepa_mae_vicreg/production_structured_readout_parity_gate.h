#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace cuwacunu::tests::production_structured_readout_parity_gate {

constexpr double kDeviceTranslationTolerance = 2.0e-5;
constexpr std::uint64_t kExpectedSeedCount = 3;
constexpr std::uint64_t kExpectedDatasetCount = 6;
constexpr std::uint64_t kExpectedRetainedCaptureCount = 18;
constexpr std::uint64_t kExpectedRepeatCaptureCount = 18;
constexpr std::uint64_t kExpectedRetainedRowCount = 3840;
constexpr std::uint64_t kExpectedRepeatRowCount = 3840;
constexpr std::uint64_t kExpectedRetainedValueCount = 368640;
constexpr std::uint64_t kExpectedRepeatValueCount = 368640;
constexpr std::uint64_t kExpectedRetainedValidityCount = 11520;
constexpr std::uint64_t kExpectedRepeatValidityCount = 11520;

// Every field is a forbidden authorization. The authoritative run is valid
// only when all eight remain false.
struct AuthorizationInput {
  bool training_authorized{false};
  bool augmentation_change_authorized{false};
  bool long_run_authorized{false};
  bool active_policy_change_authorized{false};
  bool checkpoint_migration_authorized{false};
  bool downstream_retraining_authorized{false};
  bool end_to_end_authorized{false};
  bool deployment_authorized{false};
};

// These counters cover every scientific or mutating path forbidden by SRR-2.
// A production selector/capture call is deliberately not one of these paths.
struct ForbiddenCounterInput {
  std::uint64_t training_step_count{0};
  std::uint64_t optimizer_construction_count{0};
  std::uint64_t optimizer_step_count{0};
  std::uint64_t backward_call_count{0};
  std::uint64_t weight_update_count{0};
  std::uint64_t augmentation_change_count{0};
  std::uint64_t target_generation_count{0};
  std::uint64_t probe_construction_count{0};
  std::uint64_t probe_fit_count{0};
  std::uint64_t validation_selection_count{0};
  std::uint64_t prediction_count{0};
  std::uint64_t permutation_count{0};
  std::uint64_t bootstrap_count{0};
  std::uint64_t downstream_retraining_count{0};
  std::uint64_t end_to_end_count{0};
  std::uint64_t deployment_count{0};
};

struct MechanicsInput {
  bool local_contracts_exact{false};
  bool source_boundary_exact{false};
  bool command_exact{false};
  bool environment_exact{false};
  bool cuda_available{false};
  bool attempt_marker_exact{false};
  bool capture_contracts_exact{false};
  bool purity_exact{false};
  bool finite_outputs_exact{false};
  bool deterministic_execution_exact{false};
  bool manifest_exact{false};
  bool audit_input_exact{false};
  std::uint64_t authoritative_attempt_count{0};
  AuthorizationInput authorizations{};
  ForbiddenCounterInput counters{};
};

struct ParentEvidenceInput {
  bool artifacts_exact{false};
  bool hashes_exact{false};
  bool terminal_classification_exact{false};
  bool audit_pass{false};
  bool authorizations_false{false};
  std::uint64_t authoritative_attempt_count{0};
  std::uint64_t audit_error_count{0};
  std::uint64_t optimizer_step_count{0};
  std::uint64_t backward_call_count{0};
};

struct BackwardCompatibilityInput {
  bool legacy_enum_ordinals_exact{false};
  bool legacy_policy_names_exact{false};
  bool structured_policy_appended{false};
  bool structured_policy_name_exact{false};
  bool parser_round_trip_exact{false};
  bool unknown_policy_rejected{false};
  bool cpp_default_all_tokens{false};
  bool omitted_dsl_all_tokens{false};
  bool active_dsl_all_tokens{false};
  bool protocol_fingerprint_distinct{false};
  bool structured_checkpoint_round_trip_exact{false};
  bool legacy_checkpoint_all_tokens{false};
  bool legacy_checkpoint_does_not_inherit_structured{false};
  bool checkpoint_mismatch_rejected{false};
  bool malformed_checkpoint_rejected{false};
  bool legacy_policy_bytes_exact{false};
  bool public_selector_contract_exact{false};
  bool adapter_reaches_structured_selector{false};
};

struct SealedReferenceInput {
  bool archived_base_custody_exact{false};
  bool candidate_delta_custody_exact{false};
  bool production_shadow_source_independent{false};
  bool q0_identity_exact{false};
  bool qpsm_identity_exact{false};
  bool projection_invariants_exact{false};
  bool layout_and_metadata_exact{false};
  bool canonical_plan_exact{false};
  bool parent_shadow_identities_exact{false};
  bool canonical_reference_identity_exact{false};
  bool all_reference_keys_exact{false};
};

struct ProductionShadowParityInput {
  bool shape_exact{false};
  bool strides_and_contiguity_exact{false};
  bool dtype_exact{false};
  bool device_exact{false};
  bool valid_mask_bytes_exact{false};
  bool value_bytes_exact{false};
  bool cpu64_valid_mask_bytes_exact{false};
  bool cpu64_value_bytes_exact{false};
  bool stable_hashes_exact{false};
  bool repeat_capture_identity_exact{false};
  bool per_capture_coverage_exact{false};
  bool coverage_counts_recomputed_from_records{false};
  double cpu64_max_abs{0.0};
  double device_max_abs{0.0};
  std::uint64_t seed_count{0};
  std::uint64_t dataset_count{0};
  std::uint64_t retained_capture_count{0};
  std::uint64_t repeat_capture_count{0};
  std::uint64_t retained_row_count{0};
  std::uint64_t repeat_row_count{0};
  std::uint64_t retained_value_count{0};
  std::uint64_t repeat_value_count{0};
  std::uint64_t retained_validity_count{0};
  std::uint64_t repeat_validity_count{0};
};

struct DeviceTranslationInput {
  bool cpu64_reference_shape_exact{false};
  bool cpu64_reference_mask_bytes_exact{false};
  bool cpu64_production_reference_bytes_exact{false};
  bool cpu64_shadow_reference_bytes_exact{false};
  bool device_reference_contract_exact{false};
  double cpu64_production_reference_max_abs{0.0};
  double cpu64_shadow_reference_max_abs{0.0};
  double device_production_reference_max_abs{0.0};
  double device_shadow_reference_max_abs{0.0};
};

// These are the complete deterministic-domain and independently audited SRR-1
// premises needed to transport the sealed shadow quality result to production.
struct QualityTransportInput {
  bool features_and_masks_cover_parent_domain{false};
  bool targets_exact{false};
  bool group_splits_exact{false};
  bool sample_ladder_exact{false};
  bool alpha_grid_exact{false};
  bool standardization_exact{false};
  bool target_centering_exact{false};
  bool fit_and_validation_selection_exact{false};
  bool test_rows_exact{false};
  bool permutations_exact{false};
  bool bootstrap_rows_exact{false};
  bool decision_thresholds_exact{false};
  bool parent_material_gain_over_channel{false};
  bool parent_noninferior_to_encoder{false};
  bool parent_order_decodable{false};
  bool parent_continuous_shuffle_pass{false};
  bool parent_order_shuffle_pass{false};
  bool parent_terminal_reproduced{false};
};

struct GateInput {
  MechanicsInput mechanics{};
  ParentEvidenceInput parent{};
  BackwardCompatibilityInput compatibility{};
  SealedReferenceInput sealed_reference{};
  ProductionShadowParityInput parity{};
  DeviceTranslationInput device_translation{};
  QualityTransportInput quality_transport{};
};

enum class TerminalClassification {
  invalid_mechanics,
  parent_evidence_failure,
  backward_compatibility_failure,
  sealed_reference_failure,
  production_shadow_parity_failure,
  device_translation_failure,
  production_readout_gate_failure,
  production_structured_readout_parity_reproduced,
};

[[nodiscard]] inline const char *
terminal_classification_name(TerminalClassification classification) {
  switch (classification) {
  case TerminalClassification::invalid_mechanics:
    return "invalid_mechanics";
  case TerminalClassification::parent_evidence_failure:
    return "parent_evidence_failure";
  case TerminalClassification::backward_compatibility_failure:
    return "backward_compatibility_failure";
  case TerminalClassification::sealed_reference_failure:
    return "sealed_reference_failure";
  case TerminalClassification::production_shadow_parity_failure:
    return "production_shadow_parity_failure";
  case TerminalClassification::device_translation_failure:
    return "device_translation_failure";
  case TerminalClassification::production_readout_gate_failure:
    return "production_readout_gate_failure";
  case TerminalClassification::production_structured_readout_parity_reproduced:
    return "production_structured_readout_parity_reproduced";
  }
  return "invalid_mechanics";
}

enum class FailureReason {
  none,
  invalid_numeric,
  local_contract,
  source_boundary,
  command,
  environment,
  cuda_unavailable,
  attempt_contract,
  capture_contract,
  purity_contract,
  finite_output_contract,
  deterministic_contract,
  manifest,
  audit_input,
  authorization,
  nonzero_counter,
  parent_artifact,
  parent_hash,
  parent_classification,
  parent_attempt_count,
  parent_audit,
  parent_counter,
  parent_authorization,
  enum_contract,
  policy_name,
  parser_contract,
  default_contract,
  dsl_contract,
  fingerprint_contract,
  checkpoint_contract,
  legacy_policy_regression,
  selector_adapter_contract,
  archived_base_custody,
  candidate_delta_custody,
  production_shadow_source_boundary,
  projection_contract,
  layout_contract,
  shadow_identity,
  canonical_reference_identity,
  reference_keys,
  production_shadow_tensor_contract,
  production_shadow_mask_identity,
  production_shadow_value_identity,
  production_shadow_hash_identity,
  production_shadow_repeat_identity,
  production_shadow_exact_zero,
  production_shadow_coverage,
  cpu64_reference_contract,
  cpu64_reference_exact_zero,
  device_contract,
  device_tolerance,
  quality_transport_identity,
  parent_material_gain,
  parent_noninferiority,
  parent_order,
  parent_continuous_control,
  parent_order_control,
  parent_terminal,
};

[[nodiscard]] inline const char *failure_reason_name(FailureReason reason) {
  switch (reason) {
  case FailureReason::none:
    return "none";
  case FailureReason::invalid_numeric:
    return "invalid_numeric";
  case FailureReason::local_contract:
    return "local_contract";
  case FailureReason::source_boundary:
    return "source_boundary";
  case FailureReason::command:
    return "command";
  case FailureReason::environment:
    return "environment";
  case FailureReason::cuda_unavailable:
    return "cuda_unavailable";
  case FailureReason::attempt_contract:
    return "attempt_contract";
  case FailureReason::capture_contract:
    return "capture_contract";
  case FailureReason::purity_contract:
    return "purity_contract";
  case FailureReason::finite_output_contract:
    return "finite_output_contract";
  case FailureReason::deterministic_contract:
    return "deterministic_contract";
  case FailureReason::manifest:
    return "manifest";
  case FailureReason::audit_input:
    return "audit_input";
  case FailureReason::authorization:
    return "authorization";
  case FailureReason::nonzero_counter:
    return "nonzero_counter";
  case FailureReason::parent_artifact:
    return "parent_artifact";
  case FailureReason::parent_hash:
    return "parent_hash";
  case FailureReason::parent_classification:
    return "parent_classification";
  case FailureReason::parent_attempt_count:
    return "parent_attempt_count";
  case FailureReason::parent_audit:
    return "parent_audit";
  case FailureReason::parent_counter:
    return "parent_counter";
  case FailureReason::parent_authorization:
    return "parent_authorization";
  case FailureReason::enum_contract:
    return "enum_contract";
  case FailureReason::policy_name:
    return "policy_name";
  case FailureReason::parser_contract:
    return "parser_contract";
  case FailureReason::default_contract:
    return "default_contract";
  case FailureReason::dsl_contract:
    return "dsl_contract";
  case FailureReason::fingerprint_contract:
    return "fingerprint_contract";
  case FailureReason::checkpoint_contract:
    return "checkpoint_contract";
  case FailureReason::legacy_policy_regression:
    return "legacy_policy_regression";
  case FailureReason::selector_adapter_contract:
    return "selector_adapter_contract";
  case FailureReason::archived_base_custody:
    return "archived_base_custody";
  case FailureReason::candidate_delta_custody:
    return "candidate_delta_custody";
  case FailureReason::production_shadow_source_boundary:
    return "production_shadow_source_boundary";
  case FailureReason::projection_contract:
    return "projection_contract";
  case FailureReason::layout_contract:
    return "layout_contract";
  case FailureReason::shadow_identity:
    return "shadow_identity";
  case FailureReason::canonical_reference_identity:
    return "canonical_reference_identity";
  case FailureReason::reference_keys:
    return "reference_keys";
  case FailureReason::production_shadow_tensor_contract:
    return "production_shadow_tensor_contract";
  case FailureReason::production_shadow_mask_identity:
    return "production_shadow_mask_identity";
  case FailureReason::production_shadow_value_identity:
    return "production_shadow_value_identity";
  case FailureReason::production_shadow_hash_identity:
    return "production_shadow_hash_identity";
  case FailureReason::production_shadow_repeat_identity:
    return "production_shadow_repeat_identity";
  case FailureReason::production_shadow_exact_zero:
    return "production_shadow_exact_zero";
  case FailureReason::production_shadow_coverage:
    return "production_shadow_coverage";
  case FailureReason::cpu64_reference_contract:
    return "cpu64_reference_contract";
  case FailureReason::cpu64_reference_exact_zero:
    return "cpu64_reference_exact_zero";
  case FailureReason::device_contract:
    return "device_contract";
  case FailureReason::device_tolerance:
    return "device_tolerance";
  case FailureReason::quality_transport_identity:
    return "quality_transport_identity";
  case FailureReason::parent_material_gain:
    return "parent_material_gain";
  case FailureReason::parent_noninferiority:
    return "parent_noninferiority";
  case FailureReason::parent_order:
    return "parent_order";
  case FailureReason::parent_continuous_control:
    return "parent_continuous_control";
  case FailureReason::parent_order_control:
    return "parent_order_control";
  case FailureReason::parent_terminal:
    return "parent_terminal";
  }
  return "invalid_numeric";
}

struct GateResult {
  bool numeric_inputs_valid{false};
  bool authorization_boundary_valid{false};
  bool zero_counters_valid{false};
  bool mechanics_valid{false};
  bool parent_evidence_valid{false};
  bool backward_compatibility_valid{false};
  bool sealed_reference_valid{false};
  bool coverage_valid{false};
  bool production_shadow_parity_valid{false};
  bool cpu64_reference_valid{false};
  bool device_translation_valid{false};
  bool production_readout_gate_valid{false};
  FailureReason failure_reason{FailureReason::invalid_numeric};
  TerminalClassification classification{
      TerminalClassification::invalid_mechanics};
};

namespace detail {

[[nodiscard]] inline bool finite_nonnegative(double value) {
  return std::isfinite(value) && value >= 0.0;
}

[[nodiscard]] inline bool
authorizations_clear(const AuthorizationInput &input) {
  return !input.training_authorized && !input.augmentation_change_authorized &&
         !input.long_run_authorized && !input.active_policy_change_authorized &&
         !input.checkpoint_migration_authorized &&
         !input.downstream_retraining_authorized &&
         !input.end_to_end_authorized && !input.deployment_authorized;
}

[[nodiscard]] inline bool counters_zero(const ForbiddenCounterInput &input) {
  return input.training_step_count == 0 &&
         input.optimizer_construction_count == 0 &&
         input.optimizer_step_count == 0 && input.backward_call_count == 0 &&
         input.weight_update_count == 0 &&
         input.augmentation_change_count == 0 &&
         input.target_generation_count == 0 &&
         input.probe_construction_count == 0 && input.probe_fit_count == 0 &&
         input.validation_selection_count == 0 && input.prediction_count == 0 &&
         input.permutation_count == 0 && input.bootstrap_count == 0 &&
         input.downstream_retraining_count == 0 &&
         input.end_to_end_count == 0 && input.deployment_count == 0;
}

[[nodiscard]] inline bool
coverage_exact(const ProductionShadowParityInput &input) {
  return input.per_capture_coverage_exact &&
         input.coverage_counts_recomputed_from_records &&
         input.seed_count == kExpectedSeedCount &&
         input.dataset_count == kExpectedDatasetCount &&
         input.retained_capture_count == kExpectedRetainedCaptureCount &&
         input.repeat_capture_count == kExpectedRepeatCaptureCount &&
         input.retained_row_count == kExpectedRetainedRowCount &&
         input.repeat_row_count == kExpectedRepeatRowCount &&
         input.retained_value_count == kExpectedRetainedValueCount &&
         input.repeat_value_count == kExpectedRepeatValueCount &&
         input.retained_validity_count == kExpectedRetainedValidityCount &&
         input.repeat_validity_count == kExpectedRepeatValidityCount;
}

} // namespace detail

[[nodiscard]] inline GateResult evaluate(const GateInput &input) {
  GateResult result{};

  result.numeric_inputs_valid =
      detail::finite_nonnegative(input.parity.cpu64_max_abs) &&
      detail::finite_nonnegative(input.parity.device_max_abs) &&
      detail::finite_nonnegative(
          input.device_translation.cpu64_production_reference_max_abs) &&
      detail::finite_nonnegative(
          input.device_translation.cpu64_shadow_reference_max_abs) &&
      detail::finite_nonnegative(
          input.device_translation.device_production_reference_max_abs) &&
      detail::finite_nonnegative(
          input.device_translation.device_shadow_reference_max_abs);
  if (!result.numeric_inputs_valid) {
    result.failure_reason = FailureReason::invalid_numeric;
    return result;
  }

  if (!input.mechanics.local_contracts_exact) {
    result.failure_reason = FailureReason::local_contract;
    return result;
  }
  if (!input.mechanics.source_boundary_exact) {
    result.failure_reason = FailureReason::source_boundary;
    return result;
  }
  if (!input.mechanics.command_exact) {
    result.failure_reason = FailureReason::command;
    return result;
  }
  if (!input.mechanics.environment_exact) {
    result.failure_reason = FailureReason::environment;
    return result;
  }
  if (!input.mechanics.cuda_available) {
    result.failure_reason = FailureReason::cuda_unavailable;
    return result;
  }
  if (!input.mechanics.attempt_marker_exact ||
      input.mechanics.authoritative_attempt_count != 1) {
    result.failure_reason = FailureReason::attempt_contract;
    return result;
  }
  if (!input.mechanics.capture_contracts_exact) {
    result.failure_reason = FailureReason::capture_contract;
    return result;
  }
  if (!input.mechanics.purity_exact) {
    result.failure_reason = FailureReason::purity_contract;
    return result;
  }
  if (!input.mechanics.finite_outputs_exact) {
    result.failure_reason = FailureReason::finite_output_contract;
    return result;
  }
  if (!input.mechanics.deterministic_execution_exact) {
    result.failure_reason = FailureReason::deterministic_contract;
    return result;
  }
  if (!input.mechanics.manifest_exact) {
    result.failure_reason = FailureReason::manifest;
    return result;
  }
  if (!input.mechanics.audit_input_exact) {
    result.failure_reason = FailureReason::audit_input;
    return result;
  }
  result.authorization_boundary_valid =
      detail::authorizations_clear(input.mechanics.authorizations);
  if (!result.authorization_boundary_valid) {
    result.failure_reason = FailureReason::authorization;
    return result;
  }
  result.zero_counters_valid = detail::counters_zero(input.mechanics.counters);
  if (!result.zero_counters_valid) {
    result.failure_reason = FailureReason::nonzero_counter;
    return result;
  }
  result.mechanics_valid = true;

  const auto parent_failure = [&](FailureReason reason) {
    result.failure_reason = reason;
    result.classification = TerminalClassification::parent_evidence_failure;
  };
  if (!input.parent.artifacts_exact) {
    parent_failure(FailureReason::parent_artifact);
    return result;
  }
  if (!input.parent.hashes_exact) {
    parent_failure(FailureReason::parent_hash);
    return result;
  }
  if (!input.parent.terminal_classification_exact) {
    parent_failure(FailureReason::parent_classification);
    return result;
  }
  if (input.parent.authoritative_attempt_count != 1) {
    parent_failure(FailureReason::parent_attempt_count);
    return result;
  }
  if (!input.parent.audit_pass || input.parent.audit_error_count != 0) {
    parent_failure(FailureReason::parent_audit);
    return result;
  }
  if (input.parent.optimizer_step_count != 0 ||
      input.parent.backward_call_count != 0) {
    parent_failure(FailureReason::parent_counter);
    return result;
  }
  if (!input.parent.authorizations_false) {
    parent_failure(FailureReason::parent_authorization);
    return result;
  }
  result.parent_evidence_valid = true;

  const auto compatibility_failure = [&](FailureReason reason) {
    result.failure_reason = reason;
    result.classification =
        TerminalClassification::backward_compatibility_failure;
  };
  if (!input.compatibility.legacy_enum_ordinals_exact ||
      !input.compatibility.structured_policy_appended) {
    compatibility_failure(FailureReason::enum_contract);
    return result;
  }
  if (!input.compatibility.legacy_policy_names_exact ||
      !input.compatibility.structured_policy_name_exact) {
    compatibility_failure(FailureReason::policy_name);
    return result;
  }
  if (!input.compatibility.parser_round_trip_exact ||
      !input.compatibility.unknown_policy_rejected) {
    compatibility_failure(FailureReason::parser_contract);
    return result;
  }
  if (!input.compatibility.cpp_default_all_tokens) {
    compatibility_failure(FailureReason::default_contract);
    return result;
  }
  if (!input.compatibility.omitted_dsl_all_tokens ||
      !input.compatibility.active_dsl_all_tokens) {
    compatibility_failure(FailureReason::dsl_contract);
    return result;
  }
  if (!input.compatibility.protocol_fingerprint_distinct) {
    compatibility_failure(FailureReason::fingerprint_contract);
    return result;
  }
  if (!input.compatibility.structured_checkpoint_round_trip_exact ||
      !input.compatibility.legacy_checkpoint_all_tokens ||
      !input.compatibility.legacy_checkpoint_does_not_inherit_structured ||
      !input.compatibility.checkpoint_mismatch_rejected ||
      !input.compatibility.malformed_checkpoint_rejected) {
    compatibility_failure(FailureReason::checkpoint_contract);
    return result;
  }
  if (!input.compatibility.legacy_policy_bytes_exact) {
    compatibility_failure(FailureReason::legacy_policy_regression);
    return result;
  }
  if (!input.compatibility.public_selector_contract_exact ||
      !input.compatibility.adapter_reaches_structured_selector) {
    compatibility_failure(FailureReason::selector_adapter_contract);
    return result;
  }
  result.backward_compatibility_valid = true;

  const auto sealed_failure = [&](FailureReason reason) {
    result.failure_reason = reason;
    result.classification = TerminalClassification::sealed_reference_failure;
  };
  if (!input.sealed_reference.archived_base_custody_exact) {
    sealed_failure(FailureReason::archived_base_custody);
    return result;
  }
  if (!input.sealed_reference.candidate_delta_custody_exact) {
    sealed_failure(FailureReason::candidate_delta_custody);
    return result;
  }
  if (!input.sealed_reference.production_shadow_source_independent) {
    sealed_failure(FailureReason::production_shadow_source_boundary);
    return result;
  }
  if (!input.sealed_reference.q0_identity_exact ||
      !input.sealed_reference.qpsm_identity_exact ||
      !input.sealed_reference.projection_invariants_exact) {
    sealed_failure(FailureReason::projection_contract);
    return result;
  }
  if (!input.sealed_reference.layout_and_metadata_exact ||
      !input.sealed_reference.canonical_plan_exact) {
    sealed_failure(FailureReason::layout_contract);
    return result;
  }
  if (!input.sealed_reference.parent_shadow_identities_exact) {
    sealed_failure(FailureReason::shadow_identity);
    return result;
  }
  if (!input.sealed_reference.canonical_reference_identity_exact) {
    sealed_failure(FailureReason::canonical_reference_identity);
    return result;
  }
  if (!input.sealed_reference.all_reference_keys_exact) {
    sealed_failure(FailureReason::reference_keys);
    return result;
  }
  result.sealed_reference_valid = true;

  const auto parity_failure = [&](FailureReason reason) {
    result.failure_reason = reason;
    result.classification =
        TerminalClassification::production_shadow_parity_failure;
  };
  if (!input.parity.shape_exact || !input.parity.strides_and_contiguity_exact ||
      !input.parity.dtype_exact || !input.parity.device_exact) {
    parity_failure(FailureReason::production_shadow_tensor_contract);
    return result;
  }
  if (!input.parity.valid_mask_bytes_exact ||
      !input.parity.cpu64_valid_mask_bytes_exact) {
    parity_failure(FailureReason::production_shadow_mask_identity);
    return result;
  }
  if (!input.parity.value_bytes_exact ||
      !input.parity.cpu64_value_bytes_exact) {
    parity_failure(FailureReason::production_shadow_value_identity);
    return result;
  }
  if (!input.parity.stable_hashes_exact) {
    parity_failure(FailureReason::production_shadow_hash_identity);
    return result;
  }
  if (!input.parity.repeat_capture_identity_exact) {
    parity_failure(FailureReason::production_shadow_repeat_identity);
    return result;
  }
  if (input.parity.cpu64_max_abs != 0.0 || input.parity.device_max_abs != 0.0) {
    parity_failure(FailureReason::production_shadow_exact_zero);
    return result;
  }
  result.coverage_valid = detail::coverage_exact(input.parity);
  if (!result.coverage_valid) {
    parity_failure(FailureReason::production_shadow_coverage);
    return result;
  }
  result.production_shadow_parity_valid = true;

  const auto device_failure = [&](FailureReason reason) {
    result.failure_reason = reason;
    result.classification = TerminalClassification::device_translation_failure;
  };
  if (!input.device_translation.cpu64_reference_shape_exact ||
      !input.device_translation.cpu64_reference_mask_bytes_exact ||
      !input.device_translation.cpu64_production_reference_bytes_exact ||
      !input.device_translation.cpu64_shadow_reference_bytes_exact) {
    device_failure(FailureReason::cpu64_reference_contract);
    return result;
  }
  if (input.device_translation.cpu64_production_reference_max_abs != 0.0 ||
      input.device_translation.cpu64_shadow_reference_max_abs != 0.0) {
    device_failure(FailureReason::cpu64_reference_exact_zero);
    return result;
  }
  result.cpu64_reference_valid = true;
  if (!input.device_translation.device_reference_contract_exact) {
    device_failure(FailureReason::device_contract);
    return result;
  }
  if (input.device_translation.device_production_reference_max_abs >
          kDeviceTranslationTolerance ||
      input.device_translation.device_shadow_reference_max_abs >
          kDeviceTranslationTolerance) {
    device_failure(FailureReason::device_tolerance);
    return result;
  }
  result.device_translation_valid = true;

  const auto quality_failure = [&](FailureReason reason) {
    result.failure_reason = reason;
    result.classification =
        TerminalClassification::production_readout_gate_failure;
  };
  const auto &quality = input.quality_transport;
  if (!quality.features_and_masks_cover_parent_domain ||
      !quality.targets_exact || !quality.group_splits_exact ||
      !quality.sample_ladder_exact || !quality.alpha_grid_exact ||
      !quality.standardization_exact || !quality.target_centering_exact ||
      !quality.fit_and_validation_selection_exact || !quality.test_rows_exact ||
      !quality.permutations_exact || !quality.bootstrap_rows_exact ||
      !quality.decision_thresholds_exact) {
    quality_failure(FailureReason::quality_transport_identity);
    return result;
  }
  if (!quality.parent_material_gain_over_channel) {
    quality_failure(FailureReason::parent_material_gain);
    return result;
  }
  if (!quality.parent_noninferior_to_encoder) {
    quality_failure(FailureReason::parent_noninferiority);
    return result;
  }
  if (!quality.parent_order_decodable) {
    quality_failure(FailureReason::parent_order);
    return result;
  }
  if (!quality.parent_continuous_shuffle_pass) {
    quality_failure(FailureReason::parent_continuous_control);
    return result;
  }
  if (!quality.parent_order_shuffle_pass) {
    quality_failure(FailureReason::parent_order_control);
    return result;
  }
  if (!quality.parent_terminal_reproduced) {
    quality_failure(FailureReason::parent_terminal);
    return result;
  }
  result.production_readout_gate_valid = true;
  result.failure_reason = FailureReason::none;
  result.classification =
      TerminalClassification::production_structured_readout_parity_reproduced;
  return result;
}

} // namespace cuwacunu::tests::production_structured_readout_parity_gate
