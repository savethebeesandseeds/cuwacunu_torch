#include "outer_augmentation_causal_attribution_gate.h"
#include "piaabo/digest/sha256.h"

#include <chrono>

// OAA-1 is a test-only continuation of OCA-1. The embedding guard suppresses
// only OCA's command-line entry point while keeping its certified-anchor
// custody, cache, data, and RMC machinery in this translation unit.
#define CUWACUNU_OCA_EMBEDDED
#include "quality_wikimyei_mtf_jepa_mae_vicreg_four_objective_causal_attribution.cpp"
#undef CUWACUNU_OCA_EMBEDDED

namespace {

constexpr std::string_view kOaaProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "OUTER_AUGMENTATION_CAUSAL_ATTRIBUTION_PROTOCOL.md";
constexpr std::string_view kOaaProtocolSha256 =
    "3370817d8e81686961ce87ab8cd99157616e4bc2cee3a9d262f674b1f2f3b4a2";
constexpr std::string_view kOaaOcaLogPath =
    ".build/tests/oca1/oca1_full_run.log";
constexpr std::string_view kOaaOcaLogSha256 =
    "3ade025b37525c45d376eabaca2c31771ec7d23fbd2f32f26277cec0d9be606d";
constexpr std::string_view kOaaSemanticCapturePath =
    ".build/tests/representation_outer_augmentation_semantic_recheck_v1.log";
constexpr std::string_view kOaaSemanticCaptureSha256 =
    "bded59aa609a703ff776cbaee4a5f02c38706015d7ba6a2ae2aded4c613cc728";
constexpr std::string_view kOaaHelperHeaderPath =
    "src/include/jkimyei/training/representation/"
    "mtf_jepa_mae_vicreg_graph_first_launcher.h";
constexpr std::string_view kOaaHelperHeaderSha256 =
    "5c1ed715c5926be0ceb2b4553006138145ba6a138641509d32c098d0428a4502";
constexpr std::string_view kOaaCacheSchema =
    "oaa1.interleaved_completed_seed_cache.v1";
constexpr std::string_view kOaaCacheImplementation =
    "shared_profile_object_exact_identity_masks_atomic_v1";
constexpr std::string_view kOaaReadoutPolicy = "structured_cdsb_sparse_v1";
constexpr std::string_view kOaaCertificateId =
    "representation_certified_fspa4_minimal_spectral_repair_v1";
constexpr int64_t kOaaSteps = 512;
constexpr double kOaaLearningRate = 1.0e-3;
constexpr double kOaaClipNorm = 5.0;
constexpr uint64_t kOaaAugmentationSeedDomain = 0x6f6161315f617567ULL;
constexpr std::array<uint8_t, 2> kOaaObjectiveMasks{1U, 8U};
constexpr std::array<const char *, 2> kOaaObjectiveNames{"jepa", "vicreg"};
constexpr std::array<std::string_view, 3> kOaaAnchorSha256{
    "5d96b2961daa2bbd08a07a157ddab5debd9d4928234d8341b3961678327e9434",
    "a85c00d5694d1e3f0063e8bce6fc3c2e3132a393e5bcee195909742754e76775",
    "b9c2f82f26a5516069f8460095d3a2b85c482b7cd0e20db120ce2e0bbd68e392"};
constexpr std::array<std::string_view, 3> kOaaIdentityCacheSha256{
    "5290559894fa6e3f5d2fd57f32a90e97bb0eec924ec22cc9354ae26d0e629c92",
    "bb5391840ede1ad4aaf8ff760d39b796a7a300918e1aab881fbb5255051fcc39",
    "aaa7dd4b7638f240d1e940db7ede3bc40eb8ad01000baa90dc043801a29256a6"};

namespace oaa_gate = cuwacunu::tests::oaa1_gate;
static_assert(kOaaProtocolSha256 ==
              std::string_view(oaa_gate::kProtocolSha256));

enum class OaaProfile : uint8_t {
  gaussian = 1,
  amplitude = 2,
  frequency_gain = 3,
  safe_stack = 4,
};

constexpr std::array<OaaProfile, 4> kOaaProfiles{
    OaaProfile::gaussian, OaaProfile::amplitude, OaaProfile::frequency_gain,
    OaaProfile::safe_stack};
constexpr std::array<const char *, 4> kOaaProfileNames{
    "gaussian_only", "amplitude_only", "frequency_gain_only",
    "candidate_safe_stack"};
constexpr std::array<uint8_t, 5> kOaaOcaChallengeMasks{4U, 1U, 2U, 8U, 15U};

struct OaaReceipt {
  int64_t steps{0};
  std::vector<double> losses{};
  std::vector<double> gradient_norms{};
  std::vector<double> clip_factors{};
  std::vector<double> served_update_norms{};
  std::array<double, 4> component_loss_sums{};
  std::vector<uint64_t> row_hashes{};
  std::vector<uint64_t> clean_data_hashes{};
  std::vector<uint64_t> clean_mask_hashes{};
  std::vector<uint64_t> served_data_hashes{};
  std::vector<uint64_t> served_mask_hashes{};
  std::vector<uint64_t> actual_forward_data_hashes{};
  std::vector<uint64_t> actual_forward_mask_hashes{};
  std::vector<uint64_t> target_mask_hashes{};
  std::vector<uint64_t> context_mask_hashes{};
  std::vector<uint64_t> view_a_mask_hashes{};
  std::vector<uint64_t> view_b_mask_hashes{};
  std::vector<uint64_t> view_a_data_hashes{};
  std::vector<uint64_t> view_b_data_hashes{};
  std::vector<uint64_t> forward_pre_cpu_hashes{};
  std::vector<uint64_t> forward_pre_cuda_hashes{};
  std::vector<uint64_t> forward_post_cpu_hashes{};
  std::vector<uint64_t> forward_post_cuda_hashes{};
  std::vector<int64_t> augmentation_seeds{};
  std::vector<int64_t> retention_counts{};
  std::vector<double> retention_values{};
  std::vector<int64_t> step_flags{};
  int64_t adam_steps{0};
  int64_t ema_steps{0};
  int64_t clipping_count{0};
  double all_trainable_delta{0.0};
  double served_delta{0.0};
  double predictor_delta{0.0};
  double mae_decoder_delta{0.0};
  double vicreg_head_delta{0.0};
  double target_ema_delta{0.0};
  bool finite{true};
  bool expected_partitions{false};
  bool mechanics{true};
  bool pass{false};
};

struct OaaSeedResult {
  std::vector<mtf::MtfJepaMaeVicreg> models{};
  std::array<OaaReceipt, 8> receipts{};
  std::array<bool, 8> initialization_exact{};
  bool anchor_metadata_exact{true};
  bool shared_profile_objects_exact{true};
  bool reference_state_unchanged{true};
  bool pass{false};
};

struct OaaCustody {
  bool protocol{false};
  bool oca_protocol{false};
  bool oca_log_hash{false};
  bool oca_fields{false};
  bool semantic_hash{false};
  bool semantic_fields{false};
  bool helper_hash{false};
  bool fspa_protocol{false};
  bool archives{false};
  bool archive_replay{false};
  bool neutral_cache_hashes{false};
  bool neutral_caches{false};
  bool pass{false};
};

[[nodiscard]] constexpr std::size_t oaa_arm_index(std::size_t objective,
                                                  std::size_t profile) {
  return objective * kOaaProfiles.size() + profile;
}

[[nodiscard]] const char *oaa_profile_name(OaaProfile profile) {
  const auto value = static_cast<std::size_t>(profile);
  if (value == 0 || value > kOaaProfileNames.size()) {
    throw std::runtime_error("OAA profile id is invalid");
  }
  return kOaaProfileNames[value - 1];
}

[[nodiscard]] std::filesystem::path oaa_cache_path(std::string_view phase,
                                                   int64_t seed) {
  return std::filesystem::path(".build") / "tests" / "oaa1" /
         (std::string(phase) + "_seed_" + std::to_string(seed) +
          "_interleaved_v1.complete.pt");
}

[[nodiscard]] std::filesystem::path
oaa_cache_marker_path(const std::filesystem::path &path) {
  auto result = path;
  result += ".sha256";
  return result;
}

[[nodiscard]] int64_t oaa_augmentation_seed(int64_t model_seed,
                                            OaaProfile profile, int64_t step) {
  const uint64_t mixed = splitmix64(
      kOaaAugmentationSeedDomain ^ static_cast<uint64_t>(model_seed) ^
      (static_cast<uint64_t>(static_cast<uint8_t>(profile)) << 56U) ^
      (static_cast<uint64_t>(step) << 24U));
  return static_cast<int64_t>(mixed & 0x7fffffffffffffffULL);
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t
oaa_preprocessing_config(const torch::Device &device, OaaProfile profile) {
  auto config = attribution_config(device, oca_arm(1U));
  config.augmentation_profile =
      std::string("oaa1_") + oaa_profile_name(profile);
  config.gaussian_jitter_std = 0.0;
  config.feature_dropout_prob = 0.0;
  config.history_dropout_prob = 0.0;
  config.time_crop_jitter_max = 0;
  config.time_dilation_min = 1.0;
  config.time_dilation_max = 1.0;
  config.time_warp_max = 0.0;
  config.amplitude_scale_min = 1.0;
  config.amplitude_scale_max = 1.0;
  config.amplitude_shift_std = 0.0;
  config.frequency_mask_ratio = 0.0;
  config.frequency_jitter_std = 0.0;
  config.phase_jitter_max = 0.0;
  config.channel_dropout_prob = 0.0;
  config.cross_channel_dropout_prob = 0.0;
  config.node_dropout_prob = 0.0;
  config.edge_dropout_prob = 0.0;
  config.magnitude_normalization_noise_std = 0.0;
  if (profile == OaaProfile::gaussian || profile == OaaProfile::safe_stack) {
    config.gaussian_jitter_std = 0.001;
  }
  if (profile == OaaProfile::amplitude || profile == OaaProfile::safe_stack) {
    config.amplitude_scale_min = 0.98;
    config.amplitude_scale_max = 1.02;
  }
  if (profile == OaaProfile::frequency_gain ||
      profile == OaaProfile::safe_stack) {
    config.frequency_jitter_std = 0.01;
  }
  return config;
}

[[nodiscard]] std::string oaa_model_manifest(const torch::Device &device,
                                             std::size_t objective) {
  return canonical_config_manifest(
      attribution_config(device, oca_arm(kOaaObjectiveMasks.at(objective))));
}

[[nodiscard]] std::string
oaa_preprocessing_manifest(const torch::Device &device, std::size_t profile) {
  return canonical_preprocessing_manifest(
      oaa_preprocessing_config(device, kOaaProfiles.at(profile)));
}

[[nodiscard]] bool oaa_line_count(const std::string &text,
                                  std::string_view line,
                                  std::size_t expected = 1) {
  std::size_t count = 0;
  std::istringstream input(text);
  std::string observed;
  while (std::getline(input, observed)) {
    if (!observed.empty() && observed.back() == '\r') {
      observed.pop_back();
    }
    count += observed == line ? 1U : 0U;
  }
  return count == expected;
}

[[nodiscard]] bool
oaa_semantic_result_inventory_exact(const std::string &text) {
  constexpr std::array<std::string_view, 10> expected{
      "attribution.neutral_reference.result=QUALIFIED",
      "attribution.gaussian_jitter_only.result=QUALIFIED",
      "attribution.amplitude_scale_only.result=QUALIFIED",
      "attribution.frequency_mask_only.result=NOT_QUALIFIED",
      "attribution.frequency_gain_jitter_only.result=QUALIFIED",
      "attribution.candidate_safe_stack.result=QUALIFIED",
      "attribution.dilation_only.result=NOT_QUALIFIED",
      "attribution.warp_only.result=NOT_QUALIFIED",
      "attribution.temporal_dilation_plus_warp.result=NOT_QUALIFIED",
      "attribution.full_active_stack.result=NOT_QUALIFIED"};
  std::array<std::size_t, expected.size()> counts{};
  std::size_t attribution_result_lines = 0;
  std::size_t global_result_lines = 0;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.rfind("attribution.", 0) == 0 &&
        line.find(".result=") != std::string::npos) {
      ++attribution_result_lines;
      for (std::size_t index = 0; index < expected.size(); ++index) {
        counts[index] += line == expected[index] ? 1U : 0U;
      }
    } else if (line.rfind("result=", 0) == 0) {
      ++global_result_lines;
      if (line != "result=NOT_QUALIFIED") {
        return false;
      }
    }
  }
  return attribution_result_lines == expected.size() &&
         global_result_lines == 1 &&
         std::all_of(counts.begin(), counts.end(),
                     [](std::size_t count) { return count == 1; });
}

