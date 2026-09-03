#include "piaabo/digest/sha256.h"

// IMA-4A reuses only the fixed data, FSPA-4 anchors, probe machinery, and
// archive helpers from OCA-1.  IMA-3 itself is deliberately not embedded:
// its caches are evidence inputs and are verified read-only below, so this
// executable has no path that can train or resume an IMA-3 arm.
#define CUWACUNU_OCA_EMBEDDED
#include "quality_wikimyei_mtf_jepa_mae_vicreg_four_objective_causal_attribution.cpp"
#undef CUWACUNU_OCA_EMBEDDED

#include <functional>
#include <map>

namespace {

using torch::indexing::Slice;

constexpr int64_t kIma3Steps = kOcaAnchorChallengeSteps;
constexpr int64_t kIma3TargetsPerSample = 2;
constexpr std::array<mtf::mtf_jepa_mask_policy_t, 2> kIma3Policies{
    mtf::mtf_jepa_mask_policy_t::paired_target_legacy_context_v1,
    mtf::mtf_jepa_mask_policy_t::support_separated_pair_v1};
constexpr std::array<std::string_view, 2> kIma3ArmNames{
    "paired_target_leaky_context", "support_separated_pair"};
constexpr std::array<std::string_view, 3> kIma3LegacyCacheSha256{
    "5290559894fa6e3f5d2fd57f32a90e97bb0eec924ec22cc9354ae26d0e629c92",
    "bb5391840ede1ad4aaf8ff760d39b796a7a300918e1aab881fbb5255051fcc39",
    "aaa7dd4b7638f240d1e940db7ede3bc40eb8ad01000baa90dc043801a29256a6"};
constexpr std::array<std::string_view, 3> kIma3SeedCacheSha256{
    "16bacf85d0dbffaccb5c78c0f144a504c72ed9476677f6c30cb337661a775ce2",
    "80cf4b15acc046f798e7b8cd3bf0e83de761fa0919eee8d583c7257553814d6c",
    "4f92f545043770b2671154c43d4dafbb78c4ae18fbfd8e098fd14ff86a117626"};
constexpr std::string_view kIma3CacheSchema =
    "ima3.dose_matched_mask_repair.seed_cache.v1";
constexpr std::string_view kIma3SourcePath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_dose_matched_mask_repair.cpp";
constexpr std::string_view kIma3HeaderPath =
    "src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/"
    "mtf_jepa_mae_vicreg.h";

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t
ima3_config(const torch::Device &device, mtf::mtf_jepa_mask_policy_t policy) {
  auto config = attribution_config(device, oca_arm(1U));
  config.jepa_mask_policy = policy;
  return config;
}

[[nodiscard]] std::string ima3_sha256(std::string_view path) {
  return digest::sha256_hex(rmc_read_file(std::filesystem::path(path)));
}

[[nodiscard]] std::filesystem::path ima3_seed_cache_path(int64_t seed) {
  return std::filesystem::path(".build") / "tests" / "ima3" /
         ("dose_matched_seed_" + std::to_string(seed) + "_v1.complete.pt");
}

[[nodiscard]] std::filesystem::path
ima3_seed_cache_marker_path(const std::filesystem::path &path) {
  auto marker = path;
  marker += ".sha256";
  return marker;
}

[[nodiscard]] std::string
ima4a_marker_checksum(const std::filesystem::path &path) {
  if (!std::filesystem::exists(path)) {
    return {};
  }
  std::ifstream input(path, std::ios::binary);
  std::string value;
  std::getline(input, value);
  if (!value.empty() && value.back() == '\r') {
    value.pop_back();
  }
  return value;
}

[[nodiscard]] bool ima3_pins_exact(std::size_t seed_index) {
  const int64_t seed = kAttributionSeeds.at(seed_index);
  const auto legacy_path = oca_seed_cache_path("anchor_challenge", seed);
  return std::filesystem::exists(legacy_path) &&
         std::filesystem::exists(oca_seed_cache_marker_path(legacy_path)) &&
         digest::sha256_hex(rmc_read_file(legacy_path)) ==
             kIma3LegacyCacheSha256.at(seed_index) &&
         std::filesystem::exists(oca_archive_path(seed)) &&
         digest::sha256_hex(rmc_read_file(oca_archive_path(seed))) ==
             kOcaAnchorSha256.at(seed_index);
}

[[nodiscard]] bool
ima3_mask_counts_exact(const mtf::jepa_context_target_mask_t &masks,
                       const mtf::mtf_token_batch_t &tokens,
                       double min_context_ratio) {
  const auto valid =
      tokens.token_mask.to(torch::kCPU, torch::kBool).contiguous();
  const auto targets =
      masks.target_mask.to(torch::kCPU, torch::kBool).contiguous();
  const auto contexts =
      masks.context_mask.to(torch::kCPU, torch::kBool).contiguous();
  for (int64_t batch = 0; batch < valid.size(0); ++batch) {
    const int64_t valid_count = valid[batch].sum().item<int64_t>();
    const int64_t expected_context = std::max<int64_t>(
        1, static_cast<int64_t>(std::ceil(min_context_ratio * valid_count)));
    if (targets[batch].sum().item<int64_t>() != kIma3TargetsPerSample ||
        contexts[batch].sum().item<int64_t>() != expected_context ||
        targets[batch].logical_and(contexts[batch]).any().item<bool>()) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] uint64_t ima3_expected_row_schedule_digest(const Dataset &ssl,
                                                         int64_t seed) {
  uint64_t value = 0xcbf29ce484222325ULL;
  for (int64_t step = 0; step < kIma3Steps; ++step) {
    mix_hash_value(value, hash_batch_rows(training_rows(ssl, seed, step)));
  }
  return value;
}

[[nodiscard]] std::filesystem::path ima3_preflight_receipt_path() {
  return std::filesystem::path(".build") / "tests" / "ima3" /
         "preflight_v1.complete.txt";
}

[[nodiscard]] std::string ima3_preflight_receipt_contents() {
  std::ostringstream output;
  output << "schema=ima3.dose_matched_mask_repair.preflight.v1\n";
  output << "source_sha256=" << ima3_sha256(kIma3SourcePath) << '\n';
  output << "header_sha256=" << ima3_sha256(kIma3HeaderPath) << '\n';
  output << "audited_updates=" << kIma3Steps * 3 << '\n';
  output << "pass=true\n";
  return output.str();
}

[[nodiscard]] bool ima3_preflight_receipt_exact() {
  const auto path = ima3_preflight_receipt_path();
  return std::filesystem::exists(path) &&
         rmc_read_file(path) == ima3_preflight_receipt_contents();
}

[[nodiscard]] bool ima4a_validate_ima3_seed_cache(
    const Dataset &ssl, const torch::Device &device, int64_t seed,
    std::size_t seed_index) {
  const auto path = ima3_seed_cache_path(seed);
  const auto marker = ima3_seed_cache_marker_path(path);
  if (!std::filesystem::exists(path) || !std::filesystem::exists(marker)) {
    return false;
  }
  const auto checksum = digest::sha256_hex(rmc_read_file(path));
  if (checksum != ima4a_marker_checksum(marker) ||
      checksum != kIma3SeedCacheSha256.at(seed_index)) {
    throw std::runtime_error("IMA-4A IMA-3 cache checksum failed");
  }

  torch::serialize::InputArchive root;
  root.load_from(path.string(), device);
  const auto read_tensor = [&](const std::string &name) {
    torch::Tensor value{};
    root.read(name, value);
    return value;
  };
  const auto ssl_hashes = oca_seed_cache_ssl_hashes(ssl);
  const auto schedule =
      oca_u64_le_bytes_vector(read_tensor("meta/schedule_digest"));
  const auto counts = read_tensor("meta/pairing_counts")
                          .to(torch::kCPU, torch::kInt64)
                          .contiguous();
  const auto flags = read_tensor("meta/result_flags")
                         .to(torch::kCPU, torch::kInt64)
                         .contiguous();
  const auto saved_seed =
      read_tensor("meta/seed").to(torch::kCPU, torch::kInt64).contiguous();
  const auto saved_steps =
      read_tensor("meta/steps").to(torch::kCPU, torch::kInt64).contiguous();
  bool valid =
      oca_tensor_string(read_tensor("meta/schema")) == kIma3CacheSchema &&
      oca_tensor_string(read_tensor("meta/source_sha256")) ==
          ima3_sha256(kIma3SourcePath) &&
      oca_tensor_string(read_tensor("meta/header_sha256")) ==
          ima3_sha256(kIma3HeaderPath) &&
      oca_tensor_string(read_tensor("meta/ssl_data_hash")) == ssl_hashes[0] &&
      oca_tensor_string(read_tensor("meta/ssl_mask_hash")) == ssl_hashes[1] &&
      oca_tensor_string(read_tensor("meta/ssl_target_hash")) ==
          ssl_hashes[2] &&
      oca_tensor_string(read_tensor("meta/anchor_sha256")) ==
          kOcaAnchorSha256.at(seed_index) &&
      saved_seed.numel() == 1 && saved_seed.item<int64_t>() == seed &&
      saved_steps.numel() == 1 && saved_steps.item<int64_t>() == kIma3Steps &&
      schedule.size() == 1 &&
      schedule[0] == ima3_expected_row_schedule_digest(ssl, seed) &&
      counts.numel() == 4 && flags.numel() == 4;
  if (!valid) {
    throw std::runtime_error("IMA-4A IMA-3 cache metadata failed");
  }
  for (int64_t index = 0; index < 4; ++index) {
    valid = valid && counts[index].item<int64_t>() == kIma3Steps &&
            flags[index].item<int64_t>() == 1;
  }
  for (std::size_t arm = 0; arm < 2; ++arm) {
    const std::string prefix = "arm_" + std::to_string(arm) + "/";
    const auto scalars = read_tensor(prefix + "scalars")
                             .to(torch::kCPU, torch::kFloat64)
                             .contiguous();
    const auto arm_flags = read_tensor(prefix + "flags")
                               .to(torch::kCPU, torch::kInt64)
                               .contiguous();
    valid = valid &&
            oca_tensor_string(read_tensor(prefix + "name")) ==
                kIma3ArmNames[arm] &&
            oca_tensor_string(read_tensor(prefix + "config_manifest")) ==
                canonical_config_manifest(
                    ima3_config(device, kIma3Policies[arm])) &&
            scalars.numel() == 12 && arm_flags.numel() == 5;
    if (!valid) {
      throw std::runtime_error("IMA-4A IMA-3 cache arm identity failed");
    }
    valid = valid && torch::isfinite(scalars).all().item<bool>() &&
            arm_flags[0].item<int64_t>() == kIma3Steps &&
            arm_flags[1].item<int64_t>() >= 0 &&
            arm_flags[2].item<int64_t>() == 1 &&
            arm_flags[3].item<int64_t>() == 1 &&
            arm_flags[4].item<int64_t>() == 1 &&
            scalars[2].item<double>() > 0.0 &&
            scalars[4].item<double>() > 0.0 &&
            scalars[6].item<double>() > 0.0 &&
            scalars[7].item<double>() > 0.0 &&
            scalars[8].item<double>() > 0.0 &&
            scalars[9].item<double>() == 0.0 &&
            scalars[10].item<double>() == 0.0 &&
            scalars[11].item<double>() > 0.0;
  }
  if (!valid) {
    throw std::runtime_error("IMA-4A IMA-3 cache receipt failed");
  }
  return true;
}

constexpr std::array<mtf::mtf_jepa_mask_policy_t, 2> kIma4aPolicies{
    mtf::mtf_jepa_mask_policy_t::paired_target_legacy_context_v1,
    mtf::mtf_jepa_mask_policy_t::support_separated_pair_v1};
constexpr std::array<std::string_view, 2> kIma4aPolicyNames{
    "paired_leaky", "support_separated"};
constexpr std::array<std::string_view, 7> kIma4aSurfaceNames{
    "Q", "G", "S", "F", "R", "B", "C"};
constexpr int64_t kIma4aSurfaceCount = 7;
// Attempt 1 fail-closed at the inherited upper edge alpha=1 for every S/F/R
// fit.  Attempt 2 then exposed a lower-edge-only C fit in legacy M1.  The
// recorded continuations broaden those deterministic tails; the final two
// values also provide a numerically stable intercept-limit check.
constexpr std::array<double, 16> kIma4aRidgeGrid{
    1.0e-10, 1.0e-8, 1.0e-6, 1.0e-5, 1.0e-4, 1.0e-3,
    1.0e-2,  1.0e-1, 1.0,    1.0e1,  1.0e2,  1.0e3,
    1.0e4,   1.0e6, 1.0e8,  1.0e10};
constexpr int64_t kIma4aCellCount = kChannels * 2 * 4;
constexpr int64_t kIma4aMetadataWidth = 6;
constexpr int64_t kIma4aTokenCount = 72;
constexpr int64_t kIma4aContextCount = 54;
constexpr int64_t kIma4aTargetsPerGroup = 2;
constexpr double kIma4aMaterialR2 = 0.05;
constexpr double kIma4aHiddenGradientFloor = 0.25;
constexpr double kIma4aTolerance = 2.0e-6;
constexpr std::string_view kIma4aProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/JEPA_TARGET_PREDICTABILITY_CEILING_PROTOCOL.md";
constexpr std::string_view kIma4aSourcePath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_target_predictability_ceiling.cpp";

struct Ima4aModules {
  std::shared_ptr<mtf::TimeFrequencyViewBuilderImpl> tokenizer{};
  std::shared_ptr<mtf::TimeFrequencyViewBuilderImpl> target_tokenizer{};
  std::shared_ptr<mtf::SharedTokenEncoderImpl> encoder{};
  std::shared_ptr<mtf::SharedTokenEncoderImpl> target_encoder{};
  std::shared_ptr<mtf::LatentPredictorImpl> predictor{};
};

struct Ima4aContextReceipt {
  std::array<std::array<std::array<std::array<int64_t, 4>, 2>, kChannels>, 2>
      context{};
  std::array<std::array<std::array<int64_t, 4>, 2>, kChannels> removed{};
  std::array<std::array<std::array<int64_t, 4>, 2>, kChannels> replacement{};
  std::array<std::array<std::array<int64_t, kHistory + 1>, kChannels>, 2>
      non_target_coverage_hist{};
  std::array<std::array<int64_t, kChannels>, 2>
      non_target_coverage_sum{};
  std::array<std::array<int64_t, kChannels>, 2> target_overlap_coverage_sum{};
  std::array<std::array<int64_t, kIma4aTokenCount>, 2> token_selection{};
  int64_t updates{0};
  int64_t samples{0};
  int64_t paired_targets{0};
  int64_t paired_counts{0};
  int64_t paired_rng{0};
  bool pass{false};
};

struct Ima4aCapture {
  std::array<torch::Tensor, kIma4aSurfaceCount> features{};
  torch::Tensor target{};
  torch::Tensor current{};
  torch::Tensor target0{};
  torch::Tensor support0{};
  torch::Tensor self{};
  torch::Tensor alias{};
  torch::Tensor hidden{};
  torch::Tensor target_id{};
  torch::Tensor domain{};
  torch::Tensor channel{};
  torch::Tensor scale{};
  torch::Tensor group_id{};
  bool decomposition_exact{false};
  bool finite{false};
};

struct Ima4aCaptureBuilder {
  std::array<std::vector<torch::Tensor>, kIma4aSurfaceCount> features{};
  std::vector<torch::Tensor> target{};
  std::vector<torch::Tensor> current{};
  std::vector<torch::Tensor> target0{};
  std::vector<torch::Tensor> support0{};
  std::vector<torch::Tensor> self{};
  std::vector<torch::Tensor> alias{};
  std::vector<torch::Tensor> hidden{};
  std::vector<torch::Tensor> target_id{};
  std::vector<torch::Tensor> domain{};
  std::vector<torch::Tensor> channel{};
  std::vector<torch::Tensor> scale{};
  std::vector<torch::Tensor> group_id{};
  bool decomposition_exact{true};
  bool finite{true};
};

struct Ima4aOracleMetric {
  double nmse{std::numeric_limits<double>::quiet_NaN()};
  double r2{std::numeric_limits<double>::quiet_NaN()};
};

struct Ima4aOracleFit {
  torch::Tensor prediction{};
  std::array<double, kIma4aRidgeGrid.size()> validation_nmse{};
  std::size_t alpha_index{0};
  double alpha{0.0};
  double intercept_validation_nmse{std::numeric_limits<double>::quiet_NaN()};
  bool tail_matches_intercept{false};
  bool edge_improving{false};
  Ima4aOracleMetric metric{};
};

enum class Ima4aOracleMap { standard, categorical_slot_by_field };

struct Ima4aDecompositionMetric {
  double hidden_energy_fraction{0.0};
  double self_energy_fraction{0.0};
  double alias_energy_fraction{0.0};
  double residual_hidden_projection_fraction{0.0};
  double hidden_dominant_dimension_fraction{0.0};
  Geometry hidden_geometry{};
};

struct Ima4aGradientMetric {
  torch::Tensor full{};
  torch::Tensor support0{};
  torch::Tensor hidden{};
  torch::Tensor time{};
  torch::Tensor frequency{};
  torch::Tensor predictor_full{};
  double hidden_fraction{0.0};
  double time_frequency_cosine{0.0};
  double full_order_cosine{0.0};
  double full_cross_cosine{0.0};
  double hidden_order_cosine{0.0};
  double hidden_cross_cosine{0.0};
  double full_order_first_order{0.0};
  double full_cross_first_order{0.0};
  double hidden_order_first_order{0.0};
  double hidden_cross_first_order{0.0};
  double served_norm{0.0};
  double predictor_norm{0.0};
  bool finite{false};
};

struct Ima4aFrozenProbe {
  RidgeModel model{};
  std::array<int64_t, 2> task_indices{};
  double alpha{0.0};
  bool finite{false};
};

struct Ima4aSeedEvidence {
  int64_t seed{0};
  std::array<std::array<Ima4aOracleFit, kIma4aSurfaceCount>, 2> oracle{};
  std::array<Ima4aOracleMetric, 2> current{};
  std::array<std::array<Ima4aDecompositionMetric, 3>, 2> decomposition{};
  std::array<Ima4aGradientMetric, 2> gradient{};
  torch::Tensor order_gradient{};
  torch::Tensor cross_gradient{};
  torch::Tensor target{};
  torch::Tensor target_id{};
  std::array<torch::Tensor, 2> current_prediction{};
  std::array<std::array<torch::Tensor, kIma4aSurfaceCount>, 2>
      oracle_prediction{};
  bool custody{false};
  bool component_equivalence{false};
  bool capture_pairing{false};
  bool pass{false};
};

template <typename ModuleImpl>
[[nodiscard]] std::shared_ptr<ModuleImpl>
ima4a_child(const mtf::MtfJepaMaeVicreg &model, const std::string &name) {
  for (const auto &item : model->named_children()) {
    if (item.key() == name) {
      auto value = std::dynamic_pointer_cast<ModuleImpl>(item.value());
      if (!value) {
        throw std::runtime_error("IMA-4A child type mismatch: " + name);
      }
      return value;
    }
  }
  throw std::runtime_error("IMA-4A child missing: " + name);
}

[[nodiscard]] Ima4aModules
ima4a_modules(const mtf::MtfJepaMaeVicreg &model) {
  return {.tokenizer =
              ima4a_child<mtf::TimeFrequencyViewBuilderImpl>(model,
                                                             "tokenizer"),
          .target_tokenizer =
              ima4a_child<mtf::TimeFrequencyViewBuilderImpl>(
                  model, "target_tokenizer"),
          .encoder =
              ima4a_child<mtf::SharedTokenEncoderImpl>(model, "encoder"),
          .target_encoder = ima4a_child<mtf::SharedTokenEncoderImpl>(
              model, "target_encoder"),
          .predictor =
              ima4a_child<mtf::LatentPredictorImpl>(model, "predictor")};
}

[[nodiscard]] int64_t ima4a_mask_seed(int64_t model_seed,
                                      int64_t absolute_group) {
  const auto mixed = splitmix64(
      0x696d6134615f6d73ULL ^ static_cast<uint64_t>(model_seed) ^
      splitmix64(static_cast<uint64_t>(absolute_group)));
  return static_cast<int64_t>(mixed & 0x7fffffffffffffffULL);
}

[[nodiscard]] bool ima4a_metadata_equal(
    const mtf::mtf_token_metadata_t &left,
    const mtf::mtf_token_metadata_t &right) {
  return torch::equal(left.start_index, right.start_index) &&
         torch::equal(left.width, right.width) &&
         torch::equal(left.scale_id, right.scale_id) &&
         torch::equal(left.channel_id, right.channel_id) &&
         torch::equal(left.domain_id, right.domain_id);
}

[[nodiscard]] double ima4a_cosine(const torch::Tensor &left_input,
                                  const torch::Tensor &right_input) {
  const auto left = left_input.to(torch::kCPU, torch::kFloat64).reshape({-1});
  const auto right =
      right_input.to(torch::kCPU, torch::kFloat64).reshape({-1});
  const double denom = left.norm().item<double>() * right.norm().item<double>();
  if (!(denom > 1.0e-30)) {
    return 0.0;
  }
  return left.dot(right).item<double>() / denom;
}

[[nodiscard]] double
ima4a_first_order_loss_change(const torch::Tensor &direction_input,
                              const torch::Tensor &loss_gradient_input) {
  const auto direction =
      direction_input.to(torch::kCPU, torch::kFloat64).reshape({-1});
  const auto loss_gradient =
      loss_gradient_input.to(torch::kCPU, torch::kFloat64).reshape({-1});
  const double norm = direction.norm().item<double>();
  if (!(norm > 1.0e-30)) {
    return 0.0;
  }
  return -direction.dot(loss_gradient).item<double>() / norm;
}

[[nodiscard]] torch::Tensor ima4a_parameter_gradient(
    const mtf::MtfJepaMaeVicreg &model,
    const std::function<bool(const std::string &)> &include) {
  std::vector<torch::Tensor> chunks;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    if (!include(item.key())) {
      continue;
    }
    if (item.value().grad().defined()) {
      chunks.push_back(item.value().grad()
                           .detach()
                           .to(torch::kCPU, torch::kFloat64)
                           .reshape({-1})
                           .clone());
    } else {
      chunks.push_back(torch::zeros(
          {item.value().numel()}, torch::TensorOptions().dtype(torch::kFloat64)));
    }
  }
  if (chunks.empty()) {
    throw std::runtime_error("IMA-4A gradient partition is empty");
  }
  return torch::cat(chunks).contiguous();
}

void ima4a_clear_gradients(const mtf::MtfJepaMaeVicreg &model) {
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    if (item.value().grad().defined()) {
      item.value().mutable_grad() = torch::Tensor();
    }
  }
}

