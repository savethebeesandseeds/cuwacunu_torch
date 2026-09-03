#include "piaabo/digest/sha256.h"

// Reuse the sealed FSPA-2 teacher, RMC data, probes, controls, and gate.
#define CUWACUNU_FSPA_EMBEDDED
#include "quality_wikimyei_mtf_jepa_mae_vicreg_frozen_sequence_projection_alignment.cpp"
#undef CUWACUNU_FSPA_EMBEDDED

namespace {

constexpr std::string_view kGpwdProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "GEOMETRY_PRESERVING_WHITENING_DISTILLATION_PROTOCOL.md";
constexpr std::string_view kGpwdProtocolSha256 =
    "c9f9c7d5d0028b74e645f0e4c49070037b0296951d3d990584ce929dd45a65bf";
constexpr int64_t kGpwdTeacherSteps = 1024;
constexpr int64_t kGpwdStudentSteps = 512;
constexpr double kGpwdEigenvalueFloorRatio = 1.0e-4;

struct GpwdWhiteningPlan {
  std::array<torch::Tensor, kChannels> means{};    // [1,32], CPU float64
  std::array<torch::Tensor, kChannels> matrices{}; // [32,32], CPU float64
  std::array<double, kChannels> eigenvalue_floors{};
  std::array<double, kChannels> minimum_matrix_eigenvalues{};
  std::array<Geometry, kChannels> ssl_geometry_before{};
  std::array<Geometry, kChannels> ssl_geometry_after{};
  bool finite{true};
  bool full_rank{true};
  bool pass{false};
};

struct GpwdTrainingReceipt {
  FspaTrainingReceipt base{};
  double target_ema_parameter_delta{0.0};
  bool target_ema_unchanged{false};
  bool pass{false};
};

struct GpwdEvaluationPair {
  RmcEvaluation teacher{};
  RmcEvaluation shadow{};
};

[[nodiscard]] Embeddings
gpwd_apply_whitening(const Embeddings &input,
                     const GpwdWhiteningPlan &plan) {
  if (input.by_channel.dim() != 3 || input.by_channel.size(1) != kChannels ||
      input.by_channel.size(2) != kLatentDim ||
      input.by_channel.scalar_type() != torch::kFloat64 ||
      input.by_channel.device().is_cuda()) {
    throw std::runtime_error("GPWD whitening input contract failed");
  }
  std::vector<torch::Tensor> channels;
  channels.reserve(kChannels);
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    const auto index = static_cast<std::size_t>(channel);
    channels.push_back((input.by_channel.select(1, channel) - plan.means[index])
                           .matmul(plan.matrices[index]));
  }
  auto by_channel = torch::stack(channels, 1).contiguous();
  if (!torch::isfinite(by_channel).all().item<bool>()) {
    throw std::runtime_error("GPWD whitening produced a non-finite value");
  }
  return {.by_channel = by_channel,
          .flat = by_channel.reshape({by_channel.size(0), kServedWidth})
                      .contiguous()};
}

[[nodiscard]] GpwdWhiteningPlan
gpwd_fit_whitening(const Embeddings &ssl_embeddings) {
  if (ssl_embeddings.by_channel.sizes() !=
          torch::IntArrayRef({256, kChannels, kLatentDim}) ||
      ssl_embeddings.by_channel.scalar_type() != torch::kFloat64 ||
      ssl_embeddings.by_channel.device().is_cuda()) {
    throw std::runtime_error("GPWD SSL capture contract failed");
  }
  GpwdWhiteningPlan result{};
  result.ssl_geometry_before = geometry(ssl_embeddings);
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    const auto index = static_cast<std::size_t>(channel);
    const auto values = ssl_embeddings.by_channel.select(1, channel);
    const auto mean = values.mean(0, true).contiguous();
    const auto centered = values - mean;
    auto covariance = centered.transpose(0, 1).matmul(centered) /
                      static_cast<double>(values.size(0) - 1);
    covariance = 0.5 * (covariance + covariance.transpose(0, 1));
    auto [eigenvalues, eigenvectors] = at::linalg_eigh(covariance, "L");
    const double maximum = eigenvalues.max().item<double>();
    const double minimum = eigenvalues.min().item<double>();
    const double floor = kGpwdEigenvalueFloorRatio * maximum;
    const auto safe_values = eigenvalues.clamp_min(floor);
    const auto scales = safe_values.rsqrt();
    const auto matrix =
        (eigenvectors * scales.view({1, kLatentDim}))
            .matmul(eigenvectors.transpose(0, 1))
            .contiguous();
    const auto matrix_eigenvalues = at::linalg_eigvalsh(matrix, "L");
    const double matrix_minimum = matrix_eigenvalues.min().item<double>();
    const bool finite = torch::isfinite(mean).all().item<bool>() &&
                        torch::isfinite(eigenvalues).all().item<bool>() &&
                        torch::isfinite(matrix).all().item<bool>() &&
                        std::isfinite(floor) &&
                        std::isfinite(matrix_minimum);
    const bool covariance_valid = maximum > 0.0 && floor > 0.0 &&
                                  minimum >= -1.0e-10 * maximum;
    const bool full_rank = matrix_minimum > 0.0;
    result.means[index] = mean;
    result.matrices[index] = matrix;
    result.eigenvalue_floors[index] = floor;
    result.minimum_matrix_eigenvalues[index] = matrix_minimum;
    result.finite = result.finite && finite && covariance_valid;
    result.full_rank = result.full_rank && full_rank;
  }
  const auto whitened = gpwd_apply_whitening(ssl_embeddings, result);
  result.ssl_geometry_after = geometry(whitened);
  result.pass = result.finite && result.full_rank;
  return result;
}

