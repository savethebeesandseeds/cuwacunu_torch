// SPDX-License-Identifier: MIT

// Reuse the complete, already-reviewed frozen-representation data adapter and
// metric implementation in the same translation unit. The runner freezes this
// dependency under this exact reference name before compiling.
#define main cuwacunu_frozen_representation_k1_reference_main
#include "mdn_frozen_representation_k1_isolation.reference.cpp"
#undef main

#include <map>

namespace {

constexpr double kRegressionTargetScale = 36.0;
constexpr double kRegressionHuberBeta = 0.5;
constexpr double kLinearRidgeLambda = 1.0e-4;
constexpr double kLinearScaleFloor = 1.0e-8;
constexpr double kMaterialGain = 0.10;
constexpr double kLinearParityTolerance = 0.05;
constexpr int64_t kLinearFeatureCount = kCaptureFeatureCount;
constexpr int64_t kLinearCoefficientCount = kLinearFeatureCount + 1;

struct ObjectiveOptions {
  std::filesystem::path representation_probe;
  std::filesystem::path raw_history_report;
  std::filesystem::path frozen_k1_report;
  std::filesystem::path alpha_prefix;
  std::filesystem::path beta_prefix;
  std::filesystem::path gamma_prefix;
  std::filesystem::path output;
  std::string schema_id{
      "synthetic_mdn_frozen_representation_k1_objective_ab_v1"};
  bool force_cpu{false};
};

ObjectiveOptions parse_objective_options(int argc, char **argv) {
  ObjectiveOptions options{};
  for (int index = 1; index < argc; ++index) {
    const std::string arg = argv[index];
    if (arg == "--representation-probe") {
      options.representation_probe = required_value(argc, argv, index, arg);
    } else if (arg == "--raw-history-report") {
      options.raw_history_report = required_value(argc, argv, index, arg);
    } else if (arg == "--frozen-k1-report") {
      options.frozen_k1_report = required_value(argc, argv, index, arg);
    } else if (arg == "--alpha-prefix") {
      options.alpha_prefix = required_value(argc, argv, index, arg);
    } else if (arg == "--beta-prefix") {
      options.beta_prefix = required_value(argc, argv, index, arg);
    } else if (arg == "--gamma-prefix") {
      options.gamma_prefix = required_value(argc, argv, index, arg);
    } else if (arg == "--output") {
      options.output = required_value(argc, argv, index, arg);
    } else if (arg == "--schema-id") {
      options.schema_id = required_value(argc, argv, index, arg);
    } else if (arg == "--cpu") {
      options.force_cpu = true;
    } else {
      fail("unknown argument: " + arg);
    }
  }
  require(!options.representation_probe.empty(),
          "--representation-probe is required");
  require(!options.raw_history_report.empty(),
          "--raw-history-report is required");
  require(!options.frozen_k1_report.empty(), "--frozen-k1-report is required");
  require(!options.alpha_prefix.empty(), "--alpha-prefix is required");
  require(!options.beta_prefix.empty(), "--beta-prefix is required");
  require(!options.gamma_prefix.empty(), "--gamma-prefix is required");
  require(!options.output.empty(), "--output is required");
  require(options.schema_id ==
              "synthetic_mdn_frozen_representation_k1_objective_ab_v1",
          "unexpected schema id");
  return options;
}

Options reference_options(const ObjectiveOptions &options) {
  Options reference{};
  reference.representation_probe = options.representation_probe;
  reference.raw_history_report = options.raw_history_report;
  reference.alpha_prefix = options.alpha_prefix;
  reference.beta_prefix = options.beta_prefix;
  reference.gamma_prefix = options.gamma_prefix;
  reference.output = options.output;
  reference.force_cpu = options.force_cpu;
  return reference;
}

using ReportValues = std::unordered_map<std::string, std::string>;

ReportValues validate_frozen_k1_reference(const ObjectiveOptions &options) {
  const auto values = read_key_value_report(options.frozen_k1_report);
  require(required_report_value(values, "schema_id") ==
              "synthetic_mdn_frozen_representation_k1_isolation_v1",
          "unexpected frozen K1 reference schema");
  require(required_report_value(values, "status") == "complete",
          "frozen K1 reference is incomplete");
  require(
      required_report_value(values, "boundary.effective_fit") == "[1,554)" &&
          required_report_value(values, "boundary.purge") == "[554,584)" &&
          required_report_value(values, "boundary.validation") == "[584,730)",
      "frozen K1 reference split changed");
  require(
      required_report_value(values, "execution.seed") == "31" &&
          required_report_value(values, "execution.steps") == "3500" &&
          required_report_value(values, "execution.batch_size") == "64" &&
          required_report_value(values, "execution.learning_rate") ==
              "0.001000000000" &&
          required_report_value(values, "execution.grad_clip_norm") ==
              "5.000000000000" &&
          required_report_value(values, "execution.hidden_width") == "128" &&
          required_report_value(values, "execution.residual_depth") == "2" &&
          required_report_value(values, "execution.feature_embedding_dim") ==
              "8" &&
          required_report_value(values, "execution.channel_adapter_rank") ==
              "16" &&
          required_report_value(values, "execution.sigma_floor") ==
              "0.001000000000" &&
          required_report_value(values, "execution.mixture_count") == "1",
      "frozen K1 reference execution contract changed");
  require(required_report_value(
              values, "classification.fit_featurewise_zscore_k1_pass") ==
              "false",
          "frozen K1 reference no longer records the objective failure");
  require(required_report_value(
              values, "arm.fit_featurewise_zscore.k1.canary_pass") == "false",
          "frozen K1 reference canary result changed");
  return values;
}

enum class ObjectiveKind { TrainableSigmaGaussianNll, TargetScaledEdgeHuber };

const char *objective_name(ObjectiveKind kind) {
  switch (kind) {
  case ObjectiveKind::TrainableSigmaGaussianNll:
    return "trainable_sigma_gaussian_nll";
  case ObjectiveKind::TargetScaledEdgeHuber:
    return "target_scaled_edge_huber";
  }
  fail("unknown objective kind");
}

mdn::MdnNllOptions objective_nll_options() {
  mdn::MdnNllOptions options{};
  options.eps = 1.0e-6;
  options.sigma_min = kSigmaFloor;
  options.sigma_max = 0.0;
  return options;
}

torch::Tensor target_scaled_edge_huber(const mdn::MdnOut &output,
                                       const mdn::channel_mdn_input_t &input,
                                       int64_t *valid_count = nullptr) {
  const auto expected = mdn::mdn_expectation(output);
  require(expected.dim() == 4 && expected.size(1) == kNodeCount &&
              expected.size(2) == kChannelCount &&
              expected.size(3) == kTargetFeatureCount,
          "unexpected mixture expectation shape");
  const auto combined_mask = mdn::combine_channel_context_and_future_mask(
      input.context_mask, input.future_mask);
  const auto predicted_quote = expected.select(1, 0).unsqueeze(1);
  const auto predicted_base = expected.narrow(1, 1, kEdgeCount);
  const auto realized = input.future.to(expected.options());
  const auto realized_quote = realized.select(1, 0).unsqueeze(1);
  const auto realized_base = realized.narrow(1, 1, kEdgeCount);
  const auto valid_quote = combined_mask.select(1, 0).unsqueeze(1);
  const auto valid_base = combined_mask.narrow(1, 1, kEdgeCount);
  const auto valid = valid_base.logical_and(valid_quote);
  const auto count = valid.sum().template item<int64_t>();
  require(count > 0, "edge Huber objective has no valid targets");
  if (valid_count != nullptr) {
    *valid_count = count;
  }
  const auto predicted_edge = predicted_base - predicted_quote;
  const auto realized_edge = realized_base - realized_quote;
  const auto diff = (predicted_edge - realized_edge).masked_select(valid) *
                    kRegressionTargetScale;
  return mdn::channel_context_mdn_train_detail::smooth_l1_mean(
      diff, kRegressionHuberBeta);
}

torch::Tensor objective_tensor(const mdn::MdnOut &output,
                               const mdn::channel_mdn_input_t &input,
                               ObjectiveKind kind,
                               int64_t *valid_edge_count = nullptr) {
  if (kind == ObjectiveKind::TrainableSigmaGaussianNll) {
    if (valid_edge_count != nullptr) {
      *valid_edge_count = 0;
    }
    return mdn::compute_channel_context_mdn_nll(output, input,
                                                objective_nll_options());
  }
  return target_scaled_edge_huber(output, input, valid_edge_count);
}

using ParameterSnapshot = std::vector<std::pair<std::string, torch::Tensor>>;

ParameterSnapshot clone_parameters(const mdn::ChannelContextMdn &model) {
  ParameterSnapshot snapshot;
  for (const auto &parameter : model->named_parameters(true)) {
    snapshot.emplace_back(parameter.key(), parameter.value().detach().clone());
  }
  require(!snapshot.empty(), "model has no parameters");
  return snapshot;
}

double parameter_max_delta(const mdn::ChannelContextMdn &model,
                           const ParameterSnapshot &snapshot) {
  double maximum = 0.0;
  std::size_t index = 0;
  for (const auto &parameter : model->named_parameters(true)) {
    require(index < snapshot.size() && snapshot[index].first == parameter.key(),
            "parameter identity/order changed");
    maximum =
        std::max(maximum, (parameter.value().detach() - snapshot[index].second)
                              .abs()
                              .max()
                              .to(torch::kCPU)
                              .template item<double>());
    ++index;
  }
  require(index == snapshot.size(), "parameter count changed");
  return maximum;
}

double
inactive_distribution_slice_max_delta(const mdn::ChannelContextMdn &model,
                                      const ParameterSnapshot &snapshot) {
  double maximum = 0.0;
  bool saw_weight = false;
  bool saw_bias = false;
  std::size_t index = 0;
  for (const auto &parameter : model->named_parameters(true)) {
    require(index < snapshot.size() && snapshot[index].first == parameter.key(),
            "parameter identity/order changed while checking inactive slices");
    if (parameter.key() == "head.projection.weight" ||
        parameter.key() == "head.projection.bias") {
      auto difference =
          (parameter.value().detach() - snapshot[index].second).abs();
      require(difference.size(0) == 3,
              "K1 distribution projection width changed");
      maximum = std::max(maximum, difference.slice(0, 0, 1)
                                      .max()
                                      .to(torch::kCPU)
                                      .template item<double>());
      maximum = std::max(maximum, difference.slice(0, 2, 3)
                                      .max()
                                      .to(torch::kCPU)
                                      .template item<double>());
      saw_weight = saw_weight || parameter.key().ends_with("weight");
      saw_bias = saw_bias || parameter.key().ends_with("bias");
    }
    ++index;
  }
  require(saw_weight && saw_bias,
          "distribution projection parameters are missing");
  return maximum;
}

bool direct_head_has_gradient(const mdn::ChannelContextMdn &model) {
  bool saw_direct_parameter = false;
  for (const auto &parameter : model->named_parameters(true)) {
    if (parameter.key().find("direct_edge_head") == std::string::npos) {
      continue;
    }
    saw_direct_parameter = true;
    if (parameter.value().grad().defined()) {
      return true;
    }
  }
  require(saw_direct_parameter, "direct edge head parameters are missing");
  return false;
}

double inactive_distribution_gradient_max(const mdn::ChannelContextMdn &model) {
  double maximum = 0.0;
  bool saw_projection_gradient = false;
  for (const auto &parameter : model->named_parameters(true)) {
    if (parameter.key() != "head.projection.weight" &&
        parameter.key() != "head.projection.bias") {
      continue;
    }
    const auto gradient = parameter.value().grad();
    require(gradient.defined() && gradient.size(0) == 3,
            "distribution projection gradient is missing");
    saw_projection_gradient = true;
    maximum = std::max(maximum, gradient.slice(0, 0, 1)
                                    .abs()
                                    .max()
                                    .to(torch::kCPU)
                                    .template item<double>());
    maximum = std::max(maximum, gradient.slice(0, 2, 3)
                                    .abs()
                                    .max()
                                    .to(torch::kCPU)
                                    .template item<double>());
  }
  require(saw_projection_gradient,
          "distribution projection gradient was not observed");
  return maximum;
}

struct ManualStep {
  double objective_loss{std::numeric_limits<double>::quiet_NaN()};
  double diagnostic_nll{std::numeric_limits<double>::quiet_NaN()};
  double grad_norm{std::numeric_limits<double>::quiet_NaN()};
  int64_t valid_edge_count{0};
  bool direct_head_gradient_absent{false};
  double inactive_distribution_gradient_max{
      std::numeric_limits<double>::quiet_NaN()};
};

class ManualObjectiveTrainer {
public:
  ManualObjectiveTrainer(mdn::ChannelContextMdn model, ObjectiveKind objective)
      : model_(std::move(model)), objective_(objective),
        params_(model_->parameters()),
        optimizer_(params_, torch::optim::AdamOptions(kLearningRate)) {}

