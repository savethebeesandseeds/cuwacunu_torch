#include "representation_module_certification_gate.h"
#include "piaabo/digest/sha256.h"

// Reuse the already-audited synthetic data, paired training, probe, bootstrap,
// and augmentation machinery without changing its established experiments.
#pragma push_macro("main")
#undef main
#define main representation_parent_embedded_main
#include "quality_wikimyei_mtf_jepa_mae_vicreg_representation.cpp"
#undef main
#pragma pop_macro("main")

namespace rmc_gate =
    cuwacunu::tests::representation_module_certification_gate;
namespace digest = cuwacunu::piaabo::digest;

namespace {

constexpr std::size_t kRmcArmCount = 2;
constexpr std::array<std::size_t, kRmcArmCount> kRmcOuterArmIndices{
    kOuterNeutralIndex, kOuterQualifiedIndex};
constexpr std::array<const char *, kRmcArmCount> kRmcArmNames{
    "neutral", "qualified"};
constexpr std::string_view kRmcProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/REPRESENTATION_MODULE_CERTIFICATION_PROTOCOL.md";
constexpr std::string_view kRmcProtocolSha256 =
    "d9bae7d732af55b0e649e823e42b360ea83707f0b23df0c82ba87d42ca734ff7";
constexpr uint64_t kRmcDevelopmentShuffleTag = 0x726d635f64657631ULL;
constexpr uint64_t kRmcConfirmationShuffleTag = 0x726d635f636f6e31ULL;
constexpr uint64_t kRmcDevelopmentOrderShuffleTag =
    0x726d635f646f7264ULL;
constexpr uint64_t kRmcConfirmationOrderShuffleTag =
    0x726d635f636f7264ULL;
constexpr int64_t kRmcBootstrapReplicates = 4096;
constexpr uint64_t kRmcBootstrapSeed = 0x726d635f626f6f74ULL;

struct RmcData {
  Normalization normalization{};
  Dataset ssl{};
  Dataset probe_train{};
  Dataset probe_validation{};
  Dataset development{};
  Dataset confirmation{};
  Dataset reversed_train{};
  Dataset reversed_validation{};
  Dataset reversed_development{};
  Dataset reversed_confirmation{};
  torch::Tensor raw_train{};
  torch::Tensor raw_validation{};
  torch::Tensor raw_development{};
  torch::Tensor raw_confirmation{};
};

struct RmcEvalTargets {
  torch::Tensor shuffled_train{};
  torch::Tensor shuffled_validation{};
  torch::Tensor shuffled_evaluation{};
  RssmOrderFitTargets order_fit{};
  RssmOrderFitTargets shuffled_order_fit{};
  torch::Tensor order_validation{};
  torch::Tensor order_evaluation{};
  torch::Tensor shuffled_order_validation{};
  torch::Tensor shuffled_order_evaluation{};
};

struct RmcEvaluation {
  ProbeCurve probe{};
  ProbeCurve shuffled_probe{};
  RssmOrderCurve order{};
  RssmOrderCurve shuffled_order{};
  std::array<Geometry, kChannels> geometry{};
  uint64_t train_hash{0};
  uint64_t validation_hash{0};
  uint64_t evaluation_hash{0};
  uint64_t reversed_train_hash{0};
  uint64_t reversed_validation_hash{0};
  uint64_t reversed_evaluation_hash{0};
};

struct RmcArmRun {
  int64_t seed{0};
  std::size_t arm{0};
  RmcEvaluation initialization{};
  RmcEvaluation trained{};
  AttributionArmResult training_receipt{};
};

struct RmcPairing {
  bool initialization_parameters_exact{true};
  bool initialization_representation_exact{true};
  bool batch_rows_exact{true};
  bool clean_inputs_exact{true};
  bool clean_masks_exact{true};
  bool actual_masks_exact{true};
  bool weak_support_masks_exact{true};
  bool generator_schedule_exact{true};
  bool per_update_mechanics{true};
  bool qualified_support_exact{true};
  bool all_tokens_reference_reproduced{true};
  bool pass{true};
};

struct RmcReadoutSmoke {
  bool same_encoded_object{false};
  bool contract{false};
  bool complete_v1_identity{false};
  bool deterministic{false};
  bool parameters_unchanged{false};
  bool rng_unchanged{false};
  bool pass{false};
};

struct RmcCandidateSummary {
  rmc_gate::CandidateInput gate{};
  RssmPointInterval final{};
  RssmPointInterval final_order{};
};

struct RmcSummary {
  std::array<RmcCandidateSummary, kRmcArmCount> candidate{};
  rmc_gate::Contrast qualified_minus_neutral{};
  rmc_gate::GateResult gate{};
};

[[nodiscard]] std::string rmc_read_file(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) {
    throw std::runtime_error("cannot open RMC protocol: " + path.string());
  }
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

[[nodiscard]] uint64_t rmc_metadata_hash(
    const mtf::mtf_token_metadata_t &metadata) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.start_index));
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.width));
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.scale_id));
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.channel_id));
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.domain_id));
  return hash;
}

[[nodiscard]] uint64_t rmc_embeddings_hash(const Embeddings &value) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  mix_hash_value(hash, hash_tensor_stable_bytes(value.by_channel));
  mix_hash_value(hash, hash_tensor_stable_bytes(value.flat));
  return hash;
}

