#include "piaabo/digest/sha256.h"

#define CUWACUNU_OCA_EMBEDDED
#include "quality_wikimyei_mtf_jepa_mae_vicreg_four_objective_causal_attribution.cpp"
#undef CUWACUNU_OCA_EMBEDDED

namespace {

constexpr std::string_view kVvaProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "VICREG_VIEW_AUGMENTATION_CAUSAL_ATTRIBUTION_PROTOCOL.md";
constexpr std::string_view kVvaProtocolSha256 =
    "8e5f3e0aecf9990cb5adb54e090d781cc91b22c5880b8ae622c1fea10fa33616";
constexpr std::string_view kVvaOcaLogPath =
    ".build/tests/oca1/oca1_full_run.log";
constexpr std::string_view kVvaOcaLogSha256 =
    "3ade025b37525c45d376eabaca2c31771ec7d23fbd2f32f26277cec0d9be606d";
constexpr std::string_view kVvaOaaFindingsPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "OUTER_AUGMENTATION_CAUSAL_ATTRIBUTION_FINDINGS.md";
constexpr std::string_view kVvaOaaFindingsSha256 =
    "42abd19f65f9a41ce50bed1d481ecf983750499a34a6f9d9799e232d7503a9c7";
constexpr std::string_view kVvaIma1FindingsPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/INTRINSIC_MASK_VIEW_CAUSAL_ATTRIBUTION_FINDINGS.md";
constexpr std::string_view kVvaIma1FindingsSha256 =
    "ee53b9a97bf1b80153f7fd22ecf5c6dd9857cb0b3dccdb183729e5cfa05854d6";
constexpr std::string_view kVvaModulePath =
    "src/include/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h";
constexpr std::string_view kVvaModuleSha256 =
    "93640972e497dc49f37e7690e59c2f2e55f12ece25687fe4e6f6c96b28c3c9ea";
constexpr std::string_view kVvaReadoutPolicy = "structured_cdsb_sparse_v1";
constexpr int64_t kVvaSteps = 512;
constexpr double kVvaLearningRate = 1.0e-3;
constexpr double kVvaClipNorm = 5.0;
constexpr double kVvaCausalFloor = 0.0025;
constexpr double kVvaRescueFloor = 0.005;
constexpr double kVvaAnchorNoninferiority = -0.005;
constexpr double kVvaFamilyFloor = -0.02;
constexpr int64_t kVvaConfirmationGroupBegin = 9000000;
constexpr int64_t kVvaConfirmationRows = 256;
constexpr std::array<const char *, 4> kVvaProfileNames{
    "clean_identical", "time_only", "jitter_only", "current_time_jitter"};
constexpr std::array<std::string_view, 3> kVvaAnchorSha256{
    "5d96b2961daa2bbd08a07a157ddab5debd9d4928234d8341b3961678327e9434",
    "a85c00d5694d1e3f0063e8bce6fc3c2e3132a393e5bcee195909742754e76775",
    "b9c2f82f26a5516069f8460095d3a2b85c482b7cd0e20db120ce2e0bbd68e392"};
constexpr std::array<std::string_view, 3> kVvaCurrentCacheSha256{
    "5290559894fa6e3f5d2fd57f32a90e97bb0eec924ec22cc9354ae26d0e629c92",
    "bb5391840ede1ad4aaf8ff760d39b796a7a300918e1aab881fbb5255051fcc39",
    "aaa7dd4b7638f240d1e940db7ede3bc40eb8ad01000baa90dc043801a29256a6"};
constexpr std::array<uint8_t, 5> kVvaChallengeMasks{4U, 1U, 2U, 8U, 15U};
constexpr std::size_t kVvaCurrentCachePosition = 3;

struct VvaCustody {
  bool protocol{false};
  bool oca_log{false};
  bool oca_fields{false};
  bool oaa_findings{false};
  bool oaa_fields{false};
  bool ima1_findings{false};
  bool ima1_fields{false};
  bool module{false};
  bool archives{false};
  bool current_caches{false};
  bool pass{false};
};

struct VvaStage0 {
  bool custody{false};
  bool profile_inventory{false};
  bool only_two_scalars_differ{false};
  bool current_manifest_exact{false};
  bool initialization_exact{false};
  bool feature_dropout_inactive{false};
  bool rng_schedule_exact{false};
  bool jepa_masks_exact{false};
  bool clean_views_exact{false};
  bool time_masks_exact{false};
  bool jitter_masks_exact{false};
  bool jitter_values_exact_on_retained_support{false};
  bool weak_views_finite_and_zero_masked{false};
  bool current_default_output_exact{false};
  bool current_default_loss_exact{false};
  bool current_default_gradients_exact{false};
  bool current_default_rng_exact{false};
  bool pass{false};
};

struct VvaReceipt {
  int64_t steps{0};
  int64_t adam_steps{0};
  int64_t ema_steps{0};
  int64_t clipping_count{0};
  double minimum_loss{std::numeric_limits<double>::infinity()};
  double maximum_loss{0.0};
  double minimum_gradient_norm{std::numeric_limits<double>::infinity()};
  double maximum_gradient_norm{0.0};
  double minimum_served_update_norm{std::numeric_limits<double>::infinity()};
  double maximum_served_update_norm{0.0};
  double all_trainable_delta{0.0};
  double served_delta{0.0};
  double predictor_delta{0.0};
  double mae_decoder_delta{0.0};
  double vicreg_head_delta{0.0};
  double target_ema_delta{0.0};
  uint64_t row_schedule_hash{0xcbf29ce484222325ULL};
  uint64_t rng_schedule_hash{0xcbf29ce484222325ULL};
  bool finite{true};
  bool row_schedule_exact{true};
  bool rng_schedule_exact{true};
  bool jepa_masks_exact{true};
  bool view_semantics_exact{true};
  bool expected_partitions{false};
  bool pass{false};
};

struct VvaSeedTraining {
  std::vector<mtf::MtfJepaMaeVicreg> models{};
  std::array<VvaReceipt, 3> receipts{};
  std::array<bool, 3> initialization_exact{};
  bool anchor_metadata_exact{false};
  bool pass{false};
};

using VvaEvaluations = std::array<std::array<RmcEvaluation, 4>, 3>;
using VvaBootstrapAreaTable =
    std::vector<std::array<std::array<double, 4>, 3>>;

struct VvaWeightedContrast {
  rmc_gate::Contrast summary{};
  std::array<double, 3> per_seed{};
  std::array<double, kFamilies> family{};
};

enum class VvaClassification {
  invalid_numeric_or_mechanics,
  representation_rescue,
  objective_made_safe,
  view_augmentation_mitigates_harm_only,
  view_augmentation_not_causal_at_quality_boundary,
};

[[nodiscard]] const char *vva_classification_name(VvaClassification value) {
  switch (value) {
  case VvaClassification::invalid_numeric_or_mechanics:
    return "invalid_numeric_or_mechanics";
  case VvaClassification::representation_rescue:
    return "representation_rescue";
  case VvaClassification::objective_made_safe:
    return "objective_made_safe";
  case VvaClassification::view_augmentation_mitigates_harm_only:
    return "view_augmentation_mitigates_harm_only";
  case VvaClassification::view_augmentation_not_causal_at_quality_boundary:
    return "view_augmentation_not_causal_at_quality_boundary";
  }
  return "invalid_numeric_or_mechanics";
}

[[nodiscard]] bool vva_contains(const std::string &text,
                                std::string_view value) {
  return text.find(value) != std::string::npos;
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t
vva_profile_config(const torch::Device &device, std::size_t profile) {
  if (profile >= kVvaProfileNames.size()) {
    throw std::runtime_error("VVA profile id is invalid");
  }
  auto config = attribution_config(device, oca_arm(8U));
  config.vicreg_view_time_dropout_scale = (profile & 0x1U) != 0U ? 0.10 : 0.0;
  config.vicreg_view_gaussian_jitter_std =
      (profile & 0x2U) != 0U ? 0.005 : 0.0;
  return config;
}

[[nodiscard]] std::string vva_common_manifest(std::string manifest) {
  std::istringstream input(manifest);
  std::ostringstream output;
  std::string line;
  while (std::getline(input, line)) {
    if (line.rfind("vicreg_view_gaussian_jitter_std=", 0) == 0 ||
        line.rfind("vicreg_view_time_dropout_scale=", 0) == 0) {
      continue;
    }
    output << line << '\n';
  }
  return output.str();
}

[[nodiscard]] VvaCustody vva_validate_custody() {
  VvaCustody result{};
  const auto protocol = rmc_read_file(std::filesystem::path(kVvaProtocolPath));
  result.protocol = digest::sha256_hex(protocol) == kVvaProtocolSha256;
  const auto oca_log = rmc_read_file(std::filesystem::path(kVvaOcaLogPath));
  result.oca_log = digest::sha256_hex(oca_log) == kVvaOcaLogSha256;
  result.oca_fields =
      vva_contains(oca_log, "outer_augmentation_calls=0") &&
      vva_contains(oca_log,
                   "oca1.verdict.vicreg=harmful_at_certified_boundary") &&
      vva_contains(oca_log, "execution_status=oca1_measurements_complete");
  const auto oaa = rmc_read_file(std::filesystem::path(kVvaOaaFindingsPath));
  result.oaa_findings = digest::sha256_hex(oaa) == kVvaOaaFindingsSha256;
  result.oaa_fields =
      vva_contains(oaa, "VICReg paired-view dropout and paired-view jitter") &&
      vva_contains(oaa, "did not rescue VICReg");
  const auto ima1 = rmc_read_file(std::filesystem::path(kVvaIma1FindingsPath));
  result.ima1_findings = digest::sha256_hex(ima1) == kVvaIma1FindingsSha256;
  result.ima1_fields =
      vva_contains(ima1, "V0`, current independent weak views") &&
      vva_contains(ima1, "V1`, tied weak view") &&
      vva_contains(ima1, "V2`, clean identical view") &&
      vva_contains(ima1, "training triad was correctly withheld");
  result.module =
      digest::sha256_hex(rmc_read_file(std::filesystem::path(kVvaModulePath))) ==
      kVvaModuleSha256;
  result.archives = true;
  result.current_caches = true;
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    result.archives =
        result.archives &&
        digest::sha256_hex(rmc_read_file(oca_archive_path(
            kAttributionSeeds[seed]))) == kVvaAnchorSha256[seed];
    result.current_caches =
        result.current_caches &&
        digest::sha256_hex(rmc_read_file(oca_seed_cache_path(
            "anchor_challenge", kAttributionSeeds[seed]))) ==
            kVvaCurrentCacheSha256[seed];
  }
  result.pass = result.protocol && result.oca_log && result.oca_fields &&
                result.oaa_findings && result.oaa_fields &&
                result.ima1_findings && result.ima1_fields && result.module &&
                result.archives && result.current_caches;
  return result;
}

[[nodiscard]] bool vva_zero_masked(const torch::Tensor &data,
                                   const torch::Tensor &mask) {
  return torch::isfinite(data).all().item<bool>() &&
         torch::eq(data.masked_select(mask.logical_not()), 0).all().item<bool>();
}

[[nodiscard]] bool
vva_outputs_exact(const mtf::mtf_jepa_mae_vicreg_output_t &left,
                  const mtf::mtf_jepa_mae_vicreg_output_t &right) {
  return torch::equal(left.embeddings, right.embeddings) &&
         torch::equal(left.pooled_by_channel, right.pooled_by_channel) &&
         torch::equal(left.loss_jepa, right.loss_jepa) &&
         torch::equal(left.loss_mae, right.loss_mae) &&
         torch::equal(left.loss_tf_align, right.loss_tf_align) &&
         torch::equal(left.loss_vicreg, right.loss_vicreg) &&
         torch::equal(left.jepa_target_mask, right.jepa_target_mask) &&
         torch::equal(left.jepa_context_mask, right.jepa_context_mask) &&
         torch::equal(left.vicreg_view_a_data, right.vicreg_view_a_data) &&
         torch::equal(left.vicreg_view_a_feature_mask,
                      right.vicreg_view_a_feature_mask) &&
         torch::equal(left.vicreg_view_b_data, right.vicreg_view_b_data) &&
         torch::equal(left.vicreg_view_b_feature_mask,
                      right.vicreg_view_b_feature_mask);
}

[[nodiscard]] VvaStage0 vva_run_stage0(const RmcData &data,
                                       const torch::Device &device,
                                       const VvaCustody &custody) {
  VvaStage0 result{};
  result.custody = custody.pass;
  result.profile_inventory = kVvaProfileNames.size() == 4;
  const auto default_config = attribution_config(device, oca_arm(8U));
  const auto default_manifest = canonical_config_manifest(default_config);
  const auto common_manifest = vva_common_manifest(default_manifest);
  result.current_manifest_exact =
      canonical_config_manifest(vva_profile_config(device, 3)) ==
      default_manifest;
  result.only_two_scalars_differ = true;
  for (std::size_t profile = 0; profile < 4; ++profile) {
    result.only_two_scalars_differ =
        result.only_two_scalars_differ &&
        vva_common_manifest(canonical_config_manifest(
            vva_profile_config(device, profile))) == common_manifest;
  }
  result.feature_dropout_inactive = default_config.mask_ratio_channel == 0.0;

  const int64_t seed = kAttributionSeeds.front();
  const auto rows = training_rows(data.ssl, seed, 0);
  const auto indices = torch::tensor(rows, torch::kInt64);
  const auto input = data.ssl.data.index_select(0, indices).to(device);
  const auto feature_mask = data.ssl.mask.index_select(0, indices).to(device);
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));

  std::vector<mtf::MtfJepaMaeVicreg> models;
  models.reserve(4);
  std::array<mtf::mtf_jepa_mae_vicreg_output_t, 4> outputs{};
  std::array<GeneratorStateSnapshot, 4> post{};
  ParameterSnapshot canonical_initial{};
  result.initialization_exact = true;
  for (std::size_t profile = 0; profile < 4; ++profile) {
    set_paired_rng(seed, device);
    auto model =
        mtf::MtfJepaMaeVicreg(vva_profile_config(device, profile));
    result.initialization_exact =
        result.initialization_exact &&
        oca_load_archive(oca_archive_path(seed), model, device, seed,
                         anchor_hash);
    if (profile == 0) {
      canonical_initial = snapshot_parameters(model);
    } else {
      result.initialization_exact =
          result.initialization_exact &&
          parameter_max_abs_diff(model, canonical_initial) == 0.0;
    }
    model->train();
    set_paired_rng(paired_step_seed(seed, 0), device);
    {
      torch::NoGradGuard no_grad;
      outputs[profile] = model->forward(input, feature_mask);
    }
    post[profile] = current_generator_state_snapshot(device);
    validate_weak_view_debug_tensors(outputs[profile], input, feature_mask);
    models.push_back(model);
  }
  result.rng_schedule_exact = true;
  result.jepa_masks_exact = true;
  for (std::size_t profile = 1; profile < 4; ++profile) {
    result.rng_schedule_exact =
        result.rng_schedule_exact &&
        generator_state_snapshot_equal(post[0], post[profile]);
    result.jepa_masks_exact =
        result.jepa_masks_exact &&
        torch::equal(outputs[0].jepa_target_mask,
                     outputs[profile].jepa_target_mask) &&
        torch::equal(outputs[0].jepa_context_mask,
                     outputs[profile].jepa_context_mask);
  }
  const auto canonical_input = torch::where(
      feature_mask, input, torch::zeros_like(input));
  result.clean_views_exact =
      torch::equal(outputs[0].vicreg_view_a_feature_mask, feature_mask) &&
      torch::equal(outputs[0].vicreg_view_b_feature_mask, feature_mask) &&
      torch::equal(outputs[0].vicreg_view_a_data, canonical_input) &&
      torch::equal(outputs[0].vicreg_view_b_data, canonical_input) &&
      torch::equal(outputs[0].vicreg_view_a_data,
                   outputs[0].vicreg_view_b_data);
  result.time_masks_exact =
      torch::equal(outputs[1].vicreg_view_a_feature_mask,
                   outputs[3].vicreg_view_a_feature_mask) &&
      torch::equal(outputs[1].vicreg_view_b_feature_mask,
                   outputs[3].vicreg_view_b_feature_mask) &&
      (!torch::equal(outputs[1].vicreg_view_a_feature_mask, feature_mask) ||
       !torch::equal(outputs[1].vicreg_view_b_feature_mask, feature_mask));
  result.jitter_masks_exact =
      torch::equal(outputs[2].vicreg_view_a_feature_mask, feature_mask) &&
      torch::equal(outputs[2].vicreg_view_b_feature_mask, feature_mask) &&
      (!torch::equal(outputs[2].vicreg_view_a_data, canonical_input) ||
       !torch::equal(outputs[2].vicreg_view_b_data, canonical_input));
  const auto retained_a = outputs[3].vicreg_view_a_feature_mask;
  const auto retained_b = outputs[3].vicreg_view_b_feature_mask;
  result.jitter_values_exact_on_retained_support =
      torch::equal(outputs[2].vicreg_view_a_data.masked_select(retained_a),
                   outputs[3].vicreg_view_a_data.masked_select(retained_a)) &&
      torch::equal(outputs[2].vicreg_view_b_data.masked_select(retained_b),
                   outputs[3].vicreg_view_b_data.masked_select(retained_b));
  result.weak_views_finite_and_zero_masked = true;
  for (const auto &output : outputs) {
    result.weak_views_finite_and_zero_masked =
        result.weak_views_finite_and_zero_masked &&
        vva_zero_masked(output.vicreg_view_a_data,
                        output.vicreg_view_a_feature_mask) &&
        vva_zero_masked(output.vicreg_view_b_data,
                        output.vicreg_view_b_feature_mask);
  }

  set_paired_rng(seed, device);
  auto default_model = mtf::MtfJepaMaeVicreg(default_config);
  result.initialization_exact =
      result.initialization_exact &&
      oca_load_archive(oca_archive_path(seed), default_model, device, seed,
                       anchor_hash) &&
      parameter_max_abs_diff(default_model, canonical_initial) == 0.0;
  default_model->train();
  for (auto &parameter : models[3]->parameters()) {
    if (parameter.grad().defined()) {
      parameter.grad().zero_();
    }
  }
  for (auto &parameter : default_model->parameters()) {
    if (parameter.grad().defined()) {
      parameter.grad().zero_();
    }
  }
  const auto arm = oca_arm(8U);
  const auto weights = attribution_arm_weights(arm, 0);
  set_paired_rng(paired_step_seed(seed, 0), device);
  const auto current_output = models[3]->forward(input, feature_mask);
  const auto current_loss = attribution_arm_loss(current_output, arm, weights);
  current_loss.backward();
  const auto current_gradient =
      gradient_vector(models[3], GradientPartition::all_trainable);
  const auto current_post = current_generator_state_snapshot(device);
  set_paired_rng(paired_step_seed(seed, 0), device);
  const auto default_output = default_model->forward(input, feature_mask);
  const auto default_loss = attribution_arm_loss(default_output, arm, weights);
  default_loss.backward();
  const auto default_gradient =
      gradient_vector(default_model, GradientPartition::all_trainable);
  const auto default_post = current_generator_state_snapshot(device);
  result.current_default_output_exact =
      vva_outputs_exact(current_output, default_output);
  result.current_default_loss_exact = torch::equal(current_loss, default_loss);
  result.current_default_gradients_exact =
      torch::equal(current_gradient, default_gradient);
  result.current_default_rng_exact =
      generator_state_snapshot_equal(current_post, default_post);
  result.pass =
      result.custody && result.profile_inventory &&
      result.only_two_scalars_differ && result.current_manifest_exact &&
      result.initialization_exact && result.feature_dropout_inactive &&
      result.rng_schedule_exact && result.jepa_masks_exact &&
      result.clean_views_exact && result.time_masks_exact &&
      result.jitter_masks_exact &&
      result.jitter_values_exact_on_retained_support &&
      result.weak_views_finite_and_zero_masked &&
      result.current_default_output_exact &&
      result.current_default_loss_exact &&
      result.current_default_gradients_exact &&
      result.current_default_rng_exact;
  return result;
}