[[nodiscard]] bool ima4a_served_name(const std::string &name) {
  return name.rfind("tokenizer.", 0) == 0 ||
         name.rfind("encoder.", 0) == 0;
}

[[nodiscard]] bool ima4a_predictor_name(const std::string &name) {
  return name.rfind("predictor.", 0) == 0;
}

[[nodiscard]] std::string ima4a_sha256(std::string_view path) {
  return digest::sha256_hex(rmc_read_file(std::filesystem::path(path)));
}

[[nodiscard]] mtf::mtf_token_batch_t
ima4a_token_row(const mtf::mtf_token_batch_t &batch, int64_t row) {
  auto result = batch;
  result.tokens = batch.tokens.narrow(0, row, 1);
  result.reconstruction_targets =
      batch.reconstruction_targets.narrow(0, row, 1);
  result.time_reconstruction_targets =
      batch.time_reconstruction_targets.narrow(0, row, 1);
  result.frequency_reconstruction_targets =
      batch.frequency_reconstruction_targets.narrow(0, row, 1);
  result.time_reconstruction_mask =
      batch.time_reconstruction_mask.narrow(0, row, 1);
  result.frequency_reconstruction_mask =
      batch.frequency_reconstruction_mask.narrow(0, row, 1);
  result.token_mask = batch.token_mask.narrow(0, row, 1);
  return result;
}

[[nodiscard]] torch::Tensor ima4a_identity_centered(
    const torch::Tensor &target_input, const torch::Tensor &identity_input) {
  const auto target = target_input.to(torch::kCPU, torch::kFloat64).contiguous();
  const auto identity =
      identity_input.to(torch::kCPU, torch::kInt64).contiguous();
  auto centered = torch::empty_like(target);
  for (int64_t token = 0; token < kIma4aTokenCount; ++token) {
    const auto rows = identity.eq(token).nonzero().reshape({-1});
    if (rows.numel() == 0) {
      continue;
    }
    const auto selected = target.index_select(0, rows);
    centered.index_copy_(0, rows, selected - selected.mean(0, true));
  }
  return centered;
}

[[nodiscard]] Ima4aOracleMetric ima4a_oracle_metric(
    const torch::Tensor &prediction_input, const torch::Tensor &target_input,
    const torch::Tensor &identity_input) {
  const auto prediction =
      prediction_input.to(torch::kCPU, torch::kFloat64).contiguous();
  const auto target = target_input.to(torch::kCPU, torch::kFloat64).contiguous();
  const auto centered = ima4a_identity_centered(target, identity_input);
  const double denominator = centered.pow(2).sum().item<double>();
  const double numerator = (prediction - target).pow(2).sum().item<double>();
  if (!(denominator > 1.0e-20) || !std::isfinite(numerator)) {
    throw std::runtime_error("IMA-4A invalid target-centered metric");
  }
  const double nmse = numerator / denominator;
  return {.nmse = nmse, .r2 = 1.0 - nmse};
}

[[nodiscard]] RidgeModel ima4a_fit_ridge(const torch::Tensor &features,
                                         const torch::Tensor &target,
                                         double alpha) {
  return features.size(1) >= features.size(0)
             ? fit_ridge_dual(features, target, alpha)
             : fit_ridge(features, target, alpha);
}

struct Ima4aEdgeStatus {
  bool tail_matches_intercept{false};
  bool improving{false};
};

[[nodiscard]] Ima4aEdgeStatus ima4a_edge_status(
    const std::array<double, kIma4aRidgeGrid.size()> &curve,
    std::size_t selected, double intercept) {
  const double scale = std::max({1.0, std::abs(intercept),
                                 std::abs(curve.front()),
                                 std::abs(curve.back())});
  const double tolerance = 1.0e-9 * scale;
  const bool tail_matches = std::abs(curve.back() - intercept) <= tolerance;
  const bool lower =
      selected == 0 && curve[0] + tolerance < curve[1];
  const bool upper = selected + 1 == curve.size() && !tail_matches &&
                     curve.back() + tolerance < curve[curve.size() - 2];
  return {.tail_matches_intercept = tail_matches,
          .improving = lower || upper};
}

