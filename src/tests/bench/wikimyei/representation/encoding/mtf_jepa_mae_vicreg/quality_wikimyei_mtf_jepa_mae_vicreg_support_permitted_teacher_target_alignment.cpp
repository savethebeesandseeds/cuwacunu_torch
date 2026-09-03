#include "piaabo/digest/sha256.h"

// Reuse the frozen FSPA-4/OCA data, archive, and evaluation machinery.  IMA-4B
// itself remains byte-sealed; the few capture and schedule primitives needed
// below are reproduced locally and checked against its authoritative result.
#define CUWACUNU_OCA_EMBEDDED
#include "quality_wikimyei_mtf_jepa_mae_vicreg_four_objective_causal_attribution.cpp"
#undef CUWACUNU_OCA_EMBEDDED

namespace {

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
constexpr int64_t kIma4bUpdates = 1024;
constexpr int64_t kIma4bGroupBatch = 32;
constexpr int64_t kIma4bTargetsPerGroup = 2;
constexpr int64_t kIma4bSlotCount = 72;
constexpr int64_t kIma4bLatentDim = 32;
constexpr double kIma4bLearningRate = 1.0e-3;
constexpr double kIma4bGradientClip = 5.0;
constexpr double kIma4bTolerance = 2.0e-6;
constexpr double kIma4bComputeCensorDelta = 0.005;
constexpr std::array<int64_t, 13> kIma4bValidationSteps{
    0,   64,  128, 192, 256, 320, 384,
    448, 512, 640, 768, 896, 1024};

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t
ima3_config(const torch::Device &device,
            mtf::mtf_jepa_mask_policy_t policy) {
  auto config = attribution_config(device, oca_arm(1U));
  config.jepa_mask_policy = policy;
  return config;
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

template <typename ModuleImpl>
[[nodiscard]] std::shared_ptr<ModuleImpl>
ima4a_child(const mtf::MtfJepaMaeVicreg &model, const std::string &name) {
  for (const auto &item : model->named_children()) {
    if (item.key() == name) {
      auto value = std::dynamic_pointer_cast<ModuleImpl>(item.value());
      if (!value) {
        throw std::runtime_error("IMA-5A child type mismatch: " + name);
      }
      return value;
    }
  }
  throw std::runtime_error("IMA-5A child missing: " + name);
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

[[nodiscard]] double ima4a_cosine(const torch::Tensor &left_input,
                                  const torch::Tensor &right_input) {
  const auto left = left_input.to(torch::kCPU, torch::kFloat64).reshape({-1});
  const auto right =
      right_input.to(torch::kCPU, torch::kFloat64).reshape({-1});
  const double denominator =
      left.norm().item<double>() * right.norm().item<double>();
  return denominator > 1.0e-30
             ? left.dot(right).item<double>() / denominator
             : 0.0;
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

[[nodiscard]] torch::Tensor ima4a_parameter_gradient(
    const mtf::MtfJepaMaeVicreg &model,
    const std::function<bool(const std::string &)> &include) {
  std::vector<torch::Tensor> chunks;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    if (!include(item.key())) {
      continue;
    }
    chunks.push_back(
        item.value().grad().defined()
            ? item.value().grad().detach().to(torch::kCPU, torch::kFloat64)
                  .reshape({-1})
                  .clone()
            : torch::zeros({item.value().numel()}, torch::kFloat64));
  }
  if (chunks.empty()) {
    throw std::runtime_error("IMA-5A served gradient is empty");
  }
  return torch::cat(chunks).contiguous();
}

[[nodiscard]] torch::Tensor ima4a_identity_centered(
    const torch::Tensor &target_input, const torch::Tensor &identity_input) {
  const auto target = target_input.to(torch::kCPU, torch::kFloat64).contiguous();
  const auto identity =
      identity_input.to(torch::kCPU, torch::kInt64).contiguous();
  auto centered = torch::empty_like(target);
  for (int64_t token = 0; token < kIma4aTokenCount; ++token) {
    const auto rows = identity.eq(token).nonzero().reshape({-1});
    if (rows.numel() > 0) {
      const auto selected = target.index_select(0, rows);
      centered.index_copy_(0, rows, selected - selected.mean(0, true));
    }
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
    throw std::runtime_error("IMA-5A invalid target-centered metric");
  }
  const double nmse = numerator / denominator;
  return {.nmse = nmse, .r2 = 1.0 - nmse};
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

[[nodiscard]] int64_t ima4a_mask_seed(int64_t model_seed,
                                      int64_t absolute_group) {
  const auto mixed = splitmix64(
      0x696d6134615f6d73ULL ^ static_cast<uint64_t>(model_seed) ^
      splitmix64(static_cast<uint64_t>(absolute_group)));
  return static_cast<int64_t>(mixed & 0x7fffffffffffffffULL);
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
    result.target_exact = result.target_exact &&
                          torch::equal(targets[0].back(), targets[1].back());
    result.rng_exact = result.rng_exact &&
                       generator_state_snapshot_equal(post[0], post[1]);
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
  if (!result.target_exact || !result.counts_exact || !result.rng_exact) {
    throw std::runtime_error("IMA-5A group-paired mask contract failed");
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
  const auto widths =
      metadata.width.to(torch::kCPU, torch::kInt64).contiguous();
  const auto target = target_cpu.accessor<int64_t, 1>();
  const auto channel = channels.accessor<int64_t, 1>();
  const auto start = starts.accessor<int64_t, 1>();
  const auto width = widths.accessor<int64_t, 1>();
  auto closure = torch::zeros({target_cpu.size(0), channels.size(0)},
                              torch::kBool);
  auto output = closure.accessor<bool, 2>();
  for (int64_t row = 0; row < target_cpu.size(0); ++row) {
    const int64_t selected = target[row];
    for (int64_t token = 0; token < channels.size(0); ++token) {
      output[row][token] =
          channel[token] == channel[selected] &&
          std::max(start[token], start[selected]) <
              std::min(start[token] + width[token],
                       start[selected] + width[selected]);
    }
  }
  return closure.to(device, torch::kBool);
}

[[nodiscard]] torch::Tensor ima4a_group_resample(
    const torch::Tensor &value, const torch::Tensor &group_rows) {
  if (value.size(0) % kIma4aTargetsPerGroup != 0) {
    throw std::runtime_error("IMA-5A group resample shape failed");
  }
  auto shape = value.sizes().vec();
  shape[0] = value.size(0) / kIma4aTargetsPerGroup;
  shape.insert(shape.begin() + 1, kIma4aTargetsPerGroup);
  auto selected = value.reshape(shape).index_select(0, group_rows);
  auto output_shape = value.sizes().vec();
  output_shape[0] = group_rows.size(0) * kIma4aTargetsPerGroup;
  return selected.reshape(output_shape).contiguous();
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
    throw std::runtime_error("IMA-5A target flatten contract failed");
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
      throw std::runtime_error("IMA-5A online/teacher layout failed");
    }
    const auto pair = ima4a_group_paired_masks(
        online, seed, dataset.group_begin + begin, config, device);
    pairing = pairing && pair.target_exact && pair.counts_exact &&
              pair.rng_exact;
    const auto &masks = pair.masks[1];
    const auto locations = masks.target_mask.nonzero();
    if (locations.sizes() !=
        torch::IntArrayRef({size * kIma4bTargetsPerGroup, 2})) {
      throw std::runtime_error("IMA-5A selected-target shape failed");
    }
    const auto target_rows = locations.select(1, 0).contiguous();
    const auto target_indices = locations.select(1, 1).contiguous();
    const auto expected_rows =
        ima4b_repeated_group_rows(size).to(device, torch::kInt64);
    if (!torch::equal(target_rows, expected_rows)) {
      throw std::runtime_error("IMA-5A selected-target grouping failed");
    }
    const auto full_target =
        modules.target_encoder->forward(teacher.tokens, teacher.token_mask);
    const auto selected_target = full_target.index(
        {target_rows, target_indices, torch::indexing::Slice()});
    const auto context_tokens = online.tokens.masked_fill(
        masks.context_mask.logical_not().unsqueeze(-1), 0.0);
    const auto context_latents =
        modules.encoder->forward(context_tokens, masks.context_mask);
    const auto prediction = modules.predictor->forward(
        context_latents, masks.context_mask, online.metadata);
    const auto selected_prediction = prediction.index(
        {target_rows, target_indices, torch::indexing::Slice()});
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
    throw std::runtime_error("IMA-5A frozen capture contract failed");
  }
  result.hash = ima4b_capture_hash(result);
  return result;
}

[[nodiscard]] std::vector<int64_t>
ima4b_training_groups(int64_t groups, int64_t seed, int64_t step) {
  if (groups <= 0 || groups % kIma4bGroupBatch != 0 || step < 0) {
    throw std::runtime_error("IMA-5A training schedule contract failed");
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

[[nodiscard]] bool ima4b_frozen_gradients_clear(
    const mtf::MtfJepaMaeVicreg &model) {
  const auto parameters = model->named_parameters(/*recurse=*/true);
  return std::all_of(parameters.begin(), parameters.end(),
                     [](const auto &item) {
                       return !item.value().grad().defined();
                     });
}


constexpr int64_t kIma5aTeacherFitBegin = 6000000;
constexpr int64_t kIma5aTeacherFitGroups = 256;
constexpr int64_t kIma5aTeacherSelectionBegin = 7000000;
constexpr int64_t kIma5aTeacherSelectionGroups = 128;
constexpr int64_t kIma5aConfirmationBegin = 8000000;
constexpr int64_t kIma5aConfirmationGroups = 256;
constexpr int64_t kIma5aDiagnosticGroups = 96;
constexpr double kIma5aVarianceFloor = 1.0e-8;
constexpr double kIma5aRankFloor = 4.0;
constexpr double kIma5aTopShareCeiling = 0.75;
constexpr double kIma5aSlotShareCeiling = 0.50;
constexpr double kIma5aChannelShareFloor = 0.01;
constexpr double kIma5aPredictorMeanFloor = 0.50;
constexpr double kIma5aPredictorSeedFloor = 0.25;
constexpr double kIma5aGradientCosineFloor = -0.05;
constexpr std::string_view kIma5aProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "SUPPORT_PERMITTED_TEACHER_TARGET_ALIGNMENT_PROTOCOL.md";
constexpr std::string_view kIma5aProtocolSha256 =
    "a44654c7a1d4499b7884555b985bd502abce4d4ae53a8a6cf323eb7981fc1d17";
constexpr std::string_view kIma5aAmendmentA1Path =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "SUPPORT_PERMITTED_TEACHER_TARGET_ALIGNMENT_PROTOCOL_AMENDMENT_A1.md";
constexpr std::string_view kIma5aAmendmentA1Sha256 =
    "a325c187f57adff9eca184055da9efde2fcc08420c71455cf06f837d6db409ef";
constexpr std::string_view kIma5aAmendmentA2Path =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "SUPPORT_PERMITTED_TEACHER_TARGET_ALIGNMENT_PROTOCOL_AMENDMENT_A2.md";
constexpr std::string_view kIma5aAmendmentA2Sha256 =
    "7b0ad04e927415522396a43e793c437a86991226d850c29f158fe2dbb62f533b";
constexpr std::string_view kIma5aSourcePath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_"
    "support_permitted_teacher_target_alignment.cpp";
constexpr std::string_view kIma5aIma4cSourcePath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_"
    "canonical_slot_interaction_sufficiency.cpp";
constexpr std::string_view kIma5aIma4cProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "JEPA_CANONICAL_SLOT_INTERACTION_SUFFICIENCY_PROTOCOL.md";
constexpr std::string_view kIma5aIma4cLogPath =
    ".build/tests/representation_ima4c_v1_authoritative.log";
constexpr std::string_view kIma5aIma4cSourceSha256 =
    "38a8792ea48ecb5aa319921c7402fab2bdca426eb29239d3d027e274fc4b95ef";
constexpr std::string_view kIma5aIma4cProtocolSha256 =
    "885c16d3f40cb57d291a2ba4481ed264166765e318e1189b67f8267b7f72c667";
constexpr std::string_view kIma5aIma4cLogSha256 =
    "dd145f8f2b5a314e9850bb4c1860e276740e3e0aaeac4dad897adba539eb9196";

struct Ima5aData {
  RmcData inherited{};
  Dataset teacher_fit{};
  Dataset teacher_selection{};
};

[[nodiscard]] Ima5aData ima5a_make_data() {
  Ima5aData result{};
  result.inherited = rmc_make_data();
  result.teacher_fit =
      generate_dataset(kIma5aTeacherFitBegin, kIma5aTeacherFitGroups);
  result.teacher_selection = generate_dataset(
      kIma5aTeacherSelectionBegin, kIma5aTeacherSelectionGroups);
  for (Dataset *dataset : {&result.teacher_fit, &result.teacher_selection}) {
    normalize(*dataset, result.inherited.normalization);
    validate_dataset(*dataset);
  }
  return result;
}

struct Ima5aAuxiliaryCapture {
  torch::Tensor support0{}; // [G,2,32], CPU float32
  torch::Tensor channel{};  // [G,2], CPU int64
  torch::Tensor domain{};   // [G,2], CPU int64
  torch::Tensor scale{};    // [G,2], CPU int64
  uint64_t hash{0};
  bool exact{false};
};

struct Ima5aSplit {
  Ima4bFrozenSplit frozen{};
  Ima5aAuxiliaryCapture auxiliary{};
};

[[nodiscard]] Ima5aAuxiliaryCapture ima5a_capture_auxiliary(
    const Ima4aModules &modules, const Dataset &dataset,
    const mtf::mtf_jepa_mae_vicreg_config_t &config,
    const torch::Device &device, int64_t seed,
    const Ima4bFrozenSplit &reference) {
  std::vector<torch::Tensor> support0;
  std::vector<torch::Tensor> channels;
  std::vector<torch::Tensor> domains;
  std::vector<torch::Tensor> scales;
  std::vector<torch::Tensor> slots;
  torch::NoGradGuard no_grad;
  for (int64_t begin = 0; begin < dataset.data.size(0);
       begin += kModelRowBatchSize) {
    const int64_t size = std::min<int64_t>(
        kModelRowBatchSize, dataset.data.size(0) - begin);
    const auto data = dataset.data.narrow(0, begin, size).to(device);
    const auto feature_mask = dataset.mask.narrow(0, begin, size).to(device);
    const auto online = modules.tokenizer->forward(data, feature_mask);
    const auto teacher = modules.target_tokenizer->forward(data, feature_mask);
    const auto pair = ima4a_group_paired_masks(
        online, seed, dataset.group_begin + begin, config, device);
    const auto &masks = pair.masks[1];
    const auto locations = masks.target_mask.nonzero();
    const auto target_rows = locations.select(1, 0).contiguous();
    const auto target_slots = locations.select(1, 1).contiguous();
    const auto repeated_tokens = teacher.tokens.index_select(0, target_rows);
    const auto repeated_mask = teacher.token_mask.index_select(0, target_rows);
    const auto closure =
        ima4a_support_closure(teacher.metadata, target_slots, device);
    if (closure.logical_and(
                    masks.context_mask.index_select(0, target_rows))
            .any()
            .item<bool>()) {
      throw std::runtime_error("IMA-5A hidden support intersects M2 context");
    }
    const auto zeroed =
        repeated_tokens.masked_fill(closure.unsqueeze(-1), 0.0);
    const auto field = modules.target_encoder->forward(zeroed, repeated_mask);
    const auto selected_rows = torch::arange(
        target_slots.size(0),
        torch::TensorOptions().dtype(torch::kInt64).device(device));
    support0.push_back(
        field.index({selected_rows, target_slots, torch::indexing::Slice()})
            .reshape({size, kIma4bTargetsPerGroup, kIma4bLatentDim})
            .detach()
            .to(torch::kCPU, torch::kFloat32)
            .contiguous()
            .clone());
    channels.push_back(
        teacher.metadata.channel_id.to(device)
            .index_select(0, target_slots)
            .reshape({size, kIma4bTargetsPerGroup})
            .to(torch::kCPU, torch::kInt64)
            .contiguous());
    domains.push_back(
        teacher.metadata.domain_id.to(device)
            .index_select(0, target_slots)
            .reshape({size, kIma4bTargetsPerGroup})
            .to(torch::kCPU, torch::kInt64)
            .contiguous());
    scales.push_back(
        teacher.metadata.scale_id.to(device)
            .index_select(0, target_slots)
            .reshape({size, kIma4bTargetsPerGroup})
            .to(torch::kCPU, torch::kInt64)
            .contiguous());
    slots.push_back(target_slots.reshape({size, kIma4bTargetsPerGroup})
                        .to(torch::kCPU, torch::kInt64)
                        .contiguous());
  }
  Ima5aAuxiliaryCapture result{};
  result.support0 = torch::cat(support0, 0).contiguous();
  result.channel = torch::cat(channels, 0).contiguous();
  result.domain = torch::cat(domains, 0).contiguous();
  result.scale = torch::cat(scales, 0).contiguous();
  const auto captured_slots = torch::cat(slots, 0).contiguous();
  result.exact =
      torch::equal(captured_slots, reference.target_slot) &&
      result.support0.sizes() == reference.target.sizes() &&
      torch::isfinite(result.support0).all().item<bool>();
  uint64_t hash = 0xcbf29ce484222325ULL;
  for (const auto &value : {result.support0, result.channel, result.domain,
                            result.scale, captured_slots}) {
    mix_hash_value(hash, hash_tensor_stable_bytes(value));
  }
  result.hash = hash;
  if (!result.exact) {
    throw std::runtime_error("IMA-5A auxiliary capture contract failed");
  }
  return result;
}

[[nodiscard]] Ima5aSplit ima5a_capture_split(
    const Ima4aModules &modules, const Dataset &dataset,
    const mtf::mtf_jepa_mae_vicreg_config_t &config,
    const torch::Device &device, int64_t seed) {
  Ima5aSplit result{};
  result.frozen =
      ima4b_capture_split(modules, dataset, config, device, seed);
  result.auxiliary = ima5a_capture_auxiliary(
      modules, dataset, config, device, seed, result.frozen);
  return result;
}

[[nodiscard]] torch::Tensor ima5a_eligible_target_slots(
    const mtf::mtf_token_metadata_t &metadata) {
  const auto channel =
      metadata.channel_id.to(torch::kCPU, torch::kInt64).contiguous();
  const auto domain =
      metadata.domain_id.to(torch::kCPU, torch::kInt64).contiguous();
  const auto scale =
      metadata.scale_id.to(torch::kCPU, torch::kInt64).contiguous();
  const auto start =
      metadata.start_index.to(torch::kCPU, torch::kInt64).contiguous();
  const auto width =
      metadata.width.to(torch::kCPU, torch::kInt64).contiguous();
  if (channel.sizes() != torch::IntArrayRef({kIma4bSlotCount}) ||
      domain.sizes() != channel.sizes() || scale.sizes() != channel.sizes() ||
      start.sizes() != channel.sizes() || width.sizes() != channel.sizes()) {
    throw std::runtime_error("IMA-5A target eligibility metadata failed");
  }
  const auto channel_value = channel.accessor<int64_t, 1>();
  const auto domain_value = domain.accessor<int64_t, 1>();
  const auto scale_value = scale.accessor<int64_t, 1>();
  const auto start_value = start.accessor<int64_t, 1>();
  const auto width_value = width.accessor<int64_t, 1>();
  int64_t minimum_width = std::numeric_limits<int64_t>::max();
  std::vector<std::pair<int64_t, int64_t>> pairs;
  for (int64_t time = 0; time < kIma4bSlotCount; ++time) {
    if (domain_value[time] != 0) {
      continue;
    }
    for (int64_t frequency = 0; frequency < kIma4bSlotCount; ++frequency) {
      if (domain_value[frequency] != 1 ||
          channel_value[time] != channel_value[frequency] ||
          scale_value[time] != scale_value[frequency] ||
          start_value[time] != start_value[frequency] ||
          width_value[time] != width_value[frequency]) {
        continue;
      }
      if (width_value[time] < minimum_width) {
        minimum_width = width_value[time];
        pairs.clear();
      }
      if (width_value[time] == minimum_width) {
        pairs.emplace_back(time, frequency);
      }
    }
  }
  if (pairs.empty()) {
    throw std::runtime_error("IMA-5A target eligibility is empty");
  }
  auto eligible = torch::zeros({kIma4bSlotCount}, torch::kBool);
  auto eligible_value = eligible.accessor<bool, 1>();
  for (const auto &[time, frequency] : pairs) {
    eligible_value[time] = true;
    eligible_value[frequency] = true;
  }
  return eligible.contiguous();
}

[[nodiscard]] bool ima5a_eligible_slot_coverage(
    const torch::Tensor &slot_input,
    const torch::Tensor &eligible_input) {
  const auto slot = slot_input.to(torch::kCPU, torch::kInt64).contiguous();
  const auto eligible =
      eligible_input.to(torch::kCPU, torch::kBool).contiguous();
  if (slot.dim() != 1 ||
      eligible.sizes() != torch::IntArrayRef({kIma4bSlotCount})) {
    return false;
  }
  const auto selected = slot.accessor<int64_t, 1>();
  const auto allowed = eligible.accessor<bool, 1>();
  std::array<int64_t, kIma4bSlotCount> counts{};
  for (int64_t row = 0; row < slot.size(0); ++row) {
    if (selected[row] < 0 || selected[row] >= kIma4bSlotCount ||
        !allowed[selected[row]]) {
      return false;
    }
    ++counts[static_cast<std::size_t>(selected[row])];
  }
  for (int64_t identity = 0; identity < kIma4bSlotCount; ++identity) {
    if (allowed[identity] && counts[static_cast<std::size_t>(identity)] == 0) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] torch::Tensor ima5a_slot_means(
    const torch::Tensor &target_input, const torch::Tensor &slot_input,
    bool require_all) {
  const auto target = target_input.to(torch::kCPU, torch::kFloat64).contiguous();
  const auto slot = slot_input.to(torch::kCPU, torch::kInt64).contiguous();
  auto means = torch::zeros({kIma4bSlotCount, target.size(1)},
                            torch::TensorOptions().dtype(torch::kFloat64));
  for (int64_t identity = 0; identity < kIma4bSlotCount; ++identity) {
    const auto rows = slot.eq(identity).nonzero().reshape({-1});
    if (rows.numel() == 0) {
      if (require_all) {
        throw std::runtime_error("IMA-5A calibration omits a target slot");
      }
      continue;
    }
    means.select(0, identity).copy_(
        target.index_select(0, rows).mean(0));
  }
  return means.contiguous();
}

[[nodiscard]] torch::Tensor ima5a_center_by_reference_means(
    const torch::Tensor &target, const torch::Tensor &slot,
    const torch::Tensor &means) {
  return target.to(torch::kCPU, torch::kFloat64) -
         means.index_select(0, slot.to(torch::kCPU, torch::kInt64));
}

struct Ima5aTeacher {
  torch::Tensor feature_mean{};
  torch::Tensor feature_inv_std{};
  torch::Tensor train_x{};
  torch::Tensor train_field{};
  torch::Tensor train_slot{};
  torch::Tensor train_scale{};
  torch::Tensor slot_mean{};
  torch::Tensor eligible_slot_mask{};
  torch::Tensor residual_bias{};
  torch::Tensor dual{};
  std::array<double, kIma4aRidgeGrid.size()> validation_nmse{};
  std::size_t alpha_index{0};
  double alpha{0.0};
  double calibration_validation_r2{0.0};
  int64_t eligible_slot_count{0};
  uint64_t hash{0};
  bool all_eligible_slots_present{false};
  bool edge_closed{false};
  bool finite{false};
};

[[nodiscard]] torch::Tensor ima5a_identity_scale(
    const torch::Tensor &fit_slot_input, const torch::Tensor &slot_input) {
  const auto fit_slot =
      fit_slot_input.to(torch::kCPU, torch::kInt64).contiguous();
  const auto slot = slot_input.to(torch::kCPU, torch::kInt64).contiguous();
  std::array<int64_t, kIma4bSlotCount> counts{};
  const auto fit = fit_slot.accessor<int64_t, 1>();
  for (int64_t row = 0; row < fit_slot.size(0); ++row) {
    if (fit[row] < 0 || fit[row] >= kIma4bSlotCount) {
      throw std::runtime_error("IMA-5A target identity out of range");
    }
    ++counts[static_cast<std::size_t>(fit[row])];
  }
  std::vector<double> values;
  values.reserve(static_cast<std::size_t>(slot.size(0)));
  const auto selected = slot.accessor<int64_t, 1>();
  for (int64_t row = 0; row < slot.size(0); ++row) {
    const int64_t identity = selected[row];
    if (identity < 0 || identity >= kIma4bSlotCount ||
        counts[static_cast<std::size_t>(identity)] == 0) {
      throw std::runtime_error("IMA-5A unseen target identity");
    }
    values.push_back(std::sqrt(
        static_cast<double>(fit_slot.size(0)) /
        static_cast<double>(counts[static_cast<std::size_t>(identity)])));
  }
  return torch::tensor(values, torch::kFloat64);
}

[[nodiscard]] torch::Tensor ima5a_teacher_kernel(
    const Ima5aTeacher &teacher, const torch::Tensor &features_input,
    const torch::Tensor &slot_input) {
  const auto features =
      features_input.to(torch::kCPU, torch::kFloat64).contiguous();
  const auto slot = slot_input.to(torch::kCPU, torch::kInt64).contiguous();
  const auto x = (features - teacher.feature_mean) * teacher.feature_inv_std;
  auto kernel = x.matmul(teacher.train_x.transpose(0, 1));
  const auto field = x.slice(1, kIma4aMetadataWidth);
  const auto scale = ima5a_identity_scale(teacher.train_slot, slot);
  const auto same =
      slot.unsqueeze(1).eq(teacher.train_slot.unsqueeze(0)).to(torch::kFloat64);
  kernel = kernel + field.matmul(teacher.train_field.transpose(0, 1)) * same *
                        scale.unsqueeze(1) * teacher.train_scale.unsqueeze(0);
  return kernel.contiguous();
}

[[nodiscard]] torch::Tensor ima5a_teacher_predict(
    const Ima5aTeacher &teacher, const Ima4bFrozenSplit &split) {
  const auto features = ima4b_c_features(split);
  const auto slot = ima4b_flatten_targets(split.target_slot);
  return (ima5a_teacher_kernel(teacher, features, slot).matmul(teacher.dual) +
          teacher.residual_bias)
      .contiguous();
}

[[nodiscard]] Ima5aTeacher ima5a_fit_teacher(
    const Ima4bFrozenSplit &fit, const Ima4bFrozenSplit &selection,
    const torch::Tensor &eligible_slot_mask) {
  Ima5aTeacher result{};
  const auto fit_target =
      ima4b_flatten_targets(fit.target).to(torch::kCPU, torch::kFloat64);
  const auto fit_slot = ima4b_flatten_targets(fit.target_slot)
                            .to(torch::kCPU, torch::kInt64);
  const auto selection_target = ima4b_flatten_targets(selection.target)
                                    .to(torch::kCPU, torch::kFloat64);
  const auto selection_slot = ima4b_flatten_targets(selection.target_slot)
                                  .to(torch::kCPU, torch::kInt64);
  result.eligible_slot_mask =
      eligible_slot_mask.to(torch::kCPU, torch::kBool).contiguous();
  result.eligible_slot_count =
      result.eligible_slot_mask.sum().item<int64_t>();
  result.all_eligible_slots_present =
      ima5a_eligible_slot_coverage(fit_slot, result.eligible_slot_mask);
  if (!result.all_eligible_slots_present) {
    throw std::runtime_error(
        "IMA-5A calibration omits an eligible target slot");
  }
  result.slot_mean = ima5a_slot_means(fit_target, fit_slot, false);
  const auto fit_residual = ima5a_center_by_reference_means(
      fit_target, fit_slot, result.slot_mean);
  const auto selection_residual = ima5a_center_by_reference_means(
      selection_target, selection_slot, result.slot_mean);
  const auto fit_features = ima4b_c_features(fit);
  result.feature_mean = fit_features.mean(0);
  const auto variance =
      (fit_features - result.feature_mean).pow(2).mean(0);
  result.feature_inv_std = torch::where(
      variance > 1.0e-12, variance.rsqrt(), torch::ones_like(variance));
  result.train_x =
      ((fit_features - result.feature_mean) * result.feature_inv_std)
          .contiguous();
  result.train_field =
      result.train_x.slice(1, kIma4aMetadataWidth).contiguous();
  result.train_slot = fit_slot.contiguous();
  result.train_scale = ima5a_identity_scale(fit_slot, fit_slot);
  result.residual_bias = fit_residual.mean(0).contiguous();
  const auto y = fit_residual - result.residual_bias;
  auto gram = result.train_x.matmul(result.train_x.transpose(0, 1));
  const auto same = fit_slot.unsqueeze(1)
                        .eq(fit_slot.unsqueeze(0))
                        .to(torch::kFloat64);
  gram = gram +
         result.train_field.matmul(result.train_field.transpose(0, 1)) * same *
             result.train_scale.unsqueeze(1) *
             result.train_scale.unsqueeze(0);
  const auto selection_kernel = ima5a_teacher_kernel(
      result, ima4b_c_features(selection), selection_slot);
  double best = std::numeric_limits<double>::infinity();
  std::array<torch::Tensor, kIma4aRidgeGrid.size()> duals{};
  for (std::size_t index = 0; index < kIma4aRidgeGrid.size(); ++index) {
    auto regularized = gram.clone();
    regularized.diagonal(0, 0, 1).add_(
        fit_target.size(0) * kIma4aRidgeGrid[index]);
    auto [cholesky, info] =
        at::linalg_cholesky_ex(regularized, false, false);
    if (info.max().item<int64_t>() != 0) {
      throw std::runtime_error("IMA-5A teacher factorization failed");
    }
    duals[index] = at::cholesky_solve(y, cholesky, false);
    const auto prediction =
        selection_kernel.matmul(duals[index]) + result.residual_bias;
    result.validation_nmse[index] =
        ima4a_oracle_metric(prediction, selection_residual, selection_slot)
            .nmse;
    if (result.validation_nmse[index] < best) {
      best = result.validation_nmse[index];
      result.alpha_index = index;
    }
  }
  result.alpha = kIma4aRidgeGrid[result.alpha_index];
  result.dual = duals[result.alpha_index].contiguous();
  const auto selection_prediction =
      selection_kernel.matmul(result.dual) + result.residual_bias;
  result.calibration_validation_r2 =
      ima4a_oracle_metric(selection_prediction, selection_residual,
                          selection_slot)
          .r2;
  const double intercept_nmse = ima4a_oracle_metric(
                                    result.residual_bias.unsqueeze(0).expand_as(
                                        selection_residual),
                                    selection_residual, selection_slot)
                                    .nmse;
  result.edge_closed =
      !ima4a_edge_status(result.validation_nmse, result.alpha_index,
                         intercept_nmse)
           .improving;
  uint64_t hash = 0xcbf29ce484222325ULL;
  for (const auto &value :
       {result.feature_mean, result.feature_inv_std, result.train_x,
        result.train_slot, result.train_scale, result.slot_mean,
        result.eligible_slot_mask, result.residual_bias, result.dual}) {
    mix_hash_value(hash, hash_tensor_stable_bytes(value));
  }
  result.hash = hash;
  result.finite = result.all_eligible_slots_present && result.edge_closed &&
                  torch::isfinite(result.feature_mean).all().item<bool>() &&
                  torch::isfinite(result.feature_inv_std).all().item<bool>() &&
                  torch::isfinite(result.dual).all().item<bool>() &&
                  std::isfinite(result.calibration_validation_r2);
  if (!result.finite) {
    throw std::runtime_error("IMA-5A teacher fit failed");
  }
  return result;
}

[[nodiscard]] Ima4bFrozenSplit ima5a_candidate_split(
    const Ima4bFrozenSplit &source, const torch::Tensor &candidate_flat) {
  Ima4bFrozenSplit result = source;
  result.target = candidate_flat
                      .reshape({source.context.size(0), kIma4bTargetsPerGroup,
                                kIma4bLatentDim})
                      .to(torch::kCPU, torch::kFloat32)
                      .contiguous()
                      .clone();
  result.hash = ima4b_capture_hash(result);
  result.finite = result.finite &&
                  torch::isfinite(result.target).all().item<bool>();
  return result;
}

struct Ima5aTargetStats {
  double total_variance{0.0};
  double within_slot_variance{0.0};
  double slot_variance_share{1.0};
  double effective_rank{0.0};
  double participation_rank{0.0};
  double top_eigenvalue_share{1.0};
  std::array<double, 2> domain_variance{};
  std::array<double, kChannels> channel_variance{};
  std::array<double, kChannels> channel_variance_share{};
  bool pass{false};
};

[[nodiscard]] Ima5aTargetStats ima5a_target_stats(
    const torch::Tensor &value_input, const torch::Tensor &slot_input,
    const torch::Tensor &domain_input, const torch::Tensor &channel_input,
    int64_t domain_filter = -1) {
  auto value = value_input.to(torch::kCPU, torch::kFloat64).contiguous();
  auto slot = slot_input.to(torch::kCPU, torch::kInt64).contiguous();
  auto domain =
      domain_input.to(torch::kCPU, torch::kInt64).contiguous();
  auto channel =
      channel_input.to(torch::kCPU, torch::kInt64).contiguous();
  if (value.dim() != 2 || value.size(1) != kIma4bLatentDim ||
      slot.sizes() != torch::IntArrayRef({value.size(0)}) ||
      domain.sizes() != slot.sizes() || channel.sizes() != slot.sizes()) {
    throw std::runtime_error("IMA-5A target statistics shape failed");
  }
  if (domain_filter >= 0) {
    if (domain_filter > 1) {
      throw std::runtime_error("IMA-5A target statistics domain failed");
    }
    const auto rows = domain.eq(domain_filter).nonzero().reshape({-1});
    value = value.index_select(0, rows).contiguous();
    slot = slot.index_select(0, rows).contiguous();
    domain = domain.index_select(0, rows).contiguous();
    channel = channel.index_select(0, rows).contiguous();
  }
  const auto global_centered = value - value.mean(0, true);
  const auto slot_mean = ima5a_slot_means(value, slot, false);
  const auto centered = ima5a_center_by_reference_means(value, slot, slot_mean);
  const double total_energy = global_centered.pow(2).sum().item<double>();
  const double within_energy = centered.pow(2).sum().item<double>();
  if (!(total_energy > 0.0)) {
    return {};
  }
  const auto covariance = centered.transpose(0, 1).matmul(centered) /
                          static_cast<double>(
                              std::max<int64_t>(1, centered.size(0) - 1));
  const auto eigenvalues =
      at::linalg_eigvalsh(covariance, "L").clamp_min(0.0);
  const double spectrum_total = eigenvalues.sum().item<double>();
  Ima5aTargetStats result{};
  result.total_variance = total_energy / static_cast<double>(value.numel());
  result.within_slot_variance =
      within_energy / static_cast<double>(value.numel());
  result.slot_variance_share =
      std::clamp((total_energy - within_energy) / total_energy, 0.0, 1.0);
  if (spectrum_total > 1.0e-20) {
    const auto probabilities = eigenvalues / spectrum_total;
    const double entropy =
        -(probabilities * probabilities.clamp_min(1.0e-30).log())
             .sum()
             .item<double>();
    result.effective_rank = std::exp(entropy);
    result.participation_rank =
        spectrum_total * spectrum_total /
        eigenvalues.pow(2).sum().clamp_min(1.0e-30).item<double>();
    result.top_eigenvalue_share =
        eigenvalues.max().item<double>() / spectrum_total;
  }
  for (int64_t identity = 0; identity < 2; ++identity) {
    const auto rows = domain.eq(identity).nonzero().reshape({-1});
    if (rows.numel() > 0) {
      result.domain_variance[static_cast<std::size_t>(identity)] =
          centered.index_select(0, rows).pow(2).mean().item<double>();
    }
  }
  for (int64_t identity = 0; identity < kChannels; ++identity) {
    const auto rows = channel.eq(identity).nonzero().reshape({-1});
    if (rows.numel() > 0) {
      const double energy =
          centered.index_select(0, rows).pow(2).sum().item<double>();
      result.channel_variance[static_cast<std::size_t>(identity)] =
          energy /
          static_cast<double>(rows.numel() * kIma4bLatentDim);
      result.channel_variance_share[static_cast<std::size_t>(identity)] =
          energy / std::max(within_energy, 1.0e-30);
    }
  }
  const bool domains_pass =
      domain_filter < 0
          ? result.domain_variance[0] > kIma5aVarianceFloor &&
                result.domain_variance[1] > kIma5aVarianceFloor
          : result.domain_variance[static_cast<std::size_t>(domain_filter)] >
                kIma5aVarianceFloor;
  result.pass = result.total_variance > kIma5aVarianceFloor &&
                result.within_slot_variance > kIma5aVarianceFloor &&
                result.effective_rank >= kIma5aRankFloor &&
                result.participation_rank >= kIma5aRankFloor &&
                result.top_eigenvalue_share < kIma5aTopShareCeiling &&
                result.slot_variance_share < kIma5aSlotShareCeiling &&
                domains_pass;
  for (std::size_t identity = 0; identity < kChannels; ++identity) {
    result.pass =
        result.pass &&
        result.channel_variance[identity] > kIma5aVarianceFloor &&
        result.channel_variance_share[identity] >= kIma5aChannelShareFloor;
  }
  return result;
}

struct Ima5aPredictorSnapshot {
  std::vector<std::string> names{};
  std::vector<torch::Tensor> values{};
};

[[nodiscard]] Ima5aPredictorSnapshot
ima5a_snapshot_predictor(const mtf::LatentPredictor &predictor) {
  Ima5aPredictorSnapshot result{};
  torch::NoGradGuard no_grad;
  for (const auto &item : predictor->named_parameters(/*recurse=*/true)) {
    result.names.push_back(item.key());
    result.values.push_back(
        item.value().detach().to(torch::kCPU).contiguous().clone());
  }
  return result;
}

void ima5a_restore_predictor(mtf::LatentPredictor &predictor,
                             const Ima5aPredictorSnapshot &snapshot) {
  const auto parameters = predictor->named_parameters(/*recurse=*/true);
  if (parameters.size() != snapshot.values.size()) {
    throw std::runtime_error("IMA-5A predictor restore count failed");
  }
  torch::NoGradGuard no_grad;
  std::size_t index = 0;
  for (const auto &item : parameters) {
    if (item.key() != snapshot.names[index] ||
        item.value().sizes() != snapshot.values[index].sizes()) {
      throw std::runtime_error("IMA-5A predictor restore layout failed");
    }
    item.value().copy_(snapshot.values[index].to(item.value().device()));
    ++index;
  }
}

void ima5a_copy_predictor(
    const std::shared_ptr<mtf::LatentPredictorImpl> &source,
    mtf::LatentPredictor &destination) {
  const auto source_parameters = source->named_parameters(/*recurse=*/true);
  const auto destination_parameters =
      destination->named_parameters(/*recurse=*/true);
  if (source_parameters.size() != destination_parameters.size()) {
    throw std::runtime_error("IMA-5A production predictor count failed");
  }
  torch::NoGradGuard no_grad;
  for (const auto &source_item : source_parameters) {
    auto *destination_item = destination_parameters.find(source_item.key());
    if (destination_item == nullptr ||
        destination_item->sizes() != source_item.value().sizes()) {
      throw std::runtime_error("IMA-5A production predictor layout failed");
    }
    destination_item->copy_(source_item.value());
  }
}

[[nodiscard]] double ima5a_predictor_delta(
    const mtf::LatentPredictor &predictor,
    const Ima5aPredictorSnapshot &reference) {
  const auto parameters = predictor->named_parameters(/*recurse=*/true);
  if (parameters.size() != reference.values.size()) {
    return std::numeric_limits<double>::infinity();
  }
  double maximum = 0.0;
  std::size_t index = 0;
  for (const auto &item : parameters) {
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

[[nodiscard]] torch::Tensor ima5a_selected_prediction(
    mtf::LatentPredictor &predictor, const torch::Tensor &context,
    const torch::Tensor &context_mask, const torch::Tensor &target_slot,
    const mtf::mtf_token_metadata_t &metadata) {
  const auto prediction = predictor->forward(context, context_mask, metadata);
  return prediction.gather(
      1, target_slot.unsqueeze(-1).expand(
             {target_slot.size(0), target_slot.size(1), kIma4bLatentDim}));
}

[[nodiscard]] torch::Tensor ima5a_predict_split(
    mtf::LatentPredictor &predictor, const Ima4bFrozenSplit &split,
    const mtf::mtf_token_metadata_t &metadata, const torch::Device &device) {
  const bool was_training = predictor->is_training();
  predictor->eval();
  torch::NoGradGuard no_grad;
  std::vector<torch::Tensor> chunks;
  for (int64_t begin = 0; begin < split.context.size(0);
       begin += kIma4bGroupBatch) {
    const int64_t size = std::min<int64_t>(
        kIma4bGroupBatch, split.context.size(0) - begin);
    chunks.push_back(
        ima5a_selected_prediction(
            predictor,
            split.context.narrow(0, begin, size).to(device, torch::kFloat32),
            split.context_mask.narrow(0, begin, size)
                .to(device, torch::kBool),
            split.target_slot.narrow(0, begin, size)
                .to(device, torch::kInt64),
            metadata)
            .detach()
            .to(torch::kCPU, torch::kFloat32)
            .contiguous());
  }
  predictor->train(was_training);
  return torch::cat(chunks, 0).contiguous();
}

[[nodiscard]] Ima4aOracleMetric ima5a_metric(
    const torch::Tensor &prediction, const Ima4bFrozenSplit &split,
    const torch::Tensor &domain = {}) {
  auto predicted = ima4b_flatten_targets(prediction);
  auto target = ima4b_flatten_targets(split.target);
  auto slot = ima4b_flatten_targets(split.target_slot);
  if (domain.defined()) {
    const auto rows = domain.to(torch::kCPU, torch::kBool)
                          .nonzero()
                          .reshape({-1});
    predicted = predicted.index_select(0, rows);
    target = target.index_select(0, rows);
    slot = slot.index_select(0, rows);
  }
  return ima4a_oracle_metric(predicted, target, slot);
}

struct Ima5aPredictorTraining {
  mtf::LatentPredictor predictor{nullptr};
  Ima5aPredictorSnapshot initial{};
  Ima5aPredictorSnapshot selected{};
  std::array<Ima4aOracleMetric, kIma4bValidationSteps.size()> validation{};
  torch::Tensor development_prediction{};
  std::size_t selected_index{0};
  int64_t selected_step{0};
  int64_t updates{0};
  double parameter_delta{0.0};
  bool production_reproduced{false};
  bool compute_censored{false};
  bool pass{false};
};

[[nodiscard]] Ima5aPredictorTraining ima5a_train_predictor(
    const std::shared_ptr<mtf::LatentPredictorImpl> &production,
    const mtf::mtf_jepa_mae_vicreg_config_t &config,
    const mtf::mtf_token_metadata_t &metadata,
    const Ima4bFrozenSplit &fit, const Ima4bFrozenSplit &validation,
    const Ima4bFrozenSplit &development, const torch::Device &device,
    int64_t seed) {
  Ima5aPredictorTraining result{};
  result.predictor = mtf::LatentPredictor(config);
  ima5a_copy_predictor(production, result.predictor);
  result.initial = ima5a_snapshot_predictor(result.predictor);
  result.selected = result.initial;
  auto optimizer = torch::optim::Adam(
      result.predictor->parameters(),
      torch::optim::AdamOptions(kIma4bLearningRate));
  const auto initial_prediction =
      ima5a_predict_split(result.predictor, validation, metadata, device);
  result.production_reproduced =
      ima4b_max_abs_difference(initial_prediction, validation.current) <=
      kIma4bTolerance;
  result.validation[0] = ima5a_metric(initial_prediction, validation);

  std::size_t validation_index = 1;
  for (int64_t zero_step = 0; zero_step < kIma4bUpdates; ++zero_step) {
    const auto rows =
        ima4b_training_groups(fit.context.size(0), seed, zero_step);
    const auto row = torch::tensor(rows, torch::kInt64);
    const auto context =
        fit.context.index_select(0, row).to(device, torch::kFloat32);
    const auto context_mask =
        fit.context_mask.index_select(0, row).to(device, torch::kBool);
    const auto target_slot =
        fit.target_slot.index_select(0, row).to(device, torch::kInt64);
    const auto target =
        fit.target.index_select(0, row).to(device, torch::kFloat32);
    set_paired_rng(ima4b_dropout_seed(seed, zero_step), device);
    result.predictor->train();
    optimizer.zero_grad();
    const auto prediction = ima5a_selected_prediction(
        result.predictor, context, context_mask, target_slot, metadata);
    const auto loss = torch::mse_loss(prediction, target);
    if (!torch::isfinite(loss).item<bool>()) {
      throw std::runtime_error("IMA-5A predictor loss is nonfinite");
    }
    loss.backward();
    double gradient_square = 0.0;
    for (const auto &parameter : result.predictor->parameters()) {
      if (parameter.grad().defined()) {
        if (!torch::isfinite(parameter.grad()).all().item<bool>()) {
          throw std::runtime_error("IMA-5A predictor gradient is nonfinite");
        }
        gradient_square += parameter.grad().pow(2).sum().item<double>();
      }
    }
    const double gradient_norm = std::sqrt(gradient_square);
    if (!(gradient_norm > 0.0) || !std::isfinite(gradient_norm)) {
      throw std::runtime_error("IMA-5A predictor gradient vanished");
    }
    const double factor =
        gradient_norm > kIma4bGradientClip
            ? kIma4bGradientClip / gradient_norm
            : 1.0;
    if (factor < 1.0) {
      for (const auto &parameter : result.predictor->parameters()) {
        if (parameter.grad().defined()) {
          parameter.grad().mul_(factor);
        }
      }
    }
    optimizer.step();
    ++result.updates;
    const int64_t completed = zero_step + 1;
    if (validation_index < kIma4bValidationSteps.size() &&
        completed == kIma4bValidationSteps[validation_index]) {
      const auto prediction_now =
          ima5a_predict_split(result.predictor, validation, metadata, device);
      result.validation[validation_index] =
          ima5a_metric(prediction_now, validation);
      if (result.validation[validation_index].nmse <
          result.validation[result.selected_index].nmse) {
        result.selected_index = validation_index;
        result.selected_step = completed;
        result.selected = ima5a_snapshot_predictor(result.predictor);
      }
      ++validation_index;
    }
  }
  if (validation_index != kIma4bValidationSteps.size()) {
    throw std::runtime_error("IMA-5A validation schedule failed");
  }
  result.parameter_delta =
      ima5a_predictor_delta(result.predictor, result.initial);
  result.compute_censored =
      result.selected_step == kIma4bUpdates &&
      result.validation.back().r2 -
              result.validation[kIma4bValidationSteps.size() - 2].r2 >=
          kIma4bComputeCensorDelta;
  const bool adam_complete = ima4b_adam_complete(optimizer, kIma4bUpdates);
  ima5a_restore_predictor(result.predictor, result.selected);
  result.development_prediction =
      ima5a_predict_split(result.predictor, development, metadata, device);
  result.pass = result.production_reproduced &&
                result.updates == kIma4bUpdates &&
                result.parameter_delta > 0.0 && adam_complete &&
                torch::isfinite(result.development_prediction)
                    .all()
                    .item<bool>();
  return result;
}

[[nodiscard]] mtf::mtf_token_metadata_t ima5a_metadata(
    const Ima4aModules &modules, const Dataset &dataset,
    const torch::Device &device) {
  torch::NoGradGuard no_grad;
  const auto tokens = modules.tokenizer->forward(
      dataset.data.narrow(0, 0, 1).to(device),
      dataset.mask.narrow(0, 0, 1).to(device));
  return {.start_index = tokens.metadata.start_index.detach().clone(),
          .width = tokens.metadata.width.detach().clone(),
          .scale_id = tokens.metadata.scale_id.detach().clone(),
          .channel_id = tokens.metadata.channel_id.detach().clone(),
          .domain_id = tokens.metadata.domain_id.detach().clone()};
}

[[nodiscard]] torch::Tensor ima5a_c_features_from_context(
    const torch::Tensor &context_input, const torch::Tensor &mask_input,
    const torch::Tensor &target_metadata_input) {
  const auto context = context_input.to(torch::kCPU, torch::kFloat64);
  const auto mask = mask_input.to(torch::kCPU, torch::kBool);
  const auto target_metadata =
      target_metadata_input.to(torch::kCPU, torch::kFloat64);
  const auto field = torch::cat(
      {context.masked_fill(mask.logical_not().unsqueeze(-1), 0.0)
           .reshape({context.size(0), -1}),
       mask.to(torch::kFloat64)},
      1);
  const auto repeated = ima4b_repeated_group_rows(context.size(0));
  return torch::cat(
             {ima4b_flatten_targets(target_metadata),
              field.index_select(0, repeated)},
             1)
      .contiguous();
}

struct Ima5aInterventionCertificate {
  std::array<double, 4> field_max_abs{};
  std::array<double, 4> target_max_abs{};
  bool support_disjoint{false};
  bool legal_input_exact{false};
  bool pass{false};
};

[[nodiscard]] Ima5aInterventionCertificate ima5a_intervention_certificate(
    const Ima4aModules &modules, const Dataset &dataset,
    const mtf::mtf_jepa_mae_vicreg_config_t &config,
    const torch::Device &device, int64_t seed,
    const Ima5aTeacher &teacher) {
  constexpr int64_t kRows = 8;
  torch::NoGradGuard no_grad;
  const auto data = dataset.data.narrow(0, 0, kRows).to(device);
  const auto feature_mask = dataset.mask.narrow(0, 0, kRows).to(device);
  const auto online = modules.tokenizer->forward(data, feature_mask);
  const auto pair = ima4a_group_paired_masks(
      online, seed, dataset.group_begin, config, device);
  const auto &masks = pair.masks[1];
  const auto locations = masks.target_mask.nonzero();
  const auto target_rows = locations.select(1, 0).contiguous();
  const auto target_slots = locations.select(1, 1).contiguous();
  const auto closure =
      ima4a_support_closure(online.metadata, target_slots, device)
          .reshape({kRows, kIma4bTargetsPerGroup, kIma4bSlotCount})
          .any(1);
  Ima5aInterventionCertificate result{};
  result.support_disjoint =
      !closure.logical_and(masks.context_mask).any().item<bool>();
  const auto base_legal = online.tokens.masked_fill(
      masks.context_mask.logical_not().unsqueeze(-1), 0.0);
  const auto base_context =
      modules.encoder->forward(base_legal, masks.context_mask);
  const auto all_metadata =
      mtf::detail::metadata_features(online.metadata, config);
  const auto target_metadata =
      all_metadata.index_select(0, target_slots)
          .reshape({kRows, kIma4bTargetsPerGroup, kIma4aMetadataWidth});
  const auto base_features = ima5a_c_features_from_context(
      base_context, masks.context_mask, target_metadata);
  const auto base_target =
      ima5a_teacher_kernel(teacher, base_features,
                           target_slots.to(torch::kCPU, torch::kInt64))
          .matmul(teacher.dual) +
      teacher.residual_bias;
  result.legal_input_exact = true;
  const auto noise =
      (torch::arange(online.tokens.numel(), online.tokens.options())
           .reshape_as(online.tokens) +
       1.0) *
      1000.0;
  const std::array<torch::Tensor, 4> replacements{
      torch::zeros_like(online.tokens), online.tokens.roll({1}, {1}), noise,
      online.tokens.roll({1}, {0})};
  for (std::size_t index = 0; index < replacements.size(); ++index) {
    const auto modified = torch::where(
        closure.unsqueeze(-1), replacements[index], online.tokens);
    const auto legal = modified.masked_fill(
        masks.context_mask.logical_not().unsqueeze(-1), 0.0);
    result.legal_input_exact =
        result.legal_input_exact && rssm_tensor_bytes_equal(legal, base_legal);
    const auto context = modules.encoder->forward(legal, masks.context_mask);
    const auto features = ima5a_c_features_from_context(
        context, masks.context_mask, target_metadata);
    const auto target =
        ima5a_teacher_kernel(teacher, features,
                             target_slots.to(torch::kCPU, torch::kInt64))
            .matmul(teacher.dual) +
        teacher.residual_bias;
    result.field_max_abs[index] =
        ima4b_max_abs_difference(features, base_features);
    result.target_max_abs[index] =
        ima4b_max_abs_difference(target, base_target);
  }
  result.pass = result.support_disjoint && result.legal_input_exact &&
                std::all_of(result.field_max_abs.begin(),
                            result.field_max_abs.end(),
                            [](double value) { return value == 0.0; }) &&
                std::all_of(result.target_max_abs.begin(),
                            result.target_max_abs.end(),
                            [](double value) { return value == 0.0; });
  return result;
}

[[nodiscard]] torch::Tensor ima5a_group_features(
    const torch::Tensor &candidate_flat, int64_t groups) {
  return candidate_flat
      .reshape({groups, kIma4bTargetsPerGroup * kIma4bLatentDim})
      .to(torch::kCPU, torch::kFloat64)
      .contiguous();
}

[[nodiscard]] torch::Tensor ima5a_domain_rows(
    const torch::Tensor &domain_input, int64_t domain_filter) {
  const auto domain =
      domain_input.to(torch::kCPU, torch::kInt64).contiguous();
  if (domain.dim() != 2 || domain.size(1) != kIma4bTargetsPerGroup ||
      domain_filter < 0 || domain_filter > 1 ||
      !domain.eq(domain_filter).sum(1).eq(1).all().item<bool>()) {
    throw std::runtime_error(
        "IMA-5A expected one target per domain and group");
  }
  return domain.reshape({-1}).eq(domain_filter).nonzero().reshape({-1});
}

[[nodiscard]] torch::Tensor ima5a_domain_group_features(
    const torch::Tensor &candidate_flat, const torch::Tensor &domain,
    int64_t domain_filter) {
  const auto rows = ima5a_domain_rows(domain, domain_filter);
  if (candidate_flat.dim() != 2 || candidate_flat.size(0) != domain.numel()) {
    throw std::runtime_error("IMA-5A domain feature shape failed");
  }
  return candidate_flat.to(torch::kCPU, torch::kFloat64)
      .index_select(0, rows)
      .contiguous();
}

[[nodiscard]] torch::Tensor ima5a_maybe_domain_rows(
    const torch::Tensor &value, const torch::Tensor &domain,
    int64_t domain_filter) {
  if (domain_filter < 0) {
    return value;
  }
  return value.index_select(0, ima5a_domain_rows(domain, domain_filter))
      .contiguous();
}

struct Ima5aChannelProbe {
  double accuracy{0.0};
  double selected_alpha{0.0};
  bool finite{false};
};

[[nodiscard]] Ima5aChannelProbe ima5a_channel_probe(
    const torch::Tensor &fit_value, const torch::Tensor &fit_slot,
    const torch::Tensor &fit_channel, const torch::Tensor &validation_value,
    const torch::Tensor &validation_slot,
    const torch::Tensor &validation_channel,
    const torch::Tensor &development_value,
    const torch::Tensor &development_slot,
    const torch::Tensor &development_channel) {
  const auto means = ima5a_slot_means(fit_value, fit_slot, false);
  const auto fit =
      ima5a_center_by_reference_means(fit_value, fit_slot, means);
  const auto validation = ima5a_center_by_reference_means(
      validation_value, validation_slot, means);
  const auto development = ima5a_center_by_reference_means(
      development_value, development_slot, means);
  const auto fit_target =
      torch::one_hot(fit_channel.to(torch::kInt64), kChannels)
          .to(torch::kFloat64);
  const auto validation_target =
      torch::one_hot(validation_channel.to(torch::kInt64), kChannels)
          .to(torch::kFloat64);
  const auto development_target =
      development_channel.to(torch::kCPU, torch::kInt64);
  double best = std::numeric_limits<double>::infinity();
  RidgeModel selected{};
  double alpha = 0.0;
  for (const double candidate : kRidgeGrid) {
    const auto model = fit_ridge(fit, fit_target, candidate);
    const double mse =
        (predict(model, validation) - validation_target)
            .pow(2)
            .mean()
            .item<double>();
    if (mse < best) {
      best = mse;
      selected = model;
      alpha = candidate;
    }
  }
  const auto labels = predict(selected, development).argmax(1);
  Ima5aChannelProbe result{};
  result.accuracy = labels.eq(development_target)
                        .to(torch::kFloat64)
                        .mean()
                        .item<double>();
  result.selected_alpha = alpha;
  result.finite = std::isfinite(result.accuracy) &&
                  result.accuracy >= 0.0 && result.accuracy <= 1.0;
  return result;
}

[[nodiscard]] torch::Tensor ima5a_channel_permutation(
    const mtf::mtf_token_metadata_t &metadata) {
  const auto start =
      metadata.start_index.to(torch::kCPU, torch::kInt64).contiguous();
  const auto width = metadata.width.to(torch::kCPU, torch::kInt64).contiguous();
  const auto scale =
      metadata.scale_id.to(torch::kCPU, torch::kInt64).contiguous();
  const auto channel =
      metadata.channel_id.to(torch::kCPU, torch::kInt64).contiguous();
  const auto domain =
      metadata.domain_id.to(torch::kCPU, torch::kInt64).contiguous();
  std::vector<int64_t> mapping(kIma4bSlotCount, -1);
  for (int64_t source = 0; source < kIma4bSlotCount; ++source) {
    const int64_t destination_channel =
        (channel[source].item<int64_t>() + 1) % kChannels;
    for (int64_t destination = 0; destination < kIma4bSlotCount;
         ++destination) {
      if (channel[destination].item<int64_t>() == destination_channel &&
          start[destination].item<int64_t>() ==
              start[source].item<int64_t>() &&
          width[destination].item<int64_t>() ==
              width[source].item<int64_t>() &&
          scale[destination].item<int64_t>() ==
              scale[source].item<int64_t>() &&
          domain[destination].item<int64_t>() ==
              domain[source].item<int64_t>()) {
        mapping[static_cast<std::size_t>(source)] = destination;
        break;
      }
    }
    if (mapping[static_cast<std::size_t>(source)] < 0) {
      throw std::runtime_error("IMA-5A channel permutation is incomplete");
    }
  }
  return torch::tensor(mapping, torch::kInt64);
}

struct Ima5aPermutationMetric {
  double cosine{0.0};
  double normalized_rmse{0.0};
  bool pass{false};
};

[[nodiscard]] Ima5aPermutationMetric ima5a_permutation_metric(
    const Ima5aTeacher &teacher, const Ima4bFrozenSplit &split,
    const torch::Tensor &candidate_flat,
    const mtf::mtf_token_metadata_t &metadata,
    const mtf::mtf_jepa_mae_vicreg_config_t &config,
    int64_t domain_filter = -1) {
  const auto mapping = ima5a_channel_permutation(metadata);
  const auto source_slots = torch::arange(kIma4bSlotCount, torch::kInt64);
  auto context = torch::zeros_like(split.context);
  auto mask = torch::zeros_like(split.context_mask);
  context.index_copy_(1, mapping,
                      split.context.index_select(1, source_slots));
  mask.index_copy_(1, mapping,
                   split.context_mask.index_select(1, source_slots));
  const auto mapped_slot =
      mapping.index_select(0, ima4b_flatten_targets(split.target_slot))
          .reshape_as(split.target_slot)
          .contiguous();
  const auto features = mtf::detail::metadata_features(metadata, config)
                            .to(torch::kCPU, torch::kFloat32);
  const auto target_metadata =
      features.index_select(0, ima4b_flatten_targets(mapped_slot))
          .reshape({split.context.size(0), kIma4bTargetsPerGroup,
                    kIma4aMetadataWidth})
          .contiguous();
  const auto permuted_features = ima5a_c_features_from_context(
      context, mask, target_metadata);
  const auto permuted =
      ima5a_teacher_kernel(teacher, permuted_features,
                           ima4b_flatten_targets(mapped_slot))
          .matmul(teacher.dual) +
      teacher.residual_bias;
  auto original =
      candidate_flat.to(torch::kCPU, torch::kFloat64).contiguous();
  auto compared = permuted.contiguous();
  auto slot = ima4b_flatten_targets(split.target_slot);
  if (domain_filter >= 0) {
    const auto slot_domain = metadata.domain_id
                                 .to(torch::kCPU, torch::kInt64)
                                 .index_select(0, slot);
    const auto rows =
        slot_domain.eq(domain_filter).nonzero().reshape({-1});
    if (rows.numel() * kIma4bTargetsPerGroup != slot.numel()) {
      throw std::runtime_error("IMA-5A permutation domain balance failed");
    }
    original = original.index_select(0, rows).contiguous();
    compared = compared.index_select(0, rows).contiguous();
    slot = slot.index_select(0, rows).contiguous();
  }
  const auto centered = ima5a_center_by_reference_means(
      original, slot, ima5a_slot_means(original, slot, false));
  const double scale = std::sqrt(centered.pow(2).mean().item<double>());
  Ima5aPermutationMetric result{};
  result.cosine = ima4a_cosine(original, compared);
  result.normalized_rmse =
      std::sqrt((original - compared).pow(2).mean().item<double>()) /
      std::max(scale, 1.0e-30);
  result.pass = result.cosine >= 0.50 && result.normalized_rmse <= 1.0;
  return result;
}

struct Ima5aStructure {
  ProbeCurve protected_probe{};
  ProbeCurve shuffled_probe{};
  RssmOrderCurve order{};
  RssmOrderCurve shuffled_order{};
  RssmOrderCurve deletion_order{};
  Ima5aChannelProbe channel{};
  Ima5aChannelProbe deletion_channel{};
  Ima5aPermutationMetric permutation{};
  double center_phase_r2{0.0};
  double cross_channel_r2{0.0};
  bool pass{false};
};

[[nodiscard]] Ima5aStructure ima5a_structure(
    const Ima5aTeacher &teacher, const Ima5aSplit &fit,
    const Ima5aSplit &validation, const Ima5aSplit &development,
    const Ima5aSplit &reversed_fit,
    const Ima5aSplit &reversed_validation,
    const Ima5aSplit &reversed_development,
    const torch::Tensor &fit_candidate,
    const torch::Tensor &validation_candidate,
    const torch::Tensor &development_candidate,
    const torch::Tensor &reversed_fit_candidate,
    const torch::Tensor &reversed_validation_candidate,
    const torch::Tensor &reversed_development_candidate,
    const torch::Tensor &fit_task, const torch::Tensor &validation_task,
    const torch::Tensor &development_task,
    const mtf::mtf_token_metadata_t &metadata,
    const mtf::mtf_jepa_mae_vicreg_config_t &config,
    int64_t domain_filter = -1) {
  if (!torch::equal(fit.frozen.target_slot,
                    reversed_fit.frozen.target_slot) ||
      !torch::equal(validation.frozen.target_slot,
                    reversed_validation.frozen.target_slot) ||
      !torch::equal(development.frozen.target_slot,
                    reversed_development.frozen.target_slot) ||
      !torch::equal(fit.auxiliary.domain,
                    reversed_fit.auxiliary.domain) ||
      !torch::equal(validation.auxiliary.domain,
                    reversed_validation.auxiliary.domain) ||
      !torch::equal(development.auxiliary.domain,
                    reversed_development.auxiliary.domain)) {
    throw std::runtime_error("IMA-5A reversal target slots changed");
  }
  const auto group_features =
      [domain_filter](const torch::Tensor &value, const Ima5aSplit &split) {
        return domain_filter < 0
                   ? ima5a_group_features(value,
                                          split.frozen.context.size(0))
                   : ima5a_domain_group_features(
                         value, split.auxiliary.domain, domain_filter);
      };
  const auto selected_rows =
      [domain_filter](const torch::Tensor &value, const Ima5aSplit &split) {
        return ima5a_maybe_domain_rows(
            value, split.auxiliary.domain, domain_filter);
      };
  const auto fit_features = group_features(fit_candidate, fit);
  const auto validation_features =
      group_features(validation_candidate, validation);
  const auto development_features =
      group_features(development_candidate, development);
  const auto reversed_fit_features =
      group_features(reversed_fit_candidate, reversed_fit);
  const auto reversed_validation_features =
      group_features(reversed_validation_candidate, reversed_validation);
  const auto reversed_development_features =
      group_features(reversed_development_candidate, reversed_development);
  Ima5aStructure result{};
  result.protected_probe = rssm_probe_curve(
      fit_features, validation_features, development_features,
      fit_task, validation_task, development_task, false);
  const auto shuffle_fit =
      rssm_sattolo_permutation(fit_task.size(0), kRssmShuffleTrainTag);
  const auto shuffle_validation = rssm_sattolo_permutation(
      validation_task.size(0), kRssmShuffleValidationTag);
  const auto shuffle_development = rssm_sattolo_permutation(
      development_task.size(0), kRssmShuffleTestTag);
  result.shuffled_probe = rssm_probe_curve(
      fit_features, validation_features, development_features,
      fit_task.index_select(0, shuffle_fit),
      validation_task.index_select(0, shuffle_validation),
      development_task.index_select(0, shuffle_development), false);

  const auto order_fit_targets = rssm_order_fit_targets(nullptr);
  const auto order_validation_target =
      rssm_order_labels(validation_features.size(0));
  const auto order_development_target =
      rssm_order_labels(development_features.size(0));
  result.order = rssm_order_curve(
      rssm_interleave_pairs(fit_features, reversed_fit_features),
      rssm_interleave_pairs(validation_features,
                            reversed_validation_features),
      rssm_interleave_pairs(development_features,
                            reversed_development_features),
      order_fit_targets, order_validation_target, order_development_target,
      false);
  const auto order_fit_permutations = rssm_order_fit_permutations();
  const auto shuffled_order_fit_targets =
      rssm_order_fit_targets(&order_fit_permutations);
  const auto shuffled_validation_target =
      order_validation_target
          .index_select(0, rssm_sattolo_permutation(
                               order_validation_target.size(0),
                               kRssmOrderShuffleValidationTag))
          .contiguous();
  const auto shuffled_development_target =
      order_development_target
          .index_select(0, rssm_sattolo_permutation(
                               order_development_target.size(0),
                               kRssmOrderShuffleTestTag))
          .contiguous();
  result.shuffled_order = rssm_order_curve(
      rssm_interleave_pairs(fit_features, reversed_fit_features),
      rssm_interleave_pairs(validation_features,
                            reversed_validation_features),
      rssm_interleave_pairs(development_features,
                            reversed_development_features),
      shuffled_order_fit_targets, shuffled_validation_target,
      shuffled_development_target, false);

  const auto deletion_fit = group_features(
      ima4b_flatten_targets(fit.auxiliary.support0), fit);
  const auto deletion_validation = group_features(
      ima4b_flatten_targets(validation.auxiliary.support0), validation);
  const auto deletion_development = group_features(
      ima4b_flatten_targets(development.auxiliary.support0), development);
  const auto deletion_reversed_fit = group_features(
      ima4b_flatten_targets(reversed_fit.auxiliary.support0), reversed_fit);
  const auto deletion_reversed_validation = group_features(
      ima4b_flatten_targets(reversed_validation.auxiliary.support0),
      reversed_validation);
  const auto deletion_reversed_development = group_features(
      ima4b_flatten_targets(reversed_development.auxiliary.support0),
      reversed_development);
  result.deletion_order = rssm_order_curve(
      rssm_interleave_pairs(deletion_fit, deletion_reversed_fit),
      rssm_interleave_pairs(deletion_validation,
                            deletion_reversed_validation),
      rssm_interleave_pairs(deletion_development,
                            deletion_reversed_development),
      order_fit_targets, order_validation_target, order_development_target,
      false);

  result.channel = ima5a_channel_probe(
      selected_rows(fit_candidate, fit),
      selected_rows(ima4b_flatten_targets(fit.frozen.target_slot), fit),
      selected_rows(ima4b_flatten_targets(fit.auxiliary.channel), fit),
      selected_rows(validation_candidate, validation),
      selected_rows(
          ima4b_flatten_targets(validation.frozen.target_slot), validation),
      selected_rows(
          ima4b_flatten_targets(validation.auxiliary.channel), validation),
      selected_rows(development_candidate, development),
      selected_rows(
          ima4b_flatten_targets(development.frozen.target_slot), development),
      selected_rows(
          ima4b_flatten_targets(development.auxiliary.channel), development));
  result.deletion_channel = ima5a_channel_probe(
      selected_rows(ima4b_flatten_targets(fit.auxiliary.support0), fit),
      selected_rows(ima4b_flatten_targets(fit.frozen.target_slot), fit),
      selected_rows(ima4b_flatten_targets(fit.auxiliary.channel), fit),
      selected_rows(
          ima4b_flatten_targets(validation.auxiliary.support0), validation),
      selected_rows(
          ima4b_flatten_targets(validation.frozen.target_slot), validation),
      selected_rows(
          ima4b_flatten_targets(validation.auxiliary.channel), validation),
      selected_rows(
          ima4b_flatten_targets(development.auxiliary.support0), development),
      selected_rows(
          ima4b_flatten_targets(development.frozen.target_slot), development),
      selected_rows(
          ima4b_flatten_targets(development.auxiliary.channel), development));
  result.permutation = ima5a_permutation_metric(
      teacher, development.frozen, development_candidate, metadata, config,
      domain_filter);
  const auto &final = result.protected_probe.points.back().score.task;
  result.center_phase_r2 = (final[0] + final[3] + final[4]) / 3.0;
  result.cross_channel_r2 = (final[7] + final[8]) / 2.0;
  result.pass = result.order.area > 0.50 && result.channel.finite &&
                result.channel.accuracy >= 0.40 &&
                result.center_phase_r2 > 0.0 &&
                result.cross_channel_r2 > 0.0 && result.permutation.pass &&
                result.shuffled_probe.points.back().score.macro <= 0.05;
  return result;
}

struct Ima5aR2Summary {
  double point{0.0};
  Interval interval{};
};

[[nodiscard]] Ima5aR2Summary ima5a_r2_summary(
    const std::array<torch::Tensor, 3> &prediction,
    const std::array<torch::Tensor, 3> &target,
    const std::array<torch::Tensor, 3> &slot,
    const std::array<torch::Tensor, 3> &domain, int64_t domain_filter,
    const std::vector<torch::Tensor> &bootstrap_rows) {
  Ima5aR2Summary result{};
  for (std::size_t seed = 0; seed < prediction.size(); ++seed) {
    auto selected_prediction = prediction[seed];
    auto selected_target = target[seed];
    auto selected_slot = slot[seed];
    if (domain_filter >= 0) {
      const auto rows = domain[seed].eq(domain_filter).nonzero().reshape({-1});
      selected_prediction = selected_prediction.index_select(0, rows);
      selected_target = selected_target.index_select(0, rows);
      selected_slot = selected_slot.index_select(0, rows);
    }
    result.point += ima4a_oracle_metric(
                        selected_prediction, selected_target, selected_slot)
                        .r2;
  }
  result.point /= static_cast<double>(prediction.size());
  std::vector<double> replicates;
  replicates.reserve(bootstrap_rows.size());
  for (const auto &rows : bootstrap_rows) {
    double value = 0.0;
    for (std::size_t seed = 0; seed < prediction.size(); ++seed) {
      auto sampled_prediction =
          ima4a_group_resample(prediction[seed], rows);
      auto sampled_target = ima4a_group_resample(target[seed], rows);
      auto sampled_slot = ima4a_group_resample(slot[seed], rows);
      if (domain_filter >= 0) {
        const auto sampled_domain =
            ima4a_group_resample(domain[seed], rows);
        const auto selected =
            sampled_domain.eq(domain_filter).nonzero().reshape({-1});
        sampled_prediction = sampled_prediction.index_select(0, selected);
        sampled_target = sampled_target.index_select(0, selected);
        sampled_slot = sampled_slot.index_select(0, selected);
      }
      value += ima4a_oracle_metric(sampled_prediction, sampled_target,
                                   sampled_slot)
                   .r2;
    }
    replicates.push_back(value / static_cast<double>(prediction.size()));
  }
  result.interval = percentile_interval(std::move(replicates));
  return result;
}

void ima5a_clear_predictor_gradients(mtf::LatentPredictor &predictor) {
  for (const auto &parameter : predictor->parameters()) {
    if (parameter.grad().defined()) {
      parameter.mutable_grad() = torch::Tensor();
    }
  }
}

struct Ima5aGradientPanel {
  torch::Tensor time{};
  torch::Tensor frequency{};
  double cosine{0.0};
  std::array<double, 2> predictor_norm{};
  bool finite{false};
};

[[nodiscard]] torch::Tensor ima5a_gradient_for_domain(
    mtf::MtfJepaMaeVicreg &model, const Ima4aModules &modules,
    mtf::LatentPredictor &predictor, const Dataset &dataset,
    const torch::Tensor &candidate_flat, const torch::Tensor &domain_flat,
    const mtf::mtf_token_metadata_t &metadata, const torch::Device &device,
    int64_t seed, int64_t domain, double &predictor_norm) {
  ima4a_clear_gradients(model);
  ima5a_clear_predictor_gradients(predictor);
  const auto data =
      dataset.data.narrow(0, 0, kIma5aDiagnosticGroups).to(device);
  const auto feature_mask =
      dataset.mask.narrow(0, 0, kIma5aDiagnosticGroups).to(device);
  const auto online = modules.tokenizer->forward(data, feature_mask);
  const auto pair = ima4a_group_paired_masks(
      online, seed, dataset.group_begin, model->config(), device);
  const auto &masks = pair.masks[1];
  const auto locations = masks.target_mask.nonzero();
  const auto target_rows = locations.select(1, 0).contiguous();
  const auto target_slots = locations.select(1, 1).contiguous();
  const auto expected_rows =
      ima4b_repeated_group_rows(kIma5aDiagnosticGroups).to(device);
  if (!torch::equal(target_rows, expected_rows)) {
    throw std::runtime_error("IMA-5A gradient target grouping failed");
  }
  const auto context_tokens = online.tokens.masked_fill(
      masks.context_mask.logical_not().unsqueeze(-1), 0.0);
  const auto context =
      modules.encoder->forward(context_tokens, masks.context_mask);
  const auto full_prediction =
      predictor->forward(context, masks.context_mask, metadata);
  const auto selected_prediction =
      full_prediction.index({target_rows, target_slots,
                             torch::indexing::Slice()});
  const auto candidate = candidate_flat
                             .narrow(0, 0, target_slots.size(0))
                             .to(device, torch::kFloat32)
                             .detach();
  const auto domain_rows =
      domain_flat.narrow(0, 0, target_slots.size(0))
          .eq(domain)
          .nonzero()
          .reshape({-1})
          .to(device, torch::kInt64);
  if (domain_rows.numel() == 0) {
    throw std::runtime_error("IMA-5A gradient domain is empty");
  }
  const auto loss = torch::mse_loss(
      selected_prediction.index_select(0, domain_rows),
      candidate.index_select(0, domain_rows));
  loss.backward();
  auto served = ima4a_parameter_gradient(model, ima4a_served_name);
  double predictor_square = 0.0;
  for (const auto &parameter : predictor->parameters()) {
    if (parameter.grad().defined()) {
      predictor_square += parameter.grad().pow(2).sum().item<double>();
    }
  }
  predictor_norm = std::sqrt(predictor_square);
  ima4a_clear_gradients(model);
  ima5a_clear_predictor_gradients(predictor);
  return served;
}

[[nodiscard]] Ima5aGradientPanel ima5a_gradient_panel(
    mtf::MtfJepaMaeVicreg &model, const Ima4aModules &modules,
    mtf::LatentPredictor &predictor, const Dataset &development,
    const torch::Tensor &candidate_flat, const torch::Tensor &domain_flat,
    const mtf::mtf_token_metadata_t &metadata, const torch::Device &device,
    int64_t seed) {
  const bool predictor_training = predictor->is_training();
  predictor->eval();
  Ima5aGradientPanel result{};
  result.time = ima5a_gradient_for_domain(
      model, modules, predictor, development, candidate_flat, domain_flat,
      metadata, device, seed, 0, result.predictor_norm[0]);
  result.frequency = ima5a_gradient_for_domain(
      model, modules, predictor, development, candidate_flat, domain_flat,
      metadata, device, seed, 1, result.predictor_norm[1]);
  result.cosine = ima4a_cosine(result.time, result.frequency);
  result.finite = torch::isfinite(result.time).all().item<bool>() &&
                  torch::isfinite(result.frequency).all().item<bool>() &&
                  result.time.norm().item<double>() > 0.0 &&
                  result.frequency.norm().item<double>() > 0.0 &&
                  result.predictor_norm[0] > 0.0 &&
                  result.predictor_norm[1] > 0.0 &&
                  std::isfinite(result.cosine);
  predictor->train(predictor_training);
  return result;
}

[[nodiscard]] bool ima5a_settled_evidence_exact() {
  const auto log_path = std::filesystem::path(kIma5aIma4cLogPath);
  if (!std::filesystem::exists(log_path)) {
    return false;
  }
  const auto log = rmc_read_file(log_path);
  return ima4a_sha256(kIma5aProtocolPath) == kIma5aProtocolSha256 &&
         ima4a_sha256(kIma5aAmendmentA1Path) ==
             kIma5aAmendmentA1Sha256 &&
         ima4a_sha256(kIma5aAmendmentA2Path) ==
             kIma5aAmendmentA2Sha256 &&
         ima4a_sha256(kIma5aIma4cSourcePath) ==
             kIma5aIma4cSourceSha256 &&
         ima4a_sha256(kIma5aIma4cProtocolPath) ==
             kIma5aIma4cProtocolSha256 &&
         digest::sha256_hex(log) == kIma5aIma4cLogSha256 &&
         log.find("ima4c.mechanics_pass=true") != std::string::npos &&
         log.find("ima4c.decision="
                  "predictor_capacity_exhausted_teacher_redesign") !=
             std::string::npos &&
         log.find("execution_status=ima4c_measurements_complete") !=
             std::string::npos;
}

struct Ima5aSeedEvidence {
  int64_t seed{0};
  Ima5aTeacher teacher{};
  Ima5aInterventionCertificate intervention{};
  Ima5aTargetStats candidate_stats{};
  std::array<Ima5aTargetStats, 2> candidate_domain_stats{};
  Ima5aTargetStats deletion_stats{};
  Ima5aPredictorTraining training{};
  Ima4aOracleMetric predictor_combined{};
  std::array<Ima4aOracleMetric, 2> predictor_domain{};
  Ima5aStructure structure{};
  std::array<Ima5aStructure, 2> domain_structure{};
  Ima5aGradientPanel gradient{};
  torch::Tensor prediction{};
  torch::Tensor target{};
  torch::Tensor slot{};
  torch::Tensor domain{};
  double candidate_exact_residual_r2{0.0};
  std::array<uint64_t, 5> capture_hash{};
  uint64_t development_group_hash{0};
  uint64_t development_target_hash{0};
  uint64_t development_context_mask_hash{0};
  uint64_t development_candidate_field_hash{0};
  bool deletion_collapsed{false};
  bool custody{false};
  bool component_equivalence{false};
  bool frozen_state_exact{false};
  bool gradients_clear{false};
  bool confirmation_sealed{false};
  bool pass{false};
};

[[nodiscard]] Ima5aSeedEvidence ima5a_run_seed(
    const Ima5aData &data, const torch::Device &device,
    std::size_t seed_index) {
  Ima5aSeedEvidence result{};
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
  const bool anchor_digest_exact =
      digest::sha256_hex(rmc_read_file(oca_archive_path(result.seed))) ==
      kOcaAnchorSha256.at(seed_index);
  result.custody = anchor_loaded && anchor_digest_exact &&
                   ima5a_settled_evidence_exact();
  if (!result.custody) {
    throw std::runtime_error("IMA-5A frozen custody failed");
  }
  model->eval();
  result.component_equivalence = ima4a_component_equivalence(
      model, data.inherited.ssl, device, result.seed);
  if (!result.component_equivalence) {
    throw std::runtime_error("IMA-5A component equivalence failed");
  }
  const auto frozen_before = oca_snapshot_state(model);
  const auto modules = ima4a_modules(model);
  const auto metadata =
      ima5a_metadata(modules, data.inherited.development, device);

  const auto teacher_fit = ima4b_capture_split(
      modules, data.teacher_fit, model->config(), device, result.seed);
  const auto teacher_selection = ima4b_capture_split(
      modules, data.teacher_selection, model->config(), device, result.seed);
  result.capture_hash[0] = teacher_fit.hash;
  result.capture_hash[1] = teacher_selection.hash;
  result.teacher = ima5a_fit_teacher(
      teacher_fit, teacher_selection,
      ima5a_eligible_target_slots(metadata));
  result.intervention = ima5a_intervention_certificate(
      modules, data.inherited.development, model->config(), device,
      result.seed, result.teacher);

  const auto fit = ima5a_capture_split(
      modules, data.inherited.probe_train, model->config(), device,
      result.seed);
  const auto validation = ima5a_capture_split(
      modules, data.inherited.probe_validation, model->config(), device,
      result.seed);
  const auto development = ima5a_capture_split(
      modules, data.inherited.development, model->config(), device,
      result.seed);
  result.capture_hash[2] = fit.frozen.hash;
  result.capture_hash[3] = validation.frozen.hash;
  result.capture_hash[4] = development.frozen.hash;
  result.development_group_hash =
      hash_tensor_stable_bytes(development.frozen.group_id);
  result.development_target_hash =
      hash_tensor_stable_bytes(development.frozen.target);
  result.development_context_mask_hash =
      hash_tensor_stable_bytes(development.frozen.context_mask);
  result.development_candidate_field_hash =
      hash_tensor_stable_bytes(ima4b_c_features(development.frozen));
  const auto reversed_fit = ima5a_capture_split(
      modules, data.inherited.reversed_train, model->config(), device,
      result.seed);
  const auto reversed_validation = ima5a_capture_split(
      modules, data.inherited.reversed_validation, model->config(), device,
      result.seed);
  const auto reversed_development = ima5a_capture_split(
      modules, data.inherited.reversed_development, model->config(), device,
      result.seed);

  const auto fit_candidate =
      ima5a_teacher_predict(result.teacher, fit.frozen);
  const auto validation_candidate =
      ima5a_teacher_predict(result.teacher, validation.frozen);
  const auto development_candidate =
      ima5a_teacher_predict(result.teacher, development.frozen);
  const auto reversed_fit_candidate =
      ima5a_teacher_predict(result.teacher, reversed_fit.frozen);
  const auto reversed_validation_candidate =
      ima5a_teacher_predict(result.teacher, reversed_validation.frozen);
  const auto reversed_development_candidate =
      ima5a_teacher_predict(result.teacher, reversed_development.frozen);
  const auto candidate_fit =
      ima5a_candidate_split(fit.frozen, fit_candidate);
  const auto candidate_validation =
      ima5a_candidate_split(validation.frozen, validation_candidate);
  const auto candidate_development =
      ima5a_candidate_split(development.frozen, development_candidate);

  const auto development_slot =
      ima4b_flatten_targets(development.frozen.target_slot);
  const auto development_domain =
      ima4b_flatten_targets(development.auxiliary.domain);
  const auto development_channel =
      ima4b_flatten_targets(development.auxiliary.channel);
  result.candidate_stats = ima5a_target_stats(
      development_candidate, development_slot, development_domain,
      development_channel);
  for (int64_t domain = 0; domain < 2; ++domain) {
    result.candidate_domain_stats[static_cast<std::size_t>(domain)] =
        ima5a_target_stats(development_candidate, development_slot,
                           development_domain, development_channel, domain);
  }
  result.deletion_stats = ima5a_target_stats(
      ima4b_flatten_targets(development.auxiliary.support0),
      development_slot, development_domain, development_channel);
  const auto exact_residual = ima5a_center_by_reference_means(
      ima4b_flatten_targets(development.frozen.target), development_slot,
      result.teacher.slot_mean);
  result.candidate_exact_residual_r2 =
      ima4a_oracle_metric(development_candidate, exact_residual,
                          development_slot)
          .r2;

  result.training = ima5a_train_predictor(
      modules.predictor, model->config(), metadata, candidate_fit,
      candidate_validation, candidate_development, device, result.seed);
  result.predictor_combined = ima5a_metric(
      result.training.development_prediction, candidate_development);
  for (int64_t domain = 0; domain < 2; ++domain) {
    result.predictor_domain[static_cast<std::size_t>(domain)] = ima5a_metric(
        result.training.development_prediction, candidate_development,
        development_domain.eq(domain));
  }

  result.structure = ima5a_structure(
      result.teacher, fit, validation, development, reversed_fit,
      reversed_validation, reversed_development, fit_candidate,
      validation_candidate, development_candidate, reversed_fit_candidate,
      reversed_validation_candidate, reversed_development_candidate,
      data.inherited.probe_train.target,
      data.inherited.probe_validation.target,
      data.inherited.development.target, metadata, model->config());
  for (int64_t domain = 0; domain < 2; ++domain) {
    result.domain_structure[static_cast<std::size_t>(domain)] =
        ima5a_structure(
            result.teacher, fit, validation, development, reversed_fit,
            reversed_validation, reversed_development, fit_candidate,
            validation_candidate, development_candidate,
            reversed_fit_candidate, reversed_validation_candidate,
            reversed_development_candidate,
            data.inherited.probe_train.target,
            data.inherited.probe_validation.target,
            data.inherited.development.target, metadata, model->config(),
            domain);
  }
  result.deletion_collapsed =
      !result.deletion_stats.pass ||
      result.structure.deletion_order.area <= 0.55 ||
      result.structure.deletion_channel.accuracy < 0.40;
  result.gradient = ima5a_gradient_panel(
      model, modules, result.training.predictor,
      data.inherited.development, development_candidate,
      development_domain, metadata, device, result.seed);

  result.prediction =
      ima4b_flatten_targets(result.training.development_prediction);
  result.target = development_candidate;
  result.slot = development_slot;
  result.domain = development_domain;
  result.frozen_state_exact = oca_state_exact(model, frozen_before);
  result.gradients_clear = ima4b_frozen_gradients_clear(model);
  result.confirmation_sealed =
      !data.inherited.confirmation.data.defined() &&
      kIma5aConfirmationBegin == 8000000 &&
      kIma5aConfirmationGroups == 256;
  result.pass = result.custody && result.component_equivalence &&
                result.teacher.finite &&
                result.training.pass && result.gradient.finite &&
                result.frozen_state_exact && result.gradients_clear &&
                result.confirmation_sealed;
  rng_guard.restore();
  return result;
}

[[nodiscard]] Ima4bFrozenSplit ima5a_synthetic_split(int64_t groups,
                                                      int64_t offset) {
  torch::manual_seed(0x5a17 + offset);
  Ima4bFrozenSplit result{};
  result.context = torch::randn({groups, kIma4bSlotCount, kIma4bLatentDim},
                                torch::kFloat32);
  result.context_mask =
      torch::ones({groups, kIma4bSlotCount}, torch::kBool);
  auto slots = torch::empty({groups, kIma4bTargetsPerGroup}, torch::kInt64);
  auto slot = slots.accessor<int64_t, 2>();
  for (int64_t group = 0; group < groups; ++group) {
    for (int64_t target = 0; target < kIma4bTargetsPerGroup; ++target) {
      slot[group][target] =
          (offset + group * kIma4bTargetsPerGroup + target) %
          kIma4bSlotCount;
    }
  }
  result.target_slot = slots.contiguous();
  result.target_metadata = torch::empty(
      {groups, kIma4bTargetsPerGroup, kIma4aMetadataWidth}, torch::kFloat32);
  for (int64_t coordinate = 0; coordinate < kIma4aMetadataWidth;
       ++coordinate) {
    result.target_metadata.select(2, coordinate).copy_(
        slots.to(torch::kFloat32) /
        static_cast<double>(kIma4bSlotCount + coordinate + 1));
  }
  result.target = torch::randn(
      {groups, kIma4bTargetsPerGroup, kIma4bLatentDim}, torch::kFloat32);
  result.current = torch::zeros_like(result.target);
  result.group_id = torch::arange(offset, offset + groups, torch::kInt64);
  result.pairing_exact = true;
  result.finite = true;
  result.hash = ima4b_capture_hash(result);
  return result;
}

int run_ima5a_self_test() {
  const auto fit = ima5a_synthetic_split(72, 0);
  const auto selection = ima5a_synthetic_split(72, 1000);
  const auto teacher = ima5a_fit_teacher(
      fit, selection, torch::ones({kIma4bSlotCount}, torch::kBool));
  const auto prediction = ima5a_teacher_predict(teacher, selection);
  const bool shapes =
      prediction.sizes() == torch::IntArrayRef({144, kIma4bLatentDim}) &&
      teacher.slot_mean.sizes() ==
          torch::IntArrayRef({kIma4bSlotCount, kIma4bLatentDim});
  const bool finite = teacher.finite &&
                      torch::isfinite(prediction).all().item<bool>();
  const auto stats = ima5a_target_stats(
      prediction, ima4b_flatten_targets(selection.target_slot),
      ima4b_flatten_targets(selection.target_slot).remainder(2),
      ima4b_flatten_targets(selection.target_slot).remainder(kChannels));
  const bool pass = shapes && finite &&
                    std::isfinite(stats.total_variance);
  std::cout << std::boolalpha;
  std::cout << "ima5a.self_test.sealed_ima4b_dependency_removed=true\n";
  std::cout << "ima5a.self_test.teacher_finite=" << finite << '\n';
  std::cout << "ima5a.self_test.shapes_pass=" << shapes << '\n';
  std::cout << "ima5a.self_test.pass=" << pass << '\n';
  std::cout << "execution_status=ima5a_self_test_complete\n";
  return pass ? 0 : 3;
}

[[nodiscard]] double ima5a_mean(const std::array<double, 3> &values) {
  return std::accumulate(values.begin(), values.end(), 0.0) /
         static_cast<double>(values.size());
}

[[nodiscard]] bool ima5a_all_at_least(const std::array<double, 3> &values,
                                      double floor) {
  return std::all_of(values.begin(), values.end(),
                     [floor](double value) { return value >= floor; });
}

int run_ima5a_audit(const Options &options) {
  if (options.device != "cuda" || !torch::cuda::is_available()) {
    throw std::runtime_error("IMA-5A audit requires CUDA");
  }
  const torch::Device device(torch::kCUDA, 0);
  DefaultGeneratorStateGuard audit_rng(device);
  const auto data = ima5a_make_data();
  std::array<Ima5aSeedEvidence, 3> evidence{};
  for (std::size_t seed = 0; seed < evidence.size(); ++seed) {
    evidence[seed] = ima5a_run_seed(data, device, seed);
  }

  std::array<torch::Tensor, 3> predictions{};
  std::array<torch::Tensor, 3> targets{};
  std::array<torch::Tensor, 3> slots{};
  std::array<torch::Tensor, 3> domains{};
  std::array<double, 3> combined_seed{};
  std::array<double, 3> time_seed{};
  std::array<double, 3> frequency_seed{};
  std::array<double, 3> order_seed{};
  std::array<double, 3> channel_seed{};
  std::array<double, 3> center_phase_seed{};
  std::array<double, 3> cross_channel_seed{};
  std::array<double, 3> gradient_cosine{};
  std::array<std::array<double, 3>, 2> domain_order_seed{};
  std::array<std::array<double, 3>, 2> domain_channel_seed{};
  std::array<std::array<double, 3>, 2> domain_center_phase_seed{};
  std::array<std::array<double, 3>, 2> domain_cross_channel_seed{};
  RssmOrderCurveBySeed order_curves{};
  RssmOrderCurveBySeed shuffled_order_curves{};
  std::array<RssmOrderCurveBySeed, 2> domain_order_curves{};
  std::array<RssmOrderCurveBySeed, 2> domain_shuffled_order_curves{};
  bool mechanics = true;
  bool intervention = true;
  bool noncollapse = true;
  std::array<bool, 2> domain_noncollapse{true, true};
  bool permutation = true;
  std::array<bool, 2> domain_permutation{true, true};
  bool deletion_collapsed = true;
  bool no_censor = true;
  double shuffled_macro = 0.0;
  std::array<double, 2> domain_shuffled_macro{};
  for (std::size_t seed = 0; seed < evidence.size(); ++seed) {
    const auto &item = evidence[seed];
    predictions[seed] = item.prediction;
    targets[seed] = item.target;
    slots[seed] = item.slot;
    domains[seed] = item.domain;
    combined_seed[seed] = item.predictor_combined.r2;
    time_seed[seed] = item.predictor_domain[0].r2;
    frequency_seed[seed] = item.predictor_domain[1].r2;
    order_seed[seed] = item.structure.order.area;
    channel_seed[seed] = item.structure.channel.accuracy;
    center_phase_seed[seed] = item.structure.center_phase_r2;
    cross_channel_seed[seed] = item.structure.cross_channel_r2;
    gradient_cosine[seed] = item.gradient.cosine;
    order_curves[seed] = item.structure.order;
    shuffled_order_curves[seed] = item.structure.shuffled_order;
    shuffled_macro +=
        item.structure.shuffled_probe.points.back().score.macro;
    for (std::size_t domain = 0; domain < 2; ++domain) {
      const auto &domain_structure = item.domain_structure[domain];
      domain_order_seed[domain][seed] = domain_structure.order.area;
      domain_channel_seed[domain][seed] = domain_structure.channel.accuracy;
      domain_center_phase_seed[domain][seed] =
          domain_structure.center_phase_r2;
      domain_cross_channel_seed[domain][seed] =
          domain_structure.cross_channel_r2;
      domain_order_curves[domain][seed] = domain_structure.order;
      domain_shuffled_order_curves[domain][seed] =
          domain_structure.shuffled_order;
      domain_shuffled_macro[domain] +=
          domain_structure.shuffled_probe.points.back().score.macro;
      domain_noncollapse[domain] =
          domain_noncollapse[domain] &&
          item.candidate_domain_stats[domain].pass;
      domain_permutation[domain] =
          domain_permutation[domain] && domain_structure.permutation.pass;
    }
    mechanics = mechanics && item.pass;
    intervention = intervention && item.intervention.pass;
    noncollapse = noncollapse && item.candidate_stats.pass;
    permutation = permutation && item.structure.permutation.pass;
    deletion_collapsed = deletion_collapsed && item.deletion_collapsed;
    no_censor = no_censor && !item.training.compute_censored;
  }
  shuffled_macro /= static_cast<double>(evidence.size());
  for (auto &value : domain_shuffled_macro) {
    value /= static_cast<double>(evidence.size());
  }
  const auto bootstrap_rows = rmc_bootstrap_rows(256);
  const auto combined = ima5a_r2_summary(
      predictions, targets, slots, domains, -1, bootstrap_rows);
  const auto time = ima5a_r2_summary(
      predictions, targets, slots, domains, 0, bootstrap_rows);
  const auto frequency = ima5a_r2_summary(
      predictions, targets, slots, domains, 1, bootstrap_rows);
  const auto order = rssm_order_curve_interval(
      order_curves, rssm_order_labels(256), bootstrap_rows);
  const auto shuffled_order = rssm_order_curve_interval(
      shuffled_order_curves,
      rssm_order_labels(256)
          .index_select(0, rssm_sattolo_permutation(
                               512, kRssmOrderShuffleTestTag)),
      bootstrap_rows);
  std::array<RssmPointInterval, 2> domain_order{};
  std::array<RssmPointInterval, 2> domain_shuffled_order{};
  for (std::size_t domain = 0; domain < 2; ++domain) {
    domain_order[domain] = rssm_order_curve_interval(
        domain_order_curves[domain], rssm_order_labels(256),
        bootstrap_rows);
    domain_shuffled_order[domain] = rssm_order_curve_interval(
        domain_shuffled_order_curves[domain],
        rssm_order_labels(256)
            .index_select(0, rssm_sattolo_permutation(
                                 512, kRssmOrderShuffleTestTag)),
        bootstrap_rows);
  }

  const bool combined_predictor =
      combined.point >= kIma5aPredictorMeanFloor &&
      ima5a_all_at_least(combined_seed, kIma5aPredictorSeedFloor) &&
      combined.interval.low > 0.0 && no_censor;
  const bool time_predictor =
      time.point >= kIma5aPredictorMeanFloor &&
      ima5a_all_at_least(time_seed, kIma5aPredictorSeedFloor) &&
      time.interval.low > 0.0 && no_censor;
  const bool frequency_predictor =
      frequency.point >= kIma5aPredictorMeanFloor &&
      ima5a_all_at_least(frequency_seed, kIma5aPredictorSeedFloor) &&
      frequency.interval.low > 0.0 && no_censor;
  const int center_positive = static_cast<int>(std::count_if(
      center_phase_seed.begin(), center_phase_seed.end(),
      [](double value) { return value > 0.0; }));
  const int cross_positive = static_cast<int>(std::count_if(
      cross_channel_seed.begin(), cross_channel_seed.end(),
      [](double value) { return value > 0.0; }));
  const bool structure =
      order.point >= 0.60 && order.interval.low > 0.50 &&
      ima5a_all_at_least(order_seed, std::nextafter(0.50, 1.0)) &&
      shuffled_order.interval.high <= 0.60 &&
      ima5a_mean(channel_seed) >= 0.50 &&
      ima5a_all_at_least(channel_seed, 0.40) &&
      ima5a_mean(center_phase_seed) > 0.0 && center_positive >= 2 &&
      ima5a_mean(cross_channel_seed) > 0.0 && cross_positive >= 2 &&
      permutation && shuffled_macro <= 0.05;
  const auto domain_structure_gate =
      [&](std::size_t domain) {
        const int center_positive = static_cast<int>(std::count_if(
            domain_center_phase_seed[domain].begin(),
            domain_center_phase_seed[domain].end(),
            [](double value) { return value > 0.0; }));
        const int cross_positive = static_cast<int>(std::count_if(
            domain_cross_channel_seed[domain].begin(),
            domain_cross_channel_seed[domain].end(),
            [](double value) { return value > 0.0; }));
        return domain_order[domain].point >= 0.60 &&
               domain_order[domain].interval.low > 0.50 &&
               ima5a_all_at_least(
                   domain_order_seed[domain],
                   std::nextafter(0.50, 1.0)) &&
               domain_shuffled_order[domain].interval.high <= 0.60 &&
               ima5a_mean(domain_channel_seed[domain]) >= 0.50 &&
               ima5a_all_at_least(domain_channel_seed[domain], 0.40) &&
               ima5a_mean(domain_center_phase_seed[domain]) > 0.0 &&
               center_positive >= 2 &&
               ima5a_mean(domain_cross_channel_seed[domain]) > 0.0 &&
               cross_positive >= 2 && domain_permutation[domain] &&
               domain_shuffled_macro[domain] <= 0.05;
      };
  const bool time_structure = domain_structure_gate(0);
  const bool frequency_structure = domain_structure_gate(1);
  const bool joint_gradient = ima5a_all_at_least(
      gradient_cosine, kIma5aGradientCosineFloor);

  std::string decision;
  std::string admitted_target = "none";
  if (!intervention) {
    decision = "support_permitted_teacher_rejected_leakage";
  } else if (!domain_noncollapse[0]) {
    decision = "jepa_branch_closed_time_target_noncollapse";
  } else if (!time_structure) {
    decision = "jepa_branch_closed_time_target_structure";
  } else if (!time_predictor) {
    decision = "jepa_branch_closed_production_boundary";
  } else if (noncollapse && domain_noncollapse[1] && structure &&
             frequency_structure &&
             combined_predictor && frequency_predictor && joint_gradient) {
    decision = "admit_joint_teacher_for_ima5b";
    admitted_target = "slot_conditioned_legal_context_teacher_v1_joint";
  } else {
    decision = "admit_time_only_teacher_for_ima5b";
    admitted_target = "slot_conditioned_legal_context_teacher_v1_time_only";
  }

  std::cout << std::setprecision(12) << std::boolalpha;
  std::cout << "ima5a.protocol_sha256=" << ima4a_sha256(kIma5aProtocolPath)
            << '\n';
  std::cout << "ima5a.amendment_a1_sha256="
            << ima4a_sha256(kIma5aAmendmentA1Path) << '\n';
  std::cout << "ima5a.amendment_a2_sha256="
            << ima4a_sha256(kIma5aAmendmentA2Path) << '\n';
  std::cout << "ima5a.source_sha256=" << ima4a_sha256(kIma5aSourcePath)
            << '\n';
  std::cout << "ima5a.teacher_fit_group_begin=" << kIma5aTeacherFitBegin
            << '\n';
  std::cout << "ima5a.teacher_selection_group_begin="
            << kIma5aTeacherSelectionBegin << '\n';
  std::cout << "ima5a.confirmation_group_begin=" << kIma5aConfirmationBegin
            << '\n';
  std::cout << "ima5a.confirmation_opened=false\n";
  for (std::size_t seed = 0; seed < evidence.size(); ++seed) {
    const auto &item = evidence[seed];
    const auto root =
        std::string("ima5a.seed_") + std::to_string(item.seed);
    std::cout << root << ".teacher.hash=" << oca_hex_u64(item.teacher.hash)
              << '\n';
    constexpr std::array<std::string_view, 5> kCaptureNames{
        "teacher_fit", "teacher_selection", "predictor_fit",
        "predictor_validation", "development"};
    for (std::size_t capture = 0; capture < kCaptureNames.size(); ++capture) {
      std::cout << root << ".capture." << kCaptureNames[capture]
                << ".hash=" << oca_hex_u64(item.capture_hash[capture])
                << '\n';
    }
    std::cout << root << ".development.group_hash="
              << oca_hex_u64(item.development_group_hash) << '\n';
    std::cout << root << ".development.exact_target_hash="
              << oca_hex_u64(item.development_target_hash) << '\n';
    std::cout << root << ".development.context_mask_hash="
              << oca_hex_u64(item.development_context_mask_hash) << '\n';
    std::cout << root << ".development.candidate_field_hash="
              << oca_hex_u64(item.development_candidate_field_hash) << '\n';
    std::cout << root << ".teacher.alpha=" << item.teacher.alpha << '\n';
    std::cout << root << ".teacher.calibration_validation_r2="
              << item.teacher.calibration_validation_r2 << '\n';
    std::cout << root << ".teacher.eligible_slot_count="
              << item.teacher.eligible_slot_count << '\n';
    std::cout << root << ".teacher.eligible_slot_mask_hash="
              << oca_hex_u64(
                     hash_tensor_stable_bytes(item.teacher.eligible_slot_mask))
              << '\n';
    std::cout << root << ".teacher.all_eligible_slots_present="
              << item.teacher.all_eligible_slots_present << '\n';
    std::cout << root << ".teacher.ridge_edge_closed="
              << item.teacher.edge_closed << '\n';
    std::cout << root << ".candidate_exact_residual_r2="
              << item.candidate_exact_residual_r2 << '\n';
    std::cout << root << ".intervention.pass=" << item.intervention.pass
              << '\n';
    std::cout << root << ".intervention.support_disjoint="
              << item.intervention.support_disjoint << '\n';
    std::cout << root << ".intervention.legal_input_exact="
              << item.intervention.legal_input_exact << '\n';
    constexpr std::array<std::string_view, 4> kInterventionNames{
        "zeros", "permutation", "finite_noise", "other_sample"};
    for (std::size_t intervention_index = 0;
         intervention_index < kInterventionNames.size();
         ++intervention_index) {
      std::cout << root << ".intervention."
                << kInterventionNames[intervention_index]
                << ".field_max_abs="
                << item.intervention.field_max_abs[intervention_index]
                << '\n';
      std::cout << root << ".intervention."
                << kInterventionNames[intervention_index]
                << ".target_max_abs="
                << item.intervention.target_max_abs[intervention_index]
                << '\n';
    }
    std::cout << root << ".candidate.total_variance="
              << item.candidate_stats.total_variance << '\n';
    std::cout << root << ".candidate.within_slot_variance="
              << item.candidate_stats.within_slot_variance << '\n';
    std::cout << root << ".candidate.slot_variance_share="
              << item.candidate_stats.slot_variance_share << '\n';
    std::cout << root << ".candidate.effective_rank="
              << item.candidate_stats.effective_rank << '\n';
    std::cout << root << ".candidate.participation_rank="
              << item.candidate_stats.participation_rank << '\n';
    std::cout << root << ".candidate.top_eigenvalue_share="
              << item.candidate_stats.top_eigenvalue_share << '\n';
    std::cout << root << ".candidate.noncollapse_pass="
              << item.candidate_stats.pass << '\n';
    for (std::size_t domain = 0; domain < 2; ++domain) {
      const auto domain_name = domain == 0 ? "time" : "frequency";
      const auto &stats = item.candidate_domain_stats[domain];
      std::cout << root << ".candidate." << domain_name
                << ".total_variance=" << stats.total_variance << '\n';
      std::cout << root << ".candidate." << domain_name
                << ".within_slot_variance=" << stats.within_slot_variance
                << '\n';
      std::cout << root << ".candidate." << domain_name
                << ".slot_variance_share=" << stats.slot_variance_share
                << '\n';
      std::cout << root << ".candidate." << domain_name
                << ".effective_rank=" << stats.effective_rank << '\n';
      std::cout << root << ".candidate." << domain_name
                << ".participation_rank=" << stats.participation_rank
                << '\n';
      std::cout << root << ".candidate." << domain_name
                << ".top_eigenvalue_share=" << stats.top_eigenvalue_share
                << '\n';
      std::cout << root << ".candidate." << domain_name
                << ".noncollapse_pass=" << stats.pass << '\n';
    }
    for (std::size_t domain = 0; domain < 2; ++domain) {
      std::cout << root << ".candidate.domain_" << domain
                << ".variance="
                << item.candidate_stats.domain_variance[domain] << '\n';
    }
    for (std::size_t channel = 0; channel < kChannels; ++channel) {
      std::cout << root << ".candidate.channel_" << channel
                << ".variance="
                << item.candidate_stats.channel_variance[channel] << '\n';
      std::cout << root << ".candidate.channel_" << channel
                << ".variance_share="
                << item.candidate_stats.channel_variance_share[channel]
                << '\n';
    }
    std::cout << root << ".deletion.collapsed="
              << item.deletion_collapsed << '\n';
    std::cout << root << ".predictor.combined_r2=" << combined_seed[seed]
              << '\n';
    std::cout << root << ".predictor.time_r2=" << time_seed[seed] << '\n';
    std::cout << root << ".predictor.frequency_r2=" << frequency_seed[seed]
              << '\n';
    std::cout << root << ".predictor.selected_step="
              << item.training.selected_step << '\n';
    std::cout << root << ".predictor.updates=" << item.training.updates
              << '\n';
    std::cout << root << ".predictor.compute_censored="
              << item.training.compute_censored << '\n';
    std::cout << root << ".structure.order_aulc=" << order_seed[seed]
              << '\n';
    std::cout << root << ".structure.channel_accuracy="
              << channel_seed[seed] << '\n';
    std::cout << root << ".structure.center_phase_r2="
              << center_phase_seed[seed] << '\n';
    std::cout << root << ".structure.cross_channel_r2="
              << cross_channel_seed[seed] << '\n';
    std::cout << root << ".structure.permutation_cosine="
              << item.structure.permutation.cosine << '\n';
    std::cout << root << ".structure.permutation_nrmse="
              << item.structure.permutation.normalized_rmse << '\n';
    std::cout << root << ".gradient.time_frequency_cosine="
              << gradient_cosine[seed] << '\n';
    std::cout << root << ".gradient.time_encoder_norm="
              << item.gradient.time.norm().item<double>() << '\n';
    std::cout << root << ".gradient.frequency_encoder_norm="
              << item.gradient.frequency.norm().item<double>() << '\n';
    std::cout << root << ".gradient.time_predictor_norm="
              << item.gradient.predictor_norm[0] << '\n';
    std::cout << root << ".gradient.frequency_predictor_norm="
              << item.gradient.predictor_norm[1] << '\n';
    for (std::size_t domain = 0; domain < 2; ++domain) {
      const auto domain_name = domain == 0 ? "time" : "frequency";
      const auto &domain_structure = item.domain_structure[domain];
      std::cout << root << ".structure." << domain_name
                << ".order_aulc=" << domain_structure.order.area << '\n';
      std::cout << root << ".structure." << domain_name
                << ".channel_accuracy="
                << domain_structure.channel.accuracy << '\n';
      std::cout << root << ".structure." << domain_name
                << ".center_phase_r2="
                << domain_structure.center_phase_r2 << '\n';
      std::cout << root << ".structure." << domain_name
                << ".cross_channel_r2="
                << domain_structure.cross_channel_r2 << '\n';
      std::cout << root << ".structure." << domain_name
                << ".permutation_cosine="
                << domain_structure.permutation.cosine << '\n';
      std::cout << root << ".structure." << domain_name
                << ".permutation_nrmse="
                << domain_structure.permutation.normalized_rmse << '\n';
    }
    std::cout << root << ".pass=" << item.pass << '\n';
  }
  const auto emit_summary = [](const std::string &name,
                               const Ima5aR2Summary &summary) {
    std::cout << "ima5a.summary." << name << ".r2=" << summary.point << '\n';
    std::cout << "ima5a.summary." << name
              << ".bootstrap_95_low=" << summary.interval.low << '\n';
    std::cout << "ima5a.summary." << name
              << ".bootstrap_95_high=" << summary.interval.high << '\n';
  };
  emit_summary("combined", combined);
  emit_summary("time", time);
  emit_summary("frequency", frequency);
  std::cout << "ima5a.summary.order.point=" << order.point << '\n';
  std::cout << "ima5a.summary.order.bootstrap_95_low="
            << order.interval.low << '\n';
  std::cout << "ima5a.summary.order_shuffled.bootstrap_95_high="
            << shuffled_order.interval.high << '\n';
  for (std::size_t domain = 0; domain < 2; ++domain) {
    const auto domain_name = domain == 0 ? "time" : "frequency";
    std::cout << "ima5a.summary." << domain_name
              << ".order.point=" << domain_order[domain].point << '\n';
    std::cout << "ima5a.summary." << domain_name
              << ".order.bootstrap_95_low="
              << domain_order[domain].interval.low << '\n';
    std::cout << "ima5a.summary." << domain_name
              << ".order_shuffled.bootstrap_95_high="
              << domain_shuffled_order[domain].interval.high << '\n';
    std::cout << "ima5a.summary." << domain_name
              << ".noncollapse_pass=" << domain_noncollapse[domain]
              << '\n';
  }
  std::cout << "ima5a.intervention_pass=" << intervention << '\n';
  std::cout << "ima5a.noncollapse_pass=" << noncollapse << '\n';
  std::cout << "ima5a.predictor_combined_pass=" << combined_predictor
            << '\n';
  std::cout << "ima5a.predictor_time_pass=" << time_predictor << '\n';
  std::cout << "ima5a.predictor_frequency_pass=" << frequency_predictor
            << '\n';
  std::cout << "ima5a.structure_pass=" << structure << '\n';
  std::cout << "ima5a.structure_time_pass=" << time_structure << '\n';
  std::cout << "ima5a.structure_frequency_pass=" << frequency_structure
            << '\n';
  std::cout << "ima5a.joint_gradient_pass=" << joint_gradient << '\n';
  std::cout << "ima5a.deletion_control_collapsed=" << deletion_collapsed
            << '\n';
  std::cout << "ima5a.mechanics_pass=" << mechanics << '\n';
  std::cout << "ima5a.decision=" << decision << '\n';
  std::cout << "ima5a.admitted_target=" << admitted_target << '\n';
  std::cout << "ima5a.representation_optimizer_steps=0\n";
  std::cout << "ima5a.ema_updates=0\n";
  std::cout << "execution_status=ima5a_measurements_complete\n";
  audit_rng.restore();
  return mechanics ? 0 : 3;
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment ==
        "support-permitted-teacher-target-alignment-self-test") {
      return run_ima5a_self_test();
    }
    if (options.experiment ==
        "support-permitted-teacher-target-alignment-audit") {
      return run_ima5a_audit(options);
    }
    throw std::runtime_error(
        "--experiment must be support-permitted-teacher-target-alignment-"
        "self-test or support-permitted-teacher-target-alignment-audit");
  } catch (const c10::Error &error) {
    std::cerr << "support_permitted_teacher_target_alignment_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "support_permitted_teacher_target_alignment_error="
              << error.what() << '\n';
  }
  return 2;
}