[[nodiscard]] bool
vva_training_view_semantics(const mtf::mtf_jepa_mae_vicreg_output_t &output,
                            std::size_t profile, const torch::Tensor &input,
                            const torch::Tensor &feature_mask) {
  const auto canonical =
      torch::where(feature_mask, input, torch::zeros_like(input));
  const bool finite_and_masked =
      vva_zero_masked(output.vicreg_view_a_data,
                      output.vicreg_view_a_feature_mask) &&
      vva_zero_masked(output.vicreg_view_b_data,
                      output.vicreg_view_b_feature_mask);
  if (!finite_and_masked) {
    return false;
  }
  if (profile == 0) {
    return torch::equal(output.vicreg_view_a_feature_mask, feature_mask) &&
           torch::equal(output.vicreg_view_b_feature_mask, feature_mask) &&
           torch::equal(output.vicreg_view_a_data, canonical) &&
           torch::equal(output.vicreg_view_b_data, canonical);
  }
  if (profile == 1) {
    return torch::equal(
               output.vicreg_view_a_data,
               torch::where(output.vicreg_view_a_feature_mask, canonical,
                            torch::zeros_like(canonical))) &&
           torch::equal(
               output.vicreg_view_b_data,
               torch::where(output.vicreg_view_b_feature_mask, canonical,
                            torch::zeros_like(canonical)));
  }
  if (profile == 2) {
    return torch::equal(output.vicreg_view_a_feature_mask, feature_mask) &&
           torch::equal(output.vicreg_view_b_feature_mask, feature_mask);
  }
  return false;
}

