#include "piaabo/digest/sha256.h"

// Reuse the sealed RMC data, structured-readout evaluation, gate, and reports.
#define CUWACUNU_RMC_EMBEDDED
#include "quality_wikimyei_mtf_jepa_mae_vicreg_representation_module_certification.cpp"
#undef CUWACUNU_RMC_EMBEDDED

namespace {

constexpr std::string_view kFspaProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/FROZEN_SEQUENCE_PROJECTION_ALIGNMENT_PROTOCOL.md";
constexpr std::string_view kFspaProtocolSha256 =
    "e529478b5f34b279e5f79406700ea05138b09f3c24b34a8c627f770770cc749c";
constexpr int64_t kFspaSteps = 128;
constexpr std::string_view kFspaConvergenceProtocolPath =
    "src/tests/bench/wikimyei/representation/encoding/"
    "mtf_jepa_mae_vicreg/"
    "FROZEN_SEQUENCE_PROJECTION_ALIGNMENT_CONVERGENCE_PROTOCOL.md";
constexpr std::string_view kFspaConvergenceProtocolSha256 =
    "277766448ca39049515ba3f97d9bad24fc2f64ac9f37b224e34985effc20210d";
constexpr int64_t kFspaConvergenceSteps = 1024;
constexpr std::array<double, 3> kExpectedInitialAulc{
    0.60310338720930712, 0.58334878194586337, 0.59273312965635050};

struct FspaTrainingReceipt {
  std::vector<double> losses{};
  double first_eight_mean{0.0};
  double last_eight_mean{0.0};
  double minimum_gradient_norm{std::numeric_limits<double>::infinity()};
  double maximum_gradient_norm{0.0};
  double minimum_update_norm{std::numeric_limits<double>::infinity()};
  double maximum_update_norm{0.0};
  double served_parameter_delta{0.0};
  double predictor_parameter_delta{0.0};
  double mae_decoder_parameter_delta{0.0};
  double vicreg_head_parameter_delta{0.0};
  bool target_contract{true};
  bool sparse_contract{true};
  bool finite{true};
  bool loss_decreased{false};
  bool served_updated{false};
  bool inactive_heads_unchanged{false};
  bool pass{false};
};

[[nodiscard]] torch::Tensor fspa_target(
    const torch::Tensor &data, const torch::Tensor &projection) {
  if (data.dim() != 4 || data.size(1) != kChannels ||
      data.size(2) != kHistory || data.size(3) != kFeatures ||
      projection.sizes() !=
          torch::IntArrayRef({kRawChannelWidth, kLatentDim})) {
    throw std::runtime_error("FSPA target input contract failed");
  }
  return data.reshape({data.size(0), kChannels, kRawChannelWidth})
      .matmul(projection)
      .detach()
      .contiguous();
}

[[nodiscard]] FspaTrainingReceipt
fspa_train(mtf::MtfJepaMaeVicreg &model, const Dataset &ssl,
           const torch::Device &device, int64_t seed, int64_t steps) {
  if (steps <= 0) {
    throw std::runtime_error("FSPA training steps must be positive");
  }
  const auto projection = make_raw_equal_width_projection()
                              .to(device, torch::kFloat32)
                              .contiguous();
  const auto initial_parameters = snapshot_parameters(model);
  auto parameters = model->parameters();
  torch::optim::Adam optimizer(parameters, torch::optim::AdamOptions(1.0e-3));
  FspaTrainingReceipt receipt{};
  receipt.losses.reserve(static_cast<std::size_t>(steps));
  model->train();
  for (int64_t step = 0; step < steps; ++step) {
    const auto rows = training_rows(ssl, seed, step);
    const auto indices = torch::tensor(rows, torch::kInt64);
    const auto data = ssl.data.index_select(0, indices).to(device);
    const auto mask = ssl.mask.index_select(0, indices).to(device);
    set_paired_rng(paired_step_seed(seed, step), device);
    optimizer.zero_grad();
    const auto encoded = model->encode(data, mask);
    const auto served = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1,
        model->config());
    const auto target = fspa_target(data, projection);
    receipt.target_contract =
        receipt.target_contract && !target.requires_grad() &&
        target.sizes() == torch::IntArrayRef({kModelRowBatchSize, 3, 32}) &&
        torch::isfinite(target).all().item<bool>();
    receipt.sparse_contract =
        receipt.sparse_contract && served.valid_mask.all().item<bool>() &&
        served.values.sizes() == target.sizes() &&
        torch::isfinite(served.values).all().item<bool>();
    const auto loss = (served.values - target).pow(2).mean();
    const double loss_value = loss.item<double>();
    receipt.finite = receipt.finite && std::isfinite(loss_value);
    receipt.losses.push_back(loss_value);
    loss.backward();
    double gradient_square_sum = 0.0;
    for (const auto &parameter : parameters) {
      if (!parameter.grad().defined()) {
        continue;
      }
      receipt.finite = receipt.finite &&
                       torch::isfinite(parameter.grad()).all().item<bool>();
      gradient_square_sum +=
          parameter.grad().detach().pow(2).sum().item<double>();
    }
    const double gradient_norm = std::sqrt(gradient_square_sum);
    receipt.minimum_gradient_norm =
        std::min(receipt.minimum_gradient_norm, gradient_norm);
    receipt.maximum_gradient_norm =
        std::max(receipt.maximum_gradient_norm, gradient_norm);
    const auto served_before = served_parameter_vector(model);
    optimizer.step();
    const auto served_after = served_parameter_vector(model);
    const double update_norm =
        (served_after - served_before).norm().item<double>();
    receipt.minimum_update_norm =
        std::min(receipt.minimum_update_norm, update_norm);
    receipt.maximum_update_norm =
        std::max(receipt.maximum_update_norm, update_norm);
    receipt.finite = receipt.finite && std::isfinite(gradient_norm) &&
                     std::isfinite(update_norm);
    model->update_target_network();
  }
  receipt.first_eight_mean = mean_loss_window(receipt.losses, true);
  receipt.last_eight_mean = mean_loss_window(receipt.losses, false);
  receipt.loss_decreased = receipt.last_eight_mean < receipt.first_eight_mean;
  receipt.served_parameter_delta = parameter_partition_max_abs_diff(
      model, initial_parameters, ParameterDeltaPartition::served);
  receipt.predictor_parameter_delta = parameter_partition_max_abs_diff(
      model, initial_parameters, ParameterDeltaPartition::predictor);
  receipt.mae_decoder_parameter_delta = parameter_partition_max_abs_diff(
      model, initial_parameters, ParameterDeltaPartition::mae_decoder);
  receipt.vicreg_head_parameter_delta = parameter_partition_max_abs_diff(
      model, initial_parameters, ParameterDeltaPartition::vicreg_head);
  receipt.served_updated = receipt.served_parameter_delta > 0.0 &&
                           receipt.minimum_gradient_norm > 0.0 &&
                           receipt.minimum_update_norm > 0.0;
  receipt.inactive_heads_unchanged =
      receipt.predictor_parameter_delta == 0.0 &&
      receipt.mae_decoder_parameter_delta == 0.0 &&
      receipt.vicreg_head_parameter_delta == 0.0;
  receipt.pass =
                 receipt.losses.size() == static_cast<std::size_t>(steps) &&
                 receipt.target_contract && receipt.sparse_contract &&
                 receipt.finite && receipt.loss_decreased &&
                 receipt.served_updated && receipt.inactive_heads_unchanged;
  return receipt;
}