[[nodiscard]] OaaCustody oaa_validate_custody(const RmcData &data,
                                              const torch::Device &device,
                                              bool replay_evaluations) {
  OaaCustody result{};
  const auto protocol = rmc_read_file(std::filesystem::path(kOaaProtocolPath));
  result.protocol = digest::sha256_hex(protocol) == kOaaProtocolSha256;
  result.oca_protocol = digest::sha256_hex(rmc_read_file(std::filesystem::path(
                            kOcaProtocolPath))) == kOcaProtocolSha256;
  result.fspa_protocol =
      digest::sha256_hex(
          rmc_read_file(std::filesystem::path(kMpsrProtocolPath))) ==
      std::string_view(
          "4cf4f81ffac1665f85bd233203ccf2f039617ec8d52b41a40258002b42999b00");
  const auto oca_log = rmc_read_file(std::filesystem::path(kOaaOcaLogPath));
  result.oca_log_hash = digest::sha256_hex(oca_log) == kOaaOcaLogSha256;
  result.oca_fields =
      oaa_line_count(oca_log, "outer_augmentation_calls=0") &&
      oaa_line_count(oca_log,
                     "oca1.verdict.jepa=harmful_at_certified_boundary") &&
      oaa_line_count(oca_log,
                     "oca1.verdict.mae=harmful_at_certified_boundary") &&
      oaa_line_count(oca_log,
                     "oca1.verdict.tf_align=harmful_at_certified_boundary") &&
      oaa_line_count(oca_log,
                     "oca1.verdict.vicreg=harmful_at_certified_boundary") &&
      oaa_line_count(oca_log, "execution_status=oca1_measurements_complete");
  const auto semantic =
      rmc_read_file(std::filesystem::path(kOaaSemanticCapturePath));
  result.semantic_hash =
      digest::sha256_hex(semantic) == kOaaSemanticCaptureSha256;
  result.semantic_fields =
      oaa_semantic_result_inventory_exact(semantic) &&
      oaa_line_count(semantic,
                     "schema_id=mtf_augmentation_semantic_qualification.v1") &&
      oaa_line_count(semantic,
                     "attribution.neutral_reference.result=QUALIFIED") &&
      oaa_line_count(semantic,
                     "attribution.gaussian_jitter_only.result=QUALIFIED") &&
      oaa_line_count(semantic,
                     "attribution.amplitude_scale_only.result=QUALIFIED") &&
      oaa_line_count(semantic,
                     "attribution.frequency_mask_only.result=NOT_QUALIFIED") &&
      oaa_line_count(
          semantic,
          "attribution.frequency_gain_jitter_only.result=QUALIFIED") &&
      oaa_line_count(semantic,
                     "attribution.candidate_safe_stack.result=QUALIFIED") &&
      oaa_line_count(semantic,
                     "attribution.dilation_only.result=NOT_QUALIFIED") &&
      oaa_line_count(semantic, "attribution.warp_only.result=NOT_QUALIFIED") &&
      oaa_line_count(
          semantic,
          "attribution.temporal_dilation_plus_warp.result=NOT_QUALIFIED") &&
      oaa_line_count(semantic,
                     "attribution.full_active_stack.result=NOT_QUALIFIED") &&
      oaa_line_count(semantic, "result=NOT_QUALIFIED");
  result.helper_hash = digest::sha256_hex(rmc_read_file(std::filesystem::path(
                           kOaaHelperHeaderPath))) == kOaaHelperHeaderSha256;

  const auto targets = rmc_make_targets(data, false);
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_config_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  result.archives = true;
  result.archive_replay = true;
  result.neutral_cache_hashes = true;
  result.neutral_caches = true;
  for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
       ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    const auto archive_path = oca_archive_path(seed);
    result.archives =
        result.archives && digest::sha256_hex(rmc_read_file(archive_path)) ==
                               kOaaAnchorSha256[seed_index];
    set_paired_rng(seed, device);
    auto first = mtf::MtfJepaMaeVicreg(anchor_config);
    set_paired_rng(seed, device);
    auto second = mtf::MtfJepaMaeVicreg(anchor_config);
    const bool first_metadata =
        oca_load_archive(archive_path, first, device, seed, anchor_config_hash);
    const bool second_metadata = oca_load_archive(archive_path, second, device,
                                                  seed, anchor_config_hash);
    bool exact = first_metadata && second_metadata &&
                 oca_state_exact(second, oca_snapshot_state(first)) &&
                 oca_capture_exact(
                     oca_capture_structured(first, data.development, device),
                     oca_capture_structured(second, data.development, device));
    if (replay_evaluations) {
      const auto first_evaluation = rmc_evaluate(
          first, data.probe_train, data.probe_validation, data.development,
          data.reversed_train, data.reversed_validation,
          data.reversed_development, targets, device);
      const auto second_evaluation = rmc_evaluate(
          second, data.probe_train, data.probe_validation, data.development,
          data.reversed_train, data.reversed_validation,
          data.reversed_development, targets, device);
      exact =
          exact && rmc_evaluations_exact(first_evaluation, second_evaluation);
    }
    result.archive_replay = result.archive_replay && exact;
    OcaInterleavedTrainingResult neutral{};
    const std::vector<uint8_t> masks(kOaaOcaChallengeMasks.begin(),
                                     kOaaOcaChallengeMasks.end());
    const auto neutral_cache_path =
        oca_seed_cache_path("anchor_challenge", seed);
    result.neutral_cache_hashes =
        result.neutral_cache_hashes &&
        digest::sha256_hex(rmc_read_file(neutral_cache_path)) ==
            kOaaIdentityCacheSha256[seed_index];
    result.neutral_caches =
        result.neutral_caches &&
        oca_load_seed_cache("anchor_challenge", data.ssl, device, seed, masks,
                            kOcaAnchorChallengeSteps,
                            /*load_certified_anchor=*/true, neutral) &&
        neutral.pass;
  }
  result.pass = result.protocol && result.oca_protocol && result.oca_log_hash &&
                result.oca_fields && result.semantic_hash &&
                result.semantic_fields && result.helper_hash &&
                result.fspa_protocol && result.archives &&
                result.archive_replay && result.neutral_cache_hashes &&
                result.neutral_caches;
  return result;
}

} // namespace