[[nodiscard]] VvaSeedTraining vva_train_seed(const Dataset &ssl,
                                             const torch::Device &device,
                                             int64_t seed) {
  VvaSeedTraining result{};
  result.initialization_exact.fill(true);
  result.models.reserve(3);
  std::vector<std::unique_ptr<torch::optim::Adam>> optimizers;
  optimizers.reserve(3);
  std::array<ParameterSnapshot, 3> initial{};
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  ParameterSnapshot canonical_initial{};
  result.anchor_metadata_exact = true;
  for (std::size_t profile = 0; profile < 3; ++profile) {
    set_paired_rng(seed, device);
    auto model =
        mtf::MtfJepaMaeVicreg(vva_profile_config(device, profile));
    result.anchor_metadata_exact =
        result.anchor_metadata_exact &&
        oca_load_archive(oca_archive_path(seed), model, device, seed,
                         anchor_hash);
    initial[profile] = snapshot_parameters(model);
    if (profile == 0) {
      canonical_initial = initial[profile];
    } else {
      result.initialization_exact[profile] =
          parameter_max_abs_diff(model, canonical_initial) == 0.0;
    }
    model->train();
    result.models.push_back(model);
    optimizers.push_back(std::make_unique<torch::optim::Adam>(
        result.models.back()->parameters(),
        torch::optim::AdamOptions(kVvaLearningRate)));
    result.receipts[profile].steps = kVvaSteps;
  }

  const auto arm = oca_arm(8U);
  for (int64_t step = 0; step < kVvaSteps; ++step) {
    const auto rows = training_rows(ssl, seed, step);
    const uint64_t row_hash = hash_batch_rows(rows);
    const auto indices = torch::tensor(rows, torch::kInt64);
    const auto input = ssl.data.index_select(0, indices).to(device);
    const auto feature_mask = ssl.mask.index_select(0, indices).to(device);
    std::array<mtf::mtf_jepa_mae_vicreg_output_t, 3> outputs{};
    std::array<GeneratorStateSnapshot, 3> pre{};
    std::array<GeneratorStateSnapshot, 3> post{};

    for (std::size_t profile = 0; profile < 3; ++profile) {
      auto &receipt = result.receipts[profile];
      mix_hash_value(receipt.row_schedule_hash, row_hash);
      set_paired_rng(paired_step_seed(seed, step), device);
      pre[profile] = current_generator_state_snapshot(device);
      optimizers[profile]->zero_grad();
      outputs[profile] = result.models[profile]->forward(input, feature_mask);
      post[profile] = current_generator_state_snapshot(device);
      validate_weak_view_debug_tensors(outputs[profile], input, feature_mask);
      validate_stratified_vicreg_forward(outputs[profile], arm,
                                         kModelRowBatchSize);
      receipt.view_semantics_exact =
          receipt.view_semantics_exact &&
          vva_training_view_semantics(outputs[profile], profile, input,
                                      feature_mask);
      mix_hash_value(receipt.rng_schedule_hash, post[profile].digest.cpu);
      mix_hash_value(receipt.rng_schedule_hash, post[profile].digest.cuda);
    }

    for (std::size_t profile = 1; profile < 3; ++profile) {
      result.receipts[profile].row_schedule_exact =
          result.receipts[profile].row_schedule_exact &&
          result.receipts[profile].row_schedule_hash ==
              result.receipts[0].row_schedule_hash;
      const bool rng_exact =
          generator_state_snapshot_equal(pre[0], pre[profile]) &&
          generator_state_snapshot_equal(post[0], post[profile]);
      result.receipts[0].rng_schedule_exact =
          result.receipts[0].rng_schedule_exact && rng_exact;
      result.receipts[profile].rng_schedule_exact =
          result.receipts[profile].rng_schedule_exact && rng_exact;
      const bool masks_exact =
          torch::equal(outputs[0].jepa_target_mask,
                       outputs[profile].jepa_target_mask) &&
          torch::equal(outputs[0].jepa_context_mask,
                       outputs[profile].jepa_context_mask);
      result.receipts[0].jepa_masks_exact =
          result.receipts[0].jepa_masks_exact && masks_exact;
      result.receipts[profile].jepa_masks_exact =
          result.receipts[profile].jepa_masks_exact && masks_exact;
    }

    const auto weights = attribution_arm_weights(arm, step);
    for (std::size_t profile = 0; profile < 3; ++profile) {
      auto &model = result.models[profile];
      auto &receipt = result.receipts[profile];
      const auto loss = attribution_arm_loss(outputs[profile], arm, weights);
      const double loss_value = loss.item<double>();
      receipt.minimum_loss = std::min(receipt.minimum_loss, loss_value);
      receipt.maximum_loss = std::max(receipt.maximum_loss, loss_value);
      loss.backward();
      auto gradient_square =
          torch::zeros({}, torch::TensorOptions().device(device));
      for (const auto &parameter : model->parameters()) {
        if (parameter.grad().defined()) {
          gradient_square = gradient_square +
                            parameter.grad().detach().pow(2).sum();
        }
      }
      const double gradient_norm = gradient_square.sqrt().item<double>();
      receipt.minimum_gradient_norm =
          std::min(receipt.minimum_gradient_norm, gradient_norm);
      receipt.maximum_gradient_norm =
          std::max(receipt.maximum_gradient_norm, gradient_norm);
      const double clip_factor =
          gradient_norm > kVvaClipNorm
              ? kVvaClipNorm / std::max(1.0e-30, gradient_norm)
              : 1.0;
      if (clip_factor < 1.0) {
        ++receipt.clipping_count;
        for (auto &parameter : model->parameters()) {
          if (parameter.grad().defined()) {
            parameter.grad().mul_(clip_factor);
          }
        }
      }
      const auto served_parameters = gpwd_served_parameters(model);
      std::vector<torch::Tensor> served_before;
      served_before.reserve(served_parameters.size());
      for (const auto &parameter : served_parameters) {
        served_before.push_back(parameter.detach().clone());
      }
      optimizers[profile]->step();
      ++receipt.adam_steps;
      auto update_square =
          torch::zeros({}, torch::TensorOptions().device(device));
      for (std::size_t parameter = 0; parameter < served_parameters.size();
           ++parameter) {
        update_square =
            update_square +
            (served_parameters[parameter].detach() - served_before[parameter])
                .pow(2)
                .sum();
      }
      const double update_norm = update_square.sqrt().item<double>();
      receipt.minimum_served_update_norm =
          std::min(receipt.minimum_served_update_norm, update_norm);
      receipt.maximum_served_update_norm =
          std::max(receipt.maximum_served_update_norm, update_norm);
      model->update_target_network();
      ++receipt.ema_steps;
      receipt.finite =
          receipt.finite && std::isfinite(loss_value) && loss_value > 0.0 &&
          std::isfinite(gradient_norm) && gradient_norm > 0.0 &&
          std::isfinite(update_norm) && update_norm > 0.0 &&
          torch::isfinite(outputs[profile].embeddings).all().item<bool>() &&
          torch::isfinite(outputs[profile].loss_vicreg).all().item<bool>();
    }
  }

  result.pass = result.anchor_metadata_exact;
  for (std::size_t profile = 0; profile < 3; ++profile) {
    auto &receipt = result.receipts[profile];
    auto &model = result.models[profile];
    receipt.all_trainable_delta = parameter_partition_max_abs_diff(
        model, initial[profile], ParameterDeltaPartition::all_trainable);
    receipt.served_delta = parameter_partition_max_abs_diff(
        model, initial[profile], ParameterDeltaPartition::served);
    receipt.predictor_delta = parameter_partition_max_abs_diff(
        model, initial[profile], ParameterDeltaPartition::predictor);
    receipt.mae_decoder_delta = parameter_partition_max_abs_diff(
        model, initial[profile], ParameterDeltaPartition::mae_decoder);
    receipt.vicreg_head_delta = parameter_partition_max_abs_diff(
        model, initial[profile], ParameterDeltaPartition::vicreg_head);
    receipt.target_ema_delta = parameter_partition_max_abs_diff(
        model, initial[profile], ParameterDeltaPartition::target_ema);
    receipt.expected_partitions =
        receipt.all_trainable_delta > 0.0 && receipt.served_delta > 0.0 &&
        receipt.predictor_delta == 0.0 && receipt.mae_decoder_delta == 0.0 &&
        receipt.vicreg_head_delta > 0.0 && receipt.target_ema_delta > 0.0;
    receipt.pass =
        receipt.finite && receipt.adam_steps == kVvaSteps &&
        receipt.ema_steps == kVvaSteps && receipt.minimum_loss > 0.0 &&
        receipt.minimum_gradient_norm > 0.0 &&
        receipt.minimum_served_update_norm > 0.0 &&
        receipt.row_schedule_exact && receipt.rng_schedule_exact &&
        receipt.jepa_masks_exact && receipt.view_semantics_exact &&
        receipt.expected_partitions;
    result.pass = result.pass && result.initialization_exact[profile] &&
                  receipt.pass;
  }
  return result;
}

