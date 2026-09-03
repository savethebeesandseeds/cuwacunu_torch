#include "piaabo/digest/sha256.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <cuda_runtime_api.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

// GPV-1 is deliberately a test-only continuation of OCA-1.  Embedding OCA
// keeps the sealed FSPA-4 archives, OCA completed-cache reader, exact row/RNG
// schedule, and RMC clean evaluation in this single translation unit.
#define CUWACUNU_OCA_EMBEDDED
#include "quality_wikimyei_mtf_jepa_mae_vicreg_four_objective_causal_attribution.cpp"
#undef CUWACUNU_OCA_EMBEDDED

namespace {

class GpvExclusiveRunLock final {
public:
  explicit GpvExclusiveRunLock(const std::filesystem::path &path) {
    std::filesystem::create_directories(path.parent_path());
    descriptor_ = ::open(path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0660);
    if (descriptor_ < 0) {
      throw std::runtime_error("GPV exclusive lock open failed: " +
                               std::string(std::strerror(errno)));
    }
    if (::flock(descriptor_, LOCK_EX | LOCK_NB) != 0) {
      const auto message = std::string(std::strerror(errno));
      ::close(descriptor_);
      descriptor_ = -1;
      throw std::runtime_error("another GPV process holds the exclusive lock: " +
                               message);
    }
  }

  GpvExclusiveRunLock(const GpvExclusiveRunLock &) = delete;
  GpvExclusiveRunLock &operator=(const GpvExclusiveRunLock &) = delete;

  ~GpvExclusiveRunLock() {
    if (descriptor_ >= 0) {
      ::flock(descriptor_, LOCK_UN);
      ::close(descriptor_);
    }
  }

private:
  int descriptor_{-1};
};

constexpr std::string_view kGpvProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "GLOBAL_POOL_PROJECTOR_VARIANCE_CAUSAL_DECOMPOSITION_PROTOCOL.md";
constexpr std::string_view kGpvHarnessPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_"
    "causal_decomposition.cpp";
constexpr std::string_view kGpvProtocolSha256 =
    "01c6b1d9fcc95a0c831426a481c866cb196f413030d2f3c195b5219d84d57a2a";
constexpr std::string_view kGpvRecoveryPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "GLOBAL_POOL_PROJECTOR_VARIANCE_CAUSAL_DECOMPOSITION_CODEC_RECOVERY_V2.md";
constexpr std::string_view kGpvRecoveryMarkerPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "GLOBAL_POOL_PROJECTOR_VARIANCE_CAUSAL_DECOMPOSITION_CODEC_RECOVERY_V2."
    "sha256";
constexpr std::string_view kGpvRecoverySha256 =
    "d4989abcc4dc3f71b40bf0743c8e5ba43210165493595ca84347956d8768c2d4";
constexpr std::string_view kGpvLegacyV1CachePath =
    ".build/tests/gpv1/seed_17_mask_1_v1.complete.pt";
constexpr std::string_view kGpvLegacyV1CacheSha256 =
    "202e0cc06372624884673ebd821f3dd05eb04141f6b93c688431ef1b73182ffd";
constexpr std::string_view kGpvLegacyV1MarkerSha256 =
    "476c247ceffb2ddf500bce7d9f3cfd13c0d16adf6013bafd7af1c1fa120f4c79";
constexpr std::string_view kGpvVvaProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "VICREG_VIEW_AUGMENTATION_CAUSAL_ATTRIBUTION_PROTOCOL.md";
constexpr std::string_view kGpvVvaProtocolSha256 =
    "8e5f3e0aecf9990cb5adb54e090d781cc91b22c5880b8ae622c1fea10fa33616";
constexpr std::string_view kGpvVvaFindingsPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "VICREG_VIEW_AUGMENTATION_CAUSAL_ATTRIBUTION_FINDINGS.md";
constexpr std::string_view kGpvVvaFindingsSha256 =
    "8e651276f444bbafe4f534132245b48b718b16d98de0f31b62a21bec0b6851f0";
constexpr std::string_view kGpvVvaLogPath =
    ".build/tests/representation_vva1_v1_authoritative.log";
constexpr std::string_view kGpvVvaLogSha256 =
    "d73635a87d96f6d251a8a008b442657066893d3074194bf7f9de055ff61d9d33";
constexpr std::string_view kGpvVvaHarnessPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_view_augmentation_causal_"
    "attribution.cpp";
constexpr std::string_view kGpvVvaHarnessSha256 =
    "5b807c5dfd9bb371a40e1ee062c72f0754b817b91158d8cbdfe16ee3b9642d31";
constexpr std::string_view kGpvOcaLogPath =
    ".build/tests/oca1/oca1_full_run.log";
constexpr std::string_view kGpvOcaLogSha256 =
    "3ade025b37525c45d376eabaca2c31771ec7d23fbd2f32f26277cec0d9be606d";
constexpr std::string_view kGpvModulePath =
    "src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/"
    "mtf_jepa_mae_vicreg.h";
constexpr std::string_view kGpvModuleSha256 =
    "93640972e497dc49f37e7690e59c2f2e55f12ece25687fe4e6f6c96b28c3c9ea";
constexpr std::string_view kGpvReadoutPolicy = "structured_cdsb_sparse_v1";
constexpr std::string_view kGpvCertificateId =
    "representation_certified_fspa4_minimal_spectral_repair_v1";
constexpr std::string_view kGpvCellCacheSchema =
    "gpv1.cell_cache.v2";
constexpr std::string_view kGpvCellCacheImplementation =
    "retained_oca_views_single_graph_atomic_cell_fresh_tensor_codec_v2";
constexpr int64_t kGpvSteps = 512;
constexpr int64_t kGpvCanonicalSlots = 72;
constexpr double kGpvLearningRate = 1.0e-3;
constexpr double kGpvClipNorm = 5.0;
constexpr double kGpvOuterWeight = 0.05;
constexpr double kGpvInnerWeight = 0.25;
constexpr double kGpvImprovementFloor = 0.0025;
constexpr double kGpvRescueFloor = 0.005;
constexpr double kGpvAnchorNoninferiority = -0.005;
constexpr double kGpvFamilyFloor = -0.02;
constexpr double kGpvProductionAtol = 1.0e-7;
constexpr double kGpvProductionRtol = 1.0e-6;
constexpr double kGpvReconstructionAbs = 5.0e-5;
constexpr double kGpvReconstructionRelative = 1.0e-4;
constexpr std::array<uint8_t, 5> kGpvOcaChallengeMasks{4U, 1U, 2U, 8U,
                                                       15U};
constexpr std::array<std::string_view, 8> kGpvMaskNames{
    "global_gelu_coupled",       "slot_gelu_coupled",
    "global_affine_coupled",     "slot_affine_coupled",
    "global_gelu_head_only",     "slot_gelu_head_only",
    "global_affine_head_only",   "slot_affine_head_only"};
constexpr std::array<std::string_view, 3> kGpvAnchorSha256{
    "5d96b2961daa2bbd08a07a157ddab5debd9d4928234d8341b3961678327e9434",
    "a85c00d5694d1e3f0063e8bce6fc3c2e3132a393e5bcee195909742754e76775",
    "b9c2f82f26a5516069f8460095d3a2b85c482b7cd0e20db120ce2e0bbd68e392"};
constexpr std::array<std::string_view, 3> kGpvCurrentCacheSha256{
    "5290559894fa6e3f5d2fd57f32a90e97bb0eec924ec22cc9354ae26d0e629c92",
    "bb5391840ede1ad4aaf8ff760d39b796a7a300918e1aab881fbb5255051fcc39",
    "aaa7dd4b7638f240d1e940db7ede3bc40eb8ad01000baa90dc043801a29256a6"};
constexpr std::array<double, 3> kGpvExpectedCurrentAulc{
    0.60479711475045383, 0.58888535837559386, 0.64066353570869317};

static_assert(kGpvMaskNames.size() == 8);
static_assert(kGpvSteps * 7 * 3 == 10752);

struct GpvProjectorLinears {
  std::shared_ptr<torch::nn::LinearImpl> input{};
  std::shared_ptr<torch::nn::LinearImpl> hidden{};
  std::shared_ptr<torch::nn::LinearImpl> output{};
};

struct GpvObjective {
  torch::Tensor projected_a{};
  torch::Tensor projected_b{};
  torch::Tensor variance_projected_a{};
  torch::Tensor variance_projected_b{};
  torch::Tensor invariance{};
  torch::Tensor variance{};
  torch::Tensor covariance{};
  torch::Tensor raw{};
  torch::Tensor branch{};
  torch::Tensor total{};
  int64_t valid_rows{0};
  int64_t active_slots{0};
  bool canonical_layout{false};
  bool finite{false};
};

struct GpvReceipt {
  int64_t steps{0};
  int64_t adam_steps{0};
  int64_t ema_steps{0};
  int64_t clipping_count{0};
  std::vector<double> losses{};
  std::vector<double> gradient_norms{};
  std::vector<double> trunk_gradient_norms{};
  std::vector<double> head_gradient_norms{};
  std::vector<double> clip_factors{};
  std::vector<double> served_update_norms{};
  std::array<std::vector<double>, 3> component_losses{};
  std::array<double, 3> component_loss_sums{};
  std::vector<uint64_t> row_hashes{};
  std::vector<uint64_t> target_mask_hashes{};
  std::vector<uint64_t> context_mask_hashes{};
  std::vector<uint64_t> view_a_data_hashes{};
  std::vector<uint64_t> view_a_mask_hashes{};
  std::vector<uint64_t> view_b_data_hashes{};
  std::vector<uint64_t> view_b_mask_hashes{};
  std::vector<uint64_t> rng_pre_cpu_hashes{};
  std::vector<uint64_t> rng_pre_cuda_hashes{};
  std::vector<uint64_t> rng_post_cpu_hashes{};
  std::vector<uint64_t> rng_post_cuda_hashes{};
  std::vector<int64_t> step_flags{};
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
  std::vector<std::string> parameter_delta_names{};
  std::vector<double> parameter_deltas{};
  bool initialization_exact{false};
  bool finite{true};
  bool schedule_exact{true};
  bool expected_partitions{false};
  bool pass{false};
};

struct GpvCell {
  mtf::MtfJepaMaeVicreg model{nullptr};
  GpvReceipt receipt{};
  bool resumed{false};
};

struct GpvCustody {
  bool protocol{false};
  bool recovery{false};
  bool recovery_marker{false};
  bool legacy_v1_archive{false};
  bool legacy_v1_marker{false};
  bool legacy_v1_inventory{false};
  bool legacy_v1_log_absent{false};
  bool vva_protocol{false};
  bool vva_findings{false};
  bool vva_log{false};
  bool vva_harness{false};
  bool vva_fields{false};
  bool oca_log{false};
  bool oca_fields{false};
  bool module{false};
  bool anchors{false};
  bool anchor_replay{false};
  bool current_cache_hashes{false};
  bool current_caches{false};
  bool pass{false};
};

struct GpvBindings {
  std::string harness_sha256{};
  std::string executable_sha256{};
  std::string scientific_manifest_sha256{};
  std::string dataset_sha256{};
  std::string splits_sha256{};
  std::string bootstrap_sha256{};
  std::string scientific_manifest{};
  bool pass{false};
};

struct GpvInventory {
  std::array<mtf::MtfJepaMaeVicreg, 3> anchors{
      mtf::MtfJepaMaeVicreg(nullptr), mtf::MtfJepaMaeVicreg(nullptr),
      mtf::MtfJepaMaeVicreg(nullptr)};
  std::array<mtf::MtfJepaMaeVicreg, 3> current{
      mtf::MtfJepaMaeVicreg(nullptr), mtf::MtfJepaMaeVicreg(nullptr),
      mtf::MtfJepaMaeVicreg(nullptr)};
  std::array<OcaLegacyTrainingReceipt, 3> current_receipts{};
  bool pass{false};
};

struct GpvStageZero {
  bool current_route_parity{true};
  bool canonical_slots{true};
  bool retained_views{true};
  bool projector_identity{true};
  bool variance_value_and_head_gradient{true};
  bool variance_trunk_decomposition{true};
  bool component_gradients_finite{true};
  bool intended_gradients_active{true};
  bool disposable_updates{true};
  bool finite_and_active{true};
  bool inactive_partitions{true};
  bool view_dose{true};
  int64_t disposable_clones{0};
  int64_t disposable_adam_updates{0};
  int64_t disposable_ema_updates{0};
  bool pass{false};
};

struct GpvFactorGradientAudit {
  bool finite{true};
  bool intended_active{true};
};

struct GpvDisposableUpdateAudit {
  int64_t adam_updates{0};
  int64_t ema_updates{0};
  bool gradients_finite{false};
  bool partitions_exact{false};
  bool model_finite{false};
  bool pass{false};
};

using GpvEvaluations = std::array<std::array<RmcEvaluation, 8>, 3>;
using GpvCells = std::array<std::array<GpvCell, 8>, 3>;
using GpvCachePresence = std::array<std::array<bool, 8>, 3>;
using GpvBootstrapAreaTable =
    std::vector<std::array<std::array<double, 8>, 3>>;

struct GpvWeightedContrast {
  rmc_gate::Contrast summary{};
  std::array<double, 3> per_seed{};
  std::array<double, kFamilies> family{};
  bool no_new_safeguard_failure{false};
  bool gate{false};
};

struct GpvSafeguards {
  std::array<bool, 10> values{};
  bool all{false};
};

enum class GpvClassification {
  representation_rescue,
  objective_made_safe,
  mechanism_mitigates_harm_only,
  no_safe_candidate,
  invalid_numeric_or_mechanics,
};

[[nodiscard]] const char *gpv_classification_name(GpvClassification value) {
  switch (value) {
  case GpvClassification::representation_rescue:
    return "representation_rescue";
  case GpvClassification::objective_made_safe:
    return "objective_made_safe";
  case GpvClassification::mechanism_mitigates_harm_only:
    return "mechanism_mitigates_harm_only";
  case GpvClassification::no_safe_candidate:
    return "no_safe_candidate";
  case GpvClassification::invalid_numeric_or_mechanics:
    return "invalid_numeric_or_mechanics";
  }
  throw std::runtime_error("GPV classification is invalid");
}

struct GpvDevelopmentResult {
  GpvEvaluations evaluations{};
  std::array<RmcEvaluation, 3> anchors{};
  std::array<GpvWeightedContrast, 8> direct{};
  std::array<GpvWeightedContrast, 8> versus_anchor{};
  std::array<GpvClassification, 8> classification{};
  std::array<GpvSafeguards, 8> safeguards{};
  std::array<GpvWeightedContrast, 3> main_effects{};
  std::array<GpvWeightedContrast, 3> two_way{};
  GpvWeightedContrast three_way{};
  std::array<std::array<GpvWeightedContrast, 4>, 3> simple{};
  std::optional<uint8_t> selected{};
  GpvClassification selected_classification{
      GpvClassification::no_safe_candidate};
  bool mechanics{false};
};

struct GpvConfirmationResult {
  bool mechanics{true};
  bool pass{false};
};

[[nodiscard]] bool gpv_tensor_exact(const torch::Tensor &left,
                                    const torch::Tensor &right) {
  return left.defined() == right.defined() &&
         (!left.defined() || (left.sizes() == right.sizes() &&
                             left.scalar_type() == right.scalar_type() &&
                             rssm_tensor_bytes_equal(left, right)));
}

[[nodiscard]] torch::Tensor gpv_read_tensor(
    torch::serialize::InputArchive &archive, const std::string &key) {
  torch::Tensor value{};
  archive.read(key, value);
  return value;
}

void gpv_require_archive_tensor(const torch::Tensor &value,
                                torch::ScalarType dtype, int64_t rank,
                                int64_t count, std::string_view key) {
  if (!value.defined() || value.scalar_type() != dtype ||
      value.dim() != rank || value.numel() != count) {
    throw std::runtime_error("GPV archived tensor contract failed: " +
                             std::string(key));
  }
}

[[nodiscard]] bool gpv_tensor_close(const torch::Tensor &left,
                                    const torch::Tensor &right,
                                    double tolerance = kGpvReconstructionAbs) {
  return left.defined() && right.defined() && left.sizes() == right.sizes() &&
         torch::isfinite(left).all().item<bool>() &&
         torch::isfinite(right).all().item<bool>() &&
         (left.detach().to(torch::kCPU, torch::kFloat64) -
          right.detach().to(torch::kCPU, torch::kFloat64))
                 .abs()
                 .max()
                 .item<double>() <= tolerance;
}

[[nodiscard]] bool gpv_production_close(const torch::Tensor &left,
                                        const torch::Tensor &right) {
  return left.defined() && right.defined() && left.sizes() == right.sizes() &&
         torch::allclose(left, right, kGpvProductionRtol,
                         kGpvProductionAtol);
}

[[nodiscard]] bool gpv_metadata_exact(const mtf::mtf_token_metadata_t &left,
                                      const mtf::mtf_token_metadata_t &right) {
  return gpv_tensor_exact(left.start_index, right.start_index) &&
         gpv_tensor_exact(left.width, right.width) &&
         gpv_tensor_exact(left.scale_id, right.scale_id) &&
         gpv_tensor_exact(left.channel_id, right.channel_id) &&
         gpv_tensor_exact(left.domain_id, right.domain_id);
}

using GpvMetadataTuple = std::array<int64_t, 5>;

[[nodiscard]] std::vector<GpvMetadataTuple> gpv_expected_metadata_sequence(
    const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  constexpr std::array<int64_t, 4> kExpectedScales{8, 16, 32, 64};
  constexpr std::array<int64_t, 4> kExpectedStrides{4, 8, 16, 32};
  constexpr std::array<int64_t, 12> kScaleIds{0, 0, 0, 0, 0, 0,
                                              0, 1, 1, 1, 2, 3};
  constexpr std::array<int64_t, 12> kStarts{0, 4, 8, 12, 16, 20,
                                            22, 0, 8, 14, 0, 0};
  constexpr std::array<int64_t, 12> kWidths{8, 8, 8, 8,  8,  8,
                                            8, 16, 16, 16, 30, 30};
  const auto strides = mtf::detail::resolved_scale_strides(config);
  const bool frozen_config =
      config.history_length == 30 && config.channel_count == 3 &&
      config.use_frequency_tokens &&
      config.time_scales.size() == kExpectedScales.size() &&
      strides.size() == kExpectedStrides.size() &&
      std::equal(config.time_scales.begin(), config.time_scales.end(),
                 kExpectedScales.begin()) &&
      std::equal(strides.begin(), strides.end(), kExpectedStrides.begin());
  if (!frozen_config) {
    throw std::runtime_error("GPV frozen metadata configuration failed");
  }

  // This frozen table is derived from the production construction:
  // concat(time, frequency), then channel -> scale -> window_plan.  Keeping
  // the resulting twelve-slot plan local makes this check independent of an
  // observed encode and catches a same-cardinality production planner drift.
  std::vector<GpvMetadataTuple> expected;
  expected.reserve(kGpvCanonicalSlots);
  for (int64_t domain = 0; domain < 2; ++domain) {
    for (int64_t channel = 0; channel < config.channel_count; ++channel) {
      for (std::size_t position = 0; position < kScaleIds.size(); ++position) {
        expected.push_back({domain, channel, kScaleIds[position],
                            kStarts[position], kWidths[position]});
      }
    }
  }
  if (expected.size() != static_cast<std::size_t>(kGpvCanonicalSlots)) {
    throw std::runtime_error("GPV frozen metadata sequence size failed");
  }
  return expected;
}

[[nodiscard]] bool gpv_canonical_metadata(
    const mtf::mtf_token_metadata_t &metadata,
    const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  for (const auto &value : {metadata.start_index, metadata.width,
                            metadata.scale_id, metadata.channel_id,
                            metadata.domain_id}) {
    if (!value.defined() || value.dim() != 1 ||
        value.numel() != kGpvCanonicalSlots ||
        value.scalar_type() != torch::kInt64) {
      return false;
    }
  }
  const auto start = metadata.start_index.to(torch::kCPU).contiguous();
  const auto width = metadata.width.to(torch::kCPU).contiguous();
  const auto scale = metadata.scale_id.to(torch::kCPU).contiguous();
  const auto channel = metadata.channel_id.to(torch::kCPU).contiguous();
  const auto domain = metadata.domain_id.to(torch::kCPU).contiguous();
  const auto starts = start.accessor<int64_t, 1>();
  const auto widths = width.accessor<int64_t, 1>();
  const auto scales = scale.accessor<int64_t, 1>();
  const auto channels = channel.accessor<int64_t, 1>();
  const auto domains = domain.accessor<int64_t, 1>();
  const auto expected = gpv_expected_metadata_sequence(config);
  for (int64_t index = 0; index < kGpvCanonicalSlots; ++index) {
    const auto &tuple = expected[static_cast<std::size_t>(index)];
    if (domains[index] != tuple[0] || channels[index] != tuple[1] ||
        scales[index] != tuple[2] || starts[index] != tuple[3] ||
        widths[index] != tuple[4]) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] GpvProjectorLinears
gpv_projector_linears(const mtf::MtfJepaMaeVicreg &model) {
  GpvProjectorLinears result{};
  for (const auto &item : model->named_modules("", /*include_self=*/false)) {
    auto linear = std::dynamic_pointer_cast<torch::nn::LinearImpl>(item.value());
    if (item.key() == "vicreg_stability_head.projector_in") {
      result.input = std::move(linear);
    } else if (item.key() ==
               "vicreg_stability_head.projector_hidden") {
      result.hidden = std::move(linear);
    } else if (item.key() == "vicreg_stability_head.projector_out") {
      result.output = std::move(linear);
    }
  }
  if (!result.input || !result.hidden || !result.output) {
    throw std::runtime_error("GPV live projector module inventory failed");
  }
  return result;
}

[[nodiscard]] torch::Tensor
gpv_project(mtf::MtfJepaMaeVicreg &model, const torch::Tensor &pool,
            bool affine) {
  if (!pool.defined() || (pool.dim() != 2 && pool.dim() != 3) ||
      pool.size(-1) != model->config().latent_dim) {
    throw std::runtime_error("GPV projector input contract failed");
  }
  if (!affine) {
    return model->project_vicreg(pool);
  }
  const auto layers = gpv_projector_linears(model);
  const auto shape = pool.sizes().vec();
  auto flattened = pool.reshape({-1, pool.size(-1)});
  auto projected = layers.output->forward(
      layers.hidden->forward(layers.input->forward(flattened)));
  if (projected.size(1) != 64) {
    throw std::runtime_error("GPV affine projector output width failed");
  }
  if (pool.dim() == 2) {
    return projected;
  }
  return projected.view({shape[0], shape[1], projected.size(1)});
}

[[nodiscard]] mtf::vicreg_stability_loss_result_t gpv_reduce(
    const torch::Tensor &projected_a, const torch::Tensor &mask_a,
    const torch::Tensor &projected_b, const torch::Tensor &mask_b,
    const mtf::vicreg_stability_loss_options_t &options, bool slot_local) {
  return slot_local
             ? mtf::compute_channel_stratified_vicreg_stability_loss(
                   projected_a, mask_a, projected_b, mask_b, options)
             : mtf::compute_vicreg_stability_loss(projected_a, mask_a,
                                                  projected_b, mask_b,
                                                  options);
}

[[nodiscard]] GpvObjective
gpv_objective_from_pools(mtf::MtfJepaMaeVicreg &model,
                         const torch::Tensor &pool_a,
                         const torch::Tensor &mask_a,
                         const torch::Tensor &pool_b,
                         const torch::Tensor &mask_b, uint8_t factor_mask) {
  if (factor_mask >= 8U) {
    throw std::runtime_error("GPV factor mask is invalid");
  }
  const bool slot_local = (factor_mask & 0x1U) != 0U;
  const bool affine = (factor_mask & 0x2U) != 0U;
  const bool head_only = (factor_mask & 0x4U) != 0U;
  const int64_t expected_groups = slot_local ? kGpvCanonicalSlots : 1;
  if (pool_a.dim() != 3 || pool_b.dim() != 3 ||
      pool_a.sizes() != pool_b.sizes() || mask_a.dim() != 2 ||
      mask_a.sizes() != mask_b.sizes() || pool_a.size(0) != mask_a.size(0) ||
      pool_a.size(1) != mask_a.size(1) ||
      pool_a.size(1) != expected_groups) {
    throw std::runtime_error("GPV selected-pool shape contract failed");
  }

  mtf::vicreg_stability_loss_options_t options{};
  options.invariance_weight = model->config().vicreg_sim_weight;
  options.variance_weight = model->config().vicreg_var_weight;
  options.covariance_weight = model->config().vicreg_cov_weight;
  options.variance_floor = model->config().vicreg_variance_floor;
  options.eps = model->config().vicreg_variance_epsilon;

  GpvObjective result{};
  result.projected_a = gpv_project(model, pool_a, affine);
  result.projected_b = gpv_project(model, pool_b, affine);
  result.variance_projected_a =
      head_only ? gpv_project(model, pool_a.detach(), affine)
                : result.projected_a;
  result.variance_projected_b =
      head_only ? gpv_project(model, pool_b.detach(), affine)
                : result.projected_b;
  const auto attached =
      gpv_reduce(result.projected_a, mask_a, result.projected_b, mask_b,
                 options, slot_local);
  const auto variance =
      head_only ? gpv_reduce(result.variance_projected_a, mask_a,
                             result.variance_projected_b, mask_b, options,
                             slot_local)
                : attached;
  result.invariance = attached.invariance_loss;
  result.variance = variance.variance_loss;
  result.covariance = attached.covariance_loss;
  result.raw = options.invariance_weight * result.invariance +
               options.variance_weight * result.variance +
               options.covariance_weight * result.covariance;
  result.branch = kGpvInnerWeight * result.raw;
  result.total = kGpvOuterWeight * result.branch;
  result.valid_rows = attached.valid_rows;
  result.active_slots = attached.active_groups;
  result.finite = torch::isfinite(torch::stack(
                                      {result.invariance, result.variance,
                                       result.covariance, result.raw,
                                       result.branch, result.total}))
                      .all()
                      .item<bool>();
  return result;
}

[[nodiscard]] GpvObjective gpv_route_retained_views(
    mtf::MtfJepaMaeVicreg &model, const torch::Tensor &view_a_data,
    const torch::Tensor &view_a_mask, const torch::Tensor &view_b_data,
    const torch::Tensor &view_b_mask, uint8_t factor_mask) {
  const auto encoded_a = model->encode(view_a_data, view_a_mask);
  const auto encoded_b = model->encode(view_b_data, view_b_mask);
  const bool layout = encoded_a.embeddings.size(1) == kGpvCanonicalSlots &&
                       encoded_b.embeddings.size(1) == kGpvCanonicalSlots &&
                       gpv_canonical_metadata(encoded_a.metadata,
                                              model->config()) &&
                       gpv_canonical_metadata(encoded_b.metadata,
                                              model->config()) &&
                       gpv_metadata_exact(encoded_a.metadata,
                                          encoded_b.metadata);
  if (!layout) {
    throw std::runtime_error("GPV canonical slot layout failed");
  }
  const bool slot_local = (factor_mask & 0x1U) != 0U;
  const auto pool_a = slot_local ? encoded_a.embeddings
                                 : encoded_a.pooled_embedding.unsqueeze(1);
  const auto pool_b = slot_local ? encoded_b.embeddings
                                 : encoded_b.pooled_embedding.unsqueeze(1);
  const auto mask_a = slot_local ? encoded_a.token_mask
                                 : encoded_a.sample_valid_mask.unsqueeze(1);
  const auto mask_b = slot_local ? encoded_b.token_mask
                                 : encoded_b.sample_valid_mask.unsqueeze(1);
  auto result = gpv_objective_from_pools(model, pool_a, mask_a, pool_b,
                                         mask_b, factor_mask);
  result.canonical_layout = layout;
  return result;
}

[[nodiscard]] std::string gpv_factor_manifest(uint8_t mask) {
  if (mask >= 8U) {
    throw std::runtime_error("GPV factor manifest mask failed");
  }
  std::ostringstream out;
  out << "factor_mask=" << static_cast<int>(mask) << '\n';
  out << "factor_name=" << kGpvMaskNames[mask] << '\n';
  out << "pool=" << ((mask & 0x1U) ? "canonical_slot_local_72"
                                      : "global_mean")
      << '\n';
  out << "projector=" << ((mask & 0x2U) ? "same_linears_no_gelu"
                                           : "production_gelu")
      << '\n';
  out << "variance_path=" << ((mask & 0x4U) ? "head_only_pool_detach"
                                               : "trunk_coupled")
      << '\n';
  out << "slot_divisor=72\nprojector_width=64\nobjective_mask=8\n";
  return out.str();
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t
gpv_config(const torch::Device &device) {
  auto config = attribution_config(device, oca_arm(8U));
  if (config.lambda_jepa != 0.0 || config.lambda_mae != 0.0 ||
      config.lambda_tf_align != 0.0 || config.lambda_vicreg != kGpvOuterWeight ||
      config.lambda_global_vicreg != kGpvInnerWeight ||
      config.vicreg_sim_weight != 25.0 || config.vicreg_var_weight != 25.0 ||
      config.vicreg_cov_weight != 1.0 || config.projector_dim != 64 ||
      config.predictor_hidden_dim != 64 || config.feature_dropout_prob != 0.0 ||
      std::abs(mtf::detail::resolved_vicreg_view_time_dropout_prob(config) -
               0.01) > 1.0e-12 ||
      config.vicreg_view_gaussian_jitter_std != 0.005) {
    throw std::runtime_error("GPV frozen configuration contract failed");
  }
  return config;
}

[[nodiscard]] std::string gpv_config_manifest(const torch::Device &device) {
  return canonical_config_manifest(gpv_config(device));
}

[[nodiscard]] std::string gpv_anchor_config_hash(
    const torch::Device &device) {
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  return oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
}

[[nodiscard]] bool gpv_line_count(const std::string &text,
                                  std::string_view expected,
                                  std::size_t count = 1) {
  std::istringstream input(text);
  std::string line;
  std::size_t observed = 0;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    observed += line == expected ? 1U : 0U;
  }
  return observed == count;
}

[[nodiscard]] std::string
gpv_sha256_file(const std::filesystem::path &path) {
  return digest::sha256_hex(rmc_read_file(path));
}

[[nodiscard]] std::string
gpv_dataset_manifest(const RmcData &data, bool splits_only) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  const auto add = [&](std::string_view name, const Dataset &dataset) {
    out << name << ".group_begin=" << std::dec << dataset.group_begin << '\n'
        << std::hex;
    if (!splits_only) {
      out << name << ".data=" << std::setw(16)
          << hash_tensor_stable_bytes(dataset.data) << '\n';
      out << name << ".mask=" << std::setw(16)
          << hash_tensor_stable_bytes(dataset.mask) << '\n';
    }
    out << name << ".target=" << std::setw(16)
        << hash_tensor_stable_bytes(dataset.target) << '\n';
  };
  add("ssl", data.ssl);
  add("probe_train", data.probe_train);
  add("probe_validation", data.probe_validation);
  add("development", data.development);
  add("reversed_train", data.reversed_train);
  add("reversed_validation", data.reversed_validation);
  add("reversed_development", data.reversed_development);
  if (!splits_only) {
    out << "normalization.mean=" << std::setw(16)
        << hash_tensor_stable_bytes(data.normalization.mean) << '\n';
    out << "normalization.inv_std=" << std::setw(16)
        << hash_tensor_stable_bytes(data.normalization.inv_std) << '\n';
  }
  return out.str();
}

[[nodiscard]] std::string gpv_bootstrap_manifest(
    const std::vector<torch::Tensor> &bootstrap_rows) {
  std::ostringstream out;
  out << "replicates=" << bootstrap_rows.size() << '\n' << std::hex
      << std::setfill('0');
  for (std::size_t index = 0; index < bootstrap_rows.size(); ++index) {
    out << index << '=' << std::setw(16)
        << hash_tensor_stable_bytes(bootstrap_rows[index]) << '\n';
  }
  return out.str();
}

[[nodiscard]] std::map<std::string, std::filesystem::path>
gpv_transitive_dependencies() {
  const std::filesystem::path depfile =
      ".build/tests/gpv1_full_dependencies_v1.d";
  const auto bytes = rmc_read_file(depfile);
  std::string flattened;
  flattened.reserve(bytes.size());
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    if (bytes[index] == '\\' && index + 1 < bytes.size() &&
        bytes[index + 1] == '\n') {
      ++index;
      flattened.push_back(' ');
    } else if (bytes[index] != '\r') {
      flattened.push_back(bytes[index]);
    }
  }
  const auto separator = flattened.find(':');
  if (separator == std::string::npos) {
    throw std::runtime_error("GPV compiler depfile has no primary rule");
  }
  std::istringstream input(flattened.substr(separator + 1));
  std::map<std::string, std::filesystem::path> paths;
  std::string token;
  const auto repository_root =
      std::filesystem::absolute(std::filesystem::current_path())
          .lexically_normal();
  constexpr std::string_view cuda_prefix = "/usr/local/cuda-12.4/";
  while (input >> token) {
    if (token == "\\") {
      continue;
    }
    std::filesystem::path physical(token);
    if (physical.is_relative()) {
      physical = repository_root / physical;
    }
    physical = physical.lexically_normal();
    if (!std::filesystem::is_regular_file(physical)) {
      throw std::runtime_error("GPV dependency is missing or not regular: " +
                               token);
    }
    std::string logical;
    const auto relative = physical.lexically_relative(repository_root);
    const bool inside_repository =
        !relative.empty() && *relative.begin() != "..";
    const auto physical_string = physical.generic_string();
    if (inside_repository) {
      logical = "repo/" + relative.generic_string();
    } else if (physical_string.starts_with(cuda_prefix)) {
      logical = "cuda/" +
                physical_string.substr(std::string(cuda_prefix).size());
    } else {
      logical = "host" + physical_string;
    }
    const auto [position, inserted] = paths.emplace(logical, physical);
    if (!inserted && position->second != physical) {
      throw std::runtime_error("GPV logical dependency collision: " + logical);
    }
  }
  constexpr std::array<std::string_view, 10> required{
      kGpvHarnessPath,
      "src/include/piaabo/digest/sha256.h",
      "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
      "vicreg/quality_wikimyei_mtf_jepa_mae_vicreg_four_objective_"
      "causal_attribution.cpp",
      "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
      "vicreg/quality_wikimyei_mtf_jepa_mae_vicreg_minimal_"
      "participation_spectral_repair.cpp",
      "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
      "vicreg/quality_wikimyei_mtf_jepa_mae_vicreg_geometry_"
      "preserving_whitening_distillation.cpp",
      "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
      "vicreg/quality_wikimyei_mtf_jepa_mae_vicreg_frozen_sequence_"
      "projection_alignment.cpp",
      "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
      "vicreg/quality_wikimyei_mtf_jepa_mae_vicreg_representation_"
      "module_certification.cpp",
      "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
      "vicreg/quality_wikimyei_mtf_jepa_mae_vicreg_representation.cpp",
      kGpvModulePath,
      "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
      "vicreg/representation_module_certification_gate.h"};
  for (const auto path : required) {
    if (!paths.contains("repo/" + std::string(path))) {
      throw std::runtime_error("GPV transitive dependency missing: " +
                               std::string(path));
    }
  }
  const bool system_headers = std::any_of(
      paths.begin(), paths.end(), [](const auto &entry) {
        return entry.first.starts_with("host/usr/include/") ||
               entry.first.starts_with("host/usr/lib/");
      });
  const bool cuda_headers = std::any_of(
      paths.begin(), paths.end(), [](const auto &entry) {
        return entry.first.starts_with("cuda/") &&
               entry.first.ends_with("cuda_runtime_api.h");
      });
  if (!system_headers || !cuda_headers) {
    throw std::runtime_error(
        "GPV full dependency scan omitted system or CUDA headers");
  }
  return paths;
}

