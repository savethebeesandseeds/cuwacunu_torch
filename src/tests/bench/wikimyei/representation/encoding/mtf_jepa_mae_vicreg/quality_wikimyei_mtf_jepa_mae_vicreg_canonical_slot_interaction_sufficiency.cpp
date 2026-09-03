#include "piaabo/digest/sha256.h"

// IMA-4C consumes the same sealed FSPA-4/OCA anchor and fixed RMC probes as
// IMA-4A.  Embed OCA directly: embedding IMA-4A is unsafe because the older
// nested RMC translation unit owns the `main` macro while it is expanded.
#define CUWACUNU_OCA_EMBEDDED
#include "quality_wikimyei_mtf_jepa_mae_vicreg_four_objective_causal_attribution.cpp"
#undef CUWACUNU_OCA_EMBEDDED

namespace {

using torch::indexing::Slice;

// Narrow, read-only IMA-3/IMA-4A custody surface.  These definitions mirror
// the sealed audit; they neither include nor mutate either sealed source.
constexpr int64_t kIma3Steps = kOcaAnchorChallengeSteps;
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
constexpr std::array<mtf::mtf_jepa_mask_policy_t, 2> kIma4aPolicies{
    mtf::mtf_jepa_mask_policy_t::paired_target_legacy_context_v1,
    mtf::mtf_jepa_mask_policy_t::support_separated_pair_v1};
constexpr std::array<double, 16> kIma4aRidgeGrid{
    1.0e-10, 1.0e-8, 1.0e-6, 1.0e-5, 1.0e-4, 1.0e-3,
    1.0e-2,  1.0e-1, 1.0,    1.0e1,  1.0e2,  1.0e3,
    1.0e4,   1.0e6, 1.0e8,  1.0e10};
constexpr int64_t kIma4aMetadataWidth = 6;
constexpr int64_t kIma4aTokenCount = 72;
constexpr int64_t kIma4aContextCount = 54;
constexpr int64_t kIma4aTargetsPerGroup = 2;
constexpr std::string_view kIma4aProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/JEPA_TARGET_PREDICTABILITY_CEILING_PROTOCOL.md";
constexpr std::string_view kIma4aSourcePath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_target_predictability_ceiling.cpp";

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
    throw std::runtime_error("IMA-4C IMA-3 cache checksum failed");
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
    throw std::runtime_error("IMA-4C IMA-3 cache metadata failed");
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
      throw std::runtime_error("IMA-4C IMA-3 cache arm identity failed");
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
    throw std::runtime_error("IMA-4C IMA-3 cache receipt failed");
  }
  return true;
}

