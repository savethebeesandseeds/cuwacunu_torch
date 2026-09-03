#include "piaabo/digest/sha256.h"
#include "piaabo/log/dlogs.h"
#include "production_structured_readout_parity_gate.h"
#include "structured_readout_shadow.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg_spec.h"

// Reuse the sealed representation-quality data/model/projection machinery in
// this translation unit without editing or linking through its executable
// entry point.  SRR-1 uses this same renamed-main inclusion boundary.
#define main srr2_sealed_quality_parent_main
#include "quality_wikimyei_mtf_jepa_mae_vicreg_representation.cpp"
#undef main

#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <system_error>

#if !defined(__linux__)
#error "SRR-2 durable attempt ledger requires Linux"
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace srr2_gate =
    cuwacunu::tests::production_structured_readout_parity_gate;
namespace srr2_shadow = mtf::structured_readout_shadow;
namespace srr2_digest = cuwacunu::piaabo::digest;

namespace {

constexpr std::string_view kSrr2PreflightExperiment =
    "production-structured-readout-parity-preflight";
constexpr std::string_view kSrr2AuthoritativeExperiment =
    "production-structured-readout-parity";
constexpr std::string_view kSrr2PrerunManifestPath =
    ".build/tests/representation_srr2_v1_prerun.sha256";
constexpr std::string_view kSrr2A2PrerunManifestPath =
    ".build/tests/representation_srr2_v1_prerun_a2.sha256";
constexpr std::string_view kSrr2AttemptLedgerPath =
    ".build/tests/representation_srr2_v1_attempt.lock";
constexpr std::string_view kSrr2AuthoritativeLogPath =
    ".build/tests/representation_srr2_v1_authoritative_a3.log";
constexpr std::string_view kSrr2A2AuthoritativeLogPath =
    ".build/tests/representation_srr2_v1_authoritative.log";
constexpr std::string_view kSrr2BaselineArchivePath =
    ".build/tests/representation_srr2_v1_production_baseline.tar";
constexpr std::string_view kSrr2CandidatePatchPath =
    ".build/tests/representation_srr2_v1_candidate.patch";
constexpr std::string_view kSrr2MechanicsLogPath =
    ".build/tests/representation_srr2_v1_mechanics.log";
constexpr std::string_view kSrr2PreflightLogPath =
    ".build/tests/representation_srr2_v1_preflight.log";
constexpr std::string_view kSrr2ContainerIdentityPath =
    ".build/tests/representation_srr2_v1_container_identity.txt";
constexpr std::string_view kSrr2BuildReceiptPath =
    ".build/tests/representation_srr2_v1_build_receipt.txt";
constexpr std::string_view kSrr2CandidatePatchVerifierPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "verify_production_structured_readout_parity_candidate_patch.sh";
constexpr std::string_view kSrr2BinaryPath =
    ".build/tests/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity";
constexpr std::string_view kSrr2SealedQualityParentSourcePath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_representation.cpp";
constexpr std::string_view kSrr2SealedQualityParentSourceSha256 =
    "14ae77e2fcada70f45c2f14e69e7693db96716199d22fab28134fccb79248a56";
constexpr std::size_t kSrr2SealedQualityParentSourceBytes = 552065;
constexpr std::string_view kSrr2AuthoritativeCommand =
    "docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && "
    "set -o noclobber && CUBLAS_WORKSPACE_CONFIG=:4096:8 "
    "./.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_production_"
    "structured_readout_parity --experiment production-structured-readout-"
    "parity --device cuda > "
    ".build/tests/representation_srr2_v1_authoritative_a3.log 2>&1'";
constexpr std::string_view kSrr2BuildCommand =
    "docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && make -C "
    "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg "
    "srr2-screen'";
constexpr std::string_view kSrr2MechanicsCommand =
    "docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && make -C "
    "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg "
    "run-srr2-mechanics'";
constexpr std::string_view kSrr2PreflightCommand =
    "docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && "
    "CUBLAS_WORKSPACE_CONFIG=:4096:8 "
    "./.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_production_"
    "structured_readout_parity --experiment production-structured-readout-"
    "parity-preflight --device cuda > "
    ".build/tests/representation_srr2_v1_preflight.log 2>&1'";
constexpr std::string_view kSrr2AuditCommand =
    "docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && "
    "./.build/tests/test_production_structured_readout_parity_log_auditor > "
    ".build/tests/representation_srr2_v1_audit.log 2>&1'";
constexpr std::string_view kSrr2ProtocolSha256 =
    "742def90993850ab7ed381e860d60f5adbf1a258c2d9a7de0568bc0067af985e";
constexpr std::size_t kSrr2ProtocolBytes = 20061;
constexpr std::string_view kSrr2ProtocolAmendmentSha256 =
    "03ba84fe2fa318594c2da9812aebda1d9370008e0b37ad24159388e1213c0d73";
constexpr std::size_t kSrr2ProtocolAmendmentBytes = 2313;
constexpr std::string_view kSrr2ProtocolAmendmentA2Sha256 =
    "5cc7e519c25899d309b76df32bd15e5a24cb731a3eafc9f269ea3905eea84f11";
constexpr std::size_t kSrr2ProtocolAmendmentA2Bytes = 5031;
constexpr std::string_view kSrr2ProtocolAmendmentA3Sha256 =
    "0b7cbde36a46bbade2366e5e42fcd7cbb40345766d6c9841178501cc501f6990";
constexpr std::size_t kSrr2ProtocolAmendmentA3Bytes = 10854;
constexpr std::string_view kSrr2ProtocolSidecarSha256 =
    "258ef89c09ef6db995281e1c40c681a537e1fe9f985c88c6644bc285136ff2e0";
constexpr std::size_t kSrr2ProtocolSidecarBytes = 115;
constexpr std::string_view kSrr2ProtocolAmendmentSidecarSha256 =
    "abb2d48fffefac920056b91fa6d2a112e56aa7a87bfbd30aa1840b179d4679a4";
constexpr std::size_t kSrr2ProtocolAmendmentSidecarBytes = 128;
constexpr std::string_view kSrr2ProtocolAmendmentA2SidecarSha256 =
    "1d8091c1b538d63cb15dd90f1f5d7543f57d9685474cd52d867e03a59778981d";
constexpr std::size_t kSrr2ProtocolAmendmentA2SidecarBytes = 128;
constexpr std::string_view kSrr2ProtocolAmendmentA3SidecarSha256 =
    "00410b8d68dc1baa3b689fdf9f1b876fc75d0c923b07b967abba82b2c4390276";
constexpr std::size_t kSrr2ProtocolAmendmentA3SidecarBytes = 128;
constexpr std::string_view kSrr2A2PrerunManifestSha256 =
    "339edec05bd1d5ae686532bf9c44a1c27e3b2e664ea6df1d682059b6157ab613";
constexpr std::size_t kSrr2A2PrerunManifestBytes = 12737;
constexpr std::string_view kSrr2A2AuthoritativeLogSha256 =
    "c60de68c496bd43a73ea7a327264a605eb3d4755df9af97b713ffe0916846768";
constexpr std::size_t kSrr2A2AuthoritativeLogBytes = 647;
constexpr std::string_view kSrr2ContainerIdentitySha256 =
    "18370d88848d6236c14d67ff2322863b7b51925436c940c4067a5db271eb308c";
constexpr std::size_t kSrr2ContainerIdentityBytes = 217;
constexpr std::string_view kSrr2ImageId =
    "sha256:ee37b64a84a5a803ef11061304de62741b41b1f1b9e2a743b1e7686b12029d79";

constexpr std::array<const char *, 6> kSrr2DatasetNames{
    "probe_train",    "probe_validation",    "test",
    "reversed_train", "reversed_validation", "reversed_test"};
constexpr std::array<int64_t, 6> kSrr2DatasetRows{256, 128, 256, 256, 128, 256};
constexpr std::array<int64_t, 6> kSrr2DatasetStarts{1000000, 2000000, 3000000,
                                                    1000000, 2000000, 3000000};
constexpr std::array<int64_t, 24> kSrr2CellVector{
    0, 0, 1, 1, 1, 2,  2,  3,  4,  5,  6,  7,
    8, 8, 9, 9, 9, 10, 10, 11, 12, 13, 14, 15};
static_assert(kModelRowBatchSize == 96,
              "SRR-2 protocol freezes capture batch size 96");

constexpr std::string_view kSrr2AuthorizationTail =
    "training_authorized=false\n"
    "augmentation_change_authorized=false\n"
    "long_run_authorized=false\n"
    "active_policy_change_authorized=false\n"
    "checkpoint_migration_authorized=false\n"
    "downstream_retraining_authorized=false\n"
    "end_to_end_authorized=false\n"
    "deployment_authorized=false\n";
constexpr std::size_t kSrr2ExpectedAuthoritativeKeyCount = 4541;
constexpr std::string_view kSrr2ExpectedAuthoritativeKeysetSha256 =
    "24ab25ff06abb36a6cd59b4bbc8debc0404473b9cbd2227d3d2bb099e5d4d470";

constexpr std::array<std::string_view, 49> kSrr2PreflightCaptureSuffixes{
    "rows",
    "batch_size",
    "value_count",
    "validity_count",
    "shape",
    "values_stride",
    "mask_shape",
    "mask_stride",
    "dtype",
    "device",
    "values_contiguous",
    "mask_contiguous",
    "finite",
    "complete",
    "same_encoded_object",
    "public_selector_sandwich_exact",
    "layout_exact",
    "input_unchanged",
    "parameter_unchanged",
    "buffer_unchanged",
    "cpu_rng_unchanged",
    "cuda_rng_unchanged",
    "model_mode_unchanged",
    "encoder_hash",
    "served_hash",
    "metadata_structure_hash",
    "production_hash",
    "shadow_hash",
    "reference_hash",
    "cpu64_production_hash",
    "cpu64_shadow_hash",
    "production_mask_hash",
    "shadow_mask_hash",
    "reference_mask_hash",
    "production_shadow_value_bytes_exact",
    "production_shadow_mask_bytes_exact",
    "cpu64_production_shadow_value_bytes_exact",
    "cpu64_production_shadow_mask_bytes_exact",
    "cpu64_production_reference_value_bytes_exact",
    "cpu64_production_reference_mask_bytes_exact",
    "cpu64_shadow_reference_value_bytes_exact",
    "cpu64_shadow_reference_mask_bytes_exact",
    "production_shadow_device_max_abs",
    "cpu64_production_shadow_max_abs",
    "cpu64_production_reference_max_abs",
    "cpu64_shadow_reference_max_abs",
    "device_production_reference_max_abs",
    "device_shadow_reference_max_abs",
    "invalid_zero_exact"};

constexpr std::array<std::string_view, 64> kSrr2CapturePrimitiveSuffixes{
    "captured_row_count",
    "production_value_count",
    "shadow_value_count",
    "reference_value_count",
    "cpu64_production_value_count",
    "cpu64_shadow_value_count",
    "production_mask_count",
    "shadow_mask_count",
    "reference_mask_count",
    "cpu64_production_mask_count",
    "cpu64_shadow_mask_count",
    "production_valid_count",
    "shadow_valid_count",
    "reference_valid_count",
    "cpu64_production_valid_count",
    "cpu64_shadow_valid_count",
    "input_data_before_hash",
    "input_data_after_hash",
    "input_mask_before_hash",
    "input_mask_after_hash",
    "target_defined_before",
    "target_defined_after",
    "parameter_before_hash",
    "parameter_after_hash",
    "parameter_max_abs",
    "buffer_before_hash",
    "buffer_after_hash",
    "cpu_rng_before_hash",
    "cpu_rng_after_hash",
    "cuda_rng_before_hash",
    "cuda_rng_after_hash",
    "training_mode_before",
    "training_mode_after",
    "encoded_before_production_hash",
    "encoded_after_production_hash",
    "encoded_before_shadow_hash",
    "encoded_after_shadow_hash",
    "old_selector_before_hash",
    "old_selector_after_hash",
    "token_before_hash",
    "token_after_hash",
    "token_before_mask_hash",
    "token_after_mask_hash",
    "token_before_metadata_hash",
    "token_after_metadata_hash",
    "encoded_token_mask_hash",
    "encoded_metadata_hash",
    "cpu64_production_mask_hash",
    "cpu64_shadow_mask_hash",
    "production_finite",
    "shadow_finite",
    "reference_finite",
    "cpu64_production_finite",
    "cpu64_shadow_finite",
    "production_finite_count",
    "shadow_finite_count",
    "reference_finite_count",
    "cpu64_production_finite_count",
    "cpu64_shadow_finite_count",
    "production_invalid_zero_exact",
    "shadow_invalid_zero_exact",
    "reference_invalid_zero_exact",
    "cpu64_production_invalid_zero_exact",
    "cpu64_shadow_invalid_zero_exact"};

constexpr std::string_view kSrr2AuthoritativeRetainedSuffixes[]{
    "rows",
    "value_count",
    "validity_count",
    "batch_size",
    "group_begin",
    "reversed",
    "shape",
    "values_stride",
    "mask_shape",
    "mask_stride",
    "dtype",
    "device",
    "values_contiguous",
    "mask_contiguous",
    "finite",
    "complete",
    "same_encoded_object",
    "public_selector_sandwich_exact",
    "layout_exact",
    "input_unchanged",
    "parameter_unchanged",
    "buffer_unchanged",
    "cpu_rng_unchanged",
    "cuda_rng_unchanged",
    "model_mode_unchanged",
    "input_data_hash",
    "input_mask_hash",
    "parent_input_data_hash",
    "parent_input_mask_hash",
    "parent_input_hashes_exact",
    "encoder_hash",
    "served_hash",
    "metadata_structure_hash",
    "parent_encoder_hash",
    "parent_served_hash",
    "parent_reference_hash",
    "parent_shadow_hash",
    "production_hash",
    "shadow_hash",
    "reference_hash",
    "cpu64_production_hash",
    "cpu64_shadow_hash",
    "production_mask_hash",
    "shadow_mask_hash",
    "reference_mask_hash",
    "production_shadow_value_bytes_exact",
    "production_shadow_mask_bytes_exact",
    "cpu64_production_shadow_value_bytes_exact",
    "cpu64_production_shadow_mask_bytes_exact",
    "cpu64_production_reference_value_bytes_exact",
    "cpu64_production_reference_mask_bytes_exact",
    "cpu64_shadow_reference_value_bytes_exact",
    "cpu64_shadow_reference_mask_bytes_exact",
    "production_shadow_device_max_abs",
    "cpu64_production_shadow_max_abs",
    "cpu64_production_reference_max_abs",
    "cpu64_shadow_reference_max_abs",
    "device_production_reference_max_abs",
    "device_shadow_reference_max_abs",
    "invalid_zero_exact",
    "parent_source_hashes_exact",
    "parent_reference_hash_exact",
    "parent_shadow_hash_exact"};

constexpr std::string_view kSrr2AuthoritativeRepeatSuffixes[]{
    "repeat_rows",
    "repeat_value_count",
    "repeat_validity_count",
    "repeat_batch_size",
    "repeat_shape",
    "repeat_values_stride",
    "repeat_mask_shape",
    "repeat_mask_stride",
    "repeat_dtype",
    "repeat_device",
    "repeat_values_contiguous",
    "repeat_mask_contiguous",
    "repeat_finite",
    "repeat_complete",
    "repeat_same_encoded_object",
    "repeat_public_selector_sandwich_exact",
    "repeat_layout_exact",
    "repeat_input_unchanged",
    "repeat_parameter_unchanged",
    "repeat_buffer_unchanged",
    "repeat_cpu_rng_unchanged",
    "repeat_cuda_rng_unchanged",
    "repeat_model_mode_unchanged",
    "repeat_encoder_hash",
    "repeat_served_hash",
    "repeat_metadata_structure_hash",
    "repeat_production_hash",
    "repeat_shadow_hash",
    "repeat_reference_hash",
    "repeat_cpu64_production_hash",
    "repeat_cpu64_shadow_hash",
    "repeat_production_mask_hash",
    "repeat_shadow_mask_hash",
    "repeat_reference_mask_hash",
    "repeat_production_shadow_value_bytes_exact",
    "repeat_production_shadow_mask_bytes_exact",
    "repeat_cpu64_production_shadow_value_bytes_exact",
    "repeat_cpu64_production_shadow_mask_bytes_exact",
    "repeat_cpu64_production_reference_value_bytes_exact",
    "repeat_cpu64_production_reference_mask_bytes_exact",
    "repeat_cpu64_shadow_reference_value_bytes_exact",
    "repeat_cpu64_shadow_reference_mask_bytes_exact",
    "repeat_production_shadow_device_max_abs",
    "repeat_cpu64_production_shadow_max_abs",
    "repeat_cpu64_production_reference_max_abs",
    "repeat_cpu64_shadow_reference_max_abs",
    "repeat_device_production_reference_max_abs",
    "repeat_device_shadow_reference_max_abs",
    "repeat_invalid_zero_exact",
    "repeat_identity_exact"};
static_assert(sizeof(kSrr2AuthoritativeRetainedSuffixes) /
                      sizeof(kSrr2AuthoritativeRetainedSuffixes[0]) ==
                  63,
              "SRR-2 retained capture schema is frozen at 63 base keys");
static_assert(sizeof(kSrr2AuthoritativeRepeatSuffixes) /
                      sizeof(kSrr2AuthoritativeRepeatSuffixes[0]) ==
                  50,
              "SRR-2 repeat capture schema is frozen at 50 base keys");

[[nodiscard]] std::set<std::string> srr2_expected_authoritative_keys() {
  std::set<std::string> keys{"schema",
                             "experiment",
                             "device",
                             "dtype",
                             "authoritative_command",
                             "srr2.prerun_manifest.bytes",
                             "srr2.prerun_manifest.sha256",
                             "srr2.prerun_manifest.entry_count",
                             "srr2.prerun_manifest.entries_exact",
                             "srr2.prerun_manifest.exact",
                             "srr2.attempt.ledger_path",
                             "srr2.attempt.ledger_bytes",
                             "srr2.attempt.ledger_sha256",
                             "srr2.attempt.ledger_exclusive_create",
                             "srr2.attempt.ledger_durable",
                             "srr2.attempt.ledger_content_exact",
                             "srr2.attempt.consumed",
                             "authoritative_attempt_count",
                             "failure_reason",
                             "terminal_result"};
  const auto add = [&](std::string_view prefix,
                       std::initializer_list<std::string_view> fields) {
    for (const auto field : fields) {
      keys.emplace(std::string(prefix) + std::string(field));
    }
  };
  add("srr2.environment.",
      {"device", "dtype", "cpu_threads", "cpu_interop_threads",
       "deterministic_algorithms", "deterministic_warn_only",
       "deterministic_cudnn", "tf32_cublas_disabled", "tf32_cudnn_disabled",
       "cublas_workspace_exact", "cuda_available"});
  add("srr2.projection.", {"q0_hash", "qpsm_hash", "orthogonality_error",
                           "contrast_mean_error", "block_sum_error"});
  add("srr2.layout.", {"hash", "cell_vector", "exact"});
  add("srr2.data.normalizer.",
      {"group_begin", "rows", "data_hash", "mask_hash", "mean_hash",
       "inv_std_hash", "parent_data_hash", "parent_mask_hash",
       "parent_mean_hash", "parent_inv_std_hash", "exact"});
  add("srr2.coverage.",
      {"seed_count", "batch_size", "dataset_count", "retained_capture_count",
       "repeat_capture_count", "retained_row_count", "repeat_row_count",
       "retained_value_count", "repeat_value_count", "retained_validity_count",
       "repeat_validity_count", "counts_recomputed_from_records", "exact"});
  add("",
      {"training_step_count", "optimizer_construction_count",
       "optimizer_step_count", "backward_call_count", "weight_update_count",
       "augmentation_change_count", "target_generation_count",
       "probe_construction_count", "probe_fit_count",
       "validation_selection_count", "prediction_count", "permutation_count",
       "bootstrap_count", "downstream_retraining_count", "end_to_end_count",
       "deployment_count"});
  add("srr2.mechanics.",
      {"local_contracts_exact", "source_boundary_exact", "command_exact",
       "environment_exact", "cuda_available", "attempt_marker_exact",
       "capture_contracts_exact", "purity_exact", "finite_outputs_exact",
       "deterministic_execution_exact", "manifest_exact", "audit_input_exact",
       "authoritative_attempt_count"});
  add("srr2.parent.",
      {"artifacts_exact", "hashes_exact", "terminal_classification_exact",
       "audit_pass", "authorizations_false", "authoritative_attempt_count",
       "audit_error_count", "optimizer_step_count", "backward_call_count"});
  add("srr2.compatibility.",
      {"legacy_enum_ordinals_exact", "legacy_policy_names_exact",
       "structured_policy_appended", "structured_policy_name_exact",
       "parser_round_trip_exact", "unknown_policy_rejected",
       "cpp_default_all_tokens", "omitted_dsl_all_tokens",
       "active_dsl_all_tokens", "protocol_fingerprint_distinct",
       "structured_checkpoint_round_trip_exact", "legacy_checkpoint_all_tokens",
       "legacy_checkpoint_does_not_inherit_structured",
       "checkpoint_mismatch_rejected", "malformed_checkpoint_rejected",
       "legacy_policy_bytes_exact", "public_selector_contract_exact",
       "adapter_reaches_structured_selector"});
  add("srr2.sealed_reference.",
      {"archived_base_custody_exact", "candidate_delta_custody_exact",
       "production_shadow_source_independent", "q0_identity_exact",
       "qpsm_identity_exact", "projection_invariants_exact",
       "layout_and_metadata_exact", "canonical_plan_exact",
       "parent_shadow_identities_exact", "canonical_reference_identity_exact",
       "all_reference_keys_exact"});
  add("srr2.summary.parity.", {"shape_exact",
                               "strides_and_contiguity_exact",
                               "dtype_exact",
                               "device_exact",
                               "valid_mask_bytes_exact",
                               "value_bytes_exact",
                               "cpu64_valid_mask_bytes_exact",
                               "cpu64_value_bytes_exact",
                               "stable_hashes_exact",
                               "repeat_capture_identity_exact",
                               "per_capture_coverage_exact",
                               "coverage_counts_recomputed_from_records",
                               "cpu64_max_abs",
                               "device_max_abs",
                               "seed_count",
                               "dataset_count",
                               "retained_capture_count",
                               "repeat_capture_count",
                               "retained_row_count",
                               "repeat_row_count",
                               "retained_value_count",
                               "repeat_value_count",
                               "retained_validity_count",
                               "repeat_validity_count"});
  add("srr2.summary.device_translation.",
      {"cpu64_reference_shape_exact", "cpu64_reference_mask_bytes_exact",
       "cpu64_production_reference_bytes_exact",
       "cpu64_shadow_reference_bytes_exact", "device_reference_contract_exact",
       "cpu64_production_reference_max_abs", "cpu64_shadow_reference_max_abs",
       "device_production_reference_max_abs",
       "device_shadow_reference_max_abs"});
  add("srr2.quality_transport.",
      {"features_and_masks_cover_parent_domain", "targets_exact",
       "group_splits_exact", "sample_ladder_exact", "alpha_grid_exact",
       "standardization_exact", "target_centering_exact",
       "fit_and_validation_selection_exact", "test_rows_exact",
       "permutations_exact", "bootstrap_rows_exact",
       "decision_thresholds_exact", "parent_material_gain_over_channel",
       "parent_noninferior_to_encoder", "parent_order_decodable",
       "parent_continuous_shuffle_pass", "parent_order_shuffle_pass",
       "parent_terminal_reproduced"});
  add("srr2.gate.",
      {"numeric_inputs_valid", "authorization_boundary_valid",
       "zero_counters_valid", "mechanics_valid", "parent_evidence_valid",
       "backward_compatibility_valid", "sealed_reference_valid",
       "coverage_valid", "production_shadow_parity_valid",
       "cpu64_reference_valid", "device_translation_valid",
       "production_readout_gate_valid", "failure_reason", "classification"});
  add("",
      {"training_authorized", "augmentation_change_authorized",
       "long_run_authorized", "active_policy_change_authorized",
       "checkpoint_migration_authorized", "downstream_retraining_authorized",
       "end_to_end_authorized", "deployment_authorized"});

  for (const int64_t seed : std::array<int64_t, 3>{17, 31, 47}) {
    for (const auto dataset : kSrr2DatasetNames) {
      const std::string prefix =
          "srr2.seed_" + std::to_string(seed) + ".capture." + dataset + ".";
      for (const auto suffix : kSrr2AuthoritativeRetainedSuffixes) {
        keys.emplace(prefix + std::string(suffix));
      }
      for (const auto suffix : kSrr2CapturePrimitiveSuffixes) {
        keys.emplace(prefix + std::string(suffix));
      }
      for (const auto suffix : kSrr2AuthoritativeRepeatSuffixes) {
        keys.emplace(prefix + std::string(suffix));
      }
      for (const auto suffix : kSrr2CapturePrimitiveSuffixes) {
        keys.emplace(prefix + "repeat_" + std::string(suffix));
      }
    }
  }
  return keys;
}

[[nodiscard]] std::string
srr2_keyset_sha256(const std::set<std::string> &keys) {
  std::string material;
  for (const auto &key : keys) {
    material += key;
    material.push_back('\n');
  }
  return srr2_digest::sha256_hex(material);
}

struct Srr2Options {
  bool preflight{false};
  std::string experiment{};
};

[[nodiscard]] Srr2Options srr2_parse_options(int argc, char **argv) {
  if (argc != 5 || std::string_view(argv[1]) != "--experiment" ||
      std::string_view(argv[3]) != "--device" ||
      std::string_view(argv[4]) != "cuda") {
    throw std::runtime_error(
        "expected exactly --experiment <SRR-2 mode> --device cuda");
  }
  const std::string experiment = argv[2];
  if (experiment == kSrr2PreflightExperiment) {
    return {.preflight = true, .experiment = experiment};
  }
  if (experiment == kSrr2AuthoritativeExperiment) {
    return {.preflight = false, .experiment = experiment};
  }
  throw std::runtime_error("unknown SRR-2 experiment mode: " + experiment);
}

[[nodiscard]] std::string
srr2_read_binary_file(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) {
    throw std::runtime_error("cannot open SRR-2 evidence file: " +
                             path.string());
  }
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

[[nodiscard]] std::string srr2_hex64(uint64_t value) {
  std::ostringstream out;
  out << std::hex << std::nouppercase << std::setw(16) << std::setfill('0')
      << value;
  return out.str();
}

void srr2_emit_hash(const std::string &key, uint64_t value) {
  std::cout << key << '=' << srr2_hex64(value) << '\n';
}

[[nodiscard]] bool srr2_all_zero_bytes(const torch::Tensor &values,
                                       const torch::Tensor &valid_mask) {
  const auto invalid = valid_mask.logical_not().unsqueeze(-1).expand_as(values);
  if (!invalid.any().item<bool>()) {
    return true;
  }
  const auto selected = values.masked_select(invalid).contiguous();
  return rssm_tensor_bytes_equal(selected, torch::zeros_like(selected));
}

[[nodiscard]] double srr2_max_abs(const torch::Tensor &left,
                                  const torch::Tensor &right) {
  if (!left.defined() || !right.defined() || left.sizes() != right.sizes()) {
    return std::numeric_limits<double>::infinity();
  }
  if (left.numel() == 0) {
    return 0.0;
  }
  return (left.detach().to(torch::kCPU, torch::kFloat64) -
          right.detach().to(torch::kCPU, torch::kFloat64))
      .abs()
      .max()
      .item<double>();
}

struct Srr2ScalarRecords {
  std::map<std::string, std::string> values{};
  std::size_t duplicate_count{0};
  std::size_t malformed_count{0};

  [[nodiscard]] std::optional<std::string> maybe(const std::string &key) const {
    const auto found = values.find(key);
    if (found == values.end()) {
      return std::nullopt;
    }
    return found->second;
  }

  [[nodiscard]] std::string require(const std::string &key) const {
    const auto value = maybe(key);
    if (!value.has_value()) {
      throw std::runtime_error("missing authenticated SRR-1 key: " + key);
    }
    return *value;
  }
};

[[nodiscard]] Srr2ScalarRecords
srr2_parse_scalar_records(const std::string &text) {
  Srr2ScalarRecords result{};
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    // The sealed parent contains large prediction CSV rows.  SRR-2 only
    // consumes finite scalar/hash facts and deliberately ignores payload rows.
    if (line.empty() || line.size() > 512 || line.front() == '#' ||
        line.front() == '[') {
      continue;
    }
    const auto separator = line.find('=');
    if (separator == std::string::npos || separator == 0 ||
        separator + 1 >= line.size()) {
      ++result.malformed_count;
      continue;
    }
    const std::string key = line.substr(0, separator);
    const std::string value = line.substr(separator + 1);
    if (!result.values.emplace(key, value).second) {
      ++result.duplicate_count;
    }
  }
  return result;
}

struct Srr2ArtifactExpectation {
  const char *path;
  std::uintmax_t bytes;
  const char *sha256;
};

constexpr std::array<Srr2ArtifactExpectation, 8> kSrr2ParentArtifacts{{
    {"src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
     "vicreg/STRUCTURED_READOUT_REPAIR_PLAN.md",
     14893, "1f976d5da5a79323a8fce011b0b33e53b277517bd785b2fdea68aa1888338127"},
    {"src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
     "vicreg/STRUCTURED_READOUT_REPAIR_PROTOCOL.md",
     21848, "ad7c9381d58a23e8f3cec27b59b44e6532aa561227ad22d57578cc6ba0a04946"},
    {"src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
     "vicreg/STRUCTURED_READOUT_REPAIR_PROTOCOL.sha256",
     104, "3b97b7431e34c3b875365bad27d0b9de67a5b7fd7007760eb34ab97b125140c5"},
    {"src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
     "vicreg/STRUCTURED_READOUT_REPAIR_FINDINGS.md",
     10393, "b5b3458953f2f967a0229ea910c80810303d2e709bd6d9f237966c0e6b456c6a"},
    {".build/tests/representation_srr_v1_prerun.sha256", 7166,
     "515c9c8a851b3aceb03c160e5c9c19fff5265774d51eb396c6d56123cf0d3acb"},
    {".build/tests/representation_srr_v1_authoritative.log", 7324951,
     "f38c99ef1294dab5f40f57fff79a958cd214c593eedd10284531976cda20ae6a"},
    {".build/tests/representation_srr_v1_audit.log", 2964,
     "fe943fb2aa8ad26f53953364181f7c2b452692fde17643c5a8d94ca45c9bb841"},
    {".build/tests/representation_srr_v1_receipt.sha256", 3517,
     "994be46cab5c4bbabf3b72ed30e5fa1a8ece9247722e16ae504b428dcd0fc207"},
}};