void gpv_validate_build_receipt(const std::filesystem::path &path) {
  const auto bytes = rmc_read_file(path);
  constexpr std::array<std::string_view, 12> required{
      "schema=gpv1.effective_build_receipt.v2\n",
      "workspace.logical_root=repo\n",
      "compile.command=",
      "dependency_scan.command=",
      "link.command=",
      "tool:compiler=",
      "tool:linker_driver=",
      "tool:cc1plus=",
      "tool:collect2=",
      "tool:ld=",
      "tool:assembler=",
      "dso.count="};
  for (const auto field : required) {
    if (bytes.find(field) == std::string::npos) {
      throw std::runtime_error("GPV build receipt field is missing: " +
                               std::string(field));
    }
  }
  const auto repository_root =
      std::filesystem::absolute(std::filesystem::current_path())
          .lexically_normal()
          .generic_string();
  if (bytes.find(repository_root + "/") != std::string::npos ||
      bytes.find("/cuwacunu/") != std::string::npos ||
      bytes.find("${REPO}/") == std::string::npos) {
    throw std::runtime_error("GPV build receipt is not logical-root stable");
  }
  const auto valid_digest = [](std::string_view value) {
    return value.size() == 64 &&
           std::all_of(value.begin(), value.end(), [](char byte) {
             return (byte >= '0' && byte <= '9') ||
                    (byte >= 'a' && byte <= 'f');
           });
  };
  std::size_t dso_count = 0;
  std::optional<std::size_t> declared_dso_count;
  std::istringstream lines(bytes);
  std::string line;
  while (std::getline(lines, line)) {
    if (line.starts_with("tool:") || line.starts_with("dso:")) {
      const auto equals = line.rfind('=');
      if (equals == std::string::npos ||
          !valid_digest(std::string_view(line).substr(equals + 1))) {
        throw std::runtime_error("GPV build receipt digest is malformed");
      }
    }
    if (line.starts_with("dso:")) {
      ++dso_count;
    } else if (line.starts_with("dso.count=")) {
      const auto value = line.substr(std::string("dso.count=").size());
      std::size_t consumed = 0;
      const auto parsed = std::stoull(value, &consumed);
      if (consumed != value.size()) {
        throw std::runtime_error("GPV build receipt DSO count is malformed");
      }
      declared_dso_count = static_cast<std::size_t>(parsed);
    }
  }
  if (dso_count == 0 || !declared_dso_count.has_value() ||
      *declared_dso_count != dso_count) {
    throw std::runtime_error("GPV build receipt DSO inventory is incomplete");
  }
}

[[nodiscard]] std::string gpv_scientific_manifest() {
  constexpr std::array<std::string_view, 9> scientific_inputs{
      kGpvProtocolPath,
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "GLOBAL_POOL_PROJECTOR_VARIANCE_CAUSAL_DECOMPOSITION_PROTOCOL.sha256",
      kGpvRecoveryPath,
      kGpvRecoveryMarkerPath,
      kGpvVvaHarnessPath,
      kGpvVvaProtocolPath,
      kGpvVvaFindingsPath,
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/MINIMAL_PARTICIPATION_SPECTRAL_REPAIR_PROTOCOL.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/FOUR_OBJECTIVE_CAUSAL_ATTRIBUTION_PROTOCOL.md"};
  constexpr std::array<std::string_view, 25> build_inputs{
      "src/Makefile.config",
      "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/"
      "Makefile",
      ".build/tests/gpv1_build_receipt_v1.txt",
      ".build/obj/.objs/quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_"
      "projector_variance_causal_decomposition.o",
      ".build/lib/libtorchwrap.a",
      ".build/obj/common/core/utils.o",
      ".build/obj/common/io/files.o",
      ".build/obj/common/parse/json/json_parsing.o",
      ".build/obj/common/statistics_space.o",
      ".build/obj/common/registry_data.o",
      ".build/obj/common/registry_utils.o",
      ".build/obj/common/source_contract.o",
      ".build/obj/common/source_contract_decode.o",
      ".build/obj/common/source_registry_decoder.o",
      ".build/obj/common/retrieval_channel_decoder.o",
      ".build/obj/common/graph_topology_decoder.o",
      ".build/obj/common/parser_types.o",
      ".build/obj/common/ast.o",
      ".build/obj/common/grammar_lexer.o",
      ".build/obj/common/grammar_parser.o",
      ".build/obj/common/instruction_lexer.o",
      ".build/obj/common/instruction_parser.o",
      ".build/obj/common/memory_mapped_datafile.o",
      ".build/obj/libtorch/memory_mapped_dataset.o",
      ".build/obj/libtorch/memory_mapped_dataloader.o"};
  std::ostringstream out;
  gpv_validate_build_receipt(".build/tests/gpv1_build_receipt_v1.txt");
  out << "schema=gpv1.transitive_scientific_build_manifest.v2\n";
  out << "toolchain.compiler=" << __VERSION__ << '\n';
  out << "toolchain.cplusplus=" << __cplusplus << '\n';
  out << "toolchain.torch=" << TORCH_VERSION << '\n';
  out << "toolchain.cuda_compile=" << CUDART_VERSION << '\n';
  const auto dependencies = gpv_transitive_dependencies();
  out << "transitive_dependency_count=" << dependencies.size() << '\n';
  for (const auto &[logical, path] : dependencies) {
    out << "dependency:" << logical << '=' << gpv_sha256_file(path) << '\n';
  }
  for (const auto path : scientific_inputs) {
    out << "scientific_input:" << path << '='
        << gpv_sha256_file(std::filesystem::path(path)) << '\n';
  }
  for (const auto path : build_inputs) {
    out << "build_input:" << path << '='
        << gpv_sha256_file(std::filesystem::path(path)) << '\n';
  }
  return out.str();
}

[[nodiscard]] GpvBindings gpv_make_bindings(
    const RmcData &data, const std::vector<torch::Tensor> &bootstrap_rows,
    const std::filesystem::path &executable) {
  GpvBindings result{};
  result.harness_sha256 =
      gpv_sha256_file(std::filesystem::path(kGpvHarnessPath));
  result.executable_sha256 = gpv_sha256_file(executable);
  result.scientific_manifest = gpv_scientific_manifest();
  result.scientific_manifest_sha256 =
      digest::sha256_hex(result.scientific_manifest);
  result.dataset_sha256 = digest::sha256_hex(gpv_dataset_manifest(data, false));
  result.splits_sha256 = digest::sha256_hex(gpv_dataset_manifest(data, true));
  result.bootstrap_sha256 =
      digest::sha256_hex(gpv_bootstrap_manifest(bootstrap_rows));
  result.pass = result.harness_sha256.size() == 64 &&
                result.executable_sha256.size() == 64 &&
                result.scientific_manifest_sha256.size() == 64 &&
                result.dataset_sha256.size() == 64 &&
                result.splits_sha256.size() == 64 &&
                result.bootstrap_sha256.size() == 64;
  return result;
}

void gpv_emit_bindings(const GpvBindings &value) {
  std::cout << "gpv1.binding.protocol_sha256=" << kGpvProtocolSha256 << '\n';
  std::cout << "gpv1.binding.codec_recovery_sha256="
            << kGpvRecoverySha256 << '\n';
  std::cout << "gpv1.binding.harness_sha256=" << value.harness_sha256 << '\n';
  std::cout << "gpv1.binding.executable_sha256=" << value.executable_sha256
            << '\n';
  std::cout << "gpv1.binding.module_sha256=" << kGpvModuleSha256 << '\n';
  std::cout << "gpv1.binding.scientific_manifest_sha256="
            << value.scientific_manifest_sha256 << '\n';
  std::cout << "gpv1.binding.dataset_sha256=" << value.dataset_sha256 << '\n';
  std::cout << "gpv1.binding.splits_sha256=" << value.splits_sha256 << '\n';
  std::cout << "gpv1.binding.bootstrap_sha256=" << value.bootstrap_sha256
            << '\n';
  std::cout << "gpv1.binding.pass=" << value.pass << '\n';
}

[[nodiscard]] GpvInventory gpv_load_inventory(
    const RmcData &data, const torch::Device &device, GpvCustody &custody) {
  GpvInventory inventory{};
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto config_hash = gpv_anchor_config_hash(device);
  const std::vector<uint8_t> challenge_masks(kGpvOcaChallengeMasks.begin(),
                                              kGpvOcaChallengeMasks.end());
  custody.anchors = true;
  custody.anchor_replay = true;
  custody.current_cache_hashes = true;
  custody.current_caches = true;
  for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
       ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    const auto anchor_path = oca_archive_path(seed);
    custody.anchors = custody.anchors &&
                      gpv_sha256_file(anchor_path) ==
                          kGpvAnchorSha256[seed_index];
    set_paired_rng(seed, device);
    auto first = mtf::MtfJepaMaeVicreg(anchor_config);
    set_paired_rng(seed, device);
    auto second = mtf::MtfJepaMaeVicreg(anchor_config);
    const bool first_metadata = oca_load_archive(
        anchor_path, first, device, seed, config_hash);
    const bool second_metadata = oca_load_archive(
        anchor_path, second, device, seed, config_hash);
    custody.anchor_replay =
        custody.anchor_replay && first_metadata && second_metadata &&
        oca_state_exact(second, oca_snapshot_state(first)) &&
        oca_capture_exact(
            oca_capture_structured(first, data.development, device),
            oca_capture_structured(second, data.development, device));
    inventory.anchors[seed_index] = first;

    const auto current_path =
        oca_seed_cache_path("anchor_challenge", seed);
    custody.current_cache_hashes =
        custody.current_cache_hashes &&
        gpv_sha256_file(current_path) == kGpvCurrentCacheSha256[seed_index];
    OcaInterleavedTrainingResult current{};
    const bool loaded = oca_load_seed_cache(
        "anchor_challenge", data.ssl, device, seed, challenge_masks,
        kOcaAnchorChallengeSteps, /*load_certified_anchor=*/true, current);
    const bool exact =
        loaded && current.pass && current.models.size() == 5 &&
        current.receipts.size() == 5 &&
        current.initialization_exact.size() == 5 &&
        kGpvOcaChallengeMasks[3] == 8U &&
        current.initialization_exact[3] && current.receipts[3].pass &&
        current.receipts[3].steps == kGpvSteps;
    custody.current_caches = custody.current_caches && exact;
    if (exact) {
      inventory.current[seed_index] = current.models[3];
      inventory.current_receipts[seed_index] = current.receipts[3];
    }
  }
  inventory.pass = custody.anchors && custody.anchor_replay &&
                   custody.current_cache_hashes && custody.current_caches;
  return inventory;
}