[[nodiscard]] RmcEvaluation gpwd_evaluate_embeddings(
    const Embeddings &train, const Embeddings &validation,
    const Embeddings &evaluated, const Embeddings &reverse_train,
    const Embeddings &reverse_validation,
    const Embeddings &reverse_evaluated, const Dataset &probe_train,
    const Dataset &probe_validation, const Dataset &evaluation,
    const RmcEvalTargets &targets) {
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
  validate_probe_curve_finite(result.probe, "GPWD predictive probe");
  validate_probe_curve_finite(result.shuffled_probe, "GPWD shuffled probe");
  rssm_validate_order_curve_finite(result.order, "GPWD order probe");
  rssm_validate_order_curve_finite(result.shuffled_order,
                                   "GPWD shuffled order probe");
  for (const auto &item : result.geometry) {
    validate_geometry_finite(item, "GPWD structured geometry");
  }
  return result;
}

[[nodiscard]] GpwdEvaluationPair gpwd_evaluate_teacher_and_shadow(
    mtf::MtfJepaMaeVicreg &model, const Dataset &probe_train,
    const Dataset &probe_validation, const Dataset &evaluation,
    const Dataset &reversed_train, const Dataset &reversed_validation,
    const Dataset &reversed_evaluation, const RmcEvalTargets &targets,
    const GpwdWhiteningPlan &plan, const torch::Device &device) {
  const auto train = rmc_extract_sparse_embeddings(model, probe_train, device);
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
  GpwdEvaluationPair result{};
  result.teacher = gpwd_evaluate_embeddings(
      train, validation, evaluated, reverse_train, reverse_validation,
      reverse_evaluated, probe_train, probe_validation, evaluation, targets);
  result.shadow = gpwd_evaluate_embeddings(
      gpwd_apply_whitening(train, plan),
      gpwd_apply_whitening(validation, plan),
      gpwd_apply_whitening(evaluated, plan),
      gpwd_apply_whitening(reverse_train, plan),
      gpwd_apply_whitening(reverse_validation, plan),
      gpwd_apply_whitening(reverse_evaluated, plan), probe_train,
      probe_validation, evaluation, targets);
  return result;
}

[[nodiscard]] std::vector<torch::Tensor>
gpwd_served_parameters(mtf::MtfJepaMaeVicreg &model) {
  std::vector<torch::Tensor> result;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    const auto &name = item.key();
    const bool served =
        name.rfind("tokenizer.", 0) == 0 || name.rfind("encoder.", 0) == 0;
    if (served && item.value().requires_grad()) {
      result.push_back(item.value());
    }
  }
  if (result.empty()) {
    throw std::runtime_error("GPWD served parameter set is empty");
  }
  return result;
}

