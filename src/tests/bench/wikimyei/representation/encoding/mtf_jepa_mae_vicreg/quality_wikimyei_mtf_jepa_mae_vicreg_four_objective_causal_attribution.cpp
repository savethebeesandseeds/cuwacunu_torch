#include "piaabo/digest/sha256.h"

#include <chrono>

// OCA-1 deliberately reuses the sealed FSPA-4 construction and the fixed RMC
// probes.  This translation unit never constructs a downstream model.
#define CUWACUNU_MPSR_EMBEDDED
#include "quality_wikimyei_mtf_jepa_mae_vicreg_minimal_participation_spectral_repair.cpp"
#undef CUWACUNU_MPSR_EMBEDDED

namespace {

constexpr std::string_view kOcaProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/FOUR_OBJECTIVE_CAUSAL_ATTRIBUTION_PROTOCOL.md";
constexpr std::string_view kOcaProtocolSha256 =
    "56bac408b28046e4e014ccd22aab675da9d00b0feb28ed2d25d1debacd57ead2";
constexpr std::string_view kOcaCertificateId =
    "representation_certified_fspa4_minimal_spectral_repair_v1";
constexpr std::string_view kOcaReadoutPolicy =
    "structured_cdsb_sparse_v1";
constexpr int64_t kOcaFactorialSteps = 1536;
constexpr int64_t kOcaAnchorChallengeSteps = 512;
constexpr double kOcaOptimizerLearningRate = 1.0e-3;
constexpr double kOcaGradientClipNorm = 5.0;
constexpr std::string_view kOcaSeedCacheSchema =
    "oca1.interleaved_seed_cache.v1";
constexpr std::string_view kOcaSeedCacheImplementation =
    "cpu_mask_combined_checks_first_arm_reference_durable_complete_v2";
constexpr std::array<std::string_view, 3> kOcaAnchorSha256{
    "5d96b2961daa2bbd08a07a157ddab5debd9d4928234d8341b3961678327e9434",
    "a85c00d5694d1e3f0063e8bce6fc3c2e3132a393e5bcee195909742754e76775",
    "b9c2f82f26a5516069f8460095d3a2b85c482b7cd0e20db120ce2e0bbd68e392"};

struct OcaStateSnapshot {
  std::vector<std::string> names{};
  std::vector<torch::Tensor> tensors{};
};

struct OcaStructuredCapture {
  torch::Tensor values{};
  torch::Tensor valid_mask{};
  torch::Tensor token_mask{};
  torch::Tensor sample_mask{};
  torch::Tensor channel_mask{};
};

struct OcaArchiveReceipt {
  bool metadata_exact{false};
  bool state_exact{false};
  bool structured_exact{false};
  bool evaluation_exact{false};
  std::string sha256{};
  bool pass{false};
};

struct OcaBranchUpdate {
  double raw_loss{0.0};
  double weighted_loss{0.0};
  double pre_clip_gradient_norm{0.0};
  double clip_factor{1.0};
  double served_update_norm{0.0};
  double all_trainable_delta{0.0};
  double served_delta{0.0};
  double predictor_delta{0.0};
  double mae_decoder_delta{0.0};
  double vicreg_head_delta{0.0};
  double target_ema_delta{0.0};
  torch::Tensor target_mask{};
  torch::Tensor context_mask{};
  WeakViewDigest weak_views{};
  bool finite{false};
  bool expected_partitions{false};
  bool pass{false};
};

struct OcaPhaseOneReceipt {
  GradientDiagnostic gradient{};
  std::array<OcaBranchUpdate, 4> updates{};
  bool branch_replay_exact{false};
  bool reference_unchanged{false};
  bool gradient_mechanics{false};
  bool pass{false};
};

struct OcaLegacyTrainingReceipt {
  int64_t steps{0};
  std::vector<double> losses{};
  std::array<double, 4> component_loss_sums{};
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
  std::vector<uint64_t> row_hashes{};
  std::vector<uint64_t> target_mask_hashes{};
  std::vector<uint64_t> context_mask_hashes{};
  std::vector<WeakViewDigest> weak_view_hashes{};
  bool finite{true};
  bool expected_partitions{false};
  bool pass{false};
};

using OcaFactorialEvaluations =
    std::array<std::array<RmcEvaluation, 16>, 3>;
using OcaFactorialTraining =
    std::array<std::array<OcaLegacyTrainingReceipt, 16>, 3>;
using OcaBootstrapAreaTable =
    std::vector<std::array<std::array<double, 16>, 3>>;

struct OcaWeightedContrast {
  rmc_gate::Contrast summary{};
  std::array<double, 3> per_seed{};
  std::array<double, kFamilies> family{};
};

struct OcaChallengeSummary {
  uint8_t mask{0};
  RmcSummary rmc{};
  bool mechanics{false};
  bool qualifies{false};
};

struct OcaInterleavedTrainingResult {
  std::vector<mtf::MtfJepaMaeVicreg> models{};
  std::vector<OcaLegacyTrainingReceipt> receipts{};
  std::vector<bool> initialization_exact{};
  bool metadata_exact{true};
  bool schedule_exact{true};
  bool pass{false};
};

[[nodiscard]] std::filesystem::path oca_archive_path(int64_t seed) {
  return std::filesystem::path(".build") / "tests" / "oca1" /
         ("certified_fspa4_seed_" + std::to_string(seed) + ".pt");
}

[[nodiscard]] torch::Tensor oca_string_tensor(std::string_view value) {
  if (value.empty()) {
    throw std::runtime_error("OCA metadata string cannot be empty");
  }
  return torch::from_blob(
             const_cast<char *>(value.data()),
             {static_cast<int64_t>(value.size())},
             torch::TensorOptions().dtype(torch::kUInt8))
      .clone();
}

[[nodiscard]] std::string oca_tensor_string(const torch::Tensor &value) {
  const auto bytes = value.detach().to(torch::kCPU, torch::kUInt8).contiguous();
  if (bytes.dim() != 1 || bytes.numel() <= 0) {
    throw std::runtime_error("OCA archive string tensor contract failed");
  }
  return {reinterpret_cast<const char *>(bytes.data_ptr<uint8_t>()),
          static_cast<std::size_t>(bytes.numel())};
}

[[nodiscard]] std::string oca_hex_u64(uint64_t value) {
  std::ostringstream out;
  out << std::hex << std::setw(16) << std::setfill('0') << value;
  return out.str();
}

[[nodiscard]] OcaStateSnapshot
oca_snapshot_state(const mtf::MtfJepaMaeVicreg &model) {
  OcaStateSnapshot result{};
  torch::NoGradGuard no_grad;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    result.names.push_back("parameter/" + item.key());
    result.tensors.push_back(
        item.value().detach().to(torch::kCPU).contiguous().clone());
  }
  for (const auto &item : model->named_buffers(/*recurse=*/true)) {
    result.names.push_back("buffer/" + item.key());
    result.tensors.push_back(
        item.value().detach().to(torch::kCPU).contiguous().clone());
  }
  return result;
}

[[nodiscard]] bool oca_state_exact(const mtf::MtfJepaMaeVicreg &model,
                                   const OcaStateSnapshot &reference) {
  const auto observed = oca_snapshot_state(model);
  if (observed.names != reference.names ||
      observed.tensors.size() != reference.tensors.size()) {
    return false;
  }
  for (std::size_t index = 0; index < observed.tensors.size(); ++index) {
    if (observed.tensors[index].sizes() != reference.tensors[index].sizes() ||
        observed.tensors[index].scalar_type() !=
            reference.tensors[index].scalar_type() ||
        !rssm_tensor_bytes_equal(observed.tensors[index],
                                 reference.tensors[index])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] OcaStructuredCapture oca_capture_structured(
    mtf::MtfJepaMaeVicreg &model, const Dataset &dataset,
    const torch::Device &device) {
  const bool was_training = model->is_training();
  model->eval();
  torch::NoGradGuard no_grad;
  const int64_t rows = std::min<int64_t>(kModelRowBatchSize,
                                         dataset.data.size(0));
  const auto encoded = model->encode(dataset.data.narrow(0, 0, rows).to(device),
                                     dataset.mask.narrow(0, 0, rows).to(device));
  const auto served = mtf::select_mtf_serving_pool(
      encoded, mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1,
      model->config());
  if (served.values.sizes() !=
          torch::IntArrayRef({rows, kChannels, kLatentDim}) ||
      served.valid_mask.sizes() != torch::IntArrayRef({rows, kChannels}) ||
      !served.valid_mask.all().item<bool>() ||
      !torch::isfinite(served.values).all().item<bool>()) {
    throw std::runtime_error("OCA structured capture contract failed");
  }
  OcaStructuredCapture result{
      .values = served.values.detach().to(torch::kCPU).contiguous().clone(),
      .valid_mask =
          served.valid_mask.detach().to(torch::kCPU).contiguous().clone(),
      .token_mask =
          encoded.token_mask.detach().to(torch::kCPU).contiguous().clone(),
      .sample_mask = encoded.sample_valid_mask.detach()
                         .to(torch::kCPU)
                         .contiguous()
                         .clone(),
      .channel_mask = encoded.channel_valid_mask.detach()
                          .to(torch::kCPU)
                          .contiguous()
                          .clone(),
  };
  model->train(was_training);
  return result;
}

[[nodiscard]] bool
oca_capture_exact(const OcaStructuredCapture &left,
                  const OcaStructuredCapture &right) {
  return rssm_tensor_bytes_equal(left.values, right.values) &&
         rssm_tensor_bytes_equal(left.valid_mask, right.valid_mask) &&
         rssm_tensor_bytes_equal(left.token_mask, right.token_mask) &&
         rssm_tensor_bytes_equal(left.sample_mask, right.sample_mask) &&
         rssm_tensor_bytes_equal(left.channel_mask, right.channel_mask);
}

[[nodiscard]] bool oca_evaluation_predictions_bytes_exact(
    const RmcEvaluation &left, const RmcEvaluation &right) {
  const auto probe_exact = [](const ProbeCurve &a, const ProbeCurve &b) {
    if (a.points.size() != b.points.size()) {
      return false;
    }
    for (std::size_t point = 0; point < a.points.size(); ++point) {
      if (!rssm_tensor_bytes_equal(a.points[point].prediction,
                                   b.points[point].prediction)) {
        return false;
      }
    }
    return true;
  };
  const auto order_exact = [](const RssmOrderCurve &a,
                              const RssmOrderCurve &b) {
    if (a.points.size() != b.points.size()) {
      return false;
    }
    for (std::size_t point = 0; point < a.points.size(); ++point) {
      if (!rssm_tensor_bytes_equal(a.points[point].prediction,
                                   b.points[point].prediction)) {
        return false;
      }
    }
    return true;
  };
  return probe_exact(left.probe, right.probe) &&
         probe_exact(left.shuffled_probe, right.shuffled_probe) &&
         order_exact(left.order, right.order) &&
         order_exact(left.shuffled_order, right.shuffled_order);
}

void oca_save_archive(const std::filesystem::path &path,
                      mtf::MtfJepaMaeVicreg &model, int64_t seed,
                      const std::string &config_hash) {
  std::filesystem::create_directories(path.parent_path());
  torch::serialize::OutputArchive root;
  torch::serialize::OutputArchive model_archive;
  model->save(model_archive);
  root.write("model", model_archive);
  root.write("meta/schema", oca_string_tensor("oca1.certified_anchor.v1"));
  root.write("meta/seed", torch::tensor({seed}, torch::kInt64));
  root.write("meta/certificate_id", oca_string_tensor(kOcaCertificateId));
  root.write("meta/fspa4_protocol_sha256",
             oca_string_tensor(kMpsrProtocolSha256));
  root.write("meta/oca1_protocol_sha256",
             oca_string_tensor(kOcaProtocolSha256));
  root.write("meta/config_fnv1a64", oca_string_tensor(config_hash));
  root.write("meta/readout_policy", oca_string_tensor(kOcaReadoutPolicy));
  root.save_to(path.string());
}

[[nodiscard]] bool oca_read_metadata(torch::serialize::InputArchive &root,
                                     int64_t seed,
                                     const std::string &config_hash) {
  torch::Tensor schema{};
  torch::Tensor saved_seed{};
  torch::Tensor certificate{};
  torch::Tensor fspa_hash{};
  torch::Tensor oca_hash{};
  torch::Tensor saved_config_hash{};
  torch::Tensor readout{};
  root.read("meta/schema", schema);
  root.read("meta/seed", saved_seed);
  root.read("meta/certificate_id", certificate);
  root.read("meta/fspa4_protocol_sha256", fspa_hash);
  root.read("meta/oca1_protocol_sha256", oca_hash);
  root.read("meta/config_fnv1a64", saved_config_hash);
  root.read("meta/readout_policy", readout);
  return oca_tensor_string(schema) == "oca1.certified_anchor.v1" &&
         saved_seed.numel() == 1 && saved_seed.item<int64_t>() == seed &&
         oca_tensor_string(certificate) == kOcaCertificateId &&
         oca_tensor_string(fspa_hash) == kMpsrProtocolSha256 &&
         oca_tensor_string(oca_hash) == kOcaProtocolSha256 &&
         oca_tensor_string(saved_config_hash) == config_hash &&
         oca_tensor_string(readout) == kOcaReadoutPolicy;
}

[[nodiscard]] bool oca_load_archive(const std::filesystem::path &path,
                                    mtf::MtfJepaMaeVicreg &model,
                                    const torch::Device &device, int64_t seed,
                                    const std::string &config_hash) {
  torch::serialize::InputArchive root;
  root.load_from(path.string(), device);
  const bool metadata_exact = oca_read_metadata(root, seed, config_hash);
  torch::serialize::InputArchive model_archive;
  root.read("model", model_archive);
  model->load(model_archive);
  return metadata_exact;
}

void oca_copy_model_state(mtf::MtfJepaMaeVicreg &source,
                          mtf::MtfJepaMaeVicreg &destination,
                          const torch::Device &device) {
  torch::serialize::OutputArchive output;
  source->save(output);
  std::stringstream bytes(std::ios::in | std::ios::out | std::ios::binary);
  output.save_to(bytes);
  bytes.clear();
  bytes.seekg(0, std::ios::beg);
  torch::serialize::InputArchive input;
  input.load_from(bytes, device);
  destination->load(input);
}

[[nodiscard]] AttributionArm oca_arm(uint8_t mask) {
  static constexpr std::array<const char *, 16> names{
      "none",          "jepa",          "mae",          "jepa_mae",
      "tf",            "jepa_tf",       "mae_tf",       "jepa_mae_tf",
      "vicreg",        "jepa_vicreg",   "mae_vicreg",   "jepa_mae_vicreg",
      "tf_vicreg",     "jepa_tf_vicreg", "mae_tf_vicreg", "full"};
  if (mask >= names.size()) {
    throw std::runtime_error("OCA objective mask is invalid");
  }
  return {.name = names[mask],
          .lambda_jepa = (mask & 0x1U) != 0U ? 1.0 : 0.0,
          .lambda_mae = (mask & 0x2U) != 0U ? 0.25 : 0.0,
          .lambda_tf_align = (mask & 0x4U) != 0U ? 0.10 : 0.0,
          .lambda_vicreg = (mask & 0x8U) != 0U ? 0.05 : 0.0,
          .max_context_target_time_overlap = 0.50};
}

[[nodiscard]] bool oca_head_activity_exact(
    uint8_t mask, const OcaLegacyTrainingReceipt &receipt) {
  const bool predictor = (mask & 0x1U) != 0U;
  const bool decoder = (mask & 0x2U) != 0U;
  const bool vicreg = (mask & 0x8U) != 0U;
  return (predictor ? receipt.predictor_delta > 0.0
                    : receipt.predictor_delta == 0.0) &&
         (decoder ? receipt.mae_decoder_delta > 0.0
                  : receipt.mae_decoder_delta == 0.0) &&
         (vicreg ? receipt.vicreg_head_delta > 0.0
                 : receipt.vicreg_head_delta == 0.0);
}

[[nodiscard]] OcaLegacyTrainingReceipt oca_train_legacy_arm(
    mtf::MtfJepaMaeVicreg &model, const Dataset &ssl,
    const torch::Device &device, const AttributionArm &arm, uint8_t mask,
    int64_t seed, int64_t steps) {
  if (mask == 0U || mask >= 16U || steps <= 0) {
    throw std::runtime_error("OCA legacy training contract failed");
  }
  auto parameters = model->parameters();
  torch::optim::Adam optimizer(
      parameters, torch::optim::AdamOptions(kOcaOptimizerLearningRate));
  const auto initial = snapshot_parameters(model);
  OcaLegacyTrainingReceipt result{};
  result.steps = steps;
  result.losses.reserve(static_cast<std::size_t>(steps));
  result.row_hashes.reserve(static_cast<std::size_t>(steps));
  result.target_mask_hashes.reserve(static_cast<std::size_t>(steps));
  result.context_mask_hashes.reserve(static_cast<std::size_t>(steps));
  result.weak_view_hashes.reserve(static_cast<std::size_t>(steps));
  model->train();
  for (int64_t step = 0; step < steps; ++step) {
    const auto rows = training_rows(ssl, seed, step);
    const auto indices = torch::tensor(rows, torch::kInt64);
    const auto data = ssl.data.index_select(0, indices).to(device);
    const auto feature_mask = ssl.mask.index_select(0, indices).to(device);
    result.row_hashes.push_back(hash_batch_rows(rows));
    set_paired_rng(paired_step_seed(seed, step), device);
    optimizer.zero_grad();
    const auto output = model->forward(data, feature_mask);
    validate_weak_view_debug_tensors(output, data, feature_mask);
    validate_stratified_vicreg_forward(output, arm, kModelRowBatchSize);
    if (!output.jepa_target_mask.defined() ||
        !output.jepa_context_mask.defined() ||
        output.jepa_target_mask.scalar_type() != torch::kBool ||
        output.jepa_context_mask.scalar_type() != torch::kBool) {
      throw std::runtime_error("OCA legacy mask contract failed");
    }
    result.target_mask_hashes.push_back(
        hash_tensor_stable_bytes(output.jepa_target_mask));
    result.context_mask_hashes.push_back(
        hash_tensor_stable_bytes(output.jepa_context_mask));
    result.weak_view_hashes.push_back(weak_view_digest(output));
    const auto weights = attribution_arm_weights(arm, step);
    const auto loss = attribution_arm_loss(output, arm, weights);
    const double loss_value = loss.item<double>();
    result.finite = result.finite && std::isfinite(loss_value);
    result.losses.push_back(loss_value);
    result.component_loss_sums[0] += output.loss_jepa.item<double>();
    result.component_loss_sums[1] += output.loss_mae.item<double>();
    result.component_loss_sums[2] += output.loss_tf_align.item<double>();
    result.component_loss_sums[3] += output.loss_vicreg.item<double>();
    loss.backward();
    double gradient_square_sum = 0.0;
    for (const auto &parameter : parameters) {
      if (!parameter.grad().defined()) {
        continue;
      }
      result.finite = result.finite &&
                      torch::isfinite(parameter.grad()).all().item<bool>();
      gradient_square_sum +=
          parameter.grad().detach().pow(2).sum().item<double>();
    }
    const double gradient_norm = std::sqrt(gradient_square_sum);
    result.minimum_gradient_norm =
        std::min(result.minimum_gradient_norm, gradient_norm);
    result.maximum_gradient_norm =
        std::max(result.maximum_gradient_norm, gradient_norm);
    const double clip_factor =
        gradient_norm > kOcaGradientClipNorm
            ? kOcaGradientClipNorm / std::max(1.0e-30, gradient_norm)
            : 1.0;
    if (clip_factor < 1.0) {
      ++result.clipping_count;
      for (const auto &parameter : parameters) {
        if (parameter.grad().defined()) {
          parameter.grad().mul_(clip_factor);
        }
      }
    }
    const auto served_before = served_parameter_vector(model);
    optimizer.step();
    const auto served_after = served_parameter_vector(model);
    const double update_norm =
        (served_after - served_before).norm().item<double>();
    result.minimum_served_update_norm =
        std::min(result.minimum_served_update_norm, update_norm);
    result.maximum_served_update_norm =
        std::max(result.maximum_served_update_norm, update_norm);
    result.finite = result.finite && std::isfinite(gradient_norm) &&
                    std::isfinite(update_norm);
    model->update_target_network();
  }
  result.all_trainable_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::all_trainable);
  result.served_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::served);
  result.predictor_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::predictor);
  result.mae_decoder_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::mae_decoder);
  result.vicreg_head_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::vicreg_head);
  result.target_ema_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::target_ema);
  result.expected_partitions =
      result.all_trainable_delta > 0.0 && result.served_delta > 0.0 &&
      result.target_ema_delta > 0.0 && oca_head_activity_exact(mask, result);
  result.pass = result.finite &&
                result.losses.size() == static_cast<std::size_t>(steps) &&
                result.row_hashes.size() == static_cast<std::size_t>(steps) &&
                result.target_mask_hashes.size() ==
                    static_cast<std::size_t>(steps) &&
                result.context_mask_hashes.size() ==
                    static_cast<std::size_t>(steps) &&
                result.weak_view_hashes.size() ==
                    static_cast<std::size_t>(steps) &&
                result.minimum_gradient_norm > 0.0 &&
                result.minimum_served_update_norm > 0.0 &&
                result.expected_partitions;
  return result;
}