[[nodiscard]] Embeddings rmc_extract_sparse_embeddings(
    mtf::MtfJepaMaeVicreg &model, const Dataset &dataset,
    const torch::Device &device) {
  const bool was_training = model->is_training();
  model->eval();
  torch::NoGradGuard no_grad;
  std::vector<torch::Tensor> chunks;
  for (int64_t begin = 0; begin < dataset.data.size(0);
       begin += kModelRowBatchSize) {
    const int64_t size =
        std::min<int64_t>(kModelRowBatchSize, dataset.data.size(0) - begin);
    const auto encoded =
        model->encode(dataset.data.narrow(0, begin, size).to(device),
                      dataset.mask.narrow(0, begin, size).to(device));
    const uint64_t embeddings_before =
        hash_tensor_stable_bytes(encoded.embeddings);
    const uint64_t token_mask_before =
        hash_tensor_stable_bytes(encoded.token_mask);
    const uint64_t sample_mask_before =
        hash_tensor_stable_bytes(encoded.sample_valid_mask);
    const uint64_t channel_mask_before =
        hash_tensor_stable_bytes(encoded.channel_valid_mask);
    const uint64_t metadata_before = rmc_metadata_hash(encoded.metadata);
    const auto served = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1,
        model->config());
    const bool input_unchanged =
        embeddings_before == hash_tensor_stable_bytes(encoded.embeddings) &&
        token_mask_before == hash_tensor_stable_bytes(encoded.token_mask) &&
        sample_mask_before ==
            hash_tensor_stable_bytes(encoded.sample_valid_mask) &&
        channel_mask_before ==
            hash_tensor_stable_bytes(encoded.channel_valid_mask) &&
        metadata_before == rmc_metadata_hash(encoded.metadata);
    if (!input_unchanged || !served.valid_mask.all().item<bool>() ||
        served.values.sizes() !=
            torch::IntArrayRef({size, kChannels, kLatentDim}) ||
        served.valid_mask.sizes() !=
            torch::IntArrayRef({size, kChannels}) ||
        served.values.scalar_type() != torch::kFloat32 ||
        served.values.device() != device || !served.values.is_contiguous() ||
        !torch::isfinite(served.values).all().item<bool>()) {
      throw std::runtime_error("RMC sparse readout contract failed");
    }
    chunks.push_back(served.values.detach().to(torch::kCPU, torch::kFloat64));
  }
  model->train(was_training);
  auto by_channel = torch::cat(chunks, 0).contiguous();
  return {.by_channel = by_channel,
          .flat = by_channel.reshape({by_channel.size(0), kServedWidth})
                      .contiguous()};
}

[[nodiscard]] RmcReadoutSmoke
rmc_readout_smoke(mtf::MtfJepaMaeVicreg &model, const Dataset &dataset,
                  const torch::Device &device) {
  const bool was_training = model->is_training();
  model->eval();
  torch::NoGradGuard no_grad;
  const auto data = dataset.data.narrow(0, 0, 8).to(device);
  const auto mask = dataset.mask.narrow(0, 0, 8).to(device);
  const auto encoded = model->encode(data, mask);
  const auto parameters_before = snapshot_parameters(model);
  const auto generator_before = current_generator_state_snapshot(device);
  const uint64_t embeddings_before =
      hash_tensor_stable_bytes(encoded.embeddings);
  const uint64_t token_mask_before = hash_tensor_stable_bytes(encoded.token_mask);
  const uint64_t sample_mask_before =
      hash_tensor_stable_bytes(encoded.sample_valid_mask);
  const uint64_t channel_mask_before =
      hash_tensor_stable_bytes(encoded.channel_valid_mask);
  const uint64_t metadata_before = rmc_metadata_hash(encoded.metadata);
  const auto all_tokens = mtf::select_mtf_serving_pool(
      encoded, mtf::mtf_serving_pool_policy_t::all_tokens, model->config());
  const auto sparse = mtf::select_mtf_serving_pool(
      encoded, mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1,
      model->config());
  const auto sparse_repeat = mtf::select_mtf_serving_pool(
      encoded, mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1,
      model->config());
  const auto complete_v1 = mtf::select_mtf_serving_pool(
      encoded, mtf::mtf_serving_pool_policy_t::structured_cdsb_v1,
      model->config());
  RmcReadoutSmoke result{};
  result.same_encoded_object =
      embeddings_before == hash_tensor_stable_bytes(encoded.embeddings) &&
      token_mask_before == hash_tensor_stable_bytes(encoded.token_mask) &&
      sample_mask_before ==
          hash_tensor_stable_bytes(encoded.sample_valid_mask) &&
      channel_mask_before ==
          hash_tensor_stable_bytes(encoded.channel_valid_mask) &&
      metadata_before == rmc_metadata_hash(encoded.metadata);
  result.contract =
      all_tokens.values.sizes() == torch::IntArrayRef({8, 3, 32}) &&
      sparse.values.sizes() == torch::IntArrayRef({8, 3, 32}) &&
      all_tokens.valid_mask.sizes() == torch::IntArrayRef({8, 3}) &&
      sparse.valid_mask.sizes() == torch::IntArrayRef({8, 3}) &&
      torch::equal(all_tokens.valid_mask, sparse.valid_mask) &&
      sparse.valid_mask.all().item<bool>() &&
      torch::isfinite(sparse.values).all().item<bool>();
  result.complete_v1_identity =
      rssm_tensor_bytes_equal(sparse.values, complete_v1.values) &&
      rssm_tensor_bytes_equal(sparse.valid_mask, complete_v1.valid_mask);
  result.deterministic =
      rssm_tensor_bytes_equal(sparse.values, sparse_repeat.values) &&
      rssm_tensor_bytes_equal(sparse.valid_mask, sparse_repeat.valid_mask);
  result.parameters_unchanged =
      parameter_max_abs_diff(model, parameters_before) == 0.0;
  result.rng_unchanged = generator_state_snapshot_equal(
      generator_before, current_generator_state_snapshot(device));
  result.pass = result.same_encoded_object && result.contract &&
                result.complete_v1_identity && result.deterministic &&
                result.parameters_unchanged && result.rng_unchanged;
  model->train(was_training);
  return result;
}

[[nodiscard]] RmcData rmc_make_data() {
  RmcData data{};
  data.ssl = generate_dataset(0, 256);
  data.probe_train = generate_dataset(1000000, 256);
  data.probe_validation = generate_dataset(2000000, 128);
  data.development = generate_dataset(3000000, 256);
  const auto raw_projection = make_raw_equal_width_projection();
  data.raw_train = raw_equal_width_features(data.probe_train, raw_projection);
  data.raw_validation =
      raw_equal_width_features(data.probe_validation, raw_projection);
  data.raw_development =
      raw_equal_width_features(data.development, raw_projection);
  data.normalization = fit_normalization(data.ssl);
  for (Dataset *dataset : {&data.ssl, &data.probe_train,
                           &data.probe_validation, &data.development}) {
    normalize(*dataset, data.normalization);
    validate_dataset(*dataset);
  }
  data.reversed_train = rssm_reversed_dataset(data.probe_train);
  data.reversed_validation = rssm_reversed_dataset(data.probe_validation);
  data.reversed_development = rssm_reversed_dataset(data.development);
  return data;
}

void rmc_open_confirmation(RmcData &data) {
  if (data.confirmation.data.defined()) {
    throw std::runtime_error("RMC confirmation was opened more than once");
  }
  data.confirmation = generate_dataset(4000000, 256);
  const auto raw_projection = make_raw_equal_width_projection();
  data.raw_confirmation =
      raw_equal_width_features(data.confirmation, raw_projection);
  normalize(data.confirmation, data.normalization);
  validate_dataset(data.confirmation);
  data.reversed_confirmation = rssm_reversed_dataset(data.confirmation);
}