[[nodiscard]] GpwdTrainingReceipt gpwd_train_student(
    mtf::MtfJepaMaeVicreg &model, const Dataset &ssl,
    const Embeddings &cached_target, const torch::Device &device, int64_t seed) {
  if (cached_target.by_channel.sizes() !=
          torch::IntArrayRef({ssl.data.size(0), kChannels, kLatentDim}) ||
      cached_target.by_channel.requires_grad() ||
      cached_target.by_channel.scalar_type() != torch::kFloat64 ||
      cached_target.by_channel.device().is_cuda() ||
      !torch::isfinite(cached_target.by_channel).all().item<bool>()) {
    throw std::runtime_error("GPWD cached target contract failed");
  }
  auto parameters = gpwd_served_parameters(model);
  torch::optim::Adam optimizer(parameters, torch::optim::AdamOptions(1.0e-3));
  const auto initial_parameters = snapshot_parameters(model);
  GpwdTrainingReceipt receipt{};
  receipt.base.losses.reserve(kGpwdStudentSteps);
  model->train();
  for (int64_t step = 0; step < kGpwdStudentSteps; ++step) {
    const auto rows = training_rows(ssl, seed, step);
    const auto indices = torch::tensor(rows, torch::kInt64);
    const auto data = ssl.data.index_select(0, indices).to(device);
    const auto mask = ssl.mask.index_select(0, indices).to(device);
    const auto target = cached_target.by_channel.index_select(0, indices)
                            .to(device, torch::kFloat32)
                            .detach()
                            .contiguous();
    set_paired_rng(paired_step_seed(seed, step), device);
    optimizer.zero_grad();
    const auto encoded = model->encode(data, mask);
    const auto served = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1,
        model->config());
    receipt.base.target_contract =
        receipt.base.target_contract && !target.requires_grad() &&
        target.sizes() == torch::IntArrayRef({kModelRowBatchSize, 3, 32}) &&
        torch::isfinite(target).all().item<bool>();
    receipt.base.sparse_contract =
        receipt.base.sparse_contract && served.valid_mask.all().item<bool>() &&
        served.values.sizes() == target.sizes() &&
        torch::isfinite(served.values).all().item<bool>();
    const auto loss = (served.values - target).pow(2).mean();
    const double loss_value = loss.item<double>();
    receipt.base.finite = receipt.base.finite && std::isfinite(loss_value);
    receipt.base.losses.push_back(loss_value);
    loss.backward();
    double gradient_square_sum = 0.0;
    for (const auto &parameter : parameters) {
      if (!parameter.grad().defined()) {
        continue;
      }
      receipt.base.finite =
          receipt.base.finite &&
          torch::isfinite(parameter.grad()).all().item<bool>();
      gradient_square_sum +=
          parameter.grad().detach().pow(2).sum().item<double>();
    }
    const double gradient_norm = std::sqrt(gradient_square_sum);
    receipt.base.minimum_gradient_norm =
        std::min(receipt.base.minimum_gradient_norm, gradient_norm);
    receipt.base.maximum_gradient_norm =
        std::max(receipt.base.maximum_gradient_norm, gradient_norm);
    const auto served_before = served_parameter_vector(model);
    optimizer.step();
    const auto served_after = served_parameter_vector(model);
    const double update_norm =
        (served_after - served_before).norm().item<double>();
    receipt.base.minimum_update_norm =
        std::min(receipt.base.minimum_update_norm, update_norm);
    receipt.base.maximum_update_norm =
        std::max(receipt.base.maximum_update_norm, update_norm);
    receipt.base.finite = receipt.base.finite && std::isfinite(gradient_norm) &&
                          std::isfinite(update_norm);
  }
  receipt.base.first_eight_mean =
      mean_loss_window(receipt.base.losses, true);
  receipt.base.last_eight_mean = mean_loss_window(receipt.base.losses, false);
  receipt.base.loss_decreased =
      receipt.base.last_eight_mean < receipt.base.first_eight_mean;
  receipt.base.served_parameter_delta = parameter_partition_max_abs_diff(
      model, initial_parameters, ParameterDeltaPartition::served);
  receipt.base.predictor_parameter_delta = parameter_partition_max_abs_diff(
      model, initial_parameters, ParameterDeltaPartition::predictor);
  receipt.base.mae_decoder_parameter_delta = parameter_partition_max_abs_diff(
      model, initial_parameters, ParameterDeltaPartition::mae_decoder);
  receipt.base.vicreg_head_parameter_delta = parameter_partition_max_abs_diff(
      model, initial_parameters, ParameterDeltaPartition::vicreg_head);
  receipt.target_ema_parameter_delta = parameter_partition_max_abs_diff(
      model, initial_parameters, ParameterDeltaPartition::target_ema);
  receipt.base.served_updated = receipt.base.served_parameter_delta > 0.0 &&
                                receipt.base.minimum_gradient_norm > 0.0 &&
                                receipt.base.minimum_update_norm > 0.0;
  receipt.target_ema_unchanged = receipt.target_ema_parameter_delta == 0.0;
  receipt.base.inactive_heads_unchanged =
      receipt.base.predictor_parameter_delta == 0.0 &&
      receipt.base.mae_decoder_parameter_delta == 0.0 &&
      receipt.base.vicreg_head_parameter_delta == 0.0;
  receipt.base.pass =
      receipt.base.losses.size() ==
          static_cast<std::size_t>(kGpwdStudentSteps) &&
      receipt.base.target_contract && receipt.base.sparse_contract &&
      receipt.base.finite && receipt.base.loss_decreased &&
      receipt.base.served_updated && receipt.base.inactive_heads_unchanged;
  receipt.pass = receipt.base.pass && receipt.target_ema_unchanged;
  return receipt;
}