namespace {

[[nodiscard]] torch::Tensor
oaa_double_tensor(const std::vector<double> &value) {
  if (value.empty()) {
    throw std::runtime_error("OAA double-vector archive cannot be empty");
  }
  return torch::tensor(value, torch::TensorOptions().dtype(torch::kFloat64));
}

[[nodiscard]] torch::Tensor oaa_i64_tensor(const std::vector<int64_t> &value) {
  if (value.empty()) {
    throw std::runtime_error("OAA int64-vector archive cannot be empty");
  }
  return torch::tensor(value, torch::TensorOptions().dtype(torch::kInt64));
}

[[nodiscard]] std::vector<double>
oaa_double_vector(const torch::Tensor &input) {
  const auto value =
      input.detach().to(torch::kCPU, torch::kFloat64).contiguous();
  if (value.dim() != 1 || value.numel() <= 0) {
    throw std::runtime_error("OAA double-vector archive shape failed");
  }
  const auto *data = value.data_ptr<double>();
  return {data, data + value.numel()};
}

[[nodiscard]] std::vector<int64_t> oaa_i64_vector(const torch::Tensor &input) {
  const auto value = input.detach().to(torch::kCPU, torch::kInt64).contiguous();
  if (value.dim() != 1 || value.numel() <= 0) {
    throw std::runtime_error("OAA int64-vector archive shape failed");
  }
  const auto *data = value.data_ptr<int64_t>();
  return {data, data + value.numel()};
}

[[nodiscard]] std::vector<double>
oaa_array_vector(const std::array<double, 4> &value) {
  return {value.begin(), value.end()};
}

[[nodiscard]] bool oaa_receipt_shape_valid(const OaaReceipt &receipt,
                                           int64_t steps) {
  const auto size = static_cast<std::size_t>(steps);
  return steps > 0 && receipt.steps == steps && receipt.losses.size() == size &&
         receipt.gradient_norms.size() == size &&
         receipt.clip_factors.size() == size &&
         receipt.served_update_norms.size() == size &&
         receipt.row_hashes.size() == size &&
         receipt.clean_data_hashes.size() == size &&
         receipt.clean_mask_hashes.size() == size &&
         receipt.served_data_hashes.size() == size &&
         receipt.served_mask_hashes.size() == size &&
         receipt.actual_forward_data_hashes.size() == size &&
         receipt.actual_forward_mask_hashes.size() == size &&
         receipt.target_mask_hashes.size() == size &&
         receipt.context_mask_hashes.size() == size &&
         receipt.view_a_mask_hashes.size() == size &&
         receipt.view_b_mask_hashes.size() == size &&
         receipt.view_a_data_hashes.size() == size &&
         receipt.view_b_data_hashes.size() == size &&
         receipt.forward_pre_cpu_hashes.size() == size &&
         receipt.forward_pre_cuda_hashes.size() == size &&
         receipt.forward_post_cpu_hashes.size() == size &&
         receipt.forward_post_cuda_hashes.size() == size &&
         receipt.augmentation_seeds.size() == size &&
         receipt.retention_counts.size() == size * 5U &&
         receipt.retention_values.size() == size * 5U &&
         receipt.step_flags.size() == size * 12U &&
         receipt.adam_steps == steps && receipt.ema_steps == steps;
}

void oaa_write_receipt(torch::serialize::OutputArchive &root,
                       const std::string &prefix, const OaaReceipt &receipt) {
  if (!oaa_receipt_shape_valid(receipt, receipt.steps) || !receipt.pass) {
    throw std::runtime_error("OAA receipt save contract failed");
  }
  root.write(prefix + "losses", oaa_double_tensor(receipt.losses));
  root.write(prefix + "gradient_norms",
             oaa_double_tensor(receipt.gradient_norms));
  root.write(prefix + "clip_factors", oaa_double_tensor(receipt.clip_factors));
  root.write(prefix + "served_update_norms",
             oaa_double_tensor(receipt.served_update_norms));
  root.write(prefix + "component_loss_sums",
             oaa_double_tensor(oaa_array_vector(receipt.component_loss_sums)));
  for (const auto &entry :
       std::array<std::pair<const char *, const std::vector<uint64_t> *>, 17>{
           {{"row_hashes", &receipt.row_hashes},
            {"clean_data_hashes", &receipt.clean_data_hashes},
            {"clean_mask_hashes", &receipt.clean_mask_hashes},
            {"served_data_hashes", &receipt.served_data_hashes},
            {"served_mask_hashes", &receipt.served_mask_hashes},
            {"actual_forward_data_hashes", &receipt.actual_forward_data_hashes},
            {"actual_forward_mask_hashes", &receipt.actual_forward_mask_hashes},
            {"target_mask_hashes", &receipt.target_mask_hashes},
            {"context_mask_hashes", &receipt.context_mask_hashes},
            {"view_a_mask_hashes", &receipt.view_a_mask_hashes},
            {"view_b_mask_hashes", &receipt.view_b_mask_hashes},
            {"view_a_data_hashes", &receipt.view_a_data_hashes},
            {"view_b_data_hashes", &receipt.view_b_data_hashes},
            {"forward_pre_cpu_hashes", &receipt.forward_pre_cpu_hashes},
            {"forward_pre_cuda_hashes", &receipt.forward_pre_cuda_hashes},
            {"forward_post_cpu_hashes", &receipt.forward_post_cpu_hashes},
            {"forward_post_cuda_hashes", &receipt.forward_post_cuda_hashes}}}) {
    root.write(prefix + entry.first + "_u64_le",
               oca_u64_le_bytes_tensor(*entry.second));
  }
  root.write(prefix + "augmentation_seeds",
             oaa_i64_tensor(receipt.augmentation_seeds));
  root.write(prefix + "retention_counts",
             oaa_i64_tensor(receipt.retention_counts));
  root.write(prefix + "retention_values",
             oaa_double_tensor(receipt.retention_values));
  root.write(prefix + "step_flags", oaa_i64_tensor(receipt.step_flags));
  root.write(
      prefix + "scalars",
      torch::tensor({receipt.all_trainable_delta, receipt.served_delta,
                     receipt.predictor_delta, receipt.mae_decoder_delta,
                     receipt.vicreg_head_delta, receipt.target_ema_delta},
                    torch::TensorOptions().dtype(torch::kFloat64)));
  root.write(prefix + "counts",
             torch::tensor({receipt.steps, receipt.adam_steps,
                            receipt.ema_steps, receipt.clipping_count},
                           torch::kInt64));
  root.write(prefix + "flags",
             oaa_i64_tensor(std::vector<int64_t>{
                 receipt.finite ? 1 : 0, receipt.expected_partitions ? 1 : 0,
                 receipt.mechanics ? 1 : 0, receipt.pass ? 1 : 0}));
}

[[nodiscard]] OaaReceipt oaa_read_receipt(torch::serialize::InputArchive &root,
                                          const std::string &prefix,
                                          int64_t expected_steps) {
  OaaReceipt result{};
  const auto read_tensor = [&](const std::string &key) {
    torch::Tensor observed{};
    root.read(key, observed);
    return observed;
  };
  auto value = read_tensor(prefix + "losses");
  result.losses = oaa_double_vector(value);
  value = read_tensor(prefix + "gradient_norms");
  result.gradient_norms = oaa_double_vector(value);
  value = read_tensor(prefix + "clip_factors");
  result.clip_factors = oaa_double_vector(value);
  value = read_tensor(prefix + "served_update_norms");
  result.served_update_norms = oaa_double_vector(value);
  value = read_tensor(prefix + "component_loss_sums");
  const auto components = oaa_double_vector(value);
  if (components.size() != result.component_loss_sums.size()) {
    throw std::runtime_error("OAA component loss archive failed");
  }
  std::copy(components.begin(), components.end(),
            result.component_loss_sums.begin());
  const auto read_hash = [&](const char *name, std::vector<uint64_t> &out) {
    out = oca_u64_le_bytes_vector(read_tensor(prefix + name + "_u64_le"));
  };
  read_hash("row_hashes", result.row_hashes);
  read_hash("clean_data_hashes", result.clean_data_hashes);
  read_hash("clean_mask_hashes", result.clean_mask_hashes);
  read_hash("served_data_hashes", result.served_data_hashes);
  read_hash("served_mask_hashes", result.served_mask_hashes);
  read_hash("actual_forward_data_hashes", result.actual_forward_data_hashes);
  read_hash("actual_forward_mask_hashes", result.actual_forward_mask_hashes);
  read_hash("target_mask_hashes", result.target_mask_hashes);
  read_hash("context_mask_hashes", result.context_mask_hashes);
  read_hash("view_a_mask_hashes", result.view_a_mask_hashes);
  read_hash("view_b_mask_hashes", result.view_b_mask_hashes);
  read_hash("view_a_data_hashes", result.view_a_data_hashes);
  read_hash("view_b_data_hashes", result.view_b_data_hashes);
  read_hash("forward_pre_cpu_hashes", result.forward_pre_cpu_hashes);
  read_hash("forward_pre_cuda_hashes", result.forward_pre_cuda_hashes);
  read_hash("forward_post_cpu_hashes", result.forward_post_cpu_hashes);
  read_hash("forward_post_cuda_hashes", result.forward_post_cuda_hashes);
  value = read_tensor(prefix + "augmentation_seeds");
  result.augmentation_seeds = oaa_i64_vector(value);
  value = read_tensor(prefix + "retention_counts");
  result.retention_counts = oaa_i64_vector(value);
  value = read_tensor(prefix + "retention_values");
  result.retention_values = oaa_double_vector(value);
  value = read_tensor(prefix + "step_flags");
  result.step_flags = oaa_i64_vector(value);
  value = read_tensor(prefix + "scalars");
  const auto scalars = oaa_double_vector(value);
  if (scalars.size() != 6) {
    throw std::runtime_error("OAA receipt scalar archive failed");
  }
  result.all_trainable_delta = scalars[0];
  result.served_delta = scalars[1];
  result.predictor_delta = scalars[2];
  result.mae_decoder_delta = scalars[3];
  result.vicreg_head_delta = scalars[4];
  result.target_ema_delta = scalars[5];
  value = read_tensor(prefix + "counts");
  const auto counts = oaa_i64_vector(value);
  if (counts.size() != 4) {
    throw std::runtime_error("OAA receipt count archive failed");
  }
  result.steps = counts[0];
  result.adam_steps = counts[1];
  result.ema_steps = counts[2];
  result.clipping_count = counts[3];
  value = read_tensor(prefix + "flags");
  const auto flags = oaa_i64_vector(value);
  if (flags.size() != 4) {
    throw std::runtime_error("OAA receipt flag archive failed");
  }
  result.finite = flags[0] == 1;
  result.expected_partitions = flags[1] == 1;
  result.mechanics = flags[2] == 1;
  result.pass = flags[3] == 1;
  if (!oaa_receipt_shape_valid(result, expected_steps) || !result.finite ||
      !result.expected_partitions || !result.mechanics || !result.pass) {
    throw std::runtime_error("OAA receipt validation failed");
  }
  return result;
}

void oaa_save_seed_cache(std::string_view phase, const Dataset &ssl,
                         const torch::Device &device, int64_t seed,
                         int64_t steps, const OaaSeedResult &result) {
  if (result.models.size() != 8 || steps <= 0 ||
      !result.anchor_metadata_exact || !result.shared_profile_objects_exact ||
      !result.reference_state_unchanged || !result.pass) {
    throw std::runtime_error("OAA seed cache save contract failed");
  }
  const auto path = oaa_cache_path(phase, seed);
  const auto marker = oaa_cache_marker_path(path);
  const auto nonce =
      std::chrono::steady_clock::now().time_since_epoch().count();
  auto temporary = path;
  temporary += ".tmp." + std::to_string(nonce);
  auto temporary_marker = marker;
  temporary_marker += ".tmp." + std::to_string(nonce);
  std::filesystem::create_directories(path.parent_path());
  const auto ssl_hashes = oca_seed_cache_ssl_hashes(ssl);
  const auto seed_position = static_cast<std::size_t>(std::distance(
      kAttributionSeeds.begin(),
      std::find(kAttributionSeeds.begin(), kAttributionSeeds.end(), seed)));
  if (seed_position >= kAttributionSeeds.size()) {
    throw std::runtime_error("OAA seed cache seed failed");
  }

  torch::serialize::OutputArchive root;
  root.write("meta/schema", oca_string_tensor(kOaaCacheSchema));
  root.write("meta/complete", torch::tensor({1}, torch::kInt64));
  root.write("meta/phase", oca_string_tensor(phase));
  root.write("meta/implementation", oca_string_tensor(kOaaCacheImplementation));
  root.write("meta/oaa1_protocol_sha256",
             oca_string_tensor(kOaaProtocolSha256));
  root.write("meta/oca1_protocol_sha256",
             oca_string_tensor(kOcaProtocolSha256));
  root.write("meta/fspa4_protocol_sha256",
             oca_string_tensor(kMpsrProtocolSha256));
  root.write("meta/oca1_log_sha256", oca_string_tensor(kOaaOcaLogSha256));
  root.write("meta/helper_sha256", oca_string_tensor(kOaaHelperHeaderSha256));
  root.write("meta/semantic_sha256",
             oca_string_tensor(kOaaSemanticCaptureSha256));
  root.write("meta/certificate_id", oca_string_tensor(kOaaCertificateId));
  root.write("meta/readout_policy", oca_string_tensor(kOaaReadoutPolicy));
  root.write("meta/anchor_sha256",
             oca_string_tensor(kOaaAnchorSha256[seed_position]));
  root.write("meta/oca1_identity_cache_sha256",
             oca_string_tensor(kOaaIdentityCacheSha256[seed_position]));
  root.write("meta/ssl_data_hash", oca_string_tensor(ssl_hashes[0]));
  root.write("meta/ssl_mask_hash", oca_string_tensor(ssl_hashes[1]));
  root.write("meta/ssl_target_hash", oca_string_tensor(ssl_hashes[2]));
  root.write("meta/seed", torch::tensor({seed}, torch::kInt64));
  root.write("meta/steps", torch::tensor({steps}, torch::kInt64));
  root.write("meta/arm_count", torch::tensor({8}, torch::kInt64));
  root.write("meta/optimizer_learning_rate",
             torch::tensor({kOaaLearningRate}, torch::kFloat64));
  root.write("meta/gradient_clip_norm",
             torch::tensor({kOaaClipNorm}, torch::kFloat64));
  root.write("meta/augmentation_seed_domain_u64_le",
             oca_u64_le_bytes_tensor({kOaaAugmentationSeedDomain}));
  root.write("meta/objective_masks", torch::tensor({1, 8}, torch::kInt64));
  root.write("meta/profile_ids", torch::tensor({1, 2, 3, 4}, torch::kInt64));
  root.write(
      "meta/result_flags",
      oaa_i64_tensor(std::vector<int64_t>{
          result.anchor_metadata_exact ? 1 : 0,
          result.shared_profile_objects_exact ? 1 : 0,
          result.reference_state_unchanged ? 1 : 0, result.pass ? 1 : 0}));
  for (std::size_t objective = 0; objective < kOaaObjectiveMasks.size();
       ++objective) {
    for (std::size_t profile = 0; profile < kOaaProfiles.size(); ++profile) {
      const auto index = oaa_arm_index(objective, profile);
      if (!result.initialization_exact[index] || !result.receipts[index].pass) {
        throw std::runtime_error("OAA cache arm is incomplete");
      }
      const std::string prefix = "arm_" + std::to_string(index) + "/";
      root.write(
          prefix + "objective_mask",
          torch::tensor({static_cast<int64_t>(kOaaObjectiveMasks[objective])},
                        torch::kInt64));
      root.write(prefix + "profile_id",
                 torch::tensor({static_cast<int64_t>(static_cast<uint8_t>(
                                   kOaaProfiles[profile]))},
                               torch::kInt64));
      root.write(prefix + "model_config_manifest",
                 oca_string_tensor(oaa_model_manifest(device, objective)));
      root.write(
          prefix + "preprocessing_manifest",
          oca_string_tensor(oaa_preprocessing_manifest(device, profile)));
      torch::serialize::OutputArchive model_archive;
      result.models[index]->save(model_archive);
      root.write(prefix + "model", model_archive);
      oaa_write_receipt(root, prefix + "receipt/", result.receipts[index]);
    }
  }
  root.save_to(temporary.string());
  const auto checksum = digest::sha256_hex(rmc_read_file(temporary));
  {
    std::ofstream out(temporary_marker, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
      throw std::runtime_error("cannot create OAA seed cache marker");
    }
    out << checksum << '\n';
    out.close();
    if (!out) {
      throw std::runtime_error("OAA seed cache marker write failed");
    }
  }
  std::filesystem::remove(marker);
  std::filesystem::remove(path);
  std::filesystem::rename(temporary, path);
  std::filesystem::rename(temporary_marker, marker);
}

[[nodiscard]] bool oaa_load_seed_cache(std::string_view phase,
                                       const Dataset &ssl,
                                       const torch::Device &device,
                                       int64_t seed, int64_t steps,
                                       OaaSeedResult &result) {
  const auto path = oaa_cache_path(phase, seed);
  const auto marker = oaa_cache_marker_path(path);
  const bool have_path = std::filesystem::exists(path);
  const bool have_marker = std::filesystem::exists(marker);
  if (!have_path && !have_marker) {
    return false;
  }
  if (have_path != have_marker) {
    throw std::runtime_error("OAA seed cache has an incomplete commit");
  }
  std::ifstream marker_stream(marker, std::ios::binary);
  std::string expected_checksum;
  std::getline(marker_stream, expected_checksum);
  if (!expected_checksum.empty() && expected_checksum.back() == '\r') {
    expected_checksum.pop_back();
  }
  if (expected_checksum.size() != 64 ||
      digest::sha256_hex(rmc_read_file(path)) != expected_checksum) {
    throw std::runtime_error("OAA seed cache checksum failed");
  }
  const auto seed_position = static_cast<std::size_t>(std::distance(
      kAttributionSeeds.begin(),
      std::find(kAttributionSeeds.begin(), kAttributionSeeds.end(), seed)));
  if (seed_position >= kAttributionSeeds.size()) {
    throw std::runtime_error("OAA seed cache seed failed");
  }
  const auto ssl_hashes = oca_seed_cache_ssl_hashes(ssl);
  torch::serialize::InputArchive root;
  root.load_from(path.string(), device);
  const auto read_tensor = [&](const std::string &key) {
    torch::Tensor observed{};
    root.read(key, observed);
    return observed;
  };
  const auto read_string = [&](const char *key) {
    return oca_tensor_string(read_tensor(key));
  };
  const bool metadata =
      read_string("meta/schema") == kOaaCacheSchema &&
      read_string("meta/phase") == phase &&
      read_string("meta/implementation") == kOaaCacheImplementation &&
      read_string("meta/oaa1_protocol_sha256") == kOaaProtocolSha256 &&
      read_string("meta/oca1_protocol_sha256") == kOcaProtocolSha256 &&
      read_string("meta/fspa4_protocol_sha256") == kMpsrProtocolSha256 &&
      read_string("meta/oca1_log_sha256") == kOaaOcaLogSha256 &&
      read_string("meta/helper_sha256") == kOaaHelperHeaderSha256 &&
      read_string("meta/semantic_sha256") == kOaaSemanticCaptureSha256 &&
      read_string("meta/certificate_id") == kOaaCertificateId &&
      read_string("meta/readout_policy") == kOaaReadoutPolicy &&
      read_string("meta/anchor_sha256") == kOaaAnchorSha256[seed_position] &&
      read_string("meta/oca1_identity_cache_sha256") ==
          kOaaIdentityCacheSha256[seed_position] &&
      read_string("meta/ssl_data_hash") == ssl_hashes[0] &&
      read_string("meta/ssl_mask_hash") == ssl_hashes[1] &&
      read_string("meta/ssl_target_hash") == ssl_hashes[2];
  auto value = read_tensor("meta/complete");
  const bool complete = value.numel() == 1 && value.item<int64_t>() == 1;
  value = read_tensor("meta/seed");
  const bool seed_exact = value.numel() == 1 && value.item<int64_t>() == seed;
  value = read_tensor("meta/steps");
  const bool steps_exact = value.numel() == 1 && value.item<int64_t>() == steps;
  value = read_tensor("meta/arm_count");
  const bool arms_exact = value.numel() == 1 && value.item<int64_t>() == 8;
  value = read_tensor("meta/optimizer_learning_rate");
  const bool learning_rate_exact =
      value.numel() == 1 && value.item<double>() == kOaaLearningRate;
  value = read_tensor("meta/gradient_clip_norm");
  const bool clip_exact =
      value.numel() == 1 && value.item<double>() == kOaaClipNorm;
  value = read_tensor("meta/augmentation_seed_domain_u64_le");
  const auto domains = oca_u64_le_bytes_vector(value);
  const bool domain_exact =
      domains.size() == 1 && domains[0] == kOaaAugmentationSeedDomain;
  value = read_tensor("meta/objective_masks");
  const bool objectives_exact =
      oaa_i64_vector(value) == std::vector<int64_t>({1, 8});
  value = read_tensor("meta/profile_ids");
  const bool profiles_exact =
      oaa_i64_vector(value) == std::vector<int64_t>({1, 2, 3, 4});
  value = read_tensor("meta/result_flags");
  const auto result_flags = oaa_i64_vector(value);
  const bool global_flags_exact =
      result_flags == std::vector<int64_t>({1, 1, 1, 1});
  if (!metadata || !complete || !seed_exact || !steps_exact || !arms_exact ||
      !learning_rate_exact || !clip_exact || !domain_exact ||
      !objectives_exact || !profiles_exact || !global_flags_exact) {
    throw std::runtime_error("OAA seed cache metadata failed");
  }

  OaaSeedResult loaded{};
  loaded.models.reserve(8);
  loaded.initialization_exact.fill(true);
  loaded.anchor_metadata_exact = true;
  loaded.shared_profile_objects_exact = true;
  loaded.reference_state_unchanged = true;
  loaded.pass = true;
  for (std::size_t objective = 0; objective < kOaaObjectiveMasks.size();
       ++objective) {
    for (std::size_t profile = 0; profile < kOaaProfiles.size(); ++profile) {
      const auto index = oaa_arm_index(objective, profile);
      const std::string prefix = "arm_" + std::to_string(index) + "/";
      value = read_tensor(prefix + "objective_mask");
      const bool objective_exact =
          value.numel() == 1 &&
          value.item<int64_t>() == kOaaObjectiveMasks[objective];
      value = read_tensor(prefix + "profile_id");
      const bool profile_exact =
          value.numel() == 1 &&
          value.item<int64_t>() ==
              static_cast<int64_t>(static_cast<uint8_t>(kOaaProfiles[profile]));
      const bool model_manifest_exact =
          read_string((prefix + "model_config_manifest").c_str()) ==
          oaa_model_manifest(device, objective);
      const bool preprocessing_manifest_exact =
          read_string((prefix + "preprocessing_manifest").c_str()) ==
          oaa_preprocessing_manifest(device, profile);
      if (!objective_exact || !profile_exact || !model_manifest_exact ||
          !preprocessing_manifest_exact) {
        throw std::runtime_error("OAA seed cache arm metadata failed");
      }
      set_paired_rng(seed, device);
      auto model = mtf::MtfJepaMaeVicreg(
          attribution_config(device, oca_arm(kOaaObjectiveMasks[objective])));
      torch::serialize::InputArchive model_archive;
      root.read(prefix + "model", model_archive);
      model->load(model_archive);
      loaded.models.push_back(model);
      loaded.receipts[index] =
          oaa_read_receipt(root, prefix + "receipt/", steps);
    }
  }
  result = std::move(loaded);
  return true;
}

[[nodiscard]] bool oaa_receipts_exact(const OaaReceipt &left,
                                      const OaaReceipt &right) {
  return left.steps == right.steps && left.losses == right.losses &&
         left.gradient_norms == right.gradient_norms &&
         left.clip_factors == right.clip_factors &&
         left.served_update_norms == right.served_update_norms &&
         left.component_loss_sums == right.component_loss_sums &&
         left.row_hashes == right.row_hashes &&
         left.clean_data_hashes == right.clean_data_hashes &&
         left.clean_mask_hashes == right.clean_mask_hashes &&
         left.served_data_hashes == right.served_data_hashes &&
         left.served_mask_hashes == right.served_mask_hashes &&
         left.actual_forward_data_hashes == right.actual_forward_data_hashes &&
         left.actual_forward_mask_hashes == right.actual_forward_mask_hashes &&
         left.target_mask_hashes == right.target_mask_hashes &&
         left.context_mask_hashes == right.context_mask_hashes &&
         left.view_a_mask_hashes == right.view_a_mask_hashes &&
         left.view_b_mask_hashes == right.view_b_mask_hashes &&
         left.view_a_data_hashes == right.view_a_data_hashes &&
         left.view_b_data_hashes == right.view_b_data_hashes &&
         left.forward_pre_cpu_hashes == right.forward_pre_cpu_hashes &&
         left.forward_pre_cuda_hashes == right.forward_pre_cuda_hashes &&
         left.forward_post_cpu_hashes == right.forward_post_cpu_hashes &&
         left.forward_post_cuda_hashes == right.forward_post_cuda_hashes &&
         left.augmentation_seeds == right.augmentation_seeds &&
         left.retention_counts == right.retention_counts &&
         left.retention_values == right.retention_values &&
         left.step_flags == right.step_flags &&
         left.adam_steps == right.adam_steps &&
         left.ema_steps == right.ema_steps &&
         left.clipping_count == right.clipping_count &&
         left.all_trainable_delta == right.all_trainable_delta &&
         left.served_delta == right.served_delta &&
         left.predictor_delta == right.predictor_delta &&
         left.mae_decoder_delta == right.mae_decoder_delta &&
         left.vicreg_head_delta == right.vicreg_head_delta &&
         left.target_ema_delta == right.target_ema_delta &&
         left.finite == right.finite &&
         left.expected_partitions == right.expected_partitions &&
         left.mechanics == right.mechanics && left.pass == right.pass;
}

void oaa_reserve_receipt(OaaReceipt &receipt, int64_t steps) {
  const auto size = static_cast<std::size_t>(steps);
  receipt.steps = steps;
  receipt.losses.reserve(size);
  receipt.gradient_norms.reserve(size);
  receipt.clip_factors.reserve(size);
  receipt.served_update_norms.reserve(size);
  receipt.row_hashes.reserve(size);
  receipt.clean_data_hashes.reserve(size);
  receipt.clean_mask_hashes.reserve(size);
  receipt.served_data_hashes.reserve(size);
  receipt.served_mask_hashes.reserve(size);
  receipt.actual_forward_data_hashes.reserve(size);
  receipt.actual_forward_mask_hashes.reserve(size);
  receipt.target_mask_hashes.reserve(size);
  receipt.context_mask_hashes.reserve(size);
  receipt.view_a_mask_hashes.reserve(size);
  receipt.view_b_mask_hashes.reserve(size);
  receipt.view_a_data_hashes.reserve(size);
  receipt.view_b_data_hashes.reserve(size);
  receipt.forward_pre_cpu_hashes.reserve(size);
  receipt.forward_pre_cuda_hashes.reserve(size);
  receipt.forward_post_cpu_hashes.reserve(size);
  receipt.forward_post_cuda_hashes.reserve(size);
  receipt.augmentation_seeds.reserve(size);
  receipt.retention_counts.reserve(size * 5U);
  receipt.retention_values.reserve(size * 5U);
  receipt.step_flags.reserve(size * 12U);
}

void oaa_record_outer_update(OaaReceipt &receipt, uint64_t row_hash,
                             const ReplayedOuterAugmentation &augmented,
                             const GeneratorStateSnapshot &forward_pre,
                             const GeneratorStateSnapshot &forward_post,
                             const mtf::mtf_jepa_mae_vicreg_output_t &output,
                             uint64_t actual_forward_data_hash,
                             uint64_t actual_forward_mask_hash,
                             bool input_binding, bool masks_exact,
                             bool weak_support_exact,
                             bool generator_schedule_exact) {
  const auto &diagnostic = augmented.diagnostic;
  receipt.row_hashes.push_back(row_hash);
  receipt.clean_data_hashes.push_back(diagnostic.clean_data_hash);
  receipt.clean_mask_hashes.push_back(diagnostic.clean_mask_hash);
  receipt.served_data_hashes.push_back(diagnostic.served_data_hash);
  receipt.served_mask_hashes.push_back(diagnostic.served_mask_hash);
  receipt.actual_forward_data_hashes.push_back(actual_forward_data_hash);
  receipt.actual_forward_mask_hashes.push_back(actual_forward_mask_hash);
  receipt.target_mask_hashes.push_back(
      hash_tensor_stable_bytes(output.jepa_target_mask));
  receipt.context_mask_hashes.push_back(
      hash_tensor_stable_bytes(output.jepa_context_mask));
  receipt.view_a_mask_hashes.push_back(
      hash_tensor_stable_bytes(output.vicreg_view_a_feature_mask));
  receipt.view_b_mask_hashes.push_back(
      hash_tensor_stable_bytes(output.vicreg_view_b_feature_mask));
  receipt.view_a_data_hashes.push_back(
      hash_tensor_stable_bytes(output.vicreg_view_a_data));
  receipt.view_b_data_hashes.push_back(
      hash_tensor_stable_bytes(output.vicreg_view_b_data));
  receipt.forward_pre_cpu_hashes.push_back(forward_pre.digest.cpu);
  receipt.forward_pre_cuda_hashes.push_back(forward_pre.digest.cuda);
  receipt.forward_post_cpu_hashes.push_back(forward_post.digest.cpu);
  receipt.forward_post_cuda_hashes.push_back(forward_post.digest.cuda);
  receipt.augmentation_seeds.push_back(diagnostic.seed);
  receipt.retention_counts.insert(
      receipt.retention_counts.end(),
      {diagnostic.retention.clean_valid, diagnostic.retention.augmented_valid,
       diagnostic.retention.preserved, diagnostic.retention.added,
       diagnostic.retention.removed});
  receipt.retention_values.insert(
      receipt.retention_values.end(),
      {diagnostic.retention.overall, diagnostic.retention.terminal,
       diagnostic.retention.channel[0], diagnostic.retention.channel[1],
       diagnostic.retention.channel[2]});
  const bool support_exact =
      diagnostic.qualified_mask_exact && diagnostic.retention.overall == 1.0 &&
      diagnostic.retention.added == 0 && diagnostic.retention.removed == 0;
  const bool terminal_exact =
      diagnostic.retention.terminal == 1.0 &&
      std::all_of(diagnostic.retention.terminal_channel.begin(),
                  diagnostic.retention.terminal_channel.end(),
                  [](double value) { return value == 1.0; });
  const std::array<bool, 12> flags{diagnostic.augmentation_replay_exact,
                                   diagnostic.augmentation_consumed_state_exact,
                                   diagnostic.augmentation_cuda_unchanged,
                                   diagnostic.augmentation_state_restored,
                                   diagnostic.masked_values_zero,
                                   diagnostic.qualified_data_changed,
                                   support_exact,
                                   terminal_exact,
                                   input_binding,
                                   masks_exact,
                                   weak_support_exact,
                                   generator_schedule_exact};
  for (const bool flag : flags) {
    receipt.step_flags.push_back(flag ? 1 : 0);
    receipt.mechanics = receipt.mechanics && flag;
  }
}

[[nodiscard]] OaaSeedResult oaa_train_seed(const Dataset &ssl,
                                           const torch::Device &device,
                                           int64_t seed, int64_t steps) {
  if (steps <= 0) {
    throw std::runtime_error("OAA training steps must be positive");
  }
  OaaSeedResult result{};
  result.initialization_exact.fill(true);
  result.models.reserve(8);
  std::vector<std::unique_ptr<torch::optim::Adam>> optimizers;
  optimizers.reserve(8);
  std::array<ParameterSnapshot, 8> initial_parameters{};
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_config_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  set_paired_rng(seed, device);
  auto reference_model = mtf::MtfJepaMaeVicreg(anchor_config);
  result.anchor_metadata_exact =
      oca_load_archive(oca_archive_path(seed), reference_model, device, seed,
                       anchor_config_hash);
  const auto reference_state = oca_snapshot_state(reference_model);
  const auto canonical_initial = snapshot_parameters(reference_model);
  reference_model->train();

  for (std::size_t objective = 0; objective < kOaaObjectiveMasks.size();
       ++objective) {
    for (std::size_t profile = 0; profile < kOaaProfiles.size(); ++profile) {
      const auto index = oaa_arm_index(objective, profile);
      set_paired_rng(seed, device);
      auto model = mtf::MtfJepaMaeVicreg(
          attribution_config(device, oca_arm(kOaaObjectiveMasks[objective])));
      result.anchor_metadata_exact =
          result.anchor_metadata_exact &&
          oca_load_archive(oca_archive_path(seed), model, device, seed,
                           anchor_config_hash);
      result.initialization_exact[index] =
          oca_state_exact(model, reference_state) &&
          parameter_max_abs_diff(model, canonical_initial) == 0.0;
      initial_parameters[index] = snapshot_parameters(model);
      model->train();
      result.models.push_back(model);
      optimizers.push_back(std::make_unique<torch::optim::Adam>(
          result.models.back()->parameters(),
          torch::optim::AdamOptions(kOaaLearningRate)));
      oaa_reserve_receipt(result.receipts[index], steps);
    }
  }

  for (int64_t step = 0; step < steps; ++step) {
    const auto rows = training_rows(ssl, seed, step);
    const uint64_t row_hash = hash_batch_rows(rows);
    const auto row_index = torch::tensor(rows, torch::kInt64);
    const auto clean_data_cpu =
        ssl.data.index_select(0, row_index).contiguous();
    const auto clean_mask_cpu =
        ssl.mask.index_select(0, row_index).contiguous();
    const auto clean_data = clean_data_cpu.to(device);
    const auto clean_mask = clean_mask_cpu.to(device);

    std::array<ReplayedOuterAugmentation, 4> augmented{};
    std::array<torch::Tensor, 4> served_data{};
    std::array<torch::Tensor, 4> served_mask{};
    for (std::size_t profile = 0; profile < kOaaProfiles.size(); ++profile) {
      const auto preprocessing =
          oaa_preprocessing_config(device, kOaaProfiles[profile]);
      augmented[profile] = apply_outer_augmentation_replayed(
          clean_data_cpu, clean_mask_cpu, preprocessing,
          oaa_augmentation_seed(seed, kOaaProfiles[profile], step), device);
      served_data[profile] = augmented[profile].served.data.to(device);
      served_mask[profile] = augmented[profile].served.feature_mask.to(device);
    }

    set_paired_rng(paired_step_seed(seed, step), device);
    const auto identity_pre = current_generator_state_snapshot(device);
    mtf::mtf_jepa_mae_vicreg_output_t identity_output{};
    {
      torch::NoGradGuard no_grad;
      identity_output = reference_model->forward(clean_data, clean_mask);
    }
    const auto identity_post = current_generator_state_snapshot(device);
    validate_weak_view_debug_tensors(identity_output, clean_data, clean_mask);

    for (std::size_t objective = 0; objective < kOaaObjectiveMasks.size();
         ++objective) {
      const auto arm = oca_arm(kOaaObjectiveMasks[objective]);
      for (std::size_t profile = 0; profile < kOaaProfiles.size(); ++profile) {
        const auto index = oaa_arm_index(objective, profile);
        auto &model = result.models[index];
        auto &optimizer = *optimizers[index];
        auto &receipt = result.receipts[index];
        set_paired_rng(paired_step_seed(seed, step), device);
        const auto forward_pre = current_generator_state_snapshot(device);
        optimizer.zero_grad();
        const auto output =
            model->forward(served_data[profile], served_mask[profile]);
        const auto forward_post = current_generator_state_snapshot(device);
        validate_weak_view_debug_tensors(output, served_data[profile],
                                         served_mask[profile]);
        validate_stratified_vicreg_forward(output, arm, kModelRowBatchSize);
        const bool input_binding =
            hash_tensor_stable_bytes(served_data[profile]) ==
                augmented[profile].diagnostic.served_data_hash &&
            hash_tensor_stable_bytes(served_mask[profile]) ==
                augmented[profile].diagnostic.served_mask_hash;
        const uint64_t actual_forward_data_hash =
            hash_tensor_stable_bytes(served_data[profile]);
        const uint64_t actual_forward_mask_hash =
            hash_tensor_stable_bytes(served_mask[profile]);
        const bool masks_exact =
            torch::equal(output.jepa_target_mask,
                         identity_output.jepa_target_mask) &&
            torch::equal(output.jepa_context_mask,
                         identity_output.jepa_context_mask);
        const bool weak_support_exact =
            torch::equal(output.vicreg_view_a_feature_mask,
                         identity_output.vicreg_view_a_feature_mask) &&
            torch::equal(output.vicreg_view_b_feature_mask,
                         identity_output.vicreg_view_b_feature_mask);
        const bool generator_schedule_exact =
            generator_state_snapshot_equal(forward_pre, identity_pre) &&
            generator_state_snapshot_equal(forward_post, identity_post);
        oaa_record_outer_update(
            receipt, row_hash, augmented[profile], forward_pre, forward_post,
            output, actual_forward_data_hash, actual_forward_mask_hash,
            input_binding, masks_exact, weak_support_exact,
            generator_schedule_exact);

        const auto weights = attribution_arm_weights(arm, step);
        const auto loss = attribution_arm_loss(output, arm, weights);
        const double loss_value = loss.item<double>();
        receipt.losses.push_back(loss_value);
        receipt.component_loss_sums[0] += output.loss_jepa.item<double>();
        receipt.component_loss_sums[1] += output.loss_mae.item<double>();
        receipt.component_loss_sums[2] += output.loss_tf_align.item<double>();
        receipt.component_loss_sums[3] += output.loss_vicreg.item<double>();
        loss.backward();
        auto gradient_square =
            torch::zeros({}, torch::TensorOptions().device(device));
        for (const auto &parameter : model->parameters()) {
          if (!parameter.grad().defined()) {
            continue;
          }
          receipt.finite = receipt.finite &&
                           torch::isfinite(parameter.grad()).all().item<bool>();
          gradient_square =
              gradient_square + parameter.grad().detach().pow(2).sum();
        }
        const double gradient_norm = gradient_square.sqrt().item<double>();
        const double clip_factor =
            gradient_norm > kOaaClipNorm
                ? kOaaClipNorm / std::max(1.0e-30, gradient_norm)
                : 1.0;
        receipt.gradient_norms.push_back(gradient_norm);
        receipt.clip_factors.push_back(clip_factor);
        if (clip_factor < 1.0) {
          ++receipt.clipping_count;
          for (const auto &parameter : model->parameters()) {
            if (parameter.grad().defined()) {
              parameter.grad().mul_(clip_factor);
            }
          }
        }
        const auto served_before = served_parameter_vector(model);
        optimizer.step();
        ++receipt.adam_steps;
        const auto served_after = served_parameter_vector(model);
        const double update_norm =
            (served_after - served_before).norm().item<double>();
        receipt.served_update_norms.push_back(update_norm);
        model->update_target_network();
        ++receipt.ema_steps;
        receipt.finite =
            receipt.finite && std::isfinite(loss_value) &&
            torch::isfinite(output.embeddings).all().item<bool>() &&
            torch::isfinite(output.pooled_by_channel).all().item<bool>() &&
            torch::isfinite(output.loss_jepa).all().item<bool>() &&
            torch::isfinite(output.loss_mae).all().item<bool>() &&
            torch::isfinite(output.loss_tf_align).all().item<bool>() &&
            torch::isfinite(output.loss_vicreg).all().item<bool>() &&
            std::isfinite(gradient_norm) && std::isfinite(update_norm) &&
            gradient_norm > 0.0 && update_norm > 0.0 &&
            gradient_norm * clip_factor <= kOaaClipNorm + 1.0e-9;
      }
    }

    for (std::size_t profile = 0; profile < kOaaProfiles.size(); ++profile) {
      const auto &jepa = result.receipts[oaa_arm_index(0, profile)];
      const auto &vicreg = result.receipts[oaa_arm_index(1, profile)];
      const auto index = static_cast<std::size_t>(step);
      const bool shared =
          jepa.row_hashes[index] == vicreg.row_hashes[index] &&
          jepa.clean_data_hashes[index] == vicreg.clean_data_hashes[index] &&
          jepa.clean_mask_hashes[index] == vicreg.clean_mask_hashes[index] &&
          jepa.served_data_hashes[index] == vicreg.served_data_hashes[index] &&
          jepa.served_mask_hashes[index] == vicreg.served_mask_hashes[index] &&
          jepa.augmentation_seeds[index] == vicreg.augmentation_seeds[index];
      result.shared_profile_objects_exact =
          result.shared_profile_objects_exact && shared;
    }
    const auto &reference_receipt = result.receipts.front();
    const auto update_index = static_cast<std::size_t>(step);
    for (std::size_t arm_index = 1; arm_index < result.receipts.size();
         ++arm_index) {
      const auto &receipt = result.receipts[arm_index];
      result.shared_profile_objects_exact =
          result.shared_profile_objects_exact &&
          receipt.row_hashes[update_index] ==
              reference_receipt.row_hashes[update_index] &&
          receipt.clean_data_hashes[update_index] ==
              reference_receipt.clean_data_hashes[update_index] &&
          receipt.clean_mask_hashes[update_index] ==
              reference_receipt.clean_mask_hashes[update_index];
    }
    const int64_t completed = step + 1;
    if (completed % 128 == 0 || completed == steps) {
      std::cout << "oaa1.training.seed_" << seed << ".parallel_arms=8\n";
      std::cout << "oaa1.training.seed_" << seed
                << ".completed_steps=" << completed << '\n';
      std::cout << "oaa1.training.seed_" << seed
                << ".shared_profile_objects_exact="
                << result.shared_profile_objects_exact << '\n'
                << std::flush;
    }
  }

  result.reference_state_unchanged =
      oca_state_exact(reference_model, reference_state);
  result.pass = result.anchor_metadata_exact &&
                result.shared_profile_objects_exact &&
                result.reference_state_unchanged;
  for (std::size_t objective = 0; objective < kOaaObjectiveMasks.size();
       ++objective) {
    for (std::size_t profile = 0; profile < kOaaProfiles.size(); ++profile) {
      const auto index = oaa_arm_index(objective, profile);
      auto &model = result.models[index];
      auto &receipt = result.receipts[index];
      receipt.all_trainable_delta = parameter_partition_max_abs_diff(
          model, initial_parameters[index],
          ParameterDeltaPartition::all_trainable);
      receipt.served_delta = parameter_partition_max_abs_diff(
          model, initial_parameters[index], ParameterDeltaPartition::served);
      receipt.predictor_delta = parameter_partition_max_abs_diff(
          model, initial_parameters[index], ParameterDeltaPartition::predictor);
      receipt.mae_decoder_delta = parameter_partition_max_abs_diff(
          model, initial_parameters[index],
          ParameterDeltaPartition::mae_decoder);
      receipt.vicreg_head_delta = parameter_partition_max_abs_diff(
          model, initial_parameters[index],
          ParameterDeltaPartition::vicreg_head);
      receipt.target_ema_delta =
          parameter_partition_max_abs_diff(model, initial_parameters[index],
                                           ParameterDeltaPartition::target_ema);
      OcaLegacyTrainingReceipt head_activity{};
      head_activity.predictor_delta = receipt.predictor_delta;
      head_activity.mae_decoder_delta = receipt.mae_decoder_delta;
      head_activity.vicreg_head_delta = receipt.vicreg_head_delta;
      receipt.expected_partitions =
          receipt.all_trainable_delta > 0.0 && receipt.served_delta > 0.0 &&
          receipt.target_ema_delta > 0.0 &&
          oca_head_activity_exact(kOaaObjectiveMasks[objective], head_activity);
      receipt.pass = receipt.finite && receipt.mechanics &&
                     receipt.expected_partitions &&
                     oaa_receipt_shape_valid(receipt, steps);
      result.pass =
          result.pass && result.initialization_exact[index] && receipt.pass;
    }
  }
  return result;
}

[[nodiscard]] OaaSeedResult
oaa_train_or_resume_seed(std::string_view phase, const Dataset &ssl,
                         const torch::Device &device, int64_t seed,
                         int64_t steps, bool &resumed) {
  OaaSeedResult result{};
  resumed = oaa_load_seed_cache(phase, ssl, device, seed, steps, result);
  if (!resumed) {
    result = oaa_train_seed(ssl, device, seed, steps);
    if (!result.pass) {
      throw std::runtime_error("OAA seed training mechanics failed");
    }
    oaa_save_seed_cache(phase, ssl, device, seed, steps, result);
  }
  const auto path = oaa_cache_path(phase, seed);
  std::cout << "oaa1.cache." << phase << ".seed_" << seed
            << ".resumed=" << resumed << '\n';
  std::cout << "oaa1.cache." << phase << ".seed_" << seed
            << ".sha256=" << digest::sha256_hex(rmc_read_file(path)) << '\n';
  return result;
}

struct OaaProfileSmoke {
  std::array<bool, 4> profile{};
  bool seed_domain_exact{false};
  bool anchor_metadata_exact{false};
  bool reference_state_unchanged{false};
  bool pass{false};
};

[[nodiscard]] bool oaa_options_valid(const Options &options) {
  return options.device == "cuda" &&
         (options.steps < 0 || options.steps == kOaaSteps) &&
         (options.seeds < 0 ||
          options.seeds == static_cast<int64_t>(kAttributionSeeds.size())) &&
         options.weak_views;
}

void oaa_require_options(const Options &options) {
  if (!oaa_options_valid(options)) {
    throw std::runtime_error(
        "OAA-1 requires CUDA, 512 updates per new arm, 3 seeds, and weak "
        "views enabled");
  }
  rmc_configure_cuda();
}

[[nodiscard]] bool oaa_seed_domain_exact() {
  std::set<int64_t> augmentation_seeds;
  std::set<int64_t> forbidden_seeds;
  for (const int64_t seed : kAttributionSeeds) {
    forbidden_seeds.insert(seed);
    for (int64_t update = 0; update < kOaaSteps; ++update) {
      forbidden_seeds.insert(paired_step_seed(seed, update));
    }
  }
  for (const int64_t seed : kAttributionSeeds) {
    for (const auto profile : kOaaProfiles) {
      for (int64_t update = 0; update < kOaaSteps; ++update) {
        const int64_t value = oaa_augmentation_seed(seed, profile, update);
        if (!augmentation_seeds.insert(value).second ||
            forbidden_seeds.count(value) != 0) {
          return false;
        }
      }
    }
  }
  return augmentation_seeds.size() == kAttributionSeeds.size() *
                                          kOaaProfiles.size() *
                                          static_cast<std::size_t>(kOaaSteps);
}

void oaa_emit_custody(const OaaCustody &custody) {
  std::cout << "oaa1.custody.protocol=" << custody.protocol << '\n';
  std::cout << "oaa1.custody.oca_protocol=" << custody.oca_protocol << '\n';
  std::cout << "oaa1.custody.oca_log_hash=" << custody.oca_log_hash << '\n';
  std::cout << "oaa1.custody.oca_fields=" << custody.oca_fields << '\n';
  std::cout << "oaa1.custody.semantic_hash=" << custody.semantic_hash << '\n';
  std::cout << "oaa1.custody.semantic_exact_10_plus_global="
            << custody.semantic_fields << '\n';
  std::cout << "oaa1.custody.helper_hash=" << custody.helper_hash << '\n';
  std::cout << "oaa1.custody.fspa_protocol=" << custody.fspa_protocol << '\n';
  std::cout << "oaa1.custody.archives=" << custody.archives << '\n';
  std::cout << "oaa1.custody.archive_replay=" << custody.archive_replay << '\n';
  std::cout << "oaa1.custody.identity_cache_hashes="
            << custody.neutral_cache_hashes << '\n';
  std::cout << "oaa1.custody.identity_caches=" << custody.neutral_caches
            << '\n';
  std::cout << "oaa1.custody.pass=" << custody.pass << '\n';
}

[[nodiscard]] OaaProfileSmoke oaa_run_profile_smoke(const RmcData &data,
                                                    const torch::Device &device,
                                                    bool emit) {
  OaaProfileSmoke result{};
  result.seed_domain_exact = oaa_seed_domain_exact();
  const int64_t seed = kAttributionSeeds.front();
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_config_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  set_paired_rng(seed, device);
  auto model = mtf::MtfJepaMaeVicreg(anchor_config);
  result.anchor_metadata_exact = oca_load_archive(
      oca_archive_path(seed), model, device, seed, anchor_config_hash);
  const auto reference_state = oca_snapshot_state(model);
  model->train();

  const auto rows = training_rows(data.ssl, seed, 0);
  const auto row_index = torch::tensor(rows, torch::kInt64);
  const auto clean_data_cpu =
      data.ssl.data.index_select(0, row_index).contiguous();
  const auto clean_mask_cpu =
      data.ssl.mask.index_select(0, row_index).contiguous();
  const auto clean_data = clean_data_cpu.to(device);
  const auto clean_mask = clean_mask_cpu.to(device);
  set_paired_rng(paired_step_seed(seed, 0), device);
  const auto identity_pre = current_generator_state_snapshot(device);
  mtf::mtf_jepa_mae_vicreg_output_t identity{};
  {
    torch::NoGradGuard no_grad;
    identity = model->forward(clean_data, clean_mask);
  }
  const auto identity_post = current_generator_state_snapshot(device);
  validate_weak_view_debug_tensors(identity, clean_data, clean_mask);

  for (std::size_t profile = 0; profile < kOaaProfiles.size(); ++profile) {
    const auto replayed = apply_outer_augmentation_replayed(
        clean_data_cpu, clean_mask_cpu,
        oaa_preprocessing_config(device, kOaaProfiles[profile]),
        oaa_augmentation_seed(seed, kOaaProfiles[profile], 0), device);
    const auto served_data = replayed.served.data.to(device);
    const auto served_mask = replayed.served.feature_mask.to(device);
    const uint64_t data_before = hash_tensor_stable_bytes(served_data);
    const uint64_t mask_before = hash_tensor_stable_bytes(served_mask);
    set_paired_rng(paired_step_seed(seed, 0), device);
    const auto forward_pre = current_generator_state_snapshot(device);
    mtf::mtf_jepa_mae_vicreg_output_t output{};
    {
      torch::NoGradGuard no_grad;
      output = model->forward(served_data, served_mask);
    }
    const auto forward_post = current_generator_state_snapshot(device);
    validate_weak_view_debug_tensors(output, served_data, served_mask);
    const auto &diagnostic = replayed.diagnostic;
    const bool exact_support =
        diagnostic.qualified_mask_exact &&
        diagnostic.retention.overall == 1.0 &&
        diagnostic.retention.terminal == 1.0 &&
        diagnostic.retention.added == 0 && diagnostic.retention.removed == 0 &&
        diagnostic.retention.every_sample_channel_nonempty &&
        std::all_of(diagnostic.retention.channel.begin(),
                    diagnostic.retention.channel.end(),
                    [](double value) { return value == 1.0; }) &&
        std::all_of(diagnostic.retention.terminal_channel.begin(),
                    diagnostic.retention.terminal_channel.end(),
                    [](double value) { return value == 1.0; });
    const bool input_binding =
        data_before == diagnostic.served_data_hash &&
        mask_before == diagnostic.served_mask_hash &&
        data_before == hash_tensor_stable_bytes(served_data) &&
        mask_before == hash_tensor_stable_bytes(served_mask);
    const bool module_mechanics =
        torch::equal(output.jepa_target_mask, identity.jepa_target_mask) &&
        torch::equal(output.jepa_context_mask, identity.jepa_context_mask) &&
        torch::equal(output.vicreg_view_a_feature_mask,
                     identity.vicreg_view_a_feature_mask) &&
        torch::equal(output.vicreg_view_b_feature_mask,
                     identity.vicreg_view_b_feature_mask) &&
        generator_state_snapshot_equal(forward_pre, identity_pre) &&
        generator_state_snapshot_equal(forward_post, identity_post);
    result.profile[profile] =
        diagnostic.augmentation_replay_exact &&
        diagnostic.augmentation_consumed_state_exact &&
        diagnostic.augmentation_cuda_unchanged &&
        diagnostic.augmentation_state_restored &&
        diagnostic.masked_values_zero && diagnostic.qualified_data_changed &&
        exact_support && input_binding && module_mechanics &&
        torch::isfinite(replayed.served.data).all().item<bool>() &&
        torch::isfinite(output.embeddings).all().item<bool>();
    if (emit) {
      const std::string root =
          "oaa1.preflight.profile." + std::string(kOaaProfileNames[profile]);
      std::cout << root << ".preprocessing_manifest_fingerprint="
                << oca_hex_u64(
                       fnv1a64(oaa_preprocessing_manifest(device, profile)))
                << '\n';
      std::cout << root
                << ".replay_exact=" << diagnostic.augmentation_replay_exact
                << '\n';
      std::cout << root << ".support_and_terminal_exact=" << exact_support
                << '\n';
      std::cout << root << ".actual_forward_input_binding=" << input_binding
                << '\n';
      std::cout << root << ".internal_mask_mechanics_exact=" << module_mechanics
                << '\n';
      std::cout << root << ".pass=" << result.profile[profile] << '\n';
    }
  }
  result.reference_state_unchanged = oca_state_exact(model, reference_state);
  result.pass = result.seed_domain_exact && result.anchor_metadata_exact &&
                result.reference_state_unchanged &&
                std::all_of(result.profile.begin(), result.profile.end(),
                            [](bool value) { return value; });
  return result;
}

void oaa_emit_receipt(const std::string &root, const OaaReceipt &receipt) {
  const auto minimum = [](const std::vector<double> &values) {
    return *std::min_element(values.begin(), values.end());
  };
  const auto maximum = [](const std::vector<double> &values) {
    return *std::max_element(values.begin(), values.end());
  };
  std::cout << root << ".steps=" << receipt.steps << '\n';
  std::cout << root << ".loss_first=" << receipt.losses.front() << '\n';
  std::cout << root << ".loss_last=" << receipt.losses.back() << '\n';
  std::cout << root << ".gradient_min=" << minimum(receipt.gradient_norms)
            << '\n';
  std::cout << root << ".gradient_max=" << maximum(receipt.gradient_norms)
            << '\n';
  std::cout << root
            << ".served_update_min=" << minimum(receipt.served_update_norms)
            << '\n';
  std::cout << root
            << ".served_update_max=" << maximum(receipt.served_update_norms)
            << '\n';
  std::cout << root << ".clip_count=" << receipt.clipping_count << '\n';
  std::cout << root << ".served_delta=" << receipt.served_delta << '\n';
  std::cout << root << ".predictor_delta=" << receipt.predictor_delta << '\n';
  std::cout << root << ".mae_decoder_delta=" << receipt.mae_decoder_delta
            << '\n';
  std::cout << root << ".vicreg_head_delta=" << receipt.vicreg_head_delta
            << '\n';
  std::cout << root << ".target_ema_delta=" << receipt.target_ema_delta << '\n';
  std::cout << root << ".mechanics=" << receipt.mechanics << '\n';
  std::cout << root << ".pass=" << receipt.pass << '\n';
}

int run_oaa_preflight(const Options &options) {
  oaa_require_options(options);
  const torch::Device device(torch::kCUDA, 0);
  auto data = rmc_make_data();
  const auto custody = oaa_validate_custody(data, device, true);
  const auto smoke = oaa_run_profile_smoke(data, device, true);
  const bool pass = custody.pass && smoke.pass;
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.oaa1.preflight.v1\n";
  std::cout << "experiment=outer-augmentation-causal-attribution-preflight\n";
  std::cout << "module_only=true\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "training_labels_used=false\n";
  std::cout << "optimizer_steps=0\n";
  std::cout << "eligible_profile_smokes=4\n";
  std::cout << "oaa1.protocol.sha256=" << kOaaProtocolSha256 << '\n';
  oaa_emit_custody(custody);
  std::cout << "oaa1.preflight.seed_domain_exact=" << smoke.seed_domain_exact
            << '\n';
  std::cout << "oaa1.preflight.reference_state_unchanged="
            << smoke.reference_state_unchanged << '\n';
  std::cout << "oaa1.preflight.pass=" << pass << '\n';
  std::cout << "execution_status=oaa1_preflight_complete\n";
  return pass ? 0 : 3;
}

int run_oaa_cache_smoke(const Options &options) {
  oaa_require_options(options);
  const torch::Device device(torch::kCUDA, 0);
  auto data = rmc_make_data();
  const auto custody = oaa_validate_custody(data, device, true);
  const auto smoke = oaa_run_profile_smoke(data, device, false);
  if (!custody.pass || !smoke.pass) {
    throw std::runtime_error("OAA cache smoke pre-optimizer gate failed");
  }
  constexpr int64_t smoke_steps = 2;
  const int64_t seed = kAttributionSeeds.front();
  OaaSeedResult trained{};
  const bool preexisting = oaa_load_seed_cache("cache_smoke", data.ssl, device,
                                               seed, smoke_steps, trained);
  if (!preexisting) {
    trained = oaa_train_seed(data.ssl, device, seed, smoke_steps);
    if (!trained.pass) {
      throw std::runtime_error("OAA cache smoke training mechanics failed");
    }
    oaa_save_seed_cache("cache_smoke", data.ssl, device, seed, smoke_steps,
                        trained);
  }
  OaaSeedResult restored{};
  const bool loaded = oaa_load_seed_cache("cache_smoke", data.ssl, device, seed,
                                          smoke_steps, restored);
  bool state_exact = loaded && restored.models.size() == trained.models.size();
  bool receipt_exact = state_exact;
  bool structured_exact = state_exact;
  if (state_exact) {
    for (std::size_t arm = 0; arm < trained.models.size(); ++arm) {
      state_exact = state_exact &&
                    oca_state_exact(restored.models[arm],
                                    oca_snapshot_state(trained.models[arm]));
      receipt_exact =
          receipt_exact &&
          oaa_receipts_exact(trained.receipts[arm], restored.receipts[arm]);
      structured_exact =
          structured_exact &&
          oca_capture_exact(
              oca_capture_structured(trained.models[arm], data.probe_validation,
                                     device),
              oca_capture_structured(restored.models[arm],
                                     data.probe_validation, device));
    }
  }
  const auto path = oaa_cache_path("cache_smoke", seed);
  const bool complete_files =
      std::filesystem::exists(path) &&
      std::filesystem::exists(oaa_cache_marker_path(path));
  const bool pass = trained.pass && restored.pass && loaded && state_exact &&
                    receipt_exact && structured_exact && complete_files;
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.oaa1.cache_smoke.v1\n";
  std::cout << "experiment=outer-augmentation-causal-attribution-cache-smoke\n";
  std::cout << "module_only=true\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "training_labels_used=false\n";
  std::cout << "optimizer_updates_per_arm=" << smoke_steps << '\n';
  std::cout << "new_model_updates=" << smoke_steps * 8 << '\n';
  std::cout << "oaa1.cache_smoke.preexisting=" << preexisting << '\n';
  std::cout << "oaa1.cache_smoke.loaded=" << loaded << '\n';
  std::cout << "oaa1.cache_smoke.state_exact=" << state_exact << '\n';
  std::cout << "oaa1.cache_smoke.receipt_exact=" << receipt_exact << '\n';
  std::cout << "oaa1.cache_smoke.structured_exact=" << structured_exact << '\n';
  std::cout << "oaa1.cache_smoke.complete_files=" << complete_files << '\n';
  std::cout << "oaa1.cache_smoke.path=" << path.generic_string() << '\n';
  std::cout << "oaa1.cache_smoke.pass=" << pass << '\n';
  std::cout << "execution_status=oaa1_cache_smoke_complete\n";
  return pass ? 0 : 3;
}

[[nodiscard]] oaa_gate::PairedContrast
oaa_gate_contrast(const rmc_gate::Contrast &value) {
  return {.point = value.point,
          .low = value.low,
          .high = value.high,
          .positive_seed_count = value.positive_seed_count};
}

[[nodiscard]] bool oaa_reproducible_new_safeguard_failure(
    const std::array<RmcEvaluation, 3> &anchor,
    const std::array<RmcEvaluation, 3> &identity,
    const std::array<RmcEvaluation, 3> &candidate, double raw_area) {
  bool candidate_fails_all = true;
  bool identity_passes_all = true;
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    candidate_fails_all =
        candidate_fails_all &&
        !oca_seed_safeguards_pass(candidate[seed], anchor[seed], raw_area);
    identity_passes_all =
        identity_passes_all &&
        oca_seed_safeguards_pass(identity[seed], anchor[seed], raw_area);
  }
  return candidate_fails_all && identity_passes_all;
}