void fspa_emit_training(int64_t seed, const FspaTrainingReceipt &receipt) {
  const std::string prefix =
      "fspa.seed_" + std::to_string(seed) + ".training";
  std::cout << prefix << ".first_eight_loss_mean="
            << receipt.first_eight_mean << '\n';
  std::cout << prefix << ".last_eight_loss_mean=" << receipt.last_eight_mean
            << '\n';
  std::cout << prefix << ".minimum_gradient_norm="
            << receipt.minimum_gradient_norm << '\n';
  std::cout << prefix << ".maximum_gradient_norm="
            << receipt.maximum_gradient_norm << '\n';
  std::cout << prefix << ".minimum_update_norm="
            << receipt.minimum_update_norm << '\n';
  std::cout << prefix << ".maximum_update_norm="
            << receipt.maximum_update_norm << '\n';
  std::cout << prefix << ".served_parameter_delta="
            << receipt.served_parameter_delta << '\n';
  std::cout << prefix << ".predictor_parameter_delta="
            << receipt.predictor_parameter_delta << '\n';
  std::cout << prefix << ".mae_decoder_parameter_delta="
            << receipt.mae_decoder_parameter_delta << '\n';
  std::cout << prefix << ".vicreg_head_parameter_delta="
            << receipt.vicreg_head_parameter_delta << '\n';
  std::cout << prefix << ".loss_decreased=" << receipt.loss_decreased << '\n';
  std::cout << prefix << ".inactive_heads_unchanged="
            << receipt.inactive_heads_unchanged << '\n';
  std::cout << prefix << ".pass=" << receipt.pass << '\n';
}