[[nodiscard]] std::pair<GpvCustody, GpvInventory>
gpv_validate_custody(const RmcData &data, const torch::Device &device) {
  GpvCustody custody{};
  custody.protocol =
      gpv_sha256_file(std::filesystem::path(kGpvProtocolPath)) ==
      kGpvProtocolSha256;
  custody.recovery =
      gpv_sha256_file(std::filesystem::path(kGpvRecoveryPath)) ==
      kGpvRecoverySha256;
  custody.recovery_marker =
      rmc_read_file(std::filesystem::path(kGpvRecoveryMarkerPath)) ==
      std::string(kGpvRecoverySha256) + "  " +
          std::filesystem::path(kGpvRecoveryPath).filename().string() + "\n";
  const std::filesystem::path legacy_archive(kGpvLegacyV1CachePath);
  auto legacy_marker = legacy_archive;
  legacy_marker += ".sha256";
  const auto exact_regular = [](const std::filesystem::path &path) {
    return std::filesystem::symlink_status(path).type() ==
           std::filesystem::file_type::regular;
  };
  custody.legacy_v1_archive =
      exact_regular(legacy_archive) &&
      std::filesystem::file_size(legacy_archive) == 449779U &&
      gpv_sha256_file(legacy_archive) == kGpvLegacyV1CacheSha256;
  custody.legacy_v1_marker =
      exact_regular(legacy_marker) &&
      gpv_sha256_file(legacy_marker) == kGpvLegacyV1MarkerSha256 &&
      rmc_read_file(legacy_marker) ==
          std::string(kGpvLegacyV1CacheSha256) + "\n";
  std::size_t legacy_entries = 0;
  bool legacy_names_exact = true;
  for (const auto &entry :
       std::filesystem::directory_iterator(legacy_archive.parent_path())) {
    const auto name = entry.path().filename().string();
    if (name.find("_v1.complete.pt") == std::string::npos) {
      continue;
    }
    ++legacy_entries;
    legacy_names_exact =
        legacy_names_exact && entry.symlink_status().type() ==
                                  std::filesystem::file_type::regular &&
        (entry.path() == legacy_archive || entry.path() == legacy_marker);
  }
  custody.legacy_v1_inventory =
      legacy_entries == 2 && legacy_names_exact && custody.legacy_v1_archive &&
      custody.legacy_v1_marker;
  const std::filesystem::path legacy_log =
      ".build/tests/representation_gpv1_v1_authoritative.log";
  auto legacy_log_marker = legacy_log;
  legacy_log_marker += ".sha256";
  custody.legacy_v1_log_absent =
      std::filesystem::symlink_status(legacy_log).type() ==
          std::filesystem::file_type::not_found &&
      std::filesystem::symlink_status(legacy_log_marker).type() ==
          std::filesystem::file_type::not_found;
  custody.vva_protocol =
      gpv_sha256_file(std::filesystem::path(kGpvVvaProtocolPath)) ==
      kGpvVvaProtocolSha256;
  custody.vva_findings =
      gpv_sha256_file(std::filesystem::path(kGpvVvaFindingsPath)) ==
      kGpvVvaFindingsSha256;
  custody.vva_harness =
      gpv_sha256_file(std::filesystem::path(kGpvVvaHarnessPath)) ==
      kGpvVvaHarnessSha256;
  const auto vva_log = rmc_read_file(std::filesystem::path(kGpvVvaLogPath));
  custody.vva_log = digest::sha256_hex(vva_log) == kGpvVvaLogSha256;
  custody.vva_fields =
      gpv_line_count(vva_log, "current_profile_retrained=false") &&
      gpv_line_count(vva_log, "vva1.development.selected=none") &&
      gpv_line_count(vva_log, "vva1.production_defaults_changed=false") &&
      gpv_line_count(
          vva_log,
          "vva1.global_pool_projector_variance_attribution_authorized=true") &&
      gpv_line_count(vva_log, "execution_status=vva1_measurements_complete");
  const auto oca_log = rmc_read_file(std::filesystem::path(kGpvOcaLogPath));
  custody.oca_log = digest::sha256_hex(oca_log) == kGpvOcaLogSha256;
  custody.oca_fields =
      gpv_line_count(oca_log, "outer_augmentation_calls=0") &&
      gpv_line_count(oca_log,
                     "oca1.verdict.vicreg=harmful_at_certified_boundary") &&
      gpv_line_count(oca_log, "execution_status=oca1_measurements_complete");
  custody.module =
      gpv_sha256_file(std::filesystem::path(kGpvModulePath)) ==
      kGpvModuleSha256;
  auto inventory = gpv_load_inventory(data, device, custody);
  custody.pass = custody.protocol && custody.recovery &&
                  custody.recovery_marker && custody.legacy_v1_archive &&
                  custody.legacy_v1_marker && custody.legacy_v1_inventory &&
                  custody.legacy_v1_log_absent &&
                  custody.vva_protocol &&
                 custody.vva_findings && custody.vva_log &&
                 custody.vva_harness && custody.vva_fields &&
                 custody.oca_log && custody.oca_fields && custody.module &&
                 inventory.pass;
  return {custody, inventory};
}

void gpv_emit_custody(const GpvCustody &value) {
  std::cout << "gpv1.custody.protocol=" << value.protocol << '\n';
  std::cout << "gpv1.custody.codec_recovery=" << value.recovery << '\n';
  std::cout << "gpv1.custody.codec_recovery_marker="
            << value.recovery_marker << '\n';
  std::cout << "gpv1.custody.legacy_v1_archive="
            << value.legacy_v1_archive << '\n';
  std::cout << "gpv1.custody.legacy_v1_marker=" << value.legacy_v1_marker
            << '\n';
  std::cout << "gpv1.custody.legacy_v1_inventory="
            << value.legacy_v1_inventory << '\n';
  std::cout << "gpv1.custody.legacy_v1_log_absent="
            << value.legacy_v1_log_absent << '\n';
  std::cout << "gpv1.legacy_v1_reused=false\n";
  std::cout << "gpv1.custody.vva_protocol=" << value.vva_protocol << '\n';
  std::cout << "gpv1.custody.vva_findings=" << value.vva_findings << '\n';
  std::cout << "gpv1.custody.vva_log=" << value.vva_log << '\n';
  std::cout << "gpv1.custody.vva_harness=" << value.vva_harness << '\n';
  std::cout << "gpv1.custody.vva_fields=" << value.vva_fields << '\n';
  std::cout << "gpv1.custody.oca_log=" << value.oca_log << '\n';
  std::cout << "gpv1.custody.oca_fields=" << value.oca_fields << '\n';
  std::cout << "gpv1.custody.module=" << value.module << '\n';
  std::cout << "gpv1.custody.anchors=" << value.anchors << '\n';
  std::cout << "gpv1.custody.anchor_replay=" << value.anchor_replay << '\n';
  std::cout << "gpv1.custody.current_cache_hashes="
            << value.current_cache_hashes << '\n';
  std::cout << "gpv1.custody.current_caches=" << value.current_caches << '\n';
  std::cout << "gpv1.custody.pass=" << value.pass << '\n';
}

[[nodiscard]] std::vector<torch::Tensor>
gpv_named_parameter_tensors(const mtf::MtfJepaMaeVicreg &model,
                            std::string_view prefix) {
  std::vector<torch::Tensor> result;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    if (item.value().requires_grad() && item.key().rfind(prefix, 0) == 0) {
      result.push_back(item.value());
    }
  }
  if (result.empty()) {
    throw std::runtime_error("GPV named parameter partition is empty");
  }
  return result;
}

[[nodiscard]] std::vector<torch::Tensor>
gpv_trunk_parameters(const mtf::MtfJepaMaeVicreg &model) {
  std::vector<torch::Tensor> result;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    const bool trunk = item.key().rfind("tokenizer.", 0) == 0 ||
                       item.key().rfind("encoder.", 0) == 0;
    if (item.value().requires_grad() && trunk) {
      result.push_back(item.value());
    }
  }
  if (result.empty()) {
    throw std::runtime_error("GPV served-trunk parameter set is empty");
  }
  return result;
}

[[nodiscard]] std::vector<torch::Tensor> gpv_gradients(
    const torch::Tensor &loss, const std::vector<torch::Tensor> &parameters,
    bool retain_graph = true) {
  auto gradients = torch::autograd::grad({loss}, parameters, {}, retain_graph,
                                          /*create_graph=*/false,
                                          /*allow_unused=*/true);
  if (gradients.size() != parameters.size()) {
    throw std::runtime_error("GPV autograd result size failed");
  }
  for (std::size_t index = 0; index < gradients.size(); ++index) {
    if (!gradients[index].defined()) {
      gradients[index] = torch::zeros_like(parameters[index]);
    }
    if (!torch::isfinite(gradients[index]).all().item<bool>()) {
      throw std::runtime_error("GPV component gradient is non-finite");
    }
  }
  return gradients;
}

[[nodiscard]] torch::Tensor
gpv_flatten_gradients(const std::vector<torch::Tensor> &gradients) {
  std::vector<torch::Tensor> pieces;
  pieces.reserve(gradients.size());
  for (const auto &gradient : gradients) {
    pieces.push_back(gradient.reshape(-1));
  }
  return torch::cat(pieces);
}

[[nodiscard]] bool gpv_gradient_reconstruction(
    const std::vector<torch::Tensor> &left,
    const std::vector<torch::Tensor> &right,
    const std::vector<torch::Tensor> &reference, double *maximum = nullptr,
    double *relative = nullptr) {
  const auto residual =
      gpv_flatten_gradients(left) - gpv_flatten_gradients(right) -
      gpv_flatten_gradients(reference);
  const auto reference_flat = gpv_flatten_gradients(reference);
  const double max_abs = residual.abs().max().item<double>();
  const double relative_l2 =
      residual.norm().item<double>() /
      std::max(1.0e-30, reference_flat.norm().item<double>());
  if (maximum != nullptr) {
    *maximum = max_abs;
  }
  if (relative != nullptr) {
    *relative = relative_l2;
  }
  return max_abs <= kGpvReconstructionAbs &&
         relative_l2 <= kGpvReconstructionRelative;
}

[[nodiscard]] bool gpv_gradients_close(
    const std::vector<torch::Tensor> &left,
    const std::vector<torch::Tensor> &right,
    double maximum_tolerance = kGpvReconstructionAbs,
    double relative_tolerance = kGpvReconstructionRelative) {
  if (left.size() != right.size()) {
    return false;
  }
  const auto left_flat = gpv_flatten_gradients(left);
  const auto right_flat = gpv_flatten_gradients(right);
  const auto residual = left_flat - right_flat;
  return residual.abs().max().item<double>() <= maximum_tolerance &&
         residual.norm().item<double>() /
                 std::max(1.0e-30, left_flat.norm().item<double>()) <=
             relative_tolerance;
}

[[nodiscard]] GpvObjective gpv_manual_slot_objective(
    const GpvObjective &route, const torch::Tensor &mask_a,
    const torch::Tensor &mask_b,
    const mtf::mtf_jepa_mae_vicreg_config_t &config, bool head_only) {
  if (route.projected_a.size(1) != kGpvCanonicalSlots ||
      mask_a.size(1) != kGpvCanonicalSlots) {
    throw std::runtime_error("GPV manual slot inventory failed");
  }
  mtf::vicreg_stability_loss_options_t options{};
  options.invariance_weight = config.vicreg_sim_weight;
  options.variance_weight = config.vicreg_var_weight;
  options.covariance_weight = config.vicreg_cov_weight;
  options.variance_floor = config.vicreg_variance_floor;
  options.eps = config.vicreg_variance_epsilon;
  GpvObjective result{};
  result.invariance = torch::zeros({}, route.raw.options());
  result.variance = torch::zeros({}, route.raw.options());
  result.covariance = torch::zeros({}, route.raw.options());
  for (int64_t slot = 0; slot < kGpvCanonicalSlots; ++slot) {
    const auto attached = mtf::compute_vicreg_stability_loss(
        route.projected_a.narrow(1, slot, 1), mask_a.narrow(1, slot, 1),
        route.projected_b.narrow(1, slot, 1), mask_b.narrow(1, slot, 1),
        options);
    const auto variance =
        head_only
            ? mtf::compute_vicreg_stability_loss(
                  route.variance_projected_a.narrow(1, slot, 1),
                  mask_a.narrow(1, slot, 1),
                  route.variance_projected_b.narrow(1, slot, 1),
                  mask_b.narrow(1, slot, 1), options)
            : attached;
    if (attached.valid_rows < 2 || variance.valid_rows < 2) {
      throw std::runtime_error("GPV manual slot needs two joint rows");
    }
    result.invariance = result.invariance + attached.invariance_loss;
    result.variance = result.variance + variance.variance_loss;
    result.covariance = result.covariance + attached.covariance_loss;
    result.valid_rows += attached.valid_rows;
    ++result.active_slots;
  }
  result.invariance = result.invariance / 72.0;
  result.variance = result.variance / 72.0;
  result.covariance = result.covariance / 72.0;
  result.raw = config.vicreg_sim_weight * result.invariance +
               config.vicreg_var_weight * result.variance +
               config.vicreg_cov_weight * result.covariance;
  result.branch = kGpvInnerWeight * result.raw;
  result.total = kGpvOuterWeight * result.branch;
  result.finite = torch::isfinite(torch::stack(
                                      {result.invariance, result.variance,
                                       result.covariance, result.total}))
                      .all()
                      .item<bool>();
  return result;
}

[[nodiscard]] bool gpv_slot_manual_equality(
    mtf::MtfJepaMaeVicreg &model, const torch::Tensor &view_a_data,
    const torch::Tensor &view_a_mask, const torch::Tensor &view_b_data,
    const torch::Tensor &view_b_mask, uint8_t factor_mask) {
  if ((factor_mask & 0x1U) == 0U) {
    throw std::runtime_error("GPV slot manual equality requires P");
  }
  const auto encoded_a = model->encode(view_a_data, view_a_mask);
  const auto encoded_b = model->encode(view_b_data, view_b_mask);
  if (!gpv_canonical_metadata(encoded_a.metadata, model->config()) ||
      !gpv_canonical_metadata(encoded_b.metadata, model->config()) ||
      !gpv_metadata_exact(encoded_a.metadata, encoded_b.metadata)) {
    throw std::runtime_error("GPV manual slot metadata order failed");
  }
  const auto route = gpv_objective_from_pools(
      model, encoded_a.embeddings, encoded_a.token_mask, encoded_b.embeddings,
      encoded_b.token_mask, factor_mask);
  const auto manual = gpv_manual_slot_objective(
      route, encoded_a.token_mask, encoded_b.token_mask, model->config(),
      (factor_mask & 0x4U) != 0U);
  return route.active_slots == kGpvCanonicalSlots &&
         manual.active_slots == kGpvCanonicalSlots && route.finite &&
         manual.finite && gpv_tensor_close(route.invariance, manual.invariance,
                                           1.0e-7) &&
         gpv_tensor_close(route.variance, manual.variance, 1.0e-7) &&
         gpv_tensor_close(route.covariance, manual.covariance, 1.0e-7) &&
         gpv_tensor_close(route.total, manual.total, 1.0e-7);
}

[[nodiscard]] std::pair<bool, bool> gpv_archive_codec_self_test() {
  const auto f64_first =
      torch::tensor({1.25, -2.5, 9.0}, torch::kFloat64);
  const std::vector<uint64_t> hashes{0U, 1U, 0xffffffffffffffffULL,
                                     0x6f626a5f6d61736bULL};
  const auto u8 = oca_u64_le_bytes_tensor(hashes);
  const auto i64_first = torch::tensor({1, 0, 1, 1}, torch::kInt64);
  const auto f64_second = torch::tensor({3.5, 4.5}, torch::kFloat64);
  const auto i64_second = torch::tensor({512, 512, 512}, torch::kInt64);

  torch::serialize::OutputArchive output;
  output.write("f64_first", f64_first);
  output.write("u8", u8);
  output.write("i64_first", i64_first);
  output.write("f64_second", f64_second);
  output.write("i64_second", i64_second);
  std::stringstream buffer(std::ios::in | std::ios::out | std::ios::binary);
  output.save_to(buffer);
  buffer.seekg(0);
  torch::serialize::InputArchive input;
  input.load_from(buffer, torch::Device(torch::kCPU));

  const auto loaded_f64_first = gpv_read_tensor(input, "f64_first");
  const auto loaded_u8 = gpv_read_tensor(input, "u8");
  const auto loaded_i64_first = gpv_read_tensor(input, "i64_first");
  const auto loaded_f64_second = gpv_read_tensor(input, "f64_second");
  const auto loaded_i64_second = gpv_read_tensor(input, "i64_second");
  gpv_require_archive_tensor(loaded_f64_first, torch::kFloat64, 1, 3,
                             "self_test/f64_first");
  gpv_require_archive_tensor(loaded_u8, torch::kUInt8, 1, 32,
                             "self_test/u8");
  gpv_require_archive_tensor(loaded_i64_first, torch::kInt64, 1, 4,
                             "self_test/i64_first");
  gpv_require_archive_tensor(loaded_f64_second, torch::kFloat64, 1, 2,
                             "self_test/f64_second");
  gpv_require_archive_tensor(loaded_i64_second, torch::kInt64, 1, 3,
                             "self_test/i64_second");
  const bool round_trip =
      gpv_tensor_exact(loaded_f64_first, f64_first) &&
      oca_u64_le_bytes_vector(loaded_u8) == hashes &&
      gpv_tensor_exact(loaded_i64_first, i64_first) &&
      gpv_tensor_exact(loaded_f64_second, f64_second) &&
      gpv_tensor_exact(loaded_i64_second, i64_second);

  bool wrong_dtype_rejected = false;
  bool wrong_shape_rejected = false;
  try {
    gpv_require_archive_tensor(loaded_i64_first, torch::kFloat64, 1, 4,
                               "self_test/wrong_dtype");
  } catch (const std::runtime_error &) {
    wrong_dtype_rejected = true;
  }
  try {
    gpv_require_archive_tensor(loaded_u8.reshape({4, 8}), torch::kUInt8, 1,
                               32, "self_test/wrong_shape");
  } catch (const std::runtime_error &) {
    wrong_shape_rejected = true;
  }
  return {round_trip, wrong_dtype_rejected && wrong_shape_rejected};
}

[[nodiscard]] bool gpv_cpu_self_test() {
  const torch::Device device(torch::kCPU);
  set_paired_rng(17031, device);
  auto model = mtf::MtfJepaMaeVicreg(gpv_config(device));
  model->train();
  auto pool_a = torch::randn({12, kGpvCanonicalSlots, kLatentDim},
                             torch::TensorOptions().dtype(torch::kFloat32))
                    .set_requires_grad(true);
  auto pool_b = (pool_a.detach() + 0.01 * torch::randn_like(pool_a))
                    .set_requires_grad(true);
  auto mask = torch::ones({12, kGpvCanonicalSlots}, torch::kBool);
  bool shapes = true;
  bool finite = true;
  for (uint8_t factor = 0; factor < 8U; ++factor) {
    const bool slot = (factor & 0x1U) != 0U;
    const auto selected_a = slot ? pool_a : pool_a.mean(1, true);
    const auto selected_b = slot ? pool_b : pool_b.mean(1, true);
    const auto selected_mask =
        slot ? mask : torch::ones({12, 1}, torch::kBool);
    const auto value = gpv_objective_from_pools(
        model, selected_a, selected_mask, selected_b, selected_mask, factor);
    shapes = shapes && value.projected_a.size(-1) == 64 &&
             value.projected_a.size(1) == (slot ? 72 : 1);
    finite = finite && value.finite && value.total.item<double>() > 0.0;
  }

  const auto slot_route =
      gpv_objective_from_pools(model, pool_a, mask, pool_b, mask, 1U);
  const auto slot_manual = gpv_manual_slot_objective(
      slot_route, mask, mask, model->config(), false);
  const bool fixed_divisor =
      slot_route.active_slots == 72 && slot_manual.active_slots == 72 &&
      gpv_tensor_close(slot_route.total, slot_manual.total, 1.0e-7);

  const auto coupled =
      gpv_objective_from_pools(model, pool_a, mask, pool_b, mask, 3U);
  const auto head_only =
      gpv_objective_from_pools(model, pool_a, mask, pool_b, mask, 7U);
  const auto head_parameters =
      gpv_named_parameter_tensors(model, "vicreg_stability_head.");
  const auto trunk_parameters = std::vector<torch::Tensor>{pool_a, pool_b};
  const auto coupled_head =
      gpv_gradients(coupled.total, head_parameters, true);
  const auto detached_head =
      gpv_gradients(head_only.total, head_parameters, true);
  const auto coupled_trunk =
      gpv_gradients(coupled.total, trunk_parameters, true);
  const auto detached_trunk =
      gpv_gradients(head_only.total, trunk_parameters, true);
  const auto variance_trunk_gradient = gpv_gradients(
      kGpvOuterWeight * kGpvInnerWeight * 25.0 * coupled.variance,
      trunk_parameters, true);
  const auto head_only_variance_trunk = gpv_gradients(
      kGpvOuterWeight * kGpvInnerWeight * 25.0 * head_only.variance,
      trunk_parameters, true);
  const bool variance_values =
      gpv_tensor_close(coupled.variance, head_only.variance, 1.0e-7) &&
      gpv_gradients_close(coupled_head, detached_head);
  const bool variance_trunk =
      gpv_gradient_reconstruction(coupled_trunk, detached_trunk,
                                  variance_trunk_gradient) &&
      gpv_flatten_gradients(head_only_variance_trunk)
              .abs()
              .max()
              .item<double>() == 0.0;

  const auto layers = gpv_projector_linears(model);
  const bool live_linears = layers.input && layers.hidden && layers.output &&
                             layers.input->weight.size(0) == 64 &&
                             layers.hidden->weight.size(0) == 64 &&
                             layers.output->weight.size(0) == 64;
  const auto make_metadata = [](const std::vector<GpvMetadataTuple> &tuples) {
    std::vector<int64_t> domains;
    std::vector<int64_t> channels;
    std::vector<int64_t> scales;
    std::vector<int64_t> starts;
    std::vector<int64_t> widths;
    for (const auto &tuple : tuples) {
      domains.push_back(tuple[0]);
      channels.push_back(tuple[1]);
      scales.push_back(tuple[2]);
      starts.push_back(tuple[3]);
      widths.push_back(tuple[4]);
    }
    return mtf::mtf_token_metadata_t{
        .start_index = torch::tensor(starts, torch::kInt64),
        .width = torch::tensor(widths, torch::kInt64),
        .scale_id = torch::tensor(scales, torch::kInt64),
        .channel_id = torch::tensor(channels, torch::kInt64),
        .domain_id = torch::tensor(domains, torch::kInt64)};
  };
  const auto expected_metadata =
      gpv_expected_metadata_sequence(model->config());
  const auto canonical_metadata = make_metadata(expected_metadata);
  auto permuted_metadata_tuples = expected_metadata;
  std::swap(permuted_metadata_tuples[0], permuted_metadata_tuples[1]);
  const auto permuted_metadata = make_metadata(permuted_metadata_tuples);
  const bool metadata_order =
      gpv_canonical_metadata(canonical_metadata, model->config()) &&
      !gpv_canonical_metadata(permuted_metadata, model->config());
  const std::vector<uint64_t> codec_input{0U, 1U, 0xffffffffffffffffULL,
                                          0x6f626a5f6d61736bULL};
  const bool codec = oca_u64_le_bytes_vector(
                          oca_u64_le_bytes_tensor(codec_input)) == codec_input;
  const auto [archive_codec, archive_negative] =
      gpv_archive_codec_self_test();

  std::array<double, 8> synthetic{};
  for (uint8_t mask_value = 0; mask_value < 8U; ++mask_value) {
    const double p = (mask_value & 1U) ? 1.0 : 0.0;
    const double j = (mask_value & 2U) ? 1.0 : 0.0;
    const double v = (mask_value & 4U) ? 1.0 : 0.0;
    synthetic[mask_value] = 1.0 + 2.0 * p + 3.0 * j + 5.0 * v +
                            7.0 * p * j + 11.0 * p * v + 13.0 * j * v +
                            17.0 * p * j * v;
  }
  const double p_main = ((synthetic[1] - synthetic[0]) +
                         (synthetic[3] - synthetic[2]) +
                         (synthetic[5] - synthetic[4]) +
                         (synthetic[7] - synthetic[6])) /
                        4.0;
  const double pj = ((synthetic[3] - synthetic[2] - synthetic[1] +
                      synthetic[0]) +
                     (synthetic[7] - synthetic[6] - synthetic[5] +
                      synthetic[4])) /
                    2.0;
  const double pjv = synthetic[7] - synthetic[6] - synthetic[5] -
                     synthetic[3] + synthetic[4] + synthetic[2] +
                     synthetic[1] - synthetic[0];
  const bool factorial = p_main == 15.25 && pj == 15.5 && pjv == 17.0;

  std::cout << std::boolalpha;
  std::cout << "gpv1.self_test.shapes=" << shapes << '\n';
  std::cout << "gpv1.self_test.finite=" << finite << '\n';
  std::cout << "gpv1.self_test.fixed_divisor=" << fixed_divisor << '\n';
  std::cout << "gpv1.self_test.variance_values=" << variance_values << '\n';
  std::cout << "gpv1.self_test.variance_trunk=" << variance_trunk << '\n';
  std::cout << "gpv1.self_test.live_linears=" << live_linears << '\n';
  std::cout << "gpv1.self_test.metadata_order=" << metadata_order << '\n';
  std::cout << "gpv1.self_test.codec=" << codec << '\n';
  std::cout << "gpv1.self_test.archive_codec=" << archive_codec << '\n';
  std::cout << "gpv1.self_test.archive_codec_negative="
            << archive_negative << '\n';
  std::cout << "gpv1.self_test.factorial=" << factorial << '\n';
  return shapes && finite && fixed_divisor && variance_values &&
         variance_trunk && live_linears && metadata_order && codec &&
         archive_codec && archive_negative && factorial;
}

[[nodiscard]] mtf::MtfJepaMaeVicreg
gpv_clone_model(mtf::MtfJepaMaeVicreg &reference,
                const torch::Device &device) {
  auto result = mtf::MtfJepaMaeVicreg(gpv_config(device));
  oca_copy_model_state(reference, result, device);
  return result;
}

[[nodiscard]] bool gpv_selected_parameter_name(std::string_view name) {
  return name.rfind("tokenizer.", 0) == 0 ||
         name.rfind("encoder.", 0) == 0 ||
         name.rfind("vicreg_stability_head.", 0) == 0 ||
         name.rfind("target_tokenizer.", 0) == 0 ||
         name.rfind("target_encoder.", 0) == 0;
}