[[nodiscard]] oaa_gate::CandidateInput
oaa_make_candidate_input(std::size_t objective, std::size_t profile,
                         const RmcSummary &candidate_minus_anchor,
                         const RmcSummary &candidate_minus_identity,
                         const std::array<RmcEvaluation, 3> &anchor,
                         const std::array<RmcEvaluation, 3> &identity,
                         const std::array<RmcEvaluation, 3> &candidate,
                         double raw_area, bool mechanics, bool semantic) {
  const auto &anchor_candidate = candidate_minus_anchor.candidate[0];
  const auto &anchor_gate = candidate_minus_anchor.gate.neutral;
  const auto &identity_gate = candidate_minus_identity.gate.neutral;
  oaa_gate::CandidateInput result{};
  result.objective = static_cast<oaa_gate::Objective>(objective);
  result.profile = static_cast<oaa_gate::Profile>(profile);
  result.candidate_minus_anchor =
      oaa_gate_contrast(anchor_candidate.gate.trained_minus_initialization);
  result.candidate_minus_identity_objective = oaa_gate_contrast(
      candidate_minus_identity.candidate[0].gate.trained_minus_initialization);
  result.family_deltas_vs_anchor = anchor_candidate.gate.learned_family_deltas;
  result.safeguards = {.raw_control = anchor_gate.raw_noninferiority_pass,
                       .reversal_order = anchor_gate.order_point_pass &&
                                         anchor_gate.order_lower_pass &&
                                         anchor_gate.order_retention_pass,
                       .continuous_shuffle =
                           anchor_gate.continuous_shuffle_pass,
                       .order_shuffle = anchor_gate.order_shuffle_pass,
                       .geometry = anchor_gate.geometry_pass};
  result.semantic = semantic;
  result.mechanics =
      mechanics && anchor_gate.numeric_valid && anchor_gate.mechanics_pass &&
      identity_gate.numeric_valid && identity_gate.mechanics_pass;
  result.reproducible_new_safeguard_failure =
      oaa_reproducible_new_safeguard_failure(anchor, identity, candidate,
                                             raw_area);
  return result;
}