[[nodiscard]] Ima4aOracleFit ima4a_fit_oracle(
    const torch::Tensor &train_features, const torch::Tensor &train_target,
    const torch::Tensor &train_identity,
    const torch::Tensor &validation_features,
    const torch::Tensor &validation_target,
    const torch::Tensor &validation_identity,
    const torch::Tensor &test_features, const torch::Tensor &test_target,
    const torch::Tensor &test_identity,
    Ima4aOracleMap map = Ima4aOracleMap::standard) {
  Ima4aOracleFit result{};
  double best = std::numeric_limits<double>::infinity();
  const auto target = train_target.to(torch::kCPU, torch::kFloat64);
  const auto validation_target_cpu =
      validation_target.to(torch::kCPU, torch::kFloat64);
  const auto bias = target.mean(0);
  result.intercept_validation_nmse = ima4a_oracle_metric(
      bias.unsqueeze(0).expand_as(validation_target_cpu),
      validation_target_cpu, validation_identity)
                                         .nmse;
  if (map == Ima4aOracleMap::categorical_slot_by_field ||
      train_features.size(1) >= train_features.size(0)) {
    // The bilinear control is wide.  Standardize and form its dual Gram matrix
    // once, rather than repeating the same O(N^2 D) work for every alpha.
    torch::NoGradGuard no_grad;
    const auto train = train_features.to(torch::kCPU, torch::kFloat64);
    const auto validation =
        validation_features.to(torch::kCPU, torch::kFloat64);
    const auto test = test_features.to(torch::kCPU, torch::kFloat64);
    const auto target = train_target.to(torch::kCPU, torch::kFloat64);
    const auto mean = train.mean(0);
    const auto variance = (train - mean).pow(2).mean(0);
    const auto inv_std = torch::where(variance > 1.0e-12, variance.rsqrt(),
                                      torch::ones_like(variance));
    const auto x = (train - mean) * inv_std;
    const auto validation_x = (validation - mean) * inv_std;
    const auto test_x = (test - mean) * inv_std;
    const auto y = target - bias;
    auto gram = x.matmul(x.transpose(0, 1));
    auto validation_kernel = validation_x.matmul(x.transpose(0, 1));
    auto test_kernel = test_x.matmul(x.transpose(0, 1));
    if (map == Ima4aOracleMap::categorical_slot_by_field) {
      if (x.size(1) <= kIma4aMetadataWidth) {
        throw std::runtime_error("IMA-4A categorical field is empty");
      }
      const auto train_id =
          train_identity.to(torch::kCPU, torch::kInt64).contiguous();
      const auto validation_id =
          validation_identity.to(torch::kCPU, torch::kInt64).contiguous();
      const auto test_id =
          test_identity.to(torch::kCPU, torch::kInt64).contiguous();
      std::array<int64_t, kIma4aTokenCount> counts{};
      const auto train_access = train_id.accessor<int64_t, 1>();
      for (int64_t row = 0; row < train_id.size(0); ++row) {
        if (train_access[row] < 0 ||
            train_access[row] >= kIma4aTokenCount) {
          throw std::runtime_error("IMA-4A categorical identity out of range");
        }
        ++counts[static_cast<std::size_t>(train_access[row])];
      }
      const auto identity_scale = [&](const torch::Tensor &identity) {
        const auto values = identity.accessor<int64_t, 1>();
        std::vector<double> scales;
        scales.reserve(static_cast<std::size_t>(identity.size(0)));
        for (int64_t row = 0; row < identity.size(0); ++row) {
          const int64_t id = values[row];
          if (id < 0 || id >= kIma4aTokenCount ||
              counts[static_cast<std::size_t>(id)] == 0) {
            throw std::runtime_error(
                "IMA-4A categorical validation identity unseen in fit");
          }
          scales.push_back(std::sqrt(
              static_cast<double>(train_id.size(0)) /
              static_cast<double>(counts[static_cast<std::size_t>(id)])));
        }
        return torch::tensor(scales, torch::kFloat64);
      };
      const auto train_scale = identity_scale(train_id);
      const auto validation_scale = identity_scale(validation_id);
      const auto test_scale = identity_scale(test_id);
      const auto field = x.slice(1, kIma4aMetadataWidth);
      const auto validation_field =
          validation_x.slice(1, kIma4aMetadataWidth);
      const auto test_field = test_x.slice(1, kIma4aMetadataWidth);
      const auto train_same =
          train_id.unsqueeze(1).eq(train_id.unsqueeze(0)).to(torch::kFloat64);
      const auto validation_same = validation_id
                                       .unsqueeze(1)
                                       .eq(train_id.unsqueeze(0))
                                       .to(torch::kFloat64);
      const auto test_same = test_id.unsqueeze(1)
                                 .eq(train_id.unsqueeze(0))
                                 .to(torch::kFloat64);
      gram = gram + field.matmul(field.transpose(0, 1)) * train_same *
                        train_scale.unsqueeze(1) * train_scale.unsqueeze(0);
      validation_kernel =
          validation_kernel +
          validation_field.matmul(field.transpose(0, 1)) * validation_same *
              validation_scale.unsqueeze(1) * train_scale.unsqueeze(0);
      test_kernel =
          test_kernel + test_field.matmul(field.transpose(0, 1)) * test_same *
                            test_scale.unsqueeze(1) * train_scale.unsqueeze(0);
    }
    std::array<torch::Tensor, kIma4aRidgeGrid.size()> duals{};
    for (std::size_t index = 0; index < kIma4aRidgeGrid.size(); ++index) {
      auto regularized = gram.clone();
      regularized.diagonal(0, 0, 1).add_(
          train.size(0) * kIma4aRidgeGrid[index]);
      auto [cholesky, info] =
          at::linalg_cholesky_ex(regularized, false, false);
      if (info.max().item<int64_t>() != 0) {
        throw std::runtime_error("IMA-4A dual ridge factorization failed");
      }
      duals[index] = at::cholesky_solve(y, cholesky, false);
      const auto validation_prediction =
          validation_kernel.matmul(duals[index]) + bias;
      result.validation_nmse[index] =
          ima4a_oracle_metric(validation_prediction, validation_target,
                              validation_identity)
              .nmse;
      if (result.validation_nmse[index] < best) {
        best = result.validation_nmse[index];
        result.alpha_index = index;
      }
    }
    result.prediction =
        (test_kernel.matmul(duals[result.alpha_index]) + bias).contiguous();
  } else {
    std::array<RidgeModel, kIma4aRidgeGrid.size()> models{};
    for (std::size_t index = 0; index < kIma4aRidgeGrid.size(); ++index) {
      models[index] = ima4a_fit_ridge(train_features, train_target,
                                     kIma4aRidgeGrid[index]);
      const auto validation_prediction =
          predict(models[index], validation_features);
      result.validation_nmse[index] =
          ima4a_oracle_metric(validation_prediction, validation_target,
                              validation_identity)
              .nmse;
      if (result.validation_nmse[index] < best) {
        best = result.validation_nmse[index];
        result.alpha_index = index;
      }
    }
    result.prediction =
        predict(models[result.alpha_index], test_features).contiguous();
  }
  result.alpha = kIma4aRidgeGrid[result.alpha_index];
  result.metric =
      ima4a_oracle_metric(result.prediction, test_target, test_identity);
  const auto edge = ima4a_edge_status(
      result.validation_nmse, result.alpha_index,
      result.intercept_validation_nmse);
  result.tail_matches_intercept = edge.tail_matches_intercept;
  result.edge_improving = edge.improving;
  return result;
}

[[nodiscard]] Ima4aDecompositionMetric ima4a_decomposition_metric(
    const Ima4aCapture &capture, int64_t domain_filter) {
  torch::Tensor rows{};
  if (domain_filter < 0) {
    rows = torch::arange(capture.target.size(0), torch::kInt64);
  } else {
    rows = capture.domain.eq(domain_filter).nonzero().reshape({-1});
  }
  const auto target = capture.target.index_select(0, rows);
  const auto current = capture.current.index_select(0, rows);
  const auto support0 = capture.support0.index_select(0, rows);
  const auto self = capture.self.index_select(0, rows);
  const auto alias = capture.alias.index_select(0, rows);
  const auto hidden = capture.hidden.index_select(0, rows);
  const auto identity = capture.target_id.index_select(0, rows);
  const double target_variance =
      ima4a_identity_centered(target, identity).pow(2).sum().item<double>();
  if (!(target_variance > 1.0e-20)) {
    throw std::runtime_error("IMA-4A decomposition target variance vanished");
  }
  const auto residual = current - target;
  const auto hidden_norm_square = hidden.pow(2).sum(1).clamp_min(1.0e-30);
  const auto projected_energy =
      residual.mul(hidden).sum(1).pow(2) / hidden_norm_square;
  const double residual_energy = residual.pow(2).sum().item<double>();
  const auto hidden_variance =
      (hidden - hidden.mean(0, true)).pow(2).mean(0);
  const auto support_variance =
      (support0 - support0.mean(0, true)).pow(2).mean(0);
  return {.hidden_energy_fraction =
              hidden.pow(2).sum().item<double>() / target_variance,
          .self_energy_fraction =
              self.pow(2).sum().item<double>() / target_variance,
          .alias_energy_fraction =
              alias.pow(2).sum().item<double>() / target_variance,
          .residual_hidden_projection_fraction =
              residual_energy > 1.0e-30
                  ? projected_energy.sum().item<double>() / residual_energy
                  : 0.0,
          .hidden_dominant_dimension_fraction =
              hidden_variance.gt(support_variance)
                  .to(torch::kFloat64)
                  .mean()
                  .item<double>(),
          .hidden_geometry = rssm_geometry_for_channel(hidden)};
}

[[nodiscard]] bool ima4a_component_equivalence(
    mtf::MtfJepaMaeVicreg &model, const Dataset &dataset,
    const torch::Device &device, int64_t seed) {
  const auto before = oca_snapshot_state(model);
  const auto modules = ima4a_modules(model);
  const auto data = dataset.data.narrow(0, 0, 8).to(device);
  const auto feature_mask = dataset.mask.narrow(0, 0, 8).to(device);
  model->eval();
  torch::NoGradGuard no_grad;
  const auto online = modules.tokenizer->forward(data, feature_mask);
  const auto public_tokens = model->tokenize(data, feature_mask);
  const auto target_tokens =
      modules.target_tokenizer->forward(data, feature_mask);
  const auto direct_online =
      modules.encoder->forward(online.tokens, online.token_mask);
  const auto public_online = model->encode(data, feature_mask).embeddings;
  const auto direct_target = modules.target_encoder->forward(
      target_tokens.tokens, target_tokens.token_mask);
  const auto public_target = model->target_encode(data, feature_mask);
  set_paired_rng(paired_diagnostic_seed(seed), device);
  const auto masks = model->create_masks(online);
  const auto context_tokens = online.tokens.masked_fill(
      masks.context_mask.logical_not().unsqueeze(-1), 0.0);
  const auto context_latents =
      modules.encoder->forward(context_tokens, masks.context_mask);
  const auto prediction = modules.predictor->forward(
      context_latents, masks.context_mask, online.metadata);
  const auto direct_loss =
      mtf::detail::masked_mse(prediction, direct_target, masks.target_mask);
  set_paired_rng(paired_diagnostic_seed(seed), device);
  const auto output = model->forward(data, feature_mask);
  return torch::equal(online.tokens, public_tokens.tokens) &&
         torch::equal(online.token_mask, public_tokens.token_mask) &&
         ima4a_metadata_equal(online.metadata, public_tokens.metadata) &&
         torch::equal(online.token_mask, target_tokens.token_mask) &&
         ima4a_metadata_equal(online.metadata, target_tokens.metadata) &&
         torch::equal(direct_online, public_online) &&
         torch::equal(direct_target, public_target) &&
         torch::equal(masks.target_mask, output.jepa_target_mask) &&
         torch::equal(masks.context_mask, output.jepa_context_mask) &&
         torch::allclose(direct_loss, output.loss_jepa, 1.0e-7, 1.0e-6) &&
         oca_state_exact(model, before);
}

struct Ima4aMaskPair {
  std::array<mtf::jepa_context_target_mask_t, 2> masks{};
  bool target_exact{true};
  bool counts_exact{true};
  bool rng_exact{true};
};

[[nodiscard]] Ima4aMaskPair ima4a_group_paired_masks(
    const mtf::mtf_token_batch_t &tokens, int64_t model_seed,
    int64_t group_begin, const mtf::mtf_jepa_mae_vicreg_config_t &config,
    const torch::Device &device) {
  std::array<mtf::JEPAContextTargetMasker, 2> maskers{
      mtf::JEPAContextTargetMasker(ima3_config(device, kIma4aPolicies[0])),
      mtf::JEPAContextTargetMasker(ima3_config(device, kIma4aPolicies[1]))};
  std::array<std::vector<torch::Tensor>, 2> contexts;
  std::array<std::vector<torch::Tensor>, 2> targets;
  Ima4aMaskPair result{};
  for (int64_t row = 0; row < tokens.token_mask.size(0); ++row) {
    const auto one = ima4a_token_row(tokens, row);
    std::array<GeneratorStateSnapshot, 2> post{};
    for (std::size_t arm = 0; arm < 2; ++arm) {
      set_paired_rng(ima4a_mask_seed(model_seed, group_begin + row), device);
      const auto selected = maskers[arm].create_masks(one);
      post[arm] = current_generator_state_snapshot(device);
      contexts[arm].push_back(selected.context_mask);
      targets[arm].push_back(selected.target_mask);
    }
    result.target_exact =
        result.target_exact && torch::equal(targets[0].back(), targets[1].back());
    result.rng_exact =
        result.rng_exact && generator_state_snapshot_equal(post[0], post[1]);
    for (std::size_t arm = 0; arm < 2; ++arm) {
      result.counts_exact =
          result.counts_exact &&
          targets[arm].back().sum().item<int64_t>() ==
              kIma4aTargetsPerGroup &&
          contexts[arm].back().sum().item<int64_t>() == kIma4aContextCount &&
          !targets[arm]
               .back()
               .logical_and(contexts[arm].back())
               .any()
               .item<bool>();
    }
  }
  for (std::size_t arm = 0; arm < 2; ++arm) {
    result.masks[arm].context_mask = torch::cat(contexts[arm], 0);
    result.masks[arm].target_mask = torch::cat(targets[arm], 0);
    result.masks[arm].valid_mask = tokens.token_mask;
    result.masks[arm].num_context_tokens = kIma4aContextCount;
    result.masks[arm].num_target_tokens = kIma4aTargetsPerGroup;
  }
  if (!result.target_exact || !result.counts_exact || !result.rng_exact ||
      result.masks[0].context_mask.sizes() != tokens.token_mask.sizes() ||
      result.masks[1].context_mask.sizes() != tokens.token_mask.sizes()) {
    throw std::runtime_error("IMA-4A group-paired mask contract failed");
  }
  return result;
}

