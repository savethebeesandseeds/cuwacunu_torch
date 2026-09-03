#include "piaabo/digest/sha256.h"

// Reuse the sealed FSPA-2 teacher and FSPA-3 isolation/report mechanics.
#define CUWACUNU_GPWD_EMBEDDED
#include "quality_wikimyei_mtf_jepa_mae_vicreg_geometry_preserving_whitening_distillation.cpp"
#undef CUWACUNU_GPWD_EMBEDDED

namespace {

constexpr std::string_view kMpsrProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "MINIMAL_PARTICIPATION_SPECTRAL_REPAIR_PROTOCOL.md";
constexpr std::string_view kMpsrProtocolSha256 =
    "4cf4f81ffac1665f85bd233203ccf2f039617ec8d52b41a40258002b42999b00";
constexpr double kMpsrParticipationTarget = 0.30;
constexpr int64_t kMpsrBisectionSteps = 64;

struct MpsrPlan {
  std::array<torch::Tensor, kChannels> means{};
  std::array<torch::Tensor, kChannels> matrices{};
  std::array<double, kChannels> cap_ratios{};
  std::array<double, kChannels> participation_before{};
  std::array<double, kChannels> participation_after{};
  std::array<double, kChannels> mean_relative_errors{};
  std::array<double, kChannels> trace_relative_errors{};
  std::array<double, kChannels> minimum_matrix_eigenvalues{};
  std::array<double, kChannels> matrix_identity_max_abs_diffs{};
  std::array<bool, kChannels> identity{};
  bool finite{true};
  bool positive_spectrum{true};
  bool full_rank{true};
  bool moments_preserved{true};
  bool participation_target_met{true};
  bool pass{false};
};

struct MpsrEvaluationPair {
  RmcEvaluation teacher{};
  RmcEvaluation shadow{};
};

[[nodiscard]] double
mpsr_participation(const torch::Tensor &eigenvalues) {
  const double total = eigenvalues.sum().item<double>();
  const double square_sum = eigenvalues.pow(2).sum().item<double>();
  if (!(total > 0.0) || !(square_sum > 0.0)) {
    return 0.0;
  }
  return total * total / square_sum /
         static_cast<double>(eigenvalues.numel());
}

[[nodiscard]] Embeddings mpsr_apply(const Embeddings &input,
                                    const MpsrPlan &plan) {
  if (input.by_channel.dim() != 3 || input.by_channel.size(1) != kChannels ||
      input.by_channel.size(2) != kLatentDim ||
      input.by_channel.scalar_type() != torch::kFloat64 ||
      input.by_channel.device().is_cuda()) {
    throw std::runtime_error("MPSR input contract failed");
  }
  std::vector<torch::Tensor> channels;
  channels.reserve(kChannels);
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    const auto index = static_cast<std::size_t>(channel);
    const auto centered =
        input.by_channel.select(1, channel) - plan.means[index];
    channels.push_back(
        (plan.means[index] + centered.matmul(plan.matrices[index]))
            .contiguous());
  }
  auto by_channel = torch::stack(channels, 1).contiguous();
  if (!torch::isfinite(by_channel).all().item<bool>()) {
    throw std::runtime_error("MPSR produced a non-finite value");
  }
  return {.by_channel = by_channel,
          .flat = by_channel.reshape({by_channel.size(0), kServedWidth})
                      .contiguous()};
}