void oaa_emit_gate_result(const std::string &root,
                          const oaa_gate::CandidateResult &result) {
  std::cout << root << ".numeric_inputs_valid=" << result.numeric_inputs_valid
            << '\n';
  std::cout << root << ".mechanics_pass=" << result.mechanics_pass << '\n';
  std::cout << root << ".semantic_pass=" << result.semantic_pass << '\n';
  std::cout << root << ".anchor_point_pass=" << result.anchor_point_pass
            << '\n';
  std::cout << root
            << ".anchor_lower_bound_pass=" << result.anchor_lower_bound_pass
            << '\n';
  std::cout << root
            << ".all_anchor_seeds_improve=" << result.all_anchor_seeds_improve
            << '\n';
  std::cout << root
            << ".identity_lower_bound_pass=" << result.identity_lower_bound_pass
            << '\n';
  std::cout << root << ".positive_family_count=" << result.positive_family_count
            << '\n';
  std::cout << root
            << ".all_family_floors_pass=" << result.all_family_floors_pass
            << '\n';
  std::cout << root << ".all_frozen_safeguards_pass="
            << result.all_frozen_safeguards_pass << '\n';
  std::cout << root << ".reproducible_new_safeguard_failure="
            << result.reproducible_new_safeguard_failure << '\n';
  std::cout << root << ".rescue_pass=" << result.rescue_pass << '\n';
  std::cout << root << ".worsening_pass=" << result.worsening_pass << '\n';
  std::cout << root << ".mitigation_pass=" << result.mitigation_pass << '\n';
  std::cout << root << ".classification="
            << oaa_gate::classification_name(result.classification) << '\n';
}