[[nodiscard]] std::vector<std::string> gpv_selected_parameter_names(
    const mtf::MtfJepaMaeVicreg &model) {
  std::vector<std::string> result;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    if (gpv_selected_parameter_name(item.key())) {
      result.push_back(item.key());
    }
  }
  if (result.empty()) {
    throw std::runtime_error("GPV expected parameter inventory is empty");
  }
  return result;
}

[[nodiscard]] bool gpv_model_finite(
    const mtf::MtfJepaMaeVicreg &model) {
  const auto finite = [](const torch::Tensor &tensor) {
    return tensor.defined() &&
           (!tensor.is_floating_point() ||
            torch::isfinite(tensor).all().item<bool>());
  };
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    if (!finite(item.value())) {
      return false;
    }
  }
  for (const auto &item : model->named_buffers(/*recurse=*/true)) {
    if (!finite(item.value())) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool gpv_finite_positive(
    const std::vector<double> &values) {
  return !values.empty() &&
         std::all_of(values.begin(), values.end(), [](double value) {
           return std::isfinite(value) && value > 0.0;
         });
}

[[nodiscard]] bool gpv_finite_nonnegative(
    const std::vector<double> &values) {
  return !values.empty() &&
         std::all_of(values.begin(), values.end(), [](double value) {
           return std::isfinite(value) && value >= 0.0;
         });
}

[[nodiscard]] std::pair<std::vector<std::string>, std::vector<double>>
gpv_selected_parameter_deltas(const mtf::MtfJepaMaeVicreg &model,
                              const ParameterSnapshot &initial) {
  const auto parameters = model->named_parameters(/*recurse=*/true);
  if (parameters.size() != initial.names.size()) {
    throw std::runtime_error("GPV parameter delta inventory failed");
  }
  std::vector<std::string> names;
  std::vector<double> deltas;
  std::size_t index = 0;
  for (const auto &item : parameters) {
    if (item.key() != initial.names[index]) {
      throw std::runtime_error("GPV parameter delta order failed");
    }
    if (gpv_selected_parameter_name(item.key())) {
      names.push_back(item.key());
      deltas.push_back(
          (item.value().detach().to(torch::kCPU, torch::kFloat64) -
           initial.values[index])
              .abs()
              .max()
              .item<double>());
    }
    ++index;
  }
  return {names, deltas};
}

[[nodiscard]] bool gpv_head_weight_participation(
    const mtf::MtfJepaMaeVicreg &model,
    const std::vector<torch::Tensor> &gradients) {
  std::size_t index = 0;
  std::size_t weights = 0;
  bool pass = true;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    if (!item.value().requires_grad() ||
        item.key().rfind("vicreg_stability_head.", 0) != 0) {
      continue;
    }
    if (index >= gradients.size()) {
      return false;
    }
    if (item.key().ends_with(".weight")) {
      ++weights;
      pass = pass && gradients[index].norm().item<double>() > 0.0;
    }
    ++index;
  }
  return index == gradients.size() && weights == 3 && pass;
}

void gpv_emit_head_gradients(const std::string &root,
                             const mtf::MtfJepaMaeVicreg &model,
                             const std::vector<torch::Tensor> &gradients) {
  std::size_t index = 0;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    if (!item.value().requires_grad() ||
        item.key().rfind("vicreg_stability_head.", 0) != 0) {
      continue;
    }
    if (index >= gradients.size()) {
      throw std::runtime_error("GPV head-gradient emission order failed");
    }
    std::cout << root << '.' << item.key()
              << ".norm=" << gradients[index].norm().item<double>() << '\n';
    ++index;
  }
  if (index != gradients.size()) {
    throw std::runtime_error("GPV head-gradient emission size failed");
  }
}

[[nodiscard]] bool gpv_gradient_tensors_finite(
    const std::vector<torch::Tensor> &gradients) {
  return !gradients.empty() &&
         std::all_of(gradients.begin(), gradients.end(),
                     [](const torch::Tensor &gradient) {
                       return gradient.defined() &&
                              torch::isfinite(gradient).all().item<bool>();
                     });
}

[[nodiscard]] double gpv_gradient_vector_norm(
    const std::vector<torch::Tensor> &gradients) {
  return gpv_flatten_gradients(gradients).norm().item<double>();
}

[[nodiscard]] GpvFactorGradientAudit gpv_factor_gradient_audit(
    mtf::MtfJepaMaeVicreg &model, const GpvObjective &objective,
    uint8_t factor_mask, const std::string &root) {
  const auto &config = model->config();
  const std::array<std::string_view, 4> names{"invariance", "variance",
                                               "covariance", "total"};
  const std::array<torch::Tensor, 4> losses{
      kGpvOuterWeight * kGpvInnerWeight * config.vicreg_sim_weight *
          objective.invariance,
      kGpvOuterWeight * kGpvInnerWeight * config.vicreg_var_weight *
          objective.variance,
      kGpvOuterWeight * kGpvInnerWeight * config.vicreg_cov_weight *
          objective.covariance,
      objective.total};
  const auto head =
      gpv_named_parameter_tensors(model, "vicreg_stability_head.");
  const auto trunk = gpv_trunk_parameters(model);
  GpvFactorGradientAudit audit{};
  for (std::size_t component = 0; component < losses.size(); ++component) {
    const auto head_gradients = gpv_gradients(losses[component], head, true);
    const auto trunk_gradients = gpv_gradients(losses[component], trunk, true);
    const double head_norm = gpv_gradient_vector_norm(head_gradients);
    const double trunk_norm = gpv_gradient_vector_norm(trunk_gradients);
    const double loss = losses[component].item<double>();
    const bool finite = std::isfinite(loss) && std::isfinite(head_norm) &&
                        std::isfinite(trunk_norm) &&
                        gpv_gradient_tensors_finite(head_gradients) &&
                        gpv_gradient_tensors_finite(trunk_gradients);
    const bool variance = component == 1;
    const bool detached_variance =
        variance && (factor_mask & 0x4U) != 0U;
    const bool head_active = head_norm > 0.0 &&
                             gpv_head_weight_participation(model,
                                                           head_gradients);
    const bool trunk_active = detached_variance ? trunk_norm == 0.0
                                                : trunk_norm > 0.0;
    const bool intended = finite && loss >= 0.0 && head_active && trunk_active;
    audit.finite = audit.finite && finite;
    audit.intended_active = audit.intended_active && intended;
    const std::string component_root =
        root + ".component." + std::string(names[component]);
    std::cout << component_root << ".loss=" << loss << '\n';
    std::cout << component_root << ".head_gradient_norm=" << head_norm
              << '\n';
    std::cout << component_root << ".trunk_gradient_norm=" << trunk_norm
              << '\n';
    std::cout << component_root << ".finite=" << finite << '\n';
    std::cout << component_root << ".intended_active=" << intended << '\n';
  }
  std::cout << root << ".component_gradients_finite=" << audit.finite << '\n';
  std::cout << root << ".intended_gradients_active="
            << audit.intended_active << '\n';
  return audit;
}

[[nodiscard]] GpvDisposableUpdateAudit gpv_disposable_update(
    mtf::MtfJepaMaeVicreg &anchor, const Dataset &ssl,
    const torch::Device &device, int64_t seed, uint8_t factor_mask) {
  auto model = gpv_clone_model(anchor, device);
  model->train();
  const auto initial = snapshot_parameters(model);
  auto parameters = model->parameters();
  torch::optim::Adam optimizer(parameters,
                               torch::optim::AdamOptions(kGpvLearningRate));
  const auto rows = training_rows(ssl, seed, 0);
  const auto indices = torch::tensor(rows, torch::kInt64);
  const auto data = ssl.data.index_select(0, indices).to(device);
  const auto feature_mask = ssl.mask.index_select(0, indices).to(device);
  set_paired_rng(paired_step_seed(seed, 0), device);
  optimizer.zero_grad();
  mtf::mtf_jepa_mae_vicreg_output_t prelude{};
  {
    torch::NoGradGuard no_grad;
    prelude = model->forward(data, feature_mask);
  }
  validate_weak_view_debug_tensors(prelude, data, feature_mask);
  const auto objective = gpv_route_retained_views(
      model, prelude.vicreg_view_a_data,
      prelude.vicreg_view_a_feature_mask, prelude.vicreg_view_b_data,
      prelude.vicreg_view_b_feature_mask, factor_mask);
  objective.total.backward();
  double gradient_square = 0.0;
  bool gradients_finite = true;
  std::size_t gradients_seen = 0;
  for (const auto &parameter : parameters) {
    if (parameter.grad().defined()) {
      gradients_finite =
          gradients_finite &&
          torch::isfinite(parameter.grad()).all().item<bool>();
      gradient_square += parameter.grad().pow(2).sum().item<double>();
      ++gradients_seen;
    }
  }
  const double gradient_norm = std::sqrt(gradient_square);
  const double clip = gradient_norm > kGpvClipNorm
                          ? kGpvClipNorm / gradient_norm
                          : 1.0;
  if (clip < 1.0) {
    for (const auto &parameter : parameters) {
      if (parameter.grad().defined()) {
        parameter.grad().mul_(clip);
      }
    }
  }
  GpvDisposableUpdateAudit result{};
  optimizer.step();
  result.adam_updates = 1;
  const bool ema = model->update_target_network(0.990);
  result.ema_updates = ema ? 1 : 0;
  const auto selected = gpv_selected_parameter_deltas(model, initial);
  const auto expected_names = gpv_selected_parameter_names(model);
  std::size_t head_weights = 0;
  bool head_weights_changed = true;
  bool head_changed = false;
  bool deltas_finite = selected.first == expected_names &&
                       selected.first.size() == selected.second.size();
  const std::string root =
      "gpv1.stage0.seed_" + std::to_string(seed) + ".mask_" +
      std::to_string(factor_mask) + ".shadow_update";
  for (std::size_t index = 0; index < selected.first.size(); ++index) {
    deltas_finite = deltas_finite && std::isfinite(selected.second[index]) &&
                    selected.second[index] >= 0.0;
    if (selected.first[index].rfind("vicreg_stability_head.", 0) == 0) {
      std::cout << root << '.' << selected.first[index]
                << ".delta=" << selected.second[index] << '\n';
      head_changed = head_changed || selected.second[index] > 0.0;
      if (selected.first[index].ends_with(".weight")) {
        ++head_weights;
        head_weights_changed =
            head_weights_changed && selected.second[index] > 0.0;
      }
    }
  }
  const double all_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::all_trainable);
  const double served_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::served);
  const double head_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::vicreg_head);
  const double target_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::target_ema);
  const double predictor_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::predictor);
  const double mae_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::mae_decoder);
  result.gradients_finite =
      objective.finite && objective.canonical_layout &&
      objective.total.item<double>() > 0.0 && gradients_seen > 0 &&
      gradients_finite && std::isfinite(gradient_norm) && gradient_norm > 0.0;
  result.partitions_exact =
      deltas_finite && std::isfinite(all_delta) && all_delta > 0.0 &&
      std::isfinite(served_delta) && served_delta > 0.0 &&
      std::isfinite(head_delta) && head_delta > 0.0 &&
      std::isfinite(target_delta) && target_delta > 0.0 &&
      predictor_delta == 0.0 && mae_delta == 0.0 && head_weights == 3 &&
      head_weights_changed && head_changed;
  result.model_finite = gpv_model_finite(model);
  result.pass = result.adam_updates == 1 && result.ema_updates == 1 &&
                result.gradients_finite && result.partitions_exact &&
                result.model_finite;
  std::cout << root << ".adam_updates=" << result.adam_updates << '\n';
  std::cout << root << ".ema_updates=" << result.ema_updates << '\n';
  std::cout << root << ".gradient_norm=" << gradient_norm << '\n';
  std::cout << root << ".gradients_finite=" << result.gradients_finite
            << '\n';
  std::cout << root << ".partitions_exact=" << result.partitions_exact
            << '\n';
  std::cout << root << ".model_finite=" << result.model_finite << '\n';
  std::cout << root << ".pass=" << result.pass << '\n';
  return result;
}

[[nodiscard]] GpvStageZero gpv_run_stage_zero(
    const RmcData &data, const torch::Device &device,
    GpvInventory &inventory) {
  GpvStageZero result{};
  const auto config_manifest = gpv_config_manifest(device);
  for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
       ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    const auto rows = training_rows(data.ssl, seed, 0);
    const auto indices = torch::tensor(rows, torch::kInt64);
    const auto batch_data = data.ssl.data.index_select(0, indices).to(device);
    const auto batch_mask = data.ssl.mask.index_select(0, indices).to(device);

    auto production = gpv_clone_model(inventory.current[seed_index], device);
    auto custom = gpv_clone_model(inventory.current[seed_index], device);
    production->train();
    custom->train();
    production->zero_grad();
    custom->zero_grad();
    set_paired_rng(paired_step_seed(seed, 0), device);
    const auto production_output = production->forward(batch_data, batch_mask);
    const auto production_post = current_generator_state_snapshot(device);
    production_output.loss.backward();
    const auto production_gradient =
        gradient_vector(production, GradientPartition::all_trainable);

    set_paired_rng(paired_step_seed(seed, 0), device);
    mtf::mtf_jepa_mae_vicreg_output_t custom_prelude{};
    {
      torch::NoGradGuard no_grad;
      custom_prelude = custom->forward(batch_data, batch_mask);
    }
    const auto custom_post = current_generator_state_snapshot(device);
    const auto custom_objective = gpv_route_retained_views(
        custom, custom_prelude.vicreg_view_a_data,
        custom_prelude.vicreg_view_a_feature_mask,
        custom_prelude.vicreg_view_b_data,
        custom_prelude.vicreg_view_b_feature_mask, 0U);
    const auto custom_after_route = current_generator_state_snapshot(device);
    custom_objective.total.backward();
    const auto custom_gradient =
        gradient_vector(custom, GradientPartition::all_trainable);
    const bool parity =
        gpv_tensor_exact(production_output.vicreg_view_a_data,
                         custom_prelude.vicreg_view_a_data) &&
        gpv_tensor_exact(production_output.vicreg_view_a_feature_mask,
                         custom_prelude.vicreg_view_a_feature_mask) &&
        gpv_tensor_exact(production_output.vicreg_view_b_data,
                         custom_prelude.vicreg_view_b_data) &&
        gpv_tensor_exact(production_output.vicreg_view_b_feature_mask,
                         custom_prelude.vicreg_view_b_feature_mask) &&
        gpv_tensor_exact(production_output.jepa_target_mask,
                         custom_prelude.jepa_target_mask) &&
        gpv_tensor_exact(production_output.jepa_context_mask,
                         custom_prelude.jepa_context_mask) &&
        generator_state_snapshot_equal(production_post, custom_post) &&
        generator_state_snapshot_equal(custom_post, custom_after_route) &&
        gpv_production_close(
            production_output.diagnostics.at("vicreg_global_sim_term"),
            custom_objective.invariance) &&
        gpv_production_close(
            production_output.diagnostics.at("vicreg_global_var_term"),
            custom_objective.variance) &&
        gpv_production_close(
            production_output.diagnostics.at("vicreg_global_cov_term"),
            custom_objective.covariance) &&
        gpv_production_close(production_output.loss_vicreg_global,
                             custom_objective.raw) &&
        gpv_production_close(production_output.loss_vicreg,
                             custom_objective.branch) &&
        gpv_production_close(production_output.loss, custom_objective.total) &&
        torch::allclose(production_gradient, custom_gradient,
                        kGpvProductionRtol, kGpvProductionAtol);
    result.current_route_parity = result.current_route_parity && parity;

    auto factor_model = gpv_clone_model(inventory.anchors[seed_index], device);
    factor_model->train();
    set_paired_rng(paired_step_seed(seed, 0), device);
    mtf::mtf_jepa_mae_vicreg_output_t retained{};
    {
      torch::NoGradGuard no_grad;
      retained = factor_model->forward(batch_data, batch_mask);
    }
    const auto retained_post = current_generator_state_snapshot(device);
    validate_weak_view_debug_tensors(retained, batch_data, batch_mask);
    const auto retained_digest = weak_view_digest(retained);
    const auto state_before = oca_snapshot_state(factor_model);
    for (uint8_t factor_mask = 0; factor_mask < 8U; ++factor_mask) {
      torch::NoGradGuard no_grad;
      const auto route = gpv_route_retained_views(
          factor_model, retained.vicreg_view_a_data,
          retained.vicreg_view_a_feature_mask, retained.vicreg_view_b_data,
          retained.vicreg_view_b_feature_mask, factor_mask);
      result.retained_views =
          result.retained_views && route.finite && route.canonical_layout &&
          weak_view_digests_equal(retained_digest, weak_view_digest(retained)) &&
          generator_state_snapshot_equal(
              retained_post, current_generator_state_snapshot(device));
      result.projector_identity =
          result.projector_identity && route.projected_a.size(-1) == 64 &&
          canonical_config_manifest(factor_model->config()) ==
              config_manifest &&
          gpv_factor_manifest(factor_mask).find("objective_mask=8") !=
              std::string::npos;
      if ((factor_mask & 0x1U) != 0U) {
        result.canonical_slots =
            result.canonical_slots && route.active_slots == 72 &&
            gpv_slot_manual_equality(
                factor_model, retained.vicreg_view_a_data,
                retained.vicreg_view_a_feature_mask,
                retained.vicreg_view_b_data,
                retained.vicreg_view_b_feature_mask, factor_mask);
      }
    }
    result.projector_identity =
        result.projector_identity && oca_state_exact(factor_model, state_before);

    for (uint8_t pool_policy = 0; pool_policy < 2U; ++pool_policy) {
      for (uint8_t activation_policy = 0; activation_policy < 2U;
           ++activation_policy) {
        auto probe = gpv_clone_model(inventory.anchors[seed_index], device);
        probe->train();
        const auto encoded_a = probe->encode(
            retained.vicreg_view_a_data,
            retained.vicreg_view_a_feature_mask);
        const auto encoded_b = probe->encode(
            retained.vicreg_view_b_data,
            retained.vicreg_view_b_feature_mask);
        const bool exact_metadata =
            gpv_canonical_metadata(encoded_a.metadata, probe->config()) &&
            gpv_canonical_metadata(encoded_b.metadata, probe->config()) &&
            gpv_metadata_exact(encoded_a.metadata, encoded_b.metadata);
        result.canonical_slots = result.canonical_slots && exact_metadata;
        const bool slots = pool_policy != 0U;
        const auto pool_a = slots ? encoded_a.embeddings
                                  : encoded_a.pooled_embedding.unsqueeze(1);
        const auto pool_b = slots ? encoded_b.embeddings
                                  : encoded_b.pooled_embedding.unsqueeze(1);
        const auto valid_a = slots ? encoded_a.token_mask
                                   : encoded_a.sample_valid_mask.unsqueeze(1);
        const auto valid_b = slots ? encoded_b.token_mask
                                   : encoded_b.sample_valid_mask.unsqueeze(1);
        const uint8_t base_mask = static_cast<uint8_t>(
            pool_policy | static_cast<uint8_t>(activation_policy << 1U));
        const uint8_t head_only_mask =
            static_cast<uint8_t>(base_mask | 0x4U);
        const auto coupled = gpv_objective_from_pools(
            probe, pool_a, valid_a, pool_b, valid_b, base_mask);
        const auto head_only = gpv_objective_from_pools(
            probe, pool_a, valid_a, pool_b, valid_b, head_only_mask);
        const std::string root =
            "gpv1.stage0.seed_" + std::to_string(seed) + ".p_" +
            std::to_string(pool_policy) + ".j_" +
            std::to_string(activation_policy);
        const auto coupled_audit = gpv_factor_gradient_audit(
            probe, coupled, base_mask,
            root + ".mask_" + std::to_string(base_mask));
        const auto head_only_audit = gpv_factor_gradient_audit(
            probe, head_only, head_only_mask,
            root + ".mask_" + std::to_string(head_only_mask));
        result.component_gradients_finite =
            result.component_gradients_finite && coupled_audit.finite &&
            head_only_audit.finite;
        result.intended_gradients_active =
            result.intended_gradients_active &&
            coupled_audit.intended_active && head_only_audit.intended_active;
        const auto head_parameters = gpv_named_parameter_tensors(
            probe, "vicreg_stability_head.");
        const auto trunk_parameters = gpv_trunk_parameters(probe);
        const auto coupled_head =
            gpv_gradients(coupled.total, head_parameters, true);
        const auto head_only_head =
            gpv_gradients(head_only.total, head_parameters, true);
        const auto coupled_trunk =
            gpv_gradients(coupled.total, trunk_parameters, true);
        const auto head_only_trunk =
            gpv_gradients(head_only.total, trunk_parameters, true);
        const auto effective_variance_trunk = gpv_gradients(
            kGpvOuterWeight * kGpvInnerWeight * 25.0 * coupled.variance,
            trunk_parameters, true);
        const auto detached_variance_trunk = gpv_gradients(
            kGpvOuterWeight * kGpvInnerWeight * 25.0 * head_only.variance,
            trunk_parameters, true);
        result.variance_value_and_head_gradient =
            result.variance_value_and_head_gradient && coupled.finite &&
            head_only.finite &&
            gpv_tensor_close(coupled.projected_a, head_only.projected_a) &&
            gpv_tensor_close(coupled.projected_b, head_only.projected_b) &&
            gpv_tensor_close(coupled.projected_a,
                             head_only.variance_projected_a) &&
            gpv_tensor_close(coupled.projected_b,
                             head_only.variance_projected_b) &&
            gpv_tensor_close(coupled.invariance, head_only.invariance) &&
            gpv_tensor_close(coupled.variance, head_only.variance) &&
            gpv_tensor_close(coupled.covariance, head_only.covariance) &&
            gpv_tensor_close(coupled.total, head_only.total) &&
            gpv_gradients_close(coupled_head, head_only_head) &&
            gpv_head_weight_participation(probe, coupled_head) &&
            gpv_head_weight_participation(probe, head_only_head);
        double reconstruction_max = 0.0;
        double reconstruction_relative = 0.0;
        const bool reconstruction = gpv_gradient_reconstruction(
            coupled_trunk, head_only_trunk, effective_variance_trunk,
            &reconstruction_max, &reconstruction_relative);
        const bool detached_zero =
            gpv_flatten_gradients(detached_variance_trunk)
                .abs()
                .max()
                .item<double>() == 0.0;
        const bool effective_nonzero =
            gpv_gradient_tensors_finite(effective_variance_trunk) &&
            gpv_gradient_vector_norm(effective_variance_trunk) > 0.0;
        const bool detached_finite =
            gpv_gradient_tensors_finite(detached_variance_trunk);
        result.variance_trunk_decomposition =
            result.variance_trunk_decomposition && reconstruction &&
            effective_nonzero && detached_finite && detached_zero;
        gpv_emit_head_gradients(root + ".coupled_head_gradient", probe,
                                coupled_head);
        gpv_emit_head_gradients(root + ".head_only_head_gradient", probe,
                                head_only_head);
        std::cout << root << ".trunk_reconstruction_max_abs="
                  << reconstruction_max << '\n';
        std::cout << root << ".trunk_reconstruction_relative_l2="
                  << reconstruction_relative << '\n';
      }
    }

    for (uint8_t factor_mask = 0; factor_mask < 8U; ++factor_mask) {
      const auto disposable = gpv_disposable_update(
          inventory.anchors[seed_index], data.ssl, device, seed, factor_mask);
      ++result.disposable_clones;
      result.disposable_adam_updates += disposable.adam_updates;
      result.disposable_ema_updates += disposable.ema_updates;
      result.disposable_updates = result.disposable_updates && disposable.pass;
      result.finite_and_active =
          result.finite_and_active && disposable.gradients_finite &&
          disposable.model_finite;
      result.inactive_partitions =
          result.inactive_partitions && disposable.partitions_exact;
    }
  }
  const auto config = gpv_config(device);
  result.view_dose =
      std::abs(mtf::detail::resolved_vicreg_view_time_dropout_prob(config) -
               0.01) <= 1.0e-12 &&
      config.vicreg_view_gaussian_jitter_std == 0.005 &&
      config.feature_dropout_prob == 0.0;
  const bool disposable_counts_exact =
      result.disposable_clones == 24 &&
      result.disposable_adam_updates == 24 &&
      result.disposable_ema_updates == 24;
  result.disposable_updates =
      result.disposable_updates && disposable_counts_exact;
  result.pass = result.current_route_parity && result.canonical_slots &&
                 result.retained_views && result.projector_identity &&
                 result.variance_value_and_head_gradient &&
                 result.variance_trunk_decomposition &&
                 result.component_gradients_finite &&
                 result.intended_gradients_active &&
                 result.disposable_updates &&
                 result.finite_and_active && result.inactive_partitions &&
                 result.view_dose;
  return result;
}