[[nodiscard]] std::vector<torch::Tensor> rmc_bootstrap_rows(int64_t groups) {
  std::vector<torch::Tensor> result;
  result.reserve(kRmcBootstrapReplicates);
  for (int64_t replicate = 0; replicate < kRmcBootstrapReplicates;
       ++replicate) {
    uint64_t state = splitmix64(
        kRmcBootstrapSeed ^ splitmix64(static_cast<uint64_t>(replicate)));
    std::vector<int64_t> rows;
    rows.reserve(static_cast<std::size_t>(groups));
    for (int64_t draw = 0; draw < groups; ++draw) {
      state = splitmix64(state);
      rows.push_back(
          static_cast<int64_t>(state % static_cast<uint64_t>(groups)));
    }
    result.push_back(torch::tensor(rows, torch::kInt64));
  }
  return result;
}

[[nodiscard]] bool
rmc_bootstrap_rows_valid(const std::vector<torch::Tensor> &rows,
                         int64_t groups) {
  if (rows.size() != static_cast<std::size_t>(kRmcBootstrapReplicates)) {
    return false;
  }
  for (const auto &value : rows) {
    if (value.scalar_type() != torch::kInt64 || value.dim() != 1 ||
        value.numel() != groups || value.min().item<int64_t>() < 0 ||
        value.max().item<int64_t>() >= groups) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] RmcEvalTargets
rmc_make_targets(const RmcData &data, bool confirmation) {
  const auto train_permutation =
      rssm_sattolo_permutation(256, kRssmShuffleTrainTag);
  const auto validation_permutation =
      rssm_sattolo_permutation(128, kRssmShuffleValidationTag);
  const auto evaluation_permutation = rssm_sattolo_permutation(
      256, confirmation ? kRmcConfirmationShuffleTag
                        : kRmcDevelopmentShuffleTag);
  const auto order_fit_permutations = rssm_order_fit_permutations();
  const auto order_validation_permutation =
      rssm_sattolo_permutation(256, kRssmOrderShuffleValidationTag);
  const auto order_evaluation_permutation = rssm_sattolo_permutation(
      512, confirmation ? kRmcConfirmationOrderShuffleTag
                        : kRmcDevelopmentOrderShuffleTag);
  const auto &evaluation =
      confirmation ? data.confirmation : data.development;
  RmcEvalTargets result{};
  result.shuffled_train =
      data.probe_train.target.index_select(0, train_permutation).contiguous();
  result.shuffled_validation = data.probe_validation.target
                                   .index_select(0, validation_permutation)
                                   .contiguous();
  result.shuffled_evaluation =
      evaluation.target.index_select(0, evaluation_permutation).contiguous();
  result.order_fit = rssm_order_fit_targets(nullptr);
  result.shuffled_order_fit =
      rssm_order_fit_targets(&order_fit_permutations);
  result.order_validation = rssm_order_labels(128);
  result.order_evaluation = rssm_order_labels(256);
  result.shuffled_order_validation =
      result.order_validation.index_select(0, order_validation_permutation)
          .contiguous();
  result.shuffled_order_evaluation =
      result.order_evaluation.index_select(0, order_evaluation_permutation)
          .contiguous();
  return result;
}

[[nodiscard]] RmcEvaluation rmc_evaluate(
    mtf::MtfJepaMaeVicreg &model, const Dataset &probe_train,
    const Dataset &probe_validation, const Dataset &evaluation,
    const Dataset &reversed_train, const Dataset &reversed_validation,
    const Dataset &reversed_evaluation, const RmcEvalTargets &targets,
    const torch::Device &device) {
  const auto train =
      rmc_extract_sparse_embeddings(model, probe_train, device);
  const auto validation =
      rmc_extract_sparse_embeddings(model, probe_validation, device);
  const auto evaluated =
      rmc_extract_sparse_embeddings(model, evaluation, device);
  const auto reverse_train =
      rmc_extract_sparse_embeddings(model, reversed_train, device);
  const auto reverse_validation =
      rmc_extract_sparse_embeddings(model, reversed_validation, device);
  const auto reverse_evaluated =
      rmc_extract_sparse_embeddings(model, reversed_evaluation, device);
  const auto order_train =
      rssm_interleave_pairs(train.flat, reverse_train.flat);
  const auto order_validation =
      rssm_interleave_pairs(validation.flat, reverse_validation.flat);
  const auto order_evaluation =
      rssm_interleave_pairs(evaluated.flat, reverse_evaluated.flat);

  RmcEvaluation result{};
  result.probe = rssm_probe_curve(
      train.flat, validation.flat, evaluated.flat, probe_train.target,
      probe_validation.target, evaluation.target, /*dual=*/true);
  result.shuffled_probe = rssm_probe_curve(
      train.flat, validation.flat, evaluated.flat, targets.shuffled_train,
      targets.shuffled_validation, targets.shuffled_evaluation,
      /*dual=*/true);
  result.order = rssm_order_curve(
      order_train, order_validation, order_evaluation, targets.order_fit,
      targets.order_validation, targets.order_evaluation, /*dual=*/true);
  result.shuffled_order = rssm_order_curve(
      order_train, order_validation, order_evaluation,
      targets.shuffled_order_fit, targets.shuffled_order_validation,
      targets.shuffled_order_evaluation, /*dual=*/true);
  result.geometry = geometry(evaluated);
  result.train_hash = rmc_embeddings_hash(train);
  result.validation_hash = rmc_embeddings_hash(validation);
  result.evaluation_hash = rmc_embeddings_hash(evaluated);
  result.reversed_train_hash = rmc_embeddings_hash(reverse_train);
  result.reversed_validation_hash = rmc_embeddings_hash(reverse_validation);
  result.reversed_evaluation_hash = rmc_embeddings_hash(reverse_evaluated);
  validate_probe_curve_finite(result.probe, "RMC predictive probe");
  validate_probe_curve_finite(result.shuffled_probe, "RMC shuffled probe");
  rssm_validate_order_curve_finite(result.order, "RMC order probe");
  rssm_validate_order_curve_finite(result.shuffled_order,
                                   "RMC shuffled order probe");
  for (std::size_t channel = 0; channel < result.geometry.size(); ++channel) {
    validate_geometry_finite(result.geometry[channel],
                             "RMC structured geometry");
  }
  return result;
}

[[nodiscard]] double
rmc_order_prediction_max_abs_diff(const RssmOrderCurve &left,
                                  const RssmOrderCurve &right) {
  if (left.points.size() != right.points.size()) {
    return std::numeric_limits<double>::infinity();
  }
  double maximum = 0.0;
  for (std::size_t point = 0; point < left.points.size(); ++point) {
    maximum = std::max(
        maximum,
        (left.points[point].prediction - right.points[point].prediction)
            .abs()
            .max()
            .item<double>());
  }
  return maximum;
}

[[nodiscard]] bool rmc_evaluations_exact(const RmcEvaluation &left,
                                         const RmcEvaluation &right) {
  bool geometry_identical = true;
  for (std::size_t channel = 0; channel < kChannels; ++channel) {
    geometry_identical = geometry_identical &&
                         geometry_exact(left.geometry[channel],
                                        right.geometry[channel]);
  }
  return left.train_hash == right.train_hash &&
         left.validation_hash == right.validation_hash &&
         left.evaluation_hash == right.evaluation_hash &&
         left.reversed_train_hash == right.reversed_train_hash &&
         left.reversed_validation_hash == right.reversed_validation_hash &&
         left.reversed_evaluation_hash == right.reversed_evaluation_hash &&
         probe_curve_prediction_max_abs_diff(left.probe, right.probe) == 0.0 &&
         probe_curve_prediction_max_abs_diff(left.shuffled_probe,
                                             right.shuffled_probe) == 0.0 &&
         rmc_order_prediction_max_abs_diff(left.order, right.order) == 0.0 &&
         rmc_order_prediction_max_abs_diff(left.shuffled_order,
                                           right.shuffled_order) == 0.0 &&
         probe_curve_selected_alphas_equal(left.probe, right.probe) &&
         probe_curve_selected_alphas_equal(left.shuffled_probe,
                                           right.shuffled_probe) &&
         geometry_identical;
}

[[nodiscard]] rmc_gate::Contrast rmc_predictive_contrast(
    const RssmCurveBySeed &candidate, const RssmCurveBySeed &reference,
    const torch::Tensor &target,
    const std::vector<torch::Tensor> &bootstrap_rows,
    std::array<double, kFamilies> *family_deltas = nullptr) {
  const auto value =
      rssm_contrast(candidate, target, reference, target, bootstrap_rows);
  rmc_gate::Contrast result{.point = value.point,
                            .low = value.low,
                            .high = value.high,
                            .positive_seed_count = 0};
  for (const double delta : value.seed_deltas) {
    result.positive_seed_count += delta > 0.0 ? 1 : 0;
  }
  if (family_deltas != nullptr) {
    *family_deltas = value.family_deltas;
  }
  return result;
}

[[nodiscard]] rmc_gate::Contrast rmc_order_contrast(
    const RssmOrderCurveBySeed &candidate,
    const RssmOrderCurveBySeed &reference, const torch::Tensor &target,
    const std::vector<torch::Tensor> &bootstrap_rows) {
  rmc_gate::Contrast result{};
  for (std::size_t seed = 0; seed < candidate.size(); ++seed) {
    const double delta = candidate[seed].area - reference[seed].area;
    result.point += delta;
    result.positive_seed_count += delta > 0.0 ? 1 : 0;
  }
  result.point /= static_cast<double>(candidate.size());
  std::vector<double> replicates;
  replicates.reserve(bootstrap_rows.size());
  for (const auto &rows : bootstrap_rows) {
    double delta = 0.0;
    for (std::size_t seed = 0; seed < candidate.size(); ++seed) {
      delta += rssm_resampled_order_area(candidate[seed], target, rows) -
               rssm_resampled_order_area(reference[seed], target, rows);
    }
    replicates.push_back(delta / static_cast<double>(candidate.size()));
  }
  const auto interval = percentile_interval(std::move(replicates));
  result.low = interval.low;
  result.high = interval.high;
  return result;
}

[[nodiscard]] RmcPairing
rmc_validate_pairing(const RmcArmRun &neutral, const RmcArmRun &qualified) {
  RmcPairing result{};
  result.initialization_parameters_exact =
      neutral.training_receipt.initial_parameter_max_abs_diff == 0.0 &&
      qualified.training_receipt.initial_parameter_max_abs_diff == 0.0;
  result.initialization_representation_exact =
      rmc_evaluations_exact(neutral.initialization,
                            qualified.initialization);
  const auto &left = neutral.training_receipt.training;
  const auto &right = qualified.training_receipt.training;
  const bool complete =
      left.outer_augmentation_updates.size() == kAttributionSteps &&
      right.outer_augmentation_updates.size() == kAttributionSteps &&
      left.batch_rows.size() == kAttributionSteps &&
      right.batch_rows.size() == kAttributionSteps;
  if (!complete) {
    result.pass = false;
    return result;
  }
  result.all_tokens_reference_reproduced = true;
  constexpr std::array<std::array<double, kRmcArmCount>, 3> kExpectedFinal{{
      {{0.51570330542266796, 0.51569256572543787}},
      {{0.49409955802796446, 0.49407218363349681}},
      {{0.53682698499898107, 0.53682382081320223}},
  }};
  const auto seed_position = static_cast<std::size_t>(std::distance(
      kAttributionSeeds.begin(),
      std::find(kAttributionSeeds.begin(), kAttributionSeeds.end(),
                neutral.seed)));
  if (seed_position >= kAttributionSeeds.size()) {
    result.all_tokens_reference_reproduced = false;
  } else {
    result.all_tokens_reference_reproduced =
        std::abs(neutral.training_receipt.checkpoints.back().probe.area -
                 kExpectedFinal[seed_position][0]) <= 1.0e-9 &&
        std::abs(qualified.training_receipt.checkpoints.back().probe.area -
                 kExpectedFinal[seed_position][1]) <= 1.0e-9;
  }
  result.batch_rows_exact = left.batch_rows == right.batch_rows &&
                            left.batch_row_hashes == right.batch_row_hashes;
  for (int64_t step = 0; step < kAttributionSteps; ++step) {
    const auto index = static_cast<std::size_t>(step);
    const auto &neutral_update = left.outer_augmentation_updates[index];
    const auto &qualified_update = right.outer_augmentation_updates[index];
    result.clean_inputs_exact =
        result.clean_inputs_exact &&
        torch::equal(neutral_update.clean_data_cpu,
                     qualified_update.clean_data_cpu) &&
        neutral_update.clean_data_hash == qualified_update.clean_data_hash;
    result.clean_masks_exact =
        result.clean_masks_exact &&
        torch::equal(neutral_update.clean_mask_cpu,
                     qualified_update.clean_mask_cpu) &&
        neutral_update.clean_mask_hash == qualified_update.clean_mask_hash &&
        jepa_masks_exact(neutral_update.clean_mask_plan,
                         qualified_update.clean_mask_plan);
    result.actual_masks_exact =
        result.actual_masks_exact &&
        torch::equal(left.target_masks[index], right.target_masks[index]) &&
        torch::equal(left.context_masks[index], right.context_masks[index]);
    result.weak_support_masks_exact =
        result.weak_support_masks_exact &&
        torch::equal(left.outer_view_a_feature_masks[index],
                     right.outer_view_a_feature_masks[index]) &&
        torch::equal(left.outer_view_b_feature_masks[index],
                     right.outer_view_b_feature_masks[index]);
    result.generator_schedule_exact =
        result.generator_schedule_exact &&
        generator_state_snapshot_equal(
            neutral_update.module_forward_pre_snapshot,
            qualified_update.module_forward_pre_snapshot) &&
        generator_state_snapshot_equal(
            neutral_update.module_forward_post_snapshot,
            qualified_update.module_forward_post_snapshot);
    const auto common = [](const OuterAugmentationUpdate &update) {
      return update.augmentation_replay_exact &&
             update.augmentation_consumed_state_exact &&
             update.augmentation_cuda_unchanged &&
             update.augmentation_state_restored && update.masked_values_zero &&
             update.preview_replay_exact &&
             update.support_counterfactual_exact &&
             update.actual_masks_match_preview &&
             update.actual_input_matches_served &&
             update.retention.every_sample_channel_nonempty;
    };
    result.per_update_mechanics =
        result.per_update_mechanics && common(neutral_update) &&
        neutral_update.neutral_identity_exact && common(qualified_update) &&
        qualified_update.qualified_data_changed &&
        qualified_update.qualified_mask_exact;
    result.qualified_support_exact =
        result.qualified_support_exact &&
        qualified_update.retention.removed == 0 &&
        qualified_update.retention.added == 0 &&
        qualified_update.retention.terminal == 1.0;
    for (const double terminal :
         qualified_update.retention.terminal_channel) {
      result.qualified_support_exact =
          result.qualified_support_exact && terminal == 1.0;
    }
  }
  result.pass =
      result.initialization_parameters_exact &&
      result.initialization_representation_exact && result.batch_rows_exact &&
      result.clean_inputs_exact && result.clean_masks_exact &&
      result.actual_masks_exact && result.weak_support_masks_exact &&
      result.generator_schedule_exact && result.per_update_mechanics &&
      result.qualified_support_exact &&
      result.all_tokens_reference_reproduced;
  return result;
}

[[nodiscard]] RmcSummary rmc_summarize(
    const std::array<std::array<RmcEvaluation, kRmcArmCount>, 3> &initial,
    const std::array<std::array<RmcEvaluation, kRmcArmCount>, 3> &trained,
    const ProbeCurve &raw, const torch::Tensor &target,
    const torch::Tensor &shuffled_target, const torch::Tensor &order_target,
    const torch::Tensor &shuffled_order_target,
    const std::vector<torch::Tensor> &bootstrap_rows, bool mechanics) {
  RmcSummary summary{};
  std::array<RssmCurveBySeed, kRmcArmCount> initial_probe{};
  std::array<RssmCurveBySeed, kRmcArmCount> trained_probe{};
  std::array<RssmCurveBySeed, kRmcArmCount> shuffled_probe{};
  std::array<RssmOrderCurveBySeed, kRmcArmCount> initial_order{};
  std::array<RssmOrderCurveBySeed, kRmcArmCount> trained_order{};
  std::array<RssmOrderCurveBySeed, kRmcArmCount> shuffled_order{};
  RssmCurveBySeed raw_by_seed{};
  for (std::size_t seed = 0; seed < 3; ++seed) {
    raw_by_seed[seed] = raw;
    for (std::size_t arm = 0; arm < kRmcArmCount; ++arm) {
      initial_probe[arm][seed] = initial[seed][arm].probe;
      trained_probe[arm][seed] = trained[seed][arm].probe;
      shuffled_probe[arm][seed] = trained[seed][arm].shuffled_probe;
      initial_order[arm][seed] = initial[seed][arm].order;
      trained_order[arm][seed] = trained[seed][arm].order;
      shuffled_order[arm][seed] = trained[seed][arm].shuffled_order;
    }
  }
  for (std::size_t arm = 0; arm < kRmcArmCount; ++arm) {
    auto &candidate = summary.candidate[arm];
    candidate.gate.trained_minus_initialization = rmc_predictive_contrast(
        trained_probe[arm], initial_probe[arm], target, bootstrap_rows,
        &candidate.gate.learned_family_deltas);
    candidate.gate.final_minus_raw = rmc_predictive_contrast(
        trained_probe[arm], raw_by_seed, target, bootstrap_rows);
    candidate.gate.order_trained_minus_initialization = rmc_order_contrast(
        trained_order[arm], initial_order[arm], order_target, bootstrap_rows);
    candidate.final =
        rssm_curve_interval(trained_probe[arm], target, bootstrap_rows);
    candidate.final_order = rssm_order_curve_interval(
        trained_order[arm], order_target, bootstrap_rows);
    const auto shuffled = rssm_curve_interval(
        shuffled_probe[arm], shuffled_target, bootstrap_rows);
    const auto shuffled_order_summary = rssm_order_curve_interval(
        shuffled_order[arm], shuffled_order_target, bootstrap_rows);
    candidate.gate.final_order_point = candidate.final_order.point;
    candidate.gate.final_order_low = candidate.final_order.interval.low;
    candidate.gate.continuous_shuffle_high = shuffled.interval.high;
    candidate.gate.order_shuffle_low =
        shuffled_order_summary.interval.low;
    candidate.gate.order_shuffle_high =
        shuffled_order_summary.interval.high;
    candidate.gate.mechanics = mechanics;
    for (std::size_t seed = 0; seed < 3; ++seed) {
      for (std::size_t channel = 0; channel < kChannels; ++channel) {
        const auto &source = trained[seed][arm].geometry[channel];
        candidate.gate.geometry[seed][channel] = {
            .effective = source.effective_rank_ratio,
            .participation = source.participation_rank_ratio,
            .top = source.top_eigenvalue_share,
            .active = source.active_dimension_fraction};
      }
    }
  }
  summary.qualified_minus_neutral = rmc_predictive_contrast(
      trained_probe[1], trained_probe[0], target, bootstrap_rows);
  summary.gate = rmc_gate::evaluate({
      .neutral = summary.candidate[0].gate,
      .qualified = summary.candidate[1].gate,
      .qualified_minus_neutral = summary.qualified_minus_neutral,
      .global_mechanics = mechanics,
  });
  return summary;
}

void rmc_emit_contrast(const std::string &prefix,
                       const rmc_gate::Contrast &value) {
  std::cout << prefix << ".point=" << value.point << '\n';
  std::cout << prefix << ".bootstrap_95_low=" << value.low << '\n';
  std::cout << prefix << ".bootstrap_95_high=" << value.high << '\n';
  std::cout << prefix << ".positive_seed_count="
            << value.positive_seed_count << '\n';
}

void rmc_emit_pairing(int64_t seed, const RmcPairing &value) {
  const std::string prefix = "rmc.seed_" + std::to_string(seed) + ".pairing";
  std::cout << prefix << ".initialization_parameters_exact="
            << value.initialization_parameters_exact << '\n';
  std::cout << prefix << ".initialization_representation_exact="
            << value.initialization_representation_exact << '\n';
  std::cout << prefix << ".batch_rows_exact=" << value.batch_rows_exact
            << '\n';
  std::cout << prefix << ".clean_inputs_exact=" << value.clean_inputs_exact
            << '\n';
  std::cout << prefix << ".clean_masks_exact=" << value.clean_masks_exact
            << '\n';
  std::cout << prefix << ".actual_masks_exact=" << value.actual_masks_exact
            << '\n';
  std::cout << prefix << ".weak_support_masks_exact="
            << value.weak_support_masks_exact << '\n';
  std::cout << prefix << ".generator_schedule_exact="
            << value.generator_schedule_exact << '\n';
  std::cout << prefix << ".per_update_mechanics="
            << value.per_update_mechanics << '\n';
  std::cout << prefix << ".qualified_support_exact="
            << value.qualified_support_exact << '\n';
  std::cout << prefix << ".all_tokens_reference_reproduced="
            << value.all_tokens_reference_reproduced << '\n';
  std::cout << prefix << ".pass=" << value.pass << '\n';
}

void rmc_emit_summary(const std::string &scope, const RmcSummary &summary,
                      const std::array<std::array<RmcEvaluation, 2>, 3> &initial,
                      const std::array<std::array<RmcEvaluation, 2>, 3> &trained,
                      double raw_area) {
  const std::string root = "rmc." + scope;
  std::cout << root << ".raw_control.aulc=" << raw_area << '\n';
  for (std::size_t arm = 0; arm < kRmcArmCount; ++arm) {
    const std::string prefix =
        root + ".arm." + std::string(kRmcArmNames[arm]);
    for (std::size_t seed = 0; seed < 3; ++seed) {
      const std::string item =
          prefix + ".seed_" + std::to_string(kAttributionSeeds[seed]);
      std::cout << item << ".initial.aulc="
                << initial[seed][arm].probe.area << '\n';
      std::cout << item << ".trained.aulc=" << trained[seed][arm].probe.area
                << '\n';
      std::cout << item << ".trained.order_aulc="
                << trained[seed][arm].order.area << '\n';
      for (std::size_t channel = 0; channel < kChannels; ++channel) {
        const auto &g = trained[seed][arm].geometry[channel];
        const std::string geometry_prefix =
            item + ".geometry.channel_" + std::to_string(channel);
        std::cout << geometry_prefix << ".effective="
                  << g.effective_rank_ratio << '\n';
        std::cout << geometry_prefix << ".participation="
                  << g.participation_rank_ratio << '\n';
        std::cout << geometry_prefix << ".top=" << g.top_eigenvalue_share
                  << '\n';
        std::cout << geometry_prefix << ".active="
                  << g.active_dimension_fraction << '\n';
      }
    }
    const auto &candidate = summary.candidate[arm];
    rmc_emit_contrast(prefix + ".trained_minus_initial",
                      candidate.gate.trained_minus_initialization);
    rmc_emit_contrast(prefix + ".final_minus_raw",
                      candidate.gate.final_minus_raw);
    rmc_emit_contrast(prefix + ".order_trained_minus_initial",
                      candidate.gate.order_trained_minus_initialization);
    std::cout << prefix << ".final.aulc=" << candidate.final.point << '\n';
    std::cout << prefix << ".final.bootstrap_95_low="
              << candidate.final.interval.low << '\n';
    std::cout << prefix << ".final.bootstrap_95_high="
              << candidate.final.interval.high << '\n';
    std::cout << prefix << ".final.order_aulc="
              << candidate.final_order.point << '\n';
    std::cout << prefix << ".final.order_bootstrap_95_low="
              << candidate.final_order.interval.low << '\n';
    std::cout << prefix << ".final.order_bootstrap_95_high="
              << candidate.final_order.interval.high << '\n';
    for (std::size_t family = 0; family < kFamilies; ++family) {
      std::cout << prefix << ".learned_family_" << kFamilyNames[family]
                << "_delta=" << candidate.gate.learned_family_deltas[family]
                << '\n';
    }
    const auto &gate = arm == 0 ? summary.gate.neutral : summary.gate.qualified;
    std::cout << prefix << ".gate.learned_point_pass="
              << gate.learned_point_pass << '\n';
    std::cout << prefix << ".gate.learned_lower_pass="
              << gate.learned_lower_pass << '\n';
    std::cout << prefix << ".gate.learned_seed_pass="
              << gate.learned_seed_pass << '\n';
    std::cout << prefix << ".gate.family_positive_count_pass="
              << gate.family_positive_count_pass << '\n';
    std::cout << prefix << ".gate.family_floor_pass="
              << gate.family_floor_pass << '\n';
    std::cout << prefix << ".gate.raw_noninferiority_pass="
              << gate.raw_noninferiority_pass << '\n';
    std::cout << prefix << ".gate.order_pass="
              << (gate.order_point_pass && gate.order_lower_pass &&
                  gate.order_retention_pass)
              << '\n';
    std::cout << prefix << ".gate.shuffle_pass="
              << (gate.continuous_shuffle_pass && gate.order_shuffle_pass)
              << '\n';
    std::cout << prefix << ".gate.geometry_pass=" << gate.geometry_pass
              << '\n';
    std::cout << prefix << ".gate.pass=" << gate.pass << '\n';
  }
  rmc_emit_contrast(root + ".qualified_minus_neutral",
                    summary.qualified_minus_neutral);
  std::cout << root << ".augmentation_advantage_pass="
            << summary.gate.augmentation_advantage_pass << '\n';
  std::cout << root << ".classification="
            << rmc_gate::classification_name(summary.gate.classification)
            << '\n';
}

[[nodiscard]] bool rmc_options_valid(const Options &options) {
  return options.device == "cuda" &&
         (options.steps < 0 || options.steps == kAttributionSteps) &&
         (options.seeds < 0 ||
          options.seeds == static_cast<int64_t>(kAttributionSeeds.size())) &&
         options.weak_views;
}

void rmc_configure_cuda() {
  if (!torch::cuda::is_available()) {
    throw std::runtime_error("RMC-1 requires CUDA:0");
  }
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);
}