[[nodiscard]] bool fspa_options_valid(const Options &options,
                                      int64_t expected_steps) {
  return options.device == "cuda" &&
         (options.steps < 0 || options.steps == expected_steps) &&
         (options.seeds < 0 ||
          options.seeds == static_cast<int64_t>(kAttributionSeeds.size()));
}

int run_fspa_preflight(const Options &options, bool convergence) {
  const int64_t steps = convergence ? kFspaConvergenceSteps : kFspaSteps;
  const auto protocol_path =
      convergence ? kFspaConvergenceProtocolPath : kFspaProtocolPath;
  const auto protocol_expected =
      convergence ? kFspaConvergenceProtocolSha256 : kFspaProtocolSha256;
  if (!fspa_options_valid(options, steps)) {
    throw std::runtime_error(
        "FSPA preflight requires CUDA, frozen update count, and 3 seeds");
  }
  rmc_configure_cuda();
  const torch::Device device(torch::kCUDA, 0);
  const auto protocol =
      rmc_read_file(std::filesystem::path(protocol_path));
  const auto protocol_hash = digest::sha256_hex(protocol);
  auto data = rmc_make_data();
  set_paired_rng(kAttributionSeeds.front(), device);
  auto model = mtf::MtfJepaMaeVicreg(
      attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]));
  const auto rows = training_rows(data.ssl, kAttributionSeeds.front(), 0);
  const auto indices = torch::tensor(rows, torch::kInt64);
  const auto input = data.ssl.data.index_select(0, indices).to(device);
  const auto mask = data.ssl.mask.index_select(0, indices).to(device);
  const auto projection = make_raw_equal_width_projection()
                              .to(device, torch::kFloat32)
                              .contiguous();
  const auto target = fspa_target(input, projection);
  const auto encoded = model->encode(input, mask);
  const auto served = mtf::select_mtf_serving_pool(
      encoded, mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1,
      model->config());
  const auto loss = (served.values - target).pow(2).mean();
  loss.backward();
  double gradient_square_sum = 0.0;
  bool gradients_finite = true;
  for (const auto &parameter : model->parameters()) {
    if (!parameter.grad().defined()) {
      continue;
    }
    gradients_finite = gradients_finite &&
                       torch::isfinite(parameter.grad()).all().item<bool>();
    gradient_square_sum += parameter.grad().pow(2).sum().item<double>();
  }
  const double gradient_norm = std::sqrt(gradient_square_sum);
  const bool pass = protocol_hash == protocol_expected &&
                    served.valid_mask.all().item<bool>() &&
                    target.sizes() == served.values.sizes() &&
                    torch::isfinite(loss).item<bool>() && gradients_finite &&
                    gradient_norm > 0.0;
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg."
            << (convergence ? "fspa_convergence" : "fspa")
            << ".preflight.v1\n";
  std::cout << "fspa.protocol.sha256=" << protocol_hash << '\n';
  std::cout << "fspa.protocol.exact="
            << (protocol_hash == protocol_expected) << '\n';
  std::cout << "fspa.training_steps=" << steps << '\n';
  std::cout << "fspa.target.shape_exact="
            << (target.sizes() == torch::IntArrayRef({96, 3, 32})) << '\n';
  std::cout << "fspa.target.requires_grad=" << target.requires_grad() << '\n';
  std::cout << "fspa.loss=" << loss.item<double>() << '\n';
  std::cout << "fspa.gradient_norm=" << gradient_norm << '\n';
  std::cout << "optimizer_steps=0\n";
  std::cout << "augmentation_calls=0\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "fspa.preflight.pass=" << pass << '\n';
  return pass ? 0 : 3;
}