void gpwd_emit_whitening(int64_t seed, const GpwdWhiteningPlan &plan) {
  const std::string root = "gpwd.seed_" + std::to_string(seed) + ".whitening";
  for (std::size_t channel = 0; channel < kChannels; ++channel) {
    const std::string prefix =
        root + ".channel_" + std::to_string(channel);
    std::cout << prefix << ".eigenvalue_floor="
              << plan.eigenvalue_floors[channel] << '\n';
    std::cout << prefix << ".matrix_minimum_eigenvalue="
              << plan.minimum_matrix_eigenvalues[channel] << '\n';
    std::cout << prefix << ".ssl_participation_before="
              << plan.ssl_geometry_before[channel].participation_rank_ratio
              << '\n';
    std::cout << prefix << ".ssl_participation_after="
              << plan.ssl_geometry_after[channel].participation_rank_ratio
              << '\n';
  }
  std::cout << root << ".finite=" << plan.finite << '\n';
  std::cout << root << ".full_rank=" << plan.full_rank << '\n';
  std::cout << root << ".pass=" << plan.pass << '\n';
}

void gpwd_emit_training(int64_t seed, const GpwdTrainingReceipt &receipt) {
  const std::string root = "gpwd.seed_" + std::to_string(seed) + ".student";
  std::cout << root << ".first_eight_loss_mean="
            << receipt.base.first_eight_mean << '\n';
  std::cout << root << ".last_eight_loss_mean="
            << receipt.base.last_eight_mean << '\n';
  std::cout << root << ".minimum_gradient_norm="
            << receipt.base.minimum_gradient_norm << '\n';
  std::cout << root << ".minimum_update_norm="
            << receipt.base.minimum_update_norm << '\n';
  std::cout << root << ".served_parameter_delta="
            << receipt.base.served_parameter_delta << '\n';
  std::cout << root << ".inactive_heads_unchanged="
            << receipt.base.inactive_heads_unchanged << '\n';
  std::cout << root << ".target_ema_unchanged="
            << receipt.target_ema_unchanged << '\n';
  std::cout << root << ".pass=" << receipt.pass << '\n';
}

[[nodiscard]] bool
gpwd_semantics_pass(const rmc_gate::CandidateResult &gate) {
  return gate.numeric_valid && gate.mechanics_pass && gate.learned_point_pass &&
         gate.learned_lower_pass && gate.learned_seed_pass &&
         gate.family_positive_count_pass && gate.family_floor_pass &&
         gate.raw_noninferiority_pass && gate.order_point_pass &&
         gate.order_lower_pass && gate.order_retention_pass &&
         gate.continuous_shuffle_pass && gate.order_shuffle_pass;
}