struct Srr2ParentReceipt {
  srr2_gate::ParentEvidenceInput gate{};
  bool byte_lengths_exact{false};
  bool artifact_hashes_exact{false};
  bool scalar_schema_exact{false};
  Srr2ScalarRecords authoritative{};
  Srr2ScalarRecords audit{};
};

[[nodiscard]] bool srr2_text_contains(const std::string &text,
                                      std::string_view needle) {
  return text.find(needle) != std::string::npos;
}

[[nodiscard]] Srr2ParentReceipt srr2_verify_parent_evidence() {
  Srr2ParentReceipt result{};
  result.byte_lengths_exact = true;
  result.artifact_hashes_exact = true;
  for (const auto &expected : kSrr2ParentArtifacts) {
    const std::filesystem::path path(expected.path);
    if (!std::filesystem::exists(path)) {
      result.byte_lengths_exact = false;
      result.artifact_hashes_exact = false;
      continue;
    }
    const auto contents = srr2_read_binary_file(path);
    result.byte_lengths_exact =
        result.byte_lengths_exact && contents.size() == expected.bytes;
    result.artifact_hashes_exact =
        result.artifact_hashes_exact &&
        srr2_digest::sha256_hex(contents) == expected.sha256;
  }

  const auto authoritative_text = srr2_read_binary_file(
      ".build/tests/representation_srr_v1_authoritative.log");
  const auto audit_text =
      srr2_read_binary_file(".build/tests/representation_srr_v1_audit.log");
  const auto receipt_text = srr2_read_binary_file(
      ".build/tests/representation_srr_v1_receipt.sha256");
  result.authoritative = srr2_parse_scalar_records(authoritative_text);
  result.audit = srr2_parse_scalar_records(audit_text);
  result.scalar_schema_exact = result.authoritative.duplicate_count == 0 &&
                               result.authoritative.malformed_count == 0 &&
                               result.audit.duplicate_count == 0 &&
                               result.audit.malformed_count == 0;

  result.gate.artifacts_exact = result.byte_lengths_exact;
  result.gate.hashes_exact = result.artifact_hashes_exact;
  result.gate.terminal_classification_exact =
      result.audit.maybe("audit.final_classification") ==
      std::optional<std::string>{"structured_readout_reproduced"};
  result.gate.audit_pass =
      result.audit.maybe("audit.pass") == std::optional<std::string>{"true"};
  result.gate.authorizations_false =
      result.audit.maybe("audit.parent_authorizations_false") ==
          std::optional<std::string>{"true"} &&
      srr2_text_contains(receipt_text, "# audit_pass=true\n") &&
      srr2_text_contains(receipt_text, "# audit_error_count=0\n");
  result.gate.authoritative_attempt_count =
      result.audit.maybe("audit.authoritative_attempt_count") ==
              std::optional<std::string>{"1"}
          ? 1
          : 0;
  result.gate.audit_error_count =
      result.audit.maybe("audit.error_count") == std::optional<std::string>{"0"}
          ? 0
          : 1;
  result.gate.optimizer_step_count =
      result.authoritative.maybe("optimizer_steps") ==
              std::optional<std::string>{"0"}
          ? 0
          : 1;
  result.gate.backward_call_count =
      result.authoritative.maybe("backward_calls") ==
              std::optional<std::string>{"0"}
          ? 0
          : 1;
  return result;
}

[[nodiscard]] bool srr2_parent_receipt_exact(const Srr2ParentReceipt &parent) {
  return parent.gate.artifacts_exact && parent.gate.hashes_exact &&
         parent.gate.terminal_classification_exact && parent.gate.audit_pass &&
         parent.gate.authorizations_false &&
         parent.gate.authoritative_attempt_count == 1 &&
         parent.gate.audit_error_count == 0 &&
         parent.gate.optimizer_step_count == 0 &&
         parent.gate.backward_call_count == 0 && parent.scalar_schema_exact;
}

struct Srr2ManifestReceipt {
  std::size_t bytes{0};
  std::string sha256{};
  std::uintmax_t executing_binary_bytes{0};
  std::string executing_binary_sha256{};
  std::string container_id{};
  std::size_t entry_count{0};
  bool nonempty{false};
  bool entries_exact{false};
  bool path_contract_exact{false};
  bool metadata_exact{false};
  bool mandatory_entries_exact{false};
  bool attempt_ledger_preseal_absent{false};
  bool attempt_ledger_excluded{false};
  bool authoritative_log_preseal_contract_exact{false};
  bool runtime_identity_exact{false};
  bool executable_self_bound{false};
  bool sealed_quality_parent_source_bound{false};
  bool protocol_bound{false};
  bool protocol_amendment_bound{false};
  bool protocol_amendment_a2_bound{false};
  bool protocol_amendment_a3_bound{false};
  bool a2_incident_bound{false};
  bool baseline_bound{false};
  bool container_identity_bound{false};
  bool build_receipt_bound{false};
  bool command_exact{false};
  bool projection_facts_exact{false};
  bool token_layout_bound{false};
  bool executable_bound{false};
  bool mechanics_bound{false};
  bool preflight_bound{false};
  bool candidate_patch_bound{false};
  Srr2ScalarRecords mechanics_records{};
  Srr2ScalarRecords preflight_records{};
  bool exact{false};
};

struct Srr2ManifestEntry {
  std::string sha256{};
  std::uintmax_t bytes{0};
};

[[nodiscard]] bool srr2_lower_hex_exact(std::string_view value,
                                        std::size_t size) {
  return value.size() == size &&
         std::all_of(value.begin(), value.end(), [](char character) {
           return (character >= '0' && character <= '9') ||
                  (character >= 'a' && character <= 'f');
         });
}

[[nodiscard]] bool srr2_parse_canonical_u64(std::string_view text,
                                            std::uint64_t &value) {
  if (text.empty() || (text.size() > 1 && text.front() == '0')) {
    return false;
  }
  const auto parsed =
      std::from_chars(text.data(), text.data() + text.size(), value);
  return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

[[nodiscard]] bool srr2_parse_finite_double(std::string_view text,
                                            double &value) {
  const auto parsed =
      std::from_chars(text.data(), text.data() + text.size(), value);
  return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size() &&
         std::isfinite(value) && value >= 0.0;
}

[[nodiscard]] bool srr2_receipt_key_valid(std::string_view key) {
  return !key.empty() &&
         std::all_of(key.begin(), key.end(), [](char character) {
           return (character >= 'a' && character <= 'z') ||
                  (character >= 'A' && character <= 'Z') ||
                  (character >= '0' && character <= '9') || character == '_' ||
                  character == '.';
         });
}

struct Srr2ClosedRecords {
  Srr2ScalarRecords records{};
  std::size_t initializing_diagnostic_count{0};
  std::size_t finalizing_diagnostic_count{0};
  bool exact_syntax{true};
};

[[nodiscard]] Srr2ClosedRecords
srr2_parse_closed_records(const std::string &text,
                          bool allow_source_runtime_diagnostics) {
  Srr2ClosedRecords result{};
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      result.exact_syntax = false;
      continue;
    }
    if (line.find("[source_runtime_t] initializing static-global source "
                  "snapshot") != std::string::npos) {
      ++result.initializing_diagnostic_count;
      result.exact_syntax = result.exact_syntax &&
                            allow_source_runtime_diagnostics &&
                            line.find('=') == std::string::npos;
      continue;
    }
    if (line.find("[source_runtime_t] finalizing static-global source "
                  "snapshot") != std::string::npos) {
      ++result.finalizing_diagnostic_count;
      result.exact_syntax =
          result.exact_syntax && allow_source_runtime_diagnostics;
      continue;
    }
    if (line.empty()) {
      result.exact_syntax = false;
      continue;
    }
    const auto separator = line.find('=');
    if (separator == std::string::npos || separator == 0 ||
        separator + 1 >= line.size() ||
        !srr2_receipt_key_valid(std::string_view(line).substr(0, separator))) {
      ++result.records.malformed_count;
      result.exact_syntax = false;
      continue;
    }
    if (!result.records.values
             .emplace(line.substr(0, separator), line.substr(separator + 1))
             .second) {
      ++result.records.duplicate_count;
      result.exact_syntax = false;
    }
  }
  return result;
}

[[nodiscard]] bool
srr2_authorization_tail_exact(const std::string &raw_receipt) {
  return raw_receipt.size() >= kSrr2AuthorizationTail.size() &&
         std::string_view(raw_receipt)
                 .substr(raw_receipt.size() - kSrr2AuthorizationTail.size()) ==
             kSrr2AuthorizationTail;
}

[[nodiscard]] bool
srr2_mechanics_receipt_exact(const Srr2ClosedRecords &receipt) {
  std::map<std::string, std::string> expected{
      {"schema", "wikimyei.mtf_jepa_mae_vicreg.srr2_mechanics.v1"},
      {"experiment", "production-structured-readout-mechanics"},
      {"srr2.mechanics.training_or_augmentation_used", "false"},
      {"srr2.mechanics.local_contracts_exact", "true"},
      {"srr2.mechanics.pass", "true"}};
  const std::array<std::pair<std::string_view, std::string_view>, 6> commands{{
      {"production", "CUBLAS_WORKSPACE_CONFIG=:4096:8 "
                     "./.build/tests/test_production_structured_readout"},
      {"shadow", "./.build/tests/test_structured_readout_shadow"},
      {"gate", "./.build/tests/test_production_structured_readout_parity_gate"},
      {"auditor",
       "./.build/tests/test_production_structured_readout_parity_log_auditor "
       "--self-test"},
      {"config", "./.build/tests/test_wikimyei_graph_first_specs"},
      {"adapter", "./.build/tests/test_jkimyei_channel_graph_first_launchers"},
  }};
  for (const auto &[label, command] : commands) {
    const std::string prefix = "srr2.mechanics." + std::string(label);
    expected.emplace(prefix + ".command", command);
    expected.emplace(prefix + ".exit_code", "0");
    expected.emplace(prefix + ".pass_marker", "true");
  }
  constexpr std::array<std::string_view, 18> compatibility_fields{
      "legacy_enum_ordinals_exact",
      "legacy_policy_names_exact",
      "structured_policy_appended",
      "structured_policy_name_exact",
      "parser_round_trip_exact",
      "unknown_policy_rejected",
      "cpp_default_all_tokens",
      "omitted_dsl_all_tokens",
      "active_dsl_all_tokens",
      "protocol_fingerprint_distinct",
      "structured_checkpoint_round_trip_exact",
      "legacy_checkpoint_all_tokens",
      "legacy_checkpoint_does_not_inherit_structured",
      "checkpoint_mismatch_rejected",
      "malformed_checkpoint_rejected",
      "legacy_policy_bytes_exact",
      "public_selector_contract_exact",
      "adapter_reaches_structured_selector"};
  for (const auto field : compatibility_fields) {
    expected.emplace("srr2.compatibility." + std::string(field), "true");
  }
  return receipt.exact_syntax && receipt.records.duplicate_count == 0 &&
         receipt.records.malformed_count == 0 &&
         receipt.initializing_diagnostic_count == 0 &&
         receipt.finalizing_diagnostic_count == 0 &&
         receipt.records.values == expected;
}

[[nodiscard]] bool
srr2_preflight_receipt_exact(const Srr2ClosedRecords &receipt,
                             const std::string &raw_receipt,
                             uint64_t token_layout_hash) {
  std::set<std::string> expected_keys{
      "schema",
      "experiment",
      "device",
      "dtype",
      "srr2.preflight.manifest_read",
      "srr2.preflight.normalizer_group_begin",
      "srr2.preflight.normalizer_count",
      "srr2.preflight.capture_group_begin",
      "srr2.preflight.capture_count",
      "srr2.preflight.batch_size",
      "srr2.preflight.seed",
      "srr2.preflight.target_constructed",
      "srr2.preflight.compatibility.parsed_device_type",
      "srr2.preflight.compatibility.parsed_device_index",
      "srr2.preflight.compatibility.active_alias_device",
      "srr2.preflight.compatibility.structured_alias_device",
      "srr2.preflight.compatibility.active_alias_policy",
      "srr2.preflight.compatibility.structured_alias_policy",
      "srr2.preflight.compatibility.active_manifest_hash",
      "srr2.preflight.compatibility.structured_manifest_hash",
      "srr2.preflight.compatibility.active_nonpolicy_manifest_hash",
      "srr2.preflight.compatibility.structured_nonpolicy_manifest_hash",
      "srr2.preflight.compatibility.receipt_fact_count",
      "srr2.preflight.compatibility.setup_complete",
      "srr2.preflight.expected_authoritative_key_count",
      "srr2.preflight.expected_authoritative_keyset_sha256",
      "srr2.environment.device",
      "srr2.environment.dtype",
      "srr2.environment.cpu_threads",
      "srr2.environment.cpu_interop_threads",
      "srr2.environment.deterministic_algorithms",
      "srr2.environment.deterministic_warn_only",
      "srr2.environment.deterministic_cudnn",
      "srr2.environment.tf32_cublas_disabled",
      "srr2.environment.tf32_cudnn_disabled",
      "srr2.environment.cublas_workspace_exact",
      "srr2.environment.cuda_available",
      "srr2.projection.q0_hash",
      "srr2.projection.qpsm_hash",
      "srr2.projection.orthogonality_error",
      "srr2.projection.contrast_mean_error",
      "srr2.projection.block_sum_error",
      "srr2.layout.hash",
      "srr2.layout.cell_vector",
      "srr2.layout.exact",
      "srr2.preflight.complete",
      "srr2.preflight.repeat_complete",
      "srr2.preflight.same_encoded_object",
      "srr2.preflight.public_selector_sandwich_exact",
      "srr2.preflight.layout_exact",
      "srr2.preflight.production_shadow_value_bytes_exact",
      "srr2.preflight.production_shadow_mask_bytes_exact",
      "srr2.preflight.cpu64_production_reference_bytes_exact",
      "srr2.preflight.cpu64_shadow_reference_bytes_exact",
      "srr2.preflight.production_shadow_device_max_abs",
      "srr2.preflight.cpu64_production_reference_max_abs",
      "srr2.preflight.cpu64_shadow_reference_max_abs",
      "srr2.preflight.device_production_reference_max_abs",
      "srr2.preflight.device_shadow_reference_max_abs",
      "srr2.preflight.input_unchanged",
      "srr2.preflight.parameter_unchanged",
      "srr2.preflight.buffer_unchanged",
      "srr2.preflight.cpu_rng_unchanged",
      "srr2.preflight.cuda_rng_unchanged",
      "srr2.preflight.model_mode_unchanged",
      "srr2.preflight.repeat_identity_exact",
      "srr2.preflight.parent_evidence_exact",
      "srr2.preflight.pass",
      "srr2.attempt.consumed",
      "authoritative_attempt_count"};
  constexpr std::array<std::string_view, 18> compatibility_fields{
      "legacy_enum_ordinals_exact",
      "legacy_policy_names_exact",
      "structured_policy_appended",
      "structured_policy_name_exact",
      "parser_round_trip_exact",
      "unknown_policy_rejected",
      "cpp_default_all_tokens",
      "omitted_dsl_all_tokens",
      "active_dsl_all_tokens",
      "protocol_fingerprint_distinct",
      "structured_checkpoint_round_trip_exact",
      "legacy_checkpoint_all_tokens",
      "legacy_checkpoint_does_not_inherit_structured",
      "checkpoint_mismatch_rejected",
      "malformed_checkpoint_rejected",
      "legacy_policy_bytes_exact",
      "public_selector_contract_exact",
      "adapter_reaches_structured_selector"};
  for (const auto field : compatibility_fields) {
    expected_keys.emplace("srr2.compatibility." + std::string(field));
  }
  for (const auto prefix :
       {"srr2.preflight.retained.", "srr2.preflight.repeat."}) {
    for (const auto suffix : kSrr2PreflightCaptureSuffixes) {
      expected_keys.emplace(std::string(prefix) + std::string(suffix));
    }
    for (const auto suffix : kSrr2CapturePrimitiveSuffixes) {
      expected_keys.emplace(std::string(prefix) + std::string(suffix));
    }
  }
  constexpr std::array<std::string_view, 16> counters{
      "training_step_count",     "optimizer_construction_count",
      "optimizer_step_count",    "backward_call_count",
      "weight_update_count",     "augmentation_change_count",
      "target_generation_count", "probe_construction_count",
      "probe_fit_count",         "validation_selection_count",
      "prediction_count",        "permutation_count",
      "bootstrap_count",         "downstream_retraining_count",
      "end_to_end_count",        "deployment_count"};
  constexpr std::array<std::string_view, 8> authorizations{
      "training_authorized",
      "augmentation_change_authorized",
      "long_run_authorized",
      "active_policy_change_authorized",
      "checkpoint_migration_authorized",
      "downstream_retraining_authorized",
      "end_to_end_authorized",
      "deployment_authorized"};
  for (const auto key : counters) {
    expected_keys.emplace(key);
  }
  for (const auto key : authorizations) {
    expected_keys.emplace(key);
  }
  std::set<std::string> observed_keys;
  for (const auto &[key, value] : receipt.records.values) {
    (void)value;
    observed_keys.emplace(key);
  }
  if (!receipt.exact_syntax || receipt.records.duplicate_count != 0 ||
      receipt.records.malformed_count != 0 ||
      receipt.initializing_diagnostic_count != 1 ||
      receipt.finalizing_diagnostic_count != 0 ||
      !srr2_authorization_tail_exact(raw_receipt) ||
      observed_keys != expected_keys) {
    return false;
  }
  const auto is = [&](std::string_view key, std::string_view value) {
    const auto found = receipt.records.values.find(std::string(key));
    return found != receipt.records.values.end() && found->second == value;
  };
  bool exact =
      is("schema", "wikimyei.mtf_jepa_mae_vicreg.srr2_preflight.v2") &&
      is("experiment", kSrr2PreflightExperiment) && is("device", "cuda:0") &&
      is("dtype", "float32") && is("srr2.preflight.manifest_read", "false") &&
      is("srr2.preflight.normalizer_group_begin", "4700000") &&
      is("srr2.preflight.normalizer_count", "32") &&
      is("srr2.preflight.capture_group_begin", "4800000") &&
      is("srr2.preflight.capture_count", "101") &&
      is("srr2.preflight.batch_size", "96") &&
      is("srr2.preflight.seed", "17") &&
      is("srr2.preflight.target_constructed", "false") &&
      is("srr2.preflight.compatibility.parsed_device_type", "cuda") &&
      (is("srr2.preflight.compatibility.parsed_device_index", "-1") ||
       is("srr2.preflight.compatibility.parsed_device_index", "0")) &&
      is("srr2.preflight.compatibility.active_alias_device", "cuda:0") &&
      is("srr2.preflight.compatibility.structured_alias_device", "cuda:0") &&
      is("srr2.preflight.compatibility.active_alias_policy", "all_tokens") &&
      is("srr2.preflight.compatibility.structured_alias_policy",
         "structured_cdsb_v1") &&
      is("srr2.preflight.compatibility.receipt_fact_count", "18") &&
      is("srr2.preflight.compatibility.setup_complete", "true") &&
      is("srr2.preflight.expected_authoritative_key_count",
         std::to_string(kSrr2ExpectedAuthoritativeKeyCount)) &&
      is("srr2.preflight.expected_authoritative_keyset_sha256",
         kSrr2ExpectedAuthoritativeKeysetSha256) &&
      is("srr2.environment.device", "cuda:0") &&
      is("srr2.environment.dtype", "float32") &&
      is("srr2.environment.cpu_threads", "1") &&
      is("srr2.environment.cpu_interop_threads", "1") &&
      is("srr2.environment.deterministic_algorithms", "true") &&
      is("srr2.environment.deterministic_warn_only", "false") &&
      is("srr2.environment.deterministic_cudnn", "true") &&
      is("srr2.environment.tf32_cublas_disabled", "true") &&
      is("srr2.environment.tf32_cudnn_disabled", "true") &&
      is("srr2.environment.cublas_workspace_exact", "true") &&
      is("srr2.environment.cuda_available", "true") &&
      is("srr2.projection.q0_hash", "f8c9f35282de2ee0") &&
      is("srr2.projection.qpsm_hash", "ac8a43fd65b2c8a8") &&
      is("srr2.layout.hash", srr2_hex64(token_layout_hash)) &&
      is("srr2.layout.cell_vector",
         "0,0,1,1,1,2,2,3,4,5,6,7,8,8,9,9,9,10,10,11,12,13,14,15") &&
      is("srr2.layout.exact", "true") &&
      is("srr2.preflight.complete", "true") &&
      is("srr2.preflight.repeat_complete", "true") &&
      is("srr2.preflight.same_encoded_object", "true") &&
      is("srr2.preflight.public_selector_sandwich_exact", "true") &&
      is("srr2.preflight.layout_exact", "true") &&
      is("srr2.preflight.production_shadow_value_bytes_exact", "true") &&
      is("srr2.preflight.production_shadow_mask_bytes_exact", "true") &&
      is("srr2.preflight.cpu64_production_reference_bytes_exact", "true") &&
      is("srr2.preflight.cpu64_shadow_reference_bytes_exact", "true") &&
      is("srr2.preflight.input_unchanged", "true") &&
      is("srr2.preflight.parameter_unchanged", "true") &&
      is("srr2.preflight.buffer_unchanged", "true") &&
      is("srr2.preflight.cpu_rng_unchanged", "true") &&
      is("srr2.preflight.cuda_rng_unchanged", "true") &&
      is("srr2.preflight.model_mode_unchanged", "true") &&
      is("srr2.preflight.repeat_identity_exact", "true") &&
      is("srr2.preflight.parent_evidence_exact", "true") &&
      is("srr2.preflight.pass", "true") &&
      is("srr2.attempt.consumed", "false") &&
      is("authoritative_attempt_count", "0");
  for (const auto field : compatibility_fields) {
    exact = exact && is("srr2.compatibility." + std::string(field), "true");
  }
  const auto active_manifest = receipt.records.maybe(
      "srr2.preflight.compatibility.active_manifest_hash");
  const auto structured_manifest = receipt.records.maybe(
      "srr2.preflight.compatibility.structured_manifest_hash");
  const auto active_nonpolicy = receipt.records.maybe(
      "srr2.preflight.compatibility.active_nonpolicy_manifest_hash");
  const auto structured_nonpolicy = receipt.records.maybe(
      "srr2.preflight.compatibility.structured_nonpolicy_manifest_hash");
  exact = exact && active_manifest.has_value() &&
          structured_manifest.has_value() && active_nonpolicy.has_value() &&
          structured_nonpolicy.has_value() &&
          srr2_lower_hex_exact(*active_manifest, 16) &&
          srr2_lower_hex_exact(*structured_manifest, 16) &&
          srr2_lower_hex_exact(*active_nonpolicy, 16) &&
          srr2_lower_hex_exact(*structured_nonpolicy, 16) &&
          *active_manifest != *structured_manifest &&
          *active_manifest == *active_nonpolicy &&
          *active_nonpolicy == *structured_nonpolicy;
  for (const auto key : counters) {
    exact = exact && is(key, "0");
  }
  for (const auto key : authorizations) {
    exact = exact && is(key, "false");
  }
  for (const auto key : {"srr2.projection.orthogonality_error",
                         "srr2.projection.contrast_mean_error",
                         "srr2.projection.block_sum_error"}) {
    double value = 0.0;
    const auto found = receipt.records.values.find(key);
    exact = exact && found != receipt.records.values.end() &&
            srr2_parse_finite_double(found->second, value) && value <= 1.0e-10;
  }
  for (const auto key : {"srr2.preflight.production_shadow_device_max_abs",
                         "srr2.preflight.cpu64_production_reference_max_abs",
                         "srr2.preflight.cpu64_shadow_reference_max_abs"}) {
    double value = 0.0;
    const auto found = receipt.records.values.find(key);
    exact = exact && found != receipt.records.values.end() &&
            srr2_parse_finite_double(found->second, value) && value == 0.0;
  }
  for (const auto key : {"srr2.preflight.device_production_reference_max_abs",
                         "srr2.preflight.device_shadow_reference_max_abs"}) {
    double value = 0.0;
    const auto found = receipt.records.values.find(key);
    exact = exact && found != receipt.records.values.end() &&
            srr2_parse_finite_double(found->second, value) &&
            value <= srr2_gate::kDeviceTranslationTolerance;
  }
  const auto capture_exact = [&](std::string_view prefix) {
    const auto key = [&](std::string_view suffix) {
      return std::string(prefix) + std::string(suffix);
    };
    const auto capture_is = [&](std::string_view suffix,
                                std::string_view expected) {
      return is(key(suffix), expected);
    };
    const auto same = [&](std::string_view left, std::string_view right) {
      const auto left_value = receipt.records.maybe(key(left));
      const auto right_value = receipt.records.maybe(key(right));
      return left_value.has_value() && left_value == right_value;
    };
    const auto unsigned_is = [&](std::string_view suffix,
                                 std::uint64_t expected) {
      const auto value = receipt.records.maybe(key(suffix));
      std::uint64_t parsed = 0;
      return value.has_value() && srr2_parse_canonical_u64(*value, parsed) &&
             parsed == expected;
    };
    const auto maximum_is = [&](std::string_view suffix, double maximum,
                                bool exact_zero) {
      const auto value = receipt.records.maybe(key(suffix));
      double parsed = 0.0;
      return value.has_value() && srr2_parse_finite_double(*value, parsed) &&
             (exact_zero ? parsed == 0.0 : parsed <= maximum);
    };
    bool capture =
        capture_is("rows", "101") && capture_is("batch_size", "96") &&
        capture_is("value_count", "9696") &&
        capture_is("validity_count", "303") &&
        capture_is("shape", "101,3,32") &&
        capture_is("values_stride", "96,32,1") &&
        capture_is("mask_shape", "101,3") && capture_is("mask_stride", "3,1") &&
        capture_is("dtype", "float32") && capture_is("device", "cuda:0") &&
        capture_is("values_contiguous", "true") &&
        capture_is("mask_contiguous", "true") && capture_is("finite", "true") &&
        capture_is("complete", "true") &&
        capture_is("same_encoded_object", "true") &&
        capture_is("public_selector_sandwich_exact", "true") &&
        capture_is("layout_exact", "true") &&
        capture_is("input_unchanged", "true") &&
        capture_is("parameter_unchanged", "true") &&
        capture_is("buffer_unchanged", "true") &&
        capture_is("cpu_rng_unchanged", "true") &&
        capture_is("cuda_rng_unchanged", "true") &&
        capture_is("model_mode_unchanged", "true") &&
        capture_is("production_shadow_value_bytes_exact", "true") &&
        capture_is("production_shadow_mask_bytes_exact", "true") &&
        capture_is("cpu64_production_shadow_value_bytes_exact", "true") &&
        capture_is("cpu64_production_shadow_mask_bytes_exact", "true") &&
        capture_is("cpu64_production_reference_value_bytes_exact", "true") &&
        capture_is("cpu64_production_reference_mask_bytes_exact", "true") &&
        capture_is("cpu64_shadow_reference_value_bytes_exact", "true") &&
        capture_is("cpu64_shadow_reference_mask_bytes_exact", "true") &&
        capture_is("invalid_zero_exact", "true") &&
        unsigned_is("captured_row_count", 101) &&
        unsigned_is("production_value_count", 9696) &&
        unsigned_is("shadow_value_count", 9696) &&
        unsigned_is("reference_value_count", 9696) &&
        unsigned_is("cpu64_production_value_count", 9696) &&
        unsigned_is("cpu64_shadow_value_count", 9696) &&
        unsigned_is("production_mask_count", 303) &&
        unsigned_is("shadow_mask_count", 303) &&
        unsigned_is("reference_mask_count", 303) &&
        unsigned_is("cpu64_production_mask_count", 303) &&
        unsigned_is("cpu64_shadow_mask_count", 303) &&
        unsigned_is("production_valid_count", 303) &&
        unsigned_is("shadow_valid_count", 303) &&
        unsigned_is("reference_valid_count", 303) &&
        unsigned_is("cpu64_production_valid_count", 303) &&
        unsigned_is("cpu64_shadow_valid_count", 303) &&
        capture_is("target_defined_before", "false") &&
        capture_is("target_defined_after", "false") &&
        capture_is("training_mode_before", "false") &&
        capture_is("training_mode_after", "false") &&
        capture_is("production_finite", "true") &&
        capture_is("shadow_finite", "true") &&
        capture_is("reference_finite", "true") &&
        capture_is("cpu64_production_finite", "true") &&
        capture_is("cpu64_shadow_finite", "true") &&
        unsigned_is("production_finite_count", 9696) &&
        unsigned_is("shadow_finite_count", 9696) &&
        unsigned_is("reference_finite_count", 9696) &&
        unsigned_is("cpu64_production_finite_count", 9696) &&
        unsigned_is("cpu64_shadow_finite_count", 9696) &&
        capture_is("production_invalid_zero_exact", "true") &&
        capture_is("shadow_invalid_zero_exact", "true") &&
        capture_is("reference_invalid_zero_exact", "true") &&
        capture_is("cpu64_production_invalid_zero_exact", "true") &&
        capture_is("cpu64_shadow_invalid_zero_exact", "true") &&
        maximum_is("parameter_max_abs", 0.0, true) &&
        maximum_is("production_shadow_device_max_abs", 0.0, true) &&
        maximum_is("cpu64_production_shadow_max_abs", 0.0, true) &&
        maximum_is("cpu64_production_reference_max_abs", 0.0, true) &&
        maximum_is("cpu64_shadow_reference_max_abs", 0.0, true) &&
        maximum_is("device_production_reference_max_abs",
                   srr2_gate::kDeviceTranslationTolerance, false) &&
        maximum_is("device_shadow_reference_max_abs",
                   srr2_gate::kDeviceTranslationTolerance, false) &&
        same("production_hash", "shadow_hash") &&
        same("production_mask_hash", "shadow_mask_hash") &&
        same("cpu64_production_hash", "cpu64_shadow_hash") &&
        same("cpu64_production_hash", "reference_hash") &&
        same("cpu64_production_mask_hash", "cpu64_shadow_mask_hash") &&
        same("cpu64_production_mask_hash", "reference_mask_hash") &&
        same("input_data_before_hash", "input_data_after_hash") &&
        same("input_mask_before_hash", "input_mask_after_hash") &&
        same("parameter_before_hash", "parameter_after_hash") &&
        same("buffer_before_hash", "buffer_after_hash") &&
        same("cpu_rng_before_hash", "cpu_rng_after_hash") &&
        same("cuda_rng_before_hash", "cuda_rng_after_hash") &&
        same("encoded_before_production_hash",
             "encoded_after_production_hash") &&
        same("encoded_after_production_hash", "encoded_before_shadow_hash") &&
        same("encoded_before_shadow_hash", "encoded_after_shadow_hash") &&
        same("old_selector_before_hash", "old_selector_after_hash") &&
        same("token_before_hash", "token_after_hash") &&
        same("token_before_mask_hash", "token_after_mask_hash") &&
        same("token_before_mask_hash", "encoded_token_mask_hash") &&
        same("token_before_metadata_hash", "token_after_metadata_hash") &&
        same("token_before_metadata_hash", "encoded_metadata_hash");
    for (const auto suffix : kSrr2PreflightCaptureSuffixes) {
      if (suffix.ends_with("_hash")) {
        const auto value = receipt.records.maybe(key(suffix));
        capture =
            capture && value.has_value() && srr2_lower_hex_exact(*value, 16);
      }
    }
    for (const auto suffix : kSrr2CapturePrimitiveSuffixes) {
      if (suffix.ends_with("_hash")) {
        const auto value = receipt.records.maybe(key(suffix));
        capture =
            capture && value.has_value() && srr2_lower_hex_exact(*value, 16);
      }
    }
    return capture;
  };
  exact = exact && capture_exact("srr2.preflight.retained.") &&
          capture_exact("srr2.preflight.repeat.");
  for (const auto suffix :
       {"encoder_hash", "served_hash", "metadata_structure_hash",
        "production_hash", "shadow_hash", "reference_hash",
        "cpu64_production_hash", "cpu64_shadow_hash", "production_mask_hash",
        "shadow_mask_hash", "reference_mask_hash", "cpu64_production_mask_hash",
        "cpu64_shadow_mask_hash"}) {
    exact = exact && receipt.records.maybe("srr2.preflight.retained." +
                                           std::string(suffix)) ==
                         receipt.records.maybe("srr2.preflight.repeat." +
                                               std::string(suffix));
  }
  return exact;
}