void gpv_emit_stage_zero(const GpvStageZero &value) {
  std::cout << "gpv1.stage0.current_route_parity="
            << value.current_route_parity << '\n';
  std::cout << "gpv1.stage0.canonical_slots=" << value.canonical_slots
            << '\n';
  std::cout << "gpv1.stage0.retained_views=" << value.retained_views << '\n';
  std::cout << "gpv1.stage0.projector_identity="
            << value.projector_identity << '\n';
  std::cout << "gpv1.stage0.variance_value_and_head_gradient="
            << value.variance_value_and_head_gradient << '\n';
  std::cout << "gpv1.stage0.variance_trunk_decomposition="
            << value.variance_trunk_decomposition << '\n';
  std::cout << "gpv1.stage0.component_gradients_finite="
            << value.component_gradients_finite << '\n';
  std::cout << "gpv1.stage0.intended_gradients_active="
            << value.intended_gradients_active << '\n';
  std::cout << "gpv1.stage0.disposable_updates="
            << value.disposable_updates << '\n';
  std::cout << "gpv1.stage0.disposable_clones="
            << value.disposable_clones << '\n';
  std::cout << "gpv1.stage0.disposable_adam_updates="
            << value.disposable_adam_updates << '\n';
  std::cout << "gpv1.stage0.disposable_ema_updates="
            << value.disposable_ema_updates << '\n';
  std::cout << "gpv1.stage0.finite_and_active="
            << value.finite_and_active << '\n';
  std::cout << "gpv1.stage0.inactive_partitions="
            << value.inactive_partitions << '\n';
  std::cout << "gpv1.stage0.view_dose=" << value.view_dose << '\n';
  std::cout << "gpv1.stage0.authoritative_optimizer_updates=0\n";
  std::cout << "gpv1.stage0.pass=" << value.pass << '\n';
}

[[nodiscard]] std::filesystem::path gpv_cell_path(int64_t seed,
                                                  uint8_t mask) {
  return std::filesystem::path(".build") / "tests" / "gpv1" /
         ("seed_" + std::to_string(seed) + "_mask_" +
          std::to_string(mask) + "_v2.complete.pt");
}

[[nodiscard]] std::filesystem::path
gpv_cell_marker_path(const std::filesystem::path &path) {
  auto result = path;
  result += ".sha256";
  return result;
}

[[nodiscard]] bool
gpv_v2_namespace_exact(const GpvCachePresence &cache_present) {
  const std::filesystem::path directory =
      std::filesystem::path(".build") / "tests" / "gpv1";
  std::map<std::string, bool> expected;
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    for (uint8_t mask = 1U; mask < 8U; ++mask) {
      const auto archive =
          gpv_cell_path(kAttributionSeeds[seed_index], mask).filename().string();
      const auto marker = archive + ".sha256";
      expected.emplace(archive, cache_present[seed_index][mask]);
      expected.emplace(marker, cache_present[seed_index][mask]);
    }
  }
  std::map<std::string, bool> discovered;
  for (const auto &[name, unused] : expected) {
    (void)unused;
    discovered.emplace(name, false);
  }
  const auto directory_type = std::filesystem::symlink_status(directory).type();
  if (directory_type != std::filesystem::file_type::not_found &&
      directory_type != std::filesystem::file_type::directory) {
    return false;
  }
  if (directory_type == std::filesystem::file_type::directory) {
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
      const auto name = entry.path().filename().string();
      if (name.find("_v2.complete.pt") == std::string::npos) {
        continue;
      }
      if (entry.symlink_status().type() !=
          std::filesystem::file_type::regular) {
        return false;
      }
      const auto position = discovered.find(name);
      if (position == discovered.end() || position->second) {
        return false;
      }
      position->second = true;
    }
  }
  for (const auto &[name, should_exist] : expected) {
    if (discovered.at(name) != should_exist) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] torch::Tensor
gpv_double_tensor(const std::vector<double> &values) {
  if (values.empty()) {
    throw std::runtime_error("GPV double archive vector is empty");
  }
  return torch::tensor(values, torch::kFloat64);
}

[[nodiscard]] torch::Tensor
gpv_i64_tensor(const std::vector<int64_t> &values) {
  if (values.empty()) {
    throw std::runtime_error("GPV int64 archive vector is empty");
  }
  return torch::tensor(values, torch::kInt64);
}

[[nodiscard]] std::vector<double>
gpv_double_vector(const torch::Tensor &input, int64_t expected_count) {
  gpv_require_archive_tensor(input, torch::kFloat64, 1, expected_count,
                             "float64_vector");
  const auto value = input.detach().to(torch::kCPU).contiguous();
  const auto *data = value.data_ptr<double>();
  return {data, data + value.numel()};
}

[[nodiscard]] std::vector<int64_t>
gpv_i64_vector(const torch::Tensor &input, int64_t expected_count) {
  gpv_require_archive_tensor(input, torch::kInt64, 1, expected_count,
                             "int64_vector");
  const auto value = input.detach().to(torch::kCPU).contiguous();
  const auto *data = value.data_ptr<int64_t>();
  return {data, data + value.numel()};
}

[[nodiscard]] std::vector<uint64_t>
gpv_u64_vector(const torch::Tensor &input, int64_t expected_count) {
  gpv_require_archive_tensor(
      input, torch::kUInt8, 1,
      expected_count * static_cast<int64_t>(sizeof(uint64_t)),
      "uint64_le_bytes_vector");
  return oca_u64_le_bytes_vector(input);
}

[[nodiscard]] std::string
gpv_join_lines(const std::vector<std::string> &values) {
  if (values.empty()) {
    throw std::runtime_error("GPV string archive vector is empty");
  }
  std::ostringstream out;
  for (const auto &value : values) {
    if (value.empty() || value.find('\n') != std::string::npos ||
        value.find('\r') != std::string::npos) {
      throw std::runtime_error("GPV archived parameter name is invalid");
    }
    out << value << '\n';
  }
  return out.str();
}

[[nodiscard]] std::vector<std::string>
gpv_split_lines(const std::string &encoded) {
  std::istringstream input(encoded);
  std::vector<std::string> result;
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (!line.empty()) {
      result.push_back(line);
    }
  }
  if (result.empty()) {
    throw std::runtime_error("GPV archived parameter names are empty");
  }
  return result;
}

[[nodiscard]] double gpv_mean(const std::vector<double> &values) {
  if (values.empty()) {
    throw std::runtime_error("GPV mean requires values");
  }
  return std::accumulate(values.begin(), values.end(), 0.0) /
         static_cast<double>(values.size());
}

[[nodiscard]] bool gpv_receipt_shape_valid(const GpvReceipt &receipt) {
  const auto size = static_cast<std::size_t>(kGpvSteps);
  return receipt.steps == kGpvSteps && receipt.adam_steps == kGpvSteps &&
         receipt.ema_steps == kGpvSteps && receipt.losses.size() == size &&
         receipt.gradient_norms.size() == size &&
         receipt.trunk_gradient_norms.size() == size &&
         receipt.head_gradient_norms.size() == size &&
         receipt.clip_factors.size() == size &&
         receipt.served_update_norms.size() == size &&
         std::all_of(receipt.component_losses.begin(),
                     receipt.component_losses.end(),
                     [size](const std::vector<double> &values) {
                       return values.size() == size;
                     }) &&
         receipt.row_hashes.size() == size &&
         receipt.target_mask_hashes.size() == size &&
         receipt.context_mask_hashes.size() == size &&
         receipt.view_a_data_hashes.size() == size &&
         receipt.view_a_mask_hashes.size() == size &&
         receipt.view_b_data_hashes.size() == size &&
         receipt.view_b_mask_hashes.size() == size &&
         receipt.rng_pre_cpu_hashes.size() == size &&
         receipt.rng_pre_cuda_hashes.size() == size &&
         receipt.rng_post_cpu_hashes.size() == size &&
         receipt.rng_post_cuda_hashes.size() == size &&
         receipt.step_flags.size() == size &&
         receipt.parameter_delta_names.size() ==
             receipt.parameter_deltas.size() &&
         !receipt.parameter_delta_names.empty();
}

[[nodiscard]] bool gpv_receipt_loss_reconstruction_valid(
    const GpvReceipt &receipt, double *maximum_absolute = nullptr,
    double *maximum_relative = nullptr) {
  const std::size_t size = receipt.losses.size();
  if (size == 0 ||
      std::any_of(receipt.component_losses.begin(),
                  receipt.component_losses.end(),
                  [size](const std::vector<double> &values) {
                    return values.size() != size;
                  })) {
    return false;
  }
  double observed_maximum_absolute = 0.0;
  double observed_maximum_relative = 0.0;
  bool pass = true;
  for (std::size_t step = 0; step < size; ++step) {
    const double invariance = receipt.component_losses[0][step];
    const double variance = receipt.component_losses[1][step];
    const double covariance = receipt.component_losses[2][step];
    const double observed = receipt.losses[step];
    const double reconstructed =
        kGpvOuterWeight * kGpvInnerWeight *
        (25.0 * invariance + 25.0 * variance + covariance);
    const double absolute = std::abs(observed - reconstructed);
    const double relative =
        absolute / std::max(1.0e-30, std::abs(reconstructed));
    observed_maximum_absolute =
        std::max(observed_maximum_absolute, absolute);
    observed_maximum_relative =
        std::max(observed_maximum_relative, relative);
    pass = pass && std::isfinite(observed) && observed > 0.0 &&
           std::isfinite(invariance) && invariance >= 0.0 &&
           std::isfinite(variance) && variance >= 0.0 &&
           std::isfinite(covariance) && covariance >= 0.0 &&
           std::isfinite(reconstructed) && std::isfinite(absolute) &&
           std::isfinite(relative) && absolute <= kGpvReconstructionAbs &&
           relative <= kGpvReconstructionRelative;
  }
  if (maximum_absolute != nullptr) {
    *maximum_absolute = observed_maximum_absolute;
  }
  if (maximum_relative != nullptr) {
    *maximum_relative = observed_maximum_relative;
  }
  return pass;
}

[[nodiscard]] double gpv_gradient_norm(
    const mtf::MtfJepaMaeVicreg &model, bool trunk_only, bool head_only) {
  double square = 0.0;
  std::size_t included = 0;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    const bool trunk = item.key().rfind("tokenizer.", 0) == 0 ||
                       item.key().rfind("encoder.", 0) == 0;
    const bool head = item.key().rfind("vicreg_stability_head.", 0) == 0;
    const bool include = (!trunk_only && !head_only) ||
                         (trunk_only && trunk) || (head_only && head);
    if (!item.value().requires_grad() || !include) {
      continue;
    }
    ++included;
    if (item.value().grad().defined()) {
      if (!torch::isfinite(item.value().grad()).all().item<bool>()) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      square += item.value().grad().detach().pow(2).sum().item<double>();
    }
  }
  if (included == 0) {
    throw std::runtime_error("GPV gradient norm partition is empty");
  }
  return std::sqrt(square);
}

void gpv_remove_interrupted_temporaries(const std::filesystem::path &path) {
  const auto directory_type =
      std::filesystem::symlink_status(path.parent_path()).type();
  if (directory_type == std::filesystem::file_type::not_found) {
    return;
  }
  if (directory_type != std::filesystem::file_type::directory) {
    throw std::runtime_error("GPV cache directory is not an exact directory");
  }
  const auto archive_prefix = path.filename().string() + ".tmp.";
  const auto marker_prefix =
      gpv_cell_marker_path(path).filename().string() + ".tmp.";
  for (const auto &entry :
       std::filesystem::directory_iterator(path.parent_path())) {
    const auto name = entry.path().filename().string();
    if (name.rfind(archive_prefix, 0) == 0 ||
        name.rfind(marker_prefix, 0) == 0) {
      if (entry.symlink_status().type() !=
          std::filesystem::file_type::regular) {
        throw std::runtime_error("GPV interrupted temporary is not regular");
      }
      std::filesystem::remove(entry.path());
    }
  }
}

void gpv_save_cell(const Dataset &ssl, const torch::Device &device,
                   int64_t seed, uint8_t mask, const GpvBindings &bindings,
                   const GpvCell &cell, std::size_t seed_index) {
  if (mask == 0U || mask >= 8U || !bindings.pass || !cell.model ||
      !cell.receipt.pass || !gpv_receipt_shape_valid(cell.receipt)) {
    throw std::runtime_error("GPV cell save contract failed");
  }
  const auto path = gpv_cell_path(seed, mask);
  const auto marker = gpv_cell_marker_path(path);
  std::filesystem::create_directories(path.parent_path());
  gpv_remove_interrupted_temporaries(path);
  auto archive_type = std::filesystem::symlink_status(path).type();
  const auto marker_type = std::filesystem::symlink_status(marker).type();
  if ((archive_type != std::filesystem::file_type::not_found &&
       archive_type != std::filesystem::file_type::regular) ||
      (marker_type != std::filesystem::file_type::not_found &&
       marker_type != std::filesystem::file_type::regular)) {
    throw std::runtime_error("GPV cell save target is not an exact file");
  }
  if (archive_type == std::filesystem::file_type::regular &&
      marker_type == std::filesystem::file_type::not_found) {
    std::filesystem::remove(path);
    archive_type = std::filesystem::file_type::not_found;
  }
  if (archive_type != std::filesystem::file_type::not_found ||
      marker_type != std::filesystem::file_type::not_found) {
    throw std::runtime_error("GPV immutable cell already exists");
  }
  const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
  auto temporary = path;
  temporary += ".tmp." + std::to_string(nonce);
  auto temporary_marker = marker;
  temporary_marker += ".tmp." + std::to_string(nonce);

  torch::serialize::OutputArchive root;
  root.write("meta/schema", oca_string_tensor(kGpvCellCacheSchema));
  root.write("meta/complete", torch::tensor({1}, torch::kInt64));
  root.write("meta/implementation",
             oca_string_tensor(kGpvCellCacheImplementation));
  root.write("meta/protocol_sha256", oca_string_tensor(kGpvProtocolSha256));
  root.write("meta/codec_recovery_sha256",
             oca_string_tensor(kGpvRecoverySha256));
  root.write("meta/legacy_v1_cache_sha256",
             oca_string_tensor(kGpvLegacyV1CacheSha256));
  root.write("meta/legacy_v1_marker_sha256",
             oca_string_tensor(kGpvLegacyV1MarkerSha256));
  root.write("meta/legacy_v1_reused", torch::tensor({0}, torch::kInt64));
  root.write("meta/harness_sha256",
             oca_string_tensor(bindings.harness_sha256));
  root.write("meta/executable_sha256",
             oca_string_tensor(bindings.executable_sha256));
  root.write("meta/module_sha256", oca_string_tensor(kGpvModuleSha256));
  root.write("meta/scientific_manifest_sha256",
             oca_string_tensor(bindings.scientific_manifest_sha256));
  root.write("meta/dataset_sha256",
             oca_string_tensor(bindings.dataset_sha256));
  root.write("meta/splits_sha256",
             oca_string_tensor(bindings.splits_sha256));
  root.write("meta/bootstrap_sha256",
             oca_string_tensor(bindings.bootstrap_sha256));
  root.write("meta/vva_protocol_sha256",
             oca_string_tensor(kGpvVvaProtocolSha256));
  root.write("meta/vva_findings_sha256",
             oca_string_tensor(kGpvVvaFindingsSha256));
  root.write("meta/vva_log_sha256", oca_string_tensor(kGpvVvaLogSha256));
  root.write("meta/vva_harness_sha256",
             oca_string_tensor(kGpvVvaHarnessSha256));
  root.write("meta/oca_log_sha256", oca_string_tensor(kGpvOcaLogSha256));
  root.write("meta/anchor_sha256",
             oca_string_tensor(kGpvAnchorSha256.at(seed_index)));
  root.write("meta/current_cache_sha256",
             oca_string_tensor(kGpvCurrentCacheSha256.at(seed_index)));
  root.write("meta/certificate_id", oca_string_tensor(kGpvCertificateId));
  root.write("meta/readout_policy", oca_string_tensor(kGpvReadoutPolicy));
  root.write("meta/config_manifest", oca_string_tensor(gpv_config_manifest(device)));
  root.write("meta/factor_manifest", oca_string_tensor(gpv_factor_manifest(mask)));
  root.write("meta/seed", torch::tensor({seed}, torch::kInt64));
  root.write("meta/mask", torch::tensor({mask}, torch::kInt64));
  root.write("meta/steps", torch::tensor({kGpvSteps}, torch::kInt64));
  root.write("meta/optimizer_learning_rate",
             torch::tensor({kGpvLearningRate}, torch::kFloat64));
  root.write("meta/gradient_clip_norm",
             torch::tensor({kGpvClipNorm}, torch::kFloat64));
  root.write("meta/target_ema_tau", torch::tensor({0.990}, torch::kFloat64));
  const auto ssl_hashes = oca_seed_cache_ssl_hashes(ssl);
  root.write("meta/ssl_data_hash", oca_string_tensor(ssl_hashes[0]));
  root.write("meta/ssl_mask_hash", oca_string_tensor(ssl_hashes[1]));
  root.write("meta/ssl_target_hash", oca_string_tensor(ssl_hashes[2]));

  torch::serialize::OutputArchive model_archive;
  cell.model->save(model_archive);
  root.write("model", model_archive);
  const auto &receipt = cell.receipt;
  root.write("receipt/losses", gpv_double_tensor(receipt.losses));
  root.write("receipt/gradient_norms",
             gpv_double_tensor(receipt.gradient_norms));
  root.write("receipt/trunk_gradient_norms",
             gpv_double_tensor(receipt.trunk_gradient_norms));
  root.write("receipt/head_gradient_norms",
             gpv_double_tensor(receipt.head_gradient_norms));
  root.write("receipt/clip_factors",
             gpv_double_tensor(receipt.clip_factors));
  root.write("receipt/served_update_norms",
             gpv_double_tensor(receipt.served_update_norms));
  root.write("receipt/invariance_losses",
             gpv_double_tensor(receipt.component_losses[0]));
  root.write("receipt/variance_losses",
             gpv_double_tensor(receipt.component_losses[1]));
  root.write("receipt/covariance_losses",
             gpv_double_tensor(receipt.component_losses[2]));
  root.write("receipt/row_hashes_u64_le",
             oca_u64_le_bytes_tensor(receipt.row_hashes));
  root.write("receipt/target_mask_hashes_u64_le",
             oca_u64_le_bytes_tensor(receipt.target_mask_hashes));
  root.write("receipt/context_mask_hashes_u64_le",
             oca_u64_le_bytes_tensor(receipt.context_mask_hashes));
  root.write("receipt/view_a_data_hashes_u64_le",
             oca_u64_le_bytes_tensor(receipt.view_a_data_hashes));
  root.write("receipt/view_a_mask_hashes_u64_le",
             oca_u64_le_bytes_tensor(receipt.view_a_mask_hashes));
  root.write("receipt/view_b_data_hashes_u64_le",
             oca_u64_le_bytes_tensor(receipt.view_b_data_hashes));
  root.write("receipt/view_b_mask_hashes_u64_le",
             oca_u64_le_bytes_tensor(receipt.view_b_mask_hashes));
  root.write("receipt/rng_pre_cpu_hashes_u64_le",
             oca_u64_le_bytes_tensor(receipt.rng_pre_cpu_hashes));
  root.write("receipt/rng_pre_cuda_hashes_u64_le",
             oca_u64_le_bytes_tensor(receipt.rng_pre_cuda_hashes));
  root.write("receipt/rng_post_cpu_hashes_u64_le",
             oca_u64_le_bytes_tensor(receipt.rng_post_cpu_hashes));
  root.write("receipt/rng_post_cuda_hashes_u64_le",
             oca_u64_le_bytes_tensor(receipt.rng_post_cuda_hashes));
  root.write("receipt/step_flags", gpv_i64_tensor(receipt.step_flags));
  root.write("receipt/parameter_delta_names",
             oca_string_tensor(gpv_join_lines(receipt.parameter_delta_names)));
  root.write("receipt/parameter_deltas",
             gpv_double_tensor(receipt.parameter_deltas));
  root.write(
      "receipt/statistics",
      torch::tensor(
          {receipt.losses.front(), receipt.losses.back(),
           *std::min_element(receipt.losses.begin(), receipt.losses.end()),
           *std::max_element(receipt.losses.begin(), receipt.losses.end()),
           gpv_mean(receipt.losses), receipt.minimum_gradient_norm,
           receipt.maximum_gradient_norm, gpv_mean(receipt.gradient_norms),
           *std::min_element(receipt.trunk_gradient_norms.begin(),
                             receipt.trunk_gradient_norms.end()),
           *std::max_element(receipt.trunk_gradient_norms.begin(),
                             receipt.trunk_gradient_norms.end()),
           gpv_mean(receipt.trunk_gradient_norms),
           *std::min_element(receipt.head_gradient_norms.begin(),
                             receipt.head_gradient_norms.end()),
           *std::max_element(receipt.head_gradient_norms.begin(),
                             receipt.head_gradient_norms.end()),
           gpv_mean(receipt.head_gradient_norms),
           receipt.minimum_served_update_norm,
           receipt.maximum_served_update_norm,
           gpv_mean(receipt.served_update_norms),
           receipt.component_loss_sums[0], receipt.component_loss_sums[1],
           receipt.component_loss_sums[2], receipt.all_trainable_delta,
           receipt.served_delta, receipt.predictor_delta,
           receipt.mae_decoder_delta, receipt.vicreg_head_delta,
           receipt.target_ema_delta},
          torch::kFloat64));
  root.write("receipt/counts",
             torch::tensor({receipt.steps, receipt.adam_steps,
                            receipt.ema_steps, receipt.clipping_count},
                           torch::kInt64));
  root.write("receipt/flags",
             torch::tensor({receipt.initialization_exact ? 1 : 0,
                            receipt.finite ? 1 : 0,
                            receipt.schedule_exact ? 1 : 0,
                            receipt.expected_partitions ? 1 : 0,
                            receipt.pass ? 1 : 0},
                           torch::kInt64));
  root.save_to(temporary.string());
  const auto checksum = gpv_sha256_file(temporary);
  std::filesystem::rename(temporary, path);
  {
    std::ofstream out(temporary_marker, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
      throw std::runtime_error("cannot create GPV cell marker");
    }
    out << checksum << '\n';
    out.close();
    if (!out) {
      throw std::runtime_error("GPV cell marker write failed");
    }
  }
  std::filesystem::rename(temporary_marker, marker);
}

[[nodiscard]] bool gpv_lower_hex_marker(const std::string &value) {
  return value.size() == 65 && value.back() == '\n' &&
         std::all_of(value.begin(), value.begin() + 64, [](char byte) {
           return (byte >= '0' && byte <= '9') ||
                  (byte >= 'a' && byte <= 'f');
         });
}