void gpwd_emit_candidate(
    const std::string &scope, const RmcSummary &summary,
    const std::array<std::array<RmcEvaluation, 2>, 3> &initial,
    const std::array<std::array<RmcEvaluation, 2>, 3> &trained,
    double raw_area) {
  const auto &candidate = summary.candidate[0];
  const auto &gate = summary.gate.neutral;
  const std::string root = "gpwd." + scope;
  std::cout << root << ".raw_control.aulc=" << raw_area << '\n';
  for (std::size_t seed = 0; seed < 3; ++seed) {
    const std::string item =
        root + ".seed_" + std::to_string(kAttributionSeeds[seed]);
    std::cout << item << ".initial.aulc=" << initial[seed][0].probe.area
              << '\n';
    std::cout << item << ".final.aulc=" << trained[seed][0].probe.area
              << '\n';
    std::cout << item << ".final.order_aulc="
              << trained[seed][0].order.area << '\n';
    for (std::size_t channel = 0; channel < kChannels; ++channel) {
      const auto &geometry_value = trained[seed][0].geometry[channel];
      const std::string geometry_prefix =
          item + ".geometry.channel_" + std::to_string(channel);
      std::cout << geometry_prefix << ".effective="
                << geometry_value.effective_rank_ratio << '\n';
      std::cout << geometry_prefix << ".participation="
                << geometry_value.participation_rank_ratio << '\n';
      std::cout << geometry_prefix << ".top="
                << geometry_value.top_eigenvalue_share << '\n';
      std::cout << geometry_prefix << ".active="
                << geometry_value.active_dimension_fraction << '\n';
    }
  }
  rmc_emit_contrast(root + ".trained_minus_initial",
                    candidate.gate.trained_minus_initialization);
  rmc_emit_contrast(root + ".final_minus_raw",
                    candidate.gate.final_minus_raw);
  rmc_emit_contrast(root + ".order_trained_minus_initial",
                    candidate.gate.order_trained_minus_initialization);
  std::cout << root << ".final.aulc=" << candidate.final.point << '\n';
  std::cout << root << ".final.bootstrap_95_low="
            << candidate.final.interval.low << '\n';
  std::cout << root << ".final.bootstrap_95_high="
            << candidate.final.interval.high << '\n';
  std::cout << root << ".final.order_aulc="
            << candidate.final_order.point << '\n';
  std::cout << root << ".final.order_bootstrap_95_low="
            << candidate.final_order.interval.low << '\n';
  std::cout << root << ".final.order_bootstrap_95_high="
            << candidate.final_order.interval.high << '\n';
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
  std::cout << root << ".gate.semantics_pass=" << gpwd_semantics_pass(gate)
            << '\n';
  std::cout << root << ".gate.geometry_pass=" << gate.geometry_pass << '\n';
  std::cout << root << ".gate.pass=" << gate.pass << '\n';
}

[[nodiscard]] bool gpwd_options_valid(const Options &options) {
  return options.device == "cuda" &&
         (options.steps < 0 || options.steps == kGpwdStudentSteps) &&
         (options.seeds < 0 ||
          options.seeds == static_cast<int64_t>(kAttributionSeeds.size()));
}