  ManualStep train_one_batch(const mdn::channel_mdn_input_t &input) {
    const auto clean =
        mdn::channel_context_mdn_train_detail::sanitize_train_input(input);
    const auto combined_mask = mdn::combine_channel_context_and_future_mask(
        clean.context_mask, clean.future_mask);
    require(combined_mask.sum().template item<int64_t>() > 0,
            "manual objective batch has no valid targets");

    model_->train();
    optimizer_.zero_grad();
    const auto output = model_->forward(clean.context, clean.context_mask);
    require(output.log_pi.size(-1) == 1 && output.mu.size(-1) == 1 &&
                output.sigma.size(-1) == 1,
            "objective A/B requires K=1");
    int64_t valid_edge_count = 0;
    const auto loss =
        objective_tensor(output, clean, objective_, &valid_edge_count);
    const auto nll = mdn::compute_channel_context_mdn_nll(
        output, clean, objective_nll_options());
    require(torch::isfinite(loss).all().template item<bool>() &&
                torch::isfinite(nll).all().template item<bool>(),
            "manual objective produced a non-finite loss");
    loss.backward();

    ManualStep step{};
    step.objective_loss = loss.detach().to(torch::kCPU).template item<double>();
    step.diagnostic_nll = nll.detach().to(torch::kCPU).template item<double>();
    step.grad_norm =
        mdn::channel_context_mdn_train_detail::gradient_norm(params_);
    step.valid_edge_count = valid_edge_count;
    step.direct_head_gradient_absent = !direct_head_has_gradient(model_);
    require(step.direct_head_gradient_absent,
            "direct edge head unexpectedly received a gradient");
    if (objective_ == ObjectiveKind::TargetScaledEdgeHuber) {
      step.inactive_distribution_gradient_max =
          inactive_distribution_gradient_max(model_);
      require(step.inactive_distribution_gradient_max == 0.0,
              "K1 logit/sigma projection received a regression gradient");
      require(valid_edge_count == input.context.size(0) * kEdgeCount,
              "edge Huber valid count changed");
    } else {
      require(valid_edge_count == 0,
              "NLL arm unexpectedly reported edge-objective targets");
    }
    require(std::isfinite(step.grad_norm),
            "manual objective gradient norm is non-finite");
    mdn::channel_context_mdn_train_detail::clip_gradients(
        params_, kGradClipNorm, step.grad_norm);
    require(mdn::channel_context_mdn_train_detail::parameters_are_finite(
                params_, true),
            "manual objective has non-finite parameters or gradients");
    optimizer_.step();
    require(mdn::channel_context_mdn_train_detail::parameters_are_finite(
                params_, false),
            "manual objective produced non-finite parameters");
    return step;
  }