[[nodiscard]] bool srr2_safe_manifest_path(const std::filesystem::path &path) {
  if (path.empty() || path.is_absolute()) {
    return false;
  }
  for (const auto &part : path) {
    if (part == "..") {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool
srr2_path_is_within(const std::filesystem::path &canonical_root,
                    const std::filesystem::path &canonical_target) {
  const auto relative = canonical_target.lexically_relative(canonical_root);
  if (relative.empty() || relative.is_absolute()) {
    return false;
  }
  return std::none_of(relative.begin(), relative.end(), [](const auto &part) {
    return part == "..";
  });
}

[[nodiscard]] bool srr2_attempt_ledger_present_fail_closed() {
  struct stat status {};
  errno = 0;
  if (::lstat(std::string(kSrr2AttemptLedgerPath).c_str(), &status) == 0) {
    return true;
  }
  return errno != ENOENT;
}

struct Srr2AttemptLedgerReceipt {
  std::string path{};
  std::size_t bytes{0};
  std::string sha256{};
  bool exclusive_create{false};
  bool durable{false};
  bool content_exact{false};
};

[[nodiscard]] std::string
srr2_attempt_ledger_content(const Srr2ManifestReceipt &manifest) {
  return "schema=wikimyei.mtf_jepa_mae_vicreg.srr2_attempt_ledger.v1\n"
         "experiment=production-structured-readout-parity\n"
         "attempt_count=1\n"
         "state=consumed\n"
         "authoritative_command_sha256=" +
         srr2_digest::sha256_hex(std::string(kSrr2AuthoritativeCommand)) +
         "\nprerun_manifest_bytes=" + std::to_string(manifest.bytes) +
         "\nprerun_manifest_sha256=" + manifest.sha256 +
         "\nexecuting_binary_bytes=" +
         std::to_string(manifest.executing_binary_bytes) +
         "\nexecuting_binary_sha256=" + manifest.executing_binary_sha256 +
         "\ncontainer_id=" + manifest.container_id + "\n";
}

[[nodiscard]] bool srr2_fsync_retry(int descriptor) {
  while (::fsync(descriptor) != 0) {
    if (errno != EINTR) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] Srr2AttemptLedgerReceipt
srr2_consume_authoritative_attempt(const Srr2ManifestReceipt &manifest) {
  if (!manifest.exact || manifest.executing_binary_bytes == 0 ||
      !srr2_lower_hex_exact(manifest.executing_binary_sha256, 64) ||
      !srr2_lower_hex_exact(manifest.container_id, 64)) {
    throw std::runtime_error(
        "SRR-2 attempt ledger received incomplete sealed inputs");
  }

  const std::string path(kSrr2AttemptLedgerPath);
  const std::string content = srr2_attempt_ledger_content(manifest);
  Srr2AttemptLedgerReceipt result{};
  result.path = path;
  result.bytes = content.size();
  result.sha256 = srr2_digest::sha256_hex(content);

  errno = 0;
  const int descriptor =
      ::open(path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0444);
  if (descriptor < 0) {
    throw std::system_error(errno, std::generic_category(),
                            "SRR-2 exclusive attempt ledger creation failed");
  }
  result.exclusive_create = true;

  int first_error = 0;
  const auto remember_error = [&](int code) {
    if (first_error == 0) {
      first_error = code == 0 ? EIO : code;
    }
  };

  errno = 0;
  if (::fchmod(descriptor, 0444) != 0) {
    remember_error(errno);
  }
  std::size_t written = 0;
  while (written < content.size()) {
    errno = 0;
    const ssize_t count =
        ::write(descriptor, content.data() + written, content.size() - written);
    if (count > 0) {
      written += static_cast<std::size_t>(count);
      continue;
    }
    if (count < 0 && errno == EINTR) {
      continue;
    }
    remember_error(count == 0 ? EIO : errno);
    break;
  }
  if (written != content.size()) {
    remember_error(EIO);
  }

  errno = 0;
  const bool file_synced = srr2_fsync_retry(descriptor);
  if (!file_synced) {
    remember_error(errno);
  }
  errno = 0;
  const bool file_closed = ::close(descriptor) == 0;
  if (!file_closed) {
    remember_error(errno);
  }

  const std::string parent =
      std::filesystem::path(path).parent_path().generic_string();
  errno = 0;
  const int parent_descriptor = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY);
  bool parent_synced = false;
  bool parent_closed = false;
  if (parent_descriptor < 0) {
    remember_error(errno);
  } else {
    errno = 0;
    parent_synced = srr2_fsync_retry(parent_descriptor);
    if (!parent_synced) {
      remember_error(errno);
    }
    errno = 0;
    parent_closed = ::close(parent_descriptor) == 0;
    if (!parent_closed) {
      remember_error(errno);
    }
  }

  struct stat ledger_status {};
  errno = 0;
  const bool mode_exact =
      ::lstat(path.c_str(), &ledger_status) == 0 &&
      S_ISREG(ledger_status.st_mode) && ledger_status.st_nlink == 1 &&
      (ledger_status.st_mode & static_cast<mode_t>(0777)) == 0444;
  if (!mode_exact) {
    remember_error(errno);
  }
  result.durable = written == content.size() && file_synced && file_closed &&
                   parent_synced && parent_closed && mode_exact &&
                   first_error == 0;
  if (!result.durable) {
    throw std::system_error(
        first_error == 0 ? EIO : first_error, std::generic_category(),
        "SRR-2 attempt ledger durability failed; ledger retained");
  }

  const std::string live_content = srr2_read_binary_file(path);
  result.content_exact = live_content == content &&
                         live_content.size() == result.bytes &&
                         srr2_digest::sha256_hex(live_content) == result.sha256;
  if (!result.content_exact) {
    throw std::runtime_error(
        "SRR-2 attempt ledger content verification failed; ledger retained");
  }
  return result;
}

void srr2_emit_attempt_ledger(const Srr2AttemptLedgerReceipt &ledger) {
  std::cout << "srr2.attempt.ledger_path=" << ledger.path << '\n';
  std::cout << "srr2.attempt.ledger_bytes=" << ledger.bytes << '\n';
  std::cout << "srr2.attempt.ledger_sha256=" << ledger.sha256 << '\n';
  std::cout << "srr2.attempt.ledger_exclusive_create="
            << ledger.exclusive_create << '\n';
  std::cout << "srr2.attempt.ledger_durable=" << ledger.durable << '\n';
  std::cout << "srr2.attempt.ledger_content_exact=" << ledger.content_exact
            << '\n';
}

[[nodiscard]] bool
srr2_attempt_ledger_receipt_exact(const Srr2AttemptLedgerReceipt &ledger,
                                  const Srr2ManifestReceipt &manifest) {
  const std::string expected = srr2_attempt_ledger_content(manifest);
  struct stat status {};
  if (ledger.path != std::string(kSrr2AttemptLedgerPath) ||
      ledger.bytes != expected.size() ||
      ledger.sha256 != srr2_digest::sha256_hex(expected) ||
      !ledger.exclusive_create || !ledger.durable || !ledger.content_exact ||
      ::lstat(ledger.path.c_str(), &status) != 0 || !S_ISREG(status.st_mode) ||
      status.st_nlink != 1 ||
      (status.st_mode & static_cast<mode_t>(0777)) != 0444) {
    return false;
  }
  try {
    return srr2_read_binary_file(ledger.path) == expected;
  } catch (const std::exception &) {
    return false;
  }
}

[[nodiscard]] std::set<std::string> srr2_mandatory_manifest_paths() {
  return {
      ".build/tests/representation_srr2_v1_production_baseline.tar",
      ".build/tests/representation_srr2_v1_candidate.patch",
      ".build/tests/representation_srr2_v1_mechanics.log",
      ".build/tests/representation_srr2_v1_preflight.log",
      ".build/tests/representation_srr2_v1_prerun_a2.sha256",
      ".build/tests/representation_srr2_v1_authoritative.log",
      ".build/tests/representation_srr2_v1_container_identity.txt",
      ".build/tests/representation_srr2_v1_build_receipt.txt",
      ".build/tests/test_wikimyei_mtf_jepa_mae_vicreg",
      ".build/tests/test_wikimyei_mtf_jepa_mae_vicreg_contracts",
      ".build/tests/test_structured_readout_shadow",
      ".build/tests/test_production_structured_readout",
      ".build/tests/test_production_structured_readout_parity_gate",
      ".build/tests/test_production_structured_readout_parity_log_auditor",
      ".build/tests/test_wikimyei_graph_first_specs",
      ".build/tests/test_jkimyei_channel_graph_first_launchers",
      ".build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_production_"
      "structured_readout_parity",
      ".build/tests/representation_srr_v1_prerun.sha256",
      ".build/tests/representation_srr_v1_authoritative.log",
      ".build/tests/representation_srr_v1_audit.log",
      ".build/tests/representation_srr_v1_receipt.sha256",
      "src/config/README.md",
      "src/config/grammar/wikimyei.representation.mtf_jepa_mae_vicreg.dsl."
      "bnf",
      "src/config/man/wikimyei.config.man",
      "src/config/wikimyei.representation.mtf_jepa_mae_vicreg.dsl",
      "src/config/wikimyei.representation.mtf_jepa_mae_vicreg.net",
      "src/include/jkimyei/training/inference/"
      "channel_graph_first_inference_launcher.h",
      "src/include/kikijyeba/protocol/config_bundle.h",
      "src/include/wikimyei/README.md",
      "src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/"
      "README.md",
      "src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/"
      "mtf_jepa_mae_vicreg.h",
      "src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/"
      "mtf_jepa_mae_vicreg_spec.h",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/STRUCTURED_READOUT_REPAIR_PLAN.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/STRUCTURED_READOUT_REPAIR_PROTOCOL.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/STRUCTURED_READOUT_REPAIR_PROTOCOL.sha256",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/STRUCTURED_READOUT_REPAIR_FINDINGS.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PLAN.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL."
      "sha256",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A1.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A1.sha256",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A2.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A2.sha256",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A3.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A3.sha256",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/production_structured_readout_parity_gate.h",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/structured_readout_shadow.h",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/test_structured_readout_shadow.cpp",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/test_production_structured_readout.cpp",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/test_production_structured_readout_parity_gate.cpp",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "test_production_structured_readout_parity_log_auditor.cpp",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/quality_wikimyei_mtf_jepa_mae_vicreg_"
      "production_structured_readout_parity.cpp",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "quality_wikimyei_mtf_jepa_mae_vicreg_representation.cpp",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/Makefile",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "run_production_structured_readout_parity_mechanics.sh",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "write_production_structured_readout_parity_build_receipt.sh",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "verify_production_structured_readout_parity_candidate_patch.sh",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/test_wikimyei_mtf_jepa_mae_vicreg.cpp",
      "src/tests/bench/wikimyei/config/graph_first_specs/"
      "test_wikimyei_graph_first_specs.cpp",
      "src/tests/bench/jkimyei/training/channel_graph_first_launchers/"
      "test_jkimyei_channel_graph_first_launchers.cpp"};
}

[[nodiscard]] std::pair<Srr2ClosedRecords, bool>
srr2_candidate_patch_verifier_receipt() {
  const std::string command = "bash " +
                              std::string(kSrr2CandidatePatchVerifierPath) +
                              " --verify 2>/dev/null";
  std::FILE *pipe = ::popen(command.c_str(), "r");
  if (pipe == nullptr) {
    return {{}, false};
  }
  std::string output;
  std::array<char, 4096> buffer{};
  while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) !=
         nullptr) {
    output.append(buffer.data());
  }
  const int status = ::pclose(pipe);
  return {srr2_parse_closed_records(output, false), status == 0};
}

[[nodiscard]] Srr2ManifestReceipt
srr2_verify_prerun_manifest(const std::filesystem::path &path,
                            uint64_t token_layout_hash) {
  const auto canonical_root =
      std::filesystem::canonical(std::filesystem::current_path());
  const auto manifest_status = std::filesystem::symlink_status(path);
  if (!std::filesystem::is_regular_file(manifest_status) ||
      std::filesystem::is_symlink(manifest_status) ||
      std::filesystem::hard_link_count(path) != 1 ||
      !srr2_path_is_within(canonical_root, std::filesystem::canonical(path))) {
    throw std::runtime_error(
        "SRR-2 pre-run manifest is not a contained single-link regular file");
  }
  const std::string text = srr2_read_binary_file(path);
  Srr2ManifestReceipt result{};
  result.bytes = text.size();
  result.sha256 = srr2_digest::sha256_hex(text);
  result.nonempty = !text.empty();
  result.entries_exact = true;
  result.path_contract_exact = true;
  std::map<std::string, std::string> metadata;
  std::map<std::string, Srr2ManifestEntry> entries;
  std::string previous_entry_path;
  bool entry_rows_started = false;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || (!line.empty() && line.back() == '\r')) {
      result.entries_exact = false;
      result.path_contract_exact = false;
      continue;
    }
    if (line.rfind("# ", 0) == 0) {
      if (entry_rows_started) {
        result.entries_exact = false;
      }
      const auto separator = line.find('=', 2);
      if (separator == std::string::npos || separator == 2 ||
          !srr2_receipt_key_valid(
              std::string_view(line).substr(2, separator - 2)) ||
          !metadata
               .emplace(line.substr(2, separator - 2),
                        line.substr(separator + 1))
               .second) {
        result.entries_exact = false;
      }
      continue;
    }
    entry_rows_started = true;
    if (line.size() < 70 || !srr2_lower_hex_exact(line.substr(0, 64), 64) ||
        line.substr(64, 2) != "  ") {
      result.entries_exact = false;
      continue;
    }
    const auto size_end = line.find("  ", 66);
    if (size_end == std::string::npos) {
      result.entries_exact = false;
      continue;
    }
    std::uint64_t expected_bytes = 0;
    const std::string_view size_text(line.data() + 66, size_end - 66);
    const std::string relative = line.substr(size_end + 2);
    const std::filesystem::path entry_path(relative);
    if (!srr2_parse_canonical_u64(size_text, expected_bytes) ||
        expected_bytes == 0 || relative.find('\\') != std::string::npos ||
        entry_path.generic_string() != relative ||
        (!previous_entry_path.empty() && previous_entry_path >= relative) ||
        !entries
             .emplace(relative,
                      Srr2ManifestEntry{line.substr(0, 64), expected_bytes})
             .second) {
      result.entries_exact = false;
      result.path_contract_exact = false;
      continue;
    }
    previous_entry_path = relative;
    result.path_contract_exact =
        result.path_contract_exact && srr2_safe_manifest_path(entry_path);
    bool live_path_exact = false;
    try {
      const auto status = std::filesystem::symlink_status(entry_path);
      live_path_exact =
          std::filesystem::is_regular_file(status) &&
          !std::filesystem::is_symlink(status) &&
          srr2_path_is_within(canonical_root,
                              std::filesystem::canonical(entry_path));
    } catch (const std::exception &) {
      live_path_exact = false;
    }
    if (!srr2_safe_manifest_path(entry_path) || !live_path_exact) {
      result.entries_exact = false;
      result.path_contract_exact = false;
      continue;
    }
    const auto entry = srr2_read_binary_file(entry_path);
    result.entries_exact = result.entries_exact &&
                           entry.size() == expected_bytes &&
                           srr2_digest::sha256_hex(entry) == line.substr(0, 64);
    ++result.entry_count;
  }

  const auto metadata_is = [&](std::string_view key,
                               std::string_view expected) {
    const auto found = metadata.find(std::string(key));
    return found != metadata.end() && found->second == expected;
  };
  result.metadata_exact =
      metadata.size() == 73 &&
      metadata_is("schema",
                  "wikimyei.mtf_jepa_mae_vicreg.srr2_prerun_manifest.v1") &&
      metadata_is("manifest_format",
                  "sha256_two_spaces_bytes_two_spaces_path_v1") &&
      metadata_is("canonical_entry_order", "lexicographic") &&
      metadata_is("protocol_sha256", kSrr2ProtocolSha256) &&
      metadata_is("protocol_amendment_sha256", kSrr2ProtocolAmendmentSha256) &&
      metadata_is("protocol_amendment_bytes",
                  std::to_string(kSrr2ProtocolAmendmentBytes)) &&
      metadata_is("protocol_amendment_a2_sha256",
                  kSrr2ProtocolAmendmentA2Sha256) &&
      metadata_is("protocol_amendment_a2_bytes",
                  std::to_string(kSrr2ProtocolAmendmentA2Bytes)) &&
      metadata_is("protocol_amendment_a3_sha256",
                  kSrr2ProtocolAmendmentA3Sha256) &&
      metadata_is("protocol_amendment_a3_bytes",
                  std::to_string(kSrr2ProtocolAmendmentA3Bytes)) &&
      metadata_is("a2_prerun_manifest_path", kSrr2A2PrerunManifestPath) &&
      metadata_is("a2_prerun_manifest_sha256",
                  kSrr2A2PrerunManifestSha256) &&
      metadata_is("a2_prerun_manifest_bytes",
                  std::to_string(kSrr2A2PrerunManifestBytes)) &&
      metadata_is("a2_reserved_authoritative_log_path",
                  kSrr2A2AuthoritativeLogPath) &&
      metadata_is("a2_reserved_authoritative_log_sha256",
                  kSrr2A2AuthoritativeLogSha256) &&
      metadata_is("a2_reserved_authoritative_log_bytes",
                  std::to_string(kSrr2A2AuthoritativeLogBytes)) &&
      metadata_is("a2_reserved_authoritative_log_attempt_consumed",
                  "false") &&
      metadata_is("a2_reserved_authoritative_log_attempt_count", "0") &&
      metadata_is("a2_reserved_authoritative_log_terminal_result",
                  "invalid_mechanics") &&
      metadata_is("a2_reserved_attempt_ledger_absent", "true") &&
      metadata_is("scientific_attempt_limit", "1") &&
      metadata_is("attempt_ledger_path", kSrr2AttemptLedgerPath) &&
      metadata_is("attempt_ledger_preseal_absent", "true") &&
      metadata_is("authoritative_log_preseal_absent", "true") &&
      metadata_is("preflight_pass", "true") &&
      metadata_is("scientific_rows_used_before_seal", "false") &&
      metadata_is("projection_q0_hash", "f8c9f35282de2ee0") &&
      metadata_is("projection_qpsm_hash", "ac8a43fd65b2c8a8") &&
      metadata_is("production_baseline_bytes", "829440") &&
      metadata_is(
          "production_baseline_sha256",
          "22f53f452836583f5402d1a28d67c9b3a9ac865094444a06b9726fd0f1c7b6dd") &&
      metadata_is("token_layout_hash", srr2_hex64(token_layout_hash)) &&
      metadata_is("authoritative_command", kSrr2AuthoritativeCommand) &&
      metadata_is("build_command", kSrr2BuildCommand) &&
      metadata_is("mechanics_command", kSrr2MechanicsCommand) &&
      metadata_is("preflight_command", kSrr2PreflightCommand) &&
      metadata_is("audit_command", kSrr2AuditCommand) &&
      metadata_is("working_directory", "/cuwacunu") &&
      metadata_is("container_name", "unnamed_taoist") &&
      metadata_is("image_id", kSrr2ImageId) &&
      metadata_is("environment_device", "cuda:0") &&
      metadata_is("environment_dtype", "float32") &&
      metadata_is("cpu_threads", "1") &&
      metadata_is("cpu_interop_threads", "1") &&
      metadata_is("deterministic_algorithms", "true") &&
      metadata_is("deterministic_warn_only", "false") &&
      metadata_is("deterministic_cudnn", "true") &&
      metadata_is("tf32_cublas_disabled", "true") &&
      metadata_is("tf32_cudnn_disabled", "true") &&
      metadata_is("cublas_workspace_config", ":4096:8") &&
      metadata_is("seed_vector", "17,31,47") &&
      metadata_is("dataset_vector",
                  "probe_train,probe_validation,test,reversed_train,reversed_"
                  "validation,reversed_test") &&
      metadata_is("dataset_row_vector", "256,128,256,256,128,256") &&
      metadata_is("dataset_group_begin_vector",
                  "1000000,2000000,3000000,1000000,2000000,3000000") &&
      metadata_is("dataset_reversed_vector",
                  "false,false,false,true,true,true") &&
      metadata_is("cell_vector",
                  "0,0,1,1,1,2,2,3,4,5,6,7,8,8,9,9,9,10,10,11,12,13,14,15") &&
      metadata_is("expected_seed_count", "3") &&
      metadata_is("expected_dataset_count", "6") &&
      metadata_is("expected_retained_capture_count", "18") &&
      metadata_is("expected_repeat_capture_count", "18") &&
      metadata_is("expected_retained_row_count", "3840") &&
      metadata_is("expected_repeat_row_count", "3840") &&
      metadata_is("expected_retained_value_count", "368640") &&
      metadata_is("expected_repeat_value_count", "368640") &&
      metadata_is("expected_retained_validity_count", "11520") &&
      metadata_is("expected_repeat_validity_count", "11520") &&
      metadata_is("candidate_patch_path", kSrr2CandidatePatchPath) &&
      metadata_is("baseline_path", kSrr2BaselineArchivePath) &&
      metadata_is("mechanics_log_path", kSrr2MechanicsLogPath) &&
      metadata_is("preflight_log_path", kSrr2PreflightLogPath) &&
      metadata_is("authoritative_log_path", kSrr2AuthoritativeLogPath);
  const auto key_count = metadata.find("expected_authoritative_key_count");
  const auto keyset_hash =
      metadata.find("expected_authoritative_keyset_sha256");
  const auto expected_authoritative_keys = srr2_expected_authoritative_keys();
  const auto computed_authoritative_keyset_sha256 =
      srr2_keyset_sha256(expected_authoritative_keys);
  const bool authoritative_schema_frozen =
      expected_authoritative_keys.size() ==
          kSrr2ExpectedAuthoritativeKeyCount &&
      computed_authoritative_keyset_sha256 ==
          kSrr2ExpectedAuthoritativeKeysetSha256;
  std::uint64_t parsed_key_count = 0;
  result.metadata_exact =
      result.metadata_exact && key_count != metadata.end() &&
      srr2_parse_canonical_u64(key_count->second, parsed_key_count) &&
      parsed_key_count == kSrr2ExpectedAuthoritativeKeyCount &&
      key_count->second == std::to_string(kSrr2ExpectedAuthoritativeKeyCount) &&
      keyset_hash != metadata.end() &&
      keyset_hash->second == kSrr2ExpectedAuthoritativeKeysetSha256 &&
      authoritative_schema_frozen;

  const auto container_id = metadata.find("container_id");
  if (container_id != metadata.end()) {
    result.container_id = container_id->second;
  }
  std::string hostname = srr2_read_binary_file("/etc/hostname");
  while (!hostname.empty() &&
         (hostname.back() == '\n' || hostname.back() == '\r')) {
    hostname.pop_back();
  }
  result.runtime_identity_exact =
      std::filesystem::current_path().generic_string() == "/cuwacunu" &&
      container_id != metadata.end() &&
      srr2_lower_hex_exact(container_id->second, 64) && !hostname.empty() &&
      hostname.size() <= container_id->second.size() &&
      container_id->second.substr(0, hostname.size()) == hostname;
  result.metadata_exact =
      result.metadata_exact && result.runtime_identity_exact;

  result.attempt_ledger_preseal_absent =
      !srr2_attempt_ledger_present_fail_closed();
  result.attempt_ledger_excluded =
      entries.count(std::string(kSrr2AttemptLedgerPath)) == 0;
  result.authoritative_log_preseal_contract_exact =
      metadata_is("authoritative_log_preseal_absent", "true") &&
      entries.count(std::string(kSrr2AuthoritativeLogPath)) == 0;

  const auto mandatory = srr2_mandatory_manifest_paths();
  result.mandatory_entries_exact =
      entries.size() == mandatory.size() &&
      result.entry_count == mandatory.size() &&
      std::all_of(mandatory.begin(), mandatory.end(),
                  [&](const std::string &required) {
                    return entries.count(required) == 1;
                  });
  const auto bound = [&](std::string_view relative, std::uintmax_t bytes,
                         std::string_view sha256) {
    const auto found = entries.find(std::string(relative));
    return found != entries.end() && found->second.bytes == bytes &&
           found->second.sha256 == sha256;
  };
  result.protocol_bound =
      bound("src/tests/bench/wikimyei/representation/encoding/"
            "mtf_jepa_mae_vicreg/"
            "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL.md",
            kSrr2ProtocolBytes, kSrr2ProtocolSha256) &&
      bound("src/tests/bench/wikimyei/representation/"
            "encoding/mtf_jepa_mae_vicreg/"
            "PRODUCTION_STRUCTURED_READOUT_PARITY_"
            "PROTOCOL.sha256",
            kSrr2ProtocolSidecarBytes, kSrr2ProtocolSidecarSha256);
  result.protocol_amendment_bound =
      bound("src/tests/bench/wikimyei/representation/encoding/"
            "mtf_jepa_mae_vicreg/"
            "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A1.md",
            kSrr2ProtocolAmendmentBytes, kSrr2ProtocolAmendmentSha256) &&
      bound("src/tests/bench/wikimyei/"
            "representation/encoding/"
            "mtf_jepa_mae_vicreg/"
            "PRODUCTION_STRUCTURED_READOUT_"
            "PARITY_PROTOCOL_AMENDMENT_A1."
            "sha256",
            kSrr2ProtocolAmendmentSidecarBytes,
            kSrr2ProtocolAmendmentSidecarSha256);
  result.protocol_amendment_a2_bound =
      bound("src/tests/bench/wikimyei/representation/encoding/"
            "mtf_jepa_mae_vicreg/"
            "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A2.md",
            kSrr2ProtocolAmendmentA2Bytes,
            kSrr2ProtocolAmendmentA2Sha256) &&
      bound("src/tests/bench/wikimyei/"
            "representation/encoding/"
            "mtf_jepa_mae_vicreg/"
            "PRODUCTION_STRUCTURED_READOUT_"
            "PARITY_PROTOCOL_AMENDMENT_A2."
            "sha256",
            kSrr2ProtocolAmendmentA2SidecarBytes,
            kSrr2ProtocolAmendmentA2SidecarSha256);
  result.protocol_amendment_a3_bound =
      bound("src/tests/bench/wikimyei/representation/encoding/"
            "mtf_jepa_mae_vicreg/"
            "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A3.md",
            kSrr2ProtocolAmendmentA3Bytes,
            kSrr2ProtocolAmendmentA3Sha256) &&
      bound("src/tests/bench/wikimyei/representation/encoding/"
            "mtf_jepa_mae_vicreg/"
            "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A3."
            "sha256",
            kSrr2ProtocolAmendmentA3SidecarBytes,
            kSrr2ProtocolAmendmentA3SidecarSha256);
  try {
    const auto a2_log =
        srr2_read_binary_file(std::string(kSrr2A2AuthoritativeLogPath));
    const auto occurs_once = [&](std::string_view record) {
      const auto first = a2_log.find(record);
      return first != std::string::npos &&
             a2_log.find(record, first + record.size()) == std::string::npos;
    };
    result.a2_incident_bound =
        bound(kSrr2A2PrerunManifestPath, kSrr2A2PrerunManifestBytes,
              kSrr2A2PrerunManifestSha256) &&
        bound(kSrr2A2AuthoritativeLogPath, kSrr2A2AuthoritativeLogBytes,
              kSrr2A2AuthoritativeLogSha256) &&
        std::filesystem::hard_link_count(
            std::string(kSrr2A2PrerunManifestPath)) == 1 &&
        std::filesystem::hard_link_count(
            std::string(kSrr2A2AuthoritativeLogPath)) == 1 &&
        a2_log.size() == kSrr2A2AuthoritativeLogBytes &&
        srr2_digest::sha256_hex(a2_log) == kSrr2A2AuthoritativeLogSha256 &&
        a2_log.find('\r') == std::string::npos &&
        occurs_once("srr2.attempt.consumed=false\n") &&
        occurs_once("authoritative_attempt_count=0\n") &&
        occurs_once("failure_reason=invalid_mechanics\n") &&
        occurs_once("terminal_result=invalid_mechanics\n") &&
        occurs_once("SRR-2 production structured readout parity failure: "
                    "outer augmentation model device is not cuda:0\n") &&
        srr2_authorization_tail_exact(a2_log);
  } catch (const std::exception &) {
    result.a2_incident_bound = false;
  }
  result.baseline_bound =
      bound(kSrr2BaselineArchivePath, 829440,
            "22f53f452836583f5402d1a28d67c9b3a9ac865094444a06b9726fd0f1c7b6dd");
  result.sealed_quality_parent_source_bound = bound(
      kSrr2SealedQualityParentSourcePath, kSrr2SealedQualityParentSourceBytes,
      kSrr2SealedQualityParentSourceSha256);
  result.executable_bound = entries.count(std::string(kSrr2BinaryPath)) == 1;
  result.candidate_patch_bound =
      entries.count(std::string(kSrr2CandidatePatchPath)) == 1 &&
      entries.at(std::string(kSrr2CandidatePatchPath)).bytes > 0;
  if (result.candidate_patch_bound) {
    const auto [patch_receipt, verifier_exit_exact] =
        srr2_candidate_patch_verifier_receipt();
    const auto &manifest_patch =
        entries.at(std::string(kSrr2CandidatePatchPath));
    const std::map<std::string, std::string> expected_patch{
        {"schema", "wikimyei.mtf_jepa_mae_vicreg.srr2_candidate_patch.v1"},
        {"srr2.candidate_patch.path", std::string(kSrr2CandidatePatchPath)},
        {"srr2.candidate_patch.sha256", manifest_patch.sha256},
        {"srr2.candidate_patch.bytes", std::to_string(manifest_patch.bytes)},
        {"srr2.candidate_patch.baseline_entry_count", "14"},
        {"srr2.candidate_patch.new_entry_count", "17"},
        {"srr2.candidate_patch.apply_exact", "true"},
        {"srr2.candidate_patch.live_tree_exact", "true"},
        {"srr2.candidate_patch.pass", "true"}};
    result.candidate_patch_bound =
        verifier_exit_exact && patch_receipt.exact_syntax &&
        patch_receipt.records.duplicate_count == 0 &&
        patch_receipt.records.malformed_count == 0 &&
        patch_receipt.initializing_diagnostic_count == 0 &&
        patch_receipt.finalizing_diagnostic_count == 0 &&
        patch_receipt.records.values == expected_patch;
  }
  result.mechanics_bound =
      entries.count(std::string(kSrr2MechanicsLogPath)) == 1;
  result.preflight_bound =
      entries.count(std::string(kSrr2PreflightLogPath)) == 1;
  result.container_identity_bound =
      bound(kSrr2ContainerIdentityPath, kSrr2ContainerIdentityBytes,
            kSrr2ContainerIdentitySha256);
  if (result.container_identity_bound) {
    const auto identity = srr2_parse_closed_records(
        srr2_read_binary_file(std::string(kSrr2ContainerIdentityPath)), false);
    const std::map<std::string, std::string> expected_identity{
        {"container_name", "unnamed_taoist"},
        {"container_id",
         container_id == metadata.end() ? std::string{} : container_id->second},
        {"image_id", std::string(kSrr2ImageId)},
        {"working_directory", "/cuwacunu"}};
    result.container_identity_bound =
        identity.exact_syntax && identity.records.duplicate_count == 0 &&
        identity.records.malformed_count == 0 &&
        identity.initializing_diagnostic_count == 0 &&
        identity.finalizing_diagnostic_count == 0 &&
        identity.records.values == expected_identity;
  }
  result.build_receipt_bound =
      entries.count(std::string(kSrr2BuildReceiptPath)) == 1;
  if (result.build_receipt_bound) {
    const auto build = srr2_parse_closed_records(
        srr2_read_binary_file(std::string(kSrr2BuildReceiptPath)), false);
    const std::array<std::pair<std::string_view, std::string_view>, 9> binaries{
        {
            {"production", ".build/tests/test_production_structured_readout"},
            {"shadow", ".build/tests/test_structured_readout_shadow"},
            {"gate",
             ".build/tests/test_production_structured_readout_parity_gate"},
            {"auditor",
             ".build/tests/"
             "test_production_structured_readout_parity_log_auditor"},
            {"parity", ".build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_"
                       "production_structured_readout_parity"},
            {"core", ".build/tests/test_wikimyei_mtf_jepa_mae_vicreg"},
            {"contracts",
             ".build/tests/test_wikimyei_mtf_jepa_mae_vicreg_contracts"},
            {"config", ".build/tests/test_wikimyei_graph_first_specs"},
            {"adapter",
             ".build/tests/test_jkimyei_channel_graph_first_launchers"},
        }};
    std::set<std::string> expected_build_keys{"schema", "build_command",
                                              "srr2.build.binary_count",
                                              "srr2.build.pass"};
    bool build_exact =
        build.exact_syntax && build.records.duplicate_count == 0 &&
        build.records.malformed_count == 0 &&
        build.initializing_diagnostic_count == 0 &&
        build.finalizing_diagnostic_count == 0 &&
        build.records.maybe("schema") ==
            std::optional<std::string>{
                "wikimyei.mtf_jepa_mae_vicreg.srr2_build_receipt.v1"} &&
        build.records.maybe("build_command") ==
            std::optional<std::string>{std::string(kSrr2BuildCommand)} &&
        build.records.maybe("srr2.build.binary_count") ==
            std::optional<std::string>{"9"} &&
        build.records.maybe("srr2.build.pass") ==
            std::optional<std::string>{"true"};
    for (const auto &[label, expected_path] : binaries) {
      const std::string prefix = "srr2.build." + std::string(label);
      expected_build_keys.emplace(prefix + ".path");
      expected_build_keys.emplace(prefix + ".bytes");
      expected_build_keys.emplace(prefix + ".sha256");
      const auto path_value = build.records.maybe(prefix + ".path");
      const auto bytes_value = build.records.maybe(prefix + ".bytes");
      const auto sha_value = build.records.maybe(prefix + ".sha256");
      std::uint64_t parsed_bytes = 0;
      const auto manifest_entry = entries.find(std::string(expected_path));
      build_exact =
          build_exact &&
          path_value ==
              std::optional<std::string>{std::string(expected_path)} &&
          bytes_value.has_value() &&
          srr2_parse_canonical_u64(*bytes_value, parsed_bytes) &&
          sha_value.has_value() && srr2_lower_hex_exact(*sha_value, 64) &&
          manifest_entry != entries.end() &&
          manifest_entry->second.bytes == parsed_bytes &&
          manifest_entry->second.sha256 == *sha_value;
    }
    std::set<std::string> observed_build_keys;
    for (const auto &[key, value] : build.records.values) {
      (void)value;
      observed_build_keys.emplace(key);
    }
    result.build_receipt_bound =
        build_exact && observed_build_keys == expected_build_keys;
  }
  if (result.mechanics_bound) {
    const auto receipt = srr2_parse_closed_records(
        srr2_read_binary_file(std::string(kSrr2MechanicsLogPath)), false);
    result.mechanics_records = receipt.records;
    result.mechanics_bound = srr2_mechanics_receipt_exact(receipt);
  }
  if (result.preflight_bound) {
    const auto raw_receipt =
        srr2_read_binary_file(std::string(kSrr2PreflightLogPath));
    const auto receipt = srr2_parse_closed_records(raw_receipt, true);
    result.preflight_records = receipt.records;
    result.preflight_bound =
        srr2_preflight_receipt_exact(receipt, raw_receipt, token_layout_hash);
  }
  try {
    const auto root =
        std::filesystem::canonical(std::filesystem::current_path());
    const auto executable = std::filesystem::canonical("/proc/self/exe");
    const auto relative = executable.lexically_relative(root).generic_string();
    const auto found = entries.find(relative);
    result.executing_binary_bytes = std::filesystem::file_size(executable);
    result.executing_binary_sha256 =
        srr2_digest::sha256_hex(srr2_read_binary_file(executable));
    result.executable_self_bound =
        relative == kSrr2BinaryPath && found != entries.end() &&
        found->second.bytes == result.executing_binary_bytes &&
        found->second.sha256 == result.executing_binary_sha256;
  } catch (const std::exception &) {
    result.executable_self_bound = false;
  }
  result.command_exact =
      metadata_is("authoritative_command", kSrr2AuthoritativeCommand) &&
      metadata_is("build_command", kSrr2BuildCommand) &&
      metadata_is("mechanics_command", kSrr2MechanicsCommand) &&
      metadata_is("preflight_command", kSrr2PreflightCommand) &&
      metadata_is("audit_command", kSrr2AuditCommand);
  result.projection_facts_exact =
      metadata_is("projection_q0_hash", "f8c9f35282de2ee0") &&
      metadata_is("projection_qpsm_hash", "ac8a43fd65b2c8a8");
  result.token_layout_bound =
      metadata_is("token_layout_hash", srr2_hex64(token_layout_hash));
  result.exact =
      result.nonempty && result.entry_count == entries.size() &&
      result.entries_exact && result.path_contract_exact &&
      result.metadata_exact && result.mandatory_entries_exact &&
      result.attempt_ledger_preseal_absent && result.attempt_ledger_excluded &&
      result.authoritative_log_preseal_contract_exact &&
      result.runtime_identity_exact && result.executable_bound &&
      result.executable_self_bound &&
      result.sealed_quality_parent_source_bound && result.protocol_bound &&
      result.protocol_amendment_bound && result.protocol_amendment_a2_bound &&
      result.protocol_amendment_a3_bound && result.a2_incident_bound &&
      result.baseline_bound &&
      result.container_identity_bound && result.build_receipt_bound &&
      result.command_exact && result.projection_facts_exact &&
      result.token_layout_bound && result.mechanics_bound &&
      result.preflight_bound && result.candidate_patch_bound;
  return result;
}