void vva_emit_custody(const VvaCustody &value) {
  std::cout << "vva1.custody.protocol=" << value.protocol << '\n';
  std::cout << "vva1.custody.oca_log=" << value.oca_log << '\n';
  std::cout << "vva1.custody.oca_fields=" << value.oca_fields << '\n';
  std::cout << "vva1.custody.oaa_findings=" << value.oaa_findings << '\n';
  std::cout << "vva1.custody.oaa_fields=" << value.oaa_fields << '\n';
  std::cout << "vva1.custody.ima1_findings=" << value.ima1_findings << '\n';
  std::cout << "vva1.custody.ima1_fields=" << value.ima1_fields << '\n';
  std::cout << "vva1.custody.module=" << value.module << '\n';
  std::cout << "vva1.custody.archives=" << value.archives << '\n';
  std::cout << "vva1.custody.current_caches=" << value.current_caches << '\n';
  std::cout << "vva1.custody.pass=" << value.pass << '\n';
}

void vva_emit_stage0(const VvaStage0 &value) {
  std::cout << "vva1.stage0.custody=" << value.custody << '\n';
  std::cout << "vva1.stage0.profile_inventory=" << value.profile_inventory
            << '\n';
  std::cout << "vva1.stage0.only_two_scalars_differ="
            << value.only_two_scalars_differ << '\n';
  std::cout << "vva1.stage0.current_manifest_exact="
            << value.current_manifest_exact << '\n';
  std::cout << "vva1.stage0.initialization_exact="
            << value.initialization_exact << '\n';
  std::cout << "vva1.stage0.feature_dropout_inactive="
            << value.feature_dropout_inactive << '\n';
  std::cout << "vva1.stage0.rng_schedule_exact=" << value.rng_schedule_exact
            << '\n';
  std::cout << "vva1.stage0.jepa_masks_exact=" << value.jepa_masks_exact
            << '\n';
  std::cout << "vva1.stage0.clean_views_exact=" << value.clean_views_exact
            << '\n';
  std::cout << "vva1.stage0.time_masks_exact=" << value.time_masks_exact
            << '\n';
  std::cout << "vva1.stage0.jitter_masks_exact=" << value.jitter_masks_exact
            << '\n';
  std::cout << "vva1.stage0.jitter_values_exact_on_retained_support="
            << value.jitter_values_exact_on_retained_support << '\n';
  std::cout << "vva1.stage0.weak_views_finite_and_zero_masked="
            << value.weak_views_finite_and_zero_masked << '\n';
  std::cout << "vva1.stage0.current_default_output_exact="
            << value.current_default_output_exact << '\n';
  std::cout << "vva1.stage0.current_default_loss_exact="
            << value.current_default_loss_exact << '\n';
  std::cout << "vva1.stage0.current_default_gradients_exact="
            << value.current_default_gradients_exact << '\n';
  std::cout << "vva1.stage0.current_default_rng_exact="
            << value.current_default_rng_exact << '\n';
  std::cout << "vva1.stage0.pass=" << value.pass << '\n';
}