  mdn::ChannelContextMdn &model() { return model_; }

private:
  mdn::ChannelContextMdn model_{nullptr};
  ObjectiveKind objective_;
  std::vector<torch::Tensor> params_;
  torch::optim::Adam optimizer_;
};

struct ObjectiveEvaluation {
  Evaluation forecast{};
  double optimized_loss{std::numeric_limits<double>::quiet_NaN()};
};

ObjectiveEvaluation evaluate_objective(mdn::ChannelContextMdn &model,
                                       const Dataset &dataset,
                                       const std::vector<int64_t> &indices,
                                       const torch::Device &device,
                                       ObjectiveKind objective) {
  ObjectiveEvaluation result{};
  result.forecast = evaluate(model, dataset, indices, device, 1);
  const auto input = make_input(dataset, indices, device);
  model->eval();
  torch::NoGradGuard no_grad;
  const auto output = model->forward(input.context, input.context_mask);
  result.optimized_loss = objective_tensor(output, input, objective)
                              .to(torch::kCPU)
                              .template item<double>();
  require(std::isfinite(result.optimized_loss),
          "objective evaluation is non-finite");
  return result;
}

struct ObjectiveArmResult {
  ObjectiveKind objective{ObjectiveKind::TrainableSigmaGaussianNll};
  ObjectiveEvaluation canary_initial{};
  ObjectiveEvaluation canary_final{};
  ObjectiveEvaluation fit_initial{};
  ObjectiveEvaluation validation_initial{};
  ObjectiveEvaluation fit_final{};
  ObjectiveEvaluation validation_final{};
  double first_step_loss{std::numeric_limits<double>::quiet_NaN()};
  double final_step_loss{std::numeric_limits<double>::quiet_NaN()};
  double first_step_diagnostic_nll{std::numeric_limits<double>::quiet_NaN()};
  double final_step_diagnostic_nll{std::numeric_limits<double>::quiet_NaN()};
  double maximum_preclip_grad_norm{0.0};
  double initialization_parameter_max_delta{
      std::numeric_limits<double>::quiet_NaN()};
  double canary_direct_head_parameter_max_delta{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_head_parameter_max_delta{
      std::numeric_limits<double>::quiet_NaN()};
  double inactive_distribution_parameter_max_delta{
      std::numeric_limits<double>::quiet_NaN()};
  bool direct_head_gradient_absent_every_step{true};
  bool canary_pass{false};
  bool validation_direction_gate_pass{false};
  bool validation_rank_gate_pass{false};
  bool overall_pass{false};
};

ObjectiveArmResult
run_objective_arm(ObjectiveKind objective, const Dataset &dataset,
                  const std::vector<int64_t> &fit_indices,
                  const std::vector<int64_t> &validation_indices,
                  const std::vector<int64_t> &canary_indices,
                  const BatchSchedule &schedule, const torch::Device &device,
                  const ParameterSnapshot &canonical_initial_parameters) {
  ObjectiveArmResult result{};
  result.objective = objective;

  {
    auto canary_model = make_model(1, device);
    require(parameter_max_delta(canary_model, canonical_initial_parameters) ==
                0.0,
            "canary model initialization differs between objective arms");
    const auto direct_before = clone_direct_head_parameters(canary_model);
    result.canary_initial = evaluate_objective(
        canary_model, dataset, canary_indices, device, objective);
    ManualObjectiveTrainer trainer(canary_model, objective);
    const auto input = make_input(dataset, canary_indices, device);
    for (int64_t step_index = 0; step_index < kCanarySteps; ++step_index) {
      const auto step = trainer.train_one_batch(input);
      result.direct_head_gradient_absent_every_step =
          result.direct_head_gradient_absent_every_step &&
          step.direct_head_gradient_absent;
      result.maximum_preclip_grad_norm =
          std::max(result.maximum_preclip_grad_norm, step.grad_norm);
    }
    result.canary_final = evaluate_objective(trainer.model(), dataset,
                                             canary_indices, device, objective);
    result.canary_direct_head_parameter_max_delta =
        direct_head_max_delta(trainer.model(), direct_before);
    require(result.canary_direct_head_parameter_max_delta == 0.0,
            "canary changed direct edge head parameters");
    result.canary_pass =
        result.canary_final.forecast.finite &&
        result.canary_final.optimized_loss <
            result.canary_initial.optimized_loss &&
        result.canary_final.forecast.edge.margin_count > 0 &&
        result.canary_final.forecast.edge.margin_pair_count > 0 &&
        result.canary_final.forecast.edge.margin_direction() >=
            kDirectionGate &&
        result.canary_final.forecast.edge.margin_rank() >= kRankGate;
  }

  auto model = make_model(1, device);
  result.initialization_parameter_max_delta =
      parameter_max_delta(model, canonical_initial_parameters);
  require(result.initialization_parameter_max_delta == 0.0,
          "main model initialization differs between objective arms");
  const auto all_before = clone_parameters(model);
  const auto direct_before = clone_direct_head_parameters(model);
  result.fit_initial =
      evaluate_objective(model, dataset, fit_indices, device, objective);
  result.validation_initial =
      evaluate_objective(model, dataset, validation_indices, device, objective);
  ManualObjectiveTrainer trainer(model, objective);
  for (int64_t step_index = 0; step_index < kTrainingSteps; ++step_index) {
    const auto input = make_input(
        dataset, schedule[static_cast<std::size_t>(step_index)], device);
    const auto step = trainer.train_one_batch(input);
    if (step_index == 0) {
      result.first_step_loss = step.objective_loss;
      result.first_step_diagnostic_nll = step.diagnostic_nll;
    }
    result.final_step_loss = step.objective_loss;
    result.final_step_diagnostic_nll = step.diagnostic_nll;
    result.maximum_preclip_grad_norm =
        std::max(result.maximum_preclip_grad_norm, step.grad_norm);
    result.direct_head_gradient_absent_every_step =
        result.direct_head_gradient_absent_every_step &&
        step.direct_head_gradient_absent;
    if ((step_index + 1) % 500 == 0) {
      std::cerr << "objective=" << objective_name(objective)
                << " step=" << (step_index + 1)
                << " loss=" << std::setprecision(10) << step.objective_loss
                << " diagnostic_nll=" << step.diagnostic_nll << '\n';
    }
  }
  result.direct_head_parameter_max_delta =
      direct_head_max_delta(trainer.model(), direct_before);
  require(result.direct_head_parameter_max_delta == 0.0 &&
              result.direct_head_gradient_absent_every_step,
          "main objective changed or differentiated the direct edge head");
  if (objective == ObjectiveKind::TargetScaledEdgeHuber) {
    result.inactive_distribution_parameter_max_delta =
        inactive_distribution_slice_max_delta(trainer.model(), all_before);
    require(result.inactive_distribution_parameter_max_delta == 0.0,
            "edge Huber changed K1 logit/sigma projection slices");
  }
  result.fit_final = evaluate_objective(trainer.model(), dataset, fit_indices,
                                        device, objective);
  result.validation_final = evaluate_objective(
      trainer.model(), dataset, validation_indices, device, objective);
  result.validation_direction_gate_pass =
      result.validation_final.forecast.edge.margin_count > 0 &&
      result.validation_final.forecast.edge.margin_direction() >=
          kDirectionGate;
  result.validation_rank_gate_pass =
      result.validation_final.forecast.edge.margin_pair_count > 0 &&
      result.validation_final.forecast.edge.margin_rank() >= kRankGate;
  result.overall_pass = result.canary_pass &&
                        result.validation_final.forecast.finite &&
                        result.validation_direction_gate_pass &&
                        result.validation_rank_gate_pass &&
                        result.direct_head_parameter_max_delta == 0.0;
  return result;
}

struct NllParityResult {
  double initial_parameter_max_delta{std::numeric_limits<double>::quiet_NaN()};
  double loss_abs_delta{std::numeric_limits<double>::quiet_NaN()};
  double post_step_parameter_max_delta{
      std::numeric_limits<double>::quiet_NaN()};
};

NllParityResult
validate_existing_nll_trainer_parity(const Dataset &dataset,
                                     const std::vector<int64_t> &indices,
                                     const torch::Device &device) {
  auto manual_model = make_model(1, device);
  auto existing_model = make_model(1, device);
  const auto existing_initial = clone_parameters(existing_model);
  NllParityResult parity{};
  parity.initial_parameter_max_delta =
      parameter_max_delta(manual_model, existing_initial);
  require(parity.initial_parameter_max_delta == 0.0,
          "NLL parity models did not initialize identically");
  const auto input = make_input(dataset, indices, device);
  ManualObjectiveTrainer manual(manual_model,
                                ObjectiveKind::TrainableSigmaGaussianNll);
  mdn::channel_context_mdn_train_model_t existing(existing_model, kLearningRate,
                                                  nll_only_options());
  const auto manual_step = manual.train_one_batch(input);
  const auto existing_step = existing.train_one_batch(input);
  require(!existing_step.skipped && existing_step.optimizer_step_applied,
          "existing NLL trainer parity step failed");
  require_nll_only_step(existing_step);
  parity.loss_abs_delta = std::abs(
      manual_step.objective_loss -
      existing_step.loss.detach().to(torch::kCPU).template item<double>());
  parity.post_step_parameter_max_delta =
      parameter_max_delta(manual.model(), clone_parameters(existing.model()));
  require(parity.loss_abs_delta == 0.0 &&
              parity.post_step_parameter_max_delta == 0.0,
          "manual NLL arm differs from the existing trainer after one step");
  return parity;
}

using LinearFeature = std::array<double, kLinearFeatureCount>;
using LinearCoefficients = std::array<double, kLinearCoefficientCount>;

LinearFeature linear_feature(const Dataset &dataset, int64_t anchor,
                             int64_t edge) {
  const auto context = dataset.context.accessor<float, 4>();
  LinearFeature feature{};
  for (int64_t index = 0; index < kRepresentationWidth; ++index) {
    const double base = context[anchor][edge + 1][0][index];
    const double quote = context[anchor][0][0][index];
    feature[static_cast<std::size_t>(index)] = base;
    feature[static_cast<std::size_t>(kRepresentationWidth + index)] = quote;
    feature[static_cast<std::size_t>(2 * kRepresentationWidth + index)] =
        base - quote;
  }
  return feature;
}

double edge_target(const Dataset &dataset, int64_t anchor, int64_t edge) {
  const auto future = dataset.future_node_features.accessor<float, 5>();
  return static_cast<double>(future[anchor][0][0][edge + 1][kRawCloseCoord]) -
         static_cast<double>(future[anchor][0][0][0][kRawCloseCoord]);
}

LinearCoefficients
solve_linear_system(std::array<std::array<double, kLinearCoefficientCount>,
                               kLinearCoefficientCount>
                        matrix,
                    LinearCoefficients rhs) {
  for (int64_t column = 0; column < kLinearCoefficientCount; ++column) {
    int64_t pivot = column;
    for (int64_t row = column + 1; row < kLinearCoefficientCount; ++row) {
      if (std::abs(matrix[static_cast<std::size_t>(row)]
                         [static_cast<std::size_t>(column)]) >
          std::abs(matrix[static_cast<std::size_t>(pivot)]
                         [static_cast<std::size_t>(column)])) {
        pivot = row;
      }
    }
    const double pivot_value = matrix[static_cast<std::size_t>(pivot)]
                                     [static_cast<std::size_t>(column)];
    require(std::isfinite(pivot_value) && std::abs(pivot_value) > 1.0e-12,
            "linear decoder ridge system is singular");
    if (pivot != column) {
      std::swap(matrix[static_cast<std::size_t>(pivot)],
                matrix[static_cast<std::size_t>(column)]);
      std::swap(rhs[static_cast<std::size_t>(pivot)],
                rhs[static_cast<std::size_t>(column)]);
    }
    const double divisor = matrix[static_cast<std::size_t>(column)]
                                 [static_cast<std::size_t>(column)];
    for (int64_t index = column; index < kLinearCoefficientCount; ++index) {
      matrix[static_cast<std::size_t>(column)]
            [static_cast<std::size_t>(index)] /= divisor;
    }
    rhs[static_cast<std::size_t>(column)] /= divisor;
    for (int64_t row = 0; row < kLinearCoefficientCount; ++row) {
      if (row == column) {
        continue;
      }
      const double factor = matrix[static_cast<std::size_t>(row)]
                                  [static_cast<std::size_t>(column)];
      if (factor == 0.0) {
        continue;
      }
      for (int64_t index = column; index < kLinearCoefficientCount; ++index) {
        matrix[static_cast<std::size_t>(row)]
              [static_cast<std::size_t>(index)] -=
            factor * matrix[static_cast<std::size_t>(column)]
                           [static_cast<std::size_t>(index)];
      }
      rhs[static_cast<std::size_t>(row)] -=
          factor * rhs[static_cast<std::size_t>(column)];
    }
  }
  for (const double coefficient : rhs) {
    require(std::isfinite(coefficient),
            "linear decoder coefficient is non-finite");
  }
  return rhs;
}

struct LinearDecoder {
  std::array<LinearFeature, kEdgeCount> mean{};
  std::array<LinearFeature, kEdgeCount> scale{};
  std::array<LinearCoefficients, kEdgeCount> coefficients{};
  int64_t clamped_scale_count{0};
};

LinearDecoder fit_linear_decoder(const Dataset &dataset,
                                 const std::vector<int64_t> &fit_indices) {
  LinearDecoder decoder{};
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    auto &mean = decoder.mean[static_cast<std::size_t>(edge)];
    auto &scale = decoder.scale[static_cast<std::size_t>(edge)];
    for (const int64_t anchor : fit_indices) {
      const auto feature = linear_feature(dataset, anchor, edge);
      for (int64_t index = 0; index < kLinearFeatureCount; ++index) {
        mean[static_cast<std::size_t>(index)] +=
            feature[static_cast<std::size_t>(index)];
      }
    }
    const double count = static_cast<double>(fit_indices.size());
    for (double &value : mean) {
      value /= count;
    }
    for (const int64_t anchor : fit_indices) {
      const auto feature = linear_feature(dataset, anchor, edge);
      for (int64_t index = 0; index < kLinearFeatureCount; ++index) {
        const double difference = feature[static_cast<std::size_t>(index)] -
                                  mean[static_cast<std::size_t>(index)];
        scale[static_cast<std::size_t>(index)] += difference * difference;
      }
    }
    for (double &value : scale) {
      value = std::sqrt(value / count);
      if (value < kLinearScaleFloor) {
        value = kLinearScaleFloor;
        ++decoder.clamped_scale_count;
      }
    }

    std::array<std::array<double, kLinearCoefficientCount>,
               kLinearCoefficientCount>
        normal{};
    LinearCoefficients rhs{};
    for (const int64_t anchor : fit_indices) {
      const auto feature = linear_feature(dataset, anchor, edge);
      LinearCoefficients design{};
      design[0] = 1.0;
      for (int64_t index = 0; index < kLinearFeatureCount; ++index) {
        design[static_cast<std::size_t>(index + 1)] =
            (feature[static_cast<std::size_t>(index)] -
             mean[static_cast<std::size_t>(index)]) /
            scale[static_cast<std::size_t>(index)];
      }
      const double target = edge_target(dataset, anchor, edge);
      for (int64_t row = 0; row < kLinearCoefficientCount; ++row) {
        rhs[static_cast<std::size_t>(row)] +=
            design[static_cast<std::size_t>(row)] * target;
        for (int64_t column = 0; column < kLinearCoefficientCount; ++column) {
          normal[static_cast<std::size_t>(row)]
                [static_cast<std::size_t>(column)] +=
              design[static_cast<std::size_t>(row)] *
              design[static_cast<std::size_t>(column)];
        }
      }
    }
    for (int64_t index = 1; index < kLinearCoefficientCount; ++index) {
      normal[static_cast<std::size_t>(index)]
            [static_cast<std::size_t>(index)] += kLinearRidgeLambda;
    }
    decoder.coefficients[static_cast<std::size_t>(edge)] =
        solve_linear_system(normal, rhs);
  }
  return decoder;
}

double linear_prediction(const Dataset &dataset, const LinearDecoder &decoder,
                         int64_t anchor, int64_t edge) {
  const auto feature = linear_feature(dataset, anchor, edge);
  const auto &mean = decoder.mean[static_cast<std::size_t>(edge)];
  const auto &scale = decoder.scale[static_cast<std::size_t>(edge)];
  const auto &coefficients =
      decoder.coefficients[static_cast<std::size_t>(edge)];
  double prediction = coefficients[0];
  for (int64_t index = 0; index < kLinearFeatureCount; ++index) {
    prediction += coefficients[static_cast<std::size_t>(index + 1)] *
                  (feature[static_cast<std::size_t>(index)] -
                   mean[static_cast<std::size_t>(index)]) /
                  scale[static_cast<std::size_t>(index)];
  }
  require(std::isfinite(prediction), "linear decoder prediction is non-finite");
  return prediction;
}

EdgeMetric evaluate_linear_decoder(const Dataset &dataset,
                                   const LinearDecoder &decoder,
                                   const std::vector<int64_t> &indices) {
  EdgeMetric metric{};
  for (const int64_t anchor : indices) {
    std::array<double, kEdgeCount> prediction{};
    std::array<double, kEdgeCount> target{};
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      prediction[static_cast<std::size_t>(edge)] =
          linear_prediction(dataset, decoder, anchor, edge);
      target[static_cast<std::size_t>(edge)] =
          edge_target(dataset, anchor, edge);
    }
    metric.observe(prediction, target);
  }
  return metric;
}