[[nodiscard]] bool gpv_load_cell(
    const Dataset &ssl, const torch::Device &device, int64_t seed, uint8_t mask,
    mtf::MtfJepaMaeVicreg &anchor, const GpvBindings &bindings,
    std::size_t seed_index, GpvCell &cell) {
  const auto path = gpv_cell_path(seed, mask);
  const auto marker = gpv_cell_marker_path(path);
  gpv_remove_interrupted_temporaries(path);
  const auto archive_type = std::filesystem::symlink_status(path).type();
  const auto marker_type = std::filesystem::symlink_status(marker).type();
  const bool archive_exists =
      archive_type != std::filesystem::file_type::not_found;
  const bool marker_exists = marker_type != std::filesystem::file_type::not_found;
  if ((archive_exists &&
       archive_type != std::filesystem::file_type::regular) ||
      (marker_exists && marker_type != std::filesystem::file_type::regular)) {
    throw std::runtime_error("GPV committed cell is not an exact regular file");
  }
  if (archive_exists && !marker_exists) {
    std::filesystem::remove(path);
    return false;
  }
  if (!archive_exists && marker_exists) {
    throw std::runtime_error("GPV marker exists without final archive");
  }
  if (!archive_exists) {
    return false;
  }
  const auto marker_bytes = rmc_read_file(marker);
  if (!gpv_lower_hex_marker(marker_bytes)) {
    throw std::runtime_error("GPV committed marker format failed");
  }
  const auto expected_checksum = marker_bytes.substr(0, 64);
  if (gpv_sha256_file(path) != expected_checksum) {
    throw std::runtime_error("GPV committed cell checksum failed");
  }

  torch::serialize::InputArchive root;
  root.load_from(path.string(), device);
  const auto read_string = [&](const char *key) {
    const auto value = gpv_read_tensor(root, key);
    if (!value.defined() || value.scalar_type() != torch::kUInt8 ||
        value.dim() != 1 || value.numel() <= 0) {
      throw std::runtime_error(std::string("GPV string shape failed: ") + key);
    }
    return oca_tensor_string(value);
  };
  const auto read_i64_scalar = [&](const char *key) {
    auto value = gpv_read_tensor(root, key);
    gpv_require_archive_tensor(value, torch::kInt64, 1, 1, key);
    value = value.to(torch::kCPU).contiguous();
    return value.item<int64_t>();
  };
  const auto read_f64_scalar = [&](const char *key) {
    auto value = gpv_read_tensor(root, key);
    gpv_require_archive_tensor(value, torch::kFloat64, 1, 1, key);
    value = value.to(torch::kCPU).contiguous();
    return value.item<double>();
  };
  const auto ssl_hashes = oca_seed_cache_ssl_hashes(ssl);
  const bool metadata =
      read_string("meta/schema") == kGpvCellCacheSchema &&
      read_i64_scalar("meta/complete") == 1 &&
      read_string("meta/implementation") == kGpvCellCacheImplementation &&
      read_string("meta/protocol_sha256") == kGpvProtocolSha256 &&
      read_string("meta/codec_recovery_sha256") == kGpvRecoverySha256 &&
      read_string("meta/legacy_v1_cache_sha256") ==
          kGpvLegacyV1CacheSha256 &&
      read_string("meta/legacy_v1_marker_sha256") ==
          kGpvLegacyV1MarkerSha256 &&
      read_i64_scalar("meta/legacy_v1_reused") == 0 &&
      read_string("meta/harness_sha256") == bindings.harness_sha256 &&
      read_string("meta/executable_sha256") == bindings.executable_sha256 &&
      read_string("meta/module_sha256") == kGpvModuleSha256 &&
      read_string("meta/scientific_manifest_sha256") ==
          bindings.scientific_manifest_sha256 &&
      read_string("meta/dataset_sha256") == bindings.dataset_sha256 &&
      read_string("meta/splits_sha256") == bindings.splits_sha256 &&
      read_string("meta/bootstrap_sha256") == bindings.bootstrap_sha256 &&
      read_string("meta/vva_protocol_sha256") == kGpvVvaProtocolSha256 &&
      read_string("meta/vva_findings_sha256") == kGpvVvaFindingsSha256 &&
      read_string("meta/vva_log_sha256") == kGpvVvaLogSha256 &&
      read_string("meta/vva_harness_sha256") == kGpvVvaHarnessSha256 &&
      read_string("meta/oca_log_sha256") == kGpvOcaLogSha256 &&
      read_string("meta/anchor_sha256") == kGpvAnchorSha256.at(seed_index) &&
      read_string("meta/current_cache_sha256") ==
          kGpvCurrentCacheSha256.at(seed_index) &&
      read_string("meta/certificate_id") == kGpvCertificateId &&
      read_string("meta/readout_policy") == kGpvReadoutPolicy &&
      read_string("meta/config_manifest") == gpv_config_manifest(device) &&
      read_string("meta/factor_manifest") == gpv_factor_manifest(mask) &&
      read_i64_scalar("meta/seed") == seed &&
      read_i64_scalar("meta/mask") == mask &&
      read_i64_scalar("meta/steps") == kGpvSteps &&
      read_f64_scalar("meta/optimizer_learning_rate") == kGpvLearningRate &&
      read_f64_scalar("meta/gradient_clip_norm") == kGpvClipNorm &&
      read_f64_scalar("meta/target_ema_tau") == 0.990 &&
      read_string("meta/ssl_data_hash") == ssl_hashes[0] &&
      read_string("meta/ssl_mask_hash") == ssl_hashes[1] &&
      read_string("meta/ssl_target_hash") == ssl_hashes[2];
  if (!metadata) {
    throw std::runtime_error("GPV committed cell metadata failed");
  }

  set_paired_rng(seed, device);
  auto replayed_anchor = mtf::MtfJepaMaeVicreg(gpv_config(device));
  const bool replay_metadata = oca_load_archive(
      oca_archive_path(seed), replayed_anchor, device, seed,
      gpv_anchor_config_hash(device));
  const bool initialization_replay_exact =
      replay_metadata && gpv_model_finite(replayed_anchor) &&
      oca_state_exact(replayed_anchor, oca_snapshot_state(anchor));
  if (!initialization_replay_exact) {
    throw std::runtime_error("GPV committed cell anchor replay failed");
  }
  const auto replayed_initial = snapshot_parameters(replayed_anchor);

  cell = GpvCell{};
  set_paired_rng(seed, device);
  cell.model = mtf::MtfJepaMaeVicreg(gpv_config(device));
  torch::serialize::InputArchive model_archive;
  root.read("model", model_archive);
  cell.model->load(model_archive);
  cell.model->train();
  auto &receipt = cell.receipt;
  const auto read_f64_vector = [&](const char *key) {
    return gpv_double_vector(gpv_read_tensor(root, key), kGpvSteps);
  };
  const auto read_u64_vector = [&](const char *key) {
    return gpv_u64_vector(gpv_read_tensor(root, key), kGpvSteps);
  };
  receipt.losses = read_f64_vector("receipt/losses");
  receipt.gradient_norms = read_f64_vector("receipt/gradient_norms");
  receipt.trunk_gradient_norms =
      read_f64_vector("receipt/trunk_gradient_norms");
  receipt.head_gradient_norms =
      read_f64_vector("receipt/head_gradient_norms");
  receipt.clip_factors = read_f64_vector("receipt/clip_factors");
  receipt.served_update_norms =
      read_f64_vector("receipt/served_update_norms");
  receipt.component_losses[0] =
      read_f64_vector("receipt/invariance_losses");
  receipt.component_losses[1] = read_f64_vector("receipt/variance_losses");
  receipt.component_losses[2] =
      read_f64_vector("receipt/covariance_losses");
  receipt.row_hashes = read_u64_vector("receipt/row_hashes_u64_le");
  receipt.target_mask_hashes =
      read_u64_vector("receipt/target_mask_hashes_u64_le");
  receipt.context_mask_hashes =
      read_u64_vector("receipt/context_mask_hashes_u64_le");
  receipt.view_a_data_hashes =
      read_u64_vector("receipt/view_a_data_hashes_u64_le");
  receipt.view_a_mask_hashes =
      read_u64_vector("receipt/view_a_mask_hashes_u64_le");
  receipt.view_b_data_hashes =
      read_u64_vector("receipt/view_b_data_hashes_u64_le");
  receipt.view_b_mask_hashes =
      read_u64_vector("receipt/view_b_mask_hashes_u64_le");
  receipt.rng_pre_cpu_hashes =
      read_u64_vector("receipt/rng_pre_cpu_hashes_u64_le");
  receipt.rng_pre_cuda_hashes =
      read_u64_vector("receipt/rng_pre_cuda_hashes_u64_le");
  receipt.rng_post_cpu_hashes =
      read_u64_vector("receipt/rng_post_cpu_hashes_u64_le");
  receipt.rng_post_cuda_hashes =
      read_u64_vector("receipt/rng_post_cuda_hashes_u64_le");
  receipt.step_flags =
      gpv_i64_vector(gpv_read_tensor(root, "receipt/step_flags"), kGpvSteps);
  const auto expected_parameter_names = gpv_selected_parameter_names(cell.model);
  const auto archived_parameter_names =
      read_string("receipt/parameter_delta_names");
  if (archived_parameter_names != gpv_join_lines(expected_parameter_names)) {
    throw std::runtime_error(
        "GPV committed parameter-name inventory failed");
  }
  receipt.parameter_delta_names = expected_parameter_names;
  receipt.parameter_deltas = gpv_double_vector(
      gpv_read_tensor(root, "receipt/parameter_deltas"),
      static_cast<int64_t>(expected_parameter_names.size()));
  const auto archived_parameter_delta_names = receipt.parameter_delta_names;
  const auto archived_parameter_deltas = receipt.parameter_deltas;
  auto statistics = gpv_read_tensor(root, "receipt/statistics");
  gpv_require_archive_tensor(statistics, torch::kFloat64, 1, 26,
                             "receipt/statistics");
  statistics = statistics.to(torch::kCPU).contiguous();
  auto counts = gpv_read_tensor(root, "receipt/counts");
  gpv_require_archive_tensor(counts, torch::kInt64, 1, 4,
                             "receipt/counts");
  counts = counts.to(torch::kCPU).contiguous();
  auto flags = gpv_read_tensor(root, "receipt/flags");
  gpv_require_archive_tensor(flags, torch::kInt64, 1, 5,
                             "receipt/flags");
  flags = flags.to(torch::kCPU).contiguous();
  const auto statistic = [&](int64_t index) {
    return statistics[index].item<double>();
  };
  const auto statistic_exact = [&](int64_t index, double expected) {
    return statistic(index) == expected;
  };
  const int64_t archived_clipping_count = counts[3].item<int64_t>();
  receipt.steps = counts[0].item<int64_t>();
  receipt.adam_steps = counts[1].item<int64_t>();
  receipt.ema_steps = counts[2].item<int64_t>();

  const auto vector_min = [](const std::vector<double> &values) {
    return *std::min_element(values.begin(), values.end());
  };
  const auto vector_max = [](const std::vector<double> &values) {
    return *std::max_element(values.begin(), values.end());
  };
  const bool receipt_shape = gpv_receipt_shape_valid(receipt);
  if (!receipt_shape) {
    throw std::runtime_error("GPV committed receipt vector shape failed");
  }

  receipt.minimum_gradient_norm = vector_min(receipt.gradient_norms);
  receipt.maximum_gradient_norm = vector_max(receipt.gradient_norms);
  receipt.minimum_served_update_norm =
      vector_min(receipt.served_update_norms);
  receipt.maximum_served_update_norm =
      vector_max(receipt.served_update_norms);
  for (std::size_t component = 0; component < receipt.component_losses.size();
       ++component) {
    receipt.component_loss_sums[component] = std::accumulate(
        receipt.component_losses[component].begin(),
        receipt.component_losses[component].end(), 0.0);
  }
  const std::array<double, 6> archived_partition_deltas{
      statistic(20), statistic(21), statistic(22),
      statistic(23), statistic(24), statistic(25)};

  const bool vector_statistics_exact =
      statistic_exact(0, receipt.losses.front()) &&
      statistic_exact(1, receipt.losses.back()) &&
      statistic_exact(2, vector_min(receipt.losses)) &&
      statistic_exact(3, vector_max(receipt.losses)) &&
      statistic_exact(4, gpv_mean(receipt.losses)) &&
      statistic_exact(5, receipt.minimum_gradient_norm) &&
      statistic_exact(6, receipt.maximum_gradient_norm) &&
      statistic_exact(7, gpv_mean(receipt.gradient_norms)) &&
      statistic_exact(8, vector_min(receipt.trunk_gradient_norms)) &&
      statistic_exact(9, vector_max(receipt.trunk_gradient_norms)) &&
      statistic_exact(10, gpv_mean(receipt.trunk_gradient_norms)) &&
      statistic_exact(11, vector_min(receipt.head_gradient_norms)) &&
      statistic_exact(12, vector_max(receipt.head_gradient_norms)) &&
      statistic_exact(13, gpv_mean(receipt.head_gradient_norms)) &&
      statistic_exact(14, receipt.minimum_served_update_norm) &&
      statistic_exact(15, receipt.maximum_served_update_norm) &&
      statistic_exact(16, gpv_mean(receipt.served_update_norms)) &&
      statistic_exact(17, receipt.component_loss_sums[0]) &&
      statistic_exact(18, receipt.component_loss_sums[1]) &&
      statistic_exact(19, receipt.component_loss_sums[2]);

  int64_t recomputed_clipping_count = 0;
  bool clip_factors_exact = true;
  for (std::size_t step = 0; step < receipt.clip_factors.size(); ++step) {
    const double gradient = receipt.gradient_norms[step];
    const double expected = gradient > kGpvClipNorm
                                ? kGpvClipNorm / gradient
                                : 1.0;
    const double observed = receipt.clip_factors[step];
    clip_factors_exact = clip_factors_exact && std::isfinite(observed) &&
                         observed > 0.0 && observed <= 1.0 &&
                         observed == expected;
    recomputed_clipping_count += observed < 1.0 ? 1 : 0;
  }
  receipt.clipping_count = recomputed_clipping_count;

  const bool receipt_vectors_valid =
      gpv_finite_positive(receipt.losses) &&
      gpv_finite_positive(receipt.gradient_norms) &&
      gpv_finite_positive(receipt.trunk_gradient_norms) &&
      gpv_finite_positive(receipt.head_gradient_norms) &&
      gpv_finite_positive(receipt.served_update_norms) &&
      std::all_of(receipt.component_losses.begin(),
                  receipt.component_losses.end(), gpv_finite_nonnegative) &&
      gpv_finite_nonnegative(receipt.parameter_deltas);
  const bool archived_counts_exact =
      receipt.steps == kGpvSteps && receipt.adam_steps == kGpvSteps &&
      receipt.ema_steps == kGpvSteps &&
      archived_clipping_count == recomputed_clipping_count;

  std::vector<uint64_t> expected_rows;
  expected_rows.reserve(kGpvSteps);
  for (int64_t step = 0; step < kGpvSteps; ++step) {
    expected_rows.push_back(hash_batch_rows(training_rows(ssl, seed, step)));
  }
  const std::array<double, 6> reconstructed_partition_deltas{
      parameter_partition_max_abs_diff(
          cell.model, replayed_initial,
          ParameterDeltaPartition::all_trainable),
      parameter_partition_max_abs_diff(cell.model, replayed_initial,
                                       ParameterDeltaPartition::served),
      parameter_partition_max_abs_diff(cell.model, replayed_initial,
                                       ParameterDeltaPartition::predictor),
      parameter_partition_max_abs_diff(cell.model, replayed_initial,
                                       ParameterDeltaPartition::mae_decoder),
      parameter_partition_max_abs_diff(cell.model, replayed_initial,
                                       ParameterDeltaPartition::vicreg_head),
      parameter_partition_max_abs_diff(cell.model, replayed_initial,
                                       ParameterDeltaPartition::target_ema)};
  const bool partition_archive_exact =
      reconstructed_partition_deltas == archived_partition_deltas;
  receipt.all_trainable_delta = reconstructed_partition_deltas[0];
  receipt.served_delta = reconstructed_partition_deltas[1];
  receipt.predictor_delta = reconstructed_partition_deltas[2];
  receipt.mae_decoder_delta = reconstructed_partition_deltas[3];
  receipt.vicreg_head_delta = reconstructed_partition_deltas[4];
  receipt.target_ema_delta = reconstructed_partition_deltas[5];

  auto reconstructed_parameter_deltas =
      gpv_selected_parameter_deltas(cell.model, replayed_initial);
  const bool named_delta_archive_exact =
      reconstructed_parameter_deltas.first == archived_parameter_delta_names &&
      reconstructed_parameter_deltas.second == archived_parameter_deltas;
  receipt.parameter_delta_names =
      std::move(reconstructed_parameter_deltas.first);
  receipt.parameter_deltas =
      std::move(reconstructed_parameter_deltas.second);
  const bool parameter_inventory_exact =
      receipt.parameter_delta_names == expected_parameter_names &&
      receipt.parameter_deltas.size() == expected_parameter_names.size();
  std::size_t head_weights = 0;
  bool head_weights_changed = true;
  bool head_changed = false;
  for (std::size_t index = 0; index < receipt.parameter_delta_names.size();
       ++index) {
    const auto &name = receipt.parameter_delta_names[index];
    const double delta = receipt.parameter_deltas[index];
    if (name.rfind("vicreg_stability_head.", 0) == 0) {
      head_changed = head_changed || delta > 0.0;
      if (name.ends_with(".weight")) {
        ++head_weights;
        head_weights_changed = head_weights_changed && delta > 0.0;
      }
    }
  }
  const bool schedule_reconstructed =
      receipt.row_hashes == expected_rows &&
      std::all_of(receipt.step_flags.begin(), receipt.step_flags.end(),
                  [](int64_t flag) { return flag == 1; });
  const bool partition_scalars_exact =
      partition_archive_exact && named_delta_archive_exact &&
      std::isfinite(receipt.all_trainable_delta) &&
      receipt.all_trainable_delta > 0.0 &&
      std::isfinite(receipt.served_delta) && receipt.served_delta > 0.0 &&
      receipt.predictor_delta == 0.0 && receipt.mae_decoder_delta == 0.0 &&
      std::isfinite(receipt.vicreg_head_delta) &&
      receipt.vicreg_head_delta > 0.0 &&
      std::isfinite(receipt.target_ema_delta) &&
      receipt.target_ema_delta > 0.0 && head_weights == 3 &&
      head_weights_changed && head_changed;
  const bool final_model_finite = gpv_model_finite(cell.model);
  const bool loss_reconstruction_valid =
      gpv_receipt_loss_reconstruction_valid(receipt);
  receipt.initialization_exact = initialization_replay_exact;
  receipt.finite = receipt_vectors_valid && clip_factors_exact &&
                   vector_statistics_exact && loss_reconstruction_valid &&
                   final_model_finite;
  receipt.schedule_exact = schedule_reconstructed;
  receipt.expected_partitions =
      parameter_inventory_exact && partition_scalars_exact;
  const bool reconstructed =
      receipt_shape && archived_counts_exact && receipt.initialization_exact &&
      receipt.finite &&
      receipt.schedule_exact && receipt.expected_partitions &&
      receipt.minimum_gradient_norm > 0.0 &&
      receipt.minimum_served_update_norm > 0.0;
  receipt.pass = reconstructed;
  const std::array<int64_t, 5> reconstructed_flags{
      receipt.initialization_exact ? 1 : 0, receipt.finite ? 1 : 0,
      receipt.schedule_exact ? 1 : 0, receipt.expected_partitions ? 1 : 0,
      receipt.pass ? 1 : 0};
  bool archived_flags_exact = true;
  for (std::size_t index = 0; index < reconstructed_flags.size(); ++index) {
    archived_flags_exact =
        archived_flags_exact &&
        flags[static_cast<int64_t>(index)].item<int64_t>() ==
            reconstructed_flags[index];
  }
  if (!reconstructed || !archived_flags_exact) {
    throw std::runtime_error("GPV committed cell receipt validation failed");
  }
  cell.resumed = true;
  return true;
}