int run_fspa(const Options &options, bool convergence) {
  const int64_t steps = convergence ? kFspaConvergenceSteps : kFspaSteps;
  const auto protocol_path =
      convergence ? kFspaConvergenceProtocolPath : kFspaProtocolPath;
  const auto protocol_expected =
      convergence ? kFspaConvergenceProtocolSha256 : kFspaProtocolSha256;
  if (!fspa_options_valid(options, steps)) {
    throw std::runtime_error(
        "FSPA requires CUDA, frozen update count, and 3 seeds");
  }
  rmc_configure_cuda();
  const torch::Device device(torch::kCUDA, 0);
  const auto protocol =
      rmc_read_file(std::filesystem::path(protocol_path));
  const auto protocol_hash = digest::sha256_hex(protocol);
  if (protocol_hash != protocol_expected) {
    throw std::runtime_error("FSPA protocol hash mismatch");
  }
  auto data = rmc_make_data();
  const auto development_targets = rmc_make_targets(data, false);
  const auto bootstrap_rows = rmc_bootstrap_rows(256);
  if (!rmc_bootstrap_rows_valid(bootstrap_rows, 256)) {
    throw std::runtime_error("FSPA bootstrap table failed");
  }
  const auto raw_development = rssm_probe_curve(
      data.raw_train, data.raw_validation, data.raw_development,
      data.probe_train.target, data.probe_validation.target,
      data.development.target, /*dual=*/true);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg."
            << (convergence ? "fspa_convergence" : "fspa") << ".v1\n";
  std::cout << "experiment=frozen-sequence-projection-alignment"
            << (convergence ? "-convergence" : "") << '\n';
  std::cout << "module_only=true\n";
  std::cout << "device=cuda:0\n";
  std::cout << "readout_policy=structured_cdsb_sparse_v1\n";
  std::cout << "training_objective=frozen_raw96_direct_mse\n";
  std::cout << "training_labels_used=false\n";
  std::cout << "outer_augmentation=neutral\n";
  std::cout << "augmentation_calls=0\n";
  std::cout << "training_steps=" << steps << '\n';
  std::cout << "model_seeds=17,31,47\n";
  std::cout << "downstream_models_constructed=0\n";
  std::cout << "fspa.protocol.sha256=" << protocol_hash << '\n';
  std::cout << "fspa.confirmation.opened=false\n";

  std::array<std::array<RmcEvaluation, 2>, 3> initial{};
  std::array<std::array<RmcEvaluation, 2>, 3> trained{};
  std::vector<mtf::MtfJepaMaeVicreg> retained_models;
  retained_models.reserve(3);
  bool mechanics = true;
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    set_paired_rng(seed, device);
    auto model = mtf::MtfJepaMaeVicreg(
        attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]));
    auto initial_evaluation = rmc_evaluate(
        model, data.probe_train, data.probe_validation, data.development,
        data.reversed_train, data.reversed_validation,
        data.reversed_development, development_targets, device);
    const bool initialization_reproduced =
        std::abs(initial_evaluation.probe.area -
                 kExpectedInitialAulc[seed_index]) <= 1.0e-9;
    const auto training = fspa_train(model, data.ssl, device, seed, steps);
    fspa_emit_training(seed, training);
    std::cout << "fspa.seed_" << seed
              << ".initialization_reproduced=" << initialization_reproduced
              << '\n';
    auto trained_evaluation = rmc_evaluate(
        model, data.probe_train, data.probe_validation, data.development,
        data.reversed_train, data.reversed_validation,
        data.reversed_development, development_targets, device);
    initial[seed_index][0] = initial_evaluation;
    initial[seed_index][1] = initial_evaluation;
    trained[seed_index][0] = trained_evaluation;
    trained[seed_index][1] = trained_evaluation;
    mechanics = mechanics && initialization_reproduced && training.pass;
    retained_models.push_back(model);
  }
  const auto development = rmc_summarize(
      initial, trained, raw_development, data.development.target,
      development_targets.shuffled_evaluation,
      development_targets.order_evaluation,
      development_targets.shuffled_order_evaluation, bootstrap_rows,
      mechanics);
  rmc_emit_summary("fspa_development", development, initial, trained,
                   raw_development.area);
  const bool development_pass = development.gate.neutral.pass;
  std::cout << "fspa.development.mechanics=" << mechanics << '\n';
  std::cout << "fspa.development.gate_pass=" << development_pass << '\n';
  if (!mechanics) {
    std::cout << "fspa.final_classification=invalid_mechanics\n";
    std::cout << "execution_status=fspa_invalid_mechanics\n";
    return 3;
  }
  if (!development_pass) {
    std::cout << "fspa.final_classification=development_gate_failed\n";
    std::cout << "fspa.confirmation.opened=false\n";
    std::cout << "execution_status=fspa_development_complete\n";
    return 0;
  }

  rmc_open_confirmation(data);
  const auto confirmation_targets = rmc_make_targets(data, true);
  const auto raw_confirmation = rssm_probe_curve(
      data.raw_train, data.raw_validation, data.raw_confirmation,
      data.probe_train.target, data.probe_validation.target,
      data.confirmation.target, /*dual=*/true);
  std::array<std::array<RmcEvaluation, 2>, 3> confirmation_initial{};
  std::array<std::array<RmcEvaluation, 2>, 3> confirmation_trained{};
  for (std::size_t seed_index = 0; seed_index < 3; ++seed_index) {
    const int64_t seed = kAttributionSeeds[seed_index];
    set_paired_rng(seed, device);
    auto initial_model = mtf::MtfJepaMaeVicreg(
        attribution_config(device, kOuterAugmentationArms[kOuterNeutralIndex]));
    const auto initial_evaluation = rmc_evaluate(
        initial_model, data.probe_train, data.probe_validation,
        data.confirmation, data.reversed_train, data.reversed_validation,
        data.reversed_confirmation, confirmation_targets, device);
    auto trained_evaluation = rmc_evaluate(
        retained_models[seed_index], data.probe_train, data.probe_validation,
        data.confirmation, data.reversed_train, data.reversed_validation,
        data.reversed_confirmation, confirmation_targets, device);
    confirmation_initial[seed_index][0] = initial_evaluation;
    confirmation_initial[seed_index][1] = initial_evaluation;
    confirmation_trained[seed_index][0] = trained_evaluation;
    confirmation_trained[seed_index][1] = trained_evaluation;
  }
  const auto confirmation = rmc_summarize(
      confirmation_initial, confirmation_trained, raw_confirmation,
      data.confirmation.target, confirmation_targets.shuffled_evaluation,
      confirmation_targets.order_evaluation,
      confirmation_targets.shuffled_order_evaluation, bootstrap_rows,
      mechanics);
  rmc_emit_summary("fspa_confirmation", confirmation, confirmation_initial,
                   confirmation_trained, raw_confirmation.area);
  const bool confirmation_pass = confirmation.gate.neutral.pass;
  std::cout << "fspa.confirmation.opened=true\n";
  std::cout << "fspa.confirmation.gate_pass=" << confirmation_pass << '\n';
  std::cout << "fspa.final_classification="
            << (confirmation_pass
                    ? (convergence
                           ? "representation_certified_fspa_convergence_v1"
                           : "representation_certified_fspa_v1")
                                  : "confirmation_failed")
            << '\n';
  std::cout << "execution_status=fspa_measurements_complete\n";
  return 0;
}

} // namespace

#ifndef CUWACUNU_FSPA_EMBEDDED
int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment == "frozen-sequence-projection-alignment-preflight") {
      return run_fspa_preflight(options, false);
    }
    if (options.experiment == "frozen-sequence-projection-alignment") {
      return run_fspa(options, false);
    }
    if (options.experiment ==
        "frozen-sequence-projection-alignment-convergence-preflight") {
      return run_fspa_preflight(options, true);
    }
    if (options.experiment ==
        "frozen-sequence-projection-alignment-convergence") {
      return run_fspa(options, true);
    }
    throw std::runtime_error(
        "--experiment must be frozen-sequence-projection-alignment-preflight "
        "or frozen-sequence-projection-alignment, including their "
        "-convergence variants");
  } catch (const c10::Error &error) {
    std::cerr << "frozen_sequence_projection_alignment_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "frozen_sequence_projection_alignment_error=" << error.what()
              << '\n';
  }
  return 2;
}
#endif