int run_gpwd_preflight(const Options &options) {
  if (!gpwd_options_valid(options)) {
    throw std::runtime_error(
        "GPWD preflight requires CUDA, 512 student steps, and 3 seeds");
  }
  rmc_configure_cuda();
  const torch::Device device(torch::kCUDA, 0);
  const auto protocol =
      rmc_read_file(std::filesystem::path(kGpwdProtocolPath));
  const auto protocol_hash = digest::sha256_hex(protocol);
  auto data = rmc_make_data();
  set_paired_rng(kAttributionSeeds.front(), device);
  auto model = mtf::MtfJepaMaeVicreg(
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]));
  const auto parameters_before = snapshot_parameters(model);
  const auto ssl_capture =
      rmc_extract_sparse_embeddings(model, data.ssl, device);
  const auto plan = gpwd_fit_whitening(ssl_capture);
  const auto cached_target = gpwd_apply_whitening(ssl_capture, plan);
  const auto rows = training_rows(data.ssl, kAttributionSeeds.front(), 0);
  const auto indices = torch::tensor(rows, torch::kInt64);
  const auto input = data.ssl.data.index_select(0, indices).to(device);
  const auto mask = data.ssl.mask.index_select(0, indices).to(device);
  const auto target = cached_target.by_channel.index_select(0, indices)
                          .to(device, torch::kFloat32)
                          .detach();
  const auto encoded = model->encode(input, mask);
  const auto served = mtf::select_mtf_serving_pool(
      encoded, mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1,
      model->config());
  const auto loss = (served.values - target).pow(2).mean();
  loss.backward();
  double gradient_square_sum = 0.0;
  for (const auto &parameter : gpwd_served_parameters(model)) {
    if (parameter.grad().defined()) {
      gradient_square_sum +=
          parameter.grad().detach().pow(2).sum().item<double>();
    }
  }
  const double gradient_norm = std::sqrt(gradient_square_sum);
  const bool parameters_unchanged =
      parameter_max_abs_diff(model, parameters_before) == 0.0;
  const bool pass = protocol_hash == kGpwdProtocolSha256 && plan.pass &&
                    cached_target.by_channel.sizes() ==
                        torch::IntArrayRef({256, 3, 32}) &&
                    target.sizes() == torch::IntArrayRef({96, 3, 32}) &&
                    !target.requires_grad() &&
                    torch::isfinite(loss).item<bool>() &&
                    std::isfinite(gradient_norm) && gradient_norm > 0.0 &&
                    served.valid_mask.all().item<bool>() &&
                    parameters_unchanged;
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.gpwd.preflight.v1\n";
  std::cout << "gpwd.protocol.sha256=" << protocol_hash << '\n';
  std::cout << "gpwd.protocol.exact="
            << (protocol_hash == kGpwdProtocolSha256) << '\n';
  std::cout << "gpwd.teacher_steps=" << kGpwdTeacherSteps << '\n';
  std::cout << "gpwd.student_steps=" << kGpwdStudentSteps << '\n';
  std::cout << "gpwd.ssl_capture.shape_exact="
            << (ssl_capture.by_channel.sizes() ==
                torch::IntArrayRef({256, 3, 32}))
            << '\n';
  std::cout << "gpwd.whitening.pass=" << plan.pass << '\n';
  std::cout << "gpwd.target.requires_grad=" << target.requires_grad() << '\n';
  std::cout << "gpwd.loss=" << loss.item<double>() << '\n';
  std::cout << "gpwd.gradient_norm=" << gradient_norm << '\n';
  std::cout << "gpwd.parameters_unchanged=" << parameters_unchanged << '\n';
  std::cout << "optimizer_steps=0\n";
  std::cout << "augmentation_calls=0\n";
  std::cout << "training_labels_used=false\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "gpwd.preflight.pass=" << pass << '\n';
  return pass ? 0 : 3;
}