[[nodiscard]] MpsrPlan mpsr_fit(const Embeddings &ssl_embeddings) {
  if (ssl_embeddings.by_channel.sizes() !=
          torch::IntArrayRef({256, kChannels, kLatentDim}) ||
      ssl_embeddings.by_channel.scalar_type() != torch::kFloat64 ||
      ssl_embeddings.by_channel.device().is_cuda()) {
    throw std::runtime_error("MPSR SSL capture contract failed");
  }
  MpsrPlan result{};
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    const auto index = static_cast<std::size_t>(channel);
    const auto values = ssl_embeddings.by_channel.select(1, channel);
    const auto mean = values.mean(0, true).contiguous();
    const auto centered = values - mean;
    auto covariance = centered.transpose(0, 1).matmul(centered) /
                      static_cast<double>(values.size(0) - 1);
    covariance = 0.5 * (covariance + covariance.transpose(0, 1));
    auto [eigenvalues, eigenvectors] = at::linalg_eigh(covariance, "L");
    const double minimum = eigenvalues.min().item<double>();
    const double maximum = eigenvalues.max().item<double>();
    const double total = eigenvalues.sum().item<double>();
    const double before = mpsr_participation(eigenvalues);
    const bool identity = before >= kMpsrParticipationTarget;
    double cap = maximum;
    if (!identity) {
      double lower = minimum;
      double upper = maximum;
      for (int64_t iteration = 0; iteration < kMpsrBisectionSteps;
           ++iteration) {
        const double middle = 0.5 * (lower + upper);
        const double participation =
            mpsr_participation(eigenvalues.clamp_max(middle));
        if (participation >= kMpsrParticipationTarget) {
          lower = middle;
        } else {
          upper = middle;
        }
      }
      cap = lower;
    }
    const auto capped = eigenvalues.clamp_max(cap);
    const auto target_eigenvalues = capped * (total / capped.sum());
    const auto scales = (target_eigenvalues / eigenvalues).sqrt();
    const auto matrix =
        (eigenvectors * scales.view({1, kLatentDim}))
            .matmul(eigenvectors.transpose(0, 1))
            .contiguous();
    const auto matrix_eigenvalues = at::linalg_eigvalsh(matrix, "L");
    const double matrix_minimum = matrix_eigenvalues.min().item<double>();
    const auto repaired = mean + centered.matmul(matrix);
    const auto repaired_centered = repaired - repaired.mean(0, true);
    auto repaired_covariance =
        repaired_centered.transpose(0, 1).matmul(repaired_centered) /
        static_cast<double>(repaired.size(0) - 1);
    repaired_covariance =
        0.5 * (repaired_covariance + repaired_covariance.transpose(0, 1));
    const auto repaired_eigenvalues =
        at::linalg_eigvalsh(repaired_covariance, "L");
    const double after = mpsr_participation(repaired_eigenvalues);
    const double mean_error =
        (repaired.mean(0, true) - mean).abs().max().item<double>() /
        std::max(1.0, mean.abs().max().item<double>());
    const double repaired_trace = repaired_eigenvalues.sum().item<double>();
    const double trace_error = std::abs(repaired_trace - total) / total;
    const auto identity_matrix =
        torch::eye(kLatentDim, torch::TensorOptions().dtype(torch::kFloat64));
    const double identity_difference =
        (matrix - identity_matrix).abs().max().item<double>();
    const bool finite = torch::isfinite(mean).all().item<bool>() &&
                        torch::isfinite(eigenvalues).all().item<bool>() &&
                        torch::isfinite(matrix).all().item<bool>() &&
                        torch::isfinite(repaired).all().item<bool>() &&
                        std::isfinite(after) && std::isfinite(mean_error) &&
                        std::isfinite(trace_error);
    const bool positive = minimum > 0.0 && maximum > 0.0 && cap > 0.0;
    const bool target_met =
        identity ? after >= kMpsrParticipationTarget
                 : after >= kMpsrParticipationTarget - 1.0e-10 &&
                       after <= kMpsrParticipationTarget + 1.0e-6;
    result.means[index] = mean;
    result.matrices[index] = matrix;
    result.cap_ratios[index] = cap / maximum;
    result.participation_before[index] = before;
    result.participation_after[index] = after;
    result.mean_relative_errors[index] = mean_error;
    result.trace_relative_errors[index] = trace_error;
    result.minimum_matrix_eigenvalues[index] = matrix_minimum;
    result.matrix_identity_max_abs_diffs[index] = identity_difference;
    result.identity[index] = identity;
    result.finite = result.finite && finite;
    result.positive_spectrum = result.positive_spectrum && positive;
    result.full_rank = result.full_rank && matrix_minimum > 0.0;
    result.moments_preserved = result.moments_preserved &&
                               mean_error <= 1.0e-9 &&
                               trace_error <= 1.0e-9;
    result.participation_target_met =
        result.participation_target_met && target_met;
  }
  result.pass = result.finite && result.positive_spectrum && result.full_rank &&
                result.moments_preserved &&
                result.participation_target_met;
  return result;
}