[[nodiscard]] OcaInterleavedTrainingResult oca_train_interleaved_arms(
    const Dataset &ssl, const torch::Device &device, int64_t seed,
    const std::vector<uint8_t> &masks, int64_t steps,
    bool load_certified_anchor, const std::string &anchor_config_hash) {
  if (masks.empty() || steps <= 0) {
    throw std::runtime_error("OCA interleaved training contract failed");
  }
  OcaInterleavedTrainingResult result{};
  result.models.reserve(masks.size());
  result.receipts.resize(masks.size());
  result.initialization_exact.resize(masks.size(), true);
  std::vector<std::unique_ptr<torch::optim::Adam>> optimizers;
  optimizers.reserve(masks.size());

  ParameterSnapshot canonical_initial{};
  for (std::size_t arm_index = 0; arm_index < masks.size(); ++arm_index) {
    const auto arm = oca_arm(masks[arm_index]);
    set_paired_rng(seed, device);
    auto model = mtf::MtfJepaMaeVicreg(attribution_config(device, arm));
    if (load_certified_anchor) {
      result.metadata_exact =
          result.metadata_exact &&
          oca_load_archive(oca_archive_path(seed), model, device, seed,
                           anchor_config_hash);
    }
    if (arm_index == 0) {
      canonical_initial = snapshot_parameters(model);
    } else {
      result.initialization_exact[arm_index] =
          parameter_max_abs_diff(model, canonical_initial) == 0.0;
    }
    auto &receipt = result.receipts[arm_index];
    receipt.steps = steps;
    receipt.losses.reserve(static_cast<std::size_t>(steps));
    receipt.row_hashes.reserve(static_cast<std::size_t>(steps));
    result.models.push_back(model);
    optimizers.push_back(std::make_unique<torch::optim::Adam>(
        result.models.back()->parameters(),
        torch::optim::AdamOptions(kOcaOptimizerLearningRate)));
  }

  for (auto &model : result.models) {
    model->train();
  }

  for (int64_t step = 0; step < steps; ++step) {
    const auto rows = training_rows(ssl, seed, step);
    const auto row_hash = hash_batch_rows(rows);
    const auto indices = torch::tensor(rows, torch::kInt64);
    const auto data = ssl.data.index_select(0, indices).to(device);
    const auto feature_mask = ssl.mask.index_select(0, indices).to(device);
    torch::Tensor reference_target_mask{};
    torch::Tensor reference_context_mask{};
    torch::Tensor reference_view_a_data{};
    torch::Tensor reference_view_a_feature_mask{};
    torch::Tensor reference_view_b_data{};
    torch::Tensor reference_view_b_feature_mask{};

    for (std::size_t arm_index = 0; arm_index < masks.size(); ++arm_index) {
      auto &model = result.models[arm_index];
      auto &optimizer = *optimizers[arm_index];
      auto &receipt = result.receipts[arm_index];
      const auto arm = oca_arm(masks[arm_index]);
      receipt.row_hashes.push_back(row_hash);
      set_paired_rng(paired_step_seed(seed, step), device);
      optimizer.zero_grad();
      const auto output = model->forward(data, feature_mask);
      validate_weak_view_debug_tensors(output, data, feature_mask);
      if (arm_index == 0) {
        reference_target_mask = output.jepa_target_mask.detach();
        reference_context_mask = output.jepa_context_mask.detach();
        reference_view_a_data = output.vicreg_view_a_data.detach();
        reference_view_a_feature_mask =
            output.vicreg_view_a_feature_mask.detach();
        reference_view_b_data = output.vicreg_view_b_data.detach();
        reference_view_b_feature_mask =
            output.vicreg_view_b_feature_mask.detach();
      }
      const auto exact_flags = torch::stack(std::vector<torch::Tensor>{
          torch::eq(output.jepa_target_mask,
                    reference_target_mask)
              .all(),
          torch::eq(output.jepa_context_mask,
                    reference_context_mask)
              .all(),
          torch::eq(output.vicreg_view_a_data,
                    reference_view_a_data)
              .all(),
          torch::eq(output.vicreg_view_a_feature_mask,
                    reference_view_a_feature_mask)
              .all(),
          torch::eq(output.vicreg_view_b_data,
                    reference_view_b_data)
              .all(),
          torch::eq(output.vicreg_view_b_feature_mask,
                    reference_view_b_feature_mask)
              .all()});
      const auto weights = attribution_arm_weights(arm, step);
      const auto loss = attribution_arm_loss(output, arm, weights);
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
        update_square =
            update_square +
            (served_parameters[parameter].detach() - served_before[parameter])
                .pow(2)
                .sum();
      }
      const auto update_norm_tensor = update_square.sqrt();
      model->update_target_network();
      const auto diagnostics =
          torch::stack(std::vector<torch::Tensor>{
                           loss.detach(), update_norm_tensor,
                           exact_flags.all().to(loss.scalar_type())})
              .to(torch::kCPU, torch::kFloat64);
      const double loss_value = diagnostics[0].item<double>();
      const double update_norm = diagnostics[1].item<double>();
      const bool schedule_exact = diagnostics[2].item<double>() == 1.0;
      receipt.losses.push_back(loss_value);
      receipt.minimum_served_update_norm =
          std::min(receipt.minimum_served_update_norm, update_norm);
      receipt.maximum_served_update_norm =
          std::max(receipt.maximum_served_update_norm, update_norm);
      receipt.finite = receipt.finite && std::isfinite(loss_value) &&
                       std::isfinite(gradient_norm) &&
                       std::isfinite(update_norm);
      result.schedule_exact = result.schedule_exact && schedule_exact;
    }
    const int64_t completed = step + 1;
    if (completed % 128 == 0 || completed == steps) {
      std::cout << "oca1.training.seed_" << seed
                << ".parallel_arms=" << masks.size() << '\n';
      std::cout << "oca1.training.seed_" << seed
                << ".completed_steps=" << completed << '\n';
      std::cout << "oca1.training.seed_" << seed
                << ".schedule_exact=" << result.schedule_exact << '\n'
                << std::flush;
    }
  }

  result.pass = result.metadata_exact && result.schedule_exact;
  for (std::size_t arm_index = 0; arm_index < masks.size(); ++arm_index) {
    auto &model = result.models[arm_index];
    auto &receipt = result.receipts[arm_index];
    const uint8_t mask = masks[arm_index];
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
        receipt.target_ema_delta > 0.0 &&
        oca_head_activity_exact(mask, receipt);
    receipt.pass = receipt.finite &&
                   receipt.losses.size() ==
                       static_cast<std::size_t>(steps) &&
                   receipt.row_hashes.size() ==
                       static_cast<std::size_t>(steps) &&
                   receipt.minimum_gradient_norm > 0.0 &&
                   receipt.minimum_served_update_norm > 0.0 &&
                   receipt.expected_partitions;
    result.pass = result.pass && result.initialization_exact[arm_index] &&
                  receipt.pass;
  }
  return result;
}

[[nodiscard]] std::filesystem::path
oca_seed_cache_path(std::string_view phase, int64_t seed) {
  return std::filesystem::path(".build") / "tests" / "oca1" /
         (std::string(phase) + "_seed_" + std::to_string(seed) +
          "_interleaved_v1.complete.pt");
}

[[nodiscard]] std::filesystem::path
oca_seed_cache_marker_path(const std::filesystem::path &path) {
  auto marker = path;
  marker += ".sha256";
  return marker;
}

[[nodiscard]] std::string
oca_seed_cache_config_manifest(const torch::Device &device, uint8_t mask) {
  return canonical_config_manifest(attribution_config(device, oca_arm(mask)));
}

[[nodiscard]] torch::Tensor
oca_u64_le_bytes_tensor(const std::vector<uint64_t> &values) {
  if (values.empty()) {
    throw std::runtime_error("OCA uint64 byte archive cannot be empty");
  }
  std::vector<uint8_t> bytes(values.size() * sizeof(uint64_t));
  for (std::size_t index = 0; index < values.size(); ++index) {
    for (std::size_t byte = 0; byte < sizeof(uint64_t); ++byte) {
      bytes[index * sizeof(uint64_t) + byte] = static_cast<uint8_t>(
          (values[index] >> (8U * static_cast<unsigned>(byte))) & 0xffU);
    }
  }
  return torch::from_blob(
             bytes.data(), {static_cast<int64_t>(bytes.size())},
             torch::TensorOptions().dtype(torch::kUInt8))
      .clone();
}