[[nodiscard]] Ima4aCapture ima4a_finalize_capture(Ima4aCaptureBuilder &builder) {
  Ima4aCapture result{};
  for (std::size_t surface = 0; surface < kIma4aSurfaceCount; ++surface) {
    if (builder.features[surface].empty()) {
      throw std::runtime_error("IMA-4A empty feature capture");
    }
    result.features[surface] =
        torch::cat(builder.features[surface], 0).contiguous();
  }
  result.target = torch::cat(builder.target, 0).contiguous();
  result.current = torch::cat(builder.current, 0).contiguous();
  result.target0 = torch::cat(builder.target0, 0).contiguous();
  result.support0 = torch::cat(builder.support0, 0).contiguous();
  result.self = torch::cat(builder.self, 0).contiguous();
  result.alias = torch::cat(builder.alias, 0).contiguous();
  result.hidden = torch::cat(builder.hidden, 0).contiguous();
  result.target_id = torch::cat(builder.target_id, 0).contiguous();
  result.domain = torch::cat(builder.domain, 0).contiguous();
  result.channel = torch::cat(builder.channel, 0).contiguous();
  result.scale = torch::cat(builder.scale, 0).contiguous();
  result.group_id = torch::cat(builder.group_id, 0).contiguous();
  result.decomposition_exact = builder.decomposition_exact &&
      torch::allclose(result.hidden, result.self + result.alias, 1.0e-7,
                      1.0e-6);
  result.finite = builder.finite;
  for (const auto &value : result.features) {
    result.finite = result.finite && torch::isfinite(value).all().item<bool>();
  }
  for (const auto &value : {result.target, result.current, result.target0,
                            result.support0, result.self, result.alias,
                            result.hidden}) {
    result.finite = result.finite && torch::isfinite(value).all().item<bool>();
  }
  if (!result.decomposition_exact || !result.finite ||
      result.target.size(0) != result.target_id.size(0) ||
      result.target.size(1) != kLatentDim) {
    throw std::runtime_error("IMA-4A finalized capture contract failed");
  }
  return result;
}

[[nodiscard]] Ima4aContextReceipt
ima4a_context_receipt(const RmcData &data, const torch::Device &device) {
  Ima4aContextReceipt result{};
  std::array<mtf::JEPAContextTargetMasker, 2> maskers{
      mtf::JEPAContextTargetMasker(ima3_config(device, kIma4aPolicies[0])),
      mtf::JEPAContextTargetMasker(ima3_config(device, kIma4aPolicies[1]))};
  for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
       ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    set_paired_rng(seed, device);
    auto model = mtf::MtfJepaMaeVicreg(ima3_config(device, kIma4aPolicies[0]));
    const auto rows = training_rows(data.ssl, seed, 0);
    const auto indices = torch::tensor(rows, torch::kInt64);
    const auto tokens = model->tokenize(
        data.ssl.data.index_select(0, indices).to(device),
        data.ssl.mask.index_select(0, indices).to(device));
    const auto channels =
        tokens.metadata.channel_id.to(torch::kCPU, torch::kInt64).contiguous();
    const auto domains =
        tokens.metadata.domain_id.to(torch::kCPU, torch::kInt64).contiguous();
    const auto scales =
        tokens.metadata.scale_id.to(torch::kCPU, torch::kInt64).contiguous();
    const auto starts =
        tokens.metadata.start_index.to(torch::kCPU, torch::kInt64).contiguous();
    const auto widths =
        tokens.metadata.width.to(torch::kCPU, torch::kInt64).contiguous();
    const auto channel = channels.accessor<int64_t, 1>();
    const auto domain = domains.accessor<int64_t, 1>();
    const auto scale = scales.accessor<int64_t, 1>();
    const auto start = starts.accessor<int64_t, 1>();
    const auto width = widths.accessor<int64_t, 1>();

    for (int64_t step = 0; step < kIma3Steps; ++step) {
      std::array<mtf::jepa_context_target_mask_t, 2> masks{};
      std::array<GeneratorStateSnapshot, 2> post{};
      for (std::size_t arm = 0; arm < 2; ++arm) {
        set_paired_rng(paired_step_seed(seed, step), device);
        masks[arm] = maskers[arm].create_masks(tokens);
        post[arm] = current_generator_state_snapshot(device);
      }
      ++result.updates;
      result.paired_targets +=
          torch::equal(masks[0].target_mask, masks[1].target_mask) ? 1 : 0;
      result.paired_counts +=
          ima3_mask_counts_exact(masks[0], tokens,
                                 ima3_config(device, kIma4aPolicies[0])
                                     .min_context_ratio) &&
                  ima3_mask_counts_exact(masks[1], tokens,
                                         ima3_config(device,
                                                     kIma4aPolicies[1])
                                             .min_context_ratio)
              ? 1
              : 0;
      result.paired_rng +=
          generator_state_snapshot_equal(post[0], post[1]) ? 1 : 0;

      std::array<torch::Tensor, 2> context_cpu{
          masks[0].context_mask.to(torch::kCPU, torch::kBool).contiguous(),
          masks[1].context_mask.to(torch::kCPU, torch::kBool).contiguous()};
      const auto target_cpu =
          masks[0].target_mask.to(torch::kCPU, torch::kBool).contiguous();
      const auto removed =
          context_cpu[0].logical_and(context_cpu[1].logical_not());
      const auto replacement =
          context_cpu[1].logical_and(context_cpu[0].logical_not());
      const auto target_acc = target_cpu.accessor<bool, 2>();
      const auto removed_acc = removed.accessor<bool, 2>();
      const auto replacement_acc = replacement.accessor<bool, 2>();
      std::array<torch::TensorAccessor<bool, 2>, 2> context_acc{
          context_cpu[0].accessor<bool, 2>(),
          context_cpu[1].accessor<bool, 2>()};
      for (int64_t batch = 0; batch < target_cpu.size(0); ++batch) {
        ++result.samples;
        std::array<std::array<bool, kHistory>, kChannels> target_support{};
        for (int64_t token = 0; token < target_cpu.size(1); ++token) {
          if (!target_acc[batch][token]) {
            continue;
          }
          for (int64_t time = start[token]; time < start[token] + width[token];
               ++time) {
            target_support[static_cast<std::size_t>(channel[token])]
                          [static_cast<std::size_t>(time)] = true;
          }
        }
        std::array<std::array<std::array<bool, kHistory>, kChannels>, 2>
            coverage{};
        for (int64_t token = 0; token < target_cpu.size(1); ++token) {
          const auto c = static_cast<std::size_t>(channel[token]);
          const auto d = static_cast<std::size_t>(domain[token]);
          const auto s = static_cast<std::size_t>(scale[token]);
          if (removed_acc[batch][token]) {
            ++result.removed[c][d][s];
          }
          if (replacement_acc[batch][token]) {
            ++result.replacement[c][d][s];
          }
          for (std::size_t arm = 0; arm < 2; ++arm) {
            if (!context_acc[arm][batch][token]) {
              continue;
            }
            ++result.context[arm][c][d][s];
            ++result.token_selection[arm][static_cast<std::size_t>(token)];
            for (int64_t time = start[token];
                 time < start[token] + width[token]; ++time) {
              coverage[arm][c][static_cast<std::size_t>(time)] = true;
            }
          }
        }
        for (std::size_t arm = 0; arm < 2; ++arm) {
          for (std::size_t c = 0; c < kChannels; ++c) {
            int64_t non_target = 0;
            int64_t overlap = 0;
            for (std::size_t time = 0; time < kHistory; ++time) {
              non_target +=
                  coverage[arm][c][time] && !target_support[c][time] ? 1 : 0;
              overlap +=
                  coverage[arm][c][time] && target_support[c][time] ? 1 : 0;
            }
            ++result.non_target_coverage_hist[arm][c]
                                                 [static_cast<std::size_t>(
                                                     non_target)];
            result.non_target_coverage_sum[arm][c] += non_target;
            result.target_overlap_coverage_sum[arm][c] += overlap;
          }
        }
      }
    }
  }
  result.pass = result.updates == kIma3Steps * 3 &&
                result.samples == kIma3Steps * 3 * kModelRowBatchSize &&
                result.paired_targets == result.updates &&
                result.paired_counts == result.updates &&
                result.paired_rng == result.updates;
  for (std::size_t channel = 0; channel < kChannels; ++channel) {
    result.pass = result.pass &&
                  result.target_overlap_coverage_sum[1][channel] == 0;
  }
  return result;
}

[[nodiscard]] torch::Tensor ima4a_support_closure(
    const mtf::mtf_token_metadata_t &metadata,
    const torch::Tensor &target_indices, const torch::Device &device) {
  const auto target_cpu =
      target_indices.to(torch::kCPU, torch::kInt64).contiguous();
  const auto channels =
      metadata.channel_id.to(torch::kCPU, torch::kInt64).contiguous();
  const auto starts =
      metadata.start_index.to(torch::kCPU, torch::kInt64).contiguous();
  const auto widths = metadata.width.to(torch::kCPU, torch::kInt64).contiguous();
  const auto target = target_cpu.accessor<int64_t, 1>();
  const auto channel = channels.accessor<int64_t, 1>();
  const auto start = starts.accessor<int64_t, 1>();
  const auto width = widths.accessor<int64_t, 1>();
  auto closure = torch::zeros(
      {target_cpu.size(0), channels.size(0)},
      torch::TensorOptions().dtype(torch::kBool).device(torch::kCPU));
  auto out = closure.accessor<bool, 2>();
  for (int64_t row = 0; row < target_cpu.size(0); ++row) {
    const int64_t selected = target[row];
    for (int64_t token = 0; token < channels.size(0); ++token) {
      out[row][token] =
          channel[token] == channel[selected] &&
          std::max(start[token], start[selected]) <
              std::min(start[token] + width[token],
                       start[selected] + width[selected]);
    }
  }
  return closure.to(device, torch::kBool);
}

[[nodiscard]] torch::Tensor ima4a_structured_context_surface(
    const torch::Tensor &context_latents, const torch::Tensor &context_mask,
    const mtf::mtf_token_metadata_t &metadata) {
  std::vector<torch::Tensor> pools;
  std::vector<torch::Tensor> occupancies;
  pools.reserve(kIma4aCellCount);
  occupancies.reserve(kIma4aCellCount);
  const auto channel = metadata.channel_id.to(context_mask.device());
  const auto domain = metadata.domain_id.to(context_mask.device());
  const auto scale = metadata.scale_id.to(context_mask.device());
  for (int64_t c = 0; c < kChannels; ++c) {
    for (int64_t d = 0; d < 2; ++d) {
      for (int64_t s = 0; s < 4; ++s) {
        const auto membership = channel.eq(c)
                                    .logical_and(domain.eq(d))
                                    .logical_and(scale.eq(s));
        const auto mask =
            context_mask.logical_and(membership.unsqueeze(0));
        pools.push_back(mtf::detail::masked_mean(context_latents, mask));
        const double available =
            std::max<int64_t>(1, membership.sum().item<int64_t>());
        occupancies.push_back(mask.to(context_latents.dtype()).sum(1, true) /
                              available);
      }
    }
  }
  return torch::cat(
             {torch::cat(pools, 1), torch::cat(occupancies, 1)}, 1)
      .contiguous();
}

[[nodiscard]] torch::Tensor
ima4a_bilinear_surface(const torch::Tensor &query,
                       const torch::Tensor &field) {
  if (query.dim() != 2 || field.dim() != 2 ||
      query.size(0) != field.size(0) ||
      query.size(1) != kIma4aMetadataWidth) {
    throw std::runtime_error("IMA-4A bilinear surface shape failed");
  }
  const auto interaction =
      (query.unsqueeze(2) * field.unsqueeze(1)).flatten(1);
  return torch::cat({query, field, interaction}, 1).contiguous();
}

void ima4a_append_capture(
    Ima4aCaptureBuilder &builder, const torch::Tensor &context_latents,
    const torch::Tensor &online_tokens, const torch::Tensor &context_mask,
    const torch::Tensor &prediction, const torch::Tensor &target_selected,
    const torch::Tensor &target0_selected,
    const torch::Tensor &support0_selected,
    const torch::Tensor &target_rows, const torch::Tensor &target_indices,
    const torch::Tensor &target_metadata,
    const mtf::mtf_token_metadata_t &metadata, int64_t group_begin) {
  const auto global = mtf::detail::masked_mean(context_latents, context_mask);
  const auto structured =
      ima4a_structured_context_surface(context_latents, context_mask, metadata);
  const auto context_mask_float = context_mask.to(context_latents.dtype());
  const auto full = torch::cat(
      {context_latents
           .masked_fill(context_mask.logical_not().unsqueeze(-1), 0.0)
           .reshape({context_latents.size(0), -1}),
       context_mask_float},
      1);
  const auto raw = torch::cat(
      {online_tokens
           .masked_fill(context_mask.logical_not().unsqueeze(-1), 0.0)
           .reshape({online_tokens.size(0), -1}),
       context_mask_float},
      1);
  const std::array<torch::Tensor, kIma4aSurfaceCount> row_surfaces{
      torch::zeros({context_latents.size(0), 0}, context_latents.options()),
      global, structured, full, raw,
      torch::zeros({context_latents.size(0), 0}, context_latents.options()),
      torch::zeros({context_latents.size(0), 0}, context_latents.options())};
  for (std::size_t surface = 0; surface < kIma4aSurfaceCount; ++surface) {
    if (surface == 5) {
      const auto selected_field = full.index_select(0, target_rows);
      builder.features[surface].push_back(
          ima4a_bilinear_surface(target_metadata, selected_field)
              .detach()
              .to(torch::kCPU, torch::kFloat64)
              .contiguous());
    } else if (surface == 6) {
      // C is represented compactly as [q,F] here.  Its one-hot target-slot x F
      // interaction is applied exactly in the dual kernel at fit time.
      const auto selected_field = full.index_select(0, target_rows);
      builder.features[surface].push_back(
          torch::cat({target_metadata, selected_field}, 1)
              .detach()
              .to(torch::kCPU, torch::kFloat64)
              .contiguous());
    } else {
      auto selected = row_surfaces[surface].index_select(0, target_rows);
      builder.features[surface].push_back(
          torch::cat({target_metadata, selected}, 1)
              .detach()
              .to(torch::kCPU, torch::kFloat64)
              .contiguous());
    }
  }
  const auto selected_prediction =
      prediction.index({target_rows, target_indices, Slice()});
  const auto self = target_selected - target0_selected;
  const auto alias = target0_selected - support0_selected;
  const auto hidden = target_selected - support0_selected;
  builder.target.push_back(
      target_selected.detach().to(torch::kCPU, torch::kFloat64));
  builder.current.push_back(
      selected_prediction.detach().to(torch::kCPU, torch::kFloat64));
  builder.target0.push_back(
      target0_selected.detach().to(torch::kCPU, torch::kFloat64));
  builder.support0.push_back(
      support0_selected.detach().to(torch::kCPU, torch::kFloat64));
  builder.self.push_back(self.detach().to(torch::kCPU, torch::kFloat64));
  builder.alias.push_back(alias.detach().to(torch::kCPU, torch::kFloat64));
  builder.hidden.push_back(hidden.detach().to(torch::kCPU, torch::kFloat64));
  builder.decomposition_exact =
      builder.decomposition_exact &&
      torch::allclose(hidden, self + alias, 1.0e-7, 1.0e-6);
  builder.finite =
      builder.finite && torch::isfinite(selected_prediction).all().item<bool>() &&
      torch::isfinite(target_selected).all().item<bool>() &&
      torch::isfinite(hidden).all().item<bool>();
  const auto ids_cpu =
      target_indices.detach().to(torch::kCPU, torch::kInt64).contiguous();
  builder.target_id.push_back(ids_cpu);
  builder.domain.push_back(metadata.domain_id.to(target_indices.device())
                               .index_select(0, target_indices)
                               .to(torch::kCPU, torch::kInt64));
  builder.channel.push_back(metadata.channel_id.to(target_indices.device())
                                .index_select(0, target_indices)
                                .to(torch::kCPU, torch::kInt64));
  builder.scale.push_back(metadata.scale_id.to(target_indices.device())
                              .index_select(0, target_indices)
                              .to(torch::kCPU, torch::kInt64));
  const auto absolute_groups =
      torch::arange(group_begin, group_begin + context_latents.size(0),
                    torch::TensorOptions()
                        .dtype(torch::kInt64)
                        .device(target_rows.device()));
  builder.group_id.push_back(absolute_groups.index_select(0, target_rows)
                                 .to(torch::kCPU, torch::kInt64));
}