[[nodiscard]] MpsrEvaluationPair mpsr_evaluate_teacher_and_shadow(
    mtf::MtfJepaMaeVicreg &model, const Dataset &probe_train,
    const Dataset &probe_validation, const Dataset &evaluation,
    const Dataset &reversed_train, const Dataset &reversed_validation,
    const Dataset &reversed_evaluation, const RmcEvalTargets &targets,
    const MpsrPlan &plan, const torch::Device &device) {
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
  MpsrEvaluationPair result{};
  result.teacher = gpwd_evaluate_embeddings(
      train, validation, evaluated, reverse_train, reverse_validation,
      reverse_evaluated, probe_train, probe_validation, evaluation, targets);
  result.shadow = gpwd_evaluate_embeddings(
      mpsr_apply(train, plan), mpsr_apply(validation, plan),
      mpsr_apply(evaluated, plan), mpsr_apply(reverse_train, plan),
      mpsr_apply(reverse_validation, plan),
      mpsr_apply(reverse_evaluated, plan), probe_train, probe_validation,
      evaluation, targets);
  return result;
}

void mpsr_emit_plan(int64_t seed, const MpsrPlan &plan) {
  const std::string root = "mpsr.seed_" + std::to_string(seed) + ".spectral";
  for (std::size_t channel = 0; channel < kChannels; ++channel) {
    const std::string prefix =
        root + ".channel_" + std::to_string(channel);
    std::cout << prefix << ".identity=" << plan.identity[channel] << '\n';
    std::cout << prefix << ".cap_ratio=" << plan.cap_ratios[channel] << '\n';
    std::cout << prefix << ".participation_before="
              << plan.participation_before[channel] << '\n';
    std::cout << prefix << ".participation_after="
              << plan.participation_after[channel] << '\n';
    std::cout << prefix << ".matrix_identity_max_abs_diff="
              << plan.matrix_identity_max_abs_diffs[channel] << '\n';
    std::cout << prefix << ".mean_relative_error="
              << plan.mean_relative_errors[channel] << '\n';
    std::cout << prefix << ".trace_relative_error="
              << plan.trace_relative_errors[channel] << '\n';
  }
  std::cout << root << ".finite=" << plan.finite << '\n';
  std::cout << root << ".positive_spectrum=" << plan.positive_spectrum
            << '\n';
  std::cout << root << ".full_rank=" << plan.full_rank << '\n';
  std::cout << root << ".moments_preserved=" << plan.moments_preserved
            << '\n';
  std::cout << root << ".participation_target_met="
            << plan.participation_target_met << '\n';
  std::cout << root << ".pass=" << plan.pass << '\n';
}