[[nodiscard]] std::vector<uint64_t>
oca_u64_le_bytes_vector(const torch::Tensor &encoded) {
  const auto bytes = encoded.detach().to(torch::kCPU, torch::kUInt8).contiguous();
  if (bytes.dim() != 1 || bytes.numel() <= 0 ||
      bytes.numel() % static_cast<int64_t>(sizeof(uint64_t)) != 0) {
    throw std::runtime_error("OCA uint64 byte archive shape failed");
  }
  const auto *data = bytes.data_ptr<uint8_t>();
  std::vector<uint64_t> values(
      static_cast<std::size_t>(bytes.numel()) / sizeof(uint64_t), 0U);
  for (std::size_t index = 0; index < values.size(); ++index) {
    for (std::size_t byte = 0; byte < sizeof(uint64_t); ++byte) {
      values[index] |= static_cast<uint64_t>(
                           data[index * sizeof(uint64_t) + byte])
                       << (8U * static_cast<unsigned>(byte));
    }
  }
  return values;
}

[[nodiscard]] std::array<std::string, 3>
oca_seed_cache_ssl_hashes(const Dataset &ssl) {
  return {oca_hex_u64(hash_tensor_stable_bytes(ssl.data)),
          oca_hex_u64(hash_tensor_stable_bytes(ssl.mask)),
          oca_hex_u64(hash_tensor_stable_bytes(ssl.target))};
}

void oca_save_seed_cache(
    std::string_view phase, const Dataset &ssl, int64_t seed,
    const std::vector<uint8_t> &masks, int64_t steps,
    bool load_certified_anchor,
    const torch::Device &device,
    const OcaInterleavedTrainingResult &result) {
  if (masks.empty() || steps <= 0 || result.models.size() != masks.size() ||
      result.receipts.size() != masks.size() ||
      result.initialization_exact.size() != masks.size() ||
      !result.metadata_exact || !result.schedule_exact || !result.pass) {
    throw std::runtime_error("OCA seed cache save contract failed");
  }
  const auto path = oca_seed_cache_path(phase, seed);
  const auto marker = oca_seed_cache_marker_path(path);
  const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
  auto temporary = path;
  temporary += ".tmp." + std::to_string(nonce);
  auto temporary_marker = marker;
  temporary_marker += ".tmp." + std::to_string(nonce);
  std::filesystem::create_directories(path.parent_path());
  const auto ssl_hashes = oca_seed_cache_ssl_hashes(ssl);

  torch::serialize::OutputArchive root;
  root.write("meta/schema", oca_string_tensor(kOcaSeedCacheSchema));
  root.write("meta/complete", torch::tensor({1}, torch::kInt64));
  root.write("meta/phase", oca_string_tensor(phase));
  root.write("meta/implementation",
             oca_string_tensor(kOcaSeedCacheImplementation));
  root.write("meta/oca1_protocol_sha256",
             oca_string_tensor(kOcaProtocolSha256));
  root.write("meta/fspa4_protocol_sha256",
             oca_string_tensor(kMpsrProtocolSha256));
  root.write("meta/certificate_id", oca_string_tensor(kOcaCertificateId));
  root.write("meta/readout_policy", oca_string_tensor(kOcaReadoutPolicy));
  root.write("meta/row_hash_encoding",
             oca_string_tensor("u64_le_bytes_v1"));
  root.write("meta/ssl_data_hash", oca_string_tensor(ssl_hashes[0]));
  root.write("meta/ssl_mask_hash", oca_string_tensor(ssl_hashes[1]));
  root.write("meta/ssl_target_hash", oca_string_tensor(ssl_hashes[2]));
  root.write("meta/seed", torch::tensor({seed}, torch::kInt64));
  root.write("meta/steps", torch::tensor({steps}, torch::kInt64));
  root.write("meta/completed_steps", torch::tensor({steps}, torch::kInt64));
  root.write("meta/total_steps", torch::tensor({steps}, torch::kInt64));
  root.write("meta/arm_count",
             torch::tensor({static_cast<int64_t>(masks.size())},
                           torch::kInt64));
  root.write("meta/optimizer_learning_rate",
             torch::tensor({kOcaOptimizerLearningRate}, torch::kFloat64));
  root.write("meta/gradient_clip_norm",
             torch::tensor({kOcaGradientClipNorm}, torch::kFloat64));
  root.write("meta/load_certified_anchor",
             torch::tensor({load_certified_anchor ? 1 : 0}, torch::kInt64));
  std::vector<int64_t> mask_values;
  mask_values.reserve(masks.size());
  for (const auto mask : masks) {
    mask_values.push_back(static_cast<int64_t>(mask));
  }
  root.write("meta/masks", torch::tensor(mask_values, torch::kInt64));
  root.write("meta/result_flags",
             torch::tensor({result.metadata_exact ? 1 : 0,
                            result.schedule_exact ? 1 : 0,
                            result.pass ? 1 : 0},
                           torch::kInt64));

  for (std::size_t arm_index = 0; arm_index < masks.size(); ++arm_index) {
    const auto mask = masks[arm_index];
    const auto &receipt = result.receipts[arm_index];
    if (!result.initialization_exact[arm_index] || !receipt.pass ||
        receipt.steps != steps ||
        receipt.losses.size() != static_cast<std::size_t>(steps) ||
        receipt.row_hashes.size() != static_cast<std::size_t>(steps) ||
        !receipt.target_mask_hashes.empty() ||
        !receipt.context_mask_hashes.empty() ||
        !receipt.weak_view_hashes.empty()) {
      throw std::runtime_error("OCA seed cache arm contract failed");
    }
    const std::string prefix = "arm_" + std::to_string(mask) + "/";
    torch::serialize::OutputArchive model_archive;
    result.models[arm_index]->save(model_archive);
    root.write(prefix + "model", model_archive);
    root.write(prefix + "config_manifest",
               oca_string_tensor(
                   oca_seed_cache_config_manifest(device, mask)));
    root.write(prefix + "row_hashes_u64_le",
               oca_u64_le_bytes_tensor(receipt.row_hashes));
    root.write(prefix + "losses",
               torch::tensor(receipt.losses,
                             torch::TensorOptions().dtype(torch::kFloat64)));
    root.write(
        prefix + "receipt_scalars",
        torch::tensor(
            {receipt.component_loss_sums[0], receipt.component_loss_sums[1],
             receipt.component_loss_sums[2], receipt.component_loss_sums[3],
             receipt.minimum_gradient_norm, receipt.maximum_gradient_norm,
             receipt.minimum_served_update_norm,
             receipt.maximum_served_update_norm, receipt.all_trainable_delta,
             receipt.served_delta, receipt.predictor_delta,
             receipt.mae_decoder_delta, receipt.vicreg_head_delta,
             receipt.target_ema_delta},
            torch::TensorOptions().dtype(torch::kFloat64)));
    root.write(prefix + "receipt_flags",
               torch::tensor(
                   std::vector<int64_t>{
                       result.initialization_exact[arm_index] ? 1LL : 0LL,
                       receipt.clipping_count, receipt.finite ? 1LL : 0LL,
                       receipt.expected_partitions ? 1LL : 0LL,
                       receipt.pass ? 1LL : 0LL},
                   torch::kInt64));
    root.write(prefix + "expected_empty_hash_vectors",
               torch::tensor({1, 1, 1}, torch::kInt64));
  }
  root.save_to(temporary.string());
  const auto checksum = digest::sha256_hex(rmc_read_file(temporary));
  {
    std::ofstream out(temporary_marker,
                      std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
      throw std::runtime_error("cannot create OCA seed cache marker");
    }
    out << checksum << '\n';
    out.close();
    if (!out) {
      throw std::runtime_error("OCA seed cache marker write failed");
    }
  }
  // The checksum marker is the commit record.  Remove it first so a process
  // interruption can never expose an old marker beside a new archive.
  std::filesystem::remove(marker);
  std::filesystem::remove(path);
  std::filesystem::rename(temporary, path);
  std::filesystem::rename(temporary_marker, marker);
}

[[nodiscard]] bool oca_load_seed_cache(
    std::string_view phase, const Dataset &ssl, const torch::Device &device,
    int64_t seed, const std::vector<uint8_t> &masks, int64_t steps,
    bool load_certified_anchor, OcaInterleavedTrainingResult &result) {
  const auto path = oca_seed_cache_path(phase, seed);
  const auto marker = oca_seed_cache_marker_path(path);
  if (!std::filesystem::exists(path) || !std::filesystem::exists(marker)) {
    return false;
  }
  std::ifstream marker_stream(marker, std::ios::binary);
  std::string expected_checksum;
  std::getline(marker_stream, expected_checksum);
  if (!marker_stream && expected_checksum.empty()) {
    throw std::runtime_error("OCA seed cache marker read failed");
  }
  if (!expected_checksum.empty() && expected_checksum.back() == '\r') {
    expected_checksum.pop_back();
  }
  const auto observed_checksum = digest::sha256_hex(rmc_read_file(path));
  if (expected_checksum.size() != 64 ||
      observed_checksum != expected_checksum) {
    throw std::runtime_error("OCA seed cache integrity failed");
  }

  torch::serialize::InputArchive root;
  root.load_from(path.string(), device);
  torch::Tensor schema{};
  torch::Tensor complete{};
  torch::Tensor saved_phase{};
  torch::Tensor implementation{};
  torch::Tensor protocol{};
  torch::Tensor fspa_protocol{};
  torch::Tensor certificate{};
  torch::Tensor readout{};
  torch::Tensor row_hash_encoding{};
  torch::Tensor ssl_data_hash{};
  torch::Tensor ssl_mask_hash{};
  torch::Tensor ssl_target_hash{};
  torch::Tensor saved_seed{};
  torch::Tensor saved_steps{};
  torch::Tensor completed_steps{};
  torch::Tensor total_steps{};
  torch::Tensor arm_count{};
  torch::Tensor optimizer_learning_rate{};
  torch::Tensor gradient_clip_norm{};
  torch::Tensor saved_anchor{};
  torch::Tensor saved_masks{};
  torch::Tensor result_flags{};
  root.read("meta/schema", schema);
  root.read("meta/complete", complete);
  root.read("meta/phase", saved_phase);
  root.read("meta/implementation", implementation);
  root.read("meta/oca1_protocol_sha256", protocol);
  root.read("meta/fspa4_protocol_sha256", fspa_protocol);
  root.read("meta/certificate_id", certificate);
  root.read("meta/readout_policy", readout);
  root.read("meta/row_hash_encoding", row_hash_encoding);
  root.read("meta/ssl_data_hash", ssl_data_hash);
  root.read("meta/ssl_mask_hash", ssl_mask_hash);
  root.read("meta/ssl_target_hash", ssl_target_hash);
  root.read("meta/seed", saved_seed);
  root.read("meta/steps", saved_steps);
  root.read("meta/completed_steps", completed_steps);
  root.read("meta/total_steps", total_steps);
  root.read("meta/arm_count", arm_count);
  root.read("meta/optimizer_learning_rate", optimizer_learning_rate);
  root.read("meta/gradient_clip_norm", gradient_clip_norm);
  root.read("meta/load_certified_anchor", saved_anchor);
  root.read("meta/masks", saved_masks);
  root.read("meta/result_flags", result_flags);
  const auto complete_cpu =
      complete.to(torch::kCPU, torch::kInt64).contiguous();
  const auto seed_cpu = saved_seed.to(torch::kCPU, torch::kInt64).contiguous();
  const auto steps_cpu =
      saved_steps.to(torch::kCPU, torch::kInt64).contiguous();
  const auto completed_cpu =
      completed_steps.to(torch::kCPU, torch::kInt64).contiguous();
  const auto total_cpu =
      total_steps.to(torch::kCPU, torch::kInt64).contiguous();
  const auto arm_count_cpu =
      arm_count.to(torch::kCPU, torch::kInt64).contiguous();
  const auto learning_rate_cpu =
      optimizer_learning_rate.to(torch::kCPU, torch::kFloat64).contiguous();
  const auto clip_norm_cpu =
      gradient_clip_norm.to(torch::kCPU, torch::kFloat64).contiguous();
  const auto anchor_cpu =
      saved_anchor.to(torch::kCPU, torch::kInt64).contiguous();
  const auto masks_cpu =
      saved_masks.to(torch::kCPU, torch::kInt64).contiguous();
  const auto global_cpu =
      result_flags.to(torch::kCPU, torch::kInt64).contiguous();
  const auto expected_ssl_hashes = oca_seed_cache_ssl_hashes(ssl);
  bool metadata_exact =
      oca_tensor_string(schema) == kOcaSeedCacheSchema &&
      complete_cpu.numel() == 1 && complete_cpu.item<int64_t>() == 1 &&
      oca_tensor_string(saved_phase) == phase &&
      oca_tensor_string(implementation) == kOcaSeedCacheImplementation &&
      oca_tensor_string(protocol) == kOcaProtocolSha256 &&
      oca_tensor_string(fspa_protocol) == kMpsrProtocolSha256 &&
      oca_tensor_string(certificate) == kOcaCertificateId &&
      oca_tensor_string(readout) == kOcaReadoutPolicy &&
      oca_tensor_string(row_hash_encoding) == "u64_le_bytes_v1" &&
      oca_tensor_string(ssl_data_hash) == expected_ssl_hashes[0] &&
      oca_tensor_string(ssl_mask_hash) == expected_ssl_hashes[1] &&
      oca_tensor_string(ssl_target_hash) == expected_ssl_hashes[2] &&
      seed_cpu.numel() == 1 && seed_cpu.item<int64_t>() == seed &&
      steps_cpu.numel() == 1 && steps_cpu.item<int64_t>() == steps &&
      completed_cpu.numel() == 1 &&
      completed_cpu.item<int64_t>() == steps && total_cpu.numel() == 1 &&
      total_cpu.item<int64_t>() == steps && arm_count_cpu.numel() == 1 &&
      arm_count_cpu.item<int64_t>() == static_cast<int64_t>(masks.size()) &&
      learning_rate_cpu.numel() == 1 &&
      learning_rate_cpu.item<double>() == kOcaOptimizerLearningRate &&
      clip_norm_cpu.numel() == 1 &&
      clip_norm_cpu.item<double>() == kOcaGradientClipNorm &&
      anchor_cpu.numel() == 1 &&
      (anchor_cpu.item<int64_t>() != 0) == load_certified_anchor &&
      masks_cpu.numel() == static_cast<int64_t>(masks.size()) &&
      global_cpu.numel() == 3;
  for (std::size_t index = 0; metadata_exact && index < masks.size(); ++index) {
    metadata_exact = masks_cpu[index].item<int64_t>() ==
                     static_cast<int64_t>(masks[index]);
  }
  if (!metadata_exact || global_cpu[0].item<int64_t>() != 1 ||
      global_cpu[1].item<int64_t>() != 1 ||
      global_cpu[2].item<int64_t>() != 1) {
    throw std::runtime_error("OCA seed cache metadata failed");
  }

  result = OcaInterleavedTrainingResult{};
  result.models.reserve(masks.size());
  result.receipts.resize(masks.size());
  result.initialization_exact.resize(masks.size(), false);
  result.metadata_exact = true;
  result.schedule_exact = true;
  result.pass = true;
  for (std::size_t arm_index = 0; arm_index < masks.size(); ++arm_index) {
    const auto mask = masks[arm_index];
    const std::string prefix = "arm_" + std::to_string(mask) + "/";
    torch::Tensor saved_config{};
    torch::Tensor saved_row_hashes{};
    torch::Tensor losses{};
    torch::Tensor scalars{};
    torch::Tensor flags{};
    torch::Tensor empty_hash_vectors{};
    root.read(prefix + "config_manifest", saved_config);
    root.read(prefix + "row_hashes_u64_le", saved_row_hashes);
    root.read(prefix + "losses", losses);
    root.read(prefix + "receipt_scalars", scalars);
    root.read(prefix + "receipt_flags", flags);
    root.read(prefix + "expected_empty_hash_vectors", empty_hash_vectors);
    if (oca_tensor_string(saved_config) !=
        oca_seed_cache_config_manifest(device, mask)) {
      throw std::runtime_error("OCA seed cache arm config failed");
    }
    set_paired_rng(seed, device);
    auto model =
        mtf::MtfJepaMaeVicreg(attribution_config(device, oca_arm(mask)));
    torch::serialize::InputArchive model_archive;
    root.read(prefix + "model", model_archive);
    model->load(model_archive);
    model->train();
    result.models.push_back(model);

    const auto loss_cpu =
        losses.to(torch::kCPU, torch::kFloat64).contiguous();
    const auto scalar_cpu =
        scalars.to(torch::kCPU, torch::kFloat64).contiguous();
    const auto flag_cpu = flags.to(torch::kCPU, torch::kInt64).contiguous();
    const auto empty_cpu =
        empty_hash_vectors.to(torch::kCPU, torch::kInt64).contiguous();
    if (loss_cpu.numel() != steps || scalar_cpu.numel() != 14 ||
        flag_cpu.numel() != 5 || empty_cpu.numel() != 3 ||
        empty_cpu[0].item<int64_t>() != 1 ||
        empty_cpu[1].item<int64_t>() != 1 ||
        empty_cpu[2].item<int64_t>() != 1) {
      throw std::runtime_error("OCA seed cache receipt shape failed");
    }
    auto &receipt = result.receipts[arm_index];
    receipt.steps = steps;
    const auto *loss_data = loss_cpu.data_ptr<double>();
    receipt.losses.assign(loss_data, loss_data + loss_cpu.numel());
    for (std::size_t component = 0; component < 4; ++component) {
      receipt.component_loss_sums[component] =
          scalar_cpu[static_cast<int64_t>(component)].item<double>();
    }
    receipt.minimum_gradient_norm = scalar_cpu[4].item<double>();
    receipt.maximum_gradient_norm = scalar_cpu[5].item<double>();
    receipt.minimum_served_update_norm = scalar_cpu[6].item<double>();
    receipt.maximum_served_update_norm = scalar_cpu[7].item<double>();
    receipt.all_trainable_delta = scalar_cpu[8].item<double>();
    receipt.served_delta = scalar_cpu[9].item<double>();
    receipt.predictor_delta = scalar_cpu[10].item<double>();
    receipt.mae_decoder_delta = scalar_cpu[11].item<double>();
    receipt.vicreg_head_delta = scalar_cpu[12].item<double>();
    receipt.target_ema_delta = scalar_cpu[13].item<double>();
    result.initialization_exact[arm_index] =
        flag_cpu[0].item<int64_t>() == 1;
    receipt.clipping_count = flag_cpu[1].item<int64_t>();
    receipt.finite = flag_cpu[2].item<int64_t>() == 1;
    receipt.expected_partitions = flag_cpu[3].item<int64_t>() == 1;
    receipt.pass = flag_cpu[4].item<int64_t>() == 1;
    receipt.row_hashes = oca_u64_le_bytes_vector(saved_row_hashes);
    std::vector<uint64_t> expected_row_hashes;
    expected_row_hashes.reserve(static_cast<std::size_t>(steps));
    for (int64_t step = 0; step < steps; ++step) {
      expected_row_hashes.push_back(
          hash_batch_rows(training_rows(ssl, seed, step)));
    }
    const bool reconstructed_pass =
        receipt.finite &&
        receipt.losses.size() == static_cast<std::size_t>(steps) &&
        receipt.row_hashes.size() == static_cast<std::size_t>(steps) &&
        receipt.row_hashes == expected_row_hashes &&
        receipt.target_mask_hashes.empty() &&
        receipt.context_mask_hashes.empty() &&
        receipt.weak_view_hashes.empty() &&
        receipt.minimum_gradient_norm > 0.0 &&
        receipt.minimum_served_update_norm > 0.0 &&
        receipt.expected_partitions && oca_head_activity_exact(mask, receipt);
    if (!result.initialization_exact[arm_index] || !receipt.pass ||
        !reconstructed_pass) {
      throw std::runtime_error("OCA seed cache receipt validation failed");
    }
  }
  return true;
}