[[nodiscard]] GpvCell gpv_train_cell(
    const Dataset &ssl, const torch::Device &device, int64_t seed,
    uint8_t mask, mtf::MtfJepaMaeVicreg &anchor) {
  if (mask == 0U || mask >= 8U) {
    throw std::runtime_error("GPV trains masks 1..7 only");
  }
  GpvCell cell{};
  set_paired_rng(seed, device);
  cell.model = mtf::MtfJepaMaeVicreg(gpv_config(device));
  const bool metadata = oca_load_archive(
      oca_archive_path(seed), cell.model, device, seed,
      gpv_anchor_config_hash(device));
  cell.receipt.initialization_exact =
      metadata && oca_state_exact(cell.model, oca_snapshot_state(anchor));
  if (!cell.receipt.initialization_exact) {
    throw std::runtime_error("GPV FSPA initialization replay failed");
  }
  cell.model->train();
  auto parameters = cell.model->parameters();
  torch::optim::Adam optimizer(parameters,
                               torch::optim::AdamOptions(kGpvLearningRate));
  const auto initial = snapshot_parameters(cell.model);
  auto &receipt = cell.receipt;
  receipt.steps = kGpvSteps;
  for (auto *values : {&receipt.losses, &receipt.gradient_norms,
                       &receipt.trunk_gradient_norms,
                       &receipt.head_gradient_norms,
                       &receipt.clip_factors,
                       &receipt.served_update_norms}) {
    values->reserve(kGpvSteps);
  }
  for (auto &values : receipt.component_losses) {
    values.reserve(kGpvSteps);
  }
  for (auto *hashes : {
           &receipt.row_hashes, &receipt.target_mask_hashes,
           &receipt.context_mask_hashes, &receipt.view_a_data_hashes,
           &receipt.view_a_mask_hashes, &receipt.view_b_data_hashes,
           &receipt.view_b_mask_hashes, &receipt.rng_pre_cpu_hashes,
           &receipt.rng_pre_cuda_hashes, &receipt.rng_post_cpu_hashes,
           &receipt.rng_post_cuda_hashes}) {
    hashes->reserve(kGpvSteps);
  }
  receipt.step_flags.reserve(kGpvSteps);

  for (int64_t step = 0; step < kGpvSteps; ++step) {
    const auto rows = training_rows(ssl, seed, step);
    const auto indices = torch::tensor(rows, torch::kInt64);
    const auto batch_data = ssl.data.index_select(0, indices).to(device);
    const auto batch_mask = ssl.mask.index_select(0, indices).to(device);
    receipt.row_hashes.push_back(hash_batch_rows(rows));
    set_paired_rng(paired_step_seed(seed, step), device);
    const auto rng_pre = current_generator_state_snapshot(device);
    optimizer.zero_grad();
    mtf::mtf_jepa_mae_vicreg_output_t prelude{};
    {
      torch::NoGradGuard no_grad;
      prelude = cell.model->forward(batch_data, batch_mask);
    }
    const auto rng_post = current_generator_state_snapshot(device);
    validate_weak_view_debug_tensors(prelude, batch_data, batch_mask);
    receipt.target_mask_hashes.push_back(
        hash_tensor_stable_bytes(prelude.jepa_target_mask));
    receipt.context_mask_hashes.push_back(
        hash_tensor_stable_bytes(prelude.jepa_context_mask));
    receipt.view_a_data_hashes.push_back(
        hash_tensor_stable_bytes(prelude.vicreg_view_a_data));
    receipt.view_a_mask_hashes.push_back(
        hash_tensor_stable_bytes(prelude.vicreg_view_a_feature_mask));
    receipt.view_b_data_hashes.push_back(
        hash_tensor_stable_bytes(prelude.vicreg_view_b_data));
    receipt.view_b_mask_hashes.push_back(
        hash_tensor_stable_bytes(prelude.vicreg_view_b_feature_mask));
    receipt.rng_pre_cpu_hashes.push_back(rng_pre.digest.cpu);
    receipt.rng_pre_cuda_hashes.push_back(rng_pre.digest.cuda);
    receipt.rng_post_cpu_hashes.push_back(rng_post.digest.cpu);
    receipt.rng_post_cuda_hashes.push_back(rng_post.digest.cuda);

    const auto objective = gpv_route_retained_views(
        cell.model, prelude.vicreg_view_a_data,
        prelude.vicreg_view_a_feature_mask, prelude.vicreg_view_b_data,
        prelude.vicreg_view_b_feature_mask, mask);
    const bool rng_unchanged = generator_state_snapshot_equal(
        rng_post, current_generator_state_snapshot(device));
    const double loss = objective.total.item<double>();
    objective.total.backward();
    const double gradient_norm =
        gpv_gradient_norm(cell.model, false, false);
    const double trunk_gradient_norm =
        gpv_gradient_norm(cell.model, true, false);
    const double head_gradient_norm =
        gpv_gradient_norm(cell.model, false, true);
    const bool gradients_finite = std::isfinite(gradient_norm) &&
                                  std::isfinite(trunk_gradient_norm) &&
                                  std::isfinite(head_gradient_norm);
    const double clip_factor =
        gradient_norm > kGpvClipNorm
            ? kGpvClipNorm / std::max(1.0e-30, gradient_norm)
            : 1.0;
    if (clip_factor < 1.0) {
      ++receipt.clipping_count;
      for (const auto &parameter : parameters) {
        if (parameter.grad().defined()) {
          parameter.grad().mul_(clip_factor);
        }
      }
    }
    const auto served = gpwd_served_parameters(cell.model);
    std::vector<torch::Tensor> served_before;
    served_before.reserve(served.size());
    for (const auto &parameter : served) {
      served_before.push_back(parameter.detach().clone());
    }
    optimizer.step();
    ++receipt.adam_steps;
    auto update_square = torch::zeros({}, batch_data.options());
    for (std::size_t index = 0; index < served.size(); ++index) {
      update_square =
          update_square +
          (served[index].detach() - served_before[index]).pow(2).sum();
    }
    const double update_norm = update_square.sqrt().item<double>();
    const bool ema = cell.model->update_target_network(0.990);
    receipt.ema_steps += ema ? 1 : 0;
    const bool step_pass =
        objective.finite && objective.canonical_layout &&
        std::isfinite(loss) && loss > 0.0 && gradients_finite &&
        gradient_norm > 0.0 && trunk_gradient_norm > 0.0 &&
        head_gradient_norm > 0.0 && std::isfinite(update_norm) &&
        update_norm > 0.0 && rng_unchanged && ema;
    receipt.step_flags.push_back(step_pass ? 1 : 0);
    receipt.losses.push_back(loss);
    receipt.gradient_norms.push_back(gradient_norm);
    receipt.trunk_gradient_norms.push_back(trunk_gradient_norm);
    receipt.head_gradient_norms.push_back(head_gradient_norm);
    receipt.clip_factors.push_back(clip_factor);
    receipt.served_update_norms.push_back(update_norm);
    const std::array<double, 3> component_losses{
        objective.invariance.item<double>(), objective.variance.item<double>(),
        objective.covariance.item<double>()};
    for (std::size_t component = 0; component < component_losses.size();
         ++component) {
      receipt.component_losses[component].push_back(
          component_losses[component]);
      receipt.component_loss_sums[component] += component_losses[component];
    }
    receipt.minimum_gradient_norm =
        std::min(receipt.minimum_gradient_norm, gradient_norm);
    receipt.maximum_gradient_norm =
        std::max(receipt.maximum_gradient_norm, gradient_norm);
    receipt.minimum_served_update_norm =
        std::min(receipt.minimum_served_update_norm, update_norm);
    receipt.maximum_served_update_norm =
        std::max(receipt.maximum_served_update_norm, update_norm);
    receipt.finite = receipt.finite && step_pass;
    receipt.schedule_exact = receipt.schedule_exact && rng_unchanged;
    if ((step + 1) % 128 == 0 || step + 1 == kGpvSteps) {
      std::cout << "gpv1.training.seed_" << seed << ".mask_"
                << static_cast<int>(mask)
                << ".completed_steps=" << step + 1 << '\n'
                << std::flush;
    }
  }

  receipt.all_trainable_delta = parameter_partition_max_abs_diff(
      cell.model, initial, ParameterDeltaPartition::all_trainable);
  receipt.served_delta = parameter_partition_max_abs_diff(
      cell.model, initial, ParameterDeltaPartition::served);
  receipt.predictor_delta = parameter_partition_max_abs_diff(
      cell.model, initial, ParameterDeltaPartition::predictor);
  receipt.mae_decoder_delta = parameter_partition_max_abs_diff(
      cell.model, initial, ParameterDeltaPartition::mae_decoder);
  receipt.vicreg_head_delta = parameter_partition_max_abs_diff(
      cell.model, initial, ParameterDeltaPartition::vicreg_head);
  receipt.target_ema_delta = parameter_partition_max_abs_diff(
      cell.model, initial, ParameterDeltaPartition::target_ema);
  std::tie(receipt.parameter_delta_names, receipt.parameter_deltas) =
      gpv_selected_parameter_deltas(cell.model, initial);
  std::size_t head_weights = 0;
  bool head_weights_changed = true;
  bool head_changed = false;
  for (std::size_t index = 0; index < receipt.parameter_delta_names.size();
       ++index) {
    const auto &name = receipt.parameter_delta_names[index];
    const double delta = receipt.parameter_deltas[index];
    if (name.rfind("vicreg_stability_head.", 0) == 0) {
      head_changed = head_changed || delta > 0.0;
      if (name.ends_with(".weight")) {
        ++head_weights;
        head_weights_changed = head_weights_changed && delta > 0.0;
      }
    }
  }
  receipt.expected_partitions =
      receipt.all_trainable_delta > 0.0 && receipt.served_delta > 0.0 &&
      receipt.predictor_delta == 0.0 && receipt.mae_decoder_delta == 0.0 &&
      receipt.vicreg_head_delta > 0.0 && receipt.target_ema_delta > 0.0 &&
      receipt.parameter_delta_names ==
          gpv_selected_parameter_names(cell.model) &&
      gpv_finite_nonnegative(receipt.parameter_deltas) && head_weights == 3 &&
      head_weights_changed && head_changed;
  const bool receipt_vectors_valid =
      gpv_finite_positive(receipt.losses) &&
      gpv_finite_positive(receipt.gradient_norms) &&
      gpv_finite_positive(receipt.trunk_gradient_norms) &&
      gpv_finite_positive(receipt.head_gradient_norms) &&
      gpv_finite_positive(receipt.served_update_norms) &&
      std::all_of(receipt.component_losses.begin(),
                  receipt.component_losses.end(), gpv_finite_nonnegative) &&
      std::all_of(receipt.clip_factors.begin(), receipt.clip_factors.end(),
                  [](double value) {
                    return std::isfinite(value) && value > 0.0 && value <= 1.0;
                  });
  const bool loss_reconstruction_valid =
      gpv_receipt_loss_reconstruction_valid(receipt);
  receipt.pass = receipt.initialization_exact && receipt.finite &&
                  receipt.schedule_exact && receipt.expected_partitions &&
                  receipt_vectors_valid && loss_reconstruction_valid &&
                  gpv_model_finite(cell.model) &&
                  receipt.minimum_gradient_norm > 0.0 &&
                 receipt.minimum_served_update_norm > 0.0 &&
                 receipt.adam_steps == kGpvSteps &&
                 receipt.ema_steps == kGpvSteps &&
                 gpv_receipt_shape_valid(receipt) &&
                 std::all_of(receipt.step_flags.begin(),
                             receipt.step_flags.end(),
                             [](int64_t value) { return value == 1; });
  if (!receipt.pass) {
    throw std::runtime_error("GPV cell training mechanics failed");
  }
  return cell;
}

[[nodiscard]] GpvCell gpv_train_or_use_inventoried_cell(
    const Dataset &ssl, const torch::Device &device, int64_t seed,
    uint8_t mask, mtf::MtfJepaMaeVicreg &anchor,
    const GpvBindings &bindings, std::size_t seed_index,
    bool cache_present, bool allow_training, GpvCell cell) {
  if (!cache_present) {
    if (!allow_training) {
      throw std::runtime_error(
          "GPV confirmation requires every completed cell cache");
    }
    cell = gpv_train_cell(ssl, device, seed, mask, anchor);
    gpv_save_cell(ssl, device, seed, mask, bindings, cell, seed_index);
    GpvCell verified{};
    if (!gpv_load_cell(ssl, device, seed, mask, anchor, bindings, seed_index,
                       verified)) {
      throw std::runtime_error("GPV newly committed cell did not reload");
    }
    cell = std::move(verified);
    cell.resumed = false;
  } else if (!cell.resumed || !cell.model || !cell.receipt.pass) {
    throw std::runtime_error("GPV inventoried cell state failed");
  }
  const auto path = gpv_cell_path(seed, mask);
  std::cout << "gpv1.cache.seed_" << seed << ".mask_"
            << static_cast<int>(mask) << ".resumed=" << cell.resumed << '\n';
  std::cout << "gpv1.cache.seed_" << seed << ".mask_"
            << static_cast<int>(mask) << ".path=" << path.generic_string()
            << '\n';
  std::cout << "gpv1.cache.seed_" << seed << ".mask_"
            << static_cast<int>(mask) << ".sha256=" << gpv_sha256_file(path)
            << '\n';
  return cell;
}

[[nodiscard]] bool gpv_receipt_schedule_exact(const GpvReceipt &left,
                                              const GpvReceipt &right) {
  return left.row_hashes == right.row_hashes &&
         left.target_mask_hashes == right.target_mask_hashes &&
         left.context_mask_hashes == right.context_mask_hashes &&
         left.view_a_data_hashes == right.view_a_data_hashes &&
         left.view_a_mask_hashes == right.view_a_mask_hashes &&
         left.view_b_data_hashes == right.view_b_data_hashes &&
         left.view_b_mask_hashes == right.view_b_mask_hashes &&
         left.rng_pre_cpu_hashes == right.rng_pre_cpu_hashes &&
         left.rng_pre_cuda_hashes == right.rng_pre_cuda_hashes &&
         left.rng_post_cpu_hashes == right.rng_post_cpu_hashes &&
         left.rng_post_cuda_hashes == right.rng_post_cuda_hashes;
}

void gpv_emit_receipt(const std::string &root, const GpvReceipt &value) {
  std::cout << root << ".steps=" << value.steps << '\n';
  std::cout << root << ".adam_steps=" << value.adam_steps << '\n';
  std::cout << root << ".ema_steps=" << value.ema_steps << '\n';
  std::cout << root << ".loss_min="
            << *std::min_element(value.losses.begin(), value.losses.end())
            << '\n';
  std::cout << root << ".loss_max="
            << *std::max_element(value.losses.begin(), value.losses.end())
            << '\n';
  std::cout << root << ".loss_mean=" << gpv_mean(value.losses) << '\n';
  std::cout << root << ".gradient_min=" << value.minimum_gradient_norm
            << '\n';
  std::cout << root << ".gradient_max=" << value.maximum_gradient_norm
            << '\n';
  std::cout << root << ".gradient_mean=" << gpv_mean(value.gradient_norms)
            << '\n';
  std::cout << root << ".trunk_gradient_mean="
            << gpv_mean(value.trunk_gradient_norms) << '\n';
  std::cout << root << ".head_gradient_mean="
            << gpv_mean(value.head_gradient_norms) << '\n';
  std::cout << root << ".served_update_min="
            << value.minimum_served_update_norm << '\n';
  std::cout << root << ".served_update_max="
            << value.maximum_served_update_norm << '\n';
  std::cout << root << ".served_update_mean="
            << gpv_mean(value.served_update_norms) << '\n';
  std::cout << root << ".clipping_count=" << value.clipping_count << '\n';
  std::cout << root << ".invariance_sum=" << value.component_loss_sums[0]
            << '\n';
  std::cout << root << ".variance_sum=" << value.component_loss_sums[1]
            << '\n';
  std::cout << root << ".covariance_sum=" << value.component_loss_sums[2]
            << '\n';
  std::cout << root << ".served_delta=" << value.served_delta << '\n';
  std::cout << root << ".predictor_delta=" << value.predictor_delta << '\n';
  std::cout << root << ".mae_decoder_delta=" << value.mae_decoder_delta
            << '\n';
  std::cout << root << ".vicreg_head_delta=" << value.vicreg_head_delta
            << '\n';
  std::cout << root << ".target_ema_delta=" << value.target_ema_delta << '\n';
  for (std::size_t index = 0; index < value.parameter_delta_names.size();
       ++index) {
    std::cout << root << ".parameter." << value.parameter_delta_names[index]
              << ".delta=" << value.parameter_deltas[index] << '\n';
  }
  std::cout << root << ".pass=" << value.pass << '\n';
}

[[nodiscard]] RmcEvaluation gpv_evaluate(
    mtf::MtfJepaMaeVicreg &model, const RmcData &data,
    const RmcEvalTargets &targets, const torch::Device &device,
    bool confirmation) {
  const auto &evaluation = confirmation ? data.confirmation : data.development;
  const auto &reversed = confirmation ? data.reversed_confirmation
                                      : data.reversed_development;
  return rmc_evaluate(model, data.probe_train, data.probe_validation,
                      evaluation, data.reversed_train, data.reversed_validation,
                      reversed, targets, device);
}

[[nodiscard]] ProbeCurve gpv_raw_curve(const RmcData &data,
                                       bool confirmation) {
  const auto &raw = confirmation ? data.raw_confirmation : data.raw_development;
  const auto &evaluation = confirmation ? data.confirmation : data.development;
  return rssm_probe_curve(data.raw_train, data.raw_validation, raw,
                          data.probe_train.target,
                          data.probe_validation.target, evaluation.target,
                          /*dual=*/true);
}

[[nodiscard]] GpvBootstrapAreaTable gpv_bootstrap_area_table(
    const GpvEvaluations &evaluations, const torch::Tensor &target,
    const std::vector<torch::Tensor> &bootstrap_rows) {
  GpvBootstrapAreaTable result;
  result.reserve(bootstrap_rows.size());
  for (const auto &rows : bootstrap_rows) {
    std::array<std::array<double, 8>, 3> replicate{};
    for (std::size_t seed = 0; seed < 3; ++seed) {
      for (std::size_t mask = 0; mask < 8; ++mask) {
        replicate[seed][mask] =
            rssm_resampled_area(evaluations[seed][mask].probe, target, rows)
                .macro;
      }
    }
    result.push_back(std::move(replicate));
  }
  return result;
}