void srr2_configure_determinism() {
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  auto &context = at::globalContext();
  context.setDeterministicCuDNN(true);
  context.setDeterministicAlgorithms(true, false);
  context.setAllowTF32CuBLAS(false);
  context.setAllowTF32CuDNN(false);
}

struct Srr2EnvironmentReceipt {
  int cpu_threads{0};
  int cpu_interop_threads{0};
  bool deterministic_algorithms{false};
  bool deterministic_warn_only{true};
  bool deterministic_cudnn{false};
  bool tf32_cublas_disabled{false};
  bool tf32_cudnn_disabled{false};
  bool cublas_workspace_exact{false};
  bool cuda_available{false};
  bool exact{false};
};

[[nodiscard]] Srr2EnvironmentReceipt srr2_environment_receipt() {
  auto &context = at::globalContext();
  const char *workspace = std::getenv("CUBLAS_WORKSPACE_CONFIG");
  Srr2EnvironmentReceipt result{};
  result.cpu_threads = at::get_num_threads();
  result.cpu_interop_threads = at::get_num_interop_threads();
  result.deterministic_algorithms = context.deterministicAlgorithms();
  result.deterministic_warn_only = context.deterministicAlgorithmsWarnOnly();
  result.deterministic_cudnn = context.deterministicCuDNN();
  result.tf32_cublas_disabled = !context.allowTF32CuBLAS();
  result.tf32_cudnn_disabled = !context.allowTF32CuDNN();
  result.cublas_workspace_exact =
      workspace != nullptr && std::string_view(workspace) == ":4096:8";
  result.cuda_available = torch::cuda::is_available();
  result.exact = result.cpu_threads == 1 && result.cpu_interop_threads == 1 &&
                 result.deterministic_algorithms &&
                 !result.deterministic_warn_only &&
                 result.deterministic_cudnn && result.tf32_cublas_disabled &&
                 result.tf32_cudnn_disabled && result.cublas_workspace_exact &&
                 result.cuda_available;
  return result;
}

void srr2_emit_environment(const Srr2EnvironmentReceipt &environment) {
  std::cout << "srr2.environment.device=cuda:0\n";
  std::cout << "srr2.environment.dtype=float32\n";
  std::cout << "srr2.environment.cpu_threads=" << environment.cpu_threads
            << '\n';
  std::cout << "srr2.environment.cpu_interop_threads="
            << environment.cpu_interop_threads << '\n';
  std::cout << "srr2.environment.deterministic_algorithms="
            << environment.deterministic_algorithms << '\n';
  std::cout << "srr2.environment.deterministic_warn_only="
            << environment.deterministic_warn_only << '\n';
  std::cout << "srr2.environment.deterministic_cudnn="
            << environment.deterministic_cudnn << '\n';
  std::cout << "srr2.environment.tf32_cublas_disabled="
            << environment.tf32_cublas_disabled << '\n';
  std::cout << "srr2.environment.tf32_cudnn_disabled="
            << environment.tf32_cudnn_disabled << '\n';
  std::cout << "srr2.environment.cublas_workspace_exact="
            << environment.cublas_workspace_exact << '\n';
  std::cout << "srr2.environment.cuda_available=" << environment.cuda_available
            << '\n';
}

// Target-free reproduction of the sealed deterministic input generator.  This
// is the only dataset constructor called by either SRR-2 mode.
[[nodiscard]] Dataset srr2_generate_inputs(int64_t group_begin, int64_t groups,
                                           int64_t view = 0) {
  auto data = torch::empty({groups, kChannels, kHistory, kFeatures},
                           torch::TensorOptions().dtype(torch::kFloat32));
  auto mask = torch::ones({groups, kChannels, kHistory, kFeatures},
                          torch::TensorOptions().dtype(torch::kBool));
  auto x = data.accessor<float, 4>();
  auto valid = mask.accessor<bool, 4>();
  for (int64_t row = 0; row < groups; ++row) {
    const int64_t group = group_begin + row;
    const auto factors = factors_for(group, false);
    for (int64_t channel = 0; channel < kChannels; ++channel) {
      for (int64_t history = 0; history < kHistory; ++history) {
        const double current =
            observed_value(factors, group, channel, history, view);
        const double previous =
            observed_value(factors, group, channel, history - 1, view);
        double mean3 = 0.0;
        double square3 = 0.0;
        double mean8 = 0.0;
        double square8 = 0.0;
        for (int64_t offset = 0; offset < 8; ++offset) {
          const double value =
              observed_value(factors, group, channel, history - offset, view);
          mean8 += value;
          square8 += value * value;
          if (offset < 3) {
            mean3 += value;
            square3 += value * value;
          }
        }
        mean3 /= 3.0;
        mean8 /= 8.0;
        const double std3 =
            std::sqrt(std::max(0.0, square3 / 3.0 - mean3 * mean3));
        const double std8 =
            std::sqrt(std::max(0.0, square8 / 8.0 - mean8 * mean8));
        const int64_t other = (channel + 1) % kChannels;
        const double other_value =
            observed_value(factors, group, other, history, view);
        const std::array<double, kFeatures> features{current,
                                                     current - previous,
                                                     std::fabs(current),
                                                     current * current,
                                                     mean3,
                                                     std3,
                                                     mean8,
                                                     std8,
                                                     current * other_value};
        for (int64_t feature = 0; feature < kFeatures; ++feature) {
          x[row][channel][history][feature] =
              static_cast<float>(features[static_cast<std::size_t>(feature)]);
          if (view == 1 &&
              uniform01(key(group, 40 + channel, history * kFeatures + feature,
                            view)) < 0.04) {
            valid[row][channel][history][feature] = false;
          }
        }
      }
    }
  }
  return {.data = std::move(data),
          .mask = std::move(mask),
          .target = torch::Tensor{},
          .group_begin = group_begin};
}

void srr2_validate_inputs(const Dataset &dataset, int64_t group_begin,
                          int64_t rows) {
  if (dataset.data.sizes() !=
          torch::IntArrayRef({rows, kChannels, kHistory, kFeatures}) ||
      dataset.data.scalar_type() != torch::kFloat32 ||
      !dataset.data.device().is_cpu() ||
      dataset.mask.sizes() != dataset.data.sizes() ||
      dataset.mask.scalar_type() != torch::kBool ||
      !dataset.mask.device().is_cpu() || dataset.target.defined() ||
      dataset.group_begin != group_begin ||
      !torch::isfinite(dataset.data).all().item<bool>()) {
    throw std::runtime_error("SRR-2 target-free input contract failed");
  }
}

[[nodiscard]] Dataset srr2_reversed_inputs(const Dataset &source) {
  if (source.target.defined()) {
    throw std::runtime_error("SRR-2 reversal received a target tensor");
  }
  return {.data = source.data.flip({2}).contiguous(),
          .mask = source.mask.flip({2}).contiguous(),
          .target = torch::Tensor{},
          .group_begin = source.group_begin};
}

struct Srr2ProjectionReceipt {
  torch::Tensor q0{};
  torch::Tensor qpsm{};
  uint64_t q0_hash{0};
  uint64_t qpsm_hash{0};
  double orthogonality_error{0.0};
  double contrast_mean_error{0.0};
  double block_sum_error{0.0};
  bool q0_exact{false};
  bool qpsm_exact{false};
  bool production_projection_exact{false};
  bool invariants_exact{false};
  bool exact{false};
};

[[nodiscard]] Srr2ProjectionReceipt srr2_projection_receipt() {
  Srr2ProjectionReceipt result{};
  result.q0 = rssm_make_token_projection();
  result.qpsm = psm_make_projection(result.q0);
  result.q0_hash = hash_tensor_stable_bytes(result.q0);
  result.qpsm_hash = hash_tensor_stable_bytes(result.qpsm);
  const auto production_q0 = mtf::detail::structured_cdsb_v1_make_q0_cpu64();
  const auto production_qpsm =
      mtf::detail::structured_cdsb_v1_make_qpsm_cpu64(production_q0);
  const auto production_exposed_copy =
      mtf::detail::structured_cdsb_v1_projection_for(
          torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU));
  const auto receipt = psm_projection_receipt(result.q0, result.qpsm);
  result.orthogonality_error = receipt.orthogonality_error;
  result.contrast_mean_error = receipt.contrast_mean_error;
  result.block_sum_error = receipt.block_sum_identity_error;
  result.q0_exact = result.q0_hash == 0xf8c9f35282de2ee0ULL &&
                    rssm_tensor_bytes_equal(result.q0, production_q0);
  result.qpsm_exact = result.qpsm_hash == 0xac8a43fd65b2c8a8ULL;
  result.production_projection_exact =
      rssm_tensor_bytes_equal(result.qpsm, production_qpsm) &&
      rssm_tensor_bytes_equal(result.qpsm, production_exposed_copy);
  result.invariants_exact = receipt.pass &&
                            result.orthogonality_error <= 1.0e-10 &&
                            result.contrast_mean_error <= 1.0e-10 &&
                            result.block_sum_error <= 1.0e-10;
  result.exact = result.q0_exact && result.qpsm_exact &&
                 result.production_projection_exact && result.invariants_exact;
  return result;
}

void srr2_emit_projection(const Srr2ProjectionReceipt &projection,
                          const PsmTokenLayoutReceipt &layout) {
  srr2_emit_hash("srr2.projection.q0_hash", projection.q0_hash);
  srr2_emit_hash("srr2.projection.qpsm_hash", projection.qpsm_hash);
  std::cout << "srr2.projection.orthogonality_error="
            << projection.orthogonality_error << '\n';
  std::cout << "srr2.projection.contrast_mean_error="
            << projection.contrast_mean_error << '\n';
  std::cout << "srr2.projection.block_sum_error=" << projection.block_sum_error
            << '\n';
  srr2_emit_hash("srr2.layout.hash", layout.layout_hash);
  std::cout << "srr2.layout.cell_vector=";
  for (std::size_t index = 0; index < kSrr2CellVector.size(); ++index) {
    if (index != 0) {
      std::cout << ',';
    }
    std::cout << kSrr2CellVector[index];
  }
  std::cout << '\n';
  std::cout << "srr2.layout.exact=" << layout.pass << '\n';
}

struct Srr2TensorStateSnapshot {
  std::vector<std::string> names{};
  std::vector<torch::Tensor> values{};
  std::vector<torch::Device> devices{};
};

[[nodiscard]] Srr2TensorStateSnapshot
srr2_snapshot_buffers(const mtf::MtfJepaMaeVicreg &model) {
  Srr2TensorStateSnapshot result{};
  torch::NoGradGuard no_grad;
  for (const auto &item : model->named_buffers(/*recurse=*/true)) {
    result.names.push_back(item.key());
    result.values.push_back(item.value().detach().to(torch::kCPU).clone());
    result.devices.push_back(item.value().device());
  }
  return result;
}

[[nodiscard]] bool
srr2_buffers_equal(const mtf::MtfJepaMaeVicreg &model,
                   const Srr2TensorStateSnapshot &reference) {
  const auto buffers = model->named_buffers(/*recurse=*/true);
  if (buffers.size() != reference.names.size() ||
      reference.names.size() != reference.values.size() ||
      reference.names.size() != reference.devices.size()) {
    return false;
  }
  std::size_t index = 0;
  for (const auto &item : buffers) {
    if (item.key() != reference.names[index] ||
        item.value().device() != reference.devices[index] ||
        !rssm_tensor_bytes_equal(item.value(), reference.values[index])) {
      return false;
    }
    ++index;
  }
  return true;
}

[[nodiscard]] uint64_t
srr2_buffer_snapshot_hash(const Srr2TensorStateSnapshot &snapshot) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  for (std::size_t index = 0; index < snapshot.values.size(); ++index) {
    mix_hash_value(hash, fnv1a64(snapshot.names[index]));
    mix_hash_value(hash, hash_tensor_stable_bytes(snapshot.values[index]));
    mix_hash_value(hash, static_cast<uint64_t>(snapshot.devices[index].type()));
    mix_hash_value(hash,
                   static_cast<uint64_t>(snapshot.devices[index].index() + 1));
  }
  return hash;
}

[[nodiscard]] uint64_t
srr2_metadata_hash(const mtf::mtf_token_metadata_t &metadata);

[[nodiscard]] uint64_t
srr2_token_batch_hash(const mtf::mtf_token_batch_t &tokens) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  for (const auto &tensor :
       {tokens.tokens, tokens.reconstruction_targets,
        tokens.time_reconstruction_targets,
        tokens.frequency_reconstruction_targets,
        tokens.time_reconstruction_mask, tokens.frequency_reconstruction_mask,
        tokens.token_mask}) {
    mix_hash_value(hash, hash_tensor_stable_bytes(tensor));
  }
  mix_hash_value(hash, srr2_metadata_hash(tokens.metadata));
  return hash;
}

[[nodiscard]] uint64_t srr2_finite_count(const torch::Tensor &values) {
  return static_cast<uint64_t>(torch::isfinite(values).sum().item<int64_t>());
}

[[nodiscard]] uint64_t srr2_invalid_count(const torch::Tensor &valid_mask) {
  return static_cast<uint64_t>(valid_mask.logical_not().sum().item<int64_t>());
}

[[nodiscard]] uint64_t
srr2_invalid_value_hash(const torch::Tensor &values,
                        const torch::Tensor &valid_mask) {
  return hash_tensor_stable_bytes(
      values
          .masked_select(
              valid_mask.logical_not().unsqueeze(-1).expand_as(values))
          .contiguous());
}

[[nodiscard]] uint64_t
srr2_encoded_hash(const mtf::mtf_jepa_mae_vicreg_encode_output_t &encoded) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  for (const auto &tensor :
       {encoded.embeddings, encoded.pooled_embedding, encoded.pooled_by_channel,
        encoded.pooled_time, encoded.pooled_frequency, encoded.token_mask,
        encoded.sample_valid_mask, encoded.channel_valid_mask}) {
    mix_hash_value(hash, hash_tensor_stable_bytes(tensor));
  }
  mix_hash_value(hash, hash_tensor_stable_bytes(encoded.metadata.start_index));
  mix_hash_value(hash, hash_tensor_stable_bytes(encoded.metadata.width));
  mix_hash_value(hash, hash_tensor_stable_bytes(encoded.metadata.scale_id));
  mix_hash_value(hash, hash_tensor_stable_bytes(encoded.metadata.channel_id));
  mix_hash_value(hash, hash_tensor_stable_bytes(encoded.metadata.domain_id));
  return hash;
}

[[nodiscard]] uint64_t
srr2_metadata_hash(const mtf::mtf_token_metadata_t &metadata) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.start_index));
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.width));
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.scale_id));
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.channel_id));
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.domain_id));
  return hash;
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_encode_output_t
srr2_audit_encoded(const mtf::mtf_jepa_mae_vicreg_encode_output_t &encoded) {
  const auto floating = [](const torch::Tensor &value) {
    return value.detach().to(torch::kCPU, torch::kFloat64).contiguous();
  };
  const auto plain = [](const torch::Tensor &value) {
    return value.detach().to(torch::kCPU).contiguous();
  };
  return {.embeddings = floating(encoded.embeddings),
          .pooled_embedding = floating(encoded.pooled_embedding),
          .pooled_by_channel = floating(encoded.pooled_by_channel),
          .pooled_time = floating(encoded.pooled_time),
          .pooled_frequency = floating(encoded.pooled_frequency),
          .token_mask = plain(encoded.token_mask),
          .sample_valid_mask = plain(encoded.sample_valid_mask),
          .channel_valid_mask = plain(encoded.channel_valid_mask),
          .metadata = {.start_index = plain(encoded.metadata.start_index),
                       .width = plain(encoded.metadata.width),
                       .scale_id = plain(encoded.metadata.scale_id),
                       .channel_id = plain(encoded.metadata.channel_id),
                       .domain_id = plain(encoded.metadata.domain_id)}};
}

[[nodiscard]] mtf::mtf_serving_pool_output_t
srr2_offline_reference(const mtf::mtf_jepa_mae_vicreg_encode_output_t &audit,
                       const torch::Tensor &qpsm) {
  const auto order = rssm_token_order(audit.metadata, audit.embeddings.size(1));
  if (!order.cardinality_exact) {
    throw std::runtime_error("SRR-2 offline reference token order failed");
  }
  const auto grouped = rssm_group_tokens_by_channel(audit.embeddings, order);
  const auto lifted =
      psm_partition_lift(grouped, PsmArm::channel_domain_scale_bin);
  auto values = rssm_project_by_channel(lifted, qpsm).contiguous();
  std::vector<torch::Tensor> channel_masks;
  channel_masks.reserve(kChannels);
  for (const auto &indices : order.channel_indices) {
    const auto index = torch::tensor(indices, torch::kInt64);
    channel_masks.push_back(audit.token_mask.index_select(1, index).all(1));
  }
  auto valid_mask = torch::stack(channel_masks, 1).contiguous();
  valid_mask = valid_mask.logical_and(audit.sample_valid_mask.unsqueeze(1));
  valid_mask = valid_mask.logical_and(audit.channel_valid_mask).contiguous();
  values =
      torch::where(valid_mask.unsqueeze(-1), values, torch::zeros_like(values))
          .contiguous();
  return {.values = std::move(values), .valid_mask = std::move(valid_mask)};
}