void mpsr_emit_training(int64_t seed, const GpwdTrainingReceipt &receipt) {
  const std::string root = "mpsr.seed_" + std::to_string(seed) + ".student";
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

void mpsr_emit_candidate(
    const std::string &scope, const RmcSummary &summary,
    const std::array<std::array<RmcEvaluation, 2>, 3> &initial,
    const std::array<std::array<RmcEvaluation, 2>, 3> &final,
    double raw_area) {
  const auto &candidate = summary.candidate[0];
  const auto &gate = summary.gate.neutral;
  const std::string root = "mpsr." + scope;
  std::cout << root << ".raw_control.aulc=" << raw_area << '\n';
  for (std::size_t seed = 0; seed < 3; ++seed) {
    const std::string item =
        root + ".seed_" + std::to_string(kAttributionSeeds[seed]);
    std::cout << item << ".initial.aulc=" << initial[seed][0].probe.area
              << '\n';
    std::cout << item << ".final.aulc=" << final[seed][0].probe.area << '\n';
    std::cout << item << ".final.order_aulc=" << final[seed][0].order.area
              << '\n';
    for (std::size_t channel = 0; channel < kChannels; ++channel) {
      const auto &value = final[seed][0].geometry[channel];
      const std::string geometry_prefix =
          item + ".geometry.channel_" + std::to_string(channel);
      std::cout << geometry_prefix << ".effective="
                << value.effective_rank_ratio << '\n';
      std::cout << geometry_prefix << ".participation="
                << value.participation_rank_ratio << '\n';
      std::cout << geometry_prefix << ".top=" << value.top_eigenvalue_share
                << '\n';
      std::cout << geometry_prefix << ".active="
                << value.active_dimension_fraction << '\n';
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

[[nodiscard]] bool mpsr_options_valid(const Options &options) {
  return options.device == "cuda" &&
         (options.steps < 0 || options.steps == kGpwdStudentSteps) &&
         (options.seeds < 0 ||
          options.seeds == static_cast<int64_t>(kAttributionSeeds.size()));
}

int run_mpsr_preflight(const Options &options) {
  if (!mpsr_options_valid(options)) {
    throw std::runtime_error(
        "MPSR preflight requires CUDA, 512 student steps, and 3 seeds");
  }
  rmc_configure_cuda();
  const torch::Device device(torch::kCUDA, 0);
  const auto protocol =
      rmc_read_file(std::filesystem::path(kMpsrProtocolPath));
  const auto protocol_hash = digest::sha256_hex(protocol);
  auto data = rmc_make_data();
  set_paired_rng(kAttributionSeeds.front(), device);
  auto model = mtf::MtfJepaMaeVicreg(
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]));
  const auto parameters_before = snapshot_parameters(model);
  const auto ssl_capture =
      rmc_extract_sparse_embeddings(model, data.ssl, device);
  const auto plan = mpsr_fit(ssl_capture);
  const auto cached_target = mpsr_apply(ssl_capture, plan);
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
  const bool pass = protocol_hash == kMpsrProtocolSha256 && plan.pass &&
                    cached_target.by_channel.sizes() ==
                        torch::IntArrayRef({256, 3, 32}) &&
                    target.sizes() == torch::IntArrayRef({96, 3, 32}) &&
                    !target.requires_grad() &&
                    torch::isfinite(loss).item<bool>() &&
                    std::isfinite(gradient_norm) && gradient_norm > 0.0 &&
                    served.valid_mask.all().item<bool>() &&
                    parameters_unchanged;
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.mpsr.preflight.v1\n";
  std::cout << "mpsr.protocol.sha256=" << protocol_hash << '\n';
  std::cout << "mpsr.protocol.exact="
            << (protocol_hash == kMpsrProtocolSha256) << '\n';
  std::cout << "mpsr.teacher_steps=" << kGpwdTeacherSteps << '\n';
  std::cout << "mpsr.student_steps=" << kGpwdStudentSteps << '\n';
  std::cout << "mpsr.participation_target=" << kMpsrParticipationTarget
            << '\n';
  std::cout << "mpsr.spectral.pass=" << plan.pass << '\n';
  std::cout << "mpsr.target.requires_grad=" << target.requires_grad() << '\n';
  std::cout << "mpsr.loss=" << loss.item<double>() << '\n';
  std::cout << "mpsr.gradient_norm=" << gradient_norm << '\n';
  std::cout << "mpsr.parameters_unchanged=" << parameters_unchanged << '\n';
  std::cout << "optimizer_steps=0\n";
  std::cout << "augmentation_calls=0\n";
  std::cout << "training_labels_used=false\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "mpsr.preflight.pass=" << pass << '\n';
  return pass ? 0 : 3;
}

int run_mpsr(const Options &options) {
  if (!mpsr_options_valid(options)) {
    throw std::runtime_error(
        "MPSR requires CUDA, 512 student steps, and 3 seeds");
  }
  rmc_configure_cuda();
  const torch::Device device(torch::kCUDA, 0);
  const auto protocol =
      rmc_read_file(std::filesystem::path(kMpsrProtocolPath));
  const auto protocol_hash = digest::sha256_hex(protocol);
  if (protocol_hash != kMpsrProtocolSha256) {
    throw std::runtime_error("MPSR protocol hash mismatch");
  }
  auto data = rmc_make_data();
  const auto development_targets = rmc_make_targets(data, false);
  const auto bootstrap_rows = rmc_bootstrap_rows(256);
  if (!rmc_bootstrap_rows_valid(bootstrap_rows, 256)) {
    throw std::runtime_error("MPSR bootstrap table failed");
  }
  const auto raw_development = rssm_probe_curve(
      data.raw_train, data.raw_validation, data.raw_development,
      data.probe_train.target, data.probe_validation.target,
      data.development.target, /*dual=*/true);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.mpsr.v1\n";
  std::cout << "experiment=minimal-participation-spectral-repair\n";
  std::cout << "module_only=true\n";
  std::cout << "device=cuda:0\n";
  std::cout << "readout_policy=structured_cdsb_sparse_v1\n";
  std::cout << "training_labels_used=false\n";
  std::cout << "outer_augmentation=neutral\n";
  std::cout << "augmentation_calls=0\n";
  std::cout << "teacher_steps=" << kGpwdTeacherSteps << '\n';
  std::cout << "student_steps=" << kGpwdStudentSteps << '\n';
  std::cout << "participation_target=" << kMpsrParticipationTarget << '\n';
  std::cout << "model_seeds=17,31,47\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "mpsr.protocol.sha256=" << protocol_hash << '\n';
  std::cout << "mpsr.confirmation.opened=false\n";

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
    const auto plan = mpsr_fit(ssl_capture);
    cached_targets[seed_index] = mpsr_apply(ssl_capture, plan);
    const auto evaluations = mpsr_evaluate_teacher_and_shadow(
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
    std::cout << "mpsr.seed_" << seed
              << ".initialization_reproduced="
              << initialization_reproduced << '\n';
    std::cout << "mpsr.seed_" << seed
              << ".teacher_training_pass=" << teacher_training.pass << '\n';
    std::cout << "mpsr.seed_" << seed
              << ".teacher_final_aulc=" << evaluations.teacher.probe.area
              << '\n';
    mpsr_emit_plan(seed, plan);
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
  std::cout << "mpsr.teacher.mechanics=" << teacher_mechanics << '\n';
  std::cout << "mpsr.teacher.semantic_result_reproduced="
            << teacher_reproduced << '\n';
  const auto shadow_summary = rmc_summarize(
      initial, shadow, raw_development, data.development.target,
      development_targets.shuffled_evaluation,
      development_targets.order_evaluation,
      development_targets.shuffled_order_evaluation, bootstrap_rows,
      teacher_reproduced);
  mpsr_emit_candidate("shadow_development", shadow_summary, initial, shadow,
                      raw_development.area);
  const bool shadow_pass = shadow_summary.gate.neutral.pass;
  std::cout << "mpsr.shadow.development_gate_pass=" << shadow_pass << '\n';
  if (!teacher_reproduced) {
    std::cout << "mpsr.final_classification=teacher_reproduction_failed\n";
    std::cout << "mpsr.student.opened=false\n";
    std::cout << "mpsr.confirmation.opened=false\n";
    std::cout << "execution_status=mpsr_invalid_teacher\n";
    return 3;
  }
  if (!shadow_pass) {
    std::cout << "mpsr.final_classification=shadow_gate_failed\n";
    std::cout << "mpsr.student.opened=false\n";
    std::cout << "mpsr.confirmation.opened=false\n";
    std::cout << "execution_status=mpsr_shadow_complete\n";
    return 0;
  }

  std::cout << "mpsr.student.opened=true\n";
  std::array<std::array<RmcEvaluation, 2>, 3> student{};
  bool student_mechanics = teacher_reproduced;
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    const auto training = gpwd_train_student(
        retained_models[seed_index], data.ssl, cached_targets[seed_index],
        device, seed);
    mpsr_emit_training(seed, training);
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
  mpsr_emit_candidate("student_development", student_summary, initial, student,
                      raw_development.area);
  const bool student_pass = student_summary.gate.neutral.pass;
  std::cout << "mpsr.student.mechanics=" << student_mechanics << '\n';
  std::cout << "mpsr.student.development_gate_pass=" << student_pass << '\n';
  if (!student_mechanics) {
    std::cout << "mpsr.final_classification=invalid_student_mechanics\n";
    std::cout << "mpsr.confirmation.opened=false\n";
    std::cout << "execution_status=mpsr_invalid_student\n";
    return 3;
  }
  if (!student_pass) {
    std::cout << "mpsr.final_classification=student_development_gate_failed\n";
    std::cout << "mpsr.confirmation.opened=false\n";
    std::cout << "execution_status=mpsr_development_complete\n";
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
  mpsr_emit_candidate("student_confirmation", confirmation_summary,
                      confirmation_initial, confirmation_student,
                      raw_confirmation.area);
  const bool confirmation_pass = confirmation_summary.gate.neutral.pass;
  std::cout << "mpsr.confirmation.opened=true\n";
  std::cout << "mpsr.confirmation.gate_pass=" << confirmation_pass << '\n';
  std::cout << "mpsr.final_classification="
            << (confirmation_pass
                    ? "representation_certified_fspa4_minimal_spectral_repair_v1"
                    : "confirmation_failed")
            << '\n';
  std::cout << "execution_status=mpsr_measurements_complete\n";
  return 0;
}

} // namespace

#ifndef CUWACUNU_MPSR_EMBEDDED
int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment ==
        "minimal-participation-spectral-repair-preflight") {
      return run_mpsr_preflight(options);
    }
    if (options.experiment == "minimal-participation-spectral-repair") {
      return run_mpsr(options);
    }
    throw std::runtime_error(
        "--experiment must be minimal-participation-spectral-repair-"
        "preflight or minimal-participation-spectral-repair");
  } catch (const c10::Error &error) {
    std::cerr << "minimal_participation_spectral_repair_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "minimal_participation_spectral_repair_error="
              << error.what() << '\n';
  }
  return 2;
}
#endif