int run_gpwd(const Options &options) {
  if (!gpwd_options_valid(options)) {
    throw std::runtime_error(
        "GPWD requires CUDA, 512 student steps, and 3 seeds");
  }
  rmc_configure_cuda();
  const torch::Device device(torch::kCUDA, 0);
  const auto protocol =
      rmc_read_file(std::filesystem::path(kGpwdProtocolPath));
  const auto protocol_hash = digest::sha256_hex(protocol);
  if (protocol_hash != kGpwdProtocolSha256) {
    throw std::runtime_error("GPWD protocol hash mismatch");
  }
  auto data = rmc_make_data();
  const auto development_targets = rmc_make_targets(data, false);
  const auto bootstrap_rows = rmc_bootstrap_rows(256);
  if (!rmc_bootstrap_rows_valid(bootstrap_rows, 256)) {
    throw std::runtime_error("GPWD bootstrap table failed");
  }
  const auto raw_development = rssm_probe_curve(
      data.raw_train, data.raw_validation, data.raw_development,
      data.probe_train.target, data.probe_validation.target,
      data.development.target, /*dual=*/true);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.gpwd.v1\n";
  std::cout << "experiment=geometry-preserving-whitening-distillation\n";
  std::cout << "module_only=true\n";
  std::cout << "device=cuda:0\n";
  std::cout << "readout_policy=structured_cdsb_sparse_v1\n";
  std::cout << "teacher_objective=frozen_raw96_direct_mse\n";
  std::cout << "student_objective=cached_whitened_teacher_direct_mse\n";
  std::cout << "training_labels_used=false\n";
  std::cout << "outer_augmentation=neutral\n";
  std::cout << "augmentation_calls=0\n";
  std::cout << "teacher_steps=" << kGpwdTeacherSteps << '\n';
  std::cout << "student_steps=" << kGpwdStudentSteps << '\n';
  std::cout << "model_seeds=17,31,47\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "gpwd.protocol.sha256=" << protocol_hash << '\n';
  std::cout << "gpwd.confirmation.opened=false\n";

  std::array<std::array<RmcEvaluation, 2>, 3> initial{};
  std::array<std::array<RmcEvaluation, 2>, 3> teacher{};
  std::array<std::array<RmcEvaluation, 2>, 3> shadow{};
  std::array<Embeddings, 3> cached_targets{};
  std::vector<mtf::MtfJepaMaeVicreg> retained_models;
  retained_models.reserve(3);
  bool teacher_mechanics = true;
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    set_paired_rng(seed, device);
    auto model = mtf::MtfJepaMaeVicreg(
        attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]));
    const auto initial_evaluation = rmc_evaluate(
        model, data.probe_train, data.probe_validation, data.development,
        data.reversed_train, data.reversed_validation,
        data.reversed_development, development_targets, device);
    const bool initialization_reproduced =
        std::abs(initial_evaluation.probe.area -
                 kExpectedInitialAulc[seed_index]) <= 1.0e-9;
    const auto teacher_training =
        fspa_train(model, data.ssl, device, seed, kGpwdTeacherSteps);
    const auto ssl_capture =
        rmc_extract_sparse_embeddings(model, data.ssl, device);
    const auto plan = gpwd_fit_whitening(ssl_capture);
    cached_targets[seed_index] = gpwd_apply_whitening(ssl_capture, plan);
    const auto evaluations = gpwd_evaluate_teacher_and_shadow(
        model, data.probe_train, data.probe_validation, data.development,
        data.reversed_train, data.reversed_validation,
        data.reversed_development, development_targets, plan, device);
    initial[seed_index][0] = initial_evaluation;
    initial[seed_index][1] = initial_evaluation;
    teacher[seed_index][0] = evaluations.teacher;
    teacher[seed_index][1] = evaluations.teacher;
    shadow[seed_index][0] = evaluations.shadow;
    shadow[seed_index][1] = evaluations.shadow;
    teacher_mechanics = teacher_mechanics && initialization_reproduced &&
                        teacher_training.pass && plan.pass;
    std::cout << "gpwd.seed_" << seed
              << ".initialization_reproduced="
              << initialization_reproduced << '\n';
    std::cout << "gpwd.seed_" << seed
              << ".teacher_training_pass=" << teacher_training.pass << '\n';
    std::cout << "gpwd.seed_" << seed
              << ".teacher_final_aulc=" << evaluations.teacher.probe.area
              << '\n';
    gpwd_emit_whitening(seed, plan);
    retained_models.push_back(model);
  }

  const auto teacher_summary = rmc_summarize(
      initial, teacher, raw_development, data.development.target,
      development_targets.shuffled_evaluation,
      development_targets.order_evaluation,
      development_targets.shuffled_order_evaluation, bootstrap_rows,
      teacher_mechanics);
  const bool teacher_reproduced =
      teacher_mechanics && gpwd_semantics_pass(teacher_summary.gate.neutral);
  std::cout << "gpwd.teacher.mechanics=" << teacher_mechanics << '\n';
  std::cout << "gpwd.teacher.semantic_result_reproduced="
            << teacher_reproduced << '\n';

  const auto shadow_summary = rmc_summarize(
      initial, shadow, raw_development, data.development.target,
      development_targets.shuffled_evaluation,
      development_targets.order_evaluation,
      development_targets.shuffled_order_evaluation, bootstrap_rows,
      teacher_reproduced);
  gpwd_emit_candidate("shadow_development", shadow_summary, initial, shadow,
                      raw_development.area);
  const bool shadow_pass = shadow_summary.gate.neutral.pass;
  std::cout << "gpwd.shadow.development_gate_pass=" << shadow_pass << '\n';
  if (!teacher_reproduced) {
    std::cout << "gpwd.final_classification=teacher_reproduction_failed\n";
    std::cout << "gpwd.student.opened=false\n";
    std::cout << "gpwd.confirmation.opened=false\n";
    std::cout << "execution_status=gpwd_invalid_teacher\n";
    return 3;
  }
  if (!shadow_pass) {
    std::cout << "gpwd.final_classification=shadow_gate_failed\n";
    std::cout << "gpwd.student.opened=false\n";
    std::cout << "gpwd.confirmation.opened=false\n";
    std::cout << "execution_status=gpwd_shadow_complete\n";
    return 0;
  }

  std::cout << "gpwd.student.opened=true\n";
  std::array<std::array<RmcEvaluation, 2>, 3> student{};
  bool student_mechanics = teacher_reproduced;
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    const auto training = gpwd_train_student(
        retained_models[seed_index], data.ssl, cached_targets[seed_index],
        device, seed);
    gpwd_emit_training(seed, training);
    const auto evaluation = rmc_evaluate(
        retained_models[seed_index], data.probe_train, data.probe_validation,
        data.development, data.reversed_train, data.reversed_validation,
        data.reversed_development, development_targets, device);
    student[seed_index][0] = evaluation;
    student[seed_index][1] = evaluation;
    student_mechanics = student_mechanics && training.pass;
  }
  const auto student_summary = rmc_summarize(
      initial, student, raw_development, data.development.target,
      development_targets.shuffled_evaluation,
      development_targets.order_evaluation,
      development_targets.shuffled_order_evaluation, bootstrap_rows,
      student_mechanics);
  gpwd_emit_candidate("student_development", student_summary, initial, student,
                      raw_development.area);
  const bool student_pass = student_summary.gate.neutral.pass;
  std::cout << "gpwd.student.mechanics=" << student_mechanics << '\n';
  std::cout << "gpwd.student.development_gate_pass=" << student_pass << '\n';
  if (!student_mechanics) {
    std::cout << "gpwd.final_classification=invalid_student_mechanics\n";
    std::cout << "gpwd.confirmation.opened=false\n";
    std::cout << "execution_status=gpwd_invalid_student\n";
    return 3;
  }
  if (!student_pass) {
    std::cout << "gpwd.final_classification=student_development_gate_failed\n";
    std::cout << "gpwd.confirmation.opened=false\n";
    std::cout << "execution_status=gpwd_development_complete\n";
    return 0;
  }

  rmc_open_confirmation(data);
  const auto confirmation_targets = rmc_make_targets(data, true);
  const auto raw_confirmation = rssm_probe_curve(
      data.raw_train, data.raw_validation, data.raw_confirmation,
      data.probe_train.target, data.probe_validation.target,
      data.confirmation.target, /*dual=*/true);
  std::array<std::array<RmcEvaluation, 2>, 3> confirmation_initial{};
  std::array<std::array<RmcEvaluation, 2>, 3> confirmation_student{};
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    set_paired_rng(seed, device);
    auto initial_model = mtf::MtfJepaMaeVicreg(
        attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]));
    const auto initial_evaluation = rmc_evaluate(
        initial_model, data.probe_train, data.probe_validation,
        data.confirmation, data.reversed_train, data.reversed_validation,
        data.reversed_confirmation, confirmation_targets, device);
    const auto student_evaluation = rmc_evaluate(
        retained_models[seed_index], data.probe_train, data.probe_validation,
        data.confirmation, data.reversed_train, data.reversed_validation,
        data.reversed_confirmation, confirmation_targets, device);
    confirmation_initial[seed_index][0] = initial_evaluation;
    confirmation_initial[seed_index][1] = initial_evaluation;
    confirmation_student[seed_index][0] = student_evaluation;
    confirmation_student[seed_index][1] = student_evaluation;
  }
  const auto confirmation_summary = rmc_summarize(
      confirmation_initial, confirmation_student, raw_confirmation,
      data.confirmation.target, confirmation_targets.shuffled_evaluation,
      confirmation_targets.order_evaluation,
      confirmation_targets.shuffled_order_evaluation, bootstrap_rows,
      student_mechanics);
  gpwd_emit_candidate("student_confirmation", confirmation_summary,
                      confirmation_initial, confirmation_student,
                      raw_confirmation.area);
  const bool confirmation_pass = confirmation_summary.gate.neutral.pass;
  std::cout << "gpwd.confirmation.opened=true\n";
  std::cout << "gpwd.confirmation.gate_pass=" << confirmation_pass << '\n';
  std::cout << "gpwd.final_classification="
            << (confirmation_pass
                    ? "representation_certified_fspa3_whitened_distillation_v1"
                    : "confirmation_failed")
            << '\n';
  std::cout << "execution_status=gpwd_measurements_complete\n";
  return 0;
}

} // namespace

#ifndef CUWACUNU_GPWD_EMBEDDED
int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment ==
        "geometry-preserving-whitening-distillation-preflight") {
      return run_gpwd_preflight(options);
    }
    if (options.experiment ==
        "geometry-preserving-whitening-distillation") {
      return run_gpwd(options);
    }
    throw std::runtime_error(
        "--experiment must be geometry-preserving-whitening-distillation-"
        "preflight or geometry-preserving-whitening-distillation");
  } catch (const c10::Error &error) {
    std::cerr << "geometry_preserving_whitening_distillation_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "geometry_preserving_whitening_distillation_error="
              << error.what() << '\n';
  }
  return 2;
}
#endif
