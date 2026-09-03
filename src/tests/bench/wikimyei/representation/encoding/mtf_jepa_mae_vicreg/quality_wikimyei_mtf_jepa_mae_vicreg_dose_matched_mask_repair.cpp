#include "piaabo/digest/sha256.h"

#include <chrono>

// IMA-3 reuses the fixed synthetic rows, FSPA-4 anchors, cached legacy JEPA
// arm, sparse structured readout, probes, and bootstrap implementation from
// OCA-1. It constructs no downstream model and invokes no outer augmentation.
#define CUWACUNU_OCA_EMBEDDED
#include "quality_wikimyei_mtf_jepa_mae_vicreg_four_objective_causal_attribution.cpp"
#undef CUWACUNU_OCA_EMBEDDED

namespace {

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
constexpr std::string_view kIma3CacheSchema =
    "ima3.dose_matched_mask_repair.seed_cache.v1";
constexpr std::string_view kIma3SourcePath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_dose_matched_mask_repair.cpp";
constexpr std::string_view kIma3HeaderPath =
    "src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/"
    "mtf_jepa_mae_vicreg.h";

struct Ima3ArmReceipt {
  int64_t steps{0};
  double loss_sum{0.0};
  double jepa_loss_sum{0.0};
  double minimum_gradient_norm{std::numeric_limits<double>::infinity()};
  double maximum_gradient_norm{0.0};
  double minimum_served_update_norm{std::numeric_limits<double>::infinity()};
  double maximum_served_update_norm{0.0};
  int64_t clipping_count{0};
  double all_trainable_delta{0.0};
  double served_delta{0.0};
  double predictor_delta{0.0};
  double mae_decoder_delta{0.0};
  double vicreg_head_delta{0.0};
  double target_ema_delta{0.0};
  bool finite{true};
  bool expected_partitions{false};
  bool pass{false};
};

struct Ima3SeedResult {
  std::vector<mtf::MtfJepaMaeVicreg> models{};
  std::array<Ima3ArmReceipt, 2> receipts{};
  uint64_t row_schedule_digest{0xcbf29ce484222325ULL};
  int64_t target_exact_updates{0};
  int64_t weak_view_exact_updates{0};
  int64_t context_distinct_updates{0};
  int64_t mask_count_exact_updates{0};
  bool metadata_exact{true};
  bool initialization_exact{true};
  bool config_isolated{true};
  bool resumed{false};
  bool pass{false};
};

struct Ima3Preflight {
  int64_t audited_updates{0};
  int64_t target_exact_updates{0};
  int64_t context_distinct_updates{0};
  int64_t rng_exact_updates{0};
  int64_t mask_count_exact_updates{0};
  int64_t leaky_samples_with_overlap{0};
  int64_t repaired_samples_with_overlap{0};
  int64_t audited_samples{0};
  bool pins_exact{true};
  bool cached_control_exact{true};
  bool initialization_exact{true};
  bool config_isolated{true};
  bool forward_smoke_exact{true};
  bool pass{false};
};

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t
ima3_config(const torch::Device &device, mtf::mtf_jepa_mask_policy_t policy) {
  auto config = attribution_config(device, oca_arm(1U));
  config.jepa_mask_policy = policy;
  return config;
}

[[nodiscard]] std::string ima3_sha256(std::string_view path) {
  return digest::sha256_hex(rmc_read_file(std::filesystem::path(path)));
}

[[nodiscard]] std::string
ima3_without_policy_line(const std::string &manifest) {
  std::istringstream input(manifest);
  std::ostringstream output;
  std::string line;
  while (std::getline(input, line)) {
    if (!line.starts_with("jepa_mask_policy=")) {
      output << line << '\n';
    }
  }
  return output.str();
}

[[nodiscard]] bool ima3_configs_isolated(const torch::Device &device) {
  const auto left =
      canonical_config_manifest(ima3_config(device, kIma3Policies[0]));
  const auto right =
      canonical_config_manifest(ima3_config(device, kIma3Policies[1]));
  return left != right &&
         ima3_without_policy_line(left) == ima3_without_policy_line(right);
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

[[nodiscard]] int64_t
ima3_context_overlap_samples(const mtf::mtf_token_metadata_t &metadata,
                             const torch::Tensor &context_mask,
                             const torch::Tensor &target_mask) {
  const auto contexts = context_mask.to(torch::kCPU, torch::kBool).contiguous();
  const auto targets = target_mask.to(torch::kCPU, torch::kBool).contiguous();
  const auto channels =
      metadata.channel_id.to(torch::kCPU, torch::kInt64).contiguous();
  const auto starts =
      metadata.start_index.to(torch::kCPU, torch::kInt64).contiguous();
  const auto widths =
      metadata.width.to(torch::kCPU, torch::kInt64).contiguous();
  const auto context = contexts.accessor<bool, 2>();
  const auto target = targets.accessor<bool, 2>();
  const auto channel = channels.accessor<int64_t, 1>();
  const auto start = starts.accessor<int64_t, 1>();
  const auto width = widths.accessor<int64_t, 1>();
  int64_t samples = 0;
  for (int64_t batch = 0; batch < contexts.size(0); ++batch) {
    bool overlap = false;
    for (int64_t left = 0; left < contexts.size(1) && !overlap; ++left) {
      if (!context[batch][left]) {
        continue;
      }
      for (int64_t right = 0; right < targets.size(1); ++right) {
        if (!target[batch][right] || channel[left] != channel[right]) {
          continue;
        }
        overlap =
            std::max(start[left], start[right]) <
            std::min(start[left] + width[left], start[right] + width[right]);
        if (overlap) {
          break;
        }
      }
    }
    samples += overlap ? 1 : 0;
  }
  return samples;
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

[[nodiscard]] std::filesystem::path ima3_preflight_receipt_path() {
  return std::filesystem::path(".build") / "tests" / "ima3" /
         "preflight_v1.complete.txt";
}

[[nodiscard]] std::string ima3_preflight_receipt_contents() {
  std::ostringstream out;
  out << "schema=ima3.dose_matched_mask_repair.preflight.v1\n";
  out << "source_sha256=" << ima3_sha256(kIma3SourcePath) << '\n';
  out << "header_sha256=" << ima3_sha256(kIma3HeaderPath) << '\n';
  out << "audited_updates=" << kIma3Steps * 3 << '\n';
  out << "pass=true\n";
  return out.str();
}

void ima3_write_preflight_receipt() {
  const auto path = ima3_preflight_receipt_path();
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out.is_open()) {
    throw std::runtime_error("cannot create IMA-3 preflight receipt");
  }
  out << ima3_preflight_receipt_contents();
  if (!out) {
    throw std::runtime_error("IMA-3 preflight receipt write failed");
  }
}

[[nodiscard]] bool ima3_preflight_receipt_exact() {
  const auto path = ima3_preflight_receipt_path();
  return std::filesystem::exists(path) &&
         rmc_read_file(path) == ima3_preflight_receipt_contents();
}

[[nodiscard]] Ima3Preflight ima3_run_preflight(const torch::Device &device,
                                               RmcData &data) {
  Ima3Preflight result{};
  result.config_isolated = ima3_configs_isolated(device);
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_config_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  const std::vector<uint8_t> challenge_masks{4U, 1U, 2U, 8U, 15U};

  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    result.pins_exact = result.pins_exact && ima3_pins_exact(seed_index);

    OcaInterleavedTrainingResult cached{};
    const bool loaded = oca_load_seed_cache(
        "anchor_challenge", data.ssl, device, seed, challenge_masks, kIma3Steps,
        /*load_certified_anchor=*/true, cached);
    result.cached_control_exact =
        result.cached_control_exact && loaded && cached.pass &&
        cached.models.size() == challenge_masks.size() &&
        cached.receipts.size() == challenge_masks.size() &&
        cached.receipts[1].steps == kIma3Steps && cached.receipts[1].pass;

    set_paired_rng(seed, device);
    auto leaky = mtf::MtfJepaMaeVicreg(ima3_config(device, kIma3Policies[0]));
    set_paired_rng(seed, device);
    auto repaired =
        mtf::MtfJepaMaeVicreg(ima3_config(device, kIma3Policies[1]));
    const bool leaky_metadata = oca_load_archive(
        oca_archive_path(seed), leaky, device, seed, anchor_config_hash);
    const bool repaired_metadata = oca_load_archive(
        oca_archive_path(seed), repaired, device, seed, anchor_config_hash);
    const auto canonical_initial = snapshot_parameters(leaky);
    result.initialization_exact =
        result.initialization_exact && leaky_metadata && repaired_metadata &&
        parameter_max_abs_diff(repaired, canonical_initial) == 0.0;

    const auto rows = training_rows(data.ssl, seed, 0);
    const auto indices = torch::tensor(rows, torch::kInt64);
    const auto batch_data = data.ssl.data.index_select(0, indices).to(device);
    const auto batch_mask = data.ssl.mask.index_select(0, indices).to(device);
    mtf::mtf_token_batch_t tokens{};
    {
      torch::NoGradGuard no_grad;
      tokens = leaky->tokenize(batch_data, batch_mask);
    }
    result.audited_samples += tokens.token_mask.size(0) * kIma3Steps;

    for (int64_t step = 0; step < kIma3Steps; ++step) {
      const int64_t schedule_seed = paired_step_seed(seed, step);
      set_paired_rng(schedule_seed, device);
      const auto leaky_masks = leaky->create_masks(tokens);
      const auto leaky_post = current_generator_state_snapshot(device);
      set_paired_rng(schedule_seed, device);
      const auto repaired_masks = repaired->create_masks(tokens);
      const auto repaired_post = current_generator_state_snapshot(device);
      ++result.audited_updates;
      result.target_exact_updates +=
          torch::equal(leaky_masks.target_mask, repaired_masks.target_mask) ? 1
                                                                            : 0;
      result.context_distinct_updates +=
          torch::equal(leaky_masks.context_mask, repaired_masks.context_mask)
              ? 0
              : 1;
      result.rng_exact_updates +=
          generator_state_snapshot_equal(leaky_post, repaired_post) ? 1 : 0;
      result.mask_count_exact_updates +=
          ima3_mask_counts_exact(leaky_masks, tokens,
                                 leaky->config().min_context_ratio) &&
                  ima3_mask_counts_exact(repaired_masks, tokens,
                                         repaired->config().min_context_ratio)
              ? 1
              : 0;
      result.leaky_samples_with_overlap += ima3_context_overlap_samples(
          tokens.metadata, leaky_masks.context_mask, leaky_masks.target_mask);
      result.repaired_samples_with_overlap += ima3_context_overlap_samples(
          tokens.metadata, repaired_masks.context_mask,
          repaired_masks.target_mask);
    }

    leaky->train();
    repaired->train();
    const auto leaky_before = snapshot_parameters(leaky);
    const auto repaired_before = snapshot_parameters(repaired);
    set_paired_rng(paired_step_seed(seed, 0), device);
    const auto leaky_output = leaky->forward(batch_data, batch_mask);
    const auto leaky_post = current_generator_state_snapshot(device);
    set_paired_rng(paired_step_seed(seed, 0), device);
    const auto repaired_output = repaired->forward(batch_data, batch_mask);
    const auto repaired_post = current_generator_state_snapshot(device);
    validate_weak_view_debug_tensors(leaky_output, batch_data, batch_mask);
    validate_weak_view_debug_tensors(repaired_output, batch_data, batch_mask);
    const bool finite =
        torch::stack({leaky_output.loss, repaired_output.loss,
                      leaky_output.loss_jepa, repaired_output.loss_jepa})
            .isfinite()
            .all()
            .item<bool>();
    result.forward_smoke_exact =
        result.forward_smoke_exact && finite &&
        torch::equal(leaky_output.jepa_target_mask,
                     repaired_output.jepa_target_mask) &&
        !torch::equal(leaky_output.jepa_context_mask,
                      repaired_output.jepa_context_mask) &&
        weak_view_digests_equal(weak_view_digest(leaky_output),
                                weak_view_digest(repaired_output)) &&
        generator_state_snapshot_equal(leaky_post, repaired_post) &&
        parameter_max_abs_diff(leaky, leaky_before) == 0.0 &&
        parameter_max_abs_diff(repaired, repaired_before) == 0.0;
  }

  result.pass = result.pins_exact && result.cached_control_exact &&
                result.initialization_exact && result.config_isolated &&
                result.forward_smoke_exact &&
                result.audited_updates == kIma3Steps * 3 &&
                result.target_exact_updates == result.audited_updates &&
                result.context_distinct_updates == result.audited_updates &&
                result.rng_exact_updates == result.audited_updates &&
                result.mask_count_exact_updates == result.audited_updates &&
                result.leaky_samples_with_overlap == result.audited_samples &&
                result.repaired_samples_with_overlap == 0;
  return result;
}

void ima3_emit_preflight(const Ima3Preflight &value) {
  std::cout << "ima3.preflight.audited_updates=" << value.audited_updates
            << '\n';
  std::cout << "ima3.preflight.target_exact_updates="
            << value.target_exact_updates << '\n';
  std::cout << "ima3.preflight.context_distinct_updates="
            << value.context_distinct_updates << '\n';
  std::cout << "ima3.preflight.rng_exact_updates=" << value.rng_exact_updates
            << '\n';
  std::cout << "ima3.preflight.mask_count_exact_updates="
            << value.mask_count_exact_updates << '\n';
  std::cout << "ima3.preflight.audited_samples=" << value.audited_samples
            << '\n';
  std::cout << "ima3.preflight.leaky_samples_with_overlap="
            << value.leaky_samples_with_overlap << '\n';
  std::cout << "ima3.preflight.repaired_samples_with_overlap="
            << value.repaired_samples_with_overlap << '\n';
  std::cout << "ima3.preflight.pins_exact=" << value.pins_exact << '\n';
  std::cout << "ima3.preflight.cached_control_exact="
            << value.cached_control_exact << '\n';
  std::cout << "ima3.preflight.initialization_exact="
            << value.initialization_exact << '\n';
  std::cout << "ima3.preflight.config_isolated=" << value.config_isolated
            << '\n';
  std::cout << "ima3.preflight.forward_smoke_exact="
            << value.forward_smoke_exact << '\n';
  std::cout << "ima3.preflight.pass=" << value.pass << '\n';
}

[[nodiscard]] uint64_t ima3_expected_row_schedule_digest(const Dataset &ssl,
                                                         int64_t seed) {
  uint64_t digest_value = 0xcbf29ce484222325ULL;
  for (int64_t step = 0; step < kIma3Steps; ++step) {
    mix_hash_value(digest_value,
                   hash_batch_rows(training_rows(ssl, seed, step)));
  }
  return digest_value;
}

[[nodiscard]] bool ima3_receipt_valid(const Ima3ArmReceipt &receipt) {
  return receipt.steps == kIma3Steps && receipt.finite &&
         std::isfinite(receipt.loss_sum) &&
         std::isfinite(receipt.jepa_loss_sum) &&
         receipt.minimum_gradient_norm > 0.0 &&
         receipt.minimum_served_update_norm > 0.0 &&
         receipt.all_trainable_delta > 0.0 && receipt.served_delta > 0.0 &&
         receipt.predictor_delta > 0.0 && receipt.mae_decoder_delta == 0.0 &&
         receipt.vicreg_head_delta == 0.0 && receipt.target_ema_delta > 0.0 &&
         receipt.expected_partitions && receipt.pass;
}

[[nodiscard]] Ima3SeedResult ima3_train_seed(const Dataset &ssl,
                                             const torch::Device &device,
                                             int64_t seed,
                                             std::size_t seed_index) {
  Ima3SeedResult result{};
  result.config_isolated = ima3_configs_isolated(device);
  result.metadata_exact = ima3_pins_exact(seed_index);
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_config_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  std::vector<std::unique_ptr<torch::optim::Adam>> optimizers;
  ParameterSnapshot canonical_initial{};

  for (std::size_t arm = 0; arm < 2; ++arm) {
    set_paired_rng(seed, device);
    auto model = mtf::MtfJepaMaeVicreg(ima3_config(device, kIma3Policies[arm]));
    result.metadata_exact = result.metadata_exact &&
                            oca_load_archive(oca_archive_path(seed), model,
                                             device, seed, anchor_config_hash);
    if (arm == 0) {
      canonical_initial = snapshot_parameters(model);
    } else {
      result.initialization_exact =
          parameter_max_abs_diff(model, canonical_initial) == 0.0;
    }
    model->train();
    result.receipts[arm].steps = kIma3Steps;
    result.models.push_back(model);
    optimizers.push_back(std::make_unique<torch::optim::Adam>(
        result.models.back()->parameters(),
        torch::optim::AdamOptions(kOcaOptimizerLearningRate)));
  }

  for (int64_t step = 0; step < kIma3Steps; ++step) {
    const auto rows = training_rows(ssl, seed, step);
    const uint64_t row_hash = hash_batch_rows(rows);
    mix_hash_value(result.row_schedule_digest, row_hash);
    const auto indices = torch::tensor(rows, torch::kInt64);
    const auto data = ssl.data.index_select(0, indices).to(device);
    const auto feature_mask = ssl.mask.index_select(0, indices).to(device);
    torch::Tensor reference_target{};
    torch::Tensor reference_context{};
    torch::Tensor reference_view_a_data{};
    torch::Tensor reference_view_a_mask{};
    torch::Tensor reference_view_b_data{};
    torch::Tensor reference_view_b_mask{};

    for (std::size_t arm = 0; arm < 2; ++arm) {
      auto &model = result.models[arm];
      auto &optimizer = *optimizers[arm];
      auto &receipt = result.receipts[arm];
      set_paired_rng(paired_step_seed(seed, step), device);
      optimizer.zero_grad();
      const auto output = model->forward(data, feature_mask);
      validate_weak_view_debug_tensors(output, data, feature_mask);
      if (arm == 0) {
        reference_target = output.jepa_target_mask.detach();
        reference_context = output.jepa_context_mask.detach();
        reference_view_a_data = output.vicreg_view_a_data.detach();
        reference_view_a_mask = output.vicreg_view_a_feature_mask.detach();
        reference_view_b_data = output.vicreg_view_b_data.detach();
        reference_view_b_mask = output.vicreg_view_b_feature_mask.detach();
      }
      const auto target_exact =
          torch::eq(output.jepa_target_mask, reference_target).all();
      const auto weak_exact =
          torch::stack(
              {torch::eq(output.vicreg_view_a_data, reference_view_a_data)
                   .all(),
               torch::eq(output.vicreg_view_a_feature_mask,
                         reference_view_a_mask)
                   .all(),
               torch::eq(output.vicreg_view_b_data, reference_view_b_data)
                   .all(),
               torch::eq(output.vicreg_view_b_feature_mask,
                         reference_view_b_mask)
                   .all()})
              .all();
      const auto context_distinct =
          arm == 0
              ? torch::ones(
                    {},
                    torch::TensorOptions().dtype(torch::kBool).device(device))
              : torch::ne(output.jepa_context_mask, reference_context).any();
      const int64_t valid_per_sample = output.jepa_target_mask.size(1);
      const int64_t expected_context = std::max<int64_t>(
          1, static_cast<int64_t>(std::ceil(model->config().min_context_ratio *
                                            valid_per_sample)));
      const auto mask_counts =
          output.jepa_target_mask.sum(1)
              .eq(kIma3TargetsPerSample)
              .all()
              .logical_and(
                  output.jepa_context_mask.sum(1).eq(expected_context).all());
      const auto loss = output.loss;
      loss.backward();

      auto gradient_square =
          torch::zeros({}, torch::TensorOptions().device(device));
      for (const auto &parameter : model->parameters()) {
        if (parameter.grad().defined()) {
          gradient_square =
              gradient_square + parameter.grad().detach().pow(2).sum();
        }
      }
      const double gradient_norm = gradient_square.sqrt().item<double>();
      receipt.minimum_gradient_norm =
          std::min(receipt.minimum_gradient_norm, gradient_norm);
      receipt.maximum_gradient_norm =
          std::max(receipt.maximum_gradient_norm, gradient_norm);
      const double clip_factor =
          gradient_norm > kOcaGradientClipNorm
              ? kOcaGradientClipNorm / std::max(1.0e-30, gradient_norm)
              : 1.0;
      if (clip_factor < 1.0) {
        ++receipt.clipping_count;
        for (const auto &parameter : model->parameters()) {
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
      optimizer.step();
      auto update_square =
          torch::zeros({}, torch::TensorOptions().device(device));
      for (std::size_t parameter = 0; parameter < served_parameters.size();
           ++parameter) {
        update_square = update_square + (served_parameters[parameter].detach() -
                                         served_before[parameter])
                                            .pow(2)
                                            .sum();
      }
      model->update_target_network();
      const auto diagnostics =
          torch::stack({loss.detach(), output.loss_jepa.detach(),
                        update_square.sqrt(),
                        target_exact.to(loss.scalar_type()),
                        weak_exact.to(loss.scalar_type()),
                        context_distinct.to(loss.scalar_type()),
                        mask_counts.to(loss.scalar_type())})
              .to(torch::kCPU, torch::kFloat64);
      const double loss_value = diagnostics[0].item<double>();
      const double jepa_value = diagnostics[1].item<double>();
      const double update_norm = diagnostics[2].item<double>();
      receipt.loss_sum += loss_value;
      receipt.jepa_loss_sum += jepa_value;
      receipt.minimum_served_update_norm =
          std::min(receipt.minimum_served_update_norm, update_norm);
      receipt.maximum_served_update_norm =
          std::max(receipt.maximum_served_update_norm, update_norm);
      receipt.finite = receipt.finite && std::isfinite(loss_value) &&
                       std::isfinite(jepa_value) &&
                       std::isfinite(gradient_norm) &&
                       std::isfinite(update_norm);
      if (arm == 1) {
        result.target_exact_updates +=
            diagnostics[3].item<double>() == 1.0 ? 1 : 0;
        result.weak_view_exact_updates +=
            diagnostics[4].item<double>() == 1.0 ? 1 : 0;
        result.context_distinct_updates +=
            diagnostics[5].item<double>() == 1.0 ? 1 : 0;
        result.mask_count_exact_updates +=
            diagnostics[6].item<double>() == 1.0 ? 1 : 0;
      }
    }
    const int64_t completed = step + 1;
    if (completed % 128 == 0 || completed == kIma3Steps) {
      std::cout << "ima3.training.seed_" << seed
                << ".completed_steps=" << completed << '\n'
                << std::flush;
    }
  }

  for (std::size_t arm = 0; arm < 2; ++arm) {
    auto &receipt = result.receipts[arm];
    auto &model = result.models[arm];
    receipt.all_trainable_delta = parameter_partition_max_abs_diff(
        model, canonical_initial, ParameterDeltaPartition::all_trainable);
    receipt.served_delta = parameter_partition_max_abs_diff(
        model, canonical_initial, ParameterDeltaPartition::served);
    receipt.predictor_delta = parameter_partition_max_abs_diff(
        model, canonical_initial, ParameterDeltaPartition::predictor);
    receipt.mae_decoder_delta = parameter_partition_max_abs_diff(
        model, canonical_initial, ParameterDeltaPartition::mae_decoder);
    receipt.vicreg_head_delta = parameter_partition_max_abs_diff(
        model, canonical_initial, ParameterDeltaPartition::vicreg_head);
    receipt.target_ema_delta = parameter_partition_max_abs_diff(
        model, canonical_initial, ParameterDeltaPartition::target_ema);
    receipt.expected_partitions =
        receipt.all_trainable_delta > 0.0 && receipt.served_delta > 0.0 &&
        receipt.predictor_delta > 0.0 && receipt.mae_decoder_delta == 0.0 &&
        receipt.vicreg_head_delta == 0.0 && receipt.target_ema_delta > 0.0;
    receipt.pass = receipt.finite && receipt.minimum_gradient_norm > 0.0 &&
                   receipt.minimum_served_update_norm > 0.0 &&
                   receipt.expected_partitions;
  }
  result.pass = result.metadata_exact && result.initialization_exact &&
                result.config_isolated &&
                result.row_schedule_digest ==
                    ima3_expected_row_schedule_digest(ssl, seed) &&
                result.target_exact_updates == kIma3Steps &&
                result.weak_view_exact_updates == kIma3Steps &&
                result.context_distinct_updates == kIma3Steps &&
                result.mask_count_exact_updates == kIma3Steps &&
                ima3_receipt_valid(result.receipts[0]) &&
                ima3_receipt_valid(result.receipts[1]);
  return result;
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

void ima3_save_seed_cache(const Dataset &ssl, const torch::Device &device,
                          int64_t seed, std::size_t seed_index,
                          const Ima3SeedResult &result) {
  if (!result.pass || result.models.size() != 2) {
    throw std::runtime_error("IMA-3 incomplete seed cache save rejected");
  }
  const auto path = ima3_seed_cache_path(seed);
  const auto marker = ima3_seed_cache_marker_path(path);
  const auto nonce =
      std::chrono::steady_clock::now().time_since_epoch().count();
  auto temporary = path;
  temporary += ".tmp." + std::to_string(nonce);
  auto temporary_marker = marker;
  temporary_marker += ".tmp." + std::to_string(nonce);
  std::filesystem::create_directories(path.parent_path());
  const auto ssl_hashes = oca_seed_cache_ssl_hashes(ssl);

  torch::serialize::OutputArchive root;
  root.write("meta/schema", oca_string_tensor(kIma3CacheSchema));
  root.write("meta/source_sha256",
             oca_string_tensor(ima3_sha256(kIma3SourcePath)));
  root.write("meta/header_sha256",
             oca_string_tensor(ima3_sha256(kIma3HeaderPath)));
  root.write("meta/ssl_data_hash", oca_string_tensor(ssl_hashes[0]));
  root.write("meta/ssl_mask_hash", oca_string_tensor(ssl_hashes[1]));
  root.write("meta/ssl_target_hash", oca_string_tensor(ssl_hashes[2]));
  root.write("meta/anchor_sha256",
             oca_string_tensor(kOcaAnchorSha256.at(seed_index)));
  root.write("meta/seed", torch::tensor({seed}, torch::kInt64));
  root.write("meta/steps", torch::tensor({kIma3Steps}, torch::kInt64));
  root.write("meta/schedule_digest",
             oca_u64_le_bytes_tensor({result.row_schedule_digest}));
  root.write("meta/pairing_counts",
             torch::tensor({result.target_exact_updates,
                            result.weak_view_exact_updates,
                            result.context_distinct_updates,
                            result.mask_count_exact_updates},
                           torch::kInt64));
  root.write("meta/result_flags",
             torch::tensor(
                 std::vector<int64_t>{result.metadata_exact ? 1LL : 0LL,
                                      result.initialization_exact ? 1LL : 0LL,
                                      result.config_isolated ? 1LL : 0LL,
                                      result.pass ? 1LL : 0LL},
                 torch::kInt64));

  for (std::size_t arm = 0; arm < 2; ++arm) {
    const std::string prefix = "arm_" + std::to_string(arm) + "/";
    torch::serialize::OutputArchive model_archive;
    result.models[arm]->save(model_archive);
    root.write(prefix + "model", model_archive);
    root.write(prefix + "name", oca_string_tensor(kIma3ArmNames[arm]));
    root.write(prefix + "config_manifest",
               oca_string_tensor(canonical_config_manifest(
                   ima3_config(device, kIma3Policies[arm]))));
    const auto &receipt = result.receipts[arm];
    root.write(
        prefix + "scalars",
        torch::tensor({receipt.loss_sum, receipt.jepa_loss_sum,
                       receipt.minimum_gradient_norm,
                       receipt.maximum_gradient_norm,
                       receipt.minimum_served_update_norm,
                       receipt.maximum_served_update_norm,
                       receipt.all_trainable_delta, receipt.served_delta,
                       receipt.predictor_delta, receipt.mae_decoder_delta,
                       receipt.vicreg_head_delta, receipt.target_ema_delta},
                      torch::kFloat64));
    root.write(prefix + "flags",
               torch::tensor(
                   std::vector<int64_t>{receipt.steps, receipt.clipping_count,
                                        receipt.finite ? 1LL : 0LL,
                                        receipt.expected_partitions ? 1LL : 0LL,
                                        receipt.pass ? 1LL : 0LL},
                   torch::kInt64));
  }
  root.save_to(temporary.string());
  const auto checksum = digest::sha256_hex(rmc_read_file(temporary));
  {
    std::ofstream out(temporary_marker, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
      throw std::runtime_error("cannot create IMA-3 cache marker");
    }
    out << checksum << '\n';
  }
  std::filesystem::remove(marker);
  std::filesystem::remove(path);
  std::filesystem::rename(temporary, path);
  std::filesystem::rename(temporary_marker, marker);
}

[[nodiscard]] bool ima3_load_seed_cache(const Dataset &ssl,
                                        const torch::Device &device,
                                        int64_t seed, std::size_t seed_index,
                                        Ima3SeedResult &result) {
  const auto path = ima3_seed_cache_path(seed);
  const auto marker = ima3_seed_cache_marker_path(path);
  if (!std::filesystem::exists(path) || !std::filesystem::exists(marker)) {
    return false;
  }
  std::string expected_checksum;
  {
    std::ifstream input(marker, std::ios::binary);
    std::getline(input, expected_checksum);
  }
  if (!expected_checksum.empty() && expected_checksum.back() == '\r') {
    expected_checksum.pop_back();
  }
  if (expected_checksum.size() != 64 ||
      digest::sha256_hex(rmc_read_file(path)) != expected_checksum) {
    throw std::runtime_error("IMA-3 cache integrity failed");
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
  const bool metadata =
      oca_tensor_string(read_tensor("meta/schema")) == kIma3CacheSchema &&
      oca_tensor_string(read_tensor("meta/source_sha256")) ==
          ima3_sha256(kIma3SourcePath) &&
      oca_tensor_string(read_tensor("meta/header_sha256")) ==
          ima3_sha256(kIma3HeaderPath) &&
      oca_tensor_string(read_tensor("meta/ssl_data_hash")) == ssl_hashes[0] &&
      oca_tensor_string(read_tensor("meta/ssl_mask_hash")) == ssl_hashes[1] &&
      oca_tensor_string(read_tensor("meta/ssl_target_hash")) == ssl_hashes[2] &&
      oca_tensor_string(read_tensor("meta/anchor_sha256")) ==
          kOcaAnchorSha256.at(seed_index) &&
      saved_seed.numel() == 1 && saved_seed.item<int64_t>() == seed &&
      saved_steps.numel() == 1 && saved_steps.item<int64_t>() == kIma3Steps &&
      schedule.size() == 1 &&
      schedule[0] == ima3_expected_row_schedule_digest(ssl, seed) &&
      counts.numel() == 4 && flags.numel() == 4;
  if (!metadata) {
    throw std::runtime_error("IMA-3 cache metadata failed");
  }

  result = Ima3SeedResult{};
  result.models.reserve(2);
  result.row_schedule_digest = schedule[0];
  result.target_exact_updates = counts[0].item<int64_t>();
  result.weak_view_exact_updates = counts[1].item<int64_t>();
  result.context_distinct_updates = counts[2].item<int64_t>();
  result.mask_count_exact_updates = counts[3].item<int64_t>();
  result.metadata_exact = flags[0].item<int64_t>() == 1;
  result.initialization_exact = flags[1].item<int64_t>() == 1;
  result.config_isolated = flags[2].item<int64_t>() == 1;
  result.pass = flags[3].item<int64_t>() == 1;
  for (std::size_t arm = 0; arm < 2; ++arm) {
    const std::string prefix = "arm_" + std::to_string(arm) + "/";
    if (oca_tensor_string(read_tensor(prefix + "name")) != kIma3ArmNames[arm] ||
        oca_tensor_string(read_tensor(prefix + "config_manifest")) !=
            canonical_config_manifest(
                ima3_config(device, kIma3Policies[arm]))) {
      throw std::runtime_error("IMA-3 cache arm identity failed");
    }
    set_paired_rng(seed, device);
    auto model = mtf::MtfJepaMaeVicreg(ima3_config(device, kIma3Policies[arm]));
    torch::serialize::InputArchive model_archive;
    root.read(prefix + "model", model_archive);
    model->load(model_archive);
    model->train();
    result.models.push_back(model);

    const auto scalars = read_tensor(prefix + "scalars")
                             .to(torch::kCPU, torch::kFloat64)
                             .contiguous();
    const auto arm_flags = read_tensor(prefix + "flags")
                               .to(torch::kCPU, torch::kInt64)
                               .contiguous();
    if (scalars.numel() != 12 || arm_flags.numel() != 5) {
      throw std::runtime_error("IMA-3 cache receipt shape failed");
    }
    auto &receipt = result.receipts[arm];
    receipt.steps = arm_flags[0].item<int64_t>();
    receipt.clipping_count = arm_flags[1].item<int64_t>();
    receipt.finite = arm_flags[2].item<int64_t>() == 1;
    receipt.expected_partitions = arm_flags[3].item<int64_t>() == 1;
    receipt.pass = arm_flags[4].item<int64_t>() == 1;
    receipt.loss_sum = scalars[0].item<double>();
    receipt.jepa_loss_sum = scalars[1].item<double>();
    receipt.minimum_gradient_norm = scalars[2].item<double>();
    receipt.maximum_gradient_norm = scalars[3].item<double>();
    receipt.minimum_served_update_norm = scalars[4].item<double>();
    receipt.maximum_served_update_norm = scalars[5].item<double>();
    receipt.all_trainable_delta = scalars[6].item<double>();
    receipt.served_delta = scalars[7].item<double>();
    receipt.predictor_delta = scalars[8].item<double>();
    receipt.mae_decoder_delta = scalars[9].item<double>();
    receipt.vicreg_head_delta = scalars[10].item<double>();
    receipt.target_ema_delta = scalars[11].item<double>();
  }
  result.resumed = true;
  result.pass = result.pass && result.metadata_exact &&
                result.initialization_exact && result.config_isolated &&
                result.target_exact_updates == kIma3Steps &&
                result.weak_view_exact_updates == kIma3Steps &&
                result.context_distinct_updates == kIma3Steps &&
                result.mask_count_exact_updates == kIma3Steps &&
                ima3_receipt_valid(result.receipts[0]) &&
                ima3_receipt_valid(result.receipts[1]);
  if (!result.pass) {
    throw std::runtime_error("IMA-3 cached seed failed reconstruction");
  }
  return true;
}

[[nodiscard]] Ima3SeedResult
ima3_train_or_resume_seed(const Dataset &ssl, const torch::Device &device,
                          int64_t seed, std::size_t seed_index) {
  Ima3SeedResult result{};
  if (!ima3_load_seed_cache(ssl, device, seed, seed_index, result)) {
    result = ima3_train_seed(ssl, device, seed, seed_index);
    if (result.pass) {
      ima3_save_seed_cache(ssl, device, seed, seed_index, result);
    }
  }
  const auto path = ima3_seed_cache_path(seed);
  std::cout << "ima3.cache.seed_" << seed << ".resumed=" << result.resumed
            << '\n';
  std::cout << "ima3.cache.seed_" << seed << ".complete="
            << (std::filesystem::exists(path) &&
                std::filesystem::exists(ima3_seed_cache_marker_path(path)))
            << '\n';
  if (std::filesystem::exists(path)) {
    std::cout << "ima3.cache.seed_" << seed
              << ".sha256=" << digest::sha256_hex(rmc_read_file(path)) << '\n';
  }
  return result;
}

void ima3_emit_arm_receipt(const std::string &root,
                           const Ima3ArmReceipt &receipt) {
  std::cout << root << ".mean_loss="
            << receipt.loss_sum / static_cast<double>(receipt.steps) << '\n';
  std::cout << root << ".mean_jepa_loss="
            << receipt.jepa_loss_sum / static_cast<double>(receipt.steps)
            << '\n';
  std::cout << root
            << ".minimum_gradient_norm=" << receipt.minimum_gradient_norm
            << '\n';
  std::cout << root
            << ".maximum_gradient_norm=" << receipt.maximum_gradient_norm
            << '\n';
  std::cout << root << ".minimum_served_update_norm="
            << receipt.minimum_served_update_norm << '\n';
  std::cout << root << ".maximum_served_update_norm="
            << receipt.maximum_served_update_norm << '\n';
  std::cout << root << ".clipping_count=" << receipt.clipping_count << '\n';
  std::cout << root << ".served_delta=" << receipt.served_delta << '\n';
  std::cout << root << ".predictor_delta=" << receipt.predictor_delta << '\n';
  std::cout << root << ".target_ema_delta=" << receipt.target_ema_delta << '\n';
  std::cout << root << ".pass=" << receipt.pass << '\n';
}

[[nodiscard]] int run_ima3_preflight(const Options &options) {
  if (options.device != "cuda" || !torch::cuda::is_available()) {
    throw std::runtime_error("IMA-3 preflight requires CUDA");
  }
  rmc_configure_cuda();
  const torch::Device device(torch::kCUDA, 0);
  auto data = rmc_make_data();
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.ima3.preflight.v1\n";
  std::cout << "experiment=dose-matched-mask-repair-preflight\n";
  std::cout << "module_only=true\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "outer_augmentation_calls=0\n";
  std::cout << "optimizer_updates=0\n";
  const auto result = ima3_run_preflight(device, data);
  ima3_emit_preflight(result);
  if (result.pass) {
    ima3_write_preflight_receipt();
  }
  std::cout << "ima3.training_authorized=" << result.pass << '\n';
  std::cout << "execution_status=ima3_preflight_complete\n";
  return result.pass ? 0 : 3;
}

[[nodiscard]] int run_ima3_trial(const Options &options) {
  if (options.device != "cuda" || !torch::cuda::is_available()) {
    throw std::runtime_error("IMA-3 quality trial requires CUDA");
  }
  if (!ima3_preflight_receipt_exact()) {
    throw std::runtime_error(
        "IMA-3 current-source preflight receipt is missing or stale");
  }
  rmc_configure_cuda();
  const torch::Device device(torch::kCUDA, 0);
  auto data = rmc_make_data();
  const auto targets = rmc_make_targets(data, false);
  const auto bootstrap_rows = rmc_bootstrap_rows(256);
  if (!rmc_bootstrap_rows_valid(bootstrap_rows, 256)) {
    throw std::runtime_error("IMA-3 bootstrap table failed");
  }
  const auto raw = rssm_probe_curve(
      data.raw_train, data.raw_validation, data.raw_development,
      data.probe_train.target, data.probe_validation.target,
      data.development.target, /*dual=*/true);
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_config_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  const std::vector<uint8_t> challenge_masks{4U, 1U, 2U, 8U, 15U};
  std::array<RmcEvaluation, 3> anchor_evaluations{};
  std::array<RmcEvaluation, 3> legacy_evaluations{};
  std::array<RmcEvaluation, 3> leaky_evaluations{};
  std::array<RmcEvaluation, 3> repaired_evaluations{};
  bool mechanics = true;

  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.ima3.quality.v1\n";
  std::cout << "experiment=dose-matched-mask-repair-quality-trial\n";
  std::cout << "module_only=true\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "outer_augmentation_calls=0\n";
  std::cout << "readout_policy=" << kOcaReadoutPolicy << '\n';
  std::cout << "new_arms=2\n";
  std::cout << "seeds=3\n";
  std::cout << "updates_per_new_arm=" << kIma3Steps << '\n';
  std::cout << "total_new_updates=" << 2 * 3 * kIma3Steps << '\n';

  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    mechanics = mechanics && ima3_pins_exact(seed_index);

    set_paired_rng(seed, device);
    auto anchor = mtf::MtfJepaMaeVicreg(ima3_config(device, kIma3Policies[0]));
    mechanics = mechanics && oca_load_archive(oca_archive_path(seed), anchor,
                                              device, seed, anchor_config_hash);
    anchor_evaluations[seed_index] = rmc_evaluate(
        anchor, data.probe_train, data.probe_validation, data.development,
        data.reversed_train, data.reversed_validation,
        data.reversed_development, targets, device);

    {
      OcaInterleavedTrainingResult cached{};
      const bool loaded = oca_load_seed_cache(
          "anchor_challenge", data.ssl, device, seed, challenge_masks,
          kIma3Steps, /*load_certified_anchor=*/true, cached);
      mechanics = mechanics && loaded && cached.pass &&
                  cached.models.size() == challenge_masks.size() &&
                  cached.receipts[1].pass;
      legacy_evaluations[seed_index] = rmc_evaluate(
          cached.models[1], data.probe_train, data.probe_validation,
          data.development, data.reversed_train, data.reversed_validation,
          data.reversed_development, targets, device);
    }

    auto trained =
        ima3_train_or_resume_seed(data.ssl, device, seed, seed_index);
    mechanics = mechanics && trained.pass;
    leaky_evaluations[seed_index] = rmc_evaluate(
        trained.models[0], data.probe_train, data.probe_validation,
        data.development, data.reversed_train, data.reversed_validation,
        data.reversed_development, targets, device);
    repaired_evaluations[seed_index] = rmc_evaluate(
        trained.models[1], data.probe_train, data.probe_validation,
        data.development, data.reversed_train, data.reversed_validation,
        data.reversed_development, targets, device);
    std::cout << "ima3.seed_" << seed
              << ".anchor_aulc=" << anchor_evaluations[seed_index].probe.area
              << '\n';
    std::cout << "ima3.seed_" << seed << ".legacy_six_target_aulc="
              << legacy_evaluations[seed_index].probe.area << '\n';
    std::cout << "ima3.seed_" << seed << ".paired_leaky_aulc="
              << leaky_evaluations[seed_index].probe.area << '\n';
    std::cout << "ima3.seed_" << seed << ".support_separated_aulc="
              << repaired_evaluations[seed_index].probe.area << '\n';
    ima3_emit_arm_receipt("ima3.seed_" + std::to_string(seed) +
                              ".paired_leaky.training",
                          trained.receipts[0]);
    ima3_emit_arm_receipt("ima3.seed_" + std::to_string(seed) +
                              ".support_separated.training",
                          trained.receipts[1]);
    std::cout << std::flush;
  }

  const auto anchor_to_legacy = oca_pair_summary(
      anchor_evaluations, legacy_evaluations, raw, data.development.target,
      targets, bootstrap_rows, mechanics);
  const auto anchor_to_leaky = oca_pair_summary(
      anchor_evaluations, leaky_evaluations, raw, data.development.target,
      targets, bootstrap_rows, mechanics);
  const auto anchor_to_repaired = oca_pair_summary(
      anchor_evaluations, repaired_evaluations, raw, data.development.target,
      targets, bootstrap_rows, mechanics);
  const auto legacy_to_leaky = oca_pair_summary(
      legacy_evaluations, leaky_evaluations, raw, data.development.target,
      targets, bootstrap_rows, mechanics);
  const auto leaky_to_repaired = oca_pair_summary(
      leaky_evaluations, repaired_evaluations, raw, data.development.target,
      targets, bootstrap_rows, mechanics);
  const auto legacy_to_repaired = oca_pair_summary(
      legacy_evaluations, repaired_evaluations, raw, data.development.target,
      targets, bootstrap_rows, mechanics);
  oca_emit_candidate_summary("ima3.anchor_to_legacy_six_target",
                             anchor_to_legacy);
  oca_emit_candidate_summary("ima3.anchor_to_paired_leaky", anchor_to_leaky);
  oca_emit_candidate_summary("ima3.anchor_to_support_separated",
                             anchor_to_repaired);
  oca_emit_candidate_summary("ima3.legacy_to_paired_leaky", legacy_to_leaky);
  oca_emit_candidate_summary("ima3.paired_leaky_to_support_separated",
                             leaky_to_repaired);
  oca_emit_candidate_summary("ima3.legacy_to_support_separated",
                             legacy_to_repaired);

  const auto &separation =
      leaky_to_repaired.candidate[0].gate.trained_minus_initialization;
  const auto &dose =
      legacy_to_leaky.candidate[0].gate.trained_minus_initialization;
  const auto &total =
      legacy_to_repaired.candidate[0].gate.trained_minus_initialization;
  const bool separation_effect =
      separation.point >= rmc_gate::kLearnedGainFloor && separation.low > 0.0 &&
      separation.positive_seed_count >= 2;
  const bool dose_effect = dose.point >= rmc_gate::kLearnedGainFloor &&
                           dose.low > 0.0 && dose.positive_seed_count >= 2;
  const bool total_effect = total.point >= rmc_gate::kLearnedGainFloor &&
                            total.low > 0.0 && total.positive_seed_count >= 2;
  const bool repaired_qualifies = anchor_to_repaired.gate.neutral.pass;
  const char *decision = "invalid_mechanics";
  if (mechanics && separation_effect && repaired_qualifies) {
    decision = "support_separation_repairs_and_improves_representation";
  } else if (mechanics && separation_effect) {
    decision = "support_leakage_is_harmful_but_repair_is_insufficient";
  } else if (mechanics && dose_effect && total_effect) {
    decision = "paired_target_topology_helps_but_support_separation_does_not";
  } else if (mechanics) {
    decision = "mask_repair_has_no_material_representation_effect";
  }
  std::cout << "ima3.mechanics_pass=" << mechanics << '\n';
  std::cout << "ima3.separation_effect_pass=" << separation_effect << '\n';
  std::cout << "ima3.target_dose_effect_pass=" << dose_effect << '\n';
  std::cout << "ima3.total_repair_effect_pass=" << total_effect << '\n';
  std::cout << "ima3.support_separated_anchor_gate_pass=" << repaired_qualifies
            << '\n';
  std::cout << "ima3.decision=" << decision << '\n';
  std::cout << "ima3.rollback_policy=legacy_soft_overlap\n";
  std::cout << "ima3.outer_augmentation_attribution_authorized=" << mechanics
            << '\n';
  std::cout << "execution_status=ima3_quality_measurements_complete\n";
  return mechanics ? 0 : 3;
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment == "dose-matched-mask-repair-preflight") {
      return run_ima3_preflight(options);
    }
    if (options.experiment == "dose-matched-mask-repair-quality-trial") {
      return run_ima3_trial(options);
    }
    throw std::runtime_error(
        "--experiment must be dose-matched-mask-repair-preflight or "
        "dose-matched-mask-repair-quality-trial");
  } catch (const c10::Error &error) {
    std::cerr << "dose_matched_mask_repair_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "dose_matched_mask_repair_error=" << error.what() << '\n';
  }
  return 2;
}
