#include "production_structured_readout_parity_gate.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace gate = cuwacunu::tests::production_structured_readout_parity_gate;

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

gate::GateInput passing() {
  gate::GateInput input{};

  input.mechanics.local_contracts_exact = true;
  input.mechanics.source_boundary_exact = true;
  input.mechanics.command_exact = true;
  input.mechanics.environment_exact = true;
  input.mechanics.cuda_available = true;
  input.mechanics.attempt_marker_exact = true;
  input.mechanics.capture_contracts_exact = true;
  input.mechanics.purity_exact = true;
  input.mechanics.finite_outputs_exact = true;
  input.mechanics.deterministic_execution_exact = true;
  input.mechanics.manifest_exact = true;
  input.mechanics.audit_input_exact = true;
  input.mechanics.authoritative_attempt_count = 1;

  input.parent.artifacts_exact = true;
  input.parent.hashes_exact = true;
  input.parent.terminal_classification_exact = true;
  input.parent.audit_pass = true;
  input.parent.authorizations_false = true;
  input.parent.authoritative_attempt_count = 1;

  input.compatibility.legacy_enum_ordinals_exact = true;
  input.compatibility.legacy_policy_names_exact = true;
  input.compatibility.structured_policy_appended = true;
  input.compatibility.structured_policy_name_exact = true;
  input.compatibility.parser_round_trip_exact = true;
  input.compatibility.unknown_policy_rejected = true;
  input.compatibility.cpp_default_all_tokens = true;
  input.compatibility.omitted_dsl_all_tokens = true;
  input.compatibility.active_dsl_all_tokens = true;
  input.compatibility.protocol_fingerprint_distinct = true;
  input.compatibility.structured_checkpoint_round_trip_exact = true;
  input.compatibility.legacy_checkpoint_all_tokens = true;
  input.compatibility.legacy_checkpoint_does_not_inherit_structured = true;
  input.compatibility.checkpoint_mismatch_rejected = true;
  input.compatibility.malformed_checkpoint_rejected = true;
  input.compatibility.legacy_policy_bytes_exact = true;
  input.compatibility.public_selector_contract_exact = true;
  input.compatibility.adapter_reaches_structured_selector = true;

  input.sealed_reference.archived_base_custody_exact = true;
  input.sealed_reference.candidate_delta_custody_exact = true;
  input.sealed_reference.production_shadow_source_independent = true;
  input.sealed_reference.q0_identity_exact = true;
  input.sealed_reference.qpsm_identity_exact = true;
  input.sealed_reference.projection_invariants_exact = true;
  input.sealed_reference.layout_and_metadata_exact = true;
  input.sealed_reference.canonical_plan_exact = true;
  input.sealed_reference.parent_shadow_identities_exact = true;
  input.sealed_reference.canonical_reference_identity_exact = true;
  input.sealed_reference.all_reference_keys_exact = true;

  input.parity.shape_exact = true;
  input.parity.strides_and_contiguity_exact = true;
  input.parity.dtype_exact = true;
  input.parity.device_exact = true;
  input.parity.valid_mask_bytes_exact = true;
  input.parity.value_bytes_exact = true;
  input.parity.cpu64_valid_mask_bytes_exact = true;
  input.parity.cpu64_value_bytes_exact = true;
  input.parity.stable_hashes_exact = true;
  input.parity.repeat_capture_identity_exact = true;
  input.parity.per_capture_coverage_exact = true;
  input.parity.coverage_counts_recomputed_from_records = true;
  input.parity.seed_count = gate::kExpectedSeedCount;
  input.parity.dataset_count = gate::kExpectedDatasetCount;
  input.parity.retained_capture_count = gate::kExpectedRetainedCaptureCount;
  input.parity.repeat_capture_count = gate::kExpectedRepeatCaptureCount;
  input.parity.retained_row_count = gate::kExpectedRetainedRowCount;
  input.parity.repeat_row_count = gate::kExpectedRepeatRowCount;
  input.parity.retained_value_count = gate::kExpectedRetainedValueCount;
  input.parity.repeat_value_count = gate::kExpectedRepeatValueCount;
  input.parity.retained_validity_count = gate::kExpectedRetainedValidityCount;
  input.parity.repeat_validity_count = gate::kExpectedRepeatValidityCount;

  input.device_translation.cpu64_reference_shape_exact = true;
  input.device_translation.cpu64_reference_mask_bytes_exact = true;
  input.device_translation.cpu64_production_reference_bytes_exact = true;
  input.device_translation.cpu64_shadow_reference_bytes_exact = true;
  input.device_translation.device_reference_contract_exact = true;

  input.quality_transport.features_and_masks_cover_parent_domain = true;
  input.quality_transport.targets_exact = true;
  input.quality_transport.group_splits_exact = true;
  input.quality_transport.sample_ladder_exact = true;
  input.quality_transport.alpha_grid_exact = true;
  input.quality_transport.standardization_exact = true;
  input.quality_transport.target_centering_exact = true;
  input.quality_transport.fit_and_validation_selection_exact = true;
  input.quality_transport.test_rows_exact = true;
  input.quality_transport.permutations_exact = true;
  input.quality_transport.bootstrap_rows_exact = true;
  input.quality_transport.decision_thresholds_exact = true;
  input.quality_transport.parent_material_gain_over_channel = true;
  input.quality_transport.parent_noninferior_to_encoder = true;
  input.quality_transport.parent_order_decodable = true;
  input.quality_transport.parent_continuous_shuffle_pass = true;
  input.quality_transport.parent_order_shuffle_pass = true;
  input.quality_transport.parent_terminal_reproduced = true;
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

void test_success_and_fail_closed_default() {
  const auto result = gate::evaluate(passing());
  expect(result.numeric_inputs_valid && result.authorization_boundary_valid &&
             result.zero_counters_valid && result.mechanics_valid &&
             result.parent_evidence_valid &&
             result.backward_compatibility_valid &&
             result.sealed_reference_valid && result.coverage_valid &&
             result.production_shadow_parity_valid &&
             result.cpu64_reference_valid && result.device_translation_valid &&
             result.production_readout_gate_valid,
         "passing input did not clear every gate stage");
  expect_gate(passing(),
              gate::TerminalClassification::
                  production_structured_readout_parity_reproduced,
              gate::FailureReason::none, "passing SRR-2 gate");

  expect_gate(
      gate::GateInput{}, gate::TerminalClassification::invalid_mechanics,
      gate::FailureReason::local_contract, "default input did not fail closed");
}

template <typename Owner> struct BoolCase {
  bool Owner::*member;
  gate::FailureReason reason;
  const char *name;
};

void test_mechanics_and_authorization_boundary() {
  using Mechanics = gate::MechanicsInput;
  const BoolCase<Mechanics> mechanics_cases[] = {
      {&Mechanics::local_contracts_exact, gate::FailureReason::local_contract,
       "local contracts"},
      {&Mechanics::source_boundary_exact, gate::FailureReason::source_boundary,
       "source boundary"},
      {&Mechanics::command_exact, gate::FailureReason::command, "command"},
      {&Mechanics::environment_exact, gate::FailureReason::environment,
       "environment"},
      {&Mechanics::cuda_available, gate::FailureReason::cuda_unavailable,
       "CUDA availability"},
      {&Mechanics::attempt_marker_exact, gate::FailureReason::attempt_contract,
       "attempt marker"},
      {&Mechanics::capture_contracts_exact,
       gate::FailureReason::capture_contract, "capture contracts"},
      {&Mechanics::purity_exact, gate::FailureReason::purity_contract,
       "purity"},
      {&Mechanics::finite_outputs_exact,
       gate::FailureReason::finite_output_contract, "finite outputs"},
      {&Mechanics::deterministic_execution_exact,
       gate::FailureReason::deterministic_contract, "determinism"},
      {&Mechanics::manifest_exact, gate::FailureReason::manifest, "manifest"},
      {&Mechanics::audit_input_exact, gate::FailureReason::audit_input,
       "audit input"},
  };
  for (const auto &test : mechanics_cases) {
    auto input = passing();
    input.mechanics.*(test.member) = false;
    expect_gate(input, gate::TerminalClassification::invalid_mechanics,
                test.reason, std::string("mechanics: ") + test.name);
  }

  for (const std::uint64_t count : {std::uint64_t{0}, std::uint64_t{2}}) {
    auto input = passing();
    input.mechanics.authoritative_attempt_count = count;
    expect_gate(input, gate::TerminalClassification::invalid_mechanics,
                gate::FailureReason::attempt_contract,
                "authoritative attempt count boundary");
  }

  using Authorization = gate::AuthorizationInput;
  bool Authorization::*const authorization_fields[] = {
      &Authorization::training_authorized,
      &Authorization::augmentation_change_authorized,
      &Authorization::long_run_authorized,
      &Authorization::active_policy_change_authorized,
      &Authorization::checkpoint_migration_authorized,
      &Authorization::downstream_retraining_authorized,
      &Authorization::end_to_end_authorized,
      &Authorization::deployment_authorized,
  };
  for (const auto field : authorization_fields) {
    auto input = passing();
    input.mechanics.authorizations.*field = true;
    expect_gate(input, gate::TerminalClassification::invalid_mechanics,
                gate::FailureReason::authorization,
                "forbidden authorization was accepted");
  }

  using Counters = gate::ForbiddenCounterInput;
  std::uint64_t Counters::*const counter_fields[] = {
      &Counters::training_step_count,
      &Counters::optimizer_construction_count,
      &Counters::optimizer_step_count,
      &Counters::backward_call_count,
      &Counters::weight_update_count,
      &Counters::augmentation_change_count,
      &Counters::target_generation_count,
      &Counters::probe_construction_count,
      &Counters::probe_fit_count,
      &Counters::validation_selection_count,
      &Counters::prediction_count,
      &Counters::permutation_count,
      &Counters::bootstrap_count,
      &Counters::downstream_retraining_count,
      &Counters::end_to_end_count,
      &Counters::deployment_count,
  };
  for (const auto field : counter_fields) {
    auto input = passing();
    input.mechanics.counters.*field = 1;
    expect_gate(input, gate::TerminalClassification::invalid_mechanics,
                gate::FailureReason::nonzero_counter,
                "nonzero forbidden counter was accepted");
  }
}

void test_numeric_and_tolerance_boundaries() {
  using Setter = void (*)(gate::GateInput &, double);
  const Setter maximum_setters[] = {
      [](gate::GateInput &input, double value) {
        input.parity.cpu64_max_abs = value;
      },
      [](gate::GateInput &input, double value) {
        input.parity.device_max_abs = value;
      },
      [](gate::GateInput &input, double value) {
        input.device_translation.cpu64_production_reference_max_abs = value;
      },
      [](gate::GateInput &input, double value) {
        input.device_translation.cpu64_shadow_reference_max_abs = value;
      },
      [](gate::GateInput &input, double value) {
        input.device_translation.device_production_reference_max_abs = value;
      },
      [](gate::GateInput &input, double value) {
        input.device_translation.device_shadow_reference_max_abs = value;
      },
  };
  const double invalid_values[] = {
      -std::numeric_limits<double>::denorm_min(),
      -1.0,
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::infinity(),
  };
  for (const auto setter : maximum_setters) {
    for (const double value : invalid_values) {
      auto input = passing();
      setter(input, value);
      expect_gate(input, gate::TerminalClassification::invalid_mechanics,
                  gate::FailureReason::invalid_numeric,
                  "invalid maximum was accepted");
    }
  }

  auto input = passing();
  input.parity.cpu64_max_abs = above(0.0);
  expect_gate(input,
              gate::TerminalClassification::production_shadow_parity_failure,
              gate::FailureReason::production_shadow_exact_zero,
              "CPU64 P/R maximum above exact zero");
  input = passing();
  input.parity.device_max_abs = above(0.0);
  expect_gate(input,
              gate::TerminalClassification::production_shadow_parity_failure,
              gate::FailureReason::production_shadow_exact_zero,
              "device P/R maximum above exact zero");

  input = passing();
  input.device_translation.cpu64_production_reference_max_abs = above(0.0);
  expect_gate(input, gate::TerminalClassification::device_translation_failure,
              gate::FailureReason::cpu64_reference_exact_zero,
              "CPU64 P/D maximum above exact zero");
  input = passing();
  input.device_translation.cpu64_shadow_reference_max_abs = above(0.0);
  expect_gate(input, gate::TerminalClassification::device_translation_failure,
              gate::FailureReason::cpu64_reference_exact_zero,
              "CPU64 R/D maximum above exact zero");

  for (const bool production : {true, false}) {
    input = passing();
    double &value =
        production
            ? input.device_translation.device_production_reference_max_abs
            : input.device_translation.device_shadow_reference_max_abs;
    value = below(gate::kDeviceTranslationTolerance);
    expect_gate(input,
                gate::TerminalClassification::
                    production_structured_readout_parity_reproduced,
                gate::FailureReason::none,
                "device translation immediately below tolerance");
    value = gate::kDeviceTranslationTolerance;
    expect_gate(input,
                gate::TerminalClassification::
                    production_structured_readout_parity_reproduced,
                gate::FailureReason::none,
                "device translation at inclusive tolerance");
    value = above(gate::kDeviceTranslationTolerance);
    expect_gate(input, gate::TerminalClassification::device_translation_failure,
                gate::FailureReason::device_tolerance,
                "device translation immediately above tolerance");
  }

  input = passing();
  input.parity.cpu64_max_abs = -0.0;
  input.parity.device_max_abs = -0.0;
  input.device_translation.cpu64_production_reference_max_abs = -0.0;
  input.device_translation.cpu64_shadow_reference_max_abs = -0.0;
  expect_gate(input,
              gate::TerminalClassification::
                  production_structured_readout_parity_reproduced,
              gate::FailureReason::none,
              "IEEE negative zero did not satisfy mathematical exact zero");
}

void test_parent_evidence() {
  using Parent = gate::ParentEvidenceInput;
  const BoolCase<Parent> cases[] = {
      {&Parent::artifacts_exact, gate::FailureReason::parent_artifact,
       "artifacts"},
      {&Parent::hashes_exact, gate::FailureReason::parent_hash, "hashes"},
      {&Parent::terminal_classification_exact,
       gate::FailureReason::parent_classification, "classification"},
      {&Parent::audit_pass, gate::FailureReason::parent_audit, "audit"},
      {&Parent::authorizations_false, gate::FailureReason::parent_authorization,
       "authorizations"},
  };
  for (const auto &test : cases) {
    auto input = passing();
    input.parent.*(test.member) = false;
    expect_gate(input, gate::TerminalClassification::parent_evidence_failure,
                test.reason, std::string("parent evidence: ") + test.name);
  }

  for (const std::uint64_t count : {std::uint64_t{0}, std::uint64_t{2}}) {
    auto input = passing();
    input.parent.authoritative_attempt_count = count;
    expect_gate(input, gate::TerminalClassification::parent_evidence_failure,
                gate::FailureReason::parent_attempt_count,
                "parent attempt count boundary");
  }
  auto input = passing();
  input.parent.audit_error_count = 1;
  expect_gate(input, gate::TerminalClassification::parent_evidence_failure,
              gate::FailureReason::parent_audit,
              "nonzero parent audit error count");
  input = passing();
  input.parent.optimizer_step_count = 1;
  expect_gate(input, gate::TerminalClassification::parent_evidence_failure,
              gate::FailureReason::parent_counter,
              "nonzero parent optimizer step count");
  input = passing();
  input.parent.backward_call_count = 1;
  expect_gate(input, gate::TerminalClassification::parent_evidence_failure,
              gate::FailureReason::parent_counter,
              "nonzero parent backward call count");
}

void test_backward_compatibility() {
  using Compatibility = gate::BackwardCompatibilityInput;
  const BoolCase<Compatibility> cases[] = {
      {&Compatibility::legacy_enum_ordinals_exact,
       gate::FailureReason::enum_contract, "legacy enum ordinals"},
      {&Compatibility::legacy_policy_names_exact,
       gate::FailureReason::policy_name, "legacy policy names"},
      {&Compatibility::structured_policy_appended,
       gate::FailureReason::enum_contract, "structured policy append"},
      {&Compatibility::structured_policy_name_exact,
       gate::FailureReason::policy_name, "structured_cdsb_v1 name"},
      {&Compatibility::parser_round_trip_exact,
       gate::FailureReason::parser_contract, "parser round trip"},
      {&Compatibility::unknown_policy_rejected,
       gate::FailureReason::parser_contract, "unknown parser value"},
      {&Compatibility::cpp_default_all_tokens,
       gate::FailureReason::default_contract, "C++ default"},
      {&Compatibility::omitted_dsl_all_tokens,
       gate::FailureReason::dsl_contract, "omitted DSL default"},
      {&Compatibility::active_dsl_all_tokens, gate::FailureReason::dsl_contract,
       "active DSL"},
      {&Compatibility::protocol_fingerprint_distinct,
       gate::FailureReason::fingerprint_contract, "protocol fingerprint"},
      {&Compatibility::structured_checkpoint_round_trip_exact,
       gate::FailureReason::checkpoint_contract,
       "structured checkpoint round trip"},
      {&Compatibility::legacy_checkpoint_all_tokens,
       gate::FailureReason::checkpoint_contract, "legacy checkpoint default"},
      {&Compatibility::legacy_checkpoint_does_not_inherit_structured,
       gate::FailureReason::checkpoint_contract,
       "legacy checkpoint no inheritance"},
      {&Compatibility::checkpoint_mismatch_rejected,
       gate::FailureReason::checkpoint_contract, "checkpoint mismatch"},
      {&Compatibility::malformed_checkpoint_rejected,
       gate::FailureReason::checkpoint_contract, "malformed checkpoint"},
      {&Compatibility::legacy_policy_bytes_exact,
       gate::FailureReason::legacy_policy_regression, "legacy policy bytes"},
      {&Compatibility::public_selector_contract_exact,
       gate::FailureReason::selector_adapter_contract,
       "public selector contract"},
      {&Compatibility::adapter_reaches_structured_selector,
       gate::FailureReason::selector_adapter_contract,
       "inference adapter reachability"},
  };
  for (const auto &test : cases) {
    auto input = passing();
    input.compatibility.*(test.member) = false;
    expect_gate(
        input, gate::TerminalClassification::backward_compatibility_failure,
        test.reason, std::string("backward compatibility: ") + test.name);
  }
}

void test_sealed_reference() {
  using Reference = gate::SealedReferenceInput;
  const BoolCase<Reference> cases[] = {
      {&Reference::archived_base_custody_exact,
       gate::FailureReason::archived_base_custody, "archived base custody"},
      {&Reference::candidate_delta_custody_exact,
       gate::FailureReason::candidate_delta_custody, "candidate delta custody"},
      {&Reference::production_shadow_source_independent,
       gate::FailureReason::production_shadow_source_boundary,
       "production/shadow source boundary"},
      {&Reference::q0_identity_exact, gate::FailureReason::projection_contract,
       "Q0 identity"},
      {&Reference::qpsm_identity_exact,
       gate::FailureReason::projection_contract, "Qpsm identity"},
      {&Reference::projection_invariants_exact,
       gate::FailureReason::projection_contract, "projection invariants"},
      {&Reference::layout_and_metadata_exact,
       gate::FailureReason::layout_contract, "layout and metadata"},
      {&Reference::canonical_plan_exact, gate::FailureReason::layout_contract,
       "canonical plan"},
      {&Reference::parent_shadow_identities_exact,
       gate::FailureReason::shadow_identity, "parent shadow identities"},
      {&Reference::canonical_reference_identity_exact,
       gate::FailureReason::canonical_reference_identity,
       "canonical reference identity"},
      {&Reference::all_reference_keys_exact,
       gate::FailureReason::reference_keys, "reference keys"},
  };
  for (const auto &test : cases) {
    auto input = passing();
    input.sealed_reference.*(test.member) = false;
    expect_gate(input, gate::TerminalClassification::sealed_reference_failure,
                test.reason, std::string("sealed reference: ") + test.name);
  }
}

void test_production_shadow_parity_and_full_coverage() {
  using Parity = gate::ProductionShadowParityInput;
  const BoolCase<Parity> parity_cases[] = {
      {&Parity::shape_exact,
       gate::FailureReason::production_shadow_tensor_contract, "shape"},
      {&Parity::strides_and_contiguity_exact,
       gate::FailureReason::production_shadow_tensor_contract,
       "strides/contiguity"},
      {&Parity::dtype_exact,
       gate::FailureReason::production_shadow_tensor_contract, "dtype"},
      {&Parity::device_exact,
       gate::FailureReason::production_shadow_tensor_contract, "device"},
      {&Parity::valid_mask_bytes_exact,
       gate::FailureReason::production_shadow_mask_identity,
       "device mask bytes"},
      {&Parity::cpu64_valid_mask_bytes_exact,
       gate::FailureReason::production_shadow_mask_identity,
       "CPU64 mask bytes"},
      {&Parity::value_bytes_exact,
       gate::FailureReason::production_shadow_value_identity,
       "device value bytes"},
      {&Parity::cpu64_value_bytes_exact,
       gate::FailureReason::production_shadow_value_identity,
       "CPU64 value bytes"},
      {&Parity::stable_hashes_exact,
       gate::FailureReason::production_shadow_hash_identity, "stable hashes"},
      {&Parity::repeat_capture_identity_exact,
       gate::FailureReason::production_shadow_repeat_identity,
       "repeat capture identity"},
      {&Parity::per_capture_coverage_exact,
       gate::FailureReason::production_shadow_coverage, "per-capture coverage"},
      {&Parity::coverage_counts_recomputed_from_records,
       gate::FailureReason::production_shadow_coverage,
       "coverage recomputation"},
  };
  for (const auto &test : parity_cases) {
    auto input = passing();
    input.parity.*(test.member) = false;
    expect_gate(
        input, gate::TerminalClassification::production_shadow_parity_failure,
        test.reason, std::string("production/shadow parity: ") + test.name);
  }

  struct CountCase {
    std::uint64_t Parity::*member;
    std::uint64_t expected;
    const char *name;
  };
  const CountCase count_cases[] = {
      {&Parity::seed_count, gate::kExpectedSeedCount, "seed count"},
      {&Parity::dataset_count, gate::kExpectedDatasetCount, "dataset count"},
      {&Parity::retained_capture_count, gate::kExpectedRetainedCaptureCount,
       "retained capture count"},
      {&Parity::repeat_capture_count, gate::kExpectedRepeatCaptureCount,
       "repeat capture count"},
      {&Parity::retained_row_count, gate::kExpectedRetainedRowCount,
       "retained row count"},
      {&Parity::repeat_row_count, gate::kExpectedRepeatRowCount,
       "repeat row count"},
      {&Parity::retained_value_count, gate::kExpectedRetainedValueCount,
       "retained value count"},
      {&Parity::repeat_value_count, gate::kExpectedRepeatValueCount,
       "repeat value count"},
      {&Parity::retained_validity_count, gate::kExpectedRetainedValidityCount,
       "retained validity count"},
      {&Parity::repeat_validity_count, gate::kExpectedRepeatValidityCount,
       "repeat validity count"},
  };
  for (const auto &test : count_cases) {
    for (const std::uint64_t value : {test.expected - 1, test.expected + 1}) {
      auto input = passing();
      input.parity.*(test.member) = value;
      expect_gate(
          input, gate::TerminalClassification::production_shadow_parity_failure,
          gate::FailureReason::production_shadow_coverage,
          std::string("coverage boundary: ") + test.name);
    }
  }
}

void test_device_reference_contract() {
  using Device = gate::DeviceTranslationInput;
  const BoolCase<Device> cases[] = {
      {&Device::cpu64_reference_shape_exact,
       gate::FailureReason::cpu64_reference_contract, "CPU64 shape"},
      {&Device::cpu64_reference_mask_bytes_exact,
       gate::FailureReason::cpu64_reference_contract, "CPU64 mask bytes"},
      {&Device::cpu64_production_reference_bytes_exact,
       gate::FailureReason::cpu64_reference_contract, "CPU64 P/D bytes"},
      {&Device::cpu64_shadow_reference_bytes_exact,
       gate::FailureReason::cpu64_reference_contract, "CPU64 R/D bytes"},
      {&Device::device_reference_contract_exact,
       gate::FailureReason::device_contract, "device reference contract"},
  };
  for (const auto &test : cases) {
    auto input = passing();
    input.device_translation.*(test.member) = false;
    expect_gate(input, gate::TerminalClassification::device_translation_failure,
                test.reason, std::string("device translation: ") + test.name);
  }
}

void test_parent_quality_transport_premises() {
  using Quality = gate::QualityTransportInput;
  const BoolCase<Quality> cases[] = {
      {&Quality::features_and_masks_cover_parent_domain,
       gate::FailureReason::quality_transport_identity, "feature/mask domain"},
      {&Quality::targets_exact, gate::FailureReason::quality_transport_identity,
       "targets"},
      {&Quality::group_splits_exact,
       gate::FailureReason::quality_transport_identity, "group splits"},
      {&Quality::sample_ladder_exact,
       gate::FailureReason::quality_transport_identity, "sample ladder"},
      {&Quality::alpha_grid_exact,
       gate::FailureReason::quality_transport_identity, "alpha grid"},
      {&Quality::standardization_exact,
       gate::FailureReason::quality_transport_identity, "standardization"},
      {&Quality::target_centering_exact,
       gate::FailureReason::quality_transport_identity, "target centering"},
      {&Quality::fit_and_validation_selection_exact,
       gate::FailureReason::quality_transport_identity,
       "fit/validation selection"},
      {&Quality::test_rows_exact,
       gate::FailureReason::quality_transport_identity, "test rows"},
      {&Quality::permutations_exact,
       gate::FailureReason::quality_transport_identity, "permutations"},
      {&Quality::bootstrap_rows_exact,
       gate::FailureReason::quality_transport_identity, "bootstrap rows"},
      {&Quality::decision_thresholds_exact,
       gate::FailureReason::quality_transport_identity, "decision thresholds"},
      {&Quality::parent_material_gain_over_channel,
       gate::FailureReason::parent_material_gain, "R-C material gain"},
      {&Quality::parent_noninferior_to_encoder,
       gate::FailureReason::parent_noninferiority, "R-E noninferiority"},
      {&Quality::parent_order_decodable, gate::FailureReason::parent_order,
       "R order decodability"},
      {&Quality::parent_continuous_shuffle_pass,
       gate::FailureReason::parent_continuous_control,
       "continuous shuffle control"},
      {&Quality::parent_order_shuffle_pass,
       gate::FailureReason::parent_order_control, "order shuffle control"},
      {&Quality::parent_terminal_reproduced,
       gate::FailureReason::parent_terminal, "parent terminal"},
  };
  for (const auto &test : cases) {
    auto input = passing();
    input.quality_transport.*(test.member) = false;
    expect_gate(input,
                gate::TerminalClassification::production_readout_gate_failure,
                test.reason, std::string("quality transport: ") + test.name);
  }
}

void test_frozen_terminal_precedence() {
  auto input = passing();
  input.parity.cpu64_max_abs = std::numeric_limits<double>::quiet_NaN();
  input.mechanics.local_contracts_exact = false;
  input.parent.artifacts_exact = false;
  expect_gate(input, gate::TerminalClassification::invalid_mechanics,
              gate::FailureReason::invalid_numeric,
              "invalid numeric precedes other mechanics and parent");

  input = passing();
  input.mechanics.manifest_exact = false;
  input.parent.artifacts_exact = false;
  expect_gate(input, gate::TerminalClassification::invalid_mechanics,
              gate::FailureReason::manifest,
              "invalid mechanics precedes parent evidence");

  input = passing();
  input.parent.artifacts_exact = false;
  input.compatibility.parser_round_trip_exact = false;
  expect_gate(input, gate::TerminalClassification::parent_evidence_failure,
              gate::FailureReason::parent_artifact,
              "parent evidence precedes backward compatibility");

  input = passing();
  input.compatibility.parser_round_trip_exact = false;
  input.sealed_reference.qpsm_identity_exact = false;
  expect_gate(input,
              gate::TerminalClassification::backward_compatibility_failure,
              gate::FailureReason::parser_contract,
              "backward compatibility precedes sealed reference");

  input = passing();
  input.sealed_reference.qpsm_identity_exact = false;
  input.parity.value_bytes_exact = false;
  expect_gate(input, gate::TerminalClassification::sealed_reference_failure,
              gate::FailureReason::projection_contract,
              "sealed reference precedes production/shadow parity");

  input = passing();
  input.parity.value_bytes_exact = false;
  input.device_translation.device_production_reference_max_abs =
      above(gate::kDeviceTranslationTolerance);
  expect_gate(input,
              gate::TerminalClassification::production_shadow_parity_failure,
              gate::FailureReason::production_shadow_value_identity,
              "production/shadow parity precedes device translation");

  input = passing();
  input.device_translation.device_production_reference_max_abs =
      above(gate::kDeviceTranslationTolerance);
  input.quality_transport.parent_material_gain_over_channel = false;
  expect_gate(input, gate::TerminalClassification::device_translation_failure,
              gate::FailureReason::device_tolerance,
              "device translation precedes quality transport");

  input = passing();
  input.quality_transport.parent_material_gain_over_channel = false;
  expect_gate(input,
              gate::TerminalClassification::production_readout_gate_failure,
              gate::FailureReason::parent_material_gain,
              "quality transport is the final failure stage");
}

void test_stable_names() {
  using Terminal = gate::TerminalClassification;
  const std::pair<Terminal, const char *> terminal_names[] = {
      {Terminal::invalid_mechanics, "invalid_mechanics"},
      {Terminal::parent_evidence_failure, "parent_evidence_failure"},
      {Terminal::backward_compatibility_failure,
       "backward_compatibility_failure"},
      {Terminal::sealed_reference_failure, "sealed_reference_failure"},
      {Terminal::production_shadow_parity_failure,
       "production_shadow_parity_failure"},
      {Terminal::device_translation_failure, "device_translation_failure"},
      {Terminal::production_readout_gate_failure,
       "production_readout_gate_failure"},
      {Terminal::production_structured_readout_parity_reproduced,
       "production_structured_readout_parity_reproduced"},
      {static_cast<Terminal>(999), "invalid_mechanics"},
  };
  for (const auto &[classification, name] : terminal_names) {
    expect(std::string(gate::terminal_classification_name(classification)) ==
               name,
           "terminal classification name changed");
  }

  using Reason = gate::FailureReason;
  const std::pair<Reason, const char *> reason_names[] = {
      {Reason::none, "none"},
      {Reason::invalid_numeric, "invalid_numeric"},
      {Reason::local_contract, "local_contract"},
      {Reason::source_boundary, "source_boundary"},
      {Reason::command, "command"},
      {Reason::environment, "environment"},
      {Reason::cuda_unavailable, "cuda_unavailable"},
      {Reason::attempt_contract, "attempt_contract"},
      {Reason::capture_contract, "capture_contract"},
      {Reason::purity_contract, "purity_contract"},
      {Reason::finite_output_contract, "finite_output_contract"},
      {Reason::deterministic_contract, "deterministic_contract"},
      {Reason::manifest, "manifest"},
      {Reason::audit_input, "audit_input"},
      {Reason::authorization, "authorization"},
      {Reason::nonzero_counter, "nonzero_counter"},
      {Reason::parent_artifact, "parent_artifact"},
      {Reason::parent_hash, "parent_hash"},
      {Reason::parent_classification, "parent_classification"},
      {Reason::parent_attempt_count, "parent_attempt_count"},
      {Reason::parent_audit, "parent_audit"},
      {Reason::parent_counter, "parent_counter"},
      {Reason::parent_authorization, "parent_authorization"},
      {Reason::enum_contract, "enum_contract"},
      {Reason::policy_name, "policy_name"},
      {Reason::parser_contract, "parser_contract"},
      {Reason::default_contract, "default_contract"},
      {Reason::dsl_contract, "dsl_contract"},
      {Reason::fingerprint_contract, "fingerprint_contract"},
      {Reason::checkpoint_contract, "checkpoint_contract"},
      {Reason::legacy_policy_regression, "legacy_policy_regression"},
      {Reason::selector_adapter_contract, "selector_adapter_contract"},
      {Reason::archived_base_custody, "archived_base_custody"},
      {Reason::candidate_delta_custody, "candidate_delta_custody"},
      {Reason::production_shadow_source_boundary,
       "production_shadow_source_boundary"},
      {Reason::projection_contract, "projection_contract"},
      {Reason::layout_contract, "layout_contract"},
      {Reason::shadow_identity, "shadow_identity"},
      {Reason::canonical_reference_identity, "canonical_reference_identity"},
      {Reason::reference_keys, "reference_keys"},
      {Reason::production_shadow_tensor_contract,
       "production_shadow_tensor_contract"},
      {Reason::production_shadow_mask_identity,
       "production_shadow_mask_identity"},
      {Reason::production_shadow_value_identity,
       "production_shadow_value_identity"},
      {Reason::production_shadow_hash_identity,
       "production_shadow_hash_identity"},
      {Reason::production_shadow_repeat_identity,
       "production_shadow_repeat_identity"},
      {Reason::production_shadow_exact_zero, "production_shadow_exact_zero"},
      {Reason::production_shadow_coverage, "production_shadow_coverage"},
      {Reason::cpu64_reference_contract, "cpu64_reference_contract"},
      {Reason::cpu64_reference_exact_zero, "cpu64_reference_exact_zero"},
      {Reason::device_contract, "device_contract"},
      {Reason::device_tolerance, "device_tolerance"},
      {Reason::quality_transport_identity, "quality_transport_identity"},
      {Reason::parent_material_gain, "parent_material_gain"},
      {Reason::parent_noninferiority, "parent_noninferiority"},
      {Reason::parent_order, "parent_order"},
      {Reason::parent_continuous_control, "parent_continuous_control"},
      {Reason::parent_order_control, "parent_order_control"},
      {Reason::parent_terminal, "parent_terminal"},
      {static_cast<Reason>(999), "invalid_numeric"},
  };
  for (const auto &[reason, name] : reason_names) {
    expect(std::string(gate::failure_reason_name(reason)) == name,
           "failure-reason name changed");
  }
}

} // namespace

int main() {
  try {
    test_success_and_fail_closed_default();
    test_mechanics_and_authorization_boundary();
    test_numeric_and_tolerance_boundaries();
    test_parent_evidence();
    test_backward_compatibility();
    test_sealed_reference();
    test_production_shadow_parity_and_full_coverage();
    test_device_reference_contract();
    test_parent_quality_transport_premises();
    test_frozen_terminal_precedence();
    test_stable_names();
    std::cout << "production_structured_readout_parity_gate=PASS\n";
    std::cout << "terminal_precedence=invalid,parent,compatibility,sealed,"
                 "parity,device,quality,success\n";
    std::cout << "coverage=18_retained+18_repeats_exact\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "production_structured_readout_parity_gate=FAIL: "
              << error.what() << '\n';
    return 1;
  }
}