struct Srr2Capture {
  int64_t rows{0};
  int64_t captured_row_count{0};
  torch::Tensor production{};       // device result copied once to CPU64
  torch::Tensor shadow{};           // device result copied once to CPU64
  torch::Tensor reference{};        // independent CPU64 D
  torch::Tensor production_mask{};  // CPU bool
  torch::Tensor shadow_mask{};      // CPU bool
  torch::Tensor reference_mask{};   // CPU bool
  torch::Tensor cpu64_production{}; // public production on CPU64 audit input
  torch::Tensor cpu64_shadow{};     // sealed shadow on same CPU64 audit input
  torch::Tensor cpu64_production_mask{};
  torch::Tensor cpu64_shadow_mask{};
  uint64_t encoder_hash{0};
  uint64_t served_hash{0};
  uint64_t metadata_structure_hash{0};
  uint64_t production_hash{0};
  uint64_t shadow_hash{0};
  uint64_t reference_hash{0};
  uint64_t production_mask_hash{0};
  uint64_t shadow_mask_hash{0};
  uint64_t reference_mask_hash{0};
  uint64_t cpu64_production_hash{0};
  uint64_t cpu64_shadow_hash{0};
  uint64_t cpu64_production_mask_hash{0};
  uint64_t cpu64_shadow_mask_hash{0};
  uint64_t input_data_hash_before{0};
  uint64_t input_data_hash_after{0};
  uint64_t input_mask_hash_before{0};
  uint64_t input_mask_hash_after{0};
  uint64_t parameter_hash_before{0};
  uint64_t parameter_hash_after{0};
  uint64_t buffer_hash_before{0};
  uint64_t buffer_hash_after{0};
  uint64_t cpu_rng_hash_before{0};
  uint64_t cpu_rng_hash_after{0};
  uint64_t cuda_rng_hash_before{0};
  uint64_t cuda_rng_hash_after{0};
  uint64_t encoded_hash_before{0xcbf29ce484222325ULL};
  uint64_t encoded_hash_between{0xcbf29ce484222325ULL};
  uint64_t encoded_hash_after{0xcbf29ce484222325ULL};
  uint64_t encoded_hash_final{0xcbf29ce484222325ULL};
  uint64_t tokenizer_hash_before{0xcbf29ce484222325ULL};
  uint64_t tokenizer_hash_after{0xcbf29ce484222325ULL};
  uint64_t token_mask_hash_before{0xcbf29ce484222325ULL};
  uint64_t token_mask_hash_after{0xcbf29ce484222325ULL};
  uint64_t token_metadata_hash_before{0xcbf29ce484222325ULL};
  uint64_t token_metadata_hash_after{0xcbf29ce484222325ULL};
  uint64_t encoded_token_mask_hash{0xcbf29ce484222325ULL};
  uint64_t encoded_metadata_hash{0xcbf29ce484222325ULL};
  uint64_t sandwich_values_hash_before{0};
  uint64_t sandwich_values_hash_after{0};
  uint64_t sandwich_mask_hash_before{0};
  uint64_t sandwich_mask_hash_after{0};
  uint64_t production_finite_count{0};
  uint64_t shadow_finite_count{0};
  uint64_t reference_finite_count{0};
  uint64_t cpu64_production_finite_count{0};
  uint64_t cpu64_shadow_finite_count{0};
  uint64_t production_invalid_count{0};
  uint64_t shadow_invalid_count{0};
  uint64_t reference_invalid_count{0};
  uint64_t cpu64_production_invalid_count{0};
  uint64_t cpu64_shadow_invalid_count{0};
  uint64_t production_invalid_value_hash{0};
  uint64_t shadow_invalid_value_hash{0};
  uint64_t reference_invalid_value_hash{0};
  uint64_t cpu64_production_invalid_value_hash{0};
  uint64_t cpu64_shadow_invalid_value_hash{0};
  bool mode_before{false};
  bool mode_after{false};
  bool target_defined_before{false};
  bool target_defined_after{false};
  bool production_invalid_zero_exact{true};
  bool shadow_invalid_zero_exact{true};
  bool reference_invalid_zero_exact{true};
  bool cpu64_production_invalid_zero_exact{true};
  bool cpu64_shadow_invalid_zero_exact{true};
  double parameter_max_abs{0.0};
  bool shape_exact{true};
  bool strides_and_contiguity_exact{true};
  bool dtype_exact{true};
  bool device_exact{true};
  bool finite{true};
  bool public_sandwich_exact{true};
  bool layout_exact{true};
  bool same_encoded_object{true};
  bool input_unchanged{true};
  bool parameter_unchanged{true};
  bool buffer_unchanged{true};
  bool cpu_rng_unchanged{true};
  bool cuda_rng_unchanged{true};
  bool model_mode_unchanged{true};
  bool production_shadow_value_bytes_exact{true};
  bool production_shadow_mask_bytes_exact{true};
  bool cpu64_production_shadow_value_bytes_exact{true};
  bool cpu64_production_shadow_mask_bytes_exact{true};
  bool cpu64_production_reference_value_bytes_exact{true};
  bool cpu64_production_reference_mask_bytes_exact{true};
  bool cpu64_shadow_reference_value_bytes_exact{true};
  bool cpu64_shadow_reference_mask_bytes_exact{true};
  bool invalid_zero_exact{true};
  bool complete{false};
  double production_shadow_device_max_abs{0.0};
  double cpu64_production_shadow_max_abs{0.0};
  double cpu64_production_reference_max_abs{0.0};
  double cpu64_shadow_reference_max_abs{0.0};
  double device_production_reference_max_abs{0.0};
  double device_shadow_reference_max_abs{0.0};
};

[[nodiscard]] Srr2Capture
srr2_capture_once(mtf::MtfJepaMaeVicreg &model, const Dataset &dataset,
                  const torch::Device &device, const torch::Tensor &qpsm,
                  const PsmTokenLayoutReceipt &expected_layout) {
  if (model->is_training()) {
    throw std::runtime_error("SRR-2 capture requires an eval-mode model");
  }
  const bool mode_before = model->is_training();
  const uint64_t input_data_before = hash_tensor_stable_bytes(dataset.data);
  const uint64_t input_mask_before = hash_tensor_stable_bytes(dataset.mask);
  const auto parameters_before = snapshot_parameters(model);
  const uint64_t parameter_hash_before =
      rssm_parameter_snapshot_hash(parameters_before);
  const auto buffers_before = srr2_snapshot_buffers(model);
  const auto generators_before = current_generator_state_snapshot(device);

  std::vector<torch::Tensor> encoder_chunks;
  std::vector<torch::Tensor> served_chunks;
  std::vector<torch::Tensor> production_chunks;
  std::vector<torch::Tensor> shadow_chunks;
  std::vector<torch::Tensor> reference_chunks;
  std::vector<torch::Tensor> production_mask_chunks;
  std::vector<torch::Tensor> shadow_mask_chunks;
  std::vector<torch::Tensor> reference_mask_chunks;
  std::vector<torch::Tensor> cpu64_production_chunks;
  std::vector<torch::Tensor> cpu64_shadow_chunks;
  std::vector<torch::Tensor> cpu64_production_mask_chunks;
  std::vector<torch::Tensor> cpu64_shadow_mask_chunks;
  std::vector<torch::Tensor> sandwich_values_before_chunks;
  std::vector<torch::Tensor> sandwich_values_after_chunks;
  std::vector<torch::Tensor> sandwich_mask_before_chunks;
  std::vector<torch::Tensor> sandwich_mask_after_chunks;

  Srr2Capture result{};
  result.rows = dataset.data.size(0);
  result.target_defined_before = dataset.target.defined();
  result.input_data_hash_before = input_data_before;
  result.input_mask_hash_before = input_mask_before;
  result.parameter_hash_before = parameter_hash_before;
  result.buffer_hash_before = srr2_buffer_snapshot_hash(buffers_before);
  result.cpu_rng_hash_before =
      hash_tensor_stable_bytes(generators_before.cpu_state);
  result.cuda_rng_hash_before =
      hash_tensor_stable_bytes(generators_before.cuda_state);
  result.mode_before = mode_before;
  bool metadata_initialized = false;
  int64_t captured_rows = 0;
  torch::NoGradGuard no_grad;
  for (int64_t begin = 0; begin < dataset.data.size(0);
       begin += kModelRowBatchSize) {
    const int64_t size =
        std::min<int64_t>(kModelRowBatchSize, dataset.data.size(0) - begin);
    const auto data = dataset.data.narrow(0, begin, size).to(device);
    const auto feature_mask = dataset.mask.narrow(0, begin, size).to(device);
    const auto tokens_before = model->tokenize(data, feature_mask);
    const auto encoded = model->encode(data, feature_mask);
    const auto order =
        rssm_token_order(encoded.metadata, encoded.embeddings.size(1));
    const auto encoder_by_channel =
        rssm_group_tokens_by_channel(encoded.embeddings, order);
    const auto old_served_before = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::all_tokens, model->config());

    const uint64_t encoded_before = srr2_encoded_hash(encoded);
    const auto production = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::structured_cdsb_v1,
        model->config());
    const uint64_t encoded_between = srr2_encoded_hash(encoded);
    const auto shadow = srr2_shadow::readout(encoded, qpsm, model->config());
    const uint64_t encoded_after = srr2_encoded_hash(encoded);
    const auto old_served_after = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::all_tokens, model->config());

    const auto audit = srr2_audit_encoded(encoded);
    auto audit_config = model->config();
    audit_config.device = torch::Device(torch::kCPU);
    audit_config.dtype = torch::kFloat64;
    const auto cpu64_production = mtf::select_mtf_serving_pool(
        audit, mtf::mtf_serving_pool_policy_t::structured_cdsb_v1,
        audit_config);
    const auto cpu64_shadow = srr2_shadow::readout(audit, qpsm, audit_config);
    const auto reference = srr2_offline_reference(audit, qpsm);
    const uint64_t encoded_final = srr2_encoded_hash(encoded);
    const auto tokens_after = model->tokenize(data, feature_mask);

    mix_hash_value(result.encoded_hash_before, encoded_before);
    mix_hash_value(result.encoded_hash_between, encoded_between);
    mix_hash_value(result.encoded_hash_after, encoded_after);
    mix_hash_value(result.encoded_hash_final, encoded_final);
    mix_hash_value(result.tokenizer_hash_before,
                   srr2_token_batch_hash(tokens_before));
    mix_hash_value(result.tokenizer_hash_after,
                   srr2_token_batch_hash(tokens_after));
    mix_hash_value(result.token_mask_hash_before,
                   hash_tensor_stable_bytes(tokens_before.token_mask));
    mix_hash_value(result.token_mask_hash_after,
                   hash_tensor_stable_bytes(tokens_after.token_mask));
    mix_hash_value(result.token_metadata_hash_before,
                   srr2_metadata_hash(tokens_before.metadata));
    mix_hash_value(result.token_metadata_hash_after,
                   srr2_metadata_hash(tokens_after.metadata));
    mix_hash_value(result.encoded_token_mask_hash,
                   hash_tensor_stable_bytes(encoded.token_mask));
    mix_hash_value(result.encoded_metadata_hash,
                   srr2_metadata_hash(encoded.metadata));

    result.same_encoded_object =
        result.same_encoded_object && encoded_before == encoded_between &&
        encoded_between == encoded_after && encoded_after == encoded_final;
    result.public_sandwich_exact =
        result.public_sandwich_exact &&
        rssm_token_batch_bytes_equal(tokens_before, tokens_after) &&
        rssm_tensor_bytes_equal(old_served_before.values,
                                old_served_after.values) &&
        rssm_tensor_bytes_equal(old_served_before.valid_mask,
                                old_served_after.valid_mask) &&
        rssm_tensor_bytes_equal(tokens_before.token_mask, encoded.token_mask) &&
        rssm_metadata_bytes_equal(tokens_before.metadata, encoded.metadata);

    const auto grouped_metadata_layout =
        torch::stack({encoded.metadata.domain_id, encoded.metadata.scale_id,
                      encoded.metadata.start_index, encoded.metadata.width},
                     1)
            .to(torch::kCPU, torch::kInt64)
            .contiguous();
    std::vector<torch::Tensor> layout_channels;
    layout_channels.reserve(kChannels);
    for (const auto &indices : order.channel_indices) {
      layout_channels.push_back(grouped_metadata_layout.index_select(
          0, torch::tensor(indices, torch::kInt64)));
    }
    RssmEncodedCapture layout_capture{};
    layout_capture.grouped_metadata_layout =
        torch::stack(layout_channels, 0).contiguous();
    result.layout_exact =
        result.layout_exact && order.production_order_exact &&
        order.cardinality_exact &&
        psm_capture_layout_exact(layout_capture, expected_layout);

    const uint64_t metadata_hash = srr2_metadata_hash(encoded.metadata);
    if (!metadata_initialized) {
      result.metadata_structure_hash = metadata_hash;
      metadata_initialized = true;
    } else {
      result.layout_exact = result.layout_exact &&
                            result.metadata_structure_hash == metadata_hash;
    }

    const auto expected_values_shape =
        torch::IntArrayRef({size, kChannels, kLatentDim});
    const auto expected_mask_shape = torch::IntArrayRef({size, kChannels});
    result.shape_exact =
        result.shape_exact &&
        production.values.sizes() == expected_values_shape &&
        shadow.values.sizes() == expected_values_shape &&
        cpu64_production.values.sizes() == expected_values_shape &&
        cpu64_shadow.values.sizes() == expected_values_shape &&
        reference.values.sizes() == expected_values_shape &&
        production.valid_mask.sizes() == expected_mask_shape &&
        shadow.valid_mask.sizes() == expected_mask_shape &&
        cpu64_production.valid_mask.sizes() == expected_mask_shape &&
        cpu64_shadow.valid_mask.sizes() == expected_mask_shape &&
        reference.valid_mask.sizes() == expected_mask_shape;
    result.strides_and_contiguity_exact =
        result.strides_and_contiguity_exact &&
        production.values.is_contiguous() && shadow.values.is_contiguous() &&
        cpu64_production.values.is_contiguous() &&
        cpu64_shadow.values.is_contiguous() &&
        reference.values.is_contiguous() &&
        production.valid_mask.is_contiguous() &&
        shadow.valid_mask.is_contiguous() &&
        cpu64_production.valid_mask.is_contiguous() &&
        cpu64_shadow.valid_mask.is_contiguous() &&
        reference.valid_mask.is_contiguous() &&
        production.values.strides() == torch::IntArrayRef({96, 32, 1}) &&
        shadow.values.strides() == torch::IntArrayRef({96, 32, 1}) &&
        production.valid_mask.strides() == torch::IntArrayRef({3, 1}) &&
        shadow.valid_mask.strides() == torch::IntArrayRef({3, 1});
    result.dtype_exact =
        result.dtype_exact &&
        production.values.scalar_type() == torch::kFloat32 &&
        shadow.values.scalar_type() == torch::kFloat32 &&
        production.valid_mask.scalar_type() == torch::kBool &&
        shadow.valid_mask.scalar_type() == torch::kBool &&
        cpu64_production.values.scalar_type() == torch::kFloat64 &&
        cpu64_shadow.values.scalar_type() == torch::kFloat64 &&
        reference.values.scalar_type() == torch::kFloat64;
    result.device_exact = result.device_exact &&
                          production.values.device() == device &&
                          shadow.values.device() == device &&
                          production.valid_mask.device() == device &&
                          shadow.valid_mask.device() == device &&
                          cpu64_production.values.device().is_cpu() &&
                          cpu64_shadow.values.device().is_cpu() &&
                          reference.values.device().is_cpu();
    result.finite =
        result.finite &&
        torch::isfinite(production.values).all().item<bool>() &&
        torch::isfinite(shadow.values).all().item<bool>() &&
        torch::isfinite(cpu64_production.values).all().item<bool>() &&
        torch::isfinite(cpu64_shadow.values).all().item<bool>() &&
        torch::isfinite(reference.values).all().item<bool>();

    result.production_shadow_value_bytes_exact =
        result.production_shadow_value_bytes_exact &&
        rssm_tensor_bytes_equal(production.values, shadow.values);
    result.production_shadow_mask_bytes_exact =
        result.production_shadow_mask_bytes_exact &&
        rssm_tensor_bytes_equal(production.valid_mask, shadow.valid_mask);
    result.cpu64_production_shadow_value_bytes_exact =
        result.cpu64_production_shadow_value_bytes_exact &&
        rssm_tensor_bytes_equal(cpu64_production.values, cpu64_shadow.values);
    result.cpu64_production_shadow_mask_bytes_exact =
        result.cpu64_production_shadow_mask_bytes_exact &&
        rssm_tensor_bytes_equal(cpu64_production.valid_mask,
                                cpu64_shadow.valid_mask);
    result.cpu64_production_reference_value_bytes_exact =
        result.cpu64_production_reference_value_bytes_exact &&
        rssm_tensor_bytes_equal(cpu64_production.values, reference.values);
    result.cpu64_production_reference_mask_bytes_exact =
        result.cpu64_production_reference_mask_bytes_exact &&
        rssm_tensor_bytes_equal(cpu64_production.valid_mask,
                                reference.valid_mask);
    result.cpu64_shadow_reference_value_bytes_exact =
        result.cpu64_shadow_reference_value_bytes_exact &&
        rssm_tensor_bytes_equal(cpu64_shadow.values, reference.values);
    result.cpu64_shadow_reference_mask_bytes_exact =
        result.cpu64_shadow_reference_mask_bytes_exact &&
        rssm_tensor_bytes_equal(cpu64_shadow.valid_mask, reference.valid_mask);
    result.invalid_zero_exact =
        result.invalid_zero_exact &&
        srr2_all_zero_bytes(production.values, production.valid_mask) &&
        srr2_all_zero_bytes(shadow.values, shadow.valid_mask) &&
        srr2_all_zero_bytes(cpu64_production.values,
                            cpu64_production.valid_mask) &&
        srr2_all_zero_bytes(cpu64_shadow.values, cpu64_shadow.valid_mask) &&
        srr2_all_zero_bytes(reference.values, reference.valid_mask);

    result.production_shadow_device_max_abs =
        std::max(result.production_shadow_device_max_abs,
                 srr2_max_abs(production.values, shadow.values));
    result.cpu64_production_shadow_max_abs =
        std::max(result.cpu64_production_shadow_max_abs,
                 srr2_max_abs(cpu64_production.values, cpu64_shadow.values));
    result.cpu64_production_reference_max_abs =
        std::max(result.cpu64_production_reference_max_abs,
                 srr2_max_abs(cpu64_production.values, reference.values));
    result.cpu64_shadow_reference_max_abs =
        std::max(result.cpu64_shadow_reference_max_abs,
                 srr2_max_abs(cpu64_shadow.values, reference.values));
    result.device_production_reference_max_abs =
        std::max(result.device_production_reference_max_abs,
                 srr2_max_abs(production.values, reference.values));
    result.device_shadow_reference_max_abs =
        std::max(result.device_shadow_reference_max_abs,
                 srr2_max_abs(shadow.values, reference.values));

    encoder_chunks.push_back(
        encoder_by_channel.detach().to(torch::kCPU).contiguous());
    served_chunks.push_back(
        old_served_before.values.detach().to(torch::kCPU).contiguous());
    sandwich_values_before_chunks.push_back(
        old_served_before.values.detach().to(torch::kCPU).contiguous());
    sandwich_values_after_chunks.push_back(
        old_served_after.values.detach().to(torch::kCPU).contiguous());
    sandwich_mask_before_chunks.push_back(
        old_served_before.valid_mask.detach().to(torch::kCPU).contiguous());
    sandwich_mask_after_chunks.push_back(
        old_served_after.valid_mask.detach().to(torch::kCPU).contiguous());
    production_chunks.push_back(production.values.detach()
                                    .to(torch::kCPU, torch::kFloat64)
                                    .contiguous());
    shadow_chunks.push_back(
        shadow.values.detach().to(torch::kCPU, torch::kFloat64).contiguous());
    reference_chunks.push_back(reference.values.contiguous());
    production_mask_chunks.push_back(
        production.valid_mask.detach().to(torch::kCPU).contiguous());
    shadow_mask_chunks.push_back(
        shadow.valid_mask.detach().to(torch::kCPU).contiguous());
    reference_mask_chunks.push_back(reference.valid_mask.contiguous());
    cpu64_production_chunks.push_back(cpu64_production.values.contiguous());
    cpu64_shadow_chunks.push_back(cpu64_shadow.values.contiguous());
    cpu64_production_mask_chunks.push_back(
        cpu64_production.valid_mask.contiguous());
    cpu64_shadow_mask_chunks.push_back(cpu64_shadow.valid_mask.contiguous());
    captured_rows += size;
  }

  const auto generators_after = current_generator_state_snapshot(device);
  const auto parameters_after = snapshot_parameters(model);
  const uint64_t parameter_hash_after =
      rssm_parameter_snapshot_hash(parameters_after);
  const auto buffers_after = srr2_snapshot_buffers(model);
  result.input_data_hash_after = hash_tensor_stable_bytes(dataset.data);
  result.input_mask_hash_after = hash_tensor_stable_bytes(dataset.mask);
  result.parameter_hash_after = parameter_hash_after;
  result.buffer_hash_after = srr2_buffer_snapshot_hash(buffers_after);
  result.cpu_rng_hash_after =
      hash_tensor_stable_bytes(generators_after.cpu_state);
  result.cuda_rng_hash_after =
      hash_tensor_stable_bytes(generators_after.cuda_state);
  result.mode_after = model->is_training();
  result.target_defined_after = dataset.target.defined();
  result.input_unchanged =
      result.input_data_hash_before == result.input_data_hash_after &&
      result.input_mask_hash_before == result.input_mask_hash_after &&
      !dataset.target.defined();
  result.parameter_max_abs = parameter_max_abs_diff(model, parameters_before);
  result.parameter_unchanged = parameter_hash_before == parameter_hash_after &&
                               result.parameter_max_abs == 0.0;
  result.buffer_unchanged =
      result.buffer_hash_before == result.buffer_hash_after &&
      srr2_buffers_equal(model, buffers_before);
  result.cpu_rng_unchanged = rssm_tensor_bytes_equal(
      generators_before.cpu_state, generators_after.cpu_state);
  result.cuda_rng_unchanged = rssm_tensor_bytes_equal(
      generators_before.cuda_state, generators_after.cuda_state);
  result.model_mode_unchanged = result.mode_after == result.mode_before;

  result.production = torch::cat(production_chunks, 0).contiguous();
  result.shadow = torch::cat(shadow_chunks, 0).contiguous();
  result.reference = torch::cat(reference_chunks, 0).contiguous();
  result.production_mask = torch::cat(production_mask_chunks, 0).contiguous();
  result.shadow_mask = torch::cat(shadow_mask_chunks, 0).contiguous();
  result.reference_mask = torch::cat(reference_mask_chunks, 0).contiguous();
  result.cpu64_production = torch::cat(cpu64_production_chunks, 0).contiguous();
  result.cpu64_shadow = torch::cat(cpu64_shadow_chunks, 0).contiguous();
  result.cpu64_production_mask =
      torch::cat(cpu64_production_mask_chunks, 0).contiguous();
  result.cpu64_shadow_mask =
      torch::cat(cpu64_shadow_mask_chunks, 0).contiguous();
  const auto sandwich_values_before =
      torch::cat(sandwich_values_before_chunks, 0).contiguous();
  const auto sandwich_values_after =
      torch::cat(sandwich_values_after_chunks, 0).contiguous();
  const auto sandwich_mask_before =
      torch::cat(sandwich_mask_before_chunks, 0).contiguous();
  const auto sandwich_mask_after =
      torch::cat(sandwich_mask_after_chunks, 0).contiguous();
  result.encoder_hash =
      hash_tensor_stable_bytes(torch::cat(encoder_chunks, 0).contiguous());
  result.served_hash =
      hash_tensor_stable_bytes(torch::cat(served_chunks, 0).contiguous());
  result.production_hash = hash_tensor_stable_bytes(result.production);
  result.shadow_hash = hash_tensor_stable_bytes(result.shadow);
  result.reference_hash = hash_tensor_stable_bytes(result.reference);
  result.production_mask_hash =
      hash_tensor_stable_bytes(result.production_mask);
  result.shadow_mask_hash = hash_tensor_stable_bytes(result.shadow_mask);
  result.reference_mask_hash = hash_tensor_stable_bytes(result.reference_mask);
  result.cpu64_production_hash =
      hash_tensor_stable_bytes(result.cpu64_production);
  result.cpu64_shadow_hash = hash_tensor_stable_bytes(result.cpu64_shadow);
  result.cpu64_production_mask_hash =
      hash_tensor_stable_bytes(result.cpu64_production_mask);
  result.cpu64_shadow_mask_hash =
      hash_tensor_stable_bytes(result.cpu64_shadow_mask);
  result.sandwich_values_hash_before =
      hash_tensor_stable_bytes(sandwich_values_before);
  result.sandwich_values_hash_after =
      hash_tensor_stable_bytes(sandwich_values_after);
  result.sandwich_mask_hash_before =
      hash_tensor_stable_bytes(sandwich_mask_before);
  result.sandwich_mask_hash_after =
      hash_tensor_stable_bytes(sandwich_mask_after);
  result.production_finite_count = srr2_finite_count(result.production);
  result.shadow_finite_count = srr2_finite_count(result.shadow);
  result.reference_finite_count = srr2_finite_count(result.reference);
  result.cpu64_production_finite_count =
      srr2_finite_count(result.cpu64_production);
  result.cpu64_shadow_finite_count = srr2_finite_count(result.cpu64_shadow);
  result.production_invalid_count = srr2_invalid_count(result.production_mask);
  result.shadow_invalid_count = srr2_invalid_count(result.shadow_mask);
  result.reference_invalid_count = srr2_invalid_count(result.reference_mask);
  result.cpu64_production_invalid_count =
      srr2_invalid_count(result.cpu64_production_mask);
  result.cpu64_shadow_invalid_count =
      srr2_invalid_count(result.cpu64_shadow_mask);
  result.production_invalid_value_hash =
      srr2_invalid_value_hash(result.production, result.production_mask);
  result.shadow_invalid_value_hash =
      srr2_invalid_value_hash(result.shadow, result.shadow_mask);
  result.reference_invalid_value_hash =
      srr2_invalid_value_hash(result.reference, result.reference_mask);
  result.cpu64_production_invalid_value_hash = srr2_invalid_value_hash(
      result.cpu64_production, result.cpu64_production_mask);
  result.cpu64_shadow_invalid_value_hash =
      srr2_invalid_value_hash(result.cpu64_shadow, result.cpu64_shadow_mask);
  result.production_invalid_zero_exact =
      srr2_all_zero_bytes(result.production, result.production_mask);
  result.shadow_invalid_zero_exact =
      srr2_all_zero_bytes(result.shadow, result.shadow_mask);
  result.reference_invalid_zero_exact =
      srr2_all_zero_bytes(result.reference, result.reference_mask);
  result.cpu64_production_invalid_zero_exact = srr2_all_zero_bytes(
      result.cpu64_production, result.cpu64_production_mask);
  result.cpu64_shadow_invalid_zero_exact =
      srr2_all_zero_bytes(result.cpu64_shadow, result.cpu64_shadow_mask);
  result.invalid_zero_exact =
      result.invalid_zero_exact && result.production_invalid_zero_exact &&
      result.shadow_invalid_zero_exact && result.reference_invalid_zero_exact &&
      result.cpu64_production_invalid_zero_exact &&
      result.cpu64_shadow_invalid_zero_exact;
  result.captured_row_count = captured_rows;
  result.complete = captured_rows == result.rows && result.shape_exact &&
                    result.strides_and_contiguity_exact && result.dtype_exact &&
                    result.device_exact && result.finite &&
                    result.public_sandwich_exact && result.layout_exact &&
                    result.same_encoded_object &&
                    result.production_mask.all().item<bool>() &&
                    result.shadow_mask.all().item<bool>() &&
                    result.reference_mask.all().item<bool>();
  return result;
}

[[nodiscard]] bool srr2_capture_identity_exact(const Srr2Capture &left,
                                               const Srr2Capture &right) {
  return left.rows == right.rows && left.complete && right.complete &&
         left.encoder_hash == right.encoder_hash &&
         left.served_hash == right.served_hash &&
         left.metadata_structure_hash == right.metadata_structure_hash &&
         rssm_tensor_bytes_equal(left.production, right.production) &&
         rssm_tensor_bytes_equal(left.shadow, right.shadow) &&
         rssm_tensor_bytes_equal(left.reference, right.reference) &&
         rssm_tensor_bytes_equal(left.production_mask, right.production_mask) &&
         rssm_tensor_bytes_equal(left.shadow_mask, right.shadow_mask) &&
         rssm_tensor_bytes_equal(left.reference_mask, right.reference_mask) &&
         rssm_tensor_bytes_equal(left.cpu64_production,
                                 right.cpu64_production) &&
         rssm_tensor_bytes_equal(left.cpu64_shadow, right.cpu64_shadow) &&
         rssm_tensor_bytes_equal(left.cpu64_production_mask,
                                 right.cpu64_production_mask) &&
         rssm_tensor_bytes_equal(left.cpu64_shadow_mask,
                                 right.cpu64_shadow_mask);
}

[[nodiscard]] uint64_t srr2_combined_hash(uint64_t left, uint64_t right) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  mix_hash_value(hash, left);
  mix_hash_value(hash, right);
  return hash;
}