void vva_emit_receipt(const std::string &root, const VvaReceipt &value) {
  std::cout << root << ".adam_steps=" << value.adam_steps << '\n';
  std::cout << root << ".ema_steps=" << value.ema_steps << '\n';
  std::cout << root << ".clipping_count=" << value.clipping_count << '\n';
  std::cout << root << ".minimum_loss=" << value.minimum_loss << '\n';
  std::cout << root << ".maximum_loss=" << value.maximum_loss << '\n';
  std::cout << root << ".minimum_gradient_norm="
            << value.minimum_gradient_norm << '\n';
  std::cout << root << ".maximum_gradient_norm="
            << value.maximum_gradient_norm << '\n';
  std::cout << root << ".minimum_served_update_norm="
            << value.minimum_served_update_norm << '\n';
  std::cout << root << ".maximum_served_update_norm="
            << value.maximum_served_update_norm << '\n';
  std::cout << root << ".served_delta=" << value.served_delta << '\n';
  std::cout << root << ".predictor_delta=" << value.predictor_delta << '\n';
  std::cout << root << ".mae_decoder_delta=" << value.mae_decoder_delta
            << '\n';
  std::cout << root << ".vicreg_head_delta=" << value.vicreg_head_delta
            << '\n';
  std::cout << root << ".target_ema_delta=" << value.target_ema_delta
            << '\n';
  std::cout << root << ".row_schedule_hash="
            << oca_hex_u64(value.row_schedule_hash) << '\n';
  std::cout << root << ".rng_schedule_hash="
            << oca_hex_u64(value.rng_schedule_hash) << '\n';
  std::cout << root << ".finite=" << value.finite << '\n';
  std::cout << root << ".row_schedule_exact=" << value.row_schedule_exact
            << '\n';
  std::cout << root << ".rng_schedule_exact=" << value.rng_schedule_exact
            << '\n';
  std::cout << root << ".jepa_masks_exact=" << value.jepa_masks_exact
            << '\n';
  std::cout << root << ".view_semantics_exact="
            << value.view_semantics_exact << '\n';
  std::cout << root << ".expected_partitions=" << value.expected_partitions
            << '\n';
  std::cout << root << ".pass=" << value.pass << '\n';
}

[[nodiscard]] RmcEvaluation vva_evaluate(mtf::MtfJepaMaeVicreg &model,
                                         const RmcData &data,
                                         const RmcEvalTargets &targets,
                                         const torch::Device &device,
                                         bool confirmation) {
  const auto &evaluation = confirmation ? data.confirmation : data.development;
  const auto &reversed = confirmation ? data.reversed_confirmation
                                      : data.reversed_development;
  return rmc_evaluate(model, data.probe_train, data.probe_validation,
                      evaluation, data.reversed_train, data.reversed_validation,
                      reversed, targets, device);
}

[[nodiscard]] VvaBootstrapAreaTable
vva_bootstrap_area_table(const VvaEvaluations &evaluations,
                         const torch::Tensor &target,
                         const std::vector<torch::Tensor> &bootstrap_rows) {
  VvaBootstrapAreaTable result;
  result.reserve(bootstrap_rows.size());
  for (const auto &rows : bootstrap_rows) {
    std::array<std::array<double, 4>, 3> replicate{};
    for (std::size_t seed = 0; seed < 3; ++seed) {
      for (std::size_t profile = 0; profile < 4; ++profile) {
        replicate[seed][profile] =
            rssm_resampled_area(evaluations[seed][profile].probe, target, rows)
                .macro;
      }
    }
    result.push_back(std::move(replicate));
  }
  return result;
}

[[nodiscard]] VvaWeightedContrast
vva_weighted_contrast(const VvaEvaluations &evaluations,
                      const VvaBootstrapAreaTable &bootstrap,
                      const std::array<double, 4> &weights) {
  VvaWeightedContrast result{};
  for (std::size_t seed = 0; seed < 3; ++seed) {
    for (std::size_t profile = 0; profile < 4; ++profile) {
      result.per_seed[seed] +=
          weights[profile] * evaluations[seed][profile].probe.area;
      const auto family = rssm_family_areas(evaluations[seed][profile].probe);
      for (std::size_t index = 0; index < kFamilies; ++index) {
        result.family[index] += weights[profile] * family[index] / 3.0;
      }
    }
    result.summary.point += result.per_seed[seed] / 3.0;
    result.summary.positive_seed_count += result.per_seed[seed] > 0.0 ? 1 : 0;
  }
  std::vector<double> replicates;
  replicates.reserve(bootstrap.size());
  for (const auto &table : bootstrap) {
    double value = 0.0;
    for (std::size_t seed = 0; seed < 3; ++seed) {
      for (std::size_t profile = 0; profile < 4; ++profile) {
        value += weights[profile] * table[seed][profile] / 3.0;
      }
    }
    replicates.push_back(value);
  }
  const auto interval = percentile_interval(std::move(replicates));
  result.summary.low = interval.low;
  result.summary.high = interval.high;
  return result;
}

[[nodiscard]] bool vva_family_floor_pass(const VvaWeightedContrast &value) {
  return std::all_of(value.family.begin(), value.family.end(),
                     [](double effect) {
                       return std::isfinite(effect) &&
                              effect >= kVvaFamilyFloor;
                     });
}

[[nodiscard]] bool vva_contrast_meets_causal_gate(
    const VvaWeightedContrast &value, bool no_new_safeguard_failure) {
  return std::isfinite(value.summary.point) &&
         std::isfinite(value.summary.low) &&
         std::isfinite(value.summary.high) &&
         value.summary.point >= kVvaCausalFloor && value.summary.low > 0.0 &&
         value.summary.positive_seed_count == 3 &&
         vva_family_floor_pass(value) && no_new_safeguard_failure;
}

[[nodiscard]] bool vva_summary_safeguards_pass(const RmcSummary &summary) {
  const auto &gate = summary.gate.neutral;
  return gate.numeric_valid && gate.mechanics_pass &&
         gate.family_floor_pass && gate.raw_noninferiority_pass &&
         gate.order_point_pass && gate.order_lower_pass &&
         gate.order_retention_pass && gate.continuous_shuffle_pass &&
         gate.order_shuffle_pass && gate.geometry_pass;
}

[[nodiscard]] bool vva_reproducible_new_safeguard_failure(
    const std::array<RmcEvaluation, 3> &reference,
    const std::array<RmcEvaluation, 3> &candidate,
    const std::array<RmcEvaluation, 3> &anchor, double raw_area) {
  bool reference_passes_all = true;
  bool candidate_fails_all = true;
  for (std::size_t seed = 0; seed < 3; ++seed) {
    reference_passes_all =
        reference_passes_all &&
        oca_seed_safeguards_pass(reference[seed], anchor[seed], raw_area);
    candidate_fails_all =
        candidate_fails_all &&
        !oca_seed_safeguards_pass(candidate[seed], anchor[seed], raw_area);
  }
  return reference_passes_all && candidate_fails_all;
}