[[nodiscard]] GpvWeightedContrast gpv_weighted_contrast(
    const GpvEvaluations &evaluations,
    const GpvBootstrapAreaTable &bootstrap,
    const std::array<double, 8> &weights) {
  GpvWeightedContrast result{};
  for (std::size_t seed = 0; seed < 3; ++seed) {
    for (std::size_t mask = 0; mask < 8; ++mask) {
      result.per_seed[seed] +=
          weights[mask] * evaluations[seed][mask].probe.area;
      const auto family = rssm_family_areas(evaluations[seed][mask].probe);
      for (std::size_t index = 0; index < kFamilies; ++index) {
        result.family[index] += weights[mask] * family[index] / 3.0;
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
      for (std::size_t mask = 0; mask < 8; ++mask) {
        value += weights[mask] * table[seed][mask] / 3.0;
      }
    }
    replicates.push_back(value);
  }
  const auto interval = percentile_interval(std::move(replicates));
  result.summary.low = interval.low;
  result.summary.high = interval.high;
  return result;
}

[[nodiscard]] std::array<double, 8> gpv_pair_weights(std::size_t positive,
                                                     std::size_t negative) {
  std::array<double, 8> result{};
  result.at(positive) = 1.0;
  result.at(negative) = -1.0;
  return result;
}

[[nodiscard]] std::array<double, 8>
gpv_main_weights(std::size_t factor) {
  std::array<double, 8> result{};
  const uint8_t bit = static_cast<uint8_t>(1U << factor);
  for (uint8_t mask = 0; mask < 8U; ++mask) {
    result[mask] = (mask & bit) ? 0.25 : -0.25;
  }
  return result;
}

[[nodiscard]] std::array<double, 8>
gpv_two_way_weights(std::size_t left, std::size_t right) {
  std::array<double, 8> result{};
  const uint8_t left_bit = static_cast<uint8_t>(1U << left);
  const uint8_t right_bit = static_cast<uint8_t>(1U << right);
  for (uint8_t mask = 0; mask < 8U; ++mask) {
    result[mask] = (((mask & left_bit) != 0U) ==
                    ((mask & right_bit) != 0U))
                       ? 0.5
                       : -0.5;
  }
  return result;
}

[[nodiscard]] std::array<double, 8> gpv_three_way_weights() {
  return {-1.0, 1.0, 1.0, -1.0, 1.0, -1.0, -1.0, 1.0};
}

[[nodiscard]] std::array<double, 8>
gpv_simple_weights(std::size_t factor, std::size_t stratum) {
  if (factor >= 3 || stratum >= 4) {
    throw std::runtime_error("GPV simple-effect index failed");
  }
  std::array<std::size_t, 2> other{};
  std::size_t cursor = 0;
  for (std::size_t candidate = 0; candidate < 3; ++candidate) {
    if (candidate != factor) {
      other[cursor++] = candidate;
    }
  }
  uint8_t negative = 0U;
  if ((stratum & 1U) != 0U) {
    negative |= static_cast<uint8_t>(1U << other[0]);
  }
  if ((stratum & 2U) != 0U) {
    negative |= static_cast<uint8_t>(1U << other[1]);
  }
  const uint8_t positive =
      negative | static_cast<uint8_t>(1U << factor);
  return gpv_pair_weights(positive, negative);
}

[[nodiscard]] GpvWeightedContrast gpv_pair_contrast(
    const std::array<RmcEvaluation, 3> &reference,
    const std::array<RmcEvaluation, 3> &candidate,
    const RmcSummary &summary) {
  GpvWeightedContrast result{};
  result.summary = summary.candidate[0].gate.trained_minus_initialization;
  result.family = summary.candidate[0].gate.learned_family_deltas;
  for (std::size_t seed = 0; seed < 3; ++seed) {
    result.per_seed[seed] =
        candidate[seed].probe.area - reference[seed].probe.area;
  }
  return result;
}

[[nodiscard]] GpvSafeguards
gpv_safeguards(const RmcSummary &summary) {
  const auto &gate = summary.gate.neutral;
  GpvSafeguards result{{gate.numeric_valid,
                        gate.mechanics_pass,
                        gate.family_floor_pass,
                        gate.raw_noninferiority_pass,
                        gate.order_point_pass,
                        gate.order_lower_pass,
                        gate.order_retention_pass,
                        gate.continuous_shuffle_pass,
                        gate.order_shuffle_pass,
                        gate.geometry_pass},
                       false};
  result.all = std::all_of(result.values.begin(), result.values.end(),
                           [](bool value) { return value; });
  return result;
}

[[nodiscard]] bool gpv_no_new_safeguard_failure(
    const GpvSafeguards &current, const GpvSafeguards &candidate) {
  for (std::size_t index = 0; index < current.values.size(); ++index) {
    if (current.values[index] && !candidate.values[index]) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool
gpv_family_floor(const GpvWeightedContrast &contrast) {
  return std::all_of(contrast.family.begin(), contrast.family.end(),
                     [](double value) {
                       return std::isfinite(value) &&
                              value >= kGpvFamilyFloor;
                     });
}

[[nodiscard]] bool
gpv_positive_supported(const GpvWeightedContrast &contrast) {
  return std::isfinite(contrast.summary.point) &&
         std::isfinite(contrast.summary.low) &&
         std::isfinite(contrast.summary.high) &&
         contrast.summary.point >= kGpvImprovementFloor &&
         contrast.summary.low > 0.0 &&
         contrast.summary.positive_seed_count == 3;
}

[[nodiscard]] bool
gpv_interaction_supported(const GpvWeightedContrast &contrast) {
  const bool interval_excludes_zero =
      contrast.summary.low > 0.0 || contrast.summary.high < 0.0;
  const bool positive = std::all_of(
      contrast.per_seed.begin(), contrast.per_seed.end(),
      [](double value) { return value > 0.0; });
  const bool negative = std::all_of(
      contrast.per_seed.begin(), contrast.per_seed.end(),
      [](double value) { return value < 0.0; });
  return std::isfinite(contrast.summary.point) &&
         std::abs(contrast.summary.point) >= kGpvImprovementFloor &&
         interval_excludes_zero && (positive || negative);
}

[[nodiscard]] GpvClassification gpv_classify_cell(
    const GpvWeightedContrast &direct,
    const GpvWeightedContrast &versus_anchor,
    const GpvSafeguards &safeguards, bool mechanics) {
  if (!mechanics) {
    return GpvClassification::invalid_numeric_or_mechanics;
  }
  if (!direct.gate) {
    return GpvClassification::no_safe_candidate;
  }
  std::size_t positive_families = 0;
  for (const double value : versus_anchor.family) {
    positive_families += value > 0.0 ? 1U : 0U;
  }
  const bool rescue =
      versus_anchor.summary.point >= kGpvRescueFloor &&
      versus_anchor.summary.low > 0.0 &&
      versus_anchor.summary.positive_seed_count == 3 &&
      positive_families >= 3 && gpv_family_floor(versus_anchor) &&
      safeguards.all;
  if (rescue) {
    return GpvClassification::representation_rescue;
  }
  const bool safe = versus_anchor.summary.low > kGpvAnchorNoninferiority &&
                    gpv_family_floor(versus_anchor) && safeguards.all;
  return safe ? GpvClassification::objective_made_safe
              : GpvClassification::mechanism_mitigates_harm_only;
}

[[nodiscard]] int gpv_classification_tier(GpvClassification value) {
  switch (value) {
  case GpvClassification::representation_rescue:
    return 0;
  case GpvClassification::objective_made_safe:
    return 1;
  case GpvClassification::mechanism_mitigates_harm_only:
    return 2;
  default:
    return 3;
  }
}

[[nodiscard]] std::array<RmcEvaluation, 3>
gpv_evaluation_column(const GpvEvaluations &evaluations, std::size_t mask) {
  std::array<RmcEvaluation, 3> result{};
  for (std::size_t seed = 0; seed < 3; ++seed) {
    result[seed] = evaluations[seed][mask];
  }
  return result;
}

[[nodiscard]] GpvDevelopmentResult gpv_analyze(
    const RmcData &data, const RmcEvalTargets &targets,
    const std::vector<torch::Tensor> &bootstrap_rows,
    const GpvEvaluations &evaluations,
    const std::array<RmcEvaluation, 3> &anchors, bool mechanics) {
  GpvDevelopmentResult result{};
  result.evaluations = evaluations;
  result.anchors = anchors;
  const auto raw = gpv_raw_curve(data, false);
  validate_probe_curve_finite(raw, "GPV raw development");
  const auto bootstrap = gpv_bootstrap_area_table(
      evaluations, data.development.target, bootstrap_rows);
  const auto current = gpv_evaluation_column(evaluations, 0);
  double current_mean = 0.0;
  double anchor_mean = 0.0;
  bool baseline_exact = true;
  for (std::size_t seed = 0; seed < 3; ++seed) {
    current_mean += current[seed].probe.area / 3.0;
    anchor_mean += anchors[seed].probe.area / 3.0;
    baseline_exact =
        baseline_exact &&
        std::abs(current[seed].probe.area - kGpvExpectedCurrentAulc[seed]) <=
            1.0e-15;
  }
  baseline_exact =
      baseline_exact &&
      std::abs(current_mean - 0.61144866961158029) <= 1.0e-15 &&
      std::abs(anchor_mean - 0.64154862079148123) <= 1.0e-15;
  result.mechanics = mechanics && baseline_exact;

  std::array<RmcSummary, 8> anchor_summaries{};
  bool evaluation_mechanics = true;
  for (std::size_t mask = 0; mask < 8; ++mask) {
    const auto candidate = gpv_evaluation_column(evaluations, mask);
    result.direct[mask] = gpv_weighted_contrast(
        evaluations, bootstrap, gpv_pair_weights(mask, 0));
    anchor_summaries[mask] = oca_pair_summary(
        anchors, candidate, raw, data.development.target, targets,
        bootstrap_rows, result.mechanics);
    result.versus_anchor[mask] =
        gpv_pair_contrast(anchors, candidate, anchor_summaries[mask]);
    result.safeguards[mask] = gpv_safeguards(anchor_summaries[mask]);
    evaluation_mechanics =
        evaluation_mechanics && result.safeguards[mask].values[0] &&
        result.safeguards[mask].values[1];
  }
  result.mechanics = result.mechanics && evaluation_mechanics;
  const auto current_safeguards = result.safeguards[0];
  for (std::size_t mask = 1; mask < 8; ++mask) {
    auto &direct = result.direct[mask];
    direct.no_new_safeguard_failure = gpv_no_new_safeguard_failure(
        current_safeguards, result.safeguards[mask]);
    direct.gate = gpv_positive_supported(direct) &&
                  gpv_family_floor(direct) &&
                  direct.no_new_safeguard_failure;
    result.classification[mask] = gpv_classify_cell(
        direct, result.versus_anchor[mask], result.safeguards[mask],
        result.mechanics);
  }
  result.classification[0] = result.mechanics
                                 ? GpvClassification::no_safe_candidate
                                 : GpvClassification::invalid_numeric_or_mechanics;

  for (std::size_t factor = 0; factor < 3; ++factor) {
    result.main_effects[factor] = gpv_weighted_contrast(
        evaluations, bootstrap, gpv_main_weights(factor));
    for (std::size_t stratum = 0; stratum < 4; ++stratum) {
      result.simple[factor][stratum] = gpv_weighted_contrast(
          evaluations, bootstrap, gpv_simple_weights(factor, stratum));
    }
  }
  result.two_way[0] = gpv_weighted_contrast(
      evaluations, bootstrap, gpv_two_way_weights(0, 1));
  result.two_way[1] = gpv_weighted_contrast(
      evaluations, bootstrap, gpv_two_way_weights(0, 2));
  result.two_way[2] = gpv_weighted_contrast(
      evaluations, bootstrap, gpv_two_way_weights(1, 2));
  result.three_way = gpv_weighted_contrast(
      evaluations, bootstrap, gpv_three_way_weights());

  if (!result.mechanics) {
    result.selected_classification =
        GpvClassification::invalid_numeric_or_mechanics;
    return result;
  }
  for (int tier = 0; tier < 3 && !result.selected.has_value(); ++tier) {
    for (uint8_t mask = 1U; mask < 8U; ++mask) {
      if (gpv_classification_tier(result.classification[mask]) != tier) {
        continue;
      }
      if (!result.selected.has_value()) {
        result.selected = mask;
        continue;
      }
      const uint8_t incumbent = *result.selected;
      const double candidate_point = result.direct[mask].summary.point;
      const double incumbent_point = result.direct[incumbent].summary.point;
      const bool greater = candidate_point > incumbent_point + 1.0e-12;
      const bool tie = std::abs(candidate_point - incumbent_point) <= 1.0e-12;
      const auto candidate_bits = std::popcount(static_cast<unsigned>(mask));
      const auto incumbent_bits =
          std::popcount(static_cast<unsigned>(incumbent));
      if (greater ||
          (tie && (candidate_bits < incumbent_bits ||
                   (candidate_bits == incumbent_bits && mask < incumbent)))) {
        result.selected = mask;
      }
    }
  }
  result.selected_classification =
      result.selected.has_value()
          ? result.classification[*result.selected]
          : GpvClassification::no_safe_candidate;
  return result;
}

void gpv_emit_evaluation(const std::string &root,
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
    std::cout << prefix << ".active="
              << geometry.active_dimension_fraction << '\n';
  }
}

void gpv_emit_contrast(const std::string &root,
                       const GpvWeightedContrast &value) {
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

void gpv_emit_development(const GpvDevelopmentResult &value,
                          const RmcData &data) {
  constexpr std::array<const char *, 10> safeguard_names{
      "numeric_valid",          "mechanics_pass",
      "family_floor_pass",      "raw_noninferiority_pass",
      "order_point_pass",       "order_lower_pass",
      "order_retention_pass",   "continuous_shuffle_pass",
      "order_shuffle_pass",     "geometry_pass"};
  constexpr std::array<const char *, 3> factors{"P", "J", "V"};
  constexpr std::array<const char *, 3> interactions{"PJ", "PV", "JV"};
  const auto raw = gpv_raw_curve(data, false);
  std::cout << "gpv1.development.raw_equal_width_aulc=" << raw.area << '\n';
  for (std::size_t seed = 0; seed < 3; ++seed) {
    gpv_emit_evaluation("gpv1.development.anchor.seed_" +
                            std::to_string(kAttributionSeeds[seed]),
                        value.anchors[seed]);
    for (std::size_t mask = 0; mask < 8; ++mask) {
      gpv_emit_evaluation(
          "gpv1.development.mask_" + std::to_string(mask) + ".seed_" +
              std::to_string(kAttributionSeeds[seed]),
          value.evaluations[seed][mask]);
    }
  }
  for (std::size_t mask = 0; mask < 8; ++mask) {
    const std::string root =
        "gpv1.development.mask_" + std::to_string(mask);
    gpv_emit_contrast(root + ".minus_current", value.direct[mask]);
    gpv_emit_contrast(root + ".minus_fspa4", value.versus_anchor[mask]);
    std::cout << root << ".no_new_safeguard_failure="
              << value.direct[mask].no_new_safeguard_failure << '\n';
    std::cout << root << ".direct_gate=" << value.direct[mask].gate << '\n';
    for (std::size_t safeguard = 0; safeguard < safeguard_names.size();
         ++safeguard) {
      std::cout << root << ".safeguard." << safeguard_names[safeguard] << '='
                << value.safeguards[mask].values[safeguard] << '\n';
    }
    std::cout << root << ".safeguard.all=" << value.safeguards[mask].all
              << '\n';
    std::cout << root << ".classification="
              << gpv_classification_name(value.classification[mask]) << '\n';
  }
  bool any_mechanism = false;
  for (std::size_t factor = 0; factor < 3; ++factor) {
    const std::string root =
        "gpv1.development.main_effect." + std::string(factors[factor]);
    gpv_emit_contrast(root, value.main_effects[factor]);
    const bool main_supported =
        value.mechanics && gpv_positive_supported(value.main_effects[factor]);
    std::cout << root << ".positive_supported=" << main_supported << '\n';
    bool simple_supported = false;
    for (std::size_t stratum = 0; stratum < 4; ++stratum) {
      const std::string simple_root =
          "gpv1.development.simple_effect." + std::string(factors[factor]) +
          ".stratum_" + std::to_string(stratum);
      gpv_emit_contrast(simple_root, value.simple[factor][stratum]);
      const bool supported = value.mechanics &&
                             gpv_positive_supported(value.simple[factor][stratum]);
      std::cout << simple_root << ".positive_supported=" << supported << '\n';
      simple_supported = simple_supported || supported;
    }
    const bool interaction_qualified =
        value.mechanics && factor == 0
            ? (gpv_interaction_supported(value.two_way[0]) ||
               gpv_interaction_supported(value.two_way[1]) ||
               gpv_interaction_supported(value.three_way))
        : value.mechanics && factor == 1
            ? (gpv_interaction_supported(value.two_way[0]) ||
               gpv_interaction_supported(value.two_way[2]) ||
               gpv_interaction_supported(value.three_way))
        : value.mechanics
            ? (gpv_interaction_supported(value.two_way[1]) ||
               gpv_interaction_supported(value.two_way[2]) ||
               gpv_interaction_supported(value.three_way))
            : false;
    const bool factor_supported = main_supported || simple_supported;
    std::cout << "gpv1.mechanism." << factors[factor]
              << ".effect_supported=" << factor_supported << '\n';
    std::cout << "gpv1.mechanism." << factors[factor]
              << ".wording="
              << (interaction_qualified ? "conditional_simple_effect_required"
                                        : "main_or_simple_effect")
              << '\n';
    any_mechanism = any_mechanism || factor_supported;
  }
  for (std::size_t interaction = 0; interaction < 3; ++interaction) {
    const std::string root = "gpv1.development.interaction." +
                             std::string(interactions[interaction]);
    gpv_emit_contrast(root, value.two_way[interaction]);
    const bool supported = value.mechanics &&
                           gpv_interaction_supported(value.two_way[interaction]);
    std::cout << root << ".two_sided_supported=" << supported << '\n';
    any_mechanism = any_mechanism || supported;
  }
  gpv_emit_contrast("gpv1.development.interaction.PJV", value.three_way);
  const bool three_supported =
      value.mechanics && gpv_interaction_supported(value.three_way);
  std::cout << "gpv1.development.interaction.PJV.two_sided_supported="
            << three_supported << '\n';
  any_mechanism = any_mechanism || three_supported;
  std::cout << "gpv1.mechanism_effect_supported=" << any_mechanism << '\n';
  std::cout << "gpv1.development.selected="
            << (value.selected.has_value()
                    ? std::to_string(*value.selected)
                    : std::string("none"))
            << '\n';
  std::cout << "gpv1.development.classification="
            << gpv_classification_name(value.selected_classification) << '\n';
  std::cout << "gpv1.development.mechanics=" << value.mechanics << '\n';
}

void gpv_open_confirmation(RmcData &data) {
  if (data.confirmation.data.defined()) {
    throw std::runtime_error("GPV confirmation opened more than once");
  }
  data.confirmation = generate_dataset(9000000, 256);
  const auto raw_projection = make_raw_equal_width_projection();
  data.raw_confirmation =
      raw_equal_width_features(data.confirmation, raw_projection);
  normalize(data.confirmation, data.normalization);
  validate_dataset(data.confirmation);
  data.reversed_confirmation = rssm_reversed_dataset(data.confirmation);
}

[[nodiscard]] GpvConfirmationResult gpv_run_confirmation(
    RmcData &data, const torch::Device &device,
    const std::vector<torch::Tensor> &bootstrap_rows, GpvInventory &inventory,
    GpvCells &cells, const GpvDevelopmentResult &development) {
  if (!development.selected.has_value() ||
      (development.selected_classification !=
           GpvClassification::representation_rescue &&
       development.selected_classification !=
           GpvClassification::objective_made_safe)) {
    std::cout << "gpv1.confirmation.opened=false\n";
    std::cout << "gpv1.confirmation.optimizer_updates=0\n";
    std::cout << "gpv1.confirmation.pass=false\n";
    return {development.mechanics, false};
  }
  gpv_open_confirmation(data);
  const auto targets = rmc_make_targets(data, true);
  const auto raw = gpv_raw_curve(data, true);
  const uint8_t selected = *development.selected;
  std::array<RmcEvaluation, 3> anchors{};
  std::array<RmcEvaluation, 3> current{};
  std::array<RmcEvaluation, 3> candidate{};
  for (std::size_t seed = 0; seed < 3; ++seed) {
    anchors[seed] =
        gpv_evaluate(inventory.anchors[seed], data, targets, device, true);
    current[seed] =
        gpv_evaluate(cells[seed][0].model, data, targets, device, true);
    candidate[seed] = gpv_evaluate(cells[seed][selected].model, data, targets,
                                   device, true);
    gpv_emit_evaluation("gpv1.confirmation.anchor.seed_" +
                            std::to_string(kAttributionSeeds[seed]),
                        anchors[seed]);
    gpv_emit_evaluation("gpv1.confirmation.current.seed_" +
                            std::to_string(kAttributionSeeds[seed]),
                        current[seed]);
    gpv_emit_evaluation("gpv1.confirmation.candidate.seed_" +
                            std::to_string(kAttributionSeeds[seed]),
                        candidate[seed]);
  }
  const auto current_anchor_summary = oca_pair_summary(
      anchors, current, raw, data.confirmation.target, targets, bootstrap_rows,
      development.mechanics);
  const auto candidate_anchor_summary = oca_pair_summary(
      anchors, candidate, raw, data.confirmation.target, targets,
      bootstrap_rows, development.mechanics);
  const auto direct_summary = oca_pair_summary(
      current, candidate, raw, data.confirmation.target, targets,
      bootstrap_rows, development.mechanics);
  auto direct = gpv_pair_contrast(current, candidate, direct_summary);
  const auto versus_anchor =
      gpv_pair_contrast(anchors, candidate, candidate_anchor_summary);
  const auto current_safeguards = gpv_safeguards(current_anchor_summary);
  const auto candidate_safeguards = gpv_safeguards(candidate_anchor_summary);
  const auto direct_safeguards = gpv_safeguards(direct_summary);
  const bool confirmation_mechanics =
      development.mechanics && current_safeguards.values[0] &&
      current_safeguards.values[1] && candidate_safeguards.values[0] &&
      candidate_safeguards.values[1] && direct_safeguards.values[0] &&
      direct_safeguards.values[1];
  direct.no_new_safeguard_failure = gpv_no_new_safeguard_failure(
      current_safeguards, candidate_safeguards);
  direct.gate = gpv_positive_supported(direct) && gpv_family_floor(direct) &&
                direct.no_new_safeguard_failure;
  const auto classification =
      gpv_classify_cell(direct, versus_anchor, candidate_safeguards,
                        confirmation_mechanics);
  const bool pass =
      confirmation_mechanics &&
      (classification == GpvClassification::representation_rescue ||
       (development.selected_classification ==
            GpvClassification::objective_made_safe &&
        classification == GpvClassification::objective_made_safe));
  constexpr std::array<const char *, 10> safeguard_names{
      "numeric_valid",          "mechanics_pass",
      "family_floor_pass",      "raw_noninferiority_pass",
      "order_point_pass",       "order_lower_pass",
      "order_retention_pass",   "continuous_shuffle_pass",
      "order_shuffle_pass",     "geometry_pass"};
  std::cout << "gpv1.confirmation.opened=true\n";
  std::cout << "gpv1.confirmation.group_begin=9000000\n";
  std::cout << "gpv1.confirmation.rows=256\n";
  std::cout << "gpv1.confirmation.raw_equal_width_aulc=" << raw.area << '\n';
  std::cout << "gpv1.confirmation.optimizer_updates=0\n";
  std::cout << "gpv1.confirmation.mechanics=" << confirmation_mechanics
            << '\n';
  gpv_emit_contrast("gpv1.confirmation.candidate_minus_current", direct);
  gpv_emit_contrast("gpv1.confirmation.candidate_minus_fspa4",
                    versus_anchor);
  std::cout << "gpv1.confirmation.no_new_safeguard_failure="
            << direct.no_new_safeguard_failure << '\n';
  std::cout << "gpv1.confirmation.direct_gate=" << direct.gate << '\n';
  for (std::size_t safeguard = 0; safeguard < safeguard_names.size();
       ++safeguard) {
    std::cout << "gpv1.confirmation.current.safeguard."
              << safeguard_names[safeguard] << '='
              << current_safeguards.values[safeguard] << '\n';
    std::cout << "gpv1.confirmation.candidate.safeguard."
              << safeguard_names[safeguard] << '='
              << candidate_safeguards.values[safeguard] << '\n';
    std::cout << "gpv1.confirmation.direct.safeguard."
              << safeguard_names[safeguard] << '='
              << direct_safeguards.values[safeguard] << '\n';
  }
  std::cout << "gpv1.confirmation.current.safeguard.all="
            << current_safeguards.all << '\n';
  std::cout << "gpv1.confirmation.candidate.safeguard.all="
            << candidate_safeguards.all << '\n';
  std::cout << "gpv1.confirmation.direct.safeguard.all="
            << direct_safeguards.all << '\n';
  std::cout << "gpv1.confirmation.classification="
            << gpv_classification_name(classification) << '\n';
  std::cout << "gpv1.confirmation.pass=" << pass << '\n';
  std::cout << "gpv1.promotion="
            << (pass ? "versioned_followup_authorized" : "none") << '\n';
  return {confirmation_mechanics, pass};
}

[[nodiscard]] bool gpv_cuda_options_valid(const Options &options) {
  return options.device == "cuda" &&
         (options.steps < 0 || options.steps == kGpvSteps) &&
         (options.seeds < 0 || options.seeds == 3) && options.weak_views;
}

void gpv_require_cuda(const Options &options) {
  if (!gpv_cuda_options_valid(options)) {
    throw std::runtime_error(
        "GPV-1 requires CUDA:0, 512 updates, 3 seeds, and weak views");
  }
  const char *workspace = std::getenv("CUBLAS_WORKSPACE_CONFIG");
  if (workspace == nullptr || std::string_view(workspace) != ":4096:8") {
    throw std::runtime_error(
        "GPV-1 requires CUBLAS_WORKSPACE_CONFIG=:4096:8");
  }
  rmc_configure_cuda();
}

[[nodiscard]] std::filesystem::path
gpv_executable_path(const char *argv0) {
  const auto path = std::filesystem::absolute(std::filesystem::path(argv0));
  if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
    throw std::runtime_error("GPV executable custody path is unavailable");
  }
  return path;
}

int run_gpv_self_test() {
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.gpv1.self_test.v2\n";
  std::cout << "experiment=global-pool-projector-variance-causal-"
               "decomposition-self-test\n";
  const bool pass = gpv_cpu_self_test();
  std::cout << "gpv1.self_test.pass=" << pass << '\n';
  std::cout << "execution_status="
            << (pass ? "gpv1_self_test_complete" : "gpv1_self_test_failed")
            << '\n';
  return pass ? 0 : 3;
}

int run_gpv_preflight(const Options &options, const char *argv0) {
  gpv_require_cuda(options);
  const GpvExclusiveRunLock exclusive_lock(
      ".build/tests/gpv1/gpv1_exclusive.lock");
  const torch::Device device(torch::kCUDA, 0);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.gpv1.preflight.v2\n";
  std::cout << "experiment=global-pool-projector-variance-causal-"
               "decomposition-preflight\n";
  std::cout << "module_only=true\ndownstream_models_constructed=0\n";
  std::cout << "gpv1.exclusive_lock=true\n";
  std::cout << "retained_optimizer_updates=0\n";
  auto data = rmc_make_data();
  const auto bootstrap_rows = rmc_bootstrap_rows(256);
  if (!rmc_bootstrap_rows_valid(bootstrap_rows, 256)) {
    throw std::runtime_error("GPV bootstrap table failed");
  }
  const auto bindings =
      gpv_make_bindings(data, bootstrap_rows, gpv_executable_path(argv0));
  gpv_emit_bindings(bindings);
  auto [custody, inventory] = gpv_validate_custody(data, device);
  gpv_emit_custody(custody);
  const bool self_test = gpv_cpu_self_test();
  const auto stage_zero = gpv_run_stage_zero(data, device, inventory);
  gpv_emit_stage_zero(stage_zero);
  const bool pass = bindings.pass && custody.pass && self_test && stage_zero.pass;
  std::cout << "gpv1.preflight.pass=" << pass << '\n';
  std::cout << "execution_status="
            << (pass ? "gpv1_preflight_complete" : "gpv1_preflight_failed")
            << '\n';
  return pass ? 0 : 3;
}

int run_gpv_attribution(const Options &options, const char *argv0,
                         bool allow_training) {
  gpv_require_cuda(options);
  const GpvExclusiveRunLock exclusive_lock(
      ".build/tests/gpv1/gpv1_exclusive.lock");
  const torch::Device device(torch::kCUDA, 0);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.gpv1."
            << (allow_training ? "attribution.v2\n" : "confirmation.v2\n");
  std::cout << "experiment=global-pool-projector-variance-causal-"
               "decomposition"
            << (allow_training ? "-attribution\n" : "-confirmation\n");
  std::cout << "module_only=true\ndownstream_models_constructed=0\n";
  std::cout << "gpv1.exclusive_lock=true\n";
  std::cout << "objective_mask=8\ncurrent_mask_retrained=false\n";
  std::cout << "gpv1.training.maximum_new_model_updates="
            << (allow_training ? 10752 : 0) << '\n';
  auto data = rmc_make_data();
  const auto bootstrap_rows = rmc_bootstrap_rows(256);
  if (!rmc_bootstrap_rows_valid(bootstrap_rows, 256)) {
    throw std::runtime_error("GPV bootstrap table failed");
  }
  const auto bindings =
      gpv_make_bindings(data, bootstrap_rows, gpv_executable_path(argv0));
  gpv_emit_bindings(bindings);
  auto [custody, inventory] = gpv_validate_custody(data, device);
  gpv_emit_custody(custody);
  const bool self_test = gpv_cpu_self_test();
  const auto stage_zero = gpv_run_stage_zero(data, device, inventory);
  gpv_emit_stage_zero(stage_zero);
  if (!bindings.pass || !custody.pass || !self_test || !stage_zero.pass) {
    std::cout << "gpv1.training.opened=false\n";
    std::cout << "execution_status=gpv1_preoptimizer_gate_failed\n";
    return 3;
  }

  GpvCells cells{};
  GpvCachePresence cache_present{};
  bool schedules_exact = true;
  int64_t new_model_updates = 0;
  int64_t resumed_model_updates = 0;
  int64_t resumed_cells = 0;
  int64_t inventoried_cells = 0;
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    cells[seed_index][0].model = inventory.current[seed_index];
    cells[seed_index][0].resumed = true;
    for (uint8_t mask = 1U; mask < 8U; ++mask) {
      cache_present[seed_index][mask] = gpv_load_cell(
          data.ssl, device, kAttributionSeeds[seed_index], mask,
          inventory.anchors[seed_index], bindings, seed_index,
          cells[seed_index][mask]);
      inventoried_cells += cache_present[seed_index][mask] ? 1 : 0;
    }
  }
  if (!gpv_v2_namespace_exact(cache_present)) {
    throw std::runtime_error("GPV v2 cache namespace inventory failed");
  }
  std::cout << "gpv1.cache.v2.inventory_validated=true\n";
  std::cout << "gpv1.cache.v2.existing_cells=" << inventoried_cells << '\n';
  std::cout << "gpv1.legacy_v1_reused=false\n";
  if (!allow_training && inventoried_cells != 21) {
    throw std::runtime_error(
        "GPV confirmation requires all 21 validated v2 cells");
  }
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    for (uint8_t mask = 1U; mask < 8U; ++mask) {
      cells[seed_index][mask] = gpv_train_or_use_inventoried_cell(
          data.ssl, device, kAttributionSeeds[seed_index], mask,
          inventory.anchors[seed_index], bindings, seed_index,
          cache_present[seed_index][mask], allow_training,
          std::move(cells[seed_index][mask]));
      if (cells[seed_index][mask].resumed) {
        ++resumed_cells;
        resumed_model_updates += cells[seed_index][mask].receipt.adam_steps;
      } else {
        new_model_updates += cells[seed_index][mask].receipt.adam_steps;
      }
      gpv_emit_receipt(
          "gpv1.training.seed_" +
              std::to_string(kAttributionSeeds[seed_index]) + ".mask_" +
              std::to_string(mask),
          cells[seed_index][mask].receipt);
      if (mask > 1U) {
        schedules_exact =
            schedules_exact &&
            gpv_receipt_schedule_exact(cells[seed_index][1].receipt,
                                       cells[seed_index][mask].receipt);
      }
    }
  }
  std::cout << "gpv1.training.new_model_updates=" << new_model_updates
            << '\n';
  std::cout << "gpv1.training.resumed_model_updates="
            << resumed_model_updates << '\n';
  std::cout << "gpv1.training.resumed_cells=" << resumed_cells << '\n';
  std::cout << "gpv1.training.represented_model_updates="
            << new_model_updates + resumed_model_updates << '\n';
  std::cout << "gpv1.training.schedule_exact=" << schedules_exact << '\n';
  if (!schedules_exact) {
    std::cout << "execution_status=gpv1_training_pairing_failed\n";
    return 3;
  }

  const auto targets = rmc_make_targets(data, false);
  GpvEvaluations evaluations{};
  std::array<RmcEvaluation, 3> anchor_evaluations{};
  for (std::size_t seed = 0; seed < 3; ++seed) {
    anchor_evaluations[seed] =
        gpv_evaluate(inventory.anchors[seed], data, targets, device, false);
    for (std::size_t mask = 0; mask < 8; ++mask) {
      evaluations[seed][mask] =
          gpv_evaluate(cells[seed][mask].model, data, targets, device, false);
    }
  }
  const auto development = gpv_analyze(
      data, targets, bootstrap_rows, evaluations, anchor_evaluations,
      schedules_exact && stage_zero.pass && custody.pass && bindings.pass);
  gpv_emit_development(development, data);
  GpvConfirmationResult confirmation{};
  if (development.selected_classification ==
          GpvClassification::representation_rescue ||
      development.selected_classification ==
          GpvClassification::objective_made_safe) {
    confirmation = gpv_run_confirmation(
        data, device, bootstrap_rows, inventory, cells, development);
  } else {
    std::cout << "gpv1.confirmation.opened=false\n";
    std::cout << "gpv1.confirmation.optimizer_updates=0\n";
  }
  std::cout << "gpv1.production_defaults_changed=false\n";
  std::cout << "gpv1.canonical_rollback=fspa4_structured_cdsb_sparse_v1\n";
  std::cout << "gpv1.operational_rollback=all_tokens\n";
  std::cout << "gpv1.confirmation_pass=" << confirmation.pass << '\n';
  const bool measurements_valid = development.mechanics && confirmation.mechanics;
  std::cout << "execution_status="
            << (measurements_valid ? "gpv1_measurements_complete"
                                   : "gpv1_measurements_invalid")
            << '\n';
  return measurements_valid ? 0 : 3;
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment ==
        "global-pool-projector-variance-causal-decomposition-self-test") {
      return run_gpv_self_test();
    }
    if (options.experiment ==
        "global-pool-projector-variance-causal-decomposition-preflight") {
      return run_gpv_preflight(options, argv[0]);
    }
    if (options.experiment ==
        "global-pool-projector-variance-causal-decomposition-attribution") {
      return run_gpv_attribution(options, argv[0], /*allow_training=*/true);
    }
    if (options.experiment ==
        "global-pool-projector-variance-causal-decomposition-confirmation") {
      return run_gpv_attribution(options, argv[0], /*allow_training=*/false);
    }
    throw std::runtime_error(
        "--experiment must be global-pool-projector-variance-causal-"
        "decomposition-self-test, -preflight, -attribution, or "
        "-confirmation");
  } catch (const c10::Error &error) {
    std::cerr << "global_pool_projector_variance_causal_decomposition_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "global_pool_projector_variance_causal_decomposition_error="
              << error.what() << '\n';
  }
  return 2;
}
