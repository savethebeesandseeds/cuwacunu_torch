#include "piaabo/digest/sha256.h"

// IMA-4B consumes the same sealed FSPA-4/OCA anchor and fixed RMC probes as
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
    throw std::runtime_error("IMA-4B IMA-3 cache checksum failed");
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
    throw std::runtime_error("IMA-4B IMA-3 cache metadata failed");
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
      throw std::runtime_error("IMA-4B IMA-3 cache arm identity failed");
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
    throw std::runtime_error("IMA-4B IMA-3 cache receipt failed");
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
        throw std::runtime_error("IMA-4B child type mismatch: " + name);
      }
      return value;
    }
  }
  throw std::runtime_error("IMA-4B child missing: " + name);
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
    throw std::runtime_error("IMA-4B invalid target-centered metric");
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
        throw std::runtime_error("IMA-4B categorical field is empty");
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
          throw std::runtime_error("IMA-4B categorical identity out of range");
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
                "IMA-4B categorical validation identity unseen in fit");
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
        throw std::runtime_error("IMA-4B dual ridge factorization failed");
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
    throw std::runtime_error("IMA-4B group-paired mask contract failed");
  }
  return result;
}

[[nodiscard]] torch::Tensor ima4a_group_resample(
    const torch::Tensor &value, const torch::Tensor &group_rows) {
  if (value.size(0) % kIma4aTargetsPerGroup != 0) {
    throw std::runtime_error("IMA-4B group resample shape failed");
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

constexpr int64_t kIma4bUpdates = 1024;
constexpr int64_t kIma4bGroupBatch = 32;
constexpr int64_t kIma4bTargetsPerGroup = 2;
constexpr int64_t kIma4bSlotCount = 72;
constexpr int64_t kIma4bLatentDim = 32;
constexpr int64_t kIma4bCommonParameters = 11744;
constexpr int64_t kIma4bSlotParameters =
    kIma4bSlotCount * kIma4bLatentDim;
constexpr int64_t kIma4bTrainableParameters =
    kIma4bCommonParameters + kIma4bSlotParameters;
constexpr double kIma4bLearningRate = 1.0e-3;
constexpr double kIma4bGradientClip = 5.0;
constexpr double kIma4bTolerance = 2.0e-6;
constexpr double kIma4bComputeCensorDelta = 0.005;
constexpr std::array<int64_t, 13> kIma4bValidationSteps{
    0,   64,  128, 192, 256, 320, 384,
    448, 512, 640, 768, 896, 1024};
constexpr std::array<double, 3> kIma4bSealedCurrentR2{
    -1.40016785751, -1.5562935319, -1.28212443802};
constexpr std::array<double, 3> kIma4bSealedCR2{
    0.0739973874835, 0.0421979052359, 0.00858663293665};
constexpr std::string_view kIma4bProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "JEPA_TARGET_RELATIVE_PREDICTOR_SUFFICIENCY_PROTOCOL.md";
constexpr std::string_view kIma4bProtocolAmendmentPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "JEPA_TARGET_RELATIVE_PREDICTOR_SUFFICIENCY_PROTOCOL_AMENDMENT_A1.md";
constexpr std::string_view kIma4bProtocolAmendmentA2Path =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "JEPA_TARGET_RELATIVE_PREDICTOR_SUFFICIENCY_PROTOCOL_AMENDMENT_A2.md";
constexpr std::string_view kIma4bSourcePath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_"
    "target_relative_predictor_sufficiency.cpp";
constexpr std::string_view kIma4bIma4aLogPath =
    ".build/tests/representation_ima4a_v1_authoritative.log";
constexpr std::string_view kIma4bIma4aLogSha256 =
    "eebd1e59167aad75bdfce69f5176ee4f4e040400181133e5f8f66dca3afe7606";
constexpr std::string_view kIma4bIma4aSourceSha256 =
    "d41011207cde4f5a780c6ff96a77a59aef6c2086932874087db4d8388565867c";
constexpr std::string_view kIma4bIma4aProtocolSha256 =
    "23fc3d516bfca285dda9ac901efe89acc12b29032ddcabf50020bfb0ebf77af1";
constexpr std::string_view kIma4bIma3SourceSha256 =
    "97c096b5331dcf83cea4c23067dc2806ec09d03d8d9f19614c86595028196c16";
constexpr std::string_view kIma4bHeaderSha256 =
    "93640972e497dc49f37e7690e59c2f2e55f12ece25687fe4e6f6c96b28c3c9ea";

enum class Ima4bArm : std::size_t {
  true_slot_attention = 0,
  slot_bias_control = 1,
};

constexpr std::array<std::string_view, 2> kIma4bArmNames{
    "true_slot_attention", "slot_bias_control"};

[[nodiscard]] double ima4b_rho(Ima4bArm arm) {
  return arm == Ima4bArm::true_slot_attention ? 1.0 : 0.0;
}

class Ima4bPredictorImpl : public torch::nn::Module {
public:
  Ima4bPredictorImpl(mtf::mtf_jepa_mae_vicreg_config_t config, double rho)
      : config_(std::move(config)), rho_(rho) {
    if (rho_ != 0.0 && rho_ != 1.0) {
      throw std::runtime_error("IMA-4B rho must be binary");
    }
    mtf::detail::validate_config(config_);
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
    slot_embedding_ = register_module(
        "slot_embedding",
        torch::nn::Embedding(
            torch::nn::EmbeddingOptions(kIma4bSlotCount, config_.latent_dim)));
    this->to(config_.device, config_.dtype);
    torch::NoGradGuard no_grad;
    slot_embedding_->weight.zero_();
  }

  [[nodiscard]] torch::Tensor
  forward(const torch::Tensor &context_latents,
          const torch::Tensor &context_mask,
          const torch::Tensor &target_metadata,
          const torch::Tensor &target_slot) {
    if (context_latents.dim() != 3 || context_mask.dim() != 2 ||
        target_metadata.dim() != 2 || target_metadata.size(1) != 6 ||
        target_slot.dim() != 1 ||
        context_latents.size(0) != target_metadata.size(0) ||
        context_latents.size(0) != target_slot.size(0) ||
        context_latents.size(1) != kIma4bSlotCount ||
        context_latents.size(2) != config_.latent_dim ||
        context_mask.size(0) != context_latents.size(0) ||
        context_mask.size(1) != context_latents.size(1)) {
      throw std::runtime_error("IMA-4B predictor input contract failed");
    }
    if (target_slot.min().item<int64_t>() < 0 ||
        target_slot.max().item<int64_t>() >= kIma4bSlotCount) {
      throw std::runtime_error("IMA-4B target slot is out of range");
    }
    const auto options = torch::TensorOptions()
                             .dtype(config_.dtype)
                             .device(config_.device);
    const auto q0 = q_projection_->forward(metadata_projection_->forward(
        target_metadata.to(options)));
    const auto slot = slot_embedding_->forward(
        target_slot.to(config_.device, torch::kInt64));
    const auto q_attention = q0 + rho_ * slot;
    const auto keys = k_projection_->forward(context_latents.to(options));
    const auto values = v_projection_->forward(context_latents.to(options));
    const auto mask = context_mask.to(config_.device, torch::kBool);
    auto attended = mtf::detail::multi_head_context_attention(
                        q_attention.unsqueeze(1), keys, values, mask,
                        config_.num_heads)
                        .squeeze(1);
    auto hidden = attended + q0;
    for (auto &layer : layers_) {
      hidden = dropout_->forward(torch::gelu(layer->forward(hidden)));
    }
    return out_->forward(hidden) + (1.0 - rho_) * slot;
  }

  [[nodiscard]] double rho() const { return rho_; }

private:
  mtf::mtf_jepa_mae_vicreg_config_t config_{};
  double rho_{0.0};
  torch::nn::Linear metadata_projection_{nullptr};
  torch::nn::Linear q_projection_{nullptr};
  torch::nn::Linear k_projection_{nullptr};
  torch::nn::Linear v_projection_{nullptr};
  std::vector<torch::nn::Linear> layers_{};
  torch::nn::Linear out_{nullptr};
  torch::nn::Dropout dropout_{nullptr};
  torch::nn::Embedding slot_embedding_{nullptr};
};

TORCH_MODULE(Ima4bPredictor);

struct Ima4bPredictorSnapshot {
  std::vector<std::string> names{};
  std::vector<torch::Tensor> values{};
};

[[nodiscard]] bool
ima4b_snapshots_exact(const Ima4bPredictorSnapshot &left,
                      const Ima4bPredictorSnapshot &right) {
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

[[nodiscard]] Ima4bPredictorSnapshot
ima4b_snapshot_predictor(const Ima4bPredictor &predictor) {
  Ima4bPredictorSnapshot result{};
  torch::NoGradGuard no_grad;
  for (const auto &item : predictor->named_parameters(/*recurse=*/true)) {
    result.names.push_back(item.key());
    result.values.push_back(
        item.value().detach().to(torch::kCPU).contiguous().clone());
  }
  return result;
}

void ima4b_restore_predictor(Ima4bPredictor &predictor,
                             const Ima4bPredictorSnapshot &snapshot) {
  const auto parameters = predictor->named_parameters(/*recurse=*/true);
  if (parameters.size() != snapshot.values.size()) {
    throw std::runtime_error("IMA-4B predictor restore count failed");
  }
  torch::NoGradGuard no_grad;
  std::size_t index = 0;
  for (const auto &item : parameters) {
    if (item.key() != snapshot.names[index] ||
        item.value().sizes() != snapshot.values[index].sizes() ||
        item.value().scalar_type() != snapshot.values[index].scalar_type()) {
      throw std::runtime_error("IMA-4B predictor restore layout failed");
    }
    item.value().copy_(snapshot.values[index].to(item.value().device()));
    ++index;
  }
}

[[nodiscard]] int64_t
ima4b_parameter_count(const Ima4bPredictor &predictor) {
  int64_t result = 0;
  for (const auto &parameter : predictor->parameters()) {
    result += parameter.numel();
  }
  return result;
}

[[nodiscard]] double ima4b_predictor_max_abs_diff(
    const Ima4bPredictor &predictor,
    const Ima4bPredictorSnapshot &reference) {
  const auto parameters = predictor->named_parameters(/*recurse=*/true);
  if (parameters.size() != reference.values.size()) {
    throw std::runtime_error("IMA-4B predictor comparison count failed");
  }
  double maximum = 0.0;
  std::size_t index = 0;
  torch::NoGradGuard no_grad;
  for (const auto &item : parameters) {
    if (item.key() != reference.names[index] ||
        item.value().sizes() != reference.values[index].sizes()) {
      throw std::runtime_error("IMA-4B predictor comparison layout failed");
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
ima4b_predictor_finite(const Ima4bPredictor &predictor,
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

void ima4b_copy_production_predictor(
    const std::shared_ptr<mtf::LatentPredictorImpl> &source,
    Ima4bPredictor &destination) {
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
        throw std::runtime_error("IMA-4B common predictor layout failed");
      }
      destination_item.value().copy_(source_item.value());
      copied += source_item.value().numel();
      found = true;
      break;
    }
    if (!found) {
      throw std::runtime_error("IMA-4B common predictor parameter missing");
    }
  }
  for (const auto &item : destination_parameters) {
    if (item.key() == "slot_embedding.weight") {
      item.value().zero_();
    }
  }
  if (copied != kIma4bCommonParameters ||
      ima4b_parameter_count(destination) != kIma4bTrainableParameters) {
    throw std::runtime_error("IMA-4B predictor parameter count failed");
  }
}

struct Ima4bFrozenSplit {
  torch::Tensor context{};         // [G,72,32], CPU float32
  torch::Tensor context_mask{};    // [G,72], CPU bool
  torch::Tensor target_metadata{}; // [G,2,6], CPU float32
  torch::Tensor target_slot{};     // [G,2], CPU int64
  torch::Tensor target{};          // [G,2,32], CPU float32
  torch::Tensor current{};         // [G,2,32], CPU float32
  torch::Tensor group_id{};        // [G], CPU int64
  uint64_t hash{0};
  bool pairing_exact{false};
  bool finite{false};
};

[[nodiscard]] uint64_t ima4b_capture_hash(const Ima4bFrozenSplit &split) {
  uint64_t result = 0xcbf29ce484222325ULL;
  for (const auto &value :
       {split.context, split.context_mask, split.target_metadata,
        split.target_slot, split.target, split.current, split.group_id}) {
    mix_hash_value(result, hash_tensor_stable_bytes(value));
  }
  return result;
}

[[nodiscard]] torch::Tensor ima4b_flatten_targets(const torch::Tensor &value) {
  auto shape = value.sizes().vec();
  if (shape.size() < 2 || shape[1] != kIma4bTargetsPerGroup) {
    throw std::runtime_error("IMA-4B target flatten contract failed");
  }
  shape.erase(shape.begin() + 1);
  shape[0] *= kIma4bTargetsPerGroup;
  return value.reshape(shape).contiguous();
}

[[nodiscard]] torch::Tensor ima4b_repeated_group_rows(int64_t groups) {
  std::vector<int64_t> rows;
  rows.reserve(static_cast<std::size_t>(groups * kIma4bTargetsPerGroup));
  for (int64_t group = 0; group < groups; ++group) {
    for (int64_t target = 0; target < kIma4bTargetsPerGroup; ++target) {
      rows.push_back(group);
    }
  }
  return torch::tensor(rows, torch::kInt64);
}

[[nodiscard]] Ima4bFrozenSplit ima4b_capture_split(
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
        online.tokens.size(1) != kIma4bSlotCount) {
      throw std::runtime_error("IMA-4B online/teacher layout failed");
    }
    const auto pair = ima4a_group_paired_masks(
        online, seed, dataset.group_begin + begin, config, device);
    pairing = pairing && pair.target_exact && pair.counts_exact &&
              pair.rng_exact;
    const auto &masks = pair.masks[1];
    const auto locations = masks.target_mask.nonzero();
    if (locations.sizes() !=
        torch::IntArrayRef({size * kIma4bTargetsPerGroup, 2})) {
      throw std::runtime_error("IMA-4B selected-target shape failed");
    }
    const auto target_rows = locations.select(1, 0).contiguous();
    const auto target_indices = locations.select(1, 1).contiguous();
    const auto expected_rows =
        ima4b_repeated_group_rows(size).to(device, torch::kInt64);
    if (!torch::equal(target_rows, expected_rows)) {
      throw std::runtime_error("IMA-4B selected-target grouping failed");
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
    targets.push_back(selected_target.reshape({size, 2, kIma4bLatentDim})
                          .detach()
                          .to(torch::kCPU, torch::kFloat32)
                          .contiguous()
                          .clone());
    currents.push_back(
        selected_prediction.reshape({size, 2, kIma4bLatentDim})
            .detach()
            .to(torch::kCPU, torch::kFloat32)
            .contiguous()
            .clone());
    group_ids.push_back(
        torch::arange(dataset.group_begin + begin,
                      dataset.group_begin + begin + size, torch::kInt64));
  }

  Ima4bFrozenSplit result{};
  result.context = torch::cat(contexts, 0).contiguous();
  result.context_mask = torch::cat(context_masks, 0).contiguous();
  result.target_metadata = torch::cat(target_metadata, 0).contiguous();
  result.target_slot = torch::cat(target_slots, 0).contiguous();
  result.target = torch::cat(targets, 0).contiguous();
  result.current = torch::cat(currents, 0).contiguous();
  result.group_id = torch::cat(group_ids, 0).contiguous();
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
              {dataset.data.size(0), kIma4bSlotCount, kIma4bLatentDim}) &&
      result.context_mask.sizes() ==
          torch::IntArrayRef({dataset.data.size(0), kIma4bSlotCount}) &&
      result.target.sizes() ==
          torch::IntArrayRef({dataset.data.size(0), 2, kIma4bLatentDim}) &&
      result.target_metadata.sizes() ==
          torch::IntArrayRef({dataset.data.size(0), 2, 6}) &&
      result.target_slot.sizes() ==
          torch::IntArrayRef({dataset.data.size(0), 2}) &&
      torch::equal(result.group_id, expected_groups) &&
      result.context_mask.sum(1).eq(kIma4aContextCount).all().item<bool>();
  if (!shape || !result.pairing_exact || !result.finite) {
    throw std::runtime_error("IMA-4B frozen capture contract failed");
  }
  result.hash = ima4b_capture_hash(result);
  return result;
}

struct Ima4bDeviceBatch {
  torch::Tensor context{};
  torch::Tensor context_mask{};
  torch::Tensor target_metadata{};
  torch::Tensor target_slot{};
  torch::Tensor target{};
};

[[nodiscard]] Ima4bDeviceBatch ima4b_device_batch(
    const Ima4bFrozenSplit &split, const torch::Tensor &group_rows,
    const torch::Device &device) {
  const auto rows = group_rows.to(torch::kCPU, torch::kInt64).contiguous();
  const auto repeated = ima4b_repeated_group_rows(rows.size(0));
  const auto group_context = split.context.index_select(0, rows);
  const auto group_mask = split.context_mask.index_select(0, rows);
  return {
      .context =
          group_context.index_select(0, repeated).to(device, torch::kFloat32),
      .context_mask =
          group_mask.index_select(0, repeated).to(device, torch::kBool),
      .target_metadata =
          ima4b_flatten_targets(split.target_metadata.index_select(0, rows))
              .to(device, torch::kFloat32),
      .target_slot =
          ima4b_flatten_targets(split.target_slot.index_select(0, rows))
              .to(device, torch::kInt64),
      .target = ima4b_flatten_targets(split.target.index_select(0, rows))
                    .to(device, torch::kFloat32),
  };
}

[[nodiscard]] std::vector<int64_t>
ima4b_training_groups(int64_t groups, int64_t seed, int64_t step) {
  if (groups <= 0 || groups % kIma4bGroupBatch != 0 || step < 0) {
    throw std::runtime_error("IMA-4B training schedule contract failed");
  }
  const int64_t batches_per_epoch = groups / kIma4bGroupBatch;
  const int64_t epoch = step / batches_per_epoch;
  const int64_t start = (step % batches_per_epoch) * kIma4bGroupBatch;
  const auto order = epoch_permutation(groups, seed, epoch);
  return {order.begin() + start, order.begin() + start + kIma4bGroupBatch};
}

[[nodiscard]] int64_t ima4b_dropout_seed(int64_t seed, int64_t step) {
  const auto mixed = splitmix64(
      0x696d6134625f6472ULL ^ static_cast<uint64_t>(seed) ^
      (static_cast<uint64_t>(step) << 32U));
  return static_cast<int64_t>(mixed & 0x7fffffffffffffffULL);
}

[[nodiscard]] double ima4b_train_step(Ima4bPredictor &predictor,
                                      torch::optim::Adam &optimizer,
                                      const Ima4bDeviceBatch &batch) {
  predictor->train();
  optimizer.zero_grad();
  const auto prediction = predictor->forward(
      batch.context, batch.context_mask, batch.target_metadata,
      batch.target_slot);
  const auto loss = torch::mse_loss(prediction, batch.target);
  const double value = loss.detach().item<double>();
  if (!std::isfinite(value)) {
    throw std::runtime_error("IMA-4B training loss is nonfinite");
  }
  loss.backward();
  double gradient_square = 0.0;
  for (const auto &parameter : predictor->parameters()) {
    if (parameter.grad().defined()) {
      if (!torch::isfinite(parameter.grad()).all().item<bool>()) {
        throw std::runtime_error("IMA-4B gradient is nonfinite");
      }
      gradient_square +=
          parameter.grad().detach().pow(2).sum().item<double>();
    }
  }
  const double gradient_norm = std::sqrt(gradient_square);
  if (!(gradient_norm > 0.0) || !std::isfinite(gradient_norm)) {
    throw std::runtime_error("IMA-4B gradient norm failed");
  }
  const double factor =
      gradient_norm > kIma4bGradientClip
          ? kIma4bGradientClip / std::max(gradient_norm, 1.0e-30)
          : 1.0;
  if (factor < 1.0) {
    for (const auto &parameter : predictor->parameters()) {
      if (parameter.grad().defined()) {
        parameter.grad().mul_(factor);
      }
    }
  }
  optimizer.step();
  if (!ima4b_predictor_finite(predictor, /*require_gradients=*/true)) {
    throw std::runtime_error("IMA-4B predictor finiteness failed");
  }
  return value;
}

[[nodiscard]] torch::Tensor ima4b_predict_split(
    Ima4bPredictor &predictor, const Ima4bFrozenSplit &split,
    const torch::Device &device) {
  const bool was_training = predictor->is_training();
  predictor->eval();
  torch::NoGradGuard no_grad;
  std::vector<torch::Tensor> predictions;
  for (int64_t begin = 0; begin < split.context.size(0);
       begin += kIma4bGroupBatch) {
    const int64_t size = std::min<int64_t>(
        kIma4bGroupBatch, split.context.size(0) - begin);
    const auto rows = torch::arange(begin, begin + size, torch::kInt64);
    const auto batch = ima4b_device_batch(split, rows, device);
    predictions.push_back(
        predictor
            ->forward(batch.context, batch.context_mask,
                      batch.target_metadata, batch.target_slot)
            .detach()
            .to(torch::kCPU, torch::kFloat32)
            .contiguous());
  }
  predictor->train(was_training);
  return torch::cat(predictions, 0)
      .reshape({split.context.size(0), 2, kIma4bLatentDim})
      .contiguous();
}

[[nodiscard]] Ima4aOracleMetric
ima4b_metric(const torch::Tensor &prediction,
             const Ima4bFrozenSplit &split) {
  return ima4a_oracle_metric(
      ima4b_flatten_targets(prediction),
      ima4b_flatten_targets(split.target),
      ima4b_flatten_targets(split.target_slot));
}

[[nodiscard]] torch::Tensor
ima4b_c_features(const Ima4bFrozenSplit &split) {
  const auto context =
      split.context.to(torch::kCPU, torch::kFloat64).masked_fill(
          split.context_mask.logical_not().unsqueeze(-1), 0.0);
  const auto field = torch::cat(
      {context.reshape({context.size(0), -1}),
       split.context_mask.to(torch::kFloat64)},
      1);
  const auto repeated = ima4b_repeated_group_rows(context.size(0));
  return torch::cat(
             {ima4b_flatten_targets(split.target_metadata)
                  .to(torch::kFloat64),
              field.index_select(0, repeated)},
             1)
      .contiguous();
}

[[nodiscard]] Ima4aOracleFit ima4b_reproduce_c(
    const Ima4bFrozenSplit &fit, const Ima4bFrozenSplit &validation,
    const Ima4bFrozenSplit &development) {
  return ima4a_fit_oracle(
      ima4b_c_features(fit), ima4b_flatten_targets(fit.target),
      ima4b_flatten_targets(fit.target_slot),
      ima4b_c_features(validation),
      ima4b_flatten_targets(validation.target),
      ima4b_flatten_targets(validation.target_slot),
      ima4b_c_features(development),
      ima4b_flatten_targets(development.target),
      ima4b_flatten_targets(development.target_slot),
      Ima4aOracleMap::categorical_slot_by_field);
}

[[nodiscard]] double ima4b_max_abs_difference(const torch::Tensor &left,
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

struct Ima4bArmTraining {
  Ima4bPredictor predictor{nullptr};
  std::unique_ptr<torch::optim::Adam> optimizer{};
  Ima4bPredictorSnapshot initial{};
  Ima4bPredictorSnapshot selected{};
  std::array<Ima4aOracleMetric, kIma4bValidationSteps.size()> validation{};
  std::size_t selected_validation_index{0};
  int64_t selected_step{0};
  int64_t updates{0};
  double parameter_delta{0.0};
  bool finite{true};
};

struct Ima4bTrainingResult {
  std::array<Ima4bArmTraining, 2> arms{};
  std::array<torch::Tensor, 2> development_prediction{};
  bool initial_equal{false};
  bool production_reproduced{false};
  bool row_schedule_equal{true};
  bool rng_equal{true};
  bool optimizer_layout_equal{false};
  bool compute_censored{false};
  bool pass{false};
};

[[nodiscard]] bool ima4b_adam_complete(const torch::optim::Adam &optimizer,
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

[[nodiscard]] Ima4bTrainingResult ima4b_train_predictors(
    const std::shared_ptr<mtf::LatentPredictorImpl> &production,
    const mtf::mtf_jepa_mae_vicreg_config_t &config,
    const Ima4bFrozenSplit &fit, const Ima4bFrozenSplit &validation,
    const Ima4bFrozenSplit &development, const torch::Device &device,
    int64_t seed) {
  Ima4bTrainingResult result{};
  for (std::size_t index = 0; index < result.arms.size(); ++index) {
    const auto arm = static_cast<Ima4bArm>(index);
    result.arms[index].predictor =
        Ima4bPredictor(config, ima4b_rho(arm));
    ima4b_copy_production_predictor(production,
                                    result.arms[index].predictor);
    result.arms[index].initial =
        ima4b_snapshot_predictor(result.arms[index].predictor);
    result.arms[index].optimizer = std::make_unique<torch::optim::Adam>(
        result.arms[index].predictor->parameters(),
        torch::optim::AdamOptions(kIma4bLearningRate));
  }
  const auto initial_true =
      ima4b_predict_split(result.arms[0].predictor, validation, device);
  const auto initial_control =
      ima4b_predict_split(result.arms[1].predictor, validation, device);
  result.initial_equal =
      ima4b_snapshots_exact(result.arms[0].initial,
                            result.arms[1].initial) &&
      rssm_tensor_bytes_equal(initial_true, initial_control);
  result.production_reproduced =
      ima4b_max_abs_difference(initial_true, validation.current) <=
          kIma4bTolerance &&
      ima4b_max_abs_difference(initial_control, validation.current) <=
          kIma4bTolerance;

  for (std::size_t arm = 0; arm < result.arms.size(); ++arm) {
    const auto prediction = ima4b_predict_split(
        result.arms[arm].predictor, validation, device);
    result.arms[arm].validation[0] =
        ima4b_metric(prediction, validation);
    result.arms[arm].selected = result.arms[arm].initial;
  }

  std::size_t validation_index = 1;
  for (int64_t zero_step = 0; zero_step < kIma4bUpdates; ++zero_step) {
    const auto rows = ima4b_training_groups(
        fit.context.size(0), seed, zero_step);
    const auto row_tensor = torch::tensor(rows, torch::kInt64);
    const auto batch = ima4b_device_batch(fit, row_tensor, device);
    std::array<GeneratorStateSnapshot, 2> before{};
    std::array<GeneratorStateSnapshot, 2> after{};
    for (std::size_t arm = 0; arm < result.arms.size(); ++arm) {
      set_paired_rng(ima4b_dropout_seed(seed, zero_step), device);
      before[arm] = current_generator_state_snapshot(device);
      const double loss = ima4b_train_step(
          result.arms[arm].predictor, *result.arms[arm].optimizer, batch);
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
    if (validation_index < kIma4bValidationSteps.size() &&
        completed == kIma4bValidationSteps[validation_index]) {
      for (std::size_t arm = 0; arm < result.arms.size(); ++arm) {
        const auto prediction = ima4b_predict_split(
            result.arms[arm].predictor, validation, device);
        auto &training = result.arms[arm];
        training.validation[validation_index] =
            ima4b_metric(prediction, validation);
        if (training.validation[validation_index].nmse <
            training.validation[training.selected_validation_index].nmse) {
          training.selected_validation_index = validation_index;
          training.selected_step = completed;
          training.selected =
              ima4b_snapshot_predictor(training.predictor);
        }
      }
      ++validation_index;
    }
  }
  if (validation_index != kIma4bValidationSteps.size()) {
    throw std::runtime_error("IMA-4B validation schedule failed");
  }

  for (std::size_t arm = 0; arm < result.arms.size(); ++arm) {
    auto &training = result.arms[arm];
    training.parameter_delta =
        ima4b_predictor_max_abs_diff(training.predictor, training.initial);
    training.finite =
        training.finite && training.updates == kIma4bUpdates &&
        training.parameter_delta > 0.0 &&
        ima4b_predictor_finite(training.predictor,
                               /*require_gradients=*/true) &&
        ima4b_adam_complete(*training.optimizer, kIma4bUpdates);
    result.compute_censored =
        result.compute_censored ||
        (training.selected_step == kIma4bUpdates &&
         training.validation.back().r2 -
                 training.validation[kIma4bValidationSteps.size() - 2].r2 >=
             kIma4bComputeCensorDelta);
    ima4b_restore_predictor(training.predictor, training.selected);
    result.development_prediction[arm] = ima4b_predict_split(
        training.predictor, development, device);
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

struct Ima4bSeedEvidence {
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
                        kIma4bValidationSteps.size()>,
             2>
      validation{};
  std::array<int64_t, 2> selected_step{};
  std::array<int64_t, 2> predictor_updates{};
  std::array<double, 2> parameter_delta{};
  bool initial_equal{false};
  bool production_reproduced{false};
  bool paired_rng{false};
  bool optimizer_layout_equal{false};
  bool custody{false};
  bool component_equivalence{false};
  bool frozen_state_exact{false};
  bool frozen_gradients_clear{false};
  bool captures_exact{false};
  bool compute_censored{false};
  bool pass{false};
};

[[nodiscard]] bool ima4b_frozen_gradients_clear(
    const mtf::MtfJepaMaeVicreg &model) {
  const auto parameters = model->named_parameters(/*recurse=*/true);
  return std::all_of(parameters.begin(), parameters.end(),
                     [](const auto &item) {
                       return !item.value().grad().defined();
                     });
}

[[nodiscard]] bool ima4b_settled_evidence_exact() {
  const auto log_path = std::filesystem::path(kIma4bIma4aLogPath);
  if (!std::filesystem::exists(log_path)) {
    return false;
  }
  const auto log = rmc_read_file(log_path);
  return digest::sha256_hex(log) == kIma4bIma4aLogSha256 &&
         log.find("ima4a.mechanics_pass=true") != std::string::npos &&
         log.find("ima4a.decision="
                  "categorical_target_slot_interaction_bottleneck") !=
             std::string::npos &&
         log.find("execution_status=ima4a_measurements_complete") !=
             std::string::npos &&
         ima4a_sha256(kIma4aSourcePath) == kIma4bIma4aSourceSha256 &&
         ima4a_sha256(kIma4aProtocolPath) == kIma4bIma4aProtocolSha256 &&
         ima3_sha256(kIma3SourcePath) == kIma4bIma3SourceSha256 &&
         ima3_sha256(kIma3HeaderPath) == kIma4bHeaderSha256;
}

[[nodiscard]] Ima4bSeedEvidence ima4b_run_seed(
    const RmcData &data, const torch::Device &device,
    std::size_t seed_index) {
  Ima4bSeedEvidence result{};
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
    throw std::runtime_error("IMA-4B frozen custody failed");
  }
  model->eval();
  result.component_equivalence =
      ima4a_component_equivalence(model, data.ssl, device, result.seed);
  if (!result.component_equivalence) {
    throw std::runtime_error("IMA-4B component equivalence failed");
  }
  const auto frozen_before = oca_snapshot_state(model);
  const auto modules = ima4a_modules(model);
  auto fit = ima4b_capture_split(modules, data.probe_train,
                                 model->config(), device, result.seed);
  auto validation = ima4b_capture_split(
      modules, data.probe_validation, model->config(), device, result.seed);
  auto development = ima4b_capture_split(
      modules, data.development, model->config(), device, result.seed);
  const std::array<uint64_t, 3> capture_hashes{
      fit.hash, validation.hash, development.hash};

  result.current = ima4b_metric(development.current, development);
  if (std::abs(result.current.r2 - kIma4bSealedCurrentR2[seed_index]) >
      kIma4bTolerance) {
    throw std::runtime_error("IMA-4B current score reproduction failed");
  }
  const auto c = ima4b_reproduce_c(fit, validation, development);
  result.sealed_c = c.metric;
  if (c.edge_improving ||
      std::abs(result.sealed_c.r2 - kIma4bSealedCR2[seed_index]) >
          kIma4bTolerance) {
    throw std::runtime_error("IMA-4B sealed C reproduction failed");
  }
  auto training = ima4b_train_predictors(
      modules.predictor, model->config(), fit, validation, development,
      device, result.seed);
  for (std::size_t arm = 0; arm < 2; ++arm) {
    result.trained_prediction[arm] =
        ima4b_flatten_targets(training.development_prediction[arm]);
    result.trained[arm] =
        ima4b_metric(training.development_prediction[arm], development);
    result.validation[arm] = training.arms[arm].validation;
    result.selected_step[arm] = training.arms[arm].selected_step;
    result.predictor_updates[arm] = training.arms[arm].updates;
    result.parameter_delta[arm] = training.arms[arm].parameter_delta;
  }
  result.initial_equal = training.initial_equal;
  result.production_reproduced = training.production_reproduced;
  result.paired_rng = training.rng_equal;
  result.optimizer_layout_equal = training.optimizer_layout_equal;
  result.target = ima4b_flatten_targets(development.target);
  result.target_slot = ima4b_flatten_targets(development.target_slot);
  result.current_prediction =
      ima4b_flatten_targets(development.current);
  result.c_prediction = c.prediction;
  result.compute_censored = training.compute_censored;
  result.captures_exact =
      capture_hashes[0] == ima4b_capture_hash(fit) &&
      capture_hashes[1] == ima4b_capture_hash(validation) &&
      capture_hashes[2] == ima4b_capture_hash(development);
  result.frozen_state_exact = oca_state_exact(model, frozen_before);
  result.frozen_gradients_clear = ima4b_frozen_gradients_clear(model);
  bool metrics_finite = std::isfinite(result.current.r2) &&
                        std::isfinite(result.sealed_c.r2);
  for (const auto &metric : result.trained) {
    metrics_finite = metrics_finite && std::isfinite(metric.r2) &&
                     std::isfinite(metric.nmse);
  }
  result.pass = result.custody && result.component_equivalence &&
                result.frozen_state_exact &&
                result.frozen_gradients_clear && result.captures_exact &&
                training.pass && metrics_finite;
  rng_guard.restore();
  return result;
}

struct Ima4bBootstrap {
  Interval treatment_minus_control{};
  Interval c_minus_treatment{};
  Interval c_minus_control{};
  double c_minus_treatment_upper95{0.0};
};

[[nodiscard]] double ima4b_quantile(std::vector<double> values,
                                    double probability) {
  if (values.empty() || probability < 0.0 || probability > 1.0 ||
      !std::all_of(values.begin(), values.end(),
                   [](double value) { return std::isfinite(value); })) {
    throw std::runtime_error("IMA-4B quantile contract failed");
  }
  std::sort(values.begin(), values.end());
  const double position =
      probability * static_cast<double>(values.size() - 1);
  const auto lower = static_cast<std::size_t>(std::floor(position));
  const auto upper = static_cast<std::size_t>(std::ceil(position));
  const double fraction = position - static_cast<double>(lower);
  return values[lower] + fraction * (values[upper] - values[lower]);
}

[[nodiscard]] Ima4bBootstrap ima4b_bootstrap(
    const std::array<Ima4bSeedEvidence, 3> &evidence) {
  const int64_t groups =
      evidence[0].target.size(0) / kIma4bTargetsPerGroup;
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
          ima4b_quantile(std::move(c_treatment), 0.95),
  };
}

[[nodiscard]] double ima4b_mean(const std::array<double, 3> &values) {
  return std::accumulate(values.begin(), values.end(), 0.0) /
         static_cast<double>(values.size());
}

[[nodiscard]] bool ima4b_all_positive(const std::array<double, 3> &values) {
  return std::all_of(values.begin(), values.end(),
                     [](double value) { return value > 0.0; });
}

void ima4b_emit_leave_one_out(const std::string &name,
                              const std::array<double, 3> &values) {
  for (std::size_t omitted = 0; omitted < values.size(); ++omitted) {
    double total = 0.0;
    for (std::size_t index = 0; index < values.size(); ++index) {
      if (index != omitted) {
        total += values[index];
      }
    }
    std::cout << "ima4b.summary." << name << ".leave_seed_"
              << kAttributionSeeds[omitted] << "_out=" << total / 2.0
              << '\n';
  }
}

[[nodiscard]] bool ima4b_self_test() {
  const torch::Device device(torch::kCPU);
  DefaultGeneratorStateGuard guard(device);
  auto config = ima3_config(
      device, mtf::mtf_jepa_mask_policy_t::support_separated_pair_v1);
  config.dropout = 0.0;
  set_paired_rng(410041, device);
  Ima4bPredictor treatment(config, 1.0);
  set_paired_rng(410041, device);
  Ima4bPredictor control(config, 0.0);
  {
    torch::NoGradGuard no_grad;
    for (const auto &predictor : {treatment, control}) {
      for (const auto &item :
           predictor->named_parameters(/*recurse=*/true)) {
        if (item.key() == "slot_embedding.weight") {
          item.value().zero_();
        }
      }
    }
  }
  auto context = torch::randn({2, 72, 32}, torch::kFloat32);
  context[1].mul_(-1.0);
  auto mask = torch::zeros({2, 72}, torch::kBool);
  mask.index_put_({Slice(), Slice(0, 54)}, true);
  const auto metadata = torch::zeros({2, 6}, torch::kFloat32);
  const auto slot = torch::zeros({2}, torch::kInt64);
  treatment->eval();
  control->eval();
  torch::NoGradGuard no_grad;
  const auto treatment_zero =
      treatment->forward(context, mask, metadata, slot);
  const auto control_zero = control->forward(context, mask, metadata, slot);
  const bool initial_equal =
      rssm_tensor_bytes_equal(treatment_zero, control_zero);
  for (const auto &predictor : {treatment, control}) {
    for (const auto &item :
         predictor->named_parameters(/*recurse=*/true)) {
      if (item.key() == "slot_embedding.weight") {
        auto weight = item.value();
        weight.index_put_({0, Slice()}, 0.5);
      }
    }
  }
  const auto treatment_active =
      treatment->forward(context, mask, metadata, slot);
  const auto control_active = control->forward(context, mask, metadata, slot);
  const auto treatment_delta = treatment_active - treatment_zero;
  const auto control_delta = control_active - control_zero;
  const bool control_is_bias =
      torch::allclose(control_delta[0], control_delta[1], 1.0e-7, 1.0e-7);
  const bool treatment_interacts =
      (treatment_delta[0] - treatment_delta[1])
          .abs()
          .max()
          .item<double>() > 1.0e-8;
  const bool counts =
      ima4b_parameter_count(treatment) == kIma4bTrainableParameters &&
      ima4b_parameter_count(control) == kIma4bTrainableParameters;
  const bool finite = torch::isfinite(treatment_active).all().item<bool>() &&
                      torch::isfinite(control_active).all().item<bool>();
  guard.restore();
  return initial_equal && control_is_bias && treatment_interacts && counts &&
         finite;
}

[[nodiscard]] int run_ima4b_self_test() {
  std::cout << std::boolalpha << std::setprecision(12);
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.ima4b.self_test.v1\n";
  std::cout << "experiment=target-relative-predictor-sufficiency-self-test\n";
  const bool pass = ima4b_self_test();
  std::cout << "ima4b.self_test.pass=" << pass << '\n';
  std::cout << "ima4b.representation_optimizer_steps=0\n";
  std::cout << "ima4b.ema_updates=0\n";
  std::cout << "execution_status=ima4b_self_test_complete\n";
  return pass ? 0 : 3;
}

[[nodiscard]] int run_ima4b_audit(const Options &options) {
  if (options.device != "cuda" || !torch::cuda::is_available()) {
    throw std::runtime_error("IMA-4B authoritative audit requires CUDA");
  }
  const char *workspace = std::getenv("CUBLAS_WORKSPACE_CONFIG");
  if (workspace == nullptr || std::string(workspace) != ":4096:8") {
    throw std::runtime_error(
        "IMA-4B requires CUBLAS_WORKSPACE_CONFIG=:4096:8");
  }
  const torch::Device device(torch::kCUDA, 0);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);
  DefaultGeneratorStateGuard audit_rng(device);
  const bool settled = ima4b_settled_evidence_exact();
  if (!settled) {
    throw std::runtime_error("IMA-4B settled IMA-4A evidence failed");
  }
  const bool self_test = ima4b_self_test();
  if (!self_test) {
    throw std::runtime_error("IMA-4B synthetic interaction self-test failed");
  }
  auto data = rmc_make_data();
  const bool splits_exact = data.probe_train.data.size(0) == 256 &&
                            data.probe_validation.data.size(0) == 128 &&
                            data.development.data.size(0) == 256 &&
                            data.probe_train.group_begin == 1000000 &&
                            data.probe_validation.group_begin == 2000000 &&
                            data.development.group_begin == 3000000;
  if (!splits_exact) {
    throw std::runtime_error("IMA-4B group split custody failed");
  }
  std::array<Ima4bSeedEvidence, 3> evidence{};
  for (std::size_t seed = 0; seed < evidence.size(); ++seed) {
    evidence[seed] = ima4b_run_seed(data, device, seed);
  }
  const auto bootstrap = ima4b_bootstrap(evidence);

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

  const double treatment_control_mean = ima4b_mean(treatment_control);
  const double c_treatment_mean = ima4b_mean(c_treatment);
  const double c_control_mean = ima4b_mean(c_control);
  const bool treatment_reaches_c =
      ima4b_all_positive(treatment) && c_treatment_mean <= 0.01 &&
      bootstrap.c_minus_treatment_upper95 < 0.02;
  const bool control_reaches_c =
      ima4b_all_positive(control) && c_control_mean <= 0.01 &&
      bootstrap.c_minus_control.high < 0.02;
  const bool treatment_material =
      treatment_control_mean >= 0.02 &&
      ima4b_all_positive(treatment_control) &&
      bootstrap.treatment_minus_control.low > 0.0;
  std::array<double, 3> treatment_c{};
  for (std::size_t index = 0; index < treatment_c.size(); ++index) {
    treatment_c[index] = -c_treatment[index];
  }
  const bool treatment_materially_exceeds_c =
      ima4b_mean(treatment_c) >= 0.02 &&
      ima4b_all_positive(treatment_c) &&
      -bootstrap.c_minus_treatment.high > 0.0;

  std::string decision = "invalid_mechanics";
  if (compute_censored) {
    decision = "compute_censored";
  } else if (mechanics) {
    if (treatment_materially_exceeds_c) {
      decision = "linear_C_underestimated_predictability";
    } else if (treatment_reaches_c && treatment_material) {
      decision = "target_relative_predictor_sufficient";
    } else if (treatment_reaches_c && control_reaches_c &&
               !treatment_material) {
      decision =
          "predictor_reoptimization_sufficient_target_identity_not_confirmed";
    } else if (treatment_material) {
      decision = "target_relative_query_helpful_but_insufficient";
    } else {
      decision = "narrow_target_relative_predictor_insufficient";
    }
  }

  std::cout << std::boolalpha << std::setprecision(12);
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.ima4b.audit.v1a2\n";
  std::cout << "experiment=target-relative-predictor-sufficiency-audit\n";
  std::cout << "device=cuda\n";
  std::cout << "ima4b.protocol_sha256=" << ima4a_sha256(kIma4bProtocolPath)
            << '\n';
  std::cout << "ima4b.protocol_amendment_a1_sha256="
            << ima4a_sha256(kIma4bProtocolAmendmentPath) << '\n';
  std::cout << "ima4b.protocol_amendment_a2_sha256="
            << ima4a_sha256(kIma4bProtocolAmendmentA2Path) << '\n';
  std::cout << "ima4b.source_sha256=" << ima4a_sha256(kIma4bSourcePath)
            << '\n';
  std::cout << "ima4b.ima4a_log_sha256=" << kIma4bIma4aLogSha256 << '\n';
  std::cout << "ima4b.self_test=" << self_test << '\n';
  std::cout << "ima4b.fit_groups=" << data.probe_train.data.size(0) << '\n';
  std::cout << "ima4b.validation_groups="
            << data.probe_validation.data.size(0) << '\n';
  std::cout << "ima4b.development_groups="
            << data.development.data.size(0) << '\n';
  std::cout << "ima4b.parameters_per_arm="
            << kIma4bTrainableParameters << '\n';
  std::cout << "ima4b.predictor_updates_per_arm=" << kIma4bUpdates << '\n';
  for (const auto &seed : evidence) {
    const std::string root =
        "ima4b.seed_" + std::to_string(seed.seed);
    std::cout << root << ".custody=" << seed.custody << '\n';
    std::cout << root << ".component_equivalence="
              << seed.component_equivalence << '\n';
    std::cout << root << ".frozen_state_exact="
              << seed.frozen_state_exact << '\n';
    std::cout << root << ".frozen_gradients_clear="
              << seed.frozen_gradients_clear << '\n';
    std::cout << root << ".captures_exact=" << seed.captures_exact << '\n';
    std::cout << root << ".initial_equal=" << seed.initial_equal << '\n';
    std::cout << root << ".production_reproduced="
              << seed.production_reproduced << '\n';
    std::cout << root << ".paired_rng=" << seed.paired_rng << '\n';
    std::cout << root << ".optimizer_layout_equal="
              << seed.optimizer_layout_equal << '\n';
    std::cout << root << ".current.r2=" << seed.current.r2 << '\n';
    std::cout << root << ".sealed_C.r2=" << seed.sealed_c.r2 << '\n';
    for (std::size_t arm = 0; arm < 2; ++arm) {
      std::cout << root << '.' << kIma4bArmNames[arm]
                << ".r2=" << seed.trained[arm].r2 << '\n';
      std::cout << root << '.' << kIma4bArmNames[arm]
                << ".selected_step=" << seed.selected_step[arm] << '\n';
      std::cout << root << '.' << kIma4bArmNames[arm]
                << ".updates=" << seed.predictor_updates[arm] << '\n';
      std::cout << root << '.' << kIma4bArmNames[arm]
                << ".parameter_delta=" << seed.parameter_delta[arm]
                << '\n';
      for (std::size_t checkpoint = 0;
           checkpoint < kIma4bValidationSteps.size(); ++checkpoint) {
        std::cout << root << '.' << kIma4bArmNames[arm]
                  << ".validation_step_"
                  << kIma4bValidationSteps[checkpoint]
                  << ".r2=" << seed.validation[arm][checkpoint].r2 << '\n';
      }
    }
    std::cout << root << ".treatment_minus_control="
              << seed.trained[0].r2 - seed.trained[1].r2 << '\n';
    std::cout << root << ".C_minus_treatment="
              << seed.sealed_c.r2 - seed.trained[0].r2 << '\n';
    std::cout << root << ".compute_censored="
              << seed.compute_censored << '\n';
    std::cout << root << ".pass=" << seed.pass << '\n';
  }
  std::cout << "ima4b.summary.current_r2=" << ima4b_mean(current) << '\n';
  std::cout << "ima4b.summary.control_r2=" << ima4b_mean(control) << '\n';
  std::cout << "ima4b.summary.treatment_r2=" << ima4b_mean(treatment)
            << '\n';
  std::cout << "ima4b.summary.sealed_C_r2=" << ima4b_mean(c) << '\n';
  std::cout << "ima4b.summary.treatment_minus_control.point="
            << treatment_control_mean << '\n';
  std::cout << "ima4b.summary.treatment_minus_control.low="
            << bootstrap.treatment_minus_control.low << '\n';
  std::cout << "ima4b.summary.treatment_minus_control.high="
            << bootstrap.treatment_minus_control.high << '\n';
  std::cout << "ima4b.summary.C_minus_treatment.point="
            << c_treatment_mean << '\n';
  std::cout << "ima4b.summary.C_minus_treatment.low="
            << bootstrap.c_minus_treatment.low << '\n';
  std::cout << "ima4b.summary.C_minus_treatment.high="
            << bootstrap.c_minus_treatment.high << '\n';
  std::cout << "ima4b.summary.C_minus_treatment.upper95="
            << bootstrap.c_minus_treatment_upper95 << '\n';
  ima4b_emit_leave_one_out("treatment_minus_control", treatment_control);
  ima4b_emit_leave_one_out("C_minus_treatment", c_treatment);
  std::cout << "ima4b.treatment_reaches_C=" << treatment_reaches_c << '\n';
  std::cout << "ima4b.control_reaches_C=" << control_reaches_c << '\n';
  std::cout << "ima4b.treatment_material=" << treatment_material << '\n';
  std::cout << "ima4b.compute_censored=" << compute_censored << '\n';
  std::cout << "ima4b.mechanics_pass=" << mechanics << '\n';
  std::cout << "ima4b.decision=" << decision << '\n';
  std::cout << "ima4b.representation_optimizer_steps=0\n";
  std::cout << "ima4b.ema_updates=0\n";
  std::cout << "execution_status=ima4b_measurements_complete\n";
  audit_rng.restore();
  return mechanics ? 0 : 3;
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment ==
        "target-relative-predictor-sufficiency-self-test") {
      return run_ima4b_self_test();
    }
    if (options.experiment ==
        "target-relative-predictor-sufficiency-audit") {
      return run_ima4b_audit(options);
    }
    throw std::runtime_error(
        "--experiment must be target-relative-predictor-sufficiency-"
        "self-test or target-relative-predictor-sufficiency-audit");
  } catch (const c10::Error &error) {
    std::cerr << "target_relative_predictor_sufficiency_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "target_relative_predictor_sufficiency_error="
              << error.what() << '\n';
  }
  return 2;
}