void vva_emit_weighted_contrast(const std::string &root,
                                const VvaWeightedContrast &value) {
  rmc_emit_contrast(root, value.summary);
  for (std::size_t seed = 0; seed < 3; ++seed) {
    std::cout << root << ".seed_" << kAttributionSeeds[seed] << '='
              << value.per_seed[seed] << '\n';
  }
  for (std::size_t family = 0; family < kFamilies; ++family) {
    std::cout << root << ".family_" << kFamilyNames[family] << '='
              << value.family[family] << '\n';
  }
}

[[nodiscard]] std::array<RmcEvaluation, 3>
vva_profile_evaluations(const VvaEvaluations &evaluations,
                        std::size_t profile) {
  std::array<RmcEvaluation, 3> result{};
  for (std::size_t seed = 0; seed < 3; ++seed) {
    result[seed] = evaluations[seed][profile];
  }
  return result;
}

[[nodiscard]] VvaClassification vva_classify_candidate(
    const VvaWeightedContrast &candidate_minus_current,
    const RmcSummary &candidate_minus_anchor, bool mechanics,
    bool no_new_safeguard_failure) {
  if (!mechanics || !std::isfinite(candidate_minus_current.summary.point) ||
      !candidate_minus_anchor.gate.neutral.numeric_valid ||
      !candidate_minus_anchor.gate.neutral.mechanics_pass) {
    return VvaClassification::invalid_numeric_or_mechanics;
  }
  if (!vva_contrast_meets_causal_gate(candidate_minus_current,
                                      no_new_safeguard_failure)) {
    return VvaClassification::view_augmentation_not_causal_at_quality_boundary;
  }
  const auto &anchor =
      candidate_minus_anchor.candidate[0].gate.trained_minus_initialization;
  const auto &family =
      candidate_minus_anchor.candidate[0].gate.learned_family_deltas;
  std::size_t positive_families = 0;
  bool family_floors = true;
  for (const double value : family) {
    positive_families += value > 0.0 ? 1U : 0U;
    family_floors = family_floors && value >= kVvaFamilyFloor;
  }
  const bool safeguards = vva_summary_safeguards_pass(candidate_minus_anchor);
  const bool rescue =
      anchor.point >= kVvaRescueFloor && anchor.low > 0.0 &&
      anchor.positive_seed_count == 3 && positive_families >= 3 &&
      family_floors && safeguards;
  if (rescue) {
    return VvaClassification::representation_rescue;
  }
  const bool safe = anchor.low > kVvaAnchorNoninferiority && family_floors &&
                    safeguards;
  return safe ? VvaClassification::objective_made_safe
              : VvaClassification::view_augmentation_mitigates_harm_only;
}

void vva_open_confirmation(RmcData &data) {
  if (data.confirmation.data.defined()) {
    throw std::runtime_error("VVA confirmation was opened more than once");
  }
  data.confirmation =
      generate_dataset(kVvaConfirmationGroupBegin, kVvaConfirmationRows);
  const auto raw_projection = make_raw_equal_width_projection();
  data.raw_confirmation =
      raw_equal_width_features(data.confirmation, raw_projection);
  normalize(data.confirmation, data.normalization);
  validate_dataset(data.confirmation);
  data.reversed_confirmation = rssm_reversed_dataset(data.confirmation);
}

[[nodiscard]] bool vva_options_valid(const Options &options) {
  return options.device == "cuda" &&
         (options.steps < 0 || options.steps == kVvaSteps) &&
         (options.seeds < 0 ||
          options.seeds == static_cast<int64_t>(kAttributionSeeds.size())) &&
         options.weak_views;
}

void vva_require_options(const Options &options) {
  if (!vva_options_valid(options)) {
    throw std::runtime_error(
        "VVA-1 requires CUDA, 512 updates per new arm, 3 seeds, and weak "
        "views enabled");
  }
  rmc_configure_cuda();
}

int run_vva_preflight(const Options &options) {
  vva_require_options(options);
  const torch::Device device(torch::kCUDA, 0);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.vva1.preflight.v1\n";
  std::cout << "module_only=true\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "optimizer_updates=0\n";
  std::cout << "vva1.protocol.sha256=" << kVvaProtocolSha256 << '\n';
  auto data = rmc_make_data();
  const auto custody = vva_validate_custody();
  const auto stage0 = vva_run_stage0(data, device, custody);
  vva_emit_custody(custody);
  vva_emit_stage0(stage0);
  std::cout << "execution_status="
            << (stage0.pass ? "vva1_preflight_complete"
                            : "vva1_preflight_failed")
            << '\n';
  return stage0.pass ? 0 : 3;
}

void vva_emit_evaluation(const std::string &root,
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
        root + ".channel_" + std::to_string(channel) + ".geometry";
    std::cout << prefix << ".effective=" << geometry.effective_rank_ratio
              << '\n';
    std::cout << prefix << ".participation="
              << geometry.participation_rank_ratio << '\n';
    std::cout << prefix << ".top=" << geometry.top_eigenvalue_share << '\n';
    std::cout << prefix << ".active=" << geometry.active_dimension_fraction
              << '\n';
  }
}

[[nodiscard]] VvaWeightedContrast vva_pair_contrast(
    const std::array<RmcEvaluation, 3> &reference,
    const std::array<RmcEvaluation, 3> &candidate,
    const RmcSummary &summary) {
  VvaWeightedContrast result{};
  result.summary =
      summary.candidate[0].gate.trained_minus_initialization;
  result.family = summary.candidate[0].gate.learned_family_deltas;
  for (std::size_t seed = 0; seed < 3; ++seed) {
    result.per_seed[seed] =
        candidate[seed].probe.area - reference[seed].probe.area;
  }
  return result;
}

[[nodiscard]] std::size_t vva_active_factor_count(std::size_t profile) {
  return (profile & 0x1U ? 1U : 0U) + (profile & 0x2U ? 1U : 0U);
}