void srr2_emit_capture_primitives(const std::string &stem,
                                  const Srr2Capture &capture) {
  const auto count = [](const torch::Tensor &tensor) {
    return static_cast<uint64_t>(tensor.numel());
  };
  const auto valid_count = [](const torch::Tensor &mask) {
    return static_cast<uint64_t>(mask.sum().item<int64_t>());
  };
  std::cout << stem << "captured_row_count=" << capture.captured_row_count
            << '\n';
  std::cout << stem << "production_value_count=" << count(capture.production)
            << '\n';
  std::cout << stem << "shadow_value_count=" << count(capture.shadow) << '\n';
  std::cout << stem << "reference_value_count=" << count(capture.reference)
            << '\n';
  std::cout << stem << "cpu64_production_value_count="
            << count(capture.cpu64_production) << '\n';
  std::cout << stem
            << "cpu64_shadow_value_count=" << count(capture.cpu64_shadow)
            << '\n';
  std::cout << stem
            << "production_mask_count=" << count(capture.production_mask)
            << '\n';
  std::cout << stem << "shadow_mask_count=" << count(capture.shadow_mask)
            << '\n';
  std::cout << stem << "reference_mask_count=" << count(capture.reference_mask)
            << '\n';
  std::cout << stem << "cpu64_production_mask_count="
            << count(capture.cpu64_production_mask) << '\n';
  std::cout << stem
            << "cpu64_shadow_mask_count=" << count(capture.cpu64_shadow_mask)
            << '\n';
  std::cout << stem
            << "production_valid_count=" << valid_count(capture.production_mask)
            << '\n';
  std::cout << stem << "shadow_valid_count=" << valid_count(capture.shadow_mask)
            << '\n';
  std::cout << stem
            << "reference_valid_count=" << valid_count(capture.reference_mask)
            << '\n';
  std::cout << stem << "cpu64_production_valid_count="
            << valid_count(capture.cpu64_production_mask) << '\n';
  std::cout << stem << "cpu64_shadow_valid_count="
            << valid_count(capture.cpu64_shadow_mask) << '\n';
  srr2_emit_hash(stem + "input_data_before_hash",
                 capture.input_data_hash_before);
  srr2_emit_hash(stem + "input_data_after_hash", capture.input_data_hash_after);
  srr2_emit_hash(stem + "input_mask_before_hash",
                 capture.input_mask_hash_before);
  srr2_emit_hash(stem + "input_mask_after_hash", capture.input_mask_hash_after);
  std::cout << stem << "target_defined_before=" << capture.target_defined_before
            << '\n';
  std::cout << stem << "target_defined_after=" << capture.target_defined_after
            << '\n';
  srr2_emit_hash(stem + "parameter_before_hash", capture.parameter_hash_before);
  srr2_emit_hash(stem + "parameter_after_hash", capture.parameter_hash_after);
  std::cout << stem << "parameter_max_abs=" << capture.parameter_max_abs
            << '\n';
  srr2_emit_hash(stem + "buffer_before_hash", capture.buffer_hash_before);
  srr2_emit_hash(stem + "buffer_after_hash", capture.buffer_hash_after);
  srr2_emit_hash(stem + "cpu_rng_before_hash", capture.cpu_rng_hash_before);
  srr2_emit_hash(stem + "cpu_rng_after_hash", capture.cpu_rng_hash_after);
  srr2_emit_hash(stem + "cuda_rng_before_hash", capture.cuda_rng_hash_before);
  srr2_emit_hash(stem + "cuda_rng_after_hash", capture.cuda_rng_hash_after);
  std::cout << stem << "training_mode_before=" << capture.mode_before << '\n';
  std::cout << stem << "training_mode_after=" << capture.mode_after << '\n';
  srr2_emit_hash(stem + "encoded_before_production_hash",
                 capture.encoded_hash_before);
  srr2_emit_hash(stem + "encoded_after_production_hash",
                 capture.encoded_hash_between);
  srr2_emit_hash(stem + "encoded_before_shadow_hash",
                 capture.encoded_hash_between);
  srr2_emit_hash(stem + "encoded_after_shadow_hash",
                 capture.encoded_hash_after);
  srr2_emit_hash(stem + "old_selector_before_hash",
                 srr2_combined_hash(capture.sandwich_values_hash_before,
                                    capture.sandwich_mask_hash_before));
  srr2_emit_hash(stem + "old_selector_after_hash",
                 srr2_combined_hash(capture.sandwich_values_hash_after,
                                    capture.sandwich_mask_hash_after));
  srr2_emit_hash(stem + "token_before_hash", capture.tokenizer_hash_before);
  srr2_emit_hash(stem + "token_after_hash", capture.tokenizer_hash_after);
  srr2_emit_hash(stem + "token_before_mask_hash",
                 capture.token_mask_hash_before);
  srr2_emit_hash(stem + "token_after_mask_hash", capture.token_mask_hash_after);
  srr2_emit_hash(stem + "token_before_metadata_hash",
                 capture.token_metadata_hash_before);
  srr2_emit_hash(stem + "token_after_metadata_hash",
                 capture.token_metadata_hash_after);
  srr2_emit_hash(stem + "encoded_token_mask_hash",
                 capture.encoded_token_mask_hash);
  srr2_emit_hash(stem + "encoded_metadata_hash", capture.encoded_metadata_hash);
  srr2_emit_hash(stem + "cpu64_production_mask_hash",
                 capture.cpu64_production_mask_hash);
  srr2_emit_hash(stem + "cpu64_shadow_mask_hash",
                 capture.cpu64_shadow_mask_hash);
  std::cout << stem << "production_finite="
            << (capture.production_finite_count == count(capture.production))
            << '\n';
  std::cout << stem << "shadow_finite="
            << (capture.shadow_finite_count == count(capture.shadow)) << '\n';
  std::cout << stem << "reference_finite="
            << (capture.reference_finite_count == count(capture.reference))
            << '\n';
  std::cout << stem << "cpu64_production_finite="
            << (capture.cpu64_production_finite_count ==
                count(capture.cpu64_production))
            << '\n';
  std::cout << stem << "cpu64_shadow_finite="
            << (capture.cpu64_shadow_finite_count ==
                count(capture.cpu64_shadow))
            << '\n';
  std::cout << stem
            << "production_finite_count=" << capture.production_finite_count
            << '\n';
  std::cout << stem << "shadow_finite_count=" << capture.shadow_finite_count
            << '\n';
  std::cout << stem
            << "reference_finite_count=" << capture.reference_finite_count
            << '\n';
  std::cout << stem << "cpu64_production_finite_count="
            << capture.cpu64_production_finite_count << '\n';
  std::cout << stem
            << "cpu64_shadow_finite_count=" << capture.cpu64_shadow_finite_count
            << '\n';
  std::cout << stem << "production_invalid_zero_exact="
            << capture.production_invalid_zero_exact << '\n';
  std::cout << stem
            << "shadow_invalid_zero_exact=" << capture.shadow_invalid_zero_exact
            << '\n';
  std::cout << stem << "reference_invalid_zero_exact="
            << capture.reference_invalid_zero_exact << '\n';
  std::cout << stem << "cpu64_production_invalid_zero_exact="
            << capture.cpu64_production_invalid_zero_exact << '\n';
  std::cout << stem << "cpu64_shadow_invalid_zero_exact="
            << capture.cpu64_shadow_invalid_zero_exact << '\n';
}

void srr2_emit_preflight_capture(const std::string &prefix,
                                 const Srr2Capture &capture) {
  std::cout << prefix << "rows=" << capture.rows << '\n';
  std::cout << prefix << "batch_size=" << kModelRowBatchSize << '\n';
  std::cout << prefix << "value_count=" << capture.rows * 3 * 32 << '\n';
  std::cout << prefix << "validity_count=" << capture.rows * 3 << '\n';
  std::cout << prefix << "shape=" << capture.rows << ",3,32\n";
  std::cout << prefix << "values_stride=96,32,1\n";
  std::cout << prefix << "mask_shape=" << capture.rows << ",3\n";
  std::cout << prefix << "mask_stride=3,1\n";
  std::cout << prefix << "dtype=float32\n";
  std::cout << prefix << "device=cuda:0\n";
  std::cout << prefix
            << "values_contiguous=" << capture.strides_and_contiguity_exact
            << '\n';
  std::cout << prefix
            << "mask_contiguous=" << capture.strides_and_contiguity_exact
            << '\n';
  std::cout << prefix << "finite=" << capture.finite << '\n';
  std::cout << prefix << "complete=" << capture.complete << '\n';
  std::cout << prefix << "same_encoded_object=" << capture.same_encoded_object
            << '\n';
  std::cout << prefix << "public_selector_sandwich_exact="
            << capture.public_sandwich_exact << '\n';
  std::cout << prefix << "layout_exact=" << capture.layout_exact << '\n';
  std::cout << prefix << "input_unchanged=" << capture.input_unchanged << '\n';
  std::cout << prefix << "parameter_unchanged=" << capture.parameter_unchanged
            << '\n';
  std::cout << prefix << "buffer_unchanged=" << capture.buffer_unchanged
            << '\n';
  std::cout << prefix << "cpu_rng_unchanged=" << capture.cpu_rng_unchanged
            << '\n';
  std::cout << prefix << "cuda_rng_unchanged=" << capture.cuda_rng_unchanged
            << '\n';
  std::cout << prefix << "model_mode_unchanged=" << capture.model_mode_unchanged
            << '\n';
  srr2_emit_hash(prefix + "encoder_hash", capture.encoder_hash);
  srr2_emit_hash(prefix + "served_hash", capture.served_hash);
  srr2_emit_hash(prefix + "metadata_structure_hash",
                 capture.metadata_structure_hash);
  srr2_emit_hash(prefix + "production_hash", capture.production_hash);
  srr2_emit_hash(prefix + "shadow_hash", capture.shadow_hash);
  srr2_emit_hash(prefix + "reference_hash", capture.reference_hash);
  srr2_emit_hash(prefix + "cpu64_production_hash",
                 capture.cpu64_production_hash);
  srr2_emit_hash(prefix + "cpu64_shadow_hash", capture.cpu64_shadow_hash);
  srr2_emit_hash(prefix + "production_mask_hash", capture.production_mask_hash);
  srr2_emit_hash(prefix + "shadow_mask_hash", capture.shadow_mask_hash);
  srr2_emit_hash(prefix + "reference_mask_hash", capture.reference_mask_hash);
  std::cout << prefix << "production_shadow_value_bytes_exact="
            << capture.production_shadow_value_bytes_exact << '\n';
  std::cout << prefix << "production_shadow_mask_bytes_exact="
            << capture.production_shadow_mask_bytes_exact << '\n';
  std::cout << prefix << "cpu64_production_shadow_value_bytes_exact="
            << capture.cpu64_production_shadow_value_bytes_exact << '\n';
  std::cout << prefix << "cpu64_production_shadow_mask_bytes_exact="
            << capture.cpu64_production_shadow_mask_bytes_exact << '\n';
  std::cout << prefix << "cpu64_production_reference_value_bytes_exact="
            << capture.cpu64_production_reference_value_bytes_exact << '\n';
  std::cout << prefix << "cpu64_production_reference_mask_bytes_exact="
            << capture.cpu64_production_reference_mask_bytes_exact << '\n';
  std::cout << prefix << "cpu64_shadow_reference_value_bytes_exact="
            << capture.cpu64_shadow_reference_value_bytes_exact << '\n';
  std::cout << prefix << "cpu64_shadow_reference_mask_bytes_exact="
            << capture.cpu64_shadow_reference_mask_bytes_exact << '\n';
  std::cout << prefix << "production_shadow_device_max_abs="
            << capture.production_shadow_device_max_abs << '\n';
  std::cout << prefix << "cpu64_production_shadow_max_abs="
            << capture.cpu64_production_shadow_max_abs << '\n';
  std::cout << prefix << "cpu64_production_reference_max_abs="
            << capture.cpu64_production_reference_max_abs << '\n';
  std::cout << prefix << "cpu64_shadow_reference_max_abs="
            << capture.cpu64_shadow_reference_max_abs << '\n';
  std::cout << prefix << "device_production_reference_max_abs="
            << capture.device_production_reference_max_abs << '\n';
  std::cout << prefix << "device_shadow_reference_max_abs="
            << capture.device_shadow_reference_max_abs << '\n';
  std::cout << prefix << "invalid_zero_exact=" << capture.invalid_zero_exact
            << '\n';
  srr2_emit_capture_primitives(prefix, capture);
}

[[nodiscard]] std::string srr2_replace_once(std::string source,
                                            const std::string &needle,
                                            const std::string &replacement) {
  const auto position = source.find(needle);
  if (position == std::string::npos ||
      source.find(needle, position + needle.size()) != std::string::npos) {
    throw std::runtime_error("SRR-2 expected exactly one DSL field");
  }
  source.replace(position, needle.size(), replacement);
  return source;
}

struct Srr2CompatibilityReceipt
    : srr2_gate::BackwardCompatibilityInput {
  std::string parsed_device_type{};
  int parsed_device_index{-2};
  std::string active_alias_device{};
  std::string structured_alias_device{};
  std::string active_alias_policy{};
  std::string structured_alias_policy{};
  uint64_t active_manifest_hash{0};
  uint64_t structured_manifest_hash{0};
  uint64_t active_nonpolicy_manifest_hash{0};
  uint64_t structured_nonpolicy_manifest_hash{0};
  std::size_t compatibility_fact_count{0};
  bool setup_complete{false};
};

[[nodiscard]] Srr2CompatibilityReceipt
srr2_compatibility_receipt(const Srr2ScalarRecords &mechanics) {
  const auto attested = [&](std::string_view field) {
    return mechanics.maybe("srr2.compatibility." + std::string(field)) ==
           std::optional<std::string>{"true"};
  };
  Srr2CompatibilityReceipt result{};
  result.legacy_enum_ordinals_exact =
      static_cast<int>(mtf::mtf_serving_pool_policy_t::all_tokens) == 0 &&
      static_cast<int>(mtf::mtf_serving_pool_policy_t::time_only) == 1 &&
      static_cast<int>(mtf::mtf_serving_pool_policy_t::frequency_only) == 2 &&
      static_cast<int>(mtf::mtf_serving_pool_policy_t::domain_balanced) == 3 &&
      attested("legacy_enum_ordinals_exact");
  result.structured_policy_appended =
      static_cast<int>(mtf::mtf_serving_pool_policy_t::structured_cdsb_v1) ==
          4 &&
      attested("structured_policy_appended");
  result.legacy_policy_names_exact =
      std::string_view(mtf::mtf_serving_pool_policy_name(
          mtf::mtf_serving_pool_policy_t::all_tokens)) == "all_tokens" &&
      std::string_view(mtf::mtf_serving_pool_policy_name(
          mtf::mtf_serving_pool_policy_t::time_only)) == "time_only" &&
      std::string_view(mtf::mtf_serving_pool_policy_name(
          mtf::mtf_serving_pool_policy_t::frequency_only)) ==
          "frequency_only" &&
      std::string_view(mtf::mtf_serving_pool_policy_name(
          mtf::mtf_serving_pool_policy_t::domain_balanced)) ==
          "domain_balanced" &&
      attested("legacy_policy_names_exact");
  result.structured_policy_name_exact =
      std::string_view(mtf::mtf_serving_pool_policy_name(
          mtf::mtf_serving_pool_policy_t::structured_cdsb_v1)) ==
          "structured_cdsb_v1" &&
      attested("structured_policy_name_exact");
  result.parser_round_trip_exact =
      mtf::mtf_jepa_mae_vicreg_spec_detail::parse_serving_pool_policy(
          "structured_cdsb_v1") ==
          mtf::mtf_serving_pool_policy_t::structured_cdsb_v1 &&
      attested("parser_round_trip_exact");
  try {
    (void)mtf::mtf_jepa_mae_vicreg_spec_detail::parse_serving_pool_policy(
        "structured-cdsb-v1");
  } catch (const std::exception &) {
    result.unknown_policy_rejected = true;
  }
  result.unknown_policy_rejected =
      result.unknown_policy_rejected && attested("unknown_policy_rejected");
  result.cpp_default_all_tokens =
      mtf::mtf_jepa_mae_vicreg_config_t{}.serving_pool_policy ==
          mtf::mtf_serving_pool_policy_t::all_tokens &&
      attested("cpp_default_all_tokens");

  const std::string dsl = srr2_read_binary_file(
      "src/config/wikimyei.representation.mtf_jepa_mae_vicreg.dsl");
  const std::string net = srr2_read_binary_file(
      "src/config/wikimyei.representation.mtf_jepa_mae_vicreg.net");
  constexpr std::string_view active_field =
      "  SERVING_POOL_POLICY = all_tokens;\n";
  result.active_dsl_all_tokens = dsl.find(active_field) != std::string::npos;
  const auto active =
      mtf::decode_mtf_jepa_mae_vicreg_spec_from_split_dsl(dsl, net);
  const auto omitted = mtf::decode_mtf_jepa_mae_vicreg_spec_from_split_dsl(
      srr2_replace_once(dsl, std::string(active_field), ""), net);
  result.active_dsl_all_tokens =
      result.active_dsl_all_tokens &&
      active.config.serving_pool_policy ==
          mtf::mtf_serving_pool_policy_t::all_tokens &&
      attested("active_dsl_all_tokens");
  result.omitted_dsl_all_tokens =
      omitted.config.serving_pool_policy ==
          mtf::mtf_serving_pool_policy_t::all_tokens &&
      attested("omitted_dsl_all_tokens");
  result.parsed_device_type = active.config.device.is_cuda() ? "cuda" :
                                                               "other";
  result.parsed_device_index = active.config.device.has_index()
                                   ? active.config.device.index()
                                   : -1;
  if (!active.config.device.is_cuda() ||
      (active.config.device.has_index() &&
       active.config.device.index() != 0)) {
    throw std::runtime_error(
        "SRR-2 active DSL device is neither cuda nor cuda:0");
  }
  auto active_config = active.config;
  active_config.device = torch::Device(torch::kCUDA, 0);
  auto structured_config = active_config;
  structured_config.serving_pool_policy =
      mtf::mtf_serving_pool_policy_t::structured_cdsb_v1;

  result.active_alias_device = active_config.device.str();
  result.structured_alias_device = structured_config.device.str();
  result.active_alias_policy =
      mtf::mtf_serving_pool_policy_name(active_config.serving_pool_policy);
  result.structured_alias_policy =
      mtf::mtf_serving_pool_policy_name(
          structured_config.serving_pool_policy);
  const auto active_manifest = canonical_config_manifest(active_config);
  const auto structured_manifest =
      canonical_config_manifest(structured_config);
  auto active_nonpolicy = active_config;
  auto structured_nonpolicy = structured_config;
  active_nonpolicy.serving_pool_policy =
      mtf::mtf_serving_pool_policy_t::all_tokens;
  structured_nonpolicy.serving_pool_policy =
      mtf::mtf_serving_pool_policy_t::all_tokens;
  result.active_manifest_hash = fnv1a64(active_manifest);
  result.structured_manifest_hash = fnv1a64(structured_manifest);
  result.active_nonpolicy_manifest_hash =
      fnv1a64(canonical_config_manifest(active_nonpolicy));
  result.structured_nonpolicy_manifest_hash =
      fnv1a64(canonical_config_manifest(structured_nonpolicy));
  result.compatibility_fact_count = 18;
  result.setup_complete =
      result.parsed_device_type == "cuda" &&
      (result.parsed_device_index == -1 || result.parsed_device_index == 0) &&
      result.active_alias_device == "cuda:0" &&
      result.structured_alias_device == "cuda:0" &&
      result.active_alias_policy == "all_tokens" &&
      result.structured_alias_policy == "structured_cdsb_v1" &&
      result.active_manifest_hash != result.structured_manifest_hash &&
      result.active_manifest_hash == result.active_nonpolicy_manifest_hash &&
      result.active_nonpolicy_manifest_hash ==
          result.structured_nonpolicy_manifest_hash;
  result.protocol_fingerprint_distinct =
      result.active_manifest_hash != result.structured_manifest_hash &&
      attested("protocol_fingerprint_distinct");

  // These facts require filesystem/checkpoint fixtures and the real inference
  // adapter.  The authoritative manifest binds their focused mechanics log;
  // the independent auditor re-parses those records rather than trusting this
  // provisional transport bit.
  result.structured_checkpoint_round_trip_exact =
      attested("structured_checkpoint_round_trip_exact");
  result.legacy_checkpoint_all_tokens =
      attested("legacy_checkpoint_all_tokens");
  result.legacy_checkpoint_does_not_inherit_structured =
      attested("legacy_checkpoint_does_not_inherit_structured");
  result.checkpoint_mismatch_rejected =
      attested("checkpoint_mismatch_rejected");
  result.malformed_checkpoint_rejected =
      attested("malformed_checkpoint_rejected");
  result.legacy_policy_bytes_exact = attested("legacy_policy_bytes_exact");
  result.public_selector_contract_exact =
      attested("public_selector_contract_exact");
  result.adapter_reaches_structured_selector =
      attested("adapter_reaches_structured_selector");
  return result;
}

[[nodiscard]] bool
srr2_compatibility_exact(const srr2_gate::BackwardCompatibilityInput &value) {
  return value.legacy_enum_ordinals_exact && value.legacy_policy_names_exact &&
         value.structured_policy_appended &&
         value.structured_policy_name_exact && value.parser_round_trip_exact &&
         value.unknown_policy_rejected && value.cpp_default_all_tokens &&
         value.omitted_dsl_all_tokens && value.active_dsl_all_tokens &&
         value.protocol_fingerprint_distinct &&
         value.structured_checkpoint_round_trip_exact &&
         value.legacy_checkpoint_all_tokens &&
         value.legacy_checkpoint_does_not_inherit_structured &&
         value.checkpoint_mismatch_rejected &&
         value.malformed_checkpoint_rejected &&
         value.legacy_policy_bytes_exact &&
         value.public_selector_contract_exact &&
         value.adapter_reaches_structured_selector;
}

[[nodiscard]] bool srr2_lower_hex16(const std::string &value) {
  return value.size() == 16 &&
         std::all_of(value.begin(), value.end(), [](char character) {
           return (character >= '0' && character <= '9') ||
                  (character >= 'a' && character <= 'f');
         });
}

[[nodiscard]] bool
srr2_parent_capture_keys_exact(const Srr2ParentReceipt &parent) {
  try {
    for (const int64_t seed : kAttributionSeeds) {
      for (const char *dataset : kSrr2DatasetNames) {
        const std::string prefix =
            "srr.seed_" + std::to_string(seed) + ".capture." + dataset;
        for (const char *suffix :
             {"encoder_hash", "served_hash", "metadata_structure_hash",
              "audit_hash", "shadow_hash"}) {
          if (!srr2_lower_hex16(
                  parent.authoritative.require(prefix + "." + suffix))) {
            return false;
          }
        }
        const std::string data_prefix =
            std::string("srr.data.") + dataset + ".normalized";
        if (!srr2_lower_hex16(
                parent.authoritative.require(data_prefix + ".data_hash")) ||
            !srr2_lower_hex16(
                parent.authoritative.require(data_prefix + ".mask_hash"))) {
          return false;
        }
      }
    }
  } catch (const std::exception &) {
    return false;
  }
  return parent.audit.maybe("audit.all_reference_keys_exact") ==
             std::optional<std::string>{"true"} &&
         parent.audit.maybe("audit.reference_feature_hashes_exact") ==
             std::optional<std::string>{"true"} &&
         parent.audit.maybe("audit.cpu_identities_exact") ==
             std::optional<std::string>{"true"};
}

struct Srr2DatasetIdentityReceipt {
  bool exact{false};
  std::array<uint64_t, 6> data_hashes{};
  std::array<uint64_t, 6> mask_hashes{};
};

[[nodiscard]] Srr2DatasetIdentityReceipt
srr2_dataset_identity_receipt(const std::array<const Dataset *, 6> &datasets,
                              const Srr2ParentReceipt &parent) {
  Srr2DatasetIdentityReceipt result{};
  result.exact = true;
  for (std::size_t index = 0; index < datasets.size(); ++index) {
    const auto &dataset = *datasets[index];
    result.data_hashes[index] = hash_tensor_stable_bytes(dataset.data);
    result.mask_hashes[index] = hash_tensor_stable_bytes(dataset.mask);
    const std::string prefix =
        std::string("srr.data.") + kSrr2DatasetNames[index] + ".normalized";
    result.exact = result.exact && !dataset.target.defined() &&
                   dataset.group_begin == kSrr2DatasetStarts[index] &&
                   dataset.data.size(0) == kSrr2DatasetRows[index] &&
                   srr2_hex64(result.data_hashes[index]) ==
                       parent.authoritative.require(prefix + ".data_hash") &&
                   srr2_hex64(result.mask_hashes[index]) ==
                       parent.authoritative.require(prefix + ".mask_hash");
  }
  return result;
}

struct Srr2CaptureEvidence {
  bool parent_source_hashes_exact{false};
  bool parent_reference_hash_exact{false};
  bool parent_shadow_hash_exact{false};
  bool stable_hashes_exact{false};
  bool repeat_identity_exact{false};
};