[[nodiscard]] OcaInterleavedTrainingResult oca_train_or_resume_seed(
    std::string_view phase, const Dataset &ssl, const torch::Device &device,
    int64_t seed, const std::vector<uint8_t> &masks, int64_t steps,
    bool load_certified_anchor, const std::string &anchor_config_hash) {
  OcaInterleavedTrainingResult result{};
  const bool resumed = oca_load_seed_cache(
      phase, ssl, device, seed, masks, steps, load_certified_anchor, result);
  if (!resumed) {
    result = oca_train_interleaved_arms(
        ssl, device, seed, masks, steps, load_certified_anchor,
        anchor_config_hash);
    if (result.pass) {
      oca_save_seed_cache(phase, ssl, seed, masks, steps,
                          load_certified_anchor, device, result);
    }
  }
  const auto path = oca_seed_cache_path(phase, seed);
  const std::string root = "oca1.cache." + std::string(phase) + ".seed_" +
                           std::to_string(seed);
  std::cout << root << ".resumed=" << resumed << '\n';
  std::cout << root << ".path=" << path.generic_string() << '\n';
  std::cout << root << ".complete="
            << (std::filesystem::exists(path) &&
                std::filesystem::exists(oca_seed_cache_marker_path(path)))
            << '\n';
  if (std::filesystem::exists(path)) {
    std::cout << root << ".sha256="
              << digest::sha256_hex(rmc_read_file(path)) << '\n';
  }
  std::cout << std::flush;
  return result;
}