[[nodiscard]] std::array<Ima4aCapture, 2> ima4a_capture_split(
    const Ima4aModules &modules, const Dataset &dataset,
    const mtf::mtf_jepa_mae_vicreg_config_t &config,
    const torch::Device &device, int64_t model_seed, bool &pairing_exact) {
  std::array<Ima4aCaptureBuilder, 2> builders{};
  torch::NoGradGuard no_grad;
  for (int64_t begin = 0; begin < dataset.data.size(0);
       begin += kModelRowBatchSize) {
    const int64_t size = std::min<int64_t>(
        kModelRowBatchSize, dataset.data.size(0) - begin);
    const auto data = dataset.data.narrow(0, begin, size).to(device);
    const auto feature_mask = dataset.mask.narrow(0, begin, size).to(device);
    const auto online = modules.tokenizer->forward(data, feature_mask);
    const auto teacher = modules.target_tokenizer->forward(data, feature_mask);
    if (!torch::equal(online.token_mask, teacher.token_mask) ||
        !ima4a_metadata_equal(online.metadata, teacher.metadata) ||
        online.tokens.sizes() != teacher.tokens.sizes() ||
        online.tokens.size(1) != kIma4aTokenCount) {
      throw std::runtime_error("IMA-4A online/teacher token layout mismatch");
    }
    const auto pair = ima4a_group_paired_masks(
        online, model_seed, dataset.group_begin + begin, config, device);
    pairing_exact = pairing_exact && pair.target_exact && pair.counts_exact &&
                    pair.rng_exact;
    const auto target_locations = pair.masks[0].target_mask.nonzero();
    if (target_locations.sizes() !=
        torch::IntArrayRef({size * kIma4aTargetsPerGroup, 2})) {
      throw std::runtime_error("IMA-4A selected-target shape mismatch");
    }
    const auto target_rows = target_locations.select(1, 0).contiguous();
    const auto target_indices = target_locations.select(1, 1).contiguous();
    const auto all_target_metadata =
        mtf::detail::metadata_features(online.metadata, config);
    const auto target_metadata =
        all_target_metadata.index_select(0, target_indices);

    const auto full_target =
        modules.target_encoder->forward(teacher.tokens, teacher.token_mask);
    const auto target_selected =
        full_target.index({target_rows, target_indices, Slice()});
    const auto repeated_tokens = teacher.tokens.index_select(0, target_rows);
    const auto repeated_mask = teacher.token_mask.index_select(0, target_rows);
    const auto target_sample_rows = torch::arange(
        target_indices.size(0),
        torch::TensorOptions().dtype(torch::kInt64).device(device));
    auto target0_tokens = repeated_tokens.clone();
    target0_tokens.index_put_({target_sample_rows, target_indices, Slice()},
                              0.0);
    const auto closure =
        ima4a_support_closure(teacher.metadata, target_indices, device);
    if (!closure
             .index({target_sample_rows, target_indices})
             .all()
             .item<bool>()) {
      throw std::runtime_error("IMA-4A support closure omits target");
    }
    const auto support0_tokens =
        repeated_tokens.masked_fill(closure.unsqueeze(-1), 0.0);
    const auto target0_field =
        modules.target_encoder->forward(target0_tokens, repeated_mask);
    const auto support0_field =
        modules.target_encoder->forward(support0_tokens, repeated_mask);
    const auto target0_selected = target0_field.index(
        {target_sample_rows, target_indices, Slice()});
    const auto support0_selected = support0_field.index(
        {target_sample_rows, target_indices, Slice()});

    for (std::size_t arm = 0; arm < 2; ++arm) {
      const auto context_mask = pair.masks[arm].context_mask;
      const auto context_tokens = online.tokens.masked_fill(
          context_mask.logical_not().unsqueeze(-1), 0.0);
      const auto context_latents =
          modules.encoder->forward(context_tokens, context_mask);
      const auto prediction = modules.predictor->forward(
          context_latents, context_mask, online.metadata);
      ima4a_append_capture(
          builders[arm], context_latents, online.tokens, context_mask,
          prediction, target_selected, target0_selected, support0_selected,
          target_rows, target_indices, target_metadata, online.metadata,
          dataset.group_begin + begin);
    }
  }
  std::array<Ima4aCapture, 2> result{
      ima4a_finalize_capture(builders[0]),
      ima4a_finalize_capture(builders[1])};
  pairing_exact = pairing_exact &&
                  torch::equal(result[0].target, result[1].target) &&
                  torch::equal(result[0].target_id, result[1].target_id) &&
                  torch::equal(result[0].group_id, result[1].group_id) &&
                  result[0].target.size(0) ==
                      dataset.data.size(0) * kIma4aTargetsPerGroup;
  return result;
}

[[nodiscard]] Ima4aFrozenProbe ima4a_fit_frozen_probe(
    mtf::MtfJepaMaeVicreg &model, const RmcData &data,
    const torch::Device &device, std::array<int64_t, 2> tasks) {
  const auto train = rmc_extract_sparse_embeddings(model, data.probe_train,
                                                    device)
                         .flat;
  const auto validation =
      rmc_extract_sparse_embeddings(model, data.probe_validation, device).flat;
  const auto task_index = torch::tensor(
      std::vector<int64_t>{tasks[0], tasks[1]}, torch::kInt64);
  const auto train_target = data.probe_train.target.index_select(1, task_index);
  const auto validation_target =
      data.probe_validation.target.index_select(1, task_index);
  Ima4aFrozenProbe result{};
  double best = std::numeric_limits<double>::infinity();
  for (const auto alpha : kRidgeGrid) {
    const auto candidate = fit_ridge(train, train_target, alpha);
    const double mse =
        (predict(candidate, validation) - validation_target)
            .pow(2)
            .mean()
            .item<double>();
    if (mse < best) {
      best = mse;
      result.model = candidate;
      result.alpha = alpha;
    }
  }
  result.task_indices = tasks;
  result.finite = std::isfinite(best) &&
                  torch::isfinite(result.model.weights).all().item<bool>();
  return result;
}

[[nodiscard]] torch::Tensor ima4a_structured_full_features(
    const Ima4aModules &modules, const torch::Tensor &data,
    const torch::Tensor &feature_mask,
    const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  const auto tokens = modules.tokenizer->forward(data, feature_mask);
  const auto embeddings =
      modules.encoder->forward(tokens.tokens, tokens.token_mask);
  mtf::mtf_jepa_mae_vicreg_encode_output_t encoded{};
  encoded.embeddings = embeddings;
  encoded.pooled_embedding = mtf::detail::masked_mean(embeddings,
                                                       tokens.token_mask);
  encoded.pooled_by_channel = mtf::detail::pooled_by_channel(
      embeddings, tokens.token_mask, tokens.metadata, config);
  encoded.pooled_time = mtf::detail::masked_mean(
      embeddings, mtf::detail::domain_mask(tokens.metadata, tokens.token_mask,
                                           0));
  encoded.pooled_frequency = mtf::detail::masked_mean(
      embeddings, mtf::detail::domain_mask(tokens.metadata, tokens.token_mask,
                                           1));
  encoded.token_mask = tokens.token_mask;
  encoded.sample_valid_mask = tokens.token_mask.any(1);
  encoded.channel_valid_mask = mtf::detail::channel_valid_mask(
      tokens.metadata, tokens.token_mask, config);
  encoded.metadata = tokens.metadata;
  const auto served = mtf::select_mtf_serving_pool(
      encoded, mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1,
      config);
  if (!served.valid_mask.all().item<bool>() ||
      served.values.sizes() !=
          torch::IntArrayRef({data.size(0), kChannels, kLatentDim})) {
    throw std::runtime_error("IMA-4A differentiable structured surface failed");
  }
  return served.values.reshape({data.size(0), kServedWidth});
}

[[nodiscard]] torch::Tensor ima4a_frozen_probe_loss(
    const Ima4aModules &modules, const torch::Tensor &data,
    const torch::Tensor &feature_mask, const torch::Tensor &semantic_target,
    const Ima4aFrozenProbe &probe,
    const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  const auto features =
      ima4a_structured_full_features(modules, data, feature_mask, config);
  const auto options = features.options();
  const auto prediction =
      ((features - probe.model.mean.to(options)) *
       probe.model.inv_std.to(options))
          .matmul(probe.model.weights.to(options)) +
      probe.model.bias.to(options);
  const auto tasks = torch::tensor(
      std::vector<int64_t>{probe.task_indices[0], probe.task_indices[1]},
      torch::TensorOptions().dtype(torch::kInt64).device(data.device()));
  const auto target = semantic_target.index_select(1, tasks).to(options);
  return torch::mse_loss(prediction, target);
}

struct Ima4aGradientTargets {
  std::array<mtf::jepa_context_target_mask_t, 2> masks{};
  torch::Tensor full{};
  torch::Tensor support0{};
  torch::Tensor target_rows{};
  torch::Tensor target_indices{};
  torch::Tensor domain{};
  bool paired{false};
};

[[nodiscard]] Ima4aGradientTargets ima4a_gradient_targets(
    const Ima4aModules &modules, const torch::Tensor &data,
    const torch::Tensor &feature_mask,
    const mtf::mtf_jepa_mae_vicreg_config_t &config,
    const torch::Device &device, int64_t seed, int64_t group_begin) {
  torch::NoGradGuard no_grad;
  const auto online = modules.tokenizer->forward(data, feature_mask);
  const auto teacher = modules.target_tokenizer->forward(data, feature_mask);
  const auto pair = ima4a_group_paired_masks(online, seed, group_begin, config,
                                              device);
  const auto locations = pair.masks[0].target_mask.nonzero();
  const auto target_rows = locations.select(1, 0).contiguous();
  const auto target_indices = locations.select(1, 1).contiguous();
  const auto sample_rows = torch::arange(
      target_indices.size(0),
      torch::TensorOptions().dtype(torch::kInt64).device(device));
  const auto full_field =
      modules.target_encoder->forward(teacher.tokens, teacher.token_mask);
  const auto full = full_field.index({target_rows, target_indices, Slice()});
  const auto repeated_tokens = teacher.tokens.index_select(0, target_rows);
  const auto repeated_mask = teacher.token_mask.index_select(0, target_rows);
  const auto closure =
      ima4a_support_closure(teacher.metadata, target_indices, device);
  const auto support_field = modules.target_encoder->forward(
      repeated_tokens.masked_fill(closure.unsqueeze(-1), 0.0), repeated_mask);
  const auto support0 =
      support_field.index({sample_rows, target_indices, Slice()});
  const auto domain = teacher.metadata.domain_id.to(device)
                          .index_select(0, target_indices)
                          .contiguous();
  return {.masks = pair.masks,
          .full = full.detach(),
          .support0 = support0.detach(),
          .target_rows = target_rows,
          .target_indices = target_indices,
          .domain = domain,
          .paired = pair.target_exact && pair.counts_exact && pair.rng_exact};
}

enum class Ima4aJepaGradientKind { full, support0, time, frequency };

struct Ima4aGradientVectorPair {
  torch::Tensor served{};
  torch::Tensor predictor{};
  double loss{0.0};
};

[[nodiscard]] Ima4aGradientVectorPair ima4a_jepa_gradient(
    mtf::MtfJepaMaeVicreg &model, const Ima4aModules &modules,
    const torch::Tensor &data, const torch::Tensor &feature_mask,
    const mtf::jepa_context_target_mask_t &masks,
    const Ima4aGradientTargets &targets, Ima4aJepaGradientKind kind) {
  ima4a_clear_gradients(model);
  const auto online = modules.tokenizer->forward(data, feature_mask);
  const auto context_tokens = online.tokens.masked_fill(
      masks.context_mask.logical_not().unsqueeze(-1), 0.0);
  const auto context_latents =
      modules.encoder->forward(context_tokens, masks.context_mask);
  const auto prediction = modules.predictor->forward(
      context_latents, masks.context_mask, online.metadata);
  auto selected =
      prediction.index({targets.target_rows, targets.target_indices, Slice()});
  auto target = kind == Ima4aJepaGradientKind::support0 ? targets.support0
                                                        : targets.full;
  if (kind == Ima4aJepaGradientKind::time ||
      kind == Ima4aJepaGradientKind::frequency) {
    const int64_t requested =
        kind == Ima4aJepaGradientKind::time ? int64_t{0} : int64_t{1};
    const auto rows = targets.domain.eq(requested).nonzero().reshape({-1});
    selected = selected.index_select(0, rows);
    target = target.index_select(0, rows);
  }
  const auto loss = torch::mse_loss(selected, target);
  loss.backward();
  Ima4aGradientVectorPair result{
      .served = ima4a_parameter_gradient(model, ima4a_served_name),
      .predictor = ima4a_parameter_gradient(model, ima4a_predictor_name),
      .loss = loss.detach().item<double>()};
  ima4a_clear_gradients(model);
  return result;
}