void emit_objective_evaluation(std::ostream &out, const std::string &prefix,
                               const ObjectiveEvaluation &evaluation) {
  emit_edge_metric(out, prefix, evaluation.forecast.edge);
  out << prefix << ".mean_nll=" << evaluation.forecast.mean_nll << '\n';
  out << prefix << ".sigma_min=" << evaluation.forecast.sigma_min << '\n';
  out << prefix << ".sigma_mean=" << evaluation.forecast.sigma_mean << '\n';
  out << prefix << ".sigma_max=" << evaluation.forecast.sigma_max << '\n';
  out << prefix << ".mixture_entropy=" << evaluation.forecast.mixture_entropy
      << '\n';
  for (std::size_t component = 0;
       component < evaluation.forecast.mixture_usage.size(); ++component) {
    out << prefix << ".mixture_usage.k" << component << '='
        << evaluation.forecast.mixture_usage[component] << '\n';
  }
  out << prefix << ".finite=" << bool_string(evaluation.forecast.finite)
      << '\n';
  out << prefix << ".optimized_loss=" << evaluation.optimized_loss << '\n';
}

void emit_objective_arm(std::ostream &out, const std::string &prefix,
                        const ObjectiveArmResult &arm) {
  out << prefix << ".objective=" << objective_name(arm.objective) << '\n';
  emit_objective_evaluation(out, prefix + ".canary.initial",
                            arm.canary_initial);
  emit_objective_evaluation(out, prefix + ".canary.final", arm.canary_final);
  emit_objective_evaluation(out, prefix + ".initial.fit", arm.fit_initial);
  emit_objective_evaluation(out, prefix + ".initial.validation",
                            arm.validation_initial);
  emit_objective_evaluation(out, prefix + ".final.fit", arm.fit_final);
  emit_objective_evaluation(out, prefix + ".final.validation",
                            arm.validation_final);
  out << prefix << ".first_step_loss=" << arm.first_step_loss << '\n';
  out << prefix << ".final_step_loss=" << arm.final_step_loss << '\n';
  out << prefix
      << ".first_step_diagnostic_nll=" << arm.first_step_diagnostic_nll << '\n';
  out << prefix
      << ".final_step_diagnostic_nll=" << arm.final_step_diagnostic_nll << '\n';
  out << prefix
      << ".maximum_preclip_grad_norm=" << arm.maximum_preclip_grad_norm << '\n';
  out << prefix << ".initialization_parameter_max_abs_delta="
      << arm.initialization_parameter_max_delta << '\n';
  out << prefix << ".canary_direct_head_parameter_max_abs_delta="
      << arm.canary_direct_head_parameter_max_delta << '\n';
  out << prefix << ".direct_head_parameter_max_abs_delta="
      << arm.direct_head_parameter_max_delta << '\n';
  out << prefix << ".inactive_k1_logit_sigma_parameter_max_abs_delta=";
  if (arm.objective == ObjectiveKind::TargetScaledEdgeHuber) {
    out << arm.inactive_distribution_parameter_max_delta;
  } else {
    out << "not_applicable";
  }
  out << '\n';
  out << prefix << ".direct_head_gradient_absent_every_step="
      << bool_string(arm.direct_head_gradient_absent_every_step) << '\n';
  out << prefix << ".canary_pass=" << bool_string(arm.canary_pass) << '\n';
  out << prefix << ".validation_direction_gate_pass="
      << bool_string(arm.validation_direction_gate_pass) << '\n';
  out << prefix << ".validation_rank_gate_pass="
      << bool_string(arm.validation_rank_gate_pass) << '\n';
  out << prefix << ".overall_pass=" << bool_string(arm.overall_pass) << '\n';
}