[[nodiscard]] bool
oca_training_schedule_exact(const OcaLegacyTrainingReceipt &left,
                             const OcaLegacyTrainingReceipt &right) {
  if (left.steps != right.steps || left.row_hashes != right.row_hashes ||
      left.target_mask_hashes != right.target_mask_hashes ||
      left.context_mask_hashes != right.context_mask_hashes ||
      left.weak_view_hashes.size() != right.weak_view_hashes.size()) {
    return false;
  }
  for (std::size_t step = 0; step < left.weak_view_hashes.size(); ++step) {
    if (!weak_view_digests_equal(left.weak_view_hashes[step],
                                 right.weak_view_hashes[step])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool
oca_training_receipt_exact(const OcaLegacyTrainingReceipt &left,
                           const OcaLegacyTrainingReceipt &right) {
  return oca_training_schedule_exact(left, right) &&
         left.losses == right.losses &&
         left.component_loss_sums == right.component_loss_sums &&
         left.minimum_gradient_norm == right.minimum_gradient_norm &&
         left.maximum_gradient_norm == right.maximum_gradient_norm &&
         left.minimum_served_update_norm ==
             right.minimum_served_update_norm &&
         left.maximum_served_update_norm ==
             right.maximum_served_update_norm &&
         left.clipping_count == right.clipping_count &&
         left.all_trainable_delta == right.all_trainable_delta &&
         left.served_delta == right.served_delta &&
         left.predictor_delta == right.predictor_delta &&
         left.mae_decoder_delta == right.mae_decoder_delta &&
         left.vicreg_head_delta == right.vicreg_head_delta &&
         left.target_ema_delta == right.target_ema_delta &&
         left.finite == right.finite &&
         left.expected_partitions == right.expected_partitions &&
         left.pass == right.pass;
}

[[nodiscard]] double oca_window_mean(const std::vector<double> &values,
                                     bool first) {
  constexpr std::size_t window = 8;
  if (values.size() < window) {
    throw std::runtime_error("OCA loss window is incomplete");
  }
  const auto begin = first ? values.begin() : values.end() - window;
  return std::accumulate(begin, begin + window, 0.0) /
         static_cast<double>(window);
}

void oca_emit_training(const std::string &root,
                       const OcaLegacyTrainingReceipt &receipt,
                       bool schedule_exact) {
  std::cout << root << ".steps=" << receipt.steps << '\n';
  std::cout << root << ".first_eight_loss_mean="
            << oca_window_mean(receipt.losses, true) << '\n';
  std::cout << root << ".last_eight_loss_mean="
            << oca_window_mean(receipt.losses, false) << '\n';
  std::cout << root << ".minimum_gradient_norm="
            << receipt.minimum_gradient_norm << '\n';
  std::cout << root << ".maximum_gradient_norm="
            << receipt.maximum_gradient_norm << '\n';
  std::cout << root << ".minimum_served_update_norm="
            << receipt.minimum_served_update_norm << '\n';
  std::cout << root << ".maximum_served_update_norm="
            << receipt.maximum_served_update_norm << '\n';
  std::cout << root << ".clipping_count=" << receipt.clipping_count << '\n';
  std::cout << root << ".served_parameter_delta=" << receipt.served_delta
            << '\n';
  std::cout << root << ".predictor_parameter_delta="
            << receipt.predictor_delta << '\n';
  std::cout << root << ".mae_decoder_parameter_delta="
            << receipt.mae_decoder_delta << '\n';
  std::cout << root << ".vicreg_head_parameter_delta="
            << receipt.vicreg_head_delta << '\n';
  std::cout << root << ".target_ema_parameter_delta="
            << receipt.target_ema_delta << '\n';
  std::cout << root << ".schedule_exact=" << schedule_exact << '\n';
  std::cout << root << ".pass=" << receipt.pass << '\n';
}

[[nodiscard]] OcaBootstrapAreaTable oca_bootstrap_area_table(
    const OcaFactorialEvaluations &evaluations, const torch::Tensor &target,
    const std::vector<torch::Tensor> &bootstrap_rows) {
  OcaBootstrapAreaTable result;
  result.reserve(bootstrap_rows.size());
  for (const auto &rows : bootstrap_rows) {
    std::array<std::array<double, 16>, 3> replicate{};
    for (std::size_t seed = 0; seed < 3; ++seed) {
      for (std::size_t mask = 0; mask < 16; ++mask) {
        replicate[seed][mask] =
            rssm_resampled_area(evaluations[seed][mask].probe, target, rows)
                .macro;
      }
    }
    result.push_back(std::move(replicate));
  }
  return result;
}

[[nodiscard]] OcaWeightedContrast oca_weighted_contrast(
    const OcaFactorialEvaluations &evaluations,
    const OcaBootstrapAreaTable &bootstrap,
    const std::array<double, 16> &weights) {
  OcaWeightedContrast result{};
  for (std::size_t seed = 0; seed < 3; ++seed) {
    for (std::size_t mask = 0; mask < 16; ++mask) {
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
      for (std::size_t mask = 0; mask < 16; ++mask) {
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

[[nodiscard]] std::array<double, 16> oca_main_effect_weights(
    std::size_t objective) {
  std::array<double, 16> result{};
  const uint8_t bit = static_cast<uint8_t>(1U << objective);
  for (uint8_t mask = 0; mask < 16U; ++mask) {
    result[mask] = (mask & bit) != 0U ? 1.0 / 8.0 : -1.0 / 8.0;
  }
  return result;
}

[[nodiscard]] std::array<double, 16> oca_interaction_weights(
    std::size_t left, std::size_t right) {
  std::array<double, 16> result{};
  const uint8_t left_bit = static_cast<uint8_t>(1U << left);
  const uint8_t right_bit = static_cast<uint8_t>(1U << right);
  for (uint8_t mask = 0; mask < 16U; ++mask) {
    const bool has_left = (mask & left_bit) != 0U;
    const bool has_right = (mask & right_bit) != 0U;
    result[mask] = has_left == has_right ? 1.0 / 4.0 : -1.0 / 4.0;
  }
  return result;
}

[[nodiscard]] std::array<double, 16> oca_pair_weights(std::size_t positive,
                                                      std::size_t negative) {
  std::array<double, 16> result{};
  result.at(positive) = 1.0;
  result.at(negative) = -1.0;
  return result;
}

void oca_emit_weighted_contrast(const std::string &root,
                                const OcaWeightedContrast &value) {
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

[[nodiscard]] RmcSummary oca_pair_summary(
    const std::array<RmcEvaluation, 3> &reference,
    const std::array<RmcEvaluation, 3> &candidate, const ProbeCurve &raw,
    const torch::Tensor &target, const RmcEvalTargets &targets,
    const std::vector<torch::Tensor> &bootstrap_rows, bool mechanics) {
  std::array<std::array<RmcEvaluation, 2>, 3> initial{};
  std::array<std::array<RmcEvaluation, 2>, 3> final{};
  for (std::size_t seed = 0; seed < 3; ++seed) {
    initial[seed][0] = reference[seed];
    initial[seed][1] = reference[seed];
    final[seed][0] = candidate[seed];
    final[seed][1] = candidate[seed];
  }
  return rmc_summarize(initial, final, raw, target,
                       targets.shuffled_evaluation,
                       targets.order_evaluation,
                       targets.shuffled_order_evaluation, bootstrap_rows,
                       mechanics);
}

void oca_emit_candidate_summary(const std::string &root,
                                const RmcSummary &summary) {
  const auto &candidate = summary.candidate[0];
  const auto &gate = summary.gate.neutral;
  std::cout << root << ".final_aulc=" << candidate.final.point << '\n';
  std::cout << root << ".final_aulc_low=" << candidate.final.interval.low
            << '\n';
  std::cout << root << ".final_aulc_high=" << candidate.final.interval.high
            << '\n';
  rmc_emit_contrast(root + ".trained_minus_reference",
                    candidate.gate.trained_minus_initialization);
  rmc_emit_contrast(root + ".final_minus_raw",
                    candidate.gate.final_minus_raw);
  rmc_emit_contrast(root + ".order_minus_reference",
                    candidate.gate.order_trained_minus_initialization);
  std::cout << root << ".final_order_aulc=" << candidate.final_order.point
            << '\n';
  std::cout << root << ".final_order_low="
            << candidate.final_order.interval.low << '\n';
  for (std::size_t family = 0; family < kFamilies; ++family) {
    std::cout << root << ".learned_family_" << kFamilyNames[family]
              << "_delta=" << candidate.gate.learned_family_deltas[family]
              << '\n';
  }
  std::cout << root << ".continuous_shuffle_high="
            << candidate.gate.continuous_shuffle_high << '\n';
  std::cout << root << ".order_shuffle_low="
            << candidate.gate.order_shuffle_low << '\n';
  std::cout << root << ".order_shuffle_high="
            << candidate.gate.order_shuffle_high << '\n';
  std::cout << root << ".gate.numeric_valid=" << gate.numeric_valid << '\n';
  std::cout << root << ".gate.mechanics_pass=" << gate.mechanics_pass
            << '\n';
  std::cout << root << ".gate.family_floor_pass=" << gate.family_floor_pass
            << '\n';
  std::cout << root << ".gate.raw_noninferiority_pass="
            << gate.raw_noninferiority_pass << '\n';
  std::cout << root << ".gate.order_point_pass=" << gate.order_point_pass
            << '\n';
  std::cout << root << ".gate.order_lower_pass=" << gate.order_lower_pass
            << '\n';
  std::cout << root << ".gate.order_retention_pass="
            << gate.order_retention_pass << '\n';
  std::cout << root << ".gate.continuous_shuffle_pass="
            << gate.continuous_shuffle_pass << '\n';
  std::cout << root << ".gate.order_shuffle_pass="
            << gate.order_shuffle_pass << '\n';
  std::cout << root << ".gate.geometry_pass=" << gate.geometry_pass << '\n';
  std::cout << root << ".gate.pass=" << gate.pass << '\n';
}

[[nodiscard]] std::string oca_write_factorial_artifact(
    const OcaFactorialEvaluations &evaluations,
    const OcaFactorialTraining &training,
    const std::array<bool, 16> &mechanics) {
  const auto path = std::filesystem::path(".build") / "tests" / "oca1" /
                    "factorial_complete_table.tsv";
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out.is_open()) {
    throw std::runtime_error("cannot create OCA factorial artifact");
  }
  out << std::setprecision(17) << std::boolalpha;
  out << "seed\tmask\tarm\tmechanics\taulc\torder_aulc\tclip_count"
         "\tmin_gradient\tmax_gradient\tmin_served_update"
         "\tserved_delta\tpredictor_delta\tmae_decoder_delta"
         "\tvicreg_head_delta\ttarget_ema_delta"
         "\tchannel\teffective\tparticipation\ttop\tactive\n";
  for (std::size_t seed = 0; seed < 3; ++seed) {
    for (std::size_t mask = 0; mask < 16; ++mask) {
      for (std::size_t channel = 0; channel < kChannels; ++channel) {
        const auto &receipt = training[seed][mask];
        const auto &geometry = evaluations[seed][mask].geometry[channel];
        out << kAttributionSeeds[seed] << '\t' << mask << '\t'
            << oca_arm(static_cast<uint8_t>(mask)).name << '\t'
            << mechanics[mask] << '\t' << evaluations[seed][mask].probe.area
            << '\t' << evaluations[seed][mask].order.area << '\t'
            << receipt.clipping_count << '\t'
            << receipt.minimum_gradient_norm << '\t'
            << receipt.maximum_gradient_norm << '\t'
            << receipt.minimum_served_update_norm << '\t'
            << receipt.served_delta << '\t' << receipt.predictor_delta << '\t'
            << receipt.mae_decoder_delta << '\t'
            << receipt.vicreg_head_delta << '\t' << receipt.target_ema_delta
            << '\t' << channel << '\t' << geometry.effective_rank_ratio
            << '\t' << geometry.participation_rank_ratio << '\t'
            << geometry.top_eigenvalue_share << '\t'
            << geometry.active_dimension_fraction << '\n';
      }
    }
  }
  out.close();
  if (!out) {
    throw std::runtime_error("OCA factorial artifact write failed");
  }
  return digest::sha256_hex(rmc_read_file(path));
}

void oca_open_confirmation(RmcData &data) {
  if (data.confirmation.data.defined()) {
    throw std::runtime_error("OCA confirmation was opened more than once");
  }
  data.confirmation = generate_dataset(5000000, 256);
  const auto raw_projection = make_raw_equal_width_projection();
  data.raw_confirmation =
      raw_equal_width_features(data.confirmation, raw_projection);
  normalize(data.confirmation, data.normalization);
  validate_dataset(data.confirmation);
  data.reversed_confirmation = rssm_reversed_dataset(data.confirmation);
}

[[nodiscard]] bool oca_seed_safeguards_pass(
    const RmcEvaluation &candidate, const RmcEvaluation &reference,
    double raw_area) {
  const auto candidate_family = rssm_family_areas(candidate.probe);
  const auto reference_family = rssm_family_areas(reference.probe);
  bool families = true;
  for (std::size_t family = 0; family < kFamilies; ++family) {
    families = families &&
               candidate_family[family] - reference_family[family] >=
                   rmc_gate::kFamilyFloor;
  }
  bool geometry = true;
  for (const auto &channel : candidate.geometry) {
    geometry = geometry &&
               channel.effective_rank_ratio >=
                   rmc_gate::kEffectiveRankFloor &&
               channel.participation_rank_ratio >=
                   rmc_gate::kParticipationRankFloor &&
               channel.top_eigenvalue_share <=
                   rmc_gate::kTopEigenvalueCeiling &&
               channel.active_dimension_fraction >=
                   rmc_gate::kActiveDimensionFloor;
  }
  return families &&
         candidate.probe.area - raw_area >=
             rmc_gate::kRawNoninferiorityMargin &&
         candidate.order.area >= rmc_gate::kFinalOrderPointFloor &&
         candidate.order.area - reference.order.area >=
             rmc_gate::kOrderRetentionMargin &&
         candidate.shuffled_probe.area <=
             rmc_gate::kContinuousShuffleUpper &&
         candidate.shuffled_order.area >= rmc_gate::kOrderShuffleLower &&
         candidate.shuffled_order.area <= rmc_gate::kOrderShuffleUpper &&
         geometry;
}

[[nodiscard]] bool oca_gradient_mechanics(const GradientDiagnostic &value) {
  bool valid = true;
  for (std::size_t branch = 0; branch < 4; ++branch) {
    valid = valid && std::isfinite(value.raw_loss[branch]) &&
            value.raw_loss[branch] > 0.0 &&
            std::isfinite(value.served_norm[branch]) &&
            value.served_norm[branch] >= 0.0;
  }
  std::size_t cosine = 0;
  for (std::size_t left = 0; left < 4; ++left) {
    for (std::size_t right = left + 1; right < 4; ++right) {
      const bool defined = value.served_norm[left] > 0.0 &&
                           value.served_norm[right] > 0.0;
      valid = valid &&
              (defined ? std::isfinite(value.served_cosine[cosine])
                       : std::isnan(value.served_cosine[cosine]));
      ++cosine;
    }
  }
  return valid && value.repeated_weak_views_exact &&
         value.parameters_and_ema_exact && value.training_state_exact &&
         value.optimizer_state_checked && value.optimizer_state_exact &&
         value.all_trainable_relative_decomposition_error <= 1.0e-5 &&
         value.served_relative_decomposition_error <= 1.0e-5 &&
         value.vicreg_component_trunk_relative_decomposition_error <= 1.0e-5 &&
         value.vicreg_component_head_relative_decomposition_error <= 1.0e-5;
}

[[nodiscard]] OcaBranchUpdate oca_one_branch_update(
    mtf::MtfJepaMaeVicreg &reference, const Dataset &ssl,
    const torch::Device &device, int64_t seed, std::size_t branch) {
  const uint8_t mask = static_cast<uint8_t>(1U << branch);
  const auto arm = oca_arm(mask);
  auto model = mtf::MtfJepaMaeVicreg(attribution_config(device, arm));
  oca_copy_model_state(reference, model, device);
  const auto initial = snapshot_parameters(model);
  auto parameters = model->parameters();
  torch::optim::Adam optimizer(
      parameters, torch::optim::AdamOptions(kOcaOptimizerLearningRate));
  const auto rows = training_rows(ssl, seed, 0);
  const auto indices = torch::tensor(rows, torch::kInt64);
  const auto data = ssl.data.index_select(0, indices).to(device);
  const auto feature_mask = ssl.mask.index_select(0, indices).to(device);
  model->train();
  set_paired_rng(paired_step_seed(seed, 0), device);
  optimizer.zero_grad();
  const auto output = model->forward(data, feature_mask);
  validate_weak_view_debug_tensors(output, data, feature_mask);
  const auto weights = attribution_arm_weights(arm, 0);
  const auto loss = attribution_arm_loss(output, arm, weights);
  OcaBranchUpdate result{};
  result.raw_loss = branch_tensor(output, branch).item<double>();
  result.weighted_loss = loss.item<double>();
  result.target_mask = output.jepa_target_mask.detach().to(torch::kCPU).clone();
  result.context_mask =
      output.jepa_context_mask.detach().to(torch::kCPU).clone();
  result.weak_views = weak_view_digest(output);
  loss.backward();
  double gradient_square_sum = 0.0;
  for (const auto &parameter : parameters) {
    if (parameter.grad().defined()) {
      gradient_square_sum +=
          parameter.grad().detach().pow(2).sum().item<double>();
    }
  }
  result.pre_clip_gradient_norm = std::sqrt(gradient_square_sum);
  result.clip_factor =
      result.pre_clip_gradient_norm > kOcaGradientClipNorm
          ? kOcaGradientClipNorm /
                std::max(1.0e-30, result.pre_clip_gradient_norm)
          : 1.0;
  if (result.clip_factor < 1.0) {
    for (const auto &parameter : parameters) {
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
  model->update_target_network();
  result.all_trainable_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::all_trainable);
  result.served_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::served);
  result.predictor_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::predictor);
  result.mae_decoder_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::mae_decoder);
  result.vicreg_head_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::vicreg_head);
  result.target_ema_delta = parameter_partition_max_abs_diff(
      model, initial, ParameterDeltaPartition::target_ema);
  const std::array<double, 3> head_deltas{
      result.predictor_delta, result.mae_decoder_delta,
      result.vicreg_head_delta};
  bool heads_exact = true;
  for (std::size_t head = 0; head < head_deltas.size(); ++head) {
    const bool expected_active =
        (branch == 0 && head == 0) || (branch == 1 && head == 1) ||
        (branch == 3 && head == 2);
    heads_exact = heads_exact &&
                  (expected_active ? head_deltas[head] > 0.0
                                   : head_deltas[head] == 0.0);
  }
  const bool served_and_ema_consistent =
      (result.served_delta > 0.0) == (result.target_ema_delta > 0.0);
  const bool some_expected_update =
      result.all_trainable_delta > 0.0 ||
      (branch == 2 && result.served_delta == 0.0);
  result.expected_partitions = heads_exact && served_and_ema_consistent &&
                               some_expected_update;
  result.finite = std::isfinite(result.raw_loss) && result.raw_loss > 0.0 &&
                  std::isfinite(result.weighted_loss) &&
                  std::isfinite(result.pre_clip_gradient_norm) &&
                  result.pre_clip_gradient_norm >= 0.0 &&
                  std::isfinite(result.served_update_norm) &&
                  result.served_update_norm >= 0.0;
  result.pass = result.finite && result.expected_partitions;
  return result;
}

[[nodiscard]] OcaPhaseOneReceipt oca_phase_one(
    mtf::MtfJepaMaeVicreg &reference, const Dataset &ssl,
    const torch::Device &device, int64_t seed) {
  const auto reference_state = oca_snapshot_state(reference);
  const auto full_arm = oca_arm(0x0fU);
  auto diagnostic_model =
      mtf::MtfJepaMaeVicreg(attribution_config(device, full_arm));
  oca_copy_model_state(reference, diagnostic_model, device);
  torch::optim::Adam optimizer(
      diagnostic_model->parameters(),
      torch::optim::AdamOptions(kOcaOptimizerLearningRate));
  OcaPhaseOneReceipt result{};
  result.gradient = checkpoint_gradient_diagnostic(
      diagnostic_model, ssl, device, full_arm, seed, 0, &optimizer);
  result.gradient_mechanics = oca_gradient_mechanics(result.gradient);
  for (std::size_t branch = 0; branch < result.updates.size(); ++branch) {
    result.updates[branch] =
        oca_one_branch_update(reference, ssl, device, seed, branch);
  }
  result.branch_replay_exact = true;
  for (std::size_t branch = 1; branch < result.updates.size(); ++branch) {
    result.branch_replay_exact =
        result.branch_replay_exact &&
        rssm_tensor_bytes_equal(result.updates[0].target_mask,
                                result.updates[branch].target_mask) &&
        rssm_tensor_bytes_equal(result.updates[0].context_mask,
                                result.updates[branch].context_mask) &&
        weak_view_digests_equal(result.updates[0].weak_views,
                                result.updates[branch].weak_views);
  }
  result.reference_unchanged = oca_state_exact(reference, reference_state);
  result.pass = result.gradient_mechanics && result.branch_replay_exact &&
                result.reference_unchanged;
  for (const auto &update : result.updates) {
    result.pass = result.pass && update.pass;
  }
  return result;
}

void oca_emit_gradient(const std::string &root,
                       const OcaPhaseOneReceipt &receipt) {
  for (std::size_t branch = 0; branch < 4; ++branch) {
    const std::string prefix =
        root + ".objective." + kAttributionBranchNames[branch];
    std::cout << prefix << ".raw_loss=" << receipt.gradient.raw_loss[branch]
              << '\n';
    std::cout << prefix << ".served_gradient_norm="
              << receipt.gradient.served_norm[branch] << '\n';
    std::cout << prefix << ".tokenizer_gradient_norm="
              << receipt.gradient.tokenizer_norm[branch] << '\n';
    std::cout << prefix << ".encoder_gradient_norm="
              << receipt.gradient.encoder_norm[branch] << '\n';
    std::cout << prefix << ".predictor_gradient_norm="
              << receipt.gradient.predictor_norm[branch] << '\n';
    std::cout << prefix << ".mae_decoder_gradient_norm="
              << receipt.gradient.mae_decoder_norm[branch] << '\n';
    std::cout << prefix << ".vicreg_head_gradient_norm="
              << receipt.gradient.vicreg_head_norm[branch] << '\n';
    std::cout << prefix << ".one_step_pre_clip_gradient_norm="
              << receipt.updates[branch].pre_clip_gradient_norm << '\n';
    std::cout << prefix << ".one_step_clip_factor="
              << receipt.updates[branch].clip_factor << '\n';
    std::cout << prefix << ".one_step_served_update_norm="
              << receipt.updates[branch].served_update_norm << '\n';
    std::cout << prefix << ".one_step_expected_partitions="
              << receipt.updates[branch].expected_partitions << '\n';
    std::cout << prefix << ".mechanically_connected="
              << (receipt.gradient.served_norm[branch] > 0.0 ||
                  receipt.updates[branch].served_update_norm > 0.0)
              << '\n';
  }
  std::size_t cosine = 0;
  for (std::size_t left = 0; left < 4; ++left) {
    for (std::size_t right = left + 1; right < 4; ++right) {
      std::cout << root << ".served_cosine."
                << kAttributionBranchNames[left] << "_vs_"
                << kAttributionBranchNames[right] << '='
                << receipt.gradient.served_cosine[cosine++] << '\n';
    }
  }
  std::cout << root << ".all_trainable_reconstruction_relative_error="
            << receipt.gradient.all_trainable_relative_decomposition_error
            << '\n';
  std::cout << root << ".served_reconstruction_relative_error="
            << receipt.gradient.served_relative_decomposition_error << '\n';
  std::cout << root << ".mask_and_view_replay_exact="
            << receipt.branch_replay_exact << '\n';
  std::cout << root << ".reference_unchanged="
            << receipt.reference_unchanged << '\n';
  std::cout << root << ".pass=" << receipt.pass << '\n';
}

[[nodiscard]] OcaArchiveReceipt oca_archive_roundtrip(
    const std::filesystem::path &path, mtf::MtfJepaMaeVicreg &model,
    const Dataset &evaluation_dataset, const RmcEvaluation &evaluation,
    const Dataset &probe_train, const Dataset &probe_validation,
    const Dataset &reversed_train, const Dataset &reversed_validation,
    const Dataset &reversed_evaluation, const RmcEvalTargets &targets,
    const torch::Device &device, int64_t seed,
    const mtf::mtf_jepa_mae_vicreg_config_t &anchor_config) {
  const auto config_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  const auto state = oca_snapshot_state(model);
  const auto capture = oca_capture_structured(model, evaluation_dataset, device);
  oca_save_archive(path, model, seed, config_hash);
  auto restored = mtf::MtfJepaMaeVicreg(anchor_config);
  OcaArchiveReceipt result{};
  result.metadata_exact =
      oca_load_archive(path, restored, device, seed, config_hash);
  result.state_exact = oca_state_exact(restored, state);
  const auto restored_capture =
      oca_capture_structured(restored, evaluation_dataset, device);
  result.structured_exact = oca_capture_exact(capture, restored_capture);
  const auto restored_evaluation = rmc_evaluate(
      restored, probe_train, probe_validation, evaluation_dataset,
      reversed_train, reversed_validation, reversed_evaluation, targets,
      device);
  result.evaluation_exact =
      rmc_evaluations_exact(evaluation, restored_evaluation) &&
      oca_evaluation_predictions_bytes_exact(evaluation,
                                             restored_evaluation);
  result.sha256 = digest::sha256_hex(rmc_read_file(path));
  result.pass = result.metadata_exact && result.state_exact &&
                result.structured_exact && result.evaluation_exact &&
                result.sha256.size() == 64;
  return result;
}

[[nodiscard]] bool oca_options_valid(const Options &options) {
  return options.device == "cuda" &&
         (options.steps < 0 || options.steps == kOcaFactorialSteps) &&
         (options.seeds < 0 ||
           options.seeds == static_cast<int64_t>(kAttributionSeeds.size()));
}

int run_oca_cache_smoke(const Options &options) {
  if (!oca_options_valid(options)) {
    throw std::runtime_error(
        "OCA-1 requires CUDA, 1536 factorial steps, and 3 seeds");
  }
  rmc_configure_cuda();
  const torch::Device device(torch::kCUDA, 0);
  const auto protocol_hash =
      digest::sha256_hex(rmc_read_file(std::filesystem::path(kOcaProtocolPath)));
  if (protocol_hash != kOcaProtocolSha256) {
    throw std::runtime_error("OCA-1 protocol hash mismatch");
  }
  auto data = rmc_make_data();
  constexpr int64_t smoke_steps = 2;
  const int64_t seed = kAttributionSeeds.front();
  const std::vector<uint8_t> masks{0x02U};
  auto trained = oca_train_interleaved_arms(
      data.ssl, device, seed, masks, smoke_steps,
      /*load_certified_anchor=*/false, /*anchor_config_hash=*/"");
  if (!trained.pass) {
    throw std::runtime_error("OCA seed cache smoke training failed");
  }
  const auto trained_state = oca_snapshot_state(trained.models.front());
  const auto trained_capture =
      oca_capture_structured(trained.models.front(), data.probe_validation,
                             device);
  oca_save_seed_cache("cache_smoke", data.ssl, seed, masks, smoke_steps,
                      /*load_certified_anchor=*/false, device, trained);
  OcaInterleavedTrainingResult restored{};
  const bool resumed = oca_load_seed_cache(
      "cache_smoke", data.ssl, device, seed, masks, smoke_steps,
      /*load_certified_anchor=*/false, restored);
  const bool global_exact =
      restored.metadata_exact == trained.metadata_exact &&
      restored.schedule_exact == trained.schedule_exact &&
      restored.pass == trained.pass &&
      restored.initialization_exact == trained.initialization_exact;
  const bool state_exact =
      resumed && restored.models.size() == 1 &&
      oca_state_exact(restored.models.front(), trained_state);
  const bool receipt_exact =
      resumed && restored.receipts.size() == 1 &&
      oca_training_receipt_exact(trained.receipts.front(),
                                 restored.receipts.front());
  bool structured_exact = false;
  if (resumed && restored.models.size() == 1) {
    const auto restored_capture =
        oca_capture_structured(restored.models.front(), data.probe_validation,
                               device);
    structured_exact = oca_capture_exact(trained_capture, restored_capture);
  }
  const auto path = oca_seed_cache_path("cache_smoke", seed);
  const bool complete_files =
      std::filesystem::exists(path) &&
      std::filesystem::exists(oca_seed_cache_marker_path(path));
  const bool pass = resumed && global_exact && state_exact && receipt_exact &&
                    structured_exact && complete_files;
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.oca1.cache_smoke.v1\n";
  std::cout << "experiment=four-objective-causal-attribution-cache-smoke\n";
  std::cout << "module_only=true\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "training_labels_used=false\n";
  std::cout << "optimizer_steps=" << smoke_steps << '\n';
  std::cout << "parallel_arms=" << masks.size() << '\n';
  std::cout << "oca1.cache_smoke.resumed=" << resumed << '\n';
  std::cout << "oca1.cache_smoke.global_exact=" << global_exact << '\n';
  std::cout << "oca1.cache_smoke.state_exact=" << state_exact << '\n';
  std::cout << "oca1.cache_smoke.receipt_exact=" << receipt_exact << '\n';
  std::cout << "oca1.cache_smoke.structured_exact=" << structured_exact
            << '\n';
  std::cout << "oca1.cache_smoke.complete_files=" << complete_files << '\n';
  std::cout << "oca1.cache_smoke.path=" << path.generic_string() << '\n';
  std::cout << "oca1.cache_smoke.pass=" << pass << '\n';
  return pass ? 0 : 3;
}

int run_oca_preflight(const Options &options) {
  if (!oca_options_valid(options)) {
    throw std::runtime_error(
        "OCA-1 requires CUDA, 1536 factorial steps, and 3 seeds");
  }
  rmc_configure_cuda();
  const torch::Device device(torch::kCUDA, 0);
  const auto protocol_hash =
      digest::sha256_hex(rmc_read_file(std::filesystem::path(kOcaProtocolPath)));
  auto data = rmc_make_data();
  const auto targets = rmc_make_targets(data, false);
  const int64_t seed = kAttributionSeeds.front();
  const auto full_arm = oca_arm(0x0fU);
  set_paired_rng(seed, device);
  auto model = mtf::MtfJepaMaeVicreg(attribution_config(device, full_arm));
  const auto state = oca_snapshot_state(model);
  const auto evaluation = rmc_evaluate(
      model, data.probe_train, data.probe_validation, data.development,
      data.reversed_train, data.reversed_validation,
      data.reversed_development, targets, device);
  const auto receipt = oca_archive_roundtrip(
      std::filesystem::path(".build") / "tests" / "oca1" /
          "preflight_untrained_seed_17.pt",
      model, data.development, evaluation, data.probe_train,
      data.probe_validation, data.reversed_train, data.reversed_validation,
      data.reversed_development, targets, device, seed, model->config());
  torch::optim::Adam optimizer(
      model->parameters(),
      torch::optim::AdamOptions(kOcaOptimizerLearningRate));
  const auto diagnostic = checkpoint_gradient_diagnostic(
      model, data.ssl, device, full_arm, seed, 0, &optimizer);
  const auto readout = rmc_readout_smoke(model, data.development, device);
  const bool state_unchanged = oca_state_exact(model, state);
  const bool pass = protocol_hash == kOcaProtocolSha256 && receipt.pass &&
                    oca_gradient_mechanics(diagnostic) && readout.pass &&
                    state_unchanged;
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.oca1.preflight.v1\n";
  std::cout << "experiment=four-objective-causal-attribution-preflight\n";
  std::cout << "module_only=true\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "training_labels_used=false\n";
  std::cout << "optimizer_steps=0\n";
  std::cout << "outer_augmentation_calls=0\n";
  std::cout << "oca1.protocol.sha256=" << protocol_hash << '\n';
  std::cout << "oca1.protocol.exact="
            << (protocol_hash == kOcaProtocolSha256) << '\n';
  std::cout << "oca1.archive.metadata_exact=" << receipt.metadata_exact
            << '\n';
  std::cout << "oca1.archive.state_exact=" << receipt.state_exact << '\n';
  std::cout << "oca1.archive.structured_exact=" << receipt.structured_exact
            << '\n';
  std::cout << "oca1.archive.evaluation_exact=" << receipt.evaluation_exact
            << '\n';
  std::cout << "oca1.gradient_mechanics="
            << oca_gradient_mechanics(diagnostic) << '\n';
  std::cout << "oca1.readout_smoke=" << readout.pass << '\n';
  std::cout << "oca1.reference_state_unchanged=" << state_unchanged << '\n';
  std::cout << "oca1.preflight.pass=" << pass << '\n';
  return pass ? 0 : 3;
}

int run_oca_reference(const Options &options) {
  if (!oca_options_valid(options)) {
    throw std::runtime_error(
        "OCA-1 requires CUDA, 1536 factorial steps, and 3 seeds");
  }
  rmc_configure_cuda();
  const torch::Device device(torch::kCUDA, 0);
  const auto protocol_hash =
      digest::sha256_hex(rmc_read_file(std::filesystem::path(kOcaProtocolPath)));
  if (protocol_hash != kOcaProtocolSha256) {
    throw std::runtime_error("OCA-1 protocol hash mismatch");
  }
  auto data = rmc_make_data();
  const auto targets = rmc_make_targets(data, false);
  const auto bootstrap_rows = rmc_bootstrap_rows(256);
  if (!rmc_bootstrap_rows_valid(bootstrap_rows, 256)) {
    throw std::runtime_error("OCA-1 bootstrap table failed");
  }
  const auto raw = rssm_probe_curve(
      data.raw_train, data.raw_validation, data.raw_development,
      data.probe_train.target, data.probe_validation.target,
      data.development.target, /*dual=*/true);
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  std::array<std::array<RmcEvaluation, 2>, 3> initial{};
  std::array<std::array<RmcEvaluation, 2>, 3> teacher_evaluations{};
  std::array<std::array<RmcEvaluation, 2>, 3> shadow_evaluations{};
  std::array<std::array<RmcEvaluation, 2>, 3> final{};
  std::array<Embeddings, 3> cached_targets{};
  std::vector<mtf::MtfJepaMaeVicreg> anchors;
  anchors.reserve(3);
  bool teacher_mechanics = true;

  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.oca1.reference.v1\n";
  std::cout << "experiment=four-objective-causal-attribution-reference\n";
  std::cout << "module_only=true\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "readout_policy=" << kOcaReadoutPolicy << '\n';
  std::cout << "training_labels_used=false\n";
  std::cout << "outer_augmentation_calls=0\n";
  std::cout << "oca1.protocol.sha256=" << protocol_hash << '\n';

  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    set_paired_rng(seed, device);
    auto model = mtf::MtfJepaMaeVicreg(anchor_config);
    const auto initial_evaluation = rmc_evaluate(
        model, data.probe_train, data.probe_validation, data.development,
        data.reversed_train, data.reversed_validation,
        data.reversed_development, targets, device);
    const bool initialization_reproduced =
        std::abs(initial_evaluation.probe.area -
                 kExpectedInitialAulc[seed_index]) <= 1.0e-9;
    const auto teacher =
        fspa_train(model, data.ssl, device, seed, kGpwdTeacherSteps);
    const auto ssl_capture =
        rmc_extract_sparse_embeddings(model, data.ssl, device);
    const auto spectral = mpsr_fit(ssl_capture);
    cached_targets[seed_index] = mpsr_apply(ssl_capture, spectral);
    const auto repaired = mpsr_evaluate_teacher_and_shadow(
        model, data.probe_train, data.probe_validation, data.development,
        data.reversed_train, data.reversed_validation,
        data.reversed_development, targets, spectral, device);
    initial[seed_index][0] = initial_evaluation;
    initial[seed_index][1] = initial_evaluation;
    teacher_evaluations[seed_index][0] = repaired.teacher;
    teacher_evaluations[seed_index][1] = repaired.teacher;
    shadow_evaluations[seed_index][0] = repaired.shadow;
    shadow_evaluations[seed_index][1] = repaired.shadow;
    teacher_mechanics = teacher_mechanics && initialization_reproduced &&
                        teacher.pass && spectral.pass;
    std::cout << "oca1.reference.seed_" << seed
              << ".initialization_reproduced=" << initialization_reproduced
              << '\n';
    std::cout << "oca1.reference.seed_" << seed
              << ".teacher_mechanics=" << teacher.pass << '\n';
    std::cout << "oca1.reference.seed_" << seed
              << ".spectral_mechanics=" << spectral.pass << '\n';
    std::cout << "oca1.reference.seed_" << seed
              << ".initial_aulc=" << initial_evaluation.probe.area << '\n';
    std::cout << "oca1.reference.seed_" << seed
              << ".teacher_aulc=" << repaired.teacher.probe.area << '\n';
    std::cout << "oca1.reference.seed_" << seed
              << ".shadow_aulc=" << repaired.shadow.probe.area << '\n';
    anchors.push_back(model);
  }

  const auto teacher_summary = rmc_summarize(
      initial, teacher_evaluations, raw, data.development.target,
      targets.shuffled_evaluation, targets.order_evaluation,
      targets.shuffled_order_evaluation, bootstrap_rows, teacher_mechanics);
  const bool teacher_reproduced =
      teacher_mechanics && gpwd_semantics_pass(teacher_summary.gate.neutral);
  const auto shadow_summary = rmc_summarize(
      initial, shadow_evaluations, raw, data.development.target,
      targets.shuffled_evaluation, targets.order_evaluation,
      targets.shuffled_order_evaluation, bootstrap_rows, teacher_reproduced);
  const bool shadow_pass = shadow_summary.gate.neutral.pass;
  std::cout << "oca1.reference.teacher_semantics_reproduced="
            << teacher_reproduced << '\n';
  std::cout << "oca1.reference.shadow_development_gate_pass=" << shadow_pass
            << '\n';
  if (!teacher_reproduced || !shadow_pass) {
    std::cout << "oca1.reference.student_opened=false\n";
    std::cout << "oca1.reference.archives_written=false\n";
    std::cout << "oca1.factorial_authorized=false\n";
    std::cout << "execution_status=oca1_teacher_or_shadow_gate_failed\n";
    return 3;
  }

  bool mechanics = teacher_reproduced && shadow_pass;
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    const auto student = gpwd_train_student(
        anchors[seed_index], data.ssl, cached_targets[seed_index], device, seed);
    const auto final_evaluation = rmc_evaluate(
        anchors[seed_index], data.probe_train, data.probe_validation,
        data.development, data.reversed_train, data.reversed_validation,
        data.reversed_development, targets, device);
    final[seed_index][0] = final_evaluation;
    final[seed_index][1] = final_evaluation;
    mechanics = mechanics && student.pass;
    std::cout << "oca1.reference.seed_" << seed
              << ".student_mechanics=" << student.pass << '\n';
    std::cout << "oca1.reference.seed_" << seed
              << ".final_aulc=" << final_evaluation.probe.area << '\n';
  }

  const auto summary = rmc_summarize(
      initial, final, raw, data.development.target, targets.shuffled_evaluation,
      targets.order_evaluation, targets.shuffled_order_evaluation,
      bootstrap_rows, mechanics);
  const bool reference_gate = summary.gate.neutral.pass;
  rmc_emit_contrast("oca1.reference.final_minus_initial",
                    summary.candidate[0]
                        .gate.trained_minus_initialization);
  rmc_emit_contrast("oca1.reference.final_minus_raw",
                    summary.candidate[0].gate.final_minus_raw);
  std::cout << "oca1.reference.final_aulc="
            << summary.candidate[0].final.point << '\n';
  std::cout << "oca1.reference.mechanics=" << mechanics << '\n';
  std::cout << "oca1.reference.development_gate_pass=" << reference_gate
            << '\n';
  if (!reference_gate) {
    std::cout << "oca1.reference.archives_written=false\n";
    std::cout << "oca1.factorial_authorized=false\n";
    std::cout << "execution_status=oca1_reference_gate_failed\n";
    return 3;
  }

  bool custody = true;
  bool phase_one = true;
  const auto config_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    const auto archive = oca_archive_roundtrip(
        oca_archive_path(seed), anchors[seed_index], data.development,
        final[seed_index][0], data.probe_train, data.probe_validation,
        data.reversed_train, data.reversed_validation,
        data.reversed_development, targets, device, seed, anchor_config);
    const bool archive_hash_pinned =
        archive.sha256 == kOcaAnchorSha256[seed_index];
    custody = custody && archive.pass && archive_hash_pinned;
    std::cout << "oca1.reference.seed_" << seed
              << ".archive.metadata_exact=" << archive.metadata_exact << '\n';
    std::cout << "oca1.reference.seed_" << seed
              << ".archive.state_exact=" << archive.state_exact << '\n';
    std::cout << "oca1.reference.seed_" << seed
              << ".archive.structured_exact=" << archive.structured_exact
              << '\n';
    std::cout << "oca1.reference.seed_" << seed
              << ".archive.evaluation_exact=" << archive.evaluation_exact
              << '\n';
    std::cout << "oca1.reference.seed_" << seed
              << ".archive.sha256=" << archive.sha256 << '\n';
    std::cout << "oca1.reference.seed_" << seed
              << ".archive.sha256_pinned=" << archive_hash_pinned << '\n';

    set_paired_rng(seed, device);
    auto initialized =
        mtf::MtfJepaMaeVicreg(attribution_config(device, oca_arm(0x0fU)));
    const auto init_receipt = oca_phase_one(initialized, data.ssl, device, seed);
    oca_emit_gradient("oca1.phase1.initial.seed_" + std::to_string(seed),
                      init_receipt);

    auto restored_anchor =
        mtf::MtfJepaMaeVicreg(attribution_config(device, oca_arm(0x0fU)));
    const bool metadata = oca_load_archive(
        oca_archive_path(seed), restored_anchor, device, seed, config_hash);
    const auto anchor_receipt =
        oca_phase_one(restored_anchor, data.ssl, device, seed);
    oca_emit_gradient("oca1.phase1.anchor.seed_" + std::to_string(seed),
                      anchor_receipt);
    phase_one = phase_one && metadata && init_receipt.pass &&
                anchor_receipt.pass;
  }
  const bool pass = mechanics && reference_gate && custody && phase_one;
  std::cout << "oca1.reference.custody_pass=" << custody << '\n';
  std::cout << "oca1.phase1.pass=" << phase_one << '\n';
  std::cout << "oca1.factorial_authorized=" << pass << '\n';
  std::cout << "oca1.reference.pass=" << pass << '\n';
  std::cout << "execution_status=oca1_reference_measurements_complete\n";
  return pass ? 0 : 3;
}

int run_oca_factorial(const Options &options) {
  if (!oca_options_valid(options)) {
    throw std::runtime_error(
        "OCA-1 requires CUDA, 1536 factorial steps, and 3 seeds");
  }
  rmc_configure_cuda();
  const torch::Device device(torch::kCUDA, 0);
  const auto protocol_hash =
      digest::sha256_hex(rmc_read_file(std::filesystem::path(kOcaProtocolPath)));
  if (protocol_hash != kOcaProtocolSha256) {
    throw std::runtime_error("OCA-1 protocol hash mismatch");
  }
  auto data = rmc_make_data();
  const auto targets = rmc_make_targets(data, false);
  const auto bootstrap_rows = rmc_bootstrap_rows(256);
  if (!rmc_bootstrap_rows_valid(bootstrap_rows, 256)) {
    throw std::runtime_error("OCA-1 bootstrap table failed");
  }
  const auto raw = rssm_probe_curve(
      data.raw_train, data.raw_validation, data.raw_development,
      data.probe_train.target, data.probe_validation.target,
      data.development.target, /*dual=*/true);
  const auto anchor_config =
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]);
  const auto anchor_config_hash =
      oca_hex_u64(fnv1a64(canonical_config_manifest(anchor_config)));

  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.oca1.factorial.v1\n";
  std::cout << "experiment=four-objective-causal-attribution-factorial\n";
  std::cout << "module_only=true\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "readout_policy=" << kOcaReadoutPolicy << '\n';
  std::cout << "training_labels_used=false\n";
  std::cout << "outer_augmentation_calls=0\n";
  std::cout << "factorial_updates_per_nonzero_arm=" << kOcaFactorialSteps
            << '\n';
  std::cout << "anchor_challenge_updates_per_arm="
            << kOcaAnchorChallengeSteps << '\n';
  std::cout << "gradient_clip_norm=" << kOcaGradientClipNorm << '\n';
  std::cout << "oca1.protocol.sha256=" << protocol_hash << '\n';

  std::array<RmcEvaluation, 3> initial_reference{};
  std::array<RmcEvaluation, 3> anchor_reference{};
  std::array<std::string, 3> anchor_hashes{};
  bool anchor_custody = true;
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    set_paired_rng(seed, device);
    auto initialized = mtf::MtfJepaMaeVicreg(anchor_config);
    initial_reference[seed_index] = rmc_evaluate(
        initialized, data.probe_train, data.probe_validation,
        data.development, data.reversed_train, data.reversed_validation,
        data.reversed_development, targets, device);

    auto first = mtf::MtfJepaMaeVicreg(anchor_config);
    auto second = mtf::MtfJepaMaeVicreg(anchor_config);
    const bool first_metadata = oca_load_archive(
        oca_archive_path(seed), first, device, seed, anchor_config_hash);
    const bool second_metadata = oca_load_archive(
        oca_archive_path(seed), second, device, seed, anchor_config_hash);
    const auto first_state = oca_snapshot_state(first);
    const bool state_exact = oca_state_exact(second, first_state);
    const auto first_capture =
        oca_capture_structured(first, data.development, device);
    const auto second_capture =
        oca_capture_structured(second, data.development, device);
    const bool structured_exact =
        oca_capture_exact(first_capture, second_capture);
    const auto first_evaluation = rmc_evaluate(
        first, data.probe_train, data.probe_validation, data.development,
        data.reversed_train, data.reversed_validation,
        data.reversed_development, targets, device);
    const auto second_evaluation = rmc_evaluate(
        second, data.probe_train, data.probe_validation, data.development,
        data.reversed_train, data.reversed_validation,
        data.reversed_development, targets, device);
    const bool evaluation_exact =
        rmc_evaluations_exact(first_evaluation, second_evaluation) &&
        oca_evaluation_predictions_bytes_exact(first_evaluation,
                                               second_evaluation);
    anchor_reference[seed_index] = first_evaluation;
    anchor_hashes[seed_index] =
        digest::sha256_hex(rmc_read_file(oca_archive_path(seed)));
    const bool seed_custody = first_metadata && second_metadata && state_exact &&
                              structured_exact && evaluation_exact &&
                              anchor_hashes[seed_index] ==
                                  kOcaAnchorSha256[seed_index];
    anchor_custody = anchor_custody && seed_custody;
    std::cout << "oca1.factorial.anchor.seed_" << seed
              << ".sha256=" << anchor_hashes[seed_index] << '\n';
    std::cout << "oca1.factorial.anchor.seed_" << seed
              << ".custody_pass=" << seed_custody << '\n';
  }
  const auto anchor_summary = oca_pair_summary(
      initial_reference, anchor_reference, raw, data.development.target,
      targets, bootstrap_rows, anchor_custody);
  const bool anchor_gate = anchor_summary.gate.neutral.pass;
  oca_emit_candidate_summary("oca1.factorial.anchor_revalidation",
                             anchor_summary);
  if (!anchor_custody || !anchor_gate) {
    for (const auto *objective : kAttributionBranchNames) {
      std::cout << "oca1.verdict." << objective << "=unresolved\n";
    }
    std::cout << "oca1.factorial.opened=false\n";
    std::cout << "oca1.anchor_challenge.opened=false\n";
    std::cout << "execution_status=oca1_anchor_revalidation_failed\n";
    return 3;
  }

  std::array<bool, 4> phase_one_connected{};
  bool phase_one_valid = true;
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    set_paired_rng(seed, device);
    auto initialized =
        mtf::MtfJepaMaeVicreg(attribution_config(device, oca_arm(0x0fU)));
    const auto initialized_receipt =
        oca_phase_one(initialized, data.ssl, device, seed);
    auto anchor =
        mtf::MtfJepaMaeVicreg(attribution_config(device, oca_arm(0x0fU)));
    const bool metadata = oca_load_archive(
        oca_archive_path(seed), anchor, device, seed, anchor_config_hash);
    const auto anchor_receipt = oca_phase_one(anchor, data.ssl, device, seed);
    phase_one_valid = phase_one_valid && metadata && initialized_receipt.pass &&
                      anchor_receipt.pass;
    for (std::size_t objective = 0; objective < 4; ++objective) {
      const bool initialized_connected =
          initialized_receipt.gradient.served_norm[objective] > 0.0 ||
          initialized_receipt.updates[objective].served_update_norm > 0.0;
      const bool anchor_connected =
          anchor_receipt.gradient.served_norm[objective] > 0.0 ||
          anchor_receipt.updates[objective].served_update_norm > 0.0;
      phase_one_connected[objective] = phase_one_connected[objective] ||
                                       initialized_connected ||
                                       anchor_connected;
    }
  }
  std::cout << "oca1.factorial.phase1_authorization_pass=" << phase_one_valid
            << '\n';
  for (std::size_t objective = 0; objective < 4; ++objective) {
    std::cout << "oca1.factorial.phase1."
              << kAttributionBranchNames[objective] << ".connected="
              << phase_one_connected[objective] << '\n';
  }
  if (!phase_one_valid) {
    for (const auto *objective : kAttributionBranchNames) {
      std::cout << "oca1.verdict." << objective << "=unresolved\n";
    }
    std::cout << "oca1.factorial.opened=false\n";
    std::cout << "oca1.anchor_challenge.opened=false\n";
    std::cout << "execution_status=oca1_phase1_authorization_failed\n";
    return 3;
  }

  std::cout << "oca1.factorial.opened=true\n" << std::flush;
  OcaFactorialEvaluations evaluations{};
  OcaFactorialTraining training{};
  std::array<bool, 16> arm_mechanics{};
  arm_mechanics.fill(true);
  std::vector<uint8_t> factorial_masks;
  for (uint8_t mask = 1U; mask < 16U; ++mask) {
    factorial_masks.push_back(mask);
  }
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    set_paired_rng(seed, device);
    auto zero_model = mtf::MtfJepaMaeVicreg(
        attribution_config(device, oca_arm(0U)));
    evaluations[seed_index][0] = rmc_evaluate(
        zero_model, data.probe_train, data.probe_validation, data.development,
        data.reversed_train, data.reversed_validation,
        data.reversed_development, targets, device);
    const bool zero_matches_reference = rmc_evaluations_exact(
        evaluations[seed_index][0], initial_reference[seed_index]);
    arm_mechanics[0] = arm_mechanics[0] && zero_matches_reference;
    training[seed_index][0].minimum_gradient_norm = 0.0;
    training[seed_index][0].minimum_served_update_norm = 0.0;
    training[seed_index][0].finite = true;
    training[seed_index][0].expected_partitions = true;
    training[seed_index][0].pass = zero_matches_reference;
    auto interleaved = oca_train_or_resume_seed(
        "factorial", data.ssl, device, seed, factorial_masks,
        kOcaFactorialSteps, /*load_certified_anchor=*/false,
        anchor_config_hash);
    for (std::size_t arm_index = 0; arm_index < factorial_masks.size();
         ++arm_index) {
      const uint8_t mask = factorial_masks[arm_index];
      const auto arm = oca_arm(mask);
      evaluations[seed_index][mask] = rmc_evaluate(
          interleaved.models[arm_index], data.probe_train,
          data.probe_validation, data.development,
          data.reversed_train, data.reversed_validation,
          data.reversed_development, targets, device);
      arm_mechanics[mask] =
          arm_mechanics[mask] && interleaved.initialization_exact[arm_index] &&
          interleaved.receipts[arm_index].pass &&
          interleaved.schedule_exact && interleaved.pass;
      training[seed_index][mask] =
          std::move(interleaved.receipts[arm_index]);
      const std::string root =
          "oca1.factorial.seed_" + std::to_string(seed) + ".arm." + arm.name;
      std::cout << root << ".initialization_exact="
                << interleaved.initialization_exact[arm_index] << '\n';
      std::cout << root << ".final_aulc="
                << evaluations[seed_index][mask].probe.area << '\n';
      oca_emit_training(root + ".training", training[seed_index][mask],
                        interleaved.schedule_exact);
      std::cout << root << ".complete=true\n" << std::flush;
    }
  }

  bool factorial_mechanics = anchor_custody && anchor_gate;
  std::array<RmcSummary, 16> arm_summaries{};
  for (std::size_t mask = 0; mask < 16; ++mask) {
    std::array<RmcEvaluation, 3> reference{};
    std::array<RmcEvaluation, 3> candidate{};
    for (std::size_t seed = 0; seed < 3; ++seed) {
      reference[seed] = evaluations[seed][0];
      candidate[seed] = evaluations[seed][mask];
    }
    arm_summaries[mask] = oca_pair_summary(
        reference, candidate, raw, data.development.target, targets,
        bootstrap_rows, arm_mechanics[mask]);
    const std::string root = "oca1.factorial.arm." +
                             std::string(oca_arm(static_cast<uint8_t>(mask)).name);
    oca_emit_candidate_summary(root, arm_summaries[mask]);
    std::cout << root << ".mechanics=" << arm_mechanics[mask] << '\n';
    factorial_mechanics = factorial_mechanics && arm_mechanics[mask] &&
                          arm_summaries[mask].gate.neutral.numeric_valid;
  }

  const auto bootstrap =
      oca_bootstrap_area_table(evaluations, data.development.target,
                               bootstrap_rows);
  std::array<OcaWeightedContrast, 4> main_effects{};
  std::array<std::array<OcaWeightedContrast, 4>, 4> interactions{};
  std::array<OcaWeightedContrast, 4> leave_one_out{};
  std::array<OcaWeightedContrast, 4> standalone{};
  for (std::size_t objective = 0; objective < 4; ++objective) {
    main_effects[objective] = oca_weighted_contrast(
        evaluations, bootstrap, oca_main_effect_weights(objective));
    leave_one_out[objective] = oca_weighted_contrast(
        evaluations, bootstrap,
        oca_pair_weights(15U, 15U & ~(1U << objective)));
    standalone[objective] = oca_weighted_contrast(
        evaluations, bootstrap,
        oca_pair_weights(1U << objective, 0U));
    oca_emit_weighted_contrast(
        "oca1.factorial.main_effect." +
            std::string(kAttributionBranchNames[objective]),
        main_effects[objective]);
    oca_emit_weighted_contrast(
        "oca1.factorial.leave_one_out." +
            std::string(kAttributionBranchNames[objective]),
        leave_one_out[objective]);
    oca_emit_weighted_contrast(
        "oca1.factorial.standalone." +
            std::string(kAttributionBranchNames[objective]),
        standalone[objective]);
  }
  for (std::size_t left = 0; left < 4; ++left) {
    for (std::size_t right = left + 1; right < 4; ++right) {
      interactions[left][right] = oca_weighted_contrast(
          evaluations, bootstrap, oca_interaction_weights(left, right));
      oca_emit_weighted_contrast(
          "oca1.factorial.interaction." +
              std::string(kAttributionBranchNames[left]) + "_x_" +
              kAttributionBranchNames[right],
          interactions[left][right]);
    }
  }
  const auto factorial_artifact_hash =
      oca_write_factorial_artifact(evaluations, training, arm_mechanics);
  std::cout << "oca1.factorial.complete_table.path="
            << ".build/tests/oca1/factorial_complete_table.tsv\n";
  std::cout << "oca1.factorial.complete_table.sha256="
            << factorial_artifact_hash << '\n';
  std::cout << "oca1.factorial.mechanics_pass=" << factorial_mechanics << '\n';
  if (!factorial_mechanics) {
    for (const auto *objective : kAttributionBranchNames) {
      std::cout << "oca1.verdict." << objective << "=unresolved\n";
    }
    std::cout << "oca1.anchor_challenge.opened=false\n";
    std::cout << "execution_status=oca1_factorial_mechanics_failed\n";
    return 3;
  }

  constexpr std::array<uint8_t, 5> challenge_masks{4U, 1U, 2U, 8U, 15U};
  std::array<std::array<RmcEvaluation, 3>, 5> challenge_evaluations{};
  std::array<std::array<OcaLegacyTrainingReceipt, 3>, 5> challenge_training{};
  std::array<OcaChallengeSummary, 5> challenge_summaries{};
  std::vector<mtf::MtfJepaMaeVicreg> challenge_models;
  challenge_models.reserve(15);
  std::array<bool, 5> challenge_mechanics{};
  challenge_mechanics.fill(true);
  const std::vector<uint8_t> challenge_mask_vector(challenge_masks.begin(),
                                                   challenge_masks.end());
  std::cout << "oca1.anchor_challenge.opened=true\n" << std::flush;
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    auto interleaved = oca_train_or_resume_seed(
        "anchor_challenge", data.ssl, device, seed, challenge_mask_vector,
        kOcaAnchorChallengeSteps, /*load_certified_anchor=*/true,
        anchor_config_hash);
    for (std::size_t challenge = 0; challenge < challenge_masks.size();
         ++challenge) {
      const uint8_t mask = challenge_masks[challenge];
      const auto arm = oca_arm(mask);
      challenge_evaluations[challenge][seed_index] = rmc_evaluate(
          interleaved.models[challenge], data.probe_train,
          data.probe_validation, data.development,
          data.reversed_train, data.reversed_validation,
          data.reversed_development, targets, device);
      challenge_mechanics[challenge] =
          challenge_mechanics[challenge] && interleaved.metadata_exact &&
          interleaved.schedule_exact && interleaved.pass &&
          interleaved.initialization_exact[challenge] &&
          interleaved.receipts[challenge].pass;
      challenge_training[challenge][seed_index] =
          std::move(interleaved.receipts[challenge]);
      const std::string root = "oca1.anchor_challenge.arm." +
                               std::string(arm.name) + ".seed_" +
                               std::to_string(seed);
      std::cout << root << ".final_aulc="
                << challenge_evaluations[challenge][seed_index].probe.area
                << '\n';
      oca_emit_training(root + ".training",
                        challenge_training[challenge][seed_index],
                        interleaved.schedule_exact);
      std::cout << root << ".complete=true\n" << std::flush;
      challenge_models.push_back(interleaved.models[challenge]);
    }
  }
  for (std::size_t challenge = 0; challenge < challenge_masks.size();
       ++challenge) {
    const uint8_t mask = challenge_masks[challenge];
    const auto arm = oca_arm(mask);
    challenge_summaries[challenge].mask = mask;
    challenge_summaries[challenge].mechanics =
        challenge_mechanics[challenge];
    challenge_summaries[challenge].rmc = oca_pair_summary(
        anchor_reference, challenge_evaluations[challenge], raw,
        data.development.target, targets, bootstrap_rows,
        challenge_mechanics[challenge]);
    challenge_summaries[challenge].qualifies =
        challenge_summaries[challenge].rmc.gate.neutral.pass;
    const std::string root =
        "oca1.anchor_challenge.arm." + std::string(arm.name);
    oca_emit_candidate_summary(root, challenge_summaries[challenge].rmc);
    std::cout << root << ".qualifies="
              << challenge_summaries[challenge].qualifies << '\n';
  }

  int64_t selected = -1;
  double best_gain = -std::numeric_limits<double>::infinity();
  for (std::size_t challenge = 0; challenge < challenge_masks.size();
       ++challenge) {
    if (!challenge_summaries[challenge].qualifies) {
      continue;
    }
    const double gain = challenge_summaries[challenge]
                            .rmc.candidate[0]
                            .gate.trained_minus_initialization.point;
    if (selected < 0 || gain > best_gain + 1.0e-12) {
      selected = static_cast<int64_t>(challenge);
      best_gain = gain;
    }
  }

  bool confirmation_opened = false;
  bool confirmation_pass = false;
  if (selected >= 0) {
    confirmation_opened = true;
    oca_open_confirmation(data);
    const auto confirmation_targets = rmc_make_targets(data, true);
    const auto raw_confirmation = rssm_probe_curve(
        data.raw_train, data.raw_validation, data.raw_confirmation,
        data.probe_train.target, data.probe_validation.target,
        data.confirmation.target, /*dual=*/true);
    std::array<RmcEvaluation, 3> confirmation_anchor{};
    std::array<RmcEvaluation, 3> confirmation_candidate{};
    for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
      const int64_t seed = kAttributionSeeds[seed_index];
      auto anchor = mtf::MtfJepaMaeVicreg(anchor_config);
      const bool metadata = oca_load_archive(
          oca_archive_path(seed), anchor, device, seed, anchor_config_hash);
      if (!metadata) {
        throw std::runtime_error("OCA confirmation anchor metadata failed");
      }
      confirmation_anchor[seed_index] = rmc_evaluate(
          anchor, data.probe_train, data.probe_validation, data.confirmation,
          data.reversed_train, data.reversed_validation,
          data.reversed_confirmation, confirmation_targets, device);
      auto &candidate = challenge_models[
          seed_index * challenge_masks.size() +
          static_cast<std::size_t>(selected)];
      confirmation_candidate[seed_index] = rmc_evaluate(
          candidate, data.probe_train, data.probe_validation,
          data.confirmation, data.reversed_train, data.reversed_validation,
          data.reversed_confirmation, confirmation_targets, device);
    }
    const auto confirmation_summary = oca_pair_summary(
        confirmation_anchor, confirmation_candidate, raw_confirmation,
        data.confirmation.target, confirmation_targets, bootstrap_rows,
        challenge_summaries[static_cast<std::size_t>(selected)].mechanics);
    confirmation_pass = confirmation_summary.gate.neutral.pass;
    oca_emit_candidate_summary("oca1.confirmation.selected",
                               confirmation_summary);
  }
  std::cout << "oca1.anchor_challenge.selected="
            << (selected >= 0
                    ? oca_arm(challenge_masks[static_cast<std::size_t>(selected)])
                          .name
                    : "none")
            << '\n';
  std::cout << "oca1.confirmation.opened=" << confirmation_opened << '\n';
  std::cout << "oca1.confirmation.pass=" << confirmation_pass << '\n';

  for (std::size_t objective = 0; objective < 4; ++objective) {
    const std::array<std::size_t, 4> challenge_for_objective{1, 2, 0, 3};
    const auto challenge = challenge_for_objective[objective];
    const auto &summary = challenge_summaries[challenge].rmc;
    const auto &candidate = summary.candidate[0];
    const auto &gate = summary.gate.neutral;
    const bool selected_objective =
        selected == static_cast<int64_t>(challenge);
    const bool safeguard_failure =
        !gate.numeric_valid || !gate.mechanics_pass || !gate.family_floor_pass ||
        !gate.raw_noninferiority_pass || !gate.order_point_pass ||
        !gate.order_lower_pass || !gate.order_retention_pass ||
        !gate.continuous_shuffle_pass || !gate.order_shuffle_pass ||
        !gate.geometry_pass;
    int64_t safeguard_failure_seed_count = 0;
    for (std::size_t seed = 0; seed < 3; ++seed) {
      safeguard_failure_seed_count +=
          !oca_seed_safeguards_pass(
              challenge_evaluations[challenge][seed],
              anchor_reference[seed], raw.area)
              ? 1
              : 0;
    }
    const bool reproducible_safeguard_failure =
        safeguard_failure_seed_count == 3;
    bool helpful_interaction = leave_one_out[objective].summary.low > 0.0;
    for (std::size_t other = 0; other < 4; ++other) {
      if (other == objective) {
        continue;
      }
      const std::size_t left = std::min(objective, other);
      const std::size_t right = std::max(objective, other);
      helpful_interaction =
          helpful_interaction || interactions[left][right].summary.low > 0.0;
    }
    std::string verdict;
    if (!phase_one_connected[objective]) {
      verdict = "mechanically_disconnected";
    } else if (selected_objective && confirmation_pass) {
      verdict = "beneficial_to_certified_anchor";
    } else if (!gate.mechanics_pass || !gate.numeric_valid) {
      verdict = "unresolved";
    } else if (candidate.gate.trained_minus_initialization.high < 0.0 ||
               reproducible_safeguard_failure) {
      verdict = "harmful_at_certified_boundary";
    } else if (safeguard_failure) {
      // The aggregate gate alone does not prove that a safeguard failure is
      // reproducible across seeds, so OCA stays conservative here.
      verdict = "unresolved";
    } else if (arm_summaries[1U << objective].gate.neutral.pass) {
      verdict = "standalone_capable_only";
    } else if (helpful_interaction) {
      verdict = "conditionally_helpful_legacy_interaction";
    } else if (challenge_summaries[challenge].qualifies ||
               candidate.gate.trained_minus_initialization.point >=
                   rmc_gate::kLearnedGainFloor) {
      verdict = "unresolved";
    } else {
      verdict = "neutral_at_certified_boundary";
    }
    std::cout << "oca1.verdict." << kAttributionBranchNames[objective] << '='
              << verdict << '\n';
  }

  const bool promoted = selected >= 0 && confirmation_pass;
  std::cout << "oca1.canonical_recipe="
            << (promoted ? "fspa4_plus_selected_legacy_objective"
                         : "fspa4_minimal_spectral_repair_v1")
            << '\n';
  std::cout << "oca1.rollback_policy=all_tokens\n";
  std::cout << "oca1.rollback_preserved=true\n";
  std::cout << "oca1.augmentation_attribution_authorized=true\n";
  std::cout << "execution_status=oca1_measurements_complete\n";
  return 0;
}

} // namespace

#ifndef CUWACUNU_OCA_EMBEDDED
int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment ==
        "four-objective-causal-attribution-preflight") {
      return run_oca_preflight(options);
    }
    if (options.experiment ==
        "four-objective-causal-attribution-cache-smoke") {
      return run_oca_cache_smoke(options);
    }
    if (options.experiment == "four-objective-causal-attribution-reference") {
      return run_oca_reference(options);
    }
    if (options.experiment == "four-objective-causal-attribution-factorial") {
      return run_oca_factorial(options);
    }
    throw std::runtime_error(
        "--experiment must be four-objective-causal-attribution-preflight or "
        "four-objective-causal-attribution-cache-smoke or "
        "four-objective-causal-attribution-reference or "
        "four-objective-causal-attribution-factorial");
  } catch (const c10::Error &error) {
    std::cerr << "four_objective_causal_attribution_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "four_objective_causal_attribution_error=" << error.what()
              << '\n';
  }
  return 2;
}
#endif