[[nodiscard]] RmcEvaluation oaa_evaluate(mtf::MtfJepaMaeVicreg &model,
                                         const RmcData &data,
                                         const RmcEvalTargets &targets,
                                         const torch::Device &device,
                                         bool confirmation) {
  const auto &evaluation = confirmation ? data.confirmation : data.development;
  const auto &reversed =
      confirmation ? data.reversed_confirmation : data.reversed_development;
  return rmc_evaluate(model, data.probe_train, data.probe_validation,
                      evaluation, data.reversed_train, data.reversed_validation,
                      reversed, targets, device);
}

int run_oaa_attribution(const Options &options) {
  oaa_require_options(options);
  const torch::Device device(torch::kCUDA, 0);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.oaa1.attribution.v1\n";
  std::cout << "experiment=outer-augmentation-causal-attribution\n";
  std::cout << "module_only=true\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "readout_policy=" << kOaaReadoutPolicy << '\n';
  std::cout << "training_labels_used=false\n";
  std::cout << "objective_masks=1,8\n";
  std::cout << "identity_models_retrained=false\n";
  std::cout << "new_parallel_arms=8\n";
  std::cout << "optimizer_updates_per_new_arm=" << kOaaSteps << '\n';
  std::cout << "new_model_updates=" << kOaaSteps * 8 * 3 << '\n';
  std::cout << "oaa1.protocol.sha256=" << kOaaProtocolSha256 << '\n';

  auto data = rmc_make_data();
  const auto targets = rmc_make_targets(data, false);
  const auto bootstrap_rows = rmc_bootstrap_rows(256);
  if (!rmc_bootstrap_rows_valid(bootstrap_rows, 256)) {
    throw std::runtime_error("OAA bootstrap row table failed");
  }
  const auto raw = rssm_probe_curve(
      data.raw_train, data.raw_validation, data.raw_development,
      data.probe_train.target, data.probe_validation.target,
      data.development.target, /*dual=*/true);
  const auto custody = oaa_validate_custody(data, device, false);
  const auto smoke = oaa_run_profile_smoke(data, device, true);
  oaa_emit_custody(custody);
  std::cout << "oaa1.preoptimizer.profile_smoke=" << smoke.pass << '\n';
  if (!custody.pass || !smoke.pass) {
    std::cout << "oaa1.training.opened=false\n";
    std::cout << "execution_status=oaa1_preoptimizer_gate_failed\n";
    return 3;
  }

  using SeedEvaluations = std::array<RmcEvaluation, 3>;
  SeedEvaluations anchor_evaluations{};
  std::array<SeedEvaluations, 2> identity_evaluations{};
  std::vector<mtf::MtfJepaMaeVicreg> anchor_models;
  std::vector<mtf::MtfJepaMaeVicreg> identity_models;
  anchor_models.reserve(3);
  identity_models.reserve(6);
  std::array<bool, 2> identity_mechanics{true, true};
  bool baseline_replay_exact = true;
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_config_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  const std::vector<uint8_t> challenge_masks(kOaaOcaChallengeMasks.begin(),
                                             kOaaOcaChallengeMasks.end());
  constexpr std::array<std::size_t, 2> identity_positions{1, 3};
  static_assert(kOaaOcaChallengeMasks[identity_positions[0]] == 1U);
  static_assert(kOaaOcaChallengeMasks[identity_positions[1]] == 8U);

  // Complete every frozen-baseline custody and evaluation replay before the
  // first OAA optimizer update.
  for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
       ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    set_paired_rng(seed, device);
    auto anchor = mtf::MtfJepaMaeVicreg(anchor_config);
    set_paired_rng(seed, device);
    auto anchor_replay = mtf::MtfJepaMaeVicreg(anchor_config);
    const bool metadata = oca_load_archive(oca_archive_path(seed), anchor,
                                           device, seed, anchor_config_hash);
    const bool replay_metadata =
        oca_load_archive(oca_archive_path(seed), anchor_replay, device, seed,
                         anchor_config_hash);
    const auto anchor_evaluation =
        oaa_evaluate(anchor, data, targets, device, false);
    const auto replay_evaluation =
        oaa_evaluate(anchor_replay, data, targets, device, false);
    const bool anchor_exact =
        metadata && replay_metadata &&
        oca_state_exact(anchor_replay, oca_snapshot_state(anchor)) &&
        oca_capture_exact(
            oca_capture_structured(anchor, data.development, device),
            oca_capture_structured(anchor_replay, data.development, device)) &&
        rmc_evaluations_exact(anchor_evaluation, replay_evaluation) &&
        oca_evaluation_predictions_bytes_exact(anchor_evaluation,
                                               replay_evaluation);
    baseline_replay_exact = baseline_replay_exact && anchor_exact;
    anchor_evaluations[seed_index] = anchor_evaluation;
    anchor_models.push_back(anchor);

    OcaInterleavedTrainingResult identity_cache{};
    const bool identity_loaded =
        oca_load_seed_cache("anchor_challenge", data.ssl, device, seed,
                            challenge_masks, kOcaAnchorChallengeSteps,
                            /*load_certified_anchor=*/true, identity_cache);
    const bool inventory_exact =
        identity_loaded && identity_cache.models.size() == 5 &&
        identity_cache.receipts.size() == 5 &&
        identity_cache.initialization_exact.size() == 5;
    baseline_replay_exact = baseline_replay_exact && inventory_exact;
    if (!inventory_exact) {
      throw std::runtime_error("OAA cached identity inventory failed");
    }
    for (std::size_t objective = 0; objective < 2; ++objective) {
      const auto position = identity_positions[objective];
      identity_mechanics[objective] =
          identity_mechanics[objective] && identity_cache.metadata_exact &&
          identity_cache.schedule_exact && identity_cache.pass &&
          identity_cache.initialization_exact[position] &&
          identity_cache.receipts[position].pass;
      identity_evaluations[objective][seed_index] = oaa_evaluate(
          identity_cache.models[position], data, targets, device, false);
      identity_models.push_back(identity_cache.models[position]);
      std::cout << "oaa1.identity.seed_" << seed << ".objective."
                << kOaaObjectiveNames[objective]
                << ".mask=" << static_cast<int>(kOaaObjectiveMasks[objective])
                << '\n';
    }
    std::cout << "oaa1.anchor.seed_" << seed
              << ".evaluation_replay_exact=" << anchor_exact << '\n';
  }
  baseline_replay_exact =
      baseline_replay_exact && identity_mechanics[0] && identity_mechanics[1];
  std::cout << "oaa1.baseline_replay_exact=" << baseline_replay_exact << '\n';
  if (!baseline_replay_exact) {
    std::cout << "oaa1.training.opened=false\n";
    std::cout << "execution_status=oaa1_baseline_replay_failed\n";
    return 3;
  }

  std::cout << "oaa1.training.opened=true\n" << std::flush;
  std::array<OaaSeedResult, 3> training{};
  std::array<std::array<std::array<RmcEvaluation, 3>, 4>, 2>
      candidate_evaluations{};
  std::array<std::array<bool, 4>, 2> mechanics{};
  for (auto &objective : mechanics) {
    objective.fill(true);
  }
  for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
       ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    bool resumed = false;
    training[seed_index] = oaa_train_or_resume_seed(
        "development", data.ssl, device, seed, kOaaSteps, resumed);
    for (std::size_t objective = 0; objective < 2; ++objective) {
      for (std::size_t profile = 0; profile < 4; ++profile) {
        const auto arm = oaa_arm_index(objective, profile);
        candidate_evaluations[objective][profile][seed_index] = oaa_evaluate(
            training[seed_index].models[arm], data, targets, device, false);
        mechanics[objective][profile] =
            mechanics[objective][profile] && training[seed_index].pass &&
            training[seed_index].initialization_exact[arm] &&
            training[seed_index].receipts[arm].pass &&
            identity_mechanics[objective];
        const std::string root = "oaa1.training.seed_" + std::to_string(seed) +
                                 ".objective." + kOaaObjectiveNames[objective] +
                                 ".profile." + kOaaProfileNames[profile];
        std::cout << root << ".resumed=" << resumed << '\n';
        std::cout
            << root << ".clean_final_aulc="
            << candidate_evaluations[objective][profile][seed_index].probe.area
            << '\n';
        oaa_emit_receipt(root, training[seed_index].receipts[arm]);
        std::cout << root << ".complete=true\n" << std::flush;
      }
    }
  }

  oaa_gate::CandidateInventory inventory{};
  std::array<std::array<RmcSummary, 4>, 2> anchor_summaries{};
  std::array<std::array<RmcSummary, 4>, 2> identity_summaries{};
  for (std::size_t objective = 0; objective < 2; ++objective) {
    for (std::size_t profile = 0; profile < 4; ++profile) {
      anchor_summaries[objective][profile] = oca_pair_summary(
          anchor_evaluations, candidate_evaluations[objective][profile], raw,
          data.development.target, targets, bootstrap_rows,
          mechanics[objective][profile]);
      identity_summaries[objective][profile] =
          oca_pair_summary(identity_evaluations[objective],
                           candidate_evaluations[objective][profile], raw,
                           data.development.target, targets, bootstrap_rows,
                           mechanics[objective][profile]);
      const auto index = oaa_arm_index(objective, profile);
      inventory[index] = oaa_make_candidate_input(
          objective, profile, anchor_summaries[objective][profile],
          identity_summaries[objective][profile], anchor_evaluations,
          identity_evaluations[objective],
          candidate_evaluations[objective][profile], raw.area,
          mechanics[objective][profile], custody.semantic_fields);
      const std::string root = "oaa1.development.objective." +
                               std::string(kOaaObjectiveNames[objective]) +
                               ".profile." + kOaaProfileNames[profile];
      oca_emit_candidate_summary(root + ".candidate_minus_anchor",
                                 anchor_summaries[objective][profile]);
      oca_emit_candidate_summary(root + ".candidate_minus_identity",
                                 identity_summaries[objective][profile]);
    }
  }
  const auto experiment = oaa_gate::evaluate_experiment(inventory);
  for (std::size_t objective = 0; objective < 2; ++objective) {
    for (std::size_t profile = 0; profile < 4; ++profile) {
      const auto index = oaa_arm_index(objective, profile);
      const std::string root = "oaa1.development.objective." +
                               std::string(kOaaObjectiveNames[objective]) +
                               ".profile." + kOaaProfileNames[profile] +
                               ".gate";
      oaa_emit_gate_result(root, experiment.candidates[index]);
    }
  }
  std::cout << "oaa1.development.inventory_exact=" << experiment.inventory_exact
            << '\n';
  std::cout << "oaa1.development.experiment_valid="
            << experiment.experiment_valid << '\n';

  bool confirmation_opened = false;
  bool confirmation_pass = false;
  bool confirmation_valid = true;
  std::string selected_name = "none";
  if (experiment.confirmation_open_authorized) {
    confirmation_opened = true;
    const auto selected_index = experiment.selected_candidate_index;
    const std::size_t objective = selected_index / kOaaProfiles.size();
    const std::size_t profile = selected_index % kOaaProfiles.size();
    selected_name = std::string(kOaaObjectiveNames[objective]) + ":" +
                    kOaaProfileNames[profile];
    oca_open_confirmation(data);
    if (data.confirmation.group_begin != 5000000 ||
        data.confirmation.data.size(0) != 256) {
      throw std::runtime_error("OAA confirmation row custody failed");
    }
    const auto confirmation_targets = rmc_make_targets(data, true);
    const auto raw_confirmation = rssm_probe_curve(
        data.raw_train, data.raw_validation, data.raw_confirmation,
        data.probe_train.target, data.probe_validation.target,
        data.confirmation.target, /*dual=*/true);
    SeedEvaluations confirmation_anchor{};
    SeedEvaluations confirmation_identity{};
    SeedEvaluations confirmation_candidate{};
    for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
      confirmation_anchor[seed] = oaa_evaluate(
          anchor_models[seed], data, confirmation_targets, device, true);
      confirmation_identity[seed] =
          oaa_evaluate(identity_models[seed * 2 + objective], data,
                       confirmation_targets, device, true);
      confirmation_candidate[seed] =
          oaa_evaluate(training[seed].models[oaa_arm_index(objective, profile)],
                       data, confirmation_targets, device, true);
    }
    const auto confirmation_anchor_summary = oca_pair_summary(
        confirmation_anchor, confirmation_candidate, raw_confirmation,
        data.confirmation.target, confirmation_targets, bootstrap_rows,
        mechanics[objective][profile]);
    const auto confirmation_identity_summary = oca_pair_summary(
        confirmation_identity, confirmation_candidate, raw_confirmation,
        data.confirmation.target, confirmation_targets, bootstrap_rows,
        mechanics[objective][profile]);
    const auto confirmation_input = oaa_make_candidate_input(
        objective, profile, confirmation_anchor_summary,
        confirmation_identity_summary, confirmation_anchor,
        confirmation_identity, confirmation_candidate, raw_confirmation.area,
        mechanics[objective][profile], custody.semantic_fields);
    const auto confirmation_result =
        oaa_gate::evaluate_candidate(confirmation_input);
    confirmation_pass =
        confirmation_result.classification ==
        oaa_gate::Classification::outer_augmentation_rescues_objective;
    confirmation_valid = confirmation_result.classification !=
                         oaa_gate::Classification::invalid_numeric_or_mechanics;
    oca_emit_candidate_summary("oaa1.confirmation.candidate_minus_anchor",
                               confirmation_anchor_summary);
    oca_emit_candidate_summary("oaa1.confirmation.candidate_minus_identity",
                               confirmation_identity_summary);
    oaa_emit_gate_result("oaa1.confirmation.gate", confirmation_result);
  }
  std::cout << "oaa1.development.selected_rescue=" << selected_name << '\n';
  std::cout << "oaa1.confirmation.opened=" << confirmation_opened << '\n';
  std::cout << "oaa1.confirmation.group_begin="
            << (confirmation_opened ? 5000000 : -1) << '\n';
  std::cout << "oaa1.confirmation.rows=" << (confirmation_opened ? 256 : 0)
            << '\n';
  std::cout << "oaa1.confirmation.pass=" << confirmation_pass << '\n';
  std::cout << "oaa1.confirmation.valid=" << confirmation_valid << '\n';
  std::cout << "oaa1.promotion=" << (confirmation_pass ? selected_name : "none")
            << '\n';
  std::cout << "oaa1.canonical_rollback="
            << "fspa4_minimal_spectral_repair_v1:structured_cdsb_sparse_v1\n";
  std::cout << "oaa1.operational_rollback=all_tokens\n";
  std::cout << "oaa1.rollback_preserved=true\n";
  const bool measurements_valid =
      experiment.experiment_valid && confirmation_valid;
  std::cout << "oaa1.intrinsic_view_mask_attribution_authorized="
            << (measurements_valid && !confirmation_pass) << '\n';
  std::cout << "execution_status="
            << (measurements_valid ? "oaa1_measurements_complete"
                                   : "oaa1_measurements_invalid")
            << '\n';
  return measurements_valid ? 0 : 3;
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment ==
        "outer-augmentation-causal-attribution-preflight") {
      return run_oaa_preflight(options);
    }
    if (options.experiment ==
        "outer-augmentation-causal-attribution-cache-smoke") {
      return run_oaa_cache_smoke(options);
    }
    if (options.experiment == "outer-augmentation-causal-attribution") {
      return run_oaa_attribution(options);
    }
    throw std::runtime_error(
        "--experiment must be outer-augmentation-causal-attribution-"
        "preflight or outer-augmentation-causal-attribution-cache-smoke or "
        "outer-augmentation-causal-attribution");
  } catch (const c10::Error &error) {
    std::cerr << "outer_augmentation_causal_attribution_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "outer_augmentation_causal_attribution_error=" << error.what()
              << '\n';
  }
  return 2;
}