struct Ima4aModules {
  std::shared_ptr<mtf::TimeFrequencyViewBuilderImpl> tokenizer{};
  std::shared_ptr<mtf::TimeFrequencyViewBuilderImpl> target_tokenizer{};
  std::shared_ptr<mtf::SharedTokenEncoderImpl> encoder{};
  std::shared_ptr<mtf::SharedTokenEncoderImpl> target_encoder{};
  std::shared_ptr<mtf::LatentPredictorImpl> predictor{};
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

template <typename ModuleImpl>
[[nodiscard]] std::shared_ptr<ModuleImpl>
ima4a_child(const mtf::MtfJepaMaeVicreg &model, const std::string &name) {
  for (const auto &item : model->named_children()) {
    if (item.key() == name) {
      auto value = std::dynamic_pointer_cast<ModuleImpl>(item.value());
      if (!value) {
        throw std::runtime_error("IMA-4C child type mismatch: " + name);
      }
      return value;
    }
  }
  throw std::runtime_error("IMA-4C child missing: " + name);
}

[[nodiscard]] Ima4aModules
ima4a_modules(const mtf::MtfJepaMaeVicreg &model) {
  return {.tokenizer = ima4a_child<mtf::TimeFrequencyViewBuilderImpl>(
              model, "tokenizer"),
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
    throw std::runtime_error("IMA-4C invalid target-centered metric");
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
  const bool lower = selected == 0 && curve[0] + tolerance < curve[1];
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
    torch::NoGradGuard no_grad;
    const auto train = train_features.to(torch::kCPU, torch::kFloat64);
    const auto validation =
        validation_features.to(torch::kCPU, torch::kFloat64);
    const auto test = test_features.to(torch::kCPU, torch::kFloat64);
    const auto target_cpu = train_target.to(torch::kCPU, torch::kFloat64);
    const auto mean = train.mean(0);
    const auto variance = (train - mean).pow(2).mean(0);
    const auto inv_std = torch::where(variance > 1.0e-12, variance.rsqrt(),
                                      torch::ones_like(variance));
    const auto x = (train - mean) * inv_std;
    const auto validation_x = (validation - mean) * inv_std;
    const auto test_x = (test - mean) * inv_std;
    const auto y = target_cpu - bias;
    auto gram = x.matmul(x.transpose(0, 1));
    auto validation_kernel = validation_x.matmul(x.transpose(0, 1));
    auto test_kernel = test_x.matmul(x.transpose(0, 1));
    if (map == Ima4aOracleMap::categorical_slot_by_field) {
      if (x.size(1) <= kIma4aMetadataWidth) {
        throw std::runtime_error("IMA-4C categorical field is empty");
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
        if (train_access[row] < 0 || train_access[row] >= kIma4aTokenCount) {
          throw std::runtime_error("IMA-4C categorical identity out of range");
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
                "IMA-4C categorical validation identity unseen in fit");
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
        throw std::runtime_error("IMA-4C dual ridge factorization failed");
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
  (void)config;
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
    throw std::runtime_error("IMA-4C group-paired mask contract failed");
  }
  return result;
}

[[nodiscard]] torch::Tensor ima4a_group_resample(
    const torch::Tensor &value, const torch::Tensor &group_rows) {
  if (value.size(0) % kIma4aTargetsPerGroup != 0) {
    throw std::runtime_error("IMA-4C group resample shape failed");
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

constexpr int64_t kIma4cUpdates = 1024;
constexpr int64_t kIma4cGroupBatch = 32;
constexpr int64_t kIma4cTargetsPerGroup = 2;
constexpr int64_t kIma4cSlotCount = 72;
constexpr int64_t kIma4cLatentDim = 32;
constexpr int64_t kIma4cHeadCount = 4;
constexpr int64_t kIma4cPairRank = 4;
constexpr int64_t kIma4cCommonParameters = 11744;
constexpr int64_t kIma4cOutputSlotParameters =
    kIma4cSlotCount * kIma4cLatentDim;
constexpr int64_t kIma4cPairFactorParameters =
    2 * kIma4cHeadCount * kIma4cSlotCount * kIma4cPairRank;
constexpr int64_t kIma4cTrainableParameters =
    kIma4cCommonParameters + kIma4cOutputSlotParameters +
    kIma4cPairFactorParameters;
constexpr double kIma4cLearningRate = 1.0e-3;
constexpr double kIma4cGradientClip = 5.0;
constexpr double kIma4cTolerance = 2.0e-6;
constexpr double kIma4cComputeCensorDelta = 0.005;
constexpr std::array<int64_t, 13> kIma4cValidationSteps{
    0,   64,  128, 192, 256, 320, 384,
    448, 512, 640, 768, 896, 1024};
constexpr std::array<double, 3> kIma4cSealedCurrentR2{
    -1.40016785751, -1.5562935319, -1.28212443802};
constexpr std::array<double, 3> kIma4cSealedCR2{
    0.0739973874835, 0.0421979052359, 0.00858663293665};
constexpr std::string_view kIma4cProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "JEPA_CANONICAL_SLOT_INTERACTION_SUFFICIENCY_PROTOCOL.md";
constexpr std::string_view kIma4cProtocolSha256 =
    "885c16d3f40cb57d291a2ba4481ed264166765e318e1189b67f8267b7f72c667";
constexpr std::string_view kIma4cSourcePath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_"
    "canonical_slot_interaction_sufficiency.cpp";
constexpr std::string_view kIma4cIma4aLogPath =
    ".build/tests/representation_ima4a_v1_authoritative.log";
constexpr std::string_view kIma4cIma4aLogSha256 =
    "eebd1e59167aad75bdfce69f5176ee4f4e040400181133e5f8f66dca3afe7606";
constexpr std::string_view kIma4cIma4aSourceSha256 =
    "d41011207cde4f5a780c6ff96a77a59aef6c2086932874087db4d8388565867c";
constexpr std::string_view kIma4cIma4aProtocolSha256 =
    "23fc3d516bfca285dda9ac901efe89acc12b29032ddcabf50020bfb0ebf77af1";
constexpr std::string_view kIma4cIma3SourceSha256 =
    "97c096b5331dcf83cea4c23067dc2806ec09d03d8d9f19614c86595028196c16";
constexpr std::string_view kIma4cHeaderSha256 =
    "93640972e497dc49f37e7690e59c2f2e55f12ece25687fe4e6f6c96b28c3c9ea";
constexpr std::string_view kIma4cIma4bLogPath =
    ".build/tests/representation_ima4b_v1a2_authoritative.log";
constexpr std::string_view kIma4cIma4bLogSha256 =
    "9ce1be7b00bd9338cf7d640502db3d0d7a9731e1fec163b24158a437525f9f8c";
constexpr std::string_view kIma4cIma4bSourcePath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_"
    "target_relative_predictor_sufficiency.cpp";
constexpr std::string_view kIma4cIma4bSourceSha256 =
    "53a58a72326b5c3ae070f50f36bc78cc7863d1a94d08ac55107337790e634c76";
constexpr std::array<std::string_view, 3> kIma4cIma4bProtocolPaths{
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/JEPA_TARGET_RELATIVE_PREDICTOR_SUFFICIENCY_"
    "PROTOCOL.md",
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/JEPA_TARGET_RELATIVE_PREDICTOR_SUFFICIENCY_"
    "PROTOCOL_AMENDMENT_A1.md",
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/JEPA_TARGET_RELATIVE_PREDICTOR_SUFFICIENCY_"
    "PROTOCOL_AMENDMENT_A2.md"};
constexpr std::array<std::string_view, 3> kIma4cIma4bProtocolSha256{
    "571120d44de1e1c3843ff440a14a43b27d5157d1505f691776842c3f46827b95",
    "6f9879ccd1b862eeca9515b5628b09fa028e56e071f60dbb190e9c6260a236c3",
    "5d766e77947c88b370309e281276c05b640609c3ad42b6525240de64bb36aa00"};

enum class Ima4cArm : std::size_t {
  canonical_source_labels = 0,
  permuted_source_labels = 1,
};

constexpr std::array<std::string_view, 2> kIma4cArmNames{
    "canonical_source_labels", "permuted_source_labels"};

[[nodiscard]] torch::Tensor ima4c_initial_source_factor() {
  constexpr uint64_t seed = 0x696d6134635f7631ULL;
  const int64_t count =
      kIma4cHeadCount * kIma4cSlotCount * kIma4cPairRank;
  std::vector<float> values;
  values.reserve(static_cast<std::size_t>(count));
  for (int64_t index = 0; index < count; ++index) {
    const uint64_t mixed = splitmix64(seed ^ static_cast<uint64_t>(index));
    const auto bucket = static_cast<uint32_t>((mixed >> 32U) & 0xffffU);
    float value =
        (2.0F * (static_cast<float>(bucket) + 0.5F) / 65536.0F - 1.0F) /
        std::sqrt(static_cast<float>(kIma4cPairRank));
    if (std::abs(value) < 1.0e-4F) {
      value = (index % 2 == 0 ? 1.0F : -1.0F) * 1.0e-4F;
    }
    values.push_back(value);
  }
  return torch::tensor(values, torch::kFloat32)
      .reshape({kIma4cHeadCount, kIma4cSlotCount, kIma4cPairRank})
      .contiguous();
}

[[nodiscard]] torch::Tensor ima4c_pair_bias(
    const torch::Tensor &target_factor, const torch::Tensor &source_factor,
    const torch::Tensor &target_slot, const torch::Tensor &source_label) {
  if (target_factor.sizes() !=
          torch::IntArrayRef(
              {kIma4cHeadCount, kIma4cSlotCount, kIma4cPairRank}) ||
      source_factor.sizes() != target_factor.sizes() ||
      target_slot.dim() != 1 || source_label.dim() != 2 ||
      source_label.size(0) != target_slot.size(0) ||
      source_label.size(1) != kIma4cSlotCount) {
    throw std::runtime_error("IMA-4C pair-bias shape contract failed");
  }
  const auto slot = target_slot.to(target_factor.device(), torch::kInt64);
  const auto label = source_label.to(target_factor.device(), torch::kInt64);
  if (slot.min().item<int64_t>() < 0 ||
      slot.max().item<int64_t>() >= kIma4cSlotCount ||
      label.min().item<int64_t>() < 0 ||
      label.max().item<int64_t>() >= kIma4cSlotCount) {
    throw std::runtime_error("IMA-4C pair-bias index contract failed");
  }
  const auto eye = torch::eye(kIma4cSlotCount, target_factor.options());
  const auto target_one_hot = eye.index_select(0, slot);
  const auto full = torch::bmm(target_factor,
                               source_factor.transpose(1, 2)) /
                    std::sqrt(static_cast<double>(kIma4cPairRank));
  const auto flat = full.permute({1, 0, 2})
                        .contiguous()
                        .view({kIma4cSlotCount,
                               kIma4cHeadCount * kIma4cSlotCount});
  const auto selected = target_one_hot.matmul(flat).view(
      {slot.size(0), kIma4cHeadCount, kIma4cSlotCount});
  const auto source_permutation =
      eye.index_select(0, label.reshape({-1}))
          .view({slot.size(0), kIma4cSlotCount, kIma4cSlotCount});
  return torch::bmm(selected, source_permutation.transpose(1, 2))
      .unsqueeze(2);
}

[[nodiscard]] torch::Tensor ima4c_context_attention(
    const torch::Tensor &q, const torch::Tensor &k, const torch::Tensor &v,
    const torch::Tensor &context_mask, const torch::Tensor &pair_bias,
    int64_t num_heads) {
  TORCH_CHECK(q.dim() == 3 && k.dim() == 3 && v.dim() == 3,
              "IMA-4C attention tensors must be [B,N,D]");
  TORCH_CHECK(q.size(0) == k.size(0) && k.sizes() == v.sizes() &&
                  q.size(2) == k.size(2),
              "IMA-4C attention tensor shape mismatch");
  TORCH_CHECK(context_mask.size(0) == k.size(0) &&
                  context_mask.size(1) == k.size(1),
              "IMA-4C attention mask shape mismatch");
  const int64_t batch = q.size(0);
  const int64_t query_count = q.size(1);
  const int64_t key_count = k.size(1);
  const int64_t width = q.size(2);
  TORCH_CHECK(num_heads > 0 && width % num_heads == 0,
              "IMA-4C latent width must divide heads");
  TORCH_CHECK(pair_bias.sizes() ==
                  torch::IntArrayRef(
                      {batch, num_heads, query_count, key_count}),
              "IMA-4C pair-bias attention shape mismatch");
  const int64_t head_width = width / num_heads;
  auto qh = q.contiguous()
                .view({batch, query_count, num_heads, head_width})
                .permute({0, 2, 1, 3});
  auto kh = k.contiguous()
                .view({batch, key_count, num_heads, head_width})
                .permute({0, 2, 1, 3});
  auto vh = v.contiguous()
                .view({batch, key_count, num_heads, head_width})
                .permute({0, 2, 1, 3});
  auto scores = torch::matmul(qh, kh.transpose(-2, -1)) /
                std::sqrt(static_cast<double>(head_width));
  scores = scores + pair_bias.to(scores.options());
  const auto mask = context_mask.to(torch::kBool)
                        .view({batch, 1, 1, key_count})
                        .to(scores.device());
  const auto mask_float = mask.to(scores.dtype());
  scores = scores.masked_fill(mask.logical_not(), -1.0e9);
  auto weights = torch::softmax(scores, -1) * mask_float;
  weights = weights / weights.sum(-1, true).clamp_min(1.0e-6);
  auto attended = torch::matmul(weights, vh);
  const auto has_context = context_mask.to(torch::kBool)
                               .any(1)
                               .to(scores.dtype())
                               .view({batch, 1, 1, 1})
                               .to(scores.device());
  attended = attended * has_context;
  return attended.permute({0, 2, 1, 3})
      .contiguous()
      .view({batch, query_count, width});
}

class Ima4cPredictorImpl : public torch::nn::Module {
public:
  explicit Ima4cPredictorImpl(mtf::mtf_jepa_mae_vicreg_config_t config)
      : config_(std::move(config)) {
    mtf::detail::validate_config(config_);
    if (config_.num_heads != kIma4cHeadCount ||
        config_.latent_dim != kIma4cLatentDim) {
      throw std::runtime_error("IMA-4C settled head/latent contract failed");
    }
    metadata_projection_ = register_module(
        "metadata_projection", torch::nn::Linear(6, config_.latent_dim));
    q_projection_ = register_module(
        "q_projection",
        torch::nn::Linear(config_.latent_dim, config_.latent_dim));
    k_projection_ = register_module(
        "k_projection",
        torch::nn::Linear(config_.latent_dim, config_.latent_dim));
    v_projection_ = register_module(
        "v_projection",
        torch::nn::Linear(config_.latent_dim, config_.latent_dim));
    layers_.push_back(register_module(
        "predictor_in",
        torch::nn::Linear(config_.latent_dim, config_.predictor_hidden_dim)));
    for (int64_t index = 1; index < config_.num_predictor_layers; ++index) {
      layers_.push_back(register_module(
          "predictor_hidden_" + std::to_string(index),
          torch::nn::Linear(config_.predictor_hidden_dim,
                            config_.predictor_hidden_dim)));
    }
    out_ = register_module(
        "predictor_out",
        torch::nn::Linear(config_.predictor_hidden_dim, config_.latent_dim));
    dropout_ =
        register_module("dropout", torch::nn::Dropout(config_.dropout));
    output_slot_embedding_ = register_module(
        "output_slot_embedding",
        torch::nn::Embedding(
            torch::nn::EmbeddingOptions(kIma4cSlotCount, config_.latent_dim)));
    target_factor_ = register_parameter(
        "target_factor",
        torch::zeros({kIma4cHeadCount, kIma4cSlotCount, kIma4cPairRank},
                     torch::kFloat32));
    source_factor_ = register_parameter(
        "source_factor", ima4c_initial_source_factor());
    this->to(config_.device, config_.dtype);
    torch::NoGradGuard no_grad;
    output_slot_embedding_->weight.zero_();
    target_factor_.zero_();
    source_factor_.copy_(
        ima4c_initial_source_factor().to(config_.device, config_.dtype));
  }

  [[nodiscard]] torch::Tensor
  forward(const torch::Tensor &context_latents,
          const torch::Tensor &context_mask,
          const torch::Tensor &target_metadata,
          const torch::Tensor &target_slot,
          const torch::Tensor &source_label) {
    if (context_latents.dim() != 3 || context_mask.dim() != 2 ||
        target_metadata.dim() != 2 || target_metadata.size(1) != 6 ||
        target_slot.dim() != 1 ||
        context_latents.size(0) != target_metadata.size(0) ||
        context_latents.size(0) != target_slot.size(0) ||
        context_latents.size(1) != kIma4cSlotCount ||
        context_latents.size(2) != config_.latent_dim ||
        context_mask.size(0) != context_latents.size(0) ||
        context_mask.size(1) != context_latents.size(1) ||
        source_label.sizes() != context_mask.sizes()) {
      throw std::runtime_error("IMA-4C predictor input contract failed");
    }
    if (target_slot.min().item<int64_t>() < 0 ||
        target_slot.max().item<int64_t>() >= kIma4cSlotCount) {
      throw std::runtime_error("IMA-4C target slot is out of range");
    }
    const auto options = torch::TensorOptions()
                             .dtype(config_.dtype)
                             .device(config_.device);
    const auto q0 = q_projection_->forward(metadata_projection_->forward(
        target_metadata.to(options)));
    const auto output_slot = output_slot_embedding_->forward(
        target_slot.to(config_.device, torch::kInt64));
    const auto keys = k_projection_->forward(context_latents.to(options));
    const auto values = v_projection_->forward(context_latents.to(options));
    const auto mask = context_mask.to(config_.device, torch::kBool);
    const auto bias = ima4c_pair_bias(
        target_factor_, source_factor_, target_slot, source_label);
    auto attended = ima4c_context_attention(
                        q0.unsqueeze(1), keys, values, mask, bias,
                        config_.num_heads)
                        .squeeze(1);
    auto hidden = attended + q0;
    for (auto &layer : layers_) {
      hidden = dropout_->forward(torch::gelu(layer->forward(hidden)));
    }
    return out_->forward(hidden) + output_slot;
  }

  [[nodiscard]] torch::Tensor target_factor() { return target_factor_; }
  [[nodiscard]] torch::Tensor source_factor() { return source_factor_; }
  [[nodiscard]] torch::Tensor output_slot_weight() {
    return output_slot_embedding_->weight;
  }

private:
  mtf::mtf_jepa_mae_vicreg_config_t config_{};
  torch::nn::Linear metadata_projection_{nullptr};
  torch::nn::Linear q_projection_{nullptr};
  torch::nn::Linear k_projection_{nullptr};
  torch::nn::Linear v_projection_{nullptr};
  std::vector<torch::nn::Linear> layers_{};
  torch::nn::Linear out_{nullptr};
  torch::nn::Dropout dropout_{nullptr};
  torch::nn::Embedding output_slot_embedding_{nullptr};
  torch::Tensor target_factor_{};
  torch::Tensor source_factor_{};
};

TORCH_MODULE(Ima4cPredictor);

struct Ima4cPredictorSnapshot {
  std::vector<std::string> names{};
  std::vector<torch::Tensor> values{};
};

[[nodiscard]] bool
ima4c_snapshots_exact(const Ima4cPredictorSnapshot &left,
                      const Ima4cPredictorSnapshot &right) {
  if (left.names != right.names || left.values.size() != right.values.size()) {
    return false;
  }
  for (std::size_t index = 0; index < left.values.size(); ++index) {
    if (!rssm_tensor_bytes_equal(left.values[index], right.values[index])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] double ima4c_snapshot_max_abs_diff(
    const Ima4cPredictorSnapshot &left,
    const Ima4cPredictorSnapshot &right,
    std::string_view parameter_name = {}) {
  if (left.names != right.names || left.values.size() != right.values.size()) {
    throw std::runtime_error("IMA-4C snapshot comparison layout failed");
  }
  double maximum = 0.0;
  bool found = parameter_name.empty();
  for (std::size_t index = 0; index < left.values.size(); ++index) {
    if (!parameter_name.empty() && left.names[index] != parameter_name) {
      continue;
    }
    found = true;
    maximum = std::max(
        maximum,
        (left.values[index].to(torch::kFloat64) -
         right.values[index].to(torch::kFloat64))
            .abs()
            .max()
            .item<double>());
  }
  if (!found) {
    throw std::runtime_error("IMA-4C snapshot parameter missing");
  }
  return maximum;
}

[[nodiscard]] Ima4cPredictorSnapshot
ima4c_snapshot_predictor(const Ima4cPredictor &predictor) {
  Ima4cPredictorSnapshot result{};
  torch::NoGradGuard no_grad;
  for (const auto &item : predictor->named_parameters(/*recurse=*/true)) {
    result.names.push_back(item.key());
    result.values.push_back(
        item.value().detach().to(torch::kCPU).contiguous().clone());
  }
  return result;
}

void ima4c_restore_predictor(Ima4cPredictor &predictor,
                             const Ima4cPredictorSnapshot &snapshot) {
  const auto parameters = predictor->named_parameters(/*recurse=*/true);
  if (parameters.size() != snapshot.values.size()) {
    throw std::runtime_error("IMA-4C predictor restore count failed");
  }
  torch::NoGradGuard no_grad;
  std::size_t index = 0;
  for (const auto &item : parameters) {
    if (item.key() != snapshot.names[index] ||
        item.value().sizes() != snapshot.values[index].sizes() ||
        item.value().scalar_type() != snapshot.values[index].scalar_type()) {
      throw std::runtime_error("IMA-4C predictor restore layout failed");
    }
    item.value().copy_(snapshot.values[index].to(item.value().device()));
    ++index;
  }
}

[[nodiscard]] int64_t
ima4c_parameter_count(const Ima4cPredictor &predictor) {
  int64_t result = 0;
  for (const auto &parameter : predictor->parameters()) {
    result += parameter.numel();
  }
  return result;
}

[[nodiscard]] double ima4c_predictor_max_abs_diff(
    const Ima4cPredictor &predictor,
    const Ima4cPredictorSnapshot &reference) {
  const auto parameters = predictor->named_parameters(/*recurse=*/true);
  if (parameters.size() != reference.values.size()) {
    throw std::runtime_error("IMA-4C predictor comparison count failed");
  }
  double maximum = 0.0;
  std::size_t index = 0;
  torch::NoGradGuard no_grad;
  for (const auto &item : parameters) {
    if (item.key() != reference.names[index] ||
        item.value().sizes() != reference.values[index].sizes()) {
      throw std::runtime_error("IMA-4C predictor comparison layout failed");
    }
    maximum = std::max(
        maximum,
        (item.value().detach().to(torch::kCPU, torch::kFloat64) -
         reference.values[index].to(torch::kFloat64))
            .abs()
            .max()
            .item<double>());
    ++index;
  }
  return maximum;
}

[[nodiscard]] bool
ima4c_predictor_finite(const Ima4cPredictor &predictor,
                       bool require_gradients) {
  bool saw_gradient = false;
  for (const auto &parameter : predictor->parameters()) {
    if (!torch::isfinite(parameter).all().item<bool>()) {
      return false;
    }
    if (parameter.grad().defined()) {
      saw_gradient = true;
      if (!torch::isfinite(parameter.grad()).all().item<bool>()) {
        return false;
      }
    }
  }
  return !require_gradients || saw_gradient;
}

void ima4c_copy_production_predictor(
    const std::shared_ptr<mtf::LatentPredictorImpl> &source,
    Ima4cPredictor &destination) {
  const auto source_parameters =
      source->named_parameters(/*recurse=*/true);
  const auto destination_parameters =
      destination->named_parameters(/*recurse=*/true);
  int64_t copied = 0;
  torch::NoGradGuard no_grad;
  for (const auto &source_item : source_parameters) {
    bool found = false;
    for (const auto &destination_item : destination_parameters) {
      if (source_item.key() != destination_item.key()) {
        continue;
      }
      if (source_item.value().sizes() != destination_item.value().sizes() ||
          source_item.value().scalar_type() !=
              destination_item.value().scalar_type()) {
        throw std::runtime_error("IMA-4C common predictor layout failed");
      }
      destination_item.value().copy_(source_item.value());
      copied += source_item.value().numel();
      found = true;
      break;
    }
    if (!found) {
      throw std::runtime_error("IMA-4C common predictor parameter missing");
    }
  }
  destination->output_slot_weight().zero_();
  destination->target_factor().zero_();
  destination->source_factor().copy_(
      ima4c_initial_source_factor().to(
          destination->source_factor().device(),
          destination->source_factor().scalar_type()));
  if (copied != kIma4cCommonParameters ||
      ima4c_parameter_count(destination) != kIma4cTrainableParameters) {
    throw std::runtime_error("IMA-4C predictor parameter count failed");
  }
}

[[nodiscard]] std::array<torch::Tensor, 2> ima4c_source_label_maps(
    const torch::Tensor &context_mask, const torch::Tensor &group_id,
    int64_t model_seed);

[[nodiscard]] uint64_t ima4c_source_label_hash(
    const std::array<torch::Tensor, 2> &maps) {
  uint64_t result = 0xcbf29ce484222325ULL;
  for (const auto &map : maps) {
    mix_hash_value(result, hash_tensor_stable_bytes(map));
  }
  return result;
}

struct Ima4cFrozenSplit {
  torch::Tensor context{};         // [G,72,32], CPU float32
  torch::Tensor context_mask{};    // [G,72], CPU bool
  torch::Tensor target_metadata{}; // [G,2,6], CPU float32
  torch::Tensor target_slot{};     // [G,2], CPU int64
  torch::Tensor target{};          // [G,2,32], CPU float32
  torch::Tensor current{};         // [G,2,32], CPU float32
  torch::Tensor group_id{};        // [G], CPU int64
  std::array<torch::Tensor, 2> source_label{}; // [G,72], CPU int64
  std::array<uint64_t, 2> source_label_arm_hash{};
  uint64_t source_label_hash{0};
  uint64_t hash{0};
  bool pairing_exact{false};
  bool finite{false};
};

[[nodiscard]] uint64_t ima4c_capture_hash(const Ima4cFrozenSplit &split) {
  uint64_t result = 0xcbf29ce484222325ULL;
  for (const auto &value :
       {split.context, split.context_mask, split.target_metadata,
        split.target_slot, split.target, split.current, split.group_id}) {
    mix_hash_value(result, hash_tensor_stable_bytes(value));
  }
  return result;
}

[[nodiscard]] torch::Tensor ima4c_flatten_targets(const torch::Tensor &value) {
  auto shape = value.sizes().vec();
  if (shape.size() < 2 || shape[1] != kIma4cTargetsPerGroup) {
    throw std::runtime_error("IMA-4C target flatten contract failed");
  }
  shape.erase(shape.begin() + 1);
  shape[0] *= kIma4cTargetsPerGroup;
  return value.reshape(shape).contiguous();
}

[[nodiscard]] torch::Tensor ima4c_repeated_group_rows(int64_t groups) {
  std::vector<int64_t> rows;
  rows.reserve(static_cast<std::size_t>(groups * kIma4cTargetsPerGroup));
  for (int64_t group = 0; group < groups; ++group) {
    for (int64_t target = 0; target < kIma4cTargetsPerGroup; ++target) {
      rows.push_back(group);
    }
  }
  return torch::tensor(rows, torch::kInt64);
}

[[nodiscard]] Ima4cFrozenSplit ima4c_capture_split(
    const Ima4aModules &modules, const Dataset &dataset,
    const mtf::mtf_jepa_mae_vicreg_config_t &config,
    const torch::Device &device, int64_t seed) {
  std::vector<torch::Tensor> contexts;
  std::vector<torch::Tensor> context_masks;
  std::vector<torch::Tensor> target_metadata;
  std::vector<torch::Tensor> target_slots;
  std::vector<torch::Tensor> targets;
  std::vector<torch::Tensor> currents;
  std::vector<torch::Tensor> group_ids;
  bool pairing = true;
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
        online.tokens.size(1) != kIma4cSlotCount) {
      throw std::runtime_error("IMA-4C online/teacher layout failed");
    }
    const auto pair = ima4a_group_paired_masks(
        online, seed, dataset.group_begin + begin, config, device);
    pairing = pairing && pair.target_exact && pair.counts_exact &&
              pair.rng_exact;
    const auto &masks = pair.masks[1];
    const auto locations = masks.target_mask.nonzero();
    if (locations.sizes() !=
        torch::IntArrayRef({size * kIma4cTargetsPerGroup, 2})) {
      throw std::runtime_error("IMA-4C selected-target shape failed");
    }
    const auto target_rows = locations.select(1, 0).contiguous();
    const auto target_indices = locations.select(1, 1).contiguous();
    const auto expected_rows =
        ima4c_repeated_group_rows(size).to(device, torch::kInt64);
    if (!torch::equal(target_rows, expected_rows)) {
      throw std::runtime_error("IMA-4C selected-target grouping failed");
    }

    const auto full_target =
        modules.target_encoder->forward(teacher.tokens, teacher.token_mask);
    const auto selected_target =
        full_target.index({target_rows, target_indices, Slice()});
    const auto context_tokens = online.tokens.masked_fill(
        masks.context_mask.logical_not().unsqueeze(-1), 0.0);
    const auto context_latents =
        modules.encoder->forward(context_tokens, masks.context_mask);
    const auto prediction = modules.predictor->forward(
        context_latents, masks.context_mask, online.metadata);
    const auto selected_prediction =
        prediction.index({target_rows, target_indices, Slice()});
    const auto metadata =
        mtf::detail::metadata_features(online.metadata, config)
            .index_select(0, target_indices);

    contexts.push_back(context_latents.detach()
                           .to(torch::kCPU, torch::kFloat32)
                           .contiguous()
                           .clone());
    context_masks.push_back(masks.context_mask.detach()
                                .to(torch::kCPU, torch::kBool)
                                .contiguous()
                                .clone());
    target_metadata.push_back(metadata.reshape({size, 2, 6})
                                  .detach()
                                  .to(torch::kCPU, torch::kFloat32)
                                  .contiguous()
                                  .clone());
    target_slots.push_back(target_indices.reshape({size, 2})
                               .detach()
                               .to(torch::kCPU, torch::kInt64)
                               .contiguous()
                               .clone());
    targets.push_back(selected_target.reshape({size, 2, kIma4cLatentDim})
                          .detach()
                          .to(torch::kCPU, torch::kFloat32)
                          .contiguous()
                          .clone());
    currents.push_back(
        selected_prediction.reshape({size, 2, kIma4cLatentDim})
            .detach()
            .to(torch::kCPU, torch::kFloat32)
            .contiguous()
            .clone());
    group_ids.push_back(
        torch::arange(dataset.group_begin + begin,
                      dataset.group_begin + begin + size, torch::kInt64));
  }

  Ima4cFrozenSplit result{};
  result.context = torch::cat(contexts, 0).contiguous();
  result.context_mask = torch::cat(context_masks, 0).contiguous();
  result.target_metadata = torch::cat(target_metadata, 0).contiguous();
  result.target_slot = torch::cat(target_slots, 0).contiguous();
  result.target = torch::cat(targets, 0).contiguous();
  result.current = torch::cat(currents, 0).contiguous();
  result.group_id = torch::cat(group_ids, 0).contiguous();
  result.source_label = ima4c_source_label_maps(
      result.context_mask, result.group_id, seed);
  for (std::size_t arm = 0; arm < result.source_label.size(); ++arm) {
    result.source_label_arm_hash[arm] =
        hash_tensor_stable_bytes(result.source_label[arm]);
  }
  result.source_label_hash =
      ima4c_source_label_hash(result.source_label);
  result.pairing_exact = pairing;
  result.finite = torch::isfinite(result.context).all().item<bool>() &&
                  torch::isfinite(result.target_metadata).all().item<bool>() &&
                  torch::isfinite(result.target).all().item<bool>() &&
                  torch::isfinite(result.current).all().item<bool>();
  const auto expected_groups =
      torch::arange(dataset.group_begin,
                    dataset.group_begin + dataset.data.size(0), torch::kInt64);
  const bool shape =
      result.context.sizes() ==
          torch::IntArrayRef(
              {dataset.data.size(0), kIma4cSlotCount, kIma4cLatentDim}) &&
      result.context_mask.sizes() ==
          torch::IntArrayRef({dataset.data.size(0), kIma4cSlotCount}) &&
      result.target.sizes() ==
          torch::IntArrayRef({dataset.data.size(0), 2, kIma4cLatentDim}) &&
      result.target_metadata.sizes() ==
          torch::IntArrayRef({dataset.data.size(0), 2, 6}) &&
      result.target_slot.sizes() ==
          torch::IntArrayRef({dataset.data.size(0), 2}) &&
      result.source_label[0].sizes() ==
          torch::IntArrayRef({dataset.data.size(0), kIma4cSlotCount}) &&
      result.source_label[1].sizes() ==
          torch::IntArrayRef({dataset.data.size(0), kIma4cSlotCount}) &&
      torch::equal(result.group_id, expected_groups) &&
      result.context_mask.sum(1).eq(kIma4aContextCount).all().item<bool>();
  if (!shape || !result.pairing_exact || !result.finite) {
    throw std::runtime_error("IMA-4C frozen capture contract failed");
  }
  result.hash = ima4c_capture_hash(result);
  return result;
}

struct Ima4cDeviceBatch {
  torch::Tensor context{};
  torch::Tensor context_mask{};
  torch::Tensor target_metadata{};
  torch::Tensor target_slot{};
  torch::Tensor target{};
  std::array<torch::Tensor, 2> source_label{};
};

// The control map is a property of the absolute group, not of a split,
// epoch, checkpoint, or target row.  Keeping this key fixed avoids a
// control-only train/evaluation shift while the absolute group id prevents a
// single global inverse from recovering the canonical labels.
constexpr uint64_t kIma4cPermutationProtocolKey = 0x696d6134635f7031ULL;

[[nodiscard]] torch::Tensor ima4c_canonical_source_labels(int64_t groups) {
  return torch::arange(kIma4cSlotCount, torch::kInt64)
      .unsqueeze(0)
      .expand({groups, kIma4cSlotCount})
      .clone()
      .contiguous();
}

[[nodiscard]] torch::Tensor ima4c_permuted_source_labels(
    const torch::Tensor &context_mask_input,
    const torch::Tensor &group_id_input, int64_t model_seed) {
  const auto context_mask =
      context_mask_input.to(torch::kCPU, torch::kBool).contiguous();
  const auto group_id =
      group_id_input.to(torch::kCPU, torch::kInt64).contiguous();
  if (context_mask.dim() != 2 ||
      context_mask.size(1) != kIma4cSlotCount || group_id.dim() != 1 ||
      group_id.size(0) != context_mask.size(0)) {
    throw std::runtime_error("IMA-4C permutation input contract failed");
  }
  const auto mask = context_mask.accessor<bool, 2>();
  const auto groups = group_id.accessor<int64_t, 1>();
  std::vector<int64_t> flattened(
      static_cast<std::size_t>(context_mask.size(0) * kIma4cSlotCount));
  for (int64_t row = 0; row < context_mask.size(0); ++row) {
    std::array<std::vector<int64_t>, 2> partitions;
    for (int64_t slot = 0; slot < kIma4cSlotCount; ++slot) {
      partitions[mask[row][slot] ? 1 : 0].push_back(slot);
      flattened[static_cast<std::size_t>(row * kIma4cSlotCount + slot)] =
          slot;
    }
    for (std::size_t active = 0; active < partitions.size(); ++active) {
      auto labels = partitions[active];
      if (labels.size() <= 1) {
        continue;
      }
      uint64_t state = splitmix64(
          kIma4cPermutationProtocolKey ^
          static_cast<uint64_t>(model_seed) ^
          splitmix64(static_cast<uint64_t>(groups[row])) ^
          (active == 0 ? 0x696e616374697665ULL : 0x6163746976655f31ULL));
      // Sattolo's cycle is a deterministic derangement: every physical slot
      // receives another label from the same active/inactive partition.
      for (std::size_t size = labels.size(); size > 1; --size) {
        state = splitmix64(state);
        const std::size_t swap_index =
            static_cast<std::size_t>(state % (size - 1));
        std::swap(labels[size - 1], labels[swap_index]);
      }
      for (std::size_t index = 0; index < labels.size(); ++index) {
        flattened[static_cast<std::size_t>(
            row * kIma4cSlotCount + partitions[active][index])] =
            labels[index];
      }
    }
  }
  auto result = torch::tensor(flattened, torch::kInt64)
                    .reshape({context_mask.size(0), kIma4cSlotCount})
                    .contiguous();
  const auto labels = result.accessor<int64_t, 2>();
  for (int64_t row = 0; row < result.size(0); ++row) {
    std::array<int64_t, 2> counts{};
    std::array<int64_t, 2> mapped_counts{};
    std::array<bool, kIma4cSlotCount> seen{};
    for (int64_t slot = 0; slot < kIma4cSlotCount; ++slot) {
      const int64_t label = labels[row][slot];
      if (label < 0 || label >= kIma4cSlotCount ||
          seen[static_cast<std::size_t>(label)] ||
          label == slot ||
          mask[row][slot] != mask[row][label]) {
        throw std::runtime_error("IMA-4C permutation custody failed");
      }
      seen[static_cast<std::size_t>(label)] = true;
      ++counts[mask[row][slot] ? 1 : 0];
      ++mapped_counts[mask[row][label] ? 1 : 0];
    }
    if (counts != mapped_counts) {
      throw std::runtime_error("IMA-4C factor-row multiset custody failed");
    }
  }
  return result;
}

[[nodiscard]] std::array<torch::Tensor, 2> ima4c_source_label_maps(
    const torch::Tensor &context_mask, const torch::Tensor &group_id,
    int64_t model_seed) {
  return {ima4c_canonical_source_labels(context_mask.size(0)),
          ima4c_permuted_source_labels(context_mask, group_id, model_seed)};
}

[[nodiscard]] Ima4cDeviceBatch ima4c_device_batch(
    const Ima4cFrozenSplit &split, const torch::Tensor &group_rows,
    const torch::Device &device) {
  const auto rows = group_rows.to(torch::kCPU, torch::kInt64).contiguous();
  const auto repeated = ima4c_repeated_group_rows(rows.size(0));
  const auto group_context = split.context.index_select(0, rows);
  const auto group_mask = split.context_mask.index_select(0, rows);
  return {
      .context =
          group_context.index_select(0, repeated).to(device, torch::kFloat32),
      .context_mask =
          group_mask.index_select(0, repeated).to(device, torch::kBool),
      .target_metadata =
          ima4c_flatten_targets(split.target_metadata.index_select(0, rows))
              .to(device, torch::kFloat32),
      .target_slot =
          ima4c_flatten_targets(split.target_slot.index_select(0, rows))
              .to(device, torch::kInt64),
      .target = ima4c_flatten_targets(split.target.index_select(0, rows))
                    .to(device, torch::kFloat32),
      .source_label =
          {split.source_label[0]
               .index_select(0, rows)
               .index_select(0, repeated)
               .to(device, torch::kInt64),
           split.source_label[1]
               .index_select(0, rows)
               .index_select(0, repeated)
               .to(device, torch::kInt64)},
  };
}

[[nodiscard]] std::vector<int64_t>
ima4c_training_groups(int64_t groups, int64_t seed, int64_t step) {
  if (groups <= 0 || groups % kIma4cGroupBatch != 0 || step < 0) {
    throw std::runtime_error("IMA-4C training schedule contract failed");
  }
  const int64_t batches_per_epoch = groups / kIma4cGroupBatch;
  const int64_t epoch = step / batches_per_epoch;
  const int64_t start = (step % batches_per_epoch) * kIma4cGroupBatch;
  const auto order = epoch_permutation(groups, seed, epoch);
  return {order.begin() + start, order.begin() + start + kIma4cGroupBatch};
}

[[nodiscard]] int64_t ima4c_dropout_seed(int64_t seed, int64_t step) {
  const auto mixed = splitmix64(
      0x696d6134635f6472ULL ^ static_cast<uint64_t>(seed) ^
      (static_cast<uint64_t>(step) << 32U));
  return static_cast<int64_t>(mixed & 0x7fffffffffffffffULL);
}

[[nodiscard]] double ima4c_train_step(Ima4cPredictor &predictor,
                                      torch::optim::Adam &optimizer,
                                      const Ima4cDeviceBatch &batch,
                                      Ima4cArm arm) {
  predictor->train();
  optimizer.zero_grad();
  const auto prediction = predictor->forward(
      batch.context, batch.context_mask, batch.target_metadata,
      batch.target_slot,
      batch.source_label[static_cast<std::size_t>(arm)]);
  const auto loss = torch::mse_loss(prediction, batch.target);
  const double value = loss.detach().item<double>();
  if (!std::isfinite(value)) {
    throw std::runtime_error("IMA-4C training loss is nonfinite");
  }
  loss.backward();
  double gradient_square = 0.0;
  for (const auto &parameter : predictor->parameters()) {
    if (parameter.grad().defined()) {
      if (!torch::isfinite(parameter.grad()).all().item<bool>()) {
        throw std::runtime_error("IMA-4C gradient is nonfinite");
      }
      gradient_square +=
          parameter.grad().detach().pow(2).sum().item<double>();
    }
  }
  const double gradient_norm = std::sqrt(gradient_square);
  if (!(gradient_norm > 0.0) || !std::isfinite(gradient_norm)) {
    throw std::runtime_error("IMA-4C gradient norm failed");
  }
  const double factor =
      gradient_norm > kIma4cGradientClip
          ? kIma4cGradientClip / std::max(gradient_norm, 1.0e-30)
          : 1.0;
  if (factor < 1.0) {
    for (const auto &parameter : predictor->parameters()) {
      if (parameter.grad().defined()) {
        parameter.grad().mul_(factor);
      }
    }
  }
  optimizer.step();
  if (!ima4c_predictor_finite(predictor, /*require_gradients=*/true)) {
    throw std::runtime_error("IMA-4C predictor finiteness failed");
  }
  return value;
}

[[nodiscard]] torch::Tensor ima4c_predict_split(
    Ima4cPredictor &predictor, const Ima4cFrozenSplit &split,
    const torch::Device &device, Ima4cArm arm) {
  const bool was_training = predictor->is_training();
  predictor->eval();
  torch::NoGradGuard no_grad;
  std::vector<torch::Tensor> predictions;
  for (int64_t begin = 0; begin < split.context.size(0);
       begin += kIma4cGroupBatch) {
    const int64_t size = std::min<int64_t>(
        kIma4cGroupBatch, split.context.size(0) - begin);
    const auto rows = torch::arange(begin, begin + size, torch::kInt64);
    const auto batch = ima4c_device_batch(split, rows, device);
    predictions.push_back(
        predictor
            ->forward(batch.context, batch.context_mask,
                      batch.target_metadata, batch.target_slot,
                      batch.source_label[static_cast<std::size_t>(arm)])
            .detach()
            .to(torch::kCPU, torch::kFloat32)
            .contiguous());
  }
  predictor->train(was_training);
  return torch::cat(predictions, 0)
      .reshape({split.context.size(0), 2, kIma4cLatentDim})
      .contiguous();
}

[[nodiscard]] Ima4aOracleMetric
ima4c_metric(const torch::Tensor &prediction,
             const Ima4cFrozenSplit &split) {
  return ima4a_oracle_metric(
      ima4c_flatten_targets(prediction),
      ima4c_flatten_targets(split.target),
      ima4c_flatten_targets(split.target_slot));
}

[[nodiscard]] torch::Tensor
ima4c_c_features(const Ima4cFrozenSplit &split) {
  const auto context =
      split.context.to(torch::kCPU, torch::kFloat64).masked_fill(
          split.context_mask.logical_not().unsqueeze(-1), 0.0);
  const auto field = torch::cat(
      {context.reshape({context.size(0), -1}),
       split.context_mask.to(torch::kFloat64)},
      1);
  const auto repeated = ima4c_repeated_group_rows(context.size(0));
  return torch::cat(
             {ima4c_flatten_targets(split.target_metadata)
                  .to(torch::kFloat64),
              field.index_select(0, repeated)},
             1)
      .contiguous();
}

[[nodiscard]] Ima4aOracleFit ima4c_reproduce_c(
    const Ima4cFrozenSplit &fit, const Ima4cFrozenSplit &validation,
    const Ima4cFrozenSplit &development) {
  return ima4a_fit_oracle(
      ima4c_c_features(fit), ima4c_flatten_targets(fit.target),
      ima4c_flatten_targets(fit.target_slot),
      ima4c_c_features(validation),
      ima4c_flatten_targets(validation.target),
      ima4c_flatten_targets(validation.target_slot),
      ima4c_c_features(development),
      ima4c_flatten_targets(development.target),
      ima4c_flatten_targets(development.target_slot),
      Ima4aOracleMap::categorical_slot_by_field);
}

[[nodiscard]] double ima4c_max_abs_difference(const torch::Tensor &left,
                                               const torch::Tensor &right) {
  if (left.sizes() != right.sizes()) {
    return std::numeric_limits<double>::infinity();
  }
  return (left.to(torch::kCPU, torch::kFloat64) -
          right.to(torch::kCPU, torch::kFloat64))
      .abs()
      .max()
      .item<double>();
}

struct Ima4cArmTraining {
  Ima4cPredictor predictor{nullptr};
  std::unique_ptr<torch::optim::Adam> optimizer{};
  Ima4cPredictorSnapshot initial{};
  Ima4cPredictorSnapshot selected{};
  std::array<Ima4aOracleMetric, kIma4cValidationSteps.size()> validation{};
  std::size_t selected_validation_index{0};
  int64_t selected_step{0};
  int64_t updates{0};
  double parameter_delta{0.0};
  double output_slot_delta{0.0};
  double target_factor_delta{0.0};
  double source_factor_delta{0.0};
  bool finite{true};
};

struct Ima4cTrainingResult {
  std::array<Ima4cArmTraining, 2> arms{};
  std::array<torch::Tensor, 2> development_prediction{};
  bool initial_equal{false};
  bool production_reproduced{false};
  bool row_schedule_equal{true};
  bool rng_equal{true};
  bool optimizer_layout_equal{false};
  bool compute_censored{false};
  bool pass{false};
};

[[nodiscard]] bool ima4c_adam_complete(const torch::optim::Adam &optimizer,
                                       int64_t expected_step) {
  if (optimizer.state().empty()) {
    return false;
  }
  for (const auto &entry : optimizer.state()) {
    const auto *state =
        dynamic_cast<const torch::optim::AdamParamState *>(entry.second.get());
    if (state == nullptr || state->step() != expected_step ||
        !state->exp_avg().defined() || !state->exp_avg_sq().defined() ||
        !torch::isfinite(state->exp_avg()).all().item<bool>() ||
        !torch::isfinite(state->exp_avg_sq()).all().item<bool>()) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] Ima4cTrainingResult ima4c_train_predictors(
    const std::shared_ptr<mtf::LatentPredictorImpl> &production,
    const mtf::mtf_jepa_mae_vicreg_config_t &config,
    const Ima4cFrozenSplit &fit, const Ima4cFrozenSplit &validation,
    const Ima4cFrozenSplit &development, const torch::Device &device,
    int64_t seed) {
  Ima4cTrainingResult result{};
  for (std::size_t index = 0; index < result.arms.size(); ++index) {
    result.arms[index].predictor = Ima4cPredictor(config);
    ima4c_copy_production_predictor(production,
                                    result.arms[index].predictor);
    result.arms[index].initial =
        ima4c_snapshot_predictor(result.arms[index].predictor);
    result.arms[index].optimizer = std::make_unique<torch::optim::Adam>(
        result.arms[index].predictor->parameters(),
        torch::optim::AdamOptions(kIma4cLearningRate));
  }
  const auto initial_true = ima4c_predict_split(
      result.arms[0].predictor, validation, device,
      Ima4cArm::canonical_source_labels);
  const auto initial_control = ima4c_predict_split(
      result.arms[1].predictor, validation, device,
      Ima4cArm::permuted_source_labels);
  result.initial_equal =
      ima4c_snapshots_exact(result.arms[0].initial,
                            result.arms[1].initial) &&
      rssm_tensor_bytes_equal(initial_true, initial_control);
  result.production_reproduced =
      ima4c_max_abs_difference(initial_true, validation.current) <=
          kIma4cTolerance &&
      ima4c_max_abs_difference(initial_control, validation.current) <=
          kIma4cTolerance;

  for (std::size_t arm = 0; arm < result.arms.size(); ++arm) {
    const auto prediction = ima4c_predict_split(
        result.arms[arm].predictor, validation, device,
        static_cast<Ima4cArm>(arm));
    result.arms[arm].validation[0] =
        ima4c_metric(prediction, validation);
    result.arms[arm].selected = result.arms[arm].initial;
  }

  std::size_t validation_index = 1;
  for (int64_t zero_step = 0; zero_step < kIma4cUpdates; ++zero_step) {
    const auto rows = ima4c_training_groups(
        fit.context.size(0), seed, zero_step);
    const auto row_tensor = torch::tensor(rows, torch::kInt64);
    const auto batch = ima4c_device_batch(fit, row_tensor, device);
    std::array<GeneratorStateSnapshot, 2> before{};
    std::array<GeneratorStateSnapshot, 2> after{};
    for (std::size_t arm = 0; arm < result.arms.size(); ++arm) {
      set_paired_rng(ima4c_dropout_seed(seed, zero_step), device);
      before[arm] = current_generator_state_snapshot(device);
      const double loss = ima4c_train_step(
          result.arms[arm].predictor, *result.arms[arm].optimizer, batch,
          static_cast<Ima4cArm>(arm));
      result.arms[arm].finite =
          result.arms[arm].finite && std::isfinite(loss);
      after[arm] = current_generator_state_snapshot(device);
      ++result.arms[arm].updates;
    }
    result.rng_equal =
        result.rng_equal &&
        generator_state_snapshot_equal(before[0], before[1]) &&
        generator_state_snapshot_equal(after[0], after[1]);

    const int64_t completed = zero_step + 1;
    if (validation_index < kIma4cValidationSteps.size() &&
        completed == kIma4cValidationSteps[validation_index]) {
      for (std::size_t arm = 0; arm < result.arms.size(); ++arm) {
        const auto prediction = ima4c_predict_split(
            result.arms[arm].predictor, validation, device,
            static_cast<Ima4cArm>(arm));
        auto &training = result.arms[arm];
        training.validation[validation_index] =
            ima4c_metric(prediction, validation);
        if (training.validation[validation_index].nmse <
            training.validation[training.selected_validation_index].nmse) {
          training.selected_validation_index = validation_index;
          training.selected_step = completed;
          training.selected =
              ima4c_snapshot_predictor(training.predictor);
        }
      }
      ++validation_index;
    }
  }
  if (validation_index != kIma4cValidationSteps.size()) {
    throw std::runtime_error("IMA-4C validation schedule failed");
  }

  for (std::size_t arm = 0; arm < result.arms.size(); ++arm) {
    auto &training = result.arms[arm];
    const auto final_snapshot =
        ima4c_snapshot_predictor(training.predictor);
    training.parameter_delta = ima4c_snapshot_max_abs_diff(
        final_snapshot, training.initial);
    training.output_slot_delta = ima4c_snapshot_max_abs_diff(
        final_snapshot, training.initial,
        std::string_view("output_slot_embedding.weight"));
    training.target_factor_delta = ima4c_snapshot_max_abs_diff(
        final_snapshot, training.initial,
        std::string_view("target_factor"));
    training.source_factor_delta = ima4c_snapshot_max_abs_diff(
        final_snapshot, training.initial,
        std::string_view("source_factor"));
    training.finite =
        training.finite && training.updates == kIma4cUpdates &&
        training.parameter_delta > 0.0 &&
        training.output_slot_delta > 0.0 &&
        training.target_factor_delta > 0.0 &&
        training.source_factor_delta > 0.0 &&
        ima4c_predictor_finite(training.predictor,
                               /*require_gradients=*/true) &&
        ima4c_adam_complete(*training.optimizer, kIma4cUpdates);
    result.compute_censored =
        result.compute_censored ||
        (training.selected_step == kIma4cUpdates &&
         training.validation.back().r2 -
                 training.validation[kIma4cValidationSteps.size() - 2].r2 >=
             kIma4cComputeCensorDelta);
    ima4c_restore_predictor(training.predictor, training.selected);
    result.development_prediction[arm] = ima4c_predict_split(
        training.predictor, development, device,
        static_cast<Ima4cArm>(arm));
  }
  result.optimizer_layout_equal =
      result.arms[0].optimizer->state().size() ==
          result.arms[1].optimizer->state().size() &&
      result.arms[0].predictor->parameters().size() ==
          result.arms[1].predictor->parameters().size();
  result.pass = result.initial_equal && result.production_reproduced &&
                result.row_schedule_equal && result.rng_equal &&
                result.optimizer_layout_equal && !result.compute_censored &&
                result.arms[0].finite && result.arms[1].finite;
  return result;
}

struct Ima4cSeedEvidence {
  int64_t seed{0};
  Ima4aOracleMetric current{};
  Ima4aOracleMetric sealed_c{};
  std::array<Ima4aOracleMetric, 2> trained{};
  torch::Tensor target{};
  torch::Tensor target_slot{};
  torch::Tensor current_prediction{};
  torch::Tensor c_prediction{};
  std::array<torch::Tensor, 2> trained_prediction{};
  std::array<std::array<Ima4aOracleMetric,
                        kIma4cValidationSteps.size()>,
             2>
      validation{};
  std::array<int64_t, 2> selected_step{};
  std::array<int64_t, 2> predictor_updates{};
  std::array<double, 2> parameter_delta{};
  std::array<double, 2> output_slot_delta{};
  std::array<double, 2> target_factor_delta{};
  std::array<double, 2> source_factor_delta{};
  std::array<std::size_t, 2> selected_validation_index{};
  std::array<std::array<uint64_t, 2>, 3> source_label_hash{};
  std::array<uint64_t, 3> capture_hash{};
  bool initial_equal{false};
  bool production_reproduced{false};
  bool paired_rng{false};
  bool optimizer_layout_equal{false};
  bool custody{false};
  bool component_equivalence{false};
  bool frozen_state_exact{false};
  bool frozen_gradients_clear{false};
  bool captures_exact{false};
  bool source_labels_exact{false};
  bool compute_censored{false};
  bool pass{false};
};

[[nodiscard]] bool ima4c_frozen_gradients_clear(
    const mtf::MtfJepaMaeVicreg &model) {
  const auto parameters = model->named_parameters(/*recurse=*/true);
  return std::all_of(parameters.begin(), parameters.end(),
                     [](const auto &item) {
                       return !item.value().grad().defined();
                     });
}

[[nodiscard]] bool ima4c_settled_evidence_exact() {
  const auto ima4a_log_path = std::filesystem::path(kIma4cIma4aLogPath);
  const auto ima4b_log_path = std::filesystem::path(kIma4cIma4bLogPath);
  if (!std::filesystem::exists(ima4a_log_path) ||
      !std::filesystem::exists(ima4b_log_path)) {
    return false;
  }
  const auto ima4a_log = rmc_read_file(ima4a_log_path);
  const auto ima4b_log = rmc_read_file(ima4b_log_path);
  bool ima4b_protocols_exact = true;
  for (std::size_t index = 0; index < kIma4cIma4bProtocolPaths.size();
       ++index) {
    ima4b_protocols_exact =
        ima4b_protocols_exact &&
        ima4a_sha256(kIma4cIma4bProtocolPaths[index]) ==
            kIma4cIma4bProtocolSha256[index];
  }
  return ima4a_sha256(kIma4cProtocolPath) == kIma4cProtocolSha256 &&
         digest::sha256_hex(ima4a_log) == kIma4cIma4aLogSha256 &&
         ima4a_log.find("ima4a.mechanics_pass=true") != std::string::npos &&
         ima4a_log.find("ima4a.decision="
                        "categorical_target_slot_interaction_bottleneck") !=
             std::string::npos &&
         ima4a_log.find("execution_status=ima4a_measurements_complete") !=
             std::string::npos &&
         ima4a_sha256(kIma4aSourcePath) == kIma4cIma4aSourceSha256 &&
         ima4a_sha256(kIma4aProtocolPath) == kIma4cIma4aProtocolSha256 &&
         ima3_sha256(kIma3SourcePath) == kIma4cIma3SourceSha256 &&
         ima3_sha256(kIma3HeaderPath) == kIma4cHeaderSha256 &&
         digest::sha256_hex(ima4b_log) == kIma4cIma4bLogSha256 &&
         ima4b_log.find(
             "schema=wikimyei.mtf_jepa_mae_vicreg.ima4b.audit.v1a2") !=
             std::string::npos &&
         ima4b_log.find("ima4b.compute_censored=false") !=
             std::string::npos &&
         ima4b_log.find("ima4b.mechanics_pass=true") !=
             std::string::npos &&
         ima4b_log.find("ima4b.seed_17.pass=true") !=
             std::string::npos &&
         ima4b_log.find("ima4b.seed_31.pass=true") !=
             std::string::npos &&
         ima4b_log.find("ima4b.seed_47.pass=true") !=
             std::string::npos &&
         ima4b_log.find("ima4b.decision="
                        "narrow_target_relative_predictor_insufficient") !=
             std::string::npos &&
         ima4b_log.find("ima4b.representation_optimizer_steps=0") !=
             std::string::npos &&
         ima4b_log.find("ima4b.ema_updates=0") != std::string::npos &&
         ima4b_log.find("execution_status=ima4b_measurements_complete") !=
             std::string::npos &&
         ima4a_sha256(kIma4cIma4bSourcePath) ==
             kIma4cIma4bSourceSha256 &&
         ima4b_protocols_exact;
}

[[nodiscard]] Ima4cSeedEvidence ima4c_run_seed(
    const RmcData &data, const torch::Device &device,
    std::size_t seed_index) {
  Ima4cSeedEvidence result{};
  result.seed = kAttributionSeeds.at(seed_index);
  DefaultGeneratorStateGuard rng_guard(device);
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_config_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  set_paired_rng(result.seed, device);
  auto model =
      mtf::MtfJepaMaeVicreg(ima3_config(device, kIma4aPolicies[0]));
  const bool anchor_loaded = oca_load_archive(
      oca_archive_path(result.seed), model, device, result.seed,
      anchor_config_hash);
  const bool cache_valid = ima4a_validate_ima3_seed_cache(
      data.ssl, device, result.seed, seed_index);
  result.custody = anchor_loaded && cache_valid &&
                   ima3_pins_exact(seed_index) &&
                   ima3_preflight_receipt_exact();
  if (!result.custody) {
    throw std::runtime_error("IMA-4C frozen custody failed");
  }
  model->eval();
  result.component_equivalence =
      ima4a_component_equivalence(model, data.ssl, device, result.seed);
  if (!result.component_equivalence) {
    throw std::runtime_error("IMA-4C component equivalence failed");
  }
  const auto frozen_before = oca_snapshot_state(model);
  const auto modules = ima4a_modules(model);
  auto fit = ima4c_capture_split(modules, data.probe_train,
                                 model->config(), device, result.seed);
  auto validation = ima4c_capture_split(
      modules, data.probe_validation, model->config(), device, result.seed);
  auto development = ima4c_capture_split(
      modules, data.development, model->config(), device, result.seed);
  const std::array<uint64_t, 3> capture_hashes{
      fit.hash, validation.hash, development.hash};
  result.capture_hash = capture_hashes;
  const std::array<const Ima4cFrozenSplit *, 3> frozen_splits{
      &fit, &validation, &development};
  for (std::size_t split = 0; split < frozen_splits.size(); ++split) {
    result.source_label_hash[split] =
        frozen_splits[split]->source_label_arm_hash;
  }

  // Training and validation selection complete before any development metric
  // is scored.  The captured development tensors remain locked until this
  // call returns both independently selected predictors.
  auto training = ima4c_train_predictors(
      modules.predictor, model->config(), fit, validation, development,
      device, result.seed);
  result.current = ima4c_metric(development.current, development);
  if (std::abs(result.current.r2 - kIma4cSealedCurrentR2[seed_index]) >
      kIma4cTolerance) {
    throw std::runtime_error("IMA-4C current score reproduction failed");
  }
  const auto c = ima4c_reproduce_c(fit, validation, development);
  result.sealed_c = c.metric;
  if (c.edge_improving ||
      std::abs(result.sealed_c.r2 - kIma4cSealedCR2[seed_index]) >
          kIma4cTolerance) {
    throw std::runtime_error("IMA-4C sealed C reproduction failed");
  }
  for (std::size_t arm = 0; arm < 2; ++arm) {
    result.trained_prediction[arm] =
        ima4c_flatten_targets(training.development_prediction[arm]);
    result.trained[arm] =
        ima4c_metric(training.development_prediction[arm], development);
    result.validation[arm] = training.arms[arm].validation;
    result.selected_step[arm] = training.arms[arm].selected_step;
    result.predictor_updates[arm] = training.arms[arm].updates;
    result.parameter_delta[arm] = training.arms[arm].parameter_delta;
    result.output_slot_delta[arm] = training.arms[arm].output_slot_delta;
    result.target_factor_delta[arm] = training.arms[arm].target_factor_delta;
    result.source_factor_delta[arm] = training.arms[arm].source_factor_delta;
    result.selected_validation_index[arm] =
        training.arms[arm].selected_validation_index;
  }
  result.initial_equal = training.initial_equal;
  result.production_reproduced = training.production_reproduced;
  result.paired_rng = training.rng_equal;
  result.optimizer_layout_equal = training.optimizer_layout_equal;
  result.target = ima4c_flatten_targets(development.target);
  result.target_slot = ima4c_flatten_targets(development.target_slot);
  result.current_prediction =
      ima4c_flatten_targets(development.current);
  result.c_prediction = c.prediction;
  result.compute_censored = training.compute_censored;
  result.captures_exact =
      capture_hashes[0] == ima4c_capture_hash(fit) &&
      capture_hashes[1] == ima4c_capture_hash(validation) &&
      capture_hashes[2] == ima4c_capture_hash(development);
  result.source_labels_exact = true;
  for (std::size_t split = 0; split < frozen_splits.size(); ++split) {
    const auto &frozen = *frozen_splits[split];
    result.source_labels_exact =
        result.source_labels_exact &&
        frozen.source_label_hash ==
            ima4c_source_label_hash(frozen.source_label);
    for (std::size_t arm = 0; arm < frozen.source_label.size(); ++arm) {
      result.source_labels_exact =
          result.source_labels_exact &&
          frozen.source_label_arm_hash[arm] ==
              hash_tensor_stable_bytes(frozen.source_label[arm]);
    }
  }
  result.frozen_state_exact = oca_state_exact(model, frozen_before);
  result.frozen_gradients_clear = ima4c_frozen_gradients_clear(model);
  bool metrics_finite = std::isfinite(result.current.r2) &&
                        std::isfinite(result.sealed_c.r2);
  for (const auto &metric : result.trained) {
    metrics_finite = metrics_finite && std::isfinite(metric.r2) &&
                     std::isfinite(metric.nmse);
  }
  result.pass = result.custody && result.component_equivalence &&
                result.frozen_state_exact &&
                result.frozen_gradients_clear && result.captures_exact &&
                result.source_labels_exact &&
                training.pass && metrics_finite;
  rng_guard.restore();
  return result;
}

struct Ima4cBootstrap {
  Interval treatment_minus_control{};
  Interval c_minus_treatment{};
  Interval c_minus_control{};
  double c_minus_treatment_upper95{0.0};
  double c_minus_control_upper95{0.0};
};

[[nodiscard]] double ima4c_quantile(std::vector<double> values,
                                    double probability) {
  if (values.empty() || probability < 0.0 || probability > 1.0 ||
      !std::all_of(values.begin(), values.end(),
                   [](double value) { return std::isfinite(value); })) {
    throw std::runtime_error("IMA-4C quantile contract failed");
  }
  std::sort(values.begin(), values.end());
  const double position =
      probability * static_cast<double>(values.size() - 1);
  const auto lower = static_cast<std::size_t>(std::floor(position));
  const auto upper = static_cast<std::size_t>(std::ceil(position));
  const double fraction = position - static_cast<double>(lower);
  return values[lower] + fraction * (values[upper] - values[lower]);
}

[[nodiscard]] Ima4cBootstrap ima4c_bootstrap(
    const std::array<Ima4cSeedEvidence, 3> &evidence) {
  const int64_t groups =
      evidence[0].target.size(0) / kIma4cTargetsPerGroup;
  const auto rows = rmc_bootstrap_rows(groups);
  std::vector<double> treatment_control;
  std::vector<double> c_treatment;
  std::vector<double> c_control;
  treatment_control.reserve(rows.size());
  c_treatment.reserve(rows.size());
  c_control.reserve(rows.size());
  for (const auto &group_rows : rows) {
    double tc = 0.0;
    double ct = 0.0;
    double cc = 0.0;
    for (const auto &seed : evidence) {
      const auto target = ima4a_group_resample(seed.target, group_rows);
      const auto identity =
          ima4a_group_resample(seed.target_slot, group_rows);
      const auto treatment = ima4a_oracle_metric(
          ima4a_group_resample(seed.trained_prediction[0], group_rows),
          target, identity);
      const auto control = ima4a_oracle_metric(
          ima4a_group_resample(seed.trained_prediction[1], group_rows),
          target, identity);
      const auto c = ima4a_oracle_metric(
          ima4a_group_resample(seed.c_prediction, group_rows), target,
          identity);
      tc += treatment.r2 - control.r2;
      ct += c.r2 - treatment.r2;
      cc += c.r2 - control.r2;
    }
    treatment_control.push_back(tc / evidence.size());
    c_treatment.push_back(ct / evidence.size());
    c_control.push_back(cc / evidence.size());
  }
  return {
      .treatment_minus_control =
          percentile_interval(treatment_control),
      .c_minus_treatment = percentile_interval(c_treatment),
      .c_minus_control = percentile_interval(c_control),
      .c_minus_treatment_upper95 =
          ima4c_quantile(std::move(c_treatment), 0.95),
      .c_minus_control_upper95 =
          ima4c_quantile(std::move(c_control), 0.95),
  };
}

[[nodiscard]] double ima4c_mean(const std::array<double, 3> &values) {
  return std::accumulate(values.begin(), values.end(), 0.0) /
         static_cast<double>(values.size());
}

[[nodiscard]] bool ima4c_all_positive(const std::array<double, 3> &values) {
  return std::all_of(values.begin(), values.end(),
                     [](double value) { return value > 0.0; });
}

void ima4c_emit_leave_one_out(const std::string &name,
                              const std::array<double, 3> &values) {
  for (std::size_t omitted = 0; omitted < values.size(); ++omitted) {
    double total = 0.0;
    for (std::size_t index = 0; index < values.size(); ++index) {
      if (index != omitted) {
        total += values[index];
      }
    }
    std::cout << "ima4c.summary." << name << ".leave_seed_"
              << kAttributionSeeds[omitted] << "_out=" << total / 2.0
              << '\n';
  }
}

struct Ima4cSelfTestResult {
  bool permutation_fixed{false};
  bool permutation_deranged{false};
  bool partition_multisets_preserved{false};
  bool targets_share_map{false};
  bool capture_hashes_stable{false};
  bool zero_init_exact{false};
  bool zero_init_parity{false};
  bool production_attention_parity{false};
  bool canonical_interaction_learns{false};
  bool gradient_reach{false};
  bool matched_mechanics{false};
  bool row_schedule_stable{false};
  bool validation_selection_exact{false};
  bool snapshot_restore_exact{false};
  bool optimizer_state_exact{false};
  bool finite{false};
  double interaction_initial_loss{0.0};
  double interaction_treatment_heldout_loss{0.0};
  double interaction_control_heldout_loss{0.0};
  bool pass{false};
};

[[nodiscard]] double ima4c_gradient_max_abs(const torch::Tensor &value) {
  if (!value.grad().defined()) {
    return 0.0;
  }
  return value.grad().detach().abs().max().item<double>();
}

[[nodiscard]] Ima4cSelfTestResult ima4c_self_test() {
  Ima4cSelfTestResult result{};
  const torch::Device device(torch::kCPU);
  DefaultGeneratorStateGuard guard(device);
  auto config = ima3_config(
      device, mtf::mtf_jepa_mask_policy_t::support_separated_pair_v1);
  config.dropout = 0.0;

  constexpr int64_t groups = 8;
  auto group_mask = torch::zeros({groups, kIma4cSlotCount}, torch::kBool);
  std::vector<int64_t> target_slots;
  target_slots.reserve(
      static_cast<std::size_t>(groups * kIma4cTargetsPerGroup));
  for (int64_t group = 0; group < groups; ++group) {
    const int64_t offset = (group * 7) % kIma4cSlotCount;
    for (int64_t index = 0; index < kIma4aContextCount; ++index) {
      group_mask.index_put_({group, (offset + index) % kIma4cSlotCount},
                            true);
    }
    target_slots.push_back((offset + kIma4aContextCount) %
                           kIma4cSlotCount);
    target_slots.push_back((offset + kIma4aContextCount + 1) %
                           kIma4cSlotCount);
  }
  const auto group_id =
      torch::arange(810000, 810000 + groups, torch::kInt64);
  const auto maps = ima4c_source_label_maps(group_mask, group_id, 410041);
  const auto maps_repeat =
      ima4c_source_label_maps(group_mask, group_id, 410041);
  const auto maps_other_groups = ima4c_source_label_maps(
      group_mask, group_id + 100000, 410041);
  const uint64_t mask_hash_before = hash_tensor_stable_bytes(group_mask);
  const uint64_t permutation_hash = hash_tensor_stable_bytes(maps[1]);
  const uint64_t repeated_hash = hash_tensor_stable_bytes(maps_repeat[1]);
  result.permutation_fixed =
      rssm_tensor_bytes_equal(maps[1], maps_repeat[1]) &&
      permutation_hash == repeated_hash &&
      !rssm_tensor_bytes_equal(maps[1], maps_other_groups[1]);
  result.capture_hashes_stable =
      mask_hash_before == hash_tensor_stable_bytes(group_mask);

  const auto mask_access = group_mask.accessor<bool, 2>();
  const auto map_access = maps[1].accessor<int64_t, 2>();
  bool deranged = true;
  bool partitions = true;
  for (int64_t group = 0; group < groups; ++group) {
    std::array<bool, kIma4cSlotCount> seen{};
    std::array<int64_t, 2> physical_counts{};
    std::array<int64_t, 2> label_counts{};
    for (int64_t slot = 0; slot < kIma4cSlotCount; ++slot) {
      const int64_t label = map_access[group][slot];
      deranged = deranged && label != slot && label >= 0 &&
                  label < kIma4cSlotCount &&
                  !seen[static_cast<std::size_t>(label)];
      if (label >= 0 && label < kIma4cSlotCount) {
        seen[static_cast<std::size_t>(label)] = true;
        partitions = partitions &&
                     mask_access[group][slot] == mask_access[group][label];
        ++label_counts[mask_access[group][label] ? 1 : 0];
      }
      ++physical_counts[mask_access[group][slot] ? 1 : 0];
    }
    partitions = partitions && physical_counts == label_counts;
  }
  result.permutation_deranged = deranged;
  result.partition_multisets_preserved = partitions;

  const auto repeated_rows = ima4c_repeated_group_rows(groups);
  const auto canonical_labels = maps[0].index_select(0, repeated_rows);
  const auto permuted_labels = maps[1].index_select(0, repeated_rows);
  const auto paired_maps = permuted_labels.reshape(
      {groups, kIma4cTargetsPerGroup, kIma4cSlotCount});
  result.targets_share_map = torch::equal(
      paired_maps.select(1, 0), paired_maps.select(1, 1));

  set_paired_rng(410041, device);
  Ima4cPredictor treatment(config);
  set_paired_rng(410041, device);
  Ima4cPredictor control(config);
  const auto treatment_initial = ima4c_snapshot_predictor(treatment);
  const auto control_initial = ima4c_snapshot_predictor(control);
  const auto expected_source = ima4c_initial_source_factor();
  result.zero_init_exact =
      treatment->target_factor().eq(0).all().item<bool>() &&
      control->target_factor().eq(0).all().item<bool>() &&
      treatment->output_slot_weight().eq(0).all().item<bool>() &&
      control->output_slot_weight().eq(0).all().item<bool>() &&
      rssm_tensor_bytes_equal(treatment->source_factor(), expected_source) &&
      rssm_tensor_bytes_equal(control->source_factor(), expected_source);
  result.matched_mechanics =
      ima4c_snapshots_exact(treatment_initial, control_initial) &&
      ima4c_parameter_count(treatment) == kIma4cTrainableParameters &&
      ima4c_parameter_count(control) == kIma4cTrainableParameters;

  set_paired_rng(410042, device);
  const auto group_context =
      torch::randn({groups, kIma4cSlotCount, kIma4cLatentDim},
                   torch::kFloat32);
  const auto context = group_context.index_select(0, repeated_rows);
  const auto mask = group_mask.index_select(0, repeated_rows);
  const auto metadata = torch::randn(
      {groups * kIma4cTargetsPerGroup, kIma4aMetadataWidth},
      torch::kFloat32);
  const auto slot = torch::tensor(target_slots, torch::kInt64);
  uint64_t retained_hash = 0xcbf29ce484222325ULL;
  for (const auto &value : {context, mask, metadata, slot,
                            canonical_labels, permuted_labels}) {
    mix_hash_value(retained_hash, hash_tensor_stable_bytes(value));
  }
  treatment->eval();
  control->eval();
  torch::Tensor treatment_zero{};
  torch::Tensor control_zero{};
  torch::Tensor teacher_output{};
  {
    torch::NoGradGuard no_grad;
    treatment_zero = treatment->forward(
        context, mask, metadata, slot, canonical_labels);
    control_zero = control->forward(
        context, mask, metadata, slot, permuted_labels);
    const auto zero_bias = ima4c_pair_bias(
        treatment->target_factor(), treatment->source_factor(), slot,
        canonical_labels);
    result.zero_init_parity =
        rssm_tensor_bytes_equal(treatment_zero, control_zero) &&
        zero_bias.eq(0).all().item<bool>();

    const auto q = torch::randn(
        {groups * kIma4cTargetsPerGroup, 1, kIma4cLatentDim},
        torch::kFloat32);
    const auto k = torch::randn_like(context);
    const auto v = torch::randn_like(context);
    const auto no_bias = torch::zeros(
        {q.size(0), kIma4cHeadCount, 1, kIma4cSlotCount},
        torch::kFloat32);
    const auto local = ima4c_context_attention(
        q, k, v, mask, no_bias, kIma4cHeadCount);
    const auto production = mtf::detail::multi_head_context_attention(
        q, k, v, mask, kIma4cHeadCount);
    result.production_attention_parity =
        torch::allclose(local, production, 0.0, 0.0);
  }

  // Train a genuinely matched synthetic A/B.  The same two target slots recur
  // in every group, so neither arm can memorize a target slot per group.  Both
  // U and V are optimized independently in each arm with identical full-batch
  // work.  Held-out groups receive unseen fixed derangements, while treatment
  // retains the canonical source identity shared by train and validation.
  const auto teacher_u = expected_source * 0.75;
  constexpr int64_t interaction_train_groups = 48;
  constexpr int64_t interaction_validation_groups = 24;
  const auto make_interaction_mask = [](int64_t count) {
    auto value = torch::zeros(
        {count, kIma4cSlotCount}, torch::kBool);
    value.index_put_({Slice(), Slice(0, kIma4aContextCount)}, true);
    return value;
  };
  const auto make_interaction_slots = [](int64_t count) {
    std::vector<int64_t> values;
    values.reserve(static_cast<std::size_t>(
        count * kIma4cTargetsPerGroup));
    for (int64_t group = 0; group < count; ++group) {
      values.push_back(60);
      values.push_back(61);
    }
    return torch::tensor(values, torch::kInt64);
  };
  const auto interaction_train_mask =
      make_interaction_mask(interaction_train_groups);
  const auto interaction_validation_mask =
      make_interaction_mask(interaction_validation_groups);
  const auto interaction_train_ids = torch::arange(
      820000, 820000 + interaction_train_groups, torch::kInt64);
  const auto interaction_validation_ids = torch::arange(
      920000, 920000 + interaction_validation_groups, torch::kInt64);
  const auto interaction_train_maps = ima4c_source_label_maps(
      interaction_train_mask, interaction_train_ids, 410041);
  const auto interaction_validation_maps = ima4c_source_label_maps(
      interaction_validation_mask, interaction_validation_ids, 410041);
  const auto interaction_train_rows =
      ima4c_repeated_group_rows(interaction_train_groups);
  const auto interaction_validation_rows =
      ima4c_repeated_group_rows(interaction_validation_groups);
  const auto interaction_train_slots =
      make_interaction_slots(interaction_train_groups);
  const auto interaction_validation_slots =
      make_interaction_slots(interaction_validation_groups);
  const std::array<torch::Tensor, 2> interaction_train_labels{
      interaction_train_maps[0].index_select(0, interaction_train_rows),
      interaction_train_maps[1].index_select(0, interaction_train_rows)};
  const std::array<torch::Tensor, 2> interaction_validation_labels{
      interaction_validation_maps[0].index_select(
          0, interaction_validation_rows),
      interaction_validation_maps[1].index_select(
          0, interaction_validation_rows)};
  const auto interaction_train_row_mask =
      interaction_train_mask.index_select(0, interaction_train_rows);
  const auto interaction_validation_row_mask =
      interaction_validation_mask.index_select(
          0, interaction_validation_rows);
  const auto masked_pair_loss = [](const torch::Tensor &prediction,
                                   const torch::Tensor &target,
                                   const torch::Tensor &physical_mask) {
    const auto weight = physical_mask.to(prediction.scalar_type())
                            .unsqueeze(1)
                            .unsqueeze(2);
    return ((prediction - target).pow(2) * weight).sum() /
           (weight.sum() * prediction.size(1));
  };

  auto learner_u = torch::zeros_like(teacher_u);
  auto learner_v = expected_source.clone();
  auto control_u = torch::zeros_like(teacher_u);
  auto control_v = expected_source.clone();
  learner_u.set_requires_grad(true);
  learner_v.set_requires_grad(true);
  control_u.set_requires_grad(true);
  control_v.set_requires_grad(true);
  torch::optim::Adam interaction_treatment_optimizer(
      std::vector<torch::Tensor>{learner_u, learner_v},
      torch::optim::AdamOptions(0.03));
  torch::optim::Adam interaction_control_optimizer(
      std::vector<torch::Tensor>{control_u, control_v},
      torch::optim::AdamOptions(0.03));
  torch::Tensor teacher_train_bias{};
  torch::Tensor teacher_validation_bias{};
  double initial_interaction_loss = 0.0;
  {
    torch::NoGradGuard no_grad;
    teacher_train_bias = ima4c_pair_bias(
        teacher_u, expected_source, interaction_train_slots,
        interaction_train_labels[0]).detach();
    teacher_validation_bias = ima4c_pair_bias(
        teacher_u, expected_source, interaction_validation_slots,
        interaction_validation_labels[0]).detach();
    initial_interaction_loss = masked_pair_loss(
        ima4c_pair_bias(learner_u, learner_v, interaction_train_slots,
                        interaction_train_labels[0]),
        teacher_train_bias, interaction_train_row_mask).item<double>();
  }
  for (int64_t step = 0; step < 256; ++step) {
    interaction_treatment_optimizer.zero_grad();
    const auto treatment_bias = ima4c_pair_bias(
        learner_u, learner_v, interaction_train_slots,
        interaction_train_labels[0]);
    const auto treatment_loss = masked_pair_loss(
        treatment_bias, teacher_train_bias, interaction_train_row_mask);
    treatment_loss.backward();
    interaction_treatment_optimizer.step();

    interaction_control_optimizer.zero_grad();
    const auto control_bias = ima4c_pair_bias(
        control_u, control_v, interaction_train_slots,
        interaction_train_labels[1]);
    const auto control_loss = masked_pair_loss(
        control_bias, teacher_train_bias, interaction_train_row_mask);
    control_loss.backward();
    interaction_control_optimizer.step();
  }
  double canonical_loss = 0.0;
  double permuted_loss = 0.0;
  {
    torch::NoGradGuard no_grad;
    canonical_loss = masked_pair_loss(
        ima4c_pair_bias(learner_u, learner_v,
                        interaction_validation_slots,
                        interaction_validation_labels[0]),
        teacher_validation_bias,
        interaction_validation_row_mask).item<double>();
    permuted_loss = masked_pair_loss(
        ima4c_pair_bias(control_u, control_v,
                        interaction_validation_slots,
                        interaction_validation_labels[1]),
        teacher_validation_bias,
        interaction_validation_row_mask).item<double>();
  }
  result.interaction_initial_loss = initial_interaction_loss;
  result.interaction_treatment_heldout_loss = canonical_loss;
  result.interaction_control_heldout_loss = permuted_loss;
  result.canonical_interaction_learns =
      initial_interaction_loss > 0.0 &&
      canonical_loss < 0.05 * initial_interaction_loss &&
      canonical_loss < 0.2 * permuted_loss &&
      (learner_v.detach() - expected_source).abs().max().item<double>() >
          0.0 &&
      control_u.detach().abs().max().item<double>() > 0.0 &&
      (control_v.detach() - expected_source).abs().max().item<double>() >
          0.0;

  // In the integrated predictor, U and the output embedding must receive a
  // gradient immediately.  V correctly receives an exact zero gradient while
  // U=0, then becomes reachable after the first U update.
  set_paired_rng(410041, device);
  Ima4cPredictor teacher(config);
  {
    torch::NoGradGuard no_grad;
    teacher->target_factor().copy_(teacher_u);
    teacher_output = teacher->forward(
        context, mask, metadata, slot, canonical_labels).detach();
  }
  const auto source_before =
      treatment->source_factor().detach().clone();
  const auto target_factor_before =
      treatment->target_factor().detach().clone();
  torch::optim::Adam gradient_optimizer(
      treatment->parameters(), torch::optim::AdamOptions(1.0e-3));
  gradient_optimizer.zero_grad();
  const auto first_prediction = treatment->forward(
      context, mask, metadata, slot, canonical_labels);
  const auto first_loss = torch::mse_loss(first_prediction, teacher_output);
  first_loss.backward();
  const double first_u_gradient =
      ima4c_gradient_max_abs(treatment->target_factor());
  const double first_v_gradient =
      ima4c_gradient_max_abs(treatment->source_factor());
  const double first_output_gradient =
      ima4c_gradient_max_abs(treatment->output_slot_weight());
  gradient_optimizer.step();
  gradient_optimizer.zero_grad();
  const auto second_prediction = treatment->forward(
      context, mask, metadata, slot, canonical_labels);
  const auto second_loss = torch::mse_loss(second_prediction, teacher_output);
  second_loss.backward();
  const double second_v_gradient =
      ima4c_gradient_max_abs(treatment->source_factor());
  gradient_optimizer.step();
  const double source_delta =
      (treatment->source_factor().detach() - source_before)
          .abs()
          .max()
          .item<double>();
  const double target_factor_delta =
      (treatment->target_factor().detach() - target_factor_before)
          .abs()
          .max()
          .item<double>();
  result.gradient_reach =
      first_u_gradient > 0.0 && first_output_gradient > 0.0 &&
      first_v_gradient == 0.0 && second_v_gradient > 0.0 &&
      source_delta > 0.0 && target_factor_delta > 0.0;
  const auto schedule_a = ima4c_training_groups(256, 410041, 127);
  const auto schedule_b = ima4c_training_groups(256, 410041, 127);
  result.row_schedule_stable =
      schedule_a == schedule_b &&
      hash_batch_rows(schedule_a) == hash_batch_rows(schedule_b) &&
      ima4c_dropout_seed(410041, 127) ==
          ima4c_dropout_seed(410041, 127);
  std::array<double, 3> synthetic_validation_nmse{1.0, 0.5, 0.5};
  std::size_t synthetic_selected_index = 0;
  for (std::size_t index = 1; index < synthetic_validation_nmse.size();
       ++index) {
    if (synthetic_validation_nmse[index] <
        synthetic_validation_nmse[synthetic_selected_index]) {
      synthetic_selected_index = index;
    }
  }
  result.validation_selection_exact =
      synthetic_selected_index == 1 && kIma4cValidationSteps.front() == 0 &&
      kIma4cValidationSteps.back() == kIma4cUpdates &&
      std::is_sorted(kIma4cValidationSteps.begin(),
                     kIma4cValidationSteps.end());
  const auto selected_snapshot = ima4c_snapshot_predictor(treatment);
  {
    torch::NoGradGuard no_grad;
    treatment->target_factor().add_(1.0);
  }
  ima4c_restore_predictor(treatment, selected_snapshot);
  result.snapshot_restore_exact = ima4c_snapshots_exact(
      selected_snapshot, ima4c_snapshot_predictor(treatment));
  result.optimizer_state_exact =
      ima4c_adam_complete(gradient_optimizer, /*expected_step=*/2);
  uint64_t retained_hash_after = 0xcbf29ce484222325ULL;
  for (const auto &value : {context, mask, metadata, slot,
                            canonical_labels, permuted_labels}) {
    mix_hash_value(retained_hash_after, hash_tensor_stable_bytes(value));
  }
  result.capture_hashes_stable =
      result.capture_hashes_stable && retained_hash == retained_hash_after;
  result.finite =
      torch::isfinite(treatment_zero).all().item<bool>() &&
      torch::isfinite(control_zero).all().item<bool>() &&
      torch::isfinite(learner_u).all().item<bool>() &&
      torch::isfinite(learner_v).all().item<bool>() &&
      torch::isfinite(control_u).all().item<bool>() &&
      torch::isfinite(control_v).all().item<bool>() &&
      ima4c_predictor_finite(treatment, /*require_gradients=*/true) &&
      std::isfinite(initial_interaction_loss) &&
      std::isfinite(canonical_loss) && std::isfinite(permuted_loss);
  result.pass =
      result.permutation_fixed && result.permutation_deranged &&
      result.partition_multisets_preserved && result.targets_share_map &&
      result.capture_hashes_stable && result.zero_init_exact &&
      result.zero_init_parity && result.production_attention_parity &&
      result.canonical_interaction_learns && result.gradient_reach &&
      result.matched_mechanics && result.row_schedule_stable &&
      result.validation_selection_exact && result.snapshot_restore_exact &&
      result.optimizer_state_exact && result.finite;
  guard.restore();
  return result;
}

[[nodiscard]] int run_ima4c_self_test() {
  std::cout << std::boolalpha << std::setprecision(12);
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.ima4c.self_test.v1\n";
  std::cout << "experiment=canonical-slot-interaction-sufficiency-self-test\n";
  const auto self_test = ima4c_self_test();
  std::cout << "ima4c.self_test.permutation_fixed="
            << self_test.permutation_fixed << '\n';
  std::cout << "ima4c.self_test.permutation_deranged="
            << self_test.permutation_deranged << '\n';
  std::cout << "ima4c.self_test.partition_multisets_preserved="
            << self_test.partition_multisets_preserved << '\n';
  std::cout << "ima4c.self_test.targets_share_map="
            << self_test.targets_share_map << '\n';
  std::cout << "ima4c.self_test.capture_hashes_stable="
            << self_test.capture_hashes_stable << '\n';
  std::cout << "ima4c.self_test.zero_init_exact="
            << self_test.zero_init_exact << '\n';
  std::cout << "ima4c.self_test.zero_init_parity="
            << self_test.zero_init_parity << '\n';
  std::cout << "ima4c.self_test.production_attention_parity="
            << self_test.production_attention_parity << '\n';
  std::cout << "ima4c.self_test.canonical_interaction_learns="
            << self_test.canonical_interaction_learns << '\n';
  std::cout << "ima4c.self_test.interaction_initial_loss="
            << self_test.interaction_initial_loss << '\n';
  std::cout << "ima4c.self_test.interaction_treatment_heldout_loss="
            << self_test.interaction_treatment_heldout_loss << '\n';
  std::cout << "ima4c.self_test.interaction_control_heldout_loss="
            << self_test.interaction_control_heldout_loss << '\n';
  std::cout << "ima4c.self_test.gradient_reach="
            << self_test.gradient_reach << '\n';
  std::cout << "ima4c.self_test.matched_mechanics="
            << self_test.matched_mechanics << '\n';
  std::cout << "ima4c.self_test.row_schedule_stable="
            << self_test.row_schedule_stable << '\n';
  std::cout << "ima4c.self_test.validation_selection_exact="
            << self_test.validation_selection_exact << '\n';
  std::cout << "ima4c.self_test.snapshot_restore_exact="
            << self_test.snapshot_restore_exact << '\n';
  std::cout << "ima4c.self_test.optimizer_state_exact="
            << self_test.optimizer_state_exact << '\n';
  std::cout << "ima4c.self_test.finite=" << self_test.finite << '\n';
  std::cout << "ima4c.self_test.pass=" << self_test.pass << '\n';
  std::cout << "ima4c.representation_optimizer_steps=0\n";
  std::cout << "ima4c.ema_updates=0\n";
  std::cout << "execution_status=ima4c_self_test_complete\n";
  return self_test.pass ? 0 : 3;
}

[[nodiscard]] int run_ima4c_audit(const Options &options) {
  if (options.device != "cuda" || !torch::cuda::is_available()) {
    throw std::runtime_error("IMA-4C authoritative audit requires CUDA");
  }
  const char *workspace = std::getenv("CUBLAS_WORKSPACE_CONFIG");
  if (workspace == nullptr || std::string(workspace) != ":4096:8") {
    throw std::runtime_error(
        "IMA-4C requires CUBLAS_WORKSPACE_CONFIG=:4096:8");
  }
  const torch::Device device(torch::kCUDA, 0);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);
  DefaultGeneratorStateGuard audit_rng(device);
  const bool settled = ima4c_settled_evidence_exact();
  if (!settled) {
    throw std::runtime_error("IMA-4C settled IMA-4A/IMA-4B evidence failed");
  }
  const auto self_test_result = ima4c_self_test();
  const bool self_test = self_test_result.pass;
  if (!self_test) {
    throw std::runtime_error("IMA-4C synthetic interaction self-test failed");
  }
  auto data = rmc_make_data();
  const bool splits_exact = data.probe_train.data.size(0) == 256 &&
                            data.probe_validation.data.size(0) == 128 &&
                            data.development.data.size(0) == 256 &&
                            data.probe_train.group_begin == 1000000 &&
                            data.probe_validation.group_begin == 2000000 &&
                            data.development.group_begin == 3000000;
  if (!splits_exact) {
    throw std::runtime_error("IMA-4C group split custody failed");
  }
  std::array<Ima4cSeedEvidence, 3> evidence{};
  for (std::size_t seed = 0; seed < evidence.size(); ++seed) {
    evidence[seed] = ima4c_run_seed(data, device, seed);
  }
  const auto bootstrap = ima4c_bootstrap(evidence);

  std::array<double, 3> current{};
  std::array<double, 3> control{};
  std::array<double, 3> treatment{};
  std::array<double, 3> c{};
  std::array<double, 3> treatment_control{};
  std::array<double, 3> c_treatment{};
  std::array<double, 3> c_control{};
  bool mechanics = settled && self_test && splits_exact;
  bool compute_censored = false;
  for (std::size_t seed = 0; seed < evidence.size(); ++seed) {
    const auto &item = evidence[seed];
    current[seed] = item.current.r2;
    treatment[seed] = item.trained[0].r2;
    control[seed] = item.trained[1].r2;
    c[seed] = item.sealed_c.r2;
    treatment_control[seed] = treatment[seed] - control[seed];
    c_treatment[seed] = c[seed] - treatment[seed];
    c_control[seed] = c[seed] - control[seed];
    mechanics = mechanics && item.pass;
    compute_censored = compute_censored || item.compute_censored;
  }
  mechanics = mechanics && !compute_censored;

  const double treatment_control_mean = ima4c_mean(treatment_control);
  const double c_treatment_mean = ima4c_mean(c_treatment);
  const double c_control_mean = ima4c_mean(c_control);
  const bool treatment_reaches_c =
      ima4c_all_positive(treatment) && c_treatment_mean <= 0.01 &&
      bootstrap.c_minus_treatment_upper95 < 0.02;
  const bool control_reaches_c =
      ima4c_all_positive(control) && c_control_mean <= 0.01 &&
      bootstrap.c_minus_control_upper95 < 0.02;
  const bool treatment_material =
      treatment_control_mean >= 0.02 &&
      ima4c_all_positive(treatment_control) &&
      bootstrap.treatment_minus_control.low > 0.0;
  std::array<double, 3> treatment_c{};
  for (std::size_t index = 0; index < treatment_c.size(); ++index) {
    treatment_c[index] = -c_treatment[index];
  }
  const double treatment_c_mean = ima4c_mean(treatment_c);
  const bool treatment_materially_exceeds_c =
      treatment_c_mean >= 0.02 &&
      ima4c_all_positive(treatment_c) &&
      -bootstrap.c_minus_treatment.high > 0.0;

  std::string decision = "invalid_mechanics";
  std::string utility_candidate = "none";
  if (compute_censored) {
    decision = "compute_censored";
  } else if (mechanics) {
    if (treatment_materially_exceeds_c) {
      decision = "linear_C_underestimated_predictability";
      utility_candidate = "canonical_source_labels";
    } else if (treatment_reaches_c && treatment_material) {
      decision = "canonical_pair_interaction_sufficient";
      utility_candidate = "canonical_source_labels";
    } else if (treatment_reaches_c && !treatment_material) {
      decision = "predictor_sufficient_pair_interaction_not_confirmed";
      utility_candidate = "canonical_source_labels";
    } else if (!treatment_reaches_c && control_reaches_c) {
      decision = "canonical_identity_unnecessary_mechanism_unresolved";
    } else if (treatment_material) {
      decision = "canonical_pair_interaction_helpful_but_insufficient";
    } else {
      decision = "predictor_capacity_exhausted_teacher_redesign";
    }
  }

  std::cout << std::boolalpha << std::setprecision(12);
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.ima4c.audit.v1\n";
  std::cout << "experiment=canonical-slot-interaction-sufficiency-audit\n";
  std::cout << "device=cuda\n";
  std::cout << "ima4c.protocol_sha256=" << ima4a_sha256(kIma4cProtocolPath)
            << '\n';
  std::cout << "ima4c.source_sha256=" << ima4a_sha256(kIma4cSourcePath)
            << '\n';
  std::cout << "ima4c.ima4a_log_sha256=" << kIma4cIma4aLogSha256 << '\n';
  std::cout << "ima4c.ima4b_log_sha256="
            << ima4a_sha256(kIma4cIma4bLogPath) << '\n';
  std::cout << "ima4c.ima4b_source_sha256="
            << ima4a_sha256(kIma4cIma4bSourcePath) << '\n';
  for (std::size_t index = 0; index < kIma4cIma4bProtocolPaths.size();
       ++index) {
    std::cout << "ima4c.ima4b_protocol_" << index << "_sha256="
              << ima4a_sha256(kIma4cIma4bProtocolPaths[index]) << '\n';
  }
  std::cout << "ima4c.self_test=" << self_test << '\n';
  std::cout << "ima4c.permutation_key_scope=experiment_seed_absolute_group_id"
               "_protocol_constant\n";
  std::cout << "ima4c.permutation_epoch_invariant=true\n";
  std::cout << "ima4c.permutation_split_role_invariant=true\n";
  std::cout << "ima4c.source_maps_precomputed=true\n";
  std::cout << "ima4c.fit_groups=" << data.probe_train.data.size(0) << '\n';
  std::cout << "ima4c.validation_groups="
            << data.probe_validation.data.size(0) << '\n';
  std::cout << "ima4c.development_groups="
            << data.development.data.size(0) << '\n';
  std::cout << "ima4c.parameters_per_arm="
            << kIma4cTrainableParameters << '\n';
  std::cout << "ima4c.predictor_updates_per_arm=" << kIma4cUpdates << '\n';
  constexpr std::array<std::string_view, 3> split_names{
      "fit", "validation", "development"};
  for (const auto &seed : evidence) {
    const std::string root =
        "ima4c.seed_" + std::to_string(seed.seed);
    std::cout << root << ".custody=" << seed.custody << '\n';
    std::cout << root << ".component_equivalence="
              << seed.component_equivalence << '\n';
    std::cout << root << ".frozen_state_exact="
              << seed.frozen_state_exact << '\n';
    std::cout << root << ".frozen_gradients_clear="
              << seed.frozen_gradients_clear << '\n';
    std::cout << root << ".captures_exact=" << seed.captures_exact << '\n';
    std::cout << root << ".source_labels_exact="
              << seed.source_labels_exact << '\n';
    for (std::size_t split = 0; split < split_names.size(); ++split) {
      std::cout << root << '.' << split_names[split] << ".capture_hash="
                << oca_hex_u64(seed.capture_hash[split]) << '\n';
      for (std::size_t arm = 0; arm < kIma4cArmNames.size(); ++arm) {
        std::cout << root << '.' << split_names[split] << '.'
                  << kIma4cArmNames[arm] << ".source_label_hash="
                  << oca_hex_u64(seed.source_label_hash[split][arm])
                  << '\n';
      }
    }
    std::cout << root << ".initial_equal=" << seed.initial_equal << '\n';
    std::cout << root << ".production_reproduced="
              << seed.production_reproduced << '\n';
    std::cout << root << ".paired_rng=" << seed.paired_rng << '\n';
    std::cout << root << ".optimizer_layout_equal="
              << seed.optimizer_layout_equal << '\n';
    std::cout << root << ".current.r2=" << seed.current.r2 << '\n';
    std::cout << root << ".sealed_C.r2=" << seed.sealed_c.r2 << '\n';
    for (std::size_t arm = 0; arm < 2; ++arm) {
      std::cout << root << '.' << kIma4cArmNames[arm]
                << ".r2=" << seed.trained[arm].r2 << '\n';
      std::cout << root << '.' << kIma4cArmNames[arm]
                << ".selected_step=" << seed.selected_step[arm] << '\n';
      std::cout << root << '.' << kIma4cArmNames[arm]
                << ".selected_validation_index="
                << seed.selected_validation_index[arm] << '\n';
      std::cout << root << '.' << kIma4cArmNames[arm]
                << ".updates=" << seed.predictor_updates[arm] << '\n';
      std::cout << root << '.' << kIma4cArmNames[arm]
                << ".parameter_delta=" << seed.parameter_delta[arm]
                << '\n';
      std::cout << root << '.' << kIma4cArmNames[arm]
                << ".output_slot_delta=" << seed.output_slot_delta[arm]
                << '\n';
      std::cout << root << '.' << kIma4cArmNames[arm]
                << ".target_factor_delta=" << seed.target_factor_delta[arm]
                << '\n';
      std::cout << root << '.' << kIma4cArmNames[arm]
                << ".source_factor_delta=" << seed.source_factor_delta[arm]
                << '\n';
      for (std::size_t checkpoint = 0;
           checkpoint < kIma4cValidationSteps.size(); ++checkpoint) {
        std::cout << root << '.' << kIma4cArmNames[arm]
                  << ".validation_step_"
                  << kIma4cValidationSteps[checkpoint]
                  << ".r2=" << seed.validation[arm][checkpoint].r2 << '\n';
      }
    }
    std::cout << root << ".treatment_minus_control="
              << seed.trained[0].r2 - seed.trained[1].r2 << '\n';
    std::cout << root << ".C_minus_treatment="
              << seed.sealed_c.r2 - seed.trained[0].r2 << '\n';
    std::cout << root << ".C_minus_control="
              << seed.sealed_c.r2 - seed.trained[1].r2 << '\n';
    std::cout << root << ".treatment_minus_C="
              << seed.trained[0].r2 - seed.sealed_c.r2 << '\n';
    std::cout << root << ".treatment_minus_control_positive="
              << (seed.trained[0].r2 - seed.trained[1].r2 > 0.0) << '\n';
    std::cout << root << ".C_minus_treatment_positive="
              << (seed.sealed_c.r2 - seed.trained[0].r2 > 0.0) << '\n';
    std::cout << root << ".C_minus_control_positive="
              << (seed.sealed_c.r2 - seed.trained[1].r2 > 0.0) << '\n';
    std::cout << root << ".treatment_minus_C_positive="
              << (seed.trained[0].r2 - seed.sealed_c.r2 > 0.0) << '\n';
    std::cout << root << ".compute_censored="
              << seed.compute_censored << '\n';
    std::cout << root << ".pass=" << seed.pass << '\n';
  }
  std::cout << "ima4c.summary.current_r2=" << ima4c_mean(current) << '\n';
  std::cout << "ima4c.summary.control_r2=" << ima4c_mean(control) << '\n';
  std::cout << "ima4c.summary.treatment_r2=" << ima4c_mean(treatment)
            << '\n';
  std::cout << "ima4c.summary.sealed_C_r2=" << ima4c_mean(c) << '\n';
  std::cout << "ima4c.summary.treatment_minus_control.point="
            << treatment_control_mean << '\n';
  std::cout << "ima4c.summary.treatment_minus_control.low="
            << bootstrap.treatment_minus_control.low << '\n';
  std::cout << "ima4c.summary.treatment_minus_control.high="
            << bootstrap.treatment_minus_control.high << '\n';
  std::cout << "ima4c.summary.C_minus_treatment.point="
            << c_treatment_mean << '\n';
  std::cout << "ima4c.summary.C_minus_treatment.low="
            << bootstrap.c_minus_treatment.low << '\n';
  std::cout << "ima4c.summary.C_minus_treatment.high="
            << bootstrap.c_minus_treatment.high << '\n';
  std::cout << "ima4c.summary.C_minus_treatment.upper95="
            << bootstrap.c_minus_treatment_upper95 << '\n';
  std::cout << "ima4c.summary.C_minus_control.point="
            << c_control_mean << '\n';
  std::cout << "ima4c.summary.C_minus_control.low="
            << bootstrap.c_minus_control.low << '\n';
  std::cout << "ima4c.summary.C_minus_control.high="
            << bootstrap.c_minus_control.high << '\n';
  std::cout << "ima4c.summary.C_minus_control.upper95="
            << bootstrap.c_minus_control_upper95 << '\n';
  std::cout << "ima4c.summary.treatment_minus_C.point="
            << treatment_c_mean << '\n';
  std::cout << "ima4c.summary.treatment_minus_C.low="
            << -bootstrap.c_minus_treatment.high << '\n';
  std::cout << "ima4c.summary.treatment_minus_C.high="
            << -bootstrap.c_minus_treatment.low << '\n';
  ima4c_emit_leave_one_out("treatment_minus_control", treatment_control);
  ima4c_emit_leave_one_out("C_minus_treatment", c_treatment);
  ima4c_emit_leave_one_out("C_minus_control", c_control);
  ima4c_emit_leave_one_out("treatment_minus_C", treatment_c);
  std::cout << "ima4c.summary.treatment_minus_control.all_seed_positive="
            << ima4c_all_positive(treatment_control) << '\n';
  std::cout << "ima4c.summary.C_minus_treatment.all_seed_positive="
            << ima4c_all_positive(c_treatment) << '\n';
  std::cout << "ima4c.summary.C_minus_control.all_seed_positive="
            << ima4c_all_positive(c_control) << '\n';
  std::cout << "ima4c.summary.treatment_minus_C.all_seed_positive="
            << ima4c_all_positive(treatment_c) << '\n';
  std::cout << "ima4c.treatment_reaches_C=" << treatment_reaches_c << '\n';
  std::cout << "ima4c.control_reaches_C=" << control_reaches_c << '\n';
  std::cout << "ima4c.treatment_material=" << treatment_material << '\n';
  std::cout << "ima4c.treatment_materially_exceeds_C="
            << treatment_materially_exceeds_c << '\n';
  std::cout << "ima4c.compute_censored=" << compute_censored << '\n';
  std::cout << "ima4c.mechanics_pass=" << mechanics << '\n';
  std::cout << "ima4c.decision=" << decision << '\n';
  std::cout << "ima4c.utility_candidate=" << utility_candidate << '\n';
  std::cout << "ima4c.representation_optimizer_steps=0\n";
  std::cout << "ima4c.ema_updates=0\n";
  std::cout << "execution_status=ima4c_measurements_complete\n";
  audit_rng.restore();
  return mechanics ? 0 : 3;
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment ==
        "canonical-slot-interaction-sufficiency-self-test") {
      return run_ima4c_self_test();
    }
    if (options.experiment ==
        "canonical-slot-interaction-sufficiency-audit") {
      return run_ima4c_audit(options);
    }
    throw std::runtime_error(
        "--experiment must be canonical-slot-interaction-sufficiency-"
        "self-test or canonical-slot-interaction-sufficiency-audit");
  } catch (const c10::Error &error) {
    std::cerr << "canonical_slot_interaction_sufficiency_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "canonical_slot_interaction_sufficiency_error="
              << error.what() << '\n';
  }
  return 2;
}