[[nodiscard]] Srr2CaptureEvidence
srr2_emit_capture_record(int64_t seed, std::size_t dataset_index,
                         const Dataset &dataset, const Srr2Capture &retained,
                         const Srr2Capture &repeat,
                         const Srr2ParentReceipt &parent) {
  const std::string dataset_name = kSrr2DatasetNames[dataset_index];
  const std::string prefix =
      "srr2.seed_" + std::to_string(seed) + ".capture." + dataset_name;
  const std::string parent_capture =
      "srr.seed_" + std::to_string(seed) + ".capture." + dataset_name;
  const std::string parent_data = "srr.data." + dataset_name + ".normalized";
  const std::string expected_encoder =
      parent.authoritative.require(parent_capture + ".encoder_hash");
  const std::string expected_served =
      parent.authoritative.require(parent_capture + ".served_hash");
  const std::string expected_metadata =
      parent.authoritative.require(parent_capture + ".metadata_structure_hash");
  const std::string expected_reference =
      parent.authoritative.require(parent_capture + ".audit_hash");
  const std::string expected_shadow =
      parent.authoritative.require(parent_capture + ".shadow_hash");
  const std::string expected_input_data =
      parent.authoritative.require(parent_data + ".data_hash");
  const std::string expected_input_mask =
      parent.authoritative.require(parent_data + ".mask_hash");
  const uint64_t input_data_hash = hash_tensor_stable_bytes(dataset.data);
  const uint64_t input_mask_hash = hash_tensor_stable_bytes(dataset.mask);

  Srr2CaptureEvidence evidence{};
  const bool parent_input_hashes_exact =
      srr2_hex64(input_data_hash) == expected_input_data &&
      srr2_hex64(input_mask_hash) == expected_input_mask;
  evidence.parent_source_hashes_exact =
      parent_input_hashes_exact &&
      srr2_hex64(retained.encoder_hash) == expected_encoder &&
      srr2_hex64(retained.served_hash) == expected_served &&
      srr2_hex64(retained.metadata_structure_hash) == expected_metadata;
  evidence.parent_reference_hash_exact =
      srr2_hex64(retained.reference_hash) == expected_reference &&
      srr2_hex64(retained.cpu64_production_hash) == expected_reference &&
      srr2_hex64(retained.cpu64_shadow_hash) == expected_reference;
  evidence.parent_shadow_hash_exact =
      srr2_hex64(retained.production_hash) == expected_shadow &&
      srr2_hex64(retained.shadow_hash) == expected_shadow;
  evidence.repeat_identity_exact =
      srr2_capture_identity_exact(retained, repeat);
  evidence.stable_hashes_exact = evidence.parent_source_hashes_exact &&
                                 evidence.parent_reference_hash_exact &&
                                 evidence.parent_shadow_hash_exact &&
                                 evidence.repeat_identity_exact;

  std::cout << prefix << ".rows=" << retained.rows << '\n';
  std::cout << prefix << ".value_count=" << retained.rows * 3 * 32 << '\n';
  std::cout << prefix << ".validity_count=" << retained.rows * 3 << '\n';
  std::cout << prefix << ".batch_size=" << kModelRowBatchSize << '\n';
  std::cout << prefix << ".group_begin=" << dataset.group_begin << '\n';
  std::cout << prefix << ".reversed=" << (dataset_index >= 3) << '\n';
  std::cout << prefix << ".shape=" << retained.rows << ",3,32\n";
  std::cout << prefix << ".values_stride=96,32,1\n";
  std::cout << prefix << ".mask_shape=" << retained.rows << ",3\n";
  std::cout << prefix << ".mask_stride=3,1\n";
  std::cout << prefix << ".dtype=float32\n";
  std::cout << prefix << ".device=cuda:0\n";
  std::cout << prefix
            << ".values_contiguous=" << retained.strides_and_contiguity_exact
            << '\n';
  std::cout << prefix
            << ".mask_contiguous=" << retained.strides_and_contiguity_exact
            << '\n';
  std::cout << prefix << ".finite=" << retained.finite << '\n';
  std::cout << prefix << ".complete=" << retained.complete << '\n';
  std::cout << prefix << ".same_encoded_object=" << retained.same_encoded_object
            << '\n';
  std::cout << prefix << ".public_selector_sandwich_exact="
            << retained.public_sandwich_exact << '\n';
  std::cout << prefix << ".layout_exact=" << retained.layout_exact << '\n';
  std::cout << prefix << ".input_unchanged=" << retained.input_unchanged
            << '\n';
  std::cout << prefix << ".parameter_unchanged=" << retained.parameter_unchanged
            << '\n';
  std::cout << prefix << ".buffer_unchanged=" << retained.buffer_unchanged
            << '\n';
  std::cout << prefix << ".cpu_rng_unchanged=" << retained.cpu_rng_unchanged
            << '\n';
  std::cout << prefix << ".cuda_rng_unchanged=" << retained.cuda_rng_unchanged
            << '\n';
  std::cout << prefix
            << ".model_mode_unchanged=" << retained.model_mode_unchanged
            << '\n';
  srr2_emit_hash(prefix + ".input_data_hash", input_data_hash);
  srr2_emit_hash(prefix + ".input_mask_hash", input_mask_hash);
  std::cout << prefix << ".parent_input_data_hash=" << expected_input_data
            << '\n';
  std::cout << prefix << ".parent_input_mask_hash=" << expected_input_mask
            << '\n';
  std::cout << prefix
            << ".parent_input_hashes_exact=" << parent_input_hashes_exact
            << '\n';
  srr2_emit_hash(prefix + ".encoder_hash", retained.encoder_hash);
  srr2_emit_hash(prefix + ".served_hash", retained.served_hash);
  srr2_emit_hash(prefix + ".metadata_structure_hash",
                 retained.metadata_structure_hash);
  std::cout << prefix << ".parent_encoder_hash=" << expected_encoder << '\n';
  std::cout << prefix << ".parent_served_hash=" << expected_served << '\n';
  std::cout << prefix << ".parent_reference_hash=" << expected_reference
            << '\n';
  std::cout << prefix << ".parent_shadow_hash=" << expected_shadow << '\n';
  srr2_emit_hash(prefix + ".production_hash", retained.production_hash);
  srr2_emit_hash(prefix + ".shadow_hash", retained.shadow_hash);
  srr2_emit_hash(prefix + ".reference_hash", retained.reference_hash);
  srr2_emit_hash(prefix + ".cpu64_production_hash",
                 retained.cpu64_production_hash);
  srr2_emit_hash(prefix + ".cpu64_shadow_hash", retained.cpu64_shadow_hash);
  srr2_emit_hash(prefix + ".production_mask_hash",
                 retained.production_mask_hash);
  srr2_emit_hash(prefix + ".shadow_mask_hash", retained.shadow_mask_hash);
  srr2_emit_hash(prefix + ".reference_mask_hash", retained.reference_mask_hash);
  std::cout << prefix << ".production_shadow_value_bytes_exact="
            << retained.production_shadow_value_bytes_exact << '\n';
  std::cout << prefix << ".production_shadow_mask_bytes_exact="
            << retained.production_shadow_mask_bytes_exact << '\n';
  std::cout << prefix << ".cpu64_production_shadow_value_bytes_exact="
            << retained.cpu64_production_shadow_value_bytes_exact << '\n';
  std::cout << prefix << ".cpu64_production_shadow_mask_bytes_exact="
            << retained.cpu64_production_shadow_mask_bytes_exact << '\n';
  std::cout << prefix << ".cpu64_production_reference_value_bytes_exact="
            << retained.cpu64_production_reference_value_bytes_exact << '\n';
  std::cout << prefix << ".cpu64_production_reference_mask_bytes_exact="
            << retained.cpu64_production_reference_mask_bytes_exact << '\n';
  std::cout << prefix << ".cpu64_shadow_reference_value_bytes_exact="
            << retained.cpu64_shadow_reference_value_bytes_exact << '\n';
  std::cout << prefix << ".cpu64_shadow_reference_mask_bytes_exact="
            << retained.cpu64_shadow_reference_mask_bytes_exact << '\n';
  std::cout << prefix << ".production_shadow_device_max_abs="
            << retained.production_shadow_device_max_abs << '\n';
  std::cout << prefix << ".cpu64_production_shadow_max_abs="
            << retained.cpu64_production_shadow_max_abs << '\n';
  std::cout << prefix << ".cpu64_production_reference_max_abs="
            << retained.cpu64_production_reference_max_abs << '\n';
  std::cout << prefix << ".cpu64_shadow_reference_max_abs="
            << retained.cpu64_shadow_reference_max_abs << '\n';
  std::cout << prefix << ".device_production_reference_max_abs="
            << retained.device_production_reference_max_abs << '\n';
  std::cout << prefix << ".device_shadow_reference_max_abs="
            << retained.device_shadow_reference_max_abs << '\n';
  std::cout << prefix << ".invalid_zero_exact=" << retained.invalid_zero_exact
            << '\n';
  std::cout << prefix << ".parent_source_hashes_exact="
            << evidence.parent_source_hashes_exact << '\n';
  std::cout << prefix << ".parent_reference_hash_exact="
            << evidence.parent_reference_hash_exact << '\n';
  std::cout << prefix
            << ".parent_shadow_hash_exact=" << evidence.parent_shadow_hash_exact
            << '\n';
  srr2_emit_capture_primitives(prefix + ".", retained);

  std::cout << prefix << ".repeat_rows=" << repeat.rows << '\n';
  std::cout << prefix << ".repeat_value_count=" << repeat.rows * 3 * 32 << '\n';
  std::cout << prefix << ".repeat_validity_count=" << repeat.rows * 3 << '\n';
  std::cout << prefix << ".repeat_batch_size=" << kModelRowBatchSize << '\n';
  std::cout << prefix << ".repeat_shape=" << repeat.rows << ",3,32\n";
  std::cout << prefix << ".repeat_values_stride=96,32,1\n";
  std::cout << prefix << ".repeat_mask_shape=" << repeat.rows << ",3\n";
  std::cout << prefix << ".repeat_mask_stride=3,1\n";
  std::cout << prefix << ".repeat_dtype=float32\n";
  std::cout << prefix << ".repeat_device=cuda:0\n";
  std::cout << prefix << ".repeat_values_contiguous="
            << repeat.strides_and_contiguity_exact << '\n';
  std::cout << prefix
            << ".repeat_mask_contiguous=" << repeat.strides_and_contiguity_exact
            << '\n';
  std::cout << prefix << ".repeat_finite=" << repeat.finite << '\n';
  std::cout << prefix << ".repeat_complete=" << repeat.complete << '\n';
  std::cout << prefix
            << ".repeat_same_encoded_object=" << repeat.same_encoded_object
            << '\n';
  std::cout << prefix << ".repeat_public_selector_sandwich_exact="
            << repeat.public_sandwich_exact << '\n';
  std::cout << prefix << ".repeat_layout_exact=" << repeat.layout_exact << '\n';
  std::cout << prefix << ".repeat_input_unchanged=" << repeat.input_unchanged
            << '\n';
  std::cout << prefix
            << ".repeat_parameter_unchanged=" << repeat.parameter_unchanged
            << '\n';
  std::cout << prefix << ".repeat_buffer_unchanged=" << repeat.buffer_unchanged
            << '\n';
  std::cout << prefix
            << ".repeat_cpu_rng_unchanged=" << repeat.cpu_rng_unchanged << '\n';
  std::cout << prefix
            << ".repeat_cuda_rng_unchanged=" << repeat.cuda_rng_unchanged
            << '\n';
  std::cout << prefix
            << ".repeat_model_mode_unchanged=" << repeat.model_mode_unchanged
            << '\n';
  srr2_emit_hash(prefix + ".repeat_encoder_hash", repeat.encoder_hash);
  srr2_emit_hash(prefix + ".repeat_served_hash", repeat.served_hash);
  srr2_emit_hash(prefix + ".repeat_metadata_structure_hash",
                 repeat.metadata_structure_hash);
  srr2_emit_hash(prefix + ".repeat_production_hash", repeat.production_hash);
  srr2_emit_hash(prefix + ".repeat_shadow_hash", repeat.shadow_hash);
  srr2_emit_hash(prefix + ".repeat_reference_hash", repeat.reference_hash);
  srr2_emit_hash(prefix + ".repeat_cpu64_production_hash",
                 repeat.cpu64_production_hash);
  srr2_emit_hash(prefix + ".repeat_cpu64_shadow_hash",
                 repeat.cpu64_shadow_hash);
  srr2_emit_hash(prefix + ".repeat_production_mask_hash",
                 repeat.production_mask_hash);
  srr2_emit_hash(prefix + ".repeat_shadow_mask_hash", repeat.shadow_mask_hash);
  srr2_emit_hash(prefix + ".repeat_reference_mask_hash",
                 repeat.reference_mask_hash);
  std::cout << prefix << ".repeat_production_shadow_value_bytes_exact="
            << repeat.production_shadow_value_bytes_exact << '\n';
  std::cout << prefix << ".repeat_production_shadow_mask_bytes_exact="
            << repeat.production_shadow_mask_bytes_exact << '\n';
  std::cout << prefix << ".repeat_cpu64_production_shadow_value_bytes_exact="
            << repeat.cpu64_production_shadow_value_bytes_exact << '\n';
  std::cout << prefix << ".repeat_cpu64_production_shadow_mask_bytes_exact="
            << repeat.cpu64_production_shadow_mask_bytes_exact << '\n';
  std::cout << prefix << ".repeat_cpu64_production_reference_value_bytes_exact="
            << repeat.cpu64_production_reference_value_bytes_exact << '\n';
  std::cout << prefix << ".repeat_cpu64_production_reference_mask_bytes_exact="
            << repeat.cpu64_production_reference_mask_bytes_exact << '\n';
  std::cout << prefix << ".repeat_cpu64_shadow_reference_value_bytes_exact="
            << repeat.cpu64_shadow_reference_value_bytes_exact << '\n';
  std::cout << prefix << ".repeat_cpu64_shadow_reference_mask_bytes_exact="
            << repeat.cpu64_shadow_reference_mask_bytes_exact << '\n';
  std::cout << prefix << ".repeat_production_shadow_device_max_abs="
            << repeat.production_shadow_device_max_abs << '\n';
  std::cout << prefix << ".repeat_cpu64_production_shadow_max_abs="
            << repeat.cpu64_production_shadow_max_abs << '\n';
  std::cout << prefix << ".repeat_cpu64_production_reference_max_abs="
            << repeat.cpu64_production_reference_max_abs << '\n';
  std::cout << prefix << ".repeat_cpu64_shadow_reference_max_abs="
            << repeat.cpu64_shadow_reference_max_abs << '\n';
  std::cout << prefix << ".repeat_device_production_reference_max_abs="
            << repeat.device_production_reference_max_abs << '\n';
  std::cout << prefix << ".repeat_device_shadow_reference_max_abs="
            << repeat.device_shadow_reference_max_abs << '\n';
  std::cout << prefix
            << ".repeat_invalid_zero_exact=" << repeat.invalid_zero_exact
            << '\n';
  srr2_emit_capture_primitives(prefix + ".repeat_", repeat);
  std::cout << prefix
            << ".repeat_identity_exact=" << evidence.repeat_identity_exact
            << '\n';
  return evidence;
}

void srr2_emit_forbidden_counters(
    const srr2_gate::ForbiddenCounterInput &value) {
  std::cout << "training_step_count=" << value.training_step_count << '\n';
  std::cout << "optimizer_construction_count="
            << value.optimizer_construction_count << '\n';
  std::cout << "optimizer_step_count=" << value.optimizer_step_count << '\n';
  std::cout << "backward_call_count=" << value.backward_call_count << '\n';
  std::cout << "weight_update_count=" << value.weight_update_count << '\n';
  std::cout << "augmentation_change_count=" << value.augmentation_change_count
            << '\n';
  std::cout << "target_generation_count=" << value.target_generation_count
            << '\n';
  std::cout << "probe_construction_count=" << value.probe_construction_count
            << '\n';
  std::cout << "probe_fit_count=" << value.probe_fit_count << '\n';
  std::cout << "validation_selection_count=" << value.validation_selection_count
            << '\n';
  std::cout << "prediction_count=" << value.prediction_count << '\n';
  std::cout << "permutation_count=" << value.permutation_count << '\n';
  std::cout << "bootstrap_count=" << value.bootstrap_count << '\n';
  std::cout << "downstream_retraining_count="
            << value.downstream_retraining_count << '\n';
  std::cout << "end_to_end_count=" << value.end_to_end_count << '\n';
  std::cout << "deployment_count=" << value.deployment_count << '\n';
}

void srr2_emit_authorizations(const srr2_gate::AuthorizationInput &value) {
  std::cout << "training_authorized=" << value.training_authorized << '\n';
  std::cout << "augmentation_change_authorized="
            << value.augmentation_change_authorized << '\n';
  std::cout << "long_run_authorized=" << value.long_run_authorized << '\n';
  std::cout << "active_policy_change_authorized="
            << value.active_policy_change_authorized << '\n';
  std::cout << "checkpoint_migration_authorized="
            << value.checkpoint_migration_authorized << '\n';
  std::cout << "downstream_retraining_authorized="
            << value.downstream_retraining_authorized << '\n';
  std::cout << "end_to_end_authorized=" << value.end_to_end_authorized << '\n';
  std::cout << "deployment_authorized=" << value.deployment_authorized << '\n';
}

void srr2_emit_mechanics(const srr2_gate::MechanicsInput &value) {
  std::cout << "srr2.mechanics.local_contracts_exact="
            << value.local_contracts_exact << '\n';
  std::cout << "srr2.mechanics.source_boundary_exact="
            << value.source_boundary_exact << '\n';
  std::cout << "srr2.mechanics.command_exact=" << value.command_exact << '\n';
  std::cout << "srr2.mechanics.environment_exact=" << value.environment_exact
            << '\n';
  std::cout << "srr2.mechanics.cuda_available=" << value.cuda_available << '\n';
  std::cout << "srr2.mechanics.attempt_marker_exact="
            << value.attempt_marker_exact << '\n';
  std::cout << "srr2.mechanics.capture_contracts_exact="
            << value.capture_contracts_exact << '\n';
  std::cout << "srr2.mechanics.purity_exact=" << value.purity_exact << '\n';
  std::cout << "srr2.mechanics.finite_outputs_exact="
            << value.finite_outputs_exact << '\n';
  std::cout << "srr2.mechanics.deterministic_execution_exact="
            << value.deterministic_execution_exact << '\n';
  std::cout << "srr2.mechanics.manifest_exact=" << value.manifest_exact << '\n';
  std::cout << "srr2.mechanics.audit_input_exact=" << value.audit_input_exact
            << '\n';
  std::cout << "srr2.mechanics.authoritative_attempt_count="
            << value.authoritative_attempt_count << '\n';
}

void srr2_emit_parent(const srr2_gate::ParentEvidenceInput &value) {
  std::cout << "srr2.parent.artifacts_exact=" << value.artifacts_exact << '\n';
  std::cout << "srr2.parent.hashes_exact=" << value.hashes_exact << '\n';
  std::cout << "srr2.parent.terminal_classification_exact="
            << value.terminal_classification_exact << '\n';
  std::cout << "srr2.parent.audit_pass=" << value.audit_pass << '\n';
  std::cout << "srr2.parent.authorizations_false=" << value.authorizations_false
            << '\n';
  std::cout << "srr2.parent.authoritative_attempt_count="
            << value.authoritative_attempt_count << '\n';
  std::cout << "srr2.parent.audit_error_count=" << value.audit_error_count
            << '\n';
  std::cout << "srr2.parent.optimizer_step_count=" << value.optimizer_step_count
            << '\n';
  std::cout << "srr2.parent.backward_call_count=" << value.backward_call_count
            << '\n';
}

void srr2_emit_compatibility(
    const srr2_gate::BackwardCompatibilityInput &value) {
#define SRR2_EMIT_COMPATIBILITY(field)                                         \
  std::cout << "srr2.compatibility." #field "=" << value.field << '\n'
  SRR2_EMIT_COMPATIBILITY(legacy_enum_ordinals_exact);
  SRR2_EMIT_COMPATIBILITY(legacy_policy_names_exact);
  SRR2_EMIT_COMPATIBILITY(structured_policy_appended);
  SRR2_EMIT_COMPATIBILITY(structured_policy_name_exact);
  SRR2_EMIT_COMPATIBILITY(parser_round_trip_exact);
  SRR2_EMIT_COMPATIBILITY(unknown_policy_rejected);
  SRR2_EMIT_COMPATIBILITY(cpp_default_all_tokens);
  SRR2_EMIT_COMPATIBILITY(omitted_dsl_all_tokens);
  SRR2_EMIT_COMPATIBILITY(active_dsl_all_tokens);
  SRR2_EMIT_COMPATIBILITY(protocol_fingerprint_distinct);
  SRR2_EMIT_COMPATIBILITY(structured_checkpoint_round_trip_exact);
  SRR2_EMIT_COMPATIBILITY(legacy_checkpoint_all_tokens);
  SRR2_EMIT_COMPATIBILITY(legacy_checkpoint_does_not_inherit_structured);
  SRR2_EMIT_COMPATIBILITY(checkpoint_mismatch_rejected);
  SRR2_EMIT_COMPATIBILITY(malformed_checkpoint_rejected);
  SRR2_EMIT_COMPATIBILITY(legacy_policy_bytes_exact);
  SRR2_EMIT_COMPATIBILITY(public_selector_contract_exact);
  SRR2_EMIT_COMPATIBILITY(adapter_reaches_structured_selector);
#undef SRR2_EMIT_COMPATIBILITY
}

void srr2_emit_preflight_compatibility(
    const Srr2CompatibilityReceipt &value) {
  std::cout << "srr2.preflight.compatibility.parsed_device_type="
            << value.parsed_device_type << '\n';
  std::cout << "srr2.preflight.compatibility.parsed_device_index="
            << value.parsed_device_index << '\n';
  std::cout << "srr2.preflight.compatibility.active_alias_device="
            << value.active_alias_device << '\n';
  std::cout << "srr2.preflight.compatibility.structured_alias_device="
            << value.structured_alias_device << '\n';
  std::cout << "srr2.preflight.compatibility.active_alias_policy="
            << value.active_alias_policy << '\n';
  std::cout << "srr2.preflight.compatibility.structured_alias_policy="
            << value.structured_alias_policy << '\n';
  srr2_emit_hash("srr2.preflight.compatibility.active_manifest_hash",
                 value.active_manifest_hash);
  srr2_emit_hash("srr2.preflight.compatibility.structured_manifest_hash",
                 value.structured_manifest_hash);
  srr2_emit_hash(
      "srr2.preflight.compatibility.active_nonpolicy_manifest_hash",
      value.active_nonpolicy_manifest_hash);
  srr2_emit_hash(
      "srr2.preflight.compatibility.structured_nonpolicy_manifest_hash",
      value.structured_nonpolicy_manifest_hash);
  std::cout << "srr2.preflight.compatibility.receipt_fact_count="
            << value.compatibility_fact_count << '\n';
  std::cout << "srr2.preflight.compatibility.setup_complete="
            << value.setup_complete << '\n';
  srr2_emit_compatibility(value);
}

void srr2_emit_sealed_reference(const srr2_gate::SealedReferenceInput &value) {
#define SRR2_EMIT_SEALED(field)                                                \
  std::cout << "srr2.sealed_reference." #field "=" << value.field << '\n'
  SRR2_EMIT_SEALED(archived_base_custody_exact);
  SRR2_EMIT_SEALED(candidate_delta_custody_exact);
  SRR2_EMIT_SEALED(production_shadow_source_independent);
  SRR2_EMIT_SEALED(q0_identity_exact);
  SRR2_EMIT_SEALED(qpsm_identity_exact);
  SRR2_EMIT_SEALED(projection_invariants_exact);
  SRR2_EMIT_SEALED(layout_and_metadata_exact);
  SRR2_EMIT_SEALED(canonical_plan_exact);
  SRR2_EMIT_SEALED(parent_shadow_identities_exact);
  SRR2_EMIT_SEALED(canonical_reference_identity_exact);
  SRR2_EMIT_SEALED(all_reference_keys_exact);
#undef SRR2_EMIT_SEALED
}

void srr2_emit_parity(const srr2_gate::ProductionShadowParityInput &value) {
#define SRR2_EMIT_PARITY(field)                                                \
  std::cout << "srr2.summary.parity." #field "=" << value.field << '\n'
  SRR2_EMIT_PARITY(shape_exact);
  SRR2_EMIT_PARITY(strides_and_contiguity_exact);
  SRR2_EMIT_PARITY(dtype_exact);
  SRR2_EMIT_PARITY(device_exact);
  SRR2_EMIT_PARITY(valid_mask_bytes_exact);
  SRR2_EMIT_PARITY(value_bytes_exact);
  SRR2_EMIT_PARITY(cpu64_valid_mask_bytes_exact);
  SRR2_EMIT_PARITY(cpu64_value_bytes_exact);
  SRR2_EMIT_PARITY(stable_hashes_exact);
  SRR2_EMIT_PARITY(repeat_capture_identity_exact);
  SRR2_EMIT_PARITY(per_capture_coverage_exact);
  SRR2_EMIT_PARITY(coverage_counts_recomputed_from_records);
  SRR2_EMIT_PARITY(cpu64_max_abs);
  SRR2_EMIT_PARITY(device_max_abs);
  SRR2_EMIT_PARITY(seed_count);
  SRR2_EMIT_PARITY(dataset_count);
  SRR2_EMIT_PARITY(retained_capture_count);
  SRR2_EMIT_PARITY(repeat_capture_count);
  SRR2_EMIT_PARITY(retained_row_count);
  SRR2_EMIT_PARITY(repeat_row_count);
  SRR2_EMIT_PARITY(retained_value_count);
  SRR2_EMIT_PARITY(repeat_value_count);
  SRR2_EMIT_PARITY(retained_validity_count);
  SRR2_EMIT_PARITY(repeat_validity_count);
#undef SRR2_EMIT_PARITY
}

void srr2_emit_device_translation(
    const srr2_gate::DeviceTranslationInput &value) {
#define SRR2_EMIT_DEVICE(field)                                                \
  std::cout << "srr2.summary.device_translation." #field "=" << value.field    \
            << '\n'
  SRR2_EMIT_DEVICE(cpu64_reference_shape_exact);
  SRR2_EMIT_DEVICE(cpu64_reference_mask_bytes_exact);
  SRR2_EMIT_DEVICE(cpu64_production_reference_bytes_exact);
  SRR2_EMIT_DEVICE(cpu64_shadow_reference_bytes_exact);
  SRR2_EMIT_DEVICE(device_reference_contract_exact);
  SRR2_EMIT_DEVICE(cpu64_production_reference_max_abs);
  SRR2_EMIT_DEVICE(cpu64_shadow_reference_max_abs);
  SRR2_EMIT_DEVICE(device_production_reference_max_abs);
  SRR2_EMIT_DEVICE(device_shadow_reference_max_abs);
#undef SRR2_EMIT_DEVICE
}

void srr2_emit_quality_transport(
    const srr2_gate::QualityTransportInput &value) {
#define SRR2_EMIT_QUALITY(field)                                               \
  std::cout << "srr2.quality_transport." #field "=" << value.field << '\n'
  SRR2_EMIT_QUALITY(features_and_masks_cover_parent_domain);
  SRR2_EMIT_QUALITY(targets_exact);
  SRR2_EMIT_QUALITY(group_splits_exact);
  SRR2_EMIT_QUALITY(sample_ladder_exact);
  SRR2_EMIT_QUALITY(alpha_grid_exact);
  SRR2_EMIT_QUALITY(standardization_exact);
  SRR2_EMIT_QUALITY(target_centering_exact);
  SRR2_EMIT_QUALITY(fit_and_validation_selection_exact);
  SRR2_EMIT_QUALITY(test_rows_exact);
  SRR2_EMIT_QUALITY(permutations_exact);
  SRR2_EMIT_QUALITY(bootstrap_rows_exact);
  SRR2_EMIT_QUALITY(decision_thresholds_exact);
  SRR2_EMIT_QUALITY(parent_material_gain_over_channel);
  SRR2_EMIT_QUALITY(parent_noninferior_to_encoder);
  SRR2_EMIT_QUALITY(parent_order_decodable);
  SRR2_EMIT_QUALITY(parent_continuous_shuffle_pass);
  SRR2_EMIT_QUALITY(parent_order_shuffle_pass);
  SRR2_EMIT_QUALITY(parent_terminal_reproduced);
#undef SRR2_EMIT_QUALITY
}

void srr2_emit_gate_result(const srr2_gate::GateResult &value) {
#define SRR2_EMIT_GATE(field)                                                  \
  std::cout << "srr2.gate." #field "=" << value.field << '\n'
  SRR2_EMIT_GATE(numeric_inputs_valid);
  SRR2_EMIT_GATE(authorization_boundary_valid);
  SRR2_EMIT_GATE(zero_counters_valid);
  SRR2_EMIT_GATE(mechanics_valid);
  SRR2_EMIT_GATE(parent_evidence_valid);
  SRR2_EMIT_GATE(backward_compatibility_valid);
  SRR2_EMIT_GATE(sealed_reference_valid);
  SRR2_EMIT_GATE(coverage_valid);
  SRR2_EMIT_GATE(production_shadow_parity_valid);
  SRR2_EMIT_GATE(cpu64_reference_valid);
  SRR2_EMIT_GATE(device_translation_valid);
  SRR2_EMIT_GATE(production_readout_gate_valid);
#undef SRR2_EMIT_GATE
  std::cout << "srr2.gate.failure_reason="
            << srr2_gate::failure_reason_name(value.failure_reason) << '\n';
  std::cout << "srr2.gate.classification="
            << srr2_gate::terminal_classification_name(value.classification)
            << '\n';
}

[[nodiscard]] bool srr2_archived_base_exact() {
  const std::filesystem::path path(kSrr2BaselineArchivePath);
  if (!std::filesystem::exists(path) ||
      std::filesystem::file_size(path) != 829440) {
    return false;
  }
  return srr2_digest::sha256_hex(srr2_read_binary_file(path)) ==
         "22f53f452836583f5402d1a28d67c9b3a9ac865094444a06b9726fd0f1c7b6dd";
}

[[nodiscard]] bool srr2_source_boundary_exact() {
  const auto production = srr2_read_binary_file(
      "src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/"
      "mtf_jepa_mae_vicreg.h");
  const auto shadow =
      srr2_read_binary_file("src/tests/bench/wikimyei/representation/encoding/"
                            "mtf_jepa_mae_vicreg/structured_readout_shadow.h");
  return production.find("structured_readout_shadow") == std::string::npos &&
         production.find("test_structured_readout") == std::string::npos &&
         production.find("structured_cdsb_v1") != std::string::npos &&
         shadow.find("structured_cdsb_v1") == std::string::npos &&
         shadow.find("select_mtf_serving_pool(") == std::string::npos &&
         shadow.find("structured_cdsb_v1_make_q0_cpu64") == std::string::npos &&
         shadow.find("structured_cdsb_v1_make_qpsm_cpu64") ==
             std::string::npos &&
         shadow.find("structured_cdsb_v1_projection_for") == std::string::npos;
}

[[nodiscard]] bool srr2_parent_quality_fact(const Srr2ParentReceipt &parent,
                                            const std::string &key,
                                            const std::string &expected) {
  return parent.authoritative.maybe(key) ==
         std::optional<std::string>{expected};
}

void srr2_initialize_aggregate_gate(srr2_gate::GateInput &gate) {
  auto &parity = gate.parity;
  parity.shape_exact = true;
  parity.strides_and_contiguity_exact = true;
  parity.dtype_exact = true;
  parity.device_exact = true;
  parity.valid_mask_bytes_exact = true;
  parity.value_bytes_exact = true;
  parity.cpu64_valid_mask_bytes_exact = true;
  parity.cpu64_value_bytes_exact = true;
  parity.stable_hashes_exact = true;
  parity.repeat_capture_identity_exact = true;
  parity.per_capture_coverage_exact = true;
  parity.coverage_counts_recomputed_from_records = true;
  auto &translation = gate.device_translation;
  translation.cpu64_reference_shape_exact = true;
  translation.cpu64_reference_mask_bytes_exact = true;
  translation.cpu64_production_reference_bytes_exact = true;
  translation.cpu64_shadow_reference_bytes_exact = true;
  translation.device_reference_contract_exact = true;
}

void srr2_accumulate_capture(srr2_gate::GateInput &gate,
                             const Srr2Capture &retained,
                             const Srr2Capture &repeat,
                             const Srr2CaptureEvidence &evidence,
                             int64_t expected_rows) {
  auto &parity = gate.parity;
  parity.shape_exact =
      parity.shape_exact && retained.shape_exact && repeat.shape_exact;
  parity.strides_and_contiguity_exact = parity.strides_and_contiguity_exact &&
                                        retained.strides_and_contiguity_exact &&
                                        repeat.strides_and_contiguity_exact;
  parity.dtype_exact =
      parity.dtype_exact && retained.dtype_exact && repeat.dtype_exact;
  parity.device_exact =
      parity.device_exact && retained.device_exact && repeat.device_exact;
  parity.valid_mask_bytes_exact = parity.valid_mask_bytes_exact &&
                                  retained.production_shadow_mask_bytes_exact &&
                                  repeat.production_shadow_mask_bytes_exact;
  parity.value_bytes_exact = parity.value_bytes_exact &&
                             retained.production_shadow_value_bytes_exact &&
                             repeat.production_shadow_value_bytes_exact;
  parity.cpu64_valid_mask_bytes_exact =
      parity.cpu64_valid_mask_bytes_exact &&
      retained.cpu64_production_shadow_mask_bytes_exact &&
      repeat.cpu64_production_shadow_mask_bytes_exact;
  parity.cpu64_value_bytes_exact =
      parity.cpu64_value_bytes_exact &&
      retained.cpu64_production_shadow_value_bytes_exact &&
      repeat.cpu64_production_shadow_value_bytes_exact;
  parity.stable_hashes_exact =
      parity.stable_hashes_exact && evidence.stable_hashes_exact;
  parity.repeat_capture_identity_exact =
      parity.repeat_capture_identity_exact && evidence.repeat_identity_exact;
  parity.per_capture_coverage_exact = parity.per_capture_coverage_exact &&
                                      retained.complete && repeat.complete &&
                                      retained.rows == expected_rows &&
                                      repeat.rows == expected_rows;
  parity.cpu64_max_abs =
      std::max({parity.cpu64_max_abs, retained.cpu64_production_shadow_max_abs,
                repeat.cpu64_production_shadow_max_abs});
  parity.device_max_abs = std::max({parity.device_max_abs,
                                    retained.production_shadow_device_max_abs,
                                    repeat.production_shadow_device_max_abs});
  ++parity.retained_capture_count;
  ++parity.repeat_capture_count;
  parity.retained_row_count += static_cast<uint64_t>(retained.rows);
  parity.repeat_row_count += static_cast<uint64_t>(repeat.rows);
  parity.retained_value_count +=
      static_cast<uint64_t>(retained.rows * kChannels * kLatentDim);
  parity.repeat_value_count +=
      static_cast<uint64_t>(repeat.rows * kChannels * kLatentDim);
  parity.retained_validity_count +=
      static_cast<uint64_t>(retained.rows * kChannels);
  parity.repeat_validity_count +=
      static_cast<uint64_t>(repeat.rows * kChannels);

  auto &translation = gate.device_translation;
  translation.cpu64_reference_shape_exact =
      translation.cpu64_reference_shape_exact && retained.shape_exact &&
      repeat.shape_exact;
  translation.cpu64_reference_mask_bytes_exact =
      translation.cpu64_reference_mask_bytes_exact &&
      retained.cpu64_production_reference_mask_bytes_exact &&
      retained.cpu64_shadow_reference_mask_bytes_exact &&
      repeat.cpu64_production_reference_mask_bytes_exact &&
      repeat.cpu64_shadow_reference_mask_bytes_exact;
  translation.cpu64_production_reference_bytes_exact =
      translation.cpu64_production_reference_bytes_exact &&
      retained.cpu64_production_reference_value_bytes_exact &&
      repeat.cpu64_production_reference_value_bytes_exact;
  translation.cpu64_shadow_reference_bytes_exact =
      translation.cpu64_shadow_reference_bytes_exact &&
      retained.cpu64_shadow_reference_value_bytes_exact &&
      repeat.cpu64_shadow_reference_value_bytes_exact;
  translation.device_reference_contract_exact =
      translation.device_reference_contract_exact && retained.device_exact &&
      repeat.device_exact && retained.finite && repeat.finite;
  translation.cpu64_production_reference_max_abs =
      std::max({translation.cpu64_production_reference_max_abs,
                retained.cpu64_production_reference_max_abs,
                repeat.cpu64_production_reference_max_abs});
  translation.cpu64_shadow_reference_max_abs =
      std::max({translation.cpu64_shadow_reference_max_abs,
                retained.cpu64_shadow_reference_max_abs,
                repeat.cpu64_shadow_reference_max_abs});
  translation.device_production_reference_max_abs =
      std::max({translation.device_production_reference_max_abs,
                retained.device_production_reference_max_abs,
                repeat.device_production_reference_max_abs});
  translation.device_shadow_reference_max_abs =
      std::max({translation.device_shadow_reference_max_abs,
                retained.device_shadow_reference_max_abs,
                repeat.device_shadow_reference_max_abs});
}

