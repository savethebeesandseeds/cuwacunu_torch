#include "piaabo/digest/sha256.h"

#define main vva1_legacy_embedded_main
#include "quality_wikimyei_mtf_jepa_mae_vicreg_view_augmentation_causal_attribution.cpp"
#undef main

// VVA-1B deliberately embeds the settled VVA/OCA/RMC harness.  It changes
// only the module-owned post-draw view pairing policy and never constructs a
// downstream model.
namespace {

constexpr std::string_view kVva1bProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL.md";
constexpr std::string_view kVva1bProtocolMarkerPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL.sha256";
constexpr std::string_view kVva1bProtocolSha256 =
    "453d8f71f2b5adfd3b724d5fcb80c4841e467ea71e1abb94cbbc2362ae93ef78";
constexpr std::string_view kVva1bProtocolAmendmentA1Path =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A1.md";
constexpr std::string_view kVva1bProtocolAmendmentA1MarkerPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A1.sha256";
constexpr std::string_view kVva1bProtocolAmendmentA1Sha256 =
    "1fb3eaadf0f62745526be5faab2e1539cb499109d5bdb1a0c7392cbcf754410c";
constexpr std::string_view kVva1bProtocolAmendmentA2Path =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A2.md";
constexpr std::string_view kVva1bProtocolAmendmentA2MarkerPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A2.sha256";
constexpr std::string_view kVva1bProtocolAmendmentA2Sha256 =
    "6135c469defecbbe37e60840f59c15a170b9212bf4576baef6949a503c742f07";
constexpr std::string_view kVva1bProtocolAmendmentA3Path =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A3.md";
constexpr std::string_view kVva1bProtocolAmendmentA3MarkerPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A3.sha256";
constexpr std::string_view kVva1bProtocolAmendmentA3Sha256 =
    "cb8a00caee454437aab19016e85c5e3caccf5259f1c5b154e2573ad40fd2c161";
constexpr std::string_view kVva1bSeamAuditPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/VICREG_VIEW_LOSS_BOUNDARY_TRIAD_SEAM_AUDIT.md";
constexpr std::string_view kVva1bSeamAuditMarkerPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/VICREG_VIEW_LOSS_BOUNDARY_TRIAD_SEAM_AUDIT.sha256";
constexpr std::string_view kVva1bSeamAuditSha256 =
    "e4de60edd42dc87013a9bbcf5cdf03960a720d2f43fef4572de09333fbe7cf75";
constexpr std::string_view kVva1bBuildManifestPath =
    ".build/tests/vva1b_build_manifest_v3.txt";
constexpr std::string_view kVva1bBuildManifestMarkerPath =
    ".build/tests/vva1b_build_manifest_v3.txt.sha256";
constexpr std::string_view kVva1bFailedAttemptPath =
    ".build/tests/representation_vva1b_v1_authoritative_failed.40698.0.log";
constexpr std::string_view kVva1bFailedAttemptMarkerPath =
    ".build/tests/representation_vva1b_v1_authoritative_failed.40698.0.log.sha256";
constexpr std::string_view kVva1bFailedAttemptSha256 =
    "a6e44879660e32c21f08f0f70afc542fa60525459c9cc3531c2ce09b3674367c";
constexpr std::string_view kVva1bFailedExecutablePath =
    ".build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_"
    "triad_failed_attempt_v1";
constexpr std::string_view kVva1bFailedExecutableMarkerPath =
    ".build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_"
    "triad_failed_attempt_v1.sha256";
constexpr std::string_view kVva1bFailedExecutableSha256 =
    "fc084d877acad99f879ab030a35c18c95fb179a6747bf6bd1d4758d412131ea4";
constexpr std::string_view kVva1bSharedCertificationPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_representation_module_"
    "certification.cpp";
constexpr std::string_view kVva1bSharedCertificationSha256 =
    "0bed2ce74444a6fe54af743ebaef9d8155424e78dd4fba1179a71fa6ebc995d0";
constexpr std::string_view kVva1bSourcePath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_triad.cpp";
constexpr std::string_view kVva1bModulePath =
    "src/include/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h";
constexpr std::string_view kVva1bModuleSha256 =
    "d52356ba3f1eb8205402e801950f6ca7b3fb0060d5344201434b91f9b2c7cc4b";
constexpr std::string_view kVva1bPreSeamModuleSha256 =
    "93640972e497dc49f37e7690e59c2f2e55f12ece25687fe4e6f6c96b28c3c9ea";
constexpr std::string_view kVva1bOldProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "VICREG_VIEW_AUGMENTATION_CAUSAL_ATTRIBUTION_PROTOCOL.md";
constexpr std::string_view kVva1bOldProtocolSha256 =
    "8e5f3e0aecf9990cb5adb54e090d781cc91b22c5880b8ae622c1fea10fa33616";
constexpr std::string_view kVva1bOldFindingsPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "VICREG_VIEW_AUGMENTATION_CAUSAL_ATTRIBUTION_FINDINGS.md";
constexpr std::string_view kVva1bOldFindingsSha256 =
    "8e651276f444bbafe4f534132245b48b718b16d98de0f31b62a21bec0b6851f0";
constexpr std::string_view kVva1bOldHarnessPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_view_augmentation_causal_"
    "attribution.cpp";
constexpr std::string_view kVva1bOldHarnessSha256 =
    "5b807c5dfd9bb371a40e1ee062c72f0754b817b91158d8cbdfe16ee3b9642d31";
constexpr std::string_view kVva1bOldLogPath =
    ".build/tests/representation_vva1_v1_authoritative.log";
constexpr std::string_view kVva1bOldLogSha256 =
    "d73635a87d96f6d251a8a008b442657066893d3074194bf7f9de055ff61d9d33";
constexpr std::string_view kVva1bIma1Path =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/INTRINSIC_MASK_VIEW_CAUSAL_ATTRIBUTION_FINDINGS.md";
constexpr std::string_view kVva1bIma1Sha256 =
    "ee53b9a97bf1b80153f7fd22ecf5c6dd9857cb0b3dccdb183729e5cfa05854d6";
constexpr std::string_view kVva1bOaa1Path =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/OUTER_AUGMENTATION_CAUSAL_ATTRIBUTION_FINDINGS.md";
constexpr std::string_view kVva1bOaa1Sha256 =
    "42abd19f65f9a41ce50bed1d481ecf983750499a34a6f9d9799e232d7503a9c7";
constexpr std::string_view kVva1bGpv1ProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "GLOBAL_POOL_PROJECTOR_VARIANCE_CAUSAL_DECOMPOSITION_PROTOCOL.md";
constexpr std::string_view kVva1bGpv1ProtocolSha256 =
    "01c6b1d9fcc95a0c831426a481c866cb196f413030d2f3c195b5219d84d57a2a";
constexpr std::string_view kVva1bGpv1Path =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "GLOBAL_POOL_PROJECTOR_VARIANCE_CAUSAL_DECOMPOSITION_FINDINGS.md";
constexpr std::string_view kVva1bGpv1Sha256 =
    "fed02d5efb021d745d0ba72310c634c84be7a2aaf1427d538da9d28560a662d7";
constexpr std::string_view kVva1bGpv1LogPath =
    ".build/tests/representation_gpv1_v2_authoritative.log";
constexpr std::string_view kVva1bGpv1LogSha256 =
    "eb0b8a5821a9aa613ae60508574d10abc18dd37155c2bc68b0ead1d8a68eef27";
constexpr std::string_view kVva1bIma5Path =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/SUPPORT_PERMITTED_TEACHER_TARGET_ALIGNMENT_FINDINGS.md";
constexpr std::string_view kVva1bIma5Sha256 =
    "734aed1fdd289f647c6323e535fa49edddde7efb36064255a85e3c678e0f4299";

constexpr int64_t kVva1bSteps = 512;
constexpr int64_t kVva1bArms = 3;
constexpr int64_t kVva1bTotalUpdates =
    kVva1bSteps * kVva1bArms *
    static_cast<int64_t>(kAttributionSeeds.size());
constexpr int64_t kVva1bUpdatesPerSeed = kVva1bSteps * kVva1bArms;
constexpr int64_t kVva1bFailedAttemptUpdates = kVva1bUpdatesPerSeed;
constexpr double kVva1bLearningRate = 1.0e-3;
constexpr double kVva1bClipNorm = 5.0;
constexpr double kVva1bCausalFloor = 0.005;
constexpr double kVva1bFamilyFloor = -0.02;
constexpr double kVva1bAnchorNoninferiority = -0.005;
constexpr double kVva1bLossAtol = 1.0e-7;
constexpr double kVva1bLossRtol = 1.0e-6;
constexpr double kVva1bZeroTolerance = 1.0e-12;
constexpr double kVva1bProjectionTolerance = 1.0e-7;
constexpr int64_t kVva1bConfirmationGroupBegin = 9000000;
constexpr int64_t kVva1bConfirmationRows = 256;
constexpr std::string_view kVva1bReadoutPolicy =
    "structured_cdsb_sparse_v1";
constexpr std::string_view kVva1bCacheSchema =
    "vva1b.full_triad_4608.interleaved_seed_cache.v3";
constexpr std::array<const char *, 3> kVva1bArmNames{
    "V0_current", "V1_tied_weak", "V2_clean_identical"};
constexpr std::array<mtf::mtf_vicreg_view_pairing_policy_t, 3>
    kVva1bPolicies{
        mtf::mtf_vicreg_view_pairing_policy_t::independent_weak,
        mtf::mtf_vicreg_view_pairing_policy_t::tied_weak,
        mtf::mtf_vicreg_view_pairing_policy_t::clean_identical};
constexpr std::array<std::string_view, 3> kVva1bAnchorSha256{
    "5d96b2961daa2bbd08a07a157ddab5debd9d4928234d8341b3961678327e9434",
    "a85c00d5694d1e3f0063e8bce6fc3c2e3132a393e5bcee195909742754e76775",
    "b9c2f82f26a5516069f8460095d3a2b85c482b7cd0e20db120ce2e0bbd68e392"};
constexpr std::array<std::string_view, 3> kVva1bCurrentCacheSha256{
    "5290559894fa6e3f5d2fd57f32a90e97bb0eec924ec22cc9354ae26d0e629c92",
    "bb5391840ede1ad4aaf8ff760d39b796a7a300918e1aab881fbb5255051fcc39",
    "aaa7dd4b7638f240d1e940db7ede3bc40eb8ad01000baa90dc043801a29256a6"};
constexpr std::array<double, 3> kVva1bFrozenCurrentAulc{
    0.60479711475045383, 0.58888535837559386, 0.64066353570869317};
constexpr std::array<double, 3> kVva1bFrozenAnchorAulc{
    0.62830368194544461, 0.64703100859121065, 0.64931117183778853};
constexpr std::array<uint8_t, 5> kVva1bChallengeMasks{4U, 1U, 2U, 8U,
                                                      15U};
constexpr std::size_t kVva1bCurrentCachePosition = 3;

std::filesystem::path g_vva1b_executable_path{};
bool g_vva1b_preoptimizer_exception_receipt_pending{false};
bool g_vva1b_measurement_active{false};
bool g_vva1b_training_opened{false};
int64_t g_vva1b_current_adam_updates{0};
int64_t g_vva1b_current_ema_updates{0};
int64_t g_vva1b_current_fresh_committed_adam_updates{0};
int64_t g_vva1b_current_fresh_committed_ema_updates{0};
int64_t g_vva1b_validated_replacement_adam_updates{0};
int64_t g_vva1b_validated_replacement_ema_updates{0};
int64_t g_vva1b_recovery_validated_seed_caches{0};

struct Vva1bCustody {
  std::string protocol_sha256{};
  std::string protocol_amendment_a1_sha256{};
  std::string protocol_amendment_a2_sha256{};
  std::string protocol_amendment_a3_sha256{};
  std::string failed_attempt_sha256{};
  std::string seam_audit_sha256{};
  std::string build_manifest_sha256{};
  std::string source_sha256{};
  std::string executable_sha256{};
  bool protocol{false};
  bool protocol_amendment_a1{false};
  bool protocol_amendment_a2{false};
  bool protocol_amendment_a3{false};
  bool failed_attempt{false};
  bool seam_audit{false};
  bool build_manifest{false};
  bool old_protocol{false};
  bool old_findings{false};
  bool old_harness{false};
  bool old_log{false};
  bool settled_fields{false};
  bool ima1{false};
  bool oaa1{false};
  bool gpv1_protocol{false};
  bool gpv1{false};
  bool gpv1_log{false};
  bool ima5{false};
  bool pre_seam_module_bound{false};
  bool post_seam_module{false};
  bool anchors{false};
  bool current_caches{false};
  bool pass{false};
};

struct Vva1bCosine {
  double value{0.0};
  bool active{false};
};

struct Vva1bComponentDiagnostic {
  double raw{0.0};
  double weighted{0.0};
  double tokenizer_gradient_norm{0.0};
  double encoder_gradient_norm{0.0};
  double projector_gradient_norm{0.0};
  torch::Tensor tokenizer_gradient{};
  torch::Tensor encoder_gradient{};
  torch::Tensor projector_gradient{};
  torch::Tensor all_gradient{};
};

struct Vva1bVirtualStepDiagnostic {
  double reversal_before{0.0};
  double reversal_after{0.0};
  double reversal_delta{0.0};
  double channel_separation_before{0.0};
  double channel_separation_after{0.0};
  double channel_separation_delta{0.0};
  double effective_rank_before{0.0};
  double effective_rank_after{0.0};
  double effective_rank_delta{0.0};
  double participation_rank_before{0.0};
  double participation_rank_after{0.0};
  double participation_rank_delta{0.0};
  double order_regime_before{0.0};
  double order_regime_after{0.0};
  double order_regime_delta{0.0};
  double cross_channel_before{0.0};
  double cross_channel_after{0.0};
  double cross_channel_delta{0.0};
  std::array<Geometry, kChannels> geometry_before{};
  std::array<Geometry, kChannels> geometry_after{};
  double parameter_delta{0.0};
  bool parameter_state_exact{false};
  bool optimizer_state_exact{false};
  bool ema_state_exact{false};
  bool active{false};
  bool optimizer_step{false};
  bool ema_step{false};
  bool finite{false};
};

struct Vva1bArmDiagnostic {
  std::array<Vva1bComponentDiagnostic, 3> component{};
  std::array<std::array<Vva1bCosine, 3>, 3> cosine{};
  // Full objective, then invariance, variance, covariance.
  std::array<Vva1bVirtualStepDiagnostic, 4> virtual_step{};
  double total_loss{0.0};
  double reconstruction_abs{0.0};
  double reconstruction_rel{0.0};
  double gradient_reconstruction_max_abs{0.0};
  double gradient_reconstruction_relative_l2{0.0};
  double view_a_to_clean_rms{0.0};
  double view_b_to_clean_rms{0.0};
  double view_to_view_rms{0.0};
  int64_t view_a_mask_difference{0};
  int64_t view_b_mask_difference{0};
  int64_t branch_token_mask_difference{0};
  int64_t branch_sample_valid_difference{0};
  double pooled_max_abs_difference{0.0};
  double projected_max_abs_difference{0.0};
  int64_t projected_a_below_floor{0};
  int64_t projected_b_below_floor{0};
  Geometry projected_geometry{};
  WeakViewDigest drawn_view_hashes{};
  WeakViewDigest used_view_hashes{};
  GeneratorStateDigest pre_rng_hashes{};
  GeneratorStateDigest post_rng_hashes{};
  uint64_t jepa_target_mask_hash{0};
  uint64_t jepa_context_mask_hash{0};
  uint64_t branch_a_token_mask_hash{0};
  uint64_t branch_b_token_mask_hash{0};
  uint64_t branch_a_sample_valid_hash{0};
  uint64_t branch_b_sample_valid_hash{0};
  uint64_t global_validity_mask_hash{0};
  int64_t global_valid_rows{0};
  int64_t encoder_call_count{0};
  int64_t projector_call_count{0};
  bool treatment_exact{false};
  bool finite_zero_masked{false};
  bool separate_forward_graphs{false};
  bool reconstruction_exact{false};
  bool independent_reconstruction_close{false};
  bool gradient_reconstruction_exact{false};
  bool projected_identity{false};
  bool invariance_zero{false};
  bool pass{false};
};

struct Vva1bSeedStage0 {
  int64_t seed{0};
  std::array<Vva1bArmDiagnostic, 3> arm{};
  bool initialization_exact{false};
  bool manifests_exact_except_policy{false};
  bool drawn_views_exact{false};
  bool pre_rng_exact{false};
  bool post_rng_exact{false};
  bool jepa_masks_exact{false};
  bool retained_rows_exact{false};
  bool default_output_exact{false};
  bool default_components_exact{false};
  bool default_loss_exact{false};
  bool default_gradients_exact{false};
  bool default_rng_exact{false};
  bool default_update_exact{false};
  std::array<int64_t, 3> default_parity_update_indices{0, 255, 511};
  std::array<bool, 3> default_parity_pass{false, false, false};
  bool cached_v0_metadata{false};
  bool cached_v0_row_match{false};
  bool cached_v0_loss_match{false};
  bool cached_v0_first_update_receipts_available{false};
  bool cached_v0_clean_replay{false};
  bool cache_reuse{false};
  bool pass{false};
};

struct Vva1bStage0 {
  std::array<Vva1bSeedStage0, 3> seed{};
  bool cpu_self_test{false};
  bool custody{false};
  bool explicit_default_seam_parity{false};
  bool treatment_mechanics{false};
  bool cached_v0_reuse{false};
  std::string cached_v0_reuse_reason{"insufficient_cached_receipts"};
  bool full_triad_4608_required{true};
  bool pass{false};
};

struct Vva1bDataIdentity {
  uint64_t normalization_mean{0};
  uint64_t normalization_inv_std{0};
  std::array<uint64_t, 4> ssl{};
  std::array<uint64_t, 4> fit{};
  std::array<uint64_t, 4> selection{};
  std::array<uint64_t, 4> development{};
  uint64_t bootstrap{0};
  bool confirmation_sealed{false};
  bool pass{false};
};

struct Vva1bSelfTest {
  Vva1bDataIdentity data{};
  bool policy_inventory{false};
  bool default_policy{false};
  bool post_draw_assignment{false};
  bool both_draws_consumed{false};
  bool rng_parity{false};
  bool identical_invariance_zero{false};
  bool identical_invariance_gradient_zero{false};
  bool loss_reconstruction{false};
  bool inactive_cosine{false};
  bool digest_codec{false};
  bool cache_integer_archive_roundtrip{false};
  bool cache_resume_accounting{false};
  bool post_resume_failure_accounting{false};
  int64_t cache_codec_optimizer_updates{0};
  int64_t cache_codec_ema_updates{0};
  bool causal_safety_separated{false};
  bool cache_plan_full_triad{false};
  bool pass{false};
};

struct Vva1bUpdateLedger {
  int64_t current_executed{0};
  int64_t current_fresh_committed{0};
  int64_t validated_replacement{0};
  int64_t current_uncommitted_discarded{0};
  int64_t lifetime_physical{0};
  bool valid{false};
};

[[nodiscard]] Vva1bUpdateLedger vva1b_update_ledger(
    int64_t current_executed, int64_t current_fresh_committed,
    int64_t validated_replacement) {
  Vva1bUpdateLedger result{
      .current_executed = current_executed,
      .current_fresh_committed = current_fresh_committed,
      .validated_replacement = validated_replacement};
  result.valid =
      current_executed >= 0 && current_fresh_committed >= 0 &&
      validated_replacement >= 0 &&
      current_fresh_committed <= current_executed &&
      current_fresh_committed <= validated_replacement &&
      validated_replacement <= kVva1bTotalUpdates &&
      current_fresh_committed % kVva1bUpdatesPerSeed == 0 &&
      validated_replacement % kVva1bUpdatesPerSeed == 0;
  if (!result.valid) {
    return result;
  }
  result.current_uncommitted_discarded =
      current_executed - current_fresh_committed;
  result.lifetime_physical = kVva1bFailedAttemptUpdates +
                             validated_replacement +
                             result.current_uncommitted_discarded;
  return result;
}

struct Vva1bReceipt {
  int64_t steps{0};
  int64_t adam_steps{0};
  int64_t ema_steps{0};
  int64_t clipping_count{0};
  std::vector<uint64_t> row_hashes{};
  std::vector<uint64_t> target_mask_hashes{};
  std::vector<uint64_t> context_mask_hashes{};
  std::vector<uint64_t> branch_a_token_mask_hashes{};
  std::vector<uint64_t> branch_b_token_mask_hashes{};
  std::vector<uint64_t> branch_a_sample_valid_hashes{};
  std::vector<uint64_t> branch_b_sample_valid_hashes{};
  std::vector<uint64_t> global_validity_mask_hashes{};
  std::vector<WeakViewDigest> drawn_view_hashes{};
  std::vector<WeakViewDigest> used_view_hashes{};
  std::vector<GeneratorStateDigest> pre_rng_hashes{};
  std::vector<GeneratorStateDigest> post_rng_hashes{};
  std::vector<double> total_losses{};
  std::array<std::vector<double>, 3> component_losses{};
  std::vector<double> gradient_norms{};
  std::vector<double> served_update_norms{};
  std::vector<double> clip_factors{};
  std::vector<int64_t> global_valid_rows{};
  std::vector<int64_t> encoder_call_counts{};
  std::vector<int64_t> projector_call_counts{};
  double all_trainable_delta{0.0};
  double served_delta{0.0};
  double predictor_delta{0.0};
  double mae_decoder_delta{0.0};
  double vicreg_head_delta{0.0};
  double target_ema_delta{0.0};
  bool initialization_exact{false};
  bool row_schedule_exact{true};
  bool mask_schedule_exact{true};
  bool ordinary_draw_schedule_exact{true};
  bool rng_schedule_exact{true};
  bool treatment_semantics_exact{true};
  bool global_validity_exact{true};
  bool finite{true};
  bool expected_partitions{false};
  bool pass{false};
};

struct Vva1bSeedTraining {
  std::vector<mtf::MtfJepaMaeVicreg> models{};
  std::array<Vva1bReceipt, 3> receipts{};
  bool metadata_exact{false};
  bool interleaving_exact{false};
  bool resumed{false};
  bool pass{false};
};

using Vva1bEvaluations = std::array<std::array<RmcEvaluation, 3>, 3>;
using Vva1bSeedEvaluations = std::array<RmcEvaluation, 3>;
using Vva1bBootstrapAreaTable =
    std::vector<std::array<std::array<double, 3>, 3>>;

struct Vva1bContrast {
  rmc_gate::Contrast summary{};
  std::array<double, 3> per_seed{};
  std::array<double, kFamilies> family{};
  bool mechanism_supported{false};
  bool rescue_sized_ruled_out{false};
  bool family_floor_pass{false};
  bool no_new_safeguard_failure{false};
  bool safe_direct_candidate{false};
};

struct Vva1bSafeguards {
  bool raw_noninferiority{false};
  bool order_point{false};
  bool order_lower{false};
  bool order_retention{false};
  bool continuous_shuffle{false};
  bool order_shuffle{false};
  std::array<std::array<bool, kChannels>, 3> effective{};
  std::array<std::array<bool, kChannels>, 3> participation{};
  std::array<std::array<bool, kChannels>, 3> top{};
  std::array<std::array<bool, kChannels>, 3> active{};
  bool geometry{false};
  bool pass{false};
};

struct Vva1bCandidatePredicates {
  bool candidate_eligible{false};
  bool all_safeguards_pass{false};
  bool objective_made_safe{false};
  bool representation_rescue{false};
};

enum class Vva1bClassification {
  invalid_numeric_or_mechanics,
  representation_rescue,
  objective_made_safe,
  mitigation_only,
  supported_mechanism_without_safe_candidate,
  no_candidate,
};

[[nodiscard]] const char *
vva1b_classification_name(Vva1bClassification value) {
  switch (value) {
  case Vva1bClassification::invalid_numeric_or_mechanics:
    return "invalid_numeric_or_mechanics";
  case Vva1bClassification::representation_rescue:
    return "representation_rescue";
  case Vva1bClassification::objective_made_safe:
    return "objective_made_safe";
  case Vva1bClassification::mitigation_only:
    return "mitigation_only";
  case Vva1bClassification::supported_mechanism_without_safe_candidate:
    return "supported_mechanism_without_safe_candidate";
  case Vva1bClassification::no_candidate:
    return "no_candidate";
  }
  return "invalid_numeric_or_mechanics";
}

[[nodiscard]] std::string vva1b_trim(std::string value) {
  while (!value.empty() &&
         (value.back() == '\n' || value.back() == '\r' ||
          value.back() == ' ' || value.back() == '\t')) {
    value.pop_back();
  }
  return value;
}

[[nodiscard]] bool vva1b_contains(const std::string &text,
                                  std::string_view value) {
  return text.find(value) != std::string::npos;
}

[[nodiscard]] std::string
vva1b_file_sha256(const std::filesystem::path &path) {
  return digest::sha256_hex(rmc_read_file(path));
}

[[nodiscard]] bool vva1b_sidecar_matches(
    const std::filesystem::path &path, std::string_view expected_sha256,
    std::string_view expected_filename) {
  std::istringstream input(rmc_read_file(path));
  std::string digest_value;
  std::string filename;
  std::string trailing;
  return static_cast<bool>(input >> digest_value >> filename) &&
         !(input >> trailing) && digest_value == expected_sha256 &&
         filename == expected_filename;
}

[[nodiscard]] torch::Tensor
vva1b_int64_tensor(std::initializer_list<int64_t> values) {
  return torch::tensor(std::vector<int64_t>(values), torch::kInt64);
}

[[nodiscard]] torch::Tensor vva1b_cache_objective_tensor() {
  return vva1b_int64_tensor({8, 512, 512});
}

[[nodiscard]] torch::Tensor
vva1b_cache_result_flags_tensor(const Vva1bSeedTraining &result) {
  return vva1b_int64_tensor(
      {result.metadata_exact ? int64_t{1} : int64_t{0},
       result.interleaving_exact ? int64_t{1} : int64_t{0},
       result.pass ? int64_t{1} : int64_t{0}});
}

[[nodiscard]] torch::Tensor
vva1b_cache_receipt_flags_tensor(const Vva1bReceipt &receipt) {
  return vva1b_int64_tensor(
      {receipt.steps, receipt.adam_steps, receipt.ema_steps,
       receipt.clipping_count,
       receipt.initialization_exact ? int64_t{1} : int64_t{0},
       receipt.row_schedule_exact ? int64_t{1} : int64_t{0},
       receipt.mask_schedule_exact ? int64_t{1} : int64_t{0},
       receipt.ordinary_draw_schedule_exact ? int64_t{1} : int64_t{0},
       receipt.rng_schedule_exact ? int64_t{1} : int64_t{0},
       receipt.treatment_semantics_exact ? int64_t{1} : int64_t{0},
       receipt.global_validity_exact ? int64_t{1} : int64_t{0},
       receipt.finite ? int64_t{1} : int64_t{0},
       receipt.expected_partitions ? int64_t{1} : int64_t{0},
       receipt.pass ? int64_t{1} : int64_t{0}});
}

[[nodiscard]] bool vva1b_cache_integer_archive_roundtrip() {
  Vva1bSeedTraining result{};
  result.metadata_exact = true;
  result.interleaving_exact = false;
  result.pass = true;
  Vva1bReceipt receipt{};
  receipt.steps = 512;
  receipt.adam_steps = 511;
  receipt.ema_steps = 510;
  receipt.clipping_count = 7;
  receipt.initialization_exact = true;
  receipt.row_schedule_exact = false;
  receipt.mask_schedule_exact = true;
  receipt.ordinary_draw_schedule_exact = false;
  receipt.rng_schedule_exact = true;
  receipt.treatment_semantics_exact = false;
  receipt.global_validity_exact = true;
  receipt.finite = false;
  receipt.expected_partitions = true;
  receipt.pass = false;
  try {
    torch::serialize::OutputArchive output;
    output.write("objective", vva1b_cache_objective_tensor());
    output.write("result_flags", vva1b_cache_result_flags_tensor(result));
    output.write("receipt_flags", vva1b_cache_receipt_flags_tensor(receipt));
    std::stringstream stream(std::ios::in | std::ios::out |
                             std::ios::binary);
    output.save_to(stream);
    stream.seekg(0);
    torch::serialize::InputArchive input;
    input.load_from(stream, torch::Device(torch::kCPU));
    torch::Tensor objective{}, result_flags{}, receipt_flags{};
    input.read("objective", objective);
    input.read("result_flags", result_flags);
    input.read("receipt_flags", receipt_flags);
    const auto exact = [](const torch::Tensor &actual,
                          const torch::Tensor &expected) {
      return actual.device().is_cpu() &&
             actual.scalar_type() == torch::kInt64 && actual.dim() == 1 &&
             actual.sizes() == expected.sizes() && torch::equal(actual, expected);
    };
    return exact(objective, vva1b_int64_tensor({8, 512, 512})) &&
           exact(result_flags, vva1b_int64_tensor({1, 0, 1})) &&
           exact(receipt_flags,
                 vva1b_int64_tensor(
                     {512, 511, 510, 7, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0}));
  } catch (...) {
    return false;
  }
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t
vva1b_config(const torch::Device &device, std::size_t arm) {
  if (arm >= kVva1bPolicies.size()) {
    throw std::runtime_error("VVA-1B arm is invalid");
  }
  auto config = attribution_config(device, oca_arm(8U));
  config.vicreg_view_pairing_policy = kVva1bPolicies[arm];
  return config;
}

[[nodiscard]] std::string
vva1b_common_manifest(const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  std::istringstream input(canonical_config_manifest(config));
  std::ostringstream output;
  std::string line;
  while (std::getline(input, line)) {
    if (line.rfind("vicreg_view_pairing_policy=", 0) == 0) {
      continue;
    }
    output << line << '\n';
  }
  return output.str();
}

[[nodiscard]] bool vva1b_zero_masked(const torch::Tensor &data,
                                     const torch::Tensor &mask) {
  return data.defined() && mask.defined() &&
         torch::isfinite(data).all().item<bool>() &&
         torch::eq(data.masked_select(mask.logical_not()), 0)
             .all()
             .item<bool>();
}

[[nodiscard]] WeakViewDigest vva1b_drawn_digest(
    const mtf::mtf_jepa_mae_vicreg_output_t &output) {
  return {.view_a_data = hash_tensor_stable_bytes(output.vicreg_drawn_a_data),
          .view_a_feature_mask = hash_tensor_stable_bytes(
              output.vicreg_drawn_a_feature_mask),
          .view_b_data = hash_tensor_stable_bytes(output.vicreg_drawn_b_data),
          .view_b_feature_mask = hash_tensor_stable_bytes(
              output.vicreg_drawn_b_feature_mask)};
}

[[nodiscard]] bool vva1b_tensors_exact(
    const mtf::mtf_jepa_mae_vicreg_output_t &left,
    const mtf::mtf_jepa_mae_vicreg_output_t &right) {
  const auto equal = [](const torch::Tensor &lhs, const torch::Tensor &rhs) {
    return lhs.defined() == rhs.defined() &&
           (!lhs.defined() || torch::equal(lhs, rhs));
  };
  return equal(left.embeddings, right.embeddings) &&
         equal(left.pooled_by_channel, right.pooled_by_channel) &&
         equal(left.loss, right.loss) &&
         equal(left.loss_vicreg, right.loss_vicreg) &&
         equal(left.loss_vicreg_global, right.loss_vicreg_global) &&
         equal(left.jepa_target_mask, right.jepa_target_mask) &&
         equal(left.jepa_context_mask, right.jepa_context_mask) &&
         equal(left.vicreg_drawn_a_data, right.vicreg_drawn_a_data) &&
         equal(left.vicreg_drawn_a_feature_mask,
               right.vicreg_drawn_a_feature_mask) &&
         equal(left.vicreg_drawn_b_data, right.vicreg_drawn_b_data) &&
         equal(left.vicreg_drawn_b_feature_mask,
               right.vicreg_drawn_b_feature_mask) &&
         equal(left.vicreg_view_a_data, right.vicreg_view_a_data) &&
         equal(left.vicreg_view_a_feature_mask,
               right.vicreg_view_a_feature_mask) &&
         equal(left.vicreg_view_b_data, right.vicreg_view_b_data) &&
         equal(left.vicreg_view_b_feature_mask,
               right.vicreg_view_b_feature_mask) &&
         equal(left.vicreg_view_a_pooled_global,
               right.vicreg_view_a_pooled_global) &&
         equal(left.vicreg_view_b_pooled_global,
               right.vicreg_view_b_pooled_global) &&
         equal(left.vicreg_view_a_projected_global,
               right.vicreg_view_a_projected_global) &&
         equal(left.vicreg_view_b_projected_global,
               right.vicreg_view_b_projected_global) &&
         equal(left.vicreg_global_joint_mask,
               right.vicreg_global_joint_mask);
}

void vva1b_zero_gradients(mtf::MtfJepaMaeVicreg &model) {
  for (auto &parameter : model->parameters()) {
    if (parameter.grad().defined()) {
      parameter.grad().zero_();
    }
  }
}

[[nodiscard]] bool vva1b_parameter_snapshots_exact(
    const ParameterSnapshot &left, const ParameterSnapshot &right) {
  if (left.names != right.names || left.values.size() != right.values.size()) {
    return false;
  }
  for (std::size_t index = 0; index < left.values.size(); ++index) {
    if (!torch::equal(left.values[index], right.values[index])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] double vva1b_tensor_rms(const torch::Tensor &left,
                                      const torch::Tensor &right) {
  if (left.sizes() != right.sizes()) {
    throw std::runtime_error("VVA-1B RMS tensor shape mismatch");
  }
  return (left.detach().to(torch::kFloat64) -
          right.detach().to(torch::kFloat64))
      .pow(2)
      .mean()
      .sqrt()
      .item<double>();
}

[[nodiscard]] Vva1bCosine vva1b_cosine(const torch::Tensor &left,
                                       const torch::Tensor &right) {
  const double left_norm = left.norm().item<double>();
  const double right_norm = right.norm().item<double>();
  if (!(left_norm > kVva1bZeroTolerance) ||
      !(right_norm > kVva1bZeroTolerance)) {
    return {};
  }
  return {.value = left.dot(right).item<double>() /
                   (left_norm * right_norm),
          .active = true};
}

[[nodiscard]] Vva1bCustody vva1b_validate_custody() {
  Vva1bCustody result{};
  const auto protocol = rmc_read_file(std::filesystem::path(kVva1bProtocolPath));
  result.protocol_sha256 = digest::sha256_hex(protocol);
  const auto amendment_a1 = rmc_read_file(
      std::filesystem::path(kVva1bProtocolAmendmentA1Path));
  result.protocol_amendment_a1_sha256 = digest::sha256_hex(amendment_a1);
  const auto amendment_a2 = rmc_read_file(
      std::filesystem::path(kVva1bProtocolAmendmentA2Path));
  result.protocol_amendment_a2_sha256 = digest::sha256_hex(amendment_a2);
  const auto amendment_a3 = rmc_read_file(
      std::filesystem::path(kVva1bProtocolAmendmentA3Path));
  result.protocol_amendment_a3_sha256 = digest::sha256_hex(amendment_a3);
  const auto failed_attempt =
      rmc_read_file(std::filesystem::path(kVva1bFailedAttemptPath));
  result.failed_attempt_sha256 = digest::sha256_hex(failed_attempt);
  const auto seam_audit =
      rmc_read_file(std::filesystem::path(kVva1bSeamAuditPath));
  result.seam_audit_sha256 = digest::sha256_hex(seam_audit);
  // The independently written frozen sidecar and protocol bytes are both
  // runtime custody anchors; source and executable checksums bind the exact
  // validation logic used for the measurement.
  result.protocol =
      vva1b_sidecar_matches(std::filesystem::path(kVva1bProtocolMarkerPath),
                            kVva1bProtocolSha256,
                            "VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL.md") &&
      result.protocol_sha256 == kVva1bProtocolSha256;
  result.protocol_amendment_a1 =
      vva1b_sidecar_matches(
          std::filesystem::path(kVva1bProtocolAmendmentA1MarkerPath),
          kVva1bProtocolAmendmentA1Sha256,
          "VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A1.md") &&
      result.protocol_amendment_a1_sha256 ==
          kVva1bProtocolAmendmentA1Sha256;
  result.protocol_amendment_a2 =
      vva1b_sidecar_matches(
          std::filesystem::path(kVva1bProtocolAmendmentA2MarkerPath),
          kVva1bProtocolAmendmentA2Sha256,
          "VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A2.md") &&
      result.protocol_amendment_a2_sha256 ==
          kVva1bProtocolAmendmentA2Sha256;
  result.protocol_amendment_a3 =
      vva1b_sidecar_matches(
          std::filesystem::path(kVva1bProtocolAmendmentA3MarkerPath),
          kVva1bProtocolAmendmentA3Sha256,
          "VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A3.md") &&
      result.protocol_amendment_a3_sha256 ==
          kVva1bProtocolAmendmentA3Sha256;
  result.failed_attempt =
      vva1b_sidecar_matches(
          std::filesystem::path(kVva1bFailedAttemptMarkerPath),
          kVva1bFailedAttemptSha256,
          "representation_vva1b_v1_authoritative_failed.40698.0.log") &&
      result.failed_attempt_sha256 == kVva1bFailedAttemptSha256 &&
      vva1b_contains(failed_attempt, "vva1b.training.opened=true") &&
      vva1b_contains(
          failed_attempt,
          "vva1b.training.seed_17.completed_steps=512") &&
      vva1b_contains(
          failed_attempt,
          "Expected all elements of the tensor to have the same scalar type: "
          "Long, but got element of scalar type: Int") &&
      !vva1b_contains(failed_attempt,
                      "execution_status=vva1b_measurements_complete") &&
      vva1b_sidecar_matches(
          std::filesystem::path(kVva1bFailedExecutableMarkerPath),
          kVva1bFailedExecutableSha256,
          "quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_triad_"
          "failed_attempt_v1") &&
      vva1b_file_sha256(std::filesystem::path(kVva1bFailedExecutablePath)) ==
          kVva1bFailedExecutableSha256;
  result.seam_audit =
      vva1b_sidecar_matches(
          std::filesystem::path(kVva1bSeamAuditMarkerPath),
          kVva1bSeamAuditSha256,
          "VICREG_VIEW_LOSS_BOUNDARY_TRIAD_SEAM_AUDIT.md") &&
      result.seam_audit_sha256 == kVva1bSeamAuditSha256 &&
      vva1b_contains(seam_audit,
                     "audit_schema=vva1b.authorized_seam_audit.v1") &&
      vva1b_contains(seam_audit,
                     "pre_seam_header_sha256=" +
                         std::string(kVva1bPreSeamModuleSha256)) &&
      vva1b_contains(seam_audit,
                     "post_seam_header_sha256=" +
                         std::string(kVva1bModuleSha256)) &&
      vva1b_contains(seam_audit, "isolated_contract_runtime=PASS") &&
      vva1b_contains(seam_audit, "only_authorized_changes=true") &&
      vva1b_contains(seam_audit, "parameter_change=false") &&
      vva1b_contains(seam_audit,
                     "production_default_semantic_change=false") &&
      vva1b_contains(seam_audit, "pool_definition_change=false") &&
      vva1b_contains(seam_audit, "projector_definition_change=false") &&
      vva1b_contains(seam_audit, "loss_definition_change=false") &&
      vva1b_contains(seam_audit, "validity_definition_change=false") &&
      vva1b_contains(seam_audit, "optimizer_change=false");
  result.source_sha256 =
      vva1b_file_sha256(std::filesystem::path(kVva1bSourcePath));
  result.executable_sha256 =
      g_vva1b_executable_path.empty()
          ? "unresolved"
          : vva1b_file_sha256(g_vva1b_executable_path);
  const auto build_manifest =
      rmc_read_file(std::filesystem::path(kVva1bBuildManifestPath));
  result.build_manifest_sha256 = digest::sha256_hex(build_manifest);
  const auto repo_record = [&](std::string_view path,
                               std::string_view sha256) {
    return vva1b_contains(
        build_manifest,
        "repo_dependency:${REPO}/" + std::string(path) + "=" +
            std::string(sha256));
  };
  result.build_manifest =
      vva1b_sidecar_matches(
          std::filesystem::path(kVva1bBuildManifestMarkerPath),
          result.build_manifest_sha256, "vva1b_build_manifest_v3.txt") &&
      vva1b_contains(build_manifest,
                     "schema=vva1b.transitive_build_manifest.v1") &&
      repo_record(kVva1bSourcePath, result.source_sha256) &&
      repo_record(kVva1bModulePath, kVva1bModuleSha256) &&
      repo_record(kVva1bSharedCertificationPath,
                  kVva1bSharedCertificationSha256) &&
      repo_record(kVva1bProtocolPath, kVva1bProtocolSha256) &&
      repo_record(kVva1bProtocolAmendmentA1Path,
                  kVva1bProtocolAmendmentA1Sha256) &&
      repo_record(kVva1bProtocolAmendmentA2Path,
                  kVva1bProtocolAmendmentA2Sha256) &&
      repo_record(kVva1bProtocolAmendmentA3Path,
                  kVva1bProtocolAmendmentA3Sha256) &&
      repo_record(kVva1bFailedAttemptPath, kVva1bFailedAttemptSha256) &&
      repo_record(kVva1bFailedExecutablePath,
                  kVva1bFailedExecutableSha256) &&
      repo_record(kVva1bSeamAuditPath, kVva1bSeamAuditSha256) &&
      vva1b_contains(
          build_manifest,
          "repo_dependency:${REPO}/src/tests/bench/wikimyei/representation/"
          "encoding/mtf_jepa_mae_vicreg/Makefile=") &&
      vva1b_contains(
          build_manifest,
          "repo_dependency:${REPO}/src/tests/bench/wikimyei/representation/"
          "encoding/mtf_jepa_mae_vicreg/write_vva1b_build_manifest.sh=") &&
      vva1b_contains(
          build_manifest,
          "binary:${REPO}/.build/tests/quality_wikimyei_mtf_jepa_mae_"
          "vicreg_view_loss_boundary_triad=" + result.executable_sha256) &&
      vva1b_contains(build_manifest, "repo_dependency.count=") &&
      vva1b_contains(build_manifest, "dso.count=");
  const auto old_protocol =
      rmc_read_file(std::filesystem::path(kVva1bOldProtocolPath));
  const auto old_findings =
      rmc_read_file(std::filesystem::path(kVva1bOldFindingsPath));
  const auto old_harness =
      rmc_read_file(std::filesystem::path(kVva1bOldHarnessPath));
  const auto old_log = rmc_read_file(std::filesystem::path(kVva1bOldLogPath));
  const auto ima1 = rmc_read_file(std::filesystem::path(kVva1bIma1Path));
  const auto oaa1 = rmc_read_file(std::filesystem::path(kVva1bOaa1Path));
  const auto gpv1_protocol =
      rmc_read_file(std::filesystem::path(kVva1bGpv1ProtocolPath));
  const auto gpv1 = rmc_read_file(std::filesystem::path(kVva1bGpv1Path));
  const auto gpv1_log =
      rmc_read_file(std::filesystem::path(kVva1bGpv1LogPath));
  const auto ima5 = rmc_read_file(std::filesystem::path(kVva1bIma5Path));
  result.old_protocol = digest::sha256_hex(old_protocol) ==
                        kVva1bOldProtocolSha256;
  result.old_findings = digest::sha256_hex(old_findings) ==
                        kVva1bOldFindingsSha256;
  result.old_harness = digest::sha256_hex(old_harness) ==
                       kVva1bOldHarnessSha256;
  result.old_log = digest::sha256_hex(old_log) == kVva1bOldLogSha256;
  result.ima1 = digest::sha256_hex(ima1) == kVva1bIma1Sha256;
  result.oaa1 = digest::sha256_hex(oaa1) == kVva1bOaa1Sha256;
  result.gpv1_protocol =
      digest::sha256_hex(gpv1_protocol) == kVva1bGpv1ProtocolSha256;
  result.gpv1 = digest::sha256_hex(gpv1) == kVva1bGpv1Sha256;
  result.gpv1_log =
      digest::sha256_hex(gpv1_log) == kVva1bGpv1LogSha256;
  result.ima5 = digest::sha256_hex(ima5) == kVva1bIma5Sha256;
  result.settled_fields =
      vva1b_contains(old_log, "vva1.custody.pass=true") &&
      vva1b_contains(old_log, "execution_status=vva1_measurements_complete") &&
      vva1b_contains(old_log, "vva1.confirmation.opened=false") &&
      vva1b_contains(old_log, "vva1.confirmation.rows=0") &&
      vva1b_contains(old_findings, "clean-identical-view model") &&
      vva1b_contains(ima1, "V1`, tied weak view") &&
      vva1b_contains(oaa1, "did not rescue VICReg") &&
      vva1b_contains(gpv1, "current global pool") &&
      vva1b_contains(gpv1_log, "gpv1.confirmation.opened=false") &&
      vva1b_contains(gpv1_log, "gpv1.confirmation.optimizer_updates=0") &&
      vva1b_contains(gpv1_log, "gpv1.development.selected=none") &&
      vva1b_contains(gpv1_log, "gpv1.production_defaults_changed=false") &&
      vva1b_contains(gpv1_log, "execution_status=gpv1_measurements_complete") &&
      vva1b_contains(ima5, "jepa_branch_closed_time_target_noncollapse") &&
      vva1b_contains(ima5, "No target was admitted") &&
      vva1b_contains(ima5, "IMA-5B is forbidden and was not run") &&
      vva1b_contains(
          ima5,
          "IMA-5 does not prove that JEPA is a bad representation method") &&
      vva1b_contains(ima5, "held-out group-level uncertainty") &&
      vva1b_contains(ima5, "contain their arithmetic mean");
  result.pre_seam_module_bound =
      result.old_harness && result.old_log &&
      vva1b_contains(old_harness, kVva1bPreSeamModuleSha256) &&
      vva1b_contains(old_log, "vva1.custody.module=true");
  result.post_seam_module =
      vva1b_file_sha256(std::filesystem::path(kVva1bModulePath)) ==
      kVva1bModuleSha256;
  result.anchors = true;
  result.current_caches = true;
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    result.anchors =
        result.anchors &&
        vva1b_file_sha256(oca_archive_path(kAttributionSeeds[seed])) ==
            kVva1bAnchorSha256[seed];
    result.current_caches =
        result.current_caches &&
        vva1b_file_sha256(oca_seed_cache_path(
            "anchor_challenge", kAttributionSeeds[seed])) ==
            kVva1bCurrentCacheSha256[seed];
  }
  result.pass = result.protocol && result.protocol_amendment_a1 &&
                result.protocol_amendment_a2 &&
                result.protocol_amendment_a3 && result.failed_attempt &&
                result.seam_audit &&
                result.build_manifest &&
                result.old_protocol &&
                result.old_findings && result.old_harness && result.old_log &&
                result.settled_fields && result.ima1 && result.oaa1 &&
                result.gpv1_protocol && result.gpv1 && result.gpv1_log &&
                result.ima5 && result.pre_seam_module_bound &&
                result.post_seam_module && result.anchors &&
                result.current_caches && result.source_sha256.size() == 64 &&
                result.executable_sha256.size() == 64;
  return result;
}

[[nodiscard]] std::array<uint64_t, 4>
vva1b_dataset_identity(const Dataset &value) {
  return {hash_tensor_stable_bytes(rssm_group_ids(value)),
          hash_tensor_stable_bytes(value.data),
          hash_tensor_stable_bytes(value.mask),
          hash_tensor_stable_bytes(value.target)};
}

[[nodiscard]] Vva1bDataIdentity
vva1b_data_identity(const RmcData &data,
                    const std::vector<torch::Tensor> &bootstrap_rows) {
  Vva1bDataIdentity result{};
  result.normalization_mean =
      hash_tensor_stable_bytes(data.normalization.mean);
  result.normalization_inv_std =
      hash_tensor_stable_bytes(data.normalization.inv_std);
  result.ssl = vva1b_dataset_identity(data.ssl);
  result.fit = vva1b_dataset_identity(data.probe_train);
  result.selection = vva1b_dataset_identity(data.probe_validation);
  result.development = vva1b_dataset_identity(data.development);
  result.bootstrap = rssm_tensor_vector_hash(bootstrap_rows);
  result.confirmation_sealed = !data.confirmation.data.defined();
  result.pass =
      result.normalization_mean == 0xebf16130302b08d1ULL &&
      result.normalization_inv_std == 0x5119e32115e11e61ULL &&
      result.ssl == std::array<uint64_t, 4>{
                        0x33f4cd8e310fc6e2ULL, 0xf0249ea4c1ab0bd8ULL,
                        0x68c14e56f93e72b6ULL, 0x1500317bf51b7d5aULL} &&
      result.fit == std::array<uint64_t, 4>{
                        0x974760d5271987e2ULL, 0x3541ed3b5052ec33ULL,
                        0x68c14e56f93e72b6ULL, 0x98b9551af0036ecbULL} &&
      result.selection == std::array<uint64_t, 4>{
                              0x1625dc5d75ab0162ULL,
                              0xced27c3675d6abfcULL,
                              0x7e6e36e18d0f4936ULL,
                              0x0f6ccf650380cf7fULL} &&
      result.development == std::array<uint64_t, 4>{
                                0xca87fc6f1c72f9e2ULL,
                                0x6abf9f0719e31621ULL,
                                0x68c14e56f93e72b6ULL,
                                0x286cb42ec5dc8460ULL} &&
      result.bootstrap == 0x408205cac33d403dULL &&
      result.confirmation_sealed &&
      rssm_bootstrap_contract(bootstrap_rows, 256);
  return result;
}

[[nodiscard]] Vva1bSelfTest vva1b_cpu_self_test() {
  Vva1bSelfTest result{};
  auto data = rmc_make_data();
  const auto bootstrap_rows = rssm_bootstrap_rows(256);
  result.data = vva1b_data_identity(data, bootstrap_rows);
  const torch::Device device(torch::kCPU);
  auto config = active_config(device, /*weak_views=*/true);
  result.policy_inventory =
      std::string(mtf::mtf_vicreg_view_pairing_policy_name(
          kVva1bPolicies[0])) == "independent_weak" &&
      std::string(mtf::mtf_vicreg_view_pairing_policy_name(
          kVva1bPolicies[1])) == "tied_weak" &&
      std::string(mtf::mtf_vicreg_view_pairing_policy_name(
          kVva1bPolicies[2])) == "clean_identical";
  result.default_policy =
      config.vicreg_view_pairing_policy == kVva1bPolicies[0];

  const auto input = data.ssl.data.narrow(0, 0, 8).to(device);
  const auto feature_mask = data.ssl.mask.narrow(0, 0, 8).to(device);
  const auto clean =
      mtf::detail::canonicalize_input(input, feature_mask, config);
  std::array<mtf::mtf_input_t, 3> used_a{};
  std::array<mtf::mtf_input_t, 3> used_b{};
  std::array<WeakViewDigest, 3> drawn{};
  std::array<GeneratorStateSnapshot, 3> post{};
  for (std::size_t arm = 0; arm < 3; ++arm) {
    set_paired_rng(170031, device);
    const auto a = mtf::detail::apply_vicreg_weak_view_augmentation(
        input, feature_mask, config);
    const auto b = mtf::detail::apply_vicreg_weak_view_augmentation(
        input, feature_mask, config);
    drawn[arm] = WeakViewDigest{
        .view_a_data = hash_tensor_stable_bytes(a.data),
        .view_a_feature_mask = hash_tensor_stable_bytes(a.feature_mask),
        .view_b_data = hash_tensor_stable_bytes(b.data),
        .view_b_feature_mask = hash_tensor_stable_bytes(b.feature_mask)};
    if (arm == 0) {
      used_a[arm] = a;
      used_b[arm] = b;
    } else if (arm == 1) {
      used_a[arm] = a;
      used_b[arm] =
          mtf::mtf_input_t{a.data.clone(), a.feature_mask.clone()};
    } else {
      used_a[arm] = clean;
      used_b[arm] = mtf::mtf_input_t{clean.data.clone(),
                                     clean.feature_mask.clone()};
    }
    post[arm] = current_generator_state_snapshot(device);
  }
  result.post_draw_assignment =
      hash_tensor_stable_bytes(used_a[0].data) == drawn[0].view_a_data &&
      hash_tensor_stable_bytes(used_a[0].feature_mask) ==
          drawn[0].view_a_feature_mask &&
      hash_tensor_stable_bytes(used_b[0].data) == drawn[0].view_b_data &&
      hash_tensor_stable_bytes(used_b[0].feature_mask) ==
          drawn[0].view_b_feature_mask &&
      torch::equal(used_a[1].data, used_b[1].data) &&
      torch::equal(used_a[1].feature_mask, used_b[1].feature_mask) &&
      torch::equal(used_a[2].data, clean.data) &&
      torch::equal(used_b[2].data, clean.data) &&
      torch::equal(used_a[2].feature_mask, clean.feature_mask) &&
      torch::equal(used_b[2].feature_mask, clean.feature_mask);
  result.both_draws_consumed =
      drawn[0].view_a_data != drawn[0].view_b_data &&
      weak_view_digests_equal(drawn[0], drawn[1]) &&
      weak_view_digests_equal(drawn[0], drawn[2]);
  result.rng_parity = generator_state_snapshot_equal(post[0], post[1]) &&
                      generator_state_snapshot_equal(post[0], post[2]);

  auto projected = torch::randn(
      {16, 1, 8},
      torch::TensorOptions().dtype(torch::kFloat64).requires_grad(true));
  const auto projected_clone = projected.clone();
  const auto mask = torch::ones({16, 1}, torch::kBool);
  mtf::vicreg_stability_loss_options_t options{};
  const auto loss = mtf::compute_vicreg_stability_loss(
      projected, mask, projected_clone, mask, options);
  loss.invariance_loss.backward();
  result.identical_invariance_zero =
      loss.invariance_loss.item<double>() <= kVva1bZeroTolerance;
  result.identical_invariance_gradient_zero =
      projected.grad().defined() &&
      projected.grad().norm().item<double>() <= kVva1bZeroTolerance;
  const auto reconstructed =
      options.invariance_weight * loss.invariance_loss +
      options.variance_weight * loss.variance_loss +
      options.covariance_weight * loss.covariance_loss;
  result.loss_reconstruction = torch::equal(reconstructed, loss.loss);
  const auto zero = torch::zeros({4}, torch::kFloat64);
  const auto nonzero = torch::ones({4}, torch::kFloat64);
  const auto inactive = vva1b_cosine(zero, nonzero);
  result.inactive_cosine = !inactive.active && inactive.value == 0.0 &&
                           std::isfinite(inactive.value);

  const std::vector<uint64_t> codec_values{
      drawn[0].view_a_data, drawn[0].view_a_feature_mask,
      drawn[0].view_b_data, drawn[0].view_b_feature_mask,
      post[0].digest.cpu, post[0].digest.cuda};
  result.digest_codec =
      oca_u64_le_bytes_vector(oca_u64_le_bytes_tensor(codec_values)) ==
      codec_values;
  result.cache_integer_archive_roundtrip =
      vva1b_cache_integer_archive_roundtrip();
  const auto resumed_then_fresh = vva1b_update_ledger(
      /*current_executed=*/2 * kVva1bUpdatesPerSeed,
      /*current_fresh_committed=*/2 * kVva1bUpdatesPerSeed,
      /*validated_replacement=*/kVva1bTotalUpdates);
  result.cache_resume_accounting =
      resumed_then_fresh.valid &&
      resumed_then_fresh.current_uncommitted_discarded == 0 &&
      resumed_then_fresh.lifetime_physical ==
          kVva1bFailedAttemptUpdates + kVva1bTotalUpdates;
  const auto post_resume_failure = vva1b_update_ledger(
      /*current_executed=*/768,
      /*current_fresh_committed=*/0,
      /*validated_replacement=*/kVva1bUpdatesPerSeed);
  result.post_resume_failure_accounting =
      post_resume_failure.valid &&
      post_resume_failure.current_uncommitted_discarded == 768 &&
      post_resume_failure.lifetime_physical ==
          kVva1bFailedAttemptUpdates + kVva1bUpdatesPerSeed + 768;
  const bool mechanism = 0.006 >= kVva1bCausalFloor && 0.001 > 0.0;
  const bool safety = false; // Deliberately independent of mechanism support.
  result.causal_safety_separated = mechanism && !safety;
  result.cache_plan_full_triad =
      kVva1bTotalUpdates == 4608 && kVva1bArms == 3 &&
      kVva1bSteps == 512;
  result.pass = result.data.pass && result.policy_inventory &&
                result.default_policy && result.post_draw_assignment &&
                result.both_draws_consumed && result.rng_parity &&
                result.identical_invariance_zero &&
                result.identical_invariance_gradient_zero &&
                result.loss_reconstruction && result.inactive_cosine &&
                result.digest_codec &&
                result.cache_integer_archive_roundtrip &&
                result.cache_resume_accounting &&
                result.post_resume_failure_accounting &&
                result.cache_codec_optimizer_updates == 0 &&
                result.cache_codec_ema_updates == 0 &&
                result.causal_safety_separated && result.cache_plan_full_triad;
  return result;
}

[[nodiscard]] bool vva1b_allclose(const torch::Tensor &left,
                                  const torch::Tensor &right,
                                  double atol = kVva1bProjectionTolerance,
                                  double rtol = kVva1bLossRtol) {
  return left.defined() == right.defined() &&
         (!left.defined() ||
          torch::allclose(left, right, rtol, atol, /*equal_nan=*/false));
}

[[nodiscard]] bool vva1b_snapshots_allclose(const ParameterSnapshot &left,
                                            const ParameterSnapshot &right) {
  if (left.names != right.names || left.values.size() != right.values.size()) {
    return false;
  }
  for (std::size_t index = 0; index < left.values.size(); ++index) {
    if (!vva1b_allclose(left.values[index], right.values[index])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::vector<uint64_t>
vva1b_optimizer_state_digests(torch::optim::Adam &optimizer,
                              const mtf::MtfJepaMaeVicreg &model) {
  std::vector<uint64_t> result;
  result.reserve(model->parameters().size());
  for (const auto &parameter : model->parameters()) {
    const auto iterator =
        optimizer.state().find(parameter.unsafeGetTensorImpl());
    if (iterator == optimizer.state().end()) {
      result.push_back(0U);
      continue;
    }
    const auto *state =
        dynamic_cast<const torch::optim::AdamParamState *>(
            iterator->second.get());
    if (state == nullptr) {
      throw std::runtime_error("VVA-1B optimizer has non-Adam state");
    }
    uint64_t digest_value = 0xcbf29ce484222325ULL;
    mix_hash_value(digest_value, static_cast<uint64_t>(state->step()));
    for (const auto &tensor :
         {state->exp_avg(), state->exp_avg_sq(), state->max_exp_avg_sq()}) {
      mix_hash_value(digest_value,
                     tensor.defined() ? hash_tensor_stable_bytes(tensor)
                                      : 0x756e646566696e65ULL);
    }
    result.push_back(digest_value);
  }
  return result;
}

struct Vva1bShadowUpdate {
  mtf::mtf_jepa_mae_vicreg_output_t output{};
  torch::Tensor loss{};
  torch::Tensor gradient{};
  GeneratorStateSnapshot pre_rng{};
  GeneratorStateSnapshot post_rng{};
  ParameterSnapshot parameters_after_adam{};
  ParameterSnapshot parameters_after_ema{};
  std::vector<uint64_t> optimizer_state{};
  double clip_factor{1.0};
  double served_update_norm{0.0};
};

[[nodiscard]] Vva1bShadowUpdate vva1b_shadow_full_update(
    mtf::MtfJepaMaeVicreg &model, const torch::Tensor &input,
    const torch::Tensor &feature_mask, const torch::Device &device,
    int64_t forward_seed) {
  model->train();
  torch::optim::Adam optimizer(model->parameters(),
                               torch::optim::AdamOptions(kVva1bLearningRate));
  set_paired_rng(forward_seed, device);
  optimizer.zero_grad();
  Vva1bShadowUpdate result{};
  result.pre_rng = current_generator_state_snapshot(device);
  result.output = model->forward(input, feature_mask);
  result.post_rng = current_generator_state_snapshot(device);
  const auto arm = oca_arm(8U);
  result.loss = attribution_arm_loss(
      result.output, arm, attribution_arm_weights(arm, 0));
  result.loss.backward();
  result.gradient =
      gradient_vector(model, GradientPartition::all_trainable);
  const double gradient_norm = result.gradient.norm().item<double>();
  result.clip_factor =
      gradient_norm > kVva1bClipNorm
          ? kVva1bClipNorm / std::max(1.0e-30, gradient_norm)
          : 1.0;
  if (result.clip_factor < 1.0) {
    for (auto &parameter : model->parameters()) {
      if (parameter.grad().defined()) {
        parameter.grad().mul_(result.clip_factor);
      }
    }
  }
  const auto served_before = served_parameter_vector(model);
  optimizer.step();
  const auto served_after = served_parameter_vector(model);
  result.served_update_norm =
      (served_after - served_before).norm().item<double>();
  result.parameters_after_adam = snapshot_parameters(model);
  result.optimizer_state = vva1b_optimizer_state_digests(optimizer, model);
  model->update_target_network();
  result.parameters_after_ema = snapshot_parameters(model);
  return result;
}

[[nodiscard]] bool vva1b_discrete_outputs_exact(
    const mtf::mtf_jepa_mae_vicreg_output_t &left,
    const mtf::mtf_jepa_mae_vicreg_output_t &right) {
  return torch::equal(left.jepa_target_mask, right.jepa_target_mask) &&
         torch::equal(left.jepa_context_mask, right.jepa_context_mask) &&
         torch::equal(left.vicreg_drawn_a_data,
                      right.vicreg_drawn_a_data) &&
         torch::equal(left.vicreg_drawn_a_feature_mask,
                      right.vicreg_drawn_a_feature_mask) &&
         torch::equal(left.vicreg_drawn_b_data,
                      right.vicreg_drawn_b_data) &&
         torch::equal(left.vicreg_drawn_b_feature_mask,
                      right.vicreg_drawn_b_feature_mask) &&
         torch::equal(left.vicreg_view_a_data,
                      right.vicreg_view_a_data) &&
         torch::equal(left.vicreg_view_a_feature_mask,
                      right.vicreg_view_a_feature_mask) &&
         torch::equal(left.vicreg_view_b_data,
                      right.vicreg_view_b_data) &&
         torch::equal(left.vicreg_view_b_feature_mask,
                      right.vicreg_view_b_feature_mask) &&
         torch::equal(left.vicreg_view_a_token_mask,
                      right.vicreg_view_a_token_mask) &&
         torch::equal(left.vicreg_view_b_token_mask,
                      right.vicreg_view_b_token_mask) &&
         torch::equal(left.vicreg_view_a_sample_valid_mask,
                      right.vicreg_view_a_sample_valid_mask) &&
         torch::equal(left.vicreg_view_b_sample_valid_mask,
                      right.vicreg_view_b_sample_valid_mask) &&
         torch::equal(left.vicreg_global_joint_mask,
                      right.vicreg_global_joint_mask) &&
         torch::equal(left.vicreg_channel_joint_mask,
                      right.vicreg_channel_joint_mask) &&
         torch::equal(left.sample_valid_mask, right.sample_valid_mask) &&
         torch::equal(left.channel_valid_mask, right.channel_valid_mask) &&
         left.vicreg_encoder_call_count == right.vicreg_encoder_call_count &&
         left.vicreg_projector_call_count ==
             right.vicreg_projector_call_count;
}

[[nodiscard]] bool vva1b_floating_outputs_allclose(
    const mtf::mtf_jepa_mae_vicreg_output_t &left,
    const mtf::mtf_jepa_mae_vicreg_output_t &right) {
  const auto close = [](const torch::Tensor &lhs, const torch::Tensor &rhs) {
    if (lhs.defined() != rhs.defined()) {
      return false;
    }
    if (!lhs.defined()) {
      return true;
    }
    if (lhs.scalar_type() != rhs.scalar_type() ||
        lhs.sizes() != rhs.sizes()) {
      return false;
    }
    return lhs.is_floating_point() || lhs.is_complex()
               ? vva1b_allclose(lhs, rhs)
               : torch::equal(lhs, rhs);
  };
  bool result =
      close(left.embeddings, right.embeddings) &&
      close(left.pooled_embedding, right.pooled_embedding) &&
      close(left.pooled_by_channel, right.pooled_by_channel) &&
      close(left.pooled_time, right.pooled_time) &&
      close(left.pooled_frequency, right.pooled_frequency) &&
      close(left.loss, right.loss) &&
      close(left.loss_jepa, right.loss_jepa) &&
      close(left.loss_mae, right.loss_mae) &&
      close(left.loss_mae_time, right.loss_mae_time) &&
      close(left.loss_mae_frequency, right.loss_mae_frequency) &&
      close(left.loss_tf_align, right.loss_tf_align) &&
      close(left.loss_vicreg, right.loss_vicreg) &&
      close(left.loss_vicreg_global, right.loss_vicreg_global) &&
      close(left.loss_vicreg_channel, right.loss_vicreg_channel) &&
      close(left.vicreg_view_a_pooled_by_channel,
            right.vicreg_view_a_pooled_by_channel) &&
      close(left.vicreg_view_b_pooled_by_channel,
            right.vicreg_view_b_pooled_by_channel) &&
      close(left.vicreg_view_a_pooled_global,
            right.vicreg_view_a_pooled_global) &&
      close(left.vicreg_view_b_pooled_global,
            right.vicreg_view_b_pooled_global) &&
      close(left.vicreg_view_a_projected_global,
            right.vicreg_view_a_projected_global) &&
      close(left.vicreg_view_b_projected_global,
            right.vicreg_view_b_projected_global);
  if (left.diagnostics.size() != right.diagnostics.size()) {
    return false;
  }
  for (const auto &[name, value] : left.diagnostics) {
    const auto iterator = right.diagnostics.find(name);
    result = result && iterator != right.diagnostics.end() &&
             close(value, iterator->second);
  }
  return result;
}

struct Vva1bParityResult {
  double loss{0.0};
  bool outputs{false};
  bool components{false};
  bool loss_exact{false};
  bool gradients{false};
  bool rng{false};
  bool clipping{false};
  bool optimizer{false};
  bool parameters{false};
  bool ema{false};
  bool pass{false};
};

[[nodiscard]] Vva1bParityResult vva1b_default_seam_update_parity(
    const Dataset &ssl, const torch::Device &device, int64_t seed,
    int64_t step) {
  const auto rows = training_rows(ssl, seed, step);
  const auto indices = torch::tensor(rows, torch::kInt64);
  const auto input = ssl.data.index_select(0, indices).to(device);
  const auto feature_mask = ssl.mask.index_select(0, indices).to(device);
  auto default_config = attribution_config(device, oca_arm(8U));
  auto explicit_config = default_config;
  explicit_config.vicreg_view_pairing_policy =
      mtf::mtf_vicreg_view_pairing_policy_t::independent_weak;
  set_paired_rng(seed, device);
  auto default_model = mtf::MtfJepaMaeVicreg(default_config);
  set_paired_rng(seed, device);
  auto explicit_model = mtf::MtfJepaMaeVicreg(explicit_config);
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  const bool loaded =
      oca_load_archive(oca_archive_path(seed), default_model, device, seed,
                       anchor_hash) &&
      oca_load_archive(oca_archive_path(seed), explicit_model, device, seed,
                       anchor_hash);
  const auto default_initial = snapshot_parameters(default_model);
  const auto explicit_initial = snapshot_parameters(explicit_model);
  const int64_t forward_seed = paired_step_seed(seed, step);
  auto default_update = vva1b_shadow_full_update(
      default_model, input, feature_mask, device, forward_seed);
  auto explicit_update = vva1b_shadow_full_update(
      explicit_model, input, feature_mask, device, forward_seed);
  Vva1bParityResult result{};
  result.loss = explicit_update.loss.item<double>();
  result.outputs = loaded &&
                   vva1b_snapshots_allclose(default_initial,
                                            explicit_initial) &&
                   vva1b_discrete_outputs_exact(default_update.output,
                                                explicit_update.output) &&
                   vva1b_floating_outputs_allclose(default_update.output,
                                                   explicit_update.output);
  result.components = true;
  const auto arm = oca_arm(8U);
  for (std::size_t component = 0; component < 3; ++component) {
    result.components =
        result.components &&
        vva1b_allclose(vicreg_component_tensor(default_update.output, arm,
                                               component),
                       vicreg_component_tensor(explicit_update.output, arm,
                                               component));
  }
  result.loss_exact = vva1b_allclose(default_update.loss,
                                     explicit_update.loss);
  result.gradients = vva1b_allclose(default_update.gradient,
                                    explicit_update.gradient);
  result.rng =
      generator_state_snapshot_equal(default_update.pre_rng,
                                     explicit_update.pre_rng) &&
      generator_state_snapshot_equal(default_update.post_rng,
                                     explicit_update.post_rng);
  result.clipping =
      default_update.clip_factor == explicit_update.clip_factor &&
      default_update.served_update_norm ==
          explicit_update.served_update_norm;
  result.optimizer =
      default_update.optimizer_state == explicit_update.optimizer_state;
  result.parameters = vva1b_snapshots_allclose(
      default_update.parameters_after_adam,
      explicit_update.parameters_after_adam);
  result.ema = vva1b_snapshots_allclose(
      default_update.parameters_after_ema,
      explicit_update.parameters_after_ema);
  result.pass = result.outputs && result.components && result.loss_exact &&
                result.gradients && result.rng && result.clipping &&
                result.optimizer && result.parameters && result.ema;
  return result;
}

[[nodiscard]] Vva1bComponentDiagnostic vva1b_component_diagnostic(
    mtf::MtfJepaMaeVicreg &model, const torch::Tensor &input,
    const torch::Tensor &feature_mask, const torch::Device &device,
    int64_t forward_seed, std::size_t component) {
  if (component >= 3) {
    throw std::runtime_error("VVA-1B component is invalid");
  }
  set_paired_rng(forward_seed, device);
  vva1b_zero_gradients(model);
  const auto output = model->forward(input, feature_mask);
  const auto raw = vicreg_component_tensor(output, oca_arm(8U), component);
  raw.backward();
  Vva1bComponentDiagnostic result{};
  result.raw = raw.item<double>();
  constexpr std::array<double, 3> component_weights{25.0, 25.0, 1.0};
  result.weighted = 0.05 * 0.25 * component_weights[component] * result.raw;
  result.tokenizer_gradient =
      gradient_vector(model, GradientPartition::tokenizer);
  result.encoder_gradient = gradient_vector(model, GradientPartition::encoder);
  result.projector_gradient =
      gradient_vector(model, GradientPartition::vicreg_head);
  result.all_gradient =
      gradient_vector(model, GradientPartition::all_trainable);
  result.tokenizer_gradient_norm = result.tokenizer_gradient.norm().item<double>();
  result.encoder_gradient_norm = result.encoder_gradient.norm().item<double>();
  result.projector_gradient_norm = result.projector_gradient.norm().item<double>();
  return result;
}

[[nodiscard]] Geometry
vva1b_projected_geometry(const torch::Tensor &projected,
                         const torch::Tensor &joint_mask) {
  const auto rows = joint_mask.reshape({-1}).nonzero().reshape({-1});
  return geometry_for_channel(
      projected.squeeze(1).index_select(0, rows));
}

[[nodiscard]] int64_t
vva1b_dimensions_below_floor(const torch::Tensor &projected,
                             const torch::Tensor &joint_mask) {
  const auto rows = joint_mask.reshape({-1}).nonzero().reshape({-1});
  const auto values = projected.squeeze(1).index_select(0, rows);
  const auto standard_deviation =
      torch::sqrt(values.var(0, /*unbiased=*/false) + 1.0e-4);
  return standard_deviation.lt(1.0).sum().item<int64_t>();
}

[[nodiscard]] Vva1bVirtualStepDiagnostic vva1b_virtual_step(
    const RmcData &data, const RmcEvalTargets &targets,
    const torch::Device &device, int64_t seed, std::size_t arm_index,
    std::size_t objective) {
  if (arm_index >= 3 || objective >= 4) {
    throw std::runtime_error("VVA-1B virtual-step index failed");
  }
  const auto config = vva1b_config(device, arm_index);
  set_paired_rng(seed, device);
  auto model = mtf::MtfJepaMaeVicreg(config);
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  if (!oca_load_archive(oca_archive_path(seed), model, device, seed,
                        anchor_hash)) {
    throw std::runtime_error("VVA-1B virtual-step anchor load failed");
  }
  const auto initial = snapshot_parameters(model);
  const auto before = vva_evaluate(model, data, targets, device, false);
  const auto before_family = rssm_family_areas(before.probe);
  const auto rows = training_rows(data.ssl, seed, 0);
  const auto indices = torch::tensor(rows, torch::kInt64);
  const auto input = data.ssl.data.index_select(0, indices).to(device);
  const auto feature_mask = data.ssl.mask.index_select(0, indices).to(device);
  model->train();
  torch::optim::Adam optimizer(model->parameters(),
                               torch::optim::AdamOptions(kVva1bLearningRate));
  const auto optimizer_before = vva1b_optimizer_state_digests(optimizer, model);
  set_paired_rng(paired_step_seed(seed, 0), device);
  optimizer.zero_grad();
  const auto output = model->forward(input, feature_mask);
  torch::Tensor loss{};
  if (objective == 0) {
    loss = attribution_arm_loss(output, oca_arm(8U),
                                attribution_arm_weights(oca_arm(8U), 0));
  } else {
    constexpr std::array<double, 3> component_weights{25.0, 25.0, 1.0};
    loss = 0.05 * 0.25 * component_weights[objective - 1] *
           vicreg_component_tensor(output, oca_arm(8U), objective - 1);
  }
  Vva1bVirtualStepDiagnostic result{};
  result.active = objective == 0 || loss.item<double>() > kVva1bZeroTolerance;
  auto after_optimizer = initial;
  if (result.active) {
    loss.backward();
    const auto gradient =
        gradient_vector(model, GradientPartition::all_trainable);
    const double gradient_norm = gradient.norm().item<double>();
    const double clip_factor =
        gradient_norm > kVva1bClipNorm
            ? kVva1bClipNorm / std::max(1.0e-30, gradient_norm)
            : 1.0;
    if (clip_factor < 1.0) {
      for (auto &parameter : model->parameters()) {
        if (parameter.grad().defined()) {
          parameter.grad().mul_(clip_factor);
        }
      }
    }
    optimizer.step();
    result.optimizer_step = true;
    after_optimizer = snapshot_parameters(model);
    model->update_target_network();
    result.ema_step = true;
  }
  const auto after_step = snapshot_parameters(model);
  const auto optimizer_after = vva1b_optimizer_state_digests(optimizer, model);
  if (result.active) {
    result.parameter_state_exact =
        !vva1b_parameter_snapshots_exact(initial, after_optimizer);
    result.optimizer_state_exact = optimizer_before != optimizer_after;
    result.ema_state_exact =
        parameter_partition_max_abs_diff(
            model, after_optimizer, ParameterDeltaPartition::target_ema) >
            0.0 &&
        parameter_partition_max_abs_diff(
            model, after_optimizer, ParameterDeltaPartition::served) == 0.0;
  } else {
    result.parameter_state_exact =
        vva1b_parameter_snapshots_exact(initial, after_step);
    result.optimizer_state_exact = optimizer_before == optimizer_after;
    result.ema_state_exact = result.parameter_state_exact;
  }
  result.parameter_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::all_trainable);
  const auto after = vva_evaluate(model, data, targets, device, false);
  const auto after_family = rssm_family_areas(after.probe);
  result.reversal_before = before.order.area;
  result.reversal_after = after.order.area;
  result.reversal_delta = result.reversal_after - result.reversal_before;
  result.order_regime_before = before_family[1];
  result.order_regime_after = after_family[1];
  result.order_regime_delta =
      result.order_regime_after - result.order_regime_before;
  result.cross_channel_before = before_family[2];
  result.cross_channel_after = after_family[2];
  result.cross_channel_delta =
      result.cross_channel_after - result.cross_channel_before;
  result.geometry_before = before.geometry;
  result.geometry_after = after.geometry;
  for (std::size_t channel = 0; channel < kChannels; ++channel) {
    result.effective_rank_before +=
        before.geometry[channel].effective_rank_ratio /
        static_cast<double>(kChannels);
    result.effective_rank_after +=
        after.geometry[channel].effective_rank_ratio /
        static_cast<double>(kChannels);
    result.participation_rank_before +=
        before.geometry[channel].participation_rank_ratio /
        static_cast<double>(kChannels);
    result.participation_rank_after +=
        after.geometry[channel].participation_rank_ratio /
        static_cast<double>(kChannels);
  }
  result.effective_rank_delta =
      result.effective_rank_after - result.effective_rank_before;
  result.participation_rank_delta =
      result.participation_rank_after - result.participation_rank_before;
  result.channel_separation_before = result.cross_channel_before;
  result.channel_separation_after = result.cross_channel_after;
  result.channel_separation_delta = result.cross_channel_delta;
  result.finite =
      std::isfinite(result.reversal_before) &&
      std::isfinite(result.reversal_after) &&
      std::isfinite(result.order_regime_before) &&
      std::isfinite(result.order_regime_after) &&
      std::isfinite(result.cross_channel_before) &&
      std::isfinite(result.cross_channel_after) &&
      std::isfinite(result.parameter_delta) &&
      (result.active ? (result.optimizer_step && result.ema_step &&
                        result.parameter_delta > 0.0 &&
                        result.parameter_state_exact &&
                        result.optimizer_state_exact &&
                        result.ema_state_exact)
                     : (!result.optimizer_step && !result.ema_step &&
                        result.parameter_delta == 0.0 &&
                        result.parameter_state_exact &&
                        result.optimizer_state_exact &&
                        result.ema_state_exact));
  return result;
}

[[nodiscard]] Vva1bStage0 vva1b_run_stage0(
    const RmcData &data, const RmcEvalTargets &targets,
    const torch::Device &device, const Vva1bCustody &custody,
    const Vva1bSelfTest &self_test) {
  Vva1bStage0 result{};
  result.cpu_self_test = self_test.pass;
  result.custody = custody.pass;
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  constexpr std::array<int64_t, 3> parity_steps{0, 255, 511};

  result.explicit_default_seam_parity = true;
  result.treatment_mechanics = true;
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    auto &seed_result = result.seed[seed_index];
    const int64_t seed = kAttributionSeeds[seed_index];
    seed_result.seed = seed;
    std::array<Vva1bParityResult, 3> parity{};
    for (std::size_t checkpoint = 0; checkpoint < parity_steps.size();
         ++checkpoint) {
      parity[checkpoint] = vva1b_default_seam_update_parity(
          data.ssl, device, seed, parity_steps[checkpoint]);
      seed_result.default_parity_update_indices[checkpoint] =
          parity_steps[checkpoint];
      seed_result.default_parity_pass[checkpoint] = parity[checkpoint].pass;
      result.explicit_default_seam_parity =
          result.explicit_default_seam_parity && parity[checkpoint].pass;
    }
    seed_result.default_output_exact =
        std::all_of(parity.begin(), parity.end(),
                    [](const auto &value) { return value.outputs; });
    seed_result.default_components_exact =
        std::all_of(parity.begin(), parity.end(),
                    [](const auto &value) { return value.components; });
    seed_result.default_loss_exact =
        std::all_of(parity.begin(), parity.end(),
                    [](const auto &value) { return value.loss_exact; });
    seed_result.default_gradients_exact =
        std::all_of(parity.begin(), parity.end(),
                    [](const auto &value) { return value.gradients; });
    seed_result.default_rng_exact =
        std::all_of(parity.begin(), parity.end(),
                    [](const auto &value) { return value.rng; });
    seed_result.default_update_exact =
        std::all_of(parity.begin(), parity.end(), [](const auto &value) {
          return value.clipping && value.optimizer && value.parameters &&
                 value.ema;
        });

    const auto rows = training_rows(data.ssl, seed, 0);
    const auto indices = torch::tensor(rows, torch::kInt64);
    const auto input = data.ssl.data.index_select(0, indices).to(device);
    const auto feature_mask =
        data.ssl.mask.index_select(0, indices).to(device);
    const auto canonical = torch::where(
        feature_mask, input, torch::zeros_like(input));
    std::array<mtf::MtfJepaMaeVicreg, 3> models{nullptr, nullptr, nullptr};
    std::array<mtf::mtf_jepa_mae_vicreg_output_t, 3> outputs{};
    std::array<GeneratorStateSnapshot, 3> pre{};
    std::array<GeneratorStateSnapshot, 3> post{};
    ParameterSnapshot initial_reference{};
    std::string common_manifest{};
    seed_result.initialization_exact = true;
    seed_result.manifests_exact_except_policy = true;
    for (std::size_t arm = 0; arm < 3; ++arm) {
      const auto config = vva1b_config(device, arm);
      const auto arm_common = vva1b_common_manifest(config);
      if (arm == 0) {
        common_manifest = arm_common;
      } else {
        seed_result.manifests_exact_except_policy =
            seed_result.manifests_exact_except_policy &&
            arm_common == common_manifest;
      }
      seed_result.manifests_exact_except_policy =
          seed_result.manifests_exact_except_policy &&
          config.mask_ratio_time == 0.10 &&
          config.mask_ratio_channel == 0.0 && config.dropout == 0.0 &&
          std::abs(mtf::detail::resolved_vicreg_view_time_dropout_prob(config) -
                   0.01) <= std::numeric_limits<double>::epsilon() &&
          config.vicreg_view_gaussian_jitter_std == 0.005 &&
          config.lambda_jepa == 0.0 && config.lambda_mae == 0.0 &&
          config.lambda_tf_align == 0.0 && config.lambda_vicreg == 0.05 &&
          config.lambda_global_vicreg == 0.25 && config.use_global_vicreg &&
          !config.use_channel_vicreg && config.vicreg_sim_weight == 25.0 &&
          config.vicreg_var_weight == 25.0 &&
          config.vicreg_cov_weight == 1.0 &&
          config.vicreg_variance_floor == 1.0 &&
          config.vicreg_variance_epsilon == 0.0001 &&
          config.target_ema_tau == 0.990;
      set_paired_rng(seed, device);
      models[arm] = mtf::MtfJepaMaeVicreg(config);
      seed_result.initialization_exact =
          seed_result.initialization_exact &&
          oca_load_archive(oca_archive_path(seed), models[arm], device, seed,
                           anchor_hash);
      if (arm == 0) {
        initial_reference = snapshot_parameters(models[arm]);
      } else {
        seed_result.initialization_exact =
            seed_result.initialization_exact &&
            parameter_max_abs_diff(models[arm], initial_reference) == 0.0;
      }
      models[arm]->train();
      set_paired_rng(paired_step_seed(seed, 0), device);
      pre[arm] = current_generator_state_snapshot(device);
      outputs[arm] = models[arm]->forward(input, feature_mask);
      post[arm] = current_generator_state_snapshot(device);
    }

    seed_result.drawn_views_exact = true;
    seed_result.pre_rng_exact = true;
    seed_result.post_rng_exact = true;
    seed_result.jepa_masks_exact = true;
    seed_result.retained_rows_exact = true;
    for (std::size_t arm = 1; arm < 3; ++arm) {
      seed_result.drawn_views_exact =
          seed_result.drawn_views_exact &&
          torch::equal(outputs[0].vicreg_drawn_a_data,
                       outputs[arm].vicreg_drawn_a_data) &&
          torch::equal(outputs[0].vicreg_drawn_a_feature_mask,
                       outputs[arm].vicreg_drawn_a_feature_mask) &&
          torch::equal(outputs[0].vicreg_drawn_b_data,
                       outputs[arm].vicreg_drawn_b_data) &&
          torch::equal(outputs[0].vicreg_drawn_b_feature_mask,
                       outputs[arm].vicreg_drawn_b_feature_mask);
      seed_result.post_rng_exact =
          seed_result.post_rng_exact &&
          generator_state_snapshot_equal(post[0], post[arm]);
      seed_result.pre_rng_exact =
          seed_result.pre_rng_exact &&
          generator_state_snapshot_equal(pre[0], pre[arm]);
      seed_result.jepa_masks_exact =
          seed_result.jepa_masks_exact &&
          torch::equal(outputs[0].jepa_target_mask,
                       outputs[arm].jepa_target_mask) &&
          torch::equal(outputs[0].jepa_context_mask,
                       outputs[arm].jepa_context_mask);
      seed_result.retained_rows_exact =
          seed_result.retained_rows_exact &&
          torch::equal(outputs[0].sample_valid_mask,
                       outputs[arm].sample_valid_mask);
    }

    for (std::size_t arm = 0; arm < 3; ++arm) {
      auto &diagnostic = seed_result.arm[arm];
      const auto &output = outputs[arm];
      diagnostic.drawn_view_hashes = vva1b_drawn_digest(output);
      diagnostic.used_view_hashes = weak_view_digest(output);
      diagnostic.pre_rng_hashes = pre[arm].digest;
      diagnostic.post_rng_hashes = post[arm].digest;
      diagnostic.jepa_target_mask_hash =
          hash_tensor_stable_bytes(output.jepa_target_mask);
      diagnostic.jepa_context_mask_hash =
          hash_tensor_stable_bytes(output.jepa_context_mask);
      diagnostic.branch_a_token_mask_hash =
          hash_tensor_stable_bytes(output.vicreg_view_a_token_mask);
      diagnostic.branch_b_token_mask_hash =
          hash_tensor_stable_bytes(output.vicreg_view_b_token_mask);
      diagnostic.branch_a_sample_valid_hash =
          hash_tensor_stable_bytes(output.vicreg_view_a_sample_valid_mask);
      diagnostic.branch_b_sample_valid_hash =
          hash_tensor_stable_bytes(output.vicreg_view_b_sample_valid_mask);
      diagnostic.global_validity_mask_hash =
          hash_tensor_stable_bytes(output.vicreg_global_joint_mask);
      const bool drawn_finite =
          vva1b_zero_masked(output.vicreg_drawn_a_data,
                            output.vicreg_drawn_a_feature_mask) &&
          vva1b_zero_masked(output.vicreg_drawn_b_data,
                            output.vicreg_drawn_b_feature_mask);
      diagnostic.finite_zero_masked =
          drawn_finite &&
          vva1b_zero_masked(output.vicreg_view_a_data,
                            output.vicreg_view_a_feature_mask) &&
          vva1b_zero_masked(output.vicreg_view_b_data,
                            output.vicreg_view_b_feature_mask);
      if (arm == 0) {
        const auto jointly_valid =
            output.vicreg_view_a_feature_mask.logical_and(
                output.vicreg_view_b_feature_mask);
        const bool independent =
            output.vicreg_view_a_data.masked_select(jointly_valid)
                .ne(output.vicreg_view_b_data.masked_select(jointly_valid))
                .any()
                .item<bool>();
        diagnostic.treatment_exact =
            torch::equal(output.vicreg_view_a_data,
                         output.vicreg_drawn_a_data) &&
            torch::equal(output.vicreg_view_a_feature_mask,
                         output.vicreg_drawn_a_feature_mask) &&
            torch::equal(output.vicreg_view_b_data,
                         output.vicreg_drawn_b_data) &&
            torch::equal(output.vicreg_view_b_feature_mask,
                         output.vicreg_drawn_b_feature_mask) &&
            independent;
      } else if (arm == 1) {
        diagnostic.treatment_exact =
            torch::equal(output.vicreg_view_a_data,
                         output.vicreg_drawn_a_data) &&
            torch::equal(output.vicreg_view_a_feature_mask,
                         output.vicreg_drawn_a_feature_mask) &&
            torch::equal(output.vicreg_view_a_data,
                         output.vicreg_view_b_data) &&
            torch::equal(output.vicreg_view_a_feature_mask,
                         output.vicreg_view_b_feature_mask);
      } else {
        diagnostic.treatment_exact =
            torch::equal(output.vicreg_view_a_data, canonical) &&
            torch::equal(output.vicreg_view_b_data, canonical) &&
            torch::equal(output.vicreg_view_a_feature_mask, feature_mask) &&
            torch::equal(output.vicreg_view_b_feature_mask, feature_mask);
      }
      diagnostic.global_valid_rows =
          output.vicreg_global_joint_mask.sum().item<int64_t>();
      diagnostic.encoder_call_count = output.vicreg_encoder_call_count;
      diagnostic.projector_call_count = output.vicreg_projector_call_count;
      diagnostic.separate_forward_graphs =
          diagnostic.encoder_call_count == 2 &&
          diagnostic.projector_call_count == 2;
      diagnostic.view_a_mask_difference =
          output.vicreg_view_a_feature_mask.ne(feature_mask)
              .sum()
              .item<int64_t>();
      diagnostic.view_b_mask_difference =
          output.vicreg_view_b_feature_mask.ne(feature_mask)
              .sum()
              .item<int64_t>();
      diagnostic.branch_token_mask_difference =
          output.vicreg_view_a_token_mask.ne(output.vicreg_view_b_token_mask)
              .sum()
              .item<int64_t>();
      diagnostic.branch_sample_valid_difference =
          output.vicreg_view_a_sample_valid_mask
              .ne(output.vicreg_view_b_sample_valid_mask)
              .sum()
              .item<int64_t>();
      if (arm > 0) {
        diagnostic.treatment_exact =
            diagnostic.treatment_exact &&
            diagnostic.branch_token_mask_difference == 0 &&
            diagnostic.branch_sample_valid_difference == 0;
      }
      diagnostic.view_a_to_clean_rms =
          vva1b_tensor_rms(output.vicreg_view_a_data, canonical);
      diagnostic.view_b_to_clean_rms =
          vva1b_tensor_rms(output.vicreg_view_b_data, canonical);
      diagnostic.view_to_view_rms = vva1b_tensor_rms(
          output.vicreg_view_a_data, output.vicreg_view_b_data);
      diagnostic.pooled_max_abs_difference =
          (output.vicreg_view_a_pooled_global -
           output.vicreg_view_b_pooled_global)
              .abs()
              .max()
              .item<double>();
      diagnostic.projected_max_abs_difference =
          (output.vicreg_view_a_projected_global -
           output.vicreg_view_b_projected_global)
              .abs()
              .max()
              .item<double>();
      diagnostic.projected_a_below_floor = vva1b_dimensions_below_floor(
          output.vicreg_view_a_projected_global,
          output.vicreg_global_joint_mask);
      diagnostic.projected_b_below_floor = vva1b_dimensions_below_floor(
          output.vicreg_view_b_projected_global,
          output.vicreg_global_joint_mask);
      diagnostic.projected_geometry = vva1b_projected_geometry(
          torch::cat({output.vicreg_view_a_projected_global,
                      output.vicreg_view_b_projected_global},
                     0),
          torch::cat({output.vicreg_global_joint_mask,
                      output.vicreg_global_joint_mask},
                     0));
      diagnostic.total_loss = output.loss.item<double>();
      for (std::size_t component = 0; component < 3; ++component) {
        diagnostic.component[component] = vva1b_component_diagnostic(
            models[arm], input, feature_mask, device,
            paired_step_seed(seed, 0), component);
      }
      for (std::size_t partition = 0; partition < 3; ++partition) {
        const auto gradient = [&](std::size_t component) -> torch::Tensor {
          if (partition == 0) {
            return diagnostic.component[component].tokenizer_gradient;
          }
          if (partition == 1) {
            return diagnostic.component[component].encoder_gradient;
          }
          return diagnostic.component[component].projector_gradient;
        };
        diagnostic.cosine[partition][0] =
            vva1b_cosine(gradient(0), gradient(1));
        diagnostic.cosine[partition][1] =
            vva1b_cosine(gradient(0), gradient(2));
        diagnostic.cosine[partition][2] =
            vva1b_cosine(gradient(1), gradient(2));
      }
      const auto independent_reconstruction =
          0.05 * (0.25 *
                  (25.0 * vicreg_component_tensor(output, oca_arm(8U), 0) +
                   25.0 * vicreg_component_tensor(output, oca_arm(8U), 1) +
                   vicreg_component_tensor(output, oca_arm(8U), 2)));
      const auto &config = models[arm]->config();
      const auto production_reconstruction =
          config.lambda_jepa * output.loss_jepa +
          config.lambda_mae * output.loss_mae +
          config.lambda_tf_align * output.loss_tf_align +
          config.lambda_vicreg * output.loss_vicreg;
      diagnostic.reconstruction_abs =
          (independent_reconstruction - output.loss).abs().item<double>();
      diagnostic.reconstruction_rel =
          diagnostic.reconstruction_abs /
          std::max(1.0e-30, std::abs(diagnostic.total_loss));
      diagnostic.reconstruction_exact =
          torch::equal(production_reconstruction, output.loss);
      diagnostic.independent_reconstruction_close =
          diagnostic.reconstruction_abs <=
          kVva1bLossAtol +
              kVva1bLossRtol * std::abs(diagnostic.total_loss);
      set_paired_rng(paired_step_seed(seed, 0), device);
      vva1b_zero_gradients(models[arm]);
      const auto total_output = models[arm]->forward(input, feature_mask);
      const auto total_loss = attribution_arm_loss(
          total_output, oca_arm(8U), attribution_arm_weights(oca_arm(8U), 0));
      total_loss.backward();
      const auto total_gradient =
          gradient_vector(models[arm], GradientPartition::all_trainable);
      const auto component_gradient =
          0.05 * 0.25 *
          (25.0 * diagnostic.component[0].all_gradient +
           25.0 * diagnostic.component[1].all_gradient +
           diagnostic.component[2].all_gradient);
      const auto residual = total_gradient - component_gradient;
      diagnostic.gradient_reconstruction_max_abs =
          residual.abs().max().item<double>();
      diagnostic.gradient_reconstruction_relative_l2 =
          residual.norm().item<double>() /
          std::max(1.0e-30, total_gradient.norm().item<double>());
      diagnostic.gradient_reconstruction_exact =
          diagnostic.gradient_reconstruction_max_abs <= 5.0e-5 &&
          diagnostic.gradient_reconstruction_relative_l2 <= 1.0e-4;
      diagnostic.projected_identity =
          arm == 0 ||
          (diagnostic.projected_max_abs_difference <=
               kVva1bProjectionTolerance &&
           vva1b_allclose(output.vicreg_view_a_pooled_global,
                          output.vicreg_view_b_pooled_global));
      diagnostic.invariance_zero =
          arm == 0 ||
          (diagnostic.component[0].raw <= kVva1bZeroTolerance &&
           diagnostic.component[0].tokenizer_gradient_norm <=
               kVva1bZeroTolerance &&
           diagnostic.component[0].encoder_gradient_norm <=
               kVva1bZeroTolerance &&
           diagnostic.component[0].projector_gradient_norm <=
               kVva1bZeroTolerance);
      for (std::size_t objective = 0; objective < 4; ++objective) {
        diagnostic.virtual_step[objective] = vva1b_virtual_step(
            data, targets, device, seed, arm, objective);
      }
      const bool virtual_pass =
          std::all_of(diagnostic.virtual_step.begin(),
                      diagnostic.virtual_step.end(),
                      [](const auto &value) { return value.finite; });
      diagnostic.pass =
          diagnostic.treatment_exact && diagnostic.finite_zero_masked &&
          diagnostic.separate_forward_graphs &&
          diagnostic.global_valid_rows == kModelRowBatchSize &&
          diagnostic.reconstruction_exact &&
          diagnostic.independent_reconstruction_close &&
          diagnostic.gradient_reconstruction_exact &&
          diagnostic.projected_identity && diagnostic.invariance_zero &&
          virtual_pass;
      result.treatment_mechanics =
          result.treatment_mechanics && diagnostic.pass;
    }

    OcaInterleavedTrainingResult current_cache{};
    const std::vector<uint8_t> challenge_masks(kVva1bChallengeMasks.begin(),
                                               kVva1bChallengeMasks.end());
    const bool current_loaded = oca_load_seed_cache(
        "anchor_challenge", data.ssl, device, seed, challenge_masks,
        kOcaAnchorChallengeSteps, /*load_certified_anchor=*/true,
        current_cache);
    seed_result.cached_v0_metadata =
        current_loaded && current_cache.models.size() == 5 &&
        current_cache.receipts.size() == 5 && current_cache.metadata_exact &&
        current_cache.schedule_exact && current_cache.pass &&
        current_cache.initialization_exact[kVva1bCurrentCachePosition] &&
        current_cache.receipts[kVva1bCurrentCachePosition].pass &&
        current_cache.receipts[kVva1bCurrentCachePosition].steps ==
            kVva1bSteps;
    if (seed_result.cached_v0_metadata) {
      const auto &receipt =
          current_cache.receipts[kVva1bCurrentCachePosition];
      seed_result.cached_v0_row_match =
          !receipt.row_hashes.empty() &&
          receipt.row_hashes.front() == hash_batch_rows(rows);
      seed_result.cached_v0_loss_match =
          !receipt.losses.empty() && receipt.losses.front() == parity[0].loss;
      seed_result.cached_v0_first_update_receipts_available =
          !receipt.target_mask_hashes.empty() &&
          !receipt.context_mask_hashes.empty() &&
          !receipt.weak_view_hashes.empty();
      const auto replay = vva_evaluate(
          current_cache.models[kVva1bCurrentCachePosition], data, targets,
          device, false);
      seed_result.cached_v0_clean_replay =
          replay.probe.area == kVva1bFrozenCurrentAulc[seed_index];
    }
    seed_result.cache_reuse = false;
    seed_result.pass =
        seed_result.initialization_exact &&
        seed_result.manifests_exact_except_policy &&
        seed_result.drawn_views_exact && seed_result.pre_rng_exact &&
        seed_result.post_rng_exact &&
        seed_result.jepa_masks_exact && seed_result.retained_rows_exact &&
        seed_result.default_output_exact &&
        seed_result.default_components_exact &&
        seed_result.default_loss_exact &&
        seed_result.default_gradients_exact &&
        seed_result.default_rng_exact && seed_result.default_update_exact &&
        seed_result.cached_v0_metadata &&
        seed_result.cached_v0_clean_replay && !seed_result.cache_reuse &&
        std::all_of(seed_result.arm.begin(), seed_result.arm.end(),
                    [](const auto &value) { return value.pass; });
    result.treatment_mechanics =
        result.treatment_mechanics && seed_result.pass;
  }
  result.cached_v0_reuse = false;
  result.full_triad_4608_required = true;
  result.pass = result.cpu_self_test && result.custody &&
                result.explicit_default_seam_parity &&
                result.treatment_mechanics && !result.cached_v0_reuse &&
                result.full_triad_4608_required;
  return result;
}

[[nodiscard]] bool vva1b_training_treatment_semantics(
    const mtf::mtf_jepa_mae_vicreg_output_t &output, std::size_t arm,
    const torch::Tensor &input, const torch::Tensor &feature_mask) {
  const auto clean = torch::where(feature_mask, input, torch::zeros_like(input));
  const bool finite =
      vva1b_zero_masked(output.vicreg_drawn_a_data,
                        output.vicreg_drawn_a_feature_mask) &&
      vva1b_zero_masked(output.vicreg_drawn_b_data,
                        output.vicreg_drawn_b_feature_mask) &&
      vva1b_zero_masked(output.vicreg_view_a_data,
                        output.vicreg_view_a_feature_mask) &&
      vva1b_zero_masked(output.vicreg_view_b_data,
                        output.vicreg_view_b_feature_mask) &&
      output.vicreg_encoder_call_count == 2 &&
      output.vicreg_projector_call_count == 2 &&
      output.vicreg_global_joint_mask.sum().item<int64_t>() ==
          kModelRowBatchSize;
  if (!finite) {
    return false;
  }
  if (arm == 0) {
    return torch::equal(output.vicreg_view_a_data,
                        output.vicreg_drawn_a_data) &&
           torch::equal(output.vicreg_view_a_feature_mask,
                        output.vicreg_drawn_a_feature_mask) &&
           torch::equal(output.vicreg_view_b_data,
                        output.vicreg_drawn_b_data) &&
           torch::equal(output.vicreg_view_b_feature_mask,
                        output.vicreg_drawn_b_feature_mask);
  }
  if (arm == 1) {
    return torch::equal(output.vicreg_view_a_data,
                        output.vicreg_drawn_a_data) &&
           torch::equal(output.vicreg_view_a_feature_mask,
                        output.vicreg_drawn_a_feature_mask) &&
           torch::equal(output.vicreg_view_a_data,
                        output.vicreg_view_b_data) &&
           torch::equal(output.vicreg_view_a_feature_mask,
                        output.vicreg_view_b_feature_mask) &&
           torch::equal(output.vicreg_view_a_token_mask,
                        output.vicreg_view_b_token_mask) &&
           torch::equal(output.vicreg_view_a_sample_valid_mask,
                        output.vicreg_view_b_sample_valid_mask);
  }
  return torch::equal(output.vicreg_view_a_data, clean) &&
         torch::equal(output.vicreg_view_b_data, clean) &&
         torch::equal(output.vicreg_view_a_feature_mask, feature_mask) &&
         torch::equal(output.vicreg_view_b_feature_mask, feature_mask) &&
         torch::equal(output.vicreg_view_a_token_mask,
                      output.vicreg_view_b_token_mask) &&
         torch::equal(output.vicreg_view_a_sample_valid_mask,
                      output.vicreg_view_b_sample_valid_mask);
}

void vva1b_reserve_receipt(Vva1bReceipt &receipt) {
  const auto reserve_u64 = [](auto &value) { value.reserve(kVva1bSteps); };
  reserve_u64(receipt.row_hashes);
  reserve_u64(receipt.target_mask_hashes);
  reserve_u64(receipt.context_mask_hashes);
  reserve_u64(receipt.branch_a_token_mask_hashes);
  reserve_u64(receipt.branch_b_token_mask_hashes);
  reserve_u64(receipt.branch_a_sample_valid_hashes);
  reserve_u64(receipt.branch_b_sample_valid_hashes);
  reserve_u64(receipt.global_validity_mask_hashes);
  reserve_u64(receipt.drawn_view_hashes);
  reserve_u64(receipt.used_view_hashes);
  reserve_u64(receipt.pre_rng_hashes);
  reserve_u64(receipt.post_rng_hashes);
  reserve_u64(receipt.total_losses);
  for (auto &component : receipt.component_losses) {
    reserve_u64(component);
  }
  reserve_u64(receipt.gradient_norms);
  reserve_u64(receipt.served_update_norms);
  reserve_u64(receipt.clip_factors);
  reserve_u64(receipt.global_valid_rows);
  reserve_u64(receipt.encoder_call_counts);
  reserve_u64(receipt.projector_call_counts);
}

[[nodiscard]] Vva1bSeedTraining
vva1b_train_seed(const Dataset &ssl, const torch::Device &device,
                 int64_t seed) {
  Vva1bSeedTraining result{};
  result.models.reserve(3);
  std::vector<std::unique_ptr<torch::optim::Adam>> optimizers;
  optimizers.reserve(3);
  std::array<ParameterSnapshot, 3> initial{};
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  ParameterSnapshot reference{};
  result.metadata_exact = true;
  result.interleaving_exact = true;
  for (std::size_t arm = 0; arm < 3; ++arm) {
    set_paired_rng(seed, device);
    auto model = mtf::MtfJepaMaeVicreg(vva1b_config(device, arm));
    result.metadata_exact =
        result.metadata_exact &&
        oca_load_archive(oca_archive_path(seed), model, device, seed,
                         anchor_hash);
    initial[arm] = snapshot_parameters(model);
    if (arm == 0) {
      reference = initial[arm];
    }
    result.receipts[arm].initialization_exact =
        vva1b_parameter_snapshots_exact(initial[arm], reference);
    result.receipts[arm].steps = kVva1bSteps;
    vva1b_reserve_receipt(result.receipts[arm]);
    model->train();
    result.models.push_back(model);
    optimizers.push_back(std::make_unique<torch::optim::Adam>(
        result.models.back()->parameters(),
        torch::optim::AdamOptions(kVva1bLearningRate)));
  }

  for (int64_t step = 0; step < kVva1bSteps; ++step) {
    const auto rows = training_rows(ssl, seed, step);
    const auto row_hash = hash_batch_rows(rows);
    const auto indices = torch::tensor(rows, torch::kInt64);
    const auto input = ssl.data.index_select(0, indices).to(device);
    const auto feature_mask = ssl.mask.index_select(0, indices).to(device);
    std::array<mtf::mtf_jepa_mae_vicreg_output_t, 3> outputs{};
    std::array<GeneratorStateSnapshot, 3> pre{};
    std::array<GeneratorStateSnapshot, 3> post{};

    for (std::size_t arm = 0; arm < 3; ++arm) {
      auto &model = result.models[arm];
      auto &optimizer = *optimizers[arm];
      auto &receipt = result.receipts[arm];
      receipt.row_hashes.push_back(row_hash);
      set_paired_rng(paired_step_seed(seed, step), device);
      pre[arm] = current_generator_state_snapshot(device);
      receipt.pre_rng_hashes.push_back(pre[arm].digest);
      optimizer.zero_grad();
      outputs[arm] = model->forward(input, feature_mask);
      post[arm] = current_generator_state_snapshot(device);
      receipt.post_rng_hashes.push_back(post[arm].digest);
      receipt.target_mask_hashes.push_back(
          hash_tensor_stable_bytes(outputs[arm].jepa_target_mask));
      receipt.context_mask_hashes.push_back(
          hash_tensor_stable_bytes(outputs[arm].jepa_context_mask));
      receipt.branch_a_token_mask_hashes.push_back(
          hash_tensor_stable_bytes(outputs[arm].vicreg_view_a_token_mask));
      receipt.branch_b_token_mask_hashes.push_back(
          hash_tensor_stable_bytes(outputs[arm].vicreg_view_b_token_mask));
      receipt.branch_a_sample_valid_hashes.push_back(
          hash_tensor_stable_bytes(
              outputs[arm].vicreg_view_a_sample_valid_mask));
      receipt.branch_b_sample_valid_hashes.push_back(
          hash_tensor_stable_bytes(
              outputs[arm].vicreg_view_b_sample_valid_mask));
      receipt.global_validity_mask_hashes.push_back(
          hash_tensor_stable_bytes(outputs[arm].vicreg_global_joint_mask));
      receipt.drawn_view_hashes.push_back(vva1b_drawn_digest(outputs[arm]));
      receipt.used_view_hashes.push_back(weak_view_digest(outputs[arm]));
      const int64_t valid_rows =
          outputs[arm].vicreg_global_joint_mask.sum().item<int64_t>();
      receipt.global_valid_rows.push_back(valid_rows);
      receipt.encoder_call_counts.push_back(
          outputs[arm].vicreg_encoder_call_count);
      receipt.projector_call_counts.push_back(
          outputs[arm].vicreg_projector_call_count);
      receipt.global_validity_exact =
          receipt.global_validity_exact &&
          valid_rows == kModelRowBatchSize &&
          outputs[arm].vicreg_encoder_call_count == 2 &&
          outputs[arm].vicreg_projector_call_count == 2;
      receipt.treatment_semantics_exact =
          receipt.treatment_semantics_exact &&
          vva1b_training_treatment_semantics(outputs[arm], arm, input,
                                             feature_mask);

      const auto loss = attribution_arm_loss(
          outputs[arm], oca_arm(8U),
          attribution_arm_weights(oca_arm(8U), step));
      const double loss_value = loss.item<double>();
      receipt.total_losses.push_back(loss_value);
      for (std::size_t component = 0; component < 3; ++component) {
        receipt.component_losses[component].push_back(
            vicreg_component_tensor(outputs[arm], oca_arm(8U), component)
                .item<double>());
      }
      loss.backward();
      const auto gradient =
          gradient_vector(model, GradientPartition::all_trainable);
      const double gradient_norm = gradient.norm().item<double>();
      receipt.gradient_norms.push_back(gradient_norm);
      const double clip_factor =
          gradient_norm > kVva1bClipNorm
              ? kVva1bClipNorm / std::max(1.0e-30, gradient_norm)
              : 1.0;
      receipt.clip_factors.push_back(clip_factor);
      if (clip_factor < 1.0) {
        ++receipt.clipping_count;
        for (auto &parameter : model->parameters()) {
          if (parameter.grad().defined()) {
            parameter.grad().mul_(clip_factor);
          }
        }
      }
      const auto served_before = served_parameter_vector(model);
      optimizer.step();
      ++receipt.adam_steps;
      ++g_vva1b_current_adam_updates;
      const auto served_after = served_parameter_vector(model);
      const double update_norm =
          (served_after - served_before).norm().item<double>();
      receipt.served_update_norms.push_back(update_norm);
      model->update_target_network();
      ++receipt.ema_steps;
      ++g_vva1b_current_ema_updates;
      receipt.finite =
          receipt.finite && std::isfinite(loss_value) && loss_value > 0.0 &&
          std::isfinite(gradient_norm) && gradient_norm > 0.0 &&
          std::isfinite(update_norm) && update_norm > 0.0 &&
          torch::isfinite(outputs[arm].embeddings).all().item<bool>() &&
          torch::isfinite(outputs[arm].loss_vicreg).all().item<bool>();
    }

    for (std::size_t arm = 1; arm < 3; ++arm) {
      const bool rows_exact =
          result.receipts[arm].row_hashes.back() ==
          result.receipts[0].row_hashes.back();
      const bool masks_exact =
          torch::equal(outputs[0].jepa_target_mask,
                       outputs[arm].jepa_target_mask) &&
          torch::equal(outputs[0].jepa_context_mask,
                       outputs[arm].jepa_context_mask);
      const bool draws_exact =
          torch::equal(outputs[0].vicreg_drawn_a_data,
                       outputs[arm].vicreg_drawn_a_data) &&
          torch::equal(outputs[0].vicreg_drawn_a_feature_mask,
                       outputs[arm].vicreg_drawn_a_feature_mask) &&
          torch::equal(outputs[0].vicreg_drawn_b_data,
                       outputs[arm].vicreg_drawn_b_data) &&
          torch::equal(outputs[0].vicreg_drawn_b_feature_mask,
                       outputs[arm].vicreg_drawn_b_feature_mask);
      const bool rng_exact =
          generator_state_snapshot_equal(pre[0], pre[arm]) &&
          generator_state_snapshot_equal(post[0], post[arm]);
      result.receipts[0].row_schedule_exact =
          result.receipts[0].row_schedule_exact && rows_exact;
      result.receipts[arm].row_schedule_exact =
          result.receipts[arm].row_schedule_exact && rows_exact;
      result.receipts[0].mask_schedule_exact =
          result.receipts[0].mask_schedule_exact && masks_exact;
      result.receipts[arm].mask_schedule_exact =
          result.receipts[arm].mask_schedule_exact && masks_exact;
      result.receipts[0].ordinary_draw_schedule_exact =
          result.receipts[0].ordinary_draw_schedule_exact && draws_exact;
      result.receipts[arm].ordinary_draw_schedule_exact =
          result.receipts[arm].ordinary_draw_schedule_exact && draws_exact;
      result.receipts[0].rng_schedule_exact =
          result.receipts[0].rng_schedule_exact && rng_exact;
      result.receipts[arm].rng_schedule_exact =
          result.receipts[arm].rng_schedule_exact && rng_exact;
      result.interleaving_exact = result.interleaving_exact && rows_exact &&
                                  masks_exact && draws_exact && rng_exact;
    }
    if ((step + 1) % 128 == 0 || step + 1 == kVva1bSteps) {
      std::cout << "vva1b.training.seed_" << seed
                << ".completed_steps=" << step + 1 << '\n';
      std::cout << "vva1b.training.seed_" << seed
                << ".interleaving_exact=" << result.interleaving_exact
                << '\n'
                << std::flush;
    }
  }

  result.pass = result.metadata_exact && result.interleaving_exact;
  for (std::size_t arm = 0; arm < 3; ++arm) {
    auto &receipt = result.receipts[arm];
    auto &model = result.models[arm];
    receipt.all_trainable_delta = parameter_partition_max_abs_diff(
        model, initial[arm], ParameterDeltaPartition::all_trainable);
    receipt.served_delta = parameter_partition_max_abs_diff(
        model, initial[arm], ParameterDeltaPartition::served);
    receipt.predictor_delta = parameter_partition_max_abs_diff(
        model, initial[arm], ParameterDeltaPartition::predictor);
    receipt.mae_decoder_delta = parameter_partition_max_abs_diff(
        model, initial[arm], ParameterDeltaPartition::mae_decoder);
    receipt.vicreg_head_delta = parameter_partition_max_abs_diff(
        model, initial[arm], ParameterDeltaPartition::vicreg_head);
    receipt.target_ema_delta = parameter_partition_max_abs_diff(
        model, initial[arm], ParameterDeltaPartition::target_ema);
    receipt.expected_partitions =
        receipt.all_trainable_delta > 0.0 && receipt.served_delta > 0.0 &&
        receipt.vicreg_head_delta > 0.0 && receipt.target_ema_delta > 0.0 &&
        receipt.predictor_delta == 0.0 && receipt.mae_decoder_delta == 0.0;
    const auto size_exact = [&](const auto &value) {
      return value.size() == static_cast<std::size_t>(kVva1bSteps);
    };
    bool vectors_exact =
        size_exact(receipt.row_hashes) &&
        size_exact(receipt.target_mask_hashes) &&
        size_exact(receipt.context_mask_hashes) &&
        size_exact(receipt.branch_a_token_mask_hashes) &&
        size_exact(receipt.branch_b_token_mask_hashes) &&
        size_exact(receipt.branch_a_sample_valid_hashes) &&
        size_exact(receipt.branch_b_sample_valid_hashes) &&
        size_exact(receipt.global_validity_mask_hashes) &&
        size_exact(receipt.drawn_view_hashes) &&
        size_exact(receipt.used_view_hashes) &&
        size_exact(receipt.pre_rng_hashes) &&
        size_exact(receipt.post_rng_hashes) &&
        size_exact(receipt.total_losses) &&
        size_exact(receipt.gradient_norms) &&
        size_exact(receipt.served_update_norms) &&
        size_exact(receipt.clip_factors) &&
        size_exact(receipt.global_valid_rows) &&
        size_exact(receipt.encoder_call_counts) &&
        size_exact(receipt.projector_call_counts);
    for (const auto &component : receipt.component_losses) {
      vectors_exact = vectors_exact && size_exact(component);
    }
    receipt.pass =
        receipt.initialization_exact && receipt.finite && vectors_exact &&
        receipt.adam_steps == kVva1bSteps &&
        receipt.ema_steps == kVva1bSteps &&
        receipt.row_schedule_exact && receipt.mask_schedule_exact &&
        receipt.ordinary_draw_schedule_exact && receipt.rng_schedule_exact &&
        receipt.treatment_semantics_exact && receipt.global_validity_exact &&
        receipt.expected_partitions;
    result.pass = result.pass && receipt.pass;
  }
  return result;
}

[[nodiscard]] std::filesystem::path vva1b_seed_cache_path(int64_t seed) {
  return std::filesystem::path(".build") / "tests" / "vva1b" /
         ("full_triad_4608_seed_" + std::to_string(seed) +
          ".complete.pt");
}

[[nodiscard]] std::filesystem::path
vva1b_seed_cache_marker_path(const std::filesystem::path &path) {
  auto marker = path;
  marker += ".sha256";
  return marker;
}

[[nodiscard]] bool vva1b_seed_cache_inventory_empty() {
  const auto directory = std::filesystem::path(".build") / "tests" / "vva1b";
  return !std::filesystem::exists(directory) ||
         std::filesystem::is_empty(directory);
}

[[nodiscard]] std::vector<uint64_t>
vva1b_flatten_view_digests(const std::vector<WeakViewDigest> &values) {
  std::vector<uint64_t> result;
  result.reserve(values.size() * 4);
  for (const auto &value : values) {
    result.push_back(value.view_a_data);
    result.push_back(value.view_a_feature_mask);
    result.push_back(value.view_b_data);
    result.push_back(value.view_b_feature_mask);
  }
  return result;
}

[[nodiscard]] std::vector<WeakViewDigest>
vva1b_unflatten_view_digests(const torch::Tensor &value) {
  const auto flat = oca_u64_le_bytes_vector(value);
  if (flat.size() % 4 != 0) {
    throw std::runtime_error("VVA-1B view digest archive shape failed");
  }
  std::vector<WeakViewDigest> result;
  result.reserve(flat.size() / 4);
  for (std::size_t index = 0; index < flat.size(); index += 4) {
    result.push_back({.view_a_data = flat[index],
                      .view_a_feature_mask = flat[index + 1],
                      .view_b_data = flat[index + 2],
                      .view_b_feature_mask = flat[index + 3]});
  }
  return result;
}

[[nodiscard]] std::vector<uint64_t>
vva1b_flatten_rng_digests(const std::vector<GeneratorStateDigest> &values) {
  std::vector<uint64_t> result;
  result.reserve(values.size() * 2);
  for (const auto &value : values) {
    result.push_back(value.cpu);
    result.push_back(value.cuda);
  }
  return result;
}

[[nodiscard]] std::vector<GeneratorStateDigest>
vva1b_unflatten_rng_digests(const torch::Tensor &value) {
  const auto flat = oca_u64_le_bytes_vector(value);
  if (flat.size() % 2 != 0) {
    throw std::runtime_error("VVA-1B RNG digest archive shape failed");
  }
  std::vector<GeneratorStateDigest> result;
  result.reserve(flat.size() / 2);
  for (std::size_t index = 0; index < flat.size(); index += 2) {
    result.push_back({.cpu = flat[index], .cuda = flat[index + 1]});
  }
  return result;
}

[[nodiscard]] std::vector<double>
vva1b_double_vector(const torch::Tensor &value) {
  const auto cpu = value.to(torch::kCPU, torch::kFloat64).contiguous();
  if (cpu.dim() != 1) {
    throw std::runtime_error("VVA-1B float archive shape failed");
  }
  const auto *data = cpu.data_ptr<double>();
  return {data, data + cpu.numel()};
}

[[nodiscard]] std::vector<int64_t>
vva1b_int64_vector(const torch::Tensor &value) {
  const auto cpu = value.to(torch::kCPU, torch::kInt64).contiguous();
  if (cpu.dim() != 1) {
    throw std::runtime_error("VVA-1B integer archive shape failed");
  }
  const auto *data = cpu.data_ptr<int64_t>();
  return {data, data + cpu.numel()};
}

[[nodiscard]] std::string
vva1b_data_manifest(const Vva1bDataIdentity &value) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  const auto emit = [&](std::string_view name, uint64_t hash) {
    out << name << '=' << std::setw(16) << hash << '\n';
  };
  emit("normalization.mean", value.normalization_mean);
  emit("normalization.inv_std", value.normalization_inv_std);
  constexpr std::array<std::string_view, 4> fields{"groups", "data", "mask",
                                                   "target"};
  const auto emit_dataset = [&](std::string_view name,
                                const std::array<uint64_t, 4> &hashes) {
    for (std::size_t index = 0; index < hashes.size(); ++index) {
      emit(std::string(name) + "." + std::string(fields[index]),
           hashes[index]);
    }
  };
  emit_dataset("ssl", value.ssl);
  emit_dataset("fit", value.fit);
  emit_dataset("selection", value.selection);
  emit_dataset("development", value.development);
  emit("bootstrap.table", value.bootstrap);
  out << "confirmation.sealed=" << std::boolalpha
      << value.confirmation_sealed << '\n';
  return out.str();
}

[[nodiscard]] std::string vva1b_custody_manifest(
    const Vva1bCustody &custody) {
  std::ostringstream out;
  out << "vva1b_protocol=" << custody.protocol_sha256 << '\n';
  out << "vva1b_protocol_amendment_a1="
      << custody.protocol_amendment_a1_sha256 << '\n';
  out << "vva1b_protocol_amendment_a2="
      << custody.protocol_amendment_a2_sha256 << '\n';
  out << "vva1b_protocol_amendment_a3="
      << custody.protocol_amendment_a3_sha256 << '\n';
  out << "vva1b_failed_attempt=" << custody.failed_attempt_sha256 << '\n';
  out << "vva1b_authorized_seam_audit=" << custody.seam_audit_sha256 << '\n';
  out << "vva1b_transitive_build_manifest="
      << custody.build_manifest_sha256 << '\n';
  out << "source=" << custody.source_sha256 << '\n';
  out << "executable=" << custody.executable_sha256 << '\n';
  out << "post_seam_header=" << kVva1bModuleSha256 << '\n';
  out << "pre_seam_header=" << kVva1bPreSeamModuleSha256 << '\n';
  out << "vva1_protocol=" << kVva1bOldProtocolSha256 << '\n';
  out << "vva1_findings=" << kVva1bOldFindingsSha256 << '\n';
  out << "vva1_harness=" << kVva1bOldHarnessSha256 << '\n';
  out << "vva1_log=" << kVva1bOldLogSha256 << '\n';
  out << "ima1=" << kVva1bIma1Sha256 << '\n';
  out << "oaa1=" << kVva1bOaa1Sha256 << '\n';
  out << "gpv1_protocol=" << kVva1bGpv1ProtocolSha256 << '\n';
  out << "gpv1=" << kVva1bGpv1Sha256 << '\n';
  out << "gpv1_log=" << kVva1bGpv1LogSha256 << '\n';
  out << "ima5=" << kVva1bIma5Sha256 << '\n';
  for (std::size_t seed = 0; seed < 3; ++seed) {
    out << "anchor.seed_" << kAttributionSeeds[seed] << '='
        << kVva1bAnchorSha256[seed] << '\n';
    out << "historical_v0.seed_" << kAttributionSeeds[seed] << '='
        << kVva1bCurrentCacheSha256[seed] << '\n';
  }
  return out.str();
}

void vva1b_save_seed_cache(const Dataset &ssl,
                           const Vva1bDataIdentity &data_identity,
                           const Vva1bCustody &custody,
                           const torch::Device &device, int64_t seed,
                           const Vva1bSeedTraining &result) {
  if (!result.pass || result.models.size() != 3 || !custody.pass ||
      !data_identity.pass) {
    throw std::runtime_error("VVA-1B seed-cache save contract failed");
  }
  const auto path = vva1b_seed_cache_path(seed);
  const auto marker = vva1b_seed_cache_marker_path(path);
  if (std::filesystem::exists(path) || std::filesystem::exists(marker)) {
    throw std::runtime_error(
        "VVA-1B immutable completed cache already exists");
  }
  const auto nonce =
      std::chrono::steady_clock::now().time_since_epoch().count();
  auto temporary = path;
  temporary += ".tmp." + std::to_string(nonce);
  auto temporary_marker = marker;
  temporary_marker += ".tmp." + std::to_string(nonce);
  std::filesystem::create_directories(path.parent_path());
  torch::serialize::OutputArchive root;
  root.write("meta/schema", oca_string_tensor(kVva1bCacheSchema));
  root.write("meta/complete", vva1b_int64_tensor({1}));
  root.write("meta/protocol_sha256",
             oca_string_tensor(kVva1bProtocolSha256));
  root.write("meta/source_sha256",
             oca_string_tensor(custody.source_sha256));
  root.write("meta/executable_sha256",
             oca_string_tensor(custody.executable_sha256));
  root.write("meta/module_sha256", oca_string_tensor(kVva1bModuleSha256));
  root.write("meta/control_source",
             oca_string_tensor("joint_retrain_vva1b"));
  root.write("meta/readout_policy", oca_string_tensor(kVva1bReadoutPolicy));
  root.write("meta/data_manifest",
             oca_string_tensor(vva1b_data_manifest(data_identity)));
  root.write("meta/custody_manifest",
             oca_string_tensor(vva1b_custody_manifest(custody)));
  root.write("meta/ssl_data_hash",
             oca_string_tensor(oca_hex_u64(
                 hash_tensor_stable_bytes(ssl.data))));
  root.write("meta/ssl_mask_hash",
             oca_string_tensor(oca_hex_u64(
                 hash_tensor_stable_bytes(ssl.mask))));
  root.write("meta/ssl_target_hash",
             oca_string_tensor(oca_hex_u64(
                 hash_tensor_stable_bytes(ssl.target))));
  root.write("meta/seed", vva1b_int64_tensor({seed}));
  root.write("meta/steps", vva1b_int64_tensor({kVva1bSteps}));
  root.write("meta/arm_count", vva1b_int64_tensor({3}));
  root.write("meta/optimizer_learning_rate",
             torch::tensor({kVva1bLearningRate}, torch::kFloat64));
  root.write("meta/gradient_clip_norm",
             torch::tensor({kVva1bClipNorm}, torch::kFloat64));
  root.write("meta/objective", vva1b_cache_objective_tensor());
  root.write("meta/result_flags", vva1b_cache_result_flags_tensor(result));
  for (std::size_t arm = 0; arm < 3; ++arm) {
    const auto &receipt = result.receipts[arm];
    if (!receipt.pass) {
      throw std::runtime_error("VVA-1B cache received invalid arm");
    }
    const std::string prefix = "arm_" + std::to_string(arm) + "/";
    torch::serialize::OutputArchive model_archive;
    result.models[arm]->save(model_archive);
    root.write(prefix + "model", model_archive);
    root.write(prefix + "name", oca_string_tensor(kVva1bArmNames[arm]));
    root.write(prefix + "policy",
               oca_string_tensor(mtf::mtf_vicreg_view_pairing_policy_name(
                   kVva1bPolicies[arm])));
    root.write(prefix + "config_manifest",
               oca_string_tensor(canonical_config_manifest(
                   vva1b_config(device, arm))));
    const auto write_u64 = [&](std::string_view name,
                               const std::vector<uint64_t> &value) {
      root.write(prefix + std::string(name),
                 oca_u64_le_bytes_tensor(value));
    };
    write_u64("row_hashes", receipt.row_hashes);
    write_u64("target_mask_hashes", receipt.target_mask_hashes);
    write_u64("context_mask_hashes", receipt.context_mask_hashes);
    write_u64("branch_a_token_mask_hashes",
              receipt.branch_a_token_mask_hashes);
    write_u64("branch_b_token_mask_hashes",
              receipt.branch_b_token_mask_hashes);
    write_u64("branch_a_sample_valid_hashes",
              receipt.branch_a_sample_valid_hashes);
    write_u64("branch_b_sample_valid_hashes",
              receipt.branch_b_sample_valid_hashes);
    write_u64("global_validity_mask_hashes",
              receipt.global_validity_mask_hashes);
    write_u64("drawn_view_hashes",
              vva1b_flatten_view_digests(receipt.drawn_view_hashes));
    write_u64("used_view_hashes",
              vva1b_flatten_view_digests(receipt.used_view_hashes));
    write_u64("pre_rng_hashes",
              vva1b_flatten_rng_digests(receipt.pre_rng_hashes));
    write_u64("post_rng_hashes",
              vva1b_flatten_rng_digests(receipt.post_rng_hashes));
    const auto write_double = [&](std::string_view name,
                                  const std::vector<double> &value) {
      root.write(prefix + std::string(name),
                 torch::tensor(value, torch::kFloat64));
    };
    write_double("total_losses", receipt.total_losses);
    for (std::size_t component = 0; component < 3; ++component) {
      write_double("component_" + std::to_string(component),
                   receipt.component_losses[component]);
    }
    write_double("gradient_norms", receipt.gradient_norms);
    write_double("served_update_norms", receipt.served_update_norms);
    write_double("clip_factors", receipt.clip_factors);
    root.write(prefix + "global_valid_rows",
               torch::tensor(receipt.global_valid_rows, torch::kInt64));
    root.write(prefix + "encoder_call_counts",
               torch::tensor(receipt.encoder_call_counts, torch::kInt64));
    root.write(prefix + "projector_call_counts",
               torch::tensor(receipt.projector_call_counts, torch::kInt64));
    root.write(prefix + "receipt_scalars",
               torch::tensor({receipt.all_trainable_delta,
                              receipt.served_delta, receipt.predictor_delta,
                              receipt.mae_decoder_delta,
                              receipt.vicreg_head_delta,
                              receipt.target_ema_delta},
                             torch::kFloat64));
    root.write(prefix + "receipt_flags",
               vva1b_cache_receipt_flags_tensor(receipt));
  }
  root.save_to(temporary.string());
  const auto checksum = vva1b_file_sha256(temporary);
  {
    std::ofstream output(temporary_marker,
                         std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
      throw std::runtime_error("VVA-1B cache marker create failed");
    }
    output << checksum << '\n';
    output.close();
    if (!output) {
      throw std::runtime_error("VVA-1B cache marker write failed");
    }
  }
  // A marked final path is immutable.  The only names replaced here are the
  // unique temporary names created by this invocation.
  std::filesystem::rename(temporary, path);
  std::filesystem::rename(temporary_marker, marker);
}

[[nodiscard]] bool vva1b_receipt_reconstructs(
    const Vva1bReceipt &receipt, const Dataset &ssl, int64_t seed,
    std::size_t arm) {
  if (!receipt.pass || receipt.steps != kVva1bSteps ||
      receipt.adam_steps != kVva1bSteps ||
      receipt.ema_steps != kVva1bSteps || !receipt.initialization_exact ||
      !receipt.row_schedule_exact || !receipt.mask_schedule_exact ||
      !receipt.ordinary_draw_schedule_exact || !receipt.rng_schedule_exact ||
      !receipt.treatment_semantics_exact || !receipt.global_validity_exact ||
      !receipt.finite || !receipt.expected_partitions ||
      !(receipt.all_trainable_delta > 0.0) || !(receipt.served_delta > 0.0) ||
      receipt.predictor_delta != 0.0 || receipt.mae_decoder_delta != 0.0 ||
      !(receipt.vicreg_head_delta > 0.0) ||
      !(receipt.target_ema_delta > 0.0)) {
    return false;
  }
  const auto size_exact = [](const auto &value) {
    return value.size() == static_cast<std::size_t>(kVva1bSteps);
  };
  bool sizes =
      size_exact(receipt.row_hashes) &&
      size_exact(receipt.target_mask_hashes) &&
      size_exact(receipt.context_mask_hashes) &&
      size_exact(receipt.branch_a_token_mask_hashes) &&
      size_exact(receipt.branch_b_token_mask_hashes) &&
      size_exact(receipt.branch_a_sample_valid_hashes) &&
      size_exact(receipt.branch_b_sample_valid_hashes) &&
      size_exact(receipt.global_validity_mask_hashes) &&
      size_exact(receipt.drawn_view_hashes) &&
      size_exact(receipt.used_view_hashes) &&
      size_exact(receipt.pre_rng_hashes) &&
      size_exact(receipt.post_rng_hashes) &&
      size_exact(receipt.total_losses) &&
      size_exact(receipt.gradient_norms) &&
      size_exact(receipt.served_update_norms) &&
      size_exact(receipt.clip_factors) &&
      size_exact(receipt.global_valid_rows) &&
      size_exact(receipt.encoder_call_counts) &&
      size_exact(receipt.projector_call_counts);
  for (const auto &component : receipt.component_losses) {
    sizes = sizes && size_exact(component);
  }
  if (!sizes) {
    return false;
  }
  const auto expected_global_validity_hash = hash_tensor_stable_bytes(
      torch::ones({kModelRowBatchSize, 1}, torch::kBool));
  for (int64_t step = 0; step < kVva1bSteps; ++step) {
    const auto index = static_cast<std::size_t>(step);
    const auto rows = training_rows(ssl, seed, step);
    if (receipt.row_hashes[index] != hash_batch_rows(rows) ||
        receipt.global_valid_rows[index] != kModelRowBatchSize ||
        receipt.global_validity_mask_hashes[index] !=
            expected_global_validity_hash ||
        receipt.encoder_call_counts[index] != 2 ||
        receipt.projector_call_counts[index] != 2 ||
        !std::isfinite(receipt.total_losses[index]) ||
        !(receipt.total_losses[index] > 0.0) ||
        !std::isfinite(receipt.gradient_norms[index]) ||
        !(receipt.gradient_norms[index] > 0.0) ||
        !std::isfinite(receipt.served_update_norms[index]) ||
        !(receipt.served_update_norms[index] > 0.0) ||
        !std::isfinite(receipt.clip_factors[index]) ||
        !(receipt.clip_factors[index] > 0.0) ||
        receipt.clip_factors[index] > 1.0) {
      return false;
    }
    for (const auto &component : receipt.component_losses) {
      if (!std::isfinite(component[index]) || component[index] < 0.0) {
        return false;
      }
    }
    const auto &drawn = receipt.drawn_view_hashes[index];
    const auto &used = receipt.used_view_hashes[index];
    if (arm == 0 && !weak_view_digests_equal(drawn, used)) {
      return false;
    }
    if (arm == 1 &&
        (used.view_a_data != drawn.view_a_data ||
         used.view_a_feature_mask != drawn.view_a_feature_mask ||
         used.view_b_data != drawn.view_a_data ||
         used.view_b_feature_mask != drawn.view_a_feature_mask ||
         receipt.branch_a_token_mask_hashes[index] !=
             receipt.branch_b_token_mask_hashes[index] ||
         receipt.branch_a_sample_valid_hashes[index] !=
             receipt.branch_b_sample_valid_hashes[index])) {
      return false;
    }
    if (arm == 2) {
      const auto indices = torch::tensor(rows, torch::kInt64);
      const auto input = ssl.data.index_select(0, indices);
      const auto mask = ssl.mask.index_select(0, indices);
      const auto clean = torch::where(mask, input, torch::zeros_like(input));
      const auto clean_data_hash = hash_tensor_stable_bytes(clean);
      const auto clean_mask_hash = hash_tensor_stable_bytes(mask);
      if (used.view_a_data != clean_data_hash ||
          used.view_b_data != clean_data_hash ||
          used.view_a_feature_mask != clean_mask_hash ||
          used.view_b_feature_mask != clean_mask_hash ||
          receipt.branch_a_token_mask_hashes[index] !=
              receipt.branch_b_token_mask_hashes[index] ||
          receipt.branch_a_sample_valid_hashes[index] !=
              receipt.branch_b_sample_valid_hashes[index]) {
        return false;
      }
    }
  }
  return true;
}

void vva1b_recover_archive_only_marker(
    const std::filesystem::path &archive,
    const std::filesystem::path &marker) {
  if (!std::filesystem::exists(archive) || std::filesystem::exists(marker)) {
    return;
  }
  const auto checksum = vva1b_file_sha256(archive);
  if (checksum.size() != 64) {
    throw std::runtime_error("VVA-1B archive-only cache hash failed");
  }
  const auto nonce =
      std::chrono::steady_clock::now().time_since_epoch().count();
  auto temporary_marker = marker;
  temporary_marker += ".recovery.tmp." + std::to_string(nonce);
  {
    std::ofstream output(temporary_marker,
                         std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
      throw std::runtime_error("VVA-1B cache marker recovery create failed");
    }
    output << checksum << '\n';
    output.close();
    if (!output) {
      throw std::runtime_error("VVA-1B cache marker recovery write failed");
    }
  }
  std::filesystem::rename(temporary_marker, marker);
}

[[nodiscard]] bool vva1b_load_seed_cache(
    const Dataset &ssl, const Vva1bDataIdentity &data_identity,
    const Vva1bCustody &custody, const torch::Device &device, int64_t seed,
    Vva1bSeedTraining &result) {
  const auto path = vva1b_seed_cache_path(seed);
  const auto marker = vva1b_seed_cache_marker_path(path);
  bool archive_exists = std::filesystem::exists(path);
  bool marker_exists = std::filesystem::exists(marker);
  if (!archive_exists && !marker_exists) {
    return false;
  }
  // Publication renames the complete archive before its checksum marker.  If
  // the process stops between those two operations, reconstruct only the
  // missing marker from the immutable final archive; full metadata and receipt
  // validation below still decides whether the cache is admissible evidence.
  if (archive_exists && !marker_exists) {
    vva1b_recover_archive_only_marker(path, marker);
    marker_exists = std::filesystem::exists(marker);
  }
  if (archive_exists != marker_exists) {
    throw std::runtime_error("VVA-1B partial final cache is not evidence");
  }
  const auto expected_checksum =
      vva1b_trim(rmc_read_file(marker));
  if (expected_checksum.size() != 64 ||
      vva1b_file_sha256(path) != expected_checksum) {
    throw std::runtime_error("VVA-1B cache checksum failed");
  }
  torch::serialize::InputArchive root;
  root.load_from(path.string(), device);
  torch::Tensor schema{}, complete{}, protocol{}, source{}, executable{};
  torch::Tensor module{}, control{}, readout{}, data_manifest{}, custody_manifest{};
  torch::Tensor ssl_data{}, ssl_mask{}, ssl_target{}, saved_seed{}, steps{};
  torch::Tensor arm_count{}, learning_rate{}, clip_norm{}, objective{}, flags{};
  root.read("meta/schema", schema);
  root.read("meta/complete", complete);
  root.read("meta/protocol_sha256", protocol);
  root.read("meta/source_sha256", source);
  root.read("meta/executable_sha256", executable);
  root.read("meta/module_sha256", module);
  root.read("meta/control_source", control);
  root.read("meta/readout_policy", readout);
  root.read("meta/data_manifest", data_manifest);
  root.read("meta/custody_manifest", custody_manifest);
  root.read("meta/ssl_data_hash", ssl_data);
  root.read("meta/ssl_mask_hash", ssl_mask);
  root.read("meta/ssl_target_hash", ssl_target);
  root.read("meta/seed", saved_seed);
  root.read("meta/steps", steps);
  root.read("meta/arm_count", arm_count);
  root.read("meta/optimizer_learning_rate", learning_rate);
  root.read("meta/gradient_clip_norm", clip_norm);
  root.read("meta/objective", objective);
  root.read("meta/result_flags", flags);
  const auto complete_value = vva1b_int64_vector(complete);
  const auto seed_value = vva1b_int64_vector(saved_seed);
  const auto steps_value = vva1b_int64_vector(steps);
  const auto arms_value = vva1b_int64_vector(arm_count);
  const auto lr_value = vva1b_double_vector(learning_rate);
  const auto clip_value = vva1b_double_vector(clip_norm);
  const auto objective_value = vva1b_int64_vector(objective);
  const auto flag_value = vva1b_int64_vector(flags);
  const bool metadata_exact =
      oca_tensor_string(schema) == kVva1bCacheSchema &&
      complete_value == std::vector<int64_t>{1} &&
      oca_tensor_string(protocol) == kVva1bProtocolSha256 &&
      oca_tensor_string(source) == custody.source_sha256 &&
      oca_tensor_string(executable) == custody.executable_sha256 &&
      oca_tensor_string(module) == kVva1bModuleSha256 &&
      oca_tensor_string(control) == "joint_retrain_vva1b" &&
      oca_tensor_string(readout) == kVva1bReadoutPolicy &&
      oca_tensor_string(data_manifest) == vva1b_data_manifest(data_identity) &&
      oca_tensor_string(custody_manifest) == vva1b_custody_manifest(custody) &&
      oca_tensor_string(ssl_data) ==
          oca_hex_u64(hash_tensor_stable_bytes(ssl.data)) &&
      oca_tensor_string(ssl_mask) ==
          oca_hex_u64(hash_tensor_stable_bytes(ssl.mask)) &&
      oca_tensor_string(ssl_target) ==
          oca_hex_u64(hash_tensor_stable_bytes(ssl.target)) &&
      seed_value == std::vector<int64_t>{seed} &&
      steps_value == std::vector<int64_t>{kVva1bSteps} &&
      arms_value == std::vector<int64_t>{3} &&
      lr_value == std::vector<double>{kVva1bLearningRate} &&
      clip_value == std::vector<double>{kVva1bClipNorm} &&
      objective_value == std::vector<int64_t>{8, 512, 512} &&
      flag_value == std::vector<int64_t>{1, 1, 1};
  if (!metadata_exact) {
    throw std::runtime_error("VVA-1B cache metadata failed");
  }

  result = Vva1bSeedTraining{};
  result.metadata_exact = true;
  result.interleaving_exact = true;
  result.resumed = true;
  result.pass = true;
  result.models.reserve(3);
  for (std::size_t arm = 0; arm < 3; ++arm) {
    const std::string prefix = "arm_" + std::to_string(arm) + "/";
    torch::Tensor name{}, policy{}, config_manifest{};
    root.read(prefix + "name", name);
    root.read(prefix + "policy", policy);
    root.read(prefix + "config_manifest", config_manifest);
    if (oca_tensor_string(name) != kVva1bArmNames[arm] ||
        oca_tensor_string(policy) !=
            mtf::mtf_vicreg_view_pairing_policy_name(kVva1bPolicies[arm]) ||
        oca_tensor_string(config_manifest) !=
            canonical_config_manifest(vva1b_config(device, arm))) {
      throw std::runtime_error("VVA-1B cache arm manifest failed");
    }
    set_paired_rng(seed, device);
    auto model = mtf::MtfJepaMaeVicreg(vva1b_config(device, arm));
    torch::serialize::InputArchive model_archive;
    root.read(prefix + "model", model_archive);
    model->load(model_archive);
    model->train();
    result.models.push_back(model);
    auto &receipt = result.receipts[arm];
    const auto read_u64 = [&](std::string_view key) {
      torch::Tensor value{};
      root.read(prefix + std::string(key), value);
      return oca_u64_le_bytes_vector(value);
    };
    receipt.row_hashes = read_u64("row_hashes");
    receipt.target_mask_hashes = read_u64("target_mask_hashes");
    receipt.context_mask_hashes = read_u64("context_mask_hashes");
    receipt.branch_a_token_mask_hashes =
        read_u64("branch_a_token_mask_hashes");
    receipt.branch_b_token_mask_hashes =
        read_u64("branch_b_token_mask_hashes");
    receipt.branch_a_sample_valid_hashes =
        read_u64("branch_a_sample_valid_hashes");
    receipt.branch_b_sample_valid_hashes =
        read_u64("branch_b_sample_valid_hashes");
    receipt.global_validity_mask_hashes =
        read_u64("global_validity_mask_hashes");
    torch::Tensor drawn{}, used{}, pre_rng{}, post_rng{};
    root.read(prefix + "drawn_view_hashes", drawn);
    root.read(prefix + "used_view_hashes", used);
    root.read(prefix + "pre_rng_hashes", pre_rng);
    root.read(prefix + "post_rng_hashes", post_rng);
    receipt.drawn_view_hashes = vva1b_unflatten_view_digests(drawn);
    receipt.used_view_hashes = vva1b_unflatten_view_digests(used);
    receipt.pre_rng_hashes = vva1b_unflatten_rng_digests(pre_rng);
    receipt.post_rng_hashes = vva1b_unflatten_rng_digests(post_rng);
    const auto read_double = [&](std::string_view key) {
      torch::Tensor value{};
      root.read(prefix + std::string(key), value);
      return vva1b_double_vector(value);
    };
    receipt.total_losses = read_double("total_losses");
    for (std::size_t component = 0; component < 3; ++component) {
      receipt.component_losses[component] =
          read_double("component_" + std::to_string(component));
    }
    receipt.gradient_norms = read_double("gradient_norms");
    receipt.served_update_norms = read_double("served_update_norms");
    receipt.clip_factors = read_double("clip_factors");
    torch::Tensor valid_rows{}, encoder_calls{}, projector_calls{}, scalars{};
    torch::Tensor receipt_flags{};
    root.read(prefix + "global_valid_rows", valid_rows);
    root.read(prefix + "encoder_call_counts", encoder_calls);
    root.read(prefix + "projector_call_counts", projector_calls);
    root.read(prefix + "receipt_scalars", scalars);
    root.read(prefix + "receipt_flags", receipt_flags);
    receipt.global_valid_rows = vva1b_int64_vector(valid_rows);
    receipt.encoder_call_counts = vva1b_int64_vector(encoder_calls);
    receipt.projector_call_counts = vva1b_int64_vector(projector_calls);
    const auto scalar = vva1b_double_vector(scalars);
    const auto receipt_flag = vva1b_int64_vector(receipt_flags);
    if (scalar.size() != 6 || receipt_flag.size() != 14) {
      throw std::runtime_error("VVA-1B cache receipt shape failed");
    }
    receipt.all_trainable_delta = scalar[0];
    receipt.served_delta = scalar[1];
    receipt.predictor_delta = scalar[2];
    receipt.mae_decoder_delta = scalar[3];
    receipt.vicreg_head_delta = scalar[4];
    receipt.target_ema_delta = scalar[5];
    receipt.steps = receipt_flag[0];
    receipt.adam_steps = receipt_flag[1];
    receipt.ema_steps = receipt_flag[2];
    receipt.clipping_count = receipt_flag[3];
    receipt.initialization_exact = receipt_flag[4] == 1;
    receipt.row_schedule_exact = receipt_flag[5] == 1;
    receipt.mask_schedule_exact = receipt_flag[6] == 1;
    receipt.ordinary_draw_schedule_exact = receipt_flag[7] == 1;
    receipt.rng_schedule_exact = receipt_flag[8] == 1;
    receipt.treatment_semantics_exact = receipt_flag[9] == 1;
    receipt.global_validity_exact = receipt_flag[10] == 1;
    receipt.finite = receipt_flag[11] == 1;
    receipt.expected_partitions = receipt_flag[12] == 1;
    receipt.pass = receipt_flag[13] == 1;
    if (!vva1b_receipt_reconstructs(receipt, ssl, seed, arm)) {
      throw std::runtime_error("VVA-1B cache receipt validation failed");
    }
  }
  for (std::size_t arm = 1; arm < 3; ++arm) {
    for (std::size_t step = 0; step < kVva1bSteps; ++step) {
      const auto rng_equal = [](const GeneratorStateDigest &left,
                                const GeneratorStateDigest &right) {
        return left.cpu == right.cpu && left.cuda == right.cuda;
      };
      result.interleaving_exact =
          result.interleaving_exact &&
          result.receipts[arm].row_hashes[step] ==
              result.receipts[0].row_hashes[step] &&
          result.receipts[arm].target_mask_hashes[step] ==
              result.receipts[0].target_mask_hashes[step] &&
          result.receipts[arm].context_mask_hashes[step] ==
              result.receipts[0].context_mask_hashes[step] &&
          weak_view_digests_equal(
              result.receipts[arm].drawn_view_hashes[step],
              result.receipts[0].drawn_view_hashes[step]) &&
          rng_equal(result.receipts[arm].pre_rng_hashes[step],
                    result.receipts[0].pre_rng_hashes[step]) &&
          rng_equal(result.receipts[arm].post_rng_hashes[step],
                    result.receipts[0].post_rng_hashes[step]);
    }
  }
  result.pass = result.metadata_exact && result.interleaving_exact &&
                std::all_of(result.receipts.begin(), result.receipts.end(),
                            [](const auto &value) { return value.pass; });
  if (!result.pass) {
    throw std::runtime_error("VVA-1B loaded cache failed reconstruction");
  }
  return true;
}

[[nodiscard]] Vva1bSeedTraining vva1b_train_or_resume_seed(
    const Dataset &ssl, const Vva1bDataIdentity &data_identity,
    const Vva1bCustody &custody, const torch::Device &device, int64_t seed) {
  Vva1bSeedTraining result{};
  const bool resumed = vva1b_load_seed_cache(
      ssl, data_identity, custody, device, seed, result);
  if (!resumed) {
    result = vva1b_train_seed(ssl, device, seed);
    if (!result.pass) {
      throw std::runtime_error("VVA-1B fresh seed training failed");
    }
    vva1b_save_seed_cache(ssl, data_identity, custody, device, seed, result);
    Vva1bSeedTraining validated{};
    if (!vva1b_load_seed_cache(ssl, data_identity, custody, device, seed,
                               validated)) {
      throw std::runtime_error("VVA-1B committed cache did not reload");
    }
    result = std::move(validated);
    result.resumed = false;
  }
  ++g_vva1b_recovery_validated_seed_caches;
  g_vva1b_validated_replacement_adam_updates += kVva1bUpdatesPerSeed;
  g_vva1b_validated_replacement_ema_updates += kVva1bUpdatesPerSeed;
  if (!resumed) {
    g_vva1b_current_fresh_committed_adam_updates += kVva1bUpdatesPerSeed;
    g_vva1b_current_fresh_committed_ema_updates += kVva1bUpdatesPerSeed;
  }
  const auto path = vva1b_seed_cache_path(seed);
  std::cout << "vva1b.cache.seed_" << seed << ".resumed=" << resumed << '\n';
  std::cout << "vva1b.cache.seed_" << seed << ".path="
            << path.generic_string() << '\n';
  std::cout << "vva1b.cache.seed_" << seed << ".sha256="
            << vva1b_file_sha256(path) << '\n';
  std::cout << "vva1b.cache.seed_" << seed << ".complete=true\n"
            << std::flush;
  return result;
}

[[nodiscard]] Vva1bSeedEvaluations
vva1b_arm_evaluations(const Vva1bEvaluations &evaluations,
                      std::size_t arm) {
  Vva1bSeedEvaluations result{};
  for (std::size_t seed = 0; seed < 3; ++seed) {
    result[seed] = evaluations[seed][arm];
  }
  return result;
}

[[nodiscard]] Vva1bBootstrapAreaTable vva1b_bootstrap_area_table(
    const Vva1bEvaluations &evaluations, const torch::Tensor &target,
    const std::vector<torch::Tensor> &bootstrap_rows) {
  Vva1bBootstrapAreaTable result;
  result.reserve(bootstrap_rows.size());
  for (const auto &rows : bootstrap_rows) {
    std::array<std::array<double, 3>, 3> replicate{};
    for (std::size_t seed = 0; seed < 3; ++seed) {
      for (std::size_t arm = 0; arm < 3; ++arm) {
        replicate[seed][arm] =
            rssm_resampled_area(evaluations[seed][arm].probe, target, rows)
                .macro;
      }
    }
    result.push_back(std::move(replicate));
  }
  return result;
}

[[nodiscard]] Vva1bContrast vva1b_contrast(
    const Vva1bEvaluations &evaluations,
    const Vva1bBootstrapAreaTable &bootstrap, std::size_t reference,
    std::size_t candidate) {
  if (reference >= 3 || candidate >= 3 || reference == candidate) {
    throw std::runtime_error("VVA-1B contrast index failed");
  }
  Vva1bContrast result{};
  for (std::size_t seed = 0; seed < 3; ++seed) {
    result.per_seed[seed] = evaluations[seed][candidate].probe.area -
                            evaluations[seed][reference].probe.area;
    result.summary.point += result.per_seed[seed] / 3.0;
    result.summary.positive_seed_count += result.per_seed[seed] > 0.0 ? 1 : 0;
    const auto reference_family =
        rssm_family_areas(evaluations[seed][reference].probe);
    const auto candidate_family =
        rssm_family_areas(evaluations[seed][candidate].probe);
    for (std::size_t family = 0; family < kFamilies; ++family) {
      result.family[family] +=
          (candidate_family[family] - reference_family[family]) / 3.0;
    }
  }
  std::vector<double> replicates;
  replicates.reserve(bootstrap.size());
  for (const auto &table : bootstrap) {
    double value = 0.0;
    for (std::size_t seed = 0; seed < 3; ++seed) {
      value += (table[seed][candidate] - table[seed][reference]) / 3.0;
    }
    replicates.push_back(value);
  }
  const auto interval = percentile_interval(std::move(replicates));
  result.summary.low = interval.low;
  result.summary.high = interval.high;
  result.mechanism_supported =
      std::isfinite(result.summary.point) &&
      std::isfinite(result.summary.low) &&
      result.summary.point >= kVva1bCausalFloor && result.summary.low > 0.0 &&
      result.summary.positive_seed_count == 3;
  result.rescue_sized_ruled_out =
      std::isfinite(result.summary.high) &&
      result.summary.high < kVva1bCausalFloor;
  result.family_floor_pass =
      std::all_of(result.family.begin(), result.family.end(),
                  [](double value) {
                    return std::isfinite(value) &&
                           value >= kVva1bFamilyFloor;
                  });
  return result;
}

[[nodiscard]] bool vva1b_practically_equivalent(const Vva1bContrast &value) {
  return value.summary.low >= -kVva1bCausalFloor &&
         value.summary.high <= kVva1bCausalFloor;
}

[[nodiscard]] bool vva1b_materially_negative(const rmc_gate::Contrast &value,
                                             const std::array<double, 3> &seed) {
  return value.point <= -kVva1bCausalFloor && value.high < 0.0 &&
         std::all_of(seed.begin(), seed.end(),
                     [](double effect) { return effect < 0.0; });
}

[[nodiscard]] Vva1bSafeguards vva1b_safeguards(
    const RmcSummary &summary,
    const Vva1bSeedEvaluations &evaluations) {
  Vva1bSafeguards result{};
  const auto &gate = summary.gate.neutral;
  result.raw_noninferiority = gate.raw_noninferiority_pass;
  result.order_point = gate.order_point_pass;
  result.order_lower = gate.order_lower_pass;
  result.order_retention = gate.order_retention_pass;
  result.continuous_shuffle = gate.continuous_shuffle_pass;
  result.order_shuffle = gate.order_shuffle_pass;
  result.geometry = true;
  for (std::size_t seed = 0; seed < 3; ++seed) {
    for (std::size_t channel = 0; channel < kChannels; ++channel) {
      const auto &geometry = evaluations[seed].geometry[channel];
      result.effective[seed][channel] =
          geometry.effective_rank_ratio >= 0.25;
      result.participation[seed][channel] =
          geometry.participation_rank_ratio >= 0.25;
      result.top[seed][channel] = geometry.top_eigenvalue_share <= 0.80;
      result.active[seed][channel] =
          geometry.active_dimension_fraction >= 0.75;
      result.geometry =
          result.geometry && result.effective[seed][channel] &&
          result.participation[seed][channel] && result.top[seed][channel] &&
          result.active[seed][channel];
    }
  }
  result.pass = result.raw_noninferiority && result.order_point &&
                result.order_lower && result.order_retention &&
                result.continuous_shuffle && result.order_shuffle &&
                result.geometry;
  return result;
}

[[nodiscard]] bool vva1b_new_safeguard_failure(
    const Vva1bSafeguards &reference, const Vva1bSafeguards &candidate) {
  const bool scalar_failure =
      (reference.raw_noninferiority && !candidate.raw_noninferiority) ||
      (reference.order_point && !candidate.order_point) ||
      (reference.order_lower && !candidate.order_lower) ||
      (reference.order_retention && !candidate.order_retention) ||
      (reference.continuous_shuffle && !candidate.continuous_shuffle) ||
      (reference.order_shuffle && !candidate.order_shuffle);
  bool geometry_failure = false;
  for (std::size_t seed = 0; seed < 3; ++seed) {
    for (std::size_t channel = 0; channel < kChannels; ++channel) {
      geometry_failure =
          geometry_failure ||
          (reference.effective[seed][channel] &&
           !candidate.effective[seed][channel]) ||
          (reference.participation[seed][channel] &&
           !candidate.participation[seed][channel]) ||
          (reference.top[seed][channel] && !candidate.top[seed][channel]) ||
          (reference.active[seed][channel] &&
           !candidate.active[seed][channel]);
    }
  }
  return scalar_failure || geometry_failure;
}

[[nodiscard]] std::string
vva1b_mechanism_route(const Vva1bContrast &pairing,
                      const Vva1bContrast &corruption,
                      const Vva1bContrast &complete,
                      const std::array<Vva1bContrast, 3> &arm_minus_anchor,
                      bool mechanics) {
  if (!mechanics) {
    return "invalid_numeric_or_mechanics";
  }
  if (pairing.mechanism_supported && corruption.mechanism_supported) {
    return "pairing_and_corruption_contribute";
  }
  if (pairing.mechanism_supported &&
      vva1b_practically_equivalent(corruption)) {
    return "independent_pairing_principal_defect";
  }
  if (vva1b_practically_equivalent(pairing) &&
      corruption.mechanism_supported) {
    return "corrupted_input_principal_defect";
  }
  if (pairing.mechanism_supported && vva1b_practically_equivalent(complete) &&
      vva1b_materially_negative(corruption.summary,
                                corruption.per_seed)) {
    return "opposing_view_effects_cancel";
  }
  if (vva1b_materially_negative(pairing.summary, pairing.per_seed)) {
    return "independent_view_invariance_mitigates_deeper_defect";
  }
  bool all_anchor_harmful = true;
  for (const auto &contrast : arm_minus_anchor) {
    all_anchor_harmful =
        all_anchor_harmful &&
        vva1b_materially_negative(contrast.summary, contrast.per_seed);
  }
  if (vva1b_practically_equivalent(pairing) &&
      vva1b_practically_equivalent(corruption) && all_anchor_harmful) {
    return "intrinsic_view_line_closed";
  }
  if (pairing.rescue_sized_ruled_out && corruption.rescue_sized_ruled_out &&
      complete.rescue_sized_ruled_out) {
    return "no_rescue_sized_view_effect";
  }
  return "mixed_or_imprecise_view_effect";
}

[[nodiscard]] Vva1bContrast vva1b_summary_contrast(
    const Vva1bSeedEvaluations &reference,
    const Vva1bSeedEvaluations &candidate, const RmcSummary &summary) {
  Vva1bContrast result{};
  result.summary = summary.candidate[0].gate.trained_minus_initialization;
  result.family = summary.candidate[0].gate.learned_family_deltas;
  for (std::size_t seed = 0; seed < 3; ++seed) {
    result.per_seed[seed] =
        candidate[seed].probe.area - reference[seed].probe.area;
  }
  result.mechanism_supported =
      result.summary.point >= kVva1bCausalFloor && result.summary.low > 0.0 &&
      std::all_of(result.per_seed.begin(), result.per_seed.end(),
                  [](double value) { return value > 0.0; });
  result.rescue_sized_ruled_out =
      result.summary.high < kVva1bCausalFloor;
  result.family_floor_pass =
      std::all_of(result.family.begin(), result.family.end(),
                  [](double value) {
                    return std::isfinite(value) &&
                           value >= kVva1bFamilyFloor;
                  });
  return result;
}

[[nodiscard]] Vva1bCandidatePredicates vva1b_candidate_predicates(
    const Vva1bContrast &direct, const Vva1bContrast &anchor,
    const Vva1bSafeguards &safeguards) {
  Vva1bCandidatePredicates result{};
  result.candidate_eligible = direct.safe_direct_candidate;
  result.all_safeguards_pass = safeguards.pass;
  std::size_t positive_families = 0;
  for (const double value : anchor.family) {
    positive_families += value > 0.0 ? 1U : 0U;
  }
  result.objective_made_safe =
      result.candidate_eligible &&
      anchor.summary.low > kVva1bAnchorNoninferiority &&
      result.all_safeguards_pass;
  result.representation_rescue =
      result.candidate_eligible && anchor.mechanism_supported &&
      positive_families >= 3 && anchor.family_floor_pass &&
      result.all_safeguards_pass;
  return result;
}

[[nodiscard]] Vva1bClassification vva1b_classify(
    bool mechanics, bool any_mechanism_supported, bool selected,
    const Vva1bCandidatePredicates &predicates) {
  if (!mechanics) {
    return Vva1bClassification::invalid_numeric_or_mechanics;
  }
  if (!selected) {
    return any_mechanism_supported
               ? Vva1bClassification::supported_mechanism_without_safe_candidate
               : Vva1bClassification::no_candidate;
  }
  if (predicates.representation_rescue) {
    return Vva1bClassification::representation_rescue;
  }
  return predicates.objective_made_safe
             ? Vva1bClassification::objective_made_safe
             : Vva1bClassification::mitigation_only;
}

void vva1b_emit_custody(const Vva1bCustody &value) {
  std::cout << "vva1b.custody.protocol_sha256=" << value.protocol_sha256
            << '\n';
  std::cout << "vva1b.custody.protocol_amendment_a1_sha256="
            << value.protocol_amendment_a1_sha256 << '\n';
  std::cout << "vva1b.custody.protocol_amendment_a2_sha256="
            << value.protocol_amendment_a2_sha256 << '\n';
  std::cout << "vva1b.custody.protocol_amendment_a3_sha256="
            << value.protocol_amendment_a3_sha256 << '\n';
  std::cout << "vva1b.custody.failed_attempt_sha256="
            << value.failed_attempt_sha256 << '\n';
  std::cout << "vva1b.custody.seam_audit_sha256="
            << value.seam_audit_sha256 << '\n';
  std::cout << "vva1b.custody.build_manifest_sha256="
            << value.build_manifest_sha256 << '\n';
  std::cout << "vva1b.custody.source_sha256=" << value.source_sha256 << '\n';
  std::cout << "vva1b.custody.executable_sha256="
            << value.executable_sha256 << '\n';
  std::cout << "vva1b.custody.post_seam_module_sha256="
            << kVva1bModuleSha256 << '\n';
  std::cout << "vva1b.custody.pre_seam_module_sha256="
            << kVva1bPreSeamModuleSha256 << '\n';
  std::cout << "vva1b.custody.protocol=" << value.protocol << '\n';
  std::cout << "vva1b.custody.protocol_amendment_a1="
            << value.protocol_amendment_a1 << '\n';
  std::cout << "vva1b.custody.protocol_amendment_a2="
            << value.protocol_amendment_a2 << '\n';
  std::cout << "vva1b.custody.protocol_amendment_a3="
            << value.protocol_amendment_a3 << '\n';
  std::cout << "vva1b.custody.failed_attempt=" << value.failed_attempt
            << '\n';
  std::cout << "vva1b.custody.seam_audit=" << value.seam_audit << '\n';
  std::cout << "vva1b.custody.build_manifest=" << value.build_manifest
            << '\n';
  std::cout << "vva1b.custody.old_protocol=" << value.old_protocol << '\n';
  std::cout << "vva1b.custody.old_findings=" << value.old_findings << '\n';
  std::cout << "vva1b.custody.old_harness=" << value.old_harness << '\n';
  std::cout << "vva1b.custody.old_log=" << value.old_log << '\n';
  std::cout << "vva1b.custody.settled_fields=" << value.settled_fields
            << '\n';
  std::cout << "vva1b.custody.ima1=" << value.ima1 << '\n';
  std::cout << "vva1b.custody.oaa1=" << value.oaa1 << '\n';
  std::cout << "vva1b.custody.gpv1_protocol=" << value.gpv1_protocol
            << '\n';
  std::cout << "vva1b.custody.gpv1=" << value.gpv1 << '\n';
  std::cout << "vva1b.custody.gpv1_log=" << value.gpv1_log << '\n';
  std::cout << "vva1b.custody.ima5=" << value.ima5 << '\n';
  std::cout << "vva1b.custody.pre_seam_module_bound="
            << value.pre_seam_module_bound << '\n';
  std::cout << "vva1b.custody.post_seam_module="
            << value.post_seam_module << '\n';
  std::cout << "vva1b.custody.anchors=" << value.anchors << '\n';
  std::cout << "vva1b.custody.current_caches=" << value.current_caches
            << '\n';
  std::cout << "vva1b.custody.pass=" << value.pass << '\n';
}

void vva1b_emit_data_identity(const std::string &root,
                              const Vva1bDataIdentity &value) {
  std::cout << root << ".normalization.mean="
            << oca_hex_u64(value.normalization_mean) << '\n';
  std::cout << root << ".normalization.inv_std="
            << oca_hex_u64(value.normalization_inv_std) << '\n';
  constexpr std::array<const char *, 4> fields{"groups", "data", "mask",
                                               "target"};
  const auto emit = [&](std::string_view name,
                        const std::array<uint64_t, 4> &hashes) {
    for (std::size_t index = 0; index < hashes.size(); ++index) {
      std::cout << root << '.' << name << '.' << fields[index] << '='
                << oca_hex_u64(hashes[index]) << '\n';
    }
  };
  emit("ssl", value.ssl);
  emit("fit", value.fit);
  emit("selection", value.selection);
  emit("development", value.development);
  std::cout << root << ".bootstrap.table=" << oca_hex_u64(value.bootstrap)
            << '\n';
  std::cout << root << ".confirmation_sealed=" << value.confirmation_sealed
            << '\n';
  std::cout << root << ".pass=" << value.pass << '\n';
}

void vva1b_emit_self_test(const Vva1bSelfTest &value) {
  vva1b_emit_data_identity("vva1b.self_test.data", value.data);
  std::cout << "vva1b.self_test.policy_inventory=" << value.policy_inventory
            << '\n';
  std::cout << "vva1b.self_test.default_policy=" << value.default_policy
            << '\n';
  std::cout << "vva1b.self_test.post_draw_assignment="
            << value.post_draw_assignment << '\n';
  std::cout << "vva1b.self_test.both_draws_consumed="
            << value.both_draws_consumed << '\n';
  std::cout << "vva1b.self_test.rng_parity=" << value.rng_parity << '\n';
  std::cout << "vva1b.self_test.identical_invariance_zero="
            << value.identical_invariance_zero << '\n';
  std::cout << "vva1b.self_test.identical_invariance_gradient_zero="
            << value.identical_invariance_gradient_zero << '\n';
  std::cout << "vva1b.self_test.loss_reconstruction="
            << value.loss_reconstruction << '\n';
  std::cout << "vva1b.self_test.inactive_cosine=" << value.inactive_cosine
            << '\n';
  std::cout << "vva1b.self_test.digest_codec=" << value.digest_codec << '\n';
  std::cout << "vva1b.self_test.cache_integer_archive_roundtrip="
            << value.cache_integer_archive_roundtrip << '\n';
  std::cout << "vva1b.self_test.cache_resume_accounting="
            << value.cache_resume_accounting << '\n';
  std::cout << "vva1b.self_test.post_resume_failure_accounting="
            << value.post_resume_failure_accounting << '\n';
  std::cout << "vva1b.self_test.cache_codec_optimizer_updates="
            << value.cache_codec_optimizer_updates << '\n';
  std::cout << "vva1b.self_test.cache_codec_ema_updates="
            << value.cache_codec_ema_updates << '\n';
  std::cout << "vva1b.self_test.causal_safety_separated="
            << value.causal_safety_separated << '\n';
  std::cout << "vva1b.self_test.cache_plan_full_triad="
            << value.cache_plan_full_triad << '\n';
  std::cout << "vva1b.self_test.pass=" << value.pass << '\n';
}

void vva1b_emit_virtual(const std::string &root,
                        const Vva1bVirtualStepDiagnostic &value) {
  std::cout << root << ".active=" << value.active << '\n';
  std::cout << root << ".optimizer_step=" << value.optimizer_step << '\n';
  std::cout << root << ".ema_step=" << value.ema_step << '\n';
  std::cout << root << ".parameter_delta=" << value.parameter_delta << '\n';
  std::cout << root << ".parameter_state_exact="
            << value.parameter_state_exact << '\n';
  std::cout << root << ".optimizer_state_exact="
            << value.optimizer_state_exact << '\n';
  std::cout << root << ".ema_state_exact=" << value.ema_state_exact << '\n';
  std::cout << root << ".reversal.before=" << value.reversal_before << '\n';
  std::cout << root << ".reversal.after=" << value.reversal_after << '\n';
  std::cout << root << ".reversal.delta=" << value.reversal_delta << '\n';
  std::cout << root << ".family.order_regime.before="
            << value.order_regime_before << '\n';
  std::cout << root << ".family.order_regime.after="
            << value.order_regime_after << '\n';
  std::cout << root << ".family.order_regime.delta="
            << value.order_regime_delta << '\n';
  std::cout << root << ".family.cross_channel.before="
            << value.cross_channel_before << '\n';
  std::cout << root << ".family.cross_channel.after="
            << value.cross_channel_after << '\n';
  std::cout << root << ".family.cross_channel.delta="
            << value.cross_channel_delta << '\n';
  for (std::size_t channel = 0; channel < kChannels; ++channel) {
    const auto emit_geometry = [&](std::string_view phase,
                                   const Geometry &geometry) {
      const std::string prefix = root + ".geometry.channel_" +
                                 std::to_string(channel) + "." +
                                 std::string(phase);
      std::cout << prefix << ".effective=" << geometry.effective_rank_ratio
                << '\n';
      std::cout << prefix << ".participation="
                << geometry.participation_rank_ratio << '\n';
      std::cout << prefix << ".top=" << geometry.top_eigenvalue_share << '\n';
      std::cout << prefix << ".active="
                << geometry.active_dimension_fraction << '\n';
    };
    emit_geometry("before", value.geometry_before[channel]);
    emit_geometry("after", value.geometry_after[channel]);
  }
  std::cout << root << ".finite=" << value.finite << '\n';
}

void vva1b_emit_stage0(const Vva1bStage0 &value) {
  constexpr std::array<const char *, 3> components{"invariance", "variance",
                                                   "covariance"};
  constexpr std::array<const char *, 3> partitions{"tokenizer", "encoder",
                                                   "projector"};
  constexpr std::array<const char *, 3> pairs{"invariance_variance",
                                              "invariance_covariance",
                                              "variance_covariance"};
  constexpr std::array<const char *, 4> virtual_names{
      "full", "invariance", "variance", "covariance"};
  for (const auto &seed : value.seed) {
    const std::string seed_root =
        "vva1b.stage0.seed_" + std::to_string(seed.seed);
    std::cout << seed_root << ".initialization_exact="
              << seed.initialization_exact << '\n';
    std::cout << seed_root << ".manifests_exact_except_policy="
              << seed.manifests_exact_except_policy << '\n';
    std::cout << seed_root << ".drawn_views_exact="
              << seed.drawn_views_exact << '\n';
    std::cout << seed_root << ".pre_rng_exact=" << seed.pre_rng_exact
              << '\n';
    std::cout << seed_root << ".post_rng_exact=" << seed.post_rng_exact
              << '\n';
    std::cout << seed_root << ".jepa_masks_exact=" << seed.jepa_masks_exact
              << '\n';
    std::cout << seed_root << ".retained_rows_exact="
              << seed.retained_rows_exact << '\n';
    std::cout << seed_root << ".default_output_exact="
              << seed.default_output_exact << '\n';
    std::cout << seed_root << ".default_components_exact="
              << seed.default_components_exact << '\n';
    std::cout << seed_root << ".default_loss_exact="
              << seed.default_loss_exact << '\n';
    std::cout << seed_root << ".default_gradients_exact="
              << seed.default_gradients_exact << '\n';
    std::cout << seed_root << ".default_rng_exact="
              << seed.default_rng_exact << '\n';
    std::cout << seed_root << ".default_update_exact="
              << seed.default_update_exact << '\n';
    for (std::size_t checkpoint = 0;
         checkpoint < seed.default_parity_update_indices.size();
         ++checkpoint) {
      const std::string parity_root =
          seed_root + ".default_seam_parity.index_" +
          std::to_string(seed.default_parity_update_indices[checkpoint]);
      std::cout << parity_root << ".update_index="
                << seed.default_parity_update_indices[checkpoint] << '\n';
      std::cout << parity_root << ".pass="
                << seed.default_parity_pass[checkpoint] << '\n';
    }
    std::cout << seed_root << ".historical_v0.metadata="
              << seed.cached_v0_metadata << '\n';
    std::cout << seed_root << ".historical_v0.row_match="
              << seed.cached_v0_row_match << '\n';
    std::cout << seed_root << ".historical_v0.loss_match="
              << seed.cached_v0_loss_match << '\n';
    std::cout << seed_root
              << ".historical_v0.first_update_receipts_available="
              << seed.cached_v0_first_update_receipts_available << '\n';
    std::cout << seed_root << ".historical_v0.clean_replay="
              << seed.cached_v0_clean_replay << '\n';
    std::cout << seed_root << ".historical_v0.reused="
              << seed.cache_reuse << '\n';
    for (std::size_t arm = 0; arm < 3; ++arm) {
      const auto &diagnostic = seed.arm[arm];
      const std::string root = seed_root + ".arm." + kVva1bArmNames[arm];
      std::cout << root << ".drawn_a.data_hash="
                << oca_hex_u64(diagnostic.drawn_view_hashes.view_a_data)
                << '\n';
      std::cout << root << ".drawn_a.feature_mask_hash="
                << oca_hex_u64(
                       diagnostic.drawn_view_hashes.view_a_feature_mask)
                << '\n';
      std::cout << root << ".drawn_b.data_hash="
                << oca_hex_u64(diagnostic.drawn_view_hashes.view_b_data)
                << '\n';
      std::cout << root << ".drawn_b.feature_mask_hash="
                << oca_hex_u64(
                       diagnostic.drawn_view_hashes.view_b_feature_mask)
                << '\n';
      std::cout << root << ".used_a.data_hash="
                << oca_hex_u64(diagnostic.used_view_hashes.view_a_data)
                << '\n';
      std::cout << root << ".used_a.feature_mask_hash="
                << oca_hex_u64(
                       diagnostic.used_view_hashes.view_a_feature_mask)
                << '\n';
      std::cout << root << ".used_b.data_hash="
                << oca_hex_u64(diagnostic.used_view_hashes.view_b_data)
                << '\n';
      std::cout << root << ".used_b.feature_mask_hash="
                << oca_hex_u64(
                       diagnostic.used_view_hashes.view_b_feature_mask)
                << '\n';
      std::cout << root << ".pre_rng.cpu="
                << oca_hex_u64(diagnostic.pre_rng_hashes.cpu) << '\n';
      std::cout << root << ".pre_rng.cuda="
                << oca_hex_u64(diagnostic.pre_rng_hashes.cuda) << '\n';
      std::cout << root << ".post_rng.cpu="
                << oca_hex_u64(diagnostic.post_rng_hashes.cpu) << '\n';
      std::cout << root << ".post_rng.cuda="
                << oca_hex_u64(diagnostic.post_rng_hashes.cuda) << '\n';
      std::cout << root << ".jepa.target_mask_hash="
                << oca_hex_u64(diagnostic.jepa_target_mask_hash) << '\n';
      std::cout << root << ".jepa.context_mask_hash="
                << oca_hex_u64(diagnostic.jepa_context_mask_hash) << '\n';
      std::cout << root << ".branch_a.token_mask_hash="
                << oca_hex_u64(diagnostic.branch_a_token_mask_hash) << '\n';
      std::cout << root << ".branch_b.token_mask_hash="
                << oca_hex_u64(diagnostic.branch_b_token_mask_hash) << '\n';
      std::cout << root << ".branch_a.sample_valid_hash="
                << oca_hex_u64(diagnostic.branch_a_sample_valid_hash) << '\n';
      std::cout << root << ".branch_b.sample_valid_hash="
                << oca_hex_u64(diagnostic.branch_b_sample_valid_hash) << '\n';
      std::cout << root << ".global_validity_mask_hash="
                << oca_hex_u64(diagnostic.global_validity_mask_hash) << '\n';
      std::cout << root << ".treatment_exact="
                << diagnostic.treatment_exact << '\n';
      std::cout << root << ".finite_zero_masked="
                << diagnostic.finite_zero_masked << '\n';
      std::cout << root << ".encoder_call_count="
                << diagnostic.encoder_call_count << '\n';
      std::cout << root << ".projector_call_count="
                << diagnostic.projector_call_count << '\n';
      std::cout << root << ".separate_forward_graphs="
                << diagnostic.separate_forward_graphs << '\n';
      std::cout << root << ".global_valid_rows="
                << diagnostic.global_valid_rows << '\n';
      std::cout << root << ".view_a_to_clean_rms="
                << diagnostic.view_a_to_clean_rms << '\n';
      std::cout << root << ".view_b_to_clean_rms="
                << diagnostic.view_b_to_clean_rms << '\n';
      std::cout << root << ".view_to_view_rms="
                << diagnostic.view_to_view_rms << '\n';
      std::cout << root << ".view_a_feature_mask_difference="
                << diagnostic.view_a_mask_difference << '\n';
      std::cout << root << ".view_b_feature_mask_difference="
                << diagnostic.view_b_mask_difference << '\n';
      std::cout << root << ".branch_token_mask_difference="
                << diagnostic.branch_token_mask_difference << '\n';
      std::cout << root << ".branch_sample_valid_difference="
                << diagnostic.branch_sample_valid_difference << '\n';
      std::cout << root << ".pooled_max_abs_difference="
                << diagnostic.pooled_max_abs_difference << '\n';
      std::cout << root << ".projected_max_abs_difference="
                << diagnostic.projected_max_abs_difference << '\n';
      std::cout << root << ".projected_a_below_floor="
                << diagnostic.projected_a_below_floor << '\n';
      std::cout << root << ".projected_b_below_floor="
                << diagnostic.projected_b_below_floor << '\n';
      std::cout << root << ".projected.effective="
                << diagnostic.projected_geometry.effective_rank_ratio << '\n';
      std::cout << root << ".projected.participation="
                << diagnostic.projected_geometry.participation_rank_ratio
                << '\n';
      std::cout << root << ".projected.top="
                << diagnostic.projected_geometry.top_eigenvalue_share << '\n';
      std::cout << root << ".total_loss=" << diagnostic.total_loss << '\n';
      std::cout << root << ".loss_reconstruction_abs="
                << diagnostic.reconstruction_abs << '\n';
      std::cout << root << ".loss_reconstruction_rel="
                << diagnostic.reconstruction_rel << '\n';
      std::cout << root << ".loss_reconstruction_exact="
                << diagnostic.reconstruction_exact << '\n';
      std::cout << root << ".independent_loss_reconstruction_close="
                << diagnostic.independent_reconstruction_close << '\n';
      std::cout << root << ".gradient_reconstruction_max_abs="
                << diagnostic.gradient_reconstruction_max_abs << '\n';
      std::cout << root << ".gradient_reconstruction_relative_l2="
                << diagnostic.gradient_reconstruction_relative_l2 << '\n';
      std::cout << root << ".gradient_reconstruction_exact="
                << diagnostic.gradient_reconstruction_exact << '\n';
      for (std::size_t component = 0; component < 3; ++component) {
        const auto &item = diagnostic.component[component];
        const std::string component_root =
            root + ".component." + components[component];
        std::cout << component_root << ".raw=" << item.raw << '\n';
        std::cout << component_root << ".weighted=" << item.weighted << '\n';
        std::cout << component_root << ".tokenizer_gradient_norm="
                  << item.tokenizer_gradient_norm << '\n';
        std::cout << component_root << ".encoder_gradient_norm="
                  << item.encoder_gradient_norm << '\n';
        std::cout << component_root << ".projector_gradient_norm="
                  << item.projector_gradient_norm << '\n';
      }
      for (std::size_t partition = 0; partition < 3; ++partition) {
        for (std::size_t pair = 0; pair < 3; ++pair) {
          const auto &cosine = diagnostic.cosine[partition][pair];
          const std::string cosine_root = root + ".cosine." +
                                          partitions[partition] + "." +
                                          pairs[pair];
          std::cout << cosine_root << ".active=" << cosine.active << '\n';
          std::cout << cosine_root << ".cosine_defined=" << cosine.active
                    << '\n';
          if (cosine.active) {
            std::cout << cosine_root << ".value=" << cosine.value << '\n';
          }
        }
      }
      for (std::size_t objective = 0; objective < 4; ++objective) {
        vva1b_emit_virtual(root + ".virtual." + virtual_names[objective],
                           diagnostic.virtual_step[objective]);
      }
      std::cout << root << ".pass=" << diagnostic.pass << '\n';
    }
    std::cout << seed_root << ".pass=" << seed.pass << '\n';
  }
  std::cout << "vva1b.stage0.cpu_self_test=" << value.cpu_self_test << '\n';
  std::cout << "vva1b.stage0.custody=" << value.custody << '\n';
  std::cout << "vva1b.stage0.explicit_default_seam_parity="
            << value.explicit_default_seam_parity << '\n';
  std::cout << "vva1b.stage0.treatment_mechanics="
            << value.treatment_mechanics << '\n';
  std::cout << "vva1b.stage0.historical_v0_reused="
            << value.cached_v0_reuse << '\n';
  std::cout << "vva1b.stage0.historical_v0_reuse_reason="
            << value.cached_v0_reuse_reason << '\n';
  std::cout << "vva1b.stage0.control_source=joint_retrain_vva1b\n";
  std::cout << "vva1b.stage0.full_triad_4608_required="
            << value.full_triad_4608_required << '\n';
  std::cout << "vva1b.stage0.pass=" << value.pass << '\n';
}

void vva1b_emit_receipt(const std::string &root,
                        const Vva1bReceipt &value) {
  const auto aggregate_u64 = [](const std::vector<uint64_t> &values) {
    uint64_t result = 0xcbf29ce484222325ULL;
    for (const auto value : values) {
      mix_hash_value(result, value);
    }
    return result;
  };
  std::cout << root << ".steps=" << value.steps << '\n';
  std::cout << root << ".adam_steps=" << value.adam_steps << '\n';
  std::cout << root << ".ema_steps=" << value.ema_steps << '\n';
  std::cout << root << ".clipping_count=" << value.clipping_count << '\n';
  std::cout << root << ".row_hashes=" << value.row_hashes.size() << '\n';
  std::cout << root << ".target_mask_hashes="
            << value.target_mask_hashes.size() << '\n';
  std::cout << root << ".context_mask_hashes="
            << value.context_mask_hashes.size() << '\n';
  std::cout << root << ".branch_a_token_mask_hashes="
            << value.branch_a_token_mask_hashes.size() << '\n';
  std::cout << root << ".branch_b_token_mask_hashes="
            << value.branch_b_token_mask_hashes.size() << '\n';
  std::cout << root << ".branch_a_sample_valid_hashes="
            << value.branch_a_sample_valid_hashes.size() << '\n';
  std::cout << root << ".branch_b_sample_valid_hashes="
            << value.branch_b_sample_valid_hashes.size() << '\n';
  std::cout << root << ".global_validity_mask_hashes="
            << value.global_validity_mask_hashes.size() << '\n';
  std::cout << root << ".drawn_view_hashes="
            << value.drawn_view_hashes.size() << '\n';
  std::cout << root << ".used_view_hashes="
            << value.used_view_hashes.size() << '\n';
  std::cout << root << ".pre_rng_hashes=" << value.pre_rng_hashes.size()
            << '\n';
  std::cout << root << ".post_rng_hashes=" << value.post_rng_hashes.size()
            << '\n';
  std::cout << root << ".encoder_call_counts="
            << value.encoder_call_counts.size() << '\n';
  std::cout << root << ".projector_call_counts="
            << value.projector_call_counts.size() << '\n';
  std::cout << root << ".row_schedule_digest="
            << oca_hex_u64(aggregate_u64(value.row_hashes)) << '\n';
  std::cout << root << ".target_mask_digest="
            << oca_hex_u64(aggregate_u64(value.target_mask_hashes)) << '\n';
  std::cout << root << ".context_mask_digest="
            << oca_hex_u64(aggregate_u64(value.context_mask_hashes)) << '\n';
  std::cout << root << ".served_delta=" << value.served_delta << '\n';
  std::cout << root << ".predictor_delta=" << value.predictor_delta << '\n';
  std::cout << root << ".mae_decoder_delta=" << value.mae_decoder_delta
            << '\n';
  std::cout << root << ".vicreg_head_delta=" << value.vicreg_head_delta
            << '\n';
  std::cout << root << ".target_ema_delta=" << value.target_ema_delta
            << '\n';
  std::cout << root << ".row_schedule_exact=" << value.row_schedule_exact
            << '\n';
  std::cout << root << ".mask_schedule_exact=" << value.mask_schedule_exact
            << '\n';
  std::cout << root << ".ordinary_draw_schedule_exact="
            << value.ordinary_draw_schedule_exact << '\n';
  std::cout << root << ".rng_schedule_exact=" << value.rng_schedule_exact
            << '\n';
  std::cout << root << ".treatment_semantics_exact="
            << value.treatment_semantics_exact << '\n';
  std::cout << root << ".global_validity_exact="
            << value.global_validity_exact << '\n';
  std::cout << root << ".finite=" << value.finite << '\n';
  std::cout << root << ".expected_partitions=" << value.expected_partitions
            << '\n';
  std::cout << root << ".pass=" << value.pass << '\n';
}

void vva1b_emit_evaluation(const std::string &root,
                           const RmcEvaluation &value) {
  std::cout << root << ".aulc=" << value.probe.area << '\n';
  std::cout << root << ".order_aulc=" << value.order.area << '\n';
  std::cout << root << ".continuous_shuffle_aulc="
            << value.shuffled_probe.area << '\n';
  std::cout << root << ".order_shuffle_aulc="
            << value.shuffled_order.area << '\n';
  const auto family = rssm_family_areas(value.probe);
  for (std::size_t index = 0; index < kFamilies; ++index) {
    std::cout << root << ".family_" << kFamilyNames[index] << '='
              << family[index] << '\n';
  }
  for (std::size_t channel = 0; channel < kChannels; ++channel) {
    const auto &geometry = value.geometry[channel];
    const std::string prefix =
        root + ".geometry.channel_" + std::to_string(channel);
    std::cout << prefix << ".effective=" << geometry.effective_rank_ratio
              << '\n';
    std::cout << prefix << ".participation="
              << geometry.participation_rank_ratio << '\n';
    std::cout << prefix << ".top=" << geometry.top_eigenvalue_share << '\n';
    std::cout << prefix << ".active="
              << geometry.active_dimension_fraction << '\n';
  }
}

void vva1b_emit_contrast(const std::string &root,
                         const Vva1bContrast &value,
                         bool emit_candidate_predicates = false) {
  rmc_emit_contrast(root, value.summary);
  for (std::size_t seed = 0; seed < 3; ++seed) {
    std::cout << root << ".seed_" << kAttributionSeeds[seed] << '='
              << value.per_seed[seed] << '\n';
  }
  for (std::size_t family = 0; family < kFamilies; ++family) {
    std::cout << root << ".family_" << kFamilyNames[family] << '='
              << value.family[family] << '\n';
  }
  std::cout << root << ".mechanism_effect_supported="
            << value.mechanism_supported << '\n';
  std::cout << root << ".mechanism_ruled_out_as_rescue_sized="
            << value.rescue_sized_ruled_out << '\n';
  std::cout << root << ".practically_equivalent="
            << vva1b_practically_equivalent(value) << '\n';
  std::cout << root << ".materially_negative="
            << vva1b_materially_negative(value.summary, value.per_seed)
            << '\n';
  std::cout << root << ".family_floor_pass=" << value.family_floor_pass
            << '\n';
  if (emit_candidate_predicates) {
    std::cout << root << ".no_new_safeguard_failure="
              << value.no_new_safeguard_failure << '\n';
    std::cout << root << ".candidate_eligible="
              << value.safe_direct_candidate << '\n';
  }
}

void vva1b_emit_safeguards(const std::string &root,
                           const Vva1bSafeguards &value) {
  std::cout << root << ".raw_noninferiority=" << value.raw_noninferiority
            << '\n';
  std::cout << root << ".order_point=" << value.order_point << '\n';
  std::cout << root << ".order_lower=" << value.order_lower << '\n';
  std::cout << root << ".order_retention=" << value.order_retention << '\n';
  std::cout << root << ".continuous_shuffle=" << value.continuous_shuffle
            << '\n';
  std::cout << root << ".order_shuffle=" << value.order_shuffle << '\n';
  for (std::size_t seed = 0; seed < 3; ++seed) {
    for (std::size_t channel = 0; channel < kChannels; ++channel) {
      const std::string prefix = root + ".seed_" +
                                 std::to_string(kAttributionSeeds[seed]) +
                                 ".channel_" + std::to_string(channel);
      std::cout << prefix << ".effective=" << value.effective[seed][channel]
                << '\n';
      std::cout << prefix << ".participation="
                << value.participation[seed][channel] << '\n';
      std::cout << prefix << ".top=" << value.top[seed][channel] << '\n';
      std::cout << prefix << ".active=" << value.active[seed][channel]
                << '\n';
    }
  }
  std::cout << root << ".geometry=" << value.geometry << '\n';
  std::cout << root << ".pass=" << value.pass << '\n';
}

void vva1b_emit_candidate_predicates(
    const std::string &root, const Vva1bCandidatePredicates &value) {
  std::cout << root << ".candidate_eligible=" << value.candidate_eligible
            << '\n';
  std::cout << root << ".all_safeguards_pass="
            << value.all_safeguards_pass << '\n';
  std::cout << root << ".objective_made_safe="
            << value.objective_made_safe << '\n';
  std::cout << root << ".representation_rescue="
            << value.representation_rescue << '\n';
}

[[nodiscard]] bool vva1b_options_valid(const Options &options) {
  return options.device == "cuda" &&
         (options.steps < 0 || options.steps == kVva1bSteps) &&
         (options.seeds < 0 ||
          options.seeds == static_cast<int64_t>(kAttributionSeeds.size())) &&
         options.weak_views;
}

void vva1b_require_cuda(const Options &options) {
  if (!vva1b_options_valid(options)) {
    throw std::runtime_error(
        "VVA-1B requires cuda:0, weak views, 512 steps, and three seeds");
  }
  rmc_configure_cuda();
}

int run_vva1b_self_test() {
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.vva1b.self_test.v3\n";
  std::cout << "experiment=vicreg-view-loss-boundary-triad-self-test\n";
  std::cout << "module_only=true\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "optimizer_updates=0\n";
  const auto self_test = vva1b_cpu_self_test();
  vva1b_emit_self_test(self_test);
  std::cout << "execution_status="
            << (self_test.pass ? "vva1b_self_test_complete"
                               : "vva1b_self_test_failed")
            << '\n';
  return self_test.pass ? 0 : 3;
}

int run_vva1b_preflight(const Options &options) {
  g_vva1b_preoptimizer_exception_receipt_pending = true;
  vva1b_require_cuda(options);
  const torch::Device device(torch::kCUDA, 0);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.vva1b.preflight.v3\n";
  std::cout << "experiment=vicreg-view-loss-boundary-triad-preflight\n";
  std::cout << "module_only=true\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "authoritative_optimizer_updates=0\n";
  std::cout << "shadow_updates_excluded_from_authoritative_count=true\n";
  std::cout << "control_source=joint_retrain_vva1b\n";
  std::cout << "historical_v0_reused=false\n";
  std::cout << "authoritative_new_arms=V0,V1,V2\n";
  std::cout << "planned_authoritative_adam_updates=" << kVva1bTotalUpdates
            << '\n';
  std::cout << "planned_authoritative_ema_updates=" << kVva1bTotalUpdates
            << '\n';
  std::cout << "bootstrap_replicates=" << kRssmBootstrapReplicates << '\n';
  std::cout << "bootstrap_estimand=paired_generated_group_fixed_seed_mean\n";
  std::cout << "vva1b.recovery.attempt_1.adam_updates_discarded=1536\n";
  std::cout << "vva1b.recovery.attempt_1.ema_updates_discarded=1536\n";
  std::cout << "vva1b.recovery.attempt_1.accepted_authoritative_adam_updates=0\n";
  std::cout << "vva1b.recovery.attempt_1.accepted_authoritative_ema_updates=0\n";
  auto data = rmc_make_data();
  const auto targets = rmc_make_targets(data, false);
  const auto bootstrap_rows = rssm_bootstrap_rows(256);
  const auto data_identity = vva1b_data_identity(data, bootstrap_rows);
  const auto self_test = vva1b_cpu_self_test();
  const auto custody = vva1b_validate_custody();
  const auto stage0 =
      vva1b_run_stage0(data, targets, device, custody, self_test);
  vva1b_emit_data_identity("vva1b.preflight.data", data_identity);
  vva1b_emit_self_test(self_test);
  vva1b_emit_custody(custody);
  vva1b_emit_stage0(stage0);
  const bool cache_inventory_empty = vva1b_seed_cache_inventory_empty();
  std::cout << "vva1b.recovery.seed_cache_inventory_empty="
            << cache_inventory_empty << '\n';
  std::cout << "vva1b.confirmation.sealed=true\n";
  std::cout << "vva1b.confirmation.opened=false\n";
  std::cout << "vva1b.confirmation.rows=0\n";
  std::cout << "vva1b.confirmation.training_updates=0\n";
  const bool pass = data_identity.pass && stage0.pass && cache_inventory_empty;
  std::cout << "vva1b.training.opened=false\n";
  std::cout << "execution_status="
            << (pass ? "vva1b_preflight_complete"
                     : "vva1b_preflight_failed")
            << '\n';
  g_vva1b_preoptimizer_exception_receipt_pending = false;
  return pass ? 0 : 3;
}

struct Vva1bConfirmationIdentity {
  std::array<uint64_t, 4> unnormalized{};
  std::array<uint64_t, 4> normalized{};
};

[[nodiscard]] Vva1bConfirmationIdentity
vva1b_open_confirmation(RmcData &data) {
  if (data.confirmation.data.defined()) {
    throw std::runtime_error("VVA-1B confirmation opened more than once");
  }
  Vva1bConfirmationIdentity result{};
  data.confirmation =
      generate_dataset(kVva1bConfirmationGroupBegin,
                       kVva1bConfirmationRows);
  result.unnormalized = vva1b_dataset_identity(data.confirmation);
  const auto raw_projection = make_raw_equal_width_projection();
  data.raw_confirmation =
      raw_equal_width_features(data.confirmation, raw_projection);
  normalize(data.confirmation, data.normalization);
  validate_dataset(data.confirmation);
  result.normalized = vva1b_dataset_identity(data.confirmation);
  data.reversed_confirmation = rssm_reversed_dataset(data.confirmation);
  return result;
}

void vva1b_emit_confirmation_identity(
    const Vva1bConfirmationIdentity &value) {
  constexpr std::array<const char *, 4> fields{"groups", "data", "mask",
                                               "target"};
  for (std::size_t index = 0; index < 4; ++index) {
    std::cout << "vva1b.confirmation.unnormalized." << fields[index] << '='
              << oca_hex_u64(value.unnormalized[index]) << '\n';
    std::cout << "vva1b.confirmation.normalized." << fields[index] << '='
              << oca_hex_u64(value.normalized[index]) << '\n';
  }
}

void vva1b_emit_rollback_receipts() {
  std::cout << "vva1b.production_defaults_changed=false\n";
  std::cout << "vva1b.rollback="
               "fspa4_minimal_spectral_repair_v1:structured_cdsb_sparse_v1\n";
  std::cout << "vva1b.operational_rollback=all_tokens\n";
}

void vva1b_emit_invalid_preoptimizer_epilogue() {
  std::cout << "vva1b.training.opened=false\n";
  std::cout << "vva1b.development.mechanism_route="
               "invalid_numeric_or_mechanics\n";
  std::cout << "vva1b.development.selected=none\n";
  std::cout << "vva1b.development.classification="
               "invalid_numeric_or_mechanics\n";
  std::cout << "vva1b.development.mechanics=false\n";
  std::cout << "vva1b.confirmation.sealed=true\n";
  std::cout << "vva1b.confirmation.opened=false\n";
  std::cout << "vva1b.confirmation.group_begin=-1\n";
  std::cout << "vva1b.confirmation.rows=0\n";
  std::cout << "vva1b.confirmation.training_updates=0\n";
  std::cout << "vva1b.confirmation.classification=not_opened\n";
  std::cout << "vva1b.confirmation.pass=false\n";
  std::cout << "vva1b.promotion=none\n";
  std::cout << "vva1b.production_defaults_changed=false\n";
  vva1b_emit_rollback_receipts();
  std::cout << "execution_status=vva1b_preoptimizer_gate_failed\n";
}

void vva1b_emit_pending_preoptimizer_exception_epilogue() {
  if (!g_vva1b_preoptimizer_exception_receipt_pending) {
    return;
  }
  vva1b_emit_invalid_preoptimizer_epilogue();
  g_vva1b_preoptimizer_exception_receipt_pending = false;
}

void vva1b_emit_post_training_exception_epilogue() {
  if (!g_vva1b_measurement_active || !g_vva1b_training_opened) {
    return;
  }
  const auto adam = vva1b_update_ledger(
      g_vva1b_current_adam_updates,
      g_vva1b_current_fresh_committed_adam_updates,
      g_vva1b_validated_replacement_adam_updates);
  const auto ema = vva1b_update_ledger(
      g_vva1b_current_ema_updates,
      g_vva1b_current_fresh_committed_ema_updates,
      g_vva1b_validated_replacement_ema_updates);
  std::cout << "vva1b.recovery.current_attempt.adam_updates_executed="
            << adam.current_executed << '\n';
  std::cout << "vva1b.recovery.current_attempt.ema_updates_executed="
            << ema.current_executed << '\n';
  std::cout << "vva1b.recovery.current_attempt.validated_seed_caches="
            << g_vva1b_recovery_validated_seed_caches << '\n';
  std::cout << "vva1b.recovery.current_attempt.fresh_committed_adam_updates="
            << adam.current_fresh_committed << '\n';
  std::cout << "vva1b.recovery.current_attempt.fresh_committed_ema_updates="
            << ema.current_fresh_committed << '\n';
  std::cout << "vva1b.recovery.current_attempt.validated_replacement_adam_updates="
            << adam.validated_replacement << '\n';
  std::cout << "vva1b.recovery.current_attempt.validated_replacement_ema_updates="
            << ema.validated_replacement << '\n';
  std::cout << "vva1b.recovery.current_attempt.uncommitted_adam_updates_discarded="
            << adam.current_uncommitted_discarded << '\n';
  std::cout << "vva1b.recovery.current_attempt.uncommitted_ema_updates_discarded="
            << ema.current_uncommitted_discarded << '\n';
  std::cout << "vva1b.recovery.current_attempt.adam_accounting_valid="
            << adam.valid << '\n';
  std::cout << "vva1b.recovery.current_attempt.ema_accounting_valid="
            << ema.valid << '\n';
  std::cout << "vva1b.recovery.known_lifetime_physical_adam_updates="
            << adam.lifetime_physical << '\n';
  std::cout << "vva1b.recovery.known_lifetime_physical_ema_updates="
            << ema.lifetime_physical << '\n';
  std::cout << "vva1b.recovery.current_attempt.accepted_authoritative_adam_updates=0\n";
  std::cout << "vva1b.recovery.current_attempt.accepted_authoritative_ema_updates=0\n";
  std::cout << "vva1b.development.selected=none\n";
  std::cout << "vva1b.development.classification=invalid_numeric_or_mechanics\n";
  std::cout << "vva1b.confirmation.sealed=true\n";
  std::cout << "vva1b.confirmation.opened=false\n";
  std::cout << "vva1b.confirmation.rows=0\n";
  std::cout << "vva1b.confirmation.training_updates=0\n";
  std::cout << "vva1b.confirmation.classification=not_opened\n";
  std::cout << "vva1b.confirmation.pass=false\n";
  std::cout << "vva1b.promotion=none\n";
  std::cout << "vva1b.production_defaults_changed=false\n";
  vva1b_emit_rollback_receipts();
  std::cout << "execution_status=vva1b_recovery_attempt_failed\n";
  g_vva1b_measurement_active = false;
}

int run_vva1b_measurement(const Options &options) {
  g_vva1b_measurement_active = true;
  g_vva1b_training_opened = false;
  g_vva1b_current_adam_updates = 0;
  g_vva1b_current_ema_updates = 0;
  g_vva1b_current_fresh_committed_adam_updates = 0;
  g_vva1b_current_fresh_committed_ema_updates = 0;
  g_vva1b_validated_replacement_adam_updates = 0;
  g_vva1b_validated_replacement_ema_updates = 0;
  g_vva1b_recovery_validated_seed_caches = 0;
  g_vva1b_preoptimizer_exception_receipt_pending = true;
  vva1b_require_cuda(options);
  const torch::Device device(torch::kCUDA, 0);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.vva1b.measurement.v3\n";
  std::cout << "experiment=vicreg-view-loss-boundary-triad\n";
  std::cout << "module_only=true\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "outer_augmentation_calls=0\n";
  std::cout << "control_source=joint_retrain_vva1b\n";
  std::cout << "historical_v0_reused=false\n";
  std::cout << "authoritative_new_arms=V0,V1,V2\n";
  std::cout << "planned_accepted_authoritative_adam_updates="
            << kVva1bTotalUpdates << '\n';
  std::cout << "planned_accepted_authoritative_ema_updates="
            << kVva1bTotalUpdates << '\n';
  std::cout << "vva1b.recovery.attempt_1.interleaved_seed_steps_executed=512\n";
  std::cout << "vva1b.recovery.attempt_1.adam_updates_executed=1536\n";
  std::cout << "vva1b.recovery.attempt_1.ema_updates_executed=1536\n";
  std::cout << "vva1b.recovery.attempt_1.adam_updates_discarded=1536\n";
  std::cout << "vva1b.recovery.attempt_1.ema_updates_discarded=1536\n";
  std::cout << "vva1b.recovery.attempt_1.accepted_authoritative_adam_updates=0\n";
  std::cout << "vva1b.recovery.attempt_1.accepted_authoritative_ema_updates=0\n";
  std::cout << "vva1b.recovery.attempt_1.completed_seed_caches=0\n";
  std::cout << "vva1b.recovery.attempt_1.classification=invalid_numeric_or_mechanics\n";
  std::cout << "optimizer_updates_per_seed_arm=" << kVva1bSteps << '\n';
  std::cout << "objective_mask=8\n";
  std::cout << "lambda_vicreg=0.05\n";
  std::cout << "lambda_global_vicreg=0.25\n";
  std::cout << "vicreg_component_weights=25,25,1\n";
  std::cout << "vicreg_variance_floor=1\n";
  std::cout << "vicreg_variance_epsilon=0.0001\n";
  std::cout << "optimizer=Adam\n";
  std::cout << "learning_rate=" << kVva1bLearningRate << '\n';
  std::cout << "gradient_clip_norm=" << kVva1bClipNorm << '\n';
  std::cout << "target_ema_tau=0.990\n";
  std::cout << "readout_policy=" << kVva1bReadoutPolicy << '\n';
  std::cout << "bootstrap_replicates=" << kRssmBootstrapReplicates << '\n';
  std::cout << "bootstrap_estimand=paired_generated_group_fixed_seed_mean\n";

  auto data = rmc_make_data();
  const auto targets = rmc_make_targets(data, false);
  const auto bootstrap_rows = rssm_bootstrap_rows(256);
  const auto data_identity = vva1b_data_identity(data, bootstrap_rows);
  const auto self_test = vva1b_cpu_self_test();
  const auto custody = vva1b_validate_custody();
  const auto stage0 =
      vva1b_run_stage0(data, targets, device, custody, self_test);
  vva1b_emit_data_identity("vva1b.data", data_identity);
  vva1b_emit_self_test(self_test);
  vva1b_emit_custody(custody);
  vva1b_emit_stage0(stage0);
  const bool cache_inventory_empty = vva1b_seed_cache_inventory_empty();
  std::cout << "vva1b.recovery.seed_cache_inventory_empty="
            << cache_inventory_empty << '\n';
  if (!data_identity.pass || !stage0.pass || !cache_inventory_empty) {
    vva1b_emit_invalid_preoptimizer_epilogue();
    return 3;
  }

  const auto raw = rssm_probe_curve(
      data.raw_train, data.raw_validation, data.raw_development,
      data.probe_train.target, data.probe_validation.target,
      data.development.target, /*dual=*/true);
  std::cout << "vva1b.development.raw_equal_width.aulc=" << raw.area << '\n';
  Vva1bSeedEvaluations anchor_evaluations{};
  std::vector<mtf::MtfJepaMaeVicreg> anchor_models;
  anchor_models.reserve(3);
  std::array<Vva1bSeedTraining, 3> training{};
  Vva1bEvaluations evaluations{};
  bool mechanics = true;
  bool anchor_replay_exact = true;
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  std::cout << "vva1b.training.opened=true\n" << std::flush;
  g_vva1b_training_opened = true;
  g_vva1b_preoptimizer_exception_receipt_pending = false;
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    set_paired_rng(seed, device);
    auto anchor = mtf::MtfJepaMaeVicreg(anchor_config);
    const bool anchor_metadata =
        oca_load_archive(oca_archive_path(seed), anchor, device, seed,
                         anchor_hash);
    anchor_evaluations[seed_index] =
        vva_evaluate(anchor, data, targets, device, false);
    anchor_replay_exact =
        anchor_replay_exact && anchor_metadata &&
        anchor_evaluations[seed_index].probe.area ==
            kVva1bFrozenAnchorAulc[seed_index];
    anchor_models.push_back(anchor);
    training[seed_index] = vva1b_train_or_resume_seed(
        data.ssl, data_identity, custody, device, seed);
    mechanics = mechanics && training[seed_index].pass;
    for (std::size_t arm = 0; arm < 3; ++arm) {
      evaluations[seed_index][arm] = vva_evaluate(
          training[seed_index].models[arm], data, targets, device, false);
      const std::string root =
          "vva1b.training.seed_" + std::to_string(seed) + ".arm." +
          kVva1bArmNames[arm];
      vva1b_emit_receipt(root, training[seed_index].receipts[arm]);
      vva1b_emit_evaluation(root + ".clean",
                            evaluations[seed_index][arm]);
      std::cout << root << ".complete=true\n";
    }
  }
  const auto recovery_adam_ledger = vva1b_update_ledger(
      g_vva1b_current_adam_updates,
      g_vva1b_current_fresh_committed_adam_updates,
      g_vva1b_validated_replacement_adam_updates);
  const auto recovery_ema_ledger = vva1b_update_ledger(
      g_vva1b_current_ema_updates,
      g_vva1b_current_fresh_committed_ema_updates,
      g_vva1b_validated_replacement_ema_updates);
  mechanics = mechanics && anchor_replay_exact && recovery_adam_ledger.valid &&
              recovery_ema_ledger.valid &&
              recovery_adam_ledger.validated_replacement ==
                  kVva1bTotalUpdates &&
              recovery_ema_ledger.validated_replacement ==
                  kVva1bTotalUpdates &&
              g_vva1b_recovery_validated_seed_caches == 3;
  std::cout << "vva1b.anchor_replay_exact=" << anchor_replay_exact << '\n';
  std::cout << "vva1b.anchor_mean_aulc="
            << (anchor_evaluations[0].probe.area +
                anchor_evaluations[1].probe.area +
                anchor_evaluations[2].probe.area) /
                   3.0
            << '\n';

  const auto bootstrap = vva1b_bootstrap_area_table(
      evaluations, data.development.target, bootstrap_rows);
  auto pairing = vva1b_contrast(evaluations, bootstrap, 0, 1);
  auto corruption = vva1b_contrast(evaluations, bootstrap, 1, 2);
  auto complete = vva1b_contrast(evaluations, bootstrap, 0, 2);
  std::array<RmcSummary, 3> anchor_summaries{};
  std::array<Vva1bContrast, 3> anchor_contrasts{};
  std::array<Vva1bSafeguards, 3> safeguards{};
  for (std::size_t arm = 0; arm < 3; ++arm) {
    const auto arm_evaluations = vva1b_arm_evaluations(evaluations, arm);
    anchor_summaries[arm] = oca_pair_summary(
        anchor_evaluations, arm_evaluations, raw, data.development.target,
        targets, bootstrap_rows, mechanics);
    anchor_contrasts[arm] = vva1b_summary_contrast(
        anchor_evaluations, arm_evaluations, anchor_summaries[arm]);
    safeguards[arm] =
        vva1b_safeguards(anchor_summaries[arm], arm_evaluations);
    const std::string root = "vva1b.development.arm." +
                             std::string(kVva1bArmNames[arm]);
    oca_emit_candidate_summary(root + ".minus_fspa4",
                               anchor_summaries[arm]);
    vva1b_emit_contrast(root + ".minus_fspa4.contrast",
                        anchor_contrasts[arm]);
    vva1b_emit_safeguards(root + ".safeguards", safeguards[arm]);
    std::cout << root << ".materially_harmful_vs_fspa4="
              << vva1b_materially_negative(anchor_contrasts[arm].summary,
                                            anchor_contrasts[arm].per_seed)
              << '\n';
  }
  pairing.no_new_safeguard_failure =
      !vva1b_new_safeguard_failure(safeguards[0], safeguards[1]);
  complete.no_new_safeguard_failure =
      !vva1b_new_safeguard_failure(safeguards[0], safeguards[2]);
  pairing.safe_direct_candidate = pairing.mechanism_supported &&
                                  pairing.family_floor_pass &&
                                  pairing.no_new_safeguard_failure;
  complete.safe_direct_candidate = complete.mechanism_supported &&
                                   complete.family_floor_pass &&
                                   complete.no_new_safeguard_failure;
  const std::array<Vva1bContrast, 3> direct_by_arm{
      Vva1bContrast{}, pairing, complete};
  std::array<Vva1bCandidatePredicates, 3> candidate_predicates{};
  for (std::size_t arm = 1; arm < 3; ++arm) {
    candidate_predicates[arm] = vva1b_candidate_predicates(
        direct_by_arm[arm], anchor_contrasts[arm], safeguards[arm]);
    const std::string root = "vva1b.development.arm." +
                             std::string(kVva1bArmNames[arm]);
    vva1b_emit_candidate_predicates(root, candidate_predicates[arm]);
  }
  vva1b_emit_contrast("vva1b.development.contrast.pairing_V1_minus_V0",
                       pairing, true);
  vva1b_emit_contrast("vva1b.development.contrast.corruption_V2_minus_V1",
                       corruption);
  vva1b_emit_contrast("vva1b.development.contrast.complete_V2_minus_V0",
                       complete, true);
  std::size_t selected = 3;
  for (std::size_t arm = 1; arm < 3; ++arm) {
    if (!direct_by_arm[arm].safe_direct_candidate) {
      continue;
    }
    if (selected == 3 ||
        direct_by_arm[arm].summary.point >
            direct_by_arm[selected].summary.point ||
        (direct_by_arm[arm].summary.point ==
             direct_by_arm[selected].summary.point &&
         arm == 2)) {
      selected = arm;
    }
  }
  const bool any_mechanism = pairing.mechanism_supported ||
                             corruption.mechanism_supported ||
                             complete.mechanism_supported;
  const auto classification =
      selected < 3
          ? vva1b_classify(mechanics, any_mechanism, true,
                            candidate_predicates[selected])
          : vva1b_classify(mechanics, any_mechanism, false,
                            Vva1bCandidatePredicates{});
  const auto route = vva1b_mechanism_route(
      pairing, corruption, complete, anchor_contrasts, mechanics);
  std::cout << "vva1b.development.mechanism_route=" << route << '\n';
  std::cout << "vva1b.development.selected="
            << (selected < 3 ? kVva1bArmNames[selected] : "none") << '\n';
  std::cout << "vva1b.development.classification="
            << vva1b_classification_name(classification) << '\n';
  std::cout << "vva1b.development.mechanics=" << mechanics << '\n';

  bool confirmation_opened = false;
  bool confirmation_pass = false;
  Vva1bClassification confirmation_classification =
      Vva1bClassification::no_candidate;
  if (selected < 3 &&
      (classification == Vva1bClassification::objective_made_safe ||
       classification == Vva1bClassification::representation_rescue)) {
    confirmation_opened = true;
    const auto confirmation_identity = vva1b_open_confirmation(data);
    vva1b_emit_confirmation_identity(confirmation_identity);
    const auto confirmation_targets = rmc_make_targets(data, true);
    const auto raw_confirmation = rssm_probe_curve(
        data.raw_train, data.raw_validation, data.raw_confirmation,
        data.probe_train.target, data.probe_validation.target,
        data.confirmation.target, /*dual=*/true);
    std::cout << "vva1b.confirmation.raw_equal_width.aulc="
              << raw_confirmation.area << '\n';
    Vva1bSeedEvaluations confirmation_anchor{};
    Vva1bSeedEvaluations confirmation_current{};
    Vva1bSeedEvaluations confirmation_candidate{};
    for (std::size_t seed = 0; seed < 3; ++seed) {
      confirmation_anchor[seed] = vva_evaluate(
          anchor_models[seed], data, confirmation_targets, device, true);
      confirmation_current[seed] = vva_evaluate(
          training[seed].models[0], data, confirmation_targets, device, true);
      confirmation_candidate[seed] = vva_evaluate(
          training[seed].models[selected], data, confirmation_targets, device,
          true);
    }
    const auto current_anchor_summary = oca_pair_summary(
        confirmation_anchor, confirmation_current, raw_confirmation,
        data.confirmation.target, confirmation_targets, bootstrap_rows,
        mechanics);
    const auto candidate_anchor_summary = oca_pair_summary(
        confirmation_anchor, confirmation_candidate, raw_confirmation,
        data.confirmation.target, confirmation_targets, bootstrap_rows,
        mechanics);
    const auto direct_summary = oca_pair_summary(
        confirmation_current, confirmation_candidate, raw_confirmation,
        data.confirmation.target, confirmation_targets, bootstrap_rows,
        mechanics);
    auto confirmation_direct = vva1b_summary_contrast(
        confirmation_current, confirmation_candidate, direct_summary);
    const auto confirmation_current_anchor_contrast = vva1b_summary_contrast(
        confirmation_anchor, confirmation_current, current_anchor_summary);
    const auto confirmation_anchor_contrast = vva1b_summary_contrast(
        confirmation_anchor, confirmation_candidate, candidate_anchor_summary);
    const auto confirmation_current_safeguards =
        vva1b_safeguards(current_anchor_summary, confirmation_current);
    const auto confirmation_candidate_safeguards =
        vva1b_safeguards(candidate_anchor_summary, confirmation_candidate);
    confirmation_direct.no_new_safeguard_failure =
        !vva1b_new_safeguard_failure(confirmation_current_safeguards,
                                     confirmation_candidate_safeguards);
    confirmation_direct.safe_direct_candidate =
        confirmation_direct.mechanism_supported &&
        confirmation_direct.family_floor_pass &&
        confirmation_direct.no_new_safeguard_failure;
    const auto confirmation_candidate_predicates = vva1b_candidate_predicates(
        confirmation_direct, confirmation_anchor_contrast,
        confirmation_candidate_safeguards);
    confirmation_classification = vva1b_classify(
        mechanics, confirmation_direct.mechanism_supported,
        confirmation_direct.safe_direct_candidate,
        confirmation_candidate_predicates);
    confirmation_pass = confirmation_classification == classification;
    oca_emit_candidate_summary("vva1b.confirmation.current_minus_fspa4",
                               current_anchor_summary);
    oca_emit_candidate_summary("vva1b.confirmation.candidate_minus_current",
                               direct_summary);
    oca_emit_candidate_summary("vva1b.confirmation.candidate_minus_fspa4",
                               candidate_anchor_summary);
    vva1b_emit_contrast(
        "vva1b.confirmation.current_minus_fspa4.contrast",
        confirmation_current_anchor_contrast);
    vva1b_emit_contrast(
        "vva1b.confirmation.candidate_minus_fspa4.contrast",
        confirmation_anchor_contrast);
    vva1b_emit_contrast("vva1b.confirmation.direct_contrast",
                         confirmation_direct, true);
    vva1b_emit_safeguards("vva1b.confirmation.current_safeguards",
                          confirmation_current_safeguards);
    vva1b_emit_safeguards("vva1b.confirmation.candidate_safeguards",
                          confirmation_candidate_safeguards);
    vva1b_emit_candidate_predicates("vva1b.confirmation.candidate",
                                     confirmation_candidate_predicates);
  }
  std::cout << "vva1b.confirmation.sealed=" << !confirmation_opened << '\n';
  std::cout << "vva1b.confirmation.opened=" << confirmation_opened << '\n';
  std::cout << "vva1b.confirmation.group_begin="
            << (confirmation_opened ? kVva1bConfirmationGroupBegin : -1)
            << '\n';
  std::cout << "vva1b.confirmation.rows="
            << (confirmation_opened ? kVva1bConfirmationRows : 0) << '\n';
  std::cout << "vva1b.confirmation.training_updates=0\n";
  std::cout << "vva1b.confirmation.classification="
            << (confirmation_opened
                    ? vva1b_classification_name(confirmation_classification)
                    : "not_opened")
            << '\n';
  std::cout << "vva1b.confirmation.pass=" << confirmation_pass << '\n';
  std::cout << "vva1b.promotion="
            << (confirmation_pass && selected < 3 ? kVva1bArmNames[selected]
                                                   : "none")
            << '\n';
  std::cout << "vva1b.production_defaults_changed=false\n";
  std::cout << "vva1b.recovery.current_attempt.adam_updates_executed="
            << recovery_adam_ledger.current_executed << '\n';
  std::cout << "vva1b.recovery.current_attempt.ema_updates_executed="
            << recovery_ema_ledger.current_executed << '\n';
  std::cout << "vva1b.recovery.current_attempt.validated_seed_caches="
            << g_vva1b_recovery_validated_seed_caches << '\n';
  std::cout << "vva1b.recovery.current_attempt.fresh_committed_adam_updates="
            << recovery_adam_ledger.current_fresh_committed << '\n';
  std::cout << "vva1b.recovery.current_attempt.fresh_committed_ema_updates="
            << recovery_ema_ledger.current_fresh_committed << '\n';
  std::cout << "vva1b.recovery.current_attempt.validated_replacement_adam_updates="
            << recovery_adam_ledger.validated_replacement << '\n';
  std::cout << "vva1b.recovery.current_attempt.validated_replacement_ema_updates="
            << recovery_ema_ledger.validated_replacement << '\n';
  std::cout << "vva1b.recovery.current_attempt.uncommitted_adam_updates_discarded="
            << recovery_adam_ledger.current_uncommitted_discarded << '\n';
  std::cout << "vva1b.recovery.current_attempt.uncommitted_ema_updates_discarded="
            << recovery_ema_ledger.current_uncommitted_discarded << '\n';
  std::cout << "vva1b.recovery.current_attempt.adam_accounting_valid="
            << recovery_adam_ledger.valid << '\n';
  std::cout << "vva1b.recovery.current_attempt.ema_accounting_valid="
            << recovery_ema_ledger.valid << '\n';
  std::cout << "vva1b.recovery.physical_attempted_adam_updates="
            << recovery_adam_ledger.lifetime_physical << '\n';
  std::cout << "vva1b.recovery.physical_attempted_ema_updates="
            << recovery_ema_ledger.lifetime_physical << '\n';
  std::cout << "vva1b.recovery.discarded_failed_attempt_adam_updates=1536\n";
  std::cout << "vva1b.recovery.discarded_failed_attempt_ema_updates=1536\n";
  std::cout << "vva1b.recovery.accepted_authoritative_adam_updates="
            << (mechanics ? kVva1bTotalUpdates : 0) << '\n';
  std::cout << "vva1b.recovery.accepted_authoritative_ema_updates="
            << (mechanics ? kVva1bTotalUpdates : 0) << '\n';
  vva1b_emit_rollback_receipts();
  std::cout << "execution_status="
            << (mechanics ? "vva1b_measurements_complete"
                          : "vva1b_measurements_invalid")
            << '\n';
  g_vva1b_measurement_active = false;
  return mechanics ? 0 : 3;
}

} // namespace

int main(int argc, char **argv) {
  try {
    if (argc > 0 && argv[0] != nullptr) {
      g_vva1b_executable_path =
          std::filesystem::absolute(std::filesystem::path(argv[0]));
    }
    const auto options = parse_options(argc, argv);
    if (options.experiment ==
        "vicreg-view-loss-boundary-triad-self-test") {
      return run_vva1b_self_test();
    }
    if (options.experiment ==
        "vicreg-view-loss-boundary-triad-preflight") {
      return run_vva1b_preflight(options);
    }
    if (options.experiment == "vicreg-view-loss-boundary-triad") {
      return run_vva1b_measurement(options);
    }
    throw std::runtime_error(
        "--experiment must be vicreg-view-loss-boundary-triad-self-test, "
        "vicreg-view-loss-boundary-triad-preflight, or "
        "vicreg-view-loss-boundary-triad");
  } catch (const c10::Error &error) {
    vva1b_emit_pending_preoptimizer_exception_epilogue();
    vva1b_emit_post_training_exception_epilogue();
    std::cerr << "vicreg_view_loss_boundary_triad_error=" << error.what()
              << '\n';
    return 2;
  } catch (const std::exception &error) {
    vva1b_emit_pending_preoptimizer_exception_epilogue();
    vva1b_emit_post_training_exception_epilogue();
    std::cerr << "vicreg_view_loss_boundary_triad_error=" << error.what()
              << '\n';
    return 2;
  }
}