int run_rmc_preflight(const Options &options) {
  if (!rmc_options_valid(options)) {
    throw std::runtime_error(
        "RMC preflight is frozen for CUDA, 32 updates, 3 seeds, weak views on");
  }
  rmc_configure_cuda();
  const torch::Device device(torch::kCUDA, 0);
  const auto protocol = rmc_read_file(std::filesystem::path(kRmcProtocolPath));
  const auto protocol_hash = digest::sha256_hex(protocol);
  const bool protocol_exact = protocol_hash == kRmcProtocolSha256;
  validate_outer_augmentation_configs(device);
  validate_outer_augmentation_seed_domain();
  auto data = rmc_make_data();
  set_paired_rng(kAttributionSeeds.front(), device);
  auto model = mtf::MtfJepaMaeVicreg(
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]));
  const auto smoke = rmc_readout_smoke(model, data.development, device);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.rmc.preflight.v1\n";
  std::cout << "rmc.protocol.sha256=" << protocol_hash << '\n';
  std::cout << "rmc.protocol.exact=" << protocol_exact << '\n';
  std::cout << "rmc.readout.same_encoded_object=" << smoke.same_encoded_object
            << '\n';
  std::cout << "rmc.readout.contract=" << smoke.contract << '\n';
  std::cout << "rmc.readout.complete_v1_identity="
            << smoke.complete_v1_identity << '\n';
  std::cout << "rmc.readout.deterministic=" << smoke.deterministic << '\n';
  std::cout << "rmc.readout.parameters_unchanged="
            << smoke.parameters_unchanged << '\n';
  std::cout << "rmc.readout.rng_unchanged=" << smoke.rng_unchanged << '\n';
  std::cout << "rmc.readout.pass=" << smoke.pass << '\n';
  std::cout << "optimizer_steps=0\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "rmc.preflight.pass=" << (protocol_exact && smoke.pass) << '\n';
  return protocol_exact && smoke.pass ? 0 : 3;
}