[[nodiscard]] torch::Tensor ima4a_protected_gradient(
    mtf::MtfJepaMaeVicreg &model, const Ima4aModules &modules,
    const torch::Tensor &data, const torch::Tensor &feature_mask,
    const torch::Tensor &semantic_target, const Ima4aFrozenProbe &probe,
    const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  ima4a_clear_gradients(model);
  const auto loss = ima4a_frozen_probe_loss(modules, data, feature_mask,
                                             semantic_target, probe, config);
  loss.backward();
  auto result = ima4a_parameter_gradient(model, ima4a_served_name);
  ima4a_clear_gradients(model);
  return result;
}

[[nodiscard]] std::array<Ima4aGradientMetric, 2> ima4a_gradient_panel(
    mtf::MtfJepaMaeVicreg &model, const RmcData &data,
    const Ima4aFrozenProbe &order_probe,
    const Ima4aFrozenProbe &cross_probe, const torch::Device &device,
    int64_t seed, torch::Tensor &order_gradient_out,
    torch::Tensor &cross_gradient_out) {
  const auto modules = ima4a_modules(model);
  const auto batch_data = data.ssl.data.narrow(0, 0, kModelRowBatchSize).to(device);
  const auto batch_mask = data.ssl.mask.narrow(0, 0, kModelRowBatchSize).to(device);
  const auto batch_target =
      data.ssl.target.narrow(0, 0, kModelRowBatchSize).to(device);
  const auto targets = ima4a_gradient_targets(
      modules, batch_data, batch_mask, model->config(), device, seed,
      data.ssl.group_begin);
  if (!targets.paired) {
    throw std::runtime_error("IMA-4A gradient target pairing failed");
  }
  const auto order_gradient = ima4a_protected_gradient(
      model, modules, batch_data, batch_mask, batch_target, order_probe,
      model->config());
  const auto cross_gradient = ima4a_protected_gradient(
      model, modules, batch_data, batch_mask, batch_target, cross_probe,
      model->config());
  order_gradient_out = order_gradient;
  cross_gradient_out = cross_gradient;
  std::array<Ima4aGradientMetric, 2> result{};
  for (std::size_t arm = 0; arm < 2; ++arm) {
    const auto full = ima4a_jepa_gradient(
        model, modules, batch_data, batch_mask, targets.masks[arm], targets,
        Ima4aJepaGradientKind::full);
    const auto support0 = ima4a_jepa_gradient(
        model, modules, batch_data, batch_mask, targets.masks[arm], targets,
        Ima4aJepaGradientKind::support0);
    const auto time = ima4a_jepa_gradient(
        model, modules, batch_data, batch_mask, targets.masks[arm], targets,
        Ima4aJepaGradientKind::time);
    const auto frequency = ima4a_jepa_gradient(
        model, modules, batch_data, batch_mask, targets.masks[arm], targets,
        Ima4aJepaGradientKind::frequency);
    auto hidden = full.served - support0.served;
    const double full_norm = full.served.norm().item<double>();
    auto &metric = result[arm];
    metric.full = full.served;
    metric.support0 = support0.served;
    metric.hidden = hidden;
    metric.time = time.served;
    metric.frequency = frequency.served;
    metric.predictor_full = full.predictor;
    metric.hidden_fraction =
        full_norm > 1.0e-30 ? hidden.norm().item<double>() / full_norm : 0.0;
    metric.time_frequency_cosine =
        ima4a_cosine(time.served, frequency.served);
    metric.full_order_cosine = ima4a_cosine(full.served, order_gradient);
    metric.full_cross_cosine = ima4a_cosine(full.served, cross_gradient);
    metric.hidden_order_cosine = ima4a_cosine(hidden, order_gradient);
    metric.hidden_cross_cosine = ima4a_cosine(hidden, cross_gradient);
    metric.full_order_first_order =
        ima4a_first_order_loss_change(full.served, order_gradient);
    metric.full_cross_first_order =
        ima4a_first_order_loss_change(full.served, cross_gradient);
    metric.hidden_order_first_order =
        ima4a_first_order_loss_change(hidden, order_gradient);
    metric.hidden_cross_first_order =
        ima4a_first_order_loss_change(hidden, cross_gradient);
    metric.served_norm = full_norm;
    metric.predictor_norm = full.predictor.norm().item<double>();
    metric.finite = torch::isfinite(full.served).all().item<bool>() &&
                    torch::isfinite(full.predictor).all().item<bool>() &&
                    full_norm > 0.0 && metric.predictor_norm > 0.0 &&
                    std::isfinite(metric.hidden_fraction) &&
                    std::isfinite(metric.time_frequency_cosine);
  }
  ima4a_clear_gradients(model);
  return result;
}

[[nodiscard]] Ima4aSeedEvidence ima4a_run_seed(
    const RmcData &data, const torch::Device &device, std::size_t seed_index) {
  Ima4aSeedEvidence result{};
  result.seed = kAttributionSeeds.at(seed_index);
  DefaultGeneratorStateGuard rng_guard(device);
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_config_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  set_paired_rng(result.seed, device);
  auto model = mtf::MtfJepaMaeVicreg(
      ima3_config(device, kIma4aPolicies[0]));
  const bool anchor_loaded = oca_load_archive(
      oca_archive_path(result.seed), model, device, result.seed,
      anchor_config_hash);
  const bool cache_valid = ima4a_validate_ima3_seed_cache(
      data.ssl, device, result.seed, seed_index);
  result.custody = anchor_loaded && cache_valid &&
                   ima3_pins_exact(seed_index) &&
                   ima3_preflight_receipt_exact();
  if (!result.custody) {
    throw std::runtime_error("IMA-4A frozen custody failed");
  }
  model->eval();
  const auto state_before = oca_snapshot_state(model);
  result.component_equivalence =
      ima4a_component_equivalence(model, data.ssl, device, result.seed);
  if (!result.component_equivalence) {
    throw std::runtime_error("IMA-4A component equivalence failed");
  }
  const auto modules = ima4a_modules(model);
  bool pairing = true;
  const auto train = ima4a_capture_split(
      modules, data.probe_train, model->config(), device, result.seed, pairing);
  const auto validation = ima4a_capture_split(
      modules, data.probe_validation, model->config(), device, result.seed,
      pairing);
  const auto test = ima4a_capture_split(
      modules, data.development, model->config(), device, result.seed, pairing);
  result.capture_pairing = pairing;
  for (std::size_t arm = 0; arm < 2; ++arm) {
    result.current[arm] = ima4a_oracle_metric(
        test[arm].current, test[arm].target, test[arm].target_id);
    result.current_prediction[arm] = test[arm].current;
    for (std::size_t surface = 0; surface < kIma4aSurfaceCount; ++surface) {
      result.oracle[arm][surface] = ima4a_fit_oracle(
          train[arm].features[surface], train[arm].target,
          train[arm].target_id,
          validation[arm].features[surface], validation[arm].target,
          validation[arm].target_id, test[arm].features[surface],
          test[arm].target, test[arm].target_id,
          surface == 6 ? Ima4aOracleMap::categorical_slot_by_field
                       : Ima4aOracleMap::standard);
      result.oracle_prediction[arm][surface] =
          result.oracle[arm][surface].prediction;
    }
    result.decomposition[arm][0] =
        ima4a_decomposition_metric(test[arm], -1);
    result.decomposition[arm][1] =
        ima4a_decomposition_metric(test[arm], 0);
    result.decomposition[arm][2] =
        ima4a_decomposition_metric(test[arm], 1);
  }
  const auto order_probe =
      ima4a_fit_frozen_probe(model, data, device, {5, 6});
  const auto cross_probe =
      ima4a_fit_frozen_probe(model, data, device, {7, 8});
  result.gradient = ima4a_gradient_panel(
      model, data, order_probe, cross_probe, device, result.seed,
      result.order_gradient, result.cross_gradient);
  result.target = test[0].target;
  result.target_id = test[0].target_id;
  bool oracle_finite = true;
  bool alpha_gate = true;
  for (const auto &arm : result.oracle) {
    for (const auto &surface : arm) {
      oracle_finite = oracle_finite && std::isfinite(surface.metric.nmse) &&
                      std::isfinite(surface.metric.r2) &&
                      torch::isfinite(surface.prediction).all().item<bool>();
      alpha_gate = alpha_gate && !surface.edge_improving;
    }
  }
  const auto parameters_after = model->named_parameters(/*recurse=*/true);
  const bool gradients_cleared = std::all_of(
      parameters_after.begin(), parameters_after.end(),
      [](const auto &item) { return !item.value().grad().defined(); });
  const bool state_exact = oca_state_exact(model, state_before);
  result.pass = result.custody && result.component_equivalence &&
                result.capture_pairing && oracle_finite && alpha_gate &&
                order_probe.finite && cross_probe.finite &&
                result.gradient[0].finite && result.gradient[1].finite &&
                gradients_cleared && state_exact;
  rng_guard.restore();
  return result;
}

[[nodiscard]] torch::Tensor ima4a_group_resample(
    const torch::Tensor &value, const torch::Tensor &group_rows) {
  if (value.size(0) % kIma4aTargetsPerGroup != 0) {
    throw std::runtime_error("IMA-4A group resample shape failed");
  }
  auto shape = value.sizes().vec();
  const int64_t groups = value.size(0) / kIma4aTargetsPerGroup;
  shape[0] = groups;
  shape.insert(shape.begin() + 1, kIma4aTargetsPerGroup);
  auto selected = value.reshape(shape).index_select(0, group_rows);
  auto output_shape = value.sizes().vec();
  output_shape[0] = group_rows.size(0) * kIma4aTargetsPerGroup;
  return selected.reshape(output_shape).contiguous();
}

template <typename Candidate, typename Reference>
[[nodiscard]] Interval ima4a_bootstrap_contrast(
    const std::array<Ima4aSeedEvidence, 3> &evidence,
    Candidate candidate, Reference reference) {
  const int64_t groups = evidence[0].target.size(0) /
                         kIma4aTargetsPerGroup;
  const auto rows = rmc_bootstrap_rows(groups);
  std::vector<double> contrasts;
  contrasts.reserve(rows.size());
  for (const auto &group_rows : rows) {
    double value = 0.0;
    for (const auto &seed : evidence) {
      const auto target = ima4a_group_resample(seed.target, group_rows);
      const auto identity = ima4a_group_resample(seed.target_id, group_rows);
      const auto candidate_prediction =
          ima4a_group_resample(candidate(seed), group_rows);
      const auto reference_prediction =
          ima4a_group_resample(reference(seed), group_rows);
      value += ima4a_oracle_metric(candidate_prediction, target, identity).r2 -
               ima4a_oracle_metric(reference_prediction, target, identity).r2;
    }
    contrasts.push_back(value / static_cast<double>(evidence.size()));
  }
  return percentile_interval(std::move(contrasts));
}

[[nodiscard]] double ima4a_mean_oracle_r2(
    const std::array<Ima4aSeedEvidence, 3> &evidence, std::size_t arm,
    std::size_t surface) {
  double result = 0.0;
  for (const auto &seed : evidence) {
    result += seed.oracle[arm][surface].metric.r2;
  }
  return result / static_cast<double>(evidence.size());
}

[[nodiscard]] double ima4a_mean_current_r2(
    const std::array<Ima4aSeedEvidence, 3> &evidence, std::size_t arm) {
  double result = 0.0;
  for (const auto &seed : evidence) {
    result += seed.current[arm].r2;
  }
  return result / static_cast<double>(evidence.size());
}

[[nodiscard]] int64_t ima4a_positive_seed_count(
    const std::array<Ima4aSeedEvidence, 3> &evidence,
    const std::function<double(const Ima4aSeedEvidence &)> &contrast) {
  return std::count_if(evidence.begin(), evidence.end(), [&](const auto &seed) {
    return contrast(seed) > 0.0;
  });
}

void ima4a_emit_context_receipt(const Ima4aContextReceipt &receipt) {
  std::cout << "ima4a.context.updates=" << receipt.updates << '\n';
  std::cout << "ima4a.context.samples=" << receipt.samples << '\n';
  std::cout << "ima4a.context.paired_targets=" << receipt.paired_targets
            << '\n';
  std::cout << "ima4a.context.paired_counts=" << receipt.paired_counts << '\n';
  std::cout << "ima4a.context.paired_rng=" << receipt.paired_rng << '\n';
  for (std::size_t arm = 0; arm < 2; ++arm) {
    for (std::size_t channel = 0; channel < kChannels; ++channel) {
      const double denominator = static_cast<double>(receipt.samples);
      std::cout << "ima4a.context." << kIma4aPolicyNames[arm] << ".channel_"
                << channel << ".non_target_unique_coverage_mean="
                << receipt.non_target_coverage_sum[arm][channel] /
                       denominator
                << '\n';
      std::cout << "ima4a.context." << kIma4aPolicyNames[arm] << ".channel_"
                << channel << ".target_overlap_unique_coverage_mean="
                << receipt.target_overlap_coverage_sum[arm][channel] /
                       denominator
                << '\n';
      for (std::size_t coverage = 0; coverage <= kHistory; ++coverage) {
        const auto count =
            receipt.non_target_coverage_hist[arm][channel][coverage];
        if (count > 0) {
          std::cout << "ima4a.context." << kIma4aPolicyNames[arm]
                    << ".channel_" << channel
                    << ".non_target_coverage_hist_" << coverage << '='
                    << count << '\n';
        }
      }
      for (std::size_t domain = 0; domain < 2; ++domain) {
        for (std::size_t scale = 0; scale < 4; ++scale) {
          std::cout << "ima4a.context." << kIma4aPolicyNames[arm]
                    << ".c" << channel << ".d" << domain << ".s" << scale
                    << ".tokens="
                    << receipt.context[arm][channel][domain][scale] << '\n';
        }
      }
    }
    for (std::size_t token = 0; token < kIma4aTokenCount; ++token) {
      std::cout << "ima4a.context." << kIma4aPolicyNames[arm] << ".token_"
                << token << ".selected="
                << receipt.token_selection[arm][token] << '\n';
    }
  }
  for (std::size_t channel = 0; channel < kChannels; ++channel) {
    for (std::size_t domain = 0; domain < 2; ++domain) {
      for (std::size_t scale = 0; scale < 4; ++scale) {
        std::cout << "ima4a.context.delta.c" << channel << ".d" << domain
                  << ".s" << scale << ".removed="
                  << receipt.removed[channel][domain][scale] << '\n';
        std::cout << "ima4a.context.delta.c" << channel << ".d" << domain
                  << ".s" << scale << ".replacement="
                  << receipt.replacement[channel][domain][scale] << '\n';
      }
    }
  }
  std::cout << "ima4a.context.pass=" << receipt.pass << '\n';
}