void write_objective_report(
    const ObjectiveOptions &options, const Dataset &dataset,
    const ReportValues &frozen_k1_values, const std::string &schedule_sha,
    const torch::Device &device, const NllParityResult &nll_parity,
    const EdgeMetric &previous_fit, const EdgeMetric &previous_validation,
    const EdgeMetric &seasonal_fit, const EdgeMetric &seasonal_validation,
    double unconditional_fit_nll, double unconditional_validation_nll,
    const LinearDecoder &linear_decoder, const EdgeMetric &linear_fit,
    const EdgeMetric &linear_validation, const ObjectiveArmResult &nll_arm,
    const ObjectiveArmResult &regression_arm) {
  std::filesystem::create_directories(options.output.parent_path());
  std::ofstream out(options.output, std::ios::trunc);
  require(out.good(), "cannot open objective A/B report");
  out << std::setprecision(12) << std::fixed;
  out << "schema_id=" << options.schema_id << '\n';
  out << "status=complete\n";
  out << "benchmark_id=synthetic_continuous_graph_v1\n";
  out << "diagnostic_authority=diagnostic_only\n";
  out << "benchmark_acceptance_authority=false\n";
  out << "question=does_replacing_trainable_sigma_node_gaussian_nll_with_"
         "target_scaled_edge_huber_rescue_k1_decoding_of_the_exact_same_fit_"
         "zscored_frozen_representation\n\n";

  out << "provenance.policy_path_used=false\n";
  out << "provenance.representation_forward_executed_by_helper=false\n";
  out << "provenance.upstream_frozen_representation_capture_used=true\n";
  out << "provenance.checkpoint_written=false\n";
  out << "provenance.historical_input_used=false\n";
  out << "provenance.holdout_input_used=false\n";
  out << "provenance.reference_helper_compiled_into_same_translation_unit="
         "true\n";
  out << "provenance.primary_replay_byte_identity_enforced_by_runner=true\n";
  out << "provenance.representation_probe_sha256="
      << dataset.representation_probe_sha256 << '\n';
  out << "provenance.raw_history_reference_report_sha256="
      << dataset.raw_history_report_sha256 << '\n';
  out << "provenance.frozen_k1_reference_report_sha256="
      << sha256_file(options.frozen_k1_report) << '\n';
  out << "provenance.frozen_k1_reference_failure_validated=true\n\n";

  out << "data.interval=1d_capture_channel_index_2\n";
  out << "data.context_shape=[B,4,1,32]\n";
  out << "data.target_shape=[B,4,1,1]\n";
  out << "data.target_feature=close_coord_3\n";
  out << "data.target_definition=next_uniform_gauge_node_close_return\n";
  out << "data.edge_metric_definition=base_node_mean_minus_quote_node_mean\n";
  out << "data.prefix_row_count=" << kRequiredPrefixRows << '\n';
  out << "data.maximum_source_row_loaded=" << kMaximumSourceRowLoaded << '\n';
  out << "data.maximum_anchor_loaded=" << kMaximumAnchorLoaded << '\n';
  out << "data.capture_target_matches_raw_causal_1d_return=true\n";
  out << "data.alpha.prefix_sha256=" << dataset.prefix_sha256[0] << '\n';
  out << "data.beta.prefix_sha256=" << dataset.prefix_sha256[1] << '\n';
  out << "data.gamma.prefix_sha256=" << dataset.prefix_sha256[2] << '\n';
  out << "conditioning.mode=fit_only_featurewise_zscore\n";
  out << "conditioning.fit_anchor_range=[1,554)\n";
  out << "conditioning.validation_statistics_used=false\n";
  out << "conditioning.scale_floor=0.000001000000\n";
  out << "conditioning.clamped_feature_count="
      << dataset.zscore_clamped_feature_count << '\n';
  out << "conditioning.fit_mean_max_abs_error="
      << dataset.zscore_fit_mean_max_abs_error << '\n';
  out << "conditioning.fit_std_max_abs_error="
      << dataset.zscore_fit_std_max_abs_error << '\n';
  out << "boundary.effective_fit=[1,554)\n";
  out << "boundary.purge=[554,584)\n";
  out << "boundary.validation=[584,730)\n";
  out << "boundary.historical=[760,1088)\n";
  out << "boundary.historical_opened=false\n";
  out << "boundary.unconsumed_holdout=[1088,1170)\n";
  out << "boundary.unconsumed_holdout_opened=false\n\n";

  out << "execution.device=" << (device.is_cuda() ? "cuda" : "cpu") << '\n';
  out << "execution.dtype=float32\n";
  out << "execution.seed=" << kSeed << '\n';
  out << "execution.steps=" << kTrainingSteps << '\n';
  out << "execution.batch_size=" << kBatchSize << '\n';
  out << "execution.learning_rate=" << kLearningRate << '\n';
  out << "execution.grad_clip_norm=" << kGradClipNorm << '\n';
  out << "execution.hidden_width=" << kHiddenWidth << '\n';
  out << "execution.residual_depth=" << kResidualDepth << '\n';
  out << "execution.feature_embedding_dim=" << kFeatureEmbeddingDim << '\n';
  out << "execution.channel_adapter_rank=" << kChannelAdapterRank << '\n';
  out << "execution.sigma_floor=" << kSigmaFloor << '\n';
  out << "execution.mixture_count=1\n";
  out << "execution.schedule_sha256=" << schedule_sha << '\n';
  out << "execution.schedule_matches_frozen_k1_reference=true\n";
  out << "execution.objective_arms_initialization_identical=true\n";
  out << "execution.objective_arms_batch_schedule_identical=true\n";
  out << "execution.objective_arms_optimizer=adam_default_options\n";
  out << "execution.objective_arms_optimizer_state_initially_empty=true\n";
  out << "execution.direct_head_objective_enabled=false\n";
  out << "execution.direct_head_untouched_asserted=true\n";
  out << "execution.nll_arm_existing_trainer_one_step_loss_abs_delta="
      << nll_parity.loss_abs_delta << '\n';
  out << "execution.nll_arm_existing_trainer_one_step_parameter_max_abs_delta="
      << nll_parity.post_step_parameter_max_delta << '\n';
  out << "execution.nll_arm_existing_trainer_initial_parameter_max_abs_delta="
      << nll_parity.initial_parameter_max_delta << '\n';
  out << "reference.frozen_k1_fit_zscore.validation_margin_direction="
      << required_report_value(
             frozen_k1_values,
             "arm.fit_featurewise_zscore.k1.final.validation.margin_"
             "directional_accuracy")
      << '\n';
  out << "reference.frozen_k1_fit_zscore.validation_margin_rank="
      << required_report_value(
             frozen_k1_values,
             "arm.fit_featurewise_zscore.k1.final.validation.margin_pairwise_"
             "rank_accuracy")
      << '\n';
  out << "reference.frozen_k1_fit_zscore.validation_correlation="
      << required_report_value(
             frozen_k1_values,
             "arm.fit_featurewise_zscore.k1.final.validation.correlation")
      << '\n';
  out << "objective.arm_a=trainable_sigma_gaussian_nll_over_uniform_gauge_"
         "node_targets\n";
  out << "objective.arm_a.nll_eps=0.000001000000\n";
  out << "objective.arm_a.sigma_min=0.001000000000\n";
  out << "objective.arm_a.sigma_max=unbounded\n";
  out << "objective.arm_b=mixture_expectation_edge_return_target_scaled_"
         "smooth_l1\n";
  out << "objective.arm_b.target_scale=" << kRegressionTargetScale << '\n';
  out << "objective.arm_b.huber_beta=" << kRegressionHuberBeta << '\n';
  out << "objective.arm_b.production_alignment=matches_current_direct_readout_"
         "regression_scale_and_beta_but_uses_mdn_mixture_mean\n";
  out << "objective.arm_b.k1_logit_and_sigma_projection_inactive_and_"
         "asserted_unchanged=true\n\n";

  emit_edge_metric(out, "control.previous_return.fit", previous_fit);
  emit_edge_metric(out, "control.previous_return.validation",
                   previous_validation);
  emit_edge_metric(out, "control.authored_seasonal_lag.fit", seasonal_fit);
  emit_edge_metric(out, "control.authored_seasonal_lag.validation",
                   seasonal_validation);
  out << "control.unconditional_per_node_gaussian.fit.mean_nll="
      << unconditional_fit_nll << '\n';
  out << "control.unconditional_per_node_gaussian.validation.mean_nll="
      << unconditional_validation_nll << '\n';
  out << "control.standardized_linear_decoder.feature_layout=base_32_quote_"
         "32_base_minus_quote_32\n";
  out << "control.standardized_linear_decoder.separate_decoder_per_edge=true\n";
  out << "control.standardized_linear_decoder.fit_only_statistics=true\n";
  out << "control.standardized_linear_decoder.ridge_lambda="
      << kLinearRidgeLambda << '\n';
  out << "control.standardized_linear_decoder.scale_floor=" << kLinearScaleFloor
      << '\n';
  out << "control.standardized_linear_decoder.clamped_scale_count="
      << linear_decoder.clamped_scale_count << '\n';
  emit_edge_metric(out, "control.standardized_linear_decoder.fit", linear_fit);
  emit_edge_metric(out, "control.standardized_linear_decoder.validation",
                   linear_validation);
  out << '\n';

  emit_objective_arm(out, "arm.a_trainable_sigma_gaussian_nll", nll_arm);
  out << '\n';
  emit_objective_arm(out, "arm.b_target_scaled_edge_huber", regression_arm);
  out << '\n';

  const double direction_gain =
      regression_arm.validation_final.forecast.edge.margin_direction() -
      nll_arm.validation_final.forecast.edge.margin_direction();
  const double rank_gain =
      regression_arm.validation_final.forecast.edge.margin_rank() -
      nll_arm.validation_final.forecast.edge.margin_rank();
  const bool diagnostic_valid =
      nll_arm.validation_final.forecast.finite &&
      regression_arm.validation_final.forecast.finite &&
      regression_arm.canary_pass &&
      nll_arm.initialization_parameter_max_delta == 0.0 &&
      regression_arm.initialization_parameter_max_delta == 0.0 &&
      nll_arm.direct_head_gradient_absent_every_step &&
      regression_arm.direct_head_gradient_absent_every_step &&
      nll_arm.direct_head_parameter_max_delta == 0.0 &&
      regression_arm.direct_head_parameter_max_delta == 0.0 &&
      regression_arm.inactive_distribution_parameter_max_delta == 0.0;
  const bool material_gain = diagnostic_valid &&
                             direction_gain >= kMaterialGain &&
                             rank_gain >= kMaterialGain;
  const bool reaches_linear_control =
      diagnostic_valid &&
      regression_arm.validation_final.forecast.edge.margin_direction() >=
          linear_validation.margin_direction() - kLinearParityTolerance &&
      regression_arm.validation_final.forecast.edge.margin_rank() >=
          linear_validation.margin_rank() - kLinearParityTolerance;
  out << "comparison.validation_margin_direction_gain_b_minus_a="
      << direction_gain << '\n';
  out << "comparison.validation_margin_rank_gain_b_minus_a=" << rank_gain
      << '\n';
  out << "comparison.material_gain_threshold=" << kMaterialGain << '\n';
  out << "comparison.linear_control_parity_tolerance=" << kLinearParityTolerance
      << '\n';
  out << "classification.validity_gate_requires_edge_huber_canary_and_finite_"
         "validation_and_invariant_checks=true\n";
  out << "classification.validity_gate_pass=" << bool_string(diagnostic_valid)
      << '\n';
  out << "classification.edge_huber_materially_outperforms_nll="
      << bool_string(material_gain) << '\n';
  out << "classification.edge_huber_reaches_linear_control_within_tolerance="
      << bool_string(reaches_linear_control) << '\n';
  out << "classification.nll_canary_pass=" << bool_string(nll_arm.canary_pass)
      << '\n';
  out << "classification.edge_huber_canary_pass="
      << bool_string(regression_arm.canary_pass) << '\n';
  std::string result;
  if (!diagnostic_valid) {
    result = "mechanical_validity_gate_failed_no_objective_rescue_claim";
  } else if (material_gain && reaches_linear_control) {
    result = "observed_objective_bundle_rescues_most_linear_decodable_signal";
  } else if (material_gain) {
    result = "observed_objective_bundle_materially_improves_but_does_not_reach_"
             "linear_control";
  } else if (regression_arm.canary_pass && !nll_arm.canary_pass) {
    result = "observed_edge_huber_mechanical_fit_rescue_without_material_"
             "validation_rescue";
  } else {
    result = "no_material_edge_huber_rescue_observed_under_tested_setup";
  }
  out << "classification.result=" << result << '\n';
  out << "classification.causal_scope=objective_bundle_only_on_frozen_"
         "fit_zscored_features_single_seed\n";
  out << "limitation.arm_b_changes_both_loss_family_and_target_geometry=true\n";
  out << "limitation.cannot_separate_sigma_optimization_from_node_gauge_vs_"
         "edge_geometry_or_huber_robustness=true\n";
  out << "limitation.production_mixture_count_is_3_probe_is_1=true\n";
  out << "limitation.production_targets_all_9_probe_close_only=true\n";
  out << "limitation.production_direct_head_not_trained_or_evaluated=true\n";
  out << "limitation.validation_is_development_slice_not_protected_eval=true\n";
  out << "limitation.single_seed_no_population_causal_claim=true\n";
  out << "summary=" << result
      << "_with_exact_same_frozen_features_initialization_schedule_optimizer_"
         "and_no_policy_checkpoint_or_protected_data_access\n";
  require(out.good(), "failed while writing objective A/B report");

  require(required_report_value(frozen_k1_values,
                                "execution.schedule_sha256") == schedule_sha,
          "frozen K1 schedule changed while writing report");
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_objective_options(argc, argv);
    const auto reference = reference_options(options);
    validate_raw_history_reference(reference);
    const auto frozen_k1_values = validate_frozen_k1_reference(options);
    at::globalContext().setBenchmarkCuDNN(false);
    at::globalContext().setDeterministicCuDNN(true);
    at::globalContext().setDeterministicAlgorithms(true, false);
    at::globalContext().setDeterministicFillUninitializedMemory(true);

    const torch::Device device =
        !options.force_cpu && torch::cuda::is_available()
            ? torch::Device(torch::kCUDA)
            : torch::Device(torch::kCPU);
    const auto dataset = build_dataset(reference);
    require(
        required_report_value(frozen_k1_values,
                              "data.representation_probe_sha256") ==
                dataset.representation_probe_sha256 &&
            required_report_value(frozen_k1_values,
                                  "data.raw_history_reference_report_sha256") ==
                dataset.raw_history_report_sha256 &&
            required_report_value(frozen_k1_values,
                                  "data.alpha.prefix_sha256") ==
                dataset.prefix_sha256[0] &&
            required_report_value(frozen_k1_values,
                                  "data.beta.prefix_sha256") ==
                dataset.prefix_sha256[1] &&
            required_report_value(frozen_k1_values,
                                  "data.gamma.prefix_sha256") ==
                dataset.prefix_sha256[2],
        "frozen K1 report does not describe the provided exact inputs");

    auto zscore_dataset = dataset;
    zscore_dataset.context = dataset.zscore_context;
    const auto fit_indices = range_indices(kEffectiveFitBegin, kFitEnd);
    const auto validation_indices = range_indices(kPurgeEnd, kValidationEnd);
    const auto canary_indices = range_indices(kCanaryBegin, kCanaryEnd);
    const auto schedule = make_schedule(fit_indices);
    const auto schedule_sha = schedule_digest(schedule);
    require(schedule_sha == required_report_value(frozen_k1_values,
                                                  "execution.schedule_sha256"),
            "schedule differs from frozen K1 objective-failure reference");

    const auto contract_input =
        make_input(zscore_dataset, std::vector<int64_t>{1, 2}, device);
    require(
        contract_input.context.sizes() ==
                torch::IntArrayRef({2, kNodeCount, 1, kRepresentationWidth}) &&
            contract_input.future.sizes() ==
                torch::IntArrayRef({2, kNodeCount, 1, 1}) &&
            torch::isfinite(contract_input.context).all().item<bool>(),
        "fit-zscore MDN input contract failed");

    const auto previous_fit =
        evaluate_control(dataset, fit_indices, /*seasonal_lag=*/false);
    const auto previous_validation =
        evaluate_control(dataset, validation_indices, /*seasonal_lag=*/false);
    const auto seasonal_fit =
        evaluate_control(dataset, fit_indices, /*seasonal_lag=*/true);
    const auto seasonal_validation =
        evaluate_control(dataset, validation_indices, /*seasonal_lag=*/true);
    require(seasonal_fit.direction() == 1.0 && seasonal_fit.rank() == 1.0 &&
                seasonal_validation.direction() == 1.0 &&
                seasonal_validation.rank() == 1.0,
            "authored seasonal control failed");
    const auto baseline = fit_unconditional_gaussian(dataset, fit_indices);
    const double unconditional_fit_nll =
        unconditional_gaussian_nll(dataset, fit_indices, baseline);
    const double unconditional_validation_nll =
        unconditional_gaussian_nll(dataset, validation_indices, baseline);

    const auto linear_decoder = fit_linear_decoder(dataset, fit_indices);
    const auto linear_fit =
        evaluate_linear_decoder(dataset, linear_decoder, fit_indices);
    const auto linear_validation =
        evaluate_linear_decoder(dataset, linear_decoder, validation_indices);

    const auto nll_parity = validate_existing_nll_trainer_parity(
        zscore_dataset, std::vector<int64_t>{1, 2, 3, 4}, device);
    auto canonical_model = make_model(1, device);
    const auto canonical_initial_parameters = clone_parameters(canonical_model);

    std::cerr << "running fit-zscore K1 trainable-sigma NLL objective on "
              << (device.is_cuda() ? "cuda" : "cpu") << '\n';
    const auto nll_arm = run_objective_arm(
        ObjectiveKind::TrainableSigmaGaussianNll, zscore_dataset, fit_indices,
        validation_indices, canary_indices, schedule, device,
        canonical_initial_parameters);
    std::cerr << "running fit-zscore K1 target-scaled edge Huber objective on "
              << (device.is_cuda() ? "cuda" : "cpu") << '\n';
    const auto regression_arm =
        run_objective_arm(ObjectiveKind::TargetScaledEdgeHuber, zscore_dataset,
                          fit_indices, validation_indices, canary_indices,
                          schedule, device, canonical_initial_parameters);

    write_objective_report(
        options, dataset, frozen_k1_values, schedule_sha, device, nll_parity,
        previous_fit, previous_validation, seasonal_fit, seasonal_validation,
        unconditional_fit_nll, unconditional_validation_nll, linear_decoder,
        linear_fit, linear_validation, nll_arm, regression_arm);
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "error: " << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "error: " << error.what() << '\n';
  }
  return 1;
}