int run_vva_attribution(const Options &options) {
  vva_require_options(options);
  const torch::Device device(torch::kCUDA, 0);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.vva1.attribution.v1\n";
  std::cout << "experiment=vicreg-view-augmentation-causal-attribution\n";
  std::cout << "module_only=true\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "outer_augmentation_calls=0\n";
  std::cout << "readout_policy=" << kVvaReadoutPolicy << '\n';
  std::cout << "objective_mask=8\n";
  std::cout << "current_profile_retrained=false\n";
  std::cout << "new_parallel_arms=3\n";
  std::cout << "optimizer_updates_per_new_arm=" << kVvaSteps << '\n';
  std::cout << "new_model_updates=" << kVvaSteps * 3 * 3 << '\n';
  std::cout << "vva1.protocol.sha256=" << kVvaProtocolSha256 << '\n';

  auto data = rmc_make_data();
  const auto targets = rmc_make_targets(data, false);
  const auto bootstrap_rows = rmc_bootstrap_rows(256);
  if (!rmc_bootstrap_rows_valid(bootstrap_rows, 256)) {
    throw std::runtime_error("VVA bootstrap row table failed");
  }
  const auto raw = rssm_probe_curve(
      data.raw_train, data.raw_validation, data.raw_development,
      data.probe_train.target, data.probe_validation.target,
      data.development.target, /*dual=*/true);
  const auto custody = vva_validate_custody();
  const auto stage0 = vva_run_stage0(data, device, custody);
  vva_emit_custody(custody);
  vva_emit_stage0(stage0);
  if (!stage0.pass) {
    std::cout << "vva1.training.opened=false\n";
    std::cout << "execution_status=vva1_preoptimizer_gate_failed\n";
    return 3;
  }

  using SeedEvaluations = std::array<RmcEvaluation, 3>;
  SeedEvaluations anchor_evaluations{};
  SeedEvaluations current_evaluations{};
  std::vector<mtf::MtfJepaMaeVicreg> anchor_models;
  std::vector<mtf::MtfJepaMaeVicreg> current_models;
  anchor_models.reserve(3);
  current_models.reserve(3);
  std::array<VvaSeedTraining, 3> training{};
  VvaEvaluations evaluations{};
  std::array<bool, 4> profile_mechanics{true, true, true, true};
  bool baseline_replay_exact = true;
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  const std::vector<uint8_t> challenge_masks(kVvaChallengeMasks.begin(),
                                             kVvaChallengeMasks.end());

  // Resolve and evaluate every immutable baseline before opening training.
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    set_paired_rng(seed, device);
    auto anchor = mtf::MtfJepaMaeVicreg(anchor_config);
    const bool anchor_metadata =
        oca_load_archive(oca_archive_path(seed), anchor, device, seed,
                         anchor_hash);
    anchor_evaluations[seed_index] =
        vva_evaluate(anchor, data, targets, device, false);
    anchor_models.push_back(anchor);

    OcaInterleavedTrainingResult current_cache{};
    const bool current_loaded = oca_load_seed_cache(
        "anchor_challenge", data.ssl, device, seed, challenge_masks,
        kOcaAnchorChallengeSteps, /*load_certified_anchor=*/true,
        current_cache);
    const bool inventory =
        current_loaded && current_cache.models.size() == 5 &&
        current_cache.receipts.size() == 5 &&
        current_cache.initialization_exact.size() == 5;
    if (!inventory) {
      throw std::runtime_error("VVA current-cache inventory failed");
    }
    profile_mechanics[3] =
        profile_mechanics[3] && current_cache.metadata_exact &&
        current_cache.schedule_exact && current_cache.pass &&
        current_cache.initialization_exact[kVvaCurrentCachePosition] &&
        current_cache.receipts[kVvaCurrentCachePosition].pass;
    auto current = current_cache.models[kVvaCurrentCachePosition];
    current_evaluations[seed_index] =
        vva_evaluate(current, data, targets, device, false);
    current_models.push_back(current);
    evaluations[seed_index][3] = current_evaluations[seed_index];
    baseline_replay_exact =
        baseline_replay_exact && anchor_metadata && profile_mechanics[3];
    std::cout << "vva1.baseline.seed_" << seed
              << ".anchor_metadata_exact=" << anchor_metadata << '\n';
    std::cout << "vva1.baseline.seed_" << seed
              << ".current_cache_exact=" << profile_mechanics[3] << '\n';
  }
  std::cout << "vva1.baseline_replay_exact=" << baseline_replay_exact << '\n';
  if (!baseline_replay_exact) {
    std::cout << "vva1.training.opened=false\n";
    std::cout << "execution_status=vva1_baseline_replay_failed\n";
    return 3;
  }

  std::cout << "vva1.training.opened=true\n" << std::flush;
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    training[seed_index] = vva_train_seed(data.ssl, device, seed);
    for (std::size_t profile = 0; profile < 3; ++profile) {
      profile_mechanics[profile] =
          profile_mechanics[profile] && training[seed_index].pass &&
          training[seed_index].initialization_exact[profile] &&
          training[seed_index].receipts[profile].pass;
      evaluations[seed_index][profile] = vva_evaluate(
          training[seed_index].models[profile], data, targets, device, false);
      const std::string root =
          "vva1.training.seed_" + std::to_string(seed) + ".profile." +
          kVvaProfileNames[profile];
      vva_emit_receipt(root, training[seed_index].receipts[profile]);
      vva_emit_evaluation(root + ".clean", evaluations[seed_index][profile]);
      std::cout << root << ".complete=true\n" << std::flush;
    }
    vva_emit_evaluation(
        "vva1.training.seed_" + std::to_string(seed) + ".profile." +
            kVvaProfileNames[3] + ".clean",
        evaluations[seed_index][3]);
  }

  bool mechanics = stage0.pass;
  for (const bool value : profile_mechanics) {
    mechanics = mechanics && value;
  }
  const auto bootstrap = vva_bootstrap_area_table(
      evaluations, data.development.target, bootstrap_rows);
  constexpr std::array<double, 4> remove_time_weights{0.0, 0.0, 1.0, -1.0};
  constexpr std::array<double, 4> remove_jitter_weights{0.0, 1.0, 0.0, -1.0};
  constexpr std::array<double, 4> remove_both_weights{1.0, 0.0, 0.0, -1.0};
  constexpr std::array<double, 4> time_main_weights{0.5, -0.5, 0.5, -0.5};
  constexpr std::array<double, 4> jitter_main_weights{0.5, 0.5, -0.5, -0.5};
  constexpr std::array<double, 4> interaction_weights{1.0, -1.0, -1.0, 1.0};
  const auto remove_time =
      vva_weighted_contrast(evaluations, bootstrap, remove_time_weights);
  const auto remove_jitter =
      vva_weighted_contrast(evaluations, bootstrap, remove_jitter_weights);
  const auto remove_both =
      vva_weighted_contrast(evaluations, bootstrap, remove_both_weights);
  const auto time_main =
      vva_weighted_contrast(evaluations, bootstrap, time_main_weights);
  const auto jitter_main =
      vva_weighted_contrast(evaluations, bootstrap, jitter_main_weights);
  const auto interaction =
      vva_weighted_contrast(evaluations, bootstrap, interaction_weights);
  vva_emit_weighted_contrast("vva1.development.remove_time_from_current",
                             remove_time);
  vva_emit_weighted_contrast("vva1.development.remove_jitter_from_current",
                             remove_jitter);
  vva_emit_weighted_contrast("vva1.development.remove_both_from_current",
                             remove_both);
  vva_emit_weighted_contrast("vva1.development.main_effect.time_off",
                             time_main);
  vva_emit_weighted_contrast("vva1.development.main_effect.jitter_off",
                             jitter_main);
  vva_emit_weighted_contrast("vva1.development.interaction.time_x_jitter",
                             interaction);

  std::array<RmcSummary, 4> anchor_summaries{};
  std::array<RmcSummary, 3> current_summaries{};
  std::array<VvaWeightedContrast, 3> current_contrasts{};
  std::array<bool, 3> no_new_safeguard_failure{};
  const auto current_by_seed = vva_profile_evaluations(evaluations, 3);
  for (std::size_t profile = 0; profile < 4; ++profile) {
    const auto candidate = vva_profile_evaluations(evaluations, profile);
    anchor_summaries[profile] = oca_pair_summary(
        anchor_evaluations, candidate, raw, data.development.target, targets,
        bootstrap_rows, mechanics && profile_mechanics[profile]);
    oca_emit_candidate_summary(
        "vva1.development.profile." + std::string(kVvaProfileNames[profile]) +
            ".minus_anchor",
        anchor_summaries[profile]);
    if (profile < 3) {
      current_summaries[profile] = oca_pair_summary(
          current_by_seed, candidate, raw, data.development.target, targets,
          bootstrap_rows, mechanics && profile_mechanics[profile]);
      current_contrasts[profile] = vva_pair_contrast(
          current_by_seed, candidate, current_summaries[profile]);
      no_new_safeguard_failure[profile] =
          !vva_reproducible_new_safeguard_failure(
              current_by_seed, candidate, anchor_evaluations, raw.area);
      oca_emit_candidate_summary(
          "vva1.development.profile." +
              std::string(kVvaProfileNames[profile]) + ".minus_current",
          current_summaries[profile]);
      std::cout << "vva1.development.profile." << kVvaProfileNames[profile]
                << ".no_new_safeguard_failure="
                << no_new_safeguard_failure[profile] << '\n';
    }
  }

  const auto profile0 = vva_profile_evaluations(evaluations, 0);
  const auto profile1 = vva_profile_evaluations(evaluations, 1);
  const auto profile2 = vva_profile_evaluations(evaluations, 2);
  const bool time_no_new_failure =
      !vva_reproducible_new_safeguard_failure(
          profile1, profile0, anchor_evaluations, raw.area) &&
      !vva_reproducible_new_safeguard_failure(
          current_by_seed, profile2, anchor_evaluations, raw.area);
  const bool jitter_no_new_failure =
      !vva_reproducible_new_safeguard_failure(
          profile2, profile0, anchor_evaluations, raw.area) &&
      !vva_reproducible_new_safeguard_failure(
          current_by_seed, profile1, anchor_evaluations, raw.area);
  const bool time_main_pass =
      vva_contrast_meets_causal_gate(time_main, time_no_new_failure);
  const bool jitter_main_pass =
      vva_contrast_meets_causal_gate(jitter_main, jitter_no_new_failure);
  const bool remove_time_pass =
      vva_contrast_meets_causal_gate(remove_time,
                                     no_new_safeguard_failure[2]);
  const bool remove_jitter_pass =
      vva_contrast_meets_causal_gate(remove_jitter,
                                     no_new_safeguard_failure[1]);
  const bool remove_both_pass =
      vva_contrast_meets_causal_gate(remove_both,
                                     no_new_safeguard_failure[0]);
  std::cout << "vva1.development.main_effect.time_off.pass="
            << time_main_pass << '\n';
  std::cout << "vva1.development.main_effect.jitter_off.pass="
            << jitter_main_pass << '\n';
  std::cout << "vva1.development.remove_time_from_current.pass="
            << remove_time_pass << '\n';
  std::cout << "vva1.development.remove_jitter_from_current.pass="
            << remove_jitter_pass << '\n';
  std::cout << "vva1.development.remove_both_from_current.pass="
            << remove_both_pass << '\n';

  std::size_t selected = 4;
  for (std::size_t profile = 0; profile < 3; ++profile) {
    const bool eligible = vva_contrast_meets_causal_gate(
        current_contrasts[profile], no_new_safeguard_failure[profile]);
    std::cout << "vva1.development.profile." << kVvaProfileNames[profile]
              << ".eligible=" << eligible << '\n';
    if (!eligible) {
      continue;
    }
    if (selected == 4 ||
        current_contrasts[profile].summary.point >
            current_contrasts[selected].summary.point ||
        (current_contrasts[profile].summary.point ==
             current_contrasts[selected].summary.point &&
         (vva_active_factor_count(profile) <
              vva_active_factor_count(selected) ||
          (vva_active_factor_count(profile) ==
               vva_active_factor_count(selected) &&
           profile < selected)))) {
      selected = profile;
    }
  }

  VvaClassification development_classification =
      mechanics
          ? VvaClassification::view_augmentation_not_causal_at_quality_boundary
          : VvaClassification::invalid_numeric_or_mechanics;
  if (mechanics && selected < 3) {
    development_classification = vva_classify_candidate(
        current_contrasts[selected], anchor_summaries[selected], mechanics,
        no_new_safeguard_failure[selected]);
  }
  std::cout << "vva1.development.selected="
            << (selected < 3 ? kVvaProfileNames[selected] : "none") << '\n';
  std::cout << "vva1.development.classification="
            << vva_classification_name(development_classification) << '\n';
  std::cout << "vva1.development.mechanics_pass=" << mechanics << '\n';

  bool confirmation_opened = false;
  bool confirmation_pass = false;
  VvaClassification confirmation_classification =
      VvaClassification::view_augmentation_not_causal_at_quality_boundary;
  if (selected < 3 &&
      (development_classification == VvaClassification::representation_rescue ||
       development_classification == VvaClassification::objective_made_safe)) {
    confirmation_opened = true;
    vva_open_confirmation(data);
    const auto confirmation_targets = rmc_make_targets(data, true);
    const auto raw_confirmation = rssm_probe_curve(
        data.raw_train, data.raw_validation, data.raw_confirmation,
        data.probe_train.target, data.probe_validation.target,
        data.confirmation.target, /*dual=*/true);
    SeedEvaluations confirmation_anchor{};
    SeedEvaluations confirmation_current{};
    SeedEvaluations confirmation_candidate{};
    for (std::size_t seed = 0; seed < 3; ++seed) {
      confirmation_anchor[seed] = vva_evaluate(
          anchor_models[seed], data, confirmation_targets, device, true);
      confirmation_current[seed] = vva_evaluate(
          current_models[seed], data, confirmation_targets, device, true);
      confirmation_candidate[seed] = vva_evaluate(
          training[seed].models[selected], data, confirmation_targets, device,
          true);
    }
    const auto candidate_minus_current = oca_pair_summary(
        confirmation_current, confirmation_candidate, raw_confirmation,
        data.confirmation.target, confirmation_targets, bootstrap_rows,
        mechanics);
    const auto candidate_minus_anchor = oca_pair_summary(
        confirmation_anchor, confirmation_candidate, raw_confirmation,
        data.confirmation.target, confirmation_targets, bootstrap_rows,
        mechanics);
    const auto confirmation_contrast = vva_pair_contrast(
        confirmation_current, confirmation_candidate,
        candidate_minus_current);
    const bool no_new_failure = !vva_reproducible_new_safeguard_failure(
        confirmation_current, confirmation_candidate, confirmation_anchor,
        raw_confirmation.area);
    confirmation_classification = vva_classify_candidate(
        confirmation_contrast, candidate_minus_anchor, mechanics,
        no_new_failure);
    confirmation_pass =
        confirmation_classification == VvaClassification::representation_rescue ||
        (development_classification == VvaClassification::objective_made_safe &&
         confirmation_classification ==
             VvaClassification::objective_made_safe);
    oca_emit_candidate_summary("vva1.confirmation.candidate_minus_current",
                               candidate_minus_current);
    oca_emit_candidate_summary("vva1.confirmation.candidate_minus_anchor",
                               candidate_minus_anchor);
  }
  std::cout << "vva1.confirmation.opened=" << confirmation_opened << '\n';
  std::cout << "vva1.confirmation.group_begin="
            << (confirmation_opened ? kVvaConfirmationGroupBegin : -1) << '\n';
  std::cout << "vva1.confirmation.rows="
            << (confirmation_opened ? kVvaConfirmationRows : 0) << '\n';
  std::cout << "vva1.confirmation.classification="
            << (confirmation_opened
                    ? vva_classification_name(confirmation_classification)
                    : "not_opened")
            << '\n';
  std::cout << "vva1.confirmation.pass=" << confirmation_pass << '\n';
  std::cout << "vva1.promotion="
            << (confirmation_pass && selected < 3 ? kVvaProfileNames[selected]
                                                   : "none")
            << '\n';
  std::cout << "vva1.time_dropout_causal_harm="
            << (time_main_pass || remove_time_pass) << '\n';
  std::cout << "vva1.gaussian_jitter_causal_harm="
            << (jitter_main_pass || remove_jitter_pass) << '\n';
  std::cout << "vva1.paired_view_stack_causal_harm=" << remove_both_pass
            << '\n';
  std::cout << "vva1.production_defaults_changed=false\n";
  std::cout << "vva1.canonical_rollback="
            << "fspa4_minimal_spectral_repair_v1:structured_cdsb_sparse_v1\n";
  std::cout << "vva1.operational_rollback=all_tokens\n";
  std::cout << "vva1.rollback_preserved=true\n";
  std::cout << "vva1.versioned_view_recipe_followup_authorized="
            << confirmation_pass << '\n';
  std::cout << "vva1.global_pool_projector_variance_attribution_authorized="
            << (mechanics && !confirmation_pass) << '\n';
  std::cout << "execution_status="
            << (mechanics ? "vva1_measurements_complete"
                          : "vva1_measurements_invalid")
            << '\n';
  return mechanics ? 0 : 3;
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment ==
        "vicreg-view-augmentation-causal-attribution-preflight") {
      return run_vva_preflight(options);
    }
    if (options.experiment ==
        "vicreg-view-augmentation-causal-attribution") {
      return run_vva_attribution(options);
    }
    throw std::runtime_error(
        "--experiment must be "
        "vicreg-view-augmentation-causal-attribution-preflight or "
        "vicreg-view-augmentation-causal-attribution");
  } catch (const c10::Error &error) {
    std::cerr << "vicreg_view_augmentation_causal_attribution_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "vicreg_view_augmentation_causal_attribution_error="
              << error.what() << '\n';
  }
  return 2;
}