void ima4a_emit_seed(const Ima4aSeedEvidence &seed) {
  const std::string root = "ima4a.seed_" + std::to_string(seed.seed);
  std::cout << root << ".custody=" << seed.custody << '\n';
  std::cout << root << ".component_equivalence="
            << seed.component_equivalence << '\n';
  std::cout << root << ".capture_pairing=" << seed.capture_pairing << '\n';
  for (std::size_t arm = 0; arm < 2; ++arm) {
    const std::string policy =
        root + "." + std::string(kIma4aPolicyNames[arm]);
    std::cout << policy << ".current.nmse=" << seed.current[arm].nmse << '\n';
    std::cout << policy << ".current.r2=" << seed.current[arm].r2 << '\n';
    for (std::size_t surface = 0; surface < kIma4aSurfaceCount; ++surface) {
      const auto &value = seed.oracle[arm][surface];
      const std::string item =
          policy + ".oracle_" + std::string(kIma4aSurfaceNames[surface]);
      std::cout << item << ".nmse=" << value.metric.nmse << '\n';
      std::cout << item << ".r2=" << value.metric.r2 << '\n';
      std::cout << item << ".alpha=" << value.alpha << '\n';
      std::cout << item << ".validation_nmse_intercept="
                << value.intercept_validation_nmse << '\n';
      std::cout << item << ".tail_matches_intercept="
                << value.tail_matches_intercept << '\n';
      std::cout << item << ".alpha_edge_improving="
                << value.edge_improving << '\n';
      for (std::size_t alpha = 0; alpha < kIma4aRidgeGrid.size(); ++alpha) {
        std::cout << item << ".validation_nmse_alpha_" << alpha << '='
                  << value.validation_nmse[alpha] << '\n';
      }
    }
    constexpr std::array<std::string_view, 3> scopes{"all", "time",
                                                     "frequency"};
    for (std::size_t scope = 0; scope < scopes.size(); ++scope) {
      const auto &value = seed.decomposition[arm][scope];
      const std::string item = policy + ".decomposition." +
                               std::string(scopes[scope]);
      std::cout << item << ".hidden_energy_fraction="
                << value.hidden_energy_fraction << '\n';
      std::cout << item << ".self_energy_fraction="
                << value.self_energy_fraction << '\n';
      std::cout << item << ".alias_energy_fraction="
                << value.alias_energy_fraction << '\n';
      std::cout << item << ".residual_hidden_projection_fraction="
                << value.residual_hidden_projection_fraction << '\n';
      std::cout << item << ".hidden_dominant_dimension_fraction="
                << value.hidden_dominant_dimension_fraction << '\n';
      std::cout << item << ".hidden_effective_rank_ratio="
                << value.hidden_geometry.effective_rank_ratio << '\n';
      std::cout << item << ".hidden_participation_rank_ratio="
                << value.hidden_geometry.participation_rank_ratio << '\n';
      std::cout << item << ".hidden_top_eigenvalue_share="
                << value.hidden_geometry.top_eigenvalue_share << '\n';
    }
    const auto &gradient = seed.gradient[arm];
    std::cout << policy << ".gradient.served_norm=" << gradient.served_norm
              << '\n';
    std::cout << policy << ".gradient.predictor_norm="
              << gradient.predictor_norm << '\n';
    std::cout << policy << ".gradient.hidden_fraction="
              << gradient.hidden_fraction << '\n';
    std::cout << policy << ".gradient.time_frequency_cosine="
              << gradient.time_frequency_cosine << '\n';
    std::cout << policy << ".gradient.full_order_cosine="
              << gradient.full_order_cosine << '\n';
    std::cout << policy << ".gradient.full_cross_cosine="
              << gradient.full_cross_cosine << '\n';
    std::cout << policy << ".gradient.hidden_order_cosine="
              << gradient.hidden_order_cosine << '\n';
    std::cout << policy << ".gradient.hidden_cross_cosine="
              << gradient.hidden_cross_cosine << '\n';
    std::cout << policy << ".gradient.full_order_first_order="
              << gradient.full_order_first_order << '\n';
    std::cout << policy << ".gradient.full_cross_first_order="
              << gradient.full_cross_first_order << '\n';
    std::cout << policy << ".gradient.hidden_order_first_order="
              << gradient.hidden_order_first_order << '\n';
    std::cout << policy << ".gradient.hidden_cross_first_order="
              << gradient.hidden_cross_first_order << '\n';
  }
  const auto extra = seed.gradient[1].full - seed.gradient[0].full;
  std::cout << root << ".gradient.m2_minus_m1.order_cosine="
            << ima4a_cosine(extra, seed.order_gradient) << '\n';
  std::cout << root << ".gradient.m2_minus_m1.cross_cosine="
            << ima4a_cosine(extra, seed.cross_gradient) << '\n';
  std::cout << root << ".gradient.m2_minus_m1.order_first_order="
            << ima4a_first_order_loss_change(extra, seed.order_gradient)
            << '\n';
  std::cout << root << ".gradient.m2_minus_m1.cross_first_order="
            << ima4a_first_order_loss_change(extra, seed.cross_gradient)
            << '\n';
  std::cout << root << ".pass=" << seed.pass << '\n';
}

[[nodiscard]] bool ima4a_self_test() {
  DefaultGeneratorStateGuard guard(torch::Device(torch::kCPU));
  torch::manual_seed(41041);
  const auto target = torch::randn({80, 4}, torch::kFloat64);
  const auto identity =
      torch::arange(80, torch::kInt64).remainder(4);
  const auto centered = ima4a_identity_centered(target, identity);
  const auto baseline = target - centered;
  const auto exact_metric = ima4a_oracle_metric(target, target, identity);
  const auto baseline_metric =
      ima4a_oracle_metric(baseline, target, identity);
  const auto train_x = torch::randn({96, 12}, torch::kFloat64);
  const auto validation_x = torch::randn({48, 12}, torch::kFloat64);
  const auto test_x = torch::randn({64, 12}, torch::kFloat64);
  const auto weights = torch::randn({12, 4}, torch::kFloat64);
  const auto train_y = train_x.matmul(weights);
  const auto validation_y = validation_x.matmul(weights);
  const auto test_y = test_x.matmul(weights);
  const auto train_id = torch::zeros({96}, torch::kInt64);
  const auto validation_id = torch::zeros({48}, torch::kInt64);
  const auto test_id = torch::zeros({64}, torch::kInt64);
  const auto oracle = ima4a_fit_oracle(
      train_x, train_y, train_id, validation_x, validation_y, validation_id,
      test_x, test_y, test_id);

  const auto make_bilinear_split = [](int64_t rows) {
    auto query = torch::randn({rows, kIma4aMetadataWidth}, torch::kFloat64);
    auto field = torch::zeros({rows, 40}, torch::kFloat64);
    field.select(1, 0).copy_(torch::randn({rows}, torch::kFloat64));
    auto target =
        (query.select(1, 0) * field.select(1, 0)).unsqueeze(1);
    return std::tuple{query, field, target};
  };
  const auto [bilinear_train_q, bilinear_train_f, bilinear_train_y] =
      make_bilinear_split(64);
  const auto [bilinear_validation_q, bilinear_validation_f,
              bilinear_validation_y] = make_bilinear_split(48);
  const auto [bilinear_test_q, bilinear_test_f, bilinear_test_y] =
      make_bilinear_split(64);
  const auto bilinear_train_id = torch::zeros({64}, torch::kInt64);
  const auto bilinear_validation_id = torch::zeros({48}, torch::kInt64);
  const auto bilinear_test_id = torch::zeros({64}, torch::kInt64);
  const auto bilinear_oracle = ima4a_fit_oracle(
      ima4a_bilinear_surface(bilinear_train_q, bilinear_train_f),
      bilinear_train_y, bilinear_train_id,
      ima4a_bilinear_surface(bilinear_validation_q, bilinear_validation_f),
      bilinear_validation_y, bilinear_validation_id,
      ima4a_bilinear_surface(bilinear_test_q, bilinear_test_f),
      bilinear_test_y, bilinear_test_id);
  const auto affine_oracle = ima4a_fit_oracle(
      torch::cat({bilinear_train_q, bilinear_train_f}, 1), bilinear_train_y,
      bilinear_train_id,
      torch::cat({bilinear_validation_q, bilinear_validation_f}, 1),
      bilinear_validation_y, bilinear_validation_id,
      torch::cat({bilinear_test_q, bilinear_test_f}, 1), bilinear_test_y,
      bilinear_test_id);

  const auto make_categorical_split = [](int64_t rows) {
    auto query =
        torch::zeros({rows, kIma4aMetadataWidth}, torch::kFloat64);
    auto field = torch::zeros({rows, 40}, torch::kFloat64);
    field.select(1, 0).copy_(torch::randn({rows}, torch::kFloat64));
    auto identity = torch::arange(rows, torch::kInt64).remainder(2);
    auto sign = torch::where(identity.eq(0), torch::ones({rows}, torch::kFloat64),
                             -torch::ones({rows}, torch::kFloat64));
    auto target = (sign * field.select(1, 0)).unsqueeze(1);
    return std::tuple{torch::cat({query, field}, 1), identity, target};
  };
  const auto [categorical_train_x, categorical_train_id,
              categorical_train_y] = make_categorical_split(96);
  const auto [categorical_validation_x, categorical_validation_id,
              categorical_validation_y] = make_categorical_split(48);
  const auto [categorical_test_x, categorical_test_id, categorical_test_y] =
      make_categorical_split(64);
  const auto categorical_oracle = ima4a_fit_oracle(
      categorical_train_x, categorical_train_y, categorical_train_id,
      categorical_validation_x, categorical_validation_y,
      categorical_validation_id, categorical_test_x, categorical_test_y,
      categorical_test_id, Ima4aOracleMap::categorical_slot_by_field);
  const auto categorical_affine = ima4a_fit_oracle(
      categorical_train_x, categorical_train_y, categorical_train_id,
      categorical_validation_x, categorical_validation_y,
      categorical_validation_id, categorical_test_x, categorical_test_y,
      categorical_test_id);

  std::array<double, kIma4aRidgeGrid.size()> lower_curve{};
  std::array<double, kIma4aRidgeGrid.size()> upper_curve{};
  std::array<double, kIma4aRidgeGrid.size()> limit_curve{};
  lower_curve.fill(1.0);
  upper_curve.fill(1.0);
  limit_curve.fill(1.0);
  lower_curve[0] = 0.5;
  lower_curve[1] = 0.7;
  upper_curve[upper_curve.size() - 2] = 0.7;
  upper_curve.back() = 0.5;
  limit_curve[limit_curve.size() - 2] = 0.7;
  limit_curve.back() = 0.5;
  const auto lower_edge = ima4a_edge_status(lower_curve, 0, 1.0);
  const auto upper_edge =
      ima4a_edge_status(upper_curve, upper_curve.size() - 1, 0.2);
  const auto intercept_limit =
      ima4a_edge_status(limit_curve, limit_curve.size() - 1, 0.5);
  const auto self = torch::randn({20, 8}, torch::kFloat64);
  const auto alias = torch::randn({20, 8}, torch::kFloat64);
  const auto hidden = self + alias;
  std::cout << "ima4a.self_test.linear_r2=" << oracle.metric.r2 << '\n';
  std::cout << "ima4a.self_test.bilinear_r2="
            << bilinear_oracle.metric.r2 << '\n';
  std::cout << "ima4a.self_test.bilinear_affine_r2="
            << affine_oracle.metric.r2 << '\n';
  std::cout << "ima4a.self_test.categorical_r2="
            << categorical_oracle.metric.r2 << '\n';
  std::cout << "ima4a.self_test.categorical_affine_r2="
            << categorical_affine.metric.r2 << '\n';
  std::cout << "ima4a.self_test.lower_edge=" << lower_edge.improving << '\n';
  std::cout << "ima4a.self_test.upper_edge=" << upper_edge.improving << '\n';
  std::cout << "ima4a.self_test.intercept_limit="
            << (intercept_limit.tail_matches_intercept &&
                !intercept_limit.improving)
            << '\n';
  const bool pass = std::abs(exact_metric.nmse) <= 1.0e-12 &&
                    std::abs(baseline_metric.nmse - 1.0) <= 1.0e-12 &&
                    oracle.metric.r2 >= 0.999 &&
                    bilinear_train_q.size(1) + bilinear_train_f.size(1) +
                            bilinear_train_q.size(1) *
                                bilinear_train_f.size(1) >
                        bilinear_train_q.size(0) &&
                    bilinear_oracle.metric.r2 >= 0.995 &&
                    affine_oracle.metric.r2 < 0.25 &&
                    categorical_oracle.metric.r2 >= 0.98 &&
                    categorical_affine.metric.r2 < 0.25 &&
                    lower_edge.improving && upper_edge.improving &&
                    intercept_limit.tail_matches_intercept &&
                    !intercept_limit.improving &&
                    torch::allclose(hidden, self + alias, 0.0, 0.0) &&
                    std::abs(ima4a_cosine(self, self) - 1.0) <= 1.0e-12;
  guard.restore();
  return pass;
}