[[nodiscard]] bool srr2_capture_purity_exact(const Srr2Capture &capture) {
  return capture.same_encoded_object && capture.input_unchanged &&
         capture.parameter_unchanged && capture.buffer_unchanged &&
         capture.cpu_rng_unchanged && capture.cuda_rng_unchanged &&
         capture.model_mode_unchanged;
}

[[nodiscard]] bool srr2_capture_reference_exact(const Srr2Capture &capture) {
  return capture.cpu64_production_reference_value_bytes_exact &&
         capture.cpu64_production_reference_mask_bytes_exact &&
         capture.cpu64_shadow_reference_value_bytes_exact &&
         capture.cpu64_shadow_reference_mask_bytes_exact &&
         capture.cpu64_production_reference_max_abs == 0.0 &&
         capture.cpu64_shadow_reference_max_abs == 0.0;
}

[[nodiscard]] int srr2_run_preflight(bool &attempt_consumed) {
  const auto parent = srr2_verify_parent_evidence();
  srr2_configure_determinism();
  const auto environment = srr2_environment_receipt();
  if (!environment.cuda_available) {
    throw std::runtime_error("SRR-2 preflight requires CUDA:0");
  }
  const torch::Device device(torch::kCUDA, 0);
  const auto projection = srr2_projection_receipt();
  const auto layout = psm_token_layout_receipt();
  const auto mechanics_raw =
      srr2_read_binary_file(std::string(kSrr2MechanicsLogPath));
  const auto mechanics_receipt =
      srr2_parse_closed_records(mechanics_raw, false);
  const bool mechanics_receipt_exact =
      srr2_mechanics_receipt_exact(mechanics_receipt);
  const auto compatibility =
      srr2_compatibility_receipt(mechanics_receipt.records);
  const bool compatibility_preboundary_exact =
      mechanics_receipt_exact && compatibility.setup_complete &&
      compatibility.compatibility_fact_count == 18 &&
      srr2_compatibility_exact(compatibility);
  const auto expected_authoritative_keys = srr2_expected_authoritative_keys();
  const auto expected_authoritative_keyset_sha256 =
      srr2_keyset_sha256(expected_authoritative_keys);
  const bool authoritative_schema_frozen =
      expected_authoritative_keys.size() ==
          kSrr2ExpectedAuthoritativeKeyCount &&
      expected_authoritative_keyset_sha256 ==
          kSrr2ExpectedAuthoritativeKeysetSha256;

  auto normalizer = srr2_generate_inputs(4700000, 32);
  auto capture_rows = srr2_generate_inputs(4800000, 101);
  const auto normalization = fit_normalization(normalizer);
  normalize(normalizer, normalization);
  normalize(capture_rows, normalization);
  srr2_validate_inputs(normalizer, 4700000, 32);
  srr2_validate_inputs(capture_rows, 4800000, 101);

  set_paired_rng(17, device);
  auto model = mtf::MtfJepaMaeVicreg(
      attribution_config(device, kJmcdArms[kJmcdCombinedIndex]));
  model->eval();
  const auto retained =
      srr2_capture_once(model, capture_rows, device, projection.qpsm, layout);
  const auto repeat =
      srr2_capture_once(model, capture_rows, device, projection.qpsm, layout);
  const bool repeat_exact = srr2_capture_identity_exact(retained, repeat);
  const bool parity_exact = retained.production_shadow_value_bytes_exact &&
                            retained.production_shadow_mask_bytes_exact &&
                            retained.production_shadow_device_max_abs == 0.0 &&
                            repeat.production_shadow_value_bytes_exact &&
                            repeat.production_shadow_mask_bytes_exact &&
                            repeat.production_shadow_device_max_abs == 0.0;
  const bool cpu64_exact = srr2_capture_reference_exact(retained) &&
                           srr2_capture_reference_exact(repeat);
  const bool translation_exact = retained.device_production_reference_max_abs <=
                                     srr2_gate::kDeviceTranslationTolerance &&
                                 retained.device_shadow_reference_max_abs <=
                                     srr2_gate::kDeviceTranslationTolerance &&
                                 repeat.device_production_reference_max_abs <=
                                     srr2_gate::kDeviceTranslationTolerance &&
                                 repeat.device_shadow_reference_max_abs <=
                                     srr2_gate::kDeviceTranslationTolerance;
  const bool purity_exact =
      srr2_capture_purity_exact(retained) && srr2_capture_purity_exact(repeat);
  const bool zero_forbidden =
      !normalizer.target.defined() && !capture_rows.target.defined();
  const bool pass =
      srr2_parent_receipt_exact(parent) && environment.exact &&
      projection.exact && layout.pass && authoritative_schema_frozen &&
      compatibility_preboundary_exact &&
      retained.complete && repeat.complete && repeat_exact && parity_exact &&
      cpu64_exact && translation_exact && purity_exact && zero_forbidden &&
      srr2_source_boundary_exact() && srr2_archived_base_exact();

  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.srr2_preflight.v2\n";
  std::cout << "experiment=" << kSrr2PreflightExperiment << '\n';
  std::cout << "device=cuda:0\n";
  std::cout << "dtype=float32\n";
  std::cout << "srr2.preflight.manifest_read=false\n";
  std::cout << "srr2.preflight.normalizer_group_begin=4700000\n";
  std::cout << "srr2.preflight.normalizer_count=32\n";
  std::cout << "srr2.preflight.capture_group_begin=4800000\n";
  std::cout << "srr2.preflight.capture_count=101\n";
  std::cout << "srr2.preflight.batch_size=" << kModelRowBatchSize << '\n';
  std::cout << "srr2.preflight.seed=17\n";
  std::cout << "srr2.preflight.target_constructed=false\n";
  std::cout << "srr2.preflight.expected_authoritative_key_count="
            << expected_authoritative_keys.size() << '\n';
  std::cout << "srr2.preflight.expected_authoritative_keyset_sha256="
            << expected_authoritative_keyset_sha256 << '\n';
  srr2_emit_preflight_compatibility(compatibility);
  srr2_emit_environment(environment);
  srr2_emit_projection(projection, layout);
  srr2_emit_preflight_capture("srr2.preflight.retained.", retained);
  srr2_emit_preflight_capture("srr2.preflight.repeat.", repeat);
  std::cout << "srr2.preflight.complete=" << retained.complete << '\n';
  std::cout << "srr2.preflight.repeat_complete=" << repeat.complete << '\n';
  std::cout << "srr2.preflight.same_encoded_object="
            << retained.same_encoded_object << '\n';
  std::cout << "srr2.preflight.public_selector_sandwich_exact="
            << retained.public_sandwich_exact << '\n';
  std::cout << "srr2.preflight.layout_exact=" << retained.layout_exact << '\n';
  std::cout << "srr2.preflight.production_shadow_value_bytes_exact="
            << retained.production_shadow_value_bytes_exact << '\n';
  std::cout << "srr2.preflight.production_shadow_mask_bytes_exact="
            << retained.production_shadow_mask_bytes_exact << '\n';
  std::cout << "srr2.preflight.cpu64_production_reference_bytes_exact="
            << retained.cpu64_production_reference_value_bytes_exact << '\n';
  std::cout << "srr2.preflight.cpu64_shadow_reference_bytes_exact="
            << retained.cpu64_shadow_reference_value_bytes_exact << '\n';
  std::cout << "srr2.preflight.production_shadow_device_max_abs="
            << retained.production_shadow_device_max_abs << '\n';
  std::cout << "srr2.preflight.cpu64_production_reference_max_abs="
            << retained.cpu64_production_reference_max_abs << '\n';
  std::cout << "srr2.preflight.cpu64_shadow_reference_max_abs="
            << retained.cpu64_shadow_reference_max_abs << '\n';
  std::cout << "srr2.preflight.device_production_reference_max_abs="
            << retained.device_production_reference_max_abs << '\n';
  std::cout << "srr2.preflight.device_shadow_reference_max_abs="
            << retained.device_shadow_reference_max_abs << '\n';
  std::cout << "srr2.preflight.input_unchanged=" << retained.input_unchanged
            << '\n';
  std::cout << "srr2.preflight.parameter_unchanged="
            << retained.parameter_unchanged << '\n';
  std::cout << "srr2.preflight.buffer_unchanged=" << retained.buffer_unchanged
            << '\n';
  std::cout << "srr2.preflight.cpu_rng_unchanged=" << retained.cpu_rng_unchanged
            << '\n';
  std::cout << "srr2.preflight.cuda_rng_unchanged="
            << retained.cuda_rng_unchanged << '\n';
  std::cout << "srr2.preflight.model_mode_unchanged="
            << retained.model_mode_unchanged << '\n';
  std::cout << "srr2.preflight.repeat_identity_exact=" << repeat_exact << '\n';
  std::cout << "srr2.preflight.parent_evidence_exact="
            << srr2_parent_receipt_exact(parent) << '\n';
  std::cout << "srr2.preflight.pass=" << pass << '\n';
  std::cout << "srr2.attempt.consumed=false\n";
  std::cout << "authoritative_attempt_count=0\n";
  const srr2_gate::ForbiddenCounterInput counters{};
  const srr2_gate::AuthorizationInput authorizations{};
  srr2_emit_forbidden_counters(counters);
  std::cout.flush();
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
  srr2_emit_authorizations(authorizations);
  std::cout.flush();
  attempt_consumed = false;
  return pass ? 0 : 3;
}

struct Srr2NormalizerReceipt {
  uint64_t data_hash{0};
  uint64_t mask_hash{0};
  uint64_t mean_hash{0};
  uint64_t inv_std_hash{0};
  std::string parent_data_hash{};
  std::string parent_mask_hash{};
  std::string parent_mean_hash{};
  std::string parent_inv_std_hash{};
  bool exact{false};
};

[[nodiscard]] Srr2NormalizerReceipt
srr2_normalizer_receipt(const Dataset &normalizer,
                        const Normalization &normalization,
                        const Srr2ParentReceipt &parent) {
  Srr2NormalizerReceipt result{};
  result.data_hash = hash_tensor_stable_bytes(normalizer.data);
  result.mask_hash = hash_tensor_stable_bytes(normalizer.mask);
  result.mean_hash = hash_tensor_stable_bytes(normalization.mean);
  result.inv_std_hash = hash_tensor_stable_bytes(normalization.inv_std);
  result.parent_data_hash =
      parent.authoritative.require("srr.data.normalizer.normalized.data_hash");
  result.parent_mask_hash =
      parent.authoritative.require("srr.data.normalizer.normalized.mask_hash");
  result.parent_mean_hash =
      parent.authoritative.require("srr.normalization.mean_hash");
  result.parent_inv_std_hash =
      parent.authoritative.require("srr.normalization.inv_std_hash");
  result.exact = !normalizer.target.defined() && normalizer.group_begin == 0 &&
                 normalizer.data.size(0) == 256 &&
                 srr2_hex64(result.data_hash) == result.parent_data_hash &&
                 srr2_hex64(result.mask_hash) == result.parent_mask_hash &&
                 srr2_hex64(result.mean_hash) == result.parent_mean_hash &&
                 srr2_hex64(result.inv_std_hash) == result.parent_inv_std_hash;
  return result;
}

void srr2_emit_normalizer(const Srr2NormalizerReceipt &value) {
  std::cout << "srr2.data.normalizer.group_begin=0\n";
  std::cout << "srr2.data.normalizer.rows=256\n";
  srr2_emit_hash("srr2.data.normalizer.data_hash", value.data_hash);
  srr2_emit_hash("srr2.data.normalizer.mask_hash", value.mask_hash);
  srr2_emit_hash("srr2.data.normalizer.mean_hash", value.mean_hash);
  srr2_emit_hash("srr2.data.normalizer.inv_std_hash", value.inv_std_hash);
  std::cout << "srr2.data.normalizer.parent_data_hash="
            << value.parent_data_hash << '\n';
  std::cout << "srr2.data.normalizer.parent_mask_hash="
            << value.parent_mask_hash << '\n';
  std::cout << "srr2.data.normalizer.parent_mean_hash="
            << value.parent_mean_hash << '\n';
  std::cout << "srr2.data.normalizer.parent_inv_std_hash="
            << value.parent_inv_std_hash << '\n';
  std::cout << "srr2.data.normalizer.exact=" << value.exact << '\n';
}

[[nodiscard]] int srr2_run_authoritative(bool &attempt_consumed) {
  const auto parent_initial = srr2_verify_parent_evidence();
  srr2_configure_determinism();
  const auto environment = srr2_environment_receipt();
  if (!environment.cuda_available) {
    throw std::runtime_error("SRR-2 authoritative mode requires CUDA:0");
  }
  const torch::Device device(torch::kCUDA, 0);
  const auto projection = srr2_projection_receipt();
  const auto layout = psm_token_layout_receipt();
  const auto manifest = srr2_verify_prerun_manifest(
      std::filesystem::path(std::string(kSrr2PrerunManifestPath)),
      layout.layout_hash);
  const auto compatibility =
      srr2_compatibility_receipt(manifest.mechanics_records);
  const bool source_boundary_exact = srr2_source_boundary_exact();
  const bool archived_base_exact = srr2_archived_base_exact();
  const bool parent_keys_exact = srr2_parent_capture_keys_exact(parent_initial);

  auto normalizer = srr2_generate_inputs(0, 256);
  auto probe_train = srr2_generate_inputs(1000000, 256);
  auto probe_validation = srr2_generate_inputs(2000000, 128);
  auto test = srr2_generate_inputs(3000000, 256);
  const auto normalization = fit_normalization(normalizer);
  for (Dataset *dataset :
       {&normalizer, &probe_train, &probe_validation, &test}) {
    normalize(*dataset, normalization);
  }
  srr2_validate_inputs(normalizer, 0, 256);
  srr2_validate_inputs(probe_train, 1000000, 256);
  srr2_validate_inputs(probe_validation, 2000000, 128);
  srr2_validate_inputs(test, 3000000, 256);
  auto reversed_train = srr2_reversed_inputs(probe_train);
  auto reversed_validation = srr2_reversed_inputs(probe_validation);
  auto reversed_test = srr2_reversed_inputs(test);
  srr2_validate_inputs(reversed_train, 1000000, 256);
  srr2_validate_inputs(reversed_validation, 2000000, 128);
  srr2_validate_inputs(reversed_test, 3000000, 256);
  const std::array<const Dataset *, 6> datasets{
      &probe_train,    &probe_validation,    &test,
      &reversed_train, &reversed_validation, &reversed_test};
  const auto dataset_identity =
      srr2_dataset_identity_receipt(datasets, parent_initial);
  const auto normalizer_identity =
      srr2_normalizer_receipt(normalizer, normalization, parent_initial);

  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.srr2.v1\n";
  std::cout << "experiment=" << kSrr2AuthoritativeExperiment << '\n';
  std::cout << "device=cuda:0\n";
  std::cout << "dtype=float32\n";
  std::cout << "authoritative_command=" << kSrr2AuthoritativeCommand << '\n';
  std::cout << "srr2.prerun_manifest.bytes=" << manifest.bytes << '\n';
  std::cout << "srr2.prerun_manifest.sha256=" << manifest.sha256 << '\n';
  std::cout << "srr2.prerun_manifest.entry_count=" << manifest.entry_count
            << '\n';
  std::cout << "srr2.prerun_manifest.entries_exact=" << manifest.entries_exact
            << '\n';
  std::cout << "srr2.prerun_manifest.exact=" << manifest.exact << '\n';
  srr2_emit_environment(environment);
  srr2_emit_projection(projection, layout);
  srr2_emit_normalizer(normalizer_identity);

  srr2_gate::GateInput gate{};
  srr2_initialize_aggregate_gate(gate);
  gate.parent = parent_initial.gate;
  gate.compatibility = compatibility;
  gate.sealed_reference.archived_base_custody_exact = archived_base_exact;
  gate.sealed_reference.candidate_delta_custody_exact =
      manifest.candidate_patch_bound;
  gate.sealed_reference.production_shadow_source_independent =
      source_boundary_exact;
  gate.sealed_reference.q0_identity_exact = projection.q0_exact;
  gate.sealed_reference.qpsm_identity_exact =
      projection.qpsm_exact && projection.production_projection_exact;
  gate.sealed_reference.projection_invariants_exact =
      projection.invariants_exact;
  gate.sealed_reference.layout_and_metadata_exact = layout.pass;
  gate.sealed_reference.canonical_plan_exact =
      layout.pass && std::equal(kSrr2CellVector.begin(), kSrr2CellVector.end(),
                                srr2_shadow::kFrozenCellIds.begin());

  bool all_capture_contracts = true;
  bool all_purity = true;
  bool all_finite = true;
  bool all_repeat = true;
  bool all_layout = true;
  bool all_invalid_zero = true;
  bool all_parent_source_hashes = true;
  bool all_parent_reference_hashes = true;
  bool all_parent_shadow_hashes = true;
  bool all_cpu64_reference = true;
  std::set<int64_t> seen_seeds;
  std::set<std::size_t> seen_datasets;

  bool first_capture = true;
  std::optional<Srr2AttemptLedgerReceipt> attempt_ledger;
  for (const int64_t seed : kAttributionSeeds) {
    seen_seeds.insert(seed);
    set_paired_rng(seed, device);
    auto model = mtf::MtfJepaMaeVicreg(
        attribution_config(device, kJmcdArms[kJmcdCombinedIndex]));
    model->eval();
    const bool model_policy_exact = model->config().serving_pool_policy ==
                                    mtf::mtf_serving_pool_policy_t::all_tokens;
    for (std::size_t dataset_index = 0; dataset_index < datasets.size();
         ++dataset_index) {
      seen_datasets.insert(dataset_index);
      if (first_capture) {
        // Re-authenticate the parent immediately before the attempt boundary.
        // No scientific capture is requested until every prerequisite passes.
        const auto parent_recheck = srr2_verify_parent_evidence();
        gate.parent.artifacts_exact =
            gate.parent.artifacts_exact && parent_recheck.gate.artifacts_exact;
        gate.parent.hashes_exact =
            gate.parent.hashes_exact && parent_recheck.gate.hashes_exact;
        gate.parent.terminal_classification_exact =
            gate.parent.terminal_classification_exact &&
            parent_recheck.gate.terminal_classification_exact;
        gate.parent.audit_pass =
            gate.parent.audit_pass && parent_recheck.gate.audit_pass;
        gate.parent.authorizations_false =
            gate.parent.authorizations_false &&
            parent_recheck.gate.authorizations_false;
        const bool pre_attempt_ready =
            srr2_parent_receipt_exact(parent_initial) &&
            srr2_parent_receipt_exact(parent_recheck) && parent_keys_exact &&
            environment.exact && manifest.exact &&
            srr2_compatibility_exact(compatibility) && projection.exact &&
            layout.pass && source_boundary_exact && archived_base_exact &&
            dataset_identity.exact && normalizer_identity.exact &&
            model_policy_exact;
        if (!pre_attempt_ready) {
          throw std::runtime_error(
              "SRR-2 pre-attempt evidence or mechanics check failed");
        }
        attempt_ledger = srr2_consume_authoritative_attempt(manifest);
        attempt_consumed = true;
        srr2_emit_attempt_ledger(*attempt_ledger);
        std::cout << "srr2.attempt.consumed=true\n" << std::flush;
        first_capture = false;
      }

      const auto retained = srr2_capture_once(model, *datasets[dataset_index],
                                              device, projection.qpsm, layout);
      const auto repeat = srr2_capture_once(model, *datasets[dataset_index],
                                            device, projection.qpsm, layout);
      const auto evidence = srr2_emit_capture_record(
          seed, dataset_index, *datasets[dataset_index], retained, repeat,
          parent_initial);
      srr2_accumulate_capture(gate, retained, repeat, evidence,
                              kSrr2DatasetRows[dataset_index]);
      all_capture_contracts =
          all_capture_contracts && retained.complete && repeat.complete &&
          retained.public_sandwich_exact && repeat.public_sandwich_exact &&
          retained.same_encoded_object && repeat.same_encoded_object;
      all_purity = all_purity && srr2_capture_purity_exact(retained) &&
                   srr2_capture_purity_exact(repeat);
      all_finite = all_finite && retained.finite && repeat.finite;
      all_repeat = all_repeat && evidence.repeat_identity_exact;
      all_layout = all_layout && retained.layout_exact && repeat.layout_exact;
      all_invalid_zero = all_invalid_zero && retained.invalid_zero_exact &&
                         repeat.invalid_zero_exact;
      all_parent_source_hashes =
          all_parent_source_hashes && evidence.parent_source_hashes_exact;
      all_parent_reference_hashes =
          all_parent_reference_hashes && evidence.parent_reference_hash_exact;
      all_parent_shadow_hashes =
          all_parent_shadow_hashes && evidence.parent_shadow_hash_exact;
      all_cpu64_reference = all_cpu64_reference &&
                            srr2_capture_reference_exact(retained) &&
                            srr2_capture_reference_exact(repeat);
    }
  }

  gate.parity.seed_count = static_cast<uint64_t>(seen_seeds.size());
  gate.parity.dataset_count = static_cast<uint64_t>(seen_datasets.size());
  gate.parity.value_bytes_exact =
      gate.parity.value_bytes_exact && all_invalid_zero;
  gate.parity.cpu64_value_bytes_exact =
      gate.parity.cpu64_value_bytes_exact && all_invalid_zero;
  gate.mechanics.local_contracts_exact =
      manifest.mechanics_bound && srr2_compatibility_exact(compatibility) &&
      projection.exact && layout.pass;
  gate.mechanics.source_boundary_exact = source_boundary_exact;
  gate.mechanics.command_exact = true;
  gate.mechanics.environment_exact = environment.exact;
  gate.mechanics.cuda_available = environment.cuda_available;
  const bool attempt_ledger_exact =
      attempt_ledger.has_value() &&
      srr2_attempt_ledger_receipt_exact(*attempt_ledger, manifest);
  gate.mechanics.attempt_marker_exact =
      attempt_consumed && attempt_ledger_exact;
  gate.mechanics.capture_contracts_exact = all_capture_contracts;
  gate.mechanics.purity_exact = all_purity;
  gate.mechanics.finite_outputs_exact = all_finite;
  gate.mechanics.deterministic_execution_exact = all_repeat;
  gate.mechanics.manifest_exact = manifest.exact;
  gate.mechanics.audit_input_exact = attempt_ledger_exact &&
                                     parent_keys_exact &&
                                     gate.parity.retained_capture_count == 18 &&
                                     gate.parity.repeat_capture_count == 18;
  gate.mechanics.authoritative_attempt_count = attempt_consumed ? 1 : 0;

  gate.sealed_reference.layout_and_metadata_exact =
      gate.sealed_reference.layout_and_metadata_exact && all_layout;
  gate.sealed_reference.parent_shadow_identities_exact =
      all_parent_shadow_hashes;
  gate.sealed_reference.canonical_reference_identity_exact =
      all_parent_reference_hashes && all_cpu64_reference;
  gate.sealed_reference.all_reference_keys_exact =
      parent_keys_exact && all_parent_source_hashes &&
      all_parent_reference_hashes && all_parent_shadow_hashes;

  const bool parent_exact = srr2_parent_receipt_exact(parent_initial);
  const bool transported_identity = gate.parity.stable_hashes_exact &&
                                    gate.parity.repeat_capture_identity_exact &&
                                    dataset_identity.exact &&
                                    normalizer_identity.exact;
  auto &quality = gate.quality_transport;
  quality.features_and_masks_cover_parent_domain = transported_identity;
  quality.targets_exact = parent_exact;
  quality.group_splits_exact = dataset_identity.exact;
  quality.sample_ladder_exact = parent_exact;
  quality.alpha_grid_exact = parent_exact;
  quality.standardization_exact = normalizer_identity.exact;
  quality.target_centering_exact = parent_exact;
  quality.fit_and_validation_selection_exact = parent_exact;
  quality.test_rows_exact = dataset_identity.exact;
  quality.permutations_exact = parent_exact;
  quality.bootstrap_rows_exact = parent_exact;
  quality.decision_thresholds_exact = parent_exact;
  quality.parent_material_gain_over_channel = srr2_parent_quality_fact(
      parent_initial,
      "srr.summary.contrast.shadow_minus_channel.classification",
      "material_gain");
  quality.parent_noninferior_to_encoder = srr2_parent_quality_fact(
      parent_initial,
      "srr.summary.contrast.shadow_minus_encoder.classification",
      "noninferior");
  quality.parent_order_decodable = srr2_parent_quality_fact(
      parent_initial, "srr.summary.arm.shadow.order.classification",
      "order_decodable");
  quality.parent_continuous_shuffle_pass = srr2_parent_quality_fact(
      parent_initial, "srr.summary.arm.shadow.continuous_shuffle_pass", "true");
  quality.parent_order_shuffle_pass = srr2_parent_quality_fact(
      parent_initial, "srr.summary.arm.shadow.order_shuffle_pass", "true");
  quality.parent_terminal_reproduced =
      parent_initial.gate.terminal_classification_exact &&
      parent_initial.gate.audit_pass;

  const auto result = srr2_gate::evaluate(gate);
  const bool coverage_exact =
      gate.parity.seed_count == srr2_gate::kExpectedSeedCount &&
      gate.parity.dataset_count == srr2_gate::kExpectedDatasetCount &&
      gate.parity.retained_capture_count ==
          srr2_gate::kExpectedRetainedCaptureCount &&
      gate.parity.repeat_capture_count ==
          srr2_gate::kExpectedRepeatCaptureCount &&
      gate.parity.retained_row_count == srr2_gate::kExpectedRetainedRowCount &&
      gate.parity.repeat_row_count == srr2_gate::kExpectedRepeatRowCount &&
      gate.parity.retained_value_count ==
          srr2_gate::kExpectedRetainedValueCount &&
      gate.parity.repeat_value_count == srr2_gate::kExpectedRepeatValueCount &&
      gate.parity.retained_validity_count ==
          srr2_gate::kExpectedRetainedValidityCount &&
      gate.parity.repeat_validity_count ==
          srr2_gate::kExpectedRepeatValidityCount;

  std::cout << "srr2.coverage.seed_count=" << gate.parity.seed_count << '\n';
  std::cout << "srr2.coverage.batch_size=" << kModelRowBatchSize << '\n';
  std::cout << "srr2.coverage.dataset_count=" << gate.parity.dataset_count
            << '\n';
  std::cout << "srr2.coverage.retained_capture_count="
            << gate.parity.retained_capture_count << '\n';
  std::cout << "srr2.coverage.repeat_capture_count="
            << gate.parity.repeat_capture_count << '\n';
  std::cout << "srr2.coverage.retained_row_count="
            << gate.parity.retained_row_count << '\n';
  std::cout << "srr2.coverage.repeat_row_count=" << gate.parity.repeat_row_count
            << '\n';
  std::cout << "srr2.coverage.retained_value_count="
            << gate.parity.retained_value_count << '\n';
  std::cout << "srr2.coverage.repeat_value_count="
            << gate.parity.repeat_value_count << '\n';
  std::cout << "srr2.coverage.retained_validity_count="
            << gate.parity.retained_validity_count << '\n';
  std::cout << "srr2.coverage.repeat_validity_count="
            << gate.parity.repeat_validity_count << '\n';
  std::cout << "srr2.coverage.counts_recomputed_from_records=true\n";
  std::cout << "srr2.coverage.exact=" << coverage_exact << '\n';
  srr2_emit_forbidden_counters(gate.mechanics.counters);
  srr2_emit_mechanics(gate.mechanics);
  srr2_emit_parent(gate.parent);
  srr2_emit_compatibility(gate.compatibility);
  srr2_emit_sealed_reference(gate.sealed_reference);
  srr2_emit_parity(gate.parity);
  srr2_emit_device_translation(gate.device_translation);
  srr2_emit_quality_transport(gate.quality_transport);
  srr2_emit_gate_result(result);
  std::cout << "authoritative_attempt_count="
            << gate.mechanics.authoritative_attempt_count << '\n';
  std::cout << "failure_reason="
            << srr2_gate::failure_reason_name(result.failure_reason) << '\n';
  std::cout << "terminal_result="
            << srr2_gate::terminal_classification_name(result.classification)
            << '\n';
  // These eight lines are intentionally the terminal authorization boundary.
  std::cout.flush();
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
  srr2_emit_authorizations(gate.mechanics.authorizations);
  std::cout.flush();
  return result.classification ==
                 srr2_gate::TerminalClassification::
                     production_structured_readout_parity_reproduced
             ? 0
             : 3;
}

} // namespace

int main(int argc, char **argv) {
  bool attempt_consumed = false;
  bool attempt_ledger_relevant = false;
  try {
    const auto options = srr2_parse_options(argc, argv);
    attempt_ledger_relevant = !options.preflight;
    return options.preflight ? srr2_run_preflight(attempt_consumed)
                             : srr2_run_authoritative(attempt_consumed);
  } catch (const std::exception &error) {
    if (attempt_ledger_relevant && srr2_attempt_ledger_present_fail_closed()) {
      attempt_consumed = true;
    }
    std::cout.imbue(std::locale::classic());
    std::cout << std::setprecision(17) << std::boolalpha;
    std::cout << "srr2.attempt.consumed=" << attempt_consumed << '\n';
    std::cout << "authoritative_attempt_count=" << (attempt_consumed ? 1 : 0)
              << '\n';
    std::cout << "failure_reason=invalid_mechanics\n";
    std::cout << "terminal_result=invalid_mechanics\n";
    std::cerr << "SRR-2 production structured readout parity failure: "
              << error.what() << '\n';
    const srr2_gate::AuthorizationInput authorizations{};
    std::cerr.flush();
    std::cout.flush();
    cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
    srr2_emit_authorizations(authorizations);
    std::cout.flush();
    return 2;
  }
}