int run_rmc(const Options &options) {
  if (!rmc_options_valid(options)) {
    throw std::runtime_error(
        "RMC-1 is frozen for CUDA, 32 updates, 3 seeds, weak views on");
  }
  rmc_configure_cuda();
  const torch::Device device(torch::kCUDA, 0);
  const auto protocol = rmc_read_file(std::filesystem::path(kRmcProtocolPath));
  const auto protocol_hash = digest::sha256_hex(protocol);
  if (protocol_hash != kRmcProtocolSha256) {
    throw std::runtime_error("RMC protocol hash mismatch");
  }
  validate_outer_augmentation_configs(device);
  validate_outer_augmentation_seed_domain();
  auto data = rmc_make_data();
  const auto development_targets = rmc_make_targets(data, false);
  const auto bootstrap_rows = rmc_bootstrap_rows(256);
  if (!rmc_bootstrap_rows_valid(bootstrap_rows, 256)) {
    throw std::runtime_error("RMC bootstrap table failed");
  }
  const auto raw_development = rssm_probe_curve(
      data.raw_train, data.raw_validation, data.raw_development,
      data.probe_train.target, data.probe_validation.target,
      data.development.target, /*dual=*/true);
  validate_probe_curve_finite(raw_development, "RMC raw development");

  set_paired_rng(kAttributionSeeds.front(), device);
  auto smoke_model = mtf::MtfJepaMaeVicreg(
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]));
  const auto readout_smoke =
      rmc_readout_smoke(smoke_model, data.development, device);
  if (!readout_smoke.pass) {
    throw std::runtime_error("RMC sparse readout smoke failed");
  }

  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.rmc.v1\n";
  std::cout << "experiment=representation-module-certification\n";
  std::cout << "module_only=true\n";
  std::cout << "device=cuda:0\n";
  std::cout << "readout_policy=structured_cdsb_sparse_v1\n";
  std::cout << "rollback_policy=all_tokens\n";
  std::cout << "objective=jepa_1_mae_0.25_tf_0_vicreg_0\n";
  std::cout << "arms=neutral,qualified\n";
  std::cout << "known_unsafe_full_augmentation_retrained=false\n";
  std::cout << "model_seeds=17,31,47\n";
  std::cout << "training_steps=32\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "rmc.protocol.sha256=" << protocol_hash << '\n';
  std::cout << "rmc.readout_smoke.pass=" << readout_smoke.pass << '\n';
  rssm_emit_dataset_identity("rmc.data.ssl", data.ssl, "normalized");
  rssm_emit_dataset_identity("rmc.data.probe_train", data.probe_train,
                             "normalized");
  rssm_emit_dataset_identity("rmc.data.probe_validation",
                             data.probe_validation, "normalized");
  rssm_emit_dataset_identity("rmc.data.development", data.development,
                             "normalized");
  std::cout << "rmc.data.confirmation.group_begin=4000000\n";
  std::cout << "rmc.data.confirmation.groups=256\n";
  std::cout << "rmc.data.confirmation.opened=false\n";

  std::array<std::array<RmcEvaluation, kRmcArmCount>, 3> initial{};
  std::array<std::array<RmcEvaluation, kRmcArmCount>, 3> trained{};
  std::array<std::array<RmcArmRun, kRmcArmCount>, 3> runs{};
  std::vector<mtf::MtfJepaMaeVicreg> retained_models;
  retained_models.reserve(kAttributionSeeds.size() * kRmcArmCount);
  bool mechanics = true;
  for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
       ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    ParameterSnapshot initial_parameters{};
    for (std::size_t arm = 0; arm < kRmcArmCount; ++arm) {
      const std::size_t outer_index = kRmcOuterArmIndices[arm];
      const auto &arm_config = kOuterAugmentationArms[outer_index];
      set_paired_rng(seed, device);
      auto model = mtf::MtfJepaMaeVicreg(attribution_config(device, arm_config));
      if (arm == 0) {
        initial_parameters = snapshot_parameters(model);
      }
      RmcArmRun run{};
      run.seed = seed;
      run.arm = arm;
      run.training_receipt.seed = seed;
      run.training_receipt.arm = arm_config;
      run.training_receipt.initial_parameter_max_abs_diff =
          parameter_max_abs_diff(model, initial_parameters);
      run.initialization = rmc_evaluate(
          model, data.probe_train, data.probe_validation, data.development,
          data.reversed_train, data.reversed_validation,
          data.reversed_development, development_targets, device);
      auto checkpoint_zero = evaluate_attribution_checkpoint(
          model, data.probe_train, data.probe_validation, data.development,
          device, 0);
      run.training_receipt.checkpoints.push_back(std::move(checkpoint_zero));
      const auto preprocessing = outer_preprocessing_config(device, outer_index);
      run.training_receipt.training = train_attribution_arm(
          model, data.ssl, data.probe_train, data.probe_validation,
          data.development, device, arm_config, seed,
          run.training_receipt.checkpoints, &preprocessing, outer_index);
      run.trained = rmc_evaluate(
          model, data.probe_train, data.probe_validation, data.development,
          data.reversed_train, data.reversed_validation,
          data.reversed_development, development_targets, device);
      initial[seed_index][arm] = run.initialization;
      trained[seed_index][arm] = run.trained;
      runs[seed_index][arm] = std::move(run);
      retained_models.push_back(model);
    }
    const auto pairing =
        rmc_validate_pairing(runs[seed_index][0], runs[seed_index][1]);
    rmc_emit_pairing(seed, pairing);
    mechanics = mechanics && pairing.pass;
  }

  const auto development = rmc_summarize(
      initial, trained, raw_development, data.development.target,
      development_targets.shuffled_evaluation,
      development_targets.order_evaluation,
      development_targets.shuffled_order_evaluation, bootstrap_rows,
      mechanics);
  rmc_emit_summary("development", development, initial, trained,
                   raw_development.area);
  std::cout << "rmc.development.mechanics=" << mechanics << '\n';

  const auto classification = development.gate.classification;
  if (classification == rmc_gate::Classification::invalid_mechanics_or_numeric) {
    std::cout << "rmc.final_classification=invalid_mechanics\n";
    std::cout << "rmc.confirmation.opened=false\n";
    std::cout << "execution_status=rmc_invalid_mechanics\n";
    return 3;
  }
  if (classification == rmc_gate::Classification::encoder_training_not_working) {
    std::cout << "rmc.final_classification=encoder_training_not_working\n";
    std::cout << "rmc.confirmation.opened=false\n";
    std::cout << "execution_status=rmc_development_complete\n";
    return 0;
  }

  const std::size_t selected_arm =
      classification == rmc_gate::Classification::qualified_candidate ? 1 : 0;
  rmc_open_confirmation(data);
  const auto confirmation_targets = rmc_make_targets(data, true);
  const auto raw_confirmation = rssm_probe_curve(
      data.raw_train, data.raw_validation, data.raw_confirmation,
      data.probe_train.target, data.probe_validation.target,
      data.confirmation.target, /*dual=*/true);
  validate_probe_curve_finite(raw_confirmation, "RMC raw confirmation");
  std::array<std::array<RmcEvaluation, kRmcArmCount>, 3>
      confirmation_initial{};
  std::array<std::array<RmcEvaluation, kRmcArmCount>, 3>
      confirmation_trained{};
  bool confirmation_mechanics = true;
  for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
       ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    const std::size_t retained_index = seed_index * kRmcArmCount + selected_arm;
    auto &trained_model = retained_models.at(retained_index);
    const auto outer_index = kRmcOuterArmIndices[selected_arm];
    set_paired_rng(seed, device);
    auto initial_model = mtf::MtfJepaMaeVicreg(
        attribution_config(device, kOuterAugmentationArms[outer_index]));
    confirmation_initial[seed_index][selected_arm] = rmc_evaluate(
        initial_model, data.probe_train, data.probe_validation,
        data.confirmation, data.reversed_train, data.reversed_validation,
        data.reversed_confirmation, confirmation_targets, device);
    confirmation_trained[seed_index][selected_arm] = rmc_evaluate(
        trained_model, data.probe_train, data.probe_validation,
        data.confirmation, data.reversed_train, data.reversed_validation,
        data.reversed_confirmation, confirmation_targets, device);
    confirmation_mechanics =
        confirmation_mechanics &&
        torch::isfinite(confirmation_trained[seed_index][selected_arm]
                            .probe.points.back().prediction)
            .all()
            .item<bool>();
    // Populate the unused arm with the selected values so the fixed two-arm
    // summarizer cannot accidentally inspect or encode the unselected model.
    const std::size_t unused = 1 - selected_arm;
    confirmation_initial[seed_index][unused] =
        confirmation_initial[seed_index][selected_arm];
    confirmation_trained[seed_index][unused] =
        confirmation_trained[seed_index][selected_arm];
  }
  const auto confirmation = rmc_summarize(
      confirmation_initial, confirmation_trained, raw_confirmation,
      data.confirmation.target, confirmation_targets.shuffled_evaluation,
      confirmation_targets.order_evaluation,
      confirmation_targets.shuffled_order_evaluation, bootstrap_rows,
      mechanics && confirmation_mechanics);
  rmc_emit_summary("confirmation", confirmation, confirmation_initial,
                   confirmation_trained, raw_confirmation.area);
  const auto &selected_gate = selected_arm == 0
                                  ? confirmation.gate.neutral
                                  : confirmation.gate.qualified;
  std::cout << "rmc.confirmation.opened=true\n";
  std::cout << "rmc.selected_arm=" << kRmcArmNames[selected_arm] << '\n';
  std::cout << "rmc.confirmation.selected_gate_pass=" << selected_gate.pass
            << '\n';
  std::cout << "rmc.final_classification="
            << (selected_gate.pass ? "representation_certified"
                                   : "confirmation_failed")
            << '\n';
  std::cout << "execution_status=rmc_measurements_complete\n";
  return 0;
}

} // namespace

#ifndef CUWACUNU_RMC_EMBEDDED
int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment == "representation-module-certification-preflight") {
      return run_rmc_preflight(options);
    }
    if (options.experiment == "representation-module-certification") {
      return run_rmc(options);
    }
    throw std::runtime_error(
        "--experiment must be representation-module-certification-preflight "
        "or representation-module-certification");
  } catch (const c10::Error &error) {
    std::cerr << "representation_module_certification_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "representation_module_certification_error=" << error.what()
              << '\n';
  }
  return 2;
}
#endif