[[nodiscard]] int run_ima4a_self_test() {
  std::cout << std::boolalpha << std::setprecision(12);
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.ima4a.self_test.v1\n";
  std::cout << "experiment=target-predictability-ceiling-self-test\n";
  const bool pass = ima4a_self_test();
  std::cout << "ima4a.self_test.pass=" << pass << '\n';
  std::cout << "ima4a.optimizer_steps=0\n";
  std::cout << "ima4a.ema_updates=0\n";
  std::cout << "execution_status=ima4a_self_test_complete\n";
  return pass ? 0 : 3;
}

[[nodiscard]] int run_ima4a_audit(const Options &options) {
  if (options.device != "cuda" || !torch::cuda::is_available()) {
    throw std::runtime_error("IMA-4A authoritative audit requires CUDA");
  }
  const char *workspace = std::getenv("CUBLAS_WORKSPACE_CONFIG");
  if (workspace == nullptr || std::string(workspace) != ":4096:8") {
    throw std::runtime_error(
        "IMA-4A requires CUBLAS_WORKSPACE_CONFIG=:4096:8");
  }
  const torch::Device device(torch::kCUDA, 0);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);
  DefaultGeneratorStateGuard audit_rng(device);
  auto data = rmc_make_data();
  const auto context = ima4a_context_receipt(data, device);
  std::array<Ima4aSeedEvidence, 3> evidence{};
  for (std::size_t seed = 0; seed < evidence.size(); ++seed) {
    evidence[seed] = ima4a_run_seed(data, device, seed);
  }

  std::cout << std::boolalpha << std::setprecision(12);
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.ima4a.audit.v1\n";
  std::cout << "experiment=target-predictability-ceiling-audit\n";
  std::cout << "device=cuda\n";
  std::cout << "ima4a.protocol_sha256=" << ima4a_sha256(kIma4aProtocolPath)
            << '\n';
  std::cout << "ima4a.source_sha256=" << ima4a_sha256(kIma4aSourcePath)
            << '\n';
  std::cout << "ima4a.ima3_source_sha256=" << ima3_sha256(kIma3SourcePath)
            << '\n';
  std::cout << "ima4a.header_sha256=" << ima3_sha256(kIma3HeaderPath) << '\n';
  std::cout << "ima4a.fit_groups=" << data.probe_train.data.size(0) << '\n';
  std::cout << "ima4a.validation_groups="
            << data.probe_validation.data.size(0) << '\n';
  std::cout << "ima4a.heldout_groups=" << data.development.data.size(0)
            << '\n';
  std::cout << "ima4a.target_rows_per_seed=" << evidence[0].target.size(0)
            << '\n';
  for (std::size_t index = 0; index < kIma4aRidgeGrid.size(); ++index) {
    std::cout << "ima4a.ridge_grid.alpha_" << index << '='
              << kIma4aRidgeGrid[index] << '\n';
  }
  ima4a_emit_context_receipt(context);
  for (const auto &seed : evidence) {
    ima4a_emit_seed(seed);
  }

  constexpr std::size_t m2 = 1;
  constexpr std::size_t q = 0;
  constexpr std::size_t g = 1;
  constexpr std::size_t s = 2;
  constexpr std::size_t f = 3;
  constexpr std::size_t r = 4;
  constexpr std::size_t b = 5;
  constexpr std::size_t c = 6;
  const double q_mean = ima4a_mean_oracle_r2(evidence, m2, q);
  const double g_mean = ima4a_mean_oracle_r2(evidence, m2, g);
  const double s_mean = ima4a_mean_oracle_r2(evidence, m2, s);
  const double f_mean = ima4a_mean_oracle_r2(evidence, m2, f);
  const double r_mean = ima4a_mean_oracle_r2(evidence, m2, r);
  const double b_mean = ima4a_mean_oracle_r2(evidence, m2, b);
  const double c_mean = ima4a_mean_oracle_r2(evidence, m2, c);
  const double current_mean = ima4a_mean_current_r2(evidence, m2);
  const double leaky_current_mean = ima4a_mean_current_r2(evidence, 0);
  const auto positive = [&](std::size_t left, std::size_t right) {
    return ima4a_positive_seed_count(
        evidence, [=](const auto &seed) {
          return seed.oracle[m2][left].metric.r2 -
                 seed.oracle[m2][right].metric.r2;
        });
  };
  const int64_t f_over_current_positive = ima4a_positive_seed_count(
      evidence, [](const auto &seed) {
        return seed.oracle[m2][f].metric.r2 - seed.current[m2].r2;
      });
  const int64_t g_over_current_positive = ima4a_positive_seed_count(
      evidence, [](const auto &seed) {
        return seed.oracle[m2][g].metric.r2 - seed.current[m2].r2;
      });
  const auto material = [](double value, int64_t positive_seeds) {
    return value >= kIma4aMaterialR2 && positive_seeds >= 2;
  };
  const bool g_over_q = material(g_mean - q_mean, positive(g, q));
  const bool s_over_g = material(s_mean - g_mean, positive(s, g));
  const bool f_over_s = material(f_mean - s_mean, positive(f, s));
  const bool r_over_f = material(r_mean - f_mean, positive(r, f));
  const bool b_over_f = material(b_mean - f_mean, positive(b, f));
  const bool c_over_f = material(c_mean - f_mean, positive(c, f));
  const bool c_over_b = material(c_mean - b_mean, positive(c, b));
  const bool f_over_current =
      material(f_mean - current_mean, f_over_current_positive);
  const bool g_over_current =
      material(g_mean - current_mean, g_over_current_positive);
  double hidden_gradient_mean = 0.0;
  int64_t protected_harmful_seeds = 0;
  for (const auto &seed : evidence) {
    hidden_gradient_mean += seed.gradient[m2].hidden_fraction;
    protected_harmful_seeds +=
        seed.gradient[m2].full_order_cosine < 0.0 ||
                seed.gradient[m2].full_cross_cosine < 0.0
            ? 1
            : 0;
  }
  hidden_gradient_mean /= static_cast<double>(evidence.size());

  const auto ci_g_q = ima4a_bootstrap_contrast(
      evidence,
      [](const auto &seed) { return seed.oracle_prediction[m2][g]; },
      [](const auto &seed) { return seed.oracle_prediction[m2][q]; });
  const auto ci_s_g = ima4a_bootstrap_contrast(
      evidence,
      [](const auto &seed) { return seed.oracle_prediction[m2][s]; },
      [](const auto &seed) { return seed.oracle_prediction[m2][g]; });
  const auto ci_f_s = ima4a_bootstrap_contrast(
      evidence,
      [](const auto &seed) { return seed.oracle_prediction[m2][f]; },
      [](const auto &seed) { return seed.oracle_prediction[m2][s]; });
  const auto ci_r_f = ima4a_bootstrap_contrast(
      evidence,
      [](const auto &seed) { return seed.oracle_prediction[m2][r]; },
      [](const auto &seed) { return seed.oracle_prediction[m2][f]; });
  const auto ci_b_f = ima4a_bootstrap_contrast(
      evidence,
      [](const auto &seed) { return seed.oracle_prediction[m2][b]; },
      [](const auto &seed) { return seed.oracle_prediction[m2][f]; });
  const auto ci_c_f = ima4a_bootstrap_contrast(
      evidence,
      [](const auto &seed) { return seed.oracle_prediction[m2][c]; },
      [](const auto &seed) { return seed.oracle_prediction[m2][f]; });
  const auto ci_c_b = ima4a_bootstrap_contrast(
      evidence,
      [](const auto &seed) { return seed.oracle_prediction[m2][c]; },
      [](const auto &seed) { return seed.oracle_prediction[m2][b]; });
  const auto ci_f_current = ima4a_bootstrap_contrast(
      evidence,
      [](const auto &seed) { return seed.oracle_prediction[m2][f]; },
      [](const auto &seed) { return seed.current_prediction[m2]; });
  const auto ci_leaky_separated = ima4a_bootstrap_contrast(
      evidence,
      [](const auto &seed) { return seed.current_prediction[0]; },
      [](const auto &seed) { return seed.current_prediction[1]; });

  const auto emit_summary = [&](std::string_view name, double point,
                                int64_t positives, const Interval &interval) {
    std::cout << "ima4a.summary." << name << ".point=" << point << '\n';
    std::cout << "ima4a.summary." << name << ".positive_seeds=" << positives
              << '\n';
    std::cout << "ima4a.summary." << name << ".low=" << interval.low << '\n';
    std::cout << "ima4a.summary." << name << ".high=" << interval.high
              << '\n';
  };
  std::cout << "ima4a.summary.m2.Q_r2=" << q_mean << '\n';
  std::cout << "ima4a.summary.m2.G_r2=" << g_mean << '\n';
  std::cout << "ima4a.summary.m2.S_r2=" << s_mean << '\n';
  std::cout << "ima4a.summary.m2.F_r2=" << f_mean << '\n';
  std::cout << "ima4a.summary.m2.R_r2=" << r_mean << '\n';
  std::cout << "ima4a.summary.m2.B_r2=" << b_mean << '\n';
  std::cout << "ima4a.summary.m2.C_r2=" << c_mean << '\n';
  std::cout << "ima4a.summary.m2.current_r2=" << current_mean << '\n';
  std::cout << "ima4a.summary.m1.current_r2=" << leaky_current_mean << '\n';
  emit_summary("m2.G_minus_Q", g_mean - q_mean, positive(g, q), ci_g_q);
  emit_summary("m2.S_minus_G", s_mean - g_mean, positive(s, g), ci_s_g);
  emit_summary("m2.F_minus_S", f_mean - s_mean, positive(f, s), ci_f_s);
  emit_summary("m2.R_minus_F", r_mean - f_mean, positive(r, f), ci_r_f);
  emit_summary("m2.B_minus_F", b_mean - f_mean, positive(b, f), ci_b_f);
  emit_summary("m2.C_minus_F", c_mean - f_mean, positive(c, f), ci_c_f);
  emit_summary("m2.C_minus_B", c_mean - b_mean, positive(c, b), ci_c_b);
  emit_summary("m2.F_minus_current", f_mean - current_mean,
               f_over_current_positive, ci_f_current);
  emit_summary(
      "current.M1_minus_M2", leaky_current_mean - current_mean,
      ima4a_positive_seed_count(evidence, [](const auto &seed) {
        return seed.current[0].r2 - seed.current[1].r2;
      }),
      ci_leaky_separated);
  std::cout << "ima4a.summary.m2.hidden_gradient_fraction="
            << hidden_gradient_mean << '\n';
  std::cout << "ima4a.summary.m2.protected_harmful_seeds="
            << protected_harmful_seeds << '\n';

  bool mechanics = context.pass;
  for (const auto &seed : evidence) {
    mechanics = mechanics && seed.pass;
  }
  std::string decision = "invalid_mechanics";
  std::string next = "repair_measurement_mechanics_without_scientific_routing";
  const double best_context =
      std::max({g_mean, s_mean, f_mean, r_mean, b_mean, c_mean});
  if (mechanics) {
    decision = "boundary_unresolved";
    next = "one_additional_zero_update_measurement_required";
    if (best_context <= 0.0 &&
        hidden_gradient_mean >= kIma4aHiddenGradientFloor) {
      decision = "bounded_context_controls_failed_target_abstraction_mismatch";
      next = "design_support_permitted_teacher_target_abstraction";
    } else if (c_over_f && c_over_b && c_mean > 0.0) {
      decision = "categorical_target_slot_interaction_bottleneck";
      next = "audit_narrow_target_relative_slot_predictor";
    } else if (b_over_f && b_mean > 0.0) {
      decision = "continuous_query_context_interaction_bottleneck";
      next = "audit_narrow_query_by_slot_predictor";
    } else if (r_over_f && r_mean > 0.0) {
      decision = "context_encoder_linear_accessibility_bottleneck";
      next = "audit_jepa_only_context_contextualizer";
    } else if (f_over_current && f_over_s && f_mean > 0.0) {
      decision = "full_slot_predictor_interaction_bottleneck";
      next = "audit_narrow_slot_aware_predictor";
    } else if (f_over_current && s_over_g && s_mean > 0.0) {
      decision = "structured_context_metadata_bottleneck";
      next = "audit_explicit_context_metadata_in_predictor_keys_values";
    } else if (g_over_q && g_over_current && g_mean > 0.0) {
      decision = "predictor_mapping_or_optimization_bottleneck";
      next = "audit_current_predictor_attention_and_optimization";
    } else if (best_context > 0.0 &&
               std::abs(best_context - current_mean) < kIma4aMaterialR2 &&
               protected_harmful_seeds >= 2) {
      decision = "predictor_capacity_ruled_out_loss_target_ema_next";
      next = "audit_normalized_loss_and_frozen_teacher_gradient_directions";
    } else if (f_over_current && f_mean > 0.0) {
      decision = "predictor_below_usable_context_ceiling_structure_unresolved";
      next = "audit_context_metadata_decodability_and_attention_use";
    }
  }
  std::cout << "ima4a.mechanics_pass=" << mechanics << '\n';
  std::cout << "ima4a.decision=" << decision << '\n';
  std::cout << "ima4a.next_measurement=" << next << '\n';
  std::cout << "ima4a.optimizer_steps=0\n";
  std::cout << "ima4a.ema_updates=0\n";
  std::cout << "ima4a.training_authorized=false\n";
  std::cout << "execution_status=ima4a_measurements_complete\n";
  audit_rng.restore();
  return mechanics ? 0 : 3;
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment == "target-predictability-ceiling-self-test") {
      return run_ima4a_self_test();
    }
    if (options.experiment == "target-predictability-ceiling-audit") {
      return run_ima4a_audit(options);
    }
    throw std::runtime_error(
        "--experiment must be target-predictability-ceiling-self-test or "
        "target-predictability-ceiling-audit");
  } catch (const c10::Error &error) {
    std::cerr << "target_predictability_ceiling_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "target_predictability_ceiling_error=" << error.what()
              << '\n';
  }
  return 2;
}
